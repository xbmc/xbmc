#include "stdafx.h"
#include "xbox/XKEEPROM.h"
#include "application.h"
#include "utils/lcd.h"
#include "xbox\iosupport.h"
#include "xbox/xbeheader.h"
#include "util.h"
#include "texturemanager.h"
#include "cores/playercorefactory.h"
#include "playlistplayer.h"
#include "musicdatabase.h"
#include "autorun.h"
#include "ActionManager.h"
#include "utils/LCDFactory.h"
#include "utils/KaiClient.h"
#include "utils/MemoryUnitManager.h"
#include "utils/FanController.h"
#include "XBVideoConfig.h"
#include "utils/LED.h"
#include "LangCodeExpander.h"
#include "lib/libGoAhead/xbmchttp.h"
#include "utils/GUIInfoManager.h"
#include "PlaylistFactory.h"
#include "GUIFontManager.h"
#include "SkinInfo.h"
#include "lib/libPython/XBPython.h"
#include "ButtonTranslator.h"
#include "GUIAudioManager.h"
#include "lib/libscrobbler/scrobbler.h"
#include "GUIPassword.h"
#include "../guilib/localizestrings.h"
#include "../guilib/guiwindowmanager.h"
#include "applicationmessenger.h"
#include "sectionloader.h"
#include "utils/charsetconverter.h"
#include "guiusermessages.h"
#include "filesystem/directoryCache.h"
#include "cores/DllLoader/DllLoaderContainer.h"
#include "filesystem/filedaap.h"
#include "filesystem/StackDirectory.h"
#include "PartyModeManager.h"
#include "FileSystem/DllLibCurl.h"
#include "audiocontext.h"
#include "GUIFontTTF.h"
#include "xbox/network.h"
#include "utils/win32exception.h"
#include "cores/videorenderers/rendermanager.h"
#include "FileSystem/UPnPDirectory.h"
#include "lib/libGoAhead/webserver.h"
#include "lib/libfilezilla/xbfilezilla.h"
#include "CdgParser.h"
#include "utils/sntp.h"

// Windows includes
#include "GUIStandardWindow.h"
#include "GUIWindowMusicPlaylist.h"
#include "GUIWindowMusicSongs.h"
//#include "GUIWindowMusicTop100.h"
#include "GUIWindowMusicNav.h"
#include "GUIWindowVideoPlaylist.h"
#include "GUIWindowMusicInfo.h"
#include "GUIWindowVideoInfo.h"
#include "GUIWindowFullScreen.h"
#include "GUIWindowVideoFiles.h"
#include "GUIWindowVideoGenre.h"
#include "GUIWindowVideoActors.h"
#include "GUIWindowVideoYear.h"
#include "GUIWindowVideoTitle.h"
#include "GUIWindowFileManager.h"
#include "GUIWindowVisualisation.h"
#include "GUIWindowSettings.h"
#include "GUIWindowSettingsProfile.h"
#include "GUIWindowSettingsCategory.h"
#include "GUIWindowSettingsScreenCalibration.h"
#include "GUIWindowSystemInfo.h"
#include "GUIWindowScreensaver.h"
#include "GUIWindowSlideshow.h"
#include "GUIWindowHome.h"
#include "GUIWindowPrograms.h"
#include "GUIWindowPictures.h"
#include "GUIWindowScripts.h"
#include "GUIWindowBuddies.h"
#include "GUIWindowWeather.h"
#include "GUIWindowLoginScreen.h"

// Dialog includes
#include "GUIDialogInvite.h"
#include "GUIDialogHost.h"
#include "GUIDialogKeyboard.h"
#include "GUIDialogYesNo.h"
#include "GUIDialogOK.h"
#include "GUIDialogProgress.h"
#include "GUIDialogSelect.h"
#include "GUIDialogFileStacking.h"
#include "GUIDialogNumeric.h"
#include "GUIDialogGamepad.h"
#include "GUIDialogSubMenu.h"
#include "GUIDialogButtonMenu.h"
#include "GUIDialogContextMenu.h"
#include "GUIDialogMusicScan.h"
#include "GUIDialogPlayerControls.h"
#include "GUIDialogMusicOSD.h"
#include "GUIDialogVisualisationSettings.h"
#include "GUIDialogVisualisationPresetList.h"
#include "GUIDialogVideoSettings.h"
#include "GUIDialogAudioSubtitleSettings.h"
#include "GUIDialogVideoBookmarks.h"
#include "GUIDialogTrainerSettings.h"
#include "GUIDialogNetworkSetup.h"
#include "GUIDialogMediaSource.h"
#include "GUIWindowOSD.h"
#include "GUIWindowScriptsInfo.h"
#include "GUIDialogProfileSettings.h"
#include "GUIDialogLockSettings.h"
// uncomment this if you want to use release libs in the debug build.
// Atm this saves you 7 mb of memory
#define USE_RELEASE_LIBS

#pragma comment (lib,"xbmc/lib/libXenium/XeniumSPIg.lib")
#pragma comment (lib,"xbmc/lib/libSpeex/libSpeex.lib")

#if defined(_DEBUG) && !defined(USE_RELEASE_LIBS)
 #pragma comment (lib,"xbmc/lib/libXBMS/libXBMSd.lib")    // SECTIONNAME=LIBXBMS
 #pragma comment (lib,"xbmc/lib/libsmb/libsmbd.lib")      // SECTIONNAME=LIBSMB
 //#pragma comment (lib,"xbmc/lib/libPython/pythond.lib")  // SECTIONNAME=PYTHON,PY_RW
 #pragma comment (lib,"xbmc/lib/libGoAhead/goaheadd.lib") // SECTIONNAME=LIBHTTP
 #pragma comment (lib,"xbmc/lib/sqlLite/libSQLite3d.lib")
 #pragma comment (lib,"xbmc/lib/libcdio/libcdiod.lib" )
 #pragma comment (lib,"xbmc/lib/libshout/libshoutd.lib" )
 #pragma comment (lib,"xbmc/lib/libRTV/libRTVd.lib")    // SECTIONNAME=LIBRTV
 #pragma comment (lib,"xbmc/lib/mikxbox/mikxboxd.lib")  // SECTIONNAME=MOD_RW,MOD_RX
/* #pragma comment (lib,"xbmc/lib/libsidplay/libsidplayd.lib")   // SECTIONNAME=SID_RW,SID_RX
 #pragma comment (lib,"xbmc/lib/libsidplay/libsidutilsd.lib")  // SECTIONNAME=SID_RW,SID_RX
 #pragma comment (lib,"xbmc/lib/libsidplay/resid_builderd.lib") // SECTIONNAME=SID_RW,SID_RX*/
 #pragma comment (lib,"xbmc/lib/libxdaap/libxdaapd.lib") // SECTIONNAME=LIBXDAAP
 #pragma comment (lib,"xbmc/lib/libiconv/libiconvd.lib")
 #pragma comment (lib,"xbmc/lib/libfribidi/libfribidid.lib")
 #pragma comment (lib,"xbmc/lib/unrarXlib/unrarxlibd.lib") 
 #pragma comment (lib,"xbmc/lib/libUPnP/libPlatinumd.lib")
#else
 #pragma comment (lib,"xbmc/lib/libXBMS/libXBMS.lib")
 #pragma comment (lib,"xbmc/lib/libsmb/libsmb.lib")
 //#pragma comment (lib,"xbmc/lib/libPython/python.lib")
 #pragma comment (lib,"xbmc/lib/libGoAhead/goahead.lib")
 #pragma comment (lib,"xbmc/lib/sqlLite/libSQLite3.lib")
 #pragma comment (lib,"xbmc/lib/libcdio/libcdio.lib")
 #pragma comment (lib,"xbmc/lib/libshout/libshout.lib")
 #pragma comment (lib,"xbmc/lib/libRTV/libRTV.lib")
 #pragma comment (lib,"xbmc/lib/mikxbox/mikxbox.lib")
 /*#pragma comment (lib,"xbmc/lib/libsidplay/libsidplay.lib")    // SECTIONNAME=SID_RW,SID_RX
 #pragma comment (lib,"xbmc/lib/libsidplay/libsidutils.lib")   // SECTIONNAME=SID_RW,SID_RX
 #pragma comment (lib,"xbmc/lib/libsidplay/resid_builder.lib") // SECTIONNAME=SID_RW,SID_RX*/
 #pragma comment (lib,"xbmc/lib/libxdaap/libxdaap.lib") // SECTIONNAME=LIBXDAAP
 #pragma comment (lib,"xbmc/lib/libiconv/libiconv.lib")
 #pragma comment (lib,"xbmc/lib/libfribidi/libfribidi.lib")
 #pragma comment (lib,"xbmc/lib/unrarXlib/unrarxlib.lib")
 #pragma comment (lib,"xbmc/lib/libUPnP/libPlatinum.lib")
#endif

#define MAX_FFWD_SPEED 5

CStdString g_LoadErrorStr;


extern "C"
{
	extern bool WINAPI NtSetSystemTime(LPFILETIME SystemTime , LPFILETIME PreviousTime );
};

//extern IDirectSoundRenderer* m_pAudioDecoder;
CApplication::CApplication(void)
    : m_ctrDpad(220, 220)
{
  m_iPlaySpeed = 1;
  m_bSpinDown = false;
  m_bNetworkSpinDown = false;
  m_dwSpinDownTime = timeGetTime();
  m_pWebServer = NULL;
  m_pFileZilla = NULL;
  pXbmcHttp = NULL;
  m_pPlayer = NULL;
  XSetProcessQuantumLength(5); //default=20msec
  XSetFileCacheSize (256*1024); //default=64kb
  m_bInactive = false;   // CB: SCREENSAVER PATCH
  m_bScreenSave = false;   // CB: SCREENSAVER PATCH
  m_iScreenSaveLock = 0;
  m_dwSaverTick = timeGetTime(); // CB: SCREENSAVER PATCH
  m_dwSkinTime = 0;

  m_bInitializing = true;
  m_eForcedNextPlayer = EPC_NONE;
  m_strPlayListFile = "";
  m_nextPlaylistItem = -1;
  m_playCountUpdated = false;

  // true while we switch to fullscreen (while video is paused)
  m_switchingToFullScreen = false;

  /* for now allways keep this around */
  m_pCdgParser = new CCdgParser();
}

CApplication::~CApplication(void)
{}

// text out routine for below
static void __cdecl FEH_TextOut(XFONT* pFont, int iLine, const wchar_t* fmt, ...)
{
  wchar_t buf[100];
  va_list args;
  va_start(args, fmt);
  _vsnwprintf(buf, 100, fmt, args);
  va_end(args);

  if (!(iLine & 0x8000))
    CLog::Log(LOGFATAL, "%S", buf);

  bool Center = (iLine & 0x10000) > 0;
  pFont->SetTextAlignment(Center ? XFONT_TOP | XFONT_CENTER : XFONT_TOP | XFONT_LEFT);

  iLine &= 0x7fff;

  for (int i = 0; i < 2; i++)
  {
    D3DRECT rc = { 0, 50 + 25 * iLine, 720, 50 + 25 * (iLine + 1) };
    D3DDevice::Clear(1, &rc, D3DCLEAR_TARGET, 0, 0, 0);
    pFont->TextOut(g_application.m_pBackBuffer, buf, -1, Center ? 360 : 80, 50 + 25*iLine);
    D3DDevice::Present(0, 0, 0, 0);
  }
}

void CApplication::InitBasicD3D()
{
  bool bPal = g_videoConfig.HasPAL();
  CLog::Log(LOGINFO, "Init display in default mode: %s", bPal ? "PAL" : "NTSC");
  // init D3D with defaults (NTSC or PAL standard res)
  m_d3dpp.BackBufferWidth = 720;
  m_d3dpp.BackBufferHeight = bPal ? 576 : 480;
  m_d3dpp.BackBufferFormat = D3DFMT_LIN_X8R8G8B8;
  m_d3dpp.BackBufferCount = 1;
  m_d3dpp.EnableAutoDepthStencil = FALSE;
  m_d3dpp.SwapEffect = D3DSWAPEFFECT_COPY;
  m_d3dpp.FullScreen_PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

  if (!(m_pD3D = Direct3DCreate8(D3D_SDK_VERSION)))
  {
    CLog::Log(LOGFATAL, "FATAL ERROR: Unable to create Direct3D!");
    Sleep(INFINITE); // die
  }

  // Check if we have the required modes available
  g_videoConfig.GetModes(m_pD3D);
  if (!g_graphicsContext.IsValidResolution(g_guiSettings.m_LookAndFeelResolution))
  {
    // Oh uh - doesn't look good for starting in their wanted screenmode
    CLog::Log(LOGERROR, "The screen resolution requested is not valid, resetting to a valid mode");
    g_guiSettings.m_LookAndFeelResolution = g_videoConfig.GetSafeMode();
    CLog::Log(LOGERROR, "Resetting to mode %s", g_settings.m_ResInfo[g_guiSettings.m_LookAndFeelResolution].strMode);
    CLog::Log(LOGERROR, "Done reset");
  }
  // Transfer the resolution information to our graphics context
  g_graphicsContext.SetD3DParameters(&m_d3dpp);
  g_graphicsContext.SetGUIResolution(g_guiSettings.m_LookAndFeelResolution);

  // Create the device
  if (m_pD3D->CreateDevice(0, D3DDEVTYPE_HAL, NULL, D3DCREATE_HARDWARE_VERTEXPROCESSING, &m_d3dpp, &m_pd3dDevice) != S_OK)
  {
    CLog::Log(LOGFATAL, "FATAL ERROR: Unable to create D3D Device!");
    Sleep(INFINITE); // die
  }

  m_pd3dDevice->GetBackBuffer(0, 0, &m_pBackBuffer);

  //  XInitDevices( m_dwNumInputDeviceTypes, m_InputDeviceTypes );

  // Create the gamepad devices
  //  HaveGamepad = (XBInput_CreateGamepads(&m_Gamepad) == S_OK);
  if (m_splash)
  {
    m_splash->Stop();
  }
  m_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET, 0, 0, 0);
  m_pd3dDevice->BlockUntilVerticalBlank();
  m_pd3dDevice->Present( NULL, NULL, NULL, NULL );
}

// This function does not return!
void CApplication::FatalErrorHandler(bool InitD3D, bool MapDrives, bool InitNetwork)
{
  // XBMC couldn't start for some reason...
  // g_LoadErrorStr should contain the reason
  CLog::Log(LOGWARNING, "Emergency recovery console starting...");

  bool HaveGamepad = true; // should always have the gamepad when we get here
  if (InitD3D)
  {
    InitBasicD3D();
    //  XInitDevices( m_dwNumInputDeviceTypes, m_InputDeviceTypes );

    // Create the gamepad devices
    //  HaveGamepad = (XBInput_CreateGamepads(&m_Gamepad) == S_OK);*/
  }
  if (m_splash)
  {
    m_splash->Stop();
  }
  m_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET, 0, 0, 0);
  m_pd3dDevice->BlockUntilVerticalBlank();
  m_pd3dDevice->Present( NULL, NULL, NULL, NULL );

  // D3D is up, load default font
  XFONT* pFont;
  if (XFONT_OpenDefaultFont(&pFont) != S_OK)
  {
    CLog::Log(LOGFATAL, "FATAL ERROR: Unable to open default font!");
    Sleep(INFINITE); // die
  }

  // defaults for text
  pFont->SetBkMode(XFONT_OPAQUE);
  pFont->SetBkColor(D3DCOLOR_XRGB(0, 0, 0));
  pFont->SetTextColor(D3DCOLOR_XRGB(0xff, 0x20, 0x20));

  int iLine = 0;
  FEH_TextOut(pFont, iLine++, L"XBMC Fatal Error:");
  char buf[500];
  strncpy(buf, g_LoadErrorStr.c_str(), 500);
  buf[499] = 0;
  char* pNewline = strtok(buf, "\n");
  while (pNewline)
  {
    FEH_TextOut(pFont, iLine++, L"%S", pNewline);
    pNewline = strtok(NULL, "\n");
  }
  ++iLine;

  if (MapDrives)
  {
    CIoSupport helper;
    // map in default drives
    helper.Remap("C:,Harddisk0\\Partition2");
    helper.Remount("D:", "Cdrom0");
    helper.Remap("E:,Harddisk0\\Partition1");
    //Add. also Drive F/G
    /*
    if ((g_advancedSettings.m_autoDetectFG && helper.IsDrivePresent("F:")) || g_advancedSettings.m_useFDrive) helper.Remap("F:,Harddisk0\\Partition6");
    if ((g_advancedSettings.m_autoDetectFG && helper.IsDrivePresent("G:")) || g_advancedSettings.m_useGDrive) helper.Remap("G:,Harddisk0\\Partition7");
    */

  }

  bool Pal = g_graphicsContext.GetVideoResolution() == PAL_4x3;

  if (HaveGamepad)
    FEH_TextOut(pFont, (Pal ? 16 : 12) | 0x18000, L"Press any button to reboot");

  bool NetworkUp = false;

  // Boot up the network for FTP
  if (InitNetwork)
  {
    std::vector<int> netorder;
    if (m_bXboxMediacenterLoaded)
    {
      if (g_guiSettings.GetInt("network.assignment") == NETWORK_DHCP)
      {
        netorder.push_back(NETWORK_DHCP);
        netorder.push_back(NETWORK_STATIC);
      }
      else if (g_guiSettings.GetInt("network.assignment") == NETWORK_STATIC)
      {
        netorder.push_back(NETWORK_STATIC);
        netorder.push_back(NETWORK_DHCP);
      }
      else
      {
        netorder.push_back(NETWORK_DASH);
        netorder.push_back(NETWORK_DHCP);
        netorder.push_back(NETWORK_STATIC);
      }
    }
    else
    {
      netorder.push_back(NETWORK_DASH);
      netorder.push_back(NETWORK_DHCP);
      netorder.push_back(NETWORK_STATIC);
    }

    while(1)
    {
      std::vector<int>::iterator it;
      for( it = netorder.begin();it != netorder.end(); it++)
      {
        g_network.Deinitialize();

        if (!(XNetGetEthernetLinkStatus() & XNET_ETHERNET_LINK_ACTIVE))
        {
          FEH_TextOut(pFont, iLine, L"Network cable unplugged");
          break;
        }
        
        switch( (*it) )
        {
          case NETWORK_DASH:
            FEH_TextOut(pFont, iLine, L"Init network using dash settings...");
            g_network.Initialize(NETWORK_DASH, "","","","");
            break;
          case NETWORK_DHCP:
            FEH_TextOut(pFont, iLine, L"Init network using DHCP...");
            g_network.Initialize(NETWORK_DHCP, "","","","");
            break;
          default:
            FEH_TextOut(pFont, iLine, L"Init network using static ip...");
            if( m_bXboxMediacenterLoaded )
            {
              g_network.Initialize(NETWORK_STATIC, 
                    g_guiSettings.GetString("network.ipaddress").c_str(), 
                    g_guiSettings.GetString("network.subnet").c_str(), 
                    g_guiSettings.GetString("network.gateway").c_str(),
                    g_guiSettings.GetString("network.dns").c_str() );
            }
            else
            {
              g_network.Initialize(NETWORK_STATIC, 
                    "192.168.0.42", 
                    "255.255.255.0", 
                    "192.168.0.1",
                    "192.168.0.1" );
            }
            break;
        }

        int count = 0;
        DWORD dwState = XNET_GET_XNADDR_PENDING;

        while(dwState == XNET_GET_XNADDR_PENDING)
        {
          dwState = g_network.UpdateState();

          if( dwState != XNET_GET_XNADDR_PENDING )
            break;

          if (HaveGamepad && AnyButtonDown())
            g_applicationMessenger.Restart();


          Sleep(50);
          ++count;
        }

        if( dwState != XNET_GET_XNADDR_PENDING && dwState != XNET_GET_XNADDR_NONE )
        {
          /* yay, we got network */
          NetworkUp = true;
          break;
        }
        /* increment line before next attempt */
        ++iLine;
      }      

      /* break out of the continous loop if we have network*/
      if( NetworkUp )
        break;
      else
      {
        int n = 10;
        while (n)
        {
          FEH_TextOut(pFont, (iLine + 1) | 0x8000, L"Unable to init network, retrying in %d seconds", n--);
          for (int i = 0; i < 20; ++i)
          {
            Sleep(50);

            if (HaveGamepad && AnyButtonDown())
              g_applicationMessenger.Restart();
          }
        }
      }
    }
  }

  if( NetworkUp )
  {
    FEH_TextOut(pFont, iLine++, L"IP Address: %S", g_network.m_networkinfo.ip);
    ++iLine;
  }

  if (NetworkUp)
  {
    if (!m_pFileZilla)
    {
      // Start FTP with default settings
      FEH_TextOut(pFont, iLine++, L"Starting FTP server...");

      m_pFileZilla = new CXBFileZilla(NULL);
      m_pFileZilla->Start();

      // Default settings
      m_pFileZilla->mSettings.SetMaxUsers(0);
      m_pFileZilla->mSettings.SetWelcomeMessage("XBMC emergency recovery console FTP.");

      // default user
      CXFUser* pUser;
      m_pFileZilla->AddUser("xbox", pUser);
      pUser->SetPassword("xbox");
      pUser->SetShortcutsEnabled(false);
      pUser->SetUseRelativePaths(false);
      pUser->SetBypassUserLimit(false);
      pUser->SetUserLimit(0);
      pUser->SetIPLimit(0);
      pUser->AddDirectory("/", XBFILE_READ | XBFILE_WRITE | XBFILE_DELETE | XBFILE_APPEND | XBDIR_DELETE | XBDIR_CREATE | XBDIR_LIST | XBDIR_SUBDIRS | XBDIR_HOME);
      pUser->AddDirectory("C:\\", XBFILE_READ | XBFILE_WRITE | XBFILE_DELETE | XBFILE_APPEND | XBDIR_DELETE | XBDIR_CREATE | XBDIR_LIST | XBDIR_SUBDIRS);
      pUser->AddDirectory("D:\\", XBFILE_READ | XBDIR_LIST | XBDIR_SUBDIRS);
      pUser->AddDirectory("E:\\", XBFILE_READ | XBFILE_WRITE | XBFILE_DELETE | XBFILE_APPEND | XBDIR_DELETE | XBDIR_CREATE | XBDIR_LIST | XBDIR_SUBDIRS);
      pUser->AddDirectory("Q:\\", XBFILE_READ | XBFILE_WRITE | XBFILE_DELETE | XBFILE_APPEND | XBDIR_DELETE | XBDIR_CREATE | XBDIR_LIST | XBDIR_SUBDIRS);
      //Add. also Drive F/G
      /*
      CIoSupport helper;
      if ((g_advancedSettings.m_autoDetectFG && helper.IsDrivePresent("F:")) || g_advancedSettings.m_useFDrive){
        pUser->AddDirectory("F:\\", XBFILE_READ | XBFILE_WRITE | XBFILE_DELETE | XBFILE_APPEND | XBDIR_DELETE | XBDIR_CREATE | XBDIR_LIST | XBDIR_SUBDIRS);
      }
      if ((g_advancedSettings.m_autoDetectFG && helper.IsDrivePresent("G:")) || g_advancedSettings.m_useGDrive){
        pUser->AddDirectory("G:\\", XBFILE_READ | XBFILE_WRITE | XBFILE_DELETE | XBFILE_APPEND | XBDIR_DELETE | XBDIR_CREATE | XBDIR_LIST | XBDIR_SUBDIRS);
      }
      */
      pUser->CommitChanges();
    }

    FEH_TextOut(pFont, iLine++, L"FTP server running on port %d, login: xbox/xbox", m_pFileZilla->mSettings.GetServerPort());
    ++iLine;
  }

  if (HaveGamepad)
  {
    for (;;)
    {
      Sleep(50);
      if (AnyButtonDown())
        g_applicationMessenger.Restart();
    }
  }
  else
    Sleep(INFINITE);
}

