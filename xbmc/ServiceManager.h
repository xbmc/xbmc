/*
 *  Copyright (C) 2005-2026 Team Kodi
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

namespace KODI::PLAYLIST
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
namespace UTILS::I18N
{
class CSubTagRegistryManager;
} // namespace UTILS::I18N
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

  /*!
   * \brief Initialize the stage 1 boot services
   *
   * Stage 1 sets up the platform bootstrap layer and the earliest core
   * services that do not depend on the GUI or user profiles.
   *
   * Subsystems initialized in stage 1 include:
   *
   *   - Platform startup glue via \ref CPlatform::InitStageOne
   *   - Python script registration when Python support is enabled
   *   - Playlist playback coordination
   *   - Slideshow delegation
   *   - Network backend discovery
   */
  bool InitStageOne();

  /*!
   * \brief Initialize the stage 2 boot services
   *
   * Stage 2 brings up the main shared subsystems used by the GUI and
   * runtime services after stage 1 has completed.
   *
   * Subsystems initialized in stage 2 include:
   *
   *   - Database manager bootstrap and addon infrastructure
   *   - Service addon manager, repository, VFS, and binary addon infrastructure
   *   - PVR, favourites, context menu, and data cache services
   *   - Input and game controller support, plus peripheral object construction
   *   - Retro player render and retro engine service construction
   *   - File extension, power, weather, media, and locale helpers
   *   - Subtag registry manager initialization
   *   - Optional optical media detection object creation and SMB discovery backends
   *   - Platform stage 2 initialization
   */
  bool InitStageTwo(const std::string& profilesUserDataFolder);

  /*!
   * \brief Initialize the stage 3 boot services
   *
   * Stage 3 runs after the WindowManager has initialized, so the GUI is
   * already available when these late services start.
   *
   * Subsystems initialized in stage 3 include:
   *
   *   - Optical media detection thread startup when supported
   *   - Peripheral activation after localized strings are loaded
   *   - Game services and retro engine service wiring
   *   - Context menu activation
   *   - Conditional PVR startup only when not using the login screen
   *   - Player core factory creation
   *   - Platform stage 3 initialization
   */
  bool InitStageThree(const std::shared_ptr<CProfileManager>& profileManager);

  void DeinitTesting();

  /*!
   * \brief Shut down the stage 3 services in reverse startup order
   */
  void DeinitStageThree();

  /*!
   * \brief Shut down the stage 2 services in reverse startup order
   */
  void DeinitStageTwo();

  /*!
   * \brief Shut down the stage 1 services in reverse startup order
   */
  void DeinitStageOne();

  ADDON::CAddonMgr& GetAddonMgr();
  ADDON::CBinaryAddonManager& GetBinaryAddonManager();
  ADDON::CBinaryAddonCache& GetBinaryAddonCache();
  KODI::ADDONS::CExtsMimeSupportList& GetExtsMimeSupportList();
  ADDON::CVFSAddonCache& GetVFSAddonCache();
  ADDON::CServiceAddonManager& GetServiceAddons();
  ADDON::CRepositoryUpdater& GetRepositoryUpdater();
  CNetworkBase& GetNetwork();
#ifdef HAS_PYTHON
  XBPython& GetXBPython();
#endif
#if defined(HAS_FILESYSTEM_SMB)
  WSDiscovery::IWSDiscovery& GetWSDiscovery();
#endif
  PVR::CPVRManager& GetPVRManager();
  CContextMenuManager& GetContextMenuManager();
  CDataCacheCore& GetDataCacheCore();
  /*!
   * \brief Get the platform object
   *
   * Safe to call after InitStageOne() completes.
   */
  CPlatform& GetPlatform();
  KODI::GAME::CControllerManager& GetGameControllerManager();
  KODI::GAME::CGameServices& GetGameServices();
  KODI::RETRO::CGUIGameRenderManager& GetGameRenderManager();
  PERIPHERALS::CPeripherals& GetPeripherals();

  KODI::PLAYLIST::CPlayListPlayer& GetPlaylistPlayer();
  CSlideShowDelegator& GetSlideShowDelegator();
  int init_level = 0;

  CFavouritesService& GetFavouritesService();
  CInputManager& GetInputManager();
  CFileExtensionProvider& GetFileExtensionProvider();

  CPowerManager& GetPowerManager();

  CWeatherManager& GetWeatherManager();

  CPlayerCoreFactory& GetPlayerCoreFactory();

  CDatabaseManager& GetDatabaseManager();

  CMediaManager& GetMediaManager();

#if !defined(TARGET_WINDOWS) && defined(HAS_OPTICAL_DRIVE)
  MEDIA_DETECT::CDetectDVDMedia& GetDetectDVDMedia();
#endif

  KODI::UTILS::I18N::CSubTagRegistryManager& GetSubTagRegistryManager();

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
  std::unique_ptr<KODI::PLAYLIST::CPlayListPlayer> m_playlistPlayer;
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
  std::unique_ptr<KODI::UTILS::I18N::CSubTagRegistryManager> m_subTagRegistryManager;
};
