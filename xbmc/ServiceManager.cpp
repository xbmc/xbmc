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

#include "ServiceManager.h"
#include "addons/BinaryAddonCache.h"
#include "addons/VFSEntry.h"
#include "addons/binary-addons/BinaryAddonManager.h"
#include "ContextMenuManager.h"
#include "cores/AudioEngine/Engines/ActiveAE/ActiveAE.h"
#include "cores/DataCacheCore.h"
#include "cores/playercorefactory/PlayerCoreFactory.h"
#include "cores/RetroPlayer/rendering/GUIGameRenderManager.h"
#include "favourites/FavouritesService.h"
#include "games/controllers/ControllerManager.h"
#include "games/GameServices.h"
#include "peripherals/Peripherals.h"
#include "PlayListPlayer.h"
#include "profiles/ProfilesManager.h"
#include "utils/log.h"
#include "input/InputManager.h"
#include "interfaces/AnnouncementManager.h"
#include "interfaces/generic/ScriptInvocationManager.h"
#include "interfaces/python/XBPython.h"
#include "pvr/PVRManager.h"
#include "network/Network.h"
#include "settings/Settings.h"
#include "utils/FileExtensionProvider.h"
#include "windowing/WinSystem.h"
#include "powermanagement/PowerManager.h"
#include "weather/WeatherManager.h"

using namespace KODI;

CServiceManager::CServiceManager()
{
}

CServiceManager::~CServiceManager()
{
  if (init_level > 2)
    DeinitStageThree();
  if (init_level > 1)
    DeinitStageTwo();
  if (init_level > 0)
    DeinitStageOne();
}

bool CServiceManager::InitForTesting()
{
  m_settings.reset(new CSettings());
  m_network.reset(SetupNetwork());
  m_fileExtensionProvider.reset(new CFileExtensionProvider());

  m_addonMgr.reset(new ADDON::CAddonMgr());
  if (!m_addonMgr->Init())
  {
    CLog::Log(LOGFATAL, "CServiceManager::%s: Unable to start CAddonMgr", __FUNCTION__);
    return false;
  }

  init_level = 1;
  return true;
}

void CServiceManager::DeinitTesting()
{
  init_level = 0;
  m_addonMgr.reset();
  m_fileExtensionProvider.reset();
  m_network.reset();
  m_settings.reset();
}

bool CServiceManager::InitStageOne()
{
  m_announcementManager.reset(new ANNOUNCEMENT::CAnnouncementManager());
  m_announcementManager->Start();

#ifdef HAS_PYTHON
  m_XBPython.reset(new XBPython());
  CScriptInvocationManager::GetInstance().RegisterLanguageInvocationHandler(m_XBPython.get(), ".py");
#endif

  m_playlistPlayer.reset(new PLAYLIST::CPlayListPlayer());

  m_settings.reset(new CSettings());
  m_network.reset(SetupNetwork());

  init_level = 1;
  return true;
}

