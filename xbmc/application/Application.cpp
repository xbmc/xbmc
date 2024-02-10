/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Application.h"

#include "Autorun.h"
#include "CompileInfo.h"
#include "DatabaseManager.h"
#include "FileItem.h"
#include "GUIInfoManager.h"
#include "GUILargeTextureManager.h"
#include "GUIPassword.h"
#include "GUIUserMessages.h"
#include "HDRStatus.h"
#include "LangInfo.h"
#include "PartyModeManager.h"
#include "PlayListPlayer.h"
#include "SectionLoader.h"
#include "SeekHandler.h"
#include "ServiceBroker.h"
#include "ServiceManager.h"
#include "TextureCache.h"
#include "URL.h"
#include "Util.h"
#include "addons/AddonManager.h"
#include "addons/AddonSystemSettings.h"
#include "addons/RepositoryUpdater.h"
#include "addons/Service.h"
#include "addons/Skin.h"
#include "addons/VFSEntry.h"
#include "addons/addoninfo/AddonInfo.h"
#include "addons/addoninfo/AddonType.h"
#include "addons/gui/GUIDialogAddonSettings.h"
#include "application/AppInboundProtocol.h"
#include "application/AppParams.h"
#include "application/ApplicationActionListeners.h"
#include "application/ApplicationPlayer.h"
#include "application/ApplicationPowerHandling.h"
#include "application/ApplicationSkinHandling.h"
#include "application/ApplicationStackHelper.h"
#include "application/ApplicationVolumeHandling.h"
#include "cores/AudioEngine/Engines/ActiveAE/ActiveAE.h"
#include "cores/DataCacheCore.h"
#include "cores/FFmpeg.h"
#include "cores/IPlayer.h"
#include "cores/playercorefactory/PlayerCoreFactory.h"
#include "dialogs/GUIDialogBusy.h"
#include "dialogs/GUIDialogCache.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "dialogs/GUIDialogSimpleMenu.h"
#include "events/EventLog.h"
#include "events/NotificationEvent.h"
#include "filesystem/Directory.h"
#include "filesystem/DirectoryCache.h"
#include "filesystem/DirectoryFactory.h"
#include "filesystem/DllLibCurl.h"
#include "filesystem/File.h"
#include "music/MusicFileItemClassify.h"
#include "network/NetworkFileItemClassify.h"
#include "video/VideoFileItemClassify.h"
#ifdef HAS_FILESYSTEM_NFS
#include "filesystem/NFSFile.h"
#endif
#include "filesystem/PluginDirectory.h"
#include "filesystem/SpecialProtocol.h"
#ifdef HAS_UPNP
#include "filesystem/UPnPDirectory.h"
#endif
#include "guilib/GUIAudioManager.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIControlProfiler.h"
#include "guilib/GUIFontManager.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/StereoscopicsManager.h"
#include "guilib/TextureManager.h"
#include "input/InertialScrollingHandler.h"
#include "input/InputManager.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "input/actions/ActionTranslator.h"
#include "input/keyboard/KeyboardLayoutManager.h"
#include "interfaces/AnnouncementManager.h"
#include "interfaces/builtins/Builtins.h"
#include "interfaces/generic/ScriptInvocationManager.h"
#include "interfaces/json-rpc/JSONRPC.h"
#ifdef HAS_PYTHON
#include "interfaces/python/XBPython.h"
#endif
#include "messaging/ApplicationMessenger.h"
#include "messaging/ThreadMessage.h"
#include "messaging/helpers/DialogHelper.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "music/MusicLibraryQueue.h"
#include "music/MusicThumbLoader.h"
#include "music/MusicUtils.h"
#include "music/infoscanner/MusicInfoScanner.h"
#include "music/tags/MusicInfoTag.h"
#include "network/EventServer.h"
#include "network/Network.h"
#include "network/ZeroconfBrowser.h"
#ifdef HAS_UPNP
#include "network/upnp/UPnP.h"
#endif
#include "peripherals/Peripherals.h"
#include "pictures/SlideShowDelegator.h"
#include "platform/Environment.h"
#include "playlists/PlayList.h"
#include "playlists/PlayListFactory.h"
#include "playlists/SmartPlayList.h"
#include "powermanagement/PowerManager.h"
#include "profiles/ProfileManager.h"
#include "pvr/PVRManager.h"
#include "pvr/guilib/PVRGUIActionsPlayback.h"
#include "pvr/guilib/PVRGUIActionsPowerManagement.h"
#include "settings/AdvancedSettings.h"
#include "settings/DisplaySettings.h"
#include "settings/MediaSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "speech/ISpeechRecognition.h"
#include "storage/MediaManager.h"
#include "threads/SingleLock.h"
#include "threads/SystemClock.h"
#include "utils/AlarmClock.h"
#include "utils/CPUInfo.h"
#include "utils/CharsetConverter.h"
#include "utils/ContentUtils.h"
#include "utils/FileExtensionProvider.h"
#include "utils/JobManager.h"
#include "utils/LangCodeExpander.h"
#include "utils/PlayerUtils.h"
#include "utils/RegExp.h"
#include "utils/Screenshot.h"
#include "utils/StringUtils.h"
#include "utils/SystemInfo.h"
#include "utils/TimeUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/XTimeUtils.h"
#include "utils/log.h"
#include "video/Bookmark.h"
#include "video/PlayerController.h"
#include "video/VideoLibraryQueue.h"
#include "video/dialogs/GUIDialogVideoBookmarks.h"
#ifdef TARGET_WINDOWS
#include "win32util.h"
#endif
#include "windowing/WinSystem.h"
#include "windowing/WindowSystemFactory.h"

#if defined(TARGET_ANDROID)
#include "platform/android/activity/XBMCApp.h"
#endif
#ifdef TARGET_DARWIN
#include "platform/darwin/DarwinUtils.h"
#endif
#ifdef TARGET_DARWIN_OSX
#ifdef HAS_XBMCHELPER
#include "platform/darwin/osx/XBMCHelper.h"
#endif
#endif
#ifdef TARGET_POSIX
#include "platform/posix/PlatformPosix.h"
#include "platform/posix/XHandle.h"
#endif
#if defined(TARGET_POSIX) && defined(HAS_FILESYSTEM_SMB)
#include "platform/posix/filesystem/SMBFile.h"
#endif
#ifndef TARGET_POSIX
#include "platform/win32/threads/Win32Exception.h"
#endif

#include <cmath>
#include <memory>
#include <mutex>

//TODO: XInitThreads
#ifdef HAVE_X11
#include <X11/Xlib.h>
#endif
#ifdef HAS_OPTICAL_DRIVE
#include <cdio/logging.h>
#endif

using namespace ADDON;
using namespace XFILE;
#ifdef HAS_OPTICAL_DRIVE
using namespace MEDIA_DETECT;
#endif
using namespace MUSIC_INFO;
using namespace EVENTSERVER;
using namespace JSONRPC;
using namespace PVR;
using namespace PERIPHERALS;
using namespace KODI;
using namespace KODI::MESSAGING;
using namespace ActiveAE;

using namespace XbmcThreads;
using namespace std::chrono_literals;

using KODI::MESSAGING::HELPERS::DialogResponse;

using namespace std::chrono_literals;

#define MAX_FFWD_SPEED 5

CApplication::CApplication(void)
  :
#ifdef HAS_OPTICAL_DRIVE
    m_Autorun(new CAutorun()),
#endif
    m_pInertialScrollingHandler(new CInertialScrollingHandler()),
    m_WaitingExternalCalls(0),
    m_itemCurrentFile(std::make_shared<CFileItem>()),
    m_playerEvent(true, true)
{
  TiXmlBase::SetCondenseWhiteSpace(false);

#ifdef HAVE_X11
  XInitThreads();
#endif

  // register application components
  RegisterComponent(std::make_shared<CApplicationActionListeners>(m_critSection));
  RegisterComponent(std::make_shared<CApplicationPlayer>());
  RegisterComponent(std::make_shared<CApplicationPowerHandling>());
  RegisterComponent(std::make_shared<CApplicationSkinHandling>(this, this, m_bInitializing));
  RegisterComponent(std::make_shared<CApplicationVolumeHandling>());
  RegisterComponent(std::make_shared<CApplicationStackHelper>());
}

CApplication::~CApplication(void)
{
  DeregisterComponent(typeid(CApplicationStackHelper));
  DeregisterComponent(typeid(CApplicationVolumeHandling));
  DeregisterComponent(typeid(CApplicationSkinHandling));
  DeregisterComponent(typeid(CApplicationPowerHandling));
  DeregisterComponent(typeid(CApplicationPlayer));
  DeregisterComponent(typeid(CApplicationActionListeners));
}

extern "C" void __stdcall init_emu_environ();
extern "C" void __stdcall update_emu_environ();
extern "C" void __stdcall cleanup_emu_environ();

bool CApplication::Create()
{
  m_bStop = false;

  RegisterSettings();

  CServiceBroker::RegisterCPUInfo(CCPUInfo::GetCPUInfo());

  // Register JobManager service
  CServiceBroker::RegisterJobManager(std::make_shared<CJobManager>());

  // Announcement service
  m_pAnnouncementManager = std::make_shared<ANNOUNCEMENT::CAnnouncementManager>();
  m_pAnnouncementManager->Start();
  CServiceBroker::RegisterAnnouncementManager(m_pAnnouncementManager);

  const auto appMessenger = std::make_shared<CApplicationMessenger>();
  CServiceBroker::RegisterAppMessenger(appMessenger);

  const auto keyboardLayoutManager = std::make_shared<KEYBOARD::CKeyboardLayoutManager>();
  CServiceBroker::RegisterKeyboardLayoutManager(keyboardLayoutManager);

  m_ServiceManager = std::make_unique<CServiceManager>();

  if (!m_ServiceManager->InitStageOne())
  {
    return false;
  }

  // here we register all global classes for the CApplicationMessenger,
  // after that we can send messages to the corresponding modules
  appMessenger->RegisterReceiver(this);
  appMessenger->RegisterReceiver(&CServiceBroker::GetPlaylistPlayer());
  appMessenger->SetGUIThread(CThread::GetCurrentThreadId());
  appMessenger->SetProcessThread(CThread::GetCurrentThreadId());

  // copy required files
  CUtil::CopyUserDataIfNeeded("special://masterprofile/", "RssFeeds.xml");
  CUtil::CopyUserDataIfNeeded("special://masterprofile/", "favourites.xml");
  CUtil::CopyUserDataIfNeeded("special://masterprofile/", "Lircmap.xml");

  CServiceBroker::GetLogging().Initialize(CSpecialProtocol::TranslatePath("special://logpath"));

#ifdef TARGET_POSIX //! @todo Win32 has no special://home/ mapping by default, so we
  //!       must create these here. Ideally this should be using special://home/ and
  //!      be platform agnostic (i.e. unify the InitDirectories*() functions)
  if (!CServiceBroker::GetAppParams()->HasPlatformDirectories())
#endif
  {
    CDirectory::Create("special://xbmc/addons");
  }

  // Init our DllLoaders emu env
  init_emu_environ();

  PrintStartupLog();

  // initialize network protocols
  avformat_network_init();
  // set avutil callback
  av_log_set_callback(ff_avutil_log);

  CLog::Log(LOGINFO, "loading settings");
  const auto settingsComponent = CServiceBroker::GetSettingsComponent();
  if (!settingsComponent->Load())
    return false;

  // Log Cache GUI settings (replacement of cache in advancedsettings.xml)
  const auto settings = settingsComponent->GetSettings();
  const float readFactor = settings->GetInt(CSettings::SETTING_FILECACHE_READFACTOR) / 100.0f;
  CLog::Log(LOGINFO,
            "New Cache GUI Settings (replacement of cache in advancedsettings.xml) are:\n  Buffer "
            "Mode: {}\n  Memory Size: {} MB\n  Read "
            "Factor: {:.2f} x {}\n  Chunk Size : {} bytes",
            settings->GetInt(CSettings::SETTING_FILECACHE_BUFFERMODE),
            settings->GetInt(CSettings::SETTING_FILECACHE_MEMORYSIZE), readFactor,
            (readFactor < 1.0f) ? "(adaptive)" : "",
            settings->GetInt(CSettings::SETTING_FILECACHE_CHUNKSIZE));

  CLog::Log(LOGINFO, "creating subdirectories");
  const std::shared_ptr<CProfileManager> profileManager = settingsComponent->GetProfileManager();
  CLog::Log(LOGINFO, "userdata folder: {}",
            CURL::GetRedacted(profileManager->GetProfileUserDataFolder()));
  CLog::Log(LOGINFO, "recording folder: {}",
            CURL::GetRedacted(settings->GetString(CSettings::SETTING_AUDIOCDS_RECORDINGPATH)));
  CLog::Log(LOGINFO, "screenshots folder: {}",
            CURL::GetRedacted(settings->GetString(CSettings::SETTING_DEBUG_SCREENSHOTPATH)));
  CDirectory::Create(profileManager->GetUserDataFolder());
  CDirectory::Create(profileManager->GetProfileUserDataFolder());
  profileManager->CreateProfileFolders();

  update_emu_environ();//apply the GUI settings

  // application inbound service
  m_pAppPort = std::make_shared<CAppInboundProtocol>(*this);
  CServiceBroker::RegisterAppPort(m_pAppPort);

  if (!m_ServiceManager->InitStageTwo(
          settingsComponent->GetProfileManager()->GetProfileUserDataFolder()))
  {
    return false;
  }

  m_pActiveAE = std::make_unique<ActiveAE::CActiveAE>();
  CServiceBroker::RegisterAE(m_pActiveAE.get());

  // initialize m_replayGainSettings
  GetComponent<CApplicationVolumeHandling>()->CacheReplayGainSettings(*settings);

  // load the keyboard layouts
  if (!keyboardLayoutManager->Load())
  {
    CLog::Log(LOGFATAL, "CApplication::Create: Unable to load keyboard layouts");
    return false;
  }

  // set user defined CA trust bundle
  std::string caCert =
      CSpecialProtocol::TranslatePath(settingsComponent->GetAdvancedSettings()->m_caTrustFile);
  if (!caCert.empty())
  {
    if (XFILE::CFile::Exists(caCert))
    {
      CEnvironment::setenv("SSL_CERT_FILE", caCert, 1);
      CLog::Log(LOGDEBUG, "CApplication::Create - SSL_CERT_FILE: {}", caCert);
    }
    else
    {
      CLog::Log(LOGDEBUG, "CApplication::Create - Error reading SSL_CERT_FILE: {} -> ignored",
                caCert);
    }
  }

  CUtil::InitRandomSeed();

  m_lastRenderTime = std::chrono::steady_clock::now();
  return true;
}

bool CApplication::CreateGUI()
{
  m_frameMoveGuard.lock();

  const auto appPower = GetComponent<CApplicationPowerHandling>();
  appPower->SetRenderGUI(true);

  auto windowSystems = KODI::WINDOWING::CWindowSystemFactory::GetWindowSystems();

  const std::string& windowing = CServiceBroker::GetAppParams()->GetWindowing();

  if (!windowing.empty())
    windowSystems = {windowing};

  for (auto& windowSystem : windowSystems)
  {
    CLog::Log(LOGDEBUG, "CApplication::{} - trying to init {} windowing system", __FUNCTION__,
              windowSystem);
    m_pWinSystem = KODI::WINDOWING::CWindowSystemFactory::CreateWindowSystem(windowSystem);

    if (!m_pWinSystem)
      continue;

    if (!windowing.empty() && windowing != windowSystem)
      continue;

    CServiceBroker::RegisterWinSystem(m_pWinSystem.get());

    if (!m_pWinSystem->InitWindowSystem())
    {
      CLog::Log(LOGDEBUG, "CApplication::{} - unable to init {} windowing system", __FUNCTION__,
                windowSystem);
      m_pWinSystem->DestroyWindowSystem();
      m_pWinSystem.reset();
      CServiceBroker::UnregisterWinSystem();
      continue;
    }
    else
    {
      CLog::Log(LOGINFO, "CApplication::{} - using the {} windowing system", __FUNCTION__,
                windowSystem);
      break;
    }
  }

  if (!m_pWinSystem)
  {
    CLog::Log(LOGFATAL, "CApplication::{} - unable to init windowing system", __FUNCTION__);
    CServiceBroker::UnregisterWinSystem();
    return false;
  }

  // Retrieve the matching resolution based on GUI settings
  bool sav_res = false;
  CDisplaySettings::GetInstance().SetCurrentResolution(CDisplaySettings::GetInstance().GetDisplayResolution());
  CLog::Log(LOGINFO, "Checking resolution {}",
            CDisplaySettings::GetInstance().GetCurrentResolution());
  if (!CServiceBroker::GetWinSystem()->GetGfxContext().IsValidResolution(CDisplaySettings::GetInstance().GetCurrentResolution()))
  {
    CLog::Log(LOGINFO, "Setting safe mode {}", RES_DESKTOP);
    // defer saving resolution after window was created
    CDisplaySettings::GetInstance().SetCurrentResolution(RES_DESKTOP);
    sav_res = true;
  }

  // update the window resolution
  const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  CServiceBroker::GetWinSystem()->SetWindowResolution(settings->GetInt(CSettings::SETTING_WINDOW_WIDTH), settings->GetInt(CSettings::SETTING_WINDOW_HEIGHT));

  if (CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_startFullScreen && CDisplaySettings::GetInstance().GetCurrentResolution() == RES_WINDOW)
  {
    // defer saving resolution after window was created
    CDisplaySettings::GetInstance().SetCurrentResolution(RES_DESKTOP);
    sav_res = true;
  }

  if (!CServiceBroker::GetWinSystem()->GetGfxContext().IsValidResolution(CDisplaySettings::GetInstance().GetCurrentResolution()))
  {
    // Oh uh - doesn't look good for starting in their wanted screenmode
    CLog::Log(LOGERROR, "The screen resolution requested is not valid, resetting to a valid mode");
    CDisplaySettings::GetInstance().SetCurrentResolution(RES_DESKTOP);
    sav_res = true;
  }
  if (!InitWindow())
  {
    return false;
  }

  // Set default screen saver mode
  auto screensaverModeSetting = std::static_pointer_cast<CSettingString>(settings->GetSetting(CSettings::SETTING_SCREENSAVER_MODE));
  // Can only set this after windowing has been initialized since it depends on it
  if (CServiceBroker::GetWinSystem()->GetOSScreenSaver())
  {
    // If OS has a screen saver, use it by default
    screensaverModeSetting->SetDefault("");
  }
  else
  {
    // If OS has no screen saver, use Kodi one by default
    screensaverModeSetting->SetDefault("screensaver.xbmc.builtin.dim");
  }

  if (sav_res)
    CDisplaySettings::GetInstance().SetCurrentResolution(RES_DESKTOP, true);

  m_pGUI = std::make_unique<CGUIComponent>();
  m_pGUI->Init();

  // Splash requires gui component!!
  CServiceBroker::GetRenderSystem()->ShowSplash("");

  // The key mappings may already have been loaded by a peripheral
  CLog::Log(LOGINFO, "load keymapping");
  if (!CServiceBroker::GetInputManager().LoadKeymaps())
    return false;

  RESOLUTION_INFO info = CServiceBroker::GetWinSystem()->GetGfxContext().GetResInfo();
  CLog::Log(LOGINFO, "GUI format {}x{}, Display {}", info.iWidth, info.iHeight, info.strMode);

  return true;
}