LONG WINAPI CApplication::UnhandledExceptionFilter(struct _EXCEPTION_POINTERS *ExceptionInfo)
{
  PCSTR pExceptionString = "Unknown exception code";

#define STRINGIFY_EXCEPTION(code) case code: pExceptionString = #code; break

  switch (ExceptionInfo->ExceptionRecord->ExceptionCode)
  {
    STRINGIFY_EXCEPTION(EXCEPTION_ACCESS_VIOLATION);
    STRINGIFY_EXCEPTION(EXCEPTION_ARRAY_BOUNDS_EXCEEDED);
    STRINGIFY_EXCEPTION(EXCEPTION_BREAKPOINT);
    STRINGIFY_EXCEPTION(EXCEPTION_FLT_DENORMAL_OPERAND);
    STRINGIFY_EXCEPTION(EXCEPTION_FLT_DIVIDE_BY_ZERO);
    STRINGIFY_EXCEPTION(EXCEPTION_FLT_INEXACT_RESULT);
    STRINGIFY_EXCEPTION(EXCEPTION_FLT_INVALID_OPERATION);
    STRINGIFY_EXCEPTION(EXCEPTION_FLT_OVERFLOW);
    STRINGIFY_EXCEPTION(EXCEPTION_FLT_STACK_CHECK);
    STRINGIFY_EXCEPTION(EXCEPTION_ILLEGAL_INSTRUCTION);
    STRINGIFY_EXCEPTION(EXCEPTION_INT_DIVIDE_BY_ZERO);
    STRINGIFY_EXCEPTION(EXCEPTION_INT_OVERFLOW);
    STRINGIFY_EXCEPTION(EXCEPTION_INVALID_DISPOSITION);
    STRINGIFY_EXCEPTION(EXCEPTION_NONCONTINUABLE_EXCEPTION);
    STRINGIFY_EXCEPTION(EXCEPTION_SINGLE_STEP);
  }
#undef STRINGIFY_EXCEPTION

  g_LoadErrorStr.Format("%s (0x%08x)\n at 0x%08x",
                        pExceptionString, ExceptionInfo->ExceptionRecord->ExceptionCode,
                        ExceptionInfo->ExceptionRecord->ExceptionAddress);

  CLog::Log(LOGFATAL, "%s", g_LoadErrorStr.c_str());

  return ExceptionInfo->ExceptionRecord->ExceptionCode;
}
#include "xbox/undocumented.h"
extern "C" HANDLE __stdcall KeGetCurrentThread(VOID);
extern "C" void __stdcall init_emu_environ();

HRESULT CApplication::Create()
{
  HRESULT hr;
  //grab a handle to our thread to be used later in identifying the render thread
  m_threadID = GetCurrentThreadId();

  //floating point precision to 24 bits (faster performance)
  _controlfp(_PC_24, _MCW_PC);

  init_emu_environ();

  /* install win32 exception translator, win32 exceptions
   * can now be caught using c++ try catch */
  win32_exception::install_handler();
  
  CIoSupport helper;
  CStdString strExecutablePath;
  char szDevicePath[1024];

  // map Q to home drive of xbe to load the config file
  CUtil::GetHomePath(strExecutablePath);
  helper.GetPartition(strExecutablePath, szDevicePath);
  strcat(szDevicePath, &strExecutablePath.c_str()[2]);

  helper.Mount("Q:", "Harddisk0\\Partition2");
  helper.Unmount("Q:");
  helper.Mount("Q:", szDevicePath);

  // check logpath
  CStdString strLogFile, strLogFileOld;
  strLogFile.Format("%sxbmc.log", g_stSettings.m_logFolder);
  strLogFileOld.Format("%sxbmc.old.log", g_stSettings.m_logFolder);

  ::DeleteFile(strLogFileOld.c_str());
  ::MoveFile(strLogFile.c_str(), strLogFileOld.c_str());
  
  CLog::Log(LOGNOTICE, "-----------------------------------------------------------------------");
  CLog::Log(LOGNOTICE, "Starting XBoxMediaCenter.  Built on %s", __DATE__);
  CLog::Log(LOGNOTICE, "Q is mapped to: %s",szDevicePath );
  CLog::Log(LOGNOTICE, "Log File is located: %s", strLogFile.c_str());
  CLog::Log(LOGNOTICE, "-----------------------------------------------------------------------");



  g_settings.m_vecProfiles.clear();
  g_settings.LoadProfiles("q:\\system\\profiles.xml");
  if (g_settings.m_vecProfiles.size() == 0)
  {
    //no profiles yet, make one based on the default settings
    CProfile profile;
    profile.setDirectory("q:\\userdata");
    profile.setName("Master user");
    profile.setLockMode(LOCK_MODE_EVERYONE);
    profile.setLockCode("");
    profile.setDate("");
    g_settings.m_vecProfiles.push_back(profile);
  }

  // if we are running from DVD our UserData location will be TDATA
  if (CUtil::IsDVD(strExecutablePath))
  {
    // TODO: Should we copy over any UserData folder from the DVD?
    if (!CFile::Exists("T:\\guisettings.xml")) // first run - cache userdata folder
    {
      CFileItem item("Q:\\UserData");
      item.m_strPath = "Q:\\UserData";
      item.m_bIsFolder = true;
      item.Select(true);
      CGUIWindowFileManager::CopyItem(&item,"T:\\",NULL);
    }
    g_settings.m_vecProfiles[0].setDirectory("T:\\");
    g_stSettings.m_logFolder = "T:\\";
  }
  else
  {
    helper.Unmount("T:");
    CStdString strMnt = g_settings.GetUserDataFolder();
    if (g_settings.GetUserDataFolder().Left(2).Equals("Q:"))
    {
      CUtil::GetHomePath(strMnt);
      strMnt += g_settings.GetUserDataFolder().substr(2);
    }    

    helper.GetPartition(strMnt, szDevicePath);
    strcat(szDevicePath, &strMnt.c_str()[2]);
    helper.Mount("T:",szDevicePath);
  }

  CLog::Log(LOGNOTICE, "Setup DirectX");
  // Create the Direct3D object
  if ( NULL == ( m_pD3D = Direct3DCreate8(D3D_SDK_VERSION) ) )
  {
    CLog::Log(LOGFATAL, "XBAppEx: Unable to create Direct3D!" );
    return E_FAIL;
  }

  //list available videomodes
  g_videoConfig.GetModes(m_pD3D);
  //init the present parameters with values that are supported
  RESOLUTION initialResolution = g_videoConfig.GetInitialMode(m_pD3D, &m_d3dpp);
  // Transfer the resolution information to our graphics context
  g_graphicsContext.SetD3DParameters(&m_d3dpp);
  g_graphicsContext.SetGUIResolution(initialResolution);

  // Initialize core peripheral port support. Note: If these parameters
  // are 0 and NULL, respectively, then the default number and types of
  // controllers will be initialized.
  XInitDevices( m_dwNumInputDeviceTypes, m_InputDeviceTypes );

  // Create the gamepad devices
  if ( FAILED(hr = XBInput_CreateGamepads(&m_Gamepad)) )
  {
    CLog::Log(LOGERROR, "XBAppEx: Call to CreateGamepads() failed!" );
    return hr;
  }

  if ( FAILED(hr = XBInput_CreateIR_Remotes()) )
  {
    CLog::Log(LOGERROR, "XBAppEx: Call to CreateIRRemotes() failed!" );
    return hr;
  }

  // Create the Mouse and Keyboard devices
  g_Mouse.Initialize();
  g_Keyboard.Initialize();

  // Wait for controller polling to finish. in an elegant way, instead of a Sleep(1000)
  while (XGetDeviceEnumerationStatus() == XDEVICE_ENUMERATION_BUSY)
  {
    ReadInput();
  }

  //Check for LTHUMBCLICK+RTHUMBCLICK and BLACK+WHITE, no LTRIGGER+RTRIGGER
  if (((m_DefaultGamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB + XINPUT_GAMEPAD_RIGHT_THUMB) && !(m_DefaultGamepad.wButtons & KEY_BUTTON_LEFT_TRIGGER+KEY_BUTTON_RIGHT_TRIGGER)) ||
      ((m_DefaultGamepad.bAnalogButtons[XINPUT_GAMEPAD_BLACK] && m_DefaultGamepad.bAnalogButtons[XINPUT_GAMEPAD_WHITE]) && !(m_DefaultGamepad.wButtons & KEY_BUTTON_LEFT_TRIGGER+KEY_BUTTON_RIGHT_TRIGGER)))
  {
    CLog::Log(LOGINFO, "Key combination detected for TDATA deletion (LTHUMB+RTHUMB or BLACK+WHITE)");
    InitBasicD3D();
    // D3D is up, load default font
    XFONT* pFont;
    if (XFONT_OpenDefaultFont(&pFont) != S_OK)
    {
      CLog::Log(LOGFATAL, "FATAL ERROR: Unable to open default font!");
      Sleep(INFINITE); // die
    }
    // defaults for text
    pFont->SetBkMode(XFONT_OPAQUE);
    pFont->SetBkColor(D3DCOLOR_XRGB(0, 0, 0));
    pFont->SetTextColor(D3DCOLOR_XRGB(0xff, 0x20, 0x20));
    int iLine = 0;
    FEH_TextOut(pFont, iLine++, L"Key combination detected for TDATA deletion:");
    FEH_TextOut(pFont, iLine++, L"Are you sure you want to proceed?");
    iLine++;
    FEH_TextOut(pFont, iLine++, L"A for yes, any other key for no");
    bool bAnyAnalogKey = false;
    while (m_DefaultGamepad.wPressedButtons == XBGAMEPAD_NONE && !bAnyAnalogKey) 
    {
      ReadInput();
      bAnyAnalogKey = m_DefaultGamepad.bPressedAnalogButtons[0] || m_DefaultGamepad.bPressedAnalogButtons[1] || m_DefaultGamepad.bPressedAnalogButtons[2] || m_DefaultGamepad.bPressedAnalogButtons[3] || m_DefaultGamepad.bPressedAnalogButtons[4] || m_DefaultGamepad.bPressedAnalogButtons[5] || m_DefaultGamepad.bPressedAnalogButtons[6] || m_DefaultGamepad.bPressedAnalogButtons[7];
    }
    if (m_DefaultGamepad.bPressedAnalogButtons[XINPUT_GAMEPAD_A])
      CUtil::DeleteGUISettings();
    m_pd3dDevice->Release();
  }

  CLog::Log(LOGNOTICE, "load settings...");
  g_LoadErrorStr = "Unable to load settings";
  g_settings.m_iLastUsedProfileIndex = g_settings.m_iLastLoadedProfileIndex;
  if (g_settings.bUseLoginScreen && g_settings.m_iLastLoadedProfileIndex != 0)
    g_settings.m_iLastLoadedProfileIndex = 0;

  m_bAllSettingsLoaded = g_settings.Load(m_bXboxMediacenterLoaded, m_bSettingsLoaded);
  if (!m_bAllSettingsLoaded)
    FatalErrorHandler(true, true, true);

  // Check for WHITE + Y for forced Error Handler (to recover if something screwy happens)
  if (m_DefaultGamepad.bAnalogButtons[XINPUT_GAMEPAD_Y] && m_DefaultGamepad.bAnalogButtons[XINPUT_GAMEPAD_WHITE])
  {
    g_LoadErrorStr = "Key code detected for Error Recovery mode";
    FatalErrorHandler(true, true, true);
  }

  //Check for X+Y - if pressed, set debug log mode and mplayer debuging on
  CheckForDebugButtonCombo();

  bool bNeedReboot = false;
  char temp[1024];
  helper.GetXbePath(temp);
  char temp2[1024];
  char temp3[2];
  temp3[0] = temp[0]; temp3[1] = '\0';
  helper.GetPartition((LPCSTR)temp3,temp2);
  CStdString strTemp(temp+2);
  int iLastSlash = strTemp.rfind('\\');
  strcat(temp2,strTemp.substr(0,iLastSlash).c_str());
  F_VIDEO ForceVideo = VIDEO_NULL;
  F_COUNTRY ForceCountry = COUNTRY_NULL;
  
  if (CUtil::RemoveTrainer())
    bNeedReboot = true;

// now check if we are switching video modes. if, are we in the wrong mode according to eeprom?
  if (g_guiSettings.GetBool("myprograms.gameautoregion"))
  {
    bool fDoPatchTest = false;
    
    // should use xkeeprom.h :/
    EEPROMDATA EEPROM;
  	ZeroMemory(&EEPROM, sizeof(EEPROMDATA));

    if( XKUtils::ReadEEPROMFromXBOX((LPBYTE)&(EEPROM),0,255) ) 
    {
      DWORD DWVideo = *(LPDWORD)(&EEPROM.VideoStandard[0]);
      char temp[1024];
      helper.GetXbePath(temp);
      char temp2[1024];
      char temp3[2];
      temp3[0] = temp[0]; temp3[1] = '\0';
      helper.GetPartition((LPCSTR)temp3,temp2);
      CStdString strTemp(temp+2);
      int iLastSlash = strTemp.rfind('\\');
      strcat(temp2,strTemp.substr(0,iLastSlash).c_str());

      if ((DWVideo == XKEEPROM::VIDEO_STANDARD::NTSC_M) && ((XGetVideoStandard() == XC_VIDEO_STANDARD_PAL_I) || (XGetVideoStandard() == XC_VIDEO_STANDARD_NTSC_J) || initialResolution > 5))
      {
        CLog::Log(LOGINFO, "Rebooting to change resolution from %s back to NTSC_M", (XGetVideoStandard() == XC_VIDEO_STANDARD_PAL_I) ? "PAL" : "NTSC_J");
        ForceVideo = VIDEO_NTSCM;
        ForceCountry = COUNTRY_USA;
        bNeedReboot = true;
        fDoPatchTest = true;
      }
      else if ((DWVideo == XKEEPROM::VIDEO_STANDARD::PAL_I) && ((XGetVideoStandard() == XC_VIDEO_STANDARD_NTSC_M) || (XGetVideoStandard() == XC_VIDEO_STANDARD_NTSC_J) || initialResolution < 6))
      {
        CLog::Log(LOGINFO, "Rebooting to change resolution from %s back to PAL_I", (XGetVideoStandard() == XC_VIDEO_STANDARD_NTSC_M) ? "NTSC_M" : "NTSC_J");
        ForceVideo = VIDEO_PAL50;
        ForceCountry = COUNTRY_EUR;
        bNeedReboot = true;
        fDoPatchTest = true;
      }
      else if ((DWVideo == XKEEPROM::VIDEO_STANDARD::NTSC_J) && ((XGetVideoStandard() == XC_VIDEO_STANDARD_NTSC_M) || (XGetVideoStandard() == XC_VIDEO_STANDARD_PAL_I) || initialResolution > 5))
      {
        CLog::Log(LOGINFO, "Rebooting to change resolution from %s back to NTSC_J", (XGetVideoStandard() == XC_VIDEO_STANDARD_PAL_I) ? "PAL" : "NTSC_M");
        ForceVideo = VIDEO_NTSCJ;
        ForceCountry = COUNTRY_JAP;
        bNeedReboot = true;
        fDoPatchTest = true;
      }
      else
        CUtil::RemoveKernelPatch(); // This removes the Resolution patch from the kernel if it is not needed (if actual resolution matches eeprom setting)	

      if (fDoPatchTest) // Is set if we have to test whether our patch is in the kernel & therefore responsible for the mismatch of resolution & eeprom setting
      {	
        if (!CUtil::LookForKernelPatch()) // If our patch is not present we are not responsible for the mismatch of current resolution & eeprom setting
        {
          // We do a hard reset to come back to default resolution and avoid infinite reboots
          CLog::Log(LOGINFO, "No infinite reboot loop...");
          g_applicationMessenger.Reset();
        }
      }
    }
  } 
  
  if (bNeedReboot)
  {
    Destroy();
    CUtil::LaunchXbe(temp2,("D:\\"+strTemp.substr(iLastSlash+1)).c_str(),NULL,ForceVideo,ForceCountry);
  }
  
  CLog::Log(LOGINFO, "map drives...");
  CLog::Log(LOGINFO, "  map drive C:");
  helper.Remap("C:,Harddisk0\\Partition2");

  CLog::Log(LOGINFO, "  map drive E:");
  helper.Remap("E:,Harddisk0\\Partition1");

  CLog::Log(LOGINFO, "  map drive D:");
  helper.Remount("D:", "Cdrom0");

  if ((g_advancedSettings.m_autoDetectFG && helper.IsDrivePresent("F:")) || g_advancedSettings.m_useFDrive)
  {
    CLog::Log(LOGINFO, "  map drive F:");
    helper.Remap("F:,Harddisk0\\Partition6");
    g_advancedSettings.m_useFDrive = true;
  }

  // used for the LBA-48 hack allowing >120 gig
  if ((g_advancedSettings.m_autoDetectFG && helper.IsDrivePresent("G:")) || g_advancedSettings.m_useGDrive)
  {
    CLog::Log(LOGINFO, "  map drive G:");
    helper.Remap("G:,Harddisk0\\Partition7");
    g_advancedSettings.m_useGDrive = true;
  }

  helper.Remap("X:,Harddisk0\\Partition3");
  helper.Remap("Y:,Harddisk0\\Partition4");
  helper.Remap("Z:,Harddisk0\\Partition5");
  
  CLog::Log(LOGINFO, "Drives are mapped");

  CStdString strHomePath = "Q:";
  CLog::Log(LOGINFO, "Checking skinpath existance, and existence of keymap.xml:%s...", (strHomePath + "\\skin").c_str());
  CStdString keymapPath;
  
  keymapPath = g_settings.GetUserDataItem("keymap.xml");
  //CUtil::AddFileToFolder(g_settings.GetUserDataFolder(), "Keymap.xml", keymapPath);
  if (access(strHomePath + "\\skin", 0) || access(keymapPath.c_str(), 0))
  {
    g_LoadErrorStr = "Unable to find skins or Keymap.xml.  Make sure you have UserData/Keymap.xml and Skins/ folder";
    FatalErrorHandler(true, false, true);
  }

  if (!g_graphicsContext.IsValidResolution(g_guiSettings.m_LookAndFeelResolution))
  {
    // Oh uh - doesn't look good for starting in their wanted screenmode
    CLog::Log(LOGERROR, "The screen resolution requested is not valid, resetting to a valid mode");
    g_guiSettings.m_LookAndFeelResolution = initialResolution;
  }
  // Transfer the new resolution information to our graphics context
  g_graphicsContext.SetD3DParameters(&m_d3dpp);
  g_graphicsContext.SetGUIResolution(g_guiSettings.m_LookAndFeelResolution);
  if ( FAILED( hr = m_pD3D->CreateDevice(0, D3DDEVTYPE_HAL, NULL,
                                         D3DCREATE_HARDWARE_VERTEXPROCESSING,
                                         &m_d3dpp, &m_pd3dDevice ) ) )
  {
    CLog::Log(LOGFATAL, "XBAppEx: Could not create D3D device!" );
    CLog::Log(LOGFATAL, " width/height:(%ix%i)" , m_d3dpp.BackBufferWidth, m_d3dpp.BackBufferHeight);
    CLog::Log(LOGFATAL, " refreshrate:%i" , m_d3dpp.FullScreen_RefreshRateInHz);
    if (m_d3dpp.Flags & D3DPRESENTFLAG_WIDESCREEN)
      CLog::Log(LOGFATAL, " 16:9 widescreen");
    else
      CLog::Log(LOGFATAL, " 4:3");

    if (m_d3dpp.Flags & D3DPRESENTFLAG_INTERLACED)
      CLog::Log(LOGFATAL, " interlaced");
    if (m_d3dpp.Flags & D3DPRESENTFLAG_PROGRESSIVE)
      CLog::Log(LOGFATAL, " progressive");
    return hr;
  }
  m_pd3dDevice->GetBackBuffer( 0, 0, &m_pBackBuffer );
  g_graphicsContext.SetD3DDevice(m_pd3dDevice);
  // set filters
  g_graphicsContext.Get3DDevice()->SetTextureStageState(0, D3DTSS_MINFILTER, D3DTEXF_LINEAR /*g_stSettings.m_minFilter*/ );
  g_graphicsContext.Get3DDevice()->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR /*g_stSettings.m_maxFilter*/ );
  CUtil::InitGamma();
  g_graphicsContext.SetGUIResolution(g_guiSettings.m_LookAndFeelResolution);

  // initialize our charset converter
  g_charsetConverter.reset();

  // Load the langinfo to have user charset <-> utf-8 conversion
  CStdString strLangInfoPath;
  strLangInfoPath.Format("Q:\\language\\%s\\langinfo.xml", g_guiSettings.GetString("lookandfeel.language"));

  CLog::Log(LOGINFO, "load language info file:%s", strLangInfoPath.c_str());
  g_langInfo.Load(strLangInfoPath);

  m_splash = new CSplash("Q:\\media\\splash.png");
  m_splash->Start();

  CStdString strLanguagePath;
  strLanguagePath.Format("Q:\\language\\%s\\strings.xml", g_guiSettings.GetString("lookandfeel.language"));

  CLog::Log(LOGINFO, "load language file:%s", strLanguagePath.c_str());
  if (!g_localizeStrings.Load(strLanguagePath ))
    FatalErrorHandler(false, false, true);

  //Load language translation database
  g_LangCodeExpander.LoadStandardCodes();

  CLog::Log(LOGINFO, "load keymapping");
  if (!g_buttonTranslator.Load())
    FatalErrorHandler(false, false, true);

  // check the skin file for testing purposes
  CStdString strSkinBase = "Q:\\skin\\";
  CStdString strSkinPath = strSkinBase + g_guiSettings.GetString("lookandfeel.skin");
  CLog::Log(LOGINFO, "Checking skin version of: %s", g_guiSettings.GetString("lookandfeel.skin").c_str());
  if (!g_SkinInfo.Check(strSkinPath))
  {
    // reset to the default skin (Project Mayhem III)
    CLog::Log(LOGINFO, "The above skin isn't suitable - checking the version of the default: %s", "Project Mayhem III");
    strSkinPath = strSkinBase + "Project Mayhem III";
    if (!g_SkinInfo.Check(strSkinPath))
    {
      g_LoadErrorStr.Format("No suitable skin version found.\nWe require at least version %5.4f \n", g_SkinInfo.GetMinVersion());
      FatalErrorHandler(false, false, true);
    }
  }
  int iResolution = g_graphicsContext.GetVideoResolution();
  CLog::Log(LOGINFO, " GUI format %ix%i %s",
            g_settings.m_ResInfo[iResolution].iWidth,
            g_settings.m_ResInfo[iResolution].iHeight,
            g_settings.m_ResInfo[iResolution].strMode);
  m_gWindowManager.Initialize();

  g_actionManager.SetScriptActionCallback(&g_pythonParser);

  // show recovery console on fatal error instead of freezing
  CLog::Log(LOGINFO, "install unhandled exception filter");
  SetUnhandledExceptionFilter(UnhandledExceptionFilter);

  return CXBApplicationEx::Create();
}

