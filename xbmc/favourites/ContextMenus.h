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
  virtual ~CFavouriteContextMenuAction() = default;
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
