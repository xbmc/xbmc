/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>

namespace ADDON {
class CAddonMgr;
class CBinaryAddonManager;
class CBinaryAddonCache;
class CVFSAddonCache;
class CServiceAddonManager;
class CRepositoryUpdater;
}

namespace ANNOUNCEMENT
{
  class CAnnouncementManager;
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
class XBPython;
class CDataCacheCore;
class IAE;
class CFavouritesService;
class CInputManager;
class CFileExtensionProvider;
class CNetworkBase;
class CWinSystemBase;
class CRenderSystemBase;
class CPowerManager;
class CWeatherManager;
class CPlayerCoreFactory;
class CDatabaseManager;
class CEventLog;
class CGUIComponent;
class CAppInboundProtocol;
class CSettingsComponent;

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

class CServiceBroker
{
public:
  static std::shared_ptr<ANNOUNCEMENT::CAnnouncementManager> GetAnnouncementManager();
  static void RegisterAnnouncementManager(std::shared_ptr<ANNOUNCEMENT::CAnnouncementManager> announcementManager);
  static void UnregisterAnnouncementManager();

  static ADDON::CAddonMgr &GetAddonMgr();
  static ADDON::CBinaryAddonManager &GetBinaryAddonManager();
  static ADDON::CBinaryAddonCache &GetBinaryAddonCache();
  static ADDON::CVFSAddonCache &GetVFSAddonCache();
  static XBPython &GetXBPython();
  static PVR::CPVRManager &GetPVRManager();
  static CContextMenuManager& GetContextMenuManager();
  static CDataCacheCore& GetDataCacheCore();
  static PLAYLIST::CPlayListPlayer& GetPlaylistPlayer();
  static KODI::GAME::CControllerManager& GetGameControllerManager();
  static KODI::GAME::CGameServices& GetGameServices();
  static KODI::RETRO::CGUIGameRenderManager& GetGameRenderManager();
  static PERIPHERALS::CPeripherals& GetPeripherals();
  static CFavouritesService& GetFavouritesService();
  static ADDON::CServiceAddonManager& GetServiceAddons();
  static ADDON::CRepositoryUpdater& GetRepositoryUpdater();
  static CInputManager& GetInputManager();
  static CFileExtensionProvider &GetFileExtensionProvider();
  static bool IsBinaryAddonCacheUp();
  static bool IsServiceManagerUp();
  static CNetworkBase& GetNetwork();
  static CPowerManager& GetPowerManager();
  static CWeatherManager& GetWeatherManager();
  static CPlayerCoreFactory &GetPlayerCoreFactory();
  static CDatabaseManager &GetDatabaseManager();
  static CEventLog &GetEventLog();

  static CGUIComponent* GetGUI();
  static void RegisterGUI(CGUIComponent *gui);
  static void UnregisterGUI();

  static void RegisterSettingsComponent(CSettingsComponent *settings);
  static void UnregisterSettingsComponent();
  static CSettingsComponent* GetSettingsComponent();

  static void RegisterWinSystem(CWinSystemBase *winsystem);
  static void UnregisterWinSystem();
  static CWinSystemBase* GetWinSystem();
  static CRenderSystemBase* GetRenderSystem();

  static IAE* GetActiveAE();
  static void RegisterAE(IAE *ae);
  static void UnregisterAE();

  static std::shared_ptr<CAppInboundProtocol> GetAppPort();
  static void RegisterAppPort(std::shared_ptr<CAppInboundProtocol> port);
  static void UnregisterAppPort();

private:
  static std::shared_ptr<ANNOUNCEMENT::CAnnouncementManager> m_pAnnouncementManager;
  static CGUIComponent* m_pGUI;
  static CWinSystemBase* m_pWinSystem;
  static IAE* m_pActiveAE;
  static std::shared_ptr<CAppInboundProtocol> m_pAppPort;
  static CSettingsComponent* m_pSettingsComponent;
};
