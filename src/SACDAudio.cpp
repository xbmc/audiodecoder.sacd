/*
 *  Copyright (C) 2020 Team Kodi <https://kodi.tv>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "SACDAudio.h"

#include "Settings.h"

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
  return true;
}

int CSACDAudioDecoder::ReadPCM(uint8_t* buffer, int size, int& actualsize)
{
  return 0;
}

int64_t CSACDAudioDecoder::Seek(int64_t time)
{
  double seconds = time / 1000.;

  return time;
}

bool CSACDAudioDecoder::ReadTag(const std::string& file, kodi::addon::AudioDecoderInfoTag& tag)
{
  return true;
}

int CSACDAudioDecoder::TrackCount(const std::string& file)
{
  return 0;
}
