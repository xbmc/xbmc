/*
 *      Copyright (C) 2005-2012 Team XBMC
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

#include "threads/SystemClock.h"
#include "system.h"
#include "Application.h"
#include "interfaces/Builtins.h"
#include "utils/Variant.h"
#include "utils/Splash.h"
#include "LangInfo.h"
#include "utils/Screenshot.h"
#include "Util.h"
#include "URL.h"
#include "guilib/TextureManager.h"
#include "cores/dvdplayer/DVDFileInfo.h"
#include "cores/AudioEngine/AEFactory.h"
#include "cores/AudioEngine/Utils/AEUtil.h"
#include "PlayListPlayer.h"
#include "Autorun.h"
#include "video/Bookmark.h"
#ifdef HAS_WEB_SERVER
#include "network/WebServer.h"
#include "network/httprequesthandler/HTTPImageHandler.h"
#include "network/httprequesthandler/HTTPVfsHandler.h"
#ifdef HAS_JSONRPC
#include "network/httprequesthandler/HTTPJsonRpcHandler.h"
#endif
#ifdef HAS_WEB_INTERFACE
#include "network/httprequesthandler/HTTPWebinterfaceHandler.h"
#include "network/httprequesthandler/HTTPWebinterfaceAddonsHandler.h"
#endif
#endif
#ifdef HAS_LCD
#include "utils/LCDFactory.h"
#endif
#include "guilib/GUIControlProfiler.h"
#include "utils/LangCodeExpander.h"
#include "GUIInfoManager.h"
#include "playlists/PlayListFactory.h"
#include "guilib/GUIFontManager.h"
#include "guilib/GUIColorManager.h"
#include "guilib/GUITextLayout.h"
#include "addons/Skin.h"
#ifdef HAS_PYTHON
#include "interfaces/python/XBPython.h"
#endif
#include "input/ButtonTranslator.h"
#include "guilib/GUIAudioManager.h"
#include "network/libscrobbler/lastfmscrobbler.h"
#include "network/libscrobbler/librefmscrobbler.h"
#include "GUIPassword.h"
#include "input/InertialScrollingHandler.h"
#include "ApplicationMessenger.h"
#include "SectionLoader.h"
#include "cores/DllLoader/DllLoaderContainer.h"
#include "GUIUserMessages.h"
#include "filesystem/DirectoryCache.h"
#include "filesystem/StackDirectory.h"
#include "filesystem/SpecialProtocol.h"
#include "filesystem/DllLibCurl.h"
#include "filesystem/MythSession.h"
#include "filesystem/PluginDirectory.h"
#ifdef HAS_FILESYSTEM_SAP
#include "filesystem/SAPDirectory.h"
#endif
#ifdef HAS_FILESYSTEM_HTSP
#include "filesystem/HTSPDirectory.h"
#endif
#include "utils/TuxBoxUtil.h"
#include "utils/SystemInfo.h"
#include "utils/TimeUtils.h"
#include "GUILargeTextureManager.h"
#include "TextureCache.h"
#include "music/LastFmManager.h"
#include "playlists/SmartPlayList.h"
#ifdef HAS_FILESYSTEM_RAR
#include "filesystem/RarManager.h"
#endif
#include "playlists/PlayList.h"
#include "windowing/WindowingFactory.h"
#include "powermanagement/PowerManager.h"
#include "powermanagement/DPMSSupport.h"
#include "settings/Settings.h"
#include "settings/AdvancedSettings.h"
#include "guilib/LocalizeStrings.h"
#include "utils/CPUInfo.h"
#include "utils/SeekHandler.h"

#include "input/KeyboardStat.h"
#include "input/XBMC_vkeys.h"
#include "input/MouseStat.h"

#ifdef HAS_SDL
#include <SDL/SDL.h>
#endif

#if defined(FILESYSTEM) && !defined(_LINUX)
#include "filesystem/FileDAAP.h"
#endif
#ifdef HAS_UPNP
#include "network/upnp/UPnP.h"
#include "filesystem/UPnPDirectory.h"
#endif
#if defined(_LINUX) && defined(HAS_FILESYSTEM_SMB)
#include "filesystem/SMBDirectory.h"
#endif
#ifdef HAS_FILESYSTEM_NFS
#include "filesystem/NFSFile.h"
#endif
#ifdef HAS_FILESYSTEM_AFP
#include "filesystem/AFPFile.h"
#endif
#ifdef HAS_FILESYSTEM_SFTP
#include "filesystem/SFTPFile.h"
#endif
#include "PartyModeManager.h"
#ifdef HAS_VIDEO_PLAYBACK
#include "cores/VideoRenderers/RenderManager.h"
#endif
#ifdef HAS_KARAOKE
#include "music/karaoke/karaokelyricsmanager.h"
#include "music/karaoke/GUIDialogKaraokeSongSelector.h"
#include "music/karaoke/GUIWindowKaraokeLyrics.h"
#endif
#include "network/Network.h"
#include "network/Zeroconf.h"
#include "network/ZeroconfBrowser.h"
#ifndef _LINUX
#include "threads/platform/win/Win32Exception.h"
#endif
#ifdef HAS_EVENT_SERVER
#include "network/EventServer.h"
#endif
#ifdef HAS_DBUS
#include <dbus/dbus.h>
#endif
#ifdef HAS_JSONRPC
#include "interfaces/json-rpc/JSONRPC.h"
#include "network/TCPServer.h"
#endif
#ifdef HAS_AIRPLAY
#include "network/AirPlayServer.h"
#endif
#ifdef HAS_AIRTUNES
#include "network/AirTunesServer.h"
#endif
#if defined(HAVE_LIBCRYSTALHD)
#include "cores/dvdplayer/DVDCodecs/Video/CrystalHD.h"
#endif
#include "interfaces/AnnouncementManager.h"
#include "peripherals/Peripherals.h"
#include "peripherals/dialogs/GUIDialogPeripheralManager.h"
#include "peripherals/dialogs/GUIDialogPeripheralSettings.h"
#include "peripherals/devices/PeripheralImon.h"
#include "music/infoscanner/MusicInfoScanner.h"

// Windows includes
#include "guilib/GUIWindowManager.h"
#include "windows/GUIWindowHome.h"
#include "settings/GUIWindowSettings.h"
#include "windows/GUIWindowFileManager.h"
#include "settings/GUIWindowSettingsCategory.h"
#include "music/windows/GUIWindowMusicPlaylist.h"
#include "music/windows/GUIWindowMusicSongs.h"
#include "music/windows/GUIWindowMusicNav.h"
#include "music/windows/GUIWindowMusicPlaylistEditor.h"
#include "video/windows/GUIWindowVideoPlaylist.h"
#include "music/dialogs/GUIDialogMusicInfo.h"
#include "video/dialogs/GUIDialogVideoInfo.h"
#include "video/windows/GUIWindowVideoNav.h"
#include "settings/GUIWindowSettingsProfile.h"
#ifdef HAS_GL
#include "rendering/gl/GUIWindowTestPatternGL.h"
#endif
#ifdef HAS_DX
#include "rendering/dx/GUIWindowTestPatternDX.h"
#endif
#include "settings/GUIWindowSettingsScreenCalibration.h"
#include "programs/GUIWindowPrograms.h"
#include "pictures/GUIWindowPictures.h"
#include "windows/GUIWindowWeather.h"
#include "windows/GUIWindowLoginScreen.h"
#include "addons/GUIWindowAddonBrowser.h"
#include "music/windows/GUIWindowVisualisation.h"
#include "windows/GUIWindowDebugInfo.h"
#include "windows/GUIWindowPointer.h"
#include "windows/GUIWindowSystemInfo.h"
#include "windows/GUIWindowScreensaver.h"
#include "windows/GUIWindowScreensaverDim.h"
#include "pictures/GUIWindowSlideShow.h"
#include "windows/GUIWindowStartup.h"
#include "video/windows/GUIWindowFullScreen.h"
#include "video/dialogs/GUIDialogVideoOSD.h"
#include "music/dialogs/GUIDialogMusicOverlay.h"
#include "video/dialogs/GUIDialogVideoOverlay.h"
#include "video/VideoInfoScanner.h"

// Dialog includes
#include "music/dialogs/GUIDialogMusicOSD.h"
#include "music/dialogs/GUIDialogVisualisationPresetList.h"
#include "dialogs/GUIDialogTextViewer.h"
#include "network/GUIDialogNetworkSetup.h"
#include "dialogs/GUIDialogMediaSource.h"
#include "video/dialogs/GUIDialogVideoSettings.h"
#include "video/dialogs/GUIDialogAudioSubtitleSettings.h"
#include "video/dialogs/GUIDialogVideoBookmarks.h"
#include "settings/GUIDialogProfileSettings.h"
#include "settings/GUIDialogLockSettings.h"
#include "settings/GUIDialogContentSettings.h"
#include "dialogs/GUIDialogBusy.h"
#include "dialogs/GUIDialogKeyboardGeneric.h"
#include "dialogs/GUIDialogYesNo.h"
#include "dialogs/GUIDialogOK.h"
#include "dialogs/GUIDialogProgress.h"
#include "dialogs/GUIDialogExtendedProgressBar.h"
#include "dialogs/GUIDialogSelect.h"
#include "dialogs/GUIDialogSeekBar.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "dialogs/GUIDialogVolumeBar.h"
#include "dialogs/GUIDialogMuteBug.h"
#include "video/dialogs/GUIDialogFileStacking.h"
#include "dialogs/GUIDialogNumeric.h"
#include "dialogs/GUIDialogGamepad.h"
#include "dialogs/GUIDialogSubMenu.h"
#include "dialogs/GUIDialogFavourites.h"
#include "dialogs/GUIDialogButtonMenu.h"
#include "dialogs/GUIDialogContextMenu.h"
#include "dialogs/GUIDialogPlayerControls.h"
#include "music/dialogs/GUIDialogSongInfo.h"
#include "dialogs/GUIDialogSmartPlaylistEditor.h"
#include "dialogs/GUIDialogSmartPlaylistRule.h"
#include "pictures/GUIDialogPictureInfo.h"
#include "addons/GUIDialogAddonSettings.h"
#include "addons/GUIDialogAddonInfo.h"
#ifdef HAS_LINUX_NETWORK
#include "network/GUIDialogAccessPoints.h"
#endif

/* PVR related include Files */
#include "pvr/PVRManager.h"
#include "pvr/timers/PVRTimers.h"
#include "pvr/windows/GUIWindowPVR.h"
#include "pvr/dialogs/GUIDialogPVRChannelManager.h"
#include "pvr/dialogs/GUIDialogPVRChannelsOSD.h"
#include "pvr/dialogs/GUIDialogPVRCutterOSD.h"
#include "pvr/dialogs/GUIDialogPVRDirectorOSD.h"
#include "pvr/dialogs/GUIDialogPVRGroupManager.h"
#include "pvr/dialogs/GUIDialogPVRGuideInfo.h"
#include "pvr/dialogs/GUIDialogPVRGuideOSD.h"
#include "pvr/dialogs/GUIDialogPVRGuideSearch.h"
#include "pvr/dialogs/GUIDialogPVRRecordingInfo.h"
#include "pvr/dialogs/GUIDialogPVRTimerSettings.h"

#include "epg/EpgContainer.h"

#include "video/dialogs/GUIDialogFullScreenInfo.h"
#include "video/dialogs/GUIDialogTeletext.h"
#include "dialogs/GUIDialogSlider.h"
#include "guilib/GUIControlFactory.h"
#include "dialogs/GUIDialogCache.h"
#include "dialogs/GUIDialogPlayEject.h"
#include "dialogs/GUIDialogMediaFilter.h"
#include "utils/XMLUtils.h"
#include "addons/AddonInstaller.h"

#ifdef HAS_PERFORMANCE_SAMPLE
#include "utils/PerformanceSample.h"
#else
#define MEASURE_FUNCTION
#endif

#ifdef TARGET_WINDOWS
#include <shlobj.h>
#include "win32util.h"
#endif
#ifdef HAS_XRANDR
#include "windowing/X11/XRandR.h"
#endif

#ifdef TARGET_DARWIN_OSX
#include "CocoaInterface.h"
#include "XBMCHelper.h"
#endif
#ifdef TARGET_DARWIN
#include "DarwinUtils.h"
#endif


#ifdef HAS_DVD_DRIVE
#include <cdio/logging.h>
#endif

#ifdef HAS_HAL
#include "linux/HALManager.h"
#endif

#include "storage/MediaManager.h"
#include "utils/JobManager.h"
#include "utils/SaveFileStateJob.h"
#include "utils/AlarmClock.h"
#include "utils/StringUtils.h"
#include "DatabaseManager.h"

#ifdef _LINUX
#include "XHandle.h"
#endif

#ifdef HAS_LIRC
#include "input/linux/LIRC.h"
#endif
#ifdef HAS_IRSERVERSUITE
  #include "input/windows/IRServerSuite.h"
#endif

#if defined(TARGET_WINDOWS)
#include "input/windows/WINJoystick.h"
#elif defined(HAS_SDL_JOYSTICK) || defined(HAS_EVENT_SERVER)
#include "input/SDLJoystick.h"
#endif

using namespace std;
using namespace ADDON;
using namespace XFILE;
#ifdef HAS_DVD_DRIVE
using namespace MEDIA_DETECT;
#endif
using namespace PLAYLIST;
using namespace VIDEO;
using namespace MUSIC_INFO;
#ifdef HAS_EVENT_SERVER
using namespace EVENTSERVER;
#endif
#ifdef HAS_JSONRPC
using namespace JSONRPC;
#endif
using namespace ANNOUNCEMENT;
using namespace PVR;
using namespace EPG;
using namespace PERIPHERALS;

using namespace XbmcThreads;

// uncomment this if you want to use release libs in the debug build.
// Atm this saves you 7 mb of memory
#define USE_RELEASE_LIBS

#define MAX_FFWD_SPEED 5

//extern IDirectSoundRenderer* m_pAudioDecoder;
CApplication::CApplication(void)
  : m_pPlayer(NULL)
#ifdef HAS_WEB_SERVER
  , m_WebServer(*new CWebServer)
  , m_httpImageHandler(*new CHTTPImageHandler)
  , m_httpVfsHandler(*new CHTTPVfsHandler)
#ifdef HAS_JSONRPC
  , m_httpJsonRpcHandler(*new CHTTPJsonRpcHandler)
#endif
#ifdef HAS_WEB_INTERFACE
  , m_httpWebinterfaceHandler(*new CHTTPWebinterfaceHandler)
  , m_httpWebinterfaceAddonsHandler(*new CHTTPWebinterfaceAddonsHandler)
#endif
#endif
  , m_itemCurrentFile(new CFileItem)
  , m_stackFileItemToUpdate(new CFileItem)
  , m_progressTrackingVideoResumeBookmark(*new CBookmark)
  , m_progressTrackingItem(new CFileItem)
  , m_videoInfoScanner(new CVideoInfoScanner)
  , m_musicInfoScanner(new CMusicInfoScanner)
  , m_seekHandler(new CSeekHandler)
{
  TiXmlBase::SetCondenseWhiteSpace(false);
  m_iPlaySpeed = 1;
  m_bInhibitIdleShutdown = false;
  m_bScreenSave = false;
  m_dpms = NULL;
  m_dpmsIsActive = false;
  m_dpmsIsManual = false;
  m_iScreenSaveLock = 0;
  m_bInitializing = true;
  m_eForcedNextPlayer = EPC_NONE;
  m_strPlayListFile = "";
  m_nextPlaylistItem = -1;
  m_bPlaybackStarting = false;
  m_skinReloading = false;

#ifdef HAS_GLX
  XInitThreads();
#endif

  //true while we in IsPaused mode! Workaround for OnPaused, which must be add. after v2.0
  m_bIsPaused = false;

  /* for now always keep this around */
#ifdef HAS_KARAOKE
  m_pKaraokeMgr = new CKaraokeLyricsManager();
#endif
  m_currentStack = new CFileItemList;

  m_frameCount = 0;

  m_bPresentFrame = false;
  m_bPlatformDirectories = true;

  m_bStandalone = false;
  m_bEnableLegacyRes = false;
  m_bSystemScreenSaverEnable = false;
  m_pInertialScrollingHandler = new CInertialScrollingHandler();
#ifdef HAS_DVD_DRIVE
  m_Autorun = new CAutorun();
#endif

  m_splash = NULL;
  m_threadID = 0;
  m_eCurrentPlayer = EPC_NONE;
  m_progressTrackingPlayCountUpdate = false;
  m_currentStackPosition = 0;
  m_lastFrameTime = 0;
  m_lastRenderTime = 0;
  m_bTestMode = false;
}

CApplication::~CApplication(void)
{
#ifdef HAS_WEB_SERVER
  delete &m_WebServer;
  delete &m_httpImageHandler;
  delete &m_httpVfsHandler;
#ifdef HAS_JSONRPC
  delete &m_httpJsonRpcHandler;
#endif
#ifdef HAS_WEB_INTERFACE
  delete &m_httpWebinterfaceHandler;
  delete &m_httpWebinterfaceAddonsHandler;
#endif
#endif
  delete m_musicInfoScanner;
  delete m_videoInfoScanner;
  delete &m_progressTrackingVideoResumeBookmark;
#ifdef HAS_DVD_DRIVE
  delete m_Autorun;
#endif
  delete m_currentStack;

#ifdef HAS_KARAOKE
  delete m_pKaraokeMgr;
#endif

  delete m_dpms;
  delete m_seekHandler;
  delete m_pInertialScrollingHandler;
}

bool CApplication::OnEvent(XBMC_Event& newEvent)
{
  switch(newEvent.type)
  {
    case XBMC_QUIT:
      if (!g_application.m_bStop)
        CApplicationMessenger::Get().Quit();
      break;
    case XBMC_KEYDOWN:
      g_application.OnKey(g_Keyboard.ProcessKeyDown(newEvent.key.keysym));
      break;
    case XBMC_KEYUP:
      g_Keyboard.ProcessKeyUp();
      break;
    case XBMC_MOUSEBUTTONDOWN:
    case XBMC_MOUSEBUTTONUP:
    case XBMC_MOUSEMOTION:
      g_Mouse.HandleEvent(newEvent);
      g_application.ProcessMouse();
      break;
    case XBMC_VIDEORESIZE:
      if (!g_application.m_bInitializing &&
          !g_advancedSettings.m_fullScreen)
      {
        g_Windowing.SetWindowResolution(newEvent.resize.w, newEvent.resize.h);
        g_graphicsContext.SetVideoResolution(RES_WINDOW, true);
        g_guiSettings.SetInt("window.width", newEvent.resize.w);
        g_guiSettings.SetInt("window.height", newEvent.resize.h);
        g_settings.Save();
      }
      break;
    case XBMC_VIDEOMOVE:
#ifdef TARGET_WINDOWS
      if (g_advancedSettings.m_fullScreen)
      {
        // when fullscreen, remain fullscreen and resize to the dimensions of the new screen
        RESOLUTION newRes = (RESOLUTION) g_Windowing.DesktopResolution(g_Windowing.GetCurrentScreen());
        if (newRes != g_graphicsContext.GetVideoResolution())
        {
          g_guiSettings.SetResolution(newRes);
          g_graphicsContext.SetVideoResolution(newRes);
        }
      }
      else
#endif
      {
        g_Windowing.OnMove(newEvent.move.x, newEvent.move.y);
      }
      break;
    case XBMC_USEREVENT:
      CApplicationMessenger::Get().UserEvent(newEvent.user.code);
      break;
    case XBMC_APPCOMMAND:
      return g_application.OnAppCommand(newEvent.appcommand.action);
  }
  return true;
}

extern "C" void __stdcall init_emu_environ();
extern "C" void __stdcall update_emu_environ();

