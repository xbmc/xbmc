/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "playlists/PlayListTypes.h"

#include <memory>
#include <string>

class CFileItem;

class CGUIListItem;

class CGUIControl;
class CGUIMediaWindow;
class CGUIWindow;

namespace KODI
{
namespace GUILIB
{
namespace GUIINFO
{

std::string GetPlaylistLabel(int item, PLAYLIST::Id playlistid = PLAYLIST::TYPE_NONE);

CGUIWindow* GetWindow(int contextWindow);
CGUIControl* GetActiveContainer(int containerId, int contextWindow);
CGUIMediaWindow* GetMediaWindow(int contextWindow);
std::shared_ptr<CGUIListItem> GetCurrentListItem(int contextWindow,
                                                 int containerId = 0,
                                                 int itemOffset = 0,
                                                 unsigned int itemFlags = 0);

std::string GetFileInfoLabelValueFromPath(int info, const std::string& filenameAndPath);

} // namespace GUIINFO
} // namespace GUILIB
} // namespace KODI
