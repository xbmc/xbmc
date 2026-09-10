/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "games/cheats/CheatPack.h"
#include "guilib/GUIDialog.h"

#include <string>
#include <vector>

namespace KODI::GAME
{
/*! \ingroup games
 *  \brief Toggle the cheats available for the game being played.
 */
class CDialogGameCheats : public CGUIDialog
{
public:
  CDialogGameCheats();
  ~CDialogGameCheats() override;

protected:
  bool OnMessage(CGUIMessage& message) override;
  void OnInitWindow() override;
  void OnDeinitWindow(int nextWindowID) override;

  virtual void InitializeControls();
  void CreateControls(std::vector<Cheat> cheats, bool getMore);

private:
  void ClearControls();
  bool IsCheatControl(int controlId) const;
  bool IsListAction(int controlId) const;
  void ToggleCheat(int controlId);
  void GetMore();
  void UpdateEnabledSummary();
  std::string EnabledSummary() const;

  std::vector<Cheat> m_cheats;
  int m_getMoreControl{0};
  bool m_restoreListFocus{false};
};
} // namespace KODI::GAME
