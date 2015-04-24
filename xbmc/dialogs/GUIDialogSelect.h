#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#include "GUIDialogBoxBase.h"
#include "view/GUIViewControl.h"

class CFileItem;
class CFileItemList;

class CGUIDialogSelect :
      public CGUIDialogBoxBase
{
public:
  CGUIDialogSelect(void);
  virtual ~CGUIDialogSelect(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnBack(int actionID);

  void Reset();
  int  Add(const std::string& strLabel);
  int  Add(const CFileItem* pItem);
  void Add(const CFileItemList& items);
  void SetItems(CFileItemList* items);
  int GetSelectedLabel() const;
  const std::string& GetSelectedLabelText() const;
  const CFileItemPtr GetSelectedItem() const;
  const CFileItemList& GetSelectedItems() const;
  void EnableButton(bool enable, int string);
  bool IsButtonPressed();
  void Sort(bool bSortOrder = true);
  void SetSelected(int iSelected);
  void SetSelected(const std::string &strSelectedLabel);
  void SetSelected(std::vector<int> selectedIndexes);
  void SetSelected(const std::vector<std::string> &selectedLabels);
  void SetUseDetails(bool useDetails);
  void SetMultiSelection(bool multiSelection);
protected:
  virtual CGUIControl *GetFirstFocusableControl(int id);
  virtual void OnWindowLoaded();
  virtual void OnInitWindow();
  virtual void OnWindowUnload();
  void SetupButton();

  bool m_bButtonEnabled;
  int m_buttonString;
  bool m_bButtonPressed;
  int m_iSelected;
  bool m_useDetails;
  bool m_multiSelection;

  CFileItemList* m_selectedItems;
  CFileItemList* m_vecList;
  CGUIViewControl m_viewControl;
};
