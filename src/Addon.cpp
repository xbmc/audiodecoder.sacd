/*
 *  Copyright (C) 2020-2021 Team Kodi <https://kodi.tv>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "Addon.h"

#include "SACDAudio.h"
#include "Settings.h"

CMyAddon::CMyAddon()
{
  CSACDSettings::GetInstance().Load();
}

ADDON_STATUS CMyAddon::CreateInstance(int instanceType,
                                      const std::string& instanceID,
                                      KODI_HANDLE instance,
                                      const std::string& version,
                                      KODI_HANDLE& addonInstance)
{
  if (instanceType == ADDON_INSTANCE_AUDIODECODER)
  {
    addonInstance = new CSACDAudioDecoder(instance, version);
    return ADDON_STATUS_OK;
  }

  return ADDON_STATUS_UNKNOWN;
}

ADDON_STATUS CMyAddon::SetSetting(const std::string& settingName,
                                  const kodi::CSettingValue& settingValue)
{
  return CSACDSettings::GetInstance().SetSetting(settingName, settingValue) ? ADDON_STATUS_OK
                                                                            : ADDON_STATUS_UNKNOWN;
}

ADDONCREATOR(CMyAddon)
