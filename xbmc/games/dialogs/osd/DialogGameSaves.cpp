/*
 *  Copyright (C) 2020-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DialogGameSaves.h"

#include "FileItem.h"
#include "cores/RetroPlayer/savestates/SavestateDatabase.h"
#include "dialogs/GUIDialogContextMenu.h"
#include "dialogs/GUIDialogYesNo.h"
#include "guilib/GUIKeyboardFactory.h"
#include "guilib/GUIMessage.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/WindowIDs.h"
#include "input/Key.h"
#include "utils/FileUtils.h"

using namespace KODI;
using namespace GAME;

#define CONTROL_SIMPLE_LIST 3

CDialogGameSaves::CDialogGameSaves() : CGUIDialogSelect(WINDOW_DIALOG_GAME_SAVES)
{
}

bool CDialogGameSaves::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
    case GUI_MSG_CLICKED:
    {
      if (m_viewControl.HasControl(CONTROL_SIMPLE_LIST))
      {
        int action = message.GetParam1();
        if (action == ACTION_CONTEXT_MENU || action == ACTION_MOUSE_RIGHT_CLICK)
        {
          int selectedItem = m_viewControl.GetSelectedItem();
          if (selectedItem >= 0 && selectedItem < m_vecList->Size())
          {
            OnPopupMenu(selectedItem);
            return true;
          }
        }
      }
    }
  }

  return CGUIDialogSelect::OnMessage(message);
}

void CDialogGameSaves::OnPopupMenu(int itemIndex)
{
  const std::string savePath = m_vecList->Get(itemIndex)->GetPath();

  CContextButtons buttons;

  buttons.Add(0, 118); // "Rename"
  buttons.Add(1, 117); // "Delete"

  int index = CGUIDialogContextMenu::Show(buttons);
  RETRO::CSavestateDatabase db;

  if (index == 0)
  {
    std::string label;
    // "Enter new filename"
    if (CGUIKeyboardFactory::ShowAndGetInput(label, CVariant{g_localizeStrings.Get(16013)}, false))
    {
      if (db.RenameSavestate(savePath, label))
      {
        CFileItemPtr item = m_vecList->Get(itemIndex);

        std::unique_ptr<RETRO::ISavestate> savestate =
            RETRO::CSavestateDatabase::AllocateSavestate();
        db.GetSavestate(savePath, *savestate);

        item->SetLabel(label);
        CDateTime date = CDateTime::FromUTCDateTime(savestate->Created());
        item->SetLabel2(date.GetAsLocalizedDateTime(false, false));
      }
    }
  }
  else if (index == 1)
  {
    if (CGUIDialogYesNo::ShowAndGetInput(CVariant{122}, CVariant{125}))
    {
      if (db.DeleteSavestate(savePath))
        m_vecList->Remove(itemIndex);
    }
  }

  m_viewControl.SetItems(*m_vecList);
}

std::string CDialogGameSaves::GetSelectedItemPath()
{
  return m_selectedItem->GetPath();
}