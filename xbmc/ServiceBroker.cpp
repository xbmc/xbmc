/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ServiceBroker.h"
#include "Application.h"
#include "profiles/ProfileManager.h"
#include "settings/SettingsComponent.h"
#include "windowing/WinSystem.h"

using namespace KODI;

// announcement
std::shared_ptr<ANNOUNCEMENT::CAnnouncementManager> CServiceBroker::m_pAnnouncementManager;
std::shared_ptr<ANNOUNCEMENT::CAnnouncementManager> CServiceBroker::GetAnnouncementManager()
{
  return m_pAnnouncementManager;
}
void CServiceBroker::RegisterAnnouncementManager(std::shared_ptr<ANNOUNCEMENT::CAnnouncementManager> port)
{
  m_pAnnouncementManager = port;
}
void CServiceBroker::UnregisterAnnouncementManager()
{
  m_pAnnouncementManager.reset();
}

ADDON::CAddonMgr &CServiceBroker::GetAddonMgr()
{
  return g_application.m_ServiceManager->GetAddonMgr();
}

ADDON::CBinaryAddonManager &CServiceBroker::GetBinaryAddonManager()
{
  return g_application.m_ServiceManager->GetBinaryAddonManager();
}

ADDON::CBinaryAddonCache &CServiceBroker::GetBinaryAddonCache()
{
  return g_application.m_ServiceManager->GetBinaryAddonCache();
}

ADDON::CVFSAddonCache &CServiceBroker::GetVFSAddonCache()
{
  return g_application.m_ServiceManager->GetVFSAddonCache();
}

#ifdef HAS_PYTHON
XBPython& CServiceBroker::GetXBPython()
{
  return g_application.m_ServiceManager->GetXBPython();
}
#endif

PVR::CPVRManager &CServiceBroker::GetPVRManager()
{
  return g_application.m_ServiceManager->GetPVRManager();
}

CContextMenuManager& CServiceBroker::GetContextMenuManager()
{
  return g_application.m_ServiceManager->GetContextMenuManager();
}

CDataCacheCore &CServiceBroker::GetDataCacheCore()
{
  return g_application.m_ServiceManager->GetDataCacheCore();
}

PLAYLIST::CPlayListPlayer &CServiceBroker::GetPlaylistPlayer()
{
  return g_application.m_ServiceManager->GetPlaylistPlayer();
}

CSettingsComponent* CServiceBroker::m_pSettingsComponent = nullptr;

void CServiceBroker::RegisterSettingsComponent(CSettingsComponent *settings)
{
  m_pSettingsComponent = settings;
}

void CServiceBroker::UnregisterSettingsComponent()
{
  m_pSettingsComponent = nullptr;
}

CSettingsComponent* CServiceBroker::GetSettingsComponent()
{
  return m_pSettingsComponent;
}

GAME::CControllerManager& CServiceBroker::GetGameControllerManager()
{
  return g_application.m_ServiceManager->GetGameControllerManager();
}

GAME::CGameServices& CServiceBroker::GetGameServices()
{
  return g_application.m_ServiceManager->GetGameServices();
}

KODI::RETRO::CGUIGameRenderManager& CServiceBroker::GetGameRenderManager()
{
  return g_application.m_ServiceManager->GetGameRenderManager();
}

PERIPHERALS::CPeripherals& CServiceBroker::GetPeripherals()
{
  return g_application.m_ServiceManager->GetPeripherals();
}

CFavouritesService& CServiceBroker::GetFavouritesService()
{
  return g_application.m_ServiceManager->GetFavouritesService();
}

ADDON::CServiceAddonManager& CServiceBroker::GetServiceAddons()
{
  return g_application.m_ServiceManager->GetServiceAddons();
}

ADDON::CRepositoryUpdater& CServiceBroker::GetRepositoryUpdater()
{
  return g_application.m_ServiceManager->GetRepositoryUpdater();
}

CInputManager& CServiceBroker::GetInputManager()
{
  return g_application.m_ServiceManager->GetInputManager();
}

CFileExtensionProvider& CServiceBroker::GetFileExtensionProvider()
{
  return g_application.m_ServiceManager->GetFileExtensionProvider();
}

CNetworkBase& CServiceBroker::GetNetwork()
{
  return g_application.m_ServiceManager->GetNetwork();
}

bool CServiceBroker::IsBinaryAddonCacheUp()
{
  return g_application.m_ServiceManager &&
         g_application.m_ServiceManager->init_level > 1;
}

bool CServiceBroker::IsServiceManagerUp()
{
  return g_application.m_ServiceManager &&
         g_application.m_ServiceManager->init_level == 3;
}

CWinSystemBase* CServiceBroker::m_pWinSystem = nullptr;

CWinSystemBase* CServiceBroker::GetWinSystem()
{
  return m_pWinSystem;
}

void CServiceBroker::RegisterWinSystem(CWinSystemBase *winsystem)
{
  m_pWinSystem = winsystem;
}

void CServiceBroker::UnregisterWinSystem()
{
  m_pWinSystem = nullptr;
}

CRenderSystemBase* CServiceBroker::GetRenderSystem()
{
  if (m_pWinSystem)
    return m_pWinSystem->GetRenderSystem();

  return nullptr;
}

CPowerManager& CServiceBroker::GetPowerManager()
{
  return g_application.m_ServiceManager->GetPowerManager();
}

CWeatherManager& CServiceBroker::GetWeatherManager()
{
  return g_application.m_ServiceManager->GetWeatherManager();
}

CPlayerCoreFactory& CServiceBroker::GetPlayerCoreFactory()
{
  return g_application.m_ServiceManager->GetPlayerCoreFactory();
}

CDatabaseManager& CServiceBroker::GetDatabaseManager()
{
  return g_application.m_ServiceManager->GetDatabaseManager();
}

CEventLog& CServiceBroker::GetEventLog()
{
  return m_pSettingsComponent->GetProfileManager()->GetEventLog();
}

CGUIComponent* CServiceBroker::m_pGUI = nullptr;

CGUIComponent* CServiceBroker::GetGUI()
{
  return m_pGUI;
}

void CServiceBroker::RegisterGUI(CGUIComponent *gui)
{
  m_pGUI = gui;
}

void CServiceBroker::UnregisterGUI()
{
  m_pGUI = nullptr;
}

// audio
IAE* CServiceBroker::m_pActiveAE = nullptr;
IAE* CServiceBroker::GetActiveAE()
{
  return m_pActiveAE;
}
void CServiceBroker::RegisterAE(IAE *ae)
{
  m_pActiveAE = ae;
}
void CServiceBroker::UnregisterAE()
{
  m_pActiveAE = nullptr;
}

// application
std::shared_ptr<CAppInboundProtocol> CServiceBroker::m_pAppPort;
std::shared_ptr<CAppInboundProtocol> CServiceBroker::GetAppPort()
{
  return m_pAppPort;
}
void CServiceBroker::RegisterAppPort(std::shared_ptr<CAppInboundProtocol> port)
{
  m_pAppPort = port;
}
void CServiceBroker::UnregisterAppPort()
{
  m_pAppPort.reset();
}
