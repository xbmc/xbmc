/*
 *  Copyright (C) 2020-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "dialogs/GUIDialogSelect.h"

class CGUIMessage;

namespace KODI
{
namespace GAME
{
class CDialogGameSaves : public CGUIDialogSelect
{
public:
  CDialogGameSaves();
  ~CDialogGameSaves() override = default;

  // implementation of CGUIControl via CGUIDialog
  bool OnMessage(CGUIMessage& message) override;

  // implementation of CGUIWindow via CGUIDialog
  void FrameMove() override;

  std::string GetSelectedItemPath();

private:
  using CGUIControl::OnFocus;

  /*!
   * \brief Called every frame with the item being focused
   */
  void OnFocus(CFileItemPtr item);

  /*!
   * \brief Called every frame if no item is focused
   */
  void OnFocusLost();

  /*!
   * \brief Called when a popup menu is opened for an item
   */
  void OnPopupMenu(CFileItemPtr item);

  /*!
   * \brief Called every frame with the caption to set
   */
  void HandleCaption(const std::string& caption);

  // State parameters
  std::string m_currentCaption;
};
} // namespace GAME
} // namespace KODI
