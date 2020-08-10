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

#include <kodi/addon-instance/AudioDecoder.h>
#include <map>
#include <tuple>
#include <utility>
#include <vector>

class id3_tags_t
{
public:
  std::vector<uint8_t> value;
  size_t id = -1;
};

typedef std::tuple<const char*, size_t> id3_value_t;

class id3_tagger_t
{
  bool single_track;
  std::vector<id3_tags_t> tagstore;

public:
  class iterator
  {
    std::vector<id3_tags_t>& tagstore;
    size_t tagindex;

  public:
    iterator(std::vector<id3_tags_t>& _tagstore, size_t _tagindex = 0)
      : tagstore(_tagstore), tagindex(_tagindex)
    {
    }
    iterator& operator++()
    {
      ++tagindex;
      return *this;
    }
    iterator operator++(int)
    {
      iterator retval = *this;
      ++(*this);
      return retval;
    }
    bool operator==(iterator other) const { return tagindex == other.tagindex; }
    bool operator!=(iterator other) const { return !(*this == other); }
    id3_value_t operator*()
    {
      return std::make_tuple(reinterpret_cast<const char*>(tagstore[tagindex].value.data()),
                             tagstore[tagindex].value.size());
    }
  };
  bool load_info(std::vector<uint8_t>& buffer, kodi::addon::AudioDecoderInfoTag& info);
  id3_tagger_t() : single_track(false) {}
  iterator begin();
  iterator end();
  size_t get_count();
  std::vector<id3_tags_t>& get_info();
  bool is_single_track();
  void set_single_track(bool is_single);
  void append(const id3_tags_t& tags);
  void remove_all();
  bool get_info(size_t track_number, kodi::addon::AudioDecoderInfoTag& info);
  void update_tags();
  bool load_info(size_t track_index, kodi::addon::AudioDecoderInfoTag& info);
  void update_tags(size_t track_index);
  void get_albumart(size_t albumart_id, std::vector<uint8_t>& albumart_data);
};
