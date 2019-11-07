/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "network/EventServer.h"
#include "network/Network.h"
#include "threads/SystemClock.h"
#include "Application.h"
#include "AppParamParser.h"
#include "AppInboundProtocol.h"
#include "dialogs/GUIDialogBusy.h"
#include "events/EventLog.h"
#include "events/NotificationEvent.h"
#include "interfaces/builtins/Builtins.h"
#include "utils/JobManager.h"
#include "utils/Variant.h"
#include "LangInfo.h"
#include "utils/Screenshot.h"
#include "Util.h"
#include "URL.h"
#include "guilib/GUIComponent.h"
#include "guilib/TextureManager.h"
#include "cores/IPlayer.h"
#include "cores/AudioEngine/Engines/ActiveAE/ActiveAE.h"
#include "cores/playercorefactory/PlayerCoreFactory.h"
#include "PlayListPlayer.h"
#include "Autorun.h"
#include "video/Bookmark.h"
#include "video/VideoLibraryQueue.h"
#include "music/MusicLibraryQueue.h"
#include "guilib/GUIControlProfiler.h"
#include "utils/LangCodeExpander.h"
#include "GUIInfoManager.h"
#include "playlists/PlayListFactory.h"
#include "guilib/GUIFontManager.h"
#include "guilib/GUIColorManager.h"
#include "guilib/StereoscopicsManager.h"
#include "addons/Skin.h"
#include "addons/VFSEntry.h"
#include "interfaces/generic/ScriptInvocationManager.h"
#ifdef HAS_PYTHON
#include "interfaces/python/XBPython.h"
#endif
#include "input/actions/ActionTranslator.h"
#include "input/ButtonTranslator.h"
#include "guilib/GUIAudioManager.h"
#include "GUIPassword.h"
#include "input/InertialScrollingHandler.h"
#include "messaging/ThreadMessage.h"
#include "messaging/ApplicationMessenger.h"
#include "messaging/helpers/DialogHelper.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "SectionLoader.h"
#include "cores/DllLoader/DllLoaderContainer.h"
#include "GUIUserMessages.h"
#include "filesystem/Directory.h"
#include "filesystem/DirectoryCache.h"
#include "filesystem/StackDirectory.h"
#include "filesystem/SpecialProtocol.h"
#include "filesystem/DllLibCurl.h"
#include "filesystem/PluginDirectory.h"
#include "utils/SystemInfo.h"
#include "utils/TimeUtils.h"
#include "GUILargeTextureManager.h"
#include "TextureCache.h"
#include "playlists/SmartPlayList.h"
#include "playlists/PlayList.h"
#include "profiles/ProfileManager.h"
#include "windowing/WinSystem.h"
#include "powermanagement/DPMSSupport.h"
#include "powermanagement/PowerManager.h"
#include "powermanagement/PowerTypes.h"
#include "settings/Settings.h"
#include "settings/AdvancedSettings.h"
#include "settings/DisplaySettings.h"
#include "settings/MediaSettings.h"
#include "settings/SettingsComponent.h"
#include "settings/SkinSettings.h"
#include "guilib/LocalizeStrings.h"
#include "utils/CPUInfo.h"
#include "utils/FileExtensionProvider.h"
#include "utils/log.h"
#include "SeekHandler.h"
#include "ServiceBroker.h"

#include "input/KeyboardLayoutManager.h"

#ifdef HAS_UPNP
#include "network/upnp/UPnP.h"
#include "filesystem/UPnPDirectory.h"
#endif
#if defined(TARGET_POSIX) && defined(HAS_FILESYSTEM_SMB)
#include "platform/posix/filesystem/SMBDirectory.h"
#endif
#ifdef HAS_FILESYSTEM_NFS
#include "filesystem/NFSFile.h"
#endif
#include "PartyModeManager.h"
#include "network/ZeroconfBrowser.h"
#ifndef TARGET_POSIX
#include "threads/platform/win/Win32Exception.h"
#endif
#ifdef HAS_DBUS
#include <dbus/dbus.h>
#endif
#include "interfaces/json-rpc/JSONRPC.h"
#include "interfaces/AnnouncementManager.h"
#include "peripherals/Peripherals.h"
#include "music/infoscanner/MusicInfoScanner.h"
#include "music/MusicUtils.h"
#include "music/MusicThumbLoader.h"

// Windows includes
#include "guilib/GUIWindowManager.h"
#include "video/dialogs/GUIDialogVideoInfo.h"
#include "windows/GUIWindowScreensaver.h"
#include "video/PlayerController.h"

// Dialog includes
#include "video/dialogs/GUIDialogVideoBookmarks.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "dialogs/GUIDialogSubMenu.h"
#include "dialogs/GUIDialogButtonMenu.h"
#include "dialogs/GUIDialogSimpleMenu.h"
#include "dialogs/GUIDialogVolumeBar.h"
#include "addons/settings/GUIDialogAddonSettings.h"

// PVR related include Files
#include "pvr/PVRManager.h"
#include "pvr/guilib/PVRGUIActions.h"

#include "video/dialogs/GUIDialogFullScreenInfo.h"
#include "dialogs/GUIDialogCache.h"
#include "dialogs/GUIDialogPlayEject.h"
#include "utils/URIUtils.h"
#include "utils/XMLUtils.h"
#include "addons/AddonInstaller.h"
#include "addons/AddonManager.h"
#include "addons/RepositoryUpdater.h"
#include "music/tags/MusicInfoTag.h"
#include "CompileInfo.h"

#ifdef TARGET_WINDOWS
#include "win32util.h"
#endif

#ifdef TARGET_DARWIN_OSX
#include "platform/darwin/osx/CocoaInterface.h"
#include "platform/darwin/osx/XBMCHelper.h"
#endif
#ifdef TARGET_DARWIN
#include "platform/darwin/DarwinUtils.h"
#endif

#ifdef HAS_DVD_DRIVE
#include <cdio/logging.h>
#endif

#include "storage/MediaManager.h"
#include "utils/SaveFileStateJob.h"
#include "utils/AlarmClock.h"
#include "utils/StringUtils.h"
#include "DatabaseManager.h"
#include "input/InputManager.h"

#ifdef TARGET_POSIX
#include "platform/posix/XHandle.h"
#include "platform/posix/XTimeUtils.h"
#include "platform/posix/filesystem/PosixDirectory.h"
#include "platform/posix/PlatformPosix.h"
#endif

#if defined(TARGET_ANDROID)
#include <androidjni/Build.h>
#include "platform/android/activity/XBMCApp.h"
#include "platform/android/activity/AndroidFeatures.h"
#endif

#ifdef TARGET_WINDOWS
#include "platform/Environment.h"
#endif

//TODO: XInitThreads
#ifdef HAVE_X11
#include <X11/Xlib.h>
#endif

#include "cores/FFmpeg.h"
#include "utils/CharsetConverter.h"
#include "pictures/GUIWindowSlideShow.h"
#include "addons/AddonSystemSettings.h"
#include "FileItem.h"

using namespace ADDON;
using namespace XFILE;
#ifdef HAS_DVD_DRIVE
using namespace MEDIA_DETECT;
#endif
using namespace PLAYLIST;
using namespace VIDEO;
using namespace MUSIC_INFO;
using namespace EVENTSERVER;
using namespace JSONRPC;
using namespace PVR;
using namespace PERIPHERALS;
using namespace KODI;
using namespace KODI::MESSAGING;
using namespace ActiveAE;

using namespace XbmcThreads;

using KODI::MESSAGING::HELPERS::DialogResponse;

#define MAX_FFWD_SPEED 5

CApplication::CApplication(void)
:
#ifdef HAS_DVD_DRIVE
  m_Autorun(new CAutorun()),
#endif
  m_itemCurrentFile(new CFileItem)
  , m_pInertialScrollingHandler(new CInertialScrollingHandler())
  , m_WaitingExternalCalls(0)
  , m_playerEvent(true, true)
{
  TiXmlBase::SetCondenseWhiteSpace(false);

#ifdef HAVE_X11
  XInitThreads();
#endif
}

CApplication::~CApplication(void)
{
  delete m_pInertialScrollingHandler;

  m_actionListeners.clear();
}

bool CApplication::OnEvent(XBMC_Event& newEvent)
{
  CSingleLock lock(m_portSection);
  m_portEvents.push_back(newEvent);
  return true;
}

void CApplication::HandlePortEvents()
{
  CSingleLock lock(m_portSection);
  while (!m_portEvents.empty())
  {
    auto newEvent = m_portEvents.front();
    m_portEvents.pop_front();
    CSingleExit lock(m_portSection);
    switch(newEvent.type)
    {
      case XBMC_QUIT:
        if (!m_bStop)
          CApplicationMessenger::GetInstance().PostMsg(TMSG_QUIT);
        break;
      case XBMC_VIDEORESIZE:
        if (CServiceBroker::GetGUI()->GetWindowManager().Initialized())
        {
          if (!CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_fullScreen)
          {
            CServiceBroker::GetWinSystem()->GetGfxContext().ApplyWindowResize(newEvent.resize.w, newEvent.resize.h);

            const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
            settings->SetInt(CSettings::SETTING_WINDOW_WIDTH, newEvent.resize.w);
            settings->SetInt(CSettings::SETTING_WINDOW_HEIGHT, newEvent.resize.h);
            settings->Save();
          }
#ifdef TARGET_WINDOWS
          else
          {
            // this may occurs when OS tries to resize application window
            //CDisplaySettings::GetInstance().SetCurrentResolution(RES_DESKTOP, true);
            //auto& gfxContext = CServiceBroker::GetWinSystem()->GetGfxContext();
            //gfxContext.SetVideoResolution(gfxContext.GetVideoResolution(), true);
            // try to resize window back to it's full screen size
            auto& res_info = CDisplaySettings::GetInstance().GetResolutionInfo(RES_DESKTOP);
            CServiceBroker::GetWinSystem()->ResizeWindow(res_info.iScreenWidth, res_info.iScreenHeight, 0, 0);
          }
#endif
        }
        break;
      case XBMC_VIDEOMOVE:
      {
        CServiceBroker::GetWinSystem()->OnMove(newEvent.move.x, newEvent.move.y);
      }
        break;
      case XBMC_MODECHANGE:
        CServiceBroker::GetWinSystem()->GetGfxContext().ApplyModeChange(newEvent.mode.res);
        break;
      case XBMC_USEREVENT:
        CApplicationMessenger::GetInstance().PostMsg(static_cast<uint32_t>(newEvent.user.code));
        break;
      case XBMC_SETFOCUS:
        // Reset the screensaver
        ResetScreenSaver();
        WakeUpScreenSaverAndDPMS();
        // Send a mouse motion event with no dx,dy for getting the current guiitem selected
        OnAction(CAction(ACTION_MOUSE_MOVE, 0, static_cast<float>(newEvent.focus.x), static_cast<float>(newEvent.focus.y), 0, 0));
        break;
      default:
        CServiceBroker::GetInputManager().OnEvent(newEvent);
    }
  }
}

extern "C" void __stdcall init_emu_environ();
extern "C" void __stdcall update_emu_environ();
extern "C" void __stdcall cleanup_emu_environ();

//
// Utility function used to copy files from the application bundle
// over to the user data directory in Application Support/Kodi.
//
static void CopyUserDataIfNeeded(const std::string &strPath, const std::string &file, const std::string &destname = "")
{
  std::string destPath;
  if (destname == "")
    destPath = URIUtils::AddFileToFolder(strPath, file);
  else
    destPath = URIUtils::AddFileToFolder(strPath, destname);

  if (!CFile::Exists(destPath))
  {
    // need to copy it across
    std::string srcPath = URIUtils::AddFileToFolder("special://xbmc/userdata/", file);
    CFile::Copy(srcPath, destPath);
  }
}

void CApplication::Preflight()
{
#ifdef HAS_DBUS
  // call 'dbus_threads_init_default' before any other dbus calls in order to
  // avoid race conditions with other threads using dbus connections
  dbus_threads_init_default();
#endif

  // run any platform preflight scripts.
#if defined(TARGET_DARWIN_OSX)
  std::string install_path;

  install_path = CUtil::GetHomePath();
  setenv("KODI_HOME", install_path.c_str(), 0);
  install_path += "/tools/darwin/runtime/preflight";
  system(install_path.c_str());
#endif
}

