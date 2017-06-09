/*
 *      Copyright (C) 2016-2017 Team Kodi
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#pragma once

#include <string>

namespace KODI
{
namespace GAME
{
  class CSavestate;

  class CSavestateUtils
  {
  public:
    /*!
     * \brief Calculate a path for the specified savestate
     *
     * Path to savestate is derived from game client and game CRC. Returns empty
     * if either of these is unknown. Format is
     *
     * Autosave (hex is game CRC):
     *     special://savegames/gameclient.id/feba62c2.sav
     *
     * Save type slot (digit after the underscore is slot 1-9):
     *     special://savegames/gameclient.id/feba62c2_1.sav
     *
     * Save type label (hex after the underscore is CRC of the label):
     *     special://savegames/gameclient.id/feba62c2_8dc22669.sav
     */
    static std::string MakePath(const CSavestate& save);

    /*!
     * \brief Calculate the thumbnail path for the specified savestate
     *
     * This is the savestate path with a different extension
     */
    static std::string MakeThumbPath(const std::string& savePath);
  };
}
}
