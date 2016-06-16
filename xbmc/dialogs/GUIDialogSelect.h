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

#include <string>
#include <vector>

#include "GUIDialogBoxBase.h"
#include "view/GUIViewControl.h"

class CFileItem;
class CFileItemList;

class CGUIDialogSelect : public CGUIDialogBoxBase
{
public:
  CGUIDialogSelect();
  virtual ~CGUIDialogSelect(void);
  virtual bool OnMessage(CGUIMessage& message) override;
  virtual bool OnBack(int actionID) override;

  void Reset();
  int  Add(const std::string& strLabel);
  int  Add(const CFileItem& item);
  void SetItems(const CFileItemList& items);
  const CFileItemPtr GetSelectedFileItem() const;
  int GetSelectedItem() const;
  const std::vector<int>& GetSelectedItems() const;
  void EnableButton(bool enable, int label);
  bool IsButtonPressed();
  void Sort(bool bSortOrder = true);
  void SetSelected(int iSelected);
  void SetSelected(const std::string &strSelectedLabel);
  void SetSelected(std::vector<int> selectedIndexes);
  void SetSelected(const std::vector<std::string> &selectedLabels);
  void SetUseDetails(bool useDetails);
  void SetMultiSelection(bool multiSelection);

protected:
  CGUIDialogSelect(int windowid);
  virtual CGUIControl *GetFirstFocusableControl(int id) override;
  virtual void OnWindowLoaded() override;
  virtual void OnInitWindow() override;
  virtual void OnDeinitWindow(int nextWindowID) override;
  virtual void OnWindowUnload() override;

  virtual void OnSelect(int idx);

private:
  bool m_bButtonEnabled;
  bool m_bButtonPressed;
  int m_buttonLabel;
  CFileItemPtr m_selectedItem;
  bool m_useDetails;
  bool m_multiSelection;

  std::vector<int> m_selectedItems;
  std::unique_ptr<CFileItemList> m_vecList;
  CGUIViewControl m_viewControl;
};