bool CApplication::Create(const CAppParamParser &params)
{
  // Grab a handle to our thread to be used later in identifying the render thread.
  m_threadID = CThread::GetCurrentThreadId();

  m_bPlatformDirectories = params.m_platformDirectories;
  m_bTestMode = params.m_testmode;
  m_bStandalone = params.m_standAlone;

  CServiceBroker::RegisterCPUInfo(CCPUInfo::GetCPUInfo());

  m_pSettingsComponent.reset(new CSettingsComponent());
  m_pSettingsComponent->Init(params);

  // Announement service
  m_pAnnouncementManager = std::make_shared<ANNOUNCEMENT::CAnnouncementManager>();
  m_pAnnouncementManager->Start();
  CServiceBroker::RegisterAnnouncementManager(m_pAnnouncementManager);

  m_ServiceManager.reset(new CServiceManager());

  if (!m_ServiceManager->InitStageOne())
  {
    return false;
  }

  Preflight();

  // here we register all global classes for the CApplicationMessenger,
  // after that we can send messages to the corresponding modules
  CApplicationMessenger::GetInstance().RegisterReceiver(this);
  CApplicationMessenger::GetInstance().RegisterReceiver(&CServiceBroker::GetPlaylistPlayer());
  CApplicationMessenger::GetInstance().SetGUIThread(m_threadID);

  //! @todo - move to CPlatformXXX
#ifdef TARGET_POSIX
  tzset();   // Initialize timezone information variables
#endif


  //! @todo - move to CPlatformXXX
  #if defined(TARGET_POSIX)
    // set special://envhome
    if (getenv("HOME"))
    {
      CSpecialProtocol::SetEnvHomePath(getenv("HOME"));
    }
    else
    {
      fprintf(stderr, "The HOME environment variable is not set!\n");
      /* Cleanup. Leaving this out would lead to another crash */
      m_ServiceManager->DeinitStageOne();
      return false;
    }
  #endif

  // copy required files
  CopyUserDataIfNeeded("special://masterprofile/", "RssFeeds.xml");
  CopyUserDataIfNeeded("special://masterprofile/", "favourites.xml");
  CopyUserDataIfNeeded("special://masterprofile/", "Lircmap.xml");

  //! @todo - move to CPlatformXXX
  #ifdef TARGET_DARWIN_IOS
    CopyUserDataIfNeeded("special://masterprofile/", "iOS/sources.xml", "sources.xml");
  #endif

  if (!CLog::Init(CSpecialProtocol::TranslatePath("special://logpath").c_str()))
  {
    fprintf(stderr,"Could not init logging classes. Log folder error (%s)\n", CSpecialProtocol::TranslatePath("special://logpath").c_str());
    return false;
  }

#ifdef TARGET_POSIX //! @todo Win32 has no special://home/ mapping by default, so we
  //!       must create these here. Ideally this should be using special://home/ and
  //!      be platform agnostic (i.e. unify the InitDirectories*() functions)
  if (!m_bPlatformDirectories)
#endif
  {
    CDirectory::Create("special://xbmc/addons");
  }

  // Init our DllLoaders emu env
  init_emu_environ();

  CLog::Log(LOGNOTICE, "-----------------------------------------------------------------------");
  CLog::Log(LOGNOTICE, "Starting %s (%s). Platform: %s %s %d-bit", CSysInfo::GetAppName().c_str(), CSysInfo::GetVersion().c_str(),
            g_sysinfo.GetBuildTargetPlatformName().c_str(), g_sysinfo.GetBuildTargetCpuFamily().c_str(), g_sysinfo.GetXbmcBitness());

  std::string buildType;
#if defined(_DEBUG)
  buildType = "Debug";
#elif defined(NDEBUG)
  buildType = "Release";
#else
  buildType = "Unknown";
#endif
  std::string specialVersion;

  //! @todo - move to CPlatformXXX
#if defined(TARGET_RASPBERRY_PI)
  specialVersion = " (version for Raspberry Pi)";
//#elif defined(some_ID) // uncomment for special version/fork
//  specialVersion = " (version for XXXX)";
#endif
  CLog::Log(LOGNOTICE, "Using %s %s x%d build%s", buildType.c_str(), CSysInfo::GetAppName().c_str(), g_sysinfo.GetXbmcBitness(), specialVersion.c_str());
  CLog::Log(LOGNOTICE, "%s compiled %s by %s for %s %s %d-bit %s (%s)", CSysInfo::GetAppName().c_str(), CSysInfo::GetBuildDate(), g_sysinfo.GetUsedCompilerNameAndVer().c_str(), g_sysinfo.GetBuildTargetPlatformName().c_str(),
            g_sysinfo.GetBuildTargetCpuFamily().c_str(), g_sysinfo.GetXbmcBitness(), g_sysinfo.GetBuildTargetPlatformVersionDecoded().c_str(),
            g_sysinfo.GetBuildTargetPlatformVersion().c_str());

  std::string deviceModel(g_sysinfo.GetModelName());
  if (!g_sysinfo.GetManufacturerName().empty())
    deviceModel = g_sysinfo.GetManufacturerName() + " " + (deviceModel.empty() ? std::string("device") : deviceModel);
  if (!deviceModel.empty())
    CLog::Log(LOGNOTICE, "Running on %s with %s, kernel: %s %s %d-bit version %s", deviceModel.c_str(), g_sysinfo.GetOsPrettyNameWithVersion().c_str(),
              g_sysinfo.GetKernelName().c_str(), g_sysinfo.GetKernelCpuFamily().c_str(), g_sysinfo.GetKernelBitness(), g_sysinfo.GetKernelVersionFull().c_str());
  else
    CLog::Log(LOGNOTICE, "Running on %s, kernel: %s %s %d-bit version %s", g_sysinfo.GetOsPrettyNameWithVersion().c_str(),
              g_sysinfo.GetKernelName().c_str(), g_sysinfo.GetKernelCpuFamily().c_str(), g_sysinfo.GetKernelBitness(), g_sysinfo.GetKernelVersionFull().c_str());

  CLog::Log(LOGNOTICE, "FFmpeg version/source: %s", av_version_info());

  std::string cpuModel(CServiceBroker::GetCPUInfo()->GetCPUModel());
  if (!cpuModel.empty())
    CLog::Log(LOGNOTICE, "Host CPU: %s, %d core%s available", cpuModel.c_str(),
              CServiceBroker::GetCPUInfo()->GetCPUCount(),
              (CServiceBroker::GetCPUInfo()->GetCPUCount() == 1) ? "" : "s");
  else
    CLog::Log(LOGNOTICE, "%d CPU core%s available", CServiceBroker::GetCPUInfo()->GetCPUCount(),
              (CServiceBroker::GetCPUInfo()->GetCPUCount() == 1) ? "" : "s");

  //! @todo - move to CPlatformXXX ???
#if defined(TARGET_WINDOWS)
  CLog::Log(LOGNOTICE, "%s", CWIN32Util::GetResInfoString().c_str());
  CLog::Log(LOGNOTICE, "Running with %s rights", (CWIN32Util::IsCurrentUserLocalAdministrator() == TRUE) ? "administrator" : "restricted");
  CLog::Log(LOGNOTICE, "Aero is %s", (g_sysinfo.IsAeroDisabled() == true) ? "disabled" : "enabled");
#endif
#if defined(TARGET_ANDROID)
  CLog::Log(LOGNOTICE,
        "Product: %s, Device: %s, Board: %s - Manufacturer: %s, Brand: %s, Model: %s, Hardware: %s",
        CJNIBuild::PRODUCT.c_str(), CJNIBuild::DEVICE.c_str(), CJNIBuild::BOARD.c_str(),
        CJNIBuild::MANUFACTURER.c_str(), CJNIBuild::BRAND.c_str(), CJNIBuild::MODEL.c_str(), CJNIBuild::HARDWARE.c_str());
  std::string extstorage;
  bool extready = CXBMCApp::GetExternalStorage(extstorage);
  CLog::Log(LOGNOTICE, "External storage path = %s; status = %s", extstorage.c_str(), extready ? "ok" : "nok");
#endif

#if defined(__arm__) || defined(__aarch64__)
  if (CServiceBroker::GetCPUInfo()->GetCPUFeatures() & CPU_FEATURE_NEON)
    CLog::Log(LOGNOTICE, "ARM Features: Neon enabled");
  else
    CLog::Log(LOGNOTICE, "ARM Features: Neon disabled");
#endif
  CSpecialProtocol::LogPaths();

  std::string executable = CUtil::ResolveExecutablePath();
  CLog::Log(LOGNOTICE, "The executable running is: %s", executable.c_str());
  std::string hostname("[unknown]");
  m_ServiceManager->GetNetwork().GetHostName(hostname);
  CLog::Log(LOGNOTICE, "Local hostname: %s", hostname.c_str());
  std::string lowerAppName = CCompileInfo::GetAppName();
  StringUtils::ToLower(lowerAppName);
  CLog::Log(LOGNOTICE, "Log File is located: %s.log", CSpecialProtocol::TranslatePath("special://logpath/" + lowerAppName).c_str());
  CRegExp::LogCheckUtf8Support();
  CLog::Log(LOGNOTICE, "-----------------------------------------------------------------------");

  std::string strExecutablePath = CUtil::GetHomePath();

  // for python scripts that check the OS
  //! @todo - move to CPlatformXXX
#if defined(TARGET_DARWIN)
  setenv("OS","OS X",true);
#elif defined(TARGET_POSIX)
  setenv("OS","Linux",true);
#elif defined(TARGET_WINDOWS)
  CEnvironment::setenv("OS", "win32");
#endif

  // initialize network protocols
  avformat_network_init();
  // set avutil callback
  av_log_set_callback(ff_avutil_log);

  CLog::Log(LOGINFO, "loading settings");
  if (!m_pSettingsComponent->Load())
    return false;

  CLog::Log(LOGINFO, "creating subdirectories");
  const std::shared_ptr<CProfileManager> profileManager = m_pSettingsComponent->GetProfileManager();
  const std::shared_ptr<CSettings> settings = m_pSettingsComponent->GetSettings();
  CLog::Log(LOGINFO, "userdata folder: %s", CURL::GetRedacted(profileManager->GetProfileUserDataFolder()).c_str());
  CLog::Log(LOGINFO, "recording folder: %s", CURL::GetRedacted(settings->GetString(CSettings::SETTING_AUDIOCDS_RECORDINGPATH)).c_str());
  CLog::Log(LOGINFO, "screenshots folder: %s", CURL::GetRedacted(settings->GetString(CSettings::SETTING_DEBUG_SCREENSHOTPATH)).c_str());
  CDirectory::Create(profileManager->GetUserDataFolder());
  CDirectory::Create(profileManager->GetProfileUserDataFolder());
  profileManager->CreateProfileFolders();

  update_emu_environ();//apply the GUI settings

  //! @todo - move to CPlatformXXX
#ifdef TARGET_WINDOWS
  CWIN32Util::SetThreadLocalLocale(true); // enable independent locale for each thread, see https://connect.microsoft.com/VisualStudio/feedback/details/794122
#endif // TARGET_WINDOWS

  // application inbound service
  m_pAppPort = std::make_shared<CAppInboundProtocol>(*this);
  CServiceBroker::RegisterAppPort(m_pAppPort);

  m_pWinSystem = CWinSystemBase::CreateWinSystem();
  CServiceBroker::RegisterWinSystem(m_pWinSystem.get());

  if (!m_ServiceManager->InitStageTwo(params, m_pSettingsComponent->GetProfileManager()->GetProfileUserDataFolder()))
  {
    return false;
  }

  m_pActiveAE.reset(new ActiveAE::CActiveAE());
  m_pActiveAE->Start();
  CServiceBroker::RegisterAE(m_pActiveAE.get());

  // restore AE's previous volume state
  SetHardwareVolume(m_volumeLevel);
  CServiceBroker::GetActiveAE()->SetMute(m_muted);

  // initialize m_replayGainSettings
  m_replayGainSettings.iType = settings->GetInt(CSettings::SETTING_MUSICPLAYER_REPLAYGAINTYPE);
  m_replayGainSettings.iPreAmp = settings->GetInt(CSettings::SETTING_MUSICPLAYER_REPLAYGAINPREAMP);
  m_replayGainSettings.iNoGainPreAmp = settings->GetInt(CSettings::SETTING_MUSICPLAYER_REPLAYGAINNOGAINPREAMP);
  m_replayGainSettings.bAvoidClipping = settings->GetBool(CSettings::SETTING_MUSICPLAYER_REPLAYGAINAVOIDCLIPPING);

  // load the keyboard layouts
  if (!CKeyboardLayoutManager::GetInstance().Load())
  {
    CLog::Log(LOGFATAL, "CApplication::Create: Unable to load keyboard layouts");
    return false;
  }

  //! @todo - move to CPlatformXXX
#if defined(TARGET_DARWIN_OSX)
  // Configure and possible manually start the helper.
  XBMCHelper::GetInstance().Configure();
#endif

  CUtil::InitRandomSeed();

  m_lastRenderTime = XbmcThreads::SystemClockMillis();
  return true;
}

bool CApplication::CreateGUI()
{
  m_frameMoveGuard.lock();

  m_renderGUI = true;

  if (!CServiceBroker::GetWinSystem()->InitWindowSystem())
  {
    CLog::Log(LOGFATAL, "CApplication::Create: Unable to init windowing system");
    return false;
  }

  // Retrieve the matching resolution based on GUI settings
  bool sav_res = false;
  CDisplaySettings::GetInstance().SetCurrentResolution(CDisplaySettings::GetInstance().GetDisplayResolution());
  CLog::Log(LOGNOTICE, "Checking resolution %i", CDisplaySettings::GetInstance().GetCurrentResolution());
  if (!CServiceBroker::GetWinSystem()->GetGfxContext().IsValidResolution(CDisplaySettings::GetInstance().GetCurrentResolution()))
  {
    CLog::Log(LOGNOTICE, "Setting safe mode %i", RES_DESKTOP);
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

  m_pGUI.reset(new CGUIComponent());
  m_pGUI->Init();

  // Splash requires gui component!!
  CServiceBroker::GetRenderSystem()->ShowSplash("");

  // The key mappings may already have been loaded by a peripheral
  CLog::Log(LOGINFO, "load keymapping");
  if (!CServiceBroker::GetInputManager().LoadKeymaps())
    return false;

  RESOLUTION_INFO info = CServiceBroker::GetWinSystem()->GetGfxContext().GetResInfo();
  CLog::Log(LOGINFO, "GUI format %ix%i, Display %s",
            info.iWidth,
            info.iHeight,
            info.strMode.c_str());

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
#if defined(HAS_DVD_DRIVE) && !defined(TARGET_WINDOWS) // somehow this throws an "unresolved external symbol" on win32
  // turn off cdio logging
  cdio_loglevel_default = CDIO_LOG_ERROR;
#endif

  // load the language and its translated strings
  if (!LoadLanguage(false))
    return false;

  const std::shared_ptr<CProfileManager> profileManager = CServiceBroker::GetSettingsComponent()->GetProfileManager();

  profileManager->GetEventLog().Add(EventPtr(new CNotificationEvent(
    StringUtils::Format(g_localizeStrings.Get(177).c_str(), g_sysinfo.GetAppName().c_str()),
    StringUtils::Format(g_localizeStrings.Get(178).c_str(), g_sysinfo.GetAppName().c_str()),
    "special://xbmc/media/icon256x256.png", EventLevel::Basic)));

  m_ServiceManager->GetNetwork().WaitForNet();

  // initialize (and update as needed) our databases
  CDatabaseManager &databaseManager = m_ServiceManager->GetDatabaseManager();

  CEvent event(true);
  CJobManager::GetInstance().Submit([&databaseManager, &event]() {
    databaseManager.Initialize();
    event.Set();
  });

  std::string localizedStr = g_localizeStrings.Get(24150);
  int iDots = 1;
  while (!event.WaitMSec(1000))
  {
    if (databaseManager.IsUpgrading())
      CServiceBroker::GetRenderSystem()->ShowSplash(std::string(iDots, ' ') + localizedStr + std::string(iDots, '.'));

    if (iDots == 3)
      iDots = 1;
    else
      ++iDots;
  }
  CServiceBroker::GetRenderSystem()->ShowSplash("");

  StartServices();

  // GUI depends on seek handler
  m_appPlayer.GetSeekHandler().Configure();

  bool uiInitializationFinished = false;

  if (CServiceBroker::GetGUI()->GetWindowManager().Initialized())
  {
    const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();

    CServiceBroker::GetGUI()->GetWindowManager().CreateWindows();

    m_confirmSkinChange = false;

    std::vector<std::string> incompatibleAddons;
    event.Reset();
    std::atomic<bool> isMigratingAddons(false);
    CJobManager::GetInstance().Submit([&event, &incompatibleAddons, &isMigratingAddons]() {
        incompatibleAddons = CAddonSystemSettings::GetInstance().MigrateAddons([&isMigratingAddons]() {
          isMigratingAddons = true;
        });
        event.Set();
      }, CJob::PRIORITY_DEDICATED);
    localizedStr = g_localizeStrings.Get(24151);
    iDots = 1;
    while (!event.WaitMSec(1000))
    {
      if (isMigratingAddons)
        CServiceBroker::GetRenderSystem()->ShowSplash(std::string(iDots, ' ') + localizedStr + std::string(iDots, '.'));
      if (iDots == 3)
        iDots = 1;
      else
        ++iDots;
    }
    CServiceBroker::GetRenderSystem()->ShowSplash("");
    m_incompatibleAddons = incompatibleAddons;
    m_confirmSkinChange = true;

    std::string defaultSkin = std::static_pointer_cast<const CSettingString>(settings->GetSetting(CSettings::SETTING_LOOKANDFEEL_SKIN))->GetDefault();
    if (!LoadSkin(settings->GetString(CSettings::SETTING_LOOKANDFEEL_SKIN)))
    {
      CLog::Log(LOGERROR, "Failed to load skin '%s'", settings->GetString(CSettings::SETTING_LOOKANDFEEL_SKIN).c_str());
      if (!LoadSkin(defaultSkin))
      {
        CLog::Log(LOGFATAL, "Default skin '%s' could not be loaded! Terminating..", defaultSkin.c_str());
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
  RegisterActionListener(&m_appPlayer.GetSeekHandler());
  RegisterActionListener(&CPlayerController::GetInstance());

  CServiceBroker::GetRepositoryUpdater().Start();
  if (!profileManager->UsingLoginScreen())
    CServiceBroker::GetServiceAddons().Start();

  CLog::Log(LOGNOTICE, "initialize done");

  CheckOSScreenSaverInhibitionSetting();
  // reset our screensaver (starts timers etc.)
  ResetScreenSaver();

  // if the user interfaces has been fully initialized let everyone know
  if (uiInitializationFinished)
  {
    CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UI_READY);
    CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
  }

  return true;
}

bool CApplication::StartServer(enum ESERVERS eServer, bool bStart, bool bWait/* = false*/)
{
  bool ret = false;
  const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  switch(eServer)
  {
    case ES_WEBSERVER:
      // the callback will take care of starting/stopping webserver
      ret = settings->SetBool(CSettings::SETTING_SERVICES_WEBSERVER, bStart);
      break;

    case ES_AIRPLAYSERVER:
      // the callback will take care of starting/stopping airplay
      ret = settings->SetBool(CSettings::SETTING_SERVICES_AIRPLAY, bStart);
      break;

    case ES_JSONRPCSERVER:
      // the callback will take care of starting/stopping jsonrpc server
      ret = settings->SetBool(CSettings::SETTING_SERVICES_ESENABLED, bStart);
      break;

    case ES_UPNPSERVER:
      // the callback will take care of starting/stopping upnp server
      ret = settings->SetBool(CSettings::SETTING_SERVICES_UPNPSERVER, bStart);
      break;

    case ES_UPNPRENDERER:
      // the callback will take care of starting/stopping upnp renderer
      ret = settings->SetBool(CSettings::SETTING_SERVICES_UPNPRENDERER, bStart);
      break;

    case ES_EVENTSERVER:
      // the callback will take care of starting/stopping event server
      ret = settings->SetBool(CSettings::SETTING_SERVICES_ESENABLED, bStart);
      break;

    case ES_ZEROCONF:
      // the callback will take care of starting/stopping zeroconf
      ret = settings->SetBool(CSettings::SETTING_SERVICES_ZEROCONF, bStart);
      break;

    default:
      ret = false;
      break;
  }
  settings->Save();

  return ret;
}

void CApplication::StartServices()
{
#if !defined(TARGET_WINDOWS) && defined(HAS_DVD_DRIVE)
  // Start Thread for DVD Mediatype detection
  CLog::Log(LOGNOTICE, "start dvd mediatype detection");
  m_DetectDVDType.Create(false);
#endif
}

void CApplication::StopServices()
{
  m_ServiceManager->GetNetwork().NetworkMessage(CNetwork::SERVICES_DOWN, 0);

#if !defined(TARGET_WINDOWS) && defined(HAS_DVD_DRIVE)
  CLog::Log(LOGNOTICE, "stop dvd detect media");
  m_DetectDVDType.StopThread();
#endif
}

void CApplication::OnSettingChanged(std::shared_ptr<const CSetting> setting)
{
  if (setting == NULL)
    return;

  const std::string &settingId = setting->GetId();

  if (settingId == CSettings::SETTING_LOOKANDFEEL_SKIN ||
      settingId == CSettings::SETTING_LOOKANDFEEL_FONT ||
      settingId == CSettings::SETTING_LOOKANDFEEL_SKINTHEME ||
      settingId == CSettings::SETTING_LOOKANDFEEL_SKINCOLORS)
  {
    // check if we should ignore this change event due to changing skins in which case we have to
    // change several settings and each one of them could lead to a complete skin reload which would
    // result in multiple skin reloads. Therefore we manually specify to ignore specific settings
    // which are going to be changed.
    if (m_ignoreSkinSettingChanges)
      return;

    // if the skin changes and the current color/theme/font is not the default one, reset
    // the it to the default value
    if (settingId == CSettings::SETTING_LOOKANDFEEL_SKIN)
    {
      const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
      SettingPtr skinRelatedSetting = settings->GetSetting(CSettings::SETTING_LOOKANDFEEL_SKINCOLORS);
      if (!skinRelatedSetting->IsDefault())
      {
        m_ignoreSkinSettingChanges = true;
        skinRelatedSetting->Reset();
      }

      skinRelatedSetting = settings->GetSetting(CSettings::SETTING_LOOKANDFEEL_SKINTHEME);
      if (!skinRelatedSetting->IsDefault())
      {
        m_ignoreSkinSettingChanges = true;
        skinRelatedSetting->Reset();
      }

      skinRelatedSetting = settings->GetSetting(CSettings::SETTING_LOOKANDFEEL_FONT);
      if (!skinRelatedSetting->IsDefault())
      {
        m_ignoreSkinSettingChanges = true;
        skinRelatedSetting->Reset();
      }
    }
    else if (settingId == CSettings::SETTING_LOOKANDFEEL_SKINTHEME)
    {
      std::shared_ptr<CSettingString> skinColorsSetting = std::static_pointer_cast<CSettingString>(CServiceBroker::GetSettingsComponent()->GetSettings()->GetSetting(CSettings::SETTING_LOOKANDFEEL_SKINCOLORS));
      m_ignoreSkinSettingChanges = true;

      // we also need to adjust the skin color setting
      std::string colorTheme = std::static_pointer_cast<const CSettingString>(setting)->GetValue();
      URIUtils::RemoveExtension(colorTheme);
      if (setting->IsDefault() || StringUtils::EqualsNoCase(colorTheme, "Textures"))
        skinColorsSetting->Reset();
      else
        skinColorsSetting->SetValue(colorTheme);
    }

    m_ignoreSkinSettingChanges = false;

    if (g_SkinInfo)
    {
      // now we can finally reload skins
      std::string builtin("ReloadSkin");
      if (settingId == CSettings::SETTING_LOOKANDFEEL_SKIN && m_confirmSkinChange)
        builtin += "(confirm)";
      CApplicationMessenger::GetInstance().PostMsg(TMSG_EXECUTE_BUILT_IN, -1, -1, nullptr, builtin);
    }
  }
  else if (settingId == CSettings::SETTING_LOOKANDFEEL_SKINZOOM)
  {
    CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_WINDOW_RESIZE);
    CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
  }
  else if (settingId == CSettings::SETTING_SCREENSAVER_MODE)
  {
    CheckOSScreenSaverInhibitionSetting();
  }
  else if (settingId == CSettings::SETTING_VIDEOSCREEN_FAKEFULLSCREEN)
  {
    if (CServiceBroker::GetWinSystem()->GetGfxContext().IsFullScreenRoot())
      CServiceBroker::GetWinSystem()->GetGfxContext().SetVideoResolution(CServiceBroker::GetWinSystem()->GetGfxContext().GetVideoResolution(), true);
  }
  else if (settingId == CSettings::SETTING_AUDIOOUTPUT_PASSTHROUGH)
  {
    CApplicationMessenger::GetInstance().PostMsg(TMSG_MEDIA_RESTART);
  }
  else if (StringUtils::EqualsNoCase(settingId, CSettings::SETTING_MUSICPLAYER_REPLAYGAINTYPE))
    m_replayGainSettings.iType = std::static_pointer_cast<const CSettingInt>(setting)->GetValue();
  else if (StringUtils::EqualsNoCase(settingId, CSettings::SETTING_MUSICPLAYER_REPLAYGAINPREAMP))
    m_replayGainSettings.iPreAmp = std::static_pointer_cast<const CSettingInt>(setting)->GetValue();
  else if (StringUtils::EqualsNoCase(settingId, CSettings::SETTING_MUSICPLAYER_REPLAYGAINNOGAINPREAMP))
    m_replayGainSettings.iNoGainPreAmp = std::static_pointer_cast<const CSettingInt>(setting)->GetValue();
  else if (StringUtils::EqualsNoCase(settingId, CSettings::SETTING_MUSICPLAYER_REPLAYGAINAVOIDCLIPPING))
    m_replayGainSettings.bAvoidClipping = std::static_pointer_cast<const CSettingBool>(setting)->GetValue();
}

void CApplication::OnSettingAction(std::shared_ptr<const CSetting> setting)
{
  if (setting == NULL)
    return;

  const std::string &settingId = setting->GetId();
  if (settingId == CSettings::SETTING_LOOKANDFEEL_SKINSETTINGS)
    CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(WINDOW_SKIN_SETTINGS);
  else if (settingId == CSettings::SETTING_SCREENSAVER_PREVIEW)
    ActivateScreenSaver(true);
  else if (settingId == CSettings::SETTING_SCREENSAVER_SETTINGS)
  {
    AddonPtr addon;
    if (CServiceBroker::GetAddonMgr().GetAddon(CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(CSettings::SETTING_SCREENSAVER_MODE), addon, ADDON_SCREENSAVER))
      CGUIDialogAddonSettings::ShowForAddon(addon);
  }
  else if (settingId == CSettings::SETTING_AUDIOCDS_SETTINGS)
  {
    AddonPtr addon;
    if (CServiceBroker::GetAddonMgr().GetAddon(CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(CSettings::SETTING_AUDIOCDS_ENCODER), addon, ADDON_AUDIOENCODER))
      CGUIDialogAddonSettings::ShowForAddon(addon);
  }
  else if (settingId == CSettings::SETTING_VIDEOSCREEN_GUICALIBRATION)
    CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(WINDOW_SCREEN_CALIBRATION);
  else if (settingId == CSettings::SETTING_SOURCE_VIDEOS)
  {
    std::vector<std::string> params{"library://video/files.xml", "return"};
    CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(WINDOW_VIDEO_NAV, params);
  }
  else if (settingId == CSettings::SETTING_SOURCE_MUSIC)
  {
    std::vector<std::string> params{"library://music/files.xml", "return"};
    CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(WINDOW_MUSIC_NAV, params);
  }
  else if (settingId == CSettings::SETTING_SOURCE_PICTURES)
    CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(WINDOW_PICTURES);
}

bool CApplication::OnSettingUpdate(std::shared_ptr<CSetting> setting, const char *oldSettingId, const TiXmlNode *oldSettingNode)
{
  if (setting == NULL)
    return false;

#if defined(TARGET_DARWIN_OSX)
  if (setting->GetId() == CSettings::SETTING_AUDIOOUTPUT_AUDIODEVICE)
  {
    std::shared_ptr<CSettingString> audioDevice = std::static_pointer_cast<CSettingString>(setting);
    // Gotham and older didn't enumerate audio devices per stream on osx
    // add stream0 per default which should be ok for all old settings.
    if (!StringUtils::EqualsNoCase(audioDevice->GetValue(), "DARWINOSX:default") &&
        StringUtils::FindWords(audioDevice->GetValue().c_str(), ":stream") == std::string::npos)
    {
      std::string newSetting = audioDevice->GetValue();
      newSetting += ":stream0";
      return audioDevice->SetValue(newSetting);
    }
  }
#endif

  return false;
}

bool CApplication::OnSettingsSaving() const
{
  // don't save settings when we're busy stopping the application
  // a lot of screens try to save settings on deinit and deinit is
  // called for every screen when the application is stopping
  return !m_bStop;
}

void CApplication::ReloadSkin(bool confirm/*=false*/)
{
  if (!g_SkinInfo || m_bInitializing)
    return; // Don't allow reload before skin is loaded by system

  std::string oldSkin = g_SkinInfo->ID();

  CGUIMessage msg(GUI_MSG_LOAD_SKIN, -1, CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow());
  CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);

  const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  std::string newSkin = settings->GetString(CSettings::SETTING_LOOKANDFEEL_SKIN);
  if (LoadSkin(newSkin))
  {
    /* The Reset() or SetString() below will cause recursion, so the m_confirmSkinChange boolean is set so as to not prompt the
       user as to whether they want to keep the current skin. */
    if (confirm && m_confirmSkinChange)
    {
      if (HELPERS::ShowYesNoDialogText(CVariant{13123}, CVariant{13111}, CVariant{""}, CVariant{""}, 10000) !=
        DialogResponse::YES)
      {
        m_confirmSkinChange = false;
        settings->SetString(CSettings::SETTING_LOOKANDFEEL_SKIN, oldSkin);
      }
    }
  }
  else
  {
    // skin failed to load - we revert to the default only if we didn't fail loading the default
    std::string defaultSkin = std::static_pointer_cast<CSettingString>(settings->GetSetting(CSettings::SETTING_LOOKANDFEEL_SKIN))->GetDefault();
    if (newSkin != defaultSkin)
    {
      m_confirmSkinChange = false;
      settings->GetSetting(CSettings::SETTING_LOOKANDFEEL_SKIN)->Reset();
      CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Error, g_localizeStrings.Get(24102), g_localizeStrings.Get(24103));
    }
  }
  m_confirmSkinChange = true;
}

