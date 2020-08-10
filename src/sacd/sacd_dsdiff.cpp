/*
 *  Copyright (C) 2020 Team Kodi <https://kodi.tv>
 *  Copyright (c) 2011-2019 Maxim V.Anisiutkin <maxim.anisiutkin@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

/*
 * Code taken SACD Decoder plugin (from foo_input_sacd) by Team Kodi.
 * Original author Maxim V.Anisiutkin.
 */

#include "sacd_dsdiff.h"

sacd_dsdiff_t::sacd_dsdiff_t()
{
  m_mode = ACCESS_MODE_NULL;
  m_track_number = 0;
  m_id3_offset = 0;
}

sacd_dsdiff_t::~sacd_dsdiff_t()
{
  close();
}

uint32_t sacd_dsdiff_t::get_track_count(uint32_t mode)
{
  uint32_t track_mode = mode ? mode : m_mode;
  uint32_t track_count = 0;
  if (track_mode & ACCESS_MODE_TWOCH)
  {
    if (m_channel_count <= 2)
    {
      track_count += m_tracklist.size();
    }
  }
  if (track_mode & ACCESS_MODE_MULCH)
  {
    if (m_channel_count > 2)
    {
      track_count += m_tracklist.size();
    }
  }
  if (track_mode & ACCESS_MODE_SINGLE_TRACK)
  {
    track_count = std::min(track_count, 1u);
  }
  return track_count;
}

uint32_t sacd_dsdiff_t::get_track_number(uint32_t track_index)
{
  return track_index + 1;
}

int sacd_dsdiff_t::get_channels(uint32_t track_number)
{
  return m_channel_count;
}

int sacd_dsdiff_t::get_loudspeaker_config(uint32_t track_number)
{
  return m_loudspeaker_config;
}

int sacd_dsdiff_t::get_samplerate(uint32_t track_number)
{
  return m_samplerate;
}

int sacd_dsdiff_t::get_framerate(uint32_t track_number)
{
  return m_framerate;
}

double sacd_dsdiff_t::get_duration(uint32_t track_number)
{
  if (track_number == TRACK_SELECTED)
  {
    track_number = m_track_number;
  }
  double duration = m_dst_encoded ? (double)m_frame_count / (double)m_framerate
                                  : (double)(m_data_size / m_channel_count) * 8 / m_samplerate;
  uint32_t track_index = track_number - 1;
  if (track_index != -1)
  {
    if (track_index < m_tracklist.size())
    {
      duration = m_tracklist[track_index].stop_time - m_tracklist[track_index].start_time;
    }
  }
  return duration;
}

bool sacd_dsdiff_t::is_dst(uint32_t track_number)
{
  return m_dst_encoded != 0;
}

void sacd_dsdiff_t::set_mode(uint32_t mode)
{
  m_mode = mode;
}

