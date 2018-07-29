/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>
#include <string>

#include "PlayListPlayer.h"

class CFileItem;

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

std::string GetFileInfoLabelValueFromPath(int info, const std::string& filenameAndPath);

} // namespace GUIINFO
} // namespace GUILIB
} // namespace KODI