bool CApplication::Load(const TiXmlNode *settings)
{
  if (settings == NULL)
    return false;

  const TiXmlElement *audioElement = settings->FirstChildElement("audio");
  if (audioElement != NULL)
  {
    XMLUtils::GetBoolean(audioElement, "mute", m_muted);
    if (!XMLUtils::GetFloat(audioElement, "fvolumelevel", m_volumeLevel, VOLUME_MINIMUM, VOLUME_MAXIMUM))
      m_volumeLevel = VOLUME_MAXIMUM;
  }

  return true;
}

bool CApplication::Save(TiXmlNode *settings) const
{
  if (settings == NULL)
    return false;

  TiXmlElement volumeNode("audio");
  TiXmlNode *audioNode = settings->InsertEndChild(volumeNode);
  if (audioNode == NULL)
    return false;

  XMLUtils::SetBoolean(audioNode, "mute", m_muted);
  XMLUtils::SetFloat(audioNode, "fvolumelevel", m_volumeLevel);

  return true;
}

bool CApplication::LoadSkin(const std::string& skinID)
{
  SkinPtr skin;
  {
    AddonPtr addon;
    if (!CServiceBroker::GetAddonMgr().GetAddon(skinID, addon, ADDON_SKIN))
      return false;
    skin = std::static_pointer_cast<ADDON::CSkinInfo>(addon);
  }

  // store player and rendering state
  bool bPreviousPlayingState = false;

  enum class RENDERING_STATE
  {
    NONE,
    VIDEO,
    GAME,
  } previousRenderingState = RENDERING_STATE::NONE;

  if (m_appPlayer.IsPlayingVideo())
  {
    bPreviousPlayingState = !m_appPlayer.IsPausedPlayback();
    if (bPreviousPlayingState)
      m_appPlayer.Pause();
    m_appPlayer.FlushRenderer();
    if (CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_FULLSCREEN_VIDEO)
    {
      CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(WINDOW_HOME);
      previousRenderingState = RENDERING_STATE::VIDEO;
    }
    else if (CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_FULLSCREEN_GAME)
    {
      CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(WINDOW_HOME);
      previousRenderingState = RENDERING_STATE::GAME;
    }

  }

  CSingleLock lock(CServiceBroker::GetWinSystem()->GetGfxContext());

  // store current active window with its focused control
  int currentWindowID = CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow();
  int currentFocusedControlID = -1;
  if (currentWindowID != WINDOW_INVALID)
  {
    CGUIWindow* pWindow = CServiceBroker::GetGUI()->GetWindowManager().GetWindow(currentWindowID);
    if (pWindow)
      currentFocusedControlID = pWindow->GetFocusedControlID();
  }

  UnloadSkin();

  skin->Start();

  // migrate any skin-specific settings that are still stored in guisettings.xml
  CSkinSettings::GetInstance().MigrateSettings(skin);

  // check if the skin has been properly loaded and if it has a Home.xml
  if (!skin->HasSkinFile("Home.xml"))
  {
    CLog::Log(LOGERROR, "failed to load requested skin '%s'", skin->ID().c_str());
    return false;
  }

  CLog::Log(LOGNOTICE, "  load skin from: %s (version: %s)", skin->Path().c_str(), skin->Version().asString().c_str());
  g_SkinInfo = skin;

  CLog::Log(LOGINFO, "  load fonts for skin...");
  CServiceBroker::GetWinSystem()->GetGfxContext().SetMediaDir(skin->Path());
  g_directoryCache.ClearSubPaths(skin->Path());

  const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  CServiceBroker::GetGUI()->GetColorManager().Load(settings->GetString(CSettings::SETTING_LOOKANDFEEL_SKINCOLORS));

  g_SkinInfo->LoadIncludes();

  g_fontManager.LoadFonts(settings->GetString(CSettings::SETTING_LOOKANDFEEL_FONT));

  // load in the skin strings
  std::string langPath = URIUtils::AddFileToFolder(skin->Path(), "language");
  URIUtils::AddSlashAtEnd(langPath);

  g_localizeStrings.LoadSkinStrings(langPath, settings->GetString(CSettings::SETTING_LOCALE_LANGUAGE));


  int64_t start;
  start = CurrentHostCounter();

  CLog::Log(LOGINFO, "  load new skin...");

  // Load custom windows
  LoadCustomWindows();

  int64_t end, freq;
  end = CurrentHostCounter();
  freq = CurrentHostFrequency();
  CLog::Log(LOGDEBUG,"Load Skin XML: %.2fms", 1000.f * (end - start) / freq);

  CLog::Log(LOGINFO, "  initialize new skin...");
  CServiceBroker::GetGUI()->GetWindowManager().AddMsgTarget(this);
  CServiceBroker::GetGUI()->GetWindowManager().AddMsgTarget(&CServiceBroker::GetPlaylistPlayer());
  CServiceBroker::GetGUI()->GetWindowManager().AddMsgTarget(&g_fontManager);
  CServiceBroker::GetGUI()->GetWindowManager().AddMsgTarget(&CServiceBroker::GetGUI()->GetStereoscopicsManager());
  CServiceBroker::GetGUI()->GetWindowManager().SetCallback(*this);
  //@todo should be done by GUIComponents
  CServiceBroker::GetGUI()->GetWindowManager().Initialize();
  CTextureCache::GetInstance().Initialize();
  CServiceBroker::GetGUI()->GetAudioManager().Enable(true);
  CServiceBroker::GetGUI()->GetAudioManager().Load();

  if (g_SkinInfo->HasSkinFile("DialogFullScreenInfo.xml"))
    CServiceBroker::GetGUI()->GetWindowManager().Add(new CGUIDialogFullScreenInfo);

  CLog::Log(LOGINFO, "  skin loaded...");

  // leave the graphics lock
  lock.Leave();

  // restore active window
  if (currentWindowID != WINDOW_INVALID)
  {
    CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(currentWindowID);
    if (currentFocusedControlID != -1)
    {
      CGUIWindow *pWindow = CServiceBroker::GetGUI()->GetWindowManager().GetWindow(currentWindowID);
      if (pWindow && pWindow->HasSaveLastControl())
      {
        CGUIMessage msg(GUI_MSG_SETFOCUS, currentWindowID, currentFocusedControlID, 0);
        pWindow->OnMessage(msg);
      }
    }
  }

  // restore player and rendering state
  if (m_appPlayer.IsPlayingVideo())
  {
    if (bPreviousPlayingState)
      m_appPlayer.Pause();

    switch (previousRenderingState)
    {
    case RENDERING_STATE::VIDEO:
      CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(WINDOW_FULLSCREEN_VIDEO);
      break;
    case RENDERING_STATE::GAME:
      CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(WINDOW_FULLSCREEN_GAME);
      break;
    default:
      break;
    }
  }

  return true;
}

void CApplication::UnloadSkin(bool forReload /* = false */)
{
  CLog::Log(LOGINFO, "Unloading old skin %s...", forReload ? "for reload " : "");

  if (g_SkinInfo != nullptr && m_saveSkinOnUnloading)
    g_SkinInfo->SaveSettings();
  else if (!m_saveSkinOnUnloading)
    m_saveSkinOnUnloading = true;

  CGUIComponent *gui = CServiceBroker::GetGUI();
  if (gui)
  {
    gui->GetAudioManager().Enable(false);

    gui->GetWindowManager().DeInitialize();
    CTextureCache::GetInstance().Deinitialize();

    // remove the skin-dependent window
    gui->GetWindowManager().Delete(WINDOW_DIALOG_FULLSCREEN_INFO);

    gui->GetTextureManager().Cleanup();
    gui->GetLargeTextureManager().CleanupUnusedImages(true);

    g_fontManager.Clear();

    gui->GetColorManager().Clear();

    gui->GetInfoManager().Clear();
  }

//  The g_SkinInfo shared_ptr ought to be reset here
// but there are too many places it's used without checking for NULL
// and as a result a race condition on exit can cause a crash.
}

bool CApplication::LoadCustomWindows()
{
  // Start from wherever home.xml is
  std::vector<std::string> vecSkinPath;
  g_SkinInfo->GetSkinPaths(vecSkinPath);

  for (const auto &skinPath : vecSkinPath)
  {
    CLog::Log(LOGINFO, "Loading custom window XMLs from skin path %s", skinPath.c_str());

    CFileItemList items;
    if (CDirectory::GetDirectory(skinPath, items, ".xml", DIR_FLAG_NO_FILE_DIRS))
    {
      for (const auto &item : items)
      {
        if (item->m_bIsFolder)
          continue;

        std::string skinFile = URIUtils::GetFileName(item->GetPath());
        if (StringUtils::StartsWithNoCase(skinFile, "custom"))
        {
          CXBMCTinyXML xmlDoc;
          if (!xmlDoc.LoadFile(item->GetPath()))
          {
            CLog::Log(LOGERROR, "Unable to load custom window XML %s. Line %d\n%s", item->GetPath().c_str(), xmlDoc.ErrorRow(), xmlDoc.ErrorDesc());
            continue;
          }

          // Root element should be <window>
          TiXmlElement* pRootElement = xmlDoc.RootElement();
          std::string strValue = pRootElement->Value();
          if (!StringUtils::EqualsNoCase(strValue, "window"))
          {
            CLog::Log(LOGERROR, "No <window> root element found for custom window in %s", skinFile.c_str());
            continue;
          }

          int id = WINDOW_INVALID;

          // Read the type attribute or element to get the window type to create
          // If no type is specified, create a CGUIWindow as default
          std::string strType;
          if (pRootElement->Attribute("type"))
            strType = pRootElement->Attribute("type");
          else
          {
            const TiXmlNode *pType = pRootElement->FirstChild("type");
            if (pType && pType->FirstChild())
              strType = pType->FirstChild()->Value();
          }

          // Read the id attribute or element to get the window id
          if (!pRootElement->Attribute("id", &id))
          {
            const TiXmlNode *pType = pRootElement->FirstChild("id");
            if (pType && pType->FirstChild())
              id = atol(pType->FirstChild()->Value());
          }

          int windowId = id + WINDOW_HOME;
          if (id == WINDOW_INVALID || CServiceBroker::GetGUI()->GetWindowManager().GetWindow(windowId))
          {
            // No id specified or id already in use
            CLog::Log(LOGERROR, "No id specified or id already in use for custom window in %s", skinFile.c_str());
            continue;
          }

          CGUIWindow* pWindow = NULL;
          bool hasVisibleCondition = false;

          if (StringUtils::EqualsNoCase(strType, "dialog"))
          {
            hasVisibleCondition = pRootElement->FirstChildElement("visible") != nullptr;
            pWindow = new CGUIDialog(windowId, skinFile);
          }
          else if (StringUtils::EqualsNoCase(strType, "submenu"))
          {
            pWindow = new CGUIDialogSubMenu(windowId, skinFile);
          }
          else if (StringUtils::EqualsNoCase(strType, "buttonmenu"))
          {
            pWindow = new CGUIDialogButtonMenu(windowId, skinFile);
          }
          else
          {
            pWindow = new CGUIWindow(windowId, skinFile);
          }

          if (!pWindow)
          {
            CLog::Log(LOGERROR, "Failed to create custom window from %s", skinFile.c_str());
            continue;
          }

          pWindow->SetCustom(true);

          // Determining whether our custom dialog is modeless (visible condition is present)
          // will be done on load. Therefore we need to initialize the custom dialog on gui init.
          pWindow->SetLoadType(hasVisibleCondition ? CGUIWindow::LOAD_ON_GUI_INIT : CGUIWindow::KEEP_IN_MEMORY);

          CServiceBroker::GetGUI()->GetWindowManager().AddCustomWindow(pWindow);
        }
      }
    }
  }
  return true;
}

void CApplication::Render()
{
  // do not render if we are stopped or in background
  if (m_bStop)
    return;

  bool hasRendered = false;

  // Whether externalplayer is playing and we're unfocused
  bool extPlayerActive = m_appPlayer.IsExternalPlaying() && !m_AppFocused;

  if (!extPlayerActive && CServiceBroker::GetWinSystem()->GetGfxContext().IsFullScreenVideo() && !m_appPlayer.IsPausedPlayback())
  {
    ResetScreenSaver();
  }

  if(!CServiceBroker::GetRenderSystem()->BeginRender())
    return;

  // render gui layer
  if (m_renderGUI && !m_skipGuiRender)
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

    m_lastRenderTime = XbmcThreads::SystemClockMillis();
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

  CServiceBroker::GetWinSystem()->GetGfxContext().Flip(hasRendered, m_appPlayer.IsRenderingVideoLayer());

  CTimeUtils::UpdateFrameTime(hasRendered);
}

