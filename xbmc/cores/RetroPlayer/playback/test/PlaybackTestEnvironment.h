/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "ServiceBroker.h"
#include "ServiceManager.h"
#include "application/Application.h"
#include "cores/RetroPlayer/guibridge/GUIGameMessenger.h"
#include "cores/RetroPlayer/guibridge/GUIGameRenderManager.h"
#include "cores/RetroPlayer/process/RPProcessInfo.h"
#include "cores/RetroPlayer/rendering/RPRenderManager.h"
#include "games/GameServices.h"
#include "games/controllers/ControllerManager.h"
#include "input/InputManager.h"
#include "interfaces/AnnouncementManager.h"
#include "peripherals/Peripherals.h"
#include "settings/SettingsComponent.h"
#include "windowing/WinSystem.h"

namespace KODI::RETRO
{
class CPlaybackTestEnvironment
{
public:
  CPlaybackTestEnvironment()
    : m_controllers(CServiceBroker::GetAddonMgr()),
      m_peripherals(m_input, m_controllers),
      m_previousWinSystem(CServiceBroker::GetWinSystem())
  {
    auto services = std::make_unique<GAME::CGameServices>(
        m_controllers, m_guiRenderer, m_peripherals,
        *CServiceBroker::GetSettingsComponent()->GetProfileManager(), m_input,
        CServiceBroker::GetAddonMgr(), CServiceBroker::GetFileExtensionProvider());
    auto& registered = CServices::GameServices(*g_application.m_ServiceManager);
    m_previousServices = std::move(registered);
    registered = std::move(services);
    CServiceBroker::RegisterWinSystem(&m_winSystem);
    m_processInfo = std::make_unique<CProcessInfo>();
    m_renderer = std::make_unique<CRPRenderManager>(*m_processInfo);
    m_messenger = std::make_unique<CGUIGameMessenger>(*m_processInfo);
  }

  ~CPlaybackTestEnvironment()
  {
    m_messenger.reset();
    m_renderer.reset();
    m_processInfo.reset();
    if (m_previousWinSystem)
      CServiceBroker::RegisterWinSystem(m_previousWinSystem);
    else
      CServiceBroker::UnregisterWinSystem();
    CServices::GameServices(*g_application.m_ServiceManager) = std::move(m_previousServices);
  }

  CRPRenderManager& Renderer() { return *m_renderer; }
  CGUIGameMessenger& Messenger() { return *m_messenger; }

private:
  class CServices : public CServiceManager
  {
  public:
    static std::unique_ptr<GAME::CGameServices>& GameServices(CServiceManager& manager)
    {
      return manager.*&CServices::m_gameServices;
    }
  };

  class CWinSystem : public CWinSystemBase
  {
  public:
    CRenderSystemBase* GetRenderSystem() override { return nullptr; }
    bool CreateNewWindow(const std::string&, bool, RESOLUTION_INFO&) override { return true; }
    bool ResizeWindow(int, int, int, int) override { return true; }
    bool SetFullScreen(bool, RESOLUTION_INFO&, bool) override { return true; }
    void Register(IDispResource*) override {}
    void Unregister(IDispResource*) override {}
  };

  class CProcessInfo : public CRPProcessInfo
  {
  public:
    CProcessInfo() : CRPProcessInfo("test") {}
  };

  class CAnnouncements
  {
  public:
    CAnnouncements() : m_previous(CServiceBroker::GetAnnouncementManager())
    {
      CServiceBroker::RegisterAnnouncementManager(
          std::make_shared<ANNOUNCEMENT::CAnnouncementManager>());
    }
    ~CAnnouncements()
    {
      if (m_previous)
        CServiceBroker::RegisterAnnouncementManager(m_previous);
      else
        CServiceBroker::UnregisterAnnouncementManager();
    }

  private:
    std::shared_ptr<ANNOUNCEMENT::CAnnouncementManager> m_previous;
  };

  CAnnouncements m_announcements;
  CInputManager m_input;
  GAME::CControllerManager m_controllers;
  PERIPHERALS::CPeripherals m_peripherals;
  CGUIGameRenderManager m_guiRenderer;
  CWinSystem m_winSystem;
  CWinSystemBase* m_previousWinSystem;
  std::unique_ptr<GAME::CGameServices> m_previousServices;
  std::unique_ptr<CProcessInfo> m_processInfo;
  std::unique_ptr<CRPRenderManager> m_renderer;
  std::unique_ptr<CGUIGameMessenger> m_messenger;
};
} // namespace KODI::RETRO
