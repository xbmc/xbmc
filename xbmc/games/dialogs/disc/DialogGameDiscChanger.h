/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "dialogs/GUIDialogProgress.h"

#include <chrono>
#include <memory>
#include <string>

class CGUIProgressControl;

namespace KODI
{
namespace GAME
{
class CDiscManagerGame;

/*!
 * \ingroup games
 *
 * \brief Dialog that runs the game for a few seconds to make the selected
 * disc "take effect"
 *
 * This dialog is needed because some libretro cores do not fully register a
 * disc swap just from changing the selected disc or closing the tray. They
 * only notice the new disc after the emulator runs briefly with the tray
 * transition in effect.
 *
 * The dialog gives users a simple one-step way to advance emulation for a
 * moment so the new disc actually takes effect, instead of forcing them to
 * manually resume, wait, and pause again.
 */
class CDialogGameDiscChanger : public CGUIDialogProgress
{
public:
  CDialogGameDiscChanger();
  ~CDialogGameDiscChanger() override = default;

  // Implementation of CGUIControl via CGUIDialogProgress
  bool OnAction(const CAction& action) override;

  // Implementation of CGUIWindow via CGUIDialogProgress
  void FrameMove() override;
  void OnDeinitWindow(int nextWindowID) override;

protected:
  // Implementation of CGUIWindow via CGUIDialogProgress
  void OnWindowLoaded() override;
  void OnInitWindow() override;

private:
  // Helper functions
  std::string GetHeader();

  // Dialog controls
  CGUIProgressControl* m_progressControl{nullptr};

  // Dialog components
  std::unique_ptr<CDiscManagerGame> m_discGame;

  // Dialog parameters
  std::chrono::steady_clock::time_point m_progressStartTime{};
  bool m_isProgressRunning{false};
};
} // namespace GAME
} // namespace KODI