bool CApplication::OnAction(const CAction &action)
{
  // special case for switching between GUI & fullscreen mode.
  if (action.GetID() == ACTION_SHOW_GUI)
  { // Switch to fullscreen mode if we can
    if (SwitchToFullScreen())
    {
      m_navigationTimer.StartZero();
      return true;
    }
  }

  if (action.GetID() == ACTION_TOGGLE_FULLSCREEN)
  {
    CServiceBroker::GetWinSystem()->GetGfxContext().ToggleFullScreen();
    m_appPlayer.TriggerUpdateResolution();
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
    if (m_appPlayer.IsPlaying() && m_appPlayer.GetPlaySpeed() == 1)
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
      m_navigationTimer.StartZero();
      return true;
    }
  }

  // handle extra global presses

  // notify action listeners
  if (NotifyActionListeners(action))
    return true;

  // screenshot : take a screenshot :)
  if (action.GetID() == ACTION_TAKE_SCREENSHOT)
  {
    CScreenShot::TakeScreenshot();
    return true;
  }
  // built in functions : execute the built-in
  if (action.GetID() == ACTION_BUILT_IN_FUNCTION)
  {
    if (!CBuiltins::GetInstance().IsSystemPowerdownCommand(action.GetName()) ||
        CServiceBroker::GetPVRManager().GUIActions()->CanSystemPowerdown())
    {
      CBuiltins::GetInstance().Execute(action.GetName());
      m_navigationTimer.StartZero();
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

  if ((action.GetID() == ACTION_SET_RATING) && m_appPlayer.IsPlayingAudio())
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

  else if ((action.GetID() == ACTION_INCREASE_RATING || action.GetID() == ACTION_DECREASE_RATING) && m_appPlayer.IsPlayingAudio())
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
  else if ((action.GetID() == ACTION_INCREASE_RATING || action.GetID() == ACTION_DECREASE_RATING) && m_appPlayer.IsPlayingVideo())
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
  if (!(action.GetID() == ACTION_PREV_ITEM && m_appPlayer.CanSeek() && GetTime() > ACTION_PREV_ITEM_THRESHOLD) )
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
    if (m_appPlayer.OnAction(action))
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
  if (action.GetID() == ACTION_PREV_ITEM && m_appPlayer.CanSeek())
  {
    SeekTime(0);
    m_appPlayer.SetPlaySpeed(1);
    return true;
  }

  // forward action to graphic context and see if it can handle it
  if (CServiceBroker::GetGUI()->GetStereoscopicsManager().OnAction(action))
    return true;

  if (m_appPlayer.IsPlaying())
  {
    // forward channel switches to the player - he knows what to do
    if (action.GetID() == ACTION_CHANNEL_UP || action.GetID() == ACTION_CHANNEL_DOWN)
    {
      m_appPlayer.OnAction(action);
      return true;
    }

    // pause : toggle pause action
    if (action.GetID() == ACTION_PAUSE)
    {
      m_appPlayer.Pause();
      // go back to normal play speed on unpause
      if (!m_appPlayer.IsPaused() && m_appPlayer.GetPlaySpeed() != 1)
        m_appPlayer.SetPlaySpeed(1);

      CGUIComponent *gui = CServiceBroker::GetGUI();
      if (gui)
        gui->GetAudioManager().Enable(m_appPlayer.IsPaused());
      return true;
    }
    // play: unpause or set playspeed back to normal
    if (action.GetID() == ACTION_PLAYER_PLAY)
    {
      // if currently paused - unpause
      if (m_appPlayer.IsPaused())
        return OnAction(CAction(ACTION_PAUSE));
      // if we do a FF/RW then go back to normal speed
      if (m_appPlayer.GetPlaySpeed() != 1)
        m_appPlayer.SetPlaySpeed(1);
      return true;
    }
    if (!m_appPlayer.IsPaused())
    {
      if (action.GetID() == ACTION_PLAYER_FORWARD || action.GetID() == ACTION_PLAYER_REWIND)
      {
        float playSpeed = m_appPlayer.GetPlaySpeed();

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

        m_appPlayer.SetPlaySpeed(playSpeed);
        return true;
      }
      else if ((action.GetAmount() || m_appPlayer.GetPlaySpeed() != 1) && (action.GetID() == ACTION_ANALOG_REWIND || action.GetID() == ACTION_ANALOG_FORWARD))
      {
        // calculate the speed based on the amount the button is held down
        int iPower = (int)(action.GetAmount() * MAX_FFWD_SPEED + 0.5f);
        // amount can be negative, for example rewind and forward share the same axis
        iPower = std::abs(iPower);
        // returns 0 -> MAX_FFWD_SPEED
        int iSpeed = 1 << iPower;
        if (iSpeed != 1 && action.GetID() == ACTION_ANALOG_REWIND)
          iSpeed = -iSpeed;
        m_appPlayer.SetPlaySpeed(static_cast<float>(iSpeed));
        if (iSpeed == 1)
          CLog::Log(LOGDEBUG,"Resetting playspeed");
        return true;
      }
    }
    // allow play to unpause
    else
    {
      if (action.GetID() == ACTION_PLAYER_PLAY)
      {
        // unpause, and set the playspeed back to normal
        m_appPlayer.Pause();

        CGUIComponent *gui = CServiceBroker::GetGUI();
        if (gui)
          gui->GetAudioManager().Enable(m_appPlayer.IsPaused());

        m_appPlayer.SetPlaySpeed(1);
        return true;
      }
    }
  }


  if (action.GetID() == ACTION_SWITCH_PLAYER)
  {
    const CPlayerCoreFactory &playerCoreFactory = m_ServiceManager->GetPlayerCoreFactory();

    if(m_appPlayer.IsPlaying())
    {
      std::vector<std::string> players;
      CFileItem item(*m_itemCurrentFile.get());
      playerCoreFactory.GetPlayers(item, players);
      std::string player = playerCoreFactory.SelectPlayerDialog(players);
      if (!player.empty())
      {
        item.m_lStartOffset = CUtil::ConvertSecsToMilliSecs(GetTime());
        PlayFile(std::move(item), player, true);
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
    ToggleMute();
    ShowVolumeBar(&action);
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
    if (!m_appPlayer.IsPassthrough())
    {
      if (m_muted)
        UnMute();
      float volume = m_volumeLevel;
      int volumesteps = CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_AUDIOOUTPUT_VOLUMESTEPS);
      // sanity check
      if (volumesteps == 0)
        volumesteps = 90;

// Android has steps based on the max available volume level
#if defined(TARGET_ANDROID)
      float step = (VOLUME_MAXIMUM - VOLUME_MINIMUM) / CXBMCApp::GetMaxSystemVolume();
#else
      float step   = (VOLUME_MAXIMUM - VOLUME_MINIMUM) / volumesteps;

      if (action.GetRepeat())
        step *= action.GetRepeat() * 50; // 50 fps
#endif
      if (action.GetID() == ACTION_VOLUME_UP)
        volume += action.GetAmount() * action.GetAmount() * step;
      else if (action.GetID() == ACTION_VOLUME_DOWN)
        volume -= action.GetAmount() * action.GetAmount() * step;
      else
        volume = action.GetAmount() * step;
      if (volume != m_volumeLevel)
        SetVolume(volume, false);
    }
    // show visual feedback of volume or passthrough indicator
    ShowVolumeBar(&action);
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
    int iPlaylist = CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist();
    if (iPlaylist == PLAYLIST_VIDEO && CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() != WINDOW_VIDEO_PLAYLIST)
      CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(WINDOW_VIDEO_PLAYLIST);
    else if (iPlaylist == PLAYLIST_MUSIC && CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() != WINDOW_MUSIC_PLAYLIST)
      CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(WINDOW_MUSIC_PLAYLIST);
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
    if (CServiceBroker::GetPVRManager().GUIActions()->CanSystemPowerdown())
      msg = pMsg->param1; // perform requested shutdown action
    else
      return; // no shutdown
  }

  switch (msg)
  {
  case TMSG_POWERDOWN:
    Stop(EXITCODE_POWERDOWN);
    CServiceBroker::GetPowerManager().Powerdown();
    break;

  case TMSG_QUIT:
    Stop(EXITCODE_QUIT);
    break;

  case TMSG_SHUTDOWN:
    HandleShutdownMessage();
    break;

  case TMSG_RENDERER_FLUSH:
    m_appPlayer.FlushRenderer();
    break;

  case TMSG_HIBERNATE:
    CServiceBroker::GetPowerManager().Hibernate();
    break;

  case TMSG_SUSPEND:
    CServiceBroker::GetPowerManager().Suspend();
    break;

  case TMSG_RESTART:
  case TMSG_RESET:
    Stop(EXITCODE_REBOOT);
    CServiceBroker::GetPowerManager().Reboot();
    break;

  case TMSG_RESTARTAPP:
#if defined(TARGET_WINDOWS) || defined(TARGET_LINUX)
    Stop(EXITCODE_RESTARTAPP);
#endif
    break;

  case TMSG_INHIBITIDLESHUTDOWN:
    InhibitIdleShutdown(pMsg->param1 != 0);
    break;

  case TMSG_INHIBITSCREENSAVER:
    InhibitScreenSaver(pMsg->param1 != 0);
    break;

  case TMSG_ACTIVATESCREENSAVER:
    ActivateScreenSaver();
    break;

  case TMSG_VOLUME_SHOW:
  {
    CAction action(pMsg->param1);
    ShowVolumeBar(&action);
  }
  break;

#ifdef TARGET_ANDROID
  case TMSG_DISPLAY_SETUP:
    // We might come from a refresh rate switch destroying the native window; use the context resolution
    *static_cast<bool*>(pMsg->lpVoid) = InitWindow(CServiceBroker::GetWinSystem()->GetGfxContext().GetVideoResolution());
    SetRenderGUI(true);
    break;

  case TMSG_DISPLAY_DESTROY:
    *static_cast<bool*>(pMsg->lpVoid) = CServiceBroker::GetWinSystem()->DestroyWindow();
    SetRenderGUI(false);
    break;
#endif

  case TMSG_START_ANDROID_ACTIVITY:
  {
#if defined(TARGET_ANDROID)
    if (pMsg->params.size())
    {
      CXBMCApp::StartActivity(pMsg->params[0],
        pMsg->params.size() > 1 ? pMsg->params[1] : "",
        pMsg->params.size() > 2 ? pMsg->params[2] : "",
        pMsg->params.size() > 3 ? pMsg->params[3] : "");
    }
#endif
  }
  break;

  case TMSG_NETWORKMESSAGE:
    m_ServiceManager->GetNetwork().NetworkMessage((CNetwork::EMESSAGE)pMsg->param1, pMsg->param2);
    break;

  case TMSG_SETLANGUAGE:
    SetLanguage(pMsg->strParam);
    break;


  case TMSG_SWITCHTOFULLSCREEN:
    if (CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() != WINDOW_FULLSCREEN_VIDEO &&
        CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() != WINDOW_FULLSCREEN_GAME)
      SwitchToFullScreen(true);
    break;

  case TMSG_VIDEORESIZE:
  {
    XBMC_Event newEvent;
    memset(&newEvent, 0, sizeof(newEvent));
    newEvent.type = XBMC_VIDEORESIZE;
    newEvent.resize.w = pMsg->param1;
    newEvent.resize.h = pMsg->param2;
    OnEvent(newEvent);
    CServiceBroker::GetGUI()->GetWindowManager().MarkDirty();
  }
    break;

  case TMSG_SETVIDEORESOLUTION:
    CServiceBroker::GetWinSystem()->GetGfxContext().SetVideoResolution(static_cast<RESOLUTION>(pMsg->param1), pMsg->param2 == 1);
    break;

  case TMSG_TOGGLEFULLSCREEN:
    CServiceBroker::GetWinSystem()->GetGfxContext().ToggleFullScreen();
    m_appPlayer.TriggerUpdateResolution();
    break;

  case TMSG_MINIMIZE:
    Minimize();
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
        CLog::Log(LOGNOTICE, "%s: Failed to suspend AudioEngine before launching external program", __FUNCTION__);
      }
    }
#if defined(TARGET_DARWIN)
    CLog::Log(LOGNOTICE, "ExecWait is not implemented on this platform");
#elif defined(TARGET_POSIX)
    CUtil::RunCommandLine(pMsg->strParam.c_str(), (pMsg->param1 == 1));
#elif defined(TARGET_WINDOWS)
    CWIN32Util::XBMCShellExecute(pMsg->strParam.c_str(), (pMsg->param1 == 1));
#endif
    // Resume AE processing of XBMC native audio
    if (audioengine)
    {
      if (!audioengine->Resume())
      {
        CLog::Log(LOGFATAL, "%s: Failed to restart AudioEngine after return from external player", __FUNCTION__);
      }
    }
    break;

  case TMSG_EXECUTE_SCRIPT:
    CScriptInvocationManager::GetInstance().ExecuteAsync(pMsg->strParam);
    break;

  case TMSG_EXECUTE_BUILT_IN:
    CBuiltins::GetInstance().Execute(pMsg->strParam.c_str());
    break;

  case TMSG_PICTURE_SHOW:
  {
    CGUIWindowSlideShow *pSlideShow = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIWindowSlideShow>(WINDOW_SLIDESHOW);
    if (!pSlideShow) return;

    // stop playing file
    if (m_appPlayer.IsPlayingVideo()) StopPlaying();

    if (CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_FULLSCREEN_VIDEO)
      CServiceBroker::GetGUI()->GetWindowManager().PreviousWindow();

    ResetScreenSaver();
    WakeUpScreenSaverAndDPMS();

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
        pSlideShow->Reset();
        for (int i = 0; i<items.Size(); ++i)
        {
          pSlideShow->Add(items[i].get());
        }
        pSlideShow->Select(items[0]->GetPath());
      }
    }
    else
    {
      CFileItem item(pMsg->strParam, false);
      pSlideShow->Reset();
      pSlideShow->Add(&item);
      pSlideShow->Select(pMsg->strParam);
    }
  }
  break;

  case TMSG_PICTURE_SLIDESHOW:
  {
    CGUIWindowSlideShow *pSlideShow = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIWindowSlideShow>(WINDOW_SLIDESHOW);
    if (!pSlideShow) return;

    if (m_appPlayer.IsPlayingVideo())
      StopPlaying();

    pSlideShow->Reset();

    CFileItemList items;
    std::string strPath = pMsg->strParam;
    std::string extensions = CServiceBroker::GetFileExtensionProvider().GetPictureExtensions();
    if (pMsg->param1)
      extensions += "|.tbn";
    CUtil::GetRecursiveListing(strPath, items, extensions);

    if (items.Size() > 0)
    {
      for (int i = 0; i<items.Size(); ++i)
        pSlideShow->Add(items[i].get());
      pSlideShow->StartSlideShow(); //Start the slideshow!
    }

    if (CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() != WINDOW_SLIDESHOW)
    {
      if (items.Size() == 0)
      {
        CServiceBroker::GetSettingsComponent()->GetSettings()->SetString(CSettings::SETTING_SCREENSAVER_MODE, "screensaver.xbmc.builtin.dim");
        ActivateScreenSaver();
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

  default:
    CLog::Log(LOGERROR, "%s: Unhandled threadmessage sent, %u", __FUNCTION__, msg);
    break;
  }
}

void CApplication::HandleShutdownMessage()
{
  switch (CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_POWERMANAGEMENT_SHUTDOWNSTATE))
  {
  case POWERSTATE_SHUTDOWN:
    CApplicationMessenger::GetInstance().PostMsg(TMSG_POWERDOWN);
    break;

  case POWERSTATE_SUSPEND:
    CApplicationMessenger::GetInstance().PostMsg(TMSG_SUSPEND);
    break;

  case POWERSTATE_HIBERNATE:
    CApplicationMessenger::GetInstance().PostMsg(TMSG_HIBERNATE);
    break;

  case POWERSTATE_QUIT:
    CApplicationMessenger::GetInstance().PostMsg(TMSG_QUIT);
    break;

  case POWERSTATE_MINIMIZE:
    CApplicationMessenger::GetInstance().PostMsg(TMSG_MINIMIZE);
    break;

  default:
    CLog::Log(LOGERROR, "%s: No valid shutdownstate matched", __FUNCTION__);
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
  if (processEvents)
  {
    // currently we calculate the repeat time (ie time from last similar keypress) just global as fps
    float frameTime = m_frameTime.GetElapsedSeconds();
    m_frameTime.StartZero();
    // never set a frametime less than 2 fps to avoid problems when debugging and on breaks
    if( frameTime > 0.5 )
      frameTime = 0.5;

    if (processGUI && m_renderGUI)
    {
      CSingleLock lock(CServiceBroker::GetWinSystem()->GetGfxContext());
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

    HandlePortEvents();
    CServiceBroker::GetInputManager().Process(CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindowOrDialog(), frameTime);

    if (processGUI && m_renderGUI)
    {
      m_pInertialScrollingHandler->ProcessInertialScroll(frameTime);
      m_appPlayer.GetSeekHandler().FrameMove();
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
      if (!m_appPlayer.IsPlayingVideo() || m_appPlayer.IsPausedPlayback())
        max_sleep = 80;
      unsigned int sleepTime = std::max(static_cast<unsigned int>(2), std::min(m_ProcessedExternalCalls >> 2, max_sleep));
      Sleep(sleepTime);
      m_frameMoveGuard.lock();
      m_ProcessedExternalDecay = 5;
    }
    if (m_ProcessedExternalDecay && --m_ProcessedExternalDecay == 0)
      m_ProcessedExternalCalls = 0;
  }

  if (processGUI && m_renderGUI)
  {
    m_skipGuiRender = false;
#if defined(TARGET_RASPBERRY_PI)
    int fps = 0;

    // This code reduces rendering fps of the GUI layer when playing videos in fullscreen mode
    // it makes only sense on architectures with multiple layers
    if (CServiceBroker::GetWinSystem()->GetGfxContext().IsFullScreenVideo() && !m_appPlayer.IsPausedPlayback() && m_appPlayer.IsRenderingVideoLayer())
      fps = CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_VIDEOPLAYER_LIMITGUIUPDATE);

    unsigned int now = XbmcThreads::SystemClockMillis();
    unsigned int frameTime = now - m_lastRenderTime;
    if (fps > 0 && frameTime * fps < 1000)
      m_skipGuiRender = true;
#endif

    if (CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_guiSmartRedraw && m_guiRefreshTimer.IsTimePast())
    {
      CServiceBroker::GetGUI()->GetWindowManager().SendMessage(GUI_MSG_REFRESH_TIMER, 0, 0);
      m_guiRefreshTimer.Set(500);
    }

    if (!m_bStop)
    {
      if (!m_skipGuiRender)
        CServiceBroker::GetGUI()->GetWindowManager().Process(CTimeUtils::GetFrameTime());
    }
    CServiceBroker::GetGUI()->GetWindowManager().FrameMove();
  }

  m_appPlayer.FrameMove();

  // this will go away when render systems gets its own thread
  CServiceBroker::GetWinSystem()->DriveRenderLoop();
}



bool CApplication::Cleanup()
{
  try
  {
    StopPlaying();

    if (m_ServiceManager)
      m_ServiceManager->DeinitStageThree();

    CLog::Log(LOGNOTICE, "unload skin");
    UnloadSkin();

    // stop all remaining scripts; must be done after skin has been unloaded,
    // not before some windows still need it when deinitializing during skin
    // unloading
    CScriptInvocationManager::GetInstance().Uninitialize();

    m_globalScreensaverInhibitor.Release();
    m_screensaverInhibitor.Release();

    CRenderSystemBase *renderSystem = CServiceBroker::GetRenderSystem();
    if (renderSystem)
      renderSystem->DestroyRenderSystem();

    CWinSystemBase *winSystem = CServiceBroker::GetWinSystem();
    if (winSystem)
      winSystem->DestroyWindow();

    if (m_pGUI)
      m_pGUI->GetWindowManager().DestroyWindows();

    CLog::Log(LOGNOTICE, "unload sections");

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
    DllLoaderContainer::Clear();
    CServiceBroker::GetPlaylistPlayer().Clear();

    if (m_ServiceManager)
      m_ServiceManager->DeinitStageTwo();

#ifdef TARGET_POSIX
    CXHandle::DumpObjectTracker();

#ifdef HAS_DVD_DRIVE
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

    m_pAnnouncementManager->Deinitialize();
    m_pAnnouncementManager.reset();

    m_pSettingsComponent->Deinit();
    m_pSettingsComponent.reset();

    CServiceBroker::UnregisterCPUInfo();

    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "Exception in CApplication::Cleanup()");
    return false;
  }
}