HRESULT CApplication::Initialize()
{
  CLog::Log(LOGINFO, "creating subdirectories");

  //CLog::Log(LOGINFO, "userdata folder: %s", g_stSettings.m_userDataFolder.c_str());
  CLog::Log(LOGINFO, "userdata folder: %s", g_settings.GetProfileUserDataFolder().c_str());
  CLog::Log(LOGINFO, "  recording folder:%s", g_guiSettings.GetString("musicfiles.recordingpath",false).c_str());
  CLog::Log(LOGINFO, "  screenshots folder:%s", g_guiSettings.GetString("pictures.screenshotpath",false).c_str());
	
  // UserData folder layout:
  // UserData/
  //   Database/
  //     CDDb/
  //     IMDb/
  //   Thumbnails/
  //     Music/
  //       temp/
  //     0 .. F/
  //     XLinkKai/

  CreateDirectory(g_settings.GetUserDataFolder().c_str(), NULL);
  CreateDirectory(g_settings.GetProfileUserDataFolder().c_str(), NULL);
  CreateDirectory(g_settings.GetDatabaseFolder().c_str(), NULL);
  CreateDirectory(g_settings.GetCDDBFolder().c_str(), NULL);
  CreateDirectory(g_settings.GetIMDbFolder().c_str(), NULL);

  // Thumbnails/
  CreateDirectory(g_settings.GetThumbnailsFolder().c_str(), NULL);
  CreateDirectory(g_settings.GetMusicThumbFolder().c_str(), NULL);
  CreateDirectory(g_settings.GetMusicArtistThumbFolder().c_str(), NULL);
  CreateDirectory(g_settings.GetVideoThumbFolder().c_str(), NULL);
  CreateDirectory(g_settings.GetBookmarksThumbFolder().c_str(), NULL);
  CreateDirectory(g_settings.GetProgramsThumbFolder().c_str(), NULL);
  CreateDirectory(g_settings.GetXLinkKaiThumbFolder().c_str(), NULL);
  CreateDirectory(g_settings.GetPicturesThumbFolder().c_str(), NULL);
  CreateDirectory(g_settings.GetProfilesThumbFolder().c_str(),NULL);
  CLog::Log(LOGINFO, "  thumbnails folder:%s", g_settings.GetThumbnailsFolder().c_str());
  for (unsigned int hex=0; hex < 16; hex++)
  {
    CStdString strThumbLoc = g_settings.GetPicturesThumbFolder();
    CStdString strHex;
    strHex.Format("%x",hex);
    strThumbLoc += "\\" + strHex;
    CreateDirectory(strThumbLoc.c_str(),NULL);
  }

  CreateDirectory("Z:\\temp", NULL); // temp directory for python
  CreateDirectory("Q:\\scripts", NULL);
  CreateDirectory("Q:\\language", NULL);
  CreateDirectory("Q:\\visualisations", NULL);
  CreateDirectory("Q:\\sounds", NULL);

  // initialize network
  if (!m_bXboxMediacenterLoaded)
  {
    CLog::Log(LOGINFO, "using default network settings");
    g_guiSettings.SetString("network.ipaddress", "192.168.0.100");
    g_guiSettings.SetString("network.subnet", "255.255.255.0");
    g_guiSettings.SetString("network.gateway", "192.168.0.1");
    g_guiSettings.SetString("network.dns", "192.168.0.1");
    g_guiSettings.SetBool("servers.ftpserver", true);
    g_guiSettings.SetBool("servers.webserver", false);
    g_guiSettings.SetBool("xbdatetime.timeserver", false);
  }
  
  StartServices();

  m_gWindowManager.Add(new CGUIWindowHome);                     // window id = 0

  CLog::Log(LOGNOTICE, "load default skin:[%s]", g_guiSettings.GetString("lookandfeel.skin").c_str());
  LoadSkin(g_guiSettings.GetString("lookandfeel.skin"));

  m_gWindowManager.Add(new CGUIWindowPrograms);                 // window id = 1
  m_gWindowManager.Add(new CGUIWindowPictures);                 // window id = 2
  m_gWindowManager.Add(new CGUIWindowFileManager);      // window id = 3
  m_gWindowManager.Add(new CGUIWindowVideoFiles);          // window id = 6
  m_gWindowManager.Add(new CGUIWindowSettings);                 // window id = 4
  m_gWindowManager.Add(new CGUIWindowSystemInfo);               // window id = 7
  m_gWindowManager.Add(new CGUIWindowSettingsScreenCalibration); // window id = 11
  m_gWindowManager.Add(new CGUIWindowSettingsCategory);         // window id = 12 slideshow:window id 2007
  m_gWindowManager.Add(new CGUIWindowScripts);                  // window id = 20
  m_gWindowManager.Add(new CGUIWindowVideoGenre);               // window id = 21
  m_gWindowManager.Add(new CGUIWindowVideoActors);              // window id = 22
  m_gWindowManager.Add(new CGUIWindowVideoYear);                // window id = 23
  m_gWindowManager.Add(new CGUIWindowVideoTitle);               // window id = 25
  m_gWindowManager.Add(new CGUIWindowVideoPlaylist);            // window id = 28
  m_gWindowManager.Add(new CGUIWindowLoginScreen);            // window id = 29
  m_gWindowManager.Add(new CGUIWindowSettingsProfile);          // window id = 34

  m_gWindowManager.Add(new CGUIDialogYesNo);              // window id = 100
  m_gWindowManager.Add(new CGUIDialogProgress);           // window id = 101
  m_gWindowManager.Add(new CGUIDialogInvite);             // window id = 102
  m_gWindowManager.Add(new CGUIDialogKeyboard);           // window id = 103
  m_gWindowManager.Add(&m_guiDialogVolumeBar);          // window id = 104
  m_gWindowManager.Add(&m_guiDialogSeekBar);            // window id = 115
  m_gWindowManager.Add(new CGUIDialogSubMenu);            // window id = 105
  m_gWindowManager.Add(new CGUIDialogContextMenu);        // window id = 106
  m_gWindowManager.Add(&m_guiDialogKaiToast);           // window id = 107
  m_gWindowManager.Add(new CGUIDialogHost);               // window id = 108
  m_gWindowManager.Add(new CGUIDialogNumeric);            // window id = 109
  m_gWindowManager.Add(new CGUIDialogGamepad);            // window id = 110
  m_gWindowManager.Add(new CGUIDialogButtonMenu);         // window id = 111
  m_gWindowManager.Add(new CGUIDialogMusicScan);          // window id = 112
  m_gWindowManager.Add(new CGUIDialogPlayerControls);     // window id = 113
  m_gWindowManager.Add(new CGUIDialogMusicOSD);           // window id = 120
  m_gWindowManager.Add(new CGUIDialogVisualisationSettings);     // window id = 121
  m_gWindowManager.Add(new CGUIDialogVisualisationPresetList);   // window id = 122
  m_gWindowManager.Add(new CGUIDialogVideoSettings);             // window id = 123
  m_gWindowManager.Add(new CGUIDialogAudioSubtitleSettings);     // window id = 124
  m_gWindowManager.Add(new CGUIDialogVideoBookmarks);      // window id = 125
  // Don't add the filebrowser dialog - it's created and added when it's needed
  m_gWindowManager.Add(new CGUIDialogTrainerSettings);  // window id = 127
  m_gWindowManager.Add(new CGUIDialogNetworkSetup);  // window id = 128
  m_gWindowManager.Add(new CGUIDialogMediaSource);   // window id = 129
  m_gWindowManager.Add(new CGUIDialogProfileSettings); // window id = 130

  CGUIDialogLockSettings* pDialog = NULL;
  CStdString strPath;
  RESOLUTION res2;
  strPath = g_SkinInfo.GetSkinPath("LockSettings.xml", &res2);
  if (CFile::Exists(strPath))
    pDialog = new CGUIDialogLockSettings;
  
  if (pDialog)
    m_gWindowManager.Add(pDialog); // window id = 131

  m_gWindowManager.Add(new CGUIWindowMusicPlayList);          // window id = 500
  m_gWindowManager.Add(new CGUIWindowMusicSongs);             // window id = 501
  m_gWindowManager.Add(new CGUIWindowMusicNav);               // window id = 502
//  m_gWindowManager.Add(new CGUIWindowMusicTop100);            // window id = 503

  m_gWindowManager.Add(new CGUIDialogSelect);             // window id = 2000
  m_gWindowManager.Add(new CGUIWindowMusicInfo);                // window id = 2001
  m_gWindowManager.Add(new CGUIDialogOK);                 // window id = 2002
  m_gWindowManager.Add(new CGUIWindowVideoInfo);                // window id = 2003
  m_gWindowManager.Add(new CGUIWindowScriptsInfo);              // window id = 2004
  m_gWindowManager.Add(new CGUIWindowFullScreen);         // window id = 2005
  m_gWindowManager.Add(new CGUIWindowVisualisation);      // window id = 2006
  m_gWindowManager.Add(new CGUIWindowSlideShow);          // window id = 2007
  m_gWindowManager.Add(new CGUIDialogFileStacking);       // window id = 2008

    m_gWindowManager.Add(new CGUIWindowOSD);                // window id = 2901
  m_gWindowManager.Add(new CGUIWindowScreensaver);        // window id = 2900 Screensaver
  m_gWindowManager.Add(new CGUIWindowWeather);                // window id = 2600 WEATHER
  m_gWindowManager.Add(new CGUIWindowBuddies);                // window id = 2700 BUDDIES

  m_gWindowManager.Add(new CGUIWindow(WINDOW_STARTUP, "Startup.xml"));  // startup window (id 2999)
  /* window id's 3000 - 3100 are reserved for python */
  g_DownloadManager.Initialize();

  m_ctrDpad.SetDelays(100, 500); //g_stSettings.m_iMoveDelayController, g_stSettings.m_iRepeatDelayController);

  SAFE_DELETE(m_splash);

  if (g_guiSettings.GetBool("masterlock.startuplock") && g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_settings.m_vecProfiles[0].getLockCode().IsEmpty()) 
  	g_passwordManager.CheckStartUpLock();

  // check if we should use the login screen
  if (g_settings.bUseLoginScreen)
  {
    m_gWindowManager.ActivateWindow(WINDOW_LOGIN_SCREEN);
  }
  else 
  {
    RESOLUTION res = INVALID;
    CStdString startupPath = g_SkinInfo.GetSkinPath("startup.xml", &res);
    int startWindow = g_guiSettings.GetInt("lookandfeel.startupwindow");
    // test for a startup window, and activate that instead of home
    if (CFile::Exists(startupPath) && (!g_SkinInfo.OnlyAnimateToHome() || startWindow == WINDOW_HOME))
    {
      m_gWindowManager.ActivateWindow(WINDOW_STARTUP);
    }
    else
    {
      // We need to Popup the WindowHome to initiate the GUIWindowManger for MasterCode popup dialog!
      // Then we can start the StartUpWindow! To prevent BlackScreen if the target Window is Protected with MasterCode!
      m_gWindowManager.ActivateWindow(WINDOW_HOME);
      if (startWindow != WINDOW_HOME)
        m_gWindowManager.ActivateWindow(startWindow);
    }
  }
  /* setup netowork based on our settings */
  /* network will start it's init procedure */
  g_network.Initialize(g_guiSettings.GetInt("network.assignment"),
    g_guiSettings.GetString("network.ipaddress").c_str(),
    g_guiSettings.GetString("network.subnet").c_str(),
    g_guiSettings.GetString("network.gateway").c_str(),
    g_guiSettings.GetString("network.dns").c_str());

  g_pythonParser.bStartup = true;
  
  CLog::Log(LOGINFO, "removing tempfiles");
  CUtil::RemoveTempFiles();

  if (!m_bAllSettingsLoaded)
  {
    CLog::Log(LOGWARNING, "settings not correct, show dialog");
    CStdString test;
    CUtil::GetHomePath(test);
    CGUIDialogOK *dialog = (CGUIDialogOK *)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
    if (dialog)
    {
      dialog->SetHeading(279);
      dialog->SetLine(0, "Error while loading settings");
      dialog->SetLine(1, test);
      dialog->SetLine(2, "");;
      dialog->DoModal();
    }
  }

  //  Show mute symbol
  if (g_stSettings.m_nVolumeLevel == VOLUME_MINIMUM)
    Mute();

  // if the user shutoff the xbox during music scan
  // restore the settings
  if (g_stSettings.m_bMyMusicIsScanning)
  {
    CLog::Log(LOGWARNING,"System rebooted during music scan! ... restoring UseTags and FindRemoteThumbs");
    RestoreMusicScanSettings();
  }

  m_slowTimer.StartZero();

  CLog::Log(LOGNOTICE, "initialize done");

  m_bInitializing = false;

  // final check for debugging combo
  CheckForDebugButtonCombo();
  return S_OK;
}

void CApplication::PrintXBEToLCD(const char* xbePath)
{
  int pLine = 0;
  CStdString strXBEName;
  if (!CUtil::GetXBEDescription(xbePath, strXBEName))
  {
    CUtil::GetDirectoryName(xbePath, strXBEName);
    CUtil::ShortenFileName(strXBEName);
    CUtil::RemoveIllegalChars(strXBEName);
  }
  // crop to LCD screen size
  if ((int)strXBEName.size() > g_advancedSettings.m_lcdColumns)
    strXBEName = strXBEName.Left(g_advancedSettings.m_lcdColumns);
  if (g_lcd)
  {
    g_lcd->SetLine(pLine++, "");
    g_lcd->SetLine(pLine++, "Playing");
    g_lcd->SetLine(pLine++, strXBEName);
    g_lcd->SetLine(pLine++, "");
  }
}

void CApplication::StartWebServer()
{
  if (g_guiSettings.GetBool("servers.webserver") && g_network.IsAvailable() )
  {
    CLog::Log(LOGNOTICE, "Webserver: Starting...");
    CSectionLoader::Load("LIBHTTP");
    m_pWebServer = new CWebServer();
    m_pWebServer->Start(g_network.m_networkinfo.ip, atoi(g_guiSettings.GetString("servers.webserverport")), "Q:\\web", false);
  }
}

void CApplication::StopWebServer()
{
  if (m_pWebServer)
  {
    CLog::Log(LOGNOTICE, "Webserver: Stopping...");
    m_pWebServer->Stop();
    delete m_pWebServer;
    m_pWebServer = NULL;
    CSectionLoader::Unload("LIBHTTP");
    CLog::Log(LOGNOTICE, "Webserver: Stopped...");
  }
}

void CApplication::StartFtpServer()
{
  if ( g_guiSettings.GetBool("servers.ftpserver") && g_network.IsAvailable() )
  {
    CLog::Log(LOGNOTICE, "XBFileZilla: Starting...");
    if (!m_pFileZilla)
    {
      // if user didn't upgrade properly...
      // check whether P:\\FileZilla Server.xml exists (UserData/FileZilla Server.xml)
      if (CFile::Exists("P:\\FileZilla Server.xml"))
      {
        m_pFileZilla = new CXBFileZilla("P:\\");
        // GeminiServer!
        // We need to set the FTP Password from the GUI to the XML! To be sure this is the right one for the internal settings! (Pass can be changed via XML!)
        // The user must always set the Password within the GUI Settings!
        // Todo: After v2.0! FTP Server Usermanager [Only Create/Delete FTP-USERS!]
        // Seems this would slow the startup of XBMC! ? What should we do!
        CUtil::SetFTPServerUserPassword(g_guiSettings.GetString("servers.ftpserveruser").c_str(),g_guiSettings.GetString("servers.ftpserverpassword").c_str()); 
      }
      else
      {
        m_pFileZilla = new CXBFileZilla("Q:\\System\\");
        m_pFileZilla->Start(false);
        CUtil::SetFTPServerUserPassword(g_guiSettings.GetString("servers.ftpserveruser").c_str(),g_guiSettings.GetString("servers.ftpserverpassword").c_str()); 
      }
    }
    //CLog::Log(LOGNOTICE, "XBFileZilla: Started");
  }
}

void CApplication::StopFtpServer()
{
  if (m_pFileZilla)
  {
    CLog::Log(LOGINFO, "XBFileZilla: Stopping...");
    
    std::vector<SXFConnection> mConnections;
    std::vector<SXFConnection>::iterator it;

    m_pFileZilla->GetAllConnections(mConnections);

    for(it = mConnections.begin();it != mConnections.end();it++)
    {
      m_pFileZilla->CloseConnection(it->mId);
    }

    m_pFileZilla->Stop();
    delete m_pFileZilla;
    m_pFileZilla = NULL;

    CLog::Log(LOGINFO, "XBFileZilla: Stopped");
  }

}

void CApplication::StartTimeServer()
{
  if (g_guiSettings.GetBool("xbdatetime.timeserver") && g_network.IsAvailable() )
  {
    if( !m_psntpClient )
    {
      CSectionLoader::Load("SNTP");
      CLog::Log(LOGNOTICE, "start timeserver client");

      m_psntpClient = new CSNTPClient();
      m_psntpClient->Create();
    }
  }
}

void CApplication::StopTimeServer()
{
  if( m_psntpClient )
  {
    CLog::Log(LOGNOTICE, "stop time server client");
    m_psntpClient->StopThread();
    SAFE_DELETE(m_psntpClient);
    CSectionLoader::Unload("SNTP");
  }
}

void CApplication::StartKai()
{
  if (g_guiSettings.GetBool("xlinkkai.enabled"))
  {
    CGUIWindowBuddies *pKai = (CGUIWindowBuddies*)m_gWindowManager.GetWindow(WINDOW_BUDDIES);
    if (pKai)
    {
      CLog::Log(LOGNOTICE, "starting kai");
      CKaiClient::GetInstance()->SetObserver(pKai);
    }
  }
}

void CApplication::StartUPnP()
{
    if (g_guiSettings.GetBool("upnp.autostart"))
    {
        CLog::Log(LOGNOTICE, "starting upnp");
        CUPnP::GetInstance();
    }
}

void CApplication::StopKai()
{
  if (CKaiClient::IsInstantiated())
  {
    CLog::Log(LOGNOTICE, "stopping kai");
    CKaiClient::GetInstance()->RemoveObserver();
    CKaiClient::RemoveInstance();
  }
}

void CApplication::StopUPnP()
{
    if (CUPnP::IsInstantiated())
    {
        CLog::Log(LOGNOTICE, "stopping upnp");
        CUPnP::ReleaseInstance();
    }
}

void CApplication::StartLEDControl(bool switchoff)
{
  if (switchoff && g_guiSettings.GetInt("led.colour") != LED_COLOUR_NO_CHANGE)
  {
    if ( (IsPlayingVideo()) && g_guiSettings.GetInt("led.disableonplayback") == LED_PLAYBACK_VIDEO)
    {
      //CLog::Log(LOGNOTICE, "LED Control: Playing Video LED is switched OFF!");
      ILED::CLEDControl(LED_COLOUR_OFF);
    }
    if ( (IsPlayingAudio()) && g_guiSettings.GetInt("led.disableonplayback") == LED_PLAYBACK_MUSIC)
    {
      //CLog::Log(LOGNOTICE, "LED Control: Playing Music LED is switched OFF!");
      ILED::CLEDControl(LED_COLOUR_OFF);
    }
    if ( ((IsPlayingVideo() || IsPlayingAudio())) && g_guiSettings.GetInt("led.disableonplayback") == LED_PLAYBACK_VIDEO_MUSIC)
    {
      //CLog::Log(LOGNOTICE, "LED Control: Playing Video Or Music LED is switched OFF!");
      ILED::CLEDControl(LED_COLOUR_OFF);
    }
  }
  else if (!switchoff)
  {
    ILED::CLEDControl(g_guiSettings.GetInt("led.colour"));
  }
}

void CApplication::DimLCDOnPlayback(bool dim)
{
  if(g_lcd && dim && (g_guiSettings.GetInt("lcd.disableonplayback") != LED_PLAYBACK_OFF) && (g_guiSettings.GetInt("lcd.type") != LCD_TYPE_NONE))
  {
    if ( (IsPlayingVideo()) && g_guiSettings.GetInt("lcd.disableonplayback") == LED_PLAYBACK_VIDEO)
    {
      //CLog::Log(LOGNOTICE, "LCD Control: Playing Video LCD is switched OFF!");
      g_lcd->SetBackLight(0);
    }
    if ( (IsPlayingAudio()) && g_guiSettings.GetInt("lcd.disableonplayback") == LED_PLAYBACK_MUSIC)
    {
      //CLog::Log(LOGNOTICE, "LCD Control: Playing Music LCD is switched OFF!");
      g_lcd->SetBackLight(0);
    }
    if ( ((IsPlayingVideo() || IsPlayingAudio())) && g_guiSettings.GetInt("lcd.disableonplayback") == LED_PLAYBACK_VIDEO_MUSIC)
    {
      //CLog::Log(LOGNOTICE, "LCD Control: Playing Video Or Music LCD is switched OFF!");
      g_lcd->SetBackLight(0);
    }
  }
  else if(!dim) 
  {
    g_lcd->SetBackLight(g_guiSettings.GetInt("lcd.backlight"));
  }
}

