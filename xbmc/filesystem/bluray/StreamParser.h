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

   The streams are from the play item's stream number table, ie. what the playlist exposes,
   in stream number order.
   \param streamDetails when INCLUDE, each stream is refined by the M2TS analysis, adding the
          details only the m2ts carries (channel counts, resolutions). When DEFER, neither the
          .clpi nor the m2ts has been read, so the .mpls is what the streams are described from -
          enough to tell playlists apart and to list their languages.
   */
  static void ConvertBlurayPlaylistInformation(const BlurayPlaylistInformation& b,
                                               PlaylistInformation& p,
                                               const StreamMap& s,
                                               StreamDetails streamDetails);
};
} // namespace XFILE
