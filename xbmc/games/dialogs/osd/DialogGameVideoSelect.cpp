/*
 *      Copyright (C) 2017 Team Kodi
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
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "DialogGameVideoSelect.h"
#include "guilib/GraphicContext.h"
#include "guilib/GUIBaseContainer.h"
#include "guilib/WindowIDs.h"
#include "input/Action.h"
#include "input/ActionIDs.h"
#include "messaging/ApplicationMessenger.h"
#include "settings/GameSettings.h"
#include "settings/MediaSettings.h"
#include "settings/Settings.h"
#include "view/GUIViewControl.h"
#include "view/ViewState.h"
#include "Application.h"
#include "ApplicationPlayer.h"
#include "FileItem.h"
#include "ServiceBroker.h"

using namespace KODI;
using namespace GAME;

#define CONTROL_THUMBS                11

CDialogGameVideoSelect::CDialogGameVideoSelect(int windowId) :
  CGUIDialog(windowId, "DialogSelect.xml"),
  m_viewControl(new CGUIViewControl),
  m_vecItems(new CFileItemList)
{
  // Initialize CGUIWindow
  m_loadType = KEEP_IN_MEMORY;
}

CDialogGameVideoSelect::~CDialogGameVideoSelect() = default;

void CDialogGameVideoSelect::RegisterCallback(RETRO::IRenderSettingsCallback *callback)
{
  m_callback = callback;
}

void CDialogGameVideoSelect::UnregisterCallback()
{
  m_callback = nullptr;
}

bool CDialogGameVideoSelect::OnMessage(CGUIMessage &message)
{
  switch (message.GetMessage())
  {
    case GUI_MSG_WINDOW_INIT:
    {
      // Don't init this dialog if we aren't playing a game
      if (m_callback == nullptr)
        return false;
      break;
    }
    case GUI_MSG_SETFOCUS:
    {
      const int controlId = message.GetControlId();
      if (m_viewControl->HasControl(controlId) && m_viewControl->GetCurrentControl() != controlId)
      {
        m_viewControl->SetFocused();
        return true;
      }
      break;
    }
    case GUI_MSG_CLICKED:
    {
      const int actionId = message.GetParam1();
      if (actionId == ACTION_SELECT_ITEM || actionId == ACTION_MOUSE_LEFT_CLICK)
      {
        const int controlId = message.GetSenderId();
        if (m_viewControl->HasControl(controlId))
        {
          using namespace MESSAGING;

          // Send OSD command to resume gameplay
          CAction *action = new CAction(ACTION_SHOW_OSD);
          CApplicationMessenger::GetInstance().PostMsg(TMSG_GUI_ACTION, WINDOW_INVALID, -1, static_cast<void*>(action));

          return true;
        }
      }
      break;
    }
    case GUI_MSG_REFRESH_LIST:
    {
      OnRefreshList();
      break;
    }
    default:
      break;
  }

  return CGUIDialog::OnMessage(message);
}

void CDialogGameVideoSelect::FrameMove()
{
  CGUIBaseContainer *thumbs = dynamic_cast<CGUIBaseContainer*>(GetControl(CONTROL_THUMBS));
  if (thumbs != nullptr)
    OnItemFocus(thumbs->GetSelectedItem());
}

void CDialogGameVideoSelect::OnWindowLoaded()
{
  CGUIDialog::OnWindowLoaded();

  m_viewControl->SetParentWindow(GetID());
  m_viewControl->AddView(GetControl(CONTROL_THUMBS));
}

void CDialogGameVideoSelect::OnWindowUnload()
{
  m_viewControl->Reset();

  CGUIDialog::OnWindowUnload();
}

void CDialogGameVideoSelect::OnInitWindow()
{
  PreInit();

  CGUIDialog::OnInitWindow();

  Update();

  CGUIMessage msg(GUI_MSG_SETFOCUS, GetID(), CONTROL_THUMBS);
  OnMessage(msg);
}

void CDialogGameVideoSelect::OnDeinitWindow(int nextWindowID)
{
  Clear();

  CGUIDialog::OnDeinitWindow(nextWindowID);

  PostExit();

  SaveSettings();
}

void CDialogGameVideoSelect::Update()
{
  //! @todo
  // Lock our display, as this window is rendered from the player thread
  //g_graphicsContext.Lock();

  m_viewControl->SetCurrentView(DEFAULT_VIEW_ICONS);

  // Empty the list ready for population
  Clear();

  OnRefreshList();

  //g_graphicsContext.Unlock();
}

void CDialogGameVideoSelect::Clear()
{
  m_viewControl->Clear();
  m_vecItems->Clear();
}

void CDialogGameVideoSelect::OnRefreshList()
{
  m_vecItems->Clear();

  GetItems(*m_vecItems);

  m_viewControl->SetItems(*m_vecItems);
  m_viewControl->SetSelectedItem(GetFocusedItem());
}

void CDialogGameVideoSelect::SaveSettings()
{
  CGameSettings &defaultSettings = CMediaSettings::GetInstance().GetDefaultGameSettings();
  CGameSettings &currentSettings = CMediaSettings::GetInstance().GetCurrentGameSettings();

  if (defaultSettings != currentSettings)
  {
    defaultSettings = currentSettings;
    CServiceBroker::GetSettings().Save();
  }
}
