/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
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
