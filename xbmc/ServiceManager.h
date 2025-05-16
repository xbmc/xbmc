/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "platform/Platform.h"

#include <memory>

namespace ADDON
{
class CAddonMgr;
class CBinaryAddonManager;
class CBinaryAddonCache;
class CVFSAddonCache;
class CServiceAddonManager;
class CRepositoryUpdater;
} // namespace ADDON

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
#if defined(HAS_FILESYSTEM_SMB)
namespace WSDiscovery
{
class IWSDiscovery;
}
#endif
class CDataCacheCore;
class CFavouritesService;
class CNetworkBase;
class CWinSystemBase;
class CPowerManager;
class CWeatherManager;
class CSlideShowDelegator;

namespace KODI
{
namespace ADDONS
{
class CExtsMimeSupportList;
}

namespace GAME
{
class CControllerManager;
class CGameServices;
} // namespace GAME

namespace RETRO
{
class CGUIGameRenderManager;
}
} // namespace KODI

namespace MEDIA_DETECT
{
class CDetectDVDMedia;
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
class CMediaManager;

class CServiceManager
{
public:
  CServiceManager();
  ~CServiceManager();

  bool InitForTesting();
  bool InitStageOne();
  bool InitStageTwo(const std::string& profilesUserDataFolder);
  bool InitStageThree(const std::shared_ptr<CProfileManager>& profileManager);
  void DeinitTesting();
  void DeinitStageThree();
  void DeinitStageTwo();
  void DeinitStageOne();

  ADDON::CAddonMgr& GetAddonMgr() const;
  ADDON::CBinaryAddonManager& GetBinaryAddonManager() const;
  ADDON::CBinaryAddonCache& GetBinaryAddonCache() const;
  KODI::ADDONS::CExtsMimeSupportList& GetExtsMimeSupportList() const;
  ADDON::CVFSAddonCache& GetVFSAddonCache() const;
  ADDON::CServiceAddonManager& GetServiceAddons() const;
  ADDON::CRepositoryUpdater& GetRepositoryUpdater() const;
  CNetworkBase& GetNetwork() const;
#ifdef HAS_PYTHON
  XBPython& GetXBPython() const;
#endif
#if defined(HAS_FILESYSTEM_SMB)
  WSDiscovery::IWSDiscovery& GetWSDiscovery() const;
#endif
  PVR::CPVRManager& GetPVRManager() const;
  CContextMenuManager& GetContextMenuManager() const;
  CDataCacheCore& GetDataCacheCore() const;
  /**\brief Get the platform object. This is save to be called after Init1() was called
   */
  CPlatform& GetPlatform() const;
  KODI::GAME::CControllerManager& GetGameControllerManager() const;
  KODI::GAME::CGameServices& GetGameServices() const;
  KODI::RETRO::CGUIGameRenderManager& GetGameRenderManager() const;
  PERIPHERALS::CPeripherals& GetPeripherals() const;

  PLAYLIST::CPlayListPlayer& GetPlaylistPlayer() const;
  CSlideShowDelegator& GetSlideShowDelegator() const;
  int init_level = 0;

  CFavouritesService& GetFavouritesService() const;
  CInputManager& GetInputManager() const;
  CFileExtensionProvider& GetFileExtensionProvider() const;

  CPowerManager& GetPowerManager() const;

  CWeatherManager& GetWeatherManager() const;

  CPlayerCoreFactory& GetPlayerCoreFactory() const;

  CDatabaseManager& GetDatabaseManager() const;

  CMediaManager& GetMediaManager() const;

#if !defined(TARGET_WINDOWS) && defined(HAS_OPTICAL_DRIVE)
  MEDIA_DETECT::CDetectDVDMedia& GetDetectDVDMedia() const;
#endif

protected:
  std::unique_ptr<ADDON::CAddonMgr> m_addonMgr;
  std::unique_ptr<ADDON::CBinaryAddonManager> m_binaryAddonManager;
  std::unique_ptr<ADDON::CBinaryAddonCache> m_binaryAddonCache;
  std::unique_ptr<KODI::ADDONS::CExtsMimeSupportList> m_extsMimeSupportList;
  std::unique_ptr<ADDON::CVFSAddonCache> m_vfsAddonCache;
  std::unique_ptr<ADDON::CServiceAddonManager> m_serviceAddons;
  std::unique_ptr<ADDON::CRepositoryUpdater> m_repositoryUpdater;
#if defined(HAS_FILESYSTEM_SMB)
  std::unique_ptr<WSDiscovery::IWSDiscovery> m_WSDiscovery;
#endif
#ifdef HAS_PYTHON
  std::unique_ptr<XBPython> m_XBPython;
#endif
  std::unique_ptr<PVR::CPVRManager> m_PVRManager;
  std::unique_ptr<CContextMenuManager> m_contextMenuManager;
  std::unique_ptr<CDataCacheCore> m_dataCacheCore;
  std::unique_ptr<CPlatform> m_Platform;
  std::unique_ptr<PLAYLIST::CPlayListPlayer> m_playlistPlayer;
  std::unique_ptr<KODI::GAME::CControllerManager> m_gameControllerManager;
  std::unique_ptr<KODI::GAME::CGameServices> m_gameServices;
  std::unique_ptr<KODI::RETRO::CGUIGameRenderManager> m_gameRenderManager;
  std::unique_ptr<PERIPHERALS::CPeripherals> m_peripherals;
  std::unique_ptr<CFavouritesService> m_favouritesService;
  std::unique_ptr<CInputManager> m_inputManager;
  std::unique_ptr<CFileExtensionProvider> m_fileExtensionProvider;
  std::unique_ptr<CNetworkBase> m_network;
  std::unique_ptr<CPowerManager> m_powerManager;
  std::unique_ptr<CWeatherManager> m_weatherManager;
  std::unique_ptr<CPlayerCoreFactory> m_playerCoreFactory;
  std::unique_ptr<CDatabaseManager> m_databaseManager;
  std::unique_ptr<CMediaManager> m_mediaManager;
#if !defined(TARGET_WINDOWS) && defined(HAS_OPTICAL_DRIVE)
  std::unique_ptr<MEDIA_DETECT::CDetectDVDMedia> m_DetectDVDType;
#endif
  std::unique_ptr<CSlideShowDelegator> m_slideShowDelegator;
};