bool sacd_dsdiff_t::open(sacd_media_t* p_file)
{
  m_file = p_file;
  m_dsti_size = 0;
  m_id3_offset = 0;
  uint32_t id3_tag_index = 0;
  uint32_t start_mark_count = 0;
  Chunk ck;
  ID id;
  if (!m_file->seek(0))
  {
    return false;
  }
  if (!(m_file->read(&ck, sizeof(ck)) == sizeof(ck) && ck == "FRM8"))
  {
    return false;
  }
  if (!(m_file->read(&id, sizeof(id)) == sizeof(id) && id == "DSD "))
  {
    return false;
  }
  m_frm8_size = ck.get_size();
  m_id3_offset = sizeof(ck) + ck.get_size();
  uint64_t pos;
  while ((pos = (uint64_t)m_file->get_position()) < m_frm8_size + sizeof(ck))
  {
    if (!(m_file->read(&ck, sizeof(ck)) == sizeof(ck)))
    {
      return false;
    }
    if (ck == "FVER" && ck.get_size() == 4)
    {
      uint32_t version;
      if (!(m_file->read(&version, sizeof(version)) == sizeof(version)))
      {
        return false;
      }
      m_version = hton32(version);
    }
    else if (ck == "PROP")
    {
      if (!(m_file->read(&id, sizeof(id)) == sizeof(id) && id == "SND "))
      {
        return false;
      }
      int64_t id_prop_end = m_file->get_position() - sizeof(id) + ck.get_size();
      while (m_file->get_position() < id_prop_end)
      {
        if (!(m_file->read(&ck, sizeof(ck)) == sizeof(ck)))
        {
          return false;
        }
        if (ck == "FS  " && ck.get_size() == 4)
        {
          uint32_t samplerate;
          if (!(m_file->read(&samplerate, sizeof(samplerate)) == sizeof(samplerate)))
          {
            return false;
          }
          m_samplerate = hton32(samplerate);
        }
        else if (ck == "CHNL")
        {
          uint16_t channel_count;
          if (!(m_file->read(&channel_count, sizeof(channel_count)) == sizeof(channel_count)))
          {
            return false;
          }
          m_channel_count = hton16(channel_count);
          switch (m_channel_count)
          {
            case 1:
              m_loudspeaker_config = 5;
              break;
            case 2:
              m_loudspeaker_config = 0;
              break;
            case 3:
              m_loudspeaker_config = 6;
              break;
            case 4:
              m_loudspeaker_config = 1;
              break;
            case 5:
              m_loudspeaker_config = 3;
              break;
            case 6:
              m_loudspeaker_config = 4;
              break;
            default:
              m_loudspeaker_config = 65535;
              break;
          }
          m_file->skip(ck.get_size() - sizeof(channel_count));
        }
        else if (ck == "CMPR")
        {
          if (!(m_file->read(&id, sizeof(id)) == sizeof(id)))
          {
            return false;
          }
          if (id == "DSD ")
          {
            m_dst_encoded = 0;
          }
          if (id == "DST ")
          {
            m_dst_encoded = 1;
          }
          m_file->skip(ck.get_size() - sizeof(id));
        }
        else if (ck == "LSCO")
        {
          uint16_t loudspeaker_config;
          if (!(m_file->read(&loudspeaker_config, sizeof(loudspeaker_config)) ==
                sizeof(loudspeaker_config)))
          {
            return false;
          }
          m_loudspeaker_config = hton16(loudspeaker_config);
          m_file->skip(ck.get_size() - sizeof(loudspeaker_config));
        }
        else
        {
          m_file->skip(ck.get_size());
        }
        m_file->skip(m_file->get_position() & 1);
      }
    }
    else if (ck == "DSD ")
    {
      m_data_offset = m_file->get_position();
      m_data_size = ck.get_size();
      m_framerate = 75;
      m_frame_size = m_samplerate / 8 * m_channel_count / m_framerate;
      m_frame_count = (uint32_t)((m_data_size + (m_frame_size - 1)) / m_frame_size);
      m_file->skip(ck.get_size());
      track_time_t t;
      t.start_time = 0.0;
      t.stop_time = (double)(m_data_size / m_channel_count) * 8 / m_samplerate;
      m_tracklist.push_back(t);
    }
    else if (ck == "DST ")
    {
      m_data_offset = m_file->get_position();
      m_data_size = ck.get_size();
      if (!(m_file->read(&ck, sizeof(ck)) == sizeof(ck) && ck == "FRTE" && ck.get_size() == 6))
      {
        return false;
      }
      m_data_offset += sizeof(ck) + ck.get_size();
      m_data_size -= sizeof(ck) + ck.get_size();
      uint32_t frame_count;
      if (!(m_file->read(&frame_count, sizeof(frame_count)) == sizeof(frame_count)))
      {
        return false;
      }
      m_frame_count = hton32(frame_count);
      uint16_t framerate;
      if (!(m_file->read(&framerate, sizeof(framerate)) == sizeof(framerate)))
      {
        return false;
      }
      m_framerate = hton16(framerate);
      m_frame_size = m_samplerate / 8 * m_channel_count / m_framerate;
      m_file->seek(m_data_offset + m_data_size);
      track_time_t s;
      s.start_time = 0.0;
      s.stop_time = (double)m_frame_count / m_framerate;
      m_tracklist.push_back(s);
    }
    else if (ck == "DSTI")
    {
      m_dsti_offset = m_file->get_position();
      m_dsti_size = ck.get_size();
      m_file->skip(ck.get_size());
    }
    else if (ck == "DIIN")
    {
      int64_t id_diin_end = m_file->get_position() + ck.get_size();
      while (m_file->get_position() < id_diin_end)
      {
        if (!(m_file->read(&ck, sizeof(ck)) == sizeof(ck)))
        {
          return false;
        }
        if (ck == "MARK" && ck.get_size() >= sizeof(Marker))
        {
          Marker m;
          if (m_file->read(&m, sizeof(Marker)) == sizeof(Marker))
          {
            m.hours = hton16(m.hours);
            m.samples = hton32(m.samples);
            m.offset = hton32(m.offset);
            m.markType = hton16(m.markType);
            m.markChannel = hton16(m.markChannel);
            m.TrackFlags = hton16(m.TrackFlags);
            m.count = hton32(m.count);
            switch (m.markType)
            {
              case TrackStart:
                if (start_mark_count > 0)
                {
                  track_time_t t;
                  m_tracklist.push_back(t);
                }
                start_mark_count++;
                if (m_tracklist.size() > 0)
                {
                  m_tracklist[m_tracklist.size() - 1].start_time = get_marker_time(m, m_samplerate);
                  m_tracklist[m_tracklist.size() - 1].stop_time =
                      m_dst_encoded ? (double)m_frame_count / (double)m_framerate
                                    : (double)(m_data_size / m_channel_count) * 8 / m_samplerate;
                  if (m_tracklist.size() - 1 > 0)
                  {
                    if (m_tracklist[m_tracklist.size() - 2].stop_time >
                        m_tracklist[m_tracklist.size() - 1].start_time)
                    {
                      m_tracklist[m_tracklist.size() - 2].stop_time =
                          m_tracklist[m_tracklist.size() - 1].start_time;
                    }
                  }
                }
                break;
              case TrackStop:
                if (m_tracklist.size() > 0)
                {
                  m_tracklist[m_tracklist.size() - 1].stop_time = get_marker_time(m, m_samplerate);
                }
                break;
            }
          }
          m_file->skip(ck.get_size() - sizeof(Marker));
        }
        else
        {
          m_file->skip(ck.get_size());
        }
        m_file->skip(m_file->get_position() & 1);
      }
    }
    else if (ck == "ID3 ")
    {
      m_id3_offset = std::min(m_id3_offset, (uint64_t)m_file->get_position() - sizeof(ck));
      id3_tags_t id3_tags;
      id3_tags.value.resize((size_t)ck.get_size());
      m_file->read(id3_tags.value.data(), id3_tags.value.size());
      m_id3_tagger.append(id3_tags);
    }
    else
    {
      m_file->skip(ck.get_size());
    }
    m_file->skip(m_file->get_position() & 1);
  }
  m_id3_tagger.set_single_track(m_tracklist.size() == 1);
  m_file->seek(m_data_offset);
  return m_tracklist.size() > 0;
}