//
// Utility function used to copy files from the application bundle
// over to the user data directory in Application Support/XBMC.
//
static void CopyUserDataIfNeeded(const CStdString &strPath, const CStdString &file)
{
  CStdString destPath = URIUtils::AddFileToFolder(strPath, file);
  if (!CFile::Exists(destPath))
  {
    // need to copy it across
    CStdString srcPath = URIUtils::AddFileToFolder("special://xbmc/userdata/", file);
    CFile::Cache(srcPath, destPath);
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
  CStdString install_path;

  CUtil::GetHomePath(install_path);
  setenv("XBMC_HOME", install_path.c_str(), 0);
  install_path += "/tools/darwin/runtime/preflight";
  system(install_path.c_str());
#endif
}

bool CApplication::Create()
{
  Preflight();
  g_settings.Initialize(); //Initialize default AdvancedSettings

#ifdef _LINUX
  tzset();   // Initialize timezone information variables
#endif

  // Grab a handle to our thread to be used later in identifying the render thread.
  m_threadID = CThread::GetCurrentThreadId();

#ifndef _LINUX
  //floating point precision to 24 bits (faster performance)
  _controlfp(_PC_24, _MCW_PC);

  /* install win32 exception translator, win32 exceptions
   * can now be caught using c++ try catch */
  win32_exception::install_handler();

#endif

  // only the InitDirectories* for the current platform should return true
  // putting this before the first log entries saves another ifdef for g_settings.m_logFolder
  bool inited = InitDirectoriesLinux();
  if (!inited)
    inited = InitDirectoriesOSX();
  if (!inited)
    inited = InitDirectoriesWin32();

  // copy required files
  CopyUserDataIfNeeded("special://masterprofile/", "RssFeeds.xml");
  CopyUserDataIfNeeded("special://masterprofile/", "favourites.xml");
  CopyUserDataIfNeeded("special://masterprofile/", "Lircmap.xml");
  CopyUserDataIfNeeded("special://masterprofile/", "LCD.xml");

  if (!CLog::Init(CSpecialProtocol::TranslatePath(g_settings.m_logFolder).c_str()))
  {
    fprintf(stderr,"Could not init logging classes. Permission errors on ~/.xbmc (%s)\n",
      CSpecialProtocol::TranslatePath(g_settings.m_logFolder).c_str());
    return false;
  }

  // Init our DllLoaders emu env
  init_emu_environ();

  g_settings.LoadProfiles(PROFILES_FILE);

  CLog::Log(LOGNOTICE, "-----------------------------------------------------------------------");
#if defined(TARGET_DARWIN_OSX)
  CLog::Log(LOGNOTICE, "Starting XBMC (%s), Platform: Darwin OSX (%s). Built on %s", g_infoManager.GetVersion().c_str(), g_sysinfo.GetUnameVersion().c_str(), __DATE__);
#elif defined(TARGET_DARWIN_IOS)
  CLog::Log(LOGNOTICE, "Starting XBMC (%s), Platform: Darwin iOS (%s). Built on %s", g_infoManager.GetVersion().c_str(), g_sysinfo.GetUnameVersion().c_str(), __DATE__);
#elif defined(__FreeBSD__)
  CLog::Log(LOGNOTICE, "Starting XBMC (%s), Platform: FreeBSD (%s). Built on %s", g_infoManager.GetVersion().c_str(), g_sysinfo.GetUnameVersion().c_str(), __DATE__);
#elif defined(_LINUX)
  CLog::Log(LOGNOTICE, "Starting XBMC (%s), Platform: Linux (%s, %s). Built on %s", g_infoManager.GetVersion().c_str(), g_sysinfo.GetLinuxDistro().c_str(), g_sysinfo.GetUnameVersion().c_str(), __DATE__);
#elif defined(_WIN32)
  CLog::Log(LOGNOTICE, "Starting XBMC (%s), Platform: %s. Built on %s (compiler %i)", g_infoManager.GetVersion().c_str(), g_sysinfo.GetKernelVersion().c_str(), __DATE__, _MSC_VER);
#if defined(__arm__)
  if (g_cpuInfo.GetCPUFeatures() & CPU_FEATURE_NEON)
    CLog::Log(LOGNOTICE, "ARM Features: Neon enabled");
  else
    CLog::Log(LOGNOTICE, "ARM Features: Neon disabled");
#endif
  CLog::Log(LOGNOTICE, g_cpuInfo.getCPUModel().c_str());
  CLog::Log(LOGNOTICE, CWIN32Util::GetResInfoString());
  CLog::Log(LOGNOTICE, "Running with %s rights", (CWIN32Util::IsCurrentUserLocalAdministrator() == TRUE) ? "administrator" : "restricted");
  CLog::Log(LOGNOTICE, "Aero is %s", (g_sysinfo.IsAeroDisabled() == true) ? "disabled" : "enabled");
#endif
  CSpecialProtocol::LogPaths();

  CStdString executable = CUtil::ResolveExecutablePath();
  CLog::Log(LOGNOTICE, "The executable running is: %s", executable.c_str());
  CLog::Log(LOGNOTICE, "Local hostname: %s", m_network.GetHostName().c_str());
  CLog::Log(LOGNOTICE, "Log File is located: %sxbmc.log", g_settings.m_logFolder.c_str());
  CLog::Log(LOGNOTICE, "-----------------------------------------------------------------------");

  CStdString strExecutablePath;
  CUtil::GetHomePath(strExecutablePath);

  // if we are running from DVD our UserData location will be TDATA
  if (URIUtils::IsDVD(strExecutablePath))
  {
    // TODO: Should we copy over any UserData folder from the DVD?
    if (!CFile::Exists("special://masterprofile/guisettings.xml")) // first run - cache userdata folder
    {
      CFileItemList items;
      CUtil::GetRecursiveListing("special://xbmc/userdata",items,"");
      for (int i=0;i<items.Size();++i)
          CFile::Cache(items[i]->GetPath(),"special://masterprofile/"+URIUtils::GetFileName(items[i]->GetPath()));
    }
    g_settings.m_logFolder = "special://masterprofile/";
  }

#ifdef HAS_XRANDR
  g_xrandr.LoadCustomModeLinesToAllOutputs();
#endif

  // for python scripts that check the OS
#if defined(TARGET_DARWIN)
  setenv("OS","OS X",true);
#elif defined(_LINUX)
  setenv("OS","Linux",true);
#elif defined(_WIN32)
  SetEnvironmentVariable("OS","win32");
#endif

  g_powerManager.Initialize();

  // Load the AudioEngine before settings as they need to query the engine
  if (!CAEFactory::LoadEngine())
  {
    CLog::Log(LOGFATAL, "CApplication::Create: Failed to load an AudioEngine");
    return false;
  }

  CLog::Log(LOGNOTICE, "load settings...");

  g_guiSettings.Initialize();  // Initialize default Settings - don't move
  g_powerManager.SetDefaults();
  if (!g_settings.Load())
  {
    CLog::Log(LOGFATAL, "%s: Failed to reset settings", __FUNCTION__);
    return false;
  }

  CLog::Log(LOGINFO, "creating subdirectories");
  CLog::Log(LOGINFO, "userdata folder: %s", g_settings.GetProfileUserDataFolder().c_str());
  CLog::Log(LOGINFO, "recording folder: %s", g_guiSettings.GetString("audiocds.recordingpath",false).c_str());
  CLog::Log(LOGINFO, "screenshots folder: %s", g_guiSettings.GetString("debug.screenshotpath",false).c_str());
  CDirectory::Create(g_settings.GetUserDataFolder());
  CDirectory::Create(g_settings.GetProfileUserDataFolder());
  g_settings.CreateProfileFolders();

  update_emu_environ();//apply the GUI settings

  // initialize our charset converter
  g_charsetConverter.reset();

  // Load the langinfo to have user charset <-> utf-8 conversion
  CStdString strLanguage = g_guiSettings.GetString("locale.language");
  strLanguage[0] = toupper(strLanguage[0]);

  CStdString strLangInfoPath;
  strLangInfoPath.Format("special://xbmc/language/%s/langinfo.xml", strLanguage.c_str());

  CLog::Log(LOGINFO, "load language info file: %s", strLangInfoPath.c_str());
  g_langInfo.Load(strLangInfoPath);

  CStdString strLanguagePath = "special://xbmc/language/";

  CLog::Log(LOGINFO, "load %s language file, from path: %s", strLanguage.c_str(), strLanguagePath.c_str());
  if (!g_localizeStrings.Load(strLanguagePath, strLanguage))
  {
    CLog::Log(LOGFATAL, "%s: Failed to load %s language file, from path: %s", __FUNCTION__, strLanguage.c_str(), strLanguagePath.c_str());
    return false;
  }

  // start the AudioEngine
  if (!CAEFactory::StartEngine())
  {
    CLog::Log(LOGFATAL, "CApplication::Create: Failed to start the AudioEngine");
    return false;
  }

  // restore AE's previous volume state
  SetHardwareVolume(g_settings.m_fVolumeLevel);
  CAEFactory::SetMute     (g_settings.m_bMute);
  CAEFactory::SetSoundMode(g_guiSettings.GetInt("audiooutput.guisoundmode"));

  // initialize the addon database (must be before the addon manager is init'd)
  CDatabaseManager::Get().Initialize(true);

  // start-up Addons Framework
  // currently bails out if either cpluff Dll is unavailable or system dir can not be scanned
  if (!CAddonMgr::Get().Init())
  {
    CLog::Log(LOGFATAL, "CApplication::Create: Unable to start CAddonMgr");
    return false;
  }

  g_peripherals.Initialise();

  // Create the Mouse, Keyboard, Remote, and Joystick devices
  // Initialize after loading settings to get joystick deadzone setting
  g_Mouse.Initialize();
  g_Mouse.SetEnabled(g_guiSettings.GetBool("input.enablemouse"));

  g_Keyboard.Initialize();
#if defined(HAS_LIRC) || defined(HAS_IRSERVERSUITE)
  g_RemoteControl.Initialize();
#endif

#if defined(TARGET_DARWIN_OSX)
  // Configure and possible manually start the helper.
  XBMCHelper::GetInstance().Configure();
#endif

  CUtil::InitRandomSeed();

  g_mediaManager.Initialize();

  m_lastFrameTime = XbmcThreads::SystemClockMillis();
  m_lastRenderTime = m_lastFrameTime;
  return true;
}

bool CApplication::CreateGUI()
{
#ifdef HAS_SDL
  CLog::Log(LOGNOTICE, "Setup SDL");

  /* Clean up on exit, exit on window close and interrupt */
  atexit(SDL_Quit);

  uint32_t sdlFlags = 0;

#if defined(HAS_SDL_OPENGL) || (HAS_GLES == 2)
  sdlFlags |= SDL_INIT_VIDEO;
#endif

#if defined(HAS_SDL_JOYSTICK) && !defined(TARGET_WINDOWS)
  sdlFlags |= SDL_INIT_JOYSTICK;
#endif

  //depending on how it's compiled, SDL periodically calls XResetScreenSaver when it's fullscreen
  //this might bring the monitor out of standby, so we have to disable it explicitly
  //by passing 0 for overwrite to setsenv, the user can still override this by setting the environment variable
#if defined(_LINUX) && !defined(TARGET_DARWIN)
  setenv("SDL_VIDEO_ALLOW_SCREENSAVER", "1", 0);
#endif

#endif // HAS_SDL

#ifdef _LINUX
  // for nvidia cards - vsync currently ALWAYS enabled.
  // the reason is that after screen has been setup changing this env var will make no difference.
  setenv("__GL_SYNC_TO_VBLANK", "1", 0);
  setenv("__GL_YIELD", "USLEEP", 0);
#endif

  m_bSystemScreenSaverEnable = g_Windowing.IsSystemScreenSaverEnabled();
  g_Windowing.EnableSystemScreenSaver(false);

#ifdef HAS_SDL
  if (SDL_Init(sdlFlags) != 0)
  {
    CLog::Log(LOGFATAL, "XBAppEx: Unable to initialize SDL: %s", SDL_GetError());
    return false;
  }
  #if defined(TARGET_DARWIN)
  // SDL_Init will install a handler for segfaults, restore the default handler.
  signal(SIGSEGV, SIG_DFL);
  #endif
#endif

  // Initialize core peripheral port support. Note: If these parameters
  // are 0 and NULL, respectively, then the default number and types of
  // controllers will be initialized.
  if (!g_Windowing.InitWindowSystem())
  {
    CLog::Log(LOGFATAL, "CApplication::Create: Unable to init windowing system");
    return false;
  }

  // Retrieve the matching resolution based on GUI settings
  g_guiSettings.m_LookAndFeelResolution = g_guiSettings.GetResolution();
  CLog::Log(LOGNOTICE, "Checking resolution %i", g_guiSettings.m_LookAndFeelResolution);
  if (!g_graphicsContext.IsValidResolution(g_guiSettings.m_LookAndFeelResolution))
  {
    CLog::Log(LOGNOTICE, "Setting safe mode %i", RES_DESKTOP);
    g_guiSettings.SetResolution(RES_DESKTOP);
  }

  // update the window resolution
  g_Windowing.SetWindowResolution(g_guiSettings.GetInt("window.width"), g_guiSettings.GetInt("window.height"));

  if (g_advancedSettings.m_startFullScreen && g_guiSettings.m_LookAndFeelResolution == RES_WINDOW)
    g_guiSettings.m_LookAndFeelResolution = RES_DESKTOP;

  if (!g_graphicsContext.IsValidResolution(g_guiSettings.m_LookAndFeelResolution))
  {
    // Oh uh - doesn't look good for starting in their wanted screenmode
    CLog::Log(LOGERROR, "The screen resolution requested is not valid, resetting to a valid mode");
    g_guiSettings.m_LookAndFeelResolution = RES_DESKTOP;
  }
  if (!InitWindow())
  {
    return false;
  }

  if (g_advancedSettings.m_splashImage)
  {
    CStdString strUserSplash = "special://home/media/Splash.png";
    if (CFile::Exists(strUserSplash))
    {
      CLog::Log(LOGINFO, "load user splash image: %s", CSpecialProtocol::TranslatePath(strUserSplash).c_str());
      m_splash = new CSplash(strUserSplash);
    }
    else
    {
      CLog::Log(LOGINFO, "load default splash image: %s", CSpecialProtocol::TranslatePath("special://xbmc/media/Splash.png").c_str());
      m_splash = new CSplash("special://xbmc/media/Splash.png");
    }
    m_splash->Show();
  }

  // The key mappings may already have been loaded by a peripheral
  CLog::Log(LOGINFO, "load keymapping");
  if (!CButtonTranslator::GetInstance().Load())
    return false;

  int iResolution = g_graphicsContext.GetVideoResolution();
  CLog::Log(LOGINFO, "GUI format %ix%i, Display %s",
            g_settings.m_ResInfo[iResolution].iWidth,
            g_settings.m_ResInfo[iResolution].iHeight,
            g_settings.m_ResInfo[iResolution].strMode.c_str());
  g_windowManager.Initialize();

  return true;
}

bool CApplication::InitWindow()
{
#ifdef TARGET_DARWIN_OSX
  // force initial window creation to be windowed, if fullscreen, it will switch to it below
  // fixes the white screen of death if starting fullscreen and switching to windowed.
  bool bFullScreen = false;
  if (!g_Windowing.CreateNewWindow("XBMC", bFullScreen, g_settings.m_ResInfo[RES_WINDOW], OnEvent))
  {
    CLog::Log(LOGFATAL, "CApplication::Create: Unable to create window");
    return false;
  }
#else
  bool bFullScreen = g_guiSettings.m_LookAndFeelResolution != RES_WINDOW;
  if (!g_Windowing.CreateNewWindow("XBMC", bFullScreen, g_settings.m_ResInfo[g_guiSettings.m_LookAndFeelResolution], OnEvent))
  {
    CLog::Log(LOGFATAL, "CApplication::Create: Unable to create window");
    return false;
  }
#endif

  if (!g_Windowing.InitRenderSystem())
  {
    CLog::Log(LOGFATAL, "CApplication::Create: Unable to init rendering system");
    return false;
  }
  // set GUI res and force the clear of the screen
  g_graphicsContext.SetVideoResolution(g_guiSettings.m_LookAndFeelResolution);
  g_fontManager.ReloadTTFFonts();
  return true;
}

bool CApplication::DestroyWindow()
{
  g_fontManager.UnloadTTFFonts();
  return g_Windowing.DestroyWindow();
}

bool CApplication::InitDirectoriesLinux()
{
/*
   The following is the directory mapping for Platform Specific Mode:

   special://xbmc/          => [read-only] system directory (/usr/share/xbmc)
   special://home/          => [read-write] user's directory that will override special://xbmc/ system-wide
                               installations like skins, screensavers, etc.
                               ($HOME/.xbmc)
                               NOTE: XBMC will look in both special://xbmc/addons and special://home/addons for addons.
   special://masterprofile/ => [read-write] userdata of master profile. It will by default be
                               mapped to special://home/userdata ($HOME/.xbmc/userdata)
   special://profile/       => [read-write] current profile's userdata directory.
                               Generally special://masterprofile for the master profile or
                               special://masterprofile/profiles/<profile_name> for other profiles.

   NOTE: All these root directories are lowercase. Some of the sub-directories
         might be mixed case.
*/

#if defined(_LINUX) && !defined(TARGET_DARWIN)
  CStdString userName;
  if (getenv("USER"))
    userName = getenv("USER");
  else
    userName = "root";

  CStdString userHome;
  if (getenv("HOME"))
    userHome = getenv("HOME");
  else
    userHome = "/root";

  CStdString xbmcBinPath, xbmcPath;
  CUtil::GetHomePath(xbmcBinPath, "XBMC_BIN_HOME");
  xbmcPath = getenv("XBMC_HOME");

  if (xbmcPath.IsEmpty())
  {
    xbmcPath = xbmcBinPath;
    /* Check if xbmc binaries and arch independent data files are being kept in
     * separate locations. */
    if (!CFile::Exists(URIUtils::AddFileToFolder(xbmcPath, "language")))
    {
      /* Attempt to locate arch independent data files. */
      CUtil::GetHomePath(xbmcPath);
      if (!CFile::Exists(URIUtils::AddFileToFolder(xbmcPath, "language")))
      {
        fprintf(stderr, "Unable to find path to XBMC data files!\n");
        exit(1);
      }
    }
  }

  /* Set some environment variables */
  setenv("XBMC_BIN_HOME", xbmcBinPath.c_str(), 0);
  setenv("XBMC_HOME", xbmcPath.c_str(), 0);

  if (m_bPlatformDirectories)
  {
    // map our special drives
    CSpecialProtocol::SetXBMCBinPath(xbmcBinPath);
    CSpecialProtocol::SetXBMCPath(xbmcPath);
    CSpecialProtocol::SetHomePath(userHome + "/.xbmc");
    CSpecialProtocol::SetMasterProfilePath(userHome + "/.xbmc/userdata");

    CStdString strTempPath = userHome;
    strTempPath = URIUtils::AddFileToFolder(strTempPath, ".xbmc/temp");
    if (getenv("XBMC_TEMP"))
      strTempPath = getenv("XBMC_TEMP");
    CSpecialProtocol::SetTempPath(strTempPath);

    URIUtils::AddSlashAtEnd(strTempPath);
    g_settings.m_logFolder = strTempPath;

    CreateUserDirs();

  }
  else
  {
    URIUtils::AddSlashAtEnd(xbmcPath);
    g_settings.m_logFolder = xbmcPath;

    CSpecialProtocol::SetXBMCBinPath(xbmcBinPath);
    CSpecialProtocol::SetXBMCPath(xbmcPath);
    CSpecialProtocol::SetHomePath(URIUtils::AddFileToFolder(xbmcPath, "portable_data"));
    CSpecialProtocol::SetMasterProfilePath(URIUtils::AddFileToFolder(xbmcPath, "portable_data/userdata"));

    CStdString strTempPath = xbmcPath;
    strTempPath = URIUtils::AddFileToFolder(strTempPath, "portable_data/temp");
    if (getenv("XBMC_TEMP"))
      strTempPath = getenv("XBMC_TEMP");
    CSpecialProtocol::SetTempPath(strTempPath);
    CreateUserDirs();

    URIUtils::AddSlashAtEnd(strTempPath);
    g_settings.m_logFolder = strTempPath;
  }

  return true;
#else
  return false;
#endif
}

bool CApplication::InitDirectoriesOSX()
{
#if defined(TARGET_DARWIN)
  CStdString userName;
  if (getenv("USER"))
    userName = getenv("USER");
  else
    userName = "root";

  CStdString userHome;
  if (getenv("HOME"))
    userHome = getenv("HOME");
  else
    userHome = "/root";

  CStdString xbmcPath;
  CUtil::GetHomePath(xbmcPath);
  setenv("XBMC_HOME", xbmcPath.c_str(), 0);

#if defined(TARGET_DARWIN_IOS)
  CStdString fontconfigPath;
  fontconfigPath = xbmcPath + "/system/players/dvdplayer/etc/fonts/fonts.conf";
  setenv("FONTCONFIG_FILE", fontconfigPath.c_str(), 0);
#endif

  // setup path to our internal dylibs so loader can find them
  CStdString frameworksPath = CUtil::GetFrameworksPath();
  CSpecialProtocol::SetXBMCFrameworksPath(frameworksPath);

  // OSX always runs with m_bPlatformDirectories == true
  if (m_bPlatformDirectories)
  {
    // map our special drives
    CSpecialProtocol::SetXBMCBinPath(xbmcPath);
    CSpecialProtocol::SetXBMCPath(xbmcPath);
    #if defined(TARGET_DARWIN_IOS)
      CSpecialProtocol::SetHomePath(userHome + "/Library/Preferences/XBMC");
      CSpecialProtocol::SetMasterProfilePath(userHome + "/Library/Preferences/XBMC/userdata");
    #else
      CSpecialProtocol::SetHomePath(userHome + "/Library/Application Support/XBMC");
      CSpecialProtocol::SetMasterProfilePath(userHome + "/Library/Application Support/XBMC/userdata");
    #endif

    // location for temp files
    #if defined(TARGET_DARWIN_IOS)
      CStdString strTempPath = URIUtils::AddFileToFolder(userHome,  "Library/Preferences/XBMC/temp");
    #else
      CStdString strTempPath = URIUtils::AddFileToFolder(userHome, ".xbmc/");
      CDirectory::Create(strTempPath);
      strTempPath = URIUtils::AddFileToFolder(userHome, ".xbmc/temp");
    #endif
    CSpecialProtocol::SetTempPath(strTempPath);

    // xbmc.log file location
    #if defined(TARGET_DARWIN_IOS)
      strTempPath = userHome + "/Library/Preferences";
    #else
      strTempPath = userHome + "/Library/Logs";
    #endif
    URIUtils::AddSlashAtEnd(strTempPath);
    g_settings.m_logFolder = strTempPath;

    CreateUserDirs();
  }
  else
  {
    URIUtils::AddSlashAtEnd(xbmcPath);
    g_settings.m_logFolder = xbmcPath;

    CSpecialProtocol::SetXBMCBinPath(xbmcPath);
    CSpecialProtocol::SetXBMCPath(xbmcPath);
    CSpecialProtocol::SetHomePath(URIUtils::AddFileToFolder(xbmcPath, "portable_data"));
    CSpecialProtocol::SetMasterProfilePath(URIUtils::AddFileToFolder(xbmcPath, "portable_data/userdata"));

    CStdString strTempPath = URIUtils::AddFileToFolder(xbmcPath, "portable_data/temp");
    CSpecialProtocol::SetTempPath(strTempPath);

    URIUtils::AddSlashAtEnd(strTempPath);
    g_settings.m_logFolder = strTempPath;
  }

  return true;
#else
  return false;
#endif
}

bool CApplication::InitDirectoriesWin32()
{
#ifdef _WIN32
  CStdString xbmcPath;

  CUtil::GetHomePath(xbmcPath);
  SetEnvironmentVariable("XBMC_HOME", xbmcPath.c_str());
  CSpecialProtocol::SetXBMCBinPath(xbmcPath);
  CSpecialProtocol::SetXBMCPath(xbmcPath);

  CStdString strWin32UserFolder = CWIN32Util::GetProfilePath();

  g_settings.m_logFolder = strWin32UserFolder;
  CSpecialProtocol::SetHomePath(strWin32UserFolder);
  CSpecialProtocol::SetMasterProfilePath(URIUtils::AddFileToFolder(strWin32UserFolder, "userdata"));
  CSpecialProtocol::SetTempPath(URIUtils::AddFileToFolder(strWin32UserFolder,"cache"));

  SetEnvironmentVariable("XBMC_PROFILE_USERDATA",CSpecialProtocol::TranslatePath("special://masterprofile/").c_str());

  CreateUserDirs();

  // Expand the DLL search path with our directories
  CWIN32Util::ExtendDllPath();

  return true;
#else
  return false;
#endif
}

void CApplication::CreateUserDirs()
{
  CDirectory::Create("special://home/");
  CDirectory::Create("special://home/addons");
  CDirectory::Create("special://home/addons/packages");
  CDirectory::Create("special://home/media");
  CDirectory::Create("special://home/sounds");
  CDirectory::Create("special://home/system");
  CDirectory::Create("special://masterprofile/");
  CDirectory::Create("special://temp/");
  CDirectory::Create("special://temp/temp"); // temp directory for python and dllGetTempPathA
}

bool CApplication::Initialize()
{
#if defined(HAS_DVD_DRIVE) && !defined(_WIN32) // somehow this throws an "unresolved external symbol" on win32
  // turn off cdio logging
  cdio_loglevel_default = CDIO_LOG_ERROR;
#endif

#ifdef _LINUX // TODO: Win32 has no special://home/ mapping by default, so we
              //       must create these here. Ideally this should be using special://home/ and
              //       be platform agnostic (i.e. unify the InitDirectories*() functions)
  if (!m_bPlatformDirectories)
#endif
  {
    CDirectory::Create("special://xbmc/language");
    CDirectory::Create("special://xbmc/addons");
    CDirectory::Create("special://xbmc/sounds");
  }

  // Load curl so curl_global_init gets called before any service threads
  // are started. Unloading will have no effect as curl is never fully unloaded.
  // To quote man curl_global_init:
  //  "This function is not thread safe. You must not call it when any other
  //  thread in the program (i.e. a thread sharing the same memory) is running.
  //  This doesn't just mean no other thread that is using libcurl. Because
  //  curl_global_init() calls functions of other libraries that are similarly
  //  thread unsafe, it could conflict with any other thread that
  //  uses these other libraries."
  g_curlInterface.Load();
  g_curlInterface.Unload();

  // initialize (and update as needed) our databases
  CDatabaseManager::Get().Initialize();

#ifdef HAS_WEB_SERVER
  CWebServer::RegisterRequestHandler(&m_httpImageHandler);
  CWebServer::RegisterRequestHandler(&m_httpVfsHandler);
#ifdef HAS_JSONRPC
  CWebServer::RegisterRequestHandler(&m_httpJsonRpcHandler);
#endif
#ifdef HAS_WEB_INTERFACE
  CWebServer::RegisterRequestHandler(&m_httpWebinterfaceAddonsHandler);
  CWebServer::RegisterRequestHandler(&m_httpWebinterfaceHandler);
#endif
#endif

  StartServices();

  // Init DPMS, before creating the corresponding setting control.
  m_dpms = new DPMSSupport();
  if (g_windowManager.Initialized())
  {
    g_guiSettings.GetSetting("powermanagement.displaysoff")->SetVisible(m_dpms->IsSupported());

    g_windowManager.Add(new CGUIWindowHome);
    g_windowManager.Add(new CGUIWindowPrograms);
    g_windowManager.Add(new CGUIWindowPictures);
    g_windowManager.Add(new CGUIWindowFileManager);
    g_windowManager.Add(new CGUIWindowSettings);
    g_windowManager.Add(new CGUIWindowSystemInfo);
#ifdef HAS_GL
    g_windowManager.Add(new CGUIWindowTestPatternGL);
#endif
#ifdef HAS_DX
    g_windowManager.Add(new CGUIWindowTestPatternDX);
#endif
    g_windowManager.Add(new CGUIWindowSettingsScreenCalibration);
    g_windowManager.Add(new CGUIWindowSettingsCategory);
    g_windowManager.Add(new CGUIWindowVideoNav);
    g_windowManager.Add(new CGUIWindowVideoPlaylist);
    g_windowManager.Add(new CGUIWindowLoginScreen);
    g_windowManager.Add(new CGUIWindowSettingsProfile);
    g_windowManager.Add(new CGUIWindow(WINDOW_SKIN_SETTINGS, "SkinSettings.xml"));
    g_windowManager.Add(new CGUIWindowAddonBrowser);
    g_windowManager.Add(new CGUIWindowScreensaverDim);
    g_windowManager.Add(new CGUIWindowDebugInfo);
    g_windowManager.Add(new CGUIWindowPointer);
    g_windowManager.Add(new CGUIDialogYesNo);
    g_windowManager.Add(new CGUIDialogProgress);
    g_windowManager.Add(new CGUIDialogExtendedProgressBar);
    g_windowManager.Add(new CGUIDialogKeyboardGeneric);
    g_windowManager.Add(new CGUIDialogVolumeBar);
    g_windowManager.Add(new CGUIDialogSeekBar);
    g_windowManager.Add(new CGUIDialogSubMenu);
    g_windowManager.Add(new CGUIDialogContextMenu);
    g_windowManager.Add(new CGUIDialogKaiToast);
    g_windowManager.Add(new CGUIDialogNumeric);
    g_windowManager.Add(new CGUIDialogGamepad);
    g_windowManager.Add(new CGUIDialogButtonMenu);
    g_windowManager.Add(new CGUIDialogMuteBug);
    g_windowManager.Add(new CGUIDialogPlayerControls);
#ifdef HAS_KARAOKE
    g_windowManager.Add(new CGUIDialogKaraokeSongSelectorSmall);
    g_windowManager.Add(new CGUIDialogKaraokeSongSelectorLarge);
#endif
    g_windowManager.Add(new CGUIDialogSlider);
    g_windowManager.Add(new CGUIDialogMusicOSD);
    g_windowManager.Add(new CGUIDialogVisualisationPresetList);
    g_windowManager.Add(new CGUIDialogVideoSettings);
    g_windowManager.Add(new CGUIDialogAudioSubtitleSettings);
    g_windowManager.Add(new CGUIDialogVideoBookmarks);
    // Don't add the filebrowser dialog - it's created and added when it's needed
    g_windowManager.Add(new CGUIDialogNetworkSetup);
    g_windowManager.Add(new CGUIDialogMediaSource);
    g_windowManager.Add(new CGUIDialogProfileSettings);
    g_windowManager.Add(new CGUIDialogFavourites);
    g_windowManager.Add(new CGUIDialogSongInfo);
    g_windowManager.Add(new CGUIDialogSmartPlaylistEditor);
    g_windowManager.Add(new CGUIDialogSmartPlaylistRule);
    g_windowManager.Add(new CGUIDialogBusy);
    g_windowManager.Add(new CGUIDialogPictureInfo);
    g_windowManager.Add(new CGUIDialogAddonInfo);
    g_windowManager.Add(new CGUIDialogAddonSettings);
#ifdef HAS_LINUX_NETWORK
    g_windowManager.Add(new CGUIDialogAccessPoints);
#endif

    g_windowManager.Add(new CGUIDialogLockSettings);

    g_windowManager.Add(new CGUIDialogContentSettings);

    g_windowManager.Add(new CGUIDialogPlayEject);

    g_windowManager.Add(new CGUIDialogPeripheralManager);
    g_windowManager.Add(new CGUIDialogPeripheralSettings);
    
    g_windowManager.Add(new CGUIDialogMediaFilter);

    g_windowManager.Add(new CGUIWindowMusicPlayList);
    g_windowManager.Add(new CGUIWindowMusicSongs);
    g_windowManager.Add(new CGUIWindowMusicNav);
    g_windowManager.Add(new CGUIWindowMusicPlaylistEditor);

    /* Load PVR related Windows and Dialogs */
    g_windowManager.Add(new CGUIDialogTeletext);
    g_windowManager.Add(new CGUIWindowPVR);
    g_windowManager.Add(new CGUIDialogPVRGuideInfo);
    g_windowManager.Add(new CGUIDialogPVRRecordingInfo);
    g_windowManager.Add(new CGUIDialogPVRTimerSettings);
    g_windowManager.Add(new CGUIDialogPVRGroupManager);
    g_windowManager.Add(new CGUIDialogPVRChannelManager);
    g_windowManager.Add(new CGUIDialogPVRGuideSearch);
    g_windowManager.Add(new CGUIDialogPVRChannelsOSD);
    g_windowManager.Add(new CGUIDialogPVRGuideOSD);
    g_windowManager.Add(new CGUIDialogPVRDirectorOSD);
    g_windowManager.Add(new CGUIDialogPVRCutterOSD);

    g_windowManager.Add(new CGUIDialogSelect);
    g_windowManager.Add(new CGUIDialogMusicInfo);
    g_windowManager.Add(new CGUIDialogOK);
    g_windowManager.Add(new CGUIDialogVideoInfo);
    g_windowManager.Add(new CGUIDialogTextViewer);
    g_windowManager.Add(new CGUIWindowFullScreen);
    g_windowManager.Add(new CGUIWindowVisualisation);
    g_windowManager.Add(new CGUIWindowSlideShow);
    g_windowManager.Add(new CGUIDialogFileStacking);
#ifdef HAS_KARAOKE
    g_windowManager.Add(new CGUIWindowKaraokeLyrics);
#endif

    g_windowManager.Add(new CGUIDialogVideoOSD);
    g_windowManager.Add(new CGUIDialogMusicOverlay);
    g_windowManager.Add(new CGUIDialogVideoOverlay);
    g_windowManager.Add(new CGUIWindowScreensaver);
    g_windowManager.Add(new CGUIWindowWeather);
    g_windowManager.Add(new CGUIWindowStartup);

    /* window id's 3000 - 3100 are reserved for python */

    // Make sure we have at least the default skin
    if (!LoadSkin(g_guiSettings.GetString("lookandfeel.skin")) && !LoadSkin(DEFAULT_SKIN))
    {
        CLog::Log(LOGERROR, "Default skin '%s' not found! Terminating..", DEFAULT_SKIN);
        return false;
    }

    StartPVRManager();

    if (g_advancedSettings.m_splashImage)
      SAFE_DELETE(m_splash);

    if (g_guiSettings.GetBool("masterlock.startuplock") &&
        g_settings.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE &&
       !g_settings.GetMasterProfile().getLockCode().IsEmpty())
    {
       g_passwordManager.CheckStartUpLock();
    }

    // check if we should use the login screen
    if (g_settings.UsingLoginScreen())
      g_windowManager.ActivateWindow(WINDOW_LOGIN_SCREEN);
    else
    {
#ifdef HAS_JSONRPC
      CJSONRPC::Initialize();
#endif
      ADDON::CAddonMgr::Get().StartServices(false);
      g_windowManager.ActivateWindow(g_SkinInfo->GetFirstWindow());
    }

  }
  else //No GUI Created
  {
#ifdef HAS_JSONRPC
    CJSONRPC::Initialize();
#endif
    ADDON::CAddonMgr::Get().StartServices(false);
  }

  g_sysinfo.Refresh();

  CLog::Log(LOGINFO, "removing tempfiles");
  CUtil::RemoveTempFiles();

  if (!g_settings.UsingLoginScreen())
  {
    UpdateLibraries();
#ifdef HAS_PYTHON
    g_pythonParser.m_bLogin = true;
#endif
  }

  m_slowTimer.StartZero();

#if defined(HAVE_LIBCRYSTALHD)
  CCrystalHD::GetInstance();
#endif

  CAddonMgr::Get().StartServices(true);

  CLog::Log(LOGNOTICE, "initialize done");

  m_bInitializing = false;

  // reset our screensaver (starts timers etc.)
  ResetScreenSaver();

#ifdef HAS_SDL_JOYSTICK
  g_Joystick.SetEnabled(g_guiSettings.GetBool("input.enablejoystick") &&
                    (CPeripheralImon::GetCountOfImonsConflictWithDInput() == 0 || !g_guiSettings.GetBool("input.disablejoystickwithimon")) );
#endif

  return true;
}

bool CApplication::StartServer(enum ESERVERS eServer, bool bStart, bool bWait/* = false*/)
{
  bool ret = true;
  bool oldSetting = false;

  switch(eServer)
  {
    case ES_WEBSERVER:
      oldSetting = g_guiSettings.GetBool("services.webserver");
      g_guiSettings.SetBool("services.webserver", bStart);

      if (bStart)
        ret = StartWebServer();
      else
        StopWebServer();

      if (!ret)
      {
        g_guiSettings.SetBool("services.webserver", oldSetting);
      }
      break;
    case ES_AIRPLAYSERVER:
      oldSetting = g_guiSettings.GetBool("services.esenabled");
      g_guiSettings.SetBool("services.airplay", bStart);

      if (bStart)
        ret = StartAirplayServer();
      else
        StopAirplayServer(bWait);

      if (!ret)
      {
        g_guiSettings.SetBool("services.esenabled", oldSetting);
      }
      break;
    case ES_JSONRPCSERVER:
      oldSetting = g_guiSettings.GetBool("services.esenabled");
      g_guiSettings.SetBool("services.esenabled", bStart);

      if (bStart)
        ret = StartJSONRPCServer();
      else
        StopJSONRPCServer(bWait);

      if (!ret)
      {
        g_guiSettings.SetBool("services.esenabled", oldSetting);
      }
      break;
    case ES_UPNPSERVER:
      g_guiSettings.SetBool("services.upnpserver", bStart);
      if (bStart)
        StartUPnPServer();
      else
        StopUPnPServer();
      break;
    case ES_UPNPRENDERER:
      g_guiSettings.SetBool("services.upnprenderer", bStart);
      if (bStart)
        StartUPnPRenderer();
      else
        StopUPnPRenderer();
      break;
    case ES_EVENTSERVER:
      oldSetting = g_guiSettings.GetBool("services.esenabled");
      g_guiSettings.SetBool("services.esenabled", bStart);

      if (bStart)
        ret = StartEventServer();
      else
        StopEventServer(bWait, false);

      if (!ret)
      {
        g_guiSettings.SetBool("services.esenabled", oldSetting);
      }

      break;
    case ES_ZEROCONF:
      g_guiSettings.SetBool("services.zeroconf", bStart);
      if (bStart)
        StartZeroconf();
      else
        StopZeroconf();
      break;
    default:
      ret = false;
      break;
  }
  g_settings.Save();

  return ret;
}

bool CApplication::StartWebServer()
{
#ifdef HAS_WEB_SERVER
  if (g_guiSettings.GetBool("services.webserver") && m_network.IsAvailable())
  {
    int webPort = atoi(g_guiSettings.GetString("services.webserverport"));
    CLog::Log(LOGNOTICE, "Webserver: Starting...");
#ifdef _LINUX
    if (webPort < 1024 && !CUtil::CanBindPrivileged())
    {
        CLog::Log(LOGERROR, "Cannot start Web Server on port %i, no permission to bind to ports below 1024", webPort);
        return false;
    }
#endif

    bool started = false;
    if (m_WebServer.Start(webPort, g_guiSettings.GetString("services.webserverusername"), g_guiSettings.GetString("services.webserverpassword")))
    {
      std::map<std::string, std::string> txt;
      started = true;
      // publish web frontend and API services
#ifdef HAS_WEB_INTERFACE
      CZeroconf::GetInstance()->PublishService("servers.webserver", "_http._tcp", g_infoManager.GetLabel(SYSTEM_FRIENDLY_NAME), webPort, txt);
#endif
#ifdef HAS_JSONRPC
      CZeroconf::GetInstance()->PublishService("servers.jsonrpc-http", "_xbmc-jsonrpc-h._tcp", g_infoManager.GetLabel(SYSTEM_FRIENDLY_NAME), webPort, txt);
#endif
    }

    return started;
  }
#endif

  return true;
}

void CApplication::StopWebServer()
{
#ifdef HAS_WEB_SERVER
  if (m_WebServer.IsStarted())
  {
    CLog::Log(LOGNOTICE, "Webserver: Stopping...");
    m_WebServer.Stop();
    if(! m_WebServer.IsStarted() )
    {
      CLog::Log(LOGNOTICE, "Webserver: Stopped...");
      CZeroconf::GetInstance()->RemoveService("servers.webserver");
      CZeroconf::GetInstance()->RemoveService("servers.jsonrpc-http");
      CZeroconf::GetInstance()->RemoveService("servers.webapi");
    } else
      CLog::Log(LOGWARNING, "Webserver: Failed to stop.");
  }
#endif
}

bool CApplication::StartAirplayServer()
{
  bool ret = false;
#ifdef HAS_AIRPLAY
  if (g_guiSettings.GetBool("services.airplay") && m_network.IsAvailable())
  {
    int listenPort = g_advancedSettings.m_airPlayPort;
    CStdString password = g_guiSettings.GetString("services.airplaypassword");
    bool usePassword = g_guiSettings.GetBool("services.useairplaypassword");

    if (CAirPlayServer::StartServer(listenPort, true))
    {
      CAirPlayServer::SetCredentials(usePassword, password);
      std::map<std::string, std::string> txt;
      CNetworkInterface* iface = g_application.getNetwork().GetFirstConnectedInterface();
      if (iface)
      {
        txt["deviceid"] = iface->GetMacAddress();
      }
      else
      {
        txt["deviceid"] = "FF:FF:FF:FF:FF:F2";
      }
      txt["features"] = "0x77";
      txt["model"] = "AppleTV2,1";
      txt["srcvers"] = AIRPLAY_SERVER_VERSION_STR;
      CZeroconf::GetInstance()->PublishService("servers.airplay", "_airplay._tcp", g_infoManager.GetLabel(SYSTEM_FRIENDLY_NAME), listenPort, txt);
      ret = true;
    }
  }
  if (ret)
#endif
  {
#ifdef HAS_AIRTUNES
    if (g_guiSettings.GetBool("services.airplay") && m_network.IsAvailable())
    {
      int listenPort = g_advancedSettings.m_airTunesPort;
      CStdString password = g_guiSettings.GetString("services.airplaypassword");
      bool usePassword = g_guiSettings.GetBool("services.useairplaypassword");

      if (!CAirTunesServer::StartServer(listenPort, true, usePassword, password))
      {
        CLog::Log(LOGERROR, "Failed to start AirTunes Server");
      }
      ret = true;
    }
#endif
  }
  return ret;
}

void CApplication::StopAirplayServer(bool bWait)
{
#ifdef HAS_AIRPLAY
  CAirPlayServer::StopServer(bWait);
  CZeroconf::GetInstance()->RemoveService("servers.airplay");
#endif
#ifdef HAS_AIRTUNES
  CAirTunesServer::StopServer(bWait);
#endif
}

bool CApplication::StartJSONRPCServer()
{
#ifdef HAS_JSONRPC
  if (g_guiSettings.GetBool("services.esenabled"))
  {
    if (CTCPServer::StartServer(g_advancedSettings.m_jsonTcpPort, g_guiSettings.GetBool("services.esallinterfaces")))
    {
      std::map<std::string, std::string> txt;
      CZeroconf::GetInstance()->PublishService("servers.jsonrpc-tpc", "_xbmc-jsonrpc._tcp", g_infoManager.GetLabel(SYSTEM_FRIENDLY_NAME), g_advancedSettings.m_jsonTcpPort, txt);
      return true;
    }
    else
      return false;
  }
#endif

  return true;
}

void CApplication::StopJSONRPCServer(bool bWait)
{
#ifdef HAS_JSONRPC
  CTCPServer::StopServer(bWait);
  CZeroconf::GetInstance()->RemoveService("servers.jsonrpc-tcp");
#endif
}

void CApplication::StartUPnP()
{
#ifdef HAS_UPNP
  StartUPnPServer();
  StartUPnPRenderer();
#endif
}

void CApplication::StopUPnP(bool bWait)
{
#ifdef HAS_UPNP
  if (UPNP::CUPnP::IsInstantiated())
  {
    CLog::Log(LOGNOTICE, "stopping upnp");
    UPNP::CUPnP::ReleaseInstance(bWait);
  }
#endif
}

bool CApplication::StartEventServer()
{
#ifdef HAS_EVENT_SERVER
  CEventServer* server = CEventServer::GetInstance();
  if (!server)
  {
    CLog::Log(LOGERROR, "ES: Out of memory");
    return false;
  }
  if (g_guiSettings.GetBool("services.esenabled"))
  {
    CLog::Log(LOGNOTICE, "ES: Starting event server");
    server->StartServer();
    return true;
  }
#endif
  return true;
}

bool CApplication::StopEventServer(bool bWait, bool promptuser)
{
#ifdef HAS_EVENT_SERVER
  CEventServer* server = CEventServer::GetInstance();
  if (!server)
  {
    CLog::Log(LOGERROR, "ES: Out of memory");
    return false;
  }
  if (promptuser)
  {
    if (server->GetNumberOfClients() > 0)
    {
      bool cancelled = false;
      if (!CGUIDialogYesNo::ShowAndGetInput(13140, 13141, 13142, 20022,
                                            -1, -1, cancelled, 10000)
          || cancelled)
      {
        CLog::Log(LOGNOTICE, "ES: Not stopping event server");
        return false;
      }
    }
    CLog::Log(LOGNOTICE, "ES: Stopping event server with confirmation");

    CEventServer::GetInstance()->StopServer(true);
  }
  else
  {
    if (!bWait)
      CLog::Log(LOGNOTICE, "ES: Stopping event server");

    CEventServer::GetInstance()->StopServer(bWait);
  }

  return true;
#endif
}

void CApplication::RefreshEventServer()
{
#ifdef HAS_EVENT_SERVER
  if (g_guiSettings.GetBool("services.esenabled"))
  {
    CEventServer::GetInstance()->RefreshSettings();
  }
#endif
}

void CApplication::StartUPnPRenderer()
{
#ifdef HAS_UPNP
  if (g_guiSettings.GetBool("services.upnprenderer"))
  {
    CLog::Log(LOGNOTICE, "starting upnp renderer");
    UPNP::CUPnP::GetInstance()->StartRenderer();
  }
#endif
}

void CApplication::StopUPnPRenderer()
{
#ifdef HAS_UPNP
  if (UPNP::CUPnP::IsInstantiated())
  {
    CLog::Log(LOGNOTICE, "stopping upnp renderer");
    UPNP::CUPnP::GetInstance()->StopRenderer();
  }
#endif
}

void CApplication::StartUPnPServer()
{
#ifdef HAS_UPNP
  if (g_guiSettings.GetBool("services.upnpserver"))
  {
    CLog::Log(LOGNOTICE, "starting upnp server");
    UPNP::CUPnP::GetInstance()->StartServer();
  }
#endif
}

void CApplication::StopUPnPServer()
{
#ifdef HAS_UPNP
  if (UPNP::CUPnP::IsInstantiated())
  {
    CLog::Log(LOGNOTICE, "stopping upnp server");
    UPNP::CUPnP::GetInstance()->StopServer();
  }
#endif
}

void CApplication::StartZeroconf()
{
#ifdef HAS_ZEROCONF
  //entry in guisetting only present if HAS_ZEROCONF is set
  if(g_guiSettings.GetBool("services.zeroconf"))
  {
    CLog::Log(LOGNOTICE, "starting zeroconf publishing");
    CZeroconf::GetInstance()->Start();
  }
#endif
}

void CApplication::StopZeroconf()
{
#ifdef HAS_ZEROCONF
  if(CZeroconf::IsInstantiated())
  {
    CLog::Log(LOGNOTICE, "stopping zeroconf publishing");
    CZeroconf::GetInstance()->Stop();
  }
#endif
}

void CApplication::StartPVRManager()
{
  if (g_guiSettings.GetBool("pvrmanager.enabled"))
    g_PVRManager.Start(true);
}

void CApplication::StopPVRManager()
{
  CLog::Log(LOGINFO, "stopping PVRManager");
  if (g_PVRManager.IsPlaying())
    StopPlaying();
  g_PVRManager.Stop();
  g_EpgContainer.Stop();
}

void CApplication::DimLCDOnPlayback(bool dim)
{
#ifdef HAS_LCD
  if (g_lcd)
  {
    if (dim)
      g_lcd->DisableOnPlayback(IsPlayingVideo(), IsPlayingAudio());
    else
      g_lcd->SetBackLight(1);
  }
#endif
}

void CApplication::StartServices()
{
#if !defined(_WIN32) && defined(HAS_DVD_DRIVE)
  // Start Thread for DVD Mediatype detection
  CLog::Log(LOGNOTICE, "start dvd mediatype detection");
  m_DetectDVDType.Create(false, THREAD_MINSTACKSIZE);
#endif

  CLog::Log(LOGNOTICE, "initializing playlistplayer");
  g_playlistPlayer.SetRepeat(PLAYLIST_MUSIC, g_settings.m_bMyMusicPlaylistRepeat ? PLAYLIST::REPEAT_ALL : PLAYLIST::REPEAT_NONE);
  g_playlistPlayer.SetShuffle(PLAYLIST_MUSIC, g_settings.m_bMyMusicPlaylistShuffle);
  g_playlistPlayer.SetRepeat(PLAYLIST_VIDEO, g_settings.m_bMyVideoPlaylistRepeat ? PLAYLIST::REPEAT_ALL : PLAYLIST::REPEAT_NONE);
  g_playlistPlayer.SetShuffle(PLAYLIST_VIDEO, g_settings.m_bMyVideoPlaylistShuffle);
  CLog::Log(LOGNOTICE, "DONE initializing playlistplayer");

#ifdef HAS_LCD
  CLCDFactory factory;
  g_lcd = factory.Create();
  if (g_lcd)
  {
    g_lcd->Initialize();
  }
#endif
}

void CApplication::StopServices()
{
  m_network.NetworkMessage(CNetwork::SERVICES_DOWN, 0);

#if !defined(_WIN32) && defined(HAS_DVD_DRIVE)
  CLog::Log(LOGNOTICE, "stop dvd detect media");
  m_DetectDVDType.StopThread();
#endif

  g_peripherals.Clear();
}

void CApplication::ReloadSkin()
{
  m_skinReloading = false;
  CGUIMessage msg(GUI_MSG_LOAD_SKIN, -1, g_windowManager.GetActiveWindow());
  g_windowManager.SendMessage(msg);
  
  // Reload the skin, restoring the previously focused control.  We need this as
  // the window unload will reset all control states.
  int iCtrlID = -1;
  CGUIWindow* pWindow = g_windowManager.GetWindow(g_windowManager.GetActiveWindow());
  if (pWindow)
    iCtrlID = pWindow->GetFocusedControlID();
  
  g_application.LoadSkin(g_guiSettings.GetString("lookandfeel.skin"));
 
  if (iCtrlID != -1)
  {
    pWindow = g_windowManager.GetWindow(g_windowManager.GetActiveWindow());
    if (pWindow && pWindow->HasSaveLastControl())
    {
      CGUIMessage msg3(GUI_MSG_SETFOCUS, g_windowManager.GetActiveWindow(), iCtrlID, 0);
      pWindow->OnMessage(msg3);
    }
  }
}

bool CApplication::LoadSkin(const CStdString& skinID)
{
  if (m_skinReloading)
    return false;

  AddonPtr addon;
  if (CAddonMgr::Get().GetAddon(skinID, addon, ADDON_SKIN))
  {
    LoadSkin(boost::dynamic_pointer_cast<ADDON::CSkinInfo>(addon));
    return true;
  }
  return false;
}

void CApplication::LoadSkin(const SkinPtr& skin)
{
  if (!skin)
  {
    CLog::Log(LOGERROR, "failed to load requested skin, fallback to \"%s\" skin", DEFAULT_SKIN);
    g_guiSettings.SetString("lookandfeel.skin", DEFAULT_SKIN);
    LoadSkin(DEFAULT_SKIN);
    return ;
  }

  skin->Start();
  if (!skin->HasSkinFile("Home.xml"))
  {
    // failed to find home.xml
    // fallback to default skin
    if (strcmpi(skin->ID().c_str(), DEFAULT_SKIN) != 0)
    {
      CLog::Log(LOGERROR, "home.xml doesn't exist in skin: %s, fallback to \"%s\" skin", skin->ID().c_str(), DEFAULT_SKIN);
      g_guiSettings.SetString("lookandfeel.skin", DEFAULT_SKIN);
      LoadSkin(DEFAULT_SKIN);
      CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Error, g_localizeStrings.Get(24102), g_localizeStrings.Get(24103));
      return ;
    }
  }

  bool bPreviousPlayingState=false;
  bool bPreviousRenderingState=false;
  if (g_application.m_pPlayer && g_application.IsPlayingVideo())
  {
    bPreviousPlayingState = !g_application.m_pPlayer->IsPaused();
    if (bPreviousPlayingState)
      g_application.m_pPlayer->Pause();
#ifdef HAS_VIDEO_PLAYBACK
    if (!g_renderManager.Paused())
    {
      if (g_windowManager.GetActiveWindow() == WINDOW_FULLSCREEN_VIDEO)
     {
        g_windowManager.ActivateWindow(WINDOW_HOME);
        bPreviousRenderingState = true;
      }
    }
#endif
  }
  // close the music and video overlays (they're re-opened automatically later)
  CSingleLock lock(g_graphicsContext);

  // save the current window details
  int currentWindow = g_windowManager.GetActiveWindow();
  vector<int> currentModelessWindows;
  g_windowManager.GetActiveModelessWindows(currentModelessWindows);

  UnloadSkin();

  CLog::Log(LOGINFO, "  load skin from: %s (version: %s)", skin->Path().c_str(), skin->Version().c_str());
  g_SkinInfo = skin;
  g_SkinInfo->Start();

  CLog::Log(LOGINFO, "  load fonts for skin...");
  g_graphicsContext.SetMediaDir(skin->Path());
  g_directoryCache.ClearSubPaths(skin->Path());
  if (g_langInfo.ForceUnicodeFont() && !g_fontManager.IsFontSetUnicode(g_guiSettings.GetString("lookandfeel.font")))
  {
    CLog::Log(LOGINFO, "    language needs a ttf font, loading first ttf font available");
    CStdString strFontSet;
    if (g_fontManager.GetFirstFontSetUnicode(strFontSet))
    {
      CLog::Log(LOGINFO, "    new font is '%s'", strFontSet.c_str());
      g_guiSettings.SetString("lookandfeel.font", strFontSet);
      g_settings.Save();
    }
    else
      CLog::Log(LOGERROR, "    no ttf font found, but needed for the language %s.", g_guiSettings.GetString("locale.language").c_str());
  }
  g_colorManager.Load(g_guiSettings.GetString("lookandfeel.skincolors"));

  g_fontManager.LoadFonts(g_guiSettings.GetString("lookandfeel.font"));

  // load in the skin strings
  CStdString langPath;
  URIUtils::AddFileToFolder(skin->Path(), "language", langPath);
  URIUtils::AddSlashAtEnd(langPath);

  g_localizeStrings.LoadSkinStrings(langPath, g_guiSettings.GetString("locale.language"));

  g_SkinInfo->LoadIncludes();

  int64_t start;
  start = CurrentHostCounter();

  CLog::Log(LOGINFO, "  load new skin...");

  // Load the user windows
  LoadUserWindows();

  int64_t end, freq;
  end = CurrentHostCounter();
  freq = CurrentHostFrequency();
  CLog::Log(LOGDEBUG,"Load Skin XML: %.2fms", 1000.f * (end - start) / freq);

  CLog::Log(LOGINFO, "  initialize new skin...");
  g_windowManager.AddMsgTarget(this);
  g_windowManager.AddMsgTarget(&g_playlistPlayer);
  g_windowManager.AddMsgTarget(&g_infoManager);
  g_windowManager.AddMsgTarget(&g_fontManager);
  g_windowManager.SetCallback(*this);
  g_windowManager.Initialize();
  CTextureCache::Get().Initialize();
  g_audioManager.Enable(true);
  g_audioManager.Load();

  if (g_SkinInfo->HasSkinFile("DialogFullScreenInfo.xml"))
    g_windowManager.Add(new CGUIDialogFullScreenInfo);

  { // we can't register visible condition in dialog's ctor because infomanager is cleared when unloading skin
    CGUIDialog *overlay = (CGUIDialog *)g_windowManager.GetWindow(WINDOW_DIALOG_VIDEO_OVERLAY);
    if (overlay) overlay->SetVisibleCondition("skin.hasvideooverlay");
    overlay = (CGUIDialog *)g_windowManager.GetWindow(WINDOW_DIALOG_MUSIC_OVERLAY);
    if (overlay) overlay->SetVisibleCondition("skin.hasmusicoverlay");
  }

  CLog::Log(LOGINFO, "  skin loaded...");

  // leave the graphics lock
  lock.Leave();

  // restore windows
  if (currentWindow != WINDOW_INVALID)
  {
    g_windowManager.ActivateWindow(currentWindow);
    for (unsigned int i = 0; i < currentModelessWindows.size(); i++)
    {
      CGUIDialog *dialog = (CGUIDialog *)g_windowManager.GetWindow(currentModelessWindows[i]);
      if (dialog) dialog->Show();
    }
  }

  if (g_application.m_pPlayer && g_application.IsPlayingVideo())
  {
    if (bPreviousPlayingState)
      g_application.m_pPlayer->Pause();
    if (bPreviousRenderingState)
      g_windowManager.ActivateWindow(WINDOW_FULLSCREEN_VIDEO);
  }
}

