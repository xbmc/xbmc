/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DialogGameIndicators.h"

#include "ServiceBroker.h"
#include "games/AchievementRuntime.h"
#include "games/GameServices.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/WindowIDs.h"
#include "messaging/ApplicationMessenger.h"

using namespace KODI::GAME;

CDialogGameIndicators::CDialogGameIndicators()
  : CGUIDialog(
        WINDOW_DIALOG_GAME_INDICATORS, "DialogGameIndicators.xml", DialogModalityType::MODELESS)
{
  m_loadType = KEEP_IN_MEMORY;
}

void CDialogGameIndicators::Process(unsigned int currentTime, CDirtyRegionList& dirtyregions)
{
  // Closing is decided here rather than where the runtime changed, because this
  // runs on the GUI thread and can act at once rather than posting and then
  // testing whether the post has been served.
  if (!AnythingToShow())
  {
    Close();
    return;
  }

  // Where the game has its own plane the GUI layer is only composited when
  // something dirties it, and a label changing its text is not enough. This is
  // only reached while an indicator is up, so the rest of the session still
  // gets the saving that optimisation exists for.
  MarkDirtyRegion();

  CGUIDialog::Process(currentTime, dirtyregions);
}

bool CDialogGameIndicators::AnythingToShow()
{
  return CServiceBroker::GetGameServices().AchievementRuntime().HasIndicators();
}

void CDialogGameIndicators::Show()
{
  if (!AnythingToShow())
    return;

  CGUIWindowManager& windowManager = CServiceBroker::GetGUI()->GetWindowManager();

  const CGUIWindow* dialog = windowManager.GetWindow(WINDOW_DIALOG_GAME_INDICATORS);
  if (dialog == nullptr || dialog->IsActive())
    return;

  // GetWindow is safe from any thread; opening one is not, and this is called
  // from the game thread
  CServiceBroker::GetAppMessenger()->PostMsg(TMSG_GUI_ACTIVATE_WINDOW,
                                             WINDOW_DIALOG_GAME_INDICATORS, 0);
}
