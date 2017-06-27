/*
 *      Copyright (C) 2005-2016 Team XBMC
 *      http://xbmc.org
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
#include "favourites/FavouritesService.h"
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
#include "settings/Settings.h"

using namespace KODI;

CServiceManager::CServiceManager() :
  m_gameServices(new GAME::CGameServices),
  m_peripherals(new PERIPHERALS::CPeripherals),
  m_inputManager(new CInputManager)
{
}

CServiceManager::~CServiceManager()
{
}

bool CServiceManager::Init1()
{
  m_announcementManager.reset(new ANNOUNCEMENT::CAnnouncementManager());
  m_announcementManager->Start();

#ifdef HAS_PYTHON
  m_XBPython.reset(new XBPython());
  CScriptInvocationManager::GetInstance().RegisterLanguageInvocationHandler(m_XBPython.get(), ".py");
#endif

  m_Platform.reset(CPlatform::CreateInstance());

  m_playlistPlayer.reset(new PLAYLIST::CPlayListPlayer());

  m_settings.reset(new CSettings());

  init_level = 1;
  return true;
}

bool CServiceManager::Init2()
{
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

  m_PVRManager.reset(new PVR::CPVRManager());
  m_dataCacheCore.reset(new CDataCacheCore());

  m_binaryAddonCache.reset( new ADDON::CBinaryAddonCache());
  m_binaryAddonCache->Init();

  m_vfsAddonCache.reset(new ADDON::CVFSAddonCache());
  m_vfsAddonCache->Init();

  m_favouritesService.reset(new CFavouritesService(CProfilesManager::GetInstance().GetProfileUserDataFolder()));
  m_contextMenuManager.reset(new CContextMenuManager(*m_addonMgr.get()));
  m_serviceAddons.reset(new ADDON::CServiceAddonManager(*m_addonMgr));

  m_inputManager->InitializeInputs();

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

bool CServiceManager::Init3()
{
  m_peripherals->Initialise();
  m_PVRManager->Init();
  m_contextMenuManager->Init();
  m_gameServices->Init(*m_peripherals);

  init_level = 3;
  return true;
}

void CServiceManager::Deinit()
{
  m_serviceAddons.reset();
  m_gameServices->Deinit();
  m_peripherals.reset();
  //m_inputManager->Deinitialize(); //! @todo
  m_contextMenuManager.reset();
  m_favouritesService.reset();
  m_vfsAddonCache.reset();
  m_binaryAddonCache.reset();
  if (m_PVRManager)
    m_PVRManager->Deinit();
  m_PVRManager.reset();
  m_binaryAddonManager.reset();
  m_addonMgr.reset();
#ifdef HAS_PYTHON
  CScriptInvocationManager::GetInstance().UnregisterLanguageInvocationHandler(m_XBPython.get());
  m_XBPython.reset();
#endif
  m_announcementManager.reset();
  init_level = 0;
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

GAME::CGameServices& CServiceManager::GetGameServices()
{
  return *m_gameServices;
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
