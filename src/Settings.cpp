/*
 *  Copyright (C) 2020-2021 Team Kodi <https://kodi.tv>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "Settings.h"

#include <math.h>

bool CSACDSettings::Load()
{
  m_volumeAdjust = kodi::addon::GetSettingFloat("volume-adjust", 0.0f);
  m_lfeAdjust = pow(10.0f, kodi::addon::GetSettingFloat("lfe-adjust", 0.0f) / 20.0f);
  m_samplerate = kodi::addon::GetSettingInt("samplerate", 352800);
  m_dsd2pcmMode = kodi::addon::GetSettingInt("dsd2pcm-mode", 0);
  m_dsd2pcmFirFile = kodi::addon::GetSettingString("firconverter", "");
  m_speakerArea = kodi::addon::GetSettingInt("area", 0);
  m_separateMultichannel = kodi::addon::GetSettingBoolean("separate-multichannel", false);
  m_separateMultichannel = kodi::addon::GetSettingBoolean("area-allow-fallback", true);

  return true;
}

bool CSACDSettings::SetSetting(const std::string& settingName,
                               const kodi::addon::CSettingValue& settingValue)
{
  if (settingName == "volume-adjust")
  {
    if (settingValue.GetFloat() != m_volumeAdjust)
      m_volumeAdjust = settingValue.GetFloat();
  }
  else if (settingName == "lfe-adjust")
  {
    if (settingValue.GetFloat() != m_lfeAdjust)
      m_lfeAdjust = settingValue.GetFloat();
  }
  else if (settingName == "samplerate")
  {
    if (settingValue.GetInt() != m_samplerate)
      m_samplerate = settingValue.GetInt();
  }
  else if (settingName == "dsd2pcm-mode")
  {
    if (settingValue.GetString() != m_dsd2pcmFirFile)
      m_dsd2pcmFirFile = settingValue.GetString();
  }
  else if (settingName == "firconverter")
  {
    if (settingValue.GetInt() != m_dsd2pcmMode)
      m_dsd2pcmMode = settingValue.GetInt();
  }
  else if (settingName == "area")
  {
    if (settingValue.GetInt() != m_speakerArea)
      m_speakerArea = settingValue.GetInt();
  }
  else if (settingName == "separate-multichannel")
  {
    if (settingValue.GetBoolean() != m_separateMultichannel)
      m_separateMultichannel = settingValue.GetBoolean();
  }

  return true;
}

conv_type_e CSACDSettings::GetConverterType() const
{
  auto conv_type = conv_type_e::MULTISTAGE;
  switch (m_dsd2pcmMode)
  {
    case 0:
    case 1:
      conv_type = conv_type_e::MULTISTAGE;
      break;
    case 2:
    case 3:
      conv_type = conv_type_e::DIRECT;
      break;
    case 4:
    case 5:
      conv_type = conv_type_e::USER;
      break;
  }
  return conv_type;
}

bool CSACDSettings::GetConverterFp64() const
{
  auto conv_fp64 = false;
  switch (m_dsd2pcmMode)
  {
    case 1:
    case 3:
    case 5:
      conv_fp64 = true;
      break;
  }
  return conv_fp64;
}
