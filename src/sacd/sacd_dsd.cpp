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

#include "sacd_dsd.h"

double get_marker_time(const Marker& m, uint32_t samplerate)
{
  return (samplerate > 0) ? (double)m.hours * 60 * 60 + (double)m.minutes * 60 + (double)m.seconds +
                                ((double)m.samples + (double)m.offset) / (double)samplerate
                          : 0.0;
};
