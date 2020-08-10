/*
 *  Copyright (C) 2020 Team Kodi <https://kodi.tv>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include "../lib/libdsdpcm/DSDPCMConverter.h"
#include "../lib/libdsdpcm/DSDPCMConverterEngine.h"
#include "../lib/libdstdec/binding/dst_decoder_mt.h"
#include "Settings.h"
#include "sacd/sacd_core.h"

#include <kodi/addon-instance/AudioDecoder.h>

constexpr int UPDATE_STATS_MS = 500;
constexpr int BITRATE_AVGS = 16;
constexpr float PCM_OVERLOAD_THRESHOLD = 1.0f;

class ATTRIBUTE_HIDDEN CSACDAudioDecoder : public kodi::addon::CInstanceAudioDecoder,
                                           public sacd_core_t
{
public:
  CSACDAudioDecoder(KODI_HANDLE instance, const std::string& version);
  virtual ~CSACDAudioDecoder() = default;

  bool Init(const std::string& filename,
            unsigned int filecache,
            int& channels,
            int& samplerate,
            int& bitspersample,
            int64_t& totaltime,
            int& bitrate,
            AudioEngineDataFormat& format,
            std::vector<AudioEngineChannel>& channellist) override;
  int ReadPCM(uint8_t* buffer, int size, int& actualsize) override;
  int64_t Seek(int64_t time) override;
  bool ReadTag(const std::string& file, kodi::addon::AudioDecoderInfoTag& tag) override;
  int TrackCount(const std::string& file) override;

private:
  std::vector<AudioEngineChannel> GetSACDChannelMapFromLoudspeakerConfig(int loudspeaker_config);
  std::vector<AudioEngineChannel> GetSACDChannelMapFromChannels(int channels);
  uint32_t GetSubsongCount(bool forceOtherIfEmpty);
  uint32_t GetSubsong(uint32_t p_index);
  void AdjustLFE(float* pcm_data,
                 size_t pcm_samples,
                 unsigned channels,
                 const std::vector<AudioEngineChannel>& channel_config);
  bool LoadFir(const std::string& path);
  std::string GetTrackName(const std::string& file, int& track);

  // Setting values
  float m_setting_dBVolumeAdjust = 0.0f;
  float m_setting_lfeAdjustCoef = 0.0f;
  int m_setting_outSamplerate = 44100;

  // Processing parts
  std::unique_ptr<dst_decoder_t> m_dstDecoder;
  std::unique_ptr<DSDPCMConverterEngine> m_dsdPCMDecoder;

  int m_dsdSamplerate;
  std::vector<uint8_t> m_dsdBuf;
  size_t m_dsdBufSize;
  std::vector<uint8_t> m_dstBuf;
  size_t m_dstBufSize;
  int m_dstThreads;
  int m_framerate;
  bool m_readFrame;

  std::vector<double> m_firData;
  std::string m_firName;

  // SACD process
  int64_t m_sacdBitrate[BITRATE_AVGS];
  int m_sacdBitrateIdx;
  int64_t m_sacdBitrateSum;

  // PCM output data
  int m_pcmOutChannels;
  std::vector<AudioEngineChannel> m_pcmOutChannelMap;
  int m_pcmOutSamplerate;
  int pcmOutBitsPerSample = 32;
  int m_pcmOutMaxSamples;
  uint64_t m_pcmOutOffset;
  int m_pcmMinSamplerate;
  std::vector<float> m_pcmBuffer;

  // Data for next call if before was not enough space in buffer.
  size_t m_bytesLeft = 0;
  float* m_bytesLeftNextPtr = nullptr;
};