void CApplication::UnloadSkin(bool forReload /* = false */)
{
  m_skinReloading = forReload;

  CLog::Log(LOGINFO, "Unloading old skin %s...", forReload ? "for reload " : "");

  g_audioManager.Enable(false);

  g_windowManager.DeInitialize();
  CTextureCache::Get().Deinitialize();

  // remove the skin-dependent window
  g_windowManager.Delete(WINDOW_DIALOG_FULLSCREEN_INFO);

  g_TextureManager.Cleanup();
  g_largeTextureManager.CleanupUnusedImages(true);

  g_fontManager.Clear();

  g_colorManager.Clear();

  g_charsetConverter.reset();

  g_infoManager.Clear();
}

bool CApplication::LoadUserWindows()
{
  // Start from wherever home.xml is
  std::vector<CStdString> vecSkinPath;
  g_SkinInfo->GetSkinPaths(vecSkinPath);
  for (unsigned int i = 0;i < vecSkinPath.size();++i)
  {
    CLog::Log(LOGINFO, "Loading user windows, path %s", vecSkinPath[i].c_str());
    CFileItemList items;
    if (CDirectory::GetDirectory(vecSkinPath[i], items, ".xml", DIR_FLAG_NO_FILE_DIRS))
    {
      for (int i = 0; i < items.Size(); ++i)
      {
        if (items[i]->m_bIsFolder)
          continue;
        CStdString skinFile = URIUtils::GetFileName(items[i]->GetPath());
        if (skinFile.Left(6).CompareNoCase("custom") == 0)
        {
          CXBMCTinyXML xmlDoc;
          if (!xmlDoc.LoadFile(items[i]->GetPath()))
          {
            CLog::Log(LOGERROR, "unable to load: %s, Line %d\n%s", items[i]->GetPath().c_str(), xmlDoc.ErrorRow(), xmlDoc.ErrorDesc());
            continue;
          }

          // Root element should be <window>
          TiXmlElement* pRootElement = xmlDoc.RootElement();
          CStdString strValue = pRootElement->Value();
          if (!strValue.Equals("window"))
          {
            CLog::Log(LOGERROR, "file: %s doesnt contain <window>", skinFile.c_str());
            continue;
          }

          // Read the <type> element to get the window type to create
          // If no type is specified, create a CGUIWindow as default
          CGUIWindow* pWindow = NULL;
          CStdString strType;
          if (pRootElement->Attribute("type"))
            strType = pRootElement->Attribute("type");
          else
          {
            const TiXmlNode *pType = pRootElement->FirstChild("type");
            if (pType && pType->FirstChild())
              strType = pType->FirstChild()->Value();
          }
          int id = WINDOW_INVALID;
          if (!pRootElement->Attribute("id", &id))
          {
            const TiXmlNode *pType = pRootElement->FirstChild("id");
            if (pType && pType->FirstChild())
              id = atol(pType->FirstChild()->Value());
          }
          CStdString visibleCondition;
          CGUIControlFactory::GetConditionalVisibility(pRootElement, visibleCondition);

          if (strType.Equals("dialog"))
            pWindow = new CGUIDialog(id + WINDOW_HOME, skinFile);
          else if (strType.Equals("submenu"))
            pWindow = new CGUIDialogSubMenu(id + WINDOW_HOME, skinFile);
          else if (strType.Equals("buttonmenu"))
            pWindow = new CGUIDialogButtonMenu(id + WINDOW_HOME, skinFile);
          else
            pWindow = new CGUIWindow(id + WINDOW_HOME, skinFile);

          // Check to make sure the pointer isn't still null
          if (pWindow == NULL)
          {
            CLog::Log(LOGERROR, "Out of memory / Failed to create new object in LoadUserWindows");
            return false;
          }
          if (id == WINDOW_INVALID || g_windowManager.GetWindow(WINDOW_HOME + id))
          {
            delete pWindow;
            continue;
          }
          pWindow->SetVisibleCondition(visibleCondition);
          pWindow->SetLoadType(CGUIWindow::KEEP_IN_MEMORY);
          g_windowManager.AddCustomWindow(pWindow);
        }
      }
    }
  }
  return true;
}

