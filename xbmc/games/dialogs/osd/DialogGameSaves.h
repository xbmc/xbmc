/*
 *  Copyright (C) 2020-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/GUIDialog.h"

#include <memory>
#include <string>

class CFileItem;
class CFileItemList;
class CGUIMessage;
class CGUIViewControl;

namespace KODI
{
namespace GAME
{
/*!
 * \ingroup games
 */
class CDialogGameSaves : public CGUIDialog
{
public:
  CDialogGameSaves();
  ~CDialogGameSaves() override = default;

  // implementation of CGUIControl via CGUIDialog
  bool OnMessage(CGUIMessage& message) override;

  // implementation of CGUIWindow via CGUIDialog
  void FrameMove() override;

  // Player interface
  void Reset();
  bool Open(const std::string& gamePath);
  bool IsConfirmed() const { return m_bConfirmed; }
  bool IsNewPressed() const { return m_bNewPressed; }
  std::string GetSelectedItemPath();

protected:
  // implementation of CGUIWIndow via CGUIDialog
  void OnInitWindow() override;
  void OnDeinitWindow(int nextWindowID) override;
  void OnWindowLoaded() override;
  void OnWindowUnload() override;

private:
  using CGUIControl::OnFocus;

  /*!
   * \breif Called when opening to set the item list
   */
  void SetItems(const CFileItemList& itemList);

  /*!
   * \brief Called when an item has been selected
   */
  void OnSelect(const CFileItem& item);

  /*!
   * \brief Called every frame with the item being focused
   */
  void OnFocus(const CFileItem& item);

  /*!
   * \brief Called every frame if no item is focused
   */
  void OnFocusLost();

  /*!
   * \brief Called when a context menu is opened for an item
   */
  void OnContextMenu(CFileItem& item);

  /*!
  * \brief Called when "Rename" is selected from the context menu
  */
  void OnRename(CFileItem& item);

  /*!
  * \brief Called when "Delete" is selected from the context menu
  */
  void OnDelete(CFileItem& item);

  /*!
   * \brief Called every frame with the caption to set
   */
  void HandleCaption(const std::string& caption);

  /*!
  * \brief Called every frame with the game client to set
  */
  void HandleGameClient(const std::string& gameClientId);

  // Dialog parameters
  std::unique_ptr<CGUIViewControl> m_viewControl;
  std::unique_ptr<CFileItemList> m_vecList;
  std::shared_ptr<CFileItem> m_selectedItem;

  // Player parameters
  bool m_bConfirmed{false};
  bool m_bNewPressed{false};

  // State parameters
  std::string m_currentCaption;
  std::string m_currentGameClient;
};
} // namespace GAME
} // namespace KODI