bool sacd_dsdiff_t::close()
{
  m_track_number = 0;
  m_tracklist.resize(0);
  m_id3_tagger.remove_all();
  m_dsti_size = 0;
  m_id3_offset = 0;
  return true;
}

bool sacd_dsdiff_t::select_track(uint32_t track_number, uint32_t offset)
{
  m_track_number = track_number;
#if __cplusplus > 201703L
  auto [start_time, stop_time] = get_track_times(track_number);
#else
  auto time = get_track_times(m_track_number);
  double start_time = std::get<0>(time);
  double stop_time = std::get<1>(time);
#endif
  m_track_start = m_data_offset + get_dstf_offset_for_time(start_time);
  m_track_end = m_data_offset + get_dstf_offset_for_time(stop_time);
  m_file->seek(m_track_start);
  return true;
}

bool sacd_dsdiff_t::read_frame(uint8_t* frame_data, size_t* frame_size, frame_type_e* frame_type)
{
  //static uint64_t s_next_frame = 0;
  //if (m_file->get_position() != s_next_frame) {
  //	console::printf("offset: %d - %d", (uint32_t)m_file->get_position(), (uint32_t)s_next_frame);
  //}
  if (m_dst_encoded)
  {
    Chunk ck;
    while (((uint64_t)m_file->get_position() < m_track_end) &&
           (m_file->read(&ck, sizeof(ck)) == sizeof(ck)))
    {
      if (ck == "DSTF" && ck.get_size() <= (uint64_t)*frame_size)
      {
        if (m_file->read(frame_data, (size_t)ck.get_size()) == ck.get_size())
        {
          m_file->skip(ck.get_size() & 1);
          *frame_size = (size_t)ck.get_size();
          *frame_type = frame_type_e::DST;
          //s_next_frame = m_file->get_position();
          return true;
        }
        break;
      }
      else if (ck == "DSTC" && ck.get_size() == 4)
      {
        uint32_t crc;
        if (ck.get_size() == sizeof(crc))
        {
          if (m_file->read(&crc, sizeof(crc)) != sizeof(crc))
          {
            break;
          }
        }
        else
        {
          m_file->skip(ck.get_size());
          m_file->skip(ck.get_size() & 1);
        }
      }
      else
      {
        m_file->seek(1 - (int)sizeof(ck), SEEK_CUR);
      }
    }
  }
  else
  {
    int64_t position = m_file->get_position();
    *frame_size = (size_t)std::min((int64_t)*frame_size,
                                   std::max((int64_t)0, (int64_t)m_track_end - position));
    if (*frame_size > 0)
    {
      *frame_size = m_file->read(frame_data, *frame_size);
      *frame_size -= *frame_size % m_channel_count;
      if (*frame_size > 0)
      {
        *frame_type = frame_type_e::DSD;
        //s_next_frame = m_file->get_position();
        return true;
      }
    }
  }
  *frame_type = frame_type_e::INVALID;
  return false;
}