bool CApplication::RenderNoPresent()
{
  MEASURE_FUNCTION;

// DXMERGE: This may have been important?
//  g_graphicsContext.AcquireCurrentContext();

  g_graphicsContext.Lock();

  // dont show GUI when playing full screen video
  if (g_graphicsContext.IsFullScreenVideo())
  {
    if (m_bPresentFrame && IsPlaying() && !IsPaused())
    {
      ResetScreenSaver();
      g_renderManager.Present();
    }
    else
      g_renderManager.RenderUpdate(true);

    // close window overlays
    CGUIDialog *overlay = (CGUIDialog *)g_windowManager.GetWindow(WINDOW_DIALOG_VIDEO_OVERLAY);
    if (overlay) overlay->Close(true);
    overlay = (CGUIDialog *)g_windowManager.GetWindow(WINDOW_DIALOG_MUSIC_OVERLAY);
    if (overlay) overlay->Close(true);

  }

  bool hasRendered = g_windowManager.Render();

  g_graphicsContext.Unlock();

  return hasRendered;
}

float CApplication::GetDimScreenSaverLevel() const
{
  if (!m_bScreenSave || !m_screenSaver ||
      (m_screenSaver->ID() != "screensaver.xbmc.builtin.dim" &&
       m_screenSaver->ID() != "screensaver.xbmc.builtin.black" &&
       m_screenSaver->ID() != "screensaver.xbmc.builtin.slideshow"))
    return 0;

  if (!m_screenSaver->GetSetting("level").IsEmpty())
    return 100.0f - (float)atof(m_screenSaver->GetSetting("level"));
  return 100.0f;
}

bool CApplication::WaitFrame(unsigned int timeout)
{
  bool done = false;

  // Wait for all other frames to be presented
  CSingleLock lock(m_frameMutex);
  //wait until event is set, but modify remaining time

  TightConditionVariable<InversePredicate<int&> > cv(m_frameCond, InversePredicate<int&>(m_frameCount));
  cv.wait(lock,timeout);
  done = m_frameCount == 0;

  return done;
}

void CApplication::NewFrame()
{
  // We just posted another frame. Keep track and notify.
  {
    CSingleLock lock(m_frameMutex);
    m_frameCount++;
  }

  m_frameCond.notifyAll();
}

void CApplication::Render()
{
  // do not render if we are stopped
  if (m_bStop)
    return;

  if (!m_AppActive && !m_bStop && (!IsPlayingVideo() || IsPaused()))
  {
    Sleep(1);
    ResetScreenSaver();
    return;
  }

  MEASURE_FUNCTION;

  int vsync_mode = g_guiSettings.GetInt("videoscreen.vsync");

  bool decrement = false;
  bool hasRendered = false;
  bool limitFrames = false;
  unsigned int singleFrameTime = 10; // default limit 100 fps

  {
    // Less fps in DPMS
    bool lowfps = m_dpmsIsActive || g_Windowing.EnableFrameLimiter();
    // Whether externalplayer is playing and we're unfocused
    bool extPlayerActive = m_eCurrentPlayer == EPC_EXTPLAYER && IsPlaying() && !m_AppFocused;

    m_bPresentFrame = false;
    if (!extPlayerActive && g_graphicsContext.IsFullScreenVideo() && !IsPaused())
    {
      CSingleLock lock(m_frameMutex);

      TightConditionVariable<int&> cv(m_frameCond,m_frameCount);
      cv.wait(lock,100);

      m_bPresentFrame = m_frameCount > 0;
      decrement = m_bPresentFrame;
      hasRendered = true;
    }
    else
    {
      // engage the frame limiter as needed
      limitFrames = lowfps || extPlayerActive;
      // DXMERGE - we checked for g_videoConfig.GetVSyncMode() before this
      //           perhaps allowing it to be set differently than the UI option??
      if (vsync_mode == VSYNC_DISABLED || vsync_mode == VSYNC_VIDEO)
        limitFrames = true; // not using vsync.
      else if ((g_infoManager.GetFPS() > g_graphicsContext.GetFPS() + 10) && g_infoManager.GetFPS() > 1000 / singleFrameTime)
        limitFrames = true; // using vsync, but it isn't working.

      if (limitFrames)
      {
        if (extPlayerActive)
        {
          ResetScreenSaver();  // Prevent screensaver dimming the screen
          singleFrameTime = 1000;  // 1 fps, high wakeup latency but v.low CPU usage
        }
        else if (lowfps)
          singleFrameTime = 200;  // 5 fps, <=200 ms latency to wake up
      }

      decrement = true;
    }
  }

  CSingleLock lock(g_graphicsContext);
  g_infoManager.UpdateFPS();

  if (g_graphicsContext.IsFullScreenVideo() && IsPlaying() && vsync_mode == VSYNC_VIDEO)
    g_Windowing.SetVSync(true);
  else if (vsync_mode == VSYNC_ALWAYS)
    g_Windowing.SetVSync(true);
  else if (vsync_mode != VSYNC_DRIVER)
    g_Windowing.SetVSync(false);

  if(!g_Windowing.BeginRender())
    return;

  CDirtyRegionList dirtyRegions = g_windowManager.GetDirty();
  if (RenderNoPresent())
    hasRendered = true;

  g_Windowing.EndRender();

  // reset our info cache - we do this at the end of Render so that it is
  // fresh for the next process(), or after a windowclose animation (where process()
  // isn't called)
  g_infoManager.ResetCache();
  lock.Leave();

  unsigned int now = XbmcThreads::SystemClockMillis();
  if (hasRendered)
    m_lastRenderTime = now;

  //when nothing has been rendered for m_guiDirtyRegionNoFlipTimeout milliseconds,
  //we don't call g_graphicsContext.Flip() anymore, this saves gpu and cpu usage
  bool flip;
  if (g_advancedSettings.m_guiDirtyRegionNoFlipTimeout >= 0)
    flip = hasRendered || (now - m_lastRenderTime) < (unsigned int)g_advancedSettings.m_guiDirtyRegionNoFlipTimeout;
  else
    flip = true;

  //fps limiter, make sure each frame lasts at least singleFrameTime milliseconds
  if (limitFrames || !flip)
  {
    if (!limitFrames)
      singleFrameTime = 40; //if not flipping, loop at 25 fps

    unsigned int frameTime = now - m_lastFrameTime;
    if (frameTime < singleFrameTime)
      Sleep(singleFrameTime - frameTime);
  }
  m_lastFrameTime = XbmcThreads::SystemClockMillis();

  if (flip)
    g_graphicsContext.Flip(dirtyRegions);
  CTimeUtils::UpdateFrameTime(flip);

  g_TextureManager.FreeUnusedTextures();

  g_renderManager.UpdateResolution();
  g_renderManager.ManageCaptures();

  {
    CSingleLock lock(m_frameMutex);
    if(m_frameCount > 0 && decrement)
      m_frameCount--;
  }
  m_frameCond.notifyAll();
}

void CApplication::SetStandAlone(bool value)
{
  g_advancedSettings.m_handleMounting = m_bStandalone = value;
}

// OnKey() translates the key into a CAction which is sent on to our Window Manager.
// The window manager will return true if the event is processed, false otherwise.
// If not already processed, this routine handles global keypresses.  It returns
// true if the key has been processed, false otherwise.

bool CApplication::OnKey(const CKey& key)
{

  // Turn the mouse off, as we've just got a keypress from controller or remote
  g_Mouse.SetActive(false);

  // get the current active window
  int iWin = g_windowManager.GetActiveWindow() & WINDOW_ID_MASK;

  // this will be checked for certain keycodes that need
  // special handling if the screensaver is active
  CAction action = CButtonTranslator::GetInstance().GetAction(iWin, key);

  // a key has been pressed.
  // reset Idle Timer
  m_idleTimer.StartZero();
  bool processKey = AlwaysProcess(action);

  ResetScreenSaver();

  // allow some keys to be processed while the screensaver is active
  if (WakeUpScreenSaverAndDPMS(processKey) && !processKey)
  {
    CLog::Log(LOGDEBUG, "%s: %s pressed, screen saver/dpms woken up", __FUNCTION__, g_Keyboard.GetKeyName((int) key.GetButtonCode()).c_str());
    return true;
  }

  // change this if we have a dialog up
  if (g_windowManager.HasModalDialog())
  {
    iWin = g_windowManager.GetTopMostModalDialogID() & WINDOW_ID_MASK;
  }
  if (iWin == WINDOW_DIALOG_FULLSCREEN_INFO)
  { // fullscreen info dialog - special case
    action = CButtonTranslator::GetInstance().GetAction(iWin, key);

    if (!key.IsAnalogButton())
      CLog::Log(LOGDEBUG, "%s: %s pressed, trying fullscreen info action %s", __FUNCTION__, g_Keyboard.GetKeyName((int) key.GetButtonCode()).c_str(), action.GetName().c_str());

    if (OnAction(action))
      return true;

    // fallthrough to the main window
    iWin = WINDOW_FULLSCREEN_VIDEO;
  }
  if (iWin == WINDOW_FULLSCREEN_VIDEO)
  {
    // current active window is full screen video.
    if (g_application.m_pPlayer && g_application.m_pPlayer->IsInMenu())
    {
      // if player is in some sort of menu, (ie DVDMENU) map buttons differently
      action = CButtonTranslator::GetInstance().GetAction(WINDOW_VIDEO_MENU, key);
    }
    else if (g_PVRManager.IsStarted() && g_application.CurrentFileItem().HasPVRChannelInfoTag())
    {
      // check for PVR specific keymaps in FULLSCREEN_VIDEO window
      action = CButtonTranslator::GetInstance().GetAction(WINDOW_FULLSCREEN_LIVETV, key, false);

      // if no PVR specific action/mapping is found, fall back to default
      if (action.GetID() == 0)
        action = CButtonTranslator::GetInstance().GetAction(iWin, key);
    }
    else
    {
      // in any other case use the fullscreen window section of keymap.xml to map key->action
      action = CButtonTranslator::GetInstance().GetAction(iWin, key);
    }
  }
  else
  {
    // current active window isnt the fullscreen window
    // just use corresponding section from keymap.xml
    // to map key->action

    // first determine if we should use keyboard input directly
    bool useKeyboard = key.FromKeyboard() && (iWin == WINDOW_DIALOG_KEYBOARD || iWin == WINDOW_DIALOG_NUMERIC);
    CGUIWindow *window = g_windowManager.GetWindow(iWin);
    if (window)
    {
      CGUIControl *control = window->GetFocusedControl();
      if (control)
      {
        // If this is an edit control set usekeyboard to true. This causes the
        // keypress to be processed directly not through the key mappings.
        if (control->GetControlType() == CGUIControl::GUICONTROL_EDIT)
          useKeyboard = true;

        // If the key pressed is shift-A to shift-Z set usekeyboard to true.
        // This causes the keypress to be used for list navigation.
        if (control->IsContainer() && key.GetModifiers() == CKey::MODIFIER_SHIFT && key.GetVKey() >= XBMCVK_A && key.GetVKey() <= XBMCVK_Z)
          useKeyboard = true;
      }
    }
    if (useKeyboard)
    {
      action = CAction(0); // reset our action
      if (g_guiSettings.GetBool("input.remoteaskeyboard"))
      {
        // users remote is executing keyboard commands, so use the virtualkeyboard section of keymap.xml
        // and send those rather than actual keyboard presses.  Only for navigation-type commands though
        action = CButtonTranslator::GetInstance().GetAction(WINDOW_DIALOG_KEYBOARD, key);
        if (!(action.GetID() == ACTION_MOVE_LEFT ||
              action.GetID() == ACTION_MOVE_RIGHT ||
              action.GetID() == ACTION_MOVE_UP ||
              action.GetID() == ACTION_MOVE_DOWN ||
              action.GetID() == ACTION_SELECT_ITEM ||
              action.GetID() == ACTION_ENTER ||
              action.GetID() == ACTION_PREVIOUS_MENU ||
              action.GetID() == ACTION_NAV_BACK))
        {
          // the action isn't plain navigation - check for a keyboard-specific keymap
          action = CButtonTranslator::GetInstance().GetAction(WINDOW_DIALOG_KEYBOARD, key, false);
          if (!(action.GetID() >= REMOTE_0 && action.GetID() <= REMOTE_9) ||
                action.GetID() == ACTION_BACKSPACE ||
                action.GetID() == ACTION_SHIFT ||
                action.GetID() == ACTION_SYMBOLS ||
                action.GetID() == ACTION_CURSOR_LEFT ||
                action.GetID() == ACTION_CURSOR_RIGHT)
            action = CAction(0); // don't bother with this action
        }
      }
      if (!action.GetID())
      {
        // keyboard entry - pass the keys through directly
        if (key.GetFromService())
          action = CAction(key.GetButtonCode() != KEY_INVALID ? key.GetButtonCode() : 0, key.GetUnicode());
        else
        { // see if we've got an ascii key
          if (key.GetUnicode())
            action = CAction(key.GetAscii() | KEY_ASCII, key.GetUnicode());
          else
            action = CAction(key.GetVKey() | KEY_VKEY);
        }
      }

      CLog::Log(LOGDEBUG, "%s: %s pressed, trying keyboard action %i", __FUNCTION__, g_Keyboard.GetKeyName((int) key.GetButtonCode()).c_str(), action.GetID());

      if (OnAction(action))
        return true;
      // failed to handle the keyboard action, drop down through to standard action
    }
    if (key.GetFromService())
    {
      if (key.GetButtonCode() != KEY_INVALID)
        action = CButtonTranslator::GetInstance().GetAction(iWin, key);
    }
    else
      action = CButtonTranslator::GetInstance().GetAction(iWin, key);
  }
  if (!key.IsAnalogButton())
    CLog::Log(LOGDEBUG, "%s: %s pressed, action is %s", __FUNCTION__, g_Keyboard.GetKeyName((int) key.GetButtonCode()).c_str(), action.GetName().c_str());

  bool bResult = false;

  // play sound before the action unless the button is held,
  // where we execute after the action as held actions aren't fired every time.
  if(action.GetHoldTime())
  {
    bResult = OnAction(action);
    if(bResult)
      g_audioManager.PlayActionSound(action);
  }
  else
  {
    g_audioManager.PlayActionSound(action);
    bResult = OnAction(action);
  }

  return bResult;
}

// OnAppCommand is called in response to a XBMC_APPCOMMAND event.
// This needs to return true if it processed the appcommand or false if it didn't
bool CApplication::OnAppCommand(const CAction &action)
{
  // Reset the screen saver
  ResetScreenSaver();

  // If we were currently in the screen saver wake up and don't process the appcommand
  if (WakeUpScreenSaverAndDPMS())
    return true;

  // The action ID is the APPCOMMAND code. We need to retrieve the action
  // associated with this appcommand from the mapping table.
  uint32_t appcmd = action.GetID();
  CKey key(appcmd | KEY_APPCOMMAND, (unsigned int) 0);
  int iWin = g_windowManager.GetActiveWindow() & WINDOW_ID_MASK;
  CAction appcmdaction = CButtonTranslator::GetInstance().GetAction(iWin, key);

  // If we couldn't find an action return false to indicate we have not
  // handled this appcommand
  if (!appcmdaction.GetID())
  {
    CLog::Log(LOGDEBUG, "%s: unknown appcommand %d", __FUNCTION__, appcmd);
    return false;
  }

  // Process the appcommand
  CLog::Log(LOGDEBUG, "%s: appcommand %d, trying action %s", __FUNCTION__, appcmd, appcmdaction.GetName().c_str());
  OnAction(appcmdaction);

  // Always return true regardless of whether the action succeeded or not.
  // This stops Windows handling the appcommand itself.
  return true;
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
    g_graphicsContext.ToggleFullScreenRoot();
    return true;
  }

  if (action.IsMouse())
    g_Mouse.SetActive(true);

  // The action PLAYPAUSE behaves as ACTION_PAUSE if we are currently
  // playing or ACTION_PLAYER_PLAY if we are not playing.
  if (action.GetID() == ACTION_PLAYER_PLAYPAUSE)
  {
    if (IsPlaying())
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
    if (g_windowManager.OnAction(action))
    {
      m_navigationTimer.StartZero();
      return true;
    }
  }

  // handle extra global presses

  // screenshot : take a screenshot :)
  if (action.GetID() == ACTION_TAKE_SCREENSHOT)
  {
    CScreenShot::TakeScreenshot();
    return true;
  }
  // built in functions : execute the built-in
  if (action.GetID() == ACTION_BUILT_IN_FUNCTION)
  {
    CBuiltins::Execute(action.GetName());
    m_navigationTimer.StartZero();
    return true;
  }

  // reload keymaps
  if (action.GetID() == ACTION_RELOAD_KEYMAPS)
  {
    CButtonTranslator::GetInstance().Clear();
    CButtonTranslator::GetInstance().Load();
  }

  // show info : Shows the current video or song information
  if (action.GetID() == ACTION_SHOW_INFO)
  {
    g_infoManager.ToggleShowInfo();
    return true;
  }

  // codec info : Shows the current song, video or picture codec information
  if (action.GetID() == ACTION_SHOW_CODEC)
  {
    g_infoManager.ToggleShowCodec();
    return true;
  }

  if ((action.GetID() == ACTION_INCREASE_RATING || action.GetID() == ACTION_DECREASE_RATING) && IsPlayingAudio())
  {
    const CMusicInfoTag *tag = g_infoManager.GetCurrentSongTag();
    if (tag)
    {
      *m_itemCurrentFile->GetMusicInfoTag() = *tag;
      char rating = tag->GetRating();
      bool needsUpdate(false);
      if (rating > '0' && action.GetID() == ACTION_DECREASE_RATING)
      {
        m_itemCurrentFile->GetMusicInfoTag()->SetRating(rating - 1);
        needsUpdate = true;
      }
      else if (rating < '5' && action.GetID() == ACTION_INCREASE_RATING)
      {
        m_itemCurrentFile->GetMusicInfoTag()->SetRating(rating + 1);
        needsUpdate = true;
      }
      if (needsUpdate)
      {
        CMusicDatabase db;
        if (db.Open())      // OpenForWrite() ?
        {
          db.SetSongRating(m_itemCurrentFile->GetPath(), m_itemCurrentFile->GetMusicInfoTag()->GetRating());
          db.Close();
        }
        // send a message to all windows to tell them to update the fileitem (eg playlistplayer, media windows)
        CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_ITEM, 0, m_itemCurrentFile);
        g_windowManager.SendMessage(msg);
      }
    }
    return true;
  }

  // stop : stops playing current audio song
  if (action.GetID() == ACTION_STOP)
  {
    StopPlaying();
    return true;
  }

  // previous : play previous song from playlist
  if (action.GetID() == ACTION_PREV_ITEM)
  {
    // first check whether we're within 3 seconds of the start of the track
    // if not, we just revert to the start of the track
    if (m_pPlayer && m_pPlayer->CanSeek() && GetTime() > 3)
    {
      SeekTime(0);
      SetPlaySpeed(1);
    }
    else
    {
      g_playlistPlayer.PlayPrevious();
    }
    return true;
  }

  // next : play next song from playlist
  if (action.GetID() == ACTION_NEXT_ITEM)
  {
    if (IsPlaying() && m_pPlayer->SkipNext())
      return true;

    if (IsPaused())
      m_pPlayer->Pause();

    g_playlistPlayer.PlayNext();

    return true;
  }

  if (IsPlaying())
  {
    // forward channel switches to the player - he knows what to do
    if (action.GetID() == ACTION_CHANNEL_UP || action.GetID() == ACTION_CHANNEL_DOWN)
    {
      m_pPlayer->OnAction(action);
      return true;
    }

    // pause : pauses current audio song
    if (action.GetID() == ACTION_PAUSE && m_iPlaySpeed == 1)
    {
      m_pPlayer->Pause();
#ifdef HAS_KARAOKE
      m_pKaraokeMgr->SetPaused( m_pPlayer->IsPaused() );
#endif
      if (!m_pPlayer->IsPaused())
      { // unpaused - set the playspeed back to normal
        SetPlaySpeed(1);
      }
      g_audioManager.Enable(m_pPlayer->IsPaused());
      return true;
    }
    if (!m_pPlayer->IsPaused())
    {
      // if we do a FF/RW in my music then map PLAY action togo back to normal speed
      // if we are playing at normal speed, then allow play to pause
      if (action.GetID() == ACTION_PLAYER_PLAY || action.GetID() == ACTION_PAUSE)
      {
        if (m_iPlaySpeed != 1)
        {
          SetPlaySpeed(1);
        }
        else
        {
          m_pPlayer->Pause();
        }
        return true;
      }
      if (action.GetID() == ACTION_PLAYER_FORWARD || action.GetID() == ACTION_PLAYER_REWIND)
      {
        int iPlaySpeed = m_iPlaySpeed;
        if (action.GetID() == ACTION_PLAYER_REWIND && iPlaySpeed == 1) // Enables Rewinding
          iPlaySpeed *= -2;
        else if (action.GetID() == ACTION_PLAYER_REWIND && iPlaySpeed > 1) //goes down a notch if you're FFing
          iPlaySpeed /= 2;
        else if (action.GetID() == ACTION_PLAYER_FORWARD && iPlaySpeed < 1) //goes up a notch if you're RWing
          iPlaySpeed /= 2;
        else
          iPlaySpeed *= 2;

        if (action.GetID() == ACTION_PLAYER_FORWARD && iPlaySpeed == -1) //sets iSpeed back to 1 if -1 (didn't plan for a -1)
          iPlaySpeed = 1;
        if (iPlaySpeed > 32 || iPlaySpeed < -32)
          iPlaySpeed = 1;

        SetPlaySpeed(iPlaySpeed);
        return true;
      }
      else if ((action.GetAmount() || GetPlaySpeed() != 1) && (action.GetID() == ACTION_ANALOG_REWIND || action.GetID() == ACTION_ANALOG_FORWARD))
      {
        // calculate the speed based on the amount the button is held down
        int iPower = (int)(action.GetAmount() * MAX_FFWD_SPEED + 0.5f);
        // returns 0 -> MAX_FFWD_SPEED
        int iSpeed = 1 << iPower;
        if (iSpeed != 1 && action.GetID() == ACTION_ANALOG_REWIND)
          iSpeed = -iSpeed;
        g_application.SetPlaySpeed(iSpeed);
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
        m_pPlayer->Pause();
        g_audioManager.Enable(m_pPlayer->IsPaused());

        g_application.SetPlaySpeed(1);
        return true;
      }
    }
  }

  if (g_peripherals.OnAction(action))
    return true;

  if (action.GetID() == ACTION_MUTE)
  {
    ToggleMute();
    return true;
  }

  if (action.GetID() == ACTION_TOGGLE_DIGITAL_ANALOG)
  {
    switch(g_guiSettings.GetInt("audiooutput.mode"))
    {
      case AUDIO_ANALOG: g_guiSettings.SetInt("audiooutput.mode", AUDIO_IEC958); break;
      case AUDIO_IEC958: g_guiSettings.SetInt("audiooutput.mode", AUDIO_HDMI  ); break;
      case AUDIO_HDMI  : g_guiSettings.SetInt("audiooutput.mode", AUDIO_ANALOG); break;
    }

    g_application.Restart();
    if (g_windowManager.GetActiveWindow() == WINDOW_SETTINGS_SYSTEM)
    {
      CGUIMessage msg(GUI_MSG_WINDOW_INIT, 0,0,WINDOW_INVALID,g_windowManager.GetActiveWindow());
      g_windowManager.SendMessage(msg);
    }
    return true;
  }

  // Check for global volume control
  if (action.GetAmount() && (action.GetID() == ACTION_VOLUME_UP || action.GetID() == ACTION_VOLUME_DOWN))
  {
    if (!m_pPlayer || !m_pPlayer->IsPassthrough())
    {
      if (g_settings.m_bMute)
        UnMute();
      float volume = g_settings.m_fVolumeLevel;
      float step   = (VOLUME_MAXIMUM - VOLUME_MINIMUM) / VOLUME_CONTROL_STEPS;
      if (action.GetRepeat())
        step *= action.GetRepeat() * 50; // 50 fps

      if (action.GetID() == ACTION_VOLUME_UP)
        volume += (float)fabs(action.GetAmount()) * action.GetAmount() * step;
      else
        volume -= (float)fabs(action.GetAmount()) * action.GetAmount() * step;

      SetVolume(volume, false);
    }
    // show visual feedback of volume change...
    ShowVolumeBar(&action);
    return true;
  }
  // Check for global seek control
  if (IsPlaying() && action.GetAmount() && (action.GetID() == ACTION_ANALOG_SEEK_FORWARD || action.GetID() == ACTION_ANALOG_SEEK_BACK))
  {
    if (!m_pPlayer->CanSeek()) return false;
    m_seekHandler->Seek(action.GetID() == ACTION_ANALOG_SEEK_FORWARD, action.GetAmount(), action.GetRepeat());
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
    int iPlaylist = g_playlistPlayer.GetCurrentPlaylist();
    if (iPlaylist == PLAYLIST_VIDEO && g_windowManager.GetActiveWindow() != WINDOW_VIDEO_PLAYLIST)
      g_windowManager.ActivateWindow(WINDOW_VIDEO_PLAYLIST);
    else if (iPlaylist == PLAYLIST_MUSIC && g_windowManager.GetActiveWindow() != WINDOW_MUSIC_PLAYLIST)
      g_windowManager.ActivateWindow(WINDOW_MUSIC_PLAYLIST);
    return true;
  }
  return false;
}

