/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "ContextMenuItem.h"

#include <memory>

class CFileItem;

namespace CONTEXTMENU
{

struct CAddonInfo : CStaticContextMenuAction
{
  CAddonInfo() : CStaticContextMenuAction(19033) {}
  bool IsVisible(const CFileItem& item) const override;
  bool Execute(const std::shared_ptr<CFileItem>& item) const override;
};

struct CAddonSettings : CStaticContextMenuAction
{
  CAddonSettings() : CStaticContextMenuAction(10004) {}
  bool IsVisible(const CFileItem& item) const override;
  bool Execute(const std::shared_ptr<CFileItem>& item) const override;
};

struct CCheckForUpdates : CStaticContextMenuAction
{
  CCheckForUpdates() : CStaticContextMenuAction(24034) {}
  bool IsVisible(const CFileItem& item) const override;
  bool Execute(const std::shared_ptr<CFileItem>& item) const override;
};

struct CEnableAddon : CStaticContextMenuAction
{
  CEnableAddon() : CStaticContextMenuAction(24022) {}
  bool IsVisible(const CFileItem& item) const override;
  bool Execute(const std::shared_ptr<CFileItem>& item) const override;
};

struct CDisableAddon : CStaticContextMenuAction
{
  CDisableAddon() : CStaticContextMenuAction(24021) {}
  bool IsVisible(const CFileItem& item) const override;
  bool Execute(const std::shared_ptr<CFileItem>& item) const override;
};
}