bool CApplication::InitWindow(RESOLUTION res)
{
  if (res == RES_INVALID)
    res = CDisplaySettings::GetInstance().GetCurrentResolution();

  bool bFullScreen = res != RES_WINDOW;
  if (!CServiceBroker::GetWinSystem()->CreateNewWindow(CSysInfo::GetAppName(),
                                                      bFullScreen, CDisplaySettings::GetInstance().GetResolutionInfo(res)))
  {
    CLog::Log(LOGFATAL, "CApplication::Create: Unable to create window");
    return false;
  }

  if (!CServiceBroker::GetRenderSystem()->InitRenderSystem())
  {
    CLog::Log(LOGFATAL, "CApplication::Create: Unable to init rendering system");
    return false;
  }
  // set GUI res and force the clear of the screen
  CServiceBroker::GetWinSystem()->GetGfxContext().SetVideoResolution(res, false);
  return true;
}

bool CApplication::Initialize()
{
  m_pActiveAE->Start();
  // restore AE's previous volume state

  const auto appVolume = GetComponent<CApplicationVolumeHandling>();
  const auto level = appVolume->GetVolumeRatio();
  const auto muted = appVolume->IsMuted();
  appVolume->SetHardwareVolume(level);
  CServiceBroker::GetActiveAE()->SetMute(muted);

#if defined(HAS_OPTICAL_DRIVE) && \
    !defined(TARGET_WINDOWS) // somehow this throws an "unresolved external symbol" on win32
  // turn off cdio logging
  cdio_loglevel_default = CDIO_LOG_ERROR;
#endif

  // load the language and its translated strings
  if (!LoadLanguage(false))
    return false;

  // load media manager sources (e.g. root addon type sources depend on language strings to be available)
  CServiceBroker::GetMediaManager().LoadSources();

  const std::shared_ptr<CProfileManager> profileManager = CServiceBroker::GetSettingsComponent()->GetProfileManager();

  profileManager->GetEventLog().Add(EventPtr(new CNotificationEvent(
      StringUtils::Format(g_localizeStrings.Get(177), g_sysinfo.GetAppName()),
      StringUtils::Format(g_localizeStrings.Get(178), g_sysinfo.GetAppName()),
      "special://xbmc/media/icon256x256.png", EventLevel::Basic)));

  m_ServiceManager->GetNetwork().WaitForNet();

  // initialize (and update as needed) our databases
  CDatabaseManager &databaseManager = m_ServiceManager->GetDatabaseManager();

  CEvent event(true);
  CServiceBroker::GetJobManager()->Submit([&databaseManager, &event]() {
    databaseManager.Initialize();
    event.Set();
  });

  std::string localizedStr = g_localizeStrings.Get(24150);
  int iDots = 1;
  while (!event.Wait(1000ms))
  {
    if (databaseManager.IsUpgrading())
      CServiceBroker::GetRenderSystem()->ShowSplash(std::string(iDots, ' ') + localizedStr + std::string(iDots, '.'));

    if (iDots == 3)
      iDots = 1;
    else
      ++iDots;
  }
  CServiceBroker::GetRenderSystem()->ShowSplash("");

  // Initialize GUI font manager to build/update fonts cache
  //! @todo Move GUIFontManager into service broker and drop the global reference
  event.Reset();
  GUIFontManager& guiFontManager = g_fontManager;
  CServiceBroker::GetJobManager()->Submit([&guiFontManager, &event]() {
    guiFontManager.Initialize();
    event.Set();
  });
  localizedStr = g_localizeStrings.Get(39175);
  iDots = 1;
  while (!event.Wait(1000ms))
  {
    if (g_fontManager.IsUpdating())
      CServiceBroker::GetRenderSystem()->ShowSplash(std::string(iDots, ' ') + localizedStr +
                                                    std::string(iDots, '.'));

    if (iDots == 3)
      iDots = 1;
    else
      ++iDots;
  }
  CServiceBroker::GetRenderSystem()->ShowSplash("");

  // GUI depends on seek handler
  GetComponent<CApplicationPlayer>()->GetSeekHandler().Configure();

  const auto skinHandling = GetComponent<CApplicationSkinHandling>();

  bool uiInitializationFinished = false;

  if (CServiceBroker::GetGUI()->GetWindowManager().Initialized())
  {
    const auto settings = CServiceBroker::GetSettingsComponent()->GetSettings();

    CServiceBroker::GetGUI()->GetWindowManager().CreateWindows();

    skinHandling->m_confirmSkinChange = false;

    std::vector<AddonInfoPtr> incompatibleAddons;
    event.Reset();

    // Addon migration
    if (CServiceBroker::GetAddonMgr().GetIncompatibleEnabledAddonInfos(incompatibleAddons))
    {
      if (CAddonSystemSettings::GetInstance().GetAddonAutoUpdateMode() == AUTO_UPDATES_ON)
      {
        CServiceBroker::GetJobManager()->Submit(
            [&event, &incompatibleAddons]() {
              if (CServiceBroker::GetRepositoryUpdater().CheckForUpdates())
                CServiceBroker::GetRepositoryUpdater().Await();

              incompatibleAddons = CServiceBroker::GetAddonMgr().MigrateAddons();
              event.Set();
            },
            CJob::PRIORITY_DEDICATED);
        localizedStr = g_localizeStrings.Get(24151);
        iDots = 1;
        while (!event.Wait(1000ms))
        {
          CServiceBroker::GetRenderSystem()->ShowSplash(std::string(iDots, ' ') + localizedStr +
                                                        std::string(iDots, '.'));
          if (iDots == 3)
            iDots = 1;
          else
            ++iDots;
        }
        m_incompatibleAddons = incompatibleAddons;
      }
      else
      {
        // If no update is active disable all incompatible addons during start
        m_incompatibleAddons =
            CServiceBroker::GetAddonMgr().DisableIncompatibleAddons(incompatibleAddons);
      }
    }

    // Start splashscreen and load skin
    CServiceBroker::GetRenderSystem()->ShowSplash("");
    skinHandling->m_confirmSkinChange = true;

    auto setting = settings->GetSetting(CSettings::SETTING_LOOKANDFEEL_SKIN);
    if (!setting)
    {
      CLog::Log(LOGFATAL, "Failed to load setting for: {}", CSettings::SETTING_LOOKANDFEEL_SKIN);
      return false;
    }

    CServiceBroker::RegisterTextureCache(std::make_shared<CTextureCache>());

    std::string skinId = settings->GetString(CSettings::SETTING_LOOKANDFEEL_SKIN);
    if (!skinHandling->LoadSkin(skinId))
    {
      CLog::Log(LOGERROR, "Failed to load skin '{}'", skinId);
      std::string defaultSkin =
          std::static_pointer_cast<const CSettingString>(setting)->GetDefault();
      if (!skinHandling->LoadSkin(defaultSkin))
      {
        CLog::Log(LOGFATAL, "Default skin '{}' could not be loaded! Terminating..", defaultSkin);
        return false;
      }
    }

    // initialize splash window after splash screen disappears
    // because we need a real window in the background which gets
    // rendered while we load the main window or enter the master lock key
    CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(WINDOW_SPLASH);

    if (settings->GetBool(CSettings::SETTING_MASTERLOCK_STARTUPLOCK) &&
        profileManager->GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE &&
        !profileManager->GetMasterProfile().getLockCode().empty())
    {
      g_passwordManager.CheckStartUpLock();
    }

    // check if we should use the login screen
    if (profileManager->UsingLoginScreen())
    {
      CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(WINDOW_LOGIN_SCREEN);
    }
    else
    {
      // activate the configured start window
      int firstWindow = g_SkinInfo->GetFirstWindow();
      CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(firstWindow);

      if (CServiceBroker::GetGUI()->GetWindowManager().IsWindowActive(WINDOW_STARTUP_ANIM))
      {
        CLog::Log(LOGWARNING, "CApplication::Initialize - startup.xml taints init process");
      }

      // the startup window is considered part of the initialization as it most likely switches to the final window
      uiInitializationFinished = firstWindow != WINDOW_STARTUP_ANIM;
    }
  }
  else //No GUI Created
  {
    uiInitializationFinished = true;
  }

  CJSONRPC::Initialize();

  CServiceBroker::RegisterSpeechRecognition(speech::ISpeechRecognition::CreateInstance());

  if (!m_ServiceManager->InitStageThree(profileManager))
  {
    CLog::Log(LOGERROR, "Application - Init3 failed");
  }

  g_sysinfo.Refresh();

  CLog::Log(LOGINFO, "removing tempfiles");
  CUtil::RemoveTempFiles();

  if (!profileManager->UsingLoginScreen())
  {
    UpdateLibraries();
    SetLoggingIn(false);
  }

  m_slowTimer.StartZero();

  // register action listeners
  const auto appListener = GetComponent<CApplicationActionListeners>();
  const auto appPlayer = GetComponent<CApplicationPlayer>();
  appListener->RegisterActionListener(&appPlayer->GetSeekHandler());
  appListener->RegisterActionListener(&CPlayerController::GetInstance());

  CServiceBroker::GetRepositoryUpdater().Start();
  if (!profileManager->UsingLoginScreen())
    CServiceBroker::GetServiceAddons().Start();

  CLog::Log(LOGINFO, "initialize done");

  const auto appPower = GetComponent<CApplicationPowerHandling>();
  appPower->CheckOSScreenSaverInhibitionSetting();
  // reset our screensaver (starts timers etc.)
  appPower->ResetScreenSaver();

  // if the user interfaces has been fully initialized let everyone know
  if (uiInitializationFinished)
  {
    CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UI_READY);
    CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
  }

  return true;
}

bool CApplication::OnSettingsSaving() const
{
  // don't save settings when we're busy stopping the application
  // a lot of screens try to save settings on deinit and deinit is
  // called for every screen when the application is stopping
  return !m_bStop;
}

void CApplication::Render()
{
  // do not render if we are stopped or in background
  if (m_bStop)
    return;

  const auto appPlayer = GetComponent<CApplicationPlayer>();
  const auto appPower = GetComponent<CApplicationPowerHandling>();

  bool hasRendered = false;

  // Whether externalplayer is playing and we're unfocused
  bool extPlayerActive = appPlayer->IsExternalPlaying() && !m_AppFocused;

  if (!extPlayerActive && CServiceBroker::GetWinSystem()->GetGfxContext().IsFullScreenVideo() &&
      !appPlayer->IsPausedPlayback())
  {
    appPower->ResetScreenSaver();
  }

  if (!CServiceBroker::GetRenderSystem()->BeginRender())
    return;

  // render gui layer
  const bool renderGUI = appPower->GetRenderGUI();
  if (m_guiRenderLastState != std::nullopt && renderGUI && m_guiRenderLastState != renderGUI)
  {
    CGUIComponent* gui = CServiceBroker::GetGUI();
    if (gui)
      CServiceBroker::GetGUI()->GetWindowManager().MarkDirty();
  }
  m_guiRenderLastState = renderGUI;

  if (renderGUI && !m_skipGuiRender)
  {
    if (CServiceBroker::GetWinSystem()->GetGfxContext().GetStereoMode())
    {
      CServiceBroker::GetWinSystem()->GetGfxContext().SetStereoView(RENDER_STEREO_VIEW_LEFT);
      hasRendered |= CServiceBroker::GetGUI()->GetWindowManager().Render();

      if (CServiceBroker::GetWinSystem()->GetGfxContext().GetStereoMode() != RENDER_STEREO_MODE_MONO)
      {
        CServiceBroker::GetWinSystem()->GetGfxContext().SetStereoView(RENDER_STEREO_VIEW_RIGHT);
        hasRendered |= CServiceBroker::GetGUI()->GetWindowManager().Render();
      }
      CServiceBroker::GetWinSystem()->GetGfxContext().SetStereoView(RENDER_STEREO_VIEW_OFF);
    }
    else
    {
      hasRendered |= CServiceBroker::GetGUI()->GetWindowManager().Render();
    }
    // execute post rendering actions (finalize window closing)
    CServiceBroker::GetGUI()->GetWindowManager().AfterRender();

    m_lastRenderTime = std::chrono::steady_clock::now();
  }

  // render video layer
  CServiceBroker::GetGUI()->GetWindowManager().RenderEx();

  CServiceBroker::GetRenderSystem()->EndRender();

  // reset our info cache - we do this at the end of Render so that it is
  // fresh for the next process(), or after a windowclose animation (where process()
  // isn't called)
  CGUIInfoManager& infoMgr = CServiceBroker::GetGUI()->GetInfoManager();
  infoMgr.ResetCache();
  infoMgr.GetInfoProviders().GetGUIControlsInfoProvider().ResetContainerMovingCache();

  if (hasRendered)
  {
    infoMgr.GetInfoProviders().GetSystemInfoProvider().UpdateFPS();
  }

  CServiceBroker::GetWinSystem()->GetGfxContext().Flip(hasRendered,
                                                       appPlayer->IsRenderingVideoLayer());

  CTimeUtils::UpdateFrameTime(hasRendered);
}

