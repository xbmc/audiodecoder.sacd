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

class ATTR_DLL_LOCAL sacd_dsf_t : public sacd_reader_t
{
  sacd_media_t* m_file;
  uint32_t m_mode;
  int m_version;
  int m_samplerate;
  int m_framerate;
  int m_channel_count;
  int m_loudspeaker_config;
  int64_t m_file_size;
  std::vector<uint8_t> m_block_data;
  int m_block_size;
  int m_sample_in_block;
  int m_block_data_end;
  int64_t m_sample_count;
  int64_t m_data_offset;
  int64_t m_data_size;
  int64_t m_data_end_offset;
  int64_t m_read_offset;
  bool m_is_lsb;
  id3_tagger_t m_id3_tagger;
  int64_t m_id3_offset;
  std::vector<uint8_t> m_id3_data;
  uint8_t swap_bits[256];

public:
  sacd_dsf_t();
  ~sacd_dsf_t();
  uint32_t get_track_count(uint32_t mode);
  uint32_t get_track_number(uint32_t track_index);
  int get_channels(uint32_t track_number);
  int get_loudspeaker_config(uint32_t track_number);
  int get_samplerate(uint32_t track_number);
  int get_framerate(uint32_t track_number);
  double get_duration(uint32_t track_number);
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
  int64_t get_size();
  int64_t get_offset();
};
