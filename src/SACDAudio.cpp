/*
 *  Copyright (C) 2020 Team Kodi <https://kodi.tv>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "SACDAudio.h"

#include "Settings.h"

#include <regex>

CSACDAudioDecoder::CSACDAudioDecoder(KODI_HANDLE instance, const std::string& version)
  : CInstanceAudioDecoder(instance, version)
{
}

bool CSACDAudioDecoder::Init(const std::string& filename,
                             unsigned int filecache,
                             int& channels,
                             int& samplerate,
                             int& bitspersample,
                             int64_t& totaltime,
                             int& bitrate,
                             AudioEngineDataFormat& format,
                             std::vector<AudioEngineChannel>& channellist)
{
  /*
   * get the track name from path
   */
  int track = 0;
  std::string toLoad = GetTrackName(filename, track);

  /*
   * Handle settings
   */
  CSACDSettings::GetInstance().Load();

  m_setting_dBVolumeAdjust = CSACDSettings::GetInstance().GetVolumeAdjust();
  m_setting_lfeAdjustCoef = CSACDSettings::GetInstance().GetLFEAdjust();
  m_setting_outSamplerate = CSACDSettings::GetInstance().Samplerate();

  /*
   * Start load and init of stream
   */
  if (!sacd_disc_t::g_is_sacd(toLoad) || !open(toLoad))
    return false;

  uint32_t subSong = GetSubsong(track);
  if (!sacd_reader->select_track(subSong))
  {
    return false;
  }

  m_dsdSamplerate = sacd_reader->get_samplerate(subSong);
  m_framerate = sacd_reader->get_framerate(subSong);
  m_pcmOutChannels = sacd_reader->get_channels(subSong);
  m_dstBufSize = m_dsdBufSize = m_dsdSamplerate / 8 / m_framerate * m_pcmOutChannels;
  m_dstThreads = std::thread::hardware_concurrency();
  if (!m_dstThreads)
    m_dstThreads = 2;
  m_dsdBuf.resize(m_dstThreads * m_dsdBufSize);
  m_dstBuf.resize(m_dstThreads * m_dstBufSize);
  int spkConfig = sacd_reader->get_loudspeaker_config(subSong);
  m_pcmOutChannelMap = GetSACDChannelMapFromLoudspeakerConfig(spkConfig);
  if (m_pcmOutChannelMap.empty())
  {
    m_pcmOutChannelMap = GetSACDChannelMapFromChannels(m_pcmOutChannels);
  }
  m_pcmMinSamplerate = 44100;
  while ((m_pcmMinSamplerate / m_framerate) * m_framerate != m_pcmMinSamplerate)
  {
    m_pcmMinSamplerate *= 2;
  }

  m_pcmOutSamplerate = std::max(m_pcmMinSamplerate, m_setting_outSamplerate);
  m_pcmOutMaxSamples = m_pcmOutSamplerate / m_framerate;
  m_pcmBuffer.resize(m_pcmOutChannels * m_pcmOutMaxSamples);
  memset(m_sacdBitrate, 0, sizeof(m_sacdBitrate));
  m_sacdBitrateIdx = 0;
  m_sacdBitrateSum = 0;

  double* fir_data = nullptr;
  int fir_size = 0;
  if (CSACDSettings::GetInstance().GetConverterType() == conv_type_e::DSDPCM_CONV_USER)
  {
    std::string path = CSACDSettings::GetInstance().GetConverterFirFile();
    if (!path.empty() && LoadFir(kodi::GetAddonPath(path)))
    {
      fir_data = m_firData.data();
      fir_size = m_firData.size();
    }
  }

  m_dsdPCMDecoder = std::make_unique<DSDPCMConverterEngine>();
  m_dsdPCMDecoder->set_gain(m_setting_dBVolumeAdjust);
  int rv =
      m_dsdPCMDecoder->init(m_pcmOutChannels, m_framerate, m_dsdSamplerate, m_pcmOutSamplerate,
                            CSACDSettings::GetInstance().GetConverterType(),
                            CSACDSettings::GetInstance().GetConverterFp64(), fir_data, fir_size);
  if (rv < 0)
  {
    if (rv == -2)
    {
      kodi::Log(ADDON_LOG_ERROR, "No installed FIR, continue with the default", "DSD2PCM");
    }
    int rv = m_dsdPCMDecoder->init(m_pcmOutChannels, m_framerate, m_dsdSamplerate,
                                   m_pcmOutSamplerate, conv_type_e::DSDPCM_CONV_DIRECT,
                                   CSACDSettings::GetInstance().GetConverterFp64(), nullptr, 0);
    if (rv < 0)
    {
      return false;
    }
  }

  m_readFrame = true;

  /*
   * Set values for Kodi
   */
  channels = m_pcmOutChannels;
  samplerate = m_pcmOutSamplerate;
  bitspersample = pcmOutBitsPerSample;
  bitrate = (int64_t)(m_dsdSamplerate * m_pcmOutChannels) + 500;
  totaltime = sacd_reader->get_duration(subSong) * 1000;
  format = AUDIOENGINE_FMT_FLOAT;
  channellist = m_pcmOutChannelMap;

  return true;
}