bool CApplication::OnAction(const CAction &action)
{
  // special case for switching between GUI & fullscreen mode.
  if (action.GetID() == ACTION_SHOW_GUI)
  { // Switch to fullscreen mode if we can
    CGUIComponent* gui = CServiceBroker::GetGUI();
    if (gui)
    {
      if (gui->GetWindowManager().SwitchToFullScreen())
      {
        GetComponent<CApplicationPowerHandling>()->m_navigationTimer.StartZero();
        return true;
      }
    }
  }

  const auto appPlayer = GetComponent<CApplicationPlayer>();

  if (action.GetID() == ACTION_TOGGLE_FULLSCREEN)
  {
    CServiceBroker::GetWinSystem()->GetGfxContext().ToggleFullScreen();
    appPlayer->TriggerUpdateResolution();
    return true;
  }

  if (action.IsMouse())
    CServiceBroker::GetInputManager().SetMouseActive(true);

  if (action.GetID() == ACTION_CREATE_EPISODE_BOOKMARK)
  {
    CGUIDialogVideoBookmarks::OnAddEpisodeBookmark();
  }
  if (action.GetID() == ACTION_CREATE_BOOKMARK)
  {
    CGUIDialogVideoBookmarks::OnAddBookmark();
  }

  // The action PLAYPAUSE behaves as ACTION_PAUSE if we are currently
  // playing or ACTION_PLAYER_PLAY if we are seeking (FF/RW) or not playing.
  if (action.GetID() == ACTION_PLAYER_PLAYPAUSE)
  {
    CSlideShowDelegator& slideShow = CServiceBroker::GetSlideShowDelegator();
    if ((appPlayer->IsPlaying() && appPlayer->GetPlaySpeed() == 1) ||
        (slideShow.InSlideShow() && !slideShow.IsPaused()))
      return OnAction(CAction(ACTION_PAUSE));
    else
      return OnAction(CAction(ACTION_PLAYER_PLAY));
  }

  //if the action would start or stop inertial scrolling
  //by gesture - bypass the normal OnAction handler of current window
  if( !m_pInertialScrollingHandler->CheckForInertialScrolling(&action) )
  {
    // in normal case
    // just pass the action to the current window and let it handle it
    if (CServiceBroker::GetGUI()->GetWindowManager().OnAction(action))
    {
      GetComponent<CApplicationPowerHandling>()->ResetNavigationTimer();
      return true;
    }
  }

  // handle extra global presses

  // notify action listeners
  if (GetComponent<CApplicationActionListeners>()->NotifyActionListeners(action))
    return true;

  // screenshot : take a screenshot :)
  if (action.GetID() == ACTION_TAKE_SCREENSHOT)
  {
    CScreenShot::TakeScreenshot();
    return true;
  }
  // Display HDR : toggle HDR on/off
  if (action.GetID() == ACTION_HDR_TOGGLE)
  {
    // Only enables manual HDR toggle if no video is playing or auto HDR switch is disabled
    if (appPlayer->IsPlayingVideo() && CServiceBroker::GetWinSystem()->IsHDRDisplaySettingEnabled())
      return true;

    HDR_STATUS hdrStatus = CServiceBroker::GetWinSystem()->ToggleHDR();

    if (hdrStatus == HDR_STATUS::HDR_OFF)
    {
      CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, g_localizeStrings.Get(34220),
                                            g_localizeStrings.Get(34221));
    }
    else if (hdrStatus == HDR_STATUS::HDR_ON)
    {
      CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, g_localizeStrings.Get(34220),
                                            g_localizeStrings.Get(34222));
    }
    return true;
  }
  // Tone Mapping : switch to next tone map method
  if (action.GetID() == ACTION_CYCLE_TONEMAP_METHOD)
  {
    // Only enables tone mapping switch if display is not HDR capable or HDR is not enabled
    if (CServiceBroker::GetWinSystem()->IsHDRDisplaySettingEnabled())
      return true;

    if (appPlayer->IsPlayingVideo())
    {
      CVideoSettings vs = appPlayer->GetVideoSettings();
      vs.m_ToneMapMethod = static_cast<ETONEMAPMETHOD>(static_cast<int>(vs.m_ToneMapMethod) + 1);
      if (vs.m_ToneMapMethod >= VS_TONEMAPMETHOD_MAX)
        vs.m_ToneMapMethod =
            static_cast<ETONEMAPMETHOD>(static_cast<int>(VS_TONEMAPMETHOD_OFF) + 1);

      appPlayer->SetVideoSettings(vs);

      int code = 0;
      switch (vs.m_ToneMapMethod)
      {
        case VS_TONEMAPMETHOD_REINHARD:
          code = 36555;
          break;
        case VS_TONEMAPMETHOD_ACES:
          code = 36557;
          break;
        case VS_TONEMAPMETHOD_HABLE:
          code = 36558;
          break;
        default:
          throw std::logic_error("Tonemapping method not found. Did you forget to add a mapping?");
      }
      CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, g_localizeStrings.Get(34224),
                                            g_localizeStrings.Get(code), 1000, false, 500);
    }
    return true;
  }
  // built in functions : execute the built-in
  if (action.GetID() == ACTION_BUILT_IN_FUNCTION)
  {
    if (!CBuiltins::GetInstance().IsSystemPowerdownCommand(action.GetName()) ||
        CServiceBroker::GetPVRManager().Get<PVR::GUI::PowerManagement>().CanSystemPowerdown())
    {
      CBuiltins::GetInstance().Execute(action.GetName());
      GetComponent<CApplicationPowerHandling>()->ResetNavigationTimer();
    }
    return true;
  }

  // reload keymaps
  if (action.GetID() == ACTION_RELOAD_KEYMAPS)
    CServiceBroker::GetInputManager().ReloadKeymaps();

  // show info : Shows the current video or song information
  if (action.GetID() == ACTION_SHOW_INFO)
  {
    CServiceBroker::GetGUI()->GetInfoManager().GetInfoProviders().GetPlayerInfoProvider().ToggleShowInfo();
    return true;
  }

  if (action.GetID() == ACTION_SET_RATING && appPlayer->IsPlayingAudio())
  {
    int userrating = MUSIC_UTILS::ShowSelectRatingDialog(m_itemCurrentFile->GetMusicInfoTag()->GetUserrating());
    if (userrating < 0) // Nothing selected, so user rating unchanged
      return true;
    userrating = std::min(userrating, 10);
    if (userrating != m_itemCurrentFile->GetMusicInfoTag()->GetUserrating())
    {
      m_itemCurrentFile->GetMusicInfoTag()->SetUserrating(userrating);
      // Mirror changes to GUI item
      CServiceBroker::GetGUI()->GetInfoManager().SetCurrentItem(*m_itemCurrentFile);

      // Asynchronously update song userrating in music library
      MUSIC_UTILS::UpdateSongRatingJob(m_itemCurrentFile, userrating);

      // Tell all windows (e.g. playlistplayer, media windows) to update the fileitem
      CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_ITEM, 0, m_itemCurrentFile);
      CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);
    }
    return true;
  }

  else if ((action.GetID() == ACTION_INCREASE_RATING || action.GetID() == ACTION_DECREASE_RATING) &&
           appPlayer->IsPlayingAudio())
  {
    int userrating = m_itemCurrentFile->GetMusicInfoTag()->GetUserrating();
    bool needsUpdate(false);
    if (userrating > 0 && action.GetID() == ACTION_DECREASE_RATING)
    {
      m_itemCurrentFile->GetMusicInfoTag()->SetUserrating(userrating - 1);
      needsUpdate = true;
    }
    else if (userrating < 10 && action.GetID() == ACTION_INCREASE_RATING)
    {
      m_itemCurrentFile->GetMusicInfoTag()->SetUserrating(userrating + 1);
      needsUpdate = true;
    }
    if (needsUpdate)
    {
      // Mirror changes to current GUI item
      CServiceBroker::GetGUI()->GetInfoManager().SetCurrentItem(*m_itemCurrentFile);

      // Asynchronously update song userrating in music library
      MUSIC_UTILS::UpdateSongRatingJob(m_itemCurrentFile, m_itemCurrentFile->GetMusicInfoTag()->GetUserrating());

      // send a message to all windows to tell them to update the fileitem (eg playlistplayer, media windows)
      CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_ITEM, 0, m_itemCurrentFile);
      CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);
    }

    return true;
  }
  else if ((action.GetID() == ACTION_INCREASE_RATING || action.GetID() == ACTION_DECREASE_RATING) &&
           appPlayer->IsPlayingVideo())
  {
    int rating = m_itemCurrentFile->GetVideoInfoTag()->m_iUserRating;
    bool needsUpdate(false);
    if (rating > 1 && action.GetID() == ACTION_DECREASE_RATING)
    {
      m_itemCurrentFile->GetVideoInfoTag()->m_iUserRating = rating - 1;
      needsUpdate = true;
    }
    else if (rating < 10 && action.GetID() == ACTION_INCREASE_RATING)
    {
      m_itemCurrentFile->GetVideoInfoTag()->m_iUserRating = rating + 1;
      needsUpdate = true;
    }
    if (needsUpdate)
    {
      // Mirror changes to GUI item
      CServiceBroker::GetGUI()->GetInfoManager().SetCurrentItem(*m_itemCurrentFile);

      CVideoDatabase db;
      if (db.Open())
      {
        db.SetVideoUserRating(m_itemCurrentFile->GetVideoInfoTag()->m_iDbId,
                              m_itemCurrentFile->GetVideoInfoTag()->m_iUserRating,
                              m_itemCurrentFile->GetVideoInfoTag()->m_type);
        db.Close();
      }
      // send a message to all windows to tell them to update the fileitem (eg playlistplayer, media windows)
      CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_ITEM, 0, m_itemCurrentFile);
      CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);
    }
    return true;
  }

  // Now check with the playlist player if action can be handled.
  // In case of ACTION_PREV_ITEM, we only allow the playlist player to take it if we're less than ACTION_PREV_ITEM_THRESHOLD seconds into playback.
  if (!(action.GetID() == ACTION_PREV_ITEM && appPlayer->CanSeek() &&
        GetTime() > ACTION_PREV_ITEM_THRESHOLD))
  {
    if (CServiceBroker::GetPlaylistPlayer().OnAction(action))
      return true;
  }

  // Now check with the player if action can be handled.
  bool bIsPlayingPVRChannel = (CServiceBroker::GetPVRManager().IsStarted() &&
                               CurrentFileItem().IsPVRChannel());

  bool bNotifyPlayer = false;
  if (CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_FULLSCREEN_VIDEO)
    bNotifyPlayer = true;
  else if (CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_FULLSCREEN_GAME)
    bNotifyPlayer = true;
  else if (CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_VISUALISATION && bIsPlayingPVRChannel)
    bNotifyPlayer = true;
  else if (CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_DIALOG_VIDEO_OSD ||
          (CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_DIALOG_MUSIC_OSD && bIsPlayingPVRChannel))
  {
    switch (action.GetID())
    {
      case ACTION_NEXT_ITEM:
      case ACTION_PREV_ITEM:
      case ACTION_CHANNEL_UP:
      case ACTION_CHANNEL_DOWN:
        bNotifyPlayer = true;
        break;
      default:
        break;
    }
  }
  else if (action.GetID() == ACTION_STOP)
    bNotifyPlayer = true;

  if (bNotifyPlayer)
  {
    if (appPlayer->OnAction(action))
      return true;
  }

  // stop : stops playing current audio song
  if (action.GetID() == ACTION_STOP)
  {
    StopPlaying();
    return true;
  }

  // In case the playlist player nor the player didn't handle PREV_ITEM, because we are past the ACTION_PREV_ITEM_THRESHOLD secs limit.
  // If so, we just jump to the start of the track.
  if (action.GetID() == ACTION_PREV_ITEM && appPlayer->CanSeek())
  {
    SeekTime(0);
    appPlayer->SetPlaySpeed(1);
    return true;
  }

  // forward action to graphic context and see if it can handle it
  if (CServiceBroker::GetGUI()->GetStereoscopicsManager().OnAction(action))
    return true;

  if (appPlayer->IsPlaying())
  {
    // forward channel switches to the player - he knows what to do
    if (action.GetID() == ACTION_CHANNEL_UP || action.GetID() == ACTION_CHANNEL_DOWN)
    {
      appPlayer->OnAction(action);
      return true;
    }

    // pause : toggle pause action
    if (action.GetID() == ACTION_PAUSE)
    {
      appPlayer->Pause();
      // go back to normal play speed on unpause
      if (!appPlayer->IsPaused() && appPlayer->GetPlaySpeed() != 1)
        appPlayer->SetPlaySpeed(1);

      CGUIComponent *gui = CServiceBroker::GetGUI();
      if (gui)
        gui->GetAudioManager().Enable(appPlayer->IsPaused());
      return true;
    }
    // play: unpause or set playspeed back to normal
    if (action.GetID() == ACTION_PLAYER_PLAY)
    {
      // if currently paused - unpause
      if (appPlayer->IsPaused())
        return OnAction(CAction(ACTION_PAUSE));
      // if we do a FF/RW then go back to normal speed
      if (appPlayer->GetPlaySpeed() != 1)
        appPlayer->SetPlaySpeed(1);
      return true;
    }
    if (!appPlayer->IsPaused())
    {
      if (action.GetID() == ACTION_PLAYER_FORWARD || action.GetID() == ACTION_PLAYER_REWIND)
      {
        float playSpeed = appPlayer->GetPlaySpeed();

        if (action.GetID() == ACTION_PLAYER_REWIND && (playSpeed == 1)) // Enables Rewinding
          playSpeed *= -2;
        else if (action.GetID() == ACTION_PLAYER_REWIND && playSpeed > 1) //goes down a notch if you're FFing
          playSpeed /= 2;
        else if (action.GetID() == ACTION_PLAYER_FORWARD && playSpeed < 1) //goes up a notch if you're RWing
          playSpeed /= 2;
        else
          playSpeed *= 2;

        if (action.GetID() == ACTION_PLAYER_FORWARD && playSpeed == -1) //sets iSpeed back to 1 if -1 (didn't plan for a -1)
          playSpeed = 1;
        if (playSpeed > 32 || playSpeed < -32)
          playSpeed = 1;

        appPlayer->SetPlaySpeed(playSpeed);
        return true;
      }
      else if ((action.GetAmount() || appPlayer->GetPlaySpeed() != 1) &&
               (action.GetID() == ACTION_ANALOG_REWIND || action.GetID() == ACTION_ANALOG_FORWARD))
      {
        // calculate the speed based on the amount the button is held down
        int iPower = (int)(action.GetAmount() * MAX_FFWD_SPEED + 0.5f);
        // amount can be negative, for example rewind and forward share the same axis
        iPower = std::abs(iPower);
        // returns 0 -> MAX_FFWD_SPEED
        int iSpeed = 1 << iPower;
        if (iSpeed != 1 && action.GetID() == ACTION_ANALOG_REWIND)
          iSpeed = -iSpeed;
        appPlayer->SetPlaySpeed(static_cast<float>(iSpeed));
        if (iSpeed == 1)
          CLog::Log(LOGDEBUG,"Resetting playspeed");
        return true;
      }
      else if (action.GetID() == ACTION_PLAYER_INCREASE_TEMPO)
      {
        CPlayerUtils::AdvanceTempoStep(appPlayer, TempoStepChange::INCREASE);
        return true;
      }
      else if (action.GetID() == ACTION_PLAYER_DECREASE_TEMPO)
      {
        CPlayerUtils::AdvanceTempoStep(appPlayer, TempoStepChange::DECREASE);
        return true;
      }
    }
    // allow play to unpause
    else
    {
      if (action.GetID() == ACTION_PLAYER_PLAY)
      {
        // unpause, and set the playspeed back to normal
        appPlayer->Pause();

        CGUIComponent *gui = CServiceBroker::GetGUI();
        if (gui)
          gui->GetAudioManager().Enable(appPlayer->IsPaused());

        appPlayer->SetPlaySpeed(1);
        return true;
      }
    }
  }


  if (action.GetID() == ACTION_SWITCH_PLAYER)
  {
    const CPlayerCoreFactory &playerCoreFactory = m_ServiceManager->GetPlayerCoreFactory();

    if (appPlayer->IsPlaying())
    {
      std::vector<std::string> players;
      CFileItem item(*m_itemCurrentFile.get());
      playerCoreFactory.GetPlayers(item, players);
      std::string player = playerCoreFactory.SelectPlayerDialog(players);
      if (!player.empty())
      {
        item.SetStartOffset(CUtil::ConvertSecsToMilliSecs(GetTime()));
        PlayFile(item, player, true);
      }
    }
    else
    {
      std::vector<std::string> players;
      playerCoreFactory.GetRemotePlayers(players);
      std::string player = playerCoreFactory.SelectPlayerDialog(players);
      if (!player.empty())
      {
        PlayFile(CFileItem(), player, false);
      }
    }
  }

  if (CServiceBroker::GetPeripherals().OnAction(action))
    return true;

  if (action.GetID() == ACTION_MUTE)
  {
    const auto appVolume = GetComponent<CApplicationVolumeHandling>();
    appVolume->ToggleMute();
    appVolume->ShowVolumeBar(&action);
    return true;
  }

  if (action.GetID() == ACTION_TOGGLE_DIGITAL_ANALOG)
  {
    const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
    bool passthrough = settings->GetBool(CSettings::SETTING_AUDIOOUTPUT_PASSTHROUGH);
    settings->SetBool(CSettings::SETTING_AUDIOOUTPUT_PASSTHROUGH, !passthrough);

    if (CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_SETTINGS_SYSTEM)
    {
      CGUIMessage msg(GUI_MSG_WINDOW_INIT, 0,0,WINDOW_INVALID,CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow());
      CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);
    }
    return true;
  }

  // Check for global volume control
  if ((action.GetAmount() && (action.GetID() == ACTION_VOLUME_UP || action.GetID() == ACTION_VOLUME_DOWN)) || action.GetID() == ACTION_VOLUME_SET)
  {
    const auto appVolume = GetComponent<CApplicationVolumeHandling>();
    if (!appPlayer->IsPassthrough())
    {
      if (appVolume->IsMuted())
        appVolume->UnMute();
      float volume = appVolume->GetVolumeRatio();
      int volumesteps = CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_AUDIOOUTPUT_VOLUMESTEPS);
      // sanity check
      if (volumesteps == 0)
        volumesteps = 90;

// Android has steps based on the max available volume level
#if defined(TARGET_ANDROID)
      float step = (CApplicationVolumeHandling::VOLUME_MAXIMUM -
                    CApplicationVolumeHandling::VOLUME_MINIMUM) /
                   CXBMCApp::GetMaxSystemVolume();
#else
      float step = (CApplicationVolumeHandling::VOLUME_MAXIMUM -
                    CApplicationVolumeHandling::VOLUME_MINIMUM) /
                   volumesteps;

      if (action.GetRepeat())
        step *= action.GetRepeat() * 50; // 50 fps
#endif
      if (action.GetID() == ACTION_VOLUME_UP)
        volume += action.GetAmount() * action.GetAmount() * step;
      else if (action.GetID() == ACTION_VOLUME_DOWN)
        volume -= action.GetAmount() * action.GetAmount() * step;
      else
        volume = action.GetAmount() * step;
      if (volume != appVolume->GetVolumeRatio())
        appVolume->SetVolume(volume, false);
    }
    // show visual feedback of volume or passthrough indicator
    appVolume->ShowVolumeBar(&action);
    return true;
  }

  if (action.GetID() == ACTION_GUIPROFILE_BEGIN)
  {
    CGUIControlProfiler::Instance().SetOutputFile(CSpecialProtocol::TranslatePath("special://home/guiprofiler.xml"));
    CGUIControlProfiler::Instance().Start();
    return true;
  }
  if (action.GetID() == ACTION_SHOW_PLAYLIST)
  {
    const PLAYLIST::Id playlistId = CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist();
    if (playlistId == PLAYLIST::TYPE_VIDEO &&
        CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() != WINDOW_VIDEO_PLAYLIST)
    {
      CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(WINDOW_VIDEO_PLAYLIST);
    }
    else if (playlistId == PLAYLIST::TYPE_MUSIC &&
             CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() !=
                 WINDOW_MUSIC_PLAYLIST)
    {
      CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(WINDOW_MUSIC_PLAYLIST);
    }
    return true;
  }
  return false;
}