void CApplication::StartServices()
{
  CheckDate();
  StartLEDControl(false);
    
  // Start Thread for DVD Mediatype detection
  CLog::Log(LOGNOTICE, "start dvd mediatype detection");
  m_DetectDVDType.Create( false);

  CLog::Log(LOGNOTICE, "initializing playlistplayer");
  g_playlistPlayer.SetRepeat(PLAYLIST_MUSIC, g_stSettings.m_bMyMusicPlaylistRepeat ? PLAYLIST::REPEAT_ALL : PLAYLIST::REPEAT_NONE);
  g_playlistPlayer.SetShuffle(PLAYLIST_MUSIC, g_stSettings.m_bMyMusicPlaylistShuffle);
  g_playlistPlayer.SetRepeat(PLAYLIST_MUSIC_TEMP, g_guiSettings.GetBool("musicfiles.repeat") ? PLAYLIST::REPEAT_ALL : PLAYLIST::REPEAT_NONE);
  g_playlistPlayer.SetRepeat(PLAYLIST_VIDEO, g_stSettings.m_bMyVideoPlaylistRepeat ? PLAYLIST::REPEAT_ALL : PLAYLIST::REPEAT_NONE);
  g_playlistPlayer.SetShuffle(PLAYLIST_VIDEO, g_stSettings.m_bMyVideoPlaylistShuffle);
  g_playlistPlayer.SetRepeat(PLAYLIST_VIDEO_TEMP, PLAYLIST::REPEAT_NONE);

  CLCDFactory factory;
  g_lcd = factory.Create();
  if (g_lcd)
  {
    g_lcd->Initialize();
  }

  if (g_guiSettings.GetBool("system.autotemperature"))
  {
    CLog::Log(LOGNOTICE, "start fancontroller");
    CFanController::Instance()->Start(g_guiSettings.GetInt("system.targettemperature"));
  }
  else if (g_guiSettings.GetBool("system.fanspeedcontrol"))
  {
    CLog::Log(LOGNOTICE, "setting fanspeed");
    CFanController::Instance()->SetFanSpeed(g_guiSettings.GetInt("system.fanspeed"));
  }
}
void CApplication::CheckDate()
{
  CLog::Log(LOGNOTICE, "Checking the Date!");	//GeminiServer Date Check
	// Check the Date: Year, if it is  above 2099 set to 2004!
	SYSTEMTIME CurTime;
	SYSTEMTIME NewTime;
	GetLocalTime(&CurTime);
	GetLocalTime(&NewTime);
	CLog::Log(LOGINFO, "- Current Date is: %i-%i-%i",CurTime.wDay, CurTime.wMonth, CurTime.wYear);
	if ((CurTime.wYear > 2099) || (CurTime.wYear < 2001) )	// XBOX MS Dashboard also uses min/max DateYear 2001/2099 !!
	{
		CLog::Log(LOGNOTICE, "- The Date is Wrong: Setting New Date!");
		NewTime.wYear		= 2004;	// 2004
		NewTime.wMonth		= 1;	// January
		NewTime.wDayOfWeek	= 1;	// Monday
		NewTime.wDay		= 5;	// Monday 05.01.2004!!	
		NewTime.wHour		= 12;
		NewTime.wMinute		= 0;

		FILETIME stNewTime, stCurTime;
		SystemTimeToFileTime(&NewTime, &stNewTime);
		SystemTimeToFileTime(&CurTime, &stCurTime);
		NtSetSystemTime(&stNewTime, &stCurTime);	// Set a Default Year 2004!

		CLog::Log(LOGNOTICE, "- New Date is now: %i-%i-%i",NewTime.wDay, NewTime.wMonth, NewTime.wYear);
	}
	return ;
}
void CApplication::StopServices()
{
  g_network.NetworkMessage(CNetwork::SERVICES_DOWN, 0);

  CLog::Log(LOGNOTICE, "stop dvd detect media");
  m_DetectDVDType.StopThread();

  CLog::Log(LOGNOTICE, "stop fancontroller");
  CFanController::Instance()->Stop();
  CFanController::RemoveInstance();
}

void CApplication::DelayLoadSkin()
{
  m_dwSkinTime = timeGetTime() + 2000;
  return ;
}

void CApplication::CancelDelayLoadSkin()
{
  m_dwSkinTime = 0;
}

void CApplication::LoadSkin(const CStdString& strSkin)
{
  bool bPreviousPlayingState=false;
  bool bPreviousRenderingState=false;
  if (g_application.m_pPlayer && g_application.IsPlayingVideo())
  {
    bPreviousPlayingState = !g_application.m_pPlayer->IsPaused();
    if (bPreviousPlayingState)
      g_application.m_pPlayer->Pause();
    if (!g_renderManager.Paused())
    {
      if (m_gWindowManager.GetActiveWindow() == WINDOW_FULLSCREEN_VIDEO)
     {
        m_gWindowManager.ActivateWindow(WINDOW_HOME);
        bPreviousRenderingState = true;
      }
    }
  }
  // close the music and video overlays (they're re-opened automatically later)
  CSingleLock lock(g_graphicsContext);

  m_dwSkinTime = 0;

  CStdString strHomePath;
  CStdString strSkinPath = "Q:\\skin\\";
  strSkinPath += strSkin;

  CLog::Log(LOGINFO, "  load skin from:%s", strSkinPath.c_str());

  // save the current window details
  int currentWindow = m_gWindowManager.GetActiveWindow();
  vector<DWORD> currentModelessWindows;
  m_gWindowManager.GetActiveModelessWindows(currentModelessWindows);

  //  When the app is started the instance of the
  //  kai client should not be created until the
  //  skin is loaded the first time, but we must
  //  disconnect from the engine when the skin is
  //  changed
  bool bKaiConnected = false;
  if (!m_bInitializing && g_guiSettings.GetBool("xlinkkai.enabled"))
  {
    bKaiConnected = CKaiClient::GetInstance()->IsEngineConnected();
    if (bKaiConnected)
    {
      CLog::Log(LOGINFO, " Disconnecting Kai...");
      CKaiClient::GetInstance()->RemoveObserver();
    }
  }

  CLog::Log(LOGINFO, "  delete old skin...");
  UnloadSkin();

  // Load in the skin.xml file if it exists
  g_SkinInfo.Load(strSkinPath);

  CLog::Log(LOGINFO, "  load fonts for skin...");
  g_graphicsContext.SetMediaDir(strSkinPath);
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
      CLog::Log(LOGERROR, "    no ttf font found, but needed for the language %s.", g_guiSettings.GetString("lookandfeel.language").c_str());
  }
  g_fontManager.LoadFonts(g_guiSettings.GetString("lookandfeel.font"));

  LARGE_INTEGER start;
  QueryPerformanceCounter(&start);

  CLog::Log(LOGINFO, "  load new skin...");
  CGUIWindowHome *pHome = (CGUIWindowHome *)m_gWindowManager.GetWindow(WINDOW_HOME);
  if (!g_SkinInfo.Check(strSkinPath) || !pHome || !pHome->Load("home.xml"))
  {
    // failed to load home.xml
    // fallback to default skin
    if ( strcmpi(strSkin.c_str(), "Project Mayhem III") != 0)
    {
      CLog::Log(LOGERROR, "failed to load home.xml for skin:%s, fallback to \"Project Mayhem III\" skin", strSkin.c_str());
      LoadSkin("Project Mayhem III");
      return ;
    }
  }

  // Load the user windows
  LoadUserWindows(strSkinPath);

  LARGE_INTEGER end, freq;
  QueryPerformanceCounter(&end);
  QueryPerformanceFrequency(&freq);
  CLog::DebugLog("Load Skin XML: %.2fms", 1000.f * (end.QuadPart - start.QuadPart) / freq.QuadPart);

  CLog::Log(LOGINFO, "  initialize new skin...");
  m_guiPointer.AllocResources(true);
  m_guiMusicOverlay.AllocResources(true);
  m_guiVideoOverlay.AllocResources(true);
  m_guiDialogVolumeBar.AllocResources(true);
  m_guiDialogSeekBar.AllocResources(true);
  m_guiDialogKaiToast.AllocResources(true);
  m_guiDialogMuteBug.AllocResources(true);
  m_gWindowManager.AddMsgTarget(this);
  m_gWindowManager.AddMsgTarget(&g_playlistPlayer);
  m_gWindowManager.SetCallback(*this);
  m_gWindowManager.Initialize();
  g_audioManager.Initialize(CAudioContext::DEFAULT_DEVICE);
  g_audioManager.Load();
  CLog::Log(LOGINFO, "  skin loaded...");

  if (bKaiConnected)
  {
    CLog::Log(LOGINFO, " Reconnecting Kai...");

    CGUIWindowBuddies *pKai = (CGUIWindowBuddies *)m_gWindowManager.GetWindow(WINDOW_BUDDIES);
    CKaiClient::GetInstance()->SetObserver(pKai);
    Sleep(3000);  //  The client need some time to "resync"
  }

  // restore windows
  if (currentWindow != WINDOW_INVALID)
  {
    m_gWindowManager.ActivateWindow(currentWindow);
    for (unsigned int i = 0; i < currentModelessWindows.size(); i++)
    {
      CGUIDialog *dialog = (CGUIDialog *)m_gWindowManager.GetWindow(currentModelessWindows[i]);
      if (dialog) dialog->Show();
    }
  }

  if (g_application.m_pPlayer && g_application.IsPlayingVideo())
  {
    lock.Leave();
    if (bPreviousPlayingState)
      g_application.m_pPlayer->Pause();
    if (bPreviousRenderingState)
      m_gWindowManager.ActivateWindow(WINDOW_FULLSCREEN_VIDEO);
  }
}

void CApplication::UnloadSkin()
{
  g_audioManager.DeInitialize(CAudioContext::DEFAULT_DEVICE);

  m_gWindowManager.DeInitialize();

  //These windows are not handled by the windowmanager (why not?) so we should unload them manually
  CGUIMessage msg(GUI_MSG_WINDOW_DEINIT, 0, 0);
  m_guiPointer.OnMessage(msg);
  m_guiPointer.ResetControlStates();
  m_guiPointer.FreeResources(true);
  m_guiDialogMuteBug.OnMessage(msg);
  m_guiDialogMuteBug.ResetControlStates();
  m_guiDialogMuteBug.FreeResources(true);
  m_guiVideoOverlay.OnMessage(msg);
  m_guiVideoOverlay.ResetControlStates();
  m_guiVideoOverlay.FreeResources(true); 	 
  m_guiMusicOverlay.OnMessage(msg);
  m_guiMusicOverlay.ResetControlStates();
  m_guiMusicOverlay.FreeResources(true);

  CGUIWindow::FlushReferenceCache(); // flush the cache

  g_TextureManager.Cleanup();

  g_fontManager.Clear();

  g_charsetConverter.reset();
}

bool CApplication::LoadUserWindows(const CStdString& strSkinPath)
{
  WIN32_FIND_DATA FindFileData;
  WIN32_FIND_DATA NextFindFileData;
  HANDLE hFind;
  TiXmlDocument xmlDoc;
  RESOLUTION resToUse = INVALID;

  // Load from wherever home.xml is
  g_SkinInfo.GetSkinPath("home.xml", &resToUse);

  CStdString strLoadPath;
  strLoadPath.Format("%s%s", strSkinPath, g_SkinInfo.GetDirFromRes(resToUse));

  CStdString strPath;
  strPath.Format("%s\\%s", strLoadPath, "custom*.xml");
  CLog::Log(LOGINFO, "Loading user windows %s", strPath.c_str());
  hFind = FindFirstFile(strPath.c_str(), &NextFindFileData);

  CStdString strFileName;
  while (hFind != INVALID_HANDLE_VALUE)
  {
    FindFileData = NextFindFileData;

    if (!FindNextFile(hFind, &NextFindFileData))
    {
      FindClose(hFind);
      hFind = INVALID_HANDLE_VALUE;
    }

    // skip "up" directories, which come in all queries
    if (!_tcscmp(FindFileData.cFileName, _T(".")) || !_tcscmp(FindFileData.cFileName, _T("..")))
      continue;

    strFileName.Format("%s\\%s", strLoadPath.c_str(), FindFileData.cFileName);
    CLog::Log(LOGINFO, "Loading skin file: %s", strFileName.c_str());
    if (!xmlDoc.LoadFile(strFileName.c_str()))
    {
      CLog::Log(LOGERROR, "unable to load:%s, Line %d\n%s", strFileName.c_str(), xmlDoc.ErrorRow(), xmlDoc.ErrorDesc());
      continue;
    }

    // Root element should be <window>
    TiXmlElement* pRootElement = xmlDoc.RootElement();
    CStdString strValue = pRootElement->Value();
    if (!strValue.Equals("window"))
    {
      CLog::Log(LOGERROR, "file :%s doesnt contain <window>", strFileName.c_str());
      continue;
    }

    // Read the <type> element to get the window type to create
    // If no type is specified, create a CGUIWindow as default
    CGUIWindow* pWindow = NULL;
    const TiXmlNode *pType = pRootElement->FirstChild("type");
    if (!pType || !pType->FirstChild())
    {
      pWindow = new CGUIStandardWindow();
    }
    else
    {
      CStdString strType = pType->FirstChild()->Value();
      if (strType == "dialog")
      {
        pWindow = new CGUIDialog(0, "");
      }
      else if (strType == "subMenu")
      {
        pWindow = new CGUIDialogSubMenu();
      }
      else if (strType == "buttonMenu")
      {
        pWindow = new CGUIDialogButtonMenu();
      }
      else
      {
        pWindow = new CGUIStandardWindow();
      }
    }
    pType = pRootElement->FirstChild("id");

    // Check to make sure the pointer isn't still null
    if (pWindow == NULL || !pType || !pType->FirstChild())
    {
      CLog::Log(LOGERROR, "Out of memory / Failed to create new object in LoadUserWindows");
      return false;
    }
    // set the window's xml file, and add it to the window manager.
    pWindow->SetXMLFile(FindFileData.cFileName);
    pWindow->SetID(WINDOW_HOME + atol(pType->FirstChild()->Value()));
    m_gWindowManager.AddCustomWindow(pWindow);
  }

  return true;
}

void CApplication::Render()
{
  // update sound
  if (m_pPlayer)
  {
    m_pPlayer->DoAudioWork();
  }
  // process karaoke
  if (m_pCdgParser)
    m_pCdgParser->ProcessVoice();

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

  // don't do anything that would require graphiccontext to be locked before here in fullscreen.
  // that stuff should go into renderfullscreen instead as that is called from the renderin thread

  // dont show GUI when playing full screen video
  if (m_gWindowManager.GetActiveWindow() == WINDOW_FULLSCREEN_VIDEO)
  {
    if ( g_graphicsContext.IsFullScreenVideo() )
    {
      if (m_pPlayer)
      {
        if (m_pPlayer->IsPaused())
        {
          CSingleLock lock(g_graphicsContext);
          extern void xbox_video_render_update(bool);
          m_gWindowManager.UpdateModelessVisibility();
          xbox_video_render_update(true);
          m_pd3dDevice->BlockUntilVerticalBlank();
          m_pd3dDevice->Present( NULL, NULL, NULL, NULL );
          return ;
        }
      }
      Sleep(50);
      ResetScreenSaver();
      return ;
    }
  }

  // enable/disable video overlay window
  if (IsPlayingVideo() && m_gWindowManager.GetActiveWindow() != WINDOW_FULLSCREEN_VIDEO && !m_bScreenSave)
  {
    g_graphicsContext.EnablePreviewWindow(true);
  }
  else
  {
    g_graphicsContext.EnablePreviewWindow(false);
  }
  // update our FPS
  g_infoManager.UpdateFPS();

  g_graphicsContext.Lock();

  m_gWindowManager.UpdateModelessVisibility();

  // check if we're playing a file
  if (!m_bScreenSave && m_gWindowManager.IsOverlayAllowed())
  {
    // if we're playing a movie
    if ( IsPlayingVideo() && m_gWindowManager.GetActiveWindow() != WINDOW_FULLSCREEN_VIDEO)
    {
      // then show video overlay window
      m_guiVideoOverlay.Show();
      m_guiMusicOverlay.Close(true);
    }
    else if ( IsPlayingAudio() )
    {
      // audio show audio overlay window
      m_guiMusicOverlay.Show();
      m_guiVideoOverlay.Close(true);
    }
    else
    {
      m_guiMusicOverlay.Close(true);
      m_guiVideoOverlay.Close(true);
    }
  }
  else
  {
    m_guiMusicOverlay.Close(true);
    m_guiVideoOverlay.Close(true);
  }

  // draw GUI
  g_graphicsContext.Clear();
  //SWATHWIDTH of 4 improves fillrates (performance investigator)
  m_pd3dDevice->SetRenderState(D3DRS_SWATHWIDTH, 4);
  m_gWindowManager.Render();

  {
    // if we're recording an audio stream then show blinking REC
    if (IsPlayingAudio())
    {
      if (m_pPlayer->IsRecording() )
      {
        static iBlinkRecord = 0;
        CGUIFont* pFont = g_fontManager.GetFont("font13");
        if (pFont)
        {
          iBlinkRecord++;
          if (iBlinkRecord > 25)
            pFont->DrawText(60, 50, 0xffff0000, 0, L"REC"); //Draw REC in RED
          if (iBlinkRecord > 50)
            iBlinkRecord = 0;
        }
      }
    }
  }

  // Now render any dialogs
  m_gWindowManager.RenderDialogs();

  // Render the mouse pointer
  if (g_Mouse.IsActive())
  {
    m_guiPointer.Render();
  }

  {
    // free memory if we got les then 10megs free ram
    MEMORYSTATUS stat;
    GlobalMemoryStatus(&stat);
    DWORD dwMegFree = stat.dwAvailPhys / (1024 * 1024);
    if (dwMegFree <= 10)
    {
      g_TextureManager.Flush();
    }

    // reset image scaling and effect states
    g_graphicsContext.SetScalingResolution(g_graphicsContext.GetVideoResolution(), 0, 0, false);

    // If we have the remote codes enabled, then show them
    if (g_advancedSettings.m_displayRemoteCodes)
    {
      XBIR_REMOTE* pRemote = &m_DefaultIR_Remote;
      static iRemoteCode = 0;
      static iShowRemoteCode = 0;
      if (pRemote->wButtons)
      {
        iRemoteCode = 255 - pRemote->wButtons; // remote OBC code is 255-wButtons
        iShowRemoteCode = 50;
      }
      if (iShowRemoteCode > 0)
      {
        CStdStringW wszText;
        wszText.Format(L"Remote Code: %i", iRemoteCode);
        CGUIFont* pFont = g_fontManager.GetFont("font13");
        if (pFont)
        {
#ifdef _DEBUG
          pFont->DrawOutlineText( 0.08f * g_graphicsContext.GetWidth(), 0.12f * g_graphicsContext.GetHeight(), 0xffffffff, 0xff000000, 2, wszText);
#else

          if (LOG_LEVEL_DEBUG_FREEMEM <= g_advancedSettings.m_logLevel)
            pFont->DrawOutlineText( 0.08f * g_graphicsContext.GetWidth(), 0.12f * g_graphicsContext.GetHeight(), 0xffffffff, 0xff000000, 2, wszText);
          else
            pFont->DrawOutlineText( 0.08f * g_graphicsContext.GetWidth(), 0.08f * g_graphicsContext.GetHeight(), 0xffffffff, 0xff000000, 2, wszText);
#endif
        }
        iShowRemoteCode--;
      }
    }

    RenderMemoryStatus();
  }

  // Present the backbuffer contents to the display
  m_pd3dDevice->Present( NULL, NULL, NULL, NULL );
  g_graphicsContext.Unlock();
  
}

void CApplication::RenderMemoryStatus()
{
#if !defined(_DEBUG) && !defined(PROFILE)
  if (LOG_LEVEL_DEBUG_FREEMEM <= g_advancedSettings.m_logLevel)
#endif
  {
    // reset the window scaling and fade status
    g_graphicsContext.SetScalingResolution(g_graphicsContext.GetVideoResolution(), 0, 0, false);

    CStdStringW wszText;
    MEMORYSTATUS stat;
    GlobalMemoryStatus(&stat);
    wszText.Format(L"FPS %2.2f, FreeMem %d/%d", g_infoManager.GetFPS(), stat.dwAvailPhys, stat.dwTotalPhys);

    CGUIFont* pFont = g_fontManager.GetFont("font13");
    if (pFont)
    {
      float x = 0.08f * g_graphicsContext.GetWidth();
      float y = 0.08f * g_graphicsContext.GetHeight();
      pFont->DrawOutlineText(x, y, 0xffffffff, 0xff000000, 2, wszText);
    }
  }
}

// OnKey() translates the key into a CAction which is sent on to our Window Manager.
// The window manager will return true if the event is processed, false otherwise.
// If not already processed, this routine handles global keypresses.  It returns
// true if the key has been processed, false otherwise.

bool CApplication::OnKey(CKey& key)
{
  // Turn the mouse off, as we've just got a keypress from controller or remote
  g_Mouse.SetInactive();
  CAction action;

  // a key has been pressed.
  // Reset the screensaver timer
  // but not for the analog thumbsticks/triggers
  if (!key.IsAnalogButton())
  {
    // reset harddisk spindown timer
    m_bSpinDown = false;
    m_bNetworkSpinDown = false;
    
    // reset Idle Timer
    m_idleTimer.StartZero();

    ResetScreenSaver();
    if (ResetScreenSaverWindow())
      return true;
  }

  // get the current active window
  int iWin = m_gWindowManager.GetActiveWindow() & WINDOW_ID_MASK;
  // change this if we have a dialog up
  if (m_gWindowManager.IsRouted())
  {
    iWin = m_gWindowManager.GetTopMostRoutedWindowID() & WINDOW_ID_MASK;
  }
  if (iWin == WINDOW_FULLSCREEN_VIDEO)
  {
    // current active window is full screen video.
    if (g_application.m_pPlayer && g_application.m_pPlayer->IsInMenu())
    {
      // if player is in some sort of menu, (ie DVDMENU) map buttons differently
      g_buttonTranslator.GetAction(WINDOW_VIDEO_MENU, key, action);
    }
    else
    {
      // no then use the fullscreen window section of keymap.xml to map key->action
      g_buttonTranslator.GetAction(iWin, key, action);
    }
  }
  else
  {
    // current active window isnt the fullscreen window
    // just use corresponding section from keymap.xml
    // to map key->action
    if (key.FromKeyboard() && (iWin == WINDOW_DIALOG_KEYBOARD || iWin == WINDOW_BUDDIES) )
    {
      if (key.GetFromHttpApi())
      {
        if (key.GetButtonCode() != KEY_INVALID)
          action.wID = (WORD) key.GetButtonCode();
      }
      else 
    // see if we've got an ascii key
    if (g_Keyboard.GetAscii() != 0)
      action.wID = (WORD)g_Keyboard.GetAscii() | KEY_ASCII;
   else
      action.wID = (WORD)g_Keyboard.GetKey() | KEY_VKEY;
    }
    else
      g_buttonTranslator.GetAction(iWin, key, action);
  }
  if (!key.IsAnalogButton())
    CLog::Log(LOGDEBUG, __FUNCTION__": %i pressed, action is %i", key.GetButtonCode(), action.wID);

  //  Play a sound based on the action
  g_audioManager.PlayActionSound(action);

  return OnAction(action);
}

