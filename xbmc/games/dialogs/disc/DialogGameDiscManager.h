/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "games/GameTypes.h"
#include "guilib/GUIDialog.h"

#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <string>

class CGUIBaseContainer;

namespace KODI
{
namespace GAME
{
class CDiscManagerActions;
class CDiscManagerButtons;
class CDiscManagerGame;
class CGameClientDiscModel;

/*!
 * \ingroup games
 *
 * \brief Dialog for managing multi-disc game media during gameplay.
 *
 * This dialog provides the in-game UI for disc-related operations such as
 * viewing the selected disc, ejecting or inserting the virtual tray, choosing
 * a different disc, adding or removing discs, and opening the disc-selection
 * submenu when needed.
 *
 * It coordinates between the GUI and the active game client’s disc model,
 * while keeping the main menu and disc-selection list in sync with the
 * emulator’s current disc state.
 */
class CDialogGameDiscManager : public CGUIDialog
{
public:
  CDialogGameDiscManager();
  ~CDialogGameDiscManager() override = default;

  // Implementation of CGUIControl via CGUIDialog
  bool OnAction(const CAction& action) override;
  bool OnMessage(CGUIMessage& message) override;

  // Dialog interface
  void UpdateMenu();
  void FocusMainMenuItem(unsigned int itemIndex);

  // Disc menu interface
  void SelectDiscToInsert(std::optional<size_t> selectedIndex,
                          std::function<void(std::optional<size_t>)> callback);
  void SelectDiscToDelete(std::function<void(size_t)> callback);
  void OnDiscSelect(size_t discIndex, bool isNoDisc);
  bool AllowSelectNoDisc() const;

protected:
  // Implementation of CGUIWindow via CGUIDialog
  void OnInitWindow() override;
  void OnDeinitWindow(int nextWindowID) override;

private:
  // Dialog helpers
  void InitializeGameClient();
  void InitializeDialog();
  void ClearDiscList();
  void ShowControl(int controlId);
  CGUIBaseContainer* GetMainMenu();
  CGUIBaseContainer* GetDiscList();

  // Game parameters
  GameClientPtr m_gameClient;

  // Dialog components
  std::unique_ptr<CDiscManagerActions> m_discActions;
  std::unique_ptr<CDiscManagerButtons> m_discButtons;
  std::unique_ptr<CDiscManagerGame> m_discGame;

  // Dialog parameters
  std::function<void(std::optional<size_t>)> m_insertCallback;
  std::function<void(size_t)> m_deleteCallback;
};
} // namespace GAME
} // namespace KODI
