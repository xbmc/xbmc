/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DialogGameDiscChanger.h"

#include "ServiceBroker.h"
#include "dialogs/GUIDialogBoxBase.h"
#include "games/dialogs/disc/DiscManagerGame.h"
#include "guilib/GUIProgressControl.h"
#include "guilib/WindowIDs.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"
#include "utils/Variant.h"

#include <algorithm>

using namespace KODI;
using namespace GAME;

namespace
{
constexpr auto DISC_CHANGE_DURATION = std::chrono::milliseconds{1500};
} // namespace

CDialogGameDiscChanger::CDialogGameDiscChanger() : m_discGame(std::make_unique<CDiscManagerGame>())
{
  // Initialize CGUIWindow via CGUIDialogProgress
  SetID(WINDOW_DIALOG_GAME_DISC_CHANGER);
  m_loadType = KEEP_IN_MEMORY;

  // Initialize CGUIDialogProgress
  m_bCanCancel = false;
}

void CDialogGameDiscChanger::OnWindowLoaded()
{
  // Call ancestor
  CGUIDialogProgress::OnWindowLoaded();

  // Initialize controls
  m_progressControl = dynamic_cast<CGUIProgressControl*>(GetControl(CONTROL_PROGRESS_BAR));
  if (m_progressControl != nullptr)
  {
    // Info is likely set to System.Progressbar, which we don't want to use
    m_progressControl->SetInfo(0);
  }
}

void CDialogGameDiscChanger::OnInitWindow()
{
  // Initialize ancestor
  CGUIDialogProgress::OnInitWindow();

  // Initialize dialog components
  m_discGame->Initialize(CDiscManagerGame::GetGameClient());

  // Initialize dialog controls
  SetHeading(CVariant{GetHeader()});
  SetLine(0, CVariant{35282}); // "Running game..."
  SetPercentage(0);
  SetCanCancel(false);

  // Initialize dialog parameters
  m_progressStartTime = std::chrono::steady_clock::now();
  m_isProgressRunning = true;
}

void CDialogGameDiscChanger::OnDeinitWindow(int nextWindowID)
{
  // Deinitialize dialog parameters
  m_progressStartTime = {};
  m_isProgressRunning = false;

  // Deinitialize dialog components
  m_discGame->Deinitialize();

  // Deinitialize ancestor
  CGUIDialogProgress::OnDeinitWindow(nextWindowID);
}

bool CDialogGameDiscChanger::OnAction(const CAction& action)
{
  switch (action.GetID())
  {
    case ACTION_SELECT_ITEM:
    case ACTION_MOUSE_LEFT_CLICK:
    case ACTION_MOUSE_RIGHT_CLICK:
    case ACTION_PARENT_DIR:
    case ACTION_PREVIOUS_MENU:
    case ACTION_NAV_BACK:
    {
      Close();
      return true;
    }
    default:
      break;
  }

  // Call ancestor
  return CGUIDialogProgress::OnAction(action);
}

void CDialogGameDiscChanger::FrameMove()
{
  if (m_isProgressRunning)
  {
    const auto elapsed = std::chrono::steady_clock::now() - m_progressStartTime;

    const float progress =
        std::clamp(std::chrono::duration<float>(elapsed).count() /
                       std::chrono::duration<float>(DISC_CHANGE_DURATION).count(),
                   0.0f, 1.0f);

    const int percent = static_cast<int>(progress * 100.0f + 0.5f);

    // Update dialog
    SetPercentage(percent);

    // Update controls
    if (m_progressControl != nullptr)
      m_progressControl->SetPercentage(progress * 100.0f);

    // Finished action
    if (progress >= 1.0f)
    {
      m_isProgressRunning = false;
      Close();
    }
  }

  // Call ancestor
  CGUIDialogProgress::FrameMove();
}

std::string CDialogGameDiscChanger::GetHeader()
{
  std::string header;

  bool ejected;
  std::string selectedDisc;
  m_discGame->GetState(ejected, selectedDisc);

  if (ejected)
    header = CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(162); // "Tray open"
  else
    header = selectedDisc;

  // Fallback to generic label
  if (header.empty())
    header = CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(35272); // "Disc menu"

  return header;
}