bool sacd_dsdiff_t::seek(double seconds)
{
  uint64_t track_offset =
      std::min((uint64_t)((m_track_end - m_track_start) * seconds / get_duration(m_track_number)),
               m_track_end - m_track_start);
#if __cplusplus > 201703L
  auto [start_time, stop_time] = get_track_times(m_track_number);
#else
  auto time = get_track_times(m_track_number);
  double start_time = std::get<0>(time);
  double stop_time = std::get<1>(time);
#endif
  uint64_t frame_offset = get_dstf_offset_for_time(start_time + seconds);
  return m_file->seek(m_data_offset + frame_offset);
}

void sacd_dsdiff_t::get_info(uint32_t track_number, kodi::addon::AudioDecoderInfoTag& info)
{
  m_id3_tagger.get_info(track_number, info);
}

void sacd_dsdiff_t::get_albumart(uint32_t albumart_id, std::vector<uint8_t>& albumart_data)
{
  m_id3_tagger.get_albumart(albumart_id, albumart_data);
}

void sacd_dsdiff_t::commit()
{
  m_file->truncate(m_id3_offset);
  m_file->seek(m_id3_offset);
#if __cplusplus > 201703L
  for (auto [tag_data, tag_size] : m_id3_tagger)
  {
#else
  for (auto tag : m_id3_tagger)
  {
    const char* tag_data = std::get<0>(tag);
    size_t tag_size = std::get<1>(tag);
#endif
    if (tag_size > 0)
    {
      write_id3_tag(tag_data, tag_size);
    }
  }
  Chunk ck;
  ck.set_id("FRM8");
  ck.set_size(m_file->get_position() - sizeof(Chunk));
  m_file->seek(0);
  m_file->write(&ck, sizeof(ck));
}

std::tuple<double, double> sacd_dsdiff_t::get_track_times(uint32_t track_number)
{
  double start_time = 0.0;
  double stop_time = m_dst_encoded ? (double)m_frame_count / (double)m_framerate
                                   : (double)(m_data_size / m_channel_count) * 8 / m_samplerate;
  uint32_t track_index = track_number - 1;
  if (track_index < m_tracklist.size())
  {
    if (m_mode & ACCESS_MODE_FULL_PLAYBACK)
    {
      if (track_index > 0)
      {
        start_time = m_tracklist[track_index].start_time;
      }
      if (track_index + 1 < m_tracklist.size())
      {
        stop_time = m_tracklist[track_index + 1].start_time;
      }
    }
    else
    {
      start_time = m_tracklist[track_index].start_time;
      stop_time = m_tracklist[track_index].stop_time;
    }
  }
  return std::make_tuple(start_time, stop_time);
}

uint64_t sacd_dsdiff_t::get_dsti_for_frame(uint32_t frame_nr)
{
  frame_nr = std::min(frame_nr, (uint32_t)(m_dsti_size / sizeof(DSTFrameIndex) - 1));
  m_file->seek(m_dsti_offset + frame_nr * sizeof(DSTFrameIndex));
  DSTFrameIndex frame_index;
  m_file->read(&frame_index, sizeof(DSTFrameIndex));
  return hton64(frame_index.offset) - sizeof(Chunk);
}

uint64_t sacd_dsdiff_t::get_dstf_offset_for_time(double seconds)
{
  uint64_t dstf_offset = 0;
  if (m_dst_encoded)
  {
    dstf_offset = (uint64_t)(seconds * m_framerate / m_frame_count * m_data_size);
    if (m_dsti_size > 0)
    {
      auto frame_nr = (uint32_t)(seconds * m_framerate);
      if (frame_nr < m_frame_count)
      {
        dstf_offset = get_dsti_for_frame(frame_nr) - m_data_offset;
      }
      else
      {
        dstf_offset = m_data_size;
      }
    }
  }
  else
  {
    dstf_offset = (uint64_t)(seconds * m_samplerate / 8) * m_channel_count;
  }
  return dstf_offset;
}

void sacd_dsdiff_t::write_id3_tag(const void* data, uint32_t size)
{
  Chunk ck;
  ck.set_id("ID3 ");
  ck.set_size(size);
  m_file->write(&ck, sizeof(ck));
  m_file->write(data, size);
  if (m_file->get_position() & 1)
  {
    const uint8_t c = 0;
    m_file->write(&c, 1);
  }
}
