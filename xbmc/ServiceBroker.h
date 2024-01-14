/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/GlobalsHandling.h"

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

namespace ANNOUNCEMENT
{
class CAnnouncementManager;
}

namespace MEDIA_DETECT
{
class CDetectDVDMedia;
}

namespace PVR
{
class CPVRManager;
}

namespace PLAYLIST
{
class CPlayListPlayer;
}

namespace KODI
{
namespace MESSAGING
{
class CApplicationMessenger;
}
} // namespace KODI

class CAppParams;
template<class T>
class CComponentContainer;
class CContextMenuManager;
class XBPython;
class CDataCacheCore;
class IAE;
class IApplicationComponent;
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
class CDecoderFilterManager;
class CMediaManager;
class CCPUInfo;
class CLog;
class CPlatform;
class CTextureCache;
class CJobManager;
class CSlideShowDelegator;

namespace WSDiscovery
{
class IWSDiscovery;
}

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

namespace KEYBOARD
{
class CKeyboardLayoutManager;
} // namespace KEYBOARD

namespace RETRO
{
class CGUIGameRenderManager;
}
} // namespace KODI

namespace PERIPHERALS
{
class CPeripherals;
}

namespace speech
{
class ISpeechRecognition;
}

class CServiceBroker
{
public:
  CServiceBroker();
  ~CServiceBroker();

  static std::shared_ptr<CAppParams> GetAppParams();
  static void RegisterAppParams(const std::shared_ptr<CAppParams>& appParams);
  static void UnregisterAppParams();

  static CLog& GetLogging();
  static void CreateLogging();
  static bool IsLoggingUp();
  static void DestroyLogging();

  static std::shared_ptr<ANNOUNCEMENT::CAnnouncementManager> GetAnnouncementManager();
  static void RegisterAnnouncementManager(
      std::shared_ptr<ANNOUNCEMENT::CAnnouncementManager> announcementManager);
  static void UnregisterAnnouncementManager();

  static ADDON::CAddonMgr& GetAddonMgr();
  static ADDON::CBinaryAddonManager& GetBinaryAddonManager();
  static ADDON::CBinaryAddonCache& GetBinaryAddonCache();
  static KODI::ADDONS::CExtsMimeSupportList& GetExtsMimeSupportList();
  static ADDON::CVFSAddonCache& GetVFSAddonCache();
  static XBPython& GetXBPython();
  static WSDiscovery::IWSDiscovery& GetWSDiscovery();
  static MEDIA_DETECT::CDetectDVDMedia& GetDetectDVDMedia();
  static PVR::CPVRManager& GetPVRManager();
  static CContextMenuManager& GetContextMenuManager();
  static CDataCacheCore& GetDataCacheCore();
  static CPlatform& GetPlatform();
  static PLAYLIST::CPlayListPlayer& GetPlaylistPlayer();
  static CSlideShowDelegator& GetSlideShowDelegator();
  static KODI::GAME::CControllerManager& GetGameControllerManager();
  static KODI::GAME::CGameServices& GetGameServices();
  static KODI::RETRO::CGUIGameRenderManager& GetGameRenderManager();
  static PERIPHERALS::CPeripherals& GetPeripherals();
  static CFavouritesService& GetFavouritesService();
  static ADDON::CServiceAddonManager& GetServiceAddons();
  static ADDON::CRepositoryUpdater& GetRepositoryUpdater();
  static CInputManager& GetInputManager();
  static CFileExtensionProvider& GetFileExtensionProvider();
  static bool IsAddonInterfaceUp();
  static bool IsServiceManagerUp();
  static CNetworkBase& GetNetwork();
  static CPowerManager& GetPowerManager();
  static CWeatherManager& GetWeatherManager();
  static CPlayerCoreFactory& GetPlayerCoreFactory();
  static CDatabaseManager& GetDatabaseManager();
  static CEventLog* GetEventLog();
  static CMediaManager& GetMediaManager();
  static CComponentContainer<IApplicationComponent>& GetAppComponents();

