/*
 *  Copyright (C) 2005-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "GUIDialogBoxBase.h"
#include "view/GUIViewControl.h"

#include <string>

class CFileItem;
class CFileItemList;

class CGUIDialogColorPicker : public CGUIDialogBoxBase
{
public:
  CGUIDialogColorPicker();
  ~CGUIDialogColorPicker() override;
  bool OnMessage(CGUIMessage& message) override;
  bool OnBack(int actionID) override;
  void Reset();
  /*! \brief Add a color item (Label property must be the color name, Label2 property must be the color hex)
      \param item The CFileItem
  */
  void AddItem(const CFileItem& item);
  /*! \brief Set a list of color items (Label property must be the color name, Label2 property must be the color hex)
      \param pList The CFileItemList
  */
  void SetItems(const CFileItemList& pList);
  /*! \brief Load a list of colors from the default xml */
  void LoadColors();
  /*! \brief Load a list of colors from the specified xml file path
      \param filePath The xml file path
  */
  void LoadColors(const std::string& filePath);
  /*! \brief Get the hex value of the selected color */
  std::string GetSelectedColor() const;
  /*! \brief Set the selected color by hex value */
  void SetSelectedColor(const std::string& hexColor);
  /*! \brief Set the focus to the control button */
  void SetButtonFocus(bool buttonFocus);

protected:
  CGUIControl* GetFirstFocusableControl(int id) override;
  void OnWindowLoaded() override;
  void OnInitWindow() override;
  void OnDeinitWindow(int nextWindowID) override;
  void OnWindowUnload() override;

  virtual void OnSelect(int idx);

private:
  int GetSelectedItem() const;

  CGUIViewControl m_viewControl;
  CFileItemList* m_vecList;
  bool m_focusToButton = false;
  std::string m_selectedColor;
};
