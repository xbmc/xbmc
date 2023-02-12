/*
 *  Copyright (C) 2020-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogSelectSavestate.h"

#include "ServiceBroker.h"
#include "games/dialogs/osd/DialogGameSaves.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "utils/log.h"

using namespace KODI;
using namespace GAME;

bool CGUIDialogSelectSavestate::ShowAndGetSavestate(const std::string& gamePath,
                                                    std::string& savestatePath)
{
  savestatePath = "";

  // Can't ask the user if there's no dialog
  CDialogGameSaves* dialog = GetDialog();
  if (dialog == nullptr)
    return true;

  if (!dialog->Open(gamePath))
    return true;

  if (dialog->IsConfirmed())
  {
    savestatePath = dialog->GetSelectedItemPath();
    return true;
  }
  else if (dialog->IsNewPressed())
  {
    CLog::Log(LOGDEBUG, "Select savestate dialog: New savestate selected");
    return true;
  }

  // User canceled the dialog
  return false;
}

CDialogGameSaves* CGUIDialogSelectSavestate::GetDialog()
{
  CDialogGameSaves* dialog =
      CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CDialogGameSaves>(
          WINDOW_DIALOG_GAME_SAVES);

  if (dialog != nullptr)
    dialog->Reset();

  return dialog;
}