void CApplication::UpdateLCD()
{
#ifdef HAS_LCD
  static unsigned int lTickCount = 0;

  if (!g_lcd || !g_guiSettings.GetBool("videoscreen.haslcd"))
    return ;
  unsigned int lTimeOut = 1000;
  if ( m_iPlaySpeed != 1)
    lTimeOut = 0;
  if ( (XbmcThreads::SystemClockMillis() - lTickCount) >= lTimeOut)
  {
    if (g_application.NavigationIdleTime() < 5)
      g_lcd->Render(ILCD::LCD_MODE_NAVIGATION);
    else if (g_PVRManager.IsPlayingTV())
      g_lcd->Render(ILCD::LCD_MODE_PVRTV);
    else if (g_PVRManager.IsPlayingRadio())
      g_lcd->Render(ILCD::LCD_MODE_PVRRADIO);
    else if (IsPlayingVideo())
      g_lcd->Render(ILCD::LCD_MODE_VIDEO);
    else if (IsPlayingAudio())
      g_lcd->Render(ILCD::LCD_MODE_MUSIC);
    else if (IsInScreenSaver())
      g_lcd->Render(ILCD::LCD_MODE_SCREENSAVER);
    else
      g_lcd->Render(ILCD::LCD_MODE_GENERAL);

    // reset tick count
    lTickCount = XbmcThreads::SystemClockMillis();
  }
#endif
}

void CApplication::FrameMove(bool processEvents, bool processGUI)
{
  MEASURE_FUNCTION;

  if (processEvents)
  {
    // currently we calculate the repeat time (ie time from last similar keypress) just global as fps
    float frameTime = m_frameTime.GetElapsedSeconds();
    m_frameTime.StartZero();
    // never set a frametime less than 2 fps to avoid problems when debuggin and on breaks
    if( frameTime > 0.5 ) frameTime = 0.5;

    if (processGUI && m_renderGUI)
    {
      g_graphicsContext.Lock();
      // check if there are notifications to display
      CGUIDialogKaiToast *toast = (CGUIDialogKaiToast *)g_windowManager.GetWindow(WINDOW_DIALOG_KAI_TOAST);
      if (toast && toast->DoWork())
      {
        if (!toast->IsDialogRunning())
        {
          toast->Show();
        }
      }
      g_graphicsContext.Unlock();
      CWinEvents::MessagePump();
    }

    UpdateLCD();

#if defined(HAS_LIRC) || defined(HAS_IRSERVERSUITE)
    // Read the input from a remote
    g_RemoteControl.Update();
#endif

    // process input actions
    ProcessRemote(frameTime);
    ProcessGamepad(frameTime);
    ProcessEventServer(frameTime);
    ProcessPeripherals(frameTime);
    if (processGUI && m_renderGUI)
    {
      m_pInertialScrollingHandler->ProcessInertialScroll(frameTime);
      m_seekHandler->Process();
    }
  }
  if (processGUI && m_renderGUI)
  {
    if (!m_bStop)
      g_windowManager.Process(CTimeUtils::GetFrameTime());
    g_windowManager.FrameMove();
  }
}

bool CApplication::ProcessGamepad(float frameTime)
{
#ifdef HAS_SDL_JOYSTICK
  if (!m_AppFocused)
    return false;

  int iWin = GetActiveWindowID();
  int bid = 0;
  g_Joystick.Update();
  if (g_Joystick.GetButton(bid))
  {
    // reset Idle Timer
    m_idleTimer.StartZero();

    ResetScreenSaver();
    if (WakeUpScreenSaverAndDPMS())
    {
      g_Joystick.Reset(true);
      return true;
    }

    int actionID;
    CStdString actionName;
    bool fullrange;
    if (CButtonTranslator::GetInstance().TranslateJoystickString(iWin, g_Joystick.GetJoystick().c_str(), bid, JACTIVE_BUTTON, actionID, actionName, fullrange))
    {
      CAction action(actionID, 1.0f, 0.0f, actionName);
      g_audioManager.PlayActionSound(action);
      g_Joystick.Reset();
      g_Mouse.SetActive(false);
      return OnAction(action);
    }
    else
    {
      g_Joystick.Reset();
    }
  }
  if (g_Joystick.GetAxis(bid))
  {
    if (g_Joystick.GetAmount() < 0)
    {
      bid = -bid;
    }

    int actionID;
    CStdString actionName;
    bool fullrange;
    if (CButtonTranslator::GetInstance().TranslateJoystickString(iWin, g_Joystick.GetJoystick().c_str(), bid, JACTIVE_AXIS, actionID, actionName, fullrange))
    {
      ResetScreenSaver();
      if (WakeUpScreenSaverAndDPMS())
      {
        return true;
      }

      CAction action(actionID, fullrange ? (g_Joystick.GetAmount() + 1.0f)/2.0f : fabs(g_Joystick.GetAmount()), 0.0f, actionName);
      g_audioManager.PlayActionSound(action);
      g_Joystick.Reset();
      g_Mouse.SetActive(false);
      return OnAction(action);
    }
    else
    {
      g_Joystick.ResetAxis(abs(bid));
    }
  }
  int position = 0;
  if (g_Joystick.GetHat(bid, position))
  {
    // reset Idle Timer
    m_idleTimer.StartZero();

    ResetScreenSaver();
    if (WakeUpScreenSaverAndDPMS())
    {
      g_Joystick.Reset();
      return true;
    }

    int actionID;
    CStdString actionName;
    bool fullrange;

    bid = position<<16|bid;

    if (bid && CButtonTranslator::GetInstance().TranslateJoystickString(iWin, g_Joystick.GetJoystick().c_str(), bid, JACTIVE_HAT, actionID, actionName, fullrange))
    {
      CAction action(actionID, 1.0f, 0.0f, actionName);
      g_audioManager.PlayActionSound(action);
      g_Joystick.Reset();
      g_Mouse.SetActive(false);
      return OnAction(action);
    }
  }
#endif
  return false;
}

bool CApplication::ProcessRemote(float frameTime)
{
#if defined(HAS_LIRC) || defined(HAS_IRSERVERSUITE)
  if (g_RemoteControl.GetButton())
  {
    CKey key(g_RemoteControl.GetButton(), g_RemoteControl.GetHoldTime());
    g_RemoteControl.Reset();
    return OnKey(key);
  }
#endif
  return false;
}

bool CApplication::ProcessPeripherals(float frameTime)
{
  CKey key;
  if (g_peripherals.GetNextKeypress(frameTime, key))
    return OnKey(key);
  return false;
}

bool CApplication::ProcessMouse()
{
  MEASURE_FUNCTION;

  if (!g_Mouse.IsActive() || !m_AppFocused)
    return false;

  // Get the mouse command ID
  uint32_t mousecommand = g_Mouse.GetAction();
  if (mousecommand == ACTION_NOOP)
    return true;

  // Reset the screensaver and idle timers
  m_idleTimer.StartZero();
  ResetScreenSaver();
  if (WakeUpScreenSaverAndDPMS())
    return true;

  // Retrieve the corresponding action
  int iWin;
  CKey key(mousecommand | KEY_MOUSE, (unsigned int) 0);
  if (g_windowManager.HasModalDialog())
    iWin = g_windowManager.GetTopMostModalDialogID() & WINDOW_ID_MASK;
  else
    iWin = g_windowManager.GetActiveWindow() & WINDOW_ID_MASK;
  CAction mouseaction = CButtonTranslator::GetInstance().GetAction(iWin, key);

  // If we couldn't find an action return false to indicate we have not
  // handled this mouse action
  if (!mouseaction.GetID())
  {
    CLog::Log(LOGDEBUG, "%s: unknown mouse command %d", __FUNCTION__, mousecommand);
    return false;
  }

  // Log mouse actions except for move and noop
  if (mouseaction.GetID() != ACTION_MOUSE_MOVE && mouseaction.GetID() != ACTION_NOOP)
    CLog::Log(LOGDEBUG, "%s: trying mouse action %s", __FUNCTION__, mouseaction.GetName().c_str());

  // The action might not be a mouse action. For example wheel moves might
  // be mapped to volume up/down in mouse.xml. In this case we do not want
  // the mouse position saved in the action.
  if (!mouseaction.IsMouse())
    return OnAction(mouseaction);

  // This is a mouse action so we need to record the mouse position
  return OnAction(CAction(mouseaction.GetID(),
                          g_Mouse.GetHold(MOUSE_LEFT_BUTTON),
                          (float)g_Mouse.GetX(),
                          (float)g_Mouse.GetY(),
                          (float)g_Mouse.GetDX(),
                          (float)g_Mouse.GetDY(),
                          mouseaction.GetName()));
}

bool CApplication::ProcessEventServer(float frameTime)
{
#ifdef HAS_EVENT_SERVER
  CEventServer* es = CEventServer::GetInstance();
  if (!es || !es->Running() || es->GetNumberOfClients()==0)
    return false;

  // process any queued up actions
  if (es->ExecuteNextAction())
  {
    // reset idle timers
    m_idleTimer.StartZero();
    ResetScreenSaver();
    WakeUpScreenSaverAndDPMS();
  }

  // now handle any buttons or axis
  std::string joystickName;
  bool isAxis = false;
  float fAmount = 0.0;

  // es->ExecuteNextAction() invalidates the ref to the CEventServer instance
  // when the action exits XBMC
  es = CEventServer::GetInstance();
  if (!es || !es->Running() || es->GetNumberOfClients()==0)
    return false;
  unsigned int wKeyID = es->GetButtonCode(joystickName, isAxis, fAmount);

  if (wKeyID)
  {
    if (joystickName.length() > 0)
    {
      if (isAxis == true)
      {
        if (fabs(fAmount) >= 0.08)
          m_lastAxisMap[joystickName][wKeyID] = fAmount;
        else
          m_lastAxisMap[joystickName].erase(wKeyID);
      }

      return ProcessJoystickEvent(joystickName, wKeyID, isAxis, fAmount);
    }
    else
    {
      CKey key;
      if (wKeyID & ES_FLAG_UNICODE)
      {
        key = CKey((uint8_t)0, wKeyID & ~ES_FLAG_UNICODE, 0, 0, 0);
        return OnKey(key);
      }

      if(wKeyID == KEY_BUTTON_LEFT_ANALOG_TRIGGER)
        key = CKey(wKeyID, (BYTE)(255*fAmount), 0, 0.0, 0.0, 0.0, 0.0, frameTime);
      else if(wKeyID == KEY_BUTTON_RIGHT_ANALOG_TRIGGER)
        key = CKey(wKeyID, 0, (BYTE)(255*fAmount), 0.0, 0.0, 0.0, 0.0, frameTime);
      else if(wKeyID == KEY_BUTTON_LEFT_THUMB_STICK_LEFT)
        key = CKey(wKeyID, 0, 0, -fAmount, 0.0, 0.0, 0.0, frameTime);
      else if(wKeyID == KEY_BUTTON_LEFT_THUMB_STICK_RIGHT)
        key = CKey(wKeyID, 0, 0,  fAmount, 0.0, 0.0, 0.0, frameTime);
      else if(wKeyID == KEY_BUTTON_LEFT_THUMB_STICK_UP)
        key = CKey(wKeyID, 0, 0, 0.0,  fAmount, 0.0, 0.0, frameTime);
      else if(wKeyID == KEY_BUTTON_LEFT_THUMB_STICK_DOWN)
        key = CKey(wKeyID, 0, 0, 0.0, -fAmount, 0.0, 0.0, frameTime);
      else if(wKeyID == KEY_BUTTON_RIGHT_THUMB_STICK_LEFT)
        key = CKey(wKeyID, 0, 0, 0.0, 0.0, -fAmount, 0.0, frameTime);
      else if(wKeyID == KEY_BUTTON_RIGHT_THUMB_STICK_RIGHT)
        key = CKey(wKeyID, 0, 0, 0.0, 0.0,  fAmount, 0.0, frameTime);
      else if(wKeyID == KEY_BUTTON_RIGHT_THUMB_STICK_UP)
        key = CKey(wKeyID, 0, 0, 0.0, 0.0, 0.0,  fAmount, frameTime);
      else if(wKeyID == KEY_BUTTON_RIGHT_THUMB_STICK_DOWN)
        key = CKey(wKeyID, 0, 0, 0.0, 0.0, 0.0, -fAmount, frameTime);
      else
        key = CKey(wKeyID);
      key.SetFromService(true);
      return OnKey(key);
    }
  }

  if (m_lastAxisMap.size() > 0)
  {
    // Process all the stored axis.
    for (map<std::string, map<int, float> >::iterator iter = m_lastAxisMap.begin(); iter != m_lastAxisMap.end(); ++iter)
    {
      for (map<int, float>::iterator iterAxis = (*iter).second.begin(); iterAxis != (*iter).second.end(); ++iterAxis)
        ProcessJoystickEvent((*iter).first, (*iterAxis).first, true, (*iterAxis).second);
    }
  }

  {
    CPoint pos;
    if (es->GetMousePos(pos.x, pos.y) && g_Mouse.IsEnabled())
      return OnAction(CAction(ACTION_MOUSE_MOVE, pos.x, pos.y));
  }
#endif
  return false;
}

bool CApplication::ProcessJoystickEvent(const std::string& joystickName, int wKeyID, bool isAxis, float fAmount, unsigned int holdTime /*=0*/)
{
#if defined(HAS_EVENT_SERVER)
  m_idleTimer.StartZero();

   // Make sure to reset screen saver, mouse.
   ResetScreenSaver();
   if (WakeUpScreenSaverAndDPMS())
     return true;

#ifdef HAS_SDL_JOYSTICK
   g_Joystick.Reset();
#endif
   g_Mouse.SetActive(false);

   int iWin = GetActiveWindowID();
   int actionID;
   CStdString actionName;
   bool fullRange = false;

   // Translate using regular joystick translator.
   if (CButtonTranslator::GetInstance().TranslateJoystickString(iWin, joystickName.c_str(), wKeyID, isAxis ? JACTIVE_AXIS : JACTIVE_BUTTON, actionID, actionName, fullRange))
   {
     CAction action(actionID, fAmount, 0.0f, actionName, holdTime);
     bool bResult = false;

     // play sound before the action unless the button is held,
     // where we execute after the action as held actions aren't fired every time.
     if(action.GetHoldTime())
     {
       bResult = OnAction(action);
       if(bResult)
         g_audioManager.PlayActionSound(action);
     }
     else
     {
       g_audioManager.PlayActionSound(action);
       bResult = OnAction(action);
     }

     return bResult;
   }
   else
   {
     CLog::Log(LOGDEBUG, "ERROR mapping joystick action. Joystick: %s %i",joystickName.c_str(), wKeyID);
   }
#endif

   return false;
}

int CApplication::GetActiveWindowID(void)
{
  // Get the currently active window
  int iWin = g_windowManager.GetActiveWindow() & WINDOW_ID_MASK;

  // If there is a dialog active get the dialog id instead
  if (g_windowManager.HasModalDialog())
    iWin = g_windowManager.GetTopMostModalDialogID() & WINDOW_ID_MASK;

  // If the window is FullScreenVideo check if we're in a DVD menu
  if (iWin == WINDOW_FULLSCREEN_VIDEO && g_application.m_pPlayer && g_application.m_pPlayer->IsInMenu())
    iWin = WINDOW_VIDEO_MENU;

  // Return the window id
  return iWin;
}