int CApplication::GetMessageMask()
{
  return TMSG_MASK_APPLICATION;
}

void CApplication::OnApplicationMessage(ThreadMessage* pMsg)
{
  uint32_t msg = pMsg->dwMessage;
  if (msg == TMSG_SYSTEM_POWERDOWN)
  {
    if (CServiceBroker::GetPVRManager().Get<PVR::GUI::PowerManagement>().CanSystemPowerdown())
      msg = pMsg->param1; // perform requested shutdown action
    else
      return; // no shutdown
  }

  const auto appPlayer = GetComponent<CApplicationPlayer>();

  switch (msg)
  {
  case TMSG_POWERDOWN:
    if (Stop(EXITCODE_POWERDOWN))
      CServiceBroker::GetPowerManager().Powerdown();
    break;

  case TMSG_QUIT:
    Stop(EXITCODE_QUIT);
    break;

  case TMSG_SHUTDOWN:
    GetComponent<CApplicationPowerHandling>()->HandleShutdownMessage();
    break;

  case TMSG_RENDERER_FLUSH:
    appPlayer->FlushRenderer();
    break;

  case TMSG_HIBERNATE:
    CServiceBroker::GetPowerManager().Hibernate();
    break;

  case TMSG_SUSPEND:
    CServiceBroker::GetPowerManager().Suspend();
    break;

  case TMSG_RESTART:
  case TMSG_RESET:
    if (Stop(EXITCODE_REBOOT))
      CServiceBroker::GetPowerManager().Reboot();
    break;

  case TMSG_RESTARTAPP:
#if defined(TARGET_WINDOWS) || defined(TARGET_LINUX)
    Stop(EXITCODE_RESTARTAPP);
#endif
    break;

  case TMSG_INHIBITIDLESHUTDOWN:
    GetComponent<CApplicationPowerHandling>()->InhibitIdleShutdown(pMsg->param1 != 0);
    break;

  case TMSG_INHIBITSCREENSAVER:
    GetComponent<CApplicationPowerHandling>()->InhibitScreenSaver(pMsg->param1 != 0);
    break;

  case TMSG_ACTIVATESCREENSAVER:
    GetComponent<CApplicationPowerHandling>()->ActivateScreenSaver();
    break;

  case TMSG_RESETSCREENSAVER:
    GetComponent<CApplicationPowerHandling>()->m_bResetScreenSaver = true;
    break;

  case TMSG_VOLUME_SHOW:
  {
    CAction action(pMsg->param1);
    GetComponent<CApplicationVolumeHandling>()->ShowVolumeBar(&action);
  }
  break;

#ifdef TARGET_ANDROID
  case TMSG_DISPLAY_SETUP:
    // We might come from a refresh rate switch destroying the native window; use the context resolution
    *static_cast<bool*>(pMsg->lpVoid) = InitWindow(CServiceBroker::GetWinSystem()->GetGfxContext().GetVideoResolution());
    GetComponent<CApplicationPowerHandling>()->SetRenderGUI(true);
    break;

  case TMSG_DISPLAY_DESTROY:
    *static_cast<bool*>(pMsg->lpVoid) = CServiceBroker::GetWinSystem()->DestroyWindow();
    GetComponent<CApplicationPowerHandling>()->SetRenderGUI(false);
    break;
#endif

  case TMSG_START_ANDROID_ACTIVITY:
  {
#if defined(TARGET_ANDROID)
    if (pMsg->params.size())
    {
      CXBMCApp::StartActivity(pMsg->params[0], pMsg->params.size() > 1 ? pMsg->params[1] : "",
                              pMsg->params.size() > 2 ? pMsg->params[2] : "",
                              pMsg->params.size() > 3 ? pMsg->params[3] : "",
                              pMsg->params.size() > 4 ? pMsg->params[4] : "",
                              pMsg->params.size() > 5 ? pMsg->params[5] : "",
                              pMsg->params.size() > 6 ? pMsg->params[6] : "",
                              pMsg->params.size() > 7 ? pMsg->params[7] : "",
                              pMsg->params.size() > 8 ? pMsg->params[8] : "");
    }
#endif
  }
  break;

  case TMSG_NETWORKMESSAGE:
    m_ServiceManager->GetNetwork().NetworkMessage(static_cast<CNetworkBase::EMESSAGE>(pMsg->param1),
                                                  pMsg->param2);
    break;

  case TMSG_SETLANGUAGE:
    SetLanguage(pMsg->strParam);
    break;


  case TMSG_SWITCHTOFULLSCREEN:
  {
    CGUIComponent* gui = CServiceBroker::GetGUI();
    if (gui)
      gui->GetWindowManager().SwitchToFullScreen(true);
    break;
  }
  case TMSG_VIDEORESIZE:
  {
    XBMC_Event newEvent = {};
    newEvent.type = XBMC_VIDEORESIZE;
    newEvent.resize.w = pMsg->param1;
    newEvent.resize.h = pMsg->param2;
    m_pAppPort->OnEvent(newEvent);
    CServiceBroker::GetGUI()->GetWindowManager().MarkDirty();
  }
    break;

  case TMSG_SETVIDEORESOLUTION:
    CServiceBroker::GetWinSystem()->GetGfxContext().SetVideoResolution(static_cast<RESOLUTION>(pMsg->param1), pMsg->param2 == 1);
    break;

  case TMSG_TOGGLEFULLSCREEN:
    CServiceBroker::GetWinSystem()->GetGfxContext().ToggleFullScreen();
    appPlayer->TriggerUpdateResolution();
    break;

  case TMSG_MOVETOSCREEN:
    CServiceBroker::GetWinSystem()->MoveToScreen(static_cast<int>(pMsg->param1));
    break;

  case TMSG_MINIMIZE:
    CServiceBroker::GetWinSystem()->Minimize();
    break;

  case TMSG_EXECUTE_OS:
    // Suspend AE temporarily so exclusive or hog-mode sinks
    // don't block external player's access to audio device
    IAE *audioengine;
    audioengine = CServiceBroker::GetActiveAE();
    if (audioengine)
    {
      if (!audioengine->Suspend())
      {
        CLog::Log(LOGINFO, "{}: Failed to suspend AudioEngine before launching external program",
                  __FUNCTION__);
      }
    }
#if defined(TARGET_DARWIN)
    CLog::Log(LOGINFO, "ExecWait is not implemented on this platform");
#elif defined(TARGET_POSIX)
    CUtil::RunCommandLine(pMsg->strParam, (pMsg->param1 == 1));
#elif defined(TARGET_WINDOWS)
    CWIN32Util::XBMCShellExecute(pMsg->strParam.c_str(), (pMsg->param1 == 1));
#endif
    // Resume AE processing of XBMC native audio
    if (audioengine)
    {
      if (!audioengine->Resume())
      {
        CLog::Log(LOGFATAL, "{}: Failed to restart AudioEngine after return from external player",
                  __FUNCTION__);
      }
    }
    break;

  case TMSG_EXECUTE_SCRIPT:
    CScriptInvocationManager::GetInstance().ExecuteAsync(pMsg->strParam);
    break;

  case TMSG_EXECUTE_BUILT_IN:
    CBuiltins::GetInstance().Execute(pMsg->strParam);
    break;

  case TMSG_PICTURE_SHOW:
  {
    CSlideShowDelegator& slideShow = CServiceBroker::GetSlideShowDelegator();

    // stop playing file
    if (appPlayer->IsPlayingVideo())
      StopPlaying();

    if (CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_FULLSCREEN_VIDEO)
      CServiceBroker::GetGUI()->GetWindowManager().PreviousWindow();

    const auto appPower = GetComponent<CApplicationPowerHandling>();
    appPower->ResetScreenSaver();
    appPower->WakeUpScreenSaverAndDPMS();

    if (CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() != WINDOW_SLIDESHOW)
      CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(WINDOW_SLIDESHOW);
    if (URIUtils::IsZIP(pMsg->strParam) || URIUtils::IsRAR(pMsg->strParam)) // actually a cbz/cbr
    {
      CFileItemList items;
      CURL pathToUrl;
      if (URIUtils::IsZIP(pMsg->strParam))
        pathToUrl = URIUtils::CreateArchivePath("zip", CURL(pMsg->strParam), "");
      else
        pathToUrl = URIUtils::CreateArchivePath("rar", CURL(pMsg->strParam), "");

      CUtil::GetRecursiveListing(pathToUrl.Get(), items, CServiceBroker::GetFileExtensionProvider().GetPictureExtensions(), XFILE::DIR_FLAG_NO_FILE_DIRS);
      if (items.Size() > 0)
      {
        slideShow.Reset();
        for (int i = 0; i<items.Size(); ++i)
        {
          slideShow.Add(items[i].get());
        }
        slideShow.Select(items[0]->GetPath());
      }
    }
    else
    {
      CFileItem item(pMsg->strParam, false);
      slideShow.Reset();
      slideShow.Add(&item);
      slideShow.Select(pMsg->strParam);
    }
  }
  break;

  case TMSG_PICTURE_SLIDESHOW:
  {
    CSlideShowDelegator& slideShow = CServiceBroker::GetSlideShowDelegator();

    if (appPlayer->IsPlayingVideo())
      StopPlaying();

    slideShow.Reset();

    CFileItemList items;
    std::string strPath = pMsg->strParam;
    std::string extensions = CServiceBroker::GetFileExtensionProvider().GetPictureExtensions();
    if (pMsg->param1)
      extensions += "|.tbn";
    CUtil::GetRecursiveListing(strPath, items, extensions);

    if (items.Size() > 0)
    {
      for (int i = 0; i<items.Size(); ++i)
        slideShow.Add(items[i].get());
      slideShow.StartSlideShow(); //Start the slideshow!
    }

    if (CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() != WINDOW_SLIDESHOW)
    {
      if (items.Size() == 0)
      {
        CServiceBroker::GetSettingsComponent()->GetSettings()->SetString(CSettings::SETTING_SCREENSAVER_MODE, "screensaver.xbmc.builtin.dim");
        GetComponent<CApplicationPowerHandling>()->ActivateScreenSaver();
      }
      else
        CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(WINDOW_SLIDESHOW);
    }

  }
  break;

  case TMSG_LOADPROFILE:
    {
      const int profile = pMsg->param1;
      if (profile >= 0)
        CServiceBroker::GetSettingsComponent()->GetProfileManager()->LoadProfile(static_cast<unsigned int>(profile));
    }

    break;

  case TMSG_EVENT:
  {
    if (pMsg->lpVoid)
    {
      XBMC_Event* event = static_cast<XBMC_Event*>(pMsg->lpVoid);
      m_pAppPort->OnEvent(*event);
      delete event;
    }
  }
  break;

  case TMSG_UPDATE_PLAYER_ITEM:
  {
    std::unique_ptr<CFileItem> item{static_cast<CFileItem*>(pMsg->lpVoid)};
    if (item)
    {
      m_itemCurrentFile->UpdateInfo(*item);
      CServiceBroker::GetGUI()->GetInfoManager().UpdateCurrentItem(*m_itemCurrentFile);
    }
  }
  break;

  case TMSG_SET_VOLUME:
  {
    const float volumedB = static_cast<float>(pMsg->param3);
    GetComponent<CApplicationVolumeHandling>()->SetVolume(volumedB);
  }
  break;

  case TMSG_SET_MUTE:
  {
    GetComponent<CApplicationVolumeHandling>()->SetMute(pMsg->param3 == 1 ? true : false);
  }
  break;

  default:
    CLog::Log(LOGERROR, "{}: Unhandled threadmessage sent, {}", __FUNCTION__, msg);
    break;
  }
}

void CApplication::LockFrameMoveGuard()
{
  ++m_WaitingExternalCalls;
  m_frameMoveGuard.lock();
  ++m_ProcessedExternalCalls;
  CServiceBroker::GetWinSystem()->GetGfxContext().lock();
};

void CApplication::UnlockFrameMoveGuard()
{
  --m_WaitingExternalCalls;
  CServiceBroker::GetWinSystem()->GetGfxContext().unlock();
  m_frameMoveGuard.unlock();
};

void CApplication::FrameMove(bool processEvents, bool processGUI)
{
  const auto appPlayer = GetComponent<CApplicationPlayer>();
  bool renderGUI = GetComponent<CApplicationPowerHandling>()->GetRenderGUI();
  if (processEvents)
  {
    // currently we calculate the repeat time (ie time from last similar keypress) just global as fps
    float frameTime = m_frameTime.GetElapsedSeconds();
    m_frameTime.StartZero();
    // never set a frametime less than 2 fps to avoid problems when debugging and on breaks
    if (frameTime > 0.5f)
      frameTime = 0.5f;

    if (processGUI && renderGUI)
    {
      std::unique_lock<CCriticalSection> lock(CServiceBroker::GetWinSystem()->GetGfxContext());
      // check if there are notifications to display
      CGUIDialogKaiToast *toast = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogKaiToast>(WINDOW_DIALOG_KAI_TOAST);
      if (toast && toast->DoWork())
      {
        if (!toast->IsDialogRunning())
        {
          toast->Open();
        }
      }
    }

    m_pAppPort->HandleEvents();
    CServiceBroker::GetInputManager().Process(CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindowOrDialog(), frameTime);

    if (processGUI && renderGUI)
    {
      m_pInertialScrollingHandler->ProcessInertialScroll(frameTime);
      appPlayer->GetSeekHandler().FrameMove();
    }

    // Open the door for external calls e.g python exactly here.
    // Window size can be between 2 and 10ms and depends on number of continuous requests
    if (m_WaitingExternalCalls)
    {
      CSingleExit ex(CServiceBroker::GetWinSystem()->GetGfxContext());
      m_frameMoveGuard.unlock();

      // Calculate a window size between 2 and 10ms, 4 continuous requests let the window grow by 1ms
      // When not playing video we allow it to increase to 80ms
      unsigned int max_sleep = 10;
      if (!appPlayer->IsPlayingVideo() || appPlayer->IsPausedPlayback())
        max_sleep = 80;
      unsigned int sleepTime = std::max(static_cast<unsigned int>(2), std::min(m_ProcessedExternalCalls >> 2, max_sleep));
      KODI::TIME::Sleep(std::chrono::milliseconds(sleepTime));
      m_frameMoveGuard.lock();
      m_ProcessedExternalDecay = 5;
    }
    if (m_ProcessedExternalDecay && --m_ProcessedExternalDecay == 0)
      m_ProcessedExternalCalls = 0;
  }

  if (processGUI && renderGUI)
  {
    m_skipGuiRender = false;

    /*! @todo look into the possibility to use this for GBM
    int fps = 0;

    // This code reduces rendering fps of the GUI layer when playing videos in fullscreen mode
    // it makes only sense on architectures with multiple layers
    if (CServiceBroker::GetWinSystem()->GetGfxContext().IsFullScreenVideo() && !m_appPlayer.IsPausedPlayback() && m_appPlayer.IsRenderingVideoLayer())
      fps = CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_VIDEOPLAYER_LIMITGUIUPDATE);

    auto now = std::chrono::steady_clock::now();

    auto frameTime = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastRenderTime).count();
    if (fps > 0 && frameTime * fps < 1000)
      m_skipGuiRender = true;
    */

    if (CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_guiSmartRedraw && m_guiRefreshTimer.IsTimePast())
    {
      CServiceBroker::GetGUI()->GetWindowManager().SendMessage(GUI_MSG_REFRESH_TIMER, 0, 0);
      m_guiRefreshTimer.Set(500ms);
    }

    if (!m_bStop)
    {
      if (!m_skipGuiRender)
        CServiceBroker::GetGUI()->GetWindowManager().Process(CTimeUtils::GetFrameTime());
    }
    CServiceBroker::GetGUI()->GetWindowManager().FrameMove();
  }

  appPlayer->FrameMove();

  // this will go away when render systems gets its own thread
  CServiceBroker::GetWinSystem()->DriveRenderLoop();
}


