#pragma once
/*
 *      Copyright (C) 2015 Team XBMC
 *      http://xbmc.org
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

#include <map>
#include "addons/ContextMenuAddon.h"
#include "addons/IAddon.h"
#include "dialogs/GUIDialogContextMenu.h"

namespace ADDON
{
  class CContextMenuAddon;
}

class CContextMenuItem
{
public:
  std::string GetLabel() const;
  bool IsVisible(const CFileItemPtr& item) const;
  bool IsParentOf(const CContextMenuItem& menuItem) const;
  bool IsGroup() const;
  bool Execute(const CFileItemPtr& item) const;
  bool operator==(const CContextMenuItem& other) const;
  std::string ToString() const;

  static CContextMenuItem CreateGroup(
    const std::string& label,
    const std::string& parent,
    const std::string& groupId);

  static CContextMenuItem CreateItem(
    const std::string& label,
    const std::string& parent,
    const std::string& library,
    const INFO::InfoPtr& condition);

  friend class ADDON::CContextMenuAddon;

private:
  std::string m_label;
  std::string m_parent;
  std::string m_groupId;
  std::string m_library;
  INFO::InfoPtr m_condition;
  ADDON::AddonPtr m_addon;
};
