/*
 *  Copyright (C) 2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIPortWindow.h"

#include "GUIPortDefines.h"
#include "GUIPortList.h"
#include "ServiceBroker.h"
#include "addons/AddonManager.h"
#include "addons/IAddon.h"
#include "addons/addoninfo/AddonType.h"
#include "cores/RetroPlayer/guibridge/GUIGameRenderManager.h"
#include "cores/RetroPlayer/guibridge/GUIGameSettingsHandle.h"
#include "games/addons/GameClient.h"
#include "guilib/GUIButtonControl.h"
#include "guilib/GUIControl.h"
#include "guilib/GUIMessage.h"
#include "guilib/WindowIDs.h"
#include "input/actions/ActionIDs.h"
#include "utils/StringUtils.h"

using namespace KODI;
using namespace GAME;

CGUIPortWindow::CGUIPortWindow()
  : CGUIDialog(WINDOW_DIALOG_GAME_PORTS, PORT_DIALOG_XML),
    m_portList(std::make_unique<CGUIPortList>(*this))
{
  // Initialize CGUIWindow
  m_loadType = KEEP_IN_MEMORY;
}

CGUIPortWindow::~CGUIPortWindow() = default;

bool CGUIPortWindow::OnMessage(CGUIMessage& message)
{
  // Set to true to block the call to the super class
  bool bHandled = false;

  switch (message.GetMessage())
  {
    case GUI_MSG_SETFOCUS:
    {
      const int controlId = message.GetControlId();
      if (m_portList->HasControl(controlId) && m_portList->GetCurrentControl() != controlId)
      {
        FocusPortList();
        bHandled = true;
      }
      break;
    }
    case GUI_MSG_CLICKED:
    {
      const int controlId = message.GetSenderId();

      if (controlId == CONTROL_CLOSE_BUTTON)
      {
        CloseDialog();
        bHandled = true;
      }
      else if (controlId == CONTROL_RESET_BUTTON)
      {
        ResetPorts();
        bHandled = true;
      }
      else if (m_portList->HasControl(controlId))
      {
        const int actionId = message.GetParam1();
        if (actionId == ACTION_SELECT_ITEM || actionId == ACTION_MOUSE_LEFT_CLICK)
        {
          OnClickAction();
          bHandled = true;
        }
      }
      break;
    }
    case GUI_MSG_REFRESH_LIST:
    {
      UpdatePortList();
      break;
    }
    default:
      break;
  }

  if (!bHandled)
    bHandled = CGUIDialog::OnMessage(message);

  return bHandled;
}

void CGUIPortWindow::OnWindowLoaded()
{
  CGUIDialog::OnWindowLoaded();

  m_portList->OnWindowLoaded();
}

void CGUIPortWindow::OnWindowUnload()
{
  m_portList->OnWindowUnload();

  CGUIDialog::OnWindowUnload();
}

void CGUIPortWindow::OnInitWindow()
{
  CGUIDialog::OnInitWindow();

  // Get active game add-on
  GameClientPtr gameClient;
  {
    auto gameSettingsHandle = CServiceBroker::GetGameRenderManager().RegisterGameSettingsDialog();
    if (gameSettingsHandle)
    {
      ADDON::AddonPtr addon;
      if (CServiceBroker::GetAddonMgr().GetAddon(gameSettingsHandle->GameClientID(), addon,
                                                 ADDON::AddonType::GAMEDLL,
                                                 ADDON::OnlyEnabled::CHOICE_YES))
        gameClient = std::static_pointer_cast<CGameClient>(addon);
    }
  }
  m_gameClient = std::move(gameClient);

  // Set the heading
  // "Port Setup - {game client name}"
  SET_CONTROL_LABEL(CONTROL_PORT_DIALOG_LABEL,
                    StringUtils::Format("$LOCALIZE[35111] - {}", m_gameClient->Name()));

  m_portList->Initialize(m_gameClient);

  UpdatePortList();

  // Focus the port list
  CGUIMessage msgFocus(GUI_MSG_SETFOCUS, GetID(), CONTROL_PORT_LIST);
  OnMessage(msgFocus);
}

void CGUIPortWindow::OnDeinitWindow(int nextWindowID)
{
  m_portList->Deinitialize();

  m_gameClient.reset();

  CGUIDialog::OnDeinitWindow(nextWindowID);
}

void CGUIPortWindow::FrameMove()
{
  CGUIDialog::FrameMove();

  m_portList->FrameMove();
}

void CGUIPortWindow::UpdatePortList()
{
  m_portList->Refresh();
}

void CGUIPortWindow::FocusPortList()
{
  m_portList->SetFocused();
}

bool CGUIPortWindow::OnClickAction()
{
  return m_portList->OnSelect();
}

void CGUIPortWindow::ResetPorts()
{
  m_portList->ResetPorts();
}

void CGUIPortWindow::CloseDialog()
{
  Close();
}