void CApplication::ResetCurrentItem()
{
  m_itemCurrentFile->Reset();
  if (m_pGUI)
    m_pGUI->GetInfoManager().ResetCurrentItem();
}

int CApplication::Run()
{
  CLog::Log(LOGINFO, "Running the application...");

  std::chrono::time_point<std::chrono::steady_clock> lastFrameTime;
  std::chrono::milliseconds frameTime;
  const unsigned int noRenderFrameTime = 15; // Simulates ~66fps

  CFileItemList& playlist = CServiceBroker::GetAppParams()->GetPlaylist();
  if (playlist.Size() > 0)
  {
    CServiceBroker::GetPlaylistPlayer().Add(PLAYLIST::TYPE_MUSIC, playlist);
    CServiceBroker::GetPlaylistPlayer().SetCurrentPlaylist(PLAYLIST::TYPE_MUSIC);
    CServiceBroker::GetAppMessenger()->PostMsg(TMSG_PLAYLISTPLAYER_PLAY, -1);
  }

  // Run the app
  while (!m_bStop)
  {
    // Animate and render a frame

    lastFrameTime = std::chrono::steady_clock::now();
    Process();

    bool renderGUI = GetComponent<CApplicationPowerHandling>()->GetRenderGUI();
    if (!m_bStop)
    {
      FrameMove(true, renderGUI);
    }

    if (renderGUI && !m_bStop)
    {
      Render();
    }
    else if (!renderGUI)
    {
      auto now = std::chrono::steady_clock::now();
      frameTime = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastFrameTime);
      if (frameTime.count() < noRenderFrameTime)
        KODI::TIME::Sleep(std::chrono::milliseconds(noRenderFrameTime - frameTime.count()));
    }
  }

  Cleanup();

  CLog::Log(LOGINFO, "Exiting the application...");
  return m_ExitCode;
}

bool CApplication::Cleanup()
{
  try
  {
    ResetCurrentItem();
    StopPlaying();

    if (m_ServiceManager)
      m_ServiceManager->DeinitStageThree();

    CServiceBroker::UnregisterSpeechRecognition();

    CLog::Log(LOGINFO, "unload skin");
    GetComponent<CApplicationSkinHandling>()->UnloadSkin();

    CServiceBroker::UnregisterTextureCache();

    // stop all remaining scripts; must be done after skin has been unloaded,
    // not before some windows still need it when deinitializing during skin
    // unloading
    CScriptInvocationManager::GetInstance().Uninitialize();

    const auto appPower = GetComponent<CApplicationPowerHandling>();
    appPower->m_globalScreensaverInhibitor.Release();
    appPower->m_screensaverInhibitor.Release();

    CRenderSystemBase *renderSystem = CServiceBroker::GetRenderSystem();
    if (renderSystem)
      renderSystem->DestroyRenderSystem();

    CWinSystemBase *winSystem = CServiceBroker::GetWinSystem();
    if (winSystem)
      winSystem->DestroyWindow();

    if (m_pGUI)
      m_pGUI->GetWindowManager().DestroyWindows();

    CLog::Log(LOGINFO, "unload sections");

    //  Shutdown as much as possible of the
    //  application, to reduce the leaks dumped
    //  to the vc output window before calling
    //  _CrtDumpMemoryLeaks(). Most of the leaks
    //  shown are no real leaks, as parts of the app
    //  are still allocated.

    g_localizeStrings.Clear();
    g_LangCodeExpander.Clear();
    g_charsetConverter.clear();
    g_directoryCache.Clear();
    //CServiceBroker::GetInputManager().ClearKeymaps(); //! @todo
    CEventServer::RemoveInstance();
    CServiceBroker::GetPlaylistPlayer().Clear();

    if (m_ServiceManager)
      m_ServiceManager->DeinitStageTwo();

#ifdef TARGET_POSIX
    CXHandle::DumpObjectTracker();

#ifdef HAS_OPTICAL_DRIVE
    CLibcdio::ReleaseInstance();
#endif
#endif
#ifdef _CRTDBG_MAP_ALLOC
    _CrtDumpMemoryLeaks();
    while(1); // execution ends
#endif

    if (m_pGUI)
    {
      m_pGUI->Deinit();
      m_pGUI.reset();
    }

    if (winSystem)
    {
      winSystem->DestroyWindowSystem();
      CServiceBroker::UnregisterWinSystem();
      winSystem = nullptr;
      m_pWinSystem.reset();
    }

    // Cleanup was called more than once on exit during my tests
    if (m_ServiceManager)
    {
      m_ServiceManager->DeinitStageOne();
      m_ServiceManager.reset();
    }

    CServiceBroker::UnregisterKeyboardLayoutManager();

    CServiceBroker::UnregisterAppMessenger();

    CServiceBroker::UnregisterAnnouncementManager();
    m_pAnnouncementManager->Deinitialize();
    m_pAnnouncementManager.reset();

    CServiceBroker::UnregisterJobManager();
    CServiceBroker::UnregisterCPUInfo();

    UnregisterSettings();

    m_bInitializing = true;

    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "Exception in CApplication::Cleanup()");
    return false;
  }
}

bool CApplication::Stop(int exitCode)
{
#if defined(TARGET_ANDROID)
  // Note: On Android, the app must be stopped asynchronously, once Android has
  // signalled that the app shall be destroyed. See android_main() implementation.
  if (!CXBMCApp::Get().Stop(exitCode))
    return false;
#endif

  CLog::Log(LOGINFO, "Stopping the application...");

  bool success = true;

  CLog::Log(LOGINFO, "Stopping player");
  const auto appPlayer = GetComponent<CApplicationPlayer>();
  appPlayer->ClosePlayer();

  {
    // close inbound port
    CServiceBroker::UnregisterAppPort();
    XbmcThreads::EndTime<> timer(1000ms);
    while (m_pAppPort.use_count() > 1)
    {
      KODI::TIME::Sleep(100ms);
      if (timer.IsTimePast())
      {
        CLog::Log(LOGERROR, "CApplication::Stop - CAppPort still in use, app may crash");
        break;
      }
    }
    m_pAppPort->Close();
  }

  try
  {
    m_frameMoveGuard.unlock();

    CVariant vExitCode(CVariant::VariantTypeObject);
    vExitCode["exitcode"] = exitCode;
    CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::System, "OnQuit", vExitCode);

    // Abort any active screensaver
    GetComponent<CApplicationPowerHandling>()->WakeUpScreenSaverAndDPMS();

    g_alarmClock.StopThread();

    CLog::Log(LOGINFO, "Storing total System Uptime");
    g_sysinfo.SetTotalUptime(g_sysinfo.GetTotalUptime() + (int)(CTimeUtils::GetFrameTime() / 60000));

    // Update the settings information (volume, uptime etc. need saving)
    if (CFile::Exists(CServiceBroker::GetSettingsComponent()->GetProfileManager()->GetSettingsFile()))
    {
      CLog::Log(LOGINFO, "Saving settings");
      CServiceBroker::GetSettingsComponent()->GetSettings()->Save();
    }
    else
      CLog::Log(LOGINFO, "Not saving settings (settings.xml is not present)");

    // kodi may crash or deadlock during exit (shutdown / reboot) due to
    // either a bug in core or misbehaving addons. so try saving
    // skin settings early
    CLog::Log(LOGINFO, "Saving skin settings");
    if (g_SkinInfo != nullptr)
      g_SkinInfo->SaveSettings();

    m_bStop = true;
    // Add this here to keep the same ordering behaviour for now
    // Needs cleaning up
    CServiceBroker::GetAppMessenger()->Stop();
    m_AppFocused = false;
    m_ExitCode = exitCode;
    CLog::Log(LOGINFO, "Stopping all");

    // cancel any jobs from the jobmanager
    CServiceBroker::GetJobManager()->CancelJobs();

    // stop scanning before we kill the network and so on
    if (CMusicLibraryQueue::GetInstance().IsRunning())
      CMusicLibraryQueue::GetInstance().CancelAllJobs();

    if (CVideoLibraryQueue::GetInstance().IsRunning())
      CVideoLibraryQueue::GetInstance().CancelAllJobs();

    CServiceBroker::GetAppMessenger()->Cleanup();

    m_ServiceManager->GetNetwork().NetworkMessage(CNetworkBase::SERVICES_DOWN, 0);

#ifdef HAS_ZEROCONF
    if(CZeroconfBrowser::IsInstantiated())
    {
      CLog::Log(LOGINFO, "Stopping zeroconf browser");
      CZeroconfBrowser::GetInstance()->Stop();
      CZeroconfBrowser::ReleaseInstance();
    }
#endif

    for (const auto& vfsAddon : CServiceBroker::GetVFSAddonCache().GetAddonInstances())
      vfsAddon->DisconnectAll();

#if defined(TARGET_POSIX) && defined(HAS_FILESYSTEM_SMB)
    smb.Deinit();
#endif

#if defined(TARGET_DARWIN_OSX) and defined(HAS_XBMCHELPER)
    if (XBMCHelper::GetInstance().IsAlwaysOn() == false)
      XBMCHelper::GetInstance().Stop();
#endif

    // Stop services before unloading Python
    CServiceBroker::GetServiceAddons().Stop();

    // Stop any other python scripts that may be looping waiting for monitor.abortRequested()
    CScriptInvocationManager::GetInstance().StopRunningScripts();

    // unregister action listeners
    const auto appListener = GetComponent<CApplicationActionListeners>();
    appListener->UnregisterActionListener(&GetComponent<CApplicationPlayer>()->GetSeekHandler());
    appListener->UnregisterActionListener(&CPlayerController::GetInstance());

    CGUIComponent *gui = CServiceBroker::GetGUI();
    if (gui)
      gui->GetAudioManager().DeInitialize();

    // shutdown the AudioEngine
    CServiceBroker::UnregisterAE();
    m_pActiveAE->Shutdown();
    m_pActiveAE.reset();

    CLog::Log(LOGINFO, "Application stopped");
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "Exception in CApplication::Stop()");
    success = false;
  }

  cleanup_emu_environ();

  KODI::TIME::Sleep(200ms);

  return success;
}

namespace
{
class CCreateAndLoadPlayList : public IRunnable
{
public:
  CCreateAndLoadPlayList(CFileItem& item, std::unique_ptr<PLAYLIST::CPlayList>& playlist)
    : m_item(item), m_playlist(playlist)
  {
  }

  void Run() override
  {
    const std::unique_ptr<PLAYLIST::CPlayList> playlist(PLAYLIST::CPlayListFactory::Create(m_item));
    if (playlist)
    {
      if (playlist->Load(m_item.GetPath()))
        *m_playlist = *playlist;
    }
  }

private:
  CFileItem& m_item;
  std::unique_ptr<PLAYLIST::CPlayList>& m_playlist;
};
} // namespace

bool CApplication::PlayMedia(CFileItem& item, const std::string& player, PLAYLIST::Id playlistId)
{
  // if the item is a plugin we need to resolve the plugin paths
  if (URIUtils::HasPluginPath(item) && !XFILE::CPluginDirectory::GetResolvedPluginResult(item))
    return false;

  if (item.IsSmartPlayList())
  {
    CFileItemList items;
    CUtil::GetRecursiveListing(item.GetPath(), items, "", DIR_FLAG_NO_FILE_DIRS);
    if (items.Size())
    {
      CSmartPlaylist smartpl;
      //get name and type of smartplaylist, this will always succeed as GetDirectory also did this.
      smartpl.OpenAndReadName(item.GetURL());
      PLAYLIST::CPlayList playlist;
      playlist.Add(items);
      PLAYLIST::Id smartplPlaylistId = PLAYLIST::TYPE_VIDEO;

      if (smartpl.GetType() == "songs" || smartpl.GetType() == "albums" ||
          smartpl.GetType() == "artists")
        smartplPlaylistId = PLAYLIST::TYPE_MUSIC;

      return ProcessAndStartPlaylist(smartpl.GetName(), playlist, smartplPlaylistId);
    }
  }
  else if (item.IsPlayList() || NETWORK::IsInternetStream(item))
  {
    // Not owner. Dialog auto-deletes itself.
    CGUIDialogCache* dlgCache =
        new CGUIDialogCache(5s, g_localizeStrings.Get(10214), item.GetLabel());

    //is or could be a playlist
    std::unique_ptr<PLAYLIST::CPlayList> playlist;
    CCreateAndLoadPlayList getPlaylist(item, playlist);
    bool cancelled = !CGUIDialogBusy::Wait(&getPlaylist, 100, true);

    if (dlgCache)
    {
      dlgCache->Close();
      if (dlgCache->IsCanceled())
        cancelled = true;
    }

    if (cancelled)
      return true;

    if (playlist)
    {

      if (playlistId != PLAYLIST::TYPE_NONE)
      {
        int track=0;
        if (item.HasProperty("playlist_starting_track"))
          track = (int)item.GetProperty("playlist_starting_track").asInteger();
        return ProcessAndStartPlaylist(item.GetPath(), *playlist, playlistId, track);
      }
      else
      {
        CLog::Log(LOGWARNING,
                  "CApplication::PlayMedia called to play a playlist {} but no idea which playlist "
                  "to use, playing first item",
                  item.GetPath());
        if (playlist->size())
          return PlayFile(*(*playlist)[0], "", false);
      }
    }
  }
  else if (item.IsPVR())
  {
    return CServiceBroker::GetPVRManager().Get<PVR::GUI::Playback>().PlayMedia(item);
  }

  CURL path(item.GetPath());
  if (path.GetProtocol() == "game")
  {
    AddonPtr addon;
    if (CServiceBroker::GetAddonMgr().GetAddon(path.GetHostName(), addon, AddonType::GAMEDLL,
                                               OnlyEnabled::CHOICE_YES))
    {
      CFileItem addonItem(addon);
      return PlayFile(addonItem, player, false);
    }
  }

  //nothing special just play
  return PlayFile(item, player, false);
}

// PlayStack()
// For playing a multi-file video.  Particularly inefficient
// on startup, as we are required to calculate the length
// of each video, so we open + close each one in turn.
// A faster calculation of video time would improve this
// substantially.
// return value: same with PlayFile()
bool CApplication::PlayStack(CFileItem& item, bool bRestart)
{
  const auto stackHelper = GetComponent<CApplicationStackHelper>();
  if (!stackHelper->InitializeStack(item))
    return false;

  std::optional<int64_t> startoffset = stackHelper->InitializeStackStartPartAndOffset(item);
  if (!startoffset)
  {
    CLog::LogF(LOGERROR, "Failed to obtain start offset for stack {}. Aborting playback.",
               item.GetDynPath());
    return false;
  }

  CFileItem selectedStackPart = stackHelper->GetCurrentStackPartFileItem();
  selectedStackPart.SetStartOffset(startoffset.value());

  if (item.HasProperty("savedplayerstate"))
  {
    selectedStackPart.SetProperty("savedplayerstate", item.GetProperty("savedplayerstate")); // pass on to part
    item.ClearProperty("savedplayerstate");
  }

  return PlayFile(selectedStackPart, "", true);
}