void CApplication::Stop(int exitCode)
{
  CLog::Log(LOGNOTICE, "Stopping player");
  m_appPlayer.ClosePlayer();

  {
    // close inbound port
    CServiceBroker::UnregisterAppPort();
    XbmcThreads::EndTime timer(1000);
    while (m_pAppPort.use_count() > 1)
    {
      Sleep(100);
      if (timer.IsTimePast())
      {
        CLog::Log(LOGERROR, "CApplication::Stop - CAppPort still in use, app may crash");
        break;
      }
    }
    m_pAppPort.reset();
  }

  try
  {
    m_frameMoveGuard.unlock();

    CVariant vExitCode(CVariant::VariantTypeObject);
    vExitCode["exitcode"] = exitCode;
    CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::System, "xbmc", "OnQuit", vExitCode);

    // Abort any active screensaver
    WakeUpScreenSaverAndDPMS();

    g_alarmClock.StopThread();

    CLog::Log(LOGNOTICE, "Storing total System Uptime");
    g_sysinfo.SetTotalUptime(g_sysinfo.GetTotalUptime() + (int)(CTimeUtils::GetFrameTime() / 60000));

    // Update the settings information (volume, uptime etc. need saving)
    if (CFile::Exists(CServiceBroker::GetSettingsComponent()->GetProfileManager()->GetSettingsFile()))
    {
      CLog::Log(LOGNOTICE, "Saving settings");
      CServiceBroker::GetSettingsComponent()->GetSettings()->Save();
    }
    else
      CLog::Log(LOGNOTICE, "Not saving settings (settings.xml is not present)");

    // kodi may crash or deadlock during exit (shutdown / reboot) due to
    // either a bug in core or misbehaving addons. so try saving
    // skin settings early
    CLog::Log(LOGNOTICE, "Saving skin settings");
    if (g_SkinInfo != nullptr)
      g_SkinInfo->SaveSettings();

    m_bStop = true;
    // Add this here to keep the same ordering behaviour for now
    // Needs cleaning up
    CApplicationMessenger::GetInstance().Stop();
    m_AppFocused = false;
    m_ExitCode = exitCode;
    CLog::Log(LOGNOTICE, "Stopping all");

    // cancel any jobs from the jobmanager
    CJobManager::GetInstance().CancelJobs();

    // stop scanning before we kill the network and so on
    if (CMusicLibraryQueue::GetInstance().IsRunning())
      CMusicLibraryQueue::GetInstance().CancelAllJobs();

    if (CVideoLibraryQueue::GetInstance().IsRunning())
      CVideoLibraryQueue::GetInstance().CancelAllJobs();

    CApplicationMessenger::GetInstance().Cleanup();

    StopServices();

#ifdef HAS_ZEROCONF
    if(CZeroconfBrowser::IsInstantiated())
    {
      CLog::Log(LOGNOTICE, "Stopping zeroconf browser");
      CZeroconfBrowser::GetInstance()->Stop();
      CZeroconfBrowser::ReleaseInstance();
    }
#endif

    for (const auto& vfsAddon : CServiceBroker::GetVFSAddonCache().GetAddonInstances())
      vfsAddon->DisconnectAll();

#if defined(TARGET_POSIX) && defined(HAS_FILESYSTEM_SMB)
    smb.Deinit();
#endif

#if defined(TARGET_DARWIN_OSX)
    if (XBMCHelper::GetInstance().IsAlwaysOn() == false)
      XBMCHelper::GetInstance().Stop();
#endif

    // Stop services before unloading Python
    CServiceBroker::GetServiceAddons().Stop();

    // unregister action listeners
    UnregisterActionListener(&m_appPlayer.GetSeekHandler());
    UnregisterActionListener(&CPlayerController::GetInstance());

    CGUIComponent *gui = CServiceBroker::GetGUI();
    if (gui)
      gui->GetAudioManager().DeInitialize();

    // shutdown the AudioEngine
    CServiceBroker::UnregisterAE();
    m_pActiveAE->Shutdown();
    m_pActiveAE.reset();

    CLog::Log(LOGNOTICE, "Application stopped");
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "Exception in CApplication::Stop()");
  }

  cleanup_emu_environ();

  Sleep(200);
}

bool CApplication::PlayMedia(CFileItem& item, const std::string &player, int iPlaylist)
{
  //If item is a plugin, expand out
  for (int i=0; URIUtils::IsPlugin(item.GetDynPath()) && i<5; ++i)
  {
    bool resume = item.m_lStartOffset == STARTOFFSET_RESUME;

    if (!XFILE::CPluginDirectory::GetPluginResult(item.GetDynPath(), item, resume))
      return false;
  }

  if (item.IsSmartPlayList())
  {
    CFileItemList items;
    CUtil::GetRecursiveListing(item.GetPath(), items, "", DIR_FLAG_NO_FILE_DIRS);
    if (items.Size())
    {
      CSmartPlaylist smartpl;
      //get name and type of smartplaylist, this will always succeed as GetDirectory also did this.
      smartpl.OpenAndReadName(item.GetURL());
      CPlayList playlist;
      playlist.Add(items);
      int iPlaylist = PLAYLIST_VIDEO;
      if (smartpl.GetType() == "songs" || smartpl.GetType() == "albums" ||
        smartpl.GetType() == "artists")
        iPlaylist = PLAYLIST_MUSIC;
      return ProcessAndStartPlaylist(smartpl.GetName(), playlist, iPlaylist);
    }
  }
  else if (item.IsPlayList() || item.IsInternetStream())
  {
    CGUIDialogCache* dlgCache = new CGUIDialogCache(5000, g_localizeStrings.Get(10214), item.GetLabel());

    //is or could be a playlist
    std::unique_ptr<CPlayList> pPlayList (CPlayListFactory::Create(item));
    bool gotPlayList = (pPlayList.get() && pPlayList->Load(item.GetPath()));

    if (dlgCache)
    {
       dlgCache->Close();
       if (dlgCache->IsCanceled())
          return true;
    }

    if (gotPlayList)
    {

      if (iPlaylist != PLAYLIST_NONE)
      {
        int track=0;
        if (item.HasProperty("playlist_starting_track"))
          track = (int)item.GetProperty("playlist_starting_track").asInteger();
        return ProcessAndStartPlaylist(item.GetPath(), *pPlayList, iPlaylist, track);
      }
      else
      {
        CLog::Log(LOGWARNING, "CApplication::PlayMedia called to play a playlist %s but no idea which playlist to use, playing first item", item.GetPath().c_str());
        if(pPlayList->size())
          return PlayFile(*(*pPlayList)[0], "", false);
      }
    }
  }
  else if (item.IsPVR())
  {
    return CServiceBroker::GetPVRManager().GUIActions()->PlayMedia(CFileItemPtr(new CFileItem(item)));
  }

  CURL path(item.GetPath());
  if (path.GetProtocol() == "game")
  {
    AddonPtr addon;
    if (CServiceBroker::GetAddonMgr().GetAddon(path.GetHostName(), addon, ADDON_GAMEDLL))
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
  if (!m_stackHelper.InitializeStack(item))
    return false;

  int startoffset = m_stackHelper.InitializeStackStartPartAndOffset(item);

  CFileItem selectedStackPart = m_stackHelper.GetCurrentStackPartFileItem();
  selectedStackPart.m_lStartOffset = startoffset;

  if (item.HasProperty("savedplayerstate"))
  {
    selectedStackPart.SetProperty("savedplayerstate", item.GetProperty("savedplayerstate")); // pass on to part
    item.ClearProperty("savedplayerstate");
  }

  return PlayFile(selectedStackPart, "", true);
}

bool CApplication::PlayFile(CFileItem item, const std::string& player, bool bRestart)
{
  // Ensure the MIME type has been retrieved for http:// and shout:// streams
  if (item.GetMimeType().empty())
    item.FillInMimeType();

  if (!bRestart)
  {
    // bRestart will be true when called from PlayStack(), skipping this block
    m_appPlayer.SetPlaySpeed(1);

    m_nextPlaylistItem = -1;
    m_stackHelper.Clear();

    if (item.IsVideo())
      CUtil::ClearSubtitles();
  }

  if (item.IsDiscStub())
  {
#ifdef HAS_DVD_DRIVE
    // Display the Play Eject dialog if there is any optical disc drive
    if (CServiceBroker::GetMediaManager().HasOpticalDrive())
    {
      if (CGUIDialogPlayEject::ShowAndGetInput(item))
        // PlayDiscAskResume takes path to disc. No parameter means default DVD drive.
        // Can't do better as CGUIDialogPlayEject calls CMediaManager::IsDiscInDrive, which assumes default DVD drive anyway
        return MEDIA_DETECT::CAutorun::PlayDiscAskResume();
    }
    else
#endif
      HELPERS::ShowOKDialogText(CVariant{435}, CVariant{436});

    return true;
  }

  if (item.IsPlayList())
    return false;

  for (int i=0; URIUtils::IsPlugin(item.GetDynPath()) && i<5; ++i)
  { // we modify the item so that it becomes a real URL
    bool resume = item.m_lStartOffset == STARTOFFSET_RESUME;

    if (!XFILE::CPluginDirectory::GetPluginResult(item.GetDynPath(), item, resume))
      return false;
  }

#ifdef HAS_UPNP
  if (URIUtils::IsUPnP(item.GetPath()))
  {
    if (!XFILE::CUPnPDirectory::GetResource(item.GetURL(), item))
      return false;
  }
#endif

  // if we have a stacked set of files, we need to setup our stack routines for
  // "seamless" seeking and total time of the movie etc.
  // will recall with restart set to true
  if (item.IsStack())
    return PlayStack(item, bRestart);

  CPlayerOptions options;

  if (item.HasProperty("StartPercent"))
  {
    options.startpercent = item.GetProperty("StartPercent").asDouble();
    item.m_lStartOffset = 0;
  }

  options.starttime = CUtil::ConvertMilliSecsToSecs(item.m_lStartOffset);

  if (bRestart)
  {
    // have to be set here due to playstack using this for starting the file
    if (item.HasVideoInfoTag())
      options.state = item.GetVideoInfoTag()->GetResumePoint().playerState;
  }
  if (!bRestart || m_stackHelper.IsPlayingISOStack())
  {
    // the following code block is only applicable when bRestart is false OR to ISO stacks

    if (item.IsVideo())
    {
      // open the d/b and retrieve the bookmarks for the current movie
      CVideoDatabase dbs;
      dbs.Open();

      std::string path = item.GetPath();
      std::string videoInfoTagPath(item.GetVideoInfoTag()->m_strFileNameAndPath);
      if (videoInfoTagPath.find("removable://") == 0)
        path = videoInfoTagPath;
      dbs.LoadVideoInfo(path, *item.GetVideoInfoTag());

      if (item.HasProperty("savedplayerstate"))
      {
        options.starttime = CUtil::ConvertMilliSecsToSecs(item.m_lStartOffset);
        options.state = item.GetProperty("savedplayerstate").asString();
        item.ClearProperty("savedplayerstate");
      }
      else if (item.m_lStartOffset == STARTOFFSET_RESUME)
      {
        options.starttime = 0.0f;
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

        if (options.starttime == 0.0f && item.HasVideoInfoTag())
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
  if (!(options.startpercent > 0.0f || options.starttime > 0.0f) && (item.IsBDFile() || item.IsDiscImage()))
  {
    //check if we must show the simplified bd menu
    if (!CGUIDialogSimpleMenu::ShowPlaySelection(item))
      return true;
  }

  // this really aught to be inside !bRestart, but since PlayStack
  // uses that to init playback, we have to keep it outside
  int playlist = CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist();
  if (item.IsVideo() && playlist == PLAYLIST_VIDEO && CServiceBroker::GetPlaylistPlayer().GetPlaylist(playlist).size() > 1)
  { // playing from a playlist by the looks
    // don't switch to fullscreen if we are not playing the first item...
    options.fullscreen = !CServiceBroker::GetPlaylistPlayer().HasPlayedFirstFile() && CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_fullScreenOnMovieStart && !CMediaSettings::GetInstance().DoesVideoStartWindowed();
  }
  else if(m_stackHelper.IsPlayingRegularStack())
  {
    //! @todo - this will fail if user seeks back to first file in stack
    if(m_stackHelper.GetCurrentPartNumber() == 0 || m_stackHelper.GetRegisteredStack(item)->m_lStartOffset != 0)
      options.fullscreen = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_fullScreenOnMovieStart && !CMediaSettings::GetInstance().DoesVideoStartWindowed();
    else
      options.fullscreen = false;
  }
  else
    options.fullscreen = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_fullScreenOnMovieStart && !CMediaSettings::GetInstance().DoesVideoStartWindowed();

  // stereo streams may have lower quality, i.e. 32bit vs 16 bit
  options.preferStereo = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoPreferStereoStream &&
                         CServiceBroker::GetActiveAE()->HasStereoAudioChannelCount();

  // reset VideoStartWindowed as it's a temp setting
  CMediaSettings::GetInstance().SetVideoStartWindowed(false);

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
      CLog::LogF(LOGDEBUG,"Ignored %d playback thread messages", dMsgCount);
  }

  m_appPlayer.OpenFile(item, options, m_ServiceManager->GetPlayerCoreFactory(), player, *this);
  m_appPlayer.SetVolume(m_volumeLevel);
  m_appPlayer.SetMute(m_muted);

#if !defined(TARGET_POSIX)
  CGUIComponent *gui = CServiceBroker::GetGUI();
  if (gui)
    gui->GetAudioManager().Enable(false);
#endif

  if (item.HasPVRChannelInfoTag())
    CServiceBroker::GetPlaylistPlayer().SetCurrentPlaylist(PLAYLIST_NONE);

  return true;
}

void CApplication::PlaybackCleanup()
{
  if (!m_appPlayer.IsPlaying())
  {
    CGUIComponent *gui = CServiceBroker::GetGUI();
    if (gui)
      CServiceBroker::GetGUI()->GetAudioManager().Enable(true);
    m_appPlayer.OpenNext(m_ServiceManager->GetPlayerCoreFactory());
  }

  if (!m_appPlayer.IsPlayingVideo())
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

  if (!m_appPlayer.IsPlayingAudio() && CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist() == PLAYLIST_NONE && CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_VISUALISATION)
  {
    CServiceBroker::GetSettingsComponent()->GetSettings()->Save();  // save vis settings
    WakeUpScreenSaverAndDPMS();
    CServiceBroker::GetGUI()->GetWindowManager().PreviousWindow();
  }

  // DVD ejected while playing in vis ?
  if (!m_appPlayer.IsPlayingAudio() &&
      (m_itemCurrentFile->IsCDDA() || m_itemCurrentFile->IsOnDVD()) &&
      !CServiceBroker::GetMediaManager().IsDiscInDrive() &&
      CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_VISUALISATION)
  {
    // yes, disable vis
    CServiceBroker::GetSettingsComponent()->GetSettings()->Save();    // save vis settings
    WakeUpScreenSaverAndDPMS();
    CServiceBroker::GetGUI()->GetWindowManager().PreviousWindow();
  }

  if (!m_appPlayer.IsPlaying())
  {
    m_stackHelper.Clear();
    m_appPlayer.ResetPlayer();
  }

  if (IsEnableTestMode())
    CApplicationMessenger::GetInstance().PostMsg(TMSG_QUIT);
}

void CApplication::OnPlayBackEnded()
{
  CLog::LogF(LOGDEBUG ,"CApplication::OnPlayBackEnded");

  CServiceBroker::GetPVRManager().OnPlaybackEnded(m_itemCurrentFile);

  CVariant data(CVariant::VariantTypeObject);
  data["end"] = true;
  CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::Player, "xbmc", "OnStop", m_itemCurrentFile, data);

  CGUIMessage msg(GUI_MSG_PLAYBACK_ENDED, 0, 0);
  CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
}

void CApplication::OnPlayBackStarted(const CFileItem &file)
{
  CLog::LogF(LOGDEBUG,"CApplication::OnPlayBackStarted");

  // check if VideoPlayer should set file item stream details from its current streams
  if (file.GetProperty("get_stream_details_from_player").asBoolean())
    m_appPlayer.SetUpdateStreamDetails();

  if (m_stackHelper.IsPlayingISOStack() || m_stackHelper.IsPlayingRegularStack())
    m_itemCurrentFile.reset(new CFileItem(*m_stackHelper.GetRegisteredStack(file)));
  else
    m_itemCurrentFile.reset(new CFileItem(file));

  /* When playing video pause any low priority jobs, they will be unpaused  when playback stops.
   * This should speed up player startup for files on internet filesystems (eg. webdav) and
   * increase performance on low powered systems (Atom/ARM).
   */
  if (file.IsVideo() || file.IsGame())
  {
    CJobManager::GetInstance().PauseJobs();
  }

  CServiceBroker::GetPVRManager().OnPlaybackStarted(m_itemCurrentFile);
  m_stackHelper.OnPlayBackStarted(file);

  m_playerEvent.Reset();

  CGUIMessage msg(GUI_MSG_PLAYBACK_STARTED, 0, 0);
  CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
}

void CApplication::OnPlayerCloseFile(const CFileItem &file, const CBookmark &bookmarkParam)
{
  CSingleLock lock(m_stackHelper.m_critSection);

  CFileItem fileItem(file);
  CBookmark bookmark = bookmarkParam;
  CBookmark resumeBookmark;
  bool playCountUpdate = false;
  float percent = 0.0f;

  // Make sure we don't reset existing bookmark etc. on eg. player start failure
  if (bookmark.timeInSeconds == 0.0f)
    return;

  if (m_stackHelper.GetRegisteredStack(fileItem) != nullptr && m_stackHelper.GetRegisteredStackTotalTimeMs(fileItem) > 0)
  {
    // regular stack case: we have to save the bookmark on the stack
    fileItem = *m_stackHelper.GetRegisteredStack(file);
    // the bookmark coming from the player is only relative to the current part, thus needs to be corrected with these attributes (start time will be 0 for non-stackparts)
    bookmark.timeInSeconds += m_stackHelper.GetRegisteredStackPartStartTimeMs(file) / 1000.0;
    if (m_stackHelper.GetRegisteredStackTotalTimeMs(file) > 0)
      bookmark.totalTimeInSeconds = m_stackHelper.GetRegisteredStackTotalTimeMs(file) / 1000.0;
    bookmark.partNumber = m_stackHelper.GetRegisteredStackPartNumber(file);
  }

  percent = bookmark.timeInSeconds / bookmark.totalTimeInSeconds * 100;

  const std::shared_ptr<CAdvancedSettings> advancedSettings = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings();

  if ((fileItem.IsAudio() && advancedSettings->m_audioPlayCountMinimumPercent > 0 &&
       percent >= advancedSettings->m_audioPlayCountMinimumPercent) ||
      (fileItem.IsVideo() && advancedSettings->m_videoPlayCountMinimumPercent > 0 &&
       percent >= advancedSettings->m_videoPlayCountMinimumPercent))
  {
    playCountUpdate = true;
  }

  if (advancedSettings->m_videoIgnorePercentAtEnd > 0 &&
      bookmark.totalTimeInSeconds - bookmark.timeInSeconds <
        0.01f * advancedSettings->m_videoIgnorePercentAtEnd * bookmark.totalTimeInSeconds)
  {
    resumeBookmark.timeInSeconds = -1.0f;
  }
  else if (bookmark.timeInSeconds > advancedSettings->m_videoIgnoreSecondsAtStart)
  {
    resumeBookmark = bookmark;
    if (m_stackHelper.GetRegisteredStack(file) != nullptr)
    {
      // also update video info tag with total time
      fileItem.GetVideoInfoTag()->m_streamDetails.SetVideoDuration(0, resumeBookmark.totalTimeInSeconds);
    }
  }
  else
  {
    resumeBookmark.timeInSeconds = 0.0f;
  }

  if (CServiceBroker::GetSettingsComponent()->GetProfileManager()->GetCurrentProfile().canWriteDatabases())
  {
    CSaveFileState::DoWork(fileItem, resumeBookmark, playCountUpdate);
  }
}

void CApplication::OnQueueNextItem()
{
  CLog::LogF(LOGDEBUG,"CApplication::OnQueueNextItem");

  // informs python script currently running that we are requesting the next track
  // (does nothing if python is not loaded)
#ifdef HAS_PYTHON
  g_pythonParser.OnQueueNextItem(); // currently unimplemented
#endif

  CGUIMessage msg(GUI_MSG_QUEUE_NEXT_ITEM, 0, 0);
  CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
}

void CApplication::OnPlayBackStopped()
{
  CLog::LogF(LOGDEBUG, "CApplication::OnPlayBackStopped");

  CServiceBroker::GetPVRManager().OnPlaybackStopped(m_itemCurrentFile);

  CVariant data(CVariant::VariantTypeObject);
  data["end"] = false;
  CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::Player, "xbmc", "OnStop", m_itemCurrentFile, data);

  CGUIMessage msg(GUI_MSG_PLAYBACK_STOPPED, 0, 0);
  CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
}

void CApplication::OnPlayBackError()
{
  //@todo Playlists can be continued by calling OnPlaybackEnded instead
  // open error dialog
  CGUIMessage msg(GUI_MSG_PLAYBACK_ERROR, 0, 0);
  CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
  OnPlayBackStopped();
}

void CApplication::OnPlayBackPaused()
{
#ifdef HAS_PYTHON
  g_pythonParser.OnPlayBackPaused();
#endif

  CVariant param;
  param["player"]["speed"] = 0;
  param["player"]["playerid"] = CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist();
  CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::Player, "xbmc", "OnPause", m_itemCurrentFile, param);
}