bool CServiceManager::InitStageTwo(const CAppParamParser &params)
{
  m_Platform.reset(CPlatform::CreateInstance());
  m_Platform->Init();

  m_binaryAddonManager.reset(new ADDON::CBinaryAddonManager()); /* Need to constructed before, GetRunningInstance() of binary CAddonDll need to call them */
  m_addonMgr.reset(new ADDON::CAddonMgr());
  if (!m_addonMgr->Init())
  {
    CLog::Log(LOGFATAL, "CServiceManager::%s: Unable to start CAddonMgr", __FUNCTION__);
    return false;
  }

  if (!m_binaryAddonManager->Init())
  {
    CLog::Log(LOGFATAL, "CServiceManager::%s: Unable to initialize CBinaryAddonManager", __FUNCTION__);
    return false;
  }

  m_repositoryUpdater.reset(new ADDON::CRepositoryUpdater(*m_addonMgr));

  m_vfsAddonCache.reset(new ADDON::CVFSAddonCache());
  m_vfsAddonCache->Init();

  m_PVRManager.reset(new PVR::CPVRManager());

  m_dataCacheCore.reset(new CDataCacheCore());

  m_binaryAddonCache.reset( new ADDON::CBinaryAddonCache());
  m_binaryAddonCache->Init();

  m_favouritesService.reset(new CFavouritesService(CProfilesManager::GetInstance().GetProfileUserDataFolder()));

  m_serviceAddons.reset(new ADDON::CServiceAddonManager(*m_addonMgr));

  m_contextMenuManager.reset(new CContextMenuManager(*m_addonMgr.get()));

  m_gameControllerManager.reset(new GAME::CControllerManager);
  m_inputManager.reset(new CInputManager(params));
  m_inputManager->InitializeInputs();

  m_peripherals.reset(new PERIPHERALS::CPeripherals(*m_announcementManager,
                                                    *m_inputManager,
                                                    *m_gameControllerManager));

  m_gameRenderManager.reset(new RETRO::CGUIGameRenderManager);

  m_fileExtensionProvider.reset(new CFileExtensionProvider());

  m_powerManager.reset(new CPowerManager());
  m_powerManager->Initialize();
  m_powerManager->SetDefaults();

  m_weatherManager.reset(new CWeatherManager());

  init_level = 2;
  return true;
}

bool CServiceManager::CreateAudioEngine()
{
  m_ActiveAE.reset(new ActiveAE::CActiveAE());

  return true;
}

bool CServiceManager::DestroyAudioEngine()
{
  if (m_ActiveAE)
  {
    m_ActiveAE->Shutdown();
    m_ActiveAE.reset();
  }

  return true;
}

bool CServiceManager::StartAudioEngine()
{
  if (!m_ActiveAE)
  {
    CLog::Log(LOGFATAL, "CServiceManager::%s: Unable to start ActiveAE", __FUNCTION__);
    return false;
  }

  return m_ActiveAE->Initialize();
}

// stage 3 is called after successful initialization of WindowManager
bool CServiceManager::InitStageThree()
{
  // Peripherals depends on strings being loaded before stage 3
  m_peripherals->Initialise();

  m_gameServices.reset(new GAME::CGameServices(*m_gameControllerManager,
    *m_gameRenderManager,
    *m_settings,
    *m_peripherals));

  m_contextMenuManager->Init();
  m_PVRManager->Init();

  m_playerCoreFactory.reset(new CPlayerCoreFactory(*m_settings,
                                                   CProfilesManager::GetInstance()));

  init_level = 3;
  return true;
}

void CServiceManager::DeinitStageThree()
{
  init_level = 2;

  m_playerCoreFactory.reset();
  m_PVRManager->Deinit();
  m_contextMenuManager->Deinit();
  m_gameServices.reset();
  m_peripherals->Clear();
}

void CServiceManager::DeinitStageTwo()
{
  init_level = 1;

  m_weatherManager.reset();
  m_powerManager.reset();
  m_fileExtensionProvider.reset();
  m_gameRenderManager.reset();
  m_peripherals.reset();
  m_inputManager.reset();
  m_gameControllerManager.reset();
  m_contextMenuManager.reset();
  m_serviceAddons.reset();
  m_favouritesService.reset();
  m_binaryAddonCache.reset();
  m_dataCacheCore.reset();
  m_PVRManager.reset();
  m_vfsAddonCache.reset();
  m_repositoryUpdater.reset();
  m_binaryAddonManager.reset();
  m_addonMgr.reset();
  m_Platform.reset();
}

void CServiceManager::DeinitStageOne()
{
  init_level = 0;

  m_network.reset();
  m_settings.reset();
  m_playlistPlayer.reset();
#ifdef HAS_PYTHON
  CScriptInvocationManager::GetInstance().UnregisterLanguageInvocationHandler(m_XBPython.get());
  m_XBPython.reset();
#endif
  m_announcementManager.reset();
}

ADDON::CAddonMgr &CServiceManager::GetAddonMgr()
{
  return *m_addonMgr.get();
}

ADDON::CBinaryAddonCache &CServiceManager::GetBinaryAddonCache()
{
  return *m_binaryAddonCache.get();
}