bool CApplication::PlayFile(CFileItem item,
                            const std::string& player,
                            bool bRestart /* = false */,
                            bool forceSelection /* = false */)
{
  // Ensure the MIME type has been retrieved for http:// and shout:// streams
  if (item.GetMimeType().empty())
    item.FillInMimeType();

  const auto appPlayer = GetComponent<CApplicationPlayer>();
  const auto stackHelper = GetComponent<CApplicationStackHelper>();

  if (!bRestart)
  {
    // bRestart will be true when called from PlayStack(), skipping this block
    appPlayer->SetPlaySpeed(1);

    m_nextPlaylistItem = -1;
    stackHelper->Clear();

    if (VIDEO::IsVideo(item))
      CUtil::ClearSubtitles();
  }

  if (VIDEO::IsDiscStub(item))
  {
    return CServiceBroker::GetMediaManager().playStubFile(item);
  }

  if (item.IsPlayList())
    return false;

  // Translate/Resolve the url if needed
  const std::unique_ptr<IDirectory> dir{CDirectoryFactory::Create(item)};
  if (dir && !dir->Resolve(item))
  {
    return false;
  }

  // if we have a stacked set of files, we need to setup our stack routines for
  // "seamless" seeking and total time of the movie etc.
  // will recall with restart set to true
  if (item.IsStack())
    return PlayStack(item, bRestart);

  CPlayerOptions options;

  if (item.HasProperty("StartPercent"))
  {
    options.startpercent = item.GetProperty("StartPercent").asDouble();
    item.SetStartOffset(0);
  }

  options.starttime = CUtil::ConvertMilliSecsToSecs(item.GetStartOffset());

  if (bRestart)
  {
    // have to be set here due to playstack using this for starting the file
    if (item.HasVideoInfoTag())
      options.state = item.GetVideoInfoTag()->GetResumePoint().playerState;
  }
  if (!bRestart || stackHelper->IsPlayingISOStack())
  {
    // the following code block is only applicable when bRestart is false OR to ISO stacks

    if (VIDEO::IsVideo(item))
    {
      // open the d/b and retrieve the bookmarks for the current movie
      CVideoDatabase dbs;
      dbs.Open();

      std::string path = item.GetPath();
      std::string videoInfoTagPath(item.GetVideoInfoTag()->m_strFileNameAndPath);
      if (videoInfoTagPath.find("removable://") == 0 || VIDEO::IsVideoDb(item))
        path = videoInfoTagPath;

      // Note that we need to load the tag from database also if the item already has a tag,
      // because for example the (full) video info for strm files will be loaded here.
      dbs.LoadVideoInfo(path, *item.GetVideoInfoTag());

      if (item.HasProperty("savedplayerstate"))
      {
        options.starttime = CUtil::ConvertMilliSecsToSecs(item.GetStartOffset());
        options.state = item.GetProperty("savedplayerstate").asString();
        item.ClearProperty("savedplayerstate");
      }
      else if (item.GetStartOffset() == STARTOFFSET_RESUME)
      {
        options.starttime = 0.0;
        if (item.IsResumePointSet())
        {
          options.starttime = item.GetCurrentResumeTime();
          if (item.HasVideoInfoTag())
            options.state = item.GetVideoInfoTag()->GetResumePoint().playerState;
        }
        else
        {
          CBookmark bookmark;
          std::string path = item.GetPath();
          if (item.HasVideoInfoTag() && StringUtils::StartsWith(item.GetVideoInfoTag()->m_strFileNameAndPath, "removable://"))
            path = item.GetVideoInfoTag()->m_strFileNameAndPath;
          else if (item.HasProperty("original_listitem_url") && URIUtils::IsPlugin(item.GetProperty("original_listitem_url").asString()))
            path = item.GetProperty("original_listitem_url").asString();
          if (dbs.GetResumeBookMark(path, bookmark))
          {
            options.starttime = bookmark.timeInSeconds;
            options.state = bookmark.playerState;
          }
        }

        if (options.starttime == 0.0 && item.HasVideoInfoTag())
        {
          // No resume point is set, but check if this item is part of a multi-episode file
          const CVideoInfoTag *tag = item.GetVideoInfoTag();

          if (tag->m_iBookmarkId > 0)
          {
            CBookmark bookmark;
            dbs.GetBookMarkForEpisode(*tag, bookmark);
            options.starttime = bookmark.timeInSeconds;
            options.state = bookmark.playerState;
          }
        }
      }
      else if (item.HasVideoInfoTag())
      {
        const CVideoInfoTag *tag = item.GetVideoInfoTag();

        if (tag->m_iBookmarkId > 0)
        {
          CBookmark bookmark;
          dbs.GetBookMarkForEpisode(*tag, bookmark);
          options.starttime = bookmark.timeInSeconds;
          options.state = bookmark.playerState;
        }
      }

      dbs.Close();
    }
  }

  // a disc image might be Blu-Ray disc
  if (!(options.startpercent > 0.0 || options.starttime > 0.0) &&
      (VIDEO::IsBDFile(item) || item.IsDiscImage() ||
       (forceSelection && VIDEO::IsBlurayPlaylist(item))))
  {
    // No video selection when using external or remote players (they handle it if supported)
    const bool isSimpleMenuAllowed = [&]()
    {
      const std::string defaulPlayer{
          player.empty() ? m_ServiceManager->GetPlayerCoreFactory().GetDefaultPlayer(item)
                         : player};
      const bool isExternalPlayer{
          m_ServiceManager->GetPlayerCoreFactory().IsExternalPlayer(defaulPlayer)};
      const bool isRemotePlayer{
          m_ServiceManager->GetPlayerCoreFactory().IsRemotePlayer(defaulPlayer)};
      return !isExternalPlayer && !isRemotePlayer;
    }();

    if (isSimpleMenuAllowed)
    {
      // Check if we must show the simplified bd menu.
      if (!CGUIDialogSimpleMenu::ShowPlaySelection(item, forceSelection))
        return true;
    }
  }

  // this really aught to be inside !bRestart, but since PlayStack
  // uses that to init playback, we have to keep it outside
  const PLAYLIST::Id playlistId = CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist();
  if (MUSIC::IsAudio(item) && playlistId == PLAYLIST::TYPE_MUSIC)
  { // playing from a playlist by the looks
    // don't switch to fullscreen if we are not playing the first item...
    options.fullscreen = !CServiceBroker::GetPlaylistPlayer().HasPlayedFirstFile() &&
        CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
        CSettings::SETTING_MUSICFILES_SELECTACTION) &&
        !CMediaSettings::GetInstance().DoesMediaStartWindowed();
  }
  else if (VIDEO::IsVideo(item) && playlistId == PLAYLIST::TYPE_VIDEO &&
           CServiceBroker::GetPlaylistPlayer().GetPlaylist(playlistId).size() > 1)
  { // playing from a playlist by the looks
    // don't switch to fullscreen if we are not playing the first item...
    options.fullscreen = !CServiceBroker::GetPlaylistPlayer().HasPlayedFirstFile() &&
        CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_fullScreenOnMovieStart &&
        !CMediaSettings::GetInstance().DoesMediaStartWindowed();
  }
  else if (stackHelper->IsPlayingRegularStack())
  {
    //! @todo - this will fail if user seeks back to first file in stack
    if (stackHelper->GetCurrentPartNumber() == 0 ||
        stackHelper->GetRegisteredStack(item)->GetStartOffset() != 0)
      options.fullscreen = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->
          m_fullScreenOnMovieStart && !CMediaSettings::GetInstance().DoesMediaStartWindowed();
    else
      options.fullscreen = false;
  }
  else
    options.fullscreen = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->
        m_fullScreenOnMovieStart && !CMediaSettings::GetInstance().DoesMediaStartWindowed();

  // stereo streams may have lower quality, i.e. 32bit vs 16 bit
  options.preferStereo = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoPreferStereoStream &&
                         CServiceBroker::GetActiveAE()->HasStereoAudioChannelCount();

  // reset VideoStartWindowed as it's a temp setting
  CMediaSettings::GetInstance().SetMediaStartWindowed(false);

  {
    // for playing a new item, previous playing item's callback may already
    // pushed some delay message into the threadmessage list, they are not
    // expected be processed after or during the new item playback starting.
    // so we clean up previous playing item's playback callback delay messages here.
    int previousMsgsIgnoredByNewPlaying[] = {
      GUI_MSG_PLAYBACK_STARTED,
      GUI_MSG_PLAYBACK_ENDED,
      GUI_MSG_PLAYBACK_STOPPED,
      GUI_MSG_PLAYLIST_CHANGED,
      GUI_MSG_PLAYLISTPLAYER_STOPPED,
      GUI_MSG_PLAYLISTPLAYER_STARTED,
      GUI_MSG_PLAYLISTPLAYER_CHANGED,
      GUI_MSG_QUEUE_NEXT_ITEM,
      0
    };
    int dMsgCount = CServiceBroker::GetGUI()->GetWindowManager().RemoveThreadMessageByMessageIds(&previousMsgsIgnoredByNewPlaying[0]);
    if (dMsgCount > 0)
      CLog::LogF(LOGDEBUG, "Ignored {} playback thread messages", dMsgCount);
  }

  const auto appVolume = GetComponent<CApplicationVolumeHandling>();
  appPlayer->OpenFile(item, options, m_ServiceManager->GetPlayerCoreFactory(), player, *this);
  appPlayer->SetVolume(appVolume->GetVolumeRatio());
  appPlayer->SetMute(appVolume->IsMuted());

#if !defined(TARGET_POSIX)
  CGUIComponent *gui = CServiceBroker::GetGUI();
  if (gui)
    gui->GetAudioManager().Enable(false);
#endif

  if (item.HasPVRChannelInfoTag())
    CServiceBroker::GetPlaylistPlayer().SetCurrentPlaylist(PLAYLIST::TYPE_NONE);

  return true;
}

void CApplication::PlaybackCleanup()
{
  const auto appPlayer = GetComponent<CApplicationPlayer>();
  const auto stackHelper = GetComponent<CApplicationStackHelper>();

  if (!appPlayer->IsPlaying())
  {
    CGUIComponent *gui = CServiceBroker::GetGUI();
    if (gui)
      CServiceBroker::GetGUI()->GetAudioManager().Enable(true);
    appPlayer->OpenNext(m_ServiceManager->GetPlayerCoreFactory());
  }

  if (!appPlayer->IsPlayingVideo())
  {
    if(CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_FULLSCREEN_VIDEO ||
       CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_FULLSCREEN_GAME)
    {
      CServiceBroker::GetGUI()->GetWindowManager().PreviousWindow();
    }
    else
    {
      //  resets to res_desktop or look&feel resolution (including refreshrate)
      CServiceBroker::GetWinSystem()->GetGfxContext().SetFullScreenVideo(false);
    }
#ifdef TARGET_DARWIN_EMBEDDED
    CDarwinUtils::SetScheduling(false);
#endif
  }

  const auto appPower = GetComponent<CApplicationPowerHandling>();

  if (!appPlayer->IsPlayingAudio() &&
      CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist() == PLAYLIST::TYPE_NONE &&
      CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_VISUALISATION)
  {
    CServiceBroker::GetSettingsComponent()->GetSettings()->Save();  // save vis settings
    appPower->WakeUpScreenSaverAndDPMS();
    CServiceBroker::GetGUI()->GetWindowManager().PreviousWindow();
  }

  // DVD ejected while playing in vis ?
  if (!appPlayer->IsPlayingAudio() &&
      (MUSIC::IsCDDA(*m_itemCurrentFile) || m_itemCurrentFile->IsOnDVD()) &&
      !CServiceBroker::GetMediaManager().IsDiscInDrive() &&
      CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_VISUALISATION)
  {
    // yes, disable vis
    CServiceBroker::GetSettingsComponent()->GetSettings()->Save();    // save vis settings
    appPower->WakeUpScreenSaverAndDPMS();
    CServiceBroker::GetGUI()->GetWindowManager().PreviousWindow();
  }

  if (!appPlayer->IsPlaying())
  {
    stackHelper->Clear();
    appPlayer->ResetPlayer();
  }

  if (CServiceBroker::GetAppParams()->IsTestMode())
    CServiceBroker::GetAppMessenger()->PostMsg(TMSG_QUIT);
}

bool CApplication::IsPlayingFullScreenVideo() const
{
  const auto appPlayer = GetComponent<CApplicationPlayer>();
  return appPlayer->IsPlayingVideo() &&
         CServiceBroker::GetWinSystem()->GetGfxContext().IsFullScreenVideo();
}

bool CApplication::IsFullScreen()
{
  return IsPlayingFullScreenVideo() ||
        (CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_VISUALISATION) ||
         CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_SLIDESHOW;
}

void CApplication::StopPlaying()
{
  CGUIComponent *gui = CServiceBroker::GetGUI();

  if (gui)
  {
    int iWin = gui->GetWindowManager().GetActiveWindow();
    const auto appPlayer = GetComponent<CApplicationPlayer>();
    if (appPlayer->IsPlaying())
    {
      appPlayer->ClosePlayer();

      // turn off visualisation window when stopping
      if ((iWin == WINDOW_VISUALISATION ||
           iWin == WINDOW_FULLSCREEN_VIDEO ||
           iWin == WINDOW_FULLSCREEN_GAME) &&
           !m_bStop)
        gui->GetWindowManager().PreviousWindow();

      g_partyModeManager.Disable();
    }
  }
}

bool CApplication::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_NOTIFY_ALL:
    {
      if (message.GetParam1()==GUI_MSG_REMOVED_MEDIA)
      {
        // Update general playlist: Remove DVD playlist items
        int nRemoved = CServiceBroker::GetPlaylistPlayer().RemoveDVDItems();
        if ( nRemoved > 0 )
        {
          CGUIMessage msg( GUI_MSG_PLAYLIST_CHANGED, 0, 0 );
          CServiceBroker::GetGUI()->GetWindowManager().SendMessage( msg );
        }
        // stop the file if it's on dvd (will set the resume point etc)
        if (m_itemCurrentFile->IsOnDVD())
          StopPlaying();
      }
      else if (message.GetParam1() == GUI_MSG_UI_READY)
      {
        // remove splash window
        CServiceBroker::GetGUI()->GetWindowManager().Delete(WINDOW_SPLASH);

        // show the volumebar if the volume is muted
        const auto appVolume = GetComponent<CApplicationVolumeHandling>();
        if (appVolume->IsMuted() ||
            appVolume->GetVolumeRatio() <= CApplicationVolumeHandling::VOLUME_MINIMUM)
          appVolume->ShowVolumeBar();

        if (!m_incompatibleAddons.empty())
        {
          // filter addons that are not dependencies
          std::vector<std::string> disabledAddonNames;
          for (const auto& addoninfo : m_incompatibleAddons)
          {
            if (!CAddonType::IsDependencyType(addoninfo->MainType()))
              disabledAddonNames.emplace_back(addoninfo->Name());
          }

          // migration (incompatible addons) dialog
          auto addonList = StringUtils::Join(disabledAddonNames, ", ");
          auto msg = StringUtils::Format(g_localizeStrings.Get(24149), addonList);
          HELPERS::ShowOKDialogText(CVariant{24148}, CVariant{std::move(msg)});
          m_incompatibleAddons.clear();
        }

        // offer enabling addons at kodi startup that are disabled due to
        // e.g. os package manager installation on linux
        ConfigureAndEnableAddons();

        m_bInitializing = false;

        if (message.GetSenderId() == WINDOW_SETTINGS_PROFILES)
          GetComponent<CApplicationSkinHandling>()->ReloadSkin(false);
      }
      else if (message.GetParam1() == GUI_MSG_UPDATE_ITEM && message.GetItem())
      {
        CFileItemPtr item = std::static_pointer_cast<CFileItem>(message.GetItem());
        if (m_itemCurrentFile->IsSamePath(item.get()))
        {
          m_itemCurrentFile->UpdateInfo(*item);
          CServiceBroker::GetGUI()->GetInfoManager().UpdateCurrentItem(*item);
        }
      }
    }
    break;

  case GUI_MSG_PLAYBACK_STARTED:
    {
#ifdef TARGET_DARWIN_EMBEDDED
      // @TODO move this away to platform code
      CDarwinUtils::SetScheduling(GetComponent<CApplicationPlayer>()->IsPlayingVideo());
#endif
      m_itemCurrentFile =
          std::make_shared<CFileItem>(*std::static_pointer_cast<CFileItem>(message.GetItem()));
      m_playerEvent.Reset();

      CServiceBroker::GetPVRManager().OnPlaybackStarted(*m_itemCurrentFile);

      PLAYLIST::CPlayList playList = CServiceBroker::GetPlaylistPlayer().GetPlaylist(
          CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist());

      // Update our infoManager with the new details etc.
      if (m_nextPlaylistItem >= 0)
      {
        // playing an item which is not in the list - player might be stopped already
        // so do nothing
        if (playList.size() <= m_nextPlaylistItem)
          return true;

        // we've started a previously queued item
        CFileItemPtr item = playList[m_nextPlaylistItem];
        // update the playlist manager
        int currentSong = CServiceBroker::GetPlaylistPlayer().GetCurrentItemIdx();
        int param = ((currentSong & 0xffff) << 16) | (m_nextPlaylistItem & 0xffff);
        CGUIMessage msg(GUI_MSG_PLAYLISTPLAYER_CHANGED, 0, 0, CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist(), param, item);
        CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
        CServiceBroker::GetPlaylistPlayer().SetCurrentItemIdx(m_nextPlaylistItem);
        m_itemCurrentFile = std::make_shared<CFileItem>(*item);
      }
      CServiceBroker::GetGUI()->GetInfoManager().SetCurrentItem(*m_itemCurrentFile);
      g_partyModeManager.OnSongChange(true);

#ifdef HAS_PYTHON
      // informs python script currently running playback has started
      // (does nothing if python is not loaded)
      CServiceBroker::GetXBPython().OnPlayBackStarted(*m_itemCurrentFile);
#endif

      CVariant param;
      param["player"]["speed"] = 1;
      param["player"]["playerid"] = CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist();

      CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::Player, "OnPlay",
                                                         m_itemCurrentFile, param);

      // we don't want a busy dialog when switching channels
      const auto appPlayer = GetComponent<CApplicationPlayer>();
      if (!m_itemCurrentFile->IsLiveTV() ||
          (!appPlayer->IsPlayingVideo() && !appPlayer->IsPlayingAudio()))
      {
        CGUIDialogBusy* dialog =
            CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogBusy>(
                WINDOW_DIALOG_BUSY);
        if (dialog && !dialog->IsDialogRunning())
          dialog->WaitOnEvent(m_playerEvent);
      }

      return true;
    }
    break;

  case GUI_MSG_QUEUE_NEXT_ITEM:
    {
      // Check to see if our playlist player has a new item for us,
      // and if so, we check whether our current player wants the file
      int iNext = CServiceBroker::GetPlaylistPlayer().GetNextItemIdx();
      PLAYLIST::CPlayList& playlist = CServiceBroker::GetPlaylistPlayer().GetPlaylist(
          CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist());
      if (iNext < 0 || iNext >= playlist.size())
      {
        GetComponent<CApplicationPlayer>()->OnNothingToQueueNotify();
        return true; // nothing to do
      }

      // ok, grab the next song
      CFileItem file(*playlist[iNext]);
      // handle plugin://
      CURL url(file.GetDynPath());
      if (url.IsProtocol("plugin"))
        XFILE::CPluginDirectory::GetPluginResult(url.Get(), file, false);

      // Don't queue if next media type is different from current one
      bool bNothingToQueue = false;

      const auto appPlayer = GetComponent<CApplicationPlayer>();
      if (!VIDEO::IsVideo(file) && appPlayer->IsPlayingVideo())
        bNothingToQueue = true;
      else if ((!MUSIC::IsAudio(file) || VIDEO::IsVideo(file)) && appPlayer->IsPlayingAudio())
        bNothingToQueue = true;

      if (bNothingToQueue)
      {
        appPlayer->OnNothingToQueueNotify();
        return true;
      }

#ifdef HAS_UPNP
      if (URIUtils::IsUPnP(file.GetDynPath()))
      {
        if (!XFILE::CUPnPDirectory::GetResource(file.GetDynURL(), file))
          return true;
      }
#endif

      // ok - send the file to the player, if it accepts it
      if (appPlayer->QueueNextFile(file))
      {
        // player accepted the next file
        m_nextPlaylistItem = iNext;
      }
      else
      {
        /* Player didn't accept next file: *ALWAYS* advance playlist in this case so the player can
            queue the next (if it wants to) and it doesn't keep looping on this song */
        CServiceBroker::GetPlaylistPlayer().SetCurrentItemIdx(iNext);
      }

      return true;
    }
    break;

    case GUI_MSG_PLAY_TRAILER:
    {
      const CFileItem* item = dynamic_cast<CFileItem*>(message.GetItem().get());
      if (item == nullptr)
      {
        CLog::LogF(LOGERROR, "Supplied item is not a CFileItem! Trailer cannot be played.");
        return false;
      }

      std::unique_ptr<CFileItem> trailerItem =
          ContentUtils::GeneratePlayableTrailerItem(*item, g_localizeStrings.Get(20410));

      if (item->IsPlayList())
      {
        std::unique_ptr<CFileItemList> fileitemList = std::make_unique<CFileItemList>();
        fileitemList->Add(std::move(trailerItem));
        CServiceBroker::GetAppMessenger()->PostMsg(TMSG_MEDIA_PLAY, -1, -1,
                                                   static_cast<void*>(fileitemList.release()));
      }
      else
      {
        CServiceBroker::GetAppMessenger()->PostMsg(TMSG_MEDIA_PLAY, 1, 0,
                                                   static_cast<void*>(trailerItem.release()));
      }
      break;
    }

  case GUI_MSG_PLAYBACK_STOPPED:
  {
    CServiceBroker::GetPVRManager().OnPlaybackStopped(*m_itemCurrentFile);

    CVariant data(CVariant::VariantTypeObject);
    data["end"] = false;
    CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::Player, "OnStop",
                                                       m_itemCurrentFile, data);

    m_playerEvent.Set();
    ResetCurrentItem();
    PlaybackCleanup();
