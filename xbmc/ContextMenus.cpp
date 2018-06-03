/*
 *      Copyright (C) 2016-present Team Kodi
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

#include "storage/MediaManager.h"
#include "ContextMenus.h"


namespace CONTEXTMENU
{

  bool CEjectDisk::IsVisible(const CFileItem& item) const
  {
#ifdef HAS_DVD_DRIVE
    return item.IsRemovable() && (item.IsDVD() || item.IsCDDA());
#else
    return false;
#endif
  }

  bool CEjectDisk::Execute(const CFileItemPtr& item) const
  {
#ifdef HAS_DVD_DRIVE
    g_mediaManager.ToggleTray(g_mediaManager.TranslateDevicePath(item->GetPath())[0]);
#endif
    return true;
  }

  bool CEjectDrive::IsVisible(const CFileItem& item) const
  {
    // Must be HDD
    return item.IsRemovable() && !item.IsDVD() && !item.IsCDDA();
  }

  bool CEjectDrive::Execute(const CFileItemPtr& item) const
  {
    return g_mediaManager.Eject(item->GetPath());
  }

} // namespace CONTEXTMENU
