#pragma once
/*
 *      Copyright (C) 2016 Team Kodi
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

#include "ContextMenuItem.h"
#include "FileItem.h"
#include "GUIDialogAddonInfo.h"

namespace CONTEXTMENU
{

struct CAddonInfo : CStaticContextMenuAction
{
  CAddonInfo() : CStaticContextMenuAction(19033) {}
  bool IsVisible(const CFileItem& item) const override { return item.HasAddonInfo(); }
  bool Execute(const CFileItemPtr& item) const override
  {
    return CGUIDialogAddonInfo::ShowForItem(item);
  }
};

struct CAddonSettings : CStaticContextMenuAction
{
  CAddonSettings() : CStaticContextMenuAction(10004) {}
  bool IsVisible(const CFileItem& item) const override;
  bool Execute(const CFileItemPtr& item) const override;
};

struct CCheckForUpdates : CStaticContextMenuAction
{
  CCheckForUpdates() : CStaticContextMenuAction(24034) {}
  bool IsVisible(const CFileItem& item) const override;
  bool Execute(const CFileItemPtr& item) const override;
};
}
