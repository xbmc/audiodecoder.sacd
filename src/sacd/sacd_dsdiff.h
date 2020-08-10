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

#pragma once

#include "endianess.h"
#include "id3_tagger.h"
#include "sacd_dsd.h"
#include "sacd_reader.h"
#include "scarletbook.h"

class ATTRIBUTE_HIDDEN sacd_dsdiff_t : public sacd_reader_t
{
  sacd_media_t* m_file;
  uint32_t m_mode;
  uint32_t m_version;
  uint32_t m_samplerate;
  uint16_t m_channel_count;
  int m_loudspeaker_config;
  int m_dst_encoded;
  uint64_t m_frm8_size;
  uint64_t m_dsti_offset;
  uint64_t m_dsti_size;
  uint64_t m_data_offset;
  uint64_t m_data_size;
  uint16_t m_framerate;
  uint32_t m_frame_size;
  uint32_t m_frame_count;
  tracklist_t m_tracklist;
  id3_tagger_t m_id3_tagger;
  uint64_t m_id3_offset;
  uint32_t m_track_number;
  uint64_t m_track_start;
  uint64_t m_track_end;

public:
  sacd_dsdiff_t();
  ~sacd_dsdiff_t();
  uint32_t get_track_count(uint32_t mode);
  uint32_t get_track_number(uint32_t track_index);
  int get_channels(uint32_t track_number);
  int get_loudspeaker_config(uint32_t track_number);
  int get_samplerate(uint32_t track_number);
  int get_framerate(uint32_t track_number);
  double get_duration(uint32_t track_number);
  bool is_dst(uint32_t track_number);
  void set_mode(uint32_t mode);
  bool open(sacd_media_t* p_file);
  bool close();
  bool select_track(uint32_t track_number, uint32_t offset);
  bool read_frame(uint8_t* frame_data, size_t* frame_size, frame_type_e* frame_type);
  bool seek(double seconds);
  void get_info(uint32_t subsong, kodi::addon::AudioDecoderInfoTag& info);
  void get_albumart(uint32_t albumart_id, std::vector<uint8_t>& albumart_data);
  void commit();

private:
  std::tuple<double, double> get_track_times(uint32_t track_number);
  uint64_t get_dsti_for_frame(uint32_t frame_nr);
  uint64_t get_dstf_offset_for_time(double seconds);
  void write_id3_tag(const void* data, uint32_t size);
};
