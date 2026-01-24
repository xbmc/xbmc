/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

class CFileItem;

namespace ADDON
{
  class CContextMenuAddon;
}

namespace INFO
{
class InfoBool;
}

class IContextMenuItem
{
public:
  virtual ~IContextMenuItem() = default;
  virtual bool IsVisible(const CFileItem& item) const = 0;
  virtual bool Execute(const std::shared_ptr<CFileItem>& item) const = 0;
  virtual std::string GetLabel(const CFileItem& item) const = 0;
  virtual bool IsGroup() const { return false; }
  virtual bool HasParent() const { return false; }
};


class CStaticContextMenuAction : public IContextMenuItem
{
public:
  explicit CStaticContextMenuAction(uint32_t label) : m_label(label) {}
  std::string GetLabel(const CFileItem& item) const final;
  bool IsGroup() const final { return false; }
private:
  const uint32_t m_label;
};


class CContextMenuItem : public IContextMenuItem
{
public:
  struct CGroup
  {
    std::string label{};
    std::string parent{};
    std::string groupId{};
    std::string addonId{};
  };

  struct CItem
  {
    std::string label{};
    std::string parent{};
    std::string library{};
    std::string condition{};
    std::string addonId{};
    std::vector<std::string> args{};
  };

  CContextMenuItem() = default;
  CContextMenuItem(CItem item);
  CContextMenuItem(CGroup item);

  std::string GetLabel(const CFileItem& item) const  override { return m_label; }
  bool IsVisible(const CFileItem& item) const override ;
  bool IsParentOf(const CContextMenuItem& menuItem) const;
  bool IsGroup() const override ;
  bool HasParent() const override;
  bool Execute(const std::shared_ptr<CFileItem>& item) const override;
  bool operator==(const CContextMenuItem& other) const;
  std::string ToString() const;

  friend class ADDON::CContextMenuAddon;

private:
  std::string m_label;
  std::string m_parent;
  std::string m_groupId;
  std::string m_library;
  std::string m_addonId; // The owner of this menu item
  std::vector<std::string> m_args;

  std::string m_visibilityCondition;
  mutable std::shared_ptr<INFO::InfoBool> m_infoBool;
  mutable bool m_infoBoolRegistered{false};
};