int CSACDAudioDecoder::ReadPCM(uint8_t* buffer, int size, int& actualsize)
{
  /*
   * Check for cases where on call before not enough buffer was available and
   * give now the rest.
   */
  if (m_bytesLeft > 0)
  {
    float* currentPtr = m_bytesLeftNextPtr;

    actualsize = m_bytesLeft;
    if (actualsize > size)
    {
      m_bytesLeft = actualsize - size;
      m_bytesLeftNextPtr = currentPtr + size / sizeof(float);
      actualsize = size;
    }
    else
    {
      m_bytesLeft = 0;
    }

    memcpy(buffer, currentPtr, actualsize);
    return 0;
  }

  /*
   * Perform needed decode processing.
   */
  uint8_t* dsd_data = nullptr;
  size_t dsd_size = 0;
  while (m_readFrame)
  {
    auto slot_nr = m_dstDecoder ? m_dstDecoder->get_slot_nr() : 0;
    dsd_data = m_dsdBuf.data() + m_dsdBufSize * slot_nr;
    dsd_size = 0;
    uint8_t* frame_data = m_dstBuf.data() + m_dstBufSize * slot_nr;
    size_t frame_size = m_dstBufSize;
    frame_type_e frame_type;
    m_readFrame = sacd_reader->read_frame(frame_data, &frame_size, &frame_type);
    if (m_readFrame)
    {
      switch (frame_type)
      {
        case frame_type_e::DSD:
          dsd_data = frame_data;
          dsd_size = frame_size;
          break;
        case frame_type_e::DST:
          if (!m_dstDecoder)
          {
            m_dstDecoder = std::make_unique<dst_decoder_t>(1);
            if (!m_dstDecoder ||
                m_dstDecoder->init(sacd_reader->get_channels(), sacd_reader->get_samplerate(),
                                   sacd_reader->get_framerate()) != 0)
            {
              return false;
            }
          }
          m_dstDecoder->decode(frame_data, frame_size, &dsd_data, &dsd_size);
          break;
        default:
          return 1;
      }
      m_sacdBitrateIdx = (++m_sacdBitrateIdx) % BITRATE_AVGS;
      m_sacdBitrateSum -= m_sacdBitrate[m_sacdBitrateIdx];
      m_sacdBitrate[m_sacdBitrateIdx] = (int64_t)8 * frame_size * m_framerate;
      m_sacdBitrateSum += m_sacdBitrate[m_sacdBitrateIdx];
    }
    if (dsd_size)
    {
      break;
    }
  }
  if (!dsd_size)
  {
    if (m_dstDecoder)
    {
      m_dstDecoder->decode(nullptr, 0, &dsd_data, &dsd_size);
    }
  }

  /*
   * Convert now the processed data to needed PCM format and give Kodi.
   */
  if (dsd_size)
  {
    auto pcm_out_samples =
        m_dsdPCMDecoder->convert(dsd_data, dsd_size, m_pcmBuffer.data()) / m_pcmOutChannels;
    AdjustLFE(m_pcmBuffer.data(), pcm_out_samples, m_pcmOutChannels, m_pcmOutChannelMap);

    float* currentPtr = m_pcmBuffer.data();

    actualsize = pcm_out_samples * m_pcmOutChannels * sizeof(float);
    if (actualsize > size)
    {
      m_bytesLeft = actualsize - size;
      m_bytesLeftNextPtr = currentPtr + size / sizeof(float);
      actualsize = size;
    }

    memcpy(buffer, currentPtr, actualsize);
  }
  else
  {
    actualsize = 0;
    return -1;
  }

  return 0;
}

