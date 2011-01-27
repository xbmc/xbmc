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

#include "GUIDialogBoxBase.h"
#include "guilib/GUIListItem.h"
#include "GUIViewControl.h"

class CFileItem;
class CFileItemList;

class CGUIDialogSelect :
      public CGUIDialogBoxBase
{
public:
  CGUIDialogSelect(void);
  virtual ~CGUIDialogSelect(void);
  virtual bool OnMessage(CGUIMessage& message);

  void Reset();
  void Add(const CStdString& strLabel);
  void Add(const CFileItem* pItem);
  void Add(const CFileItemList& items);
  void SetItems(CFileItemList* items);
  int GetSelectedLabel() const;
  const CStdString& GetSelectedLabelText();
  const CFileItem& GetSelectedItem();
  void EnableButton(bool enable, int string);
  bool IsButtonPressed();
  void Sort(bool bSortOrder = true);
  void SetSelected(int iSelected);
  void SetUseDetails(bool useDetails);
protected:
  virtual CGUIControl *GetFirstFocusableControl(int id);
  virtual void OnWindowLoaded();
  virtual void OnInitWindow();

  bool m_bButtonEnabled;
  bool m_bButtonPressed;
  int m_iSelected;
  bool m_useDetails;

  CFileItem* m_selectedItem;
  CFileItemList* m_vecListInternal;
  CFileItemList* m_vecList;
  CGUIViewControl m_viewControl;
};
