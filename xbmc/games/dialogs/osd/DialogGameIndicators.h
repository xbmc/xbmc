/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/GUIDialog.h"

#include <atomic>

namespace KODI
{
namespace GAME
{
/*!
 * \ingroup games
 *
 * \brief The corner indicators shown over a running game
 *
 * Two things want to be on screen while the player is playing: the achievement
 * they are inside an attempt at, and how far along a measured achievement is.
 *
 * \section indicator_why_a_dialog Why this is a dialog
 *
 * Where the game has its own DRM plane - the direct-to-plane path every
 * LibreELEC box takes - CGUIWindowFullScreen deliberately stops marking itself
 * dirty each frame, and CApplication then skips compositing the GUI layer
 * entirely while nothing else dirties it. A control in that window becoming
 * visible is not enough to bring the layer back; opening a dialog is, which is
 * why notifications show over a running game.
 *
 * While it is up this marks itself dirty each frame so the layer keeps being
 * composited; it is only up while there is something to show, so the saving
 * that optimisation exists for is kept the rest of the time.
 *
 * Modeless: the player is still playing, and must keep their input.
 */
class CDialogGameIndicators : public CGUIDialog
{
public:
  CDialogGameIndicators();
  ~CDialogGameIndicators() override = default;

  // Implementation of CGUIControl via CGUIDialog
  void Process(unsigned int currentTime, CDirtyRegionList& dirtyregions) override;

  // Implementation of CGUIWindow via CGUIDialog
  void OnInitWindow() override;
  void OnDeinitWindow(int nextWindowID) override;

  /*!
   * \brief Start listening for indicators worth showing
   *
   * Called once. Everything that changes an indicator reports to the runtime,
   * and the runtime reports here, so a new indicator needs no wiring of its own.
   */
  static void Register();

private:
  /*!
   * \brief Open the dialog if the runtime has something to show
   *
   * Only ever opens. Closing is decided in Process(), on the GUI thread, so
   * that the two decisions are not raced across threads.
   */
  static void Show();

  static bool AnythingToShow();

  //! Whether the dialog is up. Kept here because Show() runs on the game's
  //! thread, which cannot ask the window manager.
  static std::atomic<bool> m_showing;
};
} // namespace GAME
} // namespace KODI
