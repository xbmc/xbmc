/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/GUIDialog.h"

namespace KODI::GAME
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
 * Where the game has its own plane, CGameWindowFullScreen stops marking itself
 * dirty each frame and CApplication then skips compositing the GUI layer while
 * nothing else dirties it, so a control there quietly becoming visible does not
 * bring the layer back. Opening a dialog does.
 *
 * While it is up this marks itself dirty each frame so the layer keeps being
 * composited, and it is only up while there is something to show, so the rest
 * of the session keeps the saving that optimisation exists for.
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

  /*!
   * \brief Open the indicators if the runtime has anything to show
   *
   * Safe to call from the game thread: opening is posted to the GUI thread,
   * which is the only one allowed to do it. Closing is decided in Process(),
   * which already runs there.
   */
  static void Show();

private:

  //! \brief True while any indicator has something to show
  static bool AnythingToShow();
};
} // namespace KODI::GAME
