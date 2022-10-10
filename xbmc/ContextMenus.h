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

namespace CONTEXTMENU
{

struct CEjectDisk : CStaticContextMenuAction
{
  CEjectDisk() : CStaticContextMenuAction(13391) {} // Eject/Load CD/DVD!
  bool IsVisible(const CFileItem& item) const override;
  bool Execute(const std::shared_ptr<CFileItem>& item) const override;
};

struct CEjectDrive : CStaticContextMenuAction
{
  CEjectDrive() : CStaticContextMenuAction(13420) {} // Eject Removable HDD!
  bool IsVisible(const CFileItem& item) const override;
  bool Execute(const std::shared_ptr<CFileItem>& item) const override;
};

struct CAddRemoveFavourite : IContextMenuItem
{
  CAddRemoveFavourite() = default;
  std::string GetLabel(const CFileItem& item) const override;
  bool IsVisible(const CFileItem& item) const override;
  bool Execute(const std::shared_ptr<CFileItem>& item) const override;
};

}