void CApplication::OnPlayBackResumed()
{
#ifdef HAS_PYTHON
  g_pythonParser.OnPlayBackResumed();
#endif

  CVariant param;
  param["player"]["speed"] = 1;
  param["player"]["playerid"] = CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist();
  CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::Player, "xbmc", "OnResume", m_itemCurrentFile, param);
}

void CApplication::OnPlayBackSpeedChanged(int iSpeed)
{
#ifdef HAS_PYTHON
  g_pythonParser.OnPlayBackSpeedChanged(iSpeed);
#endif

  CVariant param;
  param["player"]["speed"] = iSpeed;
  param["player"]["playerid"] = CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist();
  CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::Player, "xbmc", "OnSpeedChanged", m_itemCurrentFile, param);
}

void CApplication::OnPlayBackSeek(int64_t iTime, int64_t seekOffset)
{
#ifdef HAS_PYTHON
  g_pythonParser.OnPlayBackSeek(static_cast<int>(iTime), static_cast<int>(seekOffset));
#endif

  CVariant param;
  CJSONUtils::MillisecondsToTimeObject(iTime, param["player"]["time"]);
  CJSONUtils::MillisecondsToTimeObject(seekOffset, param["player"]["seekoffset"]);
  param["player"]["playerid"] = CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist();
  param["player"]["speed"] = (int)m_appPlayer.GetPlaySpeed();
  CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::Player, "xbmc", "OnSeek", m_itemCurrentFile, param);
  CServiceBroker::GetGUI()->GetInfoManager().GetInfoProviders().GetPlayerInfoProvider().SetDisplayAfterSeek(2500, static_cast<int>(seekOffset));
}

void CApplication::OnPlayBackSeekChapter(int iChapter)
{
#ifdef HAS_PYTHON
  g_pythonParser.OnPlayBackSeekChapter(iChapter);
#endif
}

void CApplication::OnAVStarted(const CFileItem &file)
{
  CLog::LogF(LOGDEBUG, "CApplication::OnAVStarted");

  CGUIMessage msg(GUI_MSG_PLAYBACK_AVSTARTED, 0, 0);
  CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);

  CVariant param;
  param["player"]["speed"] = 1;
  param["player"]["playerid"] = CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist();
  CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::Player, "xbmc", "OnAVStart", m_itemCurrentFile, param);
}

void CApplication::OnAVChange()
{
  CLog::LogF(LOGDEBUG, "CApplication::OnAVChange");

  CServiceBroker::GetGUI()->GetStereoscopicsManager().OnStreamChange();

  CGUIMessage msg(GUI_MSG_PLAYBACK_AVCHANGE, 0, 0);
  CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);

  CVariant param;
  param["player"]["speed"] = 1;
  param["player"]["playerid"] = CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist();
  CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::Player, "xbmc", "OnAVChange", m_itemCurrentFile, param);
}

void CApplication::RequestVideoSettings(const CFileItem &fileItem)
{
  CVideoDatabase dbs;
  if (dbs.Open())
  {
    CLog::Log(LOGDEBUG, "Loading settings for %s", CURL::GetRedacted(fileItem.GetPath()).c_str());

    // Load stored settings if they exist, otherwise use default
    CVideoSettings vs;
    if (!dbs.GetVideoSettings(fileItem, vs))
      vs = CMediaSettings::GetInstance().GetDefaultVideoSettings();

    m_appPlayer.SetVideoSettings(vs);

    dbs.Close();
  }
}

void CApplication::StoreVideoSettings(const CFileItem &fileItem, CVideoSettings vs)
{
  CVideoDatabase dbs;
  if (dbs.Open())
  {
    if (vs != CMediaSettings::GetInstance().GetDefaultVideoSettings())
    {
      dbs.SetVideoSettings(fileItem, vs);
    }
    else
    {
      dbs.EraseVideoSettings(fileItem);
    }
    dbs.Close();
  }
}

bool CApplication::IsPlayingFullScreenVideo() const
{
  return m_appPlayer.IsPlayingVideo() && CServiceBroker::GetWinSystem()->GetGfxContext().IsFullScreenVideo();
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
    if (m_appPlayer.IsPlaying())
    {
      m_appPlayer.ClosePlayer();

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

void CApplication::ResetSystemIdleTimer()
{
  // reset system idle timer
  m_idleTimer.StartZero();
}

void CApplication::ResetScreenSaver()
{
  // reset our timers
  m_shutdownTimer.StartZero();

  // screen saver timer is reset only if we're not already in screensaver or
  // DPMS mode
  if ((!m_screensaverActive && m_iScreenSaveLock == 0) && !m_dpmsIsActive)
    ResetScreenSaverTimer();
}

void CApplication::ResetScreenSaverTimer()
{
  m_screenSaverTimer.StartZero();
}

void CApplication::StopScreenSaverTimer()
{
  m_screenSaverTimer.Stop();
}

bool CApplication::ToggleDPMS(bool manual)
{
  auto winSystem = CServiceBroker::GetWinSystem();
  if (!winSystem)
    return false;

  std::shared_ptr<CDPMSSupport> dpms = winSystem->GetDPMSManager();
  if (!dpms)
    return false;

  if (manual || (m_dpmsIsManual == manual))
  {
    if (m_dpmsIsActive)
    {
      m_dpmsIsActive = false;
      m_dpmsIsManual = false;
      SetRenderGUI(true);
      CheckOSScreenSaverInhibitionSetting();
      CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::GUI, "xbmc", "OnDPMSDeactivated");
      return dpms->DisablePowerSaving();
    }
    else
    {
      if (dpms->EnablePowerSaving(dpms->GetSupportedModes()[0]))
      {
        m_dpmsIsActive = true;
        m_dpmsIsManual = manual;
        SetRenderGUI(false);
        CheckOSScreenSaverInhibitionSetting();
        CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::GUI, "xbmc", "OnDPMSActivated");
        return true;
      }
    }
  }
  return false;
}

bool CApplication::WakeUpScreenSaverAndDPMS(bool bPowerOffKeyPressed /* = false */)
{
  bool result = false;

  // First reset DPMS, if active
  if (m_dpmsIsActive)
  {
    if (m_dpmsIsManual)
      return false;
    //! @todo if screensaver lock is specified but screensaver is not active
    //! (DPMS came first), activate screensaver now.
    ToggleDPMS(false);
    ResetScreenSaverTimer();
    result = !m_screensaverActive || WakeUpScreenSaver(bPowerOffKeyPressed);
  }
  else if (m_screensaverActive)
    result = WakeUpScreenSaver(bPowerOffKeyPressed);

  if(result)
  {
    // allow listeners to ignore the deactivation if it precedes a powerdown/suspend etc
    CVariant data(CVariant::VariantTypeObject);
    data["shuttingdown"] = bPowerOffKeyPressed;
    CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::GUI, "xbmc", "OnScreensaverDeactivated", data);
  }

  return result;
}

bool CApplication::WakeUpScreenSaver(bool bPowerOffKeyPressed /* = false */)
{
  if (m_iScreenSaveLock == 2)
    return false;

  // if Screen saver is active
  if (m_screensaverActive && !m_screensaverIdInUse.empty())
  {
    if (m_iScreenSaveLock == 0)
    {
      const std::shared_ptr<CProfileManager> profileManager = CServiceBroker::GetSettingsComponent()->GetProfileManager();
      if (profileManager->GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE &&
          (profileManager->UsingLoginScreen() || CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_MASTERLOCK_STARTUPLOCK)) &&
          profileManager->GetCurrentProfile().getLockMode() != LOCK_MODE_EVERYONE &&
          m_screensaverIdInUse != "screensaver.xbmc.builtin.dim" && m_screensaverIdInUse != "screensaver.xbmc.builtin.black" && m_screensaverIdInUse != "visualization")
      {
        m_iScreenSaveLock = 2;
        CGUIMessage msg(GUI_MSG_CHECK_LOCK,0,0);

        CGUIWindow* pWindow = CServiceBroker::GetGUI()->GetWindowManager().GetWindow(WINDOW_SCREENSAVER);
        if (pWindow)
          pWindow->OnMessage(msg);
      }
    }
    if (m_iScreenSaveLock == -1)
    {
      m_iScreenSaveLock = 0;
      return true;
    }

    // disable screensaver
    m_screensaverActive = false;
    m_iScreenSaveLock = 0;
    ResetScreenSaverTimer();

    if (m_screensaverIdInUse == "visualization")
    {
      // we can just continue as usual from vis mode
      return false;
    }
    else if (m_screensaverIdInUse == "screensaver.xbmc.builtin.dim" ||
             m_screensaverIdInUse == "screensaver.xbmc.builtin.black" ||
             m_screensaverIdInUse.empty())
    {
      return true;
    }
    else if (!m_screensaverIdInUse.empty())
    { // we're in screensaver window
      if (m_pythonScreenSaver)
      {
        // What sound does a python screensaver make?
        #define SCRIPT_ALARM "sssssscreensaver"
        #define SCRIPT_TIMEOUT 15 // seconds

        /* FIXME: This is a hack but a proper fix is non-trivial. Basically this code
        * makes sure the addon gets terminated after we've moved out of the screensaver window.
        * If we don't do this, we may simply lockup.
        */
        g_alarmClock.Start(SCRIPT_ALARM, SCRIPT_TIMEOUT, "StopScript(" + m_pythonScreenSaver->LibPath() + ")", true, false);
        m_pythonScreenSaver.reset();
      }
      if (CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_SCREENSAVER)
        CServiceBroker::GetGUI()->GetWindowManager().PreviousWindow();  // show the previous window
      else if (CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_SLIDESHOW)
        CApplicationMessenger::GetInstance().SendMsg(TMSG_GUI_ACTION, WINDOW_SLIDESHOW, -1, static_cast<void*>(new CAction(ACTION_STOP)));
    }
    return true;
  }
  else
    return false;
}

void CApplication::CheckOSScreenSaverInhibitionSetting()
{
  // Kodi screen saver overrides OS one: always inhibit OS screen saver then
  // except when DPMS is active (inhibiting the screen saver then might also
  // disable DPMS again)
  if (!m_dpmsIsActive &&
      !CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(CSettings::SETTING_SCREENSAVER_MODE).empty() &&
      CServiceBroker::GetWinSystem()->GetOSScreenSaver())
  {
    if (!m_globalScreensaverInhibitor)
    {
      m_globalScreensaverInhibitor = CServiceBroker::GetWinSystem()->GetOSScreenSaver()->CreateInhibitor();
    }
  }
  else if (m_globalScreensaverInhibitor)
  {
    m_globalScreensaverInhibitor.Release();
  }
}

void CApplication::CheckScreenSaverAndDPMS()
{
  bool maybeScreensaver = true;
  if (m_dpmsIsActive)
    maybeScreensaver = false;
  else if (m_screensaverActive)
    maybeScreensaver = false;
  else if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(CSettings::SETTING_SCREENSAVER_MODE).empty())
    maybeScreensaver = false;

  auto winSystem = CServiceBroker::GetWinSystem();
  if (!winSystem)
    return;

  std::shared_ptr<CDPMSSupport> dpms = winSystem->GetDPMSManager();

  bool maybeDPMS = true;
  if (m_dpmsIsActive)
    maybeDPMS = false;
  else if (!dpms || !dpms->IsSupported())
    maybeDPMS = false;
  else if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_POWERMANAGEMENT_DISPLAYSOFF) <= 0)
    maybeDPMS = false;

  // whether the current state of the application should be regarded as active even when there is no
  // explicit user activity such as input
  bool haveIdleActivity = false;

  // When inhibit screensaver is enabled prevent screensaver from kicking in
  if (m_bInhibitScreenSaver)
    haveIdleActivity = true;

  // Are we playing a video and it is not paused?
  if (m_appPlayer.IsPlayingVideo() && !m_appPlayer.IsPaused())
    haveIdleActivity = true;

  // Are we playing some music in fullscreen vis?
  else if (m_appPlayer.IsPlayingAudio() &&
           CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_VISUALISATION &&
           !CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(CSettings::SETTING_MUSICPLAYER_VISUALISATION).empty())
  {
    haveIdleActivity = true;
  }

  // Handle OS screen saver state
  if (haveIdleActivity && CServiceBroker::GetWinSystem()->GetOSScreenSaver())
  {
    // Always inhibit OS screen saver during these kinds of activities
    if (!m_screensaverInhibitor)
    {
      m_screensaverInhibitor = CServiceBroker::GetWinSystem()->GetOSScreenSaver()->CreateInhibitor();
    }
  }
  else if (m_screensaverInhibitor)
  {
    m_screensaverInhibitor.Release();
  }

  // Has the screen saver window become active?
  if (maybeScreensaver && CServiceBroker::GetGUI()->GetWindowManager().IsWindowActive(WINDOW_SCREENSAVER))
  {
    m_screensaverActive = true;
    maybeScreensaver = false;
  }

  if (m_screensaverActive && haveIdleActivity)
  {
    WakeUpScreenSaverAndDPMS();
    return;
  }

  if (!maybeScreensaver && !maybeDPMS) return;  // Nothing to do.

  // See if we need to reset timer.
  if (haveIdleActivity)
  {
    ResetScreenSaverTimer();
    return;
  }

  float elapsed = m_screenSaverTimer.IsRunning() ? m_screenSaverTimer.GetElapsedSeconds() : 0.f;

  // DPMS has priority (it makes the screensaver not needed)
  if (maybeDPMS
      && elapsed > CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_POWERMANAGEMENT_DISPLAYSOFF) * 60)
  {
    ToggleDPMS(false);
    WakeUpScreenSaver();
  }
  else if (maybeScreensaver
           && elapsed > CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_SCREENSAVER_TIME) * 60)
  {
    ActivateScreenSaver();
  }
}