#ifdef HAS_PYTHON
    CServiceBroker::GetXBPython().OnPlayBackStopped();
#endif
     return true;
  }

  case GUI_MSG_PLAYBACK_ENDED:
  {
    CServiceBroker::GetPVRManager().OnPlaybackEnded(*m_itemCurrentFile);

    CVariant data(CVariant::VariantTypeObject);
    data["end"] = true;
    CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::Player, "OnStop",
                                                       m_itemCurrentFile, data);

    m_playerEvent.Set();
    const auto stackHelper = GetComponent<CApplicationStackHelper>();
    if (stackHelper->IsPlayingRegularStack() && stackHelper->HasNextStackPartFileItem())
    { // just play the next item in the stack
      PlayFile(stackHelper->SetNextStackPartCurrentFileItem(), "", true);
      return true;
    }

    // For EPG playlist items we keep the player open to ensure continuous viewing experience.
    const bool isEpgPlaylistItem{
        m_itemCurrentFile->GetProperty("epg_playlist_item").asBoolean(false)};

    ResetCurrentItem();

    if (!isEpgPlaylistItem)
    {
      if (!CServiceBroker::GetPlaylistPlayer().PlayNext(1, true))
        GetComponent<CApplicationPlayer>()->ClosePlayer();

      PlaybackCleanup();
    }

#ifdef HAS_PYTHON
    CServiceBroker::GetXBPython().OnPlayBackEnded();
#endif
    return true;
  }

  case GUI_MSG_PLAYLISTPLAYER_STOPPED:
    ResetCurrentItem();
    if (GetComponent<CApplicationPlayer>()->IsPlaying())
      StopPlaying();
    PlaybackCleanup();
    return true;

  case GUI_MSG_PLAYBACK_AVSTARTED:
  {
    CVariant param;
    param["player"]["speed"] = 1;
    param["player"]["playerid"] = CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist();
    CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::Player, "OnAVStart",
                                                       m_itemCurrentFile, param);
    m_playerEvent.Set();
#ifdef HAS_PYTHON
    // informs python script currently running playback has started
    // (does nothing if python is not loaded)
    CServiceBroker::GetXBPython().OnAVStarted(*m_itemCurrentFile);
#endif
    return true;
  }

  case GUI_MSG_PLAYBACK_AVCHANGE:
  {
#ifdef HAS_PYTHON
    // informs python script currently running playback has started
    // (does nothing if python is not loaded)
    CServiceBroker::GetXBPython().OnAVChange();
#endif
    CVariant param;
    param["player"]["speed"] = 1;
    param["player"]["playerid"] = CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist();
    CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::Player, "OnAVChange",
                                                       m_itemCurrentFile, param);
    return true;
  }

  case GUI_MSG_PLAYBACK_PAUSED:
  {
    CVariant param;
    param["player"]["speed"] = 0;
    param["player"]["playerid"] = CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist();
    CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::Player, "OnPause",
                                                       m_itemCurrentFile, param);
    return true;
  }

  case GUI_MSG_PLAYBACK_RESUMED:
  {
    CVariant param;
    param["player"]["speed"] = 1;
    param["player"]["playerid"] = CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist();
    CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::Player, "OnResume",
                                                       m_itemCurrentFile, param);
    return true;
  }

  case GUI_MSG_PLAYBACK_SEEKED:
  {
    CVariant param;
    const int64_t iTime = message.GetParam1AsI64();
    const int64_t seekOffset = message.GetParam2AsI64();
    JSONRPC::CJSONUtils::MillisecondsToTimeObject(iTime, param["player"]["time"]);
    JSONRPC::CJSONUtils::MillisecondsToTimeObject(seekOffset, param["player"]["seekoffset"]);
    param["player"]["playerid"] = CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist();
    const auto& components = CServiceBroker::GetAppComponents();
    const auto appPlayer = components.GetComponent<CApplicationPlayer>();
    param["player"]["speed"] = static_cast<int>(appPlayer->GetPlaySpeed());
    CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::Player, "OnSeek",
                                                       m_itemCurrentFile, param);

    CDataCacheCore::GetInstance().SeekFinished(static_cast<int>(seekOffset));

    return true;
  }

  case GUI_MSG_PLAYBACK_SPEED_CHANGED:
  {
    CVariant param;
    param["player"]["speed"] = message.GetParam1();
    param["player"]["playerid"] = CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist();
    CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::Player, "OnSpeedChanged",
                                                       m_itemCurrentFile, param);

    return true;
  }

  case GUI_MSG_PLAYBACK_ERROR:
    HELPERS::ShowOKDialogText(CVariant{16026}, CVariant{16027});
    return true;

  case GUI_MSG_PLAYLISTPLAYER_STARTED:
  case GUI_MSG_PLAYLISTPLAYER_CHANGED:
    {
      return true;
    }
    break;
  case GUI_MSG_FULLSCREEN:
    { // Switch to fullscreen, if we can
      CGUIComponent* gui = CServiceBroker::GetGUI();
      if (gui)
        gui->GetWindowManager().SwitchToFullScreen();

      return true;
    }
    break;
  case GUI_MSG_EXECUTE:
    if (message.GetNumStringParams())
      return ExecuteXBMCAction(message.GetStringParam(), message.GetItem());
    break;
  }
  return false;
}

bool CApplication::ExecuteXBMCAction(std::string actionStr,
                                     const std::shared_ptr<CGUIListItem>& item /* = NULL */)
{
  // see if it is a user set string

  //We don't know if there is unsecure information in this yet, so we
  //postpone any logging
  const std::string in_actionStr(actionStr);
  if (item)
    actionStr = GUILIB::GUIINFO::CGUIInfoLabel::GetItemLabel(actionStr, item.get());
  else
    actionStr = GUILIB::GUIINFO::CGUIInfoLabel::GetLabel(actionStr, INFO::DEFAULT_CONTEXT);

  // user has asked for something to be executed
  if (CBuiltins::GetInstance().HasCommand(actionStr))
  {
    if (!CBuiltins::GetInstance().IsSystemPowerdownCommand(actionStr) ||
        CServiceBroker::GetPVRManager().Get<PVR::GUI::PowerManagement>().CanSystemPowerdown())
      CBuiltins::GetInstance().Execute(actionStr);
  }
  else
  {
    // try translating the action from our ButtonTranslator
    unsigned int actionID;
    if (ACTION::CActionTranslator::TranslateString(actionStr, actionID))
    {
      OnAction(CAction(actionID));
      return true;
    }
    CFileItem item(actionStr, false);
#ifdef HAS_PYTHON
    if (item.IsPythonScript())
    { // a python script
      CScriptInvocationManager::GetInstance().ExecuteAsync(item.GetPath());
    }
    else
#endif
        if (MUSIC::IsAudio(item) || VIDEO::IsVideo(item) || item.IsGame())
    { // an audio or video file
      PlayFile(item, "");
    }
    else
    {
      //At this point we have given up to translate, so even though
      //there may be insecure information, we log it.
      CLog::LogF(LOGDEBUG, "Tried translating, but failed to understand {}", in_actionStr);
      return false;
    }
  }
  return true;
}

void CApplication::ConfigureAndEnableAddons()
{
  std::vector<std::shared_ptr<IAddon>>
      disabledAddons; /*!< Installed addons, but not auto-enabled via manifest */

  auto& addonMgr = CServiceBroker::GetAddonMgr();

  if (addonMgr.GetDisabledAddons(disabledAddons) && !disabledAddons.empty())
  {
    // this applies to certain platforms only:
    // look at disabled addons with disabledReason == NONE, usually those are installed via package managers or manually.
    // also try to enable add-ons with disabledReason == INCOMPATIBLE at startup for all platforms.

    bool isConfigureAddonsAtStartupEnabled =
        m_ServiceManager->GetPlatform().IsConfigureAddonsAtStartupEnabled();

    for (const auto& addon : disabledAddons)
    {
      if (addonMgr.IsAddonDisabledWithReason(addon->ID(), ADDON::AddonDisabledReason::INCOMPATIBLE))
      {
        auto addonInfo = addonMgr.GetAddonInfo(addon->ID(), AddonType::UNKNOWN);
        if (addonInfo && addonMgr.IsCompatible(addonInfo))
        {
          CLog::Log(LOGDEBUG, "CApplication::{}: enabling the compatible version of [{}].",
                    __FUNCTION__, addon->ID());
          addonMgr.EnableAddon(addon->ID());
        }
        continue;
      }

      if (addonMgr.IsAddonDisabledExcept(addon->ID(), ADDON::AddonDisabledReason::NONE) ||
          CAddonType::IsDependencyType(addon->MainType()))
      {
        continue;
      }

      if (isConfigureAddonsAtStartupEnabled)
      {
        if (HELPERS::ShowYesNoDialogLines(CVariant{24039}, // Disabled add-ons
                                          CVariant{24059}, // Would you like to enable this add-on?
                                          CVariant{addon->Name()}) == DialogResponse::CHOICE_YES)
        {
          if (addon->CanHaveAddonOrInstanceSettings())
          {
            if (CGUIDialogAddonSettings::ShowForAddon(addon))
            {
              // only enable if settings dialog hasn't been cancelled
              addonMgr.EnableAddon(addon->ID());
            }
          }
          else
          {
            addonMgr.EnableAddon(addon->ID());
          }
        }
        else
        {
          // user chose not to configure/enable so we're not asking anymore
          addonMgr.UpdateDisabledReason(addon->ID(), ADDON::AddonDisabledReason::USER);
        }
      }
    }
  }
}

void CApplication::Process()
{
  // dispatch the messages generated by python or other threads to the current window
  CServiceBroker::GetGUI()->GetWindowManager().DispatchThreadMessages();

  // process messages which have to be send to the gui
  // (this can only be done after CServiceBroker::GetGUI()->GetWindowManager().Render())
  CServiceBroker::GetAppMessenger()->ProcessWindowMessages();

  // handle any active scripts

  {
    // Allow processing of script threads to let them shut down properly.
    CSingleExit ex(CServiceBroker::GetWinSystem()->GetGfxContext());
    m_frameMoveGuard.unlock();
    CScriptInvocationManager::GetInstance().Process();
    m_frameMoveGuard.lock();
  }

  // process messages, even if a movie is playing
  CServiceBroker::GetAppMessenger()->ProcessMessages();
  if (m_bStop) return; //we're done, everything has been unloaded

  // do any processing that isn't needed on each run
  if( m_slowTimer.GetElapsedMilliseconds() > 500 )
  {
    m_slowTimer.Reset();
    ProcessSlow();
  }
}

// We get called every 500ms
void CApplication::ProcessSlow()
{
  // process skin resources (skin timers)
  GetComponent<CApplicationSkinHandling>()->ProcessSkin();

  CServiceBroker::GetPowerManager().ProcessEvents();

  // Temporarily pause pausable jobs when viewing video/picture
  int currentWindow = CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow();
  if (VIDEO::IsVideo(CurrentFileItem()) || CurrentFileItem().IsPicture() ||
      currentWindow == WINDOW_FULLSCREEN_VIDEO || currentWindow == WINDOW_FULLSCREEN_GAME ||
      currentWindow == WINDOW_SLIDESHOW)
  {
    CServiceBroker::GetJobManager()->PauseJobs();
  }
  else
  {
    CServiceBroker::GetJobManager()->UnPauseJobs();
  }

  // Check if we need to activate the screensaver / DPMS.
  const auto appPower = GetComponent<CApplicationPowerHandling>();
  appPower->CheckScreenSaverAndDPMS();

  // Check if we need to shutdown (if enabled).
#if defined(TARGET_DARWIN)
  if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_POWERMANAGEMENT_SHUTDOWNTIME) &&
      CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_fullScreen)
#else
  if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_POWERMANAGEMENT_SHUTDOWNTIME))
#endif
  {
    appPower->CheckShutdown();
  }

#if defined(TARGET_POSIX)
  if (CPlatformPosix::TestQuitFlag())
  {
    CLog::Log(LOGINFO, "Quitting due to POSIX signal");
    CServiceBroker::GetAppMessenger()->PostMsg(TMSG_QUIT);
  }
#endif

  // check if we should restart the player
  CheckDelayedPlayerRestart();

  //  check if we can unload any unreferenced dlls or sections
  const auto appPlayer = GetComponent<CApplicationPlayer>();
  if (!appPlayer->IsPlayingVideo())
    CSectionLoader::UnloadDelayed();

#ifdef TARGET_ANDROID
  // Pass the slow loop to droid
  CXBMCApp::Get().ProcessSlow();
#endif

  // check for any idle curl connections
  g_curlInterface.CheckIdle();

  CServiceBroker::GetGUI()->GetLargeTextureManager().CleanupUnusedImages();

  CServiceBroker::GetGUI()->GetTextureManager().FreeUnusedTextures(5000);

#ifdef HAS_OPTICAL_DRIVE
  // checks whats in the DVD drive and tries to autostart the content (xbox games, dvd, cdda, avi files...)
  if (!appPlayer->IsPlayingVideo())
    m_Autorun->HandleAutorun();
#endif

  // update upnp server/renderer states
#ifdef HAS_UPNP
  if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_SERVICES_UPNP) && UPNP::CUPnP::IsInstantiated())
    UPNP::CUPnP::GetInstance()->UpdateState();
#endif

#if defined(TARGET_POSIX) && defined(HAS_FILESYSTEM_SMB)
  smb.CheckIfIdle();
#endif

#ifdef HAS_FILESYSTEM_NFS
  gNfsConnection.CheckIfIdle();
#endif

  for (const auto& vfsAddon : CServiceBroker::GetVFSAddonCache().GetAddonInstances())
    vfsAddon->ClearOutIdle();

  CServiceBroker::GetMediaManager().ProcessEvents();

  // if we don't render the gui there's no reason to start the screensaver.
  // that way the screensaver won't kick in if we maximize the XBMC window
  // after the screensaver start time.
  if (!appPower->GetRenderGUI())
    appPower->ResetScreenSaverTimer();
}

void CApplication::DelayedPlayerRestart()
{
  m_restartPlayerTimer.StartZero();
}