  static CGUIComponent* GetGUI();
  static void RegisterGUI(CGUIComponent* gui);
  static void UnregisterGUI();

  static void RegisterSettingsComponent(const std::shared_ptr<CSettingsComponent>& settings);
  static void UnregisterSettingsComponent();
  static std::shared_ptr<CSettingsComponent> GetSettingsComponent();

  static void RegisterWinSystem(CWinSystemBase* winsystem);
  static void UnregisterWinSystem();
  static CWinSystemBase* GetWinSystem();
  static CRenderSystemBase* GetRenderSystem();

  static IAE* GetActiveAE();
  static void RegisterAE(IAE* ae);
  static void UnregisterAE();

  static std::shared_ptr<CAppInboundProtocol> GetAppPort();
  static void RegisterAppPort(std::shared_ptr<CAppInboundProtocol> port);
  static void UnregisterAppPort();

  static void RegisterDecoderFilterManager(CDecoderFilterManager* manager);
  static CDecoderFilterManager* GetDecoderFilterManager();

  static std::shared_ptr<CCPUInfo> GetCPUInfo();
  static void RegisterCPUInfo(std::shared_ptr<CCPUInfo> cpuInfo);
  static void UnregisterCPUInfo();

  static void RegisterTextureCache(const std::shared_ptr<CTextureCache>& cache);
  static void UnregisterTextureCache();
  static std::shared_ptr<CTextureCache> GetTextureCache();

  static void RegisterJobManager(const std::shared_ptr<CJobManager>& jobManager);
  static void UnregisterJobManager();
  static std::shared_ptr<CJobManager> GetJobManager();

  static void RegisterAppMessenger(
      const std::shared_ptr<KODI::MESSAGING::CApplicationMessenger>& appMessenger);
  static void UnregisterAppMessenger();
  static std::shared_ptr<KODI::MESSAGING::CApplicationMessenger> GetAppMessenger();

  static void RegisterKeyboardLayoutManager(
      const std::shared_ptr<KODI::KEYBOARD::CKeyboardLayoutManager>& keyboardLayoutManager);
  static void UnregisterKeyboardLayoutManager();
  static std::shared_ptr<KODI::KEYBOARD::CKeyboardLayoutManager> GetKeyboardLayoutManager();

  static void RegisterSpeechRecognition(
      const std::shared_ptr<speech::ISpeechRecognition>& speechRecognition);
  static void UnregisterSpeechRecognition();
  static std::shared_ptr<speech::ISpeechRecognition> GetSpeechRecognition();

private:
  std::shared_ptr<CAppParams> m_appParams;
  std::unique_ptr<CLog> m_logging;
  std::shared_ptr<ANNOUNCEMENT::CAnnouncementManager> m_pAnnouncementManager;
  CGUIComponent* m_pGUI = nullptr;
  CWinSystemBase* m_pWinSystem = nullptr;
  IAE* m_pActiveAE = nullptr;
  std::shared_ptr<CAppInboundProtocol> m_pAppPort;
  std::shared_ptr<CSettingsComponent> m_pSettingsComponent;
  CDecoderFilterManager* m_decoderFilterManager = nullptr;
  std::shared_ptr<CCPUInfo> m_cpuInfo;
  std::shared_ptr<CTextureCache> m_textureCache;
  std::shared_ptr<CJobManager> m_jobManager;
  std::shared_ptr<KODI::MESSAGING::CApplicationMessenger> m_appMessenger;
  std::shared_ptr<KODI::KEYBOARD::CKeyboardLayoutManager> m_keyboardLayoutManager;
  std::shared_ptr<speech::ISpeechRecognition> m_speechRecognition;
  std::shared_ptr<CSlideShowDelegator> m_slideshowDelegator;
};

XBMC_GLOBAL_REF(CServiceBroker, g_serviceBroker);
#define g_serviceBroker XBMC_GLOBAL_USE(CServiceBroker)