// activate the screensaver.
// if forceType is true, we ignore the various conditions that can alter
// the type of screensaver displayed
void CApplication::ActivateScreenSaver(bool forceType /*= false */)
{
  const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  if (m_appPlayer.IsPlayingAudio() && settings->GetBool(CSettings::SETTING_SCREENSAVER_USEMUSICVISINSTEAD) &&
      !settings->GetString(CSettings::SETTING_MUSICPLAYER_VISUALISATION).empty())
  { // just activate the visualisation if user toggled the usemusicvisinstead option
    CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(WINDOW_VISUALISATION);
    return;
  }

  m_screensaverActive = true;
  CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::GUI, "xbmc", "OnScreensaverActivated");

  // disable screensaver lock from the login screen
  m_iScreenSaveLock = CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_LOGIN_SCREEN ? 1 : 0;

  m_screensaverIdInUse = settings->GetString(CSettings::SETTING_SCREENSAVER_MODE);

  if (!forceType)
  {
    if (m_screensaverIdInUse == "screensaver.xbmc.builtin.dim" ||
        m_screensaverIdInUse == "screensaver.xbmc.builtin.black" ||
        m_screensaverIdInUse.empty())
    {
      return;
    }

    // Enforce Dim for special cases.
    bool bUseDim = false;
    if (CServiceBroker::GetGUI()->GetWindowManager().HasModalDialog(true))
      bUseDim = true;
    else if (m_appPlayer.IsPlayingVideo() && settings->GetBool(CSettings::SETTING_SCREENSAVER_USEDIMONPAUSE))
      bUseDim = true;
    else if (CServiceBroker::GetPVRManager().GUIActions()->IsRunningChannelScan())
      bUseDim = true;

    if (bUseDim)
      m_screensaverIdInUse = "screensaver.xbmc.builtin.dim";
  }

  if (m_screensaverIdInUse == "screensaver.xbmc.builtin.dim" ||
      m_screensaverIdInUse == "screensaver.xbmc.builtin.black" ||
      m_screensaverIdInUse.empty())
  {
    return;
  }
  else if (CServiceBroker::GetAddonMgr().GetAddon(m_screensaverIdInUse, m_pythonScreenSaver, ADDON_SCREENSAVER))
  {
    std::string libPath = m_pythonScreenSaver->LibPath();
    if (CScriptInvocationManager::GetInstance().HasLanguageInvoker(libPath))
    {
      CLog::Log(LOGDEBUG, "using python screensaver add-on %s", m_screensaverIdInUse.c_str());

      // Don't allow a previously-scheduled alarm to kill our new screensaver
      g_alarmClock.Stop(SCRIPT_ALARM, true);

      if (!CScriptInvocationManager::GetInstance().Stop(libPath))
        CScriptInvocationManager::GetInstance().ExecuteAsync(libPath, AddonPtr(new CAddon(dynamic_cast<ADDON::CAddon&>(*m_pythonScreenSaver))));
      return;
    }
    m_pythonScreenSaver.reset();
  }

  CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(WINDOW_SCREENSAVER);
}

void CApplication::InhibitScreenSaver(bool inhibit)
{
  m_bInhibitScreenSaver = inhibit;
}

bool CApplication::IsScreenSaverInhibited() const
{
  return m_bInhibitScreenSaver;
}

void CApplication::CheckShutdown()
{
  // first check if we should reset the timer
  if (m_bInhibitIdleShutdown
      || m_appPlayer.IsPlaying() || m_appPlayer.IsPausedPlayback() // is something playing?
      || CMusicLibraryQueue::GetInstance().IsRunning()
      || CVideoLibraryQueue::GetInstance().IsRunning()
      || CServiceBroker::GetGUI()->GetWindowManager().IsWindowActive(WINDOW_DIALOG_PROGRESS) // progress dialog is onscreen
      || !CServiceBroker::GetPVRManager().GUIActions()->CanSystemPowerdown(false))
  {
    m_shutdownTimer.StartZero();
    return;
  }

  float elapsed = m_shutdownTimer.IsRunning() ? m_shutdownTimer.GetElapsedSeconds() : 0.f;
  if ( elapsed > CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_POWERMANAGEMENT_SHUTDOWNTIME) * 60 )
  {
    // Since it is a sleep instead of a shutdown, let's set everything to reset when we wake up.
    m_shutdownTimer.Stop();

    // Sleep the box
    CApplicationMessenger::GetInstance().PostMsg(TMSG_SHUTDOWN);
  }
}

void CApplication::InhibitIdleShutdown(bool inhibit)
{
  m_bInhibitIdleShutdown = inhibit;
}

bool CApplication::IsIdleShutdownInhibited() const
{
  return m_bInhibitIdleShutdown;
}

bool CApplication::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
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
        if (IsMuted() || GetVolumeRatio() <= VOLUME_MINIMUM)
          ShowVolumeBar();

        if (!m_incompatibleAddons.empty())
        {
          auto addonList = StringUtils::Join(m_incompatibleAddons, ", ");
          auto msg = StringUtils::Format(g_localizeStrings.Get(24149).c_str(), addonList.c_str());
          HELPERS::ShowOKDialogText(CVariant{24148}, CVariant{std::move(msg)});
          m_incompatibleAddons.clear();
        }

        // show info dialog about moved configuration files if needed
        ShowAppMigrationMessage();

        m_bInitializing = false;

        if (message.GetSenderId() == WINDOW_SETTINGS_PROFILES)
          g_application.ReloadSkin(false);
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
      CDarwinUtils::SetScheduling(m_appPlayer.IsPlayingVideo());
#endif
      CPlayList playList = CServiceBroker::GetPlaylistPlayer().GetPlaylist(CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist());

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
        int currentSong = CServiceBroker::GetPlaylistPlayer().GetCurrentSong();
        int param = ((currentSong & 0xffff) << 16) | (m_nextPlaylistItem & 0xffff);
        CGUIMessage msg(GUI_MSG_PLAYLISTPLAYER_CHANGED, 0, 0, CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist(), param, item);
        CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
        CServiceBroker::GetPlaylistPlayer().SetCurrentSong(m_nextPlaylistItem);
        m_itemCurrentFile.reset(new CFileItem(*item));
      }
      CServiceBroker::GetGUI()->GetInfoManager().SetCurrentItem(*m_itemCurrentFile);
      g_partyModeManager.OnSongChange(true);

#ifdef HAS_PYTHON
      // informs python script currently running playback has started
      // (does nothing if python is not loaded)
      g_pythonParser.OnPlayBackStarted(*m_itemCurrentFile);
#endif

      CVariant param;
      param["player"]["speed"] = 1;
      param["player"]["playerid"] = CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist();
      CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::Player, "xbmc", "OnPlay", m_itemCurrentFile, param);

      // we don't want a busy dialog when switching channels
      if (!m_itemCurrentFile->IsLiveTV())
      {
        CGUIDialogBusy* dialog = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogBusy>(WINDOW_DIALOG_BUSY);
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
      int iNext = CServiceBroker::GetPlaylistPlayer().GetNextSong();
      CPlayList& playlist = CServiceBroker::GetPlaylistPlayer().GetPlaylist(CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist());
      if (iNext < 0 || iNext >= playlist.size())
      {
        m_appPlayer.OnNothingToQueueNotify();
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

      if (!file.IsVideo() && m_appPlayer.IsPlayingVideo())
        bNothingToQueue = true;
      else if ((!file.IsAudio() || file.IsVideo()) && m_appPlayer.IsPlayingAudio())
        bNothingToQueue = true;

      if (bNothingToQueue)
      {
        m_appPlayer.OnNothingToQueueNotify();
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
      if (m_appPlayer.QueueNextFile(file))
      {
        // player accepted the next file
        m_nextPlaylistItem = iNext;
      }
      else
      {
        /* Player didn't accept next file: *ALWAYS* advance playlist in this case so the player can
            queue the next (if it wants to) and it doesn't keep looping on this song */
        CServiceBroker::GetPlaylistPlayer().SetCurrentSong(iNext);
      }

      return true;
    }
    break;

  case GUI_MSG_PLAYBACK_STOPPED:
    m_playerEvent.Set();
    m_itemCurrentFile->Reset();
    CServiceBroker::GetGUI()->GetInfoManager().ResetCurrentItem();
    PlaybackCleanup();
#ifdef HAS_PYTHON
    g_pythonParser.OnPlayBackStopped();
#endif
     return true;

  case GUI_MSG_PLAYBACK_ENDED:
    m_playerEvent.Set();
    if (m_stackHelper.IsPlayingRegularStack() && m_stackHelper.HasNextStackPartFileItem())
    { // just play the next item in the stack
      PlayFile(m_stackHelper.SetNextStackPartCurrentFileItem(), "", true);
      return true;
    }
    m_itemCurrentFile->Reset();
    CServiceBroker::GetGUI()->GetInfoManager().ResetCurrentItem();
    if (!CServiceBroker::GetPlaylistPlayer().PlayNext(1, true))
      m_appPlayer.ClosePlayer();

    PlaybackCleanup();

#ifdef HAS_PYTHON
      g_pythonParser.OnPlayBackEnded();
#endif
    return true;

  case GUI_MSG_PLAYLISTPLAYER_STOPPED:
    m_itemCurrentFile->Reset();
    CServiceBroker::GetGUI()->GetInfoManager().ResetCurrentItem();
    if (m_appPlayer.IsPlaying())
      StopPlaying();
    PlaybackCleanup();
    return true;

  case GUI_MSG_PLAYBACK_AVSTARTED:
    m_playerEvent.Set();
#ifdef HAS_PYTHON
    // informs python script currently running playback has started
    // (does nothing if python is not loaded)
    g_pythonParser.OnAVStarted(*m_itemCurrentFile);
#endif
    return true;

  case GUI_MSG_PLAYBACK_AVCHANGE:
#ifdef HAS_PYTHON
    // informs python script currently running playback has started
    // (does nothing if python is not loaded)
    g_pythonParser.OnAVChange();
#endif
      return true;

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
      SwitchToFullScreen();
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

bool CApplication::ExecuteXBMCAction(std::string actionStr, const CGUIListItemPtr &item /* = NULL */)
{
  // see if it is a user set string

  //We don't know if there is unsecure information in this yet, so we
  //postpone any logging
  const std::string in_actionStr(actionStr);
  if (item)
    actionStr = GUILIB::GUIINFO::CGUIInfoLabel::GetItemLabel(actionStr, item.get());
  else
    actionStr = GUILIB::GUIINFO::CGUIInfoLabel::GetLabel(actionStr);

  // user has asked for something to be executed
  if (CBuiltins::GetInstance().HasCommand(actionStr))
  {
    if (!CBuiltins::GetInstance().IsSystemPowerdownCommand(actionStr) ||
        CServiceBroker::GetPVRManager().GUIActions()->CanSystemPowerdown())
      CBuiltins::GetInstance().Execute(actionStr);
  }
  else
  {
    // try translating the action from our ButtonTranslator
    unsigned int actionID;
    if (CActionTranslator::TranslateString(actionStr, actionID))
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
    if (item.IsAudio() || item.IsVideo() || item.IsGame())
    { // an audio or video file
      PlayFile(item, "");
    }
    else
    {
      //At this point we have given up to translate, so even though
      //there may be insecure information, we log it.
      CLog::LogF(LOGDEBUG,"Tried translating, but failed to understand %s", in_actionStr.c_str());
      return false;
    }
  }
  return true;
}

// inform the user that the configuration data has moved from old XBMC location
// to new Kodi location - if applicable
void CApplication::ShowAppMigrationMessage()
{
  // .kodi_migration_complete will be created from the installer/packaging
  // once an old XBMC configuration was moved to the new Kodi location
  // if this is the case show the migration info to the user once which
  // tells him to have a look into the wiki where the move of configuration
  // is further explained.
  if (CFile::Exists("special://home/.kodi_data_was_migrated") &&
      !CFile::Exists("special://home/.kodi_migration_info_shown"))
  {
    HELPERS::ShowOKDialogText(CVariant{24128}, CVariant{24129});
    CFile tmpFile;
    // create the file which will prevent this dialog from appearing in the future
    tmpFile.OpenForWrite("special://home/.kodi_migration_info_shown");
    tmpFile.Close();
  }
}

void CApplication::Process()
{
  // dispatch the messages generated by python or other threads to the current window
  CServiceBroker::GetGUI()->GetWindowManager().DispatchThreadMessages();

  // process messages which have to be send to the gui
  // (this can only be done after CServiceBroker::GetGUI()->GetWindowManager().Render())
  CApplicationMessenger::GetInstance().ProcessWindowMessages();

  if (m_autoExecScriptExecuted)
  {
    m_autoExecScriptExecuted = false;

    // autoexec.py - profile
    std::string strAutoExecPy = CSpecialProtocol::TranslatePath("special://profile/autoexec.py");

    if (XFILE::CFile::Exists(strAutoExecPy))
      CScriptInvocationManager::GetInstance().ExecuteAsync(strAutoExecPy);
    else
      CLog::Log(LOGDEBUG, "no profile autoexec.py (%s) found, skipping", strAutoExecPy.c_str());
  }

  // handle any active scripts

  {
    // Allow processing of script threads to let them shut down properly.
    CSingleExit ex(CServiceBroker::GetWinSystem()->GetGfxContext());
    m_frameMoveGuard.unlock();
    CScriptInvocationManager::GetInstance().Process();
    m_frameMoveGuard.lock();
  }

  // process messages, even if a movie is playing
  CApplicationMessenger::GetInstance().ProcessMessages();
  if (m_bStop) return; //we're done, everything has been unloaded

  // update sound
  m_appPlayer.DoAudioWork();

  // do any processing that isn't needed on each run
  if( m_slowTimer.GetElapsedMilliseconds() > 500 )
  {
    m_slowTimer.Reset();
    ProcessSlow();
  }
#if !defined(TARGET_DARWIN)
  CServiceBroker::GetCPUInfo()->GetUsedPercentage(); // must call it to recalculate pct values
#endif
}

// We get called every 500ms
void CApplication::ProcessSlow()
{
  CServiceBroker::GetPowerManager().ProcessEvents();

#if defined(TARGET_DARWIN_OSX)
  // There is an issue on OS X that several system services ask the cursor to become visible
  // during their startup routines.  Given that we can't control this, we hack it in by
  // forcing the
  if (CServiceBroker::GetWinSystem()->IsFullScreen())
  { // SDL thinks it's hidden
    Cocoa_HideMouse();
  }
#endif

  // Temporarily pause pausable jobs when viewing video/picture
  int currentWindow = CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow();
  if (CurrentFileItem().IsVideo() ||
      CurrentFileItem().IsPicture() ||
      currentWindow == WINDOW_FULLSCREEN_VIDEO ||
      currentWindow == WINDOW_FULLSCREEN_GAME ||
      currentWindow == WINDOW_SLIDESHOW)
  {
    CJobManager::GetInstance().PauseJobs();
  }
  else
  {
    CJobManager::GetInstance().UnPauseJobs();
  }

  // Check if we need to activate the screensaver / DPMS.
  CheckScreenSaverAndDPMS();

  // Check if we need to shutdown (if enabled).
#if defined(TARGET_DARWIN)
  if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_POWERMANAGEMENT_SHUTDOWNTIME) &&
      CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_fullScreen)
#else
  if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_POWERMANAGEMENT_SHUTDOWNTIME))
#endif
  {
    CheckShutdown();
  }

#if defined(TARGET_POSIX)
  if (CPlatformPosix::TestQuitFlag())
  {
    CLog::Log(LOGNOTICE, "Quitting due to POSIX signal");
    CApplicationMessenger::GetInstance().PostMsg(TMSG_QUIT);
  }
#endif

  // check if we should restart the player
  CheckDelayedPlayerRestart();

  //  check if we can unload any unreferenced dlls or sections
  if (!m_appPlayer.IsPlayingVideo())
    CSectionLoader::UnloadDelayed();

#ifdef TARGET_ANDROID
  // Pass the slow loop to droid
  CXBMCApp::get()->ProcessSlow();
#endif

  // check for any idle curl connections
  g_curlInterface.CheckIdle();

  CServiceBroker::GetGUI()->GetLargeTextureManager().CleanupUnusedImages();

  CServiceBroker::GetGUI()->GetTextureManager().FreeUnusedTextures(5000);

#ifdef HAS_DVD_DRIVE
  // checks whats in the DVD drive and tries to autostart the content (xbox games, dvd, cdda, avi files...)
  if (!m_appPlayer.IsPlayingVideo())
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
  if(!m_renderGUI)
    ResetScreenSaverTimer();
}

// Global Idle Time in Seconds
// idle time will be reset if on any OnKey()
// int return: system Idle time in seconds! 0 is no idle!
int CApplication::GlobalIdleTime()
{
  if(!m_idleTimer.IsRunning())
    m_idleTimer.StartZero();
  return (int)m_idleTimer.GetElapsedSeconds();
}

