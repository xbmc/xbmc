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
#include "dialogs/GUIDialogOK.h"
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

CDialogGameSaves::CDialogGameSaves() : CGUIDialogSelect(WINDOW_DIALOG_GAME_SAVES)
{
}

bool CDialogGameSaves::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
    case GUI_MSG_CLICKED:
    {
      if (m_viewControl.HasControl(CONTROL_SAVES_DETAILED_LIST))
      {
        int action = message.GetParam1();
        if (action == ACTION_CONTEXT_MENU || action == ACTION_MOUSE_RIGHT_CLICK)
        {
          int selectedItem = m_viewControl.GetSelectedItem();
          if (selectedItem >= 0 && selectedItem < m_vecList->Size())
          {
            CFileItemPtr item = m_vecList->Get(selectedItem);
            if (item)
            {
              OnContextMenu(*item);
              return true;
            }
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
  CGUIControl* itemContainer = GetControl(CONTROL_SAVES_DETAILED_LIST);
  if (itemContainer != nullptr)
  {
    if (itemContainer->HasFocus())
    {
      int selectedItem = m_viewControl.GetSelectedItem();
      if (selectedItem >= 0 && selectedItem < m_vecList->Size())
      {
        CFileItemPtr item = m_vecList->Get(selectedItem);
        if (item)
          OnFocus(*item);
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

void CDialogGameSaves::OnFocus(const CFileItem& item)
{
  const std::string caption = item.GetProperty(SAVESTATE_CAPTION).asString();

  HandleCaption(caption);
}

void CDialogGameSaves::OnFocusLost()
{
  HandleCaption("");
}

void CDialogGameSaves::OnContextMenu(CFileItem& item)
{
  CContextButtons buttons;

  buttons.Add(0, 118); // "Rename"
  buttons.Add(1, 117); // "Delete"

  const int index = CGUIDialogContextMenu::Show(buttons);

  if (index == 0)
    OnRename(item);
  else if (index == 1)
    OnDelete(item);

  m_viewControl.SetItems(*m_vecList);
}

void CDialogGameSaves::OnRename(CFileItem& item)
{
  const std::string& savestatePath = item.GetPath();

  // Get savestate properties
  RETRO::CSavestateDatabase db;
  std::unique_ptr<RETRO::ISavestate> savestate = RETRO::CSavestateDatabase::AllocateSavestate();
  db.GetSavestate(savestatePath, *savestate);

  std::string label(savestate->Label());

  // "Enter new filename"
  if (CGUIKeyboardFactory::ShowAndGetInput(label, CVariant{g_localizeStrings.Get(16013)}, true) &&
      label != savestate->Label())
  {
    std::unique_ptr<RETRO::ISavestate> newSavestate = db.RenameSavestate(savestatePath, label);
    if (newSavestate)
    {
      RETRO::CSavestateDatabase::GetSavestateItem(*newSavestate, savestatePath, item);

      // Refresh thumbnails
      CGUIMessage message(GUI_MSG_REFRESH_LIST, GetID(), CONTROL_SAVES_DETAILED_LIST);
      OnMessage(message);
    }
    else
    {
      // "Error"
      // "An unknown error has occurred."
      CGUIDialogOK::ShowAndGetInput(257, 24071);
    }
  }
}

void CDialogGameSaves::OnDelete(CFileItem& item)
{
  // "Confirm delete"
  // "Would you like to delete the selected file(s)?[CR]Warning - this action can't be undone!"
  if (CGUIDialogYesNo::ShowAndGetInput(CVariant{122}, CVariant{125}))
  {
    const std::string& savestatePath = item.GetPath();

    RETRO::CSavestateDatabase db;
    if (db.DeleteSavestate(savestatePath))
    {
      m_vecList->Remove(&item);

      // Refresh thumbnails
      CGUIMessage message(GUI_MSG_REFRESH_LIST, GetID(), CONTROL_SAVES_DETAILED_LIST);
      OnMessage(message);
    }
    else
    {
      // "Error"
      // "An unknown error has occurred."
      CGUIDialogOK::ShowAndGetInput(257, 24071);
    }
  }
}

void CDialogGameSaves::HandleCaption(const std::string& caption)
{
  if (caption != m_currentCaption)
  {
    m_currentCaption = caption;

    // Update the GUI label
    CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), CONTROL_SAVES_DESCRIPTION);
    msg.SetLabel(m_currentCaption);
    CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg, GetID());
  }
}