int64_t CSACDAudioDecoder::Seek(int64_t time)
{
  double seconds = time / 1000.;
  if (!sacd_reader->seek(seconds))
    return -1;

  return time;
}

bool CSACDAudioDecoder::ReadTag(const std::string& file, kodi::addon::AudioDecoderInfoTag& tag)
{
  /*
   * get the track name from path
   */
  int track = 0;
  std::string toLoad = GetTrackName(file, track);

  if (!sacd_disc_t::g_is_sacd(toLoad) || !open(toLoad))
    return false;

  if (toLoad == file)
  {
    kodi::addon::AudioDecoderInfoTag tagInternal;
    uint32_t subSong = GetSubsong(1);
    sacd_reader->get_info(subSong, tagInternal);
    tag.SetAlbum(tagInternal.GetAlbum());
    tag.SetAlbumArtist(tagInternal.GetAlbumArtist());
    tag.SetDisc(tagInternal.GetDisc());
    tag.SetReleaseDate(tagInternal.GetReleaseDate());
    return true;
  }

  uint32_t subSong = GetSubsong(track);
  sacd_reader->get_info(subSong, tag);

  tag.SetDuration(sacd_reader->get_duration(subSong));
  tag.SetChannels(sacd_reader->get_channels(subSong));
  tag.SetBitrate(
      ((int64_t)(sacd_reader->get_samplerate(subSong) * sacd_reader->get_channels(subSong)) + 500));
  tag.SetSamplerate(sacd_reader->get_samplerate());

  return true;
}

int CSACDAudioDecoder::TrackCount(const std::string& file)
{
  int track = 0;
  std::string toLoad = GetTrackName(file, track);
  if (toLoad != file)
    return 0;

  if (!open(file))
    return 0;
  return GetSubsongCount(false);
}

void CSACDAudioDecoder::AdjustLFE(float* pcm_data,
                                  size_t pcm_samples,
                                  unsigned channels,
                                  const std::vector<AudioEngineChannel>& channel_config)
{
  if ((channels >= 4) &&
      std::any_of(channel_config.begin(), channel_config.end(),
                  [](AudioEngineChannel i) { return i == AUDIOENGINE_CH_LFE; }) &&
      (m_setting_lfeAdjustCoef != 1.0f))
  {
    for (size_t sample = 0; sample < pcm_samples; sample++)
    {
      pcm_data[sample * channels + 3] *= m_setting_lfeAdjustCoef;
    }
  }
}

bool CSACDAudioDecoder::LoadFir(const std::string& path)
{
  if (path.empty())
    return false;

  kodi::vfs::CFile file;
  if (!file.OpenFile(path))
    return false;

  m_firData.clear();

  bool fir_name_is_read = false;
  while (1)
  {
    std::string str;
    if (!file.ReadLine(str))
      break;

    if (!str.empty())
    {
      if (str[0] == '#')
      {
        if (str.length() > 1 && !fir_name_is_read)
        {
          m_firName = str.c_str() + 1;
          m_firName = std::regex_replace(m_firName, std::regex("^ +| +$|( ) +"), "$1");

          fir_name_is_read = true;
        }
      }
      else
      {
        m_firData.push_back(atof(str.c_str()));
      }
    }
  }

  return true;
}

