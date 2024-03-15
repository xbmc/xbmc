/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AppInboundProtocol.h"

#include "ServiceBroker.h"
#include "application/Application.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPowerHandling.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "input/InputManager.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "messaging/ApplicationMessenger.h"
#include "settings/AdvancedSettings.h"
#include "settings/DisplaySettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "threads/SingleLock.h"

CAppInboundProtocol::CAppInboundProtocol(CApplication &app) : m_pApp(app)
{
}

bool CAppInboundProtocol::OnEvent(const XBMC_Event& newEvent)
{
  std::unique_lock<CCriticalSection> lock(m_portSection);
  if (m_closed)
    return false;
  m_portEvents.push_back(newEvent);
  return true;
}

void CAppInboundProtocol::Close()
{
  std::unique_lock<CCriticalSection> lock(m_portSection);
  m_closed = true;
}

void CAppInboundProtocol::SetRenderGUI(bool renderGUI)
{
  auto& components = CServiceBroker::GetAppComponents();
  const auto appPower = components.GetComponent<CApplicationPowerHandling>();
  appPower->SetRenderGUI(renderGUI);
}

void CAppInboundProtocol::HandleEvents()
{
  std::unique_lock<CCriticalSection> lock(m_portSection);
  while (!m_portEvents.empty())
  {
    auto newEvent = m_portEvents.front();
    m_portEvents.pop_front();
    CSingleExit lock(m_portSection);
    switch (newEvent.type)
    {
      case XBMC_QUIT:
        if (!m_pApp.m_bStop)
          CServiceBroker::GetAppMessenger()->PostMsg(TMSG_QUIT);
        break;
      case XBMC_VIDEORESIZE:
        if (CServiceBroker::GetGUI()->GetWindowManager().Initialized())
        {
          if (!CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_fullScreen)
          {
            CServiceBroker::GetWinSystem()->GetGfxContext().ApplyWindowResize(newEvent.resize.w,
                                                                              newEvent.resize.h);

            const auto settings = CServiceBroker::GetSettingsComponent()->GetSettings();
            settings->SetInt(CSettings::SETTING_WINDOW_WIDTH, newEvent.resize.w);
            settings->SetInt(CSettings::SETTING_WINDOW_HEIGHT, newEvent.resize.h);
            settings->Save();
          }
          else
          {
            const auto& res_info = CDisplaySettings::GetInstance().GetResolutionInfo(RES_DESKTOP);
            CServiceBroker::GetWinSystem()->ForceFullScreen(res_info);
          }
        }
        break;
      case XBMC_VIDEOMOVE:
      {
        CServiceBroker::GetWinSystem()->OnMove(newEvent.move.x, newEvent.move.y);
      }
      break;
      case XBMC_MODECHANGE:
        CServiceBroker::GetWinSystem()->GetGfxContext().ApplyModeChange(newEvent.mode.res);
        break;
      case XBMC_SCREENCHANGE:
        CServiceBroker::GetWinSystem()->OnChangeScreen(newEvent.screen.screenIdx);
        break;
      case XBMC_USEREVENT:
        CServiceBroker::GetAppMessenger()->PostMsg(static_cast<uint32_t>(newEvent.user.code));
        break;
      case XBMC_SETFOCUS:
      {
        // Reset the screensaver
        const auto appPower = m_pApp.GetComponent<CApplicationPowerHandling>();
        appPower->ResetScreenSaver();
        appPower->WakeUpScreenSaverAndDPMS();
        // Send a mouse motion event with no dx,dy for getting the current guiitem selected
        m_pApp.OnAction(CAction(ACTION_MOUSE_MOVE, 0, static_cast<float>(newEvent.focus.x),
                                static_cast<float>(newEvent.focus.y), 0, 0));
        break;
      }
      default:
        CServiceBroker::GetInputManager().OnEvent(newEvent);
    }
  }
}
