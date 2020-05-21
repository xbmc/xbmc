/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/AddonManager.h"
#include "addons/IAddon.h"
#include "addons/settings/GUIDialogAddonSettings.h"
#include "guilib/LocalizeStrings.h"

#include <map>

namespace ADDON
{
  class CContextMenuAddon;
}

class IContextMenuItem
{
public:
  virtual ~IContextMenuItem() = default;
  virtual bool IsVisible(const CFileItem& item) const = 0;
  virtual bool Execute(const CFileItemPtr& item) const = 0;
  virtual std::string GetLabel(const CFileItem& item) const = 0;
  virtual bool IsGroup() const { return false; }
};


class CStaticContextMenuAction : public IContextMenuItem
{
public:
  explicit CStaticContextMenuAction(uint32_t label) : m_label(label) {}
  std::string GetLabel(const CFileItem& item) const final
  {
    return g_localizeStrings.Get(m_label);
  }
  bool IsGroup() const final { return false; }
private:
  const uint32_t m_label;
};


class CContextMenuItem : public IContextMenuItem
{
public:
  CContextMenuItem() = default;

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
    const std::string& condition,
    const std::string& addonId, 
    const std::vector<std::string>& args = std::vector<std::string>());
  
  friend class ADDON::CContextMenuAddon;

private:
  std::string m_label;
  std::string m_parent;
  std::string m_groupId;
  std::string m_library;
  std::string m_addonId; // The owner of this menu item
  std::vector<std::string> m_args;

  std::string m_visibilityCondition;
  mutable INFO::InfoPtr m_infoBool;
  mutable bool m_infoBoolRegistered{false};
};
