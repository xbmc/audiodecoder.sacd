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

#include <kodi/AddonBase.h>
#include <stdint.h>
#include <vector>

#pragma pack(1)

class ATTRIBUTE_HIDDEN ID
{
  char ckID[4];

public:
  bool operator==(const char* id)
  {
    return ckID[0] == id[0] && ckID[1] == id[1] && ckID[2] == id[2] && ckID[3] == id[3];
  }
  bool operator!=(const char* id) { return !(*this == id); }
  void set_id(const char* id)
  {
    ckID[0] = id[0];
    ckID[1] = id[1];
    ckID[2] = id[2];
    ckID[3] = id[3];
  }
};

class ATTRIBUTE_HIDDEN Chunk : public ID
{
public:
  uint64_t ckDataSize;
  uint64_t get_size() { return hton64(ckDataSize); }
  void set_size(uint64_t size) { ckDataSize = hton64(size); }
};

class ATTRIBUTE_HIDDEN FormDSDChunk : public Chunk
{
public:
  ID formType;
};

class ATTRIBUTE_HIDDEN DSTFrameIndex
{
public:
  uint64_t offset;
  uint32_t length;
};

enum MarkType
{
  TrackStart = 0,
  TrackStop = 1,
  ProgramStart = 2,
  Index = 4
};

class ATTRIBUTE_HIDDEN Marker
{
public:
  uint16_t hours;
  uint8_t minutes;
  uint8_t seconds;
  uint32_t samples;
  int32_t offset;
  uint16_t markType;
  uint16_t markChannel;
  uint16_t TrackFlags;
  uint32_t count;
};

class ATTRIBUTE_HIDDEN FmtDSFChunk : public Chunk
{
public:
  uint32_t format_version;
  uint32_t format_id;
  uint32_t channel_type;
  uint32_t channel_count;
  uint32_t samplerate;
  uint32_t bits_per_sample;
  uint64_t sample_count;
  uint32_t block_size;
  uint32_t reserved;
};

#pragma pack()

class ATTRIBUTE_HIDDEN track_time_t
{
public:
  double start_time;
  double stop_time;
};

typedef std::vector<track_time_t> tracklist_t;

double get_marker_time(const Marker& m, uint32_t samplerate);