bool CApplication::OnAction(const CAction &action)
{
  // special case for switching between GUI & fullscreen mode.
  if (action.wID == ACTION_SHOW_GUI)
  { // Switch to fullscreen mode if we can
    if (SwitchToFullScreen())
    {
      m_navigationTimer.StartZero();
      return true;
    }
  }

  // in normal case
  // just pass the action to the current window and let it handle it
  if (m_gWindowManager.OnAction(action))
  {
    m_navigationTimer.StartZero();
    return true;
  }

  /* handle extra global presses */

  // screenshot : take a screenshot :)
  if (action.wID == ACTION_TAKE_SCREENSHOT)
  {
    CUtil::TakeScreenshot();
    return true;
  }
  // built in functions : execute the built-in
  if (action.wID == ACTION_BUILT_IN_FUNCTION)
  {
    CUtil::ExecBuiltIn(action.strAction);
    m_navigationTimer.StartZero();
    return true;
  }

  // power down : turn off after 3 seconds of button down
  static bool PowerButtonDown = false;
  static DWORD PowerButtonCode;
  static DWORD MarkTime;
  if (action.wID == ACTION_POWERDOWN)
  {
    // Hold button for 3 secs to power down
    if (!PowerButtonDown)
    {
      MarkTime = GetTickCount();
      PowerButtonDown = true;
      PowerButtonCode = action.m_dwButtonCode;
    }
  }
  if (PowerButtonDown)
  {
    if (g_application.IsButtonDown(PowerButtonCode))
    {
      if (GetTickCount() >= MarkTime + 3000)
      {
        g_applicationMessenger.Shutdown();
        return true;
      }
    }
    else
      PowerButtonDown = false;
  }
  // show info : Shows the current video or song information
  if (action.wID == ACTION_SHOW_INFO)
  {
    g_infoManager.ToggleShowInfo();
    return true;
  }

  // codec info : Shows the current song, video or picture codec information
  if (action.wID == ACTION_SHOW_CODEC)
  {
    g_infoManager.ToggleShowCodec();
    return true;
  }

  // stop : stops playing current audio song
  if (action.wID == ACTION_STOP)
  {
    StopPlaying();
    return true;
  }

  // previous : play previous song from playlist
  if (action.wID == ACTION_PREV_ITEM)
  {
    // first check whether we're within 3 seconds of the start of the track
    // if not, we just revert to the start of the track
    if (m_pPlayer && m_pPlayer->CanSeek() && GetTime() > 3)
      SeekTime(0);
    else
      g_playlistPlayer.PlayPrevious();
    return true;
  }

  // next : play next song from playlist
  if (action.wID == ACTION_NEXT_ITEM)
  {
    if (IsPlaying() && m_pPlayer->SkipNext())
      return true;

    g_playlistPlayer.PlayNext();
    
    return true;
  }

  if ( IsPlaying())
  {
    // pause : pauses current audio song
    if (action.wID == ACTION_PAUSE)
    {
      m_pPlayer->Pause();
      if (!m_pPlayer->IsPaused())
      { // unpaused - set the playspeed back to normal
        SetPlaySpeed(1);
        return true;
      }
    }
    if (!m_pPlayer->IsPaused())
    {
      // if we do a FF/RW in my music then map PLAY action togo back to normal speed
      if (action.wID == ACTION_PLAYER_PLAY)
      {
        if (m_iPlaySpeed != 1)
        {
          SetPlaySpeed(1);
        }
        return true;
      }
      if (action.wID == ACTION_PLAYER_FORWARD || action.wID == ACTION_PLAYER_REWIND)
      {
/*        if (m_eCurrentPlayer == EPC_SIDPLAYER )
        {
          // sid uses these to track skip
          m_pPlayer->Seek(action.wID == ACTION_PLAYER_FORWARD);
          return true;
        }
        else
        {*/
          int iPlaySpeed = m_iPlaySpeed;
          if (action.wID == ACTION_PLAYER_REWIND && iPlaySpeed == 1) // Enables Rewinding
            iPlaySpeed *= -2;
          else if (action.wID == ACTION_PLAYER_REWIND && iPlaySpeed > 1) //goes down a notch if you're FFing
            iPlaySpeed /= 2;
          else if (action.wID == ACTION_PLAYER_FORWARD && iPlaySpeed < 1) //goes up a notch if you're RWing
            iPlaySpeed /= 2;
          else
            iPlaySpeed *= 2;

          if (action.wID == ACTION_PLAYER_FORWARD && iPlaySpeed == -1) //sets iSpeed back to 1 if -1 (didn't plan for a -1)
            iPlaySpeed = 1;
          if (iPlaySpeed > 32 || iPlaySpeed < -32)
            iPlaySpeed = 1;

          SetPlaySpeed(iPlaySpeed);
          return true;
        //}
      }
      else if ((action.fAmount1 || GetPlaySpeed() != 1) && (action.wID == ACTION_ANALOG_REWIND || action.wID == ACTION_ANALOG_FORWARD))
      {
        // calculate the speed based on the amount the button is held down
        int iPower = (int)(action.fAmount1 * MAX_FFWD_SPEED + 0.5f);
        // returns 0 -> MAX_FFWD_SPEED
        int iSpeed = 1 << iPower;
        if (iSpeed != 1 && action.wID == ACTION_ANALOG_REWIND)
          iSpeed = -iSpeed;
        g_application.SetPlaySpeed(iSpeed);
        if (iSpeed == 1)
          CLog::DebugLog("Resetting playspeed");
        return true;
      }
    }
    // allow play to unpause
    else
    {
      if (action.wID == ACTION_PLAYER_PLAY)
      {
        // unpause, and set the playspeed back to normal
        m_pPlayer->Pause();
        g_application.SetPlaySpeed(1);
        return true;
      }
    }
  }
  if (action.wID == ACTION_MUTE)
  {
    Mute();
    return true;
  }

  // Check for global volume control
  if (action.fAmount1 && (action.wID == ACTION_VOLUME_UP || action.wID == ACTION_VOLUME_DOWN))
  {
    // increase or decrease the volume
    int volume = g_stSettings.m_nVolumeLevel + g_stSettings.m_dynamicRangeCompressionLevel;

    // calculate speed so that a full press will equal 1 second from min to max
    float speed = float(VOLUME_MAXIMUM - VOLUME_MINIMUM);
    if( action.fRepeat )
      speed *= action.fRepeat;
    else
      speed /= 50; //50 fps

    if (action.wID == ACTION_VOLUME_UP)
      volume += (int)(action.fAmount1 * action.fAmount1 * speed);
    else
      volume -= (int)(action.fAmount1 * action.fAmount1 * speed);
    
    SetHardwareVolume(volume);

    // show visual feedback of volume change...
    m_guiDialogVolumeBar.Show();
    m_guiDialogVolumeBar.OnAction(action);
    return true;
  }
  // Check for global seek control
  if (IsPlaying() && action.fAmount1 && (action.wID == ACTION_ANALOG_SEEK_FORWARD || action.wID == ACTION_ANALOG_SEEK_BACK))
  {
    if (!m_pPlayer->CanSeek()) return false;
    CScrobbler::GetInstance()->SetSubmitSong(false);  // Do not submit songs to Audioscrobbler when seeking, see CheckAudioScrobblerStatus()
    m_guiDialogSeekBar.OnAction(action);
    return true;
  }
  return false;
}

void CApplication::SetKaiNotification(const CStdString& aCaption, const CStdString& aDescription, CGUIImage* aIcon/*=NULL*/)
{
  // queue toast notification
  if (g_guiSettings.GetBool("xlinkkai.enablenotifications"))
  {
    if (aIcon==NULL)
      m_guiDialogKaiToast.QueueNotification(aCaption, aDescription);
    else
      m_guiDialogKaiToast.QueueNotification(aIcon->GetFileName(), aCaption, aDescription);
  }
}

void CApplication::UpdateLCD()
{
  static lTickCount = 0;

  if (g_guiSettings.GetInt("lcd.type") == LCD_TYPE_NONE)
    return ;
  long lTimeOut = 1000;
  if ( m_iPlaySpeed != 1)
    lTimeOut = 0;
  if ( ((long)GetTickCount() - lTickCount) >= lTimeOut)
  {
    if (g_application.NavigationIdleTime() < 5)
      g_lcd->Render(ILCD::LCD_MODE_NAVIGATION);
    else if (IsPlayingVideo())
      g_lcd->Render(ILCD::LCD_MODE_VIDEO);
    else if (IsPlayingAudio())
      g_lcd->Render(ILCD::LCD_MODE_MUSIC);
    else
      g_lcd->Render(ILCD::LCD_MODE_GENERAL);

    // reset tick count
    lTickCount = GetTickCount();
  }
}

void CApplication::FrameMove()
{
  /* currently we calculate the repeat time (ie time from last similar keypress) just global as fps */
  float frameTime = m_frameTime.GetElapsedSeconds();
  m_frameTime.StartZero();

  /* never set a frametime less than 2 fps to avoid problems when debuggin and on breaks */
  if( frameTime > 0.5 ) frameTime = 0.5;

  if (g_guiSettings.GetBool("xlinkkai.enabled"))
  {
    CKaiClient::GetInstance()->DoWork();
  }

  // check if there are notifications to display
  if (m_guiDialogKaiToast.DoWork())
  {
    if (!m_guiDialogKaiToast.IsRunning())
    {
      m_guiDialogKaiToast.Show();
    }
  }

  if (g_lcd)
    UpdateLCD();

  // read raw input from controller, remote control, mouse and keyboard
  ReadInput();
  // process input actions
  ProcessMouse();
  ProcessHTTPApiButtons();
  ProcessKeyboard();
  ProcessRemote(frameTime);
  ProcessGamepad(frameTime);
}

bool CApplication::ProcessGamepad(float frameTime)
{
  // Handle the gamepad button presses.  We check for button down,
  // then call OnKey() which handles the translation to actions, and sends the
  // action to our window manager's OnAction() function, which filters the messages
  // to where they're supposed to end up, returning true if the message is successfully
  // processed.  If OnKey() returns false, then the key press wasn't processed at all,
  // and we can safely process the next key (or next check on the same key in the
  // case of the analog sticks which can produce more than 1 key event.)

  WORD wButtons = m_DefaultGamepad.wButtons;
  WORD wDpad = wButtons & (XINPUT_GAMEPAD_DPAD_UP | XINPUT_GAMEPAD_DPAD_DOWN | XINPUT_GAMEPAD_DPAD_LEFT | XINPUT_GAMEPAD_DPAD_RIGHT);

  BYTE bLeftTrigger = m_DefaultGamepad.bAnalogButtons[XINPUT_GAMEPAD_LEFT_TRIGGER];
  BYTE bRightTrigger = m_DefaultGamepad.bAnalogButtons[XINPUT_GAMEPAD_RIGHT_TRIGGER];
  BYTE bButtonA = m_DefaultGamepad.bAnalogButtons[XINPUT_GAMEPAD_A];
  BYTE bButtonB = m_DefaultGamepad.bAnalogButtons[XINPUT_GAMEPAD_B];
  BYTE bButtonX = m_DefaultGamepad.bAnalogButtons[XINPUT_GAMEPAD_X];
  BYTE bButtonY = m_DefaultGamepad.bAnalogButtons[XINPUT_GAMEPAD_Y];

  // pass them through the delay
  WORD wDir = m_ctrDpad.DpadInput(wDpad, 0 != bLeftTrigger, 0 != bRightTrigger);

  // map all controller & remote actions to their keys
  if (m_DefaultGamepad.fX1 || m_DefaultGamepad.fY1)
  {
    CKey key(KEY_BUTTON_LEFT_THUMB_STICK, bLeftTrigger, bRightTrigger, m_DefaultGamepad.fX1, m_DefaultGamepad.fY1, m_DefaultGamepad.fX2, m_DefaultGamepad.fY2, frameTime);
    if (OnKey(key)) return true;
  }
  if (m_DefaultGamepad.fX2 || m_DefaultGamepad.fY2)
  {
    CKey key(KEY_BUTTON_RIGHT_THUMB_STICK, bLeftTrigger, bRightTrigger, m_DefaultGamepad.fX1, m_DefaultGamepad.fY1, m_DefaultGamepad.fX2, m_DefaultGamepad.fY2, frameTime);
    if (OnKey(key)) return true;
  }
  // direction specific keys (for defining different actions for each direction)
  // We need to be able to know when it last had a direction, so that we can
  // post the reset direction code the next time around (to reset scrolling,
  // fastforwarding and other analog actions)

  // For the sticks, once it is pushed in one direction (eg up) it will only
  // detect movement in that direction of movement (eg up or down) - the other
  // direction (eg left and right) will not be registered until the stick has
  // been recentered for at least 2 frames.

  // first the right stick
  static lastRightStickKey = 0;
  int newRightStickKey = 0;
  if (lastRightStickKey == KEY_BUTTON_RIGHT_THUMB_STICK_UP || lastRightStickKey == KEY_BUTTON_RIGHT_THUMB_STICK_DOWN)
  {
    if (m_DefaultGamepad.fY2 > 0)
      newRightStickKey = KEY_BUTTON_RIGHT_THUMB_STICK_UP;
    else if (m_DefaultGamepad.fY2 < 0)
      newRightStickKey = KEY_BUTTON_RIGHT_THUMB_STICK_DOWN;
    else if (m_DefaultGamepad.fX2 != 0)
    {
      newRightStickKey = KEY_BUTTON_RIGHT_THUMB_STICK_UP;
      //m_DefaultGamepad.fY2 = 0.00001f; // small amount of movement
    }
  }
  else if (lastRightStickKey == KEY_BUTTON_RIGHT_THUMB_STICK_LEFT || lastRightStickKey == KEY_BUTTON_RIGHT_THUMB_STICK_RIGHT)
  {
    if (m_DefaultGamepad.fX2 > 0)
      newRightStickKey = KEY_BUTTON_RIGHT_THUMB_STICK_RIGHT;
    else if (m_DefaultGamepad.fX2 < 0)
      newRightStickKey = KEY_BUTTON_RIGHT_THUMB_STICK_LEFT;
    else if (m_DefaultGamepad.fY2 != 0)
    {
      newRightStickKey = KEY_BUTTON_RIGHT_THUMB_STICK_RIGHT;
      //m_DefaultGamepad.fX2 = 0.00001f; // small amount of movement
    }
  }
  else
  {
    if (m_DefaultGamepad.fY2 > 0 && m_DefaultGamepad.fX2*2 < m_DefaultGamepad.fY2 && -m_DefaultGamepad.fX2*2 < m_DefaultGamepad.fY2)
      newRightStickKey = KEY_BUTTON_RIGHT_THUMB_STICK_UP;
    else if (m_DefaultGamepad.fY2 < 0 && m_DefaultGamepad.fX2*2 < -m_DefaultGamepad.fY2 && -m_DefaultGamepad.fX2*2 < -m_DefaultGamepad.fY2)
      newRightStickKey = KEY_BUTTON_RIGHT_THUMB_STICK_DOWN;
    else if (m_DefaultGamepad.fX2 > 0 && m_DefaultGamepad.fY2*2 < m_DefaultGamepad.fX2 && -m_DefaultGamepad.fY2*2 < m_DefaultGamepad.fX2)
      newRightStickKey = KEY_BUTTON_RIGHT_THUMB_STICK_RIGHT;
    else if (m_DefaultGamepad.fX2 < 0 && m_DefaultGamepad.fY2*2 < -m_DefaultGamepad.fX2 && -m_DefaultGamepad.fY2*2 < -m_DefaultGamepad.fX2)
      newRightStickKey = KEY_BUTTON_RIGHT_THUMB_STICK_LEFT;
  }
  if (lastRightStickKey && newRightStickKey != lastRightStickKey)
  { // was held down last time - and we have a new key now
    // post old key reset message...
    CKey key(lastRightStickKey, 0, 0, 0, 0, 0, 0);
    lastRightStickKey = newRightStickKey;
    if (OnKey(key)) return true;
  }
  lastRightStickKey = newRightStickKey;
  // post the new key's message
  if (newRightStickKey)
  {
    CKey key(newRightStickKey, bLeftTrigger, bRightTrigger, m_DefaultGamepad.fX1, m_DefaultGamepad.fY1, m_DefaultGamepad.fX2, m_DefaultGamepad.fY2, frameTime);
    if (OnKey(key)) return true;
  }

  // now the left stick
  static lastLeftStickKey = 0;
  int newLeftStickKey = 0;
  if (lastLeftStickKey == KEY_BUTTON_LEFT_THUMB_STICK_UP || lastLeftStickKey == KEY_BUTTON_LEFT_THUMB_STICK_DOWN)
  {
    if (m_DefaultGamepad.fY1 > 0)
      newLeftStickKey = KEY_BUTTON_LEFT_THUMB_STICK_UP;
    else if (m_DefaultGamepad.fY1 < 0)
      newLeftStickKey = KEY_BUTTON_LEFT_THUMB_STICK_DOWN;
    /*else if (m_DefaultGamepad.fX1 != 0)
    {
      newLeftStickKey = KEY_BUTTON_LEFT_THUMB_STICK_UP;
      m_DefaultGamepad.fY1 = 0.00001f; // small amount of movement
    }*/
  }
  else if (lastLeftStickKey == KEY_BUTTON_LEFT_THUMB_STICK_LEFT || lastLeftStickKey == KEY_BUTTON_LEFT_THUMB_STICK_RIGHT)
  {
    if (m_DefaultGamepad.fX1 > 0)
      newLeftStickKey = KEY_BUTTON_LEFT_THUMB_STICK_RIGHT;
    else if (m_DefaultGamepad.fX1 < 0)
      newLeftStickKey = KEY_BUTTON_LEFT_THUMB_STICK_LEFT;
    /*else if (m_DefaultGamepad.fY1 != 0)
    {
      newLeftStickKey = KEY_BUTTON_LEFT_THUMB_STICK_RIGHT;
      m_DefaultGamepad.fX1 = 0.00001f; // small amount of movement
    }*/
  }
  else
  { // check for a new control movement
    if (m_DefaultGamepad.fY1 > 0 && m_DefaultGamepad.fX1 < m_DefaultGamepad.fY1 && -m_DefaultGamepad.fX1 < m_DefaultGamepad.fY1)
      newLeftStickKey = KEY_BUTTON_LEFT_THUMB_STICK_UP;
    else if (m_DefaultGamepad.fY1 < 0 && m_DefaultGamepad.fX1 < -m_DefaultGamepad.fY1 && -m_DefaultGamepad.fX1 < -m_DefaultGamepad.fY1)
      newLeftStickKey = KEY_BUTTON_LEFT_THUMB_STICK_DOWN;
    else if (m_DefaultGamepad.fX1 > 0 && m_DefaultGamepad.fY1 < m_DefaultGamepad.fX1 && -m_DefaultGamepad.fY1 < m_DefaultGamepad.fX1)
      newLeftStickKey = KEY_BUTTON_LEFT_THUMB_STICK_RIGHT;
    else if (m_DefaultGamepad.fX1 < 0 && m_DefaultGamepad.fY1 < -m_DefaultGamepad.fX1 && -m_DefaultGamepad.fY1 < -m_DefaultGamepad.fX1)
      newLeftStickKey = KEY_BUTTON_LEFT_THUMB_STICK_LEFT;
  }

  if (lastLeftStickKey && newLeftStickKey != lastLeftStickKey)
  { // was held down last time - and we have a new key now
    // post old key reset message...
    CKey key(lastLeftStickKey, 0, 0, 0, 0, 0, 0);
    lastLeftStickKey = newLeftStickKey;
    if (OnKey(key)) return true;
  }
  lastLeftStickKey = newLeftStickKey;
  // post the new key's message
  if (newLeftStickKey)
  {
    CKey key(newLeftStickKey, bLeftTrigger, bRightTrigger, m_DefaultGamepad.fX1, m_DefaultGamepad.fY1, m_DefaultGamepad.fX2, m_DefaultGamepad.fY2, frameTime);
    if (OnKey(key)) return true;
  }

  // Trigger detection
  static lastTriggerKey = 0;
  int newTriggerKey = 0;
  if (bLeftTrigger)
    newTriggerKey = KEY_BUTTON_LEFT_ANALOG_TRIGGER;
  else if (bRightTrigger)
    newTriggerKey = KEY_BUTTON_RIGHT_ANALOG_TRIGGER;
  if (lastTriggerKey && newTriggerKey != lastTriggerKey)
  { // was held down last time - and we have a new key now
    // post old key reset message...
    CKey key(lastTriggerKey, 0, 0, 0, 0, 0, 0);
    lastTriggerKey = newTriggerKey;
    if (OnKey(key)) return true;
  }
  lastTriggerKey = newTriggerKey;
  // post the new key's message
  if (newTriggerKey)
  {
    CKey key(newTriggerKey, bLeftTrigger, bRightTrigger, m_DefaultGamepad.fX1, m_DefaultGamepad.fY1, m_DefaultGamepad.fX2, m_DefaultGamepad.fY2, frameTime);
    if (OnKey(key)) return true;
  }

  // Now the digital buttons...
  if ( wDir & DC_LEFTTRIGGER)
  {
    CKey key(KEY_BUTTON_LEFT_TRIGGER, bLeftTrigger, bRightTrigger, m_DefaultGamepad.fX1, m_DefaultGamepad.fY1, m_DefaultGamepad.fX2, m_DefaultGamepad.fY2, frameTime);
    if (OnKey(key)) return true;
  }
  if ( wDir & DC_RIGHTTRIGGER)
  {
    CKey key(KEY_BUTTON_RIGHT_TRIGGER, bLeftTrigger, bRightTrigger, m_DefaultGamepad.fX1, m_DefaultGamepad.fY1, m_DefaultGamepad.fX2, m_DefaultGamepad.fY2, frameTime);
    if (OnKey(key)) return true;
  }
  if ( wDir & DC_LEFT )
  {
    CKey key(KEY_BUTTON_DPAD_LEFT, bLeftTrigger, bRightTrigger, m_DefaultGamepad.fX1, m_DefaultGamepad.fY1, m_DefaultGamepad.fX2, m_DefaultGamepad.fY2, frameTime);
    if (OnKey(key)) return true;
  }
  if ( wDir & DC_RIGHT)
  {
    CKey key(KEY_BUTTON_DPAD_RIGHT, bLeftTrigger, bRightTrigger, m_DefaultGamepad.fX1, m_DefaultGamepad.fY1, m_DefaultGamepad.fX2, m_DefaultGamepad.fY2, frameTime);
    if (OnKey(key)) return true;
  }
  if ( wDir & DC_UP )
  {
    CKey key(KEY_BUTTON_DPAD_UP, bLeftTrigger, bRightTrigger, m_DefaultGamepad.fX1, m_DefaultGamepad.fY1, m_DefaultGamepad.fX2, m_DefaultGamepad.fY2, frameTime);
    if (OnKey(key)) return true;
  }
  if ( wDir & DC_DOWN )
  {
    CKey key(KEY_BUTTON_DPAD_DOWN, bLeftTrigger, bRightTrigger, m_DefaultGamepad.fX1, m_DefaultGamepad.fY1, m_DefaultGamepad.fX2, m_DefaultGamepad.fY2, frameTime);
    if (OnKey(key)) return true;
  }

  if (m_DefaultGamepad.wPressedButtons & XINPUT_GAMEPAD_BACK )
  {
    CKey key(KEY_BUTTON_BACK, bLeftTrigger, bRightTrigger, m_DefaultGamepad.fX1, m_DefaultGamepad.fY1, m_DefaultGamepad.fX2, m_DefaultGamepad.fY2, frameTime);
    if (OnKey(key)) return true;
  }
  if (m_DefaultGamepad.wPressedButtons & XINPUT_GAMEPAD_START)
  {
    CKey key(KEY_BUTTON_START, bLeftTrigger, bRightTrigger, m_DefaultGamepad.fX1, m_DefaultGamepad.fY1, m_DefaultGamepad.fX2, m_DefaultGamepad.fY2, frameTime);
    if (OnKey(key)) return true;
  }
  if (m_DefaultGamepad.wPressedButtons & XINPUT_GAMEPAD_LEFT_THUMB)
  {
    CKey key(KEY_BUTTON_LEFT_THUMB_BUTTON, bLeftTrigger, bRightTrigger, m_DefaultGamepad.fX1, m_DefaultGamepad.fY1, m_DefaultGamepad.fX2, m_DefaultGamepad.fY2, frameTime);
    if (OnKey(key)) return true;
  }
  if (m_DefaultGamepad.wPressedButtons & XINPUT_GAMEPAD_RIGHT_THUMB)
  {
    CKey key(KEY_BUTTON_RIGHT_THUMB_BUTTON, bLeftTrigger, bRightTrigger, m_DefaultGamepad.fX1, m_DefaultGamepad.fY1, m_DefaultGamepad.fX2, m_DefaultGamepad.fY2, frameTime);
    if (OnKey(key)) return true;
  }
  if (m_DefaultGamepad.bPressedAnalogButtons[XINPUT_GAMEPAD_A])
  {
    CKey key(KEY_BUTTON_A, bLeftTrigger, bRightTrigger, m_DefaultGamepad.fX1, m_DefaultGamepad.fY1, m_DefaultGamepad.fX2, m_DefaultGamepad.fY2, frameTime);
    if (OnKey(key)) return true;
  }
  if (m_DefaultGamepad.bPressedAnalogButtons[XINPUT_GAMEPAD_B])
  {
    CKey key(KEY_BUTTON_B, bLeftTrigger, bRightTrigger, m_DefaultGamepad.fX1, m_DefaultGamepad.fY1, m_DefaultGamepad.fX2, m_DefaultGamepad.fY2, frameTime);
    if (OnKey(key)) return true;
  }

  if (m_DefaultGamepad.bPressedAnalogButtons[XINPUT_GAMEPAD_X])
  {
    CKey key(KEY_BUTTON_X, bLeftTrigger, bRightTrigger, m_DefaultGamepad.fX1, m_DefaultGamepad.fY1, m_DefaultGamepad.fX2, m_DefaultGamepad.fY2, frameTime);
    if (OnKey(key)) return true;
  }
  if (m_DefaultGamepad.bPressedAnalogButtons[XINPUT_GAMEPAD_Y])
  {
    CKey key(KEY_BUTTON_Y, bLeftTrigger, bRightTrigger, m_DefaultGamepad.fX1, m_DefaultGamepad.fY1, m_DefaultGamepad.fX2, m_DefaultGamepad.fY2, frameTime);
    if (OnKey(key)) return true;
  }
  if (m_DefaultGamepad.bPressedAnalogButtons[XINPUT_GAMEPAD_BLACK])
  {
    CKey key(KEY_BUTTON_BLACK, bLeftTrigger, bRightTrigger, m_DefaultGamepad.fX1, m_DefaultGamepad.fY1, m_DefaultGamepad.fX2, m_DefaultGamepad.fY2, frameTime);
    if (OnKey(key)) return true;
  }
  if (m_DefaultGamepad.bPressedAnalogButtons[XINPUT_GAMEPAD_WHITE])
  {
    CKey key(KEY_BUTTON_WHITE, bLeftTrigger, bRightTrigger, m_DefaultGamepad.fX1, m_DefaultGamepad.fY1, m_DefaultGamepad.fX2, m_DefaultGamepad.fY2, frameTime);
    if (OnKey(key)) return true;
  }
  return false;
}