bool CApplication::Cleanup()
{
  try
  {
    g_windowManager.Delete(WINDOW_MUSIC_PLAYLIST);
    g_windowManager.Delete(WINDOW_MUSIC_PLAYLIST_EDITOR);
    g_windowManager.Delete(WINDOW_MUSIC_FILES);
    g_windowManager.Delete(WINDOW_MUSIC_NAV);
    g_windowManager.Delete(WINDOW_DIALOG_MUSIC_INFO);
    g_windowManager.Delete(WINDOW_DIALOG_VIDEO_INFO);
    g_windowManager.Delete(WINDOW_VIDEO_FILES);
    g_windowManager.Delete(WINDOW_VIDEO_PLAYLIST);
    g_windowManager.Delete(WINDOW_VIDEO_NAV);
    g_windowManager.Delete(WINDOW_FILES);
    g_windowManager.Delete(WINDOW_DIALOG_YES_NO);
    g_windowManager.Delete(WINDOW_DIALOG_PROGRESS);
    g_windowManager.Delete(WINDOW_DIALOG_NUMERIC);
    g_windowManager.Delete(WINDOW_DIALOG_GAMEPAD);
    g_windowManager.Delete(WINDOW_DIALOG_SUB_MENU);
    g_windowManager.Delete(WINDOW_DIALOG_BUTTON_MENU);
    g_windowManager.Delete(WINDOW_DIALOG_CONTEXT_MENU);
    g_windowManager.Delete(WINDOW_DIALOG_PLAYER_CONTROLS);
    g_windowManager.Delete(WINDOW_DIALOG_KARAOKE_SONGSELECT);
    g_windowManager.Delete(WINDOW_DIALOG_KARAOKE_SELECTOR);
    g_windowManager.Delete(WINDOW_DIALOG_MUSIC_OSD);
    g_windowManager.Delete(WINDOW_DIALOG_VIS_PRESET_LIST);
    g_windowManager.Delete(WINDOW_DIALOG_SELECT);
    g_windowManager.Delete(WINDOW_DIALOG_OK);
    g_windowManager.Delete(WINDOW_DIALOG_FILESTACKING);
    g_windowManager.Delete(WINDOW_DIALOG_KEYBOARD);
    g_windowManager.Delete(WINDOW_FULLSCREEN_VIDEO);
    g_windowManager.Delete(WINDOW_DIALOG_PROFILE_SETTINGS);
    g_windowManager.Delete(WINDOW_DIALOG_LOCK_SETTINGS);
    g_windowManager.Delete(WINDOW_DIALOG_NETWORK_SETUP);
    g_windowManager.Delete(WINDOW_DIALOG_MEDIA_SOURCE);
    g_windowManager.Delete(WINDOW_DIALOG_VIDEO_OSD_SETTINGS);
    g_windowManager.Delete(WINDOW_DIALOG_AUDIO_OSD_SETTINGS);
    g_windowManager.Delete(WINDOW_DIALOG_VIDEO_BOOKMARKS);
    g_windowManager.Delete(WINDOW_DIALOG_CONTENT_SETTINGS);
    g_windowManager.Delete(WINDOW_DIALOG_FAVOURITES);
    g_windowManager.Delete(WINDOW_DIALOG_SONG_INFO);
    g_windowManager.Delete(WINDOW_DIALOG_SMART_PLAYLIST_EDITOR);
    g_windowManager.Delete(WINDOW_DIALOG_SMART_PLAYLIST_RULE);
    g_windowManager.Delete(WINDOW_DIALOG_BUSY);
    g_windowManager.Delete(WINDOW_DIALOG_PICTURE_INFO);
    g_windowManager.Delete(WINDOW_DIALOG_ADDON_INFO);
    g_windowManager.Delete(WINDOW_DIALOG_ADDON_SETTINGS);
    g_windowManager.Delete(WINDOW_DIALOG_ACCESS_POINTS);
    g_windowManager.Delete(WINDOW_DIALOG_SLIDER);
    g_windowManager.Delete(WINDOW_DIALOG_MEDIA_FILTER);

    /* Delete PVR related windows and dialogs */
    g_windowManager.Delete(WINDOW_PVR);
    g_windowManager.Delete(WINDOW_DIALOG_PVR_GUIDE_INFO);
    g_windowManager.Delete(WINDOW_DIALOG_PVR_RECORDING_INFO);
    g_windowManager.Delete(WINDOW_DIALOG_PVR_TIMER_SETTING);
    g_windowManager.Delete(WINDOW_DIALOG_PVR_GROUP_MANAGER);
    g_windowManager.Delete(WINDOW_DIALOG_PVR_CHANNEL_MANAGER);
    g_windowManager.Delete(WINDOW_DIALOG_PVR_GUIDE_SEARCH);
    g_windowManager.Delete(WINDOW_DIALOG_PVR_CHANNEL_SCAN);
    g_windowManager.Delete(WINDOW_DIALOG_PVR_UPDATE_PROGRESS);
    g_windowManager.Delete(WINDOW_DIALOG_PVR_OSD_CHANNELS);
    g_windowManager.Delete(WINDOW_DIALOG_PVR_OSD_GUIDE);
    g_windowManager.Delete(WINDOW_DIALOG_PVR_OSD_DIRECTOR);
    g_windowManager.Delete(WINDOW_DIALOG_PVR_OSD_CUTTER);
    g_windowManager.Delete(WINDOW_DIALOG_OSD_TELETEXT);

    g_windowManager.Delete(WINDOW_DIALOG_TEXT_VIEWER);
    g_windowManager.Delete(WINDOW_DIALOG_PLAY_EJECT);
    g_windowManager.Delete(WINDOW_STARTUP_ANIM);
    g_windowManager.Delete(WINDOW_LOGIN_SCREEN);
    g_windowManager.Delete(WINDOW_VISUALISATION);
    g_windowManager.Delete(WINDOW_KARAOKELYRICS);
    g_windowManager.Delete(WINDOW_SETTINGS_MENU);
    g_windowManager.Delete(WINDOW_SETTINGS_PROFILES);
    g_windowManager.Delete(WINDOW_SETTINGS_MYPICTURES);  // all the settings categories
    g_windowManager.Delete(WINDOW_TEST_PATTERN);
    g_windowManager.Delete(WINDOW_SCREEN_CALIBRATION);
    g_windowManager.Delete(WINDOW_SYSTEM_INFORMATION);
    g_windowManager.Delete(WINDOW_SCREENSAVER);
    g_windowManager.Delete(WINDOW_DIALOG_VIDEO_OSD);
    g_windowManager.Delete(WINDOW_DIALOG_MUSIC_OVERLAY);
    g_windowManager.Delete(WINDOW_DIALOG_VIDEO_OVERLAY);
    g_windowManager.Delete(WINDOW_SLIDESHOW);
    g_windowManager.Delete(WINDOW_ADDON_BROWSER);
    g_windowManager.Delete(WINDOW_SKIN_SETTINGS);

    g_windowManager.Delete(WINDOW_HOME);
    g_windowManager.Delete(WINDOW_PROGRAMS);
    g_windowManager.Delete(WINDOW_PICTURES);
    g_windowManager.Delete(WINDOW_WEATHER);

    g_windowManager.Delete(WINDOW_SETTINGS_MYPICTURES);
    g_windowManager.Remove(WINDOW_SETTINGS_MYPROGRAMS);
    g_windowManager.Remove(WINDOW_SETTINGS_MYWEATHER);
    g_windowManager.Remove(WINDOW_SETTINGS_MYMUSIC);
    g_windowManager.Remove(WINDOW_SETTINGS_SYSTEM);
    g_windowManager.Remove(WINDOW_SETTINGS_MYVIDEOS);
    g_windowManager.Remove(WINDOW_SETTINGS_SERVICE);
    g_windowManager.Remove(WINDOW_SETTINGS_APPEARANCE);
    g_windowManager.Remove(WINDOW_SETTINGS_MYPVR);
    g_windowManager.Remove(WINDOW_DIALOG_KAI_TOAST);

    g_windowManager.Remove(WINDOW_DIALOG_SEEK_BAR);
    g_windowManager.Remove(WINDOW_DIALOG_VOLUME_BAR);

    CAddonMgr::Get().DeInit();

#if defined(HAS_LIRC) || defined(HAS_IRSERVERSUITE)
    CLog::Log(LOGNOTICE, "closing down remote control service");
    g_RemoteControl.Disconnect();
#endif

    CLog::Log(LOGNOTICE, "unload sections");

#ifdef HAS_PERFORMANCE_SAMPLE
    CLog::Log(LOGNOTICE, "performance statistics");
    m_perfStats.DumpStats();
#endif

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
    CButtonTranslator::GetInstance().Clear();
    CLastfmScrobbler::RemoveInstance();
    CLibrefmScrobbler::RemoveInstance();
    CLastFmManager::RemoveInstance();
#ifdef HAS_EVENT_SERVER
    CEventServer::RemoveInstance();
#endif
    DllLoaderContainer::Clear();
    g_playlistPlayer.Clear();
    g_settings.Clear();
    g_guiSettings.Clear();
    g_advancedSettings.Clear();

#ifdef _LINUX
    CXHandle::DumpObjectTracker();
#endif
#if defined(TARGET_ANDROID)
    // enable for all platforms once it's safe
    g_sectionLoader.UnloadAll();
#endif
#ifdef _CRTDBG_MAP_ALLOC
    _CrtDumpMemoryLeaks();
    while(1); // execution ends
#endif
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
  try
  {
    CVariant vExitCode(exitCode);
    CAnnouncementManager::Announce(System, "xbmc", "OnQuit", vExitCode);

    SaveFileState(true);

    // cancel any jobs from the jobmanager
    CJobManager::GetInstance().CancelJobs();

    g_alarmClock.StopThread();

    if( m_bSystemScreenSaverEnable )
      g_Windowing.EnableSystemScreenSaver(true);

    CLog::Log(LOGNOTICE, "Storing total System Uptime");
    g_settings.m_iSystemTimeTotalUp = g_settings.m_iSystemTimeTotalUp + (int)(CTimeUtils::GetFrameTime() / 60000);

    // Update the settings information (volume, uptime etc. need saving)
    if (CFile::Exists(g_settings.GetSettingsFile()))
    {
      CLog::Log(LOGNOTICE, "Saving settings");
      g_settings.Save();
    }
    else
      CLog::Log(LOGNOTICE, "Not saving settings (settings.xml is not present)");

    m_bStop = true;
    m_AppActive = false;
    m_AppFocused = false;
    m_ExitCode = exitCode;
    CLog::Log(LOGNOTICE, "stop all");

    // stop scanning before we kill the network and so on
    if (m_musicInfoScanner->IsScanning())
      m_musicInfoScanner->Stop();

    if (m_videoInfoScanner->IsScanning())
      m_videoInfoScanner->Stop();

    CApplicationMessenger::Get().Cleanup();

    StopPVRManager();
    StopServices();
    //Sleep(5000);

#ifdef HAS_WEB_SERVER
  CWebServer::UnregisterRequestHandler(&m_httpImageHandler);
  CWebServer::UnregisterRequestHandler(&m_httpVfsHandler);
#ifdef HAS_JSONRPC
  CWebServer::UnregisterRequestHandler(&m_httpJsonRpcHandler);
#endif
#ifdef HAS_WEB_INTERFACE
  CWebServer::UnregisterRequestHandler(&m_httpWebinterfaceAddonsHandler);
  CWebServer::UnregisterRequestHandler(&m_httpWebinterfaceHandler);
#endif
#endif

    if (m_pPlayer)
    {
      CLog::Log(LOGNOTICE, "stop player");
      delete m_pPlayer;
      m_pPlayer = NULL;
    }

#if HAS_FILESYTEM_DAAP
    CLog::Log(LOGNOTICE, "stop daap clients");
    g_DaapClient.Release();
#endif
#ifdef HAS_FILESYSTEM_SAP
    CLog::Log(LOGNOTICE, "stop sap announcement listener");
    g_sapsessions.StopThread();
#endif
#ifdef HAS_ZEROCONF
    if(CZeroconfBrowser::IsInstantiated())
    {
      CLog::Log(LOGNOTICE, "stop zeroconf browser");
      CZeroconfBrowser::GetInstance()->Stop();
      CZeroconfBrowser::ReleaseInstance();
    }
#endif

    CLog::Log(LOGNOTICE, "clean cached files!");
#ifdef HAS_FILESYSTEM_RAR
    g_RarManager.ClearCache(true);
#endif

#ifdef HAS_FILESYSTEM_SFTP
    CSFTPSessionManager::DisconnectAllSessions();
#endif

    CLog::Log(LOGNOTICE, "unload skin");
    UnloadSkin();

#if defined(TARGET_DARWIN_OSX)
    if (XBMCHelper::GetInstance().IsAlwaysOn() == false)
      XBMCHelper::GetInstance().Stop();
#endif

#if defined(HAVE_LIBCRYSTALHD)
    CCrystalHD::RemoveInstance();
#endif

  g_mediaManager.Stop();

  // Stop services before unloading Python
  CAddonMgr::Get().StopServices(false);

/* Python resource freeing must be done after skin has been unloaded, not before
   some windows still need it when deinitializing during skin unloading. */
#ifdef HAS_PYTHON
  CLog::Log(LOGNOTICE, "stop python");
  g_pythonParser.FreeResources();
#endif
#ifdef HAS_LCD
    if (g_lcd)
    {
      g_lcd->Stop();
      delete g_lcd;
      g_lcd=NULL;
    }
#endif

    g_Windowing.DestroyRenderSystem();
    g_Windowing.DestroyWindow();
    g_Windowing.DestroyWindowSystem();

    // shutdown the AudioEngine
    CAEFactory::Shutdown();

    CLog::Log(LOGNOTICE, "stopped");
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "Exception in CApplication::Stop()");
  }

  // we may not get to finish the run cycle but exit immediately after a call to g_application.Stop()
  // so we may never get to Destroy() in CXBApplicationEx::Run(), we call it here.
  Destroy();

  //
  Sleep(200);
}

bool CApplication::PlayMedia(const CFileItem& item, int iPlaylist)
{
  //If item is a plugin, expand out now and run ourselves again
  if (item.IsPlugin())
  {
    CFileItem item_new(item);
    if (XFILE::CPluginDirectory::GetPluginResult(item.GetPath(), item_new))
      return PlayMedia(item_new, iPlaylist);
    return false;
  }
  if (item.IsLastFM())
  {
    g_partyModeManager.Disable();
    return CLastFmManager::GetInstance()->ChangeStation(item.GetAsUrl());
  }
  if (item.IsSmartPlayList())
  {
    CFileItemList items;
    CUtil::GetRecursiveListing(item.GetPath(), items, "");
    if (items.Size())
    {
      CSmartPlaylist smartpl;
      //get name and type of smartplaylist, this will always succeed as GetDirectory also did this.
      smartpl.OpenAndReadName(item.GetPath());
      CPlayList playlist;
      playlist.Add(items);
      return ProcessAndStartPlaylist(smartpl.GetName(), playlist, (smartpl.GetType() == "songs" || smartpl.GetType() == "albums") ? PLAYLIST_MUSIC:PLAYLIST_VIDEO);
    }
  }
  else if (item.IsPlayList() || item.IsInternetStream())
  {
    CGUIDialogCache* dlgCache = new CGUIDialogCache(5000, g_localizeStrings.Get(10214), item.GetLabel());

    //is or could be a playlist
    auto_ptr<CPlayList> pPlayList (CPlayListFactory::Create(item));
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
          return PlayFile(*(*pPlayList)[0], false);
      }
    }
  }

  //nothing special just play
  return PlayFile(item, false);
}

// PlayStack()
// For playing a multi-file video.  Particularly inefficient
// on startup, as we are required to calculate the length
// of each video, so we open + close each one in turn.
// A faster calculation of video time would improve this
// substantially.
bool CApplication::PlayStack(const CFileItem& item, bool bRestart)
{
  if (!item.IsStack())
    return false;

  CVideoDatabase dbs;

  // case 1: stacked ISOs
  if (CFileItem(CStackDirectory::GetFirstStackedFile(item.GetPath()),false).IsDVDImage())
  {
    CStackDirectory dir;
    CFileItemList movieList;
    dir.GetDirectory(item.GetPath(), movieList);

    // first assume values passed to the stack
    int selectedFile = item.m_lStartPartNumber;
    int startoffset = item.m_lStartOffset;

    // check if we instructed the stack to resume from default
    if (startoffset == STARTOFFSET_RESUME) // selected file is not specified, pick the 'last' resume point
    {
      if (dbs.Open())
      {
        CBookmark bookmark;
        if (dbs.GetResumeBookMark(item.GetPath(), bookmark))
        {
          startoffset = (int)(bookmark.timeInSeconds*75);
          selectedFile = bookmark.partNumber;
        }
        dbs.Close();
      }
      else
        CLog::Log(LOGERROR, "%s - Cannot open VideoDatabase", __FUNCTION__);
    }

    // make sure that the selected part is within the boundaries
    if (selectedFile <= 0)
    {
      CLog::Log(LOGWARNING, "%s - Selected part %d out of range, playing part 1", __FUNCTION__, selectedFile);
      selectedFile = 1;
    }
    else if (selectedFile > movieList.Size())
    {
      CLog::Log(LOGWARNING, "%s - Selected part %d out of range, playing part %d", __FUNCTION__, selectedFile, movieList.Size());
      selectedFile = movieList.Size();
    }

    // set startoffset in movieitem, track stack item for updating purposes, and finally play disc part
    movieList[selectedFile - 1]->m_lStartOffset = startoffset > 0 ? STARTOFFSET_RESUME : 0;
    movieList[selectedFile - 1]->SetProperty("stackFileItemToUpdate", true);
    *m_stackFileItemToUpdate = item;
    return PlayFile(*(movieList[selectedFile - 1]));
  }
  // case 2: all other stacks
  else
  {
    // see if we have the info in the database
    // TODO: If user changes the time speed (FPS via framerate conversion stuff)
    //       then these times will be wrong.
    //       Also, this is really just a hack for the slow load up times we have
    //       A much better solution is a fast reader of FPS and fileLength
    //       that we can use on a file to get it's time.
    vector<int> times;
    bool haveTimes(false);
    CVideoDatabase dbs;
    if (dbs.Open())
    {
      dbs.GetVideoSettings(item.GetPath(), g_settings.m_currentVideoSettings);
      haveTimes = dbs.GetStackTimes(item.GetPath(), times);
      dbs.Close();
    }


    // calculate the total time of the stack
    CStackDirectory dir;
    dir.GetDirectory(item.GetPath(), *m_currentStack);
    long totalTime = 0;
    for (int i = 0; i < m_currentStack->Size(); i++)
    {
      if (haveTimes)
        (*m_currentStack)[i]->m_lEndOffset = times[i];
      else
      {
        int duration;
        if (!CDVDFileInfo::GetFileDuration((*m_currentStack)[i]->GetPath(), duration))
        {
          m_currentStack->Clear();
          return false;
        }
        totalTime += duration / 1000;
        (*m_currentStack)[i]->m_lEndOffset = totalTime;
        times.push_back(totalTime);
      }
    }

    double seconds = item.m_lStartOffset / 75.0;

    if (!haveTimes || item.m_lStartOffset == STARTOFFSET_RESUME )
    {  // have our times now, so update the dB
      if (dbs.Open())
      {
        if( !haveTimes )
          dbs.SetStackTimes(item.GetPath(), times);

        if( item.m_lStartOffset == STARTOFFSET_RESUME )
        {
          // can only resume seek here, not dvdstate
          CBookmark bookmark;
          if( dbs.GetResumeBookMark(item.GetPath(), bookmark) )
            seconds = bookmark.timeInSeconds;
          else
            seconds = 0.0f;
        }
        dbs.Close();
      }
    }

    *m_itemCurrentFile = item;
    m_currentStackPosition = 0;
    m_eCurrentPlayer = EPC_NONE; // must be reset on initial play otherwise last player will be used

    if (seconds > 0)
    {
      // work out where to seek to
      for (int i = 0; i < m_currentStack->Size(); i++)
      {
        if (seconds < (*m_currentStack)[i]->m_lEndOffset)
        {
          CFileItem item(*(*m_currentStack)[i]);
          long start = (i > 0) ? (*m_currentStack)[i-1]->m_lEndOffset : 0;
          item.m_lStartOffset = (long)(seconds - start) * 75;
          m_currentStackPosition = i;
          return PlayFile(item, true);
        }
      }
    }

    return PlayFile(*(*m_currentStack)[0], true);
  }
  return false;
}

bool CApplication::PlayFile(const CFileItem& item, bool bRestart)
{
  if (!bRestart)
  {
    SaveCurrentFileSettings();

    OutputDebugString("new file set audiostream:0\n");
    // Switch to default options
    g_settings.m_currentVideoSettings = g_settings.m_defaultVideoSettings;
    // see if we have saved options in the database

    m_iPlaySpeed = 1;
    *m_itemCurrentFile = item;
    m_nextPlaylistItem = -1;
    m_currentStackPosition = 0;
    m_currentStack->Clear();

    if (item.IsVideo())
      CUtil::ClearSubtitles();
  }

  if (item.IsDiscStub())
  {
#ifdef HAS_DVD_DRIVE
    // Display the Play Eject dialog if there is any optical disc drive
    if (g_mediaManager.HasOpticalDrive())
    {
      if (CGUIDialogPlayEject::ShowAndGetInput(item))
        // PlayDiscAskResume takes path to disc. No parameter means default DVD drive.
        // Can't do better as CGUIDialogPlayEject calls CMediaManager::IsDiscInDrive, which assumes default DVD drive anyway
        return MEDIA_DETECT::CAutorun::PlayDiscAskResume();
    }
    else
#endif
      CGUIDialogOK::ShowAndGetInput(435, 0, 436, 0);

    return true;
  }

  if (item.IsPlayList())
    return false;

  if (item.IsPlugin())
  { // we modify the item so that it becomes a real URL
    CFileItem item_new(item);
    if (XFILE::CPluginDirectory::GetPluginResult(item.GetPath(), item_new))
      return PlayFile(item_new, false);
    return false;
  }

#ifdef HAS_UPNP
  if (URIUtils::IsUPnP(item.GetPath()))
  {
    CFileItem item_new(item);
    if (XFILE::CUPnPDirectory::GetResource(item.GetPath(), item_new))
      return PlayFile(item_new, false);
    return false;
  }
#endif

  // if we have a stacked set of files, we need to setup our stack routines for
  // "seamless" seeking and total time of the movie etc.
  // will recall with restart set to true
  if (item.IsStack())
    return PlayStack(item, bRestart);

  //Is TuxBox, this should probably be moved to CTuxBoxFile
  if(item.IsTuxBox())
  {
    CLog::Log(LOGDEBUG, "%s - TuxBox URL Detected %s",__FUNCTION__, item.GetPath().c_str());

    if(g_tuxboxService.IsRunning())
      g_tuxboxService.Stop();

    CFileItem item_new;
    if(g_tuxbox.CreateNewItem(item, item_new))
    {

      // Make sure it doesn't have a player
      // so we actually select one normally
      m_eCurrentPlayer = EPC_NONE;

      // keep the tuxbox:// url as playing url
      // and give the new url to the player
      if(PlayFile(item_new, true))
      {
        if(!g_tuxboxService.IsRunning())
          g_tuxboxService.Start();
        return true;
      }
    }
    return false;
  }

  CPlayerOptions options;

  if( item.HasProperty("StartPercent") )
  {
    double fallback = 0.0f;
    if(item.GetProperty("StartPercent").isString())
      fallback = (double)atof(item.GetProperty("StartPercent").asString().c_str());
    options.startpercent = item.GetProperty("StartPercent").asDouble(fallback);
  }

  PLAYERCOREID eNewCore = EPC_NONE;
  if( bRestart )
  {
    // have to be set here due to playstack using this for starting the file
    options.starttime = item.m_lStartOffset / 75.0;
    if (m_itemCurrentFile->IsStack() && m_currentStack->Size() > 0 && m_itemCurrentFile->m_lStartOffset != 0)
      m_itemCurrentFile->m_lStartOffset = STARTOFFSET_RESUME; // to force fullscreen switching

    if( m_eForcedNextPlayer != EPC_NONE )
      eNewCore = m_eForcedNextPlayer;
    else if( m_eCurrentPlayer == EPC_NONE )
      eNewCore = CPlayerCoreFactory::GetDefaultPlayer(item);
    else
      eNewCore = m_eCurrentPlayer;
  }
  else
  {
    options.starttime = item.m_lStartOffset / 75.0;

    if (item.IsVideo())
    {
      // open the d/b and retrieve the bookmarks for the current movie
      CVideoDatabase dbs;
      dbs.Open();
      dbs.GetVideoSettings(item.GetPath(), g_settings.m_currentVideoSettings);

      if( item.m_lStartOffset == STARTOFFSET_RESUME )
      {
        options.starttime = 0.0f;
        CBookmark bookmark;
        CStdString path = item.GetPath();
        if (item.HasVideoInfoTag() && item.GetVideoInfoTag()->m_strFileNameAndPath.Find("removable://") == 0)
          path = item.GetVideoInfoTag()->m_strFileNameAndPath;
        else if (item.HasProperty("original_listitem_url") && URIUtils::IsPlugin(item.GetProperty("original_listitem_url").asString()))
          path = item.GetProperty("original_listitem_url").asString();
        if(dbs.GetResumeBookMark(path, bookmark))
        {
          options.starttime = bookmark.timeInSeconds;
          options.state = bookmark.playerState;
        }
        /*
         override with information from the actual item if available.  We do this as the VFS (eg plugins)
         may set the resume point to override whatever XBMC has stored, yet we ignore it until now so that,
         should the playerState be required, it is fetched from the database.
         See the note in CGUIWindowVideoBase::ShowResumeMenu.
         */
        if (item.HasVideoInfoTag() && item.GetVideoInfoTag()->m_resumePoint.IsSet())
          options.starttime = item.GetVideoInfoTag()->m_resumePoint.timeInSeconds;
      }
      else if (item.HasVideoInfoTag())
      {
        const CVideoInfoTag *tag = item.GetVideoInfoTag();

        if (tag->m_iBookmarkId != -1 && tag->m_iBookmarkId != 0)
        {
          CBookmark bookmark;
          dbs.GetBookMarkForEpisode(*tag, bookmark);
          options.starttime = bookmark.timeInSeconds;
          options.state = bookmark.playerState;
        }
      }

      dbs.Close();
    }

    if (m_eForcedNextPlayer != EPC_NONE)
      eNewCore = m_eForcedNextPlayer;
    else
      eNewCore = CPlayerCoreFactory::GetDefaultPlayer(item);
  }

  // this really aught to be inside !bRestart, but since PlayStack
  // uses that to init playback, we have to keep it outside
  int playlist = g_playlistPlayer.GetCurrentPlaylist();
  if (item.IsVideo() && g_playlistPlayer.GetPlaylist(playlist).size() > 1)
  { // playing from a playlist by the looks
    // don't switch to fullscreen if we are not playing the first item...
    options.fullscreen = !g_playlistPlayer.HasPlayedFirstFile() && g_advancedSettings.m_fullScreenOnMovieStart && !g_settings.m_bStartVideoWindowed;
  }
  else if(m_itemCurrentFile->IsStack() && m_currentStack->Size() > 0)
  {
    // TODO - this will fail if user seeks back to first file in stack
    if(m_currentStackPosition == 0 || m_itemCurrentFile->m_lStartOffset == STARTOFFSET_RESUME)
      options.fullscreen = g_advancedSettings.m_fullScreenOnMovieStart && !g_settings.m_bStartVideoWindowed;
    else
      options.fullscreen = false;
    // reset this so we don't think we are resuming on seek
    m_itemCurrentFile->m_lStartOffset = 0;
  }
  else
    options.fullscreen = g_advancedSettings.m_fullScreenOnMovieStart && !g_settings.m_bStartVideoWindowed;

  // reset m_bStartVideoWindowed as it's a temp setting
  g_settings.m_bStartVideoWindowed = false;
  // reset any forced player
  m_eForcedNextPlayer = EPC_NONE;

#ifdef HAS_KARAOKE
  //We have to stop parsing a cdg before mplayer is deallocated
  // WHY do we have to do this????
  if (m_pKaraokeMgr)
    m_pKaraokeMgr->Stop();
#endif

  // tell system we are starting a file
  m_bPlaybackStarting = true;

  // We should restart the player, unless the previous and next tracks are using
  // one of the players that allows gapless playback (paplayer, dvdplayer)
  if (m_pPlayer)
  {
    if ( !(m_eCurrentPlayer == eNewCore && (m_eCurrentPlayer == EPC_DVDPLAYER || m_eCurrentPlayer  == EPC_PAPLAYER
#if defined(HAS_OMXPLAYER)
            || m_eCurrentPlayer == EPC_OMXPLAYER
#endif            
            )) )
    {
      delete m_pPlayer;
      m_pPlayer = NULL;
    }
  }

  if (!m_pPlayer)
  {
    m_eCurrentPlayer = eNewCore;
    m_pPlayer = CPlayerCoreFactory::CreatePlayer(eNewCore, *this);
  }

  bool bResult;
  if (m_pPlayer)
  {
    // don't hold graphicscontext here since player
    // may wait on another thread, that requires gfx
    CSingleExit ex(g_graphicsContext);
    bResult = m_pPlayer->OpenFile(item, options);
  }
  else
  {
    CLog::Log(LOGERROR, "Error creating player for item %s (File doesn't exist?)", item.GetPath().c_str());
    bResult = false;
  }

  if(bResult)
  {
    if (m_iPlaySpeed != 1)
    {
      int iSpeed = m_iPlaySpeed;
      m_iPlaySpeed = 1;
      SetPlaySpeed(iSpeed);
    }

    if( IsPlayingAudio() )
    {
      if (g_windowManager.GetActiveWindow() == WINDOW_FULLSCREEN_VIDEO)
        g_windowManager.ActivateWindow(WINDOW_VISUALISATION);
    }

#ifdef HAS_VIDEO_PLAYBACK
    if( IsPlayingVideo() )
    {
      if (g_windowManager.GetActiveWindow() == WINDOW_VISUALISATION)
        g_windowManager.ActivateWindow(WINDOW_FULLSCREEN_VIDEO);

      // if player didn't manange to switch to fullscreen by itself do it here
      if( options.fullscreen && g_renderManager.IsStarted()
       && g_windowManager.GetActiveWindow() != WINDOW_FULLSCREEN_VIDEO )
       SwitchToFullScreen();

      if (!item.IsDVDImage() && !item.IsDVDFile())
      {
        CVideoInfoTag *details = m_itemCurrentFile->GetVideoInfoTag();
        // Save information about the stream if we currently have no data
        if (!details->HasStreamDetails() ||
             details->m_streamDetails.GetVideoDuration() <= 0)
        {
          if (m_pPlayer->GetStreamDetails(details->m_streamDetails) && details->HasStreamDetails())
          {
            CVideoDatabase dbs;
            dbs.Open();
            dbs.SetStreamDetailsForFileId(details->m_streamDetails, details->m_iFileId);
            dbs.Close();
            CUtil::DeleteVideoDatabaseDirectoryCache();
          }
        }
      }
    }
#endif

#if !defined(TARGET_DARWIN) && !defined(_LINUX)
    g_audioManager.Enable(false);
#endif

    if (item.HasPVRChannelInfoTag())
      g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_NONE);
  }
  m_bPlaybackStarting = false;

  if (bResult)
  {
    // we must have started, otherwise player might send this later
    if(IsPlaying())
      OnPlayBackStarted();
    else
      OnPlayBackEnded();
  }
  else
  {
    // we send this if it isn't playlistplayer that is doing this
    int next = g_playlistPlayer.GetNextSong();
    int size = g_playlistPlayer.GetPlaylist(g_playlistPlayer.GetCurrentPlaylist()).size();
    if(next < 0
    || next >= size)
      OnPlayBackStopped();
  }

  return bResult;
}

