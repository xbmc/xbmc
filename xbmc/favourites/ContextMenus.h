/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "ContextMenuItem.h"


namespace CONTEXTMENU
{

class CFavouriteContextMenuAction : public CStaticContextMenuAction
{
public:
  explicit CFavouriteContextMenuAction(uint32_t label) : CStaticContextMenuAction(label) {}
  bool IsVisible(const CFileItem& item) const override;
  bool Execute(const CFileItemPtr& item) const override;
protected:
  ~CFavouriteContextMenuAction() override = default;
  virtual bool DoExecute(CFileItemList& items, const CFileItemPtr& item) const = 0;
};

class CRemoveFavourite : public CFavouriteContextMenuAction
{
public:
  CRemoveFavourite() : CFavouriteContextMenuAction(15015) {} // Remove
protected:
  bool DoExecute(CFileItemList& items, const CFileItemPtr& item) const override;
};

class CRenameFavourite : public CFavouriteContextMenuAction
{
public:
  CRenameFavourite() : CFavouriteContextMenuAction(118) {} // Rename
protected:
  bool DoExecute(CFileItemList& items, const CFileItemPtr& item) const override;
};

class CChooseThumbnailForFavourite : public CFavouriteContextMenuAction
{
public:
  CChooseThumbnailForFavourite() : CFavouriteContextMenuAction(20019) {} // Choose thumbnail
protected:
  bool DoExecute(CFileItemList& items, const CFileItemPtr& item) const override;
};

}