bool CApplication::ProcessRemote(float frameTime)
{
  if (m_DefaultIR_Remote.wButtons)
  {
    // time depends on whether the movement is repeated (held down) or not.
    // If it is, we use the FPS timer to get a repeatable speed.
    // If it isn't, we use 20 to get repeatable jumps.
    float time = (m_DefaultIR_Remote.bHeldDown) ? frameTime : 0.020f;
    CKey key(m_DefaultIR_Remote.wButtons, 0, 0, 0, 0, 0, 0, time);
    return OnKey(key);
  }
  return false;
}

bool CApplication::ProcessMouse()
{
  if (!g_Mouse.IsActive())
    return false;
  // Reset the screensaver and idle timers
  m_idleTimer.StartZero();
  ResetScreenSaver();
  if (ResetScreenSaverWindow())
    return true;

  // call OnAction with ACTION_MOUSE
  CAction action;
  action.wID = ACTION_MOUSE;
  action.fAmount1 = (float) m_guiPointer.GetPosX();
  action.fAmount2 = (float) m_guiPointer.GetPosY();
  // send mouse event to the music + video overlays, if they're enabled
  if (m_gWindowManager.IsOverlayAllowed())
  {
    // if we're playing a movie
    if ( IsPlayingVideo() && m_gWindowManager.GetActiveWindow() != WINDOW_FULLSCREEN_VIDEO)
    {
      // then send the action to the video overlay window
      m_guiVideoOverlay.OnAction(action);
    }
    else if ( IsPlayingAudio() )
    {
      // send message to the audio overlay window
      m_guiMusicOverlay.OnAction(action);
    }
  }
  return m_gWindowManager.OnAction(action);
}

bool CApplication::ProcessHTTPApiButtons()
{
  if (pXbmcHttp)
  {
    // copy key from webserver, and reset it in case we're called again before
    // whatever happens in OnKey()
    CKey keyHttp(pXbmcHttp->GetKey());
    pXbmcHttp->ResetKey();
    if (keyHttp.GetButtonCode() != KEY_INVALID)
    {
      if (keyHttp.GetButtonCode() == KEY_VMOUSE) //virtual mouse
      {
        CAction action;
        action.wID = ACTION_MOUSE;
        g_Mouse.iPosX=(int)keyHttp.GetLeftThumbX();
        g_Mouse.iPosY=(int)keyHttp.GetLeftThumbY();
        if (keyHttp.GetLeftTrigger()!=0)
          g_Mouse.bClick[keyHttp.GetLeftTrigger()-1]=true;
        if (keyHttp.GetRightTrigger()!=0)
          g_Mouse.bDoubleClick[keyHttp.GetRightTrigger()-1]=true;
        action.fAmount1 = keyHttp.GetLeftThumbX();
        action.fAmount2 = keyHttp.GetLeftThumbY();
        // send mouse event to the music + video overlays, if they're enabled
        if (m_gWindowManager.IsOverlayAllowed())
        {
          // if we're playing a movie
          if ( IsPlayingVideo() && m_gWindowManager.GetActiveWindow() != WINDOW_FULLSCREEN_VIDEO)
          {
            // then send the action to the video overlay window
            m_guiVideoOverlay.OnAction(action);
          }
          else if ( IsPlayingAudio() )
          {
            // send message to the audio overlay window
            m_guiMusicOverlay.OnAction(action);
          }
        }
        m_gWindowManager.OnAction(action);
      }
      else
        OnKey(keyHttp);
      return true;
    }
  }
  return false;
}

bool CApplication::ProcessKeyboard()
{
  // process the keyboard buttons etc.
  BYTE vkey = g_Keyboard.GetKey();
  if (vkey)
  {
    // got a valid keypress - convert to a key code
    WORD wkeyID = (WORD)vkey | KEY_VKEY;
    //  CLog::DebugLog("Keyboard: time=%i key=%i", timeGetTime(), vkey);
    CKey key(wkeyID);
    return OnKey(key);
  }
  return false;
}

bool CApplication::IsButtonDown(DWORD code)
{
  if (code >= KEY_BUTTON_A && code <= KEY_BUTTON_RIGHT_TRIGGER)
  {
    // analogue
    return (m_DefaultGamepad.bAnalogButtons[code - KEY_BUTTON_A + XINPUT_GAMEPAD_A] > XINPUT_GAMEPAD_MAX_CROSSTALK);
  }
  else if (code >= KEY_BUTTON_DPAD_UP && code <= KEY_BUTTON_RIGHT_THUMB_BUTTON)
  {
    // digital
    return (m_DefaultGamepad.wButtons & (1 << (code - KEY_BUTTON_DPAD_UP))) != 0;
  }
  else
  {
    // remote
    return m_DefaultIR_Remote.wButtons == code;
  }
  return false;
}

bool CApplication::AnyButtonDown()
{
  ReadInput();
  if (m_DefaultGamepad.wPressedButtons || m_DefaultIR_Remote.wButtons)
    return true;

  for (int i = 0; i < 6; ++i)
  {
    if (m_DefaultGamepad.bPressedAnalogButtons[i])
      return true;
  }
  return false;
}

void CApplication::Stop()
{
  try
  {
    CLog::Log(LOGNOTICE, "Storing total System Uptime");  //GeminiServer: Total System Up-Running Time
    g_stSettings.m_iSystemTimeTotalUp = g_stSettings.m_iSystemTimeTotalUp + (int)(timeGetTime() / 60000);

    // Update the settings information (volume, uptime etc. need saving)
    if (CFile::Exists(g_settings.GetSettingsFile()))
    {
      CLog::Log(LOGNOTICE, "Saving settings");
      g_settings.Save();
    }
    else
      CLog::Log(LOGNOTICE, "Not saving settings (settings.xml is not present)");

    m_bStop = true;
    CLog::Log(LOGNOTICE, "stop all");

    StopServices();
    //Sleep(5000);

    if (m_pPlayer)
    {
      CLog::Log(LOGNOTICE, "stop mplayer");
      delete m_pPlayer;
      m_pPlayer = NULL;
    }


    CGUIDialogMusicScan *musicScan = (CGUIDialogMusicScan *)m_gWindowManager.GetWindow(WINDOW_DIALOG_MUSIC_SCAN);
    if (musicScan && musicScan->IsRunning())
      musicScan->StopScanning();

    CLog::Log(LOGNOTICE, "stop daap clients");
    g_DaapClient.Release();

    //g_lcd->StopThread();
    CLog::Log(LOGNOTICE, "stop python");
    g_applicationMessenger.Cleanup();
    g_pythonParser.FreeResources();

    CLog::Log(LOGNOTICE, "clean cached files!");
    g_RarManager.ClearCache(true);
    
    CLog::Log(LOGNOTICE, "unload skin");
    UnloadSkin();

    m_gWindowManager.Delete(WINDOW_MUSIC_PLAYLIST);
    m_gWindowManager.Delete(WINDOW_MUSIC_FILES);
    m_gWindowManager.Delete(WINDOW_MUSIC_NAV);
    //m_gWindowManager.Delete(WINDOW_MUSIC_TOP100);
    m_gWindowManager.Delete(WINDOW_MUSIC_INFO);
    m_gWindowManager.Delete(WINDOW_VIDEO_INFO);
    m_gWindowManager.Delete(WINDOW_VIDEO_FILES);
    m_gWindowManager.Delete(WINDOW_VIDEO_PLAYLIST);
    m_gWindowManager.Delete(WINDOW_VIDEO_TITLE);
    m_gWindowManager.Delete(WINDOW_VIDEO_GENRE);
    m_gWindowManager.Delete(WINDOW_VIDEO_ACTOR);
    m_gWindowManager.Delete(WINDOW_VIDEO_YEAR);
    m_gWindowManager.Delete(WINDOW_FILES);
    m_gWindowManager.Delete(WINDOW_MUSIC_INFO);
    m_gWindowManager.Delete(WINDOW_VIDEO_INFO);
    m_gWindowManager.Delete(WINDOW_DIALOG_YES_NO);
    m_gWindowManager.Delete(WINDOW_DIALOG_PROGRESS);
    m_gWindowManager.Delete(WINDOW_DIALOG_NUMERIC);
    m_gWindowManager.Delete(WINDOW_DIALOG_GAMEPAD);
    m_gWindowManager.Delete(WINDOW_DIALOG_SUB_MENU);
    m_gWindowManager.Delete(WINDOW_DIALOG_BUTTON_MENU);
    m_gWindowManager.Delete(WINDOW_DIALOG_CONTEXT_MENU);
    m_gWindowManager.Delete(WINDOW_DIALOG_MUSIC_SCAN);
    m_gWindowManager.Delete(WINDOW_DIALOG_PLAYER_CONTROLS);
    m_gWindowManager.Delete(WINDOW_DIALOG_MUSIC_OSD);
    m_gWindowManager.Delete(WINDOW_DIALOG_VIS_SETTINGS);
    m_gWindowManager.Delete(WINDOW_DIALOG_VIS_PRESET_LIST);
    m_gWindowManager.Delete(WINDOW_DIALOG_SELECT);
    m_gWindowManager.Delete(WINDOW_DIALOG_OK);
    m_gWindowManager.Delete(WINDOW_DIALOG_FILESTACKING);
    m_gWindowManager.Delete(WINDOW_DIALOG_INVITE);
    m_gWindowManager.Delete(WINDOW_DIALOG_HOST);
    m_gWindowManager.Delete(WINDOW_DIALOG_KEYBOARD);
    m_gWindowManager.Delete(WINDOW_FULLSCREEN_VIDEO);
    m_gWindowManager.Delete(WINDOW_DIALOG_TRAINER_SETTINGS);
    m_gWindowManager.Delete(WINDOW_DIALOG_PROFILE_SETTINGS);
    m_gWindowManager.Delete(WINDOW_DIALOG_LOCK_SETTINGS);
    m_gWindowManager.Delete(WINDOW_DIALOG_NETWORK_SETUP);
    m_gWindowManager.Delete(WINDOW_DIALOG_MEDIA_SOURCE);

    m_gWindowManager.Delete(WINDOW_VISUALISATION);
    m_gWindowManager.Delete(WINDOW_SETTINGS_MENU);
    m_gWindowManager.Delete(WINDOW_SETTINGS_PROFILES);
    m_gWindowManager.Delete(WINDOW_SETTINGS_MYPICTURES);  // all the settings categories
    m_gWindowManager.Delete(WINDOW_SCREEN_CALIBRATION);
    m_gWindowManager.Delete(WINDOW_SYSTEM_INFORMATION);
    m_gWindowManager.Delete(WINDOW_SCREENSAVER);
    m_gWindowManager.Delete(WINDOW_OSD);
    m_gWindowManager.Delete(WINDOW_SCRIPTS_INFO);
    m_gWindowManager.Delete(WINDOW_SLIDESHOW);

    m_gWindowManager.Delete(WINDOW_HOME);
    m_gWindowManager.Delete(WINDOW_PROGRAMS);
    m_gWindowManager.Delete(WINDOW_PICTURES);
    m_gWindowManager.Delete(WINDOW_SCRIPTS);
    m_gWindowManager.Delete(WINDOW_BUDDIES);
    m_gWindowManager.Delete(WINDOW_WEATHER);

    CLog::Log(LOGNOTICE, "unload sections");
    CSectionLoader::UnloadAll();
    CLog::Log(LOGNOTICE, "destroy");
    Destroy();

#ifdef _DEBUG
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
    g_buttonTranslator.Clear();
    CKaiClient::RemoveInstance();
    CScrobbler::RemoveInstance();
    g_infoManager.Clear();
    if (g_lcd)
    {
      g_lcd->Stop();
      delete g_lcd;
      g_lcd=NULL;
    }
    g_dlls.Clear();
    g_settings.Clear();
    g_guiSettings.Clear();
#endif

    CLog::Log(LOGNOTICE, "stopped");
  }
  catch (...)
  {}

#ifdef _CRTDBG_MAP_ALLOC
    _CrtDumpMemoryLeaks();
    while(1); // execution ends
#endif
}

bool CApplication::PlayMedia(const CFileItem& item, int iPlaylist)
{
  if (
    (!item.IsPlayList()) &&
    (!item.IsInternetStream())
  )
  {
    //not a playlist, just play the file
    return g_application.PlayFile(item);
  }
  if (iPlaylist == PLAYLIST_NONE)
  {
    CLog::Log(LOGERROR, "CApplication::PlayMedia called to play playlist %s but no idea which playlist to use.", item.m_strPath.c_str());
    return false;
  }
  //playlist
  CStdString strPath = item.m_strPath;
  if ((!item.IsPlayList()) && (item.IsInternetStream()))
  {
    //we got an url, create a dummy .strm playlist,
    //pPlayList->Load will handle loading it from url instead of from a file
    strPath = "temp.strm";
  }

  CPlayListFactory factory;
  auto_ptr<CPlayList> pPlayList (factory.Create(strPath));
  if ( NULL == pPlayList.get())
    return false;
  // load it
  if (!pPlayList->Load(item.m_strPath))
    return false;

  return ProcessAndStartPlaylist(strPath, *pPlayList, iPlaylist);
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
  
  // see if we have the info in the database
  // TODO: If user changes the time speed (FPS via framerate conversion stuff)
  //       then these times will be wrong.
  //       Also, this is really just a hack for the slow load up times we have
  //       A much better solution is a fast reader of FPS and fileLength
  //       that we can use on a file to get it's time.
  vector<long> times;
  bool haveTimes(false);
  CVideoDatabase dbs;
  if (dbs.Open())
  {
    dbs.GetVideoSettings(item.m_strPath, g_stSettings.m_currentVideoSettings);
    haveTimes = dbs.GetStackTimes(item.m_strPath, times);
    dbs.Close();
  }

  if (m_pPlayer) SAFE_DELETE(m_pPlayer);

  // calculate the total time of the stack
  CStackDirectory dir;
  dir.GetDirectory(item.m_strPath, m_currentStack);
  long totalTime = 0;
  for (int i = 0; i < m_currentStack.Size(); i++)
  {
    if (haveTimes)
      m_currentStack[i]->m_lEndOffset = times[i];
    else
    {
      EPLAYERCORES eNewCore = m_eForcedNextPlayer;
      if (eNewCore == EPC_NONE)
        eNewCore = CPlayerCoreFactory::GetDefaultPlayer(item);

      m_pPlayer = CPlayerCoreFactory::CreatePlayer(eNewCore, *this);
      if(!m_pPlayer)
      {
        m_currentStack.Clear();
        return false;
      }

      CPlayerOptions options;
      options.identify = true;
    
      if (!m_pPlayer->OpenFile(*m_currentStack[i], options))
      {
        m_currentStack.Clear();
        return false;
      }

      totalTime += (long)m_pPlayer->GetTotalTime();

      m_pPlayer->CloseFile();
      SAFE_DELETE(m_pPlayer);

      m_currentStack[i]->m_lEndOffset = totalTime;
      times.push_back(totalTime);
    }
  }

  m_itemCurrentFile = item;
  m_currentStackPosition = 0;
  m_eCurrentPlayer = EPC_NONE; // must be reset on initial play otherwise last player will be used

  double seconds = item.m_lStartOffset / 75.0;

  if (!haveTimes || item.m_lStartOffset == STARTOFFSET_RESUME )
  {  // have our times now, so update the dB
    if (dbs.Open())
    {
      if( !haveTimes )
        dbs.SetStackTimes(item.m_strPath, times);

      if( item.m_lStartOffset == STARTOFFSET_RESUME )
      {
        /* can only resume seek here, not dvdstate */
        CBookmark bookmark;
        if( dbs.GetResumeBookMark(item.m_strPath, bookmark) )
          seconds = bookmark.timeInSeconds;
        else
          seconds = 0.0f;
      }
      dbs.Close();
    }
  }

  if (seconds > 0)
  {
    // work out where to seek to    
    for (int i = 0; i < m_currentStack.Size(); i++)
    {
      if (seconds < m_currentStack[i]->m_lEndOffset)
      {
        CFileItem item(*m_currentStack[i]);
        long start = (i > 0) ? m_currentStack[i-1]->m_lEndOffset : 0;
        item.m_lStartOffset = (long)(seconds - start) * 75;
        m_currentStackPosition = i;
        return PlayFile(item, true);
      }
    }
  }
  
  return PlayFile(*m_currentStack[0], true);
}

