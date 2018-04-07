/*
 *      Copyright (C) 2005-2013 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
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

namespace GUIINFO
{

class CGUIInfoHelper
{
public:
  static std::string GetPlaylistLabel(int item, int playlistid = PLAYLIST_NONE);

  static CGUIWindow* GetWindow(int contextWindow);
  static CGUIMediaWindow* GetMediaWindow(int contextWindow);
  static CFileItemPtr GetCurrentListItemFromWindow(int contextWindow);
  static CGUIControl* GetActiveContainer(int containerId, int contextWindow);
  static CGUIListItemPtr GetListItemFromActiveContainer(int containerId, int contextWindow, int offset, unsigned int flag);

private:
  static CGUIWindow* GetWindowWithCondition(int contextWindow, int condition);
  static bool CheckWindowCondition(CGUIWindow *window, int condition);
};

} // namespace GUIINFO