float CApplication::NavigationIdleTime()
{
  if (!m_navigationTimer.IsRunning())
    m_navigationTimer.StartZero();
  return m_navigationTimer.GetElapsedSeconds();
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
  if (!m_appPlayer.IsPlayingVideo() && !m_appPlayer.IsPlayingAudio())
    return ;

  if (!m_appPlayer.HasPlayer())
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
  std::string state = m_appPlayer.GetPlayerState();

  // set the requested starttime
  m_itemCurrentFile->m_lStartOffset = CUtil::ConvertSecsToMilliSecs(time);

  // reopen the file
  if (PlayFile(*m_itemCurrentFile, "", true))
    m_appPlayer.SetPlayerState(state);
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

CFileItem& CApplication::CurrentUnstackedItem()
{
  if (m_stackHelper.IsPlayingISOStack() || m_stackHelper.IsPlayingRegularStack())
    return m_stackHelper.GetCurrentStackPartFileItem();
  else
    return *m_itemCurrentFile;
}

void CApplication::ShowVolumeBar(const CAction *action)
{
  CGUIDialogVolumeBar *volumeBar = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogVolumeBar>(WINDOW_DIALOG_VOLUME_BAR);
  if (volumeBar != nullptr && volumeBar->IsVolumeBarEnabled())
  {
    volumeBar->Open();
    if (action)
      volumeBar->OnAction(*action);
  }
}

bool CApplication::IsMuted() const
{
  if (CServiceBroker::GetPeripherals().IsMuted())
    return true;
  IAE* ae = CServiceBroker::GetActiveAE();
  if (ae)
    return ae->IsMuted();
  return true;
}

void CApplication::ToggleMute(void)
{
  if (m_muted)
    UnMute();
  else
    Mute();
}

void CApplication::SetMute(bool mute)
{
  if (m_muted != mute)
  {
    ToggleMute();
    m_muted = mute;
  }
}

void CApplication::Mute()
{
  if (CServiceBroker::GetPeripherals().Mute())
    return;

  IAE* ae = CServiceBroker::GetActiveAE();
  if (ae)
    ae->SetMute(true);
  m_muted = true;
  VolumeChanged();
}

void CApplication::UnMute()
{
  if (CServiceBroker::GetPeripherals().UnMute())
    return;

  IAE* ae = CServiceBroker::GetActiveAE();
  if (ae)
    ae->SetMute(false);
  m_muted = false;
  VolumeChanged();
}

void CApplication::SetVolume(float iValue, bool isPercentage/*=true*/)
{
  float hardwareVolume = iValue;

  if(isPercentage)
    hardwareVolume /= 100.0f;

  SetHardwareVolume(hardwareVolume);
  VolumeChanged();
}

void CApplication::SetHardwareVolume(float hardwareVolume)
{
  hardwareVolume = std::max(VOLUME_MINIMUM, std::min(VOLUME_MAXIMUM, hardwareVolume));
  m_volumeLevel = hardwareVolume;

  IAE* ae = CServiceBroker::GetActiveAE();
  if (ae)
    ae->SetVolume(hardwareVolume);
}

float CApplication::GetVolumePercent() const
{
  // converts the hardware volume to a percentage
  return m_volumeLevel * 100.0f;
}

float CApplication::GetVolumeRatio() const
{
  return m_volumeLevel;
}

void CApplication::VolumeChanged()
{
  CVariant data(CVariant::VariantTypeObject);
  data["volume"] = GetVolumePercent();
  data["muted"] = m_muted;
  CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::Application, "xbmc", "OnVolumeChanged", data);

  // if player has volume control, set it.
  m_appPlayer.SetVolume(m_volumeLevel);
  m_appPlayer.SetMute(m_muted);
}

int CApplication::GetSubtitleDelay()
{
  // converts subtitle delay to a percentage
  return int(((m_appPlayer.GetVideoSettings().m_SubtitleDelay + CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoSubsDelayRange)) / (2 * CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoSubsDelayRange)*100.0f + 0.5f);
}

int CApplication::GetAudioDelay()
{
  // converts audio delay to a percentage
  return int(((m_appPlayer.GetVideoSettings().m_AudioDelay + CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoAudioDelayRange)) / (2 * CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoAudioDelayRange)*100.0f + 0.5f);
}

// Returns the total time in seconds of the current media.  Fractional
// portions of a second are possible - but not necessarily supported by the
// player class.  This returns a double to be consistent with GetTime() and
// SeekTime().
double CApplication::GetTotalTime() const
{
  double rc = 0.0;

  if (m_appPlayer.IsPlaying())
  {
    if (m_stackHelper.IsPlayingRegularStack())
      rc = m_stackHelper.GetStackTotalTimeMs() * 0.001f;
    else
      rc = static_cast<double>(m_appPlayer.GetTotalTime() * 0.001f);
  }

  return rc;
}

void CApplication::StopShutdownTimer()
{
  m_shutdownTimer.Stop();
}

void CApplication::ResetShutdownTimers()
{
  // reset system shutdown timer
  m_shutdownTimer.StartZero();

  // delete custom shutdown timer
  if (g_alarmClock.HasAlarm("shutdowntimer"))
    g_alarmClock.Stop("shutdowntimer", true);
}

// Returns the current time in seconds of the currently playing media.
// Fractional portions of a second are possible.  This returns a double to
// be consistent with GetTotalTime() and SeekTime().
double CApplication::GetTime() const
{
  double rc = 0.0;

  if (m_appPlayer.IsPlaying())
  {
    if (m_stackHelper.IsPlayingRegularStack())
    {
      uint64_t startOfCurrentFile = m_stackHelper.GetCurrentStackPartStartTimeMs();
      rc = (startOfCurrentFile + m_appPlayer.GetTime()) * 0.001f;
    }
    else
      rc = static_cast<double>(m_appPlayer.GetTime() * 0.001f);
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
  if (m_appPlayer.IsPlaying() && (dTime >= 0.0))
  {
    if (!m_appPlayer.CanSeek())
      return;
    if (m_stackHelper.IsPlayingRegularStack())
    {
      // find the item in the stack we are seeking to, and load the new
      // file if necessary, and calculate the correct seek within the new
      // file.  Otherwise, just fall through to the usual routine if the
      // time is higher than our total time.
      int partNumberToPlay = m_stackHelper.GetStackPartNumberAtTimeMs(static_cast<uint64_t>(dTime * 1000.0));
      uint64_t startOfNewFile = m_stackHelper.GetStackPartStartTimeMs(partNumberToPlay);
      if (partNumberToPlay == m_stackHelper.GetCurrentPartNumber())
        m_appPlayer.SeekTime(static_cast<uint64_t>(dTime * 1000.0) - startOfNewFile);
      else
      { // seeking to a new file
        m_stackHelper.SetStackPartCurrentFileItem(partNumberToPlay);
        CFileItem *item = new CFileItem(m_stackHelper.GetCurrentStackPartFileItem());
        item->m_lStartOffset = static_cast<uint64_t>(dTime * 1000.0) - startOfNewFile;
        // don't just call "PlayFile" here, as we are quite likely called from the
        // player thread, so we won't be able to delete ourselves.
        CApplicationMessenger::GetInstance().PostMsg(TMSG_MEDIA_PLAY, 1, 0, static_cast<void*>(item));
      }
      return;
    }
    // convert to milliseconds and perform seek
    m_appPlayer.SeekTime( static_cast<int64_t>( dTime * 1000.0 ) );
  }
}

float CApplication::GetPercentage() const
{
  if (m_appPlayer.IsPlaying())
  {
    if (m_appPlayer.GetTotalTime() == 0 && m_appPlayer.IsPlayingAudio() && m_itemCurrentFile->HasMusicInfoTag())
    {
      const CMusicInfoTag& tag = *m_itemCurrentFile->GetMusicInfoTag();
      if (tag.GetDuration() > 0)
        return (float)(GetTime() / tag.GetDuration() * 100);
    }

    if (m_stackHelper.IsPlayingRegularStack())
    {
      double totalTime = GetTotalTime();
      if (totalTime > 0.0f)
        return (float)(GetTime() / totalTime * 100);
    }
    else
      return m_appPlayer.GetPercentage();
  }
  return 0.0f;
}

float CApplication::GetCachePercentage() const
{
  if (m_appPlayer.IsPlaying())
  {
    // Note that the player returns a relative cache percentage and we want an absolute percentage
    if (m_stackHelper.IsPlayingRegularStack())
    {
      float stackedTotalTime = (float) GetTotalTime();
      // We need to take into account the stack's total time vs. currently playing file's total time
      if (stackedTotalTime > 0.0f)
        return std::min( 100.0f, GetPercentage() + (m_appPlayer.GetCachePercentage() * m_appPlayer.GetTotalTime() * 0.001f / stackedTotalTime ) );
    }
    else
      return std::min( 100.0f, m_appPlayer.GetPercentage() + m_appPlayer.GetCachePercentage() );
  }
  return 0.0f;
}

void CApplication::SeekPercentage(float percent)
{
  if (m_appPlayer.IsPlaying() && (percent >= 0.0))
  {
    if (!m_appPlayer.CanSeek())
      return;
    if (m_stackHelper.IsPlayingRegularStack())
      SeekTime(percent * 0.01 * GetTotalTime());
    else
      m_appPlayer.SeekPercentage(percent);
  }
}

// SwitchToFullScreen() returns true if a switch is made, else returns false
bool CApplication::SwitchToFullScreen(bool force /* = false */)
{
  // don't switch if the slideshow is active
  if (CServiceBroker::GetGUI()->GetWindowManager().IsWindowActive(WINDOW_SLIDESHOW))
    return false;

  // if playing from the video info window, close it first!
  if (CServiceBroker::GetGUI()->GetWindowManager().IsModalDialogTopmost(WINDOW_DIALOG_VIDEO_INFO))
  {
    CGUIDialogVideoInfo* pDialog = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogVideoInfo>(WINDOW_DIALOG_VIDEO_INFO);
    if (pDialog) pDialog->Close(true);
  }

  int windowID = WINDOW_INVALID;

  // See if we're playing a game, and are in GUI mode
  if (m_appPlayer.IsPlayingGame() && CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() != WINDOW_FULLSCREEN_GAME)
    windowID = WINDOW_FULLSCREEN_GAME;

  // See if we're playing a video, and are in GUI mode
  else if (m_appPlayer.IsPlayingVideo() && CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() != WINDOW_FULLSCREEN_VIDEO)
    windowID = WINDOW_FULLSCREEN_VIDEO;

  // special case for switching between GUI & visualisation mode. (only if we're playing an audio song)
  if (m_appPlayer.IsPlayingAudio() && CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() != WINDOW_VISUALISATION)
    windowID = WINDOW_VISUALISATION;


  if (windowID != WINDOW_INVALID)
  {
    if (force)
      CServiceBroker::GetGUI()->GetWindowManager().ForceActivateWindow(windowID);
    else
      CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(windowID);
    return true;
  }

  return false;
}

void CApplication::Minimize()
{
  CServiceBroker::GetWinSystem()->Minimize();
}

std::string CApplication::GetCurrentPlayer()
{
  return m_appPlayer.GetCurrentPlayer();
}

CApplicationPlayer& CApplication::GetAppPlayer()
{
  return m_appPlayer;
}

CApplicationStackHelper& CApplication::GetAppStackHelper()
{
  return m_stackHelper;
}

void CApplication::UpdateLibraries()
{
  const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  if (settings->GetBool(CSettings::SETTING_VIDEOLIBRARY_UPDATEONSTARTUP))
  {
    CLog::LogF(LOGNOTICE, "Starting video library startup scan");
    StartVideoScan("", !settings->GetBool(CSettings::SETTING_VIDEOLIBRARY_BACKGROUNDUPDATE));
  }

  if (settings->GetBool(CSettings::SETTING_MUSICLIBRARY_UPDATEONSTARTUP))
  {
    CLog::LogF(LOGNOTICE, "Starting music library startup scan");
    StartMusicScan("", !settings->GetBool(CSettings::SETTING_MUSICLIBRARY_BACKGROUNDUPDATE));
  }
}

void CApplication::UpdateCurrentPlayArt()
{
  if (!m_appPlayer.IsPlayingAudio())
    return;
  //Clear and reload the art for the currenty playing item to show updated  art on OSD
  m_itemCurrentFile->ClearArt();
  CMusicThumbLoader loader;
  loader.LoadItem(m_itemCurrentFile.get());
  // Mirror changes to GUI item
  CServiceBroker::GetGUI()->GetInfoManager().SetCurrentItem(*m_itemCurrentFile);
}

bool CApplication::IsVideoScanning() const
{
  return CVideoLibraryQueue::GetInstance().IsScanningLibrary();
}

bool CApplication::IsMusicScanning() const
{
  return CMusicLibraryQueue::GetInstance().IsScanningLibrary();
}

void CApplication::StopVideoScan()
{
  CVideoLibraryQueue::GetInstance().StopLibraryScanning();
}

void CApplication::StopMusicScan()
{
  CMusicLibraryQueue::GetInstance().StopLibraryScanning();
}

void CApplication::StartVideoCleanup(bool userInitiated /* = true */,
                                     const std::string& content /* = "" */)
{
  if (userInitiated && CVideoLibraryQueue::GetInstance().IsRunning())
    return;

  std::set<int> paths;
  if (!content.empty())
  {
    CVideoDatabase db;
    std::set<std::string> contentPaths;
    if (db.Open() && db.GetPaths(contentPaths))
    {
      for (const std::string& path : contentPaths)
      {
        if (db.GetContentForPath(path) == content)
        {
          paths.insert(db.GetPathId(path));
          std::vector<std::pair<int, std::string>> sub;
          if (db.GetSubPaths(path, sub))
          {
            for (const auto& it : sub)
              paths.insert(it.first);
          }
        }
      }
    }
    if (paths.empty())
      return;
  }
  if (userInitiated)
    CVideoLibraryQueue::GetInstance().CleanLibraryModal(paths);
  else
    CVideoLibraryQueue::GetInstance().CleanLibrary(paths, true);
}

void CApplication::StartVideoScan(const std::string &strDirectory, bool userInitiated /* = true */, bool scanAll /* = false */)
{
  CVideoLibraryQueue::GetInstance().ScanLibrary(strDirectory, scanAll, userInitiated);
}

void CApplication::StartMusicCleanup(bool userInitiated /* = true */)
{
  if (userInitiated && CMusicLibraryQueue::GetInstance().IsRunning())
    return;

  if (userInitiated)
    /*
     CMusicLibraryQueue::GetInstance().CleanLibraryModal();
     As cleaning is non-granular and does not offer many opportunities to update progress
     dialog rendering, do asynchronously with model dialog
    */
    CMusicLibraryQueue::GetInstance().CleanLibrary(true);
  else
    CMusicLibraryQueue::GetInstance().CleanLibrary(false);
}

void CApplication::StartMusicScan(const std::string &strDirectory, bool userInitiated /* = true */, int flags /* = 0 */)
{
  if (IsMusicScanning())
    return;

  // Setup default flags
  if (!flags)
  { // Online scraping of additional info during scanning
    if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_MUSICLIBRARY_DOWNLOADINFO))
      flags |= CMusicInfoScanner::SCAN_ONLINE;
  }
  if (!userInitiated || CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_MUSICLIBRARY_BACKGROUNDUPDATE))
    flags |= CMusicInfoScanner::SCAN_BACKGROUND;

  CMusicLibraryQueue::GetInstance().ScanLibrary(strDirectory, flags, !(flags & CMusicInfoScanner::SCAN_BACKGROUND));
}

void CApplication::StartMusicAlbumScan(const std::string& strDirectory, bool refresh)
{
  if (IsMusicScanning())
    return;

  CMusicLibraryQueue::GetInstance().StartAlbumScan(strDirectory, refresh);
}

void CApplication::StartMusicArtistScan(const std::string& strDirectory,
                                        bool refresh)
{
  if (IsMusicScanning())
    return;

  CMusicLibraryQueue::GetInstance().StartArtistScan(strDirectory, refresh);
}

bool CApplication::ProcessAndStartPlaylist(const std::string& strPlayList, CPlayList& playlist, int iPlaylist, int track)
{
  CLog::Log(LOGDEBUG,"CApplication::ProcessAndStartPlaylist(%s, %i)",strPlayList.c_str(), iPlaylist);

  // initial exit conditions
  // no songs in playlist just return
  if (playlist.size() == 0)
    return false;

  // illegal playlist
  if (iPlaylist < PLAYLIST_MUSIC || iPlaylist > PLAYLIST_VIDEO)
    return false;

  // setup correct playlist
  CServiceBroker::GetPlaylistPlayer().ClearPlaylist(iPlaylist);

  // if the playlist contains an internet stream, this file will be used
  // to generate a thumbnail for musicplayer.cover
  m_strPlayListFile = strPlayList;

  // add the items to the playlist player
  CServiceBroker::GetPlaylistPlayer().Add(iPlaylist, playlist);

  // if we have a playlist
  if (CServiceBroker::GetPlaylistPlayer().GetPlaylist(iPlaylist).size())
  {
    // start playing it
    CServiceBroker::GetPlaylistPlayer().SetCurrentPlaylist(iPlaylist);
    CServiceBroker::GetPlaylistPlayer().Reset();
    CServiceBroker::GetPlaylistPlayer().Play(track, "");
    return true;
  }
  return false;
}

bool CApplication::IsCurrentThread() const
{
  return m_threadID == CThread::GetCurrentThreadId();
}

void CApplication::SetRenderGUI(bool renderGUI)
{
  if (renderGUI && ! m_renderGUI)
  {
    CGUIComponent *gui = CServiceBroker::GetGUI();
    if (gui)
      CServiceBroker::GetGUI()->GetWindowManager().MarkDirty();
  }
  m_renderGUI = renderGUI;
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
  // load the configured langauge
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
  m_saveSkinOnUnloading = !switchingProfiles;

  // make sure that the autoexec.py script is executed after logging in
  m_autoExecScriptExecuted = true;
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

void CApplication::RegisterActionListener(IActionListener *listener)
{
  CSingleLock lock(m_critSection);
  std::vector<IActionListener *>::iterator it = std::find(m_actionListeners.begin(), m_actionListeners.end(), listener);
  if (it == m_actionListeners.end())
    m_actionListeners.push_back(listener);
}

void CApplication::UnregisterActionListener(IActionListener *listener)
{
  CSingleLock lock(m_critSection);
  std::vector<IActionListener *>::iterator it = std::find(m_actionListeners.begin(), m_actionListeners.end(), listener);
  if (it != m_actionListeners.end())
    m_actionListeners.erase(it);
}

bool CApplication::NotifyActionListeners(const CAction &action) const
{
  CSingleLock lock(m_critSection);
  for (std::vector<IActionListener *>::const_iterator it = m_actionListeners.begin(); it != m_actionListeners.end(); ++it)
  {
    if ((*it)->OnAction(action))
      return true;
  }

  return false;
}
