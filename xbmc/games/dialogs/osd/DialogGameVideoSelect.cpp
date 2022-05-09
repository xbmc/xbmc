/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DialogGameVideoSelect.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "cores/RetroPlayer/guibridge/GUIGameRenderManager.h"
#include "cores/RetroPlayer/guibridge/GUIGameVideoHandle.h"
#include "guilib/GUIBaseContainer.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUIWindowManager.h"
#include "input/actions/ActionIDs.h"
#include "settings/GameSettings.h"
#include "settings/MediaSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "view/GUIViewControl.h"
#include "view/ViewState.h"
#include "windowing/GraphicContext.h"

using namespace KODI;
using namespace GAME;

#define CONTROL_HEADING 1
#define CONTROL_THUMBS 11
#define CONTROL_DESCRIPTION 12

CDialogGameVideoSelect::CDialogGameVideoSelect(int windowId)
  : CGUIDialog(windowId, "DialogSelect.xml"),
    m_viewControl(new CGUIViewControl),
    m_vecItems(new CFileItemList)
{
  // Initialize CGUIWindow
  m_loadType = KEEP_IN_MEMORY;
}

CDialogGameVideoSelect::~CDialogGameVideoSelect() = default;

bool CDialogGameVideoSelect::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
    case GUI_MSG_WINDOW_INIT:
    {
      RegisterDialog();

      // Don't init this dialog if we aren't playing a game
      if (!m_gameVideoHandle || !m_gameVideoHandle->IsPlayingGame())
        return false;

      break;
    }
    case GUI_MSG_WINDOW_DEINIT:
    {
      UnregisterDialog();

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

          OnClickAction();

          // Changed from sending ACTION_SHOW_OSD to closing the dialog
          Close();

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
  CGUIBaseContainer* thumbs = dynamic_cast<CGUIBaseContainer*>(GetControl(CONTROL_THUMBS));
  if (thumbs != nullptr)
    OnItemFocus(thumbs->GetSelectedItem());

  CGUIDialog::FrameMove();
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

  std::string heading = GetHeading();
  SET_CONTROL_LABEL(CONTROL_HEADING, heading);

  FrameMove();
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
  // CServiceBroker::GetWinSystem()->GetGfxContext().Lock();

  m_viewControl->SetCurrentView(DEFAULT_VIEW_ICONS);

  // Empty the list ready for population
  Clear();

  OnRefreshList();

  // CServiceBroker::GetWinSystem()->GetGfxContext().Unlock();
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

  auto focusedIndex = GetFocusedItem();
  m_viewControl->SetSelectedItem(focusedIndex);
  OnItemFocus(focusedIndex);
}

void CDialogGameVideoSelect::SaveSettings()
{
  CGameSettings& defaultSettings = CMediaSettings::GetInstance().GetDefaultGameSettings();
  CGameSettings& currentSettings = CMediaSettings::GetInstance().GetCurrentGameSettings();

  if (defaultSettings != currentSettings)
  {
    defaultSettings = currentSettings;
    CServiceBroker::GetSettingsComponent()->GetSettings()->Save();
  }
}

void CDialogGameVideoSelect::OnDescriptionChange(const std::string& description)
{
  CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), CONTROL_DESCRIPTION);
  msg.SetLabel(description);
  CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg, GetID());
}

void CDialogGameVideoSelect::RegisterDialog()
{
  m_gameVideoHandle = CServiceBroker::GetGameRenderManager().RegisterDialog(*this);
}

void CDialogGameVideoSelect::UnregisterDialog()
{
  m_gameVideoHandle.reset();
}
