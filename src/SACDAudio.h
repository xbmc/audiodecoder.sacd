/*
 *  Copyright (C) 2020 Team Kodi <https://kodi.tv>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include <kodi/addon-instance/AudioDecoder.h>

class ATTRIBUTE_HIDDEN CSACDAudioDecoder : public kodi::addon::CInstanceAudioDecoder
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
};
