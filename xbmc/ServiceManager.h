/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>
#include "platform/Platform.h"

class CAppParamParser;

namespace ADDON
{
  class CAddonMgr;
  class CBinaryAddonManager;
  class CBinaryAddonCache;
  class CVFSAddonCache;
  class CServiceAddonManager;
  class CRepositoryUpdater;
}

namespace PVR
{
  class CPVRManager;
}

namespace PLAYLIST
{
  class CPlayListPlayer;
}

class CContextMenuManager;
#ifdef HAS_PYTHON
class XBPython;
#endif
class CDataCacheCore;
class CFavouritesService;
class CNetworkBase;
class CWinSystemBase;
class CPowerManager;
class CWeatherManager;

namespace KODI
{
namespace GAME
{
  class CControllerManager;
  class CGameServices;
}

namespace RETRO
{
  class CGUIGameRenderManager;
}
}

namespace PERIPHERALS
{
  class CPeripherals;
}

class CInputManager;
class CFileExtensionProvider;
class CPlayerCoreFactory;
class CDatabaseManager;
class CProfileManager;
class CEventLog;

class CServiceManager
{
public:
  CServiceManager();
  ~CServiceManager();

  bool InitForTesting();
  bool InitStageOne();
  bool InitStageTwo(const CAppParamParser &params, const std::string& profilesUserDataFolder);
  bool InitStageThree(const std::shared_ptr<CProfileManager>& profileManager);
  void DeinitTesting();
  void DeinitStageThree();
  void DeinitStageTwo();
  void DeinitStageOne();

  ADDON::CAddonMgr& GetAddonMgr();
  ADDON::CBinaryAddonManager& GetBinaryAddonManager();
  ADDON::CBinaryAddonCache& GetBinaryAddonCache();
  ADDON::CVFSAddonCache& GetVFSAddonCache();
  ADDON::CServiceAddonManager& GetServiceAddons();
  ADDON::CRepositoryUpdater& GetRepositoryUpdater();
  CNetworkBase& GetNetwork();
#ifdef HAS_PYTHON
  XBPython& GetXBPython();
#endif
  PVR::CPVRManager& GetPVRManager();
  CContextMenuManager& GetContextMenuManager();
  CDataCacheCore& GetDataCacheCore();
  /**\brief Get the platform object. This is save to be called after Init1() was called
   */
  CPlatform& GetPlatform();
  KODI::GAME::CControllerManager& GetGameControllerManager();
  KODI::GAME::CGameServices& GetGameServices();
  KODI::RETRO::CGUIGameRenderManager& GetGameRenderManager();
  PERIPHERALS::CPeripherals& GetPeripherals();

  PLAYLIST::CPlayListPlayer& GetPlaylistPlayer();
  int init_level = 0;

  CFavouritesService& GetFavouritesService();
  CInputManager &GetInputManager();
  CFileExtensionProvider &GetFileExtensionProvider();

  CPowerManager &GetPowerManager();

  CWeatherManager &GetWeatherManager();

  CPlayerCoreFactory &GetPlayerCoreFactory();

  CDatabaseManager &GetDatabaseManager();

protected:
  struct delete_dataCacheCore
  {
    void operator()(CDataCacheCore *p) const;
  };

  struct delete_contextMenuManager
  {
    void operator()(CContextMenuManager *p) const;
  };

  struct delete_favouritesService
  {
    void operator()(CFavouritesService *p) const;
  };

  std::unique_ptr<ADDON::CAddonMgr> m_addonMgr;
  std::unique_ptr<ADDON::CBinaryAddonManager> m_binaryAddonManager;
  std::unique_ptr<ADDON::CBinaryAddonCache> m_binaryAddonCache;
  std::unique_ptr<ADDON::CVFSAddonCache> m_vfsAddonCache;
  std::unique_ptr<ADDON::CServiceAddonManager> m_serviceAddons;
  std::unique_ptr<ADDON::CRepositoryUpdater> m_repositoryUpdater;
#ifdef HAS_PYTHON
  std::unique_ptr<XBPython> m_XBPython;
#endif
  std::unique_ptr<PVR::CPVRManager> m_PVRManager;
  std::unique_ptr<CContextMenuManager, delete_contextMenuManager> m_contextMenuManager;
  std::unique_ptr<CDataCacheCore, delete_dataCacheCore> m_dataCacheCore;
  std::unique_ptr<CPlatform> m_Platform;
  std::unique_ptr<PLAYLIST::CPlayListPlayer> m_playlistPlayer;
  std::unique_ptr<KODI::GAME::CControllerManager> m_gameControllerManager;
  std::unique_ptr<KODI::GAME::CGameServices> m_gameServices;
  std::unique_ptr<KODI::RETRO::CGUIGameRenderManager> m_gameRenderManager;
  std::unique_ptr<PERIPHERALS::CPeripherals> m_peripherals;
  std::unique_ptr<CFavouritesService, delete_favouritesService> m_favouritesService;
  std::unique_ptr<CInputManager> m_inputManager;
  std::unique_ptr<CFileExtensionProvider> m_fileExtensionProvider;
  std::unique_ptr<CNetworkBase> m_network;
  std::unique_ptr<CPowerManager> m_powerManager;
  std::unique_ptr<CWeatherManager> m_weatherManager;
  std::unique_ptr<CPlayerCoreFactory> m_playerCoreFactory;
  std::unique_ptr<CDatabaseManager> m_databaseManager;
};