bool CApplication::PlayFile(const CFileItem& item, bool bRestart)
{
  if (!bRestart)
  {
    OutputDebugString("new file set audiostream:0\n");
    // Switch to default options
    g_stSettings.m_currentVideoSettings = g_stSettings.m_defaultVideoSettings;
    // see if we have saved options in the database

    m_iPlaySpeed = 1;
    m_itemCurrentFile = item;
    m_nextPlaylistItem = -1;
  }
 
  if (item.IsPlayList())
    return false;
  
  // if we have a stacked set of files, we need to setup our stack routines for
  // "seamless" seeking and total time of the movie etc.
  // will recall with restart set to true
  if (item.IsStack())
    return PlayStack(item, bRestart);

  CPlayerOptions options;  
  EPLAYERCORES eNewCore = EPC_NONE;
  if( bRestart )
  {
    /* have to be set here due to playstack using this for starting the file */
    options.starttime = item.m_lStartOffset / 75.0;

    if( m_eCurrentPlayer == EPC_NONE )
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
      dbs.GetVideoSettings(item.m_strPath, g_stSettings.m_currentVideoSettings);      

      if( item.m_lStartOffset == STARTOFFSET_RESUME )
      {
        options.starttime = 0.0f;        
        CBookmark bookmark;
        if(dbs.GetResumeBookMark(item.m_strPath, bookmark))
        {
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

  // reset any forced player
  m_eForcedNextPlayer = EPC_NONE;

  //We have to stop parsing a cdg before mplayer is deallocated
  if(m_pCdgParser)
    m_pCdgParser->Stop();

  // We should restart the player, unless the previous and next tracks are using
  // one of the players that allows gapless playback (paplayer, dvdplayer)
  if (m_pPlayer)
  {
    if ( !(m_eCurrentPlayer == eNewCore && (m_eCurrentPlayer == EPC_DVDPLAYER || m_eCurrentPlayer  == EPC_PAPLAYER)) )
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

  if (!m_pPlayer)
  {
    CLog::Log(LOGERROR, "Error creating player for item %s (File doesn't exist?)", item.m_strPath.c_str());
    return false;
  }

  bool bResult = m_pPlayer->OpenFile(item, options);
  if (bResult)
  {
    // Enable Karaoke voice as necessary
//    if (g_guiSettings.GetBool("karaoke.voiceenabled"))
  //  {
    if( m_pCdgParser && g_guiSettings.GetBool("karaoke.enabled") )
    {
      if(!m_pCdgParser->IsRunning())
      {
        if (item.IsAudio() && !item.IsInternetStream() )
        {
          if (item.IsMusicDb())
            m_pCdgParser->Start(item.m_musicInfoTag.GetURL());
          else
            m_pCdgParser->Start(item.m_strPath);
        }
      }
    }

    // if file happens to contain video stream
    if ( IsPlayingVideo())
    {
      // reset the screensaver
      ResetScreenSaver();
      ResetScreenSaverWindow();

      bool bCanSwitch = true;
      // check whether we are playing from a playlist...
      int playlist = g_playlistPlayer.GetCurrentPlaylist();
      if (playlist == PLAYLIST_VIDEO || playlist == PLAYLIST_VIDEO_TEMP)
      { // playing from a playlist by the looks
        if (g_playlistPlayer.GetPlaylist(playlist).size() > 1 && g_playlistPlayer.HasPlayedFirstFile())
        { // don't switch to fullscreen if we are not playing the first item...
          bCanSwitch = false;
        }
      }
      // switch to fullscreen video mode if we can
      if (bCanSwitch)
      {
        // wait till the screen is configured before we pause
        // I'm sure this can be done better than just a sleep...
        while (!g_renderManager.IsStarted())
          Sleep(1);
        // pause video until screen is setup
        m_switchingToFullScreen = true;
        m_pPlayer->Pause();
        SwitchToFullScreen();
        //screen is setup, resume playing
        m_pPlayer->Pause();
        m_switchingToFullScreen = false;
      }
    }

  }
  return bResult;
}

void CApplication::OnPlayBackPaused()
{
  //TODO after v2.0
  //we need Paused state returned!
  //StartLEDControl(false);
  //DimLCDOnPlayback(false);
}
void CApplication::OnPlayBackEnded()
{
  //playback ended
  SetPlaySpeed(1);

  // informs python script currently running playback has ended
  // (does nothing if python is not loaded)
  g_pythonParser.OnPlayBackEnded();

  CLog::Log(LOGDEBUG, "Playback has finished");
  
  CGUIMessage msg(GUI_MSG_PLAYBACK_ENDED, 0, 0, 0, 0, NULL);
  m_gWindowManager.SendThreadMessage(msg);
  StartLEDControl(false);
  DimLCDOnPlayback(false);

  //  Reset audioscrobbler submit status
  CScrobbler::GetInstance()->SetSubmitSong(false);
}

void CApplication::OnPlayBackStarted()
{
  // informs python script currently running playback has started
  // (does nothing if python is not loaded)
  g_pythonParser.OnPlayBackStarted();

  CLog::Log(LOGDEBUG, "Playback has started");

  CGUIMessage msg(GUI_MSG_PLAYBACK_STARTED, 0, 0, 0, 0, NULL);
  m_gWindowManager.SendThreadMessage(msg);

  CheckNetworkHDSpinDown(true);

  StartLEDControl(true);
  DimLCDOnPlayback(true);
}

void CApplication::OnQueueNextItem()
{
  // informs python script currently running that we are requesting the next track
  // (does nothing if python is not loaded)
  g_pythonParser.OnQueueNextItem(); // currently unimplemented

  CLog::Log(LOGDEBUG, "Player has asked for the next item");

  CGUIMessage msg(GUI_MSG_QUEUE_NEXT_ITEM, 0, 0, 0, 0, NULL);
  m_gWindowManager.SendThreadMessage(msg);
}

void CApplication::OnPlayBackStopped()
{
  // informs python script currently running playback has ended
  // (does nothing if python is not loaded)
  g_pythonParser.OnPlayBackStopped();
  
  OutputDebugString("Playback was stopped\n");
  CGUIMessage msg( GUI_MSG_PLAYBACK_STOPPED, 0, 0, 0, 0, NULL );
  m_gWindowManager.SendMessage(msg);
  StartLEDControl(false);
  DimLCDOnPlayback(false);
  //  Reset audioscrobbler submit status
  CScrobbler::GetInstance()->SetSubmitSong(false);
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
  if (m_pPlayer->HasVideo() && m_switchingToFullScreen)
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


void CApplication::StopPlaying()
{
  int iWin = m_gWindowManager.GetActiveWindow();
  if ( IsPlaying() )
  {
    if( m_pCdgParser )
      m_pCdgParser->Stop();

    // turn off visualisation window when stopping
    if (iWin == WINDOW_VISUALISATION)
      m_gWindowManager.PreviousWindow();

    // TODO: Add saving of watched status in here
    if ( IsPlayingVideo() )
    { // save our position for resuming at a later date
      CVideoDatabase dbs;
      if (dbs.Open())
      {
        // mark as watched if we are passed the usual amount
        if (GetPercentage() >= g_advancedSettings.m_playCountMinimumPercent)
          dbs.MarkAsWatched(m_itemCurrentFile);

        if( m_pPlayer )
        {
          CBookmark bookmark;
          bookmark.player = CPlayerCoreFactory::GetPlayerName(m_eCurrentPlayer);
          bookmark.playerState = m_pPlayer->GetPlayerState();
          bookmark.timeInSeconds = GetTime();
          bookmark.thumbNailImage.Empty();

          dbs.AddBookMarkToMovie(CurrentFile(),bookmark, CBookmark::RESUME);
        }
        dbs.Close();
      }
    }
    m_pPlayer->CloseFile();
    g_partyModeManager.Disable();
  }
  OnPlayBackStopped();
}


bool CApplication::NeedRenderFullScreen()
{
  if (m_gWindowManager.GetActiveWindow() == WINDOW_FULLSCREEN_VIDEO)
  {
    m_gWindowManager.UpdateModelessVisibility();
    CGUIWindowFullScreen *pFSWin = (CGUIWindowFullScreen *)m_gWindowManager.GetWindow(WINDOW_FULLSCREEN_VIDEO);
    if (!pFSWin)
      return false;
    return pFSWin->NeedRenderFullScreen();
  }
  return false;
}
void CApplication::RenderFullScreen()
{
  if (m_gWindowManager.GetActiveWindow() == WINDOW_FULLSCREEN_VIDEO)
  {
    m_guiVideoOverlay.Close(true);
    m_guiMusicOverlay.Close(true);

    CGUIWindowFullScreen *pFSWin = (CGUIWindowFullScreen *)m_gWindowManager.GetWindow(WINDOW_FULLSCREEN_VIDEO);
    if (!pFSWin)
      return ;
    pFSWin->RenderFullScreen();
  }
}

void CApplication::ResetScreenSaver()
{
  if (m_bInactive && !m_bScreenSave && m_iScreenSaveLock == 0)
  {
    m_dwSaverTick = timeGetTime(); // Start the timer going ...
  }
}

bool CApplication::ResetScreenSaverWindow()
{
  if (m_iScreenSaveLock == 2)
    return false;

  m_bInactive = false;  // reset the inactive flag as a key has been pressed
  
  // if Screen saver is active
  if (m_bScreenSave)
  {
    if (m_iScreenSaveLock == 0)
      if (g_guiSettings.GetBool("screensaver.uselock") && g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].getLockMode() != LOCK_MODE_EVERYONE && !(g_application.IsPlayingAudio() && g_guiSettings.GetBool("screensaver.usemusicvisinstead")) && !g_guiSettings.GetString("screensaver.mode").Equals("Black"))
      {
        m_iScreenSaveLock = 2;
        CGUIMessage msg(GUI_MSG_CHECK_LOCK,0,0);
        m_gWindowManager.GetWindow(WINDOW_SCREENSAVER)->OnMessage(msg);
      }
    if (m_iScreenSaveLock == -1)
    {
      m_iScreenSaveLock = 0;
      return true;
    }
    
    // disable screensaver
    m_bScreenSave = false;
    m_iScreenSaveLock = 0;

    // if matrix trails screensaver is active
    int iWin = m_gWindowManager.GetActiveWindow();
    if (iWin == WINDOW_SCREENSAVER)
    {
      // then show previous window
      m_gWindowManager.PreviousWindow();
      return true;
    }
    else if (iWin == WINDOW_VISUALISATION && g_guiSettings.GetBool("screensaver.usemusicvisinstead"))
    {
      // we can just continue as usual from vis mode
      return false;
//      m_gWindowManager.PreviousWindow();
//      return true;
    }
    // Fade to dim or black screensaver is active --> fade in
    float fFadeLevel = 1.0f;
    CStdString strScreenSaver = g_guiSettings.GetString("screensaver.mode");
    if (strScreenSaver == "Dim")
    {
      fFadeLevel = (float)g_guiSettings.GetInt("screensaver.dimlevel") / 100;
    }
    else if (strScreenSaver == "Fade")
    {
      fFadeLevel = 0;
    }
    else if (strScreenSaver == "SlideShow" && iWin == WINDOW_SLIDESHOW)
    {
      m_gWindowManager.PreviousWindow();
      return true;
    }
    D3DGAMMARAMP Ramp;
    for (float fade = fFadeLevel; fade <= 1; fade += 0.01f)
    {
      for (int i = 0;i < 256;i++)
      {
        Ramp.red[i] = (int)((float)m_OldRamp.red[i] * fade);
        Ramp.green[i] = (int)((float)m_OldRamp.green[i] * fade);
        Ramp.blue[i] = (int)((float)m_OldRamp.blue[i] * fade);
      }
      Sleep(5);
      m_pd3dDevice->SetGammaRamp(D3DSGR_IMMEDIATE, &Ramp); // use immediate to get a smooth fade
    }
    m_pd3dDevice->SetGammaRamp(0, &m_OldRamp); // put the old gamma ramp back in place
    return true;
  }
  else
    return false;
}

void CApplication::CheckScreenSaver()
{
  // if the screen saver window is active, then clearly we are already active
  if (m_gWindowManager.IsWindowActive(WINDOW_SCREENSAVER))
  {
    m_bInactive = true;
    m_bScreenSave = true;
    return;
  }

  if (!m_bInactive)
  {
    if (IsPlayingVideo() && !m_pPlayer->IsPaused()) // are we playing a movie and is it paused?
    {
      m_bInactive = false;
    }
    else if (IsPlayingAudio()) // are we playing some music?
    {
      if (m_gWindowManager.GetActiveWindow() == WINDOW_VISUALISATION)
      {
        m_bInactive = false; // visualisation is on, so we cannot show a screensaver
      }
      else
      {
        m_bInactive = true; // music playing from GUI, we can display a screensaver
      }
    }
    else
    {
      // we can display a screensaver
      m_bInactive = true;
    }

    // if we can display a screensaver, then start screensaver timer
    if (m_bInactive)
    {
      m_dwSaverTick = timeGetTime(); // Start the timer going ...
    }
  }
  else
  {
    // Check we're not already in screensaver mode
    if (!m_bScreenSave)
    {
      // no, then check the timer if screensaver should pop up
      if ( (long)(timeGetTime() - m_dwSaverTick) >= (long)(g_guiSettings.GetInt("screensaver.time")*60*1000L) )
      {
        //yes, show the screensaver
        ActivateScreenSaver();
      }
    }
  }
}

// activate the screensaver.
// if forceType is true, we ignore the various conditions that can alter
// the type of screensaver displayed
void CApplication::ActivateScreenSaver(bool forceType /*= false */)
{
  D3DGAMMARAMP Ramp;
  FLOAT fFadeLevel;

  m_bInactive = true;
  m_bScreenSave = true;
  m_dwSaverTick = timeGetTime();  // Save the current time for the shutdown timeout

  // Get Screensaver Mode
  CStdString strScreenSaver = g_guiSettings.GetString("screensaver.mode");

  if (!forceType)
  {
    // set to Dim in the case of a dialog on screen or playing video
    if (m_gWindowManager.IsRouted() || IsPlayingVideo())
      strScreenSaver = "Dim";
    // Check if we are Playing Audio and Vis instead Screensaver!
    else if (IsPlayingAudio() && g_guiSettings.GetBool("screensaver.usemusicvisinstead"))
    { // activate the visualisation
      m_gWindowManager.ActivateWindow(WINDOW_VISUALISATION);
      return;
    }
  }
  // Picture slideshow
  if (strScreenSaver == "SlideShow")
  {
    // reset our codec info - don't want that on screen
    g_infoManager.SetShowCodec(false);
    g_applicationMessenger.PictureSlideShow(g_guiSettings.GetString("screensaver.slideshowpath"), true);
    return;
  }
  else if (strScreenSaver == "Dim")
  {
    fFadeLevel = (FLOAT) g_guiSettings.GetInt("screensaver.dimlevel") / 100; // 0.07f;
  }
  else if (strScreenSaver == "Black")
  {
    fFadeLevel = 0;
  }
  else if (strScreenSaver != "None")
  {
    m_gWindowManager.ActivateWindow(WINDOW_SCREENSAVER);
    return ;
  }
  // Fade to fFadeLevel
  m_pd3dDevice->GetGammaRamp(&m_OldRamp); // Store the old gamma ramp
  for (float fade = 1.f; fade >= fFadeLevel; fade -= 0.01f)
  {
    for (int i = 0;i < 256;i++)
    {
      Ramp.red[i] = (int)((float)m_OldRamp.red[i] * fade);
      Ramp.green[i] = (int)((float)m_OldRamp.green[i] * fade);
      Ramp.blue[i] = (int)((float)m_OldRamp.blue[i] * fade);
    }
    Sleep(5);
    m_pd3dDevice->SetGammaRamp(D3DSGR_IMMEDIATE, &Ramp); // use immediate to get a smooth fade
  }
}

void CApplication::CheckShutdown()
{
  // Note: if the the screensaver is switched on, the shutdown timeout is
  // counted from when the screensaver activates.
  if (!m_bInactive)
  {
    if (g_guiSettings.GetBool("system.shutdownwhileplaying")) // shutdown if active
    {
      m_bInactive = true;
    }
    else if (IsPlayingVideo() && !m_pPlayer->IsPaused()) // are we playing a movie?
    {
      m_bInactive = false;
    }
    else if (IsPlayingAudio()) // are we playing some music?
    {
      m_bInactive = false;
    }
    else if (m_pFileZilla && m_pFileZilla->GetNoConnections() != 0) // is FTP active ?
    {
      m_bInactive = false;
    }
    else    // nothing doing here, so start the timer going
    {
      m_bInactive = true;
    }

    if (m_bInactive)
    {
      m_dwSaverTick = timeGetTime();  // Start the timer going ...
    }
  }
  else
  {
    if ( (long)(timeGetTime() - m_dwSaverTick) >= (long)(g_guiSettings.GetInt("system.shutdowntime")*60*1000L) )
    {
      bool bShutDown = false;
      if (g_guiSettings.GetBool("system.shutdownwhileplaying")) // shutdown if active
      {
        bShutDown = true;
      }
      else if (m_pPlayer && m_pPlayer->IsPlaying()) // if we're playing something don't shutdown
      {
        m_dwSaverTick = timeGetTime();
      }
      else if (m_pFileZilla && m_pFileZilla->GetNoConnections() != 0) // is FTP active ?
      {
        m_dwSaverTick = timeGetTime();
      }
      else if (m_gWindowManager.IsWindowActive(WINDOW_DIALOG_PROGRESS))
      {
        m_dwSaverTick = timeGetTime();  // progress dialog is on screen
      }
      else          // not playing
      {
        bShutDown = true;
      }

      if (bShutDown)
        g_applicationMessenger.Shutdown(); // Turn off the box
    }
  }

  return ;
}

//Check if hd spindown must be blocked
bool CApplication::MustBlockHDSpinDown(bool bCheckThisForNormalSpinDown)
{
  if (IsPlayingVideo())
  {
    //block immediate spindown when playing a video non-fullscreen (videocontrol is playing)
    if ((!bCheckThisForNormalSpinDown) && (!g_graphicsContext.IsFullScreenVideo()))
    {
      return true;
    }
    //allow normal hd spindown always if the movie is paused
    if ((bCheckThisForNormalSpinDown) && (m_pPlayer->IsPaused()))
    {
      return false;
    }
    //don't allow hd spindown when playing files with vobsub subtitles.
    CStdString strSubTitelExtension;
    if (m_pPlayer->GetSubtitleExtension(strSubTitelExtension))
    {
      return (strSubTitelExtension == ".idx");
    }
  }
  return false;
}

void CApplication::CheckNetworkHDSpinDown(bool playbackStarted)
{
  int iSpinDown = g_guiSettings.GetInt("system.remoteplayhdspindown");
  if (iSpinDown == SPIN_DOWN_NONE)
    return ;
  if (m_gWindowManager.IsRouted())
    return ;
  if (MustBlockHDSpinDown(false))
    return ;

  if ((!m_bNetworkSpinDown) || playbackStarted)
  {
    int iDuration = 0;
    if (IsPlayingAudio())
    {
      //try to get duration from current tag because mplayer doesn't calculate vbr mp3 correctly
      iDuration = m_itemCurrentFile.m_musicInfoTag.GetDuration();
    }
    if (IsPlaying() && iDuration <= 0)
    {
      iDuration = (int)GetTotalTime();
    }
    //spin down harddisk when the current file being played is not on local harddrive and
    //duration is more then spindown timeoutsetting or duration is unknown (streams)
    if (
      !m_itemCurrentFile.IsHD() &&
      (
        (iSpinDown == SPIN_DOWN_VIDEO && IsPlayingVideo()) ||
        (iSpinDown == SPIN_DOWN_MUSIC && IsPlayingAudio()) ||
        (iSpinDown == SPIN_DOWN_BOTH && (IsPlayingVideo() || IsPlayingAudio()))
      ) &&
      (
        (iDuration <= 0) ||
        (iDuration > g_guiSettings.GetInt("system.remoteplayhdspindownminduration")*60)
      )
    )
    {
      m_bNetworkSpinDown = true;
      if (!playbackStarted)
      { //if we got here not because of a playback start check what screen we are in
        // get the current active window
        int iWin = m_gWindowManager.GetActiveWindow();
        if (iWin == WINDOW_FULLSCREEN_VIDEO)
        {
          // check if OSD is visible, if so don't do immediate spindown
          CGUIWindowOSD *pOSD = (CGUIWindowOSD *)m_gWindowManager.GetWindow(WINDOW_OSD);
          if (pOSD)
            m_bNetworkSpinDown = !pOSD->IsRunning();
        }
      }
      if (m_bNetworkSpinDown)
      {
        //do the spindown right now + delayseconds
        m_dwSpinDownTime = timeGetTime();
      }
    }
  }
  if (m_bNetworkSpinDown)
  {
    // check the elapsed time
    DWORD dwTimeSpan = timeGetTime() - m_dwSpinDownTime;
    if ( (m_dwSpinDownTime != 0) && (dwTimeSpan >= ((DWORD)g_guiSettings.GetInt("system.remoteplayhdspindowndelay")*1000UL)) )
    {
      // time has elapsed, spin it down
      CIoSupport::SpindownHarddisk();
      //stop checking until a key is pressed.
      m_dwSpinDownTime = 0;
      m_bNetworkSpinDown = true;
    }
    else if (m_dwSpinDownTime == 0 && IsPlaying())
    {
      // we are currently spun down - let's spin back up again if we are playing media
      // and we're within 10 seconds (or 0.5*spindown time) of the end.  This should
      // make returning to the GUI a bit snappier + speed up stacked item changes.
      int iMinSpinUp = 10;
      if (iMinSpinUp > g_guiSettings.GetInt("system.remoteplayhdspindowndelay")*0.5f)
        iMinSpinUp = (int)(g_guiSettings.GetInt("system.remoteplayhdspindowndelay")*0.5f);
      if (g_infoManager.GetPlayTimeRemaining() == iMinSpinUp)
      { // spin back up
        CIoSupport::SpindownHarddisk(false);
      }
    }
  }
}

void CApplication::CheckHDSpindown()
{
  if (!g_guiSettings.GetInt("system.hdspindowntime"))
    return ;
  if (m_gWindowManager.IsRouted())
    return ;
  if (MustBlockHDSpinDown())
    return ;

  if (!m_bSpinDown &&
      (
        !IsPlaying() ||
        (IsPlaying() && !m_itemCurrentFile.IsHD())
      )
     )
  {
    m_bSpinDown = true;
    m_bNetworkSpinDown = false; // let networkspindown override normal spindown
    m_dwSpinDownTime = timeGetTime();
  }

  //Can we do a spindown right now?
  if (m_bSpinDown)
  {
    // yes, then check the elapsed time
    DWORD dwTimeSpan = timeGetTime() - m_dwSpinDownTime;
    if ( (m_dwSpinDownTime != 0) && (dwTimeSpan >= ((DWORD)g_guiSettings.GetInt("system.hdspindowntime")*60UL*1000UL)) )
    {
      // time has elapsed, spin it down
      CIoSupport::SpindownHarddisk();
      //stop checking until a key is pressed.
      m_dwSpinDownTime = 0;
      m_bSpinDown = true;
    }
  }
}

void CApplication::ResetAllControls()
{
  m_guiMusicOverlay.ResetAllControls();
  m_guiVideoOverlay.ResetAllControls();
  m_guiDialogVolumeBar.ResetAllControls();
  m_guiDialogSeekBar.ResetAllControls();
  m_guiDialogKaiToast.ResetAllControls();
  m_guiDialogMuteBug.ResetAllControls();

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
          CGUIMessage msg( GUI_MSG_PLAYLIST_CHANGED, 0, 0, 0, 0, NULL );
          m_gWindowManager.SendMessage( msg );
        }
        // stop the file if it's on dvd (will set the resume point etc)
        if (m_itemCurrentFile.IsOnDVD())
          StopPlaying();
      }
    }
    break;

  case GUI_MSG_PLAYBACK_STARTED:
    {
      // Update our infoManager with the new details etc.
      if (m_nextPlaylistItem >= 0)
      { // we've started a previously queued item
        CPlayList::CPlayListItem &item = g_playlistPlayer.GetPlaylist(g_playlistPlayer.GetCurrentPlaylist())[m_nextPlaylistItem];
        // update the playlist manager
        WORD currentSong = g_playlistPlayer.GetCurrentSong();
        DWORD dwParam = ((currentSong & 0xffff) << 16) | (m_nextPlaylistItem & 0xffff);
        CGUIMessage msg(GUI_MSG_PLAYLISTPLAYER_CHANGED, 0, 0, g_playlistPlayer.GetCurrentPlaylist(), dwParam, (LPVOID)&item);
        m_gWindowManager.SendThreadMessage(msg);
        g_playlistPlayer.SetCurrentSong(m_nextPlaylistItem);
        // mark the song in the playlist as played
        g_playlistPlayer.GetPlaylist(g_playlistPlayer.GetCurrentPlaylist()).SetPlayed(g_playlistPlayer.GetCurrentSong());
        m_itemCurrentFile = item;
      }
      g_infoManager.SetCurrentItem(m_itemCurrentFile);
      g_partyModeManager.OnSongChange(true);

      if (IsPlayingAudio())
      {
        //  Activate audio scrobbler
        if (!m_itemCurrentFile.IsInternetStream())
        {
          CScrobbler::GetInstance()->SetSongStartTime();
          CScrobbler::GetInstance()->SetSubmitSong(true);
        }
        else
          CScrobbler::GetInstance()->SetSubmitSong(false);
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
        return true; // nothing to do
      // ok, grab the next song
      CFileItem item = playlist[iNext];
      // ok - send the file to the player if it wants it
      if (m_pPlayer && m_pPlayer->QueueNextFile(item))
      { // player wants the next file
        m_nextPlaylistItem = iNext;
      }
      return true;
    }
    break;

  case GUI_MSG_PLAYBACK_STOPPED:
  case GUI_MSG_PLAYBACK_ENDED:
    {
      // first check if we still have items in the stack to play
      if (message.GetMessage() == GUI_MSG_PLAYBACK_ENDED)
      {
        if (m_itemCurrentFile.IsStack() && m_currentStackPosition < m_currentStack.Size() - 1)
        { // just play the next item in the stack
          PlayFile(*m_currentStack[++m_currentStackPosition], true);
          return true;
        }
      }

      // reset our spindown
      m_bNetworkSpinDown = false;
      m_bSpinDown = false;

      // Save our settings for the current movie for next time
      if (m_itemCurrentFile.IsVideo())
      {
        CVideoDatabase dbs;
        dbs.Open();
        dbs.SetVideoSettings(m_itemCurrentFile.m_strPath, g_stSettings.m_currentVideoSettings);
        if (message.GetMessage() == GUI_MSG_PLAYBACK_ENDED)
        {
          dbs.MarkAsWatched(m_itemCurrentFile);
          dbs.ClearBookMarksOfMovie(m_itemCurrentFile.m_strPath, CBookmark::RESUME);
        }
        dbs.Close();
      }

      if (message.GetMessage() == GUI_MSG_PLAYBACK_ENDED)
      {
        // sending true to PlayNext() effectively passes bRestart to PlayFile()
        // which is not generally what we want (except for stacks, which are
        // handled above)
        g_playlistPlayer.PlayNext();
      }
      else
      {
        if (m_pPlayer)
        {
          delete m_pPlayer;
          m_pPlayer = 0;
          m_itemCurrentFile.Reset();
          m_currentStack.Clear();
          g_infoManager.ResetCurrentItem();
        }
      }

      /* no new player, free any cdg parser */
      if (!m_pPlayer && m_pCdgParser)
        m_pCdgParser->Free();

      if (!IsPlayingVideo() && m_gWindowManager.GetActiveWindow() == WINDOW_FULLSCREEN_VIDEO)
      {
        m_gWindowManager.PreviousWindow();
      }

      if (!IsPlayingAudio() && g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_NONE && m_gWindowManager.GetActiveWindow() == WINDOW_VISUALISATION)
      {
        g_settings.Save();  // save vis settings
        ResetScreenSaverWindow();
        m_gWindowManager.PreviousWindow();
      }

      // reset the audio playlist on finish
      if (!IsPlayingAudio() && (g_guiSettings.GetBool("musicplaylist.clearplaylistsonend")) && (g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_MUSIC)) 
      {
        g_playlistPlayer.ClearPlaylist(PLAYLIST_MUSIC);
        g_playlistPlayer.Reset();
        g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_NONE);
      }

      // DVD ejected while playing in vis ?
      if (!IsPlayingAudio() && (m_itemCurrentFile.IsCDDA() || m_itemCurrentFile.IsOnDVD()) && !CDetectDVDMedia::IsDiscInDrive() && m_gWindowManager.GetActiveWindow() == WINDOW_VISUALISATION)
      {
        // yes, disable vis
        g_settings.Save();    // save vis settings
        ResetScreenSaverWindow();
        m_gWindowManager.PreviousWindow();
      }
      return true;
    }
    break;

  case GUI_MSG_PLAYLISTPLAYER_STARTED:
  case GUI_MSG_PLAYLISTPLAYER_CHANGED:
    {
      return true;
    }
    break;
  case GUI_MSG_PLAYLISTPLAYER_STOPPED:
    {
      // if in visualisation or fullscreen video, go back to gui
      if (m_gWindowManager.GetActiveWindow() == WINDOW_VISUALISATION || m_gWindowManager.GetActiveWindow() == WINDOW_FULLSCREEN_VIDEO)
          m_gWindowManager.PreviousWindow();

      if(m_pCdgParser)
        m_pCdgParser->Free();

      SAFE_DELETE(m_pPlayer);
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
    { 
      // see if it is a user set string
      CLog::Log(LOGDEBUG,__FUNCTION__" : Translating %s", message.GetStringParam().c_str());
      vector<CInfoPortion> info;
      g_infoManager.ParseLabel(message.GetStringParam(), info);
      message.SetStringParam(g_infoManager.GetMultiLabel(info));
      CLog::Log(LOGDEBUG,__FUNCTION__" : To %s", message.GetStringParam().c_str());

      // user has asked for something to be executed
      if (CUtil::IsBuiltIn(message.GetStringParam()))
        CUtil::ExecBuiltIn(message.GetStringParam());
      else
      {
        // try translating the action from our ButtonTranslator
        WORD actionID;
        if (g_buttonTranslator.TranslateActionString(message.GetStringParam().c_str(), actionID))
        {
          CAction action;
          action.wID = actionID;
          action.fAmount1 = 1.0f;
          m_gWindowManager.OnAction(action);
          return true;
        }
        CFileItem item(message.GetStringParam(), false);
        if (item.IsPythonScript())
        { // a python script
          g_pythonParser.evalFile(item.m_strPath.c_str());
        }
        else if (item.IsXBE())
        { // an XBE
          int iRegion;
          if (g_guiSettings.GetBool("myprograms.gameautoregion"))
          {
            CXBE xbe;
            iRegion = xbe.ExtractGameRegion(item.m_strPath);
            if (iRegion < 1 || iRegion > 7)
              iRegion = 0;
            iRegion = xbe.FilterRegion(iRegion);
          }
          else
            iRegion = 0;
          CUtil::RunXBE(item.m_strPath.c_str(),NULL,F_VIDEO(iRegion));
        }
        else if (item.IsAudio() || item.IsVideo())
        { // an audio or video file
          PlayFile(item);
          if (IsPlayingVideo() && m_gWindowManager.GetActiveWindow() != WINDOW_FULLSCREEN_VIDEO)
          {
            SwitchToFullScreen();
          }
        }
        else
          return false;
      }
      return true;
    }
  }
  return false;
}

void CApplication::Process()
{
  // checks whats in the DVD drive and tries to autostart the content (xbox games, dvd, cdda, avi files...)
  m_Autorun.HandleAutorun();

  // reset our info cache
  g_infoManager.ResetCache();

  // check if we need to load a new skin
  if (m_dwSkinTime && timeGetTime() >= m_dwSkinTime)
  {
    CGUIMessage msg(GUI_MSG_LOAD_SKIN, -1, m_gWindowManager.GetActiveWindow());
    g_graphicsContext.SendMessage(msg);
     // Reload the skin.  Save the current focused control, and refocus it
    // when done.
    CGUIWindow* pWindow = m_gWindowManager.GetWindow(m_gWindowManager.GetActiveWindow());
    unsigned iCtrlID = pWindow->GetFocusedControl();
    CGUIMessage msg2(GUI_MSG_ITEM_SELECTED, m_gWindowManager.GetActiveWindow(), iCtrlID, 0, 0, NULL);
    g_graphicsContext.SendMessage(msg2);
    g_application.LoadSkin(g_guiSettings.GetString("lookandfeel.skin"));
    CGUIMessage msg3(GUI_MSG_SETFOCUS, m_gWindowManager.GetActiveWindow(), iCtrlID, 0);
    pWindow->OnMessage(msg3);
    CGUIMessage msgSelect(GUI_MSG_ITEM_SELECT, m_gWindowManager.GetActiveWindow(), iCtrlID, msg2.GetParam1(), msg2.GetParam2());
    pWindow->OnMessage(msgSelect);
  }

  // dispatch the messages generated by python or other threads to the current window
  m_gWindowManager.DispatchThreadMessages();

  // process messages which have to be send to the gui
  // (this can only be done after m_gWindowManager.Render())
  g_applicationMessenger.ProcessWindowMessages();

  // process any Python scripts
  g_pythonParser.Process();

  // process messages, even if a movie is playing
  g_applicationMessenger.ProcessMessages();

  // check for memory unit changes
  if (g_memoryUnitManager.Update())
  { // changes have occured - update our shares
    CGUIMessage msg(GUI_MSG_NOTIFY_ALL,0,0,GUI_MSG_REMOVED_MEDIA);
    m_gWindowManager.SendThreadMessage(msg);        
  }

  // check if we can free unused memory
  g_audioManager.FreeUnused();

  // check how far we are through playing the current item
  // and do anything that needs doing (lastfm submission, playcount updates etc)
  CheckPlayingProgress();

  // do any processing that isn't needed on each run
  if( m_slowTimer.GetElapsedMilliseconds() > 500 )
  {
    m_slowTimer.Reset();
    ProcessSlow();
  }
}

void CApplication::ProcessSlow()
{

  // update our network state
  g_network.UpdateState();

  // check if we need 2 spin down the harddisk
  CheckNetworkHDSpinDown();
  if (!m_bNetworkSpinDown)
    CheckHDSpindown();

  // check if we need to activate the screensaver (if enabled)
  if (g_guiSettings.GetString("screensaver.mode") != "None")
    CheckScreenSaver();

  // check if we need to shutdown (if enabled)
  if (g_guiSettings.GetInt("system.shutdowntime"))
    CheckShutdown();

  // check if we should restart the player
  CheckDelayedPlayerRestart();

  //  check if we can unload any unreferenced dlls or sections
  CSectionLoader::UnloadDelayed();

  // GeminiServer Xbox Autodetection // Send in X sec PingTime Interval
  if (m_gWindowManager.GetActiveWindow() != WINDOW_LOGIN_SCREEN) // sorry jm ;D
    CUtil::XboxAutoDetection();

  // check for any idle curl connections
  g_curlInterface.CheckIdle();
  
  // LED - LCD SwitchOn On Paused!!
  if(IsPlaying())
  {     
    if(g_guiSettings.GetBool("led.enableonpaused"))
      StartLEDControl(!IsPaused());
    if(g_guiSettings.GetBool("lcd.enableonpaused"))
      DimLCDOnPlayback(!IsPaused());
  }
}

// GeminiServer: Global Idle Time in Seconds
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

  // do we want to return to the current position in the file
  if (false == bSamePosition)
  {
    // no, then just reopen the file and start at the beginning
    PlayFile(m_itemCurrentFile, true);
    return ;
  }

  // else get current position
  double time = GetTime();

  // get player state, needed for dvd's
  CStdString state = m_pPlayer->GetPlayerState();

  // reopen the file
  if ( PlayFile(m_itemCurrentFile, true) && m_pPlayer )
  {
    // and seek to the position
    m_pPlayer->SetPlayerState(state);
    SeekTime(time);
  }
}

