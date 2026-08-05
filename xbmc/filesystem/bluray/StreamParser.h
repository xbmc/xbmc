/*
 *  Copyright (C) 2005-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "M2TSParser.h"
#include "MPLSParser.h"

namespace XFILE
{
struct BlurayPlaylistInformation;
struct PlaylistInformation;

class CStreamParser
{
public:
  /*!
   \brief Convert the parsed .mpls of a playlist into the form the directory handlers use.
   \param streamDetails when INCLUDE, the stream information comes from the clip's .clpi refined by
          the M2TS analysis in s. When DEFER, neither has been read, so it comes from the play
          item's stream number table - enough to tell playlists apart and to list their languages.
   */
  static void ConvertBlurayPlaylistInformation(const BlurayPlaylistInformation& b,
                                               PlaylistInformation& p,
                                               const StreamMap& s,
                                               StreamDetails streamDetails);
};
} // namespace XFILE
