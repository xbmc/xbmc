#pragma once

/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "guilib/GUIDialog.h"

class CFileItem;
class CFileItemList;

class CGUIDialogFavourites :
      public CGUIDialog
{
public:
  CGUIDialogFavourites(void);
  virtual ~CGUIDialogFavourites(void);
  virtual bool OnMessage(CGUIMessage &message);
  virtual void OnInitWindow();

  virtual CFileItemPtr GetCurrentListItem(int offset = 0);

  virtual bool HasListItems() const { return true; };

protected:
  int GetSelectedItem();
  void OnClick(int item);
  void OnPopupMenu(int item);
  void OnMoveItem(int item, int amount);
  void OnDelete(int item);
  void OnRename(int item);
  void UpdateList();

  CFileItemList* m_favourites;
};
