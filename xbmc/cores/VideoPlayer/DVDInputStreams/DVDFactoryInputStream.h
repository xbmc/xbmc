#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include <memory>
#include <string>
#include <vector>

#include "FileItem.h"

class CDVDInputStream;
class IVideoPlayer;

class CDVDFactoryInputStream
{
public:
  static std::shared_ptr<CDVDInputStream> CreateInputStream(IVideoPlayer* pPlayer, const CFileItem &fileitem, bool scanforextaudio = false);
  static std::shared_ptr<CDVDInputStream> CreateInputStream(IVideoPlayer* pPlayer, const CFileItem &fileitem, const std::vector<std::string>& filenames);
};
