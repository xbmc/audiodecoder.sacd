/*
 *  Copyright (C) 2020 Team Kodi <https://kodi.tv>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include <kodi/General.h>

class CSACDSettings
{
public:
  static CSACDSettings& GetInstance()
  {
    static CSACDSettings settings;
    return settings;
  }

  bool Load();
  bool SetSetting(const std::string& settingName, const kodi::CSettingValue& settingValue);


private:
  CSACDSettings() = default;
};
