/*
 *      Copyright (C) 2016-present Team Kodi
 *      http://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
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
     * The savestate path is the game path with the extension replaced by ".sav".
     */
    static std::string MakePath(const CSavestate& save);

    /*!
     * \brief Calculate a metadata path for the specified savestate
     *
     * The savestate metadata path is the game path with the extension replaced
     * by ".xml".
     */
    static std::string MakeMetadataPath(const std::string &gamePath);
  };
}
}
