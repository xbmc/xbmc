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

#include "addons/IAddon.h"
#include "addons/AddonManager.h"
#include "addons/Repository.h"
#include "addons/RepositoryUpdater.h"
#include "addons/GUIDialogAddonInfo.h"
#include "addons/GUIDialogAddonSettings.h"


namespace ADDON
{
  class CContextMenuAddon;
}

class IContextMenuItem
{
public:
  virtual bool IsVisible(const CFileItem& item) const = 0;
  virtual bool Execute(const CFileItemPtr& item) const = 0;
  virtual std::string GetLabel(const CFileItem& item) const = 0;
  virtual bool IsGroup() const { return false; }
};


class CStaticContextMenuAction : public IContextMenuItem
{
public:
  explicit CStaticContextMenuAction(uint32_t label) : m_label(label) {}
  std::string GetLabel(const CFileItem& item) const override final
  {
    return g_localizeStrings.Get(m_label);
  }
  bool IsGroup() const override final { return false; }
private:
  const uint32_t m_label;
};


class CContextMenuItem : public IContextMenuItem
{
public:
  std::string GetLabel(const CFileItem& item) const  override { return m_label; }
  bool IsVisible(const CFileItem& item) const override ;
  bool IsParentOf(const CContextMenuItem& menuItem) const;
  bool IsGroup() const override ;
  bool Execute(const CFileItemPtr& item) const override;
  bool operator==(const CContextMenuItem& other) const;
  std::string ToString() const;

  static CContextMenuItem CreateGroup(
    const std::string& label,
    const std::string& parent,
    const std::string& groupId,
    const std::string& addonId);

  static CContextMenuItem CreateItem(
    const std::string& label,
    const std::string& parent,
    const std::string& library,
    const INFO::InfoPtr& condition,
    const std::string& addonId);

  friend class ADDON::CContextMenuAddon;

private:
  std::string m_label;
  std::string m_parent;
  std::string m_groupId;
  std::string m_library;
  INFO::InfoPtr m_condition;
  std::string m_addonId; // The owner of this menu item
};
