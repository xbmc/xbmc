/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "GUIDialogBoxBase.h"
#include "view/GUIViewControl.h"

#include <string>
#include <vector>

class CFileItem;
class CFileItemList;

class CGUIDialogSelect : public CGUIDialogBoxBase
{
public:
  CGUIDialogSelect();
  ~CGUIDialogSelect(void) override;
  bool OnMessage(CGUIMessage& message) override;
  bool OnBack(int actionID) override;

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
  void SetButtonFocus(bool buttonFocus);

protected:
  explicit CGUIDialogSelect(int windowid);
  CGUIControl *GetFirstFocusableControl(int id) override;
  void OnWindowLoaded() override;
  void OnInitWindow() override;
  void OnDeinitWindow(int nextWindowID) override;
  void OnWindowUnload() override;

  virtual void OnSelect(int idx);

private:
  bool m_bButtonEnabled;
  bool m_bButtonPressed;
  int m_buttonLabel;
  CFileItemPtr m_selectedItem;
  bool m_useDetails;
  bool m_multiSelection;
  bool m_focusToButton{};

  std::vector<int> m_selectedItems;
  std::unique_ptr<CFileItemList> m_vecList;
  CGUIViewControl m_viewControl;
};
