/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/Addon.h"

#include <memory>
#include <vector>

class CContextMenuItem;

namespace ADDON
{
class CAddonExtensions;
class CAddonInfo;
using AddonInfoPtr = std::shared_ptr<CAddonInfo>;

class CContextMenuAddon : public CAddon
{
public:
  explicit CContextMenuAddon(const AddonInfoPtr& addonInfo);
  ~CContextMenuAddon() override;

  const std::vector<CContextMenuItem>& GetItems() const { return m_items; }

private:
  void ParseMenu(const CAddonExtensions* elem, const std::string& parent, int& anonGroupCount);
  std::vector<CContextMenuItem> m_items;
};
}
