/*
 *  Copyright (C) 2020-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogSelectSavestate.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "cores/RetroPlayer/savestates/ISavestate.h"
#include "cores/RetroPlayer/savestates/SavestateDatabase.h"
#include "games/dialogs/osd/DialogGameSaves.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"

using namespace KODI;
using namespace GAME;

bool CGUIDialogSelectSavestate::ShowAndGetSavestate(const std::string& gamePath,
                                                    std::string& savestatePath)
{
  RETRO::CSavestateDatabase db;
  CFileItemList items;
  bool bSuccess = false;

  savestatePath = "";

  if (!db.GetSavestatesNav(items, gamePath))
    return bSuccess;

  LogSavestates(items);

  items.Sort(SortByDate, SortOrderDescending);

  // if there are no saves there is no need to prompt the user
  if (items.Size() == 0)
  {
    savestatePath = "";
    return true;
  }
  else
  {
    // "Select savestate"
    CDialogGameSaves* dialog = GetDialog(g_localizeStrings.Get(35260));

    if (dialog != nullptr)
    {
      dialog->SetItems(items);
      dialog->Open();

      if (dialog->IsConfirmed())
      {
        std::string itemPath = dialog->GetSelectedItemPath();

        if (!itemPath.empty())
        {
          savestatePath = itemPath;
          bSuccess = true;
        }
        else
        {
          CLog::Log(LOGDEBUG, "Select savestate dialog: User selected invalid savestate");
        }
      }
      else if (dialog->IsButtonPressed())
      {
        CLog::Log(LOGDEBUG, "Select savestate dialog: New savestate selected");
        bSuccess = true;
      }
    }
  }

  return bSuccess;
}

CDialogGameSaves* CGUIDialogSelectSavestate::GetDialog(const std::string& title)
{
  CDialogGameSaves* dialog =
      CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CDialogGameSaves>(
          WINDOW_DIALOG_GAME_SAVES);

  if (dialog != nullptr)
  {
    dialog->Reset();
    dialog->SetHeading(CVariant{title});
    dialog->SetUseDetails(true);
    dialog->EnableButton(true, 35261); // "New game"
  }

  return dialog;
}

void CGUIDialogSelectSavestate::LogSavestates(const CFileItemList& items)
{
  CLog::Log(LOGDEBUG, "Select savestate dialog: Found {} saves", items.Size());
  for (int i = 0; i < items.Size(); i++)
  {
    CLog::Log(LOGDEBUG, "{}", items[i]->GetPath());
  }
}
