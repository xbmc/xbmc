/*
 *  Copyright (C) 2020-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DialogGameSaves.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "cores/RetroPlayer/savestates/ISavestate.h"
#include "cores/RetroPlayer/savestates/SavestateDatabase.h"
#include "dialogs/GUIDialogContextMenu.h"
#include "dialogs/GUIDialogYesNo.h"
#include "games/dialogs/DialogGameDefines.h"
#include "guilib/GUIBaseContainer.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIKeyboardFactory.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/WindowIDs.h"
#include "input/Key.h"
#include "utils/FileUtils.h"
#include "utils/Variant.h"

using namespace KODI;
using namespace GAME;

namespace
{
constexpr int CONTROL_DETAILED_LIST = 6;
constexpr int CONTROL_DESCRIPTION = 12;
} // namespace

CDialogGameSaves::CDialogGameSaves() : CGUIDialogSelect(WINDOW_DIALOG_GAME_SAVES)
{
}

bool CDialogGameSaves::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
    case GUI_MSG_CLICKED:
    {
      if (m_viewControl.HasControl(CONTROL_DETAILED_LIST))
      {
        int action = message.GetParam1();
        if (action == ACTION_CONTEXT_MENU || action == ACTION_MOUSE_RIGHT_CLICK)
        {
          int selectedItem = m_viewControl.GetSelectedItem();
          if (selectedItem >= 0 && selectedItem < m_vecList->Size())
          {
            CFileItemPtr item = m_vecList->Get(selectedItem);
            OnPopupMenu(std::move(item));
            return true;
          }
        }
      }
      break;
    }
    default:
      break;
  }

  return CGUIDialogSelect::OnMessage(message);
}

void CDialogGameSaves::FrameMove()
{
  CGUIControl* itemContainer = GetControl(CONTROL_DETAILED_LIST);
  if (itemContainer != nullptr)
  {
    if (itemContainer->HasFocus())
    {
      int selectedItem = m_viewControl.GetSelectedItem();
      if (selectedItem >= 0 && selectedItem < m_vecList->Size())
      {
        CFileItemPtr item = m_vecList->Get(selectedItem);
        OnFocus(std::move(item));
      }
    }
    else
    {
      OnFocusLost();
    }
  }

  CGUIDialogSelect::FrameMove();
}

std::string CDialogGameSaves::GetSelectedItemPath()
{
  if (m_selectedItem != nullptr)
    return m_selectedItem->GetPath();

  return "";
}

void CDialogGameSaves::OnFocus(CFileItemPtr item)
{
  const std::string caption = item->GetProperty(SAVESTATE_CAPTION).asString();

  HandleCaption(caption);
}

void CDialogGameSaves::OnFocusLost()
{
  HandleCaption("");
}

void CDialogGameSaves::OnPopupMenu(CFileItemPtr item)
{
  const std::string& savePath = item->GetPath();

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
        std::unique_ptr<RETRO::ISavestate> savestate =
            RETRO::CSavestateDatabase::AllocateSavestate();
        db.GetSavestate(savePath, *savestate);

        item->SetLabel(label);

        CDateTime date = CDateTime::FromUTCDateTime(savestate->Created());
        item->SetLabel2(date.GetAsLocalizedDateTime(false, false));

        item->SetProperty(SAVESTATE_CAPTION, savestate->Caption());
      }
    }
  }
  else if (index == 1)
  {
    if (CGUIDialogYesNo::ShowAndGetInput(CVariant{122}, CVariant{125}))
    {
      if (db.DeleteSavestate(savePath))
        m_vecList->Remove(item.get());
    }
  }

  m_viewControl.SetItems(*m_vecList);
}

void CDialogGameSaves::HandleCaption(const std::string& caption)
{
  if (caption != m_currentCaption)
  {
    m_currentCaption = caption;

    // Update the GUI label
    CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), CONTROL_DESCRIPTION);
    msg.SetLabel(m_currentCaption);
    CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg, GetID());
  }
}
