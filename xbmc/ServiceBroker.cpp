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
#include "utils/log.h"
#include "windowing/WinSystem.h"

using namespace KODI;


CServiceBroker::CServiceBroker() :
    m_pGUI(nullptr),
    m_pWinSystem(nullptr),
    m_pActiveAE(nullptr),
    m_pSettingsComponent(nullptr),
    m_decoderFilterManager(nullptr)
{
}

CServiceBroker::~CServiceBroker()
{
}

CLog& CServiceBroker::GetLogging()
{
  return *(g_serviceBroker.m_logging);
}

void CServiceBroker::CreateLogging()
{
  g_serviceBroker.m_logging = std::make_unique<CLog>();
}

// announcement
std::shared_ptr<ANNOUNCEMENT::CAnnouncementManager> CServiceBroker::GetAnnouncementManager()
{
  return g_serviceBroker.m_pAnnouncementManager;
}
void CServiceBroker::RegisterAnnouncementManager(std::shared_ptr<ANNOUNCEMENT::CAnnouncementManager> port)
{
  g_serviceBroker.m_pAnnouncementManager = port;
}

void CServiceBroker::UnregisterAnnouncementManager()
{
  g_serviceBroker.m_pAnnouncementManager.reset();
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

void CServiceBroker::RegisterSettingsComponent(CSettingsComponent *settings)
{
  g_serviceBroker.m_pSettingsComponent = settings;
}

void CServiceBroker::UnregisterSettingsComponent()
{
  g_serviceBroker.m_pSettingsComponent = nullptr;
}

CSettingsComponent* CServiceBroker::GetSettingsComponent()
{
  return g_serviceBroker.m_pSettingsComponent;
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

CWinSystemBase* CServiceBroker::GetWinSystem()
{
  return g_serviceBroker.m_pWinSystem;
}

void CServiceBroker::RegisterWinSystem(CWinSystemBase *winsystem)
{
  g_serviceBroker.m_pWinSystem = winsystem;
}

void CServiceBroker::UnregisterWinSystem()
{
  g_serviceBroker.m_pWinSystem = nullptr;
}

CRenderSystemBase* CServiceBroker::GetRenderSystem()
{
  if (g_serviceBroker.m_pWinSystem)
    return g_serviceBroker.m_pWinSystem->GetRenderSystem();

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
  return g_serviceBroker.m_pSettingsComponent->GetProfileManager()->GetEventLog();
}

CMediaManager& CServiceBroker::GetMediaManager()
{
  return g_application.m_ServiceManager->GetMediaManager();
}

CGUIComponent* CServiceBroker::GetGUI()
{
  return g_serviceBroker.m_pGUI;
}

void CServiceBroker::RegisterGUI(CGUIComponent *gui)
{
  g_serviceBroker.m_pGUI = gui;
}

void CServiceBroker::UnregisterGUI()
{
  g_serviceBroker.m_pGUI = nullptr;
}

// audio
IAE* CServiceBroker::GetActiveAE()
{
  return g_serviceBroker.m_pActiveAE;
}
void CServiceBroker::RegisterAE(IAE *ae)
{
  g_serviceBroker.m_pActiveAE = ae;
}
void CServiceBroker::UnregisterAE()
{
  g_serviceBroker.m_pActiveAE = nullptr;
}

// application
std::shared_ptr<CAppInboundProtocol> CServiceBroker::GetAppPort()
{
  return g_serviceBroker.m_pAppPort;
}
void CServiceBroker::RegisterAppPort(std::shared_ptr<CAppInboundProtocol> port)
{
  g_serviceBroker.m_pAppPort = port;
}
void CServiceBroker::UnregisterAppPort()
{
  g_serviceBroker.m_pAppPort.reset();
}

void CServiceBroker::RegisterDecoderFilterManager(CDecoderFilterManager* manager)
{
  g_serviceBroker.m_decoderFilterManager = manager;
}

CDecoderFilterManager* CServiceBroker::GetDecoderFilterManager()
{
  return g_serviceBroker.m_decoderFilterManager;
}

std::shared_ptr<CCPUInfo> CServiceBroker::GetCPUInfo()
{
  return g_serviceBroker.m_cpuInfo;
}

void CServiceBroker::RegisterCPUInfo(std::shared_ptr<CCPUInfo> cpuInfo)
{
  g_serviceBroker.m_cpuInfo = cpuInfo;
}

void CServiceBroker::UnregisterCPUInfo()
{
  g_serviceBroker.m_cpuInfo.reset();
}
