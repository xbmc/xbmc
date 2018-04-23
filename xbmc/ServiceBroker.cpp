/*
 *      Copyright (C) 2005-2016 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "ServiceBroker.h"
#include "Application.h"
#include "windowing/WinSystem.h"

using namespace KODI;

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

ANNOUNCEMENT::CAnnouncementManager &CServiceBroker::GetAnnouncementManager()
{
  return g_application.m_ServiceManager->GetAnnouncementManager();
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

CSettings& CServiceBroker::GetSettings()
{
  return g_application.m_ServiceManager->GetSettings();
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
  return g_application.m_ServiceManager->init_level > 1;
}

bool CServiceBroker::IsServiceManagerUp()
{
  return g_application.m_ServiceManager->init_level == 3;
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

CProfilesManager& CServiceBroker::GetProfileManager()
{
  return g_application.m_ServiceManager->GetProfileManager();
}

CEventLog& CServiceBroker::GetEventLog()
{
  return g_application.m_ServiceManager->GetEventLog();
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