void CApplication::CheckDelayedPlayerRestart()
{
  if (m_restartPlayerTimer.GetElapsedSeconds() > 3)
  {
    m_restartPlayerTimer.Stop();
    m_restartPlayerTimer.Reset();
    Restart(true);
  }
}

void CApplication::Restart(bool bSamePosition)
{
  // this function gets called when the user changes a setting (like noninterleaved)
  // and which means we gotta close & reopen the current playing file

  // first check if we're playing a file
  const auto appPlayer = GetComponent<CApplicationPlayer>();
  if (!appPlayer->IsPlayingVideo() && !appPlayer->IsPlayingAudio())
    return ;

  if (!appPlayer->HasPlayer())
    return ;

  // do we want to return to the current position in the file
  if (!bSamePosition)
  {
    // no, then just reopen the file and start at the beginning
    PlayFile(*m_itemCurrentFile, "", true);
    return ;
  }

  // else get current position
  double time = GetTime();

  // get player state, needed for dvd's
  std::string state = appPlayer->GetPlayerState();

  // set the requested starttime
  m_itemCurrentFile->SetStartOffset(CUtil::ConvertSecsToMilliSecs(time));

  // reopen the file
  if (PlayFile(*m_itemCurrentFile, "", true))
    appPlayer->SetPlayerState(state);
}

const std::string& CApplication::CurrentFile()
{
  return m_itemCurrentFile->GetPath();
}

std::shared_ptr<CFileItem> CApplication::CurrentFileItemPtr()
{
  return m_itemCurrentFile;
}

CFileItem& CApplication::CurrentFileItem()
{
  return *m_itemCurrentFile;
}

const CFileItem& CApplication::CurrentUnstackedItem()
{
  const auto stackHelper = GetComponent<CApplicationStackHelper>();

  if (stackHelper->IsPlayingISOStack() || stackHelper->IsPlayingRegularStack())
    return stackHelper->GetCurrentStackPartFileItem();
  else
    return *m_itemCurrentFile;
}

// Returns the total time in seconds of the current media.  Fractional
// portions of a second are possible - but not necessarily supported by the
// player class.  This returns a double to be consistent with GetTime() and
// SeekTime().
double CApplication::GetTotalTime() const
{
  double rc = 0.0;

  const auto appPlayer = GetComponent<CApplicationPlayer>();
  const auto stackHelper = GetComponent<CApplicationStackHelper>();

  if (appPlayer->IsPlaying())
  {
    if (stackHelper->IsPlayingRegularStack())
      rc = stackHelper->GetStackTotalTimeMs() * 0.001;
    else
      rc = appPlayer->GetTotalTime() * 0.001;
  }

  return rc;
}

// Returns the current time in seconds of the currently playing media.
// Fractional portions of a second are possible.  This returns a double to
// be consistent with GetTotalTime() and SeekTime().
double CApplication::GetTime() const
{
  double rc = 0.0;

  const auto appPlayer = GetComponent<CApplicationPlayer>();
  const auto stackHelper = GetComponent<CApplicationStackHelper>();

  if (appPlayer->IsPlaying())
  {
    if (stackHelper->IsPlayingRegularStack())
    {
      uint64_t startOfCurrentFile = stackHelper->GetCurrentStackPartStartTimeMs();
      rc = (startOfCurrentFile + appPlayer->GetTime()) * 0.001;
    }
    else
      rc = appPlayer->GetTime() * 0.001;
  }

  return rc;
}

// Sets the current position of the currently playing media to the specified
// time in seconds.  Fractional portions of a second are valid.  The passed
// time is the time offset from the beginning of the file as opposed to a
// delta from the current position.  This method accepts a double to be
// consistent with GetTime() and GetTotalTime().
void CApplication::SeekTime( double dTime )
{
  const auto appPlayer = GetComponent<CApplicationPlayer>();
  const auto stackHelper = GetComponent<CApplicationStackHelper>();

  if (appPlayer->IsPlaying() && (dTime >= 0.0))
  {
    if (!appPlayer->CanSeek())
      return;

    if (stackHelper->IsPlayingRegularStack())
    {
      // find the item in the stack we are seeking to, and load the new
      // file if necessary, and calculate the correct seek within the new
      // file.  Otherwise, just fall through to the usual routine if the
      // time is higher than our total time.
      int partNumberToPlay =
          stackHelper->GetStackPartNumberAtTimeMs(static_cast<uint64_t>(dTime * 1000.0));
      uint64_t startOfNewFile = stackHelper->GetStackPartStartTimeMs(partNumberToPlay);
      if (partNumberToPlay == stackHelper->GetCurrentPartNumber())
        appPlayer->SeekTime(static_cast<uint64_t>(dTime * 1000.0) - startOfNewFile);
      else
      { // seeking to a new file
        stackHelper->SetStackPartCurrentFileItem(partNumberToPlay);
        CFileItem* item = new CFileItem(stackHelper->GetCurrentStackPartFileItem());
        item->SetStartOffset(static_cast<uint64_t>(dTime * 1000.0) - startOfNewFile);
        // don't just call "PlayFile" here, as we are quite likely called from the
        // player thread, so we won't be able to delete ourselves.
        CServiceBroker::GetAppMessenger()->PostMsg(TMSG_MEDIA_PLAY, 1, 0, static_cast<void*>(item));
      }
      return;
    }
    // convert to milliseconds and perform seek
    appPlayer->SeekTime(static_cast<int64_t>(dTime * 1000.0));
  }
}

float CApplication::GetPercentage() const
{
  const auto appPlayer = GetComponent<CApplicationPlayer>();
  const auto stackHelper = GetComponent<CApplicationStackHelper>();

  if (appPlayer->IsPlaying())
  {
    if (appPlayer->GetTotalTime() == 0 && appPlayer->IsPlayingAudio() &&
        m_itemCurrentFile->HasMusicInfoTag())
    {
      const CMusicInfoTag& tag = *m_itemCurrentFile->GetMusicInfoTag();
      if (tag.GetDuration() > 0)
        return (float)(GetTime() / tag.GetDuration() * 100);
    }

    if (stackHelper->IsPlayingRegularStack())
    {
      double totalTime = GetTotalTime();
      if (totalTime > 0.0)
        return (float)(GetTime() / totalTime * 100);
    }
    else
      return appPlayer->GetPercentage();
  }
  return 0.0f;
}

float CApplication::GetCachePercentage() const
{
  const auto appPlayer = GetComponent<CApplicationPlayer>();
  const auto stackHelper = GetComponent<CApplicationStackHelper>();

  if (appPlayer->IsPlaying())
  {
    // Note that the player returns a relative cache percentage and we want an absolute percentage
    if (stackHelper->IsPlayingRegularStack())
    {
      float stackedTotalTime = (float) GetTotalTime();
      // We need to take into account the stack's total time vs. currently playing file's total time
      if (stackedTotalTime > 0.0f)
        return std::min(100.0f,
                        GetPercentage() + (appPlayer->GetCachePercentage() *
                                           appPlayer->GetTotalTime() * 0.001f / stackedTotalTime));
    }
    else
      return std::min(100.0f, appPlayer->GetPercentage() + appPlayer->GetCachePercentage());
  }
  return 0.0f;
}

void CApplication::SeekPercentage(float percent)
{
  const auto appPlayer = GetComponent<CApplicationPlayer>();
  const auto stackHelper = GetComponent<CApplicationStackHelper>();

  if (appPlayer->IsPlaying() && (percent >= 0.0f))
  {
    if (!appPlayer->CanSeek())
      return;
    if (stackHelper->IsPlayingRegularStack())
      SeekTime(static_cast<double>(percent) * 0.01 * GetTotalTime());
    else
      appPlayer->SeekPercentage(percent);
  }
}

std::string CApplication::GetCurrentPlayer()
{
  const auto appPlayer = GetComponent<CApplicationPlayer>();
  return appPlayer->GetCurrentPlayer();
}

void CApplication::UpdateLibraries()
{
  const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  if (settings->GetBool(CSettings::SETTING_VIDEOLIBRARY_UPDATEONSTARTUP))
  {
    CLog::LogF(LOGINFO, "Starting video library startup scan");
    CVideoLibraryQueue::GetInstance().ScanLibrary(
        "", false, !settings->GetBool(CSettings::SETTING_VIDEOLIBRARY_BACKGROUNDUPDATE));
  }

  if (settings->GetBool(CSettings::SETTING_MUSICLIBRARY_UPDATEONSTARTUP))
  {
    CLog::LogF(LOGINFO, "Starting music library startup scan");
    CMusicLibraryQueue::GetInstance().ScanLibrary(
        "", MUSIC_INFO::CMusicInfoScanner::SCAN_NORMAL,
        !settings->GetBool(CSettings::SETTING_MUSICLIBRARY_BACKGROUNDUPDATE));
  }
}

void CApplication::UpdateCurrentPlayArt()
{
  const auto appPlayer = GetComponent<CApplicationPlayer>();
  if (!appPlayer->IsPlayingAudio())
    return;
  //Clear and reload the art for the currently playing item to show updated art on OSD
  m_itemCurrentFile->ClearArt();
  CMusicThumbLoader loader;
  loader.LoadItem(m_itemCurrentFile.get());
  // Mirror changes to GUI item
  CServiceBroker::GetGUI()->GetInfoManager().SetCurrentItem(*m_itemCurrentFile);
}

bool CApplication::ProcessAndStartPlaylist(const std::string& strPlayList,
                                           PLAYLIST::CPlayList& playlist,
                                           PLAYLIST::Id playlistId,
                                           int track)
{
  CLog::Log(LOGDEBUG, "CApplication::ProcessAndStartPlaylist({}, {})", strPlayList, playlistId);

  // initial exit conditions
  // no songs in playlist just return
  if (playlist.size() == 0)
    return false;

  // illegal playlist
  if (playlistId == PLAYLIST::TYPE_NONE || playlistId == PLAYLIST::TYPE_PICTURE)
    return false;

  // setup correct playlist
  CServiceBroker::GetPlaylistPlayer().ClearPlaylist(playlistId);

  // if the playlist contains an internet stream, this file will be used
  // to generate a thumbnail for musicplayer.cover
  m_strPlayListFile = strPlayList;

  // add the items to the playlist player
  CServiceBroker::GetPlaylistPlayer().Add(playlistId, playlist);

  // if we have a playlist
  if (CServiceBroker::GetPlaylistPlayer().GetPlaylist(playlistId).size())
  {
    // start playing it
    CServiceBroker::GetPlaylistPlayer().SetCurrentPlaylist(playlistId);
    CServiceBroker::GetPlaylistPlayer().Reset();
    CServiceBroker::GetPlaylistPlayer().Play(track, "");
    return true;
  }
  return false;
}

bool CApplication::GetRenderGUI() const
{
  return GetComponent<CApplicationPowerHandling>()->GetRenderGUI();
}

bool CApplication::SetLanguage(const std::string &strLanguage)
{
  // nothing to be done if the language hasn't changed
  if (strLanguage == CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(CSettings::SETTING_LOCALE_LANGUAGE))
    return true;

  return CServiceBroker::GetSettingsComponent()->GetSettings()->SetString(CSettings::SETTING_LOCALE_LANGUAGE, strLanguage);
}

bool CApplication::LoadLanguage(bool reload)
{
  // load the configured language
  if (!g_langInfo.SetLanguage("", reload))
    return false;

  // set the proper audio and subtitle languages
  const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  g_langInfo.SetAudioLanguage(settings->GetString(CSettings::SETTING_LOCALE_AUDIOLANGUAGE));
  g_langInfo.SetSubtitleLanguage(settings->GetString(CSettings::SETTING_LOCALE_SUBTITLELANGUAGE));

  return true;
}

void CApplication::SetLoggingIn(bool switchingProfiles)
{
  // don't save skin settings on unloading when logging into another profile
  // because in that case we have already loaded the new profile and
  // would therefore write the previous skin's settings into the new profile
  // instead of into the previous one
  GetComponent<CApplicationSkinHandling>()->m_saveSkinOnUnloading = !switchingProfiles;
}

void CApplication::PrintStartupLog()
{
  CLog::Log(LOGINFO, "-----------------------------------------------------------------------");
  CLog::Log(LOGINFO, "Starting {} ({}). Platform: {} {} {}-bit", CSysInfo::GetAppName(),
            CSysInfo::GetVersion(), g_sysinfo.GetBuildTargetPlatformName(),
            g_sysinfo.GetBuildTargetCpuFamily(), g_sysinfo.GetXbmcBitness());

  std::string buildType;
#if defined(_DEBUG)
  buildType = "Debug";
#elif defined(NDEBUG)
  buildType = "Release";
#else
  buildType = "Unknown";
#endif

  CLog::Log(LOGINFO, "Using {} {} x{}", buildType, CSysInfo::GetAppName(),
            g_sysinfo.GetXbmcBitness());
  CLog::Log(LOGINFO, "{} compiled {} by {} for {} {} {}-bit {} ({})", CSysInfo::GetAppName(),
            CSysInfo::GetBuildDate(), g_sysinfo.GetUsedCompilerNameAndVer(),
            g_sysinfo.GetBuildTargetPlatformName(), g_sysinfo.GetBuildTargetCpuFamily(),
            g_sysinfo.GetXbmcBitness(), g_sysinfo.GetBuildTargetPlatformVersionDecoded(),
            g_sysinfo.GetBuildTargetPlatformVersion());

  std::string deviceModel(g_sysinfo.GetModelName());
  if (!g_sysinfo.GetManufacturerName().empty())
    deviceModel = g_sysinfo.GetManufacturerName() + " " +
                  (deviceModel.empty() ? std::string("device") : deviceModel);
  if (!deviceModel.empty())
    CLog::Log(LOGINFO, "Running on {} with {}, kernel: {} {} {}-bit version {}", deviceModel,
              g_sysinfo.GetOsPrettyNameWithVersion(), g_sysinfo.GetKernelName(),
              g_sysinfo.GetKernelCpuFamily(), g_sysinfo.GetKernelBitness(),
              g_sysinfo.GetKernelVersionFull());
  else
    CLog::Log(LOGINFO, "Running on {}, kernel: {} {} {}-bit version {}",
              g_sysinfo.GetOsPrettyNameWithVersion(), g_sysinfo.GetKernelName(),
              g_sysinfo.GetKernelCpuFamily(), g_sysinfo.GetKernelBitness(),
              g_sysinfo.GetKernelVersionFull());

  CLog::Log(LOGINFO, "FFmpeg version/source: {}", av_version_info());

  std::string cpuModel(CServiceBroker::GetCPUInfo()->GetCPUModel());
  if (!cpuModel.empty())
  {
    CLog::Log(LOGINFO, "Host CPU: {}, {} core{} available", cpuModel,
              CServiceBroker::GetCPUInfo()->GetCPUCount(),
              (CServiceBroker::GetCPUInfo()->GetCPUCount() == 1) ? "" : "s");
  }
  else
    CLog::Log(LOGINFO, "{} CPU core{} available", CServiceBroker::GetCPUInfo()->GetCPUCount(),
              (CServiceBroker::GetCPUInfo()->GetCPUCount() == 1) ? "" : "s");

  // Any system info logging that is unique to a platform
  m_ServiceManager->GetPlatform().PlatformSyslog();

#if defined(__arm__) || defined(__aarch64__)
  CLog::Log(LOGINFO, "ARM Features: Neon {}",
            (CServiceBroker::GetCPUInfo()->GetCPUFeatures() & CPU_FEATURE_NEON) ? "enabled"
                                                                                : "disabled");
#endif
  CSpecialProtocol::LogPaths();

#ifdef HAS_WEB_SERVER
  CLog::Log(LOGINFO, "Webserver extra whitelist paths: {}",
            StringUtils::Join(CCompileInfo::GetWebserverExtraWhitelist(), ", "));
#endif

  // Check, whether libkodi.so was reused (happens on Android, where the system does not unload
  // the lib on activity end, but keeps it loaded (as long as there is enough memory) and reuses
  // it on next activity start.
  static bool firstRun = true;

  CLog::Log(LOGINFO, "The executable running is: {}{}", CUtil::ResolveExecutablePath(),
            firstRun ? "" : " [reused]");

  firstRun = false;

  std::string hostname("[unknown]");
  m_ServiceManager->GetNetwork().GetHostName(hostname);
  CLog::Log(LOGINFO, "Local hostname: {}", hostname);
  std::string lowerAppName = CCompileInfo::GetAppName();
  StringUtils::ToLower(lowerAppName);
  CLog::Log(LOGINFO, "Log File is located: {}.log",
            CSpecialProtocol::TranslatePath("special://logpath/" + lowerAppName));
  CRegExp::LogCheckUtf8Support();
  CLog::Log(LOGINFO, "-----------------------------------------------------------------------");
}

void CApplication::CloseNetworkShares()
{
  CLog::Log(LOGDEBUG,"CApplication::CloseNetworkShares: Closing all network shares");

#if defined(HAS_FILESYSTEM_SMB) && !defined(TARGET_WINDOWS)
  smb.Deinit();
#endif

#ifdef HAS_FILESYSTEM_NFS
  gNfsConnection.Deinit();
#endif

  for (const auto& vfsAddon : CServiceBroker::GetVFSAddonCache().GetAddonInstances())
    vfsAddon->DisconnectAll();
}
