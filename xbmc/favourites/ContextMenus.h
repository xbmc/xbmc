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

class CFileItemList;

namespace CONTEXTMENU
{

class CFavouriteContextMenuAction : public CStaticContextMenuAction
{
public:
  explicit CFavouriteContextMenuAction(uint32_t label) : CStaticContextMenuAction(label) {}
  bool IsVisible(const CFileItem& item) const override;
  bool Execute(const std::shared_ptr<CFileItem>& item) const override;

protected:
  ~CFavouriteContextMenuAction() override = default;
  virtual bool DoExecute(CFileItemList& items, const std::shared_ptr<CFileItem>& item) const = 0;
};

class CMoveUpFavourite : public CFavouriteContextMenuAction
{
public:
  CMoveUpFavourite() : CFavouriteContextMenuAction(13332) {} // Move up
  bool IsVisible(const CFileItem& item) const override;

protected:
  bool DoExecute(CFileItemList& items, const std::shared_ptr<CFileItem>& item) const override;
};

class CMoveDownFavourite : public CFavouriteContextMenuAction
{
public:
  CMoveDownFavourite() : CFavouriteContextMenuAction(13333) {} // Move down
  bool IsVisible(const CFileItem& item) const override;

protected:
  bool DoExecute(CFileItemList& items, const std::shared_ptr<CFileItem>& item) const override;
};

class CRemoveFavourite : public CFavouriteContextMenuAction
{
public:
  CRemoveFavourite() : CFavouriteContextMenuAction(15015) {} // Remove
protected:
  bool DoExecute(CFileItemList& items, const std::shared_ptr<CFileItem>& item) const override;
};

class CRenameFavourite : public CFavouriteContextMenuAction
{
public:
  CRenameFavourite() : CFavouriteContextMenuAction(118) {} // Rename
protected:
  bool DoExecute(CFileItemList& items, const std::shared_ptr<CFileItem>& item) const override;
};

class CChooseThumbnailForFavourite : public CFavouriteContextMenuAction
{
public:
  CChooseThumbnailForFavourite() : CFavouriteContextMenuAction(20019) {} // Choose thumbnail
protected:
  bool DoExecute(CFileItemList& items, const std::shared_ptr<CFileItem>& item) const override;
};

class CFavouritesTargetBrowse : public CStaticContextMenuAction
{
public:
  explicit CFavouritesTargetBrowse() : CStaticContextMenuAction(37015) {} // Browse into
  bool IsVisible(const CFileItem& item) const override;
  bool Execute(const std::shared_ptr<CFileItem>& item) const override;
};

class CFavouritesTargetResume : public IContextMenuItem
{
public:
  std::string GetLabel(const CFileItem& item) const override;
  bool IsVisible(const CFileItem& item) const override;
  bool Execute(const std::shared_ptr<CFileItem>& item) const override;
};

class CFavouritesTargetPlay : public IContextMenuItem
{
public:
  std::string GetLabel(const CFileItem& item) const override;
  bool IsVisible(const CFileItem& item) const override;
  bool Execute(const std::shared_ptr<CFileItem>& item) const override;
};

class CFavouritesTargetInfo : public CStaticContextMenuAction
{
public:
  explicit CFavouritesTargetInfo() : CStaticContextMenuAction(19033) {} // Information
  bool IsVisible(const CFileItem& item) const override;
  bool Execute(const std::shared_ptr<CFileItem>& item) const override;
};

class CFavouritesTargetContextMenu : public CStaticContextMenuAction
{
public:
  explicit CFavouritesTargetContextMenu() : CStaticContextMenuAction(22082) {} // More...
  bool IsVisible(const CFileItem& item) const override;
  bool Execute(const std::shared_ptr<CFileItem>& item) const override;
};

} // namespace CONTEXTMENU