ADDON::CBinaryAddonManager &CServiceManager::GetBinaryAddonManager()
{
  return *m_binaryAddonManager.get();
}

ADDON::CVFSAddonCache &CServiceManager::GetVFSAddonCache()
{
  return *m_vfsAddonCache.get();
}

ADDON::CServiceAddonManager &CServiceManager::GetServiceAddons()
{
  return *m_serviceAddons;
}

ADDON::CRepositoryUpdater &CServiceManager::GetRepositoryUpdater()
{
  return *m_repositoryUpdater;
}

ANNOUNCEMENT::CAnnouncementManager& CServiceManager::GetAnnouncementManager()
{
  return *m_announcementManager;
}

#ifdef HAS_PYTHON
XBPython& CServiceManager::GetXBPython()
{
  return *m_XBPython;
}
#endif

PVR::CPVRManager& CServiceManager::GetPVRManager()
{
  return *m_PVRManager;
}

IAE& CServiceManager::GetActiveAE()
{
  ActiveAE::CActiveAE& ae = *m_ActiveAE;
  return ae;
}

CContextMenuManager& CServiceManager::GetContextMenuManager()
{
  return *m_contextMenuManager;
}

CDataCacheCore& CServiceManager::GetDataCacheCore()
{
  return *m_dataCacheCore;
}

CPlatform& CServiceManager::GetPlatform()
{
  return *m_Platform;
}

PLAYLIST::CPlayListPlayer& CServiceManager::GetPlaylistPlayer()
{
  return *m_playlistPlayer;
}

CSettings& CServiceManager::GetSettings()
{
  return *m_settings;
}

GAME::CControllerManager& CServiceManager::GetGameControllerManager()
{
  return *m_gameControllerManager;
}

GAME::CGameServices& CServiceManager::GetGameServices()
{
  return *m_gameServices;
}

KODI::RETRO::CGUIGameRenderManager& CServiceManager::GetGameRenderManager()
{
  return *m_gameRenderManager;
}

PERIPHERALS::CPeripherals& CServiceManager::GetPeripherals()
{
  return *m_peripherals;
}

CFavouritesService& CServiceManager::GetFavouritesService()
{
  return *m_favouritesService;
}

CInputManager& CServiceManager::GetInputManager()
{
  return *m_inputManager;
}

CFileExtensionProvider& CServiceManager::GetFileExtensionProvider()
{
  return *m_fileExtensionProvider;
}

CWinSystemBase &CServiceManager::GetWinSystem()
{
  return *m_winSystem.get();
}

void CServiceManager::SetWinSystem(std::unique_ptr<CWinSystemBase> winSystem)
{
  m_winSystem = std::move(winSystem);
}

CPowerManager &CServiceManager::GetPowerManager()
{
  return *m_powerManager;
}

// deleters for unique_ptr
void CServiceManager::delete_dataCacheCore::operator()(CDataCacheCore *p) const
{
  delete p;
}

void CServiceManager::delete_contextMenuManager::operator()(CContextMenuManager *p) const
{
  delete p;
}

void CServiceManager::delete_activeAE::operator()(ActiveAE::CActiveAE *p) const
{
  delete p;
}

void CServiceManager::delete_favouritesService::operator()(CFavouritesService *p) const
{
  delete p;
}

CNetwork* CServiceManager::SetupNetwork() const
{
#if defined(TARGET_ANDROID)
  return new CNetworkAndroid();
#elif defined(HAS_LINUX_NETWORK)
  return new CNetworkLinux();
#elif defined(HAS_WIN32_NETWORK)
  return new CNetworkWin32();
#elif defined(HAS_WIN10_NETWORK)
  return new CNetworkWin10();
#else
  return new CNetwork();
#endif
}

CNetwork& CServiceManager::GetNetwork()
{
  return *m_network;
}

CWeatherManager& CServiceManager::GetWeatherManager()
{
  return *m_weatherManager;
}

CPlayerCoreFactory &CServiceManager::GetPlayerCoreFactory()
{
  return *m_playerCoreFactory;
}
