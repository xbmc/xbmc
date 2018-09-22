/*
 *  Copyright (C) 2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIPlaybackControl.h"
#include "games/dialogs/osd/DialogGameOSD.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "ServiceBroker.h"

using namespace KODI;
using namespace RETRO;

CGUIPlaybackControl::CGUIPlaybackControl(IPlaybackCallback &callback) :
  m_callback(callback)
{
}

CGUIPlaybackControl::~CGUIPlaybackControl() = default;

void CGUIPlaybackControl::FrameMove()
{
  CGUIComponent *gui = CServiceBroker::GetGUI();
  if (gui == nullptr)
    return;

  const int windowId = gui->GetWindowManager().GetActiveWindow();
  const int dialogId = gui->GetWindowManager().GetActiveWindowOrDialog();

  // Check if game has entered fullscreen yet
  const bool bFullscreen = (windowId == WINDOW_FULLSCREEN_GAME);

  // Check if game is in the OSD dialog
  const bool bInMenu = (dialogId != WINDOW_FULLSCREEN_GAME);

  // Check if game should play in the background of dialog
  const bool bInBackground = GAME::CDialogGameOSD::PlayInBackground(dialogId);

  GuiState nextState = NextState(bFullscreen, bInMenu, bInBackground);
  if (nextState != m_state)
  {
    m_state = nextState;

    double targetSpeed = GetTargetSpeed(m_state);
    if (m_previousSpeed != targetSpeed)
    {
      m_previousSpeed = targetSpeed;
      m_callback.SetPlaybackSpeed(targetSpeed);
    }

    m_callback.EnableInput(AcceptsInput(m_state));
  }
}

CGUIPlaybackControl::GuiState CGUIPlaybackControl::NextState(bool bFullscreen, bool bInMenu, bool bInBackground)
{
  GuiState newState = m_state;

  switch (m_state)
  {
  case GuiState::UNKNOWN:
  {
    // Wait for game to enter fullscreen
    if (bFullscreen)
      newState = GuiState::FULLSCREEN;
    break;
  }
  case GuiState::FULLSCREEN:
  {
    if (bInMenu)
    {
      if (bInBackground)
        newState = GuiState::MENU_PLAYING;
      else
        newState = GuiState::MENU_PAUSED;
    }
    break;
  }
  case GuiState::MENU_PAUSED:
  {
    if (!bInMenu)
      newState = GuiState::FULLSCREEN;
    else if (bInBackground)
      newState = GuiState::MENU_PLAYING;
    break;
  }
  case GuiState::MENU_PLAYING:
  {
    if (!bInBackground)
    {
      if (!bInMenu)
        newState = GuiState::FULLSCREEN;
      else
        newState = GuiState::MENU_PAUSED;
    }
    break;
  }
  default:
    break;
  }

  return newState;
}

double CGUIPlaybackControl::GetTargetSpeed(GuiState state)
{
  double targetSpeed = 0.0;

  switch (state)
  {
  case GuiState::FULLSCREEN:
  {
    targetSpeed = 1.0;
    break;
  }
  case GuiState::MENU_PAUSED:
  {
    targetSpeed = 0.0;
    break;
  }
  case GuiState::MENU_PLAYING:
  {
    targetSpeed = 1.0;
    break;
  }
  default:
    break;
  }

  return targetSpeed;
}

bool CGUIPlaybackControl::AcceptsInput(GuiState state)
{
  bool bEnableInput = false;

  switch (state)
  {
    case GuiState::FULLSCREEN:
    {
      bEnableInput = true;
      break;
    }
    default:
      break;
  }

  return bEnableInput;
}
