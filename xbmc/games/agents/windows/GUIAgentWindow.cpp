/*
 *  Copyright (C) 2022-2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIAgentWindow.h"

#include "GUIAgentDefines.h"
#include "GUIAgentList.h"
#include "IAgentList.h"
#include "ServiceBroker.h"
#include "addons/AddonManager.h"
#include "addons/IAddon.h"
#include "addons/addoninfo/AddonType.h"
#include "cores/RetroPlayer/guibridge/GUIGameRenderManager.h"
#include "cores/RetroPlayer/guibridge/GUIGameSettingsHandle.h"
#include "games/addons/GameClient.h"
#include "games/controllers/types/ControllerTree.h"
#include "games/ports/guicontrols/GUIActivePortList.h"
#include "guilib/GUIButtonControl.h"
#include "guilib/GUIControl.h"
#include "guilib/GUIMessage.h"
#include "guilib/WindowIDs.h"
#include "input/actions/ActionIDs.h"
#include "utils/StringUtils.h"

using namespace KODI;
using namespace GAME;

CGUIAgentWindow::CGUIAgentWindow()
  : CGUIDialog(WINDOW_DIALOG_GAME_AGENTS, AGENT_DIALOG_XML),
    m_portList(std::make_unique<CGUIActivePortList>(*this, CONTROL_ACTIVE_PORT_LIST, true)),
    m_agentList(std::make_unique<CGUIAgentList>(*this))
{
  // Initialize CGUIWindow
  m_loadType = KEEP_IN_MEMORY;
}

CGUIAgentWindow::~CGUIAgentWindow() = default;

bool CGUIAgentWindow::OnMessage(CGUIMessage& message)
{
  // Set to true to block the call to the super class
  bool bHandled = false;

  switch (message.GetMessage())
  {
    case GUI_MSG_SETFOCUS:
    {
      const int controlId = message.GetControlId();
      if (m_agentList->HasControl(controlId) && m_agentList->GetCurrentControl() != controlId)
      {
        FocusAgentList();
        bHandled = true;
      }
      break;
    }
    case GUI_MSG_CLICKED:
    {
      const int controlId = message.GetSenderId();

      if (controlId == CONTROL_AGENT_CLOSE_BUTTON)
      {
        CloseDialog();
        bHandled = true;
      }
      else if (m_agentList->HasControl(controlId))
      {
        const int actionId = message.GetParam1();
        if (actionId == ACTION_SELECT_ITEM || actionId == ACTION_MOUSE_LEFT_CLICK)
        {
          OnAgentClick();
          bHandled = true;
        }
      }
      break;
    }
    case GUI_MSG_REFRESH_LIST:
    {
      const int controlId = message.GetControlId();
      switch (controlId)
      {
        case CONTROL_ACTIVE_PORT_LIST:
        {
          UpdateActivePortList();
          bHandled = true;
          break;
        }
        case CONTROL_AGENT_LIST:
        {
          UpdateAgentList();
          bHandled = true;
          break;
        }
        default:
          break;
      }
      break;
    }

    default:
      break;
  }

  if (!bHandled)
    bHandled = CGUIDialog::OnMessage(message);

  return bHandled;
}

void CGUIAgentWindow::FrameMove()
{
  CGUIDialog::FrameMove();

  m_agentList->FrameMove();
}

void CGUIAgentWindow::OnWindowLoaded()
{
  CGUIDialog::OnWindowLoaded();

  m_agentList->OnWindowLoaded();
}

void CGUIAgentWindow::OnWindowUnload()
{
  m_agentList->OnWindowUnload();

  CGUIDialog::OnWindowUnload();
}

void CGUIAgentWindow::OnInitWindow()
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

  // Initialize GUI
  m_portList->Initialize(m_gameClient);
  m_agentList->Initialize(m_gameClient);
}

void CGUIAgentWindow::OnDeinitWindow(int nextWindowID)
{
  // Deinitialize GUI
  m_agentList->Deinitialize();
  m_portList->Deinitialize();

  // Deinitialize game properties
  m_gameClient.reset();

  CGUIDialog::OnDeinitWindow(nextWindowID);
}

void CGUIAgentWindow::CloseDialog()
{
  Close();
}

void CGUIAgentWindow::UpdateActivePortList()
{
  m_portList->Refresh();
}

void CGUIAgentWindow::UpdateAgentList()
{
  m_agentList->Refresh();
}

void CGUIAgentWindow::FocusAgentList()
{
  m_agentList->SetFocused();
}

void CGUIAgentWindow::OnAgentClick()
{
  m_agentList->OnSelect();
}