const CStdString& CApplication::CurrentFile()
{
  return m_itemCurrentFile.m_strPath;
}

CFileItem& CApplication::CurrentFileItem()
{
  return m_itemCurrentFile;
}

void CApplication::Mute(void)
{
  if (g_stSettings.m_bMute)
  { // muted - unmute.    
    // check so we don't get stuck in some muted state
    if( g_stSettings.m_iPreMuteVolumeLevel == 0 ) g_stSettings.m_iPreMuteVolumeLevel = 1;
    SetVolume(g_stSettings.m_iPreMuteVolumeLevel);
  }
  else
  { // mute
    g_stSettings.m_iPreMuteVolumeLevel = GetVolume();
    SetVolume(0);
  }
}

void CApplication::SetVolume(int iPercent)
{
  // convert the percentage to a mB (milliBell) value (*100 for dB)
  long hardwareVolume = (long)((float)iPercent * 0.01f * (VOLUME_MAXIMUM - VOLUME_MINIMUM) + VOLUME_MINIMUM);
  SetHardwareVolume(hardwareVolume);
  g_audioManager.SetVolume(g_stSettings.m_nVolumeLevel);
}

void CApplication::SetHardwareVolume(long hardwareVolume)
{
  // TODO DRC
  if (hardwareVolume >= VOLUME_MAXIMUM /*+ VOLUME_DRC_MAXIMUM*/)
    hardwareVolume = VOLUME_MAXIMUM;// + VOLUME_DRC_MAXIMUM;
  if (hardwareVolume <= VOLUME_MINIMUM)
  {
    hardwareVolume = VOLUME_MINIMUM;
  }
  // update our settings
  if (hardwareVolume > VOLUME_MAXIMUM)
  {
    g_stSettings.m_dynamicRangeCompressionLevel = hardwareVolume - VOLUME_MAXIMUM;
    g_stSettings.m_nVolumeLevel = VOLUME_MAXIMUM;
  }
  else
  {
    g_stSettings.m_dynamicRangeCompressionLevel = 0;
    g_stSettings.m_nVolumeLevel = hardwareVolume;
  }

  // update mute state
  if(!g_stSettings.m_bMute && hardwareVolume <= VOLUME_MINIMUM)
  {
    g_stSettings.m_bMute = true;
    if (!m_guiDialogMuteBug.IsRunning())
      m_guiDialogMuteBug.Show();
  }
  else if(g_stSettings.m_bMute && hardwareVolume > VOLUME_MINIMUM)
  {
    g_stSettings.m_bMute = false;
    if (m_guiDialogMuteBug.IsRunning())
      m_guiDialogMuteBug.Close();
  }


  // and tell our player to update the volume
  if (m_pPlayer)
  {
    m_pPlayer->SetVolume(g_stSettings.m_nVolumeLevel);
    // TODO DRC
//    m_pPlayer->SetDynamicRangeCompression(g_stSettings.m_dynamicRangeCompressionLevel);
  }
}

int CApplication::GetVolume() const
{
  // converts the hardware volume (in mB) to a percentage
  return int(((float)(g_stSettings.m_nVolumeLevel + g_stSettings.m_dynamicRangeCompressionLevel - VOLUME_MINIMUM)) / (VOLUME_MAXIMUM - VOLUME_MINIMUM)*100.0f + 0.5f);
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
    m_pPlayer->SetVolume(g_stSettings.m_nVolumeLevel);
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
    if (m_itemCurrentFile.IsStack())
      rc = m_currentStack[m_currentStack.Size() - 1]->m_lEndOffset;
    else
    rc = m_pPlayer->GetTotalTime();
  }

  return rc;
}

// Returns the current time in seconds of the currently playing media.
// Fractional portions of a second are possible.  This returns a double to
// be consistent with GetTotalTime() and SeekTime().
double CApplication::GetTime() const
{
  double rc = 0.0;

  if (IsPlaying() && m_pPlayer)
  {
    if (m_itemCurrentFile.IsStack())
    {
      long startOfCurrentFile = (m_currentStackPosition > 0) ? m_currentStack[m_currentStackPosition-1]->m_lEndOffset : 0;
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
    if (m_itemCurrentFile.IsStack())
    {
      // find the item in the stack we are seeking to, and load the new
      // file if necessary, and calculate the correct seek within the new
      // file.  Otherwise, just fall through to the usual routine if the
      // time is higher than our total time.
      for (int i = 0; i < m_currentStack.Size(); i++)
      {
        if (m_currentStack[i]->m_lEndOffset > dTime)
        {
          long startOfNewFile = (i > 0) ? m_currentStack[i-1]->m_lEndOffset : 0;
          if (m_currentStackPosition == i)
            m_pPlayer->SeekTime((__int64)((dTime - startOfNewFile) * 1000.0));
          else
          { // seeking to a new file
            m_currentStackPosition = i;
            CFileItem item(*m_currentStack[i]);
            item.m_lStartOffset = (long)((dTime - startOfNewFile) * 75.0);
            // don't just call "PlayFile" here, as we are quite likely called from the
            // player thread, so we won't be able to delete ourselves.
            g_applicationMessenger.PlayFile(item, true);
          }
          return;
        }
      }
    }
    // convert to milliseconds and perform seek
    m_pPlayer->SeekTime( static_cast<__int64>( dTime * 1000.0 ) );
  }
}

float CApplication::GetPercentage() const
{
  if (IsPlaying() && m_pPlayer)
  {
    if (m_itemCurrentFile.IsStack())
      return (float)(GetTime() / GetTotalTime() * 100);
    else
      return m_pPlayer->GetPercentage();
  }
  return 0.0f;
}

void CApplication::SeekPercentage(float percent)
{
  if (IsPlaying() && m_pPlayer && (percent >= 0.0))
  {
    if (!m_pPlayer->CanSeek()) return;
    if (m_itemCurrentFile.IsStack())
      SeekTime(percent * 0.01 * GetTotalTime());
    else
      m_pPlayer->SeekPercentage(percent);
  }
}

// using InterlockedExchangePointer so the python library doesn't have to worry
// about threads
CMusicInfoTag* CApplication::GetCurrentSong()
{
  CMusicInfoTag* pointer;
  InterlockedExchangePointer(&pointer, &g_infoManager.GetCurrentSongTag());
  return pointer;
}

// using InterlockedExchangePointer so the python library doesn't have to worry
// about threads
CIMDBMovie* CApplication::GetCurrentMovie()
{
  CIMDBMovie* pointer;
  InterlockedExchangePointer(&pointer, &g_infoManager.GetCurrentMovie());
  return pointer;
}

// SwitchToFullScreen() returns true if a switch is made, else returns false
bool CApplication::SwitchToFullScreen()
{
  // if playing from the video info window, close it first!
  if (m_gWindowManager.IsRouted() && m_gWindowManager.GetTopMostRoutedWindowID() == WINDOW_VIDEO_INFO)
  {
    CGUIWindowVideoInfo* pDialog = (CGUIWindowVideoInfo*)m_gWindowManager.GetWindow(WINDOW_VIDEO_INFO);
    if (pDialog) pDialog->Close(true);
  }

  // don't switch if there is a dialog on screen or the slideshow is active
  if (m_gWindowManager.IsRouted() || m_gWindowManager.GetActiveWindow() == WINDOW_SLIDESHOW)
    return false;

  // See if we're playing a video, and are in GUI mode
  if ( IsPlayingVideo() && m_gWindowManager.GetActiveWindow() != WINDOW_FULLSCREEN_VIDEO)
  {
    // then switch to fullscreen mode
    m_gWindowManager.ActivateWindow(WINDOW_FULLSCREEN_VIDEO);
    g_TextureManager.Flush();
    return true;
  }
  // special case for switching between GUI & visualisation mode. (only if we're playing an audio song)
  if (IsPlayingAudio() && m_gWindowManager.GetActiveWindow() != WINDOW_VISUALISATION)
  { // then switch to visualisation
    m_gWindowManager.ActivateWindow(WINDOW_VISUALISATION);
    g_TextureManager.Flush();
    return true;
  }
  return false;
}

const EPLAYERCORES CApplication::GetCurrentPlayer()
{
  return m_eCurrentPlayer;
}

// when a scan is initiated, save current settings
// and enable tag reading and remote thums
void CApplication::SaveMusicScanSettings()
{
  CLog::Log(LOGINFO,"Music scan has started ... enabling Tag Reading, and Remote Thumbs");
  g_stSettings.m_bMyMusicIsScanning = true;
  g_settings.Save();
}

void CApplication::RestoreMusicScanSettings()
{
  g_stSettings.m_bMyMusicIsScanning = false;
  g_settings.Save();
}

void CApplication::CheckPlayingProgress()
{
  if (!IsPlayingAudio()) return;

  CheckAudioScrobblerStatus();

  // work out where we are in the playing item
  if (GetPercentage() >= g_advancedSettings.m_playCountMinimumPercent)
  { // consider this item as played
    if (m_playCountUpdated)
      return;
    m_playCountUpdated = true;
    if (IsPlayingAudio())
    {
      // Can't write to the musicdatabase while scanning for music info
      CGUIDialogMusicScan *dialog = (CGUIDialogMusicScan *)m_gWindowManager.GetWindow(WINDOW_DIALOG_MUSIC_SCAN);
      if (dialog && !dialog->IsRunning())
      {
        CMusicDatabase musicdatabase;
        if (musicdatabase.Open())
        {
          musicdatabase.IncrTop100CounterByFileName(m_itemCurrentFile.m_strPath);
          musicdatabase.Close();
        }
      }
    }
  }
  else
    m_playCountUpdated = false;
}

void CApplication::CheckAudioScrobblerStatus()
{
  if (IsPlayingAudio() && !m_itemCurrentFile.IsInternetStream() && 
      !CScrobbler::GetInstance()->ShouldSubmit() && GetTime()==0.0)
  {
    //  We seeked to the beginning of the file
    //  reinit audio scrobbler
    CScrobbler::GetInstance()->SetSongStartTime();
    CScrobbler::GetInstance()->SetSubmitSong(true);
    return;
  }

  if (!IsPlayingAudio() || !CScrobbler::GetInstance()->ShouldSubmit())
    return;

  //  Don't submit songs to audioscrobber when the user seeks.
  //  Rule from audioscrobbler:
  //  http://www.audioscrobbler.com/development/protocol.php
  if (GetPlaySpeed()!=1)
  {
    CScrobbler::GetInstance()->SetSubmitSong(false);
    return;
  }

  //  Submit the song if 50% or 240 seconds are played
  double dTime=(double)g_infoManager.GetPlayTime()/1000.0;
  const CMusicInfoTag& tag=g_infoManager.GetCurrentSongTag();
  double dLength=(tag.GetDuration()>0) ? (tag.GetDuration()/2.0f) : (GetTotalTime()/2.0f);
  if (!tag.Loaded() || dLength==0.0f)
  {
    CScrobbler::GetInstance()->SetSubmitSong(false);
    return;
  }
  if ((dLength)>240.0f)
    dLength=240.0f;

  int iTimeTillSubmit=(int)(dLength-dTime);
  CScrobbler::GetInstance()->SetSecsTillSubmit(iTimeTillSubmit);

  if (dTime>dLength)
  {
    CScrobbler::GetInstance()->AddSong(tag);
    CScrobbler::GetInstance()->SetSubmitSong(false);
  }
}

bool CApplication::ProcessAndStartPlaylist(const CStdString& strPlayList, CPlayList& playlist, int iPlaylist)
{
  CLog::Log(LOGDEBUG,"CApplication::ProcessAndStartPlaylist(%s, %i)",strPlayList.c_str(), iPlaylist);

  // initial exit conditions
  // no songs in playlist just return
  if (playlist.size() == 0)
    return false;

  // illegal playlist
  if (iPlaylist < PLAYLIST_MUSIC || iPlaylist > PLAYLIST_VIDEO_TEMP)
    return false;

  // setup correct playlist
  g_playlistPlayer.ClearPlaylist(iPlaylist);

  // if the playlist contains an internet stream, this file will be used
  // to generate a thumbnail for musicplayer.cover 
  g_application.m_strPlayListFile = strPlayList;

  CFileItem item(playlist[0]);

  /*
  // just 1 item? then play it
  if (playlist.size() == 1)
    return g_application.PlayFile(item);
  */

  // add each item of the playlist to the playlistplayer
  for (int i = 0; i < (int)playlist.size(); ++i)
  {
    g_playlistPlayer.GetPlaylist(iPlaylist).Add(playlist[i]);
  }

  /*
    No need for this anymore - shuffle on and off can be performed at any time

  // music option: shuffle playlist on load
  // dont do this if the first item is a stream
  if (
    (iPlaylist == PLAYLIST_MUSIC || iPlaylist == PLAYLIST_MUSIC_TEMP) &&
    !item.IsShoutCast() &&
    g_guiSettings.GetBool("musicplaylist.shuffleplaylistsonload")
    )
  {
    g_playlistPlayer.GetPlaylist(iPlaylist).Shuffle();
  }*/

  // if we have a playlist 
  if (g_playlistPlayer.GetPlaylist(iPlaylist).size())
  {
    // start playing it
    g_playlistPlayer.SetCurrentPlaylist(iPlaylist);
    g_playlistPlayer.Reset();
    g_playlistPlayer.Play();
    return true;
  }
  return false;
}

bool CApplication::SetControllerRumble(FLOAT m_fLeftMotorSpeed, FLOAT m_fRightMotorSpeed, int iDuration)
{
  // GeminiServer: Controll(Rumble): Controller Motors.
  for( DWORD i=0; i<4; i++ )
  {
      if( m_Gamepad[i].hDevice )
      { 
          if( m_Gamepad[i].Feedback.Header.dwStatus != ERROR_IO_PENDING )
          {
              m_Gamepad[i].Feedback.Rumble.wLeftMotorSpeed  = WORD( m_fLeftMotorSpeed  * 65535.0f );
              m_Gamepad[i].Feedback.Rumble.wRightMotorSpeed = WORD( m_fRightMotorSpeed * 65535.0f );
              XInputSetState( m_Gamepad[i].hDevice, &m_Gamepad[i].Feedback );
          }
          else { Sleep(iDuration);
            SetControllerRumble(m_fLeftMotorSpeed, m_fRightMotorSpeed,iDuration);
            return false;
          }
      }
  }
  return true;
}

void CApplication::CheckForDebugButtonCombo()
{
  ReadInput();
  if (m_DefaultGamepad.bAnalogButtons[XINPUT_GAMEPAD_X] && m_DefaultGamepad.bAnalogButtons[XINPUT_GAMEPAD_Y])
  {
    g_advancedSettings.m_logLevel = LOG_LEVEL_DEBUG_FREEMEM;
    CLog::Log(LOGINFO, "Key combination detected for full debug logging (X+Y)");
  }
#ifdef _DEBUG
  g_advancedSettings.m_logLevel = LOG_LEVEL_DEBUG_FREEMEM;
#endif
}

