/*
 *      Copyright (C) 2005-present Team Kodi
 *      This file is part of Kodi - https://kodi.tv
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

#include <memory>
#include <string>

#include "PlayListPlayer.h"

class CFileItem;
typedef std::shared_ptr<CFileItem> CFileItemPtr;

class CGUIListItem;
typedef std::shared_ptr<CGUIListItem> CGUIListItemPtr;

class CGUIControl;
class CGUIMediaWindow;
class CGUIWindow;

namespace KODI
{
namespace GUILIB
{
namespace GUIINFO
{

std::string GetPlaylistLabel(int item, int playlistid = PLAYLIST_NONE);

CGUIWindow* GetWindow(int contextWindow);
CGUIControl* GetActiveContainer(int containerId, int contextWindow);
CGUIMediaWindow* GetMediaWindow(int contextWindow);
CGUIListItemPtr GetCurrentListItem(int contextWindow, int containerId = 0, int itemOffset = 0, unsigned int itemFlags = 0);

} // namespace GUIINFO
} // namespace GUILIB
} // namespace KODI
