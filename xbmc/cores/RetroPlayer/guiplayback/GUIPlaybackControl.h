/*
 *  Copyright (C) 2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/RetroPlayer/playback/IPlaybackControl.h"

namespace KODI
{
namespace RETRO
{
/*!
 * \brief Class to control playback by monitoring OSD status
 */
class CGUIPlaybackControl : public IPlaybackControl
{
public:
  CGUIPlaybackControl(IPlaybackCallback& callback);

  ~CGUIPlaybackControl() override;

  // Implementation of IPlaybackControl
  void FrameMove() override;

private:
  enum class GuiState
  {
    UNKNOWN,
    FULLSCREEN,
    MENU_PAUSED,
    MENU_PLAYING,
  };

  // Helper functions
  GuiState NextState(bool bFullscreen, bool bInMenu, bool bInBackground);
  static double GetTargetSpeed(GuiState state);
  static bool AcceptsInput(GuiState state);

  // Construction parameters
  IPlaybackCallback& m_callback;

  // State parameters
  GuiState m_state = GuiState::UNKNOWN;
  double m_previousSpeed = 0.0;
};
} // namespace RETRO
} // namespace KODI
