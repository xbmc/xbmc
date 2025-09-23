/*
 *  Copyright (C) 2005-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "M2TSParser.h"

namespace XFILE
{
struct BlurayPlaylistInformation;
struct PlaylistInformation;

class CStreamParser
{
public:
  static void ConvertBlurayPlaylistInformation(const BlurayPlaylistInformation& b,
                                               PlaylistInformation& p,
                                               const StreamMap& s);
};
} // namespace XFILE
