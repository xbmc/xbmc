/*
 *      Copyright (C) 2016 Team Kodi
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "GUIDialogSavestates.h"
#include "dialogs/GUIDialogContextMenu.h"
#include "dialogs/GUIDialogOK.h"
#include "dialogs/GUIDialogYesNo.h"
#include "games/addons/savestates/Savestate.h"
#include "games/addons/savestates/SavestateDatabase.h"
#include "games/tags/GameInfoTag.h"
#include "games/GameTypes.h"
#include "guilib/GraphicContext.h"
#include "guilib/GUIKeyboardFactory.h"
#include "guilib/LocalizeStrings.h"
#include "input/Key.h"
#include "view/ViewState.h"
#include "Application.h"
#include "FileItem.h"
#include "GUIUserMessages.h"

using namespace KODI;
using namespace GAME;

#define CONTROL_HEADING           1
#define CONTROL_ADD_SAVESTATE     2
#define CONTROL_CLEAR_SAVESTATES  3
#define CONTROL_BUTTON            4
#define CONTROL_THUMBS            11

CGUIDialogSavestates::CGUIDialogSavestates() : 
  CGUIDialog(WINDOW_DIALOG_SAVESTATES, "DialogBookmarks.xml"),
  m_vecItems(new CFileItemList)
{
  // Initialize CGUIWindow
  m_loadType = LOAD_EVERY_TIME;
}

bool CGUIDialogSavestates::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_WINDOW_INIT:
  {
    CGUIWindow::OnMessage(message);

    m_gamePath = message.GetStringParam();
    m_gameClient.clear();

    if (m_gamePath.empty())
    {
      m_gamePath = g_application.CurrentFileItem().GetPath();
      if (g_application.CurrentFileItem().HasGameInfoTag())
        m_gameClient = g_application.CurrentFileItem().GetGameInfoTag()->GetGameClient();
    }

    if (m_gamePath.empty())
      return false;

    // Pause game when leaving fullscreen
    if (g_application.m_pPlayer->IsPlayingGame() && !g_application.m_pPlayer->IsPaused())
      g_application.m_pPlayer->Pause();

    if (!g_application.m_pPlayer->IsPlayingGame())
    {
      CONTROL_DISABLE(CONTROL_ADD_SAVESTATE);
      if (m_defaultControl == CONTROL_ADD_SAVESTATE)
        m_defaultControl = CONTROL_CLEAR_SAVESTATES;
    }

    Update();
    return true;
  }
  case GUI_MSG_WINDOW_DEINIT:
  {
    m_viewControl.Clear();
    m_vecItems->Clear();
    m_gamePath.clear();
    m_gameClient.clear();

    // Resume game when entering fullscreen
    if (g_application.m_pPlayer->IsPlayingGame() && g_application.m_pPlayer->IsPaused())
      g_application.m_pPlayer->Pause();

    break;
  }
  case GUI_MSG_CLICKED:
  {
    int iControl = message.GetSenderId();
    if (iControl == CONTROL_ADD_SAVESTATE)
    {
      CreateSavestate();
    }
    else if (iControl == CONTROL_CLEAR_SAVESTATES)
    {
      ClearSavestates();
    }
    else if (m_viewControl.HasControl(iControl))  // thumb control
    {
      int iItem = m_viewControl.GetSelectedItem();
      CFileItemPtr item = (*m_vecItems)[iItem];
      if (item)
      {
        int iAction = message.GetParam1();
        switch (iAction)
        {
        case ACTION_CONTEXT_MENU:
        {
          OnContextMenu(*item);
          return true;
        }
        case ACTION_DELETE_ITEM:
        {
          DeleteSavestate(*item);
          return true;
        }
        case ACTION_SELECT_ITEM:
        case ACTION_MOUSE_LEFT_CLICK:
        {
          LoadSavestate(*item);
          return true;
        }
        default:
          break;
        }
      }
    }
    break;
  }
  case GUI_MSG_SETFOCUS:
  {
    int iControl = message.GetSenderId();
    if (m_viewControl.HasControl(iControl) && m_viewControl.GetCurrentControl() != iControl)
    {
      m_viewControl.SetFocused();
      return true;
    }
    break;
  }
  default:
    break;
  }

  return CGUIDialog::OnMessage(message);
}

void CGUIDialogSavestates::OnInitWindow()
{
  SET_CONTROL_LABEL(CONTROL_HEADING, 35269);
  SET_CONTROL_LABEL(CONTROL_ADD_SAVESTATE, 35270);
  SET_CONTROL_LABEL(CONTROL_CLEAR_SAVESTATES, 35271);
  SET_CONTROL_HIDDEN(CONTROL_BUTTON);

  CGUIDialog::OnInitWindow();
}

bool CGUIDialogSavestates::OnAction(const CAction &action)
{
  switch (action.GetID())
  {
  case ACTION_CONTEXT_MENU:
  case ACTION_MOUSE_RIGHT_CLICK:
  {
    int iItem = m_viewControl.GetSelectedItem();
    CFileItemPtr item = (*m_vecItems)[iItem];
    if (item)
      OnContextMenu(*item);
    return true;
  }
  default:
    break;
  }

  return CGUIDialog::OnAction(action);
}

CGUIControl* CGUIDialogSavestates::GetFirstFocusableControl(int id)
{
  if (m_viewControl.HasControl(id))
    id = m_viewControl.GetCurrentControl();

  return CGUIWindow::GetFirstFocusableControl(id);
}

void CGUIDialogSavestates::OnWindowLoaded()
{
  CGUIDialog::OnWindowLoaded();

  m_viewControl.Reset();
  m_viewControl.SetParentWindow(GetID());
  m_viewControl.AddView(GetControl(CONTROL_THUMBS));
}

void CGUIDialogSavestates::OnWindowUnload()
{
  m_vecItems->Clear();
  CGUIDialog::OnWindowUnload();
  m_viewControl.Reset();
}

void CGUIDialogSavestates::OnContextMenu(const CFileItem& save)
{
  //! @todo
  // Highlight the item
  //(*m_vecItems)[item]->Select(true);

  CContextButtons choices;
  choices.Add(0, 35272); // Load save state
  choices.Add(1, 118); // Rename
  choices.Add(2, 117); // Delete

  int btnid = CGUIDialogContextMenu::ShowAndGetChoice(choices);

  //! @todo
  // Unhighlight the item
  //(*m_vecItems)[item]->Select(false);

  if (btnid == 0)
    LoadSavestate(save);
  else if (btnid == 1)
    RenameSavestate(save);
  else if (btnid == 2)
    DeleteSavestate(save);
}

void CGUIDialogSavestates::CreateSavestate()
{
  if (!g_application.m_pPlayer->IsPlayingGame())
    return;

  std::string savePath = g_application.m_pPlayer->GetPlayerState();
  if (!savePath.empty())
  {
    // Allow the user to rename the savestate
    CSavestateDatabase db;
    CSavestate newSave;
    if (db.GetSavestate(savePath, newSave))
    {
      std::string label = newSave.Label();
      // Enter title
      if (CGUIKeyboardFactory::ShowAndGetInput(label, g_localizeStrings.Get(528), true))
      {
        if (label != newSave.Label())
          db.RenameSavestate(savePath, label);
      }
      else
      {
        db.DeleteSavestate(savePath);
      }
    }
    Update();
  }
}

void CGUIDialogSavestates::ClearSavestates()
{
  // Confirm delete
  // Would you like to delete the selected file(s)? Warning - this action can't be undone!
  if (CGUIDialogYesNo::ShowAndGetInput(122, 125))
  {
    CSavestateDatabase db;
    if (!db.ClearSavestatesOfGame(m_gamePath, m_gameClient))
    {
      // Delete
      // Failed to delete at least one file. Check the log for more information about this message.
      CGUIDialogOK::ShowAndGetInput(117, 16206);
    }

    Update();
  }
}

void CGUIDialogSavestates::LoadSavestate(const CFileItem& save)
{
  if (g_application.m_pPlayer->IsPlayingGame())
  {
    if (g_application.m_pPlayer->SetPlayerState(save.GetPath()))
    {
      Close();
      return;
    }
  }

  CFileItem game(m_gamePath, false);

  // Set savestate
  game.m_lStartOffset = STARTOFFSET_RESUME;
  game.GetGameInfoTag()->SetSavestate(save.GetPath());
  game.GetGameInfoTag()->SetGameClient(save.GetGameInfoTag()->GetGameClient());

  if (g_application.PlayFile(game, "RetroPlayer"))
    Close();
}

void CGUIDialogSavestates::RenameSavestate(const CFileItem& save)
{
  std::string label = save.GetLabel();
  // Enter title
  if (CGUIKeyboardFactory::ShowAndGetInput(label, g_localizeStrings.Get(528), true))
  {
    if (label != save.GetLabel())
    {
      CSavestateDatabase db;
      if (db.RenameSavestate(save.GetPath(), label))
        Update();
      else
      {
        // Moved failed
        // Failed to move at least one file. Check the log for more information about this message.
        CGUIDialogOK::ShowAndGetInput(16203, 16204);
      }
    }
  }
}

void CGUIDialogSavestates::DeleteSavestate(const CFileItem& save)
{
  // Confirm delete
  // Would you like to delete the selected file(s)? Warning - this action can't be undone!
  if (CGUIDialogYesNo::ShowAndGetInput(122, 125))
  {
    CSavestateDatabase db;
    if (!db.DeleteSavestate(save.GetPath()))
    {
      // Delete
      // Failed to delete at least one file. Check the log for more information about this message.
      CGUIDialogOK::ShowAndGetInput(117, 16206);
    }
    else
    {
      Update();
    }
  }
}

void CGUIDialogSavestates::Update()
{
  m_vecItems->Clear();

  CSavestateDatabase db;
  db.GetSavestatesNav(*m_vecItems, m_gamePath, m_gameClient);

  // Lock our display, as this window is rendered from the player thread
  g_graphicsContext.Lock();

  m_viewControl.SetCurrentView(DEFAULT_VIEW_ICONS);

  // Empty the list ready for population
  m_viewControl.Clear();

  unsigned int selectedItemIndex = 0; //! @todo

  m_viewControl.SetItems(*m_vecItems);
  m_viewControl.SetSelectedItem(selectedItemIndex);

  g_graphicsContext.Unlock();
}