void CApplication::OnPlayBackEnded()
{
  if(m_bPlaybackStarting)
    return;

  if (CJobManager::GetInstance().IsPaused(kJobTypeMediaFlags))
    CJobManager::GetInstance().UnPause(kJobTypeMediaFlags);

  // informs python script currently running playback has ended
  // (does nothing if python is not loaded)
#ifdef HAS_PYTHON
  g_pythonParser.OnPlayBackEnded();
#endif

  CVariant data(CVariant::VariantTypeObject);
  data["end"] = true;
  CAnnouncementManager::Announce(Player, "xbmc", "OnStop", m_itemCurrentFile, data);

  if (IsPlayingAudio())
  {
    CLastfmScrobbler::GetInstance()->SubmitQueue();
    CLibrefmScrobbler::GetInstance()->SubmitQueue();
  }

  CGUIMessage msg(GUI_MSG_PLAYBACK_ENDED, 0, 0);
  g_windowManager.SendThreadMessage(msg);
}

void CApplication::OnPlayBackStarted()
{
  if(m_bPlaybackStarting)
    return;

  if (!CJobManager::GetInstance().IsPaused(kJobTypeMediaFlags))
    CJobManager::GetInstance().Pause(kJobTypeMediaFlags);

#ifdef HAS_PYTHON
  // informs python script currently running playback has started
  // (does nothing if python is not loaded)
  g_pythonParser.OnPlayBackStarted();
#endif

  CGUIMessage msg(GUI_MSG_PLAYBACK_STARTED, 0, 0);
  g_windowManager.SendThreadMessage(msg);
}

void CApplication::OnQueueNextItem()
{
  // informs python script currently running that we are requesting the next track
  // (does nothing if python is not loaded)
#ifdef HAS_PYTHON
  g_pythonParser.OnQueueNextItem(); // currently unimplemented
#endif

  if(IsPlayingAudio())
  {
    CLastfmScrobbler::GetInstance()->SubmitQueue();
    CLibrefmScrobbler::GetInstance()->SubmitQueue();
  }

  CGUIMessage msg(GUI_MSG_QUEUE_NEXT_ITEM, 0, 0);
  g_windowManager.SendThreadMessage(msg);
}

void CApplication::OnPlayBackStopped()
{
  if(m_bPlaybackStarting)
    return;

  if (CJobManager::GetInstance().IsPaused(kJobTypeMediaFlags))
    CJobManager::GetInstance().UnPause(kJobTypeMediaFlags);

  // informs python script currently running playback has ended
  // (does nothing if python is not loaded)
#ifdef HAS_PYTHON
  g_pythonParser.OnPlayBackStopped();
#endif

  CVariant data(CVariant::VariantTypeObject);
  data["end"] = false;
  CAnnouncementManager::Announce(Player, "xbmc", "OnStop", m_itemCurrentFile, data);

  CLastfmScrobbler::GetInstance()->SubmitQueue();
  CLibrefmScrobbler::GetInstance()->SubmitQueue();

  CGUIMessage msg( GUI_MSG_PLAYBACK_STOPPED, 0, 0 );
  g_windowManager.SendThreadMessage(msg);
}

void CApplication::OnPlayBackPaused()
{
#ifdef HAS_PYTHON
  g_pythonParser.OnPlayBackPaused();
#endif

  CVariant param;
  param["player"]["speed"] = 0;
  param["player"]["playerid"] = g_playlistPlayer.GetCurrentPlaylist();
  CAnnouncementManager::Announce(Player, "xbmc", "OnPause", m_itemCurrentFile, param);
}

void CApplication::OnPlayBackResumed()
{
#ifdef HAS_PYTHON
  g_pythonParser.OnPlayBackResumed();
#endif

  CVariant param;
  param["player"]["speed"] = 1;
  param["player"]["playerid"] = g_playlistPlayer.GetCurrentPlaylist();
  CAnnouncementManager::Announce(Player, "xbmc", "OnPlay", m_itemCurrentFile, param);
}

void CApplication::OnPlayBackSpeedChanged(int iSpeed)
{
#ifdef HAS_PYTHON
  g_pythonParser.OnPlayBackSpeedChanged(iSpeed);
#endif

  CVariant param;
  param["player"]["speed"] = iSpeed;
  param["player"]["playerid"] = g_playlistPlayer.GetCurrentPlaylist();
  CAnnouncementManager::Announce(Player, "xbmc", "OnSpeedChanged", m_itemCurrentFile, param);
}

void CApplication::OnPlayBackSeek(int iTime, int seekOffset)
{
#ifdef HAS_PYTHON
  g_pythonParser.OnPlayBackSeek(iTime, seekOffset);
#endif

  CVariant param;
  CJSONUtils::MillisecondsToTimeObject(iTime, param["player"]["time"]);
  CJSONUtils::MillisecondsToTimeObject(seekOffset, param["player"]["seekoffset"]);;
  param["player"]["playerid"] = g_playlistPlayer.GetCurrentPlaylist();
  param["player"]["speed"] = GetPlaySpeed();
  CAnnouncementManager::Announce(Player, "xbmc", "OnSeek", m_itemCurrentFile, param);
  g_infoManager.SetDisplayAfterSeek(2500, seekOffset/1000);
}

void CApplication::OnPlayBackSeekChapter(int iChapter)
{
#ifdef HAS_PYTHON
  g_pythonParser.OnPlayBackSeekChapter(iChapter);
#endif
}

bool CApplication::IsPlaying() const
{
  if (!m_pPlayer)
    return false;
  if (!m_pPlayer->IsPlaying())
    return false;
  return true;
}

bool CApplication::IsPaused() const
{
  if (!m_pPlayer)
    return false;
  if (!m_pPlayer->IsPlaying())
    return false;
  return m_pPlayer->IsPaused();
}

bool CApplication::IsPlayingAudio() const
{
  if (!m_pPlayer)
    return false;
  if (!m_pPlayer->IsPlaying())
    return false;
  if (m_pPlayer->HasVideo())
    return false;
  if (m_pPlayer->HasAudio())
    return true;
  return false;
}

bool CApplication::IsPlayingVideo() const
{
  if (!m_pPlayer)
    return false;
  if (!m_pPlayer->IsPlaying())
    return false;
  if (m_pPlayer->HasVideo())
    return true;

  return false;
}

bool CApplication::IsPlayingFullScreenVideo() const
{
  return IsPlayingVideo() && g_graphicsContext.IsFullScreenVideo();
}

bool CApplication::IsFullScreen()
{
  return IsPlayingFullScreenVideo() ||
        (g_windowManager.GetActiveWindow() == WINDOW_VISUALISATION) ||
         g_windowManager.GetActiveWindow() == WINDOW_SLIDESHOW;
}

void CApplication::SaveFileState(bool bForeground /* = false */)
{
  if (m_progressTrackingItem->IsPVRChannel() || !g_settings.GetCurrentProfile().canWriteDatabases())
    return;

  if (bForeground)
  {
    CSaveFileStateJob job(*m_progressTrackingItem,
    *m_stackFileItemToUpdate,
    m_progressTrackingVideoResumeBookmark,
    m_progressTrackingPlayCountUpdate);

    // Run job in the foreground to make sure it finishes
    job.DoWork();
  }
  else
  {
    CJob* job = new CSaveFileStateJob(*m_progressTrackingItem,
        *m_stackFileItemToUpdate,
        m_progressTrackingVideoResumeBookmark,
        m_progressTrackingPlayCountUpdate);
    CJobManager::GetInstance().AddJob(job, NULL);
  }
}

void CApplication::UpdateFileState()
{
  // Did the file change?
  if (m_progressTrackingItem->GetPath() != "" && m_progressTrackingItem->GetPath() != CurrentFile())
  {
    SaveFileState();

    // Reset tracking item
    m_progressTrackingItem->Reset();
  }
  else
  {
    if (IsPlayingVideo() || IsPlayingAudio())
    {
      if (m_progressTrackingItem->GetPath() == "")
      {
        // Init some stuff
        *m_progressTrackingItem = CurrentFileItem();
        m_progressTrackingPlayCountUpdate = false;
      }

      if ((m_progressTrackingItem->IsAudio() && g_advancedSettings.m_audioPlayCountMinimumPercent > 0 &&
          GetPercentage() >= g_advancedSettings.m_audioPlayCountMinimumPercent) ||
          (m_progressTrackingItem->IsVideo() && g_advancedSettings.m_videoPlayCountMinimumPercent > 0 &&
          GetPercentage() >= g_advancedSettings.m_videoPlayCountMinimumPercent))
      {
        m_progressTrackingPlayCountUpdate = true;
      }

      if (m_progressTrackingItem->IsVideo())
      {
        if ((m_progressTrackingItem->IsDVDImage() || m_progressTrackingItem->IsDVDFile()) && m_pPlayer->GetTotalTime() > 15*60*1000)
        {
          m_progressTrackingItem->GetVideoInfoTag()->m_streamDetails.Reset();
          m_pPlayer->GetStreamDetails(m_progressTrackingItem->GetVideoInfoTag()->m_streamDetails);
        }
        // Update bookmark for save
        m_progressTrackingVideoResumeBookmark.player = CPlayerCoreFactory::GetPlayerName(m_eCurrentPlayer);
        m_progressTrackingVideoResumeBookmark.playerState = m_pPlayer->GetPlayerState();
        m_progressTrackingVideoResumeBookmark.thumbNailImage.Empty();

        if (g_advancedSettings.m_videoIgnorePercentAtEnd > 0 &&
            GetTotalTime() - GetTime() < 0.01f * g_advancedSettings.m_videoIgnorePercentAtEnd * GetTotalTime())
        {
          // Delete the bookmark
          m_progressTrackingVideoResumeBookmark.timeInSeconds = -1.0f;
        }
        else
        if (GetTime() > g_advancedSettings.m_videoIgnoreSecondsAtStart)
        {
          // Update the bookmark
          m_progressTrackingVideoResumeBookmark.timeInSeconds = GetTime();
          m_progressTrackingVideoResumeBookmark.totalTimeInSeconds = GetTotalTime();
        }
        else
        {
          // Do nothing
          m_progressTrackingVideoResumeBookmark.timeInSeconds = 0.0f;
        }
      }
    }
  }
}

void CApplication::StopPlaying()
{
  int iWin = g_windowManager.GetActiveWindow();
  if ( IsPlaying() )
  {
#ifdef HAS_KARAOKE
    if( m_pKaraokeMgr )
      m_pKaraokeMgr->Stop();
#endif

    if (g_PVRManager.IsPlayingTV() || g_PVRManager.IsPlayingRadio())
      g_PVRManager.SaveCurrentChannelSettings();

    if (m_pPlayer)
      m_pPlayer->CloseFile();

    // turn off visualisation window when stopping
    if (iWin == WINDOW_VISUALISATION
    ||  iWin == WINDOW_FULLSCREEN_VIDEO)
      g_windowManager.PreviousWindow();

    g_partyModeManager.Disable();
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
  if ((!m_bScreenSave && m_iScreenSaveLock == 0) && !m_dpmsIsActive)
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
  if (manual || (m_dpmsIsManual == manual))
  {
    if (m_dpmsIsActive)
    {
      m_dpmsIsActive = false;
      m_dpmsIsManual = false;
      return m_dpms->DisablePowerSaving();
    }
    else
    {
      if (m_dpms->EnablePowerSaving(m_dpms->GetSupportedModes()[0]))
      {
        m_dpmsIsActive = true;
        m_dpmsIsManual = manual;
        return true;
      }
    }
  }
  return false;
}

bool CApplication::WakeUpScreenSaverAndDPMS(bool bPowerOffKeyPressed /* = false */)
{

#ifdef HAS_LCD
    // turn on lcd backlight
    if (g_lcd && g_advancedSettings.m_lcdDimOnScreenSave)
      g_lcd->SetBackLight(1);
#endif

  // First reset DPMS, if active
  if (m_dpmsIsActive)
  {
    if (m_dpmsIsManual)
      return false;
    // TODO: if screensaver lock is specified but screensaver is not active
    // (DPMS came first), activate screensaver now.
    ToggleDPMS(false);
    ResetScreenSaverTimer();
    return !m_bScreenSave || WakeUpScreenSaver(bPowerOffKeyPressed);
  }
  else
    return WakeUpScreenSaver(bPowerOffKeyPressed);
}

bool CApplication::WakeUpScreenSaver(bool bPowerOffKeyPressed /* = false */)
{
  if (m_iScreenSaveLock == 2)
    return false;

  // if Screen saver is active
  if (m_bScreenSave && m_screenSaver)
  {
    if (m_iScreenSaveLock == 0)
      if (g_settings.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE &&
          (g_settings.UsingLoginScreen() || g_guiSettings.GetBool("masterlock.startuplock")) &&
          g_settings.GetCurrentProfile().getLockMode() != LOCK_MODE_EVERYONE &&
          m_screenSaver->ID() != "screensaver.xbmc.builtin.dim" && m_screenSaver->ID() != "screensaver.xbmc.builtin.black" && m_screenSaver->ID() != "visualization")
      {
        m_iScreenSaveLock = 2;
        CGUIMessage msg(GUI_MSG_CHECK_LOCK,0,0);

        CGUIWindow* pWindow = g_windowManager.GetWindow(WINDOW_SCREENSAVER);
        if (pWindow)
          pWindow->OnMessage(msg);
      }
    if (m_iScreenSaveLock == -1)
    {
      m_iScreenSaveLock = 0;
      return true;
    }

    // disable screensaver
    m_bScreenSave = false;
    m_iScreenSaveLock = 0;
    ResetScreenSaverTimer();

    // allow listeners to ignore the deactivation if it preceeds a powerdown/suspend etc
    CVariant data(bPowerOffKeyPressed);
    CAnnouncementManager::Announce(GUI, "xbmc", "OnScreensaverDeactivated", data);

    if (m_screenSaver->ID() == "visualization")
    {
      // we can just continue as usual from vis mode
      return false;
    }
    else if (m_screenSaver->ID() == "screensaver.xbmc.builtin.dim" || m_screenSaver->ID() == "screensaver.xbmc.builtin.black")
      return true;
    else if (!m_screenSaver->ID().IsEmpty())
    { // we're in screensaver window
      if (g_windowManager.GetActiveWindow() == WINDOW_SCREENSAVER)
        g_windowManager.PreviousWindow();  // show the previous window
      if (g_windowManager.GetActiveWindow() == WINDOW_SLIDESHOW)
        CApplicationMessenger::Get().SendAction(CAction(ACTION_STOP), WINDOW_SLIDESHOW);
    }
    return true;
  }
  else
    return false;
}

void CApplication::CheckScreenSaverAndDPMS()
{
  if (!m_dpmsIsActive)
    g_Windowing.ResetOSScreensaver();

  bool maybeScreensaver =
      !m_dpmsIsActive && !m_bScreenSave
      && !g_guiSettings.GetString("screensaver.mode").IsEmpty();
  bool maybeDPMS =
      !m_dpmsIsActive && m_dpms->IsSupported()
      && g_guiSettings.GetInt("powermanagement.displaysoff") > 0;

  // Has the screen saver window become active?
  if (maybeScreensaver && g_windowManager.IsWindowActive(WINDOW_SCREENSAVER))
  {
    m_bScreenSave = true;
    maybeScreensaver = false;
  }

  if (m_bScreenSave && IsPlayingVideo() && !m_pPlayer->IsPaused())
  {
    WakeUpScreenSaverAndDPMS();
    return;
  }

  if (!maybeScreensaver && !maybeDPMS) return;  // Nothing to do.

  // See if we need to reset timer.
  // * Are we playing a video and it is not paused?
  if ((IsPlayingVideo() && !m_pPlayer->IsPaused())
      // * Are we playing some music in fullscreen vis?
      || (IsPlayingAudio() && g_windowManager.GetActiveWindow() == WINDOW_VISUALISATION
          && !g_guiSettings.GetString("musicplayer.visualisation").IsEmpty()))
  {
    ResetScreenSaverTimer();
    return;
  }

  float elapsed = m_screenSaverTimer.GetElapsedSeconds();

  // DPMS has priority (it makes the screensaver not needed)
  if (maybeDPMS
      && elapsed > g_guiSettings.GetInt("powermanagement.displaysoff") * 60)
  {
    ToggleDPMS(false);
    WakeUpScreenSaver();
  }
  else if (maybeScreensaver
           && elapsed > g_guiSettings.GetInt("screensaver.time") * 60)
  {
    ActivateScreenSaver();
  }
}

// activate the screensaver.
// if forceType is true, we ignore the various conditions that can alter
// the type of screensaver displayed
void CApplication::ActivateScreenSaver(bool forceType /*= false */)
{
  m_bScreenSave = true;

  // Get Screensaver Mode
  m_screenSaver.reset();
  if (!CAddonMgr::Get().GetAddon(g_guiSettings.GetString("screensaver.mode"), m_screenSaver))
    m_screenSaver.reset(new CScreenSaver(""));

#ifdef HAS_LCD
  // turn off lcd backlight if requested
  if (g_lcd && g_advancedSettings.m_lcdDimOnScreenSave)
    g_lcd->SetBackLight(0);
#endif

  CAnnouncementManager::Announce(GUI, "xbmc", "OnScreensaverActivated");

  // disable screensaver lock from the login screen
  m_iScreenSaveLock = g_windowManager.GetActiveWindow() == WINDOW_LOGIN_SCREEN ? 1 : 0;
  if (!forceType)
  {
    // set to Dim in the case of a dialog on screen or playing video
    if (g_windowManager.HasModalDialog() || (IsPlayingVideo() && g_guiSettings.GetBool("screensaver.usedimonpause")) || g_PVRManager.IsRunningChannelScan())
    {
      if (!CAddonMgr::Get().GetAddon("screensaver.xbmc.builtin.dim", m_screenSaver))
        m_screenSaver.reset(new CScreenSaver(""));
    }
    // Check if we are Playing Audio and Vis instead Screensaver!
    else if (IsPlayingAudio() && g_guiSettings.GetBool("screensaver.usemusicvisinstead") && !g_guiSettings.GetString("musicplayer.visualisation").IsEmpty())
    { // activate the visualisation
      m_screenSaver.reset(new CScreenSaver("visualization"));
      g_windowManager.ActivateWindow(WINDOW_VISUALISATION);
      return;
    }
  }
  // Picture slideshow
  if (m_screenSaver->ID() == "screensaver.xbmc.builtin.slideshow")
  {
    // reset our codec info - don't want that on screen
    g_infoManager.SetShowCodec(false);
    CStdString type = m_screenSaver->GetSetting("type");
    CStdString path = m_screenSaver->GetSetting("path");
    if (type == "2" && path.IsEmpty())
      type = "0";
    if (type == "0")
      path = "special://profile/Thumbnails/Video/Fanart";
    if (type == "1")
      path = "special://profile/Thumbnails/Music/Fanart";
    CApplicationMessenger::Get().PictureSlideShow(path, true, type != "2");
  }
  else if (m_screenSaver->ID() == "screensaver.xbmc.builtin.dim")
    return;
  else if (m_screenSaver->ID() == "screensaver.xbmc.builtin.black")
    return;
  else if (!m_screenSaver->ID().IsEmpty())
    g_windowManager.ActivateWindow(WINDOW_SCREENSAVER);
}

void CApplication::CheckShutdown()
{
  // first check if we should reset the timer
  bool resetTimer = m_bInhibitIdleShutdown;

  if (IsPlaying() || IsPaused()) // is something playing?
    resetTimer = true;

  if (m_musicInfoScanner->IsScanning())
    resetTimer = true;

  if (m_videoInfoScanner->IsScanning())
    resetTimer = true;

  if (g_windowManager.IsWindowActive(WINDOW_DIALOG_PROGRESS)) // progress dialog is onscreen
    resetTimer = true;

  if (g_guiSettings.GetBool("pvrmanager.enabled") &&  !g_PVRManager.IsIdle())
    resetTimer = true;

  if (resetTimer)
  {
    m_shutdownTimer.StartZero();
    return;
  }

  if ( m_shutdownTimer.GetElapsedSeconds() > g_guiSettings.GetInt("powermanagement.shutdowntime") * 60 )
  {
    // Since it is a sleep instead of a shutdown, let's set everything to reset when we wake up.
    m_shutdownTimer.Stop();

    // Sleep the box
    CApplicationMessenger::Get().Shutdown();
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
        int nRemoved = g_playlistPlayer.RemoveDVDItems();
        if ( nRemoved > 0 )
        {
          CGUIMessage msg( GUI_MSG_PLAYLIST_CHANGED, 0, 0 );
          g_windowManager.SendMessage( msg );
        }
        // stop the file if it's on dvd (will set the resume point etc)
        if (m_itemCurrentFile->IsOnDVD())
          StopPlaying();
      }
    }
    break;

  case GUI_MSG_PLAYBACK_STARTED:
    {
#ifdef TARGET_DARWIN
      DarwinSetScheduling(message.GetMessage());
#endif
      // reset the seek handler
      m_seekHandler->Reset();

      // Update our infoManager with the new details etc.
      if (m_nextPlaylistItem >= 0)
      { // we've started a previously queued item
        CFileItemPtr item = g_playlistPlayer.GetPlaylist(g_playlistPlayer.GetCurrentPlaylist())[m_nextPlaylistItem];
        // update the playlist manager
        int currentSong = g_playlistPlayer.GetCurrentSong();
        int param = ((currentSong & 0xffff) << 16) | (m_nextPlaylistItem & 0xffff);
        CGUIMessage msg(GUI_MSG_PLAYLISTPLAYER_CHANGED, 0, 0, g_playlistPlayer.GetCurrentPlaylist(), param, item);
        g_windowManager.SendThreadMessage(msg);
        g_playlistPlayer.SetCurrentSong(m_nextPlaylistItem);
        *m_itemCurrentFile = *item;
      }
      g_infoManager.SetCurrentItem(*m_itemCurrentFile);
      CLastFmManager::GetInstance()->OnSongChange(*m_itemCurrentFile);
      g_partyModeManager.OnSongChange(true);

      CVariant param;
      param["player"]["speed"] = 1;
      param["player"]["playerid"] = g_playlistPlayer.GetCurrentPlaylist();
      CAnnouncementManager::Announce(Player, "xbmc", "OnPlay", m_itemCurrentFile, param);

      DimLCDOnPlayback(true);

      if (IsPlayingAudio())
      {
        // Start our cdg parser as appropriate
#ifdef HAS_KARAOKE
        if (m_pKaraokeMgr && g_guiSettings.GetBool("karaoke.enabled") && !m_itemCurrentFile->IsInternetStream())
        {
          m_pKaraokeMgr->Stop();
          if (m_itemCurrentFile->IsMusicDb())
          {
            if (!m_itemCurrentFile->HasMusicInfoTag() || !m_itemCurrentFile->GetMusicInfoTag()->Loaded())
            {
              IMusicInfoTagLoader* tagloader = CMusicInfoTagLoaderFactory::CreateLoader(m_itemCurrentFile->GetPath());
              tagloader->Load(m_itemCurrentFile->GetPath(),*m_itemCurrentFile->GetMusicInfoTag());
              delete tagloader;
            }
            m_pKaraokeMgr->Start(m_itemCurrentFile->GetMusicInfoTag()->GetURL());
          }
          else
            m_pKaraokeMgr->Start(m_itemCurrentFile->GetPath());
        }
#endif
        // Let scrobbler know about the track
        const CMusicInfoTag* tag=g_infoManager.GetCurrentSongTag();
        if (tag)
        {
          CLastfmScrobbler::GetInstance()->AddSong(*tag, CLastFmManager::GetInstance()->IsRadioEnabled());
          CLibrefmScrobbler::GetInstance()->AddSong(*tag, CLastFmManager::GetInstance()->IsRadioEnabled());
        }
      }

      return true;
    }
    break;

  case GUI_MSG_QUEUE_NEXT_ITEM:
    {
      // Check to see if our playlist player has a new item for us,
      // and if so, we check whether our current player wants the file
      int iNext = g_playlistPlayer.GetNextSong();
      CPlayList& playlist = g_playlistPlayer.GetPlaylist(g_playlistPlayer.GetCurrentPlaylist());
      if (iNext < 0 || iNext >= playlist.size())
      {
        if (m_pPlayer) m_pPlayer->OnNothingToQueueNotify();
        return true; // nothing to do
      }
      // ok, grab the next song
      CFileItemPtr item = playlist[iNext];

#ifdef HAS_UPNP
      if (URIUtils::IsUPnP(item->GetPath()))
      {
        if (!XFILE::CUPnPDirectory::GetResource(item->GetPath(), *item))
          return true;
      }
#endif

      // ok - send the file to the player if it wants it
      if (m_pPlayer && m_pPlayer->QueueNextFile(*item))
      { // player wants the next file
        m_nextPlaylistItem = iNext;
      }
      return true;
    }
    break;

  case GUI_MSG_PLAYBACK_STOPPED:
  case GUI_MSG_PLAYBACK_ENDED:
  case GUI_MSG_PLAYLISTPLAYER_STOPPED:
    {
#ifdef HAS_KARAOKE
      if (m_pKaraokeMgr )
        m_pKaraokeMgr->Stop();
#endif
#ifdef TARGET_DARWIN
      DarwinSetScheduling(message.GetMessage());
#endif
      // first check if we still have items in the stack to play
      if (message.GetMessage() == GUI_MSG_PLAYBACK_ENDED)
      {
        if (m_itemCurrentFile->IsStack() && m_currentStack->Size() > 0 && m_currentStackPosition < m_currentStack->Size() - 1)
        { // just play the next item in the stack
          PlayFile(*(*m_currentStack)[++m_currentStackPosition], true);
          return true;
        }
      }

      // In case playback ended due to user eg. skipping over the end, clear
      // our resume bookmark here
      if (message.GetMessage() == GUI_MSG_PLAYBACK_ENDED && m_progressTrackingPlayCountUpdate && g_advancedSettings.m_videoIgnorePercentAtEnd > 0)
      {
        // Delete the bookmark
        m_progressTrackingVideoResumeBookmark.timeInSeconds = -1.0f;
      }

      // reset the current playing file
      m_itemCurrentFile->Reset();
      g_infoManager.ResetCurrentItem();
      m_currentStack->Clear();

      if (message.GetMessage() == GUI_MSG_PLAYBACK_ENDED)
      {
        g_playlistPlayer.PlayNext(1, true);
      }
      else
      {
        // stop lastfm
        if (CLastFmManager::GetInstance()->IsRadioEnabled())
          CLastFmManager::GetInstance()->StopRadio();

        delete m_pPlayer;
        m_pPlayer = 0;

        // Reset playspeed
        m_iPlaySpeed = 1;
      }

      if (!IsPlaying())
      {
        g_audioManager.Enable(true);
        DimLCDOnPlayback(false);
      }

      if (!IsPlayingVideo())
      {
        if(g_windowManager.GetActiveWindow() == WINDOW_FULLSCREEN_VIDEO)
        {
          g_windowManager.PreviousWindow();
        }
        else
        {
          CSingleLock lock(g_graphicsContext);
          //  resets to res_desktop or look&feel resolution (including refreshrate)
          g_graphicsContext.SetFullScreenVideo(false);
        }
      }

      if (!IsPlayingAudio() && g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_NONE && g_windowManager.GetActiveWindow() == WINDOW_VISUALISATION)
      {
        g_settings.Save();  // save vis settings
        WakeUpScreenSaverAndDPMS();
        g_windowManager.PreviousWindow();
      }

      // DVD ejected while playing in vis ?
      if (!IsPlayingAudio() && (m_itemCurrentFile->IsCDDA() || m_itemCurrentFile->IsOnDVD()) && !g_mediaManager.IsDiscInDrive() && g_windowManager.GetActiveWindow() == WINDOW_VISUALISATION)
      {
        // yes, disable vis
        g_settings.Save();    // save vis settings
        WakeUpScreenSaverAndDPMS();
        g_windowManager.PreviousWindow();
      }

      if (IsEnableTestMode())
        CApplicationMessenger::Get().Quit();
      return true;
    }
    break;

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
      return ExecuteXBMCAction(message.GetStringParam());
    break;
  }
  return false;
}

