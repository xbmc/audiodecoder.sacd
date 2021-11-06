/*
 *  Copyright (C) 2020 Team Kodi <https://kodi.tv>
 *  Copyright (c) 2011-2020 Maxim V.Anisiutkin <maxim.anisiutkin@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

/*
 * Code taken SACD Decoder plugin (from foo_input_sacd) by Team Kodi.
 * Original author Maxim V.Anisiutkin.
 */

#pragma once

#include "sacd_disc.h"
#include "sacd_dsdiff.h"
#include "sacd_dsf.h"
#include "sacd_reader.h"

class ATTR_DLL_LOCAL sacd_core_t
{
public:
  static bool g_is_our_content_type(const std::string& type);
  static bool g_is_our_path(const std::string& path, const std::string& ext);

  sacd_core_t();
  bool open(const std::string& path);

  media_type_e media_type;
  uint32_t access_mode;
  std::unique_ptr<sacd_media_t> sacd_media;
  std::unique_ptr<sacd_reader_t> sacd_reader;
};