std::vector<AudioEngineChannel> CSACDAudioDecoder::GetSACDChannelMapFromLoudspeakerConfig(
    int loudspeaker_config)
{
  std::vector<AudioEngineChannel> sacd_channel_map;
  switch (loudspeaker_config)
  {
    case 0:
      sacd_channel_map = {AUDIOENGINE_CH_FL, AUDIOENGINE_CH_FR};
      break;
    case 1:
      sacd_channel_map = {AUDIOENGINE_CH_FL, AUDIOENGINE_CH_FR, AUDIOENGINE_CH_BL,
                          AUDIOENGINE_CH_BR};
      break;
    case 2:
      sacd_channel_map = {AUDIOENGINE_CH_FL, AUDIOENGINE_CH_FR, AUDIOENGINE_CH_FC,
                          AUDIOENGINE_CH_LFE};
      break;
    case 3:
      sacd_channel_map = {AUDIOENGINE_CH_FL, AUDIOENGINE_CH_FR, AUDIOENGINE_CH_FC,
                          AUDIOENGINE_CH_BL, AUDIOENGINE_CH_BR};
      break;
    case 4:
      sacd_channel_map = {AUDIOENGINE_CH_FL,  AUDIOENGINE_CH_FR, AUDIOENGINE_CH_FC,
                          AUDIOENGINE_CH_LFE, AUDIOENGINE_CH_BL, AUDIOENGINE_CH_BR};
      break;
    case 5:
      sacd_channel_map = {AUDIOENGINE_CH_FC};
      break;
    case 6:
      sacd_channel_map = {AUDIOENGINE_CH_FL, AUDIOENGINE_CH_FR, AUDIOENGINE_CH_FC};
      break;
    default:
      break;
  }
  return sacd_channel_map;
}

std::vector<AudioEngineChannel> CSACDAudioDecoder::GetSACDChannelMapFromChannels(int channels)
{
  std::vector<AudioEngineChannel> sacd_channel_map;
  switch (channels)
  {
    case 1:
      sacd_channel_map = {AUDIOENGINE_CH_FC};
      break;
    case 2:
      sacd_channel_map = {AUDIOENGINE_CH_FL, AUDIOENGINE_CH_FR};
      break;
    case 3:
      sacd_channel_map = {AUDIOENGINE_CH_FL, AUDIOENGINE_CH_FR, AUDIOENGINE_CH_FC};
      break;
    case 4:
      sacd_channel_map = {AUDIOENGINE_CH_FL, AUDIOENGINE_CH_FR, AUDIOENGINE_CH_BL,
                          AUDIOENGINE_CH_BR};
      break;
    case 5:
      sacd_channel_map = {AUDIOENGINE_CH_FL, AUDIOENGINE_CH_FR, AUDIOENGINE_CH_FC,
                          AUDIOENGINE_CH_BL, AUDIOENGINE_CH_BR};
      break;
    case 6:
      sacd_channel_map = {AUDIOENGINE_CH_FL,  AUDIOENGINE_CH_FR, AUDIOENGINE_CH_FC,
                          AUDIOENGINE_CH_LFE, AUDIOENGINE_CH_BL, AUDIOENGINE_CH_BR};
      break;
    default:
      break;
  }
  return sacd_channel_map;
}

uint32_t CSACDAudioDecoder::GetSubsongCount(bool forceOtherIfEmpty)
{
  uint32_t track_count = 0;
  switch (CSACDSettings::GetInstance().GetSpeakerArea())
  {
    case AREA_TWOCH:
      track_count = sacd_reader->get_track_count(AREA_TWOCH);
      if (track_count == 0 && forceOtherIfEmpty)
      {
        sacd_reader->set_mode(access_mode | AREA_MULCH);
        track_count = sacd_reader->get_track_count(AREA_MULCH);
      }
      break;
    case AREA_MULCH:
      track_count = sacd_reader->get_track_count(AREA_MULCH);
      if (track_count == 0 && forceOtherIfEmpty)
      {
        sacd_reader->set_mode(access_mode | AREA_TWOCH);
        track_count = sacd_reader->get_track_count(AREA_TWOCH);
      }
      break;
    default:
      track_count =
          sacd_reader->get_track_count(AREA_TWOCH) + sacd_reader->get_track_count(AREA_MULCH);
      break;
  }

  return track_count;
}

uint32_t CSACDAudioDecoder::GetSubsong(uint32_t p_index)
{
  return sacd_reader->get_track_number(p_index);
}

std::string CSACDAudioDecoder::GetTrackName(const std::string& file, int& track)
{
  /*
   * get the track name from path
   */
  track = 0;
  std::string toLoad(file);
  if (toLoad.find(".sacdstream") != std::string::npos)
  {
    size_t iStart = toLoad.rfind('-') + 1;
    track = atoi(toLoad.substr(iStart, toLoad.size() - iStart - 11).c_str()) - 1;
    //  The directory we are in, is the file
    //  that contains the bitstream to play,
    //  so extract it
    size_t slash = file.rfind('\\');
    if (slash == std::string::npos)
      slash = file.rfind('/');
    toLoad = file.substr(0, slash);
  }

  return toLoad;
}