bool CApplication::ExecuteXBMCAction(std::string actionStr)
{
  // see if it is a user set string
  CLog::Log(LOGDEBUG,"%s : Translating %s", __FUNCTION__, actionStr.c_str());
  CGUIInfoLabel info(actionStr, "");
  actionStr = info.GetLabel(0);
  CLog::Log(LOGDEBUG,"%s : To %s", __FUNCTION__, actionStr.c_str());

  // user has asked for something to be executed
  if (CBuiltins::HasCommand(actionStr))
    CBuiltins::Execute(actionStr);
  else
  {
    // try translating the action from our ButtonTranslator
    int actionID;
    if (CButtonTranslator::TranslateActionString(actionStr.c_str(), actionID))
    {
      OnAction(CAction(actionID));
      return true;
    }
    CFileItem item(actionStr, false);
#ifdef HAS_PYTHON
    if (item.IsPythonScript())
    { // a python script
      g_pythonParser.evalFile(item.GetPath().c_str(),ADDON::AddonPtr());
    }
    else
#endif
    if (item.IsAudio() || item.IsVideo())
    { // an audio or video file
      PlayFile(item);
    }
    else
      return false;
  }
  return true;
}

void CApplication::Process()
{
  MEASURE_FUNCTION;

  // dispatch the messages generated by python or other threads to the current window
  g_windowManager.DispatchThreadMessages();

  // process messages which have to be send to the gui
  // (this can only be done after g_windowManager.Render())
  CApplicationMessenger::Get().ProcessWindowMessages();

#ifdef HAS_PYTHON
  // process any Python scripts
  g_pythonParser.Process();
#endif

  // process messages, even if a movie is playing
  CApplicationMessenger::Get().ProcessMessages();
  if (g_application.m_bStop) return; //we're done, everything has been unloaded

  // check how far we are through playing the current item
  // and do anything that needs doing (lastfm submission, playcount updates etc)
  CheckPlayingProgress();

  // update sound
  if (m_pPlayer)
    m_pPlayer->DoAudioWork();

  // do any processing that isn't needed on each run
  if( m_slowTimer.GetElapsedMilliseconds() > 500 )
  {
    m_slowTimer.Reset();
    ProcessSlow();
  }

  g_cpuInfo.getUsedPercentage(); // must call it to recalculate pct values
}

// We get called every 500ms
void CApplication::ProcessSlow()
{
  g_powerManager.ProcessEvents();

#if defined(TARGET_DARWIN_OSX)
  // There is an issue on OS X that several system services ask the cursor to become visible
  // during their startup routines.  Given that we can't control this, we hack it in by
  // forcing the
  if (g_Windowing.IsFullScreen())
  { // SDL thinks it's hidden
    Cocoa_HideMouse();
  }
#endif

  // Store our file state for use on close()
  UpdateFileState();

  if (IsPlayingAudio())
  {
    CLastfmScrobbler::GetInstance()->UpdateStatus();
    CLibrefmScrobbler::GetInstance()->UpdateStatus();
  }

  // Check if we need to activate the screensaver / DPMS.
  CheckScreenSaverAndDPMS();

  // Check if we need to shutdown (if enabled).
#if defined(TARGET_DARWIN)
  if (g_guiSettings.GetInt("powermanagement.shutdowntime") && g_advancedSettings.m_fullScreen)
#else
  if (g_guiSettings.GetInt("powermanagement.shutdowntime"))
#endif
  {
    CheckShutdown();
  }

  // check if we should restart the player
  CheckDelayedPlayerRestart();

  //  check if we can unload any unreferenced dlls or sections
  if (!IsPlayingVideo())
    CSectionLoader::UnloadDelayed();

  // check for any idle curl connections
  g_curlInterface.CheckIdle();

  // check for any idle myth sessions
  CMythSession::CheckIdle();

#ifdef HAS_FILESYSTEM_HTSP
  // check for any idle htsp sessions
  HTSP::CHTSPDirectorySession::CheckIdle();
#endif

#ifdef HAS_KARAOKE
  if ( m_pKaraokeMgr )
    m_pKaraokeMgr->ProcessSlow();
#endif

  // LED - LCD SwitchOn On Paused! m_bIsPaused=TRUE -> LED/LCD is ON!
  if(IsPaused() != m_bIsPaused)
  {
#ifdef HAS_LCD
    DimLCDOnPlayback(m_bIsPaused);
#endif
    m_bIsPaused = IsPaused();
  }

  if (!IsPlayingVideo())
    g_largeTextureManager.CleanupUnusedImages();

#ifdef HAS_DVD_DRIVE
  // checks whats in the DVD drive and tries to autostart the content (xbox games, dvd, cdda, avi files...)
  if (!IsPlayingVideo())
    m_Autorun->HandleAutorun();
#endif

  // update upnp server/renderer states
#ifdef HAS_UPNP
  if(UPNP::CUPnP::IsInstantiated())
    UPNP::CUPnP::GetInstance()->UpdateState();
#endif

#if defined(_LINUX) && defined(HAS_FILESYSTEM_SMB)
  smb.CheckIfIdle();
#endif

#ifdef HAS_FILESYSTEM_NFS
  gNfsConnection.CheckIfIdle();
#endif

#ifdef HAS_FILESYSTEM_AFP
  gAfpConnection.CheckIfIdle();
#endif

#ifdef HAS_FILESYSTEM_SFTP
  CSFTPSessionManager::ClearOutIdleSessions();
#endif

  g_mediaManager.ProcessEvents();

#ifdef HAS_LIRC
  if (g_RemoteControl.IsInUse() && !g_RemoteControl.IsInitialized())
    g_RemoteControl.Initialize();
#endif

#ifdef HAS_LCD
  // attempt to reinitialize the LCD (e.g. after resuming from sleep)
  if (!IsPlayingVideo())
  {
    if (g_lcd && !g_lcd->IsConnected())
    {
      g_lcd->Stop();
      g_lcd->Initialize();
    }
  }
#endif

  if (!IsPlayingVideo())
    CAddonInstaller::Get().UpdateRepos();

  CAEFactory::GarbageCollect();
}

// Global Idle Time in Seconds
// idle time will be resetet if on any OnKey()
// int return: system Idle time in seconds! 0 is no idle!
int CApplication::GlobalIdleTime()
{
  if(!m_idleTimer.IsRunning())
  {
    m_idleTimer.Stop();
    m_idleTimer.StartZero();
  }
  return (int)m_idleTimer.GetElapsedSeconds();
}

float CApplication::NavigationIdleTime()
{
  if (!m_navigationTimer.IsRunning())
  {
    m_navigationTimer.Stop();
    m_navigationTimer.StartZero();
  }
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
  if ( !IsPlayingVideo() && !IsPlayingAudio())
    return ;

  if( !m_pPlayer )
    return ;

  SaveFileState();

  // do we want to return to the current position in the file
  if (false == bSamePosition)
  {
    // no, then just reopen the file and start at the beginning
    PlayFile(*m_itemCurrentFile, true);
    return ;
  }

  // else get current position
  double time = GetTime();

  // get player state, needed for dvd's
  CStdString state = m_pPlayer->GetPlayerState();

  // set the requested starttime
  m_itemCurrentFile->m_lStartOffset = (long)(time * 75.0);

  // reopen the file
  if ( PlayFile(*m_itemCurrentFile, true) && m_pPlayer )
    m_pPlayer->SetPlayerState(state);
}

const CStdString& CApplication::CurrentFile()
{
  return m_itemCurrentFile->GetPath();
}

CFileItem& CApplication::CurrentFileItem()
{
  return *m_itemCurrentFile;
}

void CApplication::ShowVolumeBar(const CAction *action)
{
  CGUIDialog *volumeBar = (CGUIDialog *)g_windowManager.GetWindow(WINDOW_DIALOG_VOLUME_BAR);
  if (volumeBar)
  {
    volumeBar->Show();
    if (action)
      volumeBar->OnAction(*action);
  }
}

bool CApplication::IsMuted() const
{
  if (g_peripherals.IsMuted())
    return true;
  return CAEFactory::IsMuted();
}

void CApplication::ToggleMute(void)
{
  if (g_settings.m_bMute)
    UnMute();
  else
    Mute();
}

void CApplication::Mute()
{
  if (g_peripherals.Mute())
    return;

  CAEFactory::SetMute(true);
  g_settings.m_bMute = true;
  VolumeChanged();
}

void CApplication::UnMute()
{
  if (g_peripherals.UnMute())
    return;

  CAEFactory::SetMute(false);
  g_settings.m_bMute = false;
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
  g_settings.m_fVolumeLevel = hardwareVolume;

  float value = 0.0f;
  if (hardwareVolume > VOLUME_MINIMUM)
  {
    float dB = CAEUtil::PercentToGain(hardwareVolume);
    value = CAEUtil::GainToScale(dB);
  }
  if (value >= 0.99f)
    value = 1.0f;

  CAEFactory::SetVolume(value);

  /* for platforms where we do not have AE */
  if (m_pPlayer)
    m_pPlayer->SetVolume(g_settings.m_fVolumeLevel);
}

int CApplication::GetVolume() const
{
  // converts the hardware volume to a percentage
  return (int)(g_settings.m_fVolumeLevel * 100.0f);
}

void CApplication::VolumeChanged() const
{
  CVariant data(CVariant::VariantTypeObject);
  data["volume"] = GetVolume();
  data["muted"] = g_settings.m_bMute;
  CAnnouncementManager::Announce(Application, "xbmc", "OnVolumeChanged", data);
}

int CApplication::GetSubtitleDelay() const
{
  // converts subtitle delay to a percentage
  return int(((float)(g_settings.m_currentVideoSettings.m_SubtitleDelay + g_advancedSettings.m_videoSubsDelayRange)) / (2 * g_advancedSettings.m_videoSubsDelayRange)*100.0f + 0.5f);
}

int CApplication::GetAudioDelay() const
{
  // converts subtitle delay to a percentage
  return int(((float)(g_settings.m_currentVideoSettings.m_AudioDelay + g_advancedSettings.m_videoAudioDelayRange)) / (2 * g_advancedSettings.m_videoAudioDelayRange)*100.0f + 0.5f);
}

void CApplication::SetPlaySpeed(int iSpeed)
{
  if (!IsPlayingAudio() && !IsPlayingVideo())
    return ;
  if (m_iPlaySpeed == iSpeed)
    return ;
  if (!m_pPlayer->CanSeek())
    return;
  if (m_pPlayer->IsPaused())
  {
    if (
      ((m_iPlaySpeed > 1) && (iSpeed > m_iPlaySpeed)) ||
      ((m_iPlaySpeed < -1) && (iSpeed < m_iPlaySpeed))
    )
    {
      iSpeed = m_iPlaySpeed; // from pause to ff/rw, do previous ff/rw speed
    }
    m_pPlayer->Pause();
  }
  m_iPlaySpeed = iSpeed;

  m_pPlayer->ToFFRW(m_iPlaySpeed);
  if (m_iPlaySpeed == 1)
  { // restore volume
    m_pPlayer->SetVolume(VOLUME_MAXIMUM);
  }
  else
  { // mute volume
    m_pPlayer->SetVolume(VOLUME_MINIMUM);
  }
}

int CApplication::GetPlaySpeed() const
{
  return m_iPlaySpeed;
}

// Returns the total time in seconds of the current media.  Fractional
// portions of a second are possible - but not necessarily supported by the
// player class.  This returns a double to be consistent with GetTime() and
// SeekTime().
double CApplication::GetTotalTime() const
{
  double rc = 0.0;

  if (IsPlaying() && m_pPlayer)
  {
    if (m_itemCurrentFile->IsStack() && m_currentStack->Size() > 0)
      rc = (*m_currentStack)[m_currentStack->Size() - 1]->m_lEndOffset;
    else
      rc = static_cast<double>(m_pPlayer->GetTotalTime() * 0.001f);
  }

  return rc;
}

void CApplication::StopShutdownTimer()
{
  if (m_shutdownTimer.IsRunning())
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

  if (IsPlaying() && m_pPlayer)
  {
    if (m_itemCurrentFile->IsStack() && m_currentStack->Size() > 0)
    {
      long startOfCurrentFile = (m_currentStackPosition > 0) ? (*m_currentStack)[m_currentStackPosition-1]->m_lEndOffset : 0;
      rc = (double)startOfCurrentFile + m_pPlayer->GetTime() * 0.001;
    }
    else
      rc = static_cast<double>(m_pPlayer->GetTime() * 0.001f);
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
  if (IsPlaying() && m_pPlayer && (dTime >= 0.0))
  {
    if (!m_pPlayer->CanSeek()) return;
    if (m_itemCurrentFile->IsStack() && m_currentStack->Size() > 0)
    {
      // find the item in the stack we are seeking to, and load the new
      // file if necessary, and calculate the correct seek within the new
      // file.  Otherwise, just fall through to the usual routine if the
      // time is higher than our total time.
      for (int i = 0; i < m_currentStack->Size(); i++)
      {
        if ((*m_currentStack)[i]->m_lEndOffset > dTime)
        {
          long startOfNewFile = (i > 0) ? (*m_currentStack)[i-1]->m_lEndOffset : 0;
          if (m_currentStackPosition == i)
            m_pPlayer->SeekTime((int64_t)((dTime - startOfNewFile) * 1000.0));
          else
          { // seeking to a new file
            m_currentStackPosition = i;
            CFileItem item(*(*m_currentStack)[i]);
            item.m_lStartOffset = (long)((dTime - startOfNewFile) * 75.0);
            // don't just call "PlayFile" here, as we are quite likely called from the
            // player thread, so we won't be able to delete ourselves.
            CApplicationMessenger::Get().PlayFile(item, true);
          }
          return;
        }
      }
    }
    // convert to milliseconds and perform seek
    m_pPlayer->SeekTime( static_cast<int64_t>( dTime * 1000.0 ) );
  }
}

float CApplication::GetPercentage() const
{
  if (IsPlaying() && m_pPlayer)
  {
    if (m_pPlayer->GetTotalTime() == 0 && IsPlayingAudio() && m_itemCurrentFile->HasMusicInfoTag())
    {
      const CMusicInfoTag& tag = *m_itemCurrentFile->GetMusicInfoTag();
      if (tag.GetDuration() > 0)
        return (float)(GetTime() / tag.GetDuration() * 100);
    }

    if (m_itemCurrentFile->IsStack() && m_currentStack->Size() > 0)
    {
      double totalTime = GetTotalTime();
      if (totalTime > 0.0f)
        return (float)(GetTime() / totalTime * 100);
    }
    else
      return m_pPlayer->GetPercentage();
  }
  return 0.0f;
}

float CApplication::GetCachePercentage() const
{
  if (IsPlaying() && m_pPlayer)
  {
    // Note that the player returns a relative cache percentage and we want an absolute percentage
    if (m_itemCurrentFile->IsStack() && m_currentStack->Size() > 0)
    {
      float stackedTotalTime = (float) GetTotalTime();
      // We need to take into account the stack's total time vs. currently playing file's total time
      if (stackedTotalTime > 0.0f)
        return min( 100.0f, GetPercentage() + (m_pPlayer->GetCachePercentage() * m_pPlayer->GetTotalTime() * 0.001f / stackedTotalTime ) );
    }
    else
      return min( 100.0f, m_pPlayer->GetPercentage() + m_pPlayer->GetCachePercentage() );
  }
  return 0.0f;
}

void CApplication::SeekPercentage(float percent)
{
  if (IsPlaying() && m_pPlayer && (percent >= 0.0))
  {
    if (!m_pPlayer->CanSeek()) return;
    if (m_itemCurrentFile->IsStack() && m_currentStack->Size() > 0)
      SeekTime(percent * 0.01 * GetTotalTime());
    else
      m_pPlayer->SeekPercentage(percent);
  }
}

// SwitchToFullScreen() returns true if a switch is made, else returns false
bool CApplication::SwitchToFullScreen()
{
  // if playing from the video info window, close it first!
  if (g_windowManager.HasModalDialog() && g_windowManager.GetTopMostModalDialogID() == WINDOW_DIALOG_VIDEO_INFO)
  {
    CGUIDialogVideoInfo* pDialog = (CGUIDialogVideoInfo*)g_windowManager.GetWindow(WINDOW_DIALOG_VIDEO_INFO);
    if (pDialog) pDialog->Close(true);
  }

  // don't switch if there is a dialog on screen or the slideshow is active
  if (/*g_windowManager.HasModalDialog() ||*/ g_windowManager.GetActiveWindow() == WINDOW_SLIDESHOW)
    return false;

  // See if we're playing a video, and are in GUI mode
  if ( IsPlayingVideo() && g_windowManager.GetActiveWindow() != WINDOW_FULLSCREEN_VIDEO)
  {
    // Reset frame count so that timing is FPS will be correct.
    {
      CSingleLock lock(m_frameMutex);
      m_frameCount = 0;
    }

    // then switch to fullscreen mode
    g_windowManager.ActivateWindow(WINDOW_FULLSCREEN_VIDEO);
    return true;
  }
  // special case for switching between GUI & visualisation mode. (only if we're playing an audio song)
  if (IsPlayingAudio() && g_windowManager.GetActiveWindow() != WINDOW_VISUALISATION)
  { // then switch to visualisation
    g_windowManager.ActivateWindow(WINDOW_VISUALISATION);
    return true;
  }
  return false;
}

void CApplication::Minimize()
{
  g_Windowing.Minimize();
}

PLAYERCOREID CApplication::GetCurrentPlayer()
{
  return m_eCurrentPlayer;
}

void CApplication::UpdateLibraries()
{
  if (g_guiSettings.GetBool("videolibrary.updateonstartup"))
  {
    CLog::Log(LOGNOTICE, "%s - Starting video library startup scan", __FUNCTION__);
    StartVideoScan("");
  }

  if (g_guiSettings.GetBool("musiclibrary.updateonstartup"))
  {
    CLog::Log(LOGNOTICE, "%s - Starting music library startup scan", __FUNCTION__);
    StartMusicScan("");
  }
}

bool CApplication::IsVideoScanning() const
{
  return m_videoInfoScanner->IsScanning();
}

bool CApplication::IsMusicScanning() const
{
  return m_musicInfoScanner->IsScanning();
}

void CApplication::StopVideoScan()
{
  if (m_videoInfoScanner->IsScanning())
    m_videoInfoScanner->Stop();
}

void CApplication::StopMusicScan()
{
  if (m_musicInfoScanner->IsScanning())
    m_musicInfoScanner->Stop();
}

void CApplication::StartVideoCleanup()
{
  if (m_videoInfoScanner->IsScanning())
    return;

  m_videoInfoScanner->CleanDatabase();
}

void CApplication::StartVideoScan(const CStdString &strDirectory, bool scanAll)
{
  if (m_videoInfoScanner->IsScanning())
    return;

  m_videoInfoScanner->ShowDialog(true);

  m_videoInfoScanner->Start(strDirectory,scanAll);
}

void CApplication::StartMusicScan(const CStdString &strDirectory, int flags)
{
  if (m_musicInfoScanner->IsScanning())
    return;

  if (!flags)
  { // setup default flags
    if (g_guiSettings.GetBool("musiclibrary.downloadinfo"))
      flags |= CMusicInfoScanner::SCAN_ONLINE;
    if (g_guiSettings.GetBool("musiclibrary.backgroundupdate"))
      flags |= CMusicInfoScanner::SCAN_BACKGROUND;
  }

  if (!(flags & CMusicInfoScanner::SCAN_BACKGROUND))
    m_musicInfoScanner->ShowDialog(true);

  m_musicInfoScanner->Start(strDirectory, flags);
}

void CApplication::StartMusicAlbumScan(const CStdString& strDirectory,
                                       bool refresh)
{
  if (m_musicInfoScanner->IsScanning())
    return;

  m_musicInfoScanner->ShowDialog(true);

  m_musicInfoScanner->FetchAlbumInfo(strDirectory,refresh);
}

void CApplication::StartMusicArtistScan(const CStdString& strDirectory,
                                        bool refresh)
{
  if (m_musicInfoScanner->IsScanning())
    return;

  m_musicInfoScanner->ShowDialog(true);

  m_musicInfoScanner->FetchArtistInfo(strDirectory,refresh);
}

void CApplication::CheckPlayingProgress()
{
  // check if we haven't rewound past the start of the file
  if (IsPlaying())
  {
    int iSpeed = g_application.GetPlaySpeed();
    if (iSpeed < 1)
    {
      iSpeed *= -1;
      int iPower = 0;
      while (iSpeed != 1)
      {
        iSpeed >>= 1;
        iPower++;
      }
      if (g_infoManager.GetPlayTime() / 1000 < iPower)
      {
        g_application.SetPlaySpeed(1);
        g_application.SeekTime(0);
      }
    }
  }
}

bool CApplication::ProcessAndStartPlaylist(const CStdString& strPlayList, CPlayList& playlist, int iPlaylist, int track)
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
  g_playlistPlayer.ClearPlaylist(iPlaylist);

  // if the playlist contains an internet stream, this file will be used
  // to generate a thumbnail for musicplayer.cover
  g_application.m_strPlayListFile = strPlayList;

  // add the items to the playlist player
  g_playlistPlayer.Add(iPlaylist, playlist);

  // if we have a playlist
  if (g_playlistPlayer.GetPlaylist(iPlaylist).size())
  {
    // start playing it
    g_playlistPlayer.SetCurrentPlaylist(iPlaylist);
    g_playlistPlayer.Reset();
    g_playlistPlayer.Play(track);
    return true;
  }
  return false;
}

void CApplication::SaveCurrentFileSettings()
{
  // don't store settings for PVR in video database
  if (m_itemCurrentFile->IsVideo() && !m_itemCurrentFile->IsPVRChannel())
  {
    // save video settings
    if (g_settings.m_currentVideoSettings != g_settings.m_defaultVideoSettings)
    {
      CVideoDatabase dbs;
      dbs.Open();
      dbs.SetVideoSettings(m_itemCurrentFile->GetPath(), g_settings.m_currentVideoSettings);
      dbs.Close();
    }
  }
  else if (m_itemCurrentFile->IsPVRChannel())
  {
    g_PVRManager.SaveCurrentChannelSettings();
  }
}

bool CApplication::AlwaysProcess(const CAction& action)
{
  // check if this button is mapped to a built-in function
  if (!action.GetName().IsEmpty())
  {
    CStdString builtInFunction;
    vector<CStdString> params;
    CUtil::SplitExecFunction(action.GetName(), builtInFunction, params);
    builtInFunction.ToLower();

    // should this button be handled normally or just cancel the screensaver?
    if (   builtInFunction.Equals("powerdown")
        || builtInFunction.Equals("reboot")
        || builtInFunction.Equals("restart")
        || builtInFunction.Equals("restartapp")
        || builtInFunction.Equals("suspend")
        || builtInFunction.Equals("hibernate")
        || builtInFunction.Equals("quit")
        || builtInFunction.Equals("shutdown"))
    {
      return true;
    }
  }

  return false;
}

bool CApplication::IsCurrentThread() const
{
  return CThread::IsCurrentThread(m_threadID);
}

bool CApplication::IsPresentFrame()
{
  CSingleLock lock(m_frameMutex);
  bool ret = m_bPresentFrame;

  return ret;
}

void CApplication::SetRenderGUI(bool renderGUI)
{
  if (renderGUI && ! m_renderGUI)
    g_windowManager.MarkDirty();
  m_renderGUI = renderGUI;
}

#if defined(HAS_LINUX_NETWORK)
CNetworkLinux& CApplication::getNetwork()
{
  return m_network;
}
#elif defined(HAS_WIN32_NETWORK)
CNetworkWin32& CApplication::getNetwork()
{
  return m_network;
}
#else
CNetwork& CApplication::getNetwork()
{
  return m_network;
}

#endif
#ifdef HAS_PERFORMANCE_SAMPLE
CPerformanceStats &CApplication::GetPerformanceStats()
{
  return m_perfStats;
}
#endif

