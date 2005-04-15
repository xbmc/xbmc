
#include "stdafx.h"
#include "application.h"
#include "utils/lcd.h"
#include "xbox\iosupport.h"
#include "xbox/xkutils.h"
#include "util.h"
#include "texturemanager.h"
#include "cores/playercorefactory.h"
#include "playlistplayer.h"
#include "musicdatabase.h"
#include "autorun.h"
#include "ActionManager.h"
#include "utils/LCDFactory.h"
#include "cores/ModPlayer.h"
#include "cores/mplayer/ASyncDirectSound.h"
#include "GUIButtonControl.h"
#include "GUISpinControl.h"
#include "GUIListControl.h"
#include "GUIThumbnailPanel.h"
#include "utils/DownloadQueueManager.h"
#include "utils/KaiClient.h"
#include "utils/MemUnit.h"
#include "FileSystem/DAAPDirectory.h"
#include "utils/FanController.h"
#include "XBVideoConfig.h"
#include "GUIStandardWindow.h"
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

// uncomment this if you want to use release libs in the debug build.
// Atm this saves you 7 mb of memory

  #define USE_RELEASE_LIBS

#pragma comment (lib,"xbmc/lib/libXenium/XeniumSPIg.lib")
#pragma comment (lib,"xbmc/lib/libSpeex/libSpeex.lib")

#if defined(_DEBUG) && !defined(USE_RELEASE_LIBS)
 #pragma comment (lib,"xbmc/lib/libXBMS/libXBMSd.lib")    // SECTIONNAME=LIBXBMS
 #pragma comment (lib,"xbmc/lib/libsmb/libsmbd.lib")      // SECTIONNAME=LIBSMB
 #pragma comment (lib,"xbmc/lib/cximage/ImageLibd.lib")   // SECTIONNAME=CXIMAGE
 #pragma comment (lib,"xbmc/lib/libID3/i3dlibd.lib")    // SECTIONNAME=LIBID3
 #pragma comment (lib,"xbmc/lib/libPython/pythond.lib")  // SECTIONNAME=PYTHON,PY_RW
 #pragma comment (lib,"xbmc/lib/libGoAhead/goaheadd.lib") // SECTIONNAME=LIBHTTP
 #pragma comment (lib,"xbmc/lib/sqlLite/libSQLite3d.lib")
 #pragma comment (lib,"xbmc/lib/libcdio/libcdiod.lib" )
 #pragma comment (lib,"xbmc/lib/libshout/libshoutd.lib" )
 #pragma comment (lib,"xbmc/lib/libRTV/libRTVd.lib")    // SECTIONNAME=LIBRTV
 #pragma comment (lib,"xbmc/lib/mikxbox/mikxboxd.lib")  // SECTIONNAME=MOD_RW,MOD_RX
 #pragma comment (lib,"xbmc/lib/libsidplay/libsidplayd.lib")   // SECTIONNAME=SID_RW,SID_RX
 #pragma comment (lib,"xbmc/lib/libsidplay/libsidutilsd.lib")  // SECTIONNAME=SID_RW,SID_RX
 #pragma comment (lib,"xbmc/lib/libsidplay/resid_builderd.lib") // SECTIONNAME=SID_RW,SID_RX
 #pragma comment (lib,"xbmc/lib/libmp4/libmp4v2d.lib") // SECTIONNAME=LIBMP4
 #pragma comment (lib,"xbmc/lib/libxdaap/libxdaapd.lib") // SECTIONNAME=LIBXDAAP
 #pragma comment (lib,"xbmc/lib/libiconv/libiconvd.lib")
 #pragma comment (lib,"xbmc/lib/libfribidi/libfribidid.lib")
#else
 #pragma comment (lib,"xbmc/lib/libXBMS/libXBMS.lib")
 #pragma comment (lib,"xbmc/lib/libsmb/libsmb.lib")
 #pragma comment (lib,"xbmc/lib/cximage/ImageLib.lib")
 #pragma comment (lib,"xbmc/lib/libID3/i3dlib.lib")
 #pragma comment (lib,"xbmc/lib/libPython/python.lib")
 #pragma comment (lib,"xbmc/lib/libGoAhead/goahead.lib")
 #pragma comment (lib,"xbmc/lib/sqlLite/libSQLite3.lib")
 #pragma comment (lib,"xbmc/lib/libcdio/libcdio.lib")
 #pragma comment (lib,"xbmc/lib/libshout/libshout.lib")
 #pragma comment (lib,"xbmc/lib/libRTV/libRTV.lib")
 #pragma comment (lib,"xbmc/lib/mikxbox/mikxbox.lib")
 #pragma comment (lib,"xbmc/lib/libsidplay/libsidplay.lib")    // SECTIONNAME=SID_RW,SID_RX
 #pragma comment (lib,"xbmc/lib/libsidplay/libsidutils.lib")   // SECTIONNAME=SID_RW,SID_RX
 #pragma comment (lib,"xbmc/lib/libsidplay/resid_builder.lib") // SECTIONNAME=SID_RW,SID_RX
 #pragma comment (lib,"xbmc/lib/libmp4/libmp4v2.lib") // SECTIONNAME=LIBMP4
 #pragma comment (lib,"xbmc/lib/libxdaap/libxdaap.lib") // SECTIONNAME=LIBXDAAP
 #pragma comment (lib,"xbmc/lib/libiconv/libiconv.lib")
 #pragma comment (lib,"xbmc/lib/libfribidi/libfribidi.lib")
#endif

CStdString g_LoadErrorStr;

static char szHomePaths[][13] = { "E:\\Apps\\XBMC", "E:\\XBMC", "F:\\Apps\\XBMC", "F:\\XBMC" };

#define NUM_HOME_PATHS 4

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
  m_pPlayer = NULL;
  XSetProcessQuantumLength(5); //default=20msec
  XSetFileCacheSize (256*1024); //default=64kb
  m_bInactive = false;   // CB: SCREENSAVER PATCH
  m_bScreenSave = false;   // CB: SCREENSAVER PATCH
  m_dwSaverTick = timeGetTime(); // CB: SCREENSAVER PATCH
  m_dwSkinTime = 0;
  m_DAAPSong = NULL;
  m_DAAPPtr = NULL;
  m_DAAPArtistPtr = NULL;
  m_iMasterLockRetriesRemaining = 0;
  m_bMasterLockPreviouslyEntered = false;
  m_bMasterLockOverridesLocalPasswords = false;
  m_bInitializing = true;
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

// This function does not return!
void CApplication::FatalErrorHandler(bool InitD3D, bool MapDrives, bool InitNetwork)
{
  // XBMC couldn't start for some reason...
  // g_LoadErrorStr should contain the reason
  CLog::Log(LOGWARNING, "Emergency recovery console starting...");

  bool HaveGamepad = !InitD3D;
  bool Pal = g_videoConfig.HasPAL();
  if (InitD3D)
  {
    CLog::Log(LOGINFO, "Init display in default mode: %s", Pal ? "PAL" : "NTSC");
    // init D3D with defaults (NTSC or PAL standard res)
    m_d3dpp.BackBufferWidth = 720;
    m_d3dpp.BackBufferHeight = Pal ? 576 : 480;
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
    g_graphicsContext.SetD3DParameters(&m_d3dpp, g_settings.m_ResInfo);
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
  }

  Pal = g_graphicsContext.GetVideoResolution() == PAL_4x3;

  if (HaveGamepad)
    FEH_TextOut(pFont, (Pal ? 16 : 12) | 0x18000, L"Press any button to reboot");

  // Boot up the network for FTP
  bool NetworkUp = false;
  IN_ADDR ip_addr;
  ip_addr.S_un.S_addr = 0;

  if (InitNetwork)
  {
    bool TriedDash = false;
    bool ForceDHCP = false;
    bool ForceStatic = false;
    if (m_bXboxMediacenterLoaded)
    {
      if (g_guiSettings.GetInt("Network.Assignment") == NETWORK_DHCP)
      {
        TriedDash = true;
        ForceDHCP = true;
      }
      else if (g_guiSettings.GetInt("Network.Assignment") == NETWORK_STATIC)
      {
        ForceStatic = true;
        TriedDash = true;
      }
    }

    for (;;)
    {
      if (!(XNetGetEthernetLinkStatus() & XNET_ETHERNET_LINK_ACTIVE))
      {
        FEH_TextOut(pFont, iLine, L"Network cable unplugged");
      }
      else
      {
        int err = 1;
        if (!TriedDash)
        {
          if (!m_bXboxMediacenterLoaded)
            TriedDash = true;
          FEH_TextOut(pFont, iLine, L"Init network using dash settings...");
          XNetStartupParams xnsp;
          memset(&xnsp, 0, sizeof(xnsp));
          xnsp.cfgSizeOfStruct = sizeof(XNetStartupParams);

          // Bypass security so that we may connect to 'untrusted' hosts
          xnsp.cfgFlags = XNET_STARTUP_BYPASS_SECURITY;
          // create more memory for networking
          xnsp.cfgPrivatePoolSizeInPages = 64; // == 256kb, default = 12 (48kb)
          xnsp.cfgEnetReceiveQueueLength = 16; // == 32kb, default = 8 (16kb)
          xnsp.cfgIpFragMaxSimultaneous = 16; // default = 4
          xnsp.cfgIpFragMaxPacketDiv256 = 32; // == 8kb, default = 8 (2kb)
          xnsp.cfgSockMaxSockets = 64; // default = 64
          xnsp.cfgSockDefaultRecvBufsizeInK = 128; // default = 16
          xnsp.cfgSockDefaultSendBufsizeInK = 128; // default = 16
          err = XNetStartup(&xnsp);
        }

        if (err && TriedDash)
        {
          if (!ForceStatic)
          {
            FEH_TextOut(pFont, iLine, L"Init network using DHCP...");
            network_info ni;
            memset(&ni, 0, sizeof(ni));
            ni.DHCP = true;
            int iCount = 0;
            while ((err = CUtil::SetUpNetwork(iCount == 0, ni)) == 1 && iCount < 100)
            {
              Sleep(50);
              ++iCount;

              if (HaveGamepad && AnyButtonDown())
                XKUtils::XBOXPowerCycle();
            }
          }

          if ((err || ForceStatic) && !ForceDHCP)
          {
            XNetCleanup();

            FEH_TextOut(pFont, iLine, L"Init network using static ip...");
            network_info ni;
            memset(&ni, 0, sizeof(ni));
            if (ForceStatic)
            {
              strcpy(ni.ip, g_guiSettings.GetString("Network.IPAddress"));
              strcpy(ni.subnet, g_guiSettings.GetString("Network.Subnet"));
              strcpy(ni.gateway, g_guiSettings.GetString("Network.Gateway"));
              strcpy(ni.DNS1, g_guiSettings.GetString("Network.DNS"));
            }
            else
            {
              strcpy(ni.ip, "192.168.0.42");
              strcpy(ni.subnet, "255.255.255.0");
              strcpy(ni.gateway, "192.168.0.1");
              strcpy(ni.DNS1, "192.168.0.1");
            }
            int iCount = 0;
            while ((err = CUtil::SetUpNetwork(iCount == 0, ni)) == 1 && iCount < 100)
            {
              Sleep(50);
              ++iCount;

              if (HaveGamepad && AnyButtonDown())
                XKUtils::XBOXPowerCycle();
            }
          }
        }

        if (!err)
        {
          XNADDR xna;
          DWORD dwState;
          do
          {
            dwState = XNetGetTitleXnAddr(&xna);
            Sleep(50);

            if (HaveGamepad && AnyButtonDown())
              XKUtils::XBOXPowerCycle();
          }
          while (dwState == XNET_GET_XNADDR_PENDING);
          ip_addr = xna.ina;

          if (ip_addr.S_un.S_addr)
          {
            WSADATA WsaData;
            err = WSAStartup( MAKEWORD(2, 2), &WsaData );
            if (err)
              FEH_TextOut(pFont, iLine, L"Winsock init error: %d", err);
            else
              NetworkUp = true;
          }
        }
      }
      if (!NetworkUp)
      {
        XNetCleanup();
        int n = 10;
        while (n)
        {
          FEH_TextOut(pFont, (iLine + 1) | 0x8000, L"Unable to init network, retrying in %d seconds", n--);
          for (int i = 0; i < 20; ++i)
          {
            Sleep(50);

            if (HaveGamepad && AnyButtonDown())
              XKUtils::XBOXPowerCycle();
          }
        }
      }
      else
        break;
    }
    ++iLine;
  }
  else
  {
    NetworkUp = true;
    XNADDR xna;
    DWORD dwState;
    do
    {
      dwState = XNetGetTitleXnAddr(&xna);
      Sleep(50);

      if (HaveGamepad && AnyButtonDown())
        XKUtils::XBOXPowerCycle();
    }
    while (dwState == XNET_GET_XNADDR_PENDING);
    ip_addr = xna.ina;
  }
  char addr[32];
  XNetInAddrToString(ip_addr, addr, 32);
  FEH_TextOut(pFont, iLine++, L"IP Address: %S", addr);
  ++iLine;

  if (NetworkUp)
  {
    if (!m_pFileZilla)
    {
      // Start FTP with default settings
      FEH_TextOut(pFont, iLine++, L"Starting FTP server...");

      m_pFileZilla = new CXBFileZilla(NULL);
      m_pFileZilla->Start();

      // Default settings
      m_pFileZilla->mSettings.SetMaxUsers(1);
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
        XKUtils::XBOXPowerCycle();
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

  if (!g_stSettings.m_bUnhandledExceptionToFatalError)
    CLog::Log(LOGFATAL, "%s", g_LoadErrorStr.c_str());
  else
    g_application.FatalErrorHandler(false, true, true);

  return ExceptionInfo->ExceptionRecord->ExceptionCode;
}

HRESULT CApplication::Create()
{
  HRESULT hr;

  //floating point precision to 24 bits (faster performance)
  _controlfp(_PC_24, _MCW_PC);

  CLog::Log(LOGINFO, "setup DirectX");
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
  g_graphicsContext.SetD3DParameters(&m_d3dpp, g_settings.m_ResInfo);
  g_graphicsContext.SetGUIResolution(initialResolution);

  CIoSupport helper;
  CStdString strPath;
  char szDevicePath[1024];

  // map Q to home drive of xbe to load the config file
  CUtil::GetHomePath(strPath);
  helper.GetPartition(strPath, szDevicePath);
  strcat(szDevicePath, &strPath.c_str()[2]);

  helper.Mount("Q:", "Harddisk0\\Partition2");
  helper.Unmount("Q:");
  helper.Mount("Q:", szDevicePath);

  ::DeleteFile("Q:\\xbmc.old.log");
  ::MoveFile("Q:\\xbmc.log", "Q:\\xbmc.old.log");
  CLog::Log(LOGNOTICE, "-----------------------------------------------------------------------");
  CLog::Log(LOGNOTICE, "starting...");
  CLog::Log(LOGNOTICE, "Q is mapped to:%s", szDevicePath);

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

  //Check for START+BACK and BLACK+WHITE
  if (m_DefaultGamepad.wButtons & XINPUT_GAMEPAD_START + XINPUT_GAMEPAD_BACK ||
      m_DefaultGamepad.bAnalogButtons[XINPUT_GAMEPAD_BLACK] && m_DefaultGamepad.bAnalogButtons[XINPUT_GAMEPAD_WHITE])
  {
    CLog::Log(LOGINFO, "Key combination detected for TDATA deletion (START+BACK or BLACK+WHITE)");
    CUtil::DeleteTDATA();
  }

  CLog::Log(LOGNOTICE, "load settings...");
  g_LoadErrorStr = "Unable to load settings";
  m_bAllSettingsLoaded = g_settings.Load(m_bXboxMediacenterLoaded, m_bSettingsLoaded);
  if (!m_bAllSettingsLoaded)
    FatalErrorHandler(true, true, true);

  //Check for X+Y - if pressed, set debug log mode and mplayer debuging on
  if (m_DefaultGamepad.bAnalogButtons[XINPUT_GAMEPAD_X] && m_DefaultGamepad.bAnalogButtons[XINPUT_GAMEPAD_Y])
  {
    g_stSettings.m_iLogLevel = LOGDEBUG;
    g_stSettings.m_bShowFreeMem = true;
    CLog::Log(LOGINFO, "Key combination detected for full debug logging (X+Y)");
  }

  CLog::Log(LOGINFO, "map drives...");
  CLog::Log(LOGINFO, "  map drive C:");
  helper.Remap("C:,Harddisk0\\Partition2");

  CLog::Log(LOGINFO, "  map drive E:");
  helper.Remap("E:,Harddisk0\\Partition1");

  CLog::Log(LOGINFO, "  map drive D:");
  helper.Remount("D:", "Cdrom0");

  if ((g_stSettings.m_bAutoDetectFG && helper.IsDrivePresent("F:")) || g_stSettings.m_bUseFDrive)
  {
    CLog::Log(LOGINFO, "  map drive F:");
    helper.Remap("F:,Harddisk0\\Partition6");
    g_stSettings.m_bUseFDrive = true;
  }

  // used for the LBA-48 hack allowing >120 gig
  if ((g_stSettings.m_bAutoDetectFG && helper.IsDrivePresent("G:")) || g_stSettings.m_bUseGDrive)
  {
    CLog::Log(LOGINFO, "  map drive G:");
    helper.Remap("G:,Harddisk0\\Partition7");
    g_stSettings.m_bUseGDrive = true;
  }
  CLog::Log(LOGINFO, "Drives are mapped");

  // check settings to see if another home dir is defined.
  // if there is, we check if it's a xbmc dir and map to it Q:
  CStdString strHomePath = "Q:";
  if (strlen(g_stSettings.szHomeDir) > 1)
  {
    CLog::Log(LOGNOTICE, "remap Q: to homedir:%s...", g_stSettings.szHomeDir);
    // home dir is defined in xboxmediacenter.xml
    strHomePath = g_stSettings.szHomeDir;
  }
  CLog::Log(LOGINFO, "Checking skinpath existance, and existence of keymap.xml:%s...", (strHomePath + "\\skin").c_str());
  if (!access(strHomePath + "\\skin", 0) && !access(strHomePath + "\\keymap.xml", 0))
  {
    if (strHomePath != "Q:")
    {
      helper.GetPartition(strHomePath, szDevicePath);
      strcat(szDevicePath, &strHomePath.c_str()[2]);

      CLog::Close();
      helper.Unmount("Q:");
      helper.Mount("Q:", szDevicePath);
      ::DeleteFile("Q:\\xbmc.old.log");
      ::MoveFile("Q:\\xbmc.log", "Q:\\xbmc.old.log");
      CLog::Close();
      CLog::Log(LOGNOTICE, "Q is mapped to:%s", szDevicePath);
    }
  }
  else
  {
    CLog::Log(LOGNOTICE, "skinpath %s does not exist (or no keymap.xml), trying defaults", (strHomePath + "\\skin").c_str());
    // failed - lets try defaults:
    // E:\apps\xbmc
    // E:\xbmc
    // F:\apps\xbmc
    // F:\xbmc
    bool bFoundHomePath = false;
    for (int i = 0; i < NUM_HOME_PATHS; i++)
    {
      strHomePath = szHomePaths[i];
      if (!access(strHomePath + "\\skin", 0) && !access(strHomePath + "\\keymap.xml", 0))
      {
        bFoundHomePath = true;
        break;
      }
    }
    if (bFoundHomePath)
    {
      CLog::Log(LOGINFO, "Found homepath:%s...", strHomePath.c_str());
      helper.GetPartition(strHomePath, szDevicePath);
      strcat(szDevicePath, &strHomePath.c_str()[2]);
      strcpy(g_stSettings.szHomeDir, strHomePath.c_str());
      CLog::Close();
      helper.Unmount("Q:");
      helper.Mount("Q:", szDevicePath);
      ::DeleteFile("Q:\\xbmc.old.log");
      ::MoveFile("Q:\\xbmc.log", "Q:\\xbmc.old.log");
      CLog::Close();
      CLog::Log(LOGNOTICE, "Q is mapped to:%s", szDevicePath);
    }
    else
    {
      g_LoadErrorStr = "Invalid or missing <home> tag in xml - no skins found, or no keymap.xml found";
      FatalErrorHandler(true, false, true);
    }
  }


  if (!g_graphicsContext.IsValidResolution(g_guiSettings.m_LookAndFeelResolution))
  {
    // Oh uh - doesn't look good for starting in their wanted screenmode
    CLog::Log(LOGERROR, "The screen resolution requested is not valid, resetting to a valid mode");
    g_guiSettings.m_LookAndFeelResolution = initialResolution;
  }
  // Transfer the new resolution information to our graphics context
  g_graphicsContext.SetD3DParameters(&m_d3dpp, g_settings.m_ResInfo);
  g_graphicsContext.SetGUIResolution(g_guiSettings.m_LookAndFeelResolution);
  g_graphicsContext.SetOffset(g_guiSettings.GetInt("UIOffset.X"), g_guiSettings.GetInt("UIOffset.Y"));
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

  m_splash = new CSplash("Q:\\media\\splash.png");
  m_splash->Start();

  CStdString strLanguagePath;
  strLanguagePath.Format("Q:\\language\\%s\\strings.xml", g_guiSettings.GetString("LookAndFeel.Language"));

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
  CStdString strSkinPath = strSkinBase + g_guiSettings.GetString("LookAndFeel.Skin");
  CLog::Log(LOGINFO, "Checking skin version of: %s", g_guiSettings.GetString("LookAndFeel.Skin").c_str());
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
  CLog::Log(LOGINFO, " GUI screen offset (%i,%i)", g_guiSettings.GetInt("UIOffset.X"), g_guiSettings.GetInt("UIOffset.Y"));
  m_gWindowManager.Initialize();
  g_actionManager.SetScriptActionCallback(&g_pythonParser);

  g_settings.SetBookmarkLocks("myprograms", true);
  g_settings.SetBookmarkLocks("pictures", true);
  g_settings.SetBookmarkLocks("files", true);
  g_settings.SetBookmarkLocks("music", true);
  g_settings.SetBookmarkLocks("video", true);

  m_iMasterLockRetriesRemaining = g_stSettings.m_iMasterLockMaxRetry;

  if (LOCK_MODE_EVERYONE == g_stSettings.m_iMasterLockMode)
    // masterlock is disabled, so disable all share locks too
    m_bMasterLockOverridesLocalPasswords = true;

  // show recovery console on fatal error instead of freezing
  CLog::Log(LOGINFO, "install unhandled exception filter");
  SetUnhandledExceptionFilter(UnhandledExceptionFilter);

  return CXBApplicationEx::Create();
}

HRESULT CApplication::Initialize()
{
  CLog::Log(LOGINFO, "creating subdirectories");
  if (g_stSettings.szThumbnailsDirectory[0] == 0)
  {
    strcpy(g_stSettings.szThumbnailsDirectory, "Q:\\thumbs");
  }
  if (g_stSettings.m_szShortcutDirectory[0] == 0 && !g_guiSettings.GetBool("MyPrograms.NoShortcuts"))
  {
    strcpy(g_stSettings.m_szShortcutDirectory, "Q:\\shortcuts");
  }
  if (g_stSettings.m_szAlbumDirectory[0] == 0)
  {
    strcpy(g_stSettings.m_szAlbumDirectory, "Q:\\albums");
  }
  if (g_stSettings.m_szMusicRecordingDirectory[0] == 0)
  {
    strcpy(g_stSettings.m_szMusicRecordingDirectory, "Q:\\recordings");
  }
  if (g_stSettings.m_szScreenshotsDirectory[0] == 0)
  {
    strcpy(g_stSettings.m_szScreenshotsDirectory, "Q:\\screenshots");
  }

  CreateDirectory(g_stSettings.szThumbnailsDirectory, NULL);
  CStdString strThumbIMDB = g_stSettings.szThumbnailsDirectory;
  strThumbIMDB += "\\imdb";
  CreateDirectory(strThumbIMDB.c_str(), NULL);
  CStdString strThumbKai = g_stSettings.szThumbnailsDirectory;
  strThumbKai += "\\kai";
  CreateDirectory(strThumbKai.c_str(), NULL);

  if (!g_guiSettings.GetBool("MyPrograms.NoShortcuts"))
    CreateDirectory(g_stSettings.m_szShortcutDirectory, NULL);
  CreateDirectory(g_stSettings.m_szAlbumDirectory, NULL);
  CreateDirectory(g_stSettings.m_szMusicRecordingDirectory, NULL);
  CreateDirectory(g_stSettings.m_szScreenshotsDirectory, NULL);

  CLog::Log(LOGINFO, "  thumbnails folder:%s", g_stSettings.szThumbnailsDirectory);
  CLog::Log(LOGINFO, "  shortcuts folder:%s", g_stSettings.m_szShortcutDirectory);
  CLog::Log(LOGINFO, "  albums folder:%s", g_stSettings.m_szAlbumDirectory);
  CLog::Log(LOGINFO, "  recording folder:%s", g_stSettings.m_szMusicRecordingDirectory);
  CLog::Log(LOGINFO, "  screenshots folder:%s", g_stSettings.m_szScreenshotsDirectory);

  string strAlbumDir = g_stSettings.m_szAlbumDirectory;
  CreateDirectory((strAlbumDir + "\\playlists").c_str(), NULL);
  CreateDirectory((strAlbumDir + "\\cddb").c_str(), NULL);
  CreateDirectory((strAlbumDir + "\\thumbs").c_str(), NULL); // contains the album thumbs
  CreateDirectory((strAlbumDir + "\\thumbs\\temp").c_str(), NULL);
  CreateDirectory("Q:\\python", NULL);
  CreateDirectory("Q:\\python\\Lib", NULL);
  CreateDirectory("Q:\\python\\temp", NULL);
  CreateDirectory("Q:\\scripts", NULL);
  CreateDirectory("Q:\\language", NULL);
  CreateDirectory("Q:\\visualisations", NULL);
  CreateDirectory("Q:\\sounds", NULL);

  if (g_stSettings.m_szAlternateSubtitleDirectory[0] == 0)
  {
    strcpy(g_stSettings.m_szAlternateSubtitleDirectory, "Q:\\subtitles");
  }
  CLog::Log(LOGINFO, "  subtitle folder:%s", g_stSettings.m_szAlternateSubtitleDirectory);
  CreateDirectory(g_stSettings.m_szAlternateSubtitleDirectory, NULL);

  InitMemoryUnits();

  // initialize network
  if (!m_bXboxMediacenterLoaded)
  {
    CLog::Log(LOGINFO, "using default network settings");
    g_guiSettings.SetString("Network.IPAddress", "192.168.0.100");
    g_guiSettings.SetString("Network.Subnet", "255.255.255.0");
    g_guiSettings.SetString("Network.Gateway", "192.168.0.1");
    g_guiSettings.SetString("Network.DNS", "192.168.0.1");
    g_guiSettings.SetBool("Servers.FTPServer", true);
    g_guiSettings.SetBool("Servers.WebServer", false);
    g_guiSettings.SetBool("Servers.TimeServer", false);
  }
  CStdString strAssignment;
  if (g_guiSettings.GetInt("Network.Assignment") == NETWORK_DASH)
    strAssignment = "dash";
  else if (g_guiSettings.GetInt("Network.Assignment") == NETWORK_DHCP)
    strAssignment = "dhcp";
  else //if (g_guiSettings.GetInt("Network.Assignment")==NETWORK_STATIC)
    strAssignment = "static";
  CLog::Log(LOGNOTICE, "initialize assignment:[%s] network ip:[%s] netmask:[%s] gateway:[%s] nameserver:[%s]",
            strAssignment.c_str(),
            g_guiSettings.GetString("Network.IPAddress").c_str(),
            g_guiSettings.GetString("Network.Subnet").c_str(),
            g_guiSettings.GetString("Network.Gateway").c_str(),
            g_guiSettings.GetString("Network.DNS").c_str());

  if ( !CUtil::InitializeNetwork(g_guiSettings.GetInt("Network.Assignment"),
                                 g_guiSettings.GetString("Network.IPAddress"),
                                 g_guiSettings.GetString("Network.Subnet"),
                                 g_guiSettings.GetString("Network.Gateway"),
                                 g_guiSettings.GetString("Network.DNS")))
  {
    CLog::Log(LOGERROR, "initialize network failed");
  }

  StartServices();

  CLog::Log(LOGNOTICE, "load default skin:[%s]", g_guiSettings.GetString("LookAndFeel.Skin").c_str());
  LoadSkin(g_guiSettings.GetString("LookAndFeel.Skin"));

  CLog::Log(LOGINFO, "initializing skin");
  m_gWindowManager.Add(&m_guiHome);                     // window id = 0
  m_gWindowManager.Add(&m_guiPrograms);                 // window id = 1
  m_gWindowManager.Add(&m_guiPictures);                 // window id = 2
  m_gWindowManager.Add(&m_guiFileManager);              // window id = 3
  m_gWindowManager.Add(&m_guiMyVideo);                  // window id = 6
  m_gWindowManager.Add(&m_guiSettings);                 // window id = 4
  m_gWindowManager.Add(&m_guiSystemInfo);               // window id = 7
  m_gWindowManager.Add(&m_guiSettingsUICalibration);    // window id = 10
  m_gWindowManager.Add(&m_guiSettingsScreenCalibration); // window id = 11
  m_gWindowManager.Add(&m_guiSettingsCategory);         // window id = 12 slideshow:window id 2007
  m_gWindowManager.Add(&m_guiScripts);                  // window id = 20
  m_gWindowManager.Add(&m_guiVideoGenre);               // window id = 21
  m_gWindowManager.Add(&m_guiVideoActors);              // window id = 22
  m_gWindowManager.Add(&m_guiVideoYear);                // window id = 23
  m_gWindowManager.Add(&m_guiVideoTitle);               // window id = 25
  m_gWindowManager.Add(&m_guiMyVideoPlayList);          // window id = 28
  m_gWindowManager.Add(&m_guiSettingsProfile);          // window id = 34

  m_gWindowManager.Add(&m_guiDialogYesNo);              // window id = 100
  m_gWindowManager.Add(&m_guiDialogProgress);           // window id = 101
  m_gWindowManager.Add(&m_guiDialogInvite);             // window id = 102
  m_gWindowManager.Add(&m_guiDialogKeyboard);           // window id = 103
  m_gWindowManager.Add(&m_guiDialogVolumeBar);          // window id = 104
  m_gWindowManager.Add(&m_guiDialogSubMenu);            // window id = 105
  m_gWindowManager.Add(&m_guiDialogContextMenu);        // window id = 106
  m_gWindowManager.Add(&m_guiDialogKaiToast);           // window id = 107
  m_gWindowManager.Add(&m_guiDialogHost);               // window id = 108
  m_gWindowManager.Add(&m_guiDialogNumeric);            // window id = 109
  m_gWindowManager.Add(&m_guiDialogGamepad);            // window id = 110
  m_gWindowManager.Add(&m_guiDialogButtonMenu);         // window id = 111
  m_gWindowManager.Add(&m_guiDialogMusicScan);          // window id = 112
  m_gWindowManager.Add(&m_guiDialogPlayerControls);     // window id = 113

  m_gWindowManager.Add(&m_guiMyMusicPlayList);          // window id = 500
  m_gWindowManager.Add(&m_guiMyMusicSongs);             // window id = 501
  m_gWindowManager.Add(&m_guiMyMusicNav);               // window id = 502
  m_gWindowManager.Add(&m_guiMyMusicTop100);            // window id = 503

  //m_gWindowManager.Add(&m_keyboard);                    // window id = 1000
  m_gWindowManager.Add(&m_guiDialogSelect);             // window id = 2000
  m_gWindowManager.Add(&m_guiMusicInfo);                // window id = 2001
  m_gWindowManager.Add(&m_guiDialogOK);                 // window id = 2002
  m_gWindowManager.Add(&m_guiVideoInfo);                // window id = 2003
  m_gWindowManager.Add(&m_guiScriptsInfo);              // window id = 2004
  m_gWindowManager.Add(&m_guiWindowFullScreen);         // window id = 2005
  m_gWindowManager.Add(&m_guiWindowVisualisation);      // window id = 2006
  m_gWindowManager.Add(&m_guiWindowSlideshow);          // window id = 2007
  m_gWindowManager.Add(&m_guiDialogFileStacking);       // window id = 2008
  m_gWindowManager.Add(&m_guiWindowOSD);                // window id = 2901
  m_gWindowManager.Add(&m_guiWindowScreensaver);        // window id = 2900 Screensaver
  m_gWindowManager.Add(&m_guiMyWeather);                // window id = 2600 WEATHER
  m_gWindowManager.Add(&m_guiMyBuddies);                // window id = 2700 BUDDIES

  g_DownloadManager.Initialize();
  CKaiClient::GetInstance()->SetObserver(&m_guiMyBuddies);

  /* window id's 3000 - 3100 are reserved for python */
  //CLog::Log(LOGINFO, "initializing virtual keyboard");
  //m_keyboard.Initialize();


  m_ctrDpad.SetDelays(g_stSettings.m_iMoveDelayController, g_stSettings.m_iRepeatDelayController);

  SAFE_DELETE(m_splash);

  g_audioManager.PlayStartSound();

  m_gWindowManager.ActivateWindow(g_stSettings.m_iStartupWindow);
  CLog::Log(LOGINFO, "removing tempfiles");
  CUtil::RemoveTempFiles();

  if (!m_bAllSettingsLoaded)
  {
    CLog::Log(LOGWARNING, "settings not correct, show dialog");
    CStdString test;
    CUtil::GetHomePath(test);
    m_guiDialogOK.SetHeading(279);
    m_guiDialogOK.SetLine(0, "Error while loading settings");
    m_guiDialogOK.SetLine(1, test);
    m_guiDialogOK.SetLine(2, L"");
    m_guiDialogOK.DoModal(g_stSettings.m_iStartupWindow);
  }

  // if the user shutoff the xbox during music scan
  // restore the settings
  if (g_stSettings.m_bMyMusicIsScanning)
  {
    CLog::Log(LOGWARNING,"System rebooted during music scan! ... restoring UseTags and FindRemoteThumbs");
    RestoreMusicScanSettings();
  }

  // check for special dirs, ie $PLAYLISTS
  CLog::Log(LOGINFO,"Checking bookmarks for special directories");
  CUtil::TranslateBookmarks(g_settings.m_vecMyProgramsBookmarks);
  CUtil::TranslateBookmarks(g_settings.m_vecMyPictureShares);
  CUtil::TranslateBookmarks(g_settings.m_vecMyFilesShares);
  CUtil::TranslateBookmarks(g_settings.m_vecMyMusicShares);
  CUtil::TranslateBookmarks(g_settings.m_vecMyVideoShares);

  CLog::Log(LOGNOTICE, "initialize done");
  if (g_guiSettings.GetInt("LCD.Mode") == LCD_MODE_NOTV)
  {
    // jump to my music when we're in NO tv mode
    m_gWindowManager.ActivateWindow(WINDOW_MUSIC_FILES);
  }

  m_bInitializing = false;
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
  g_lcd->SetLine(pLine++, "");
  g_lcd->SetLine(pLine++, "Playing");
  g_lcd->SetLine(pLine++, strXBEName);
  g_lcd->SetLine(pLine++, "");
}
void CApplication::StartWebServer()
{
  if (g_guiSettings.GetBool("Servers.WebServer") && CUtil::IsNetworkUp())
  {
    CLog::Log(LOGNOTICE, "start webserver");
    CSectionLoader::Load("LIBHTTP");
    m_pWebServer = new CWebServer();
    CStdString ipadres;
    CUtil::GetTitleIP(ipadres);
    m_pWebServer->Start(ipadres.c_str(), atoi(g_guiSettings.GetString("Servers.WebServerPort")), "Q:\\web");
  }
}

void CApplication::StopWebServer()
{
  if (m_pWebServer)
  {
    CLog::Log(LOGNOTICE, "stop webserver");
    m_pWebServer->Stop();
    delete m_pWebServer;
    m_pWebServer = NULL;
    CSectionLoader::Unload("LIBHTTP");
  }
}

void CApplication::StartFtpServer()
{
  if ( g_guiSettings.GetBool("Servers.FTPServer") && CUtil::IsNetworkUp())
  {
    CLog::Log(LOGNOTICE, "start ftpserver");
    if (!m_pFileZilla)
    {
      m_pFileZilla = new CXBFileZilla("Q:\\");
      m_pFileZilla->Start();
    }
  }
}

void CApplication::StopFtpServer()
{
  /* filezilla doesn't like to be deleted?
  if (m_pFileZilla) {
  CLog::Log(LOGINFO, "stop ftpserver");
  m_pFileZilla->Stop();
    delete m_pFileZilla;
    m_pFileZilla = NULL;
  }
  */
}

void CApplication::StartTimeServer()
{
  if (g_guiSettings.GetBool("Servers.TimeServer") && CUtil::IsNetworkUp())
  {
    CLog::Log(LOGNOTICE, "start timeserver thread");
    m_sntpClient.Create();
  }
}

void CApplication::StopTimeServer()
{
  CLog::Log(LOGNOTICE, "stop time server");
  m_sntpClient.StopThread();
}

void CApplication::StartLEDControl(bool switchoff)
{
  CLog::Log(LOGNOTICE, "Start LED Control");
  if (switchoff == true && g_guiSettings.GetInt("LED.Colour") != LED_COLOUR_NO_CHANGE)
  {
    if ( IsPlayingVideo() && g_guiSettings.GetInt("LED.DisableOnPlayback") == LED_PLAYBACK_VIDEO)
    {
      CLog::Log(LOGNOTICE, "LED Control: Playing Video LED is switched OFF!");
      ILED::CLEDControl(LED_COLOUR_OFF);
    }
    if ( IsPlayingAudio() && g_guiSettings.GetInt("LED.DisableOnPlayback") == LED_PLAYBACK_MUSIC)
    {
      CLog::Log(LOGNOTICE, "LED Control: Playing Music LED is switched OFF!");
      ILED::CLEDControl(LED_COLOUR_OFF);
    }
    if ( (IsPlayingVideo() || IsPlayingAudio()) && g_guiSettings.GetInt("LED.DisableOnPlayback") == LED_PLAYBACK_VIDEO_MUSIC)
    {
      CLog::Log(LOGNOTICE, "LED Control: Playing Video Or Music LED is switched OFF!");
      ILED::CLEDControl(LED_COLOUR_OFF);
    }
  }
  else if (switchoff == false)
  {
    ILED::CLEDControl(g_guiSettings.GetInt("LED.Colour"));
  }
  // and dim our LCD as required as well
  DimLCDOnPlayback(switchoff);
}

void CApplication::DimLCDOnPlayback(bool dim)
{
  CLog::Log(LOGNOTICE, "Dim LCD On Playback");
  if (g_lcd && g_guiSettings.GetInt("LCD.Mode") != LCD_MODE_NONE)
  {
    if (IsPlayingVideo() && dim == true && g_guiSettings.GetInt("LED.DisableOnPlayback") == LED_PLAYBACK_VIDEO)
    {
      g_lcd->SetBackLight(0);
    }
    else if (dim == false && g_guiSettings.GetInt("LED.DisableOnPlayback") == LED_PLAYBACK_VIDEO)
    {
      g_lcd->SetBackLight(g_guiSettings.GetInt("LCD.BackLight"));
    }
  }
}

void CApplication::StartServices()
{
  StartTimeServer();
  StartWebServer();
  StartFtpServer();
  StartLEDControl(false);

  // Start Thread for DVD Mediatype detection
  CLog::Log(LOGNOTICE, "start dvd mediatype detection");
  m_DetectDVDType.Create( false);

  CLog::Log(LOGNOTICE, "initializing playlistplayer");
  g_playlistPlayer.Repeat(PLAYLIST_MUSIC, g_stSettings.m_bMyMusicPlaylistRepeat);
  g_playlistPlayer.ShufflePlay(PLAYLIST_MUSIC, g_stSettings.m_bMyMusicPlaylistShuffle);
  g_playlistPlayer.Repeat(PLAYLIST_MUSIC_TEMP, g_guiSettings.GetBool("MyMusic.Repeat"));
  g_playlistPlayer.Repeat(PLAYLIST_VIDEO, g_stSettings.m_bMyVideoPlaylistRepeat);
  g_playlistPlayer.ShufflePlay(PLAYLIST_VIDEO, g_stSettings.m_bMyVideoPlaylistShuffle);
  g_playlistPlayer.Repeat(PLAYLIST_VIDEO_TEMP, false);

  CLCDFactory factory;
  g_lcd = factory.Create();
  g_lcd->Initialize();

  CLog::Log(LOGNOTICE, "start fancontroller");
  if (g_guiSettings.GetBool("System.AutoTemperature"))
  {
    CFanController::Instance()->Start(g_guiSettings.GetInt("System.TargetTemperature"));
  }
  else if (g_guiSettings.GetBool("System.FanSpeedControl"))
  {
    CFanController::Instance()->SetFanSpeed(g_guiSettings.GetInt("System.FanSpeed"));
  }
}

void CApplication::StopServices()
{
  StopWebServer();
  StopTimeServer();
  StopFtpServer();

  CLog::Log(LOGNOTICE, "stop dvd detect media");
  m_DetectDVDType.StopThread();

  CLog::Log(LOGNOTICE, "stop fancontroller");
  CFanController::Instance()->Stop();
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
  m_dwSkinTime = 0;

  CStdString strHomePath;
  CStdString strSkinPath = "Q:\\skin\\";
  strSkinPath += strSkin;

  CLog::Log(LOGINFO, "  load skin from:%s", strSkinPath.c_str());

  if ( IsPlaying() )
  {
    CLog::Log(LOGINFO, " stop playing...");
    m_pPlayer->CloseFile();
    m_itemCurrentFile.Clear();
    delete m_pPlayer;
    m_pPlayer = NULL;
  }

  //  When the app is started the instance of the
  //  kai client should not be created until the
  //  skin is loaded the first time, but we must
  //  disconnect from the engine when the skin is
  //  changed
  bool bKaiConnected = false;
  if (!m_bInitializing)
  {
    bKaiConnected = CKaiClient::GetInstance()->IsEngineConnected();
    if (bKaiConnected)
    {
      CLog::Log(LOGINFO, " Disconnecting Kai...");
      CKaiClient::GetInstance()->RemoveObserver();
    }
  }

  CLog::Log(LOGINFO, "  delete old skin...");
  m_guiVideoOverlay.FreeResources();
  m_guiVideoOverlay.ClearAll();

  m_guiMusicOverlay.FreeResources();
  m_guiMusicOverlay.ClearAll();

  m_guiDialogVolumeBar.FreeResources();
  m_guiDialogVolumeBar.ClearAll();
  m_guiDialogKaiToast.FreeResources();
  m_guiDialogKaiToast.ClearAll();
  m_guiDialogMusicScan.FreeResources();
  m_guiDialogMusicScan.ClearAll();
  m_guiDialogMuteBug.FreeResources();
  m_guiDialogMuteBug.ClearAll();

  m_guiPointer.FreeResources();
  m_guiPointer.ClearAll();

  g_audioManager.DeInitialize();

  m_gWindowManager.DeInitialize();
  g_TextureManager.Cleanup();

  g_fontManager.Clear();

  g_charsetConverter.reset();

  // Load in the skin.xml file if it exists
  g_SkinInfo.Load(strSkinPath);

  CLog::Log(LOGINFO, "  load fonts for skin...");
  g_graphicsContext.SetMediaDir(strSkinPath);
  g_fontManager.LoadFonts(g_guiSettings.GetString("LookAndFeel.Font"));

  LARGE_INTEGER start;
  QueryPerformanceCounter(&start);

  CLog::Log(LOGINFO, "  load new skin...");
  if (!g_SkinInfo.Check(strSkinPath) || !m_guiHome.Load( "home.xml"))
  {
    // failed to load home.xml
    // fallback to default skin
    if ( CUtil::cmpnocase(strSkin.c_str(), "Project Mayhem III") != 0)
    {
      CLog::Log(LOGERROR, "failed to load home.xml for skin:%s, fallback to \"Project Mayhem III\" skin", strSkin.c_str());
      LoadSkin("Project Mayhem III");
      return ;
    }
  }

  m_guiPrograms.Load( "myprograms.xml");
  m_guiPictures.Load( "mypics.xml");
  m_guiFileManager.Load( "filemanager.xml");
  m_guiMyVideo.Load("myvideo.xml");
  m_guiVideoGenre.Load("myvideogenre.xml");
  m_guiVideoActors.Load("myvideoactors.xml");
  m_guiVideoYear.Load("myvideoYear.xml");
  m_guiVideoTitle.Load("myvideoTitle.xml");
  m_guiSettings.Load("settings.xml");
  m_guiMyVideoPlayList.Load("myvideoplaylist.xml");
  m_guiSystemInfo.Load("SettingsSystemInfo.xml");
  m_guiMusicInfo.Load("DialogAlbumInfo.xml");
  m_guiScriptsInfo.Load("DialogScriptInfo.xml");
  m_guiSettingsProfile.Load("SettingsProfile.xml");
  m_guiDialogYesNo.Load("dialogYesNo.xml");
  m_guiDialogProgress.Load("dialogProgress.xml");
  m_guiDialogVolumeBar.Load("dialogVolumeBar.xml");
  m_guiDialogKaiToast.Load("dialogKaiToast.xml");
  m_guiDialogNumeric.Load("dialogNumeric.xml");
  m_guiDialogGamepad.Load("dialogGamepad.xml");
  m_guiDialogSubMenu.Load("dialogSubMenu.xml");
  m_guiDialogButtonMenu.Load("dialogButtonMenu.xml");
  m_guiDialogContextMenu.Load("dialogContextMenu.xml");
  m_guiDialogMusicScan.Load("dialogMusicScan.xml");
  m_guiDialogPlayerControls.Load("PlayerControls.xml");
  m_guiMyMusicPlayList.Load("mymusicplaylist.xml");
  m_guiMyMusicSongs.Load("mymusicsongs.xml");
  m_guiMyMusicNav.Load("mymusicnav.xml");
  m_guiMyMusicTop100.Load("mymusictop100.xml");
  m_guiDialogSelect.Load("dialogSelect.xml");
  m_guiDialogOK.Load("dialogOK.xml");
  m_guiDialogFileStacking.Load("dialogFileStacking.xml");
  m_guiVideoInfo.Load("DialogVideoInfo.xml");
  m_guiMusicOverlay.Load("musicOverlay.xml");
  m_guiSettingsUICalibration.Load("settingsUICalibration.xml");
  m_guiSettingsScreenCalibration.Load("settingsScreenCalibration.xml");
  m_guiSettingsCategory.Load("SettingsCategory.xml");
  m_guiVideoOverlay.Load("videoOverlay.xml");
  m_guiWindowFullScreen.Load("videoFullScreen.xml");
  m_guiScripts.Load("myscripts.xml");
  m_guiWindowVisualisation.Load("musicVisualisation.xml");
  m_guiWindowSlideshow.Load("slideshow.xml");
  m_guiWindowScreensaver.SetID(WINDOW_SCREENSAVER);    // Matrix Screensaver - saves us having to have our own XML file
  m_guiWindowOSD.Load("videoOSD.xml");
  m_guiMyWeather.Load("myweather.xml");       //WEATHER
  m_guiDialogInvite.Load( "dialogInvite.xml" );
  m_guiDialogHost.Load( "dialogHost.xml" );
  m_guiDialogKeyboard.Load( "dialogKeyboard.xml" );
  m_guiMyBuddies.Load( "mybuddies.xml");
  m_guiPointer.Load("Pointer.xml");
  m_guiDialogMuteBug.Load("DialogMuteBug.xml");

  // Load the user windows
  LoadUserWindows(strSkinPath);

  CGUIWindow::FlushReferenceCache(); // flush the cache so it doesn't use memory all the time

  LARGE_INTEGER end, freq;
  QueryPerformanceCounter(&end);
  QueryPerformanceFrequency(&freq);
  CLog::DebugLog("Load Skin XML: %.2fms", 1000.f * (end.QuadPart - start.QuadPart) / freq.QuadPart);

  CLog::Log(LOGINFO, "  initialize new skin...");
  m_guiPointer.AllocResources();
  m_guiMusicOverlay.AllocResources();
  m_guiVideoOverlay.AllocResources();
  m_guiDialogVolumeBar.AllocResources();
  m_guiDialogKaiToast.AllocResources();
  m_guiDialogMusicScan.AllocResources();
  m_gWindowManager.AddMsgTarget(this);
  m_gWindowManager.AddMsgTarget(&g_playlistPlayer);
  m_gWindowManager.SetCallback(*this);
  m_gWindowManager.Initialize();
  g_audioManager.Initialize();
  g_audioManager.Load();
  CLog::Log(LOGINFO, "  skin loaded...");

  if (bKaiConnected)
  {
    CLog::Log(LOGINFO, " Reconnecting Kai...");

    CKaiClient::GetInstance()->SetObserver(&m_guiMyBuddies);
    Sleep(3000);  //  The client need some time to "resync"
  }
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
    if (pType == NULL)
    {
      pWindow = new CGUIStandardWindow();
    }
    else
    {
      CStdString strType = pType->FirstChild()->Value();
      if (strType == "dialog")
      {
        pWindow = new CGUIDialog(0);
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

    // Check to make sure the pointer isn't still null
    if (pWindow == NULL)
    {
      CLog::Log(LOGERROR, "Out of memory / Failed to create new object in LoadUserWindows");
      return false;
    }

    // Try to load the page.  If the load fails, delete the pointer
    if (pWindow->Load(pRootElement, resToUse))
    {
      m_gWindowManager.AddCustomWindow(pWindow);
    }
    else
    {
      delete pWindow;
    }
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

  // check that we haven't passed the end of the file (for queue sheets)
  if ((m_pPlayer != NULL) && m_pPlayer->IsPlaying())
  {
    int timeinsecs = (int)(m_pPlayer->GetTime() / 1000);
    if (m_itemCurrentFile.m_lEndOffset && m_itemCurrentFile.m_lEndOffset / 75 < timeinsecs)
    { // time to stop the file...
      OnPlayBackEnded();
    }
  }
  // dont show GUI when playing full screen video
  if (m_gWindowManager.GetActiveWindow() == WINDOW_FULLSCREEN_VIDEO)
  {
    if ( g_graphicsContext.IsFullScreenVideo() )
    {
      g_graphicsContext.Lock();
      g_graphicsContext.Unlock();

      if (m_pPlayer)
      {
        if (m_pPlayer->IsPaused())
        {
          g_graphicsContext.Lock();
          m_pd3dDevice->Clear( 0L, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, 0x00010001, 1.0f, 0L );
          extern void xbox_video_render_update();
          xbox_video_render_update();
          RenderFullScreen();
          m_gWindowManager.Render();
          m_pd3dDevice->BlockUntilVerticalBlank();
          m_pd3dDevice->Present( NULL, NULL, NULL, NULL );
          g_graphicsContext.Unlock();
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

  g_graphicsContext.Lock();

  // draw GUI
  g_graphicsContext.Clear();
  //SWATHWIDTH of 2 improves fillrates (performance investigator)
  m_pd3dDevice->SetRenderState(D3DRS_SWATHWIDTH, 2);

  // render current window/dialog
  m_gWindowManager.Render();

  // check if we're playing a file
  if (g_graphicsContext.IsOverlayAllowed())
  {
    // if we're playing a movie
    if ( IsPlayingVideo() && m_gWindowManager.GetActiveWindow() != WINDOW_FULLSCREEN_VIDEO)
    {
      // then show video overlay window
      m_guiVideoOverlay.Render();
    }
    else if ( IsPlayingAudio() )
    {
      // audio show audio overlay window
      m_guiMusicOverlay.Render();
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
            pFont->DrawText(60, 50, 0xffff0000, L"REC"); //Draw REC in RED
          if (iBlinkRecord > 50)
            iBlinkRecord = 0;
        }
      }
    }

    // If we have the remote codes enabled, then show them
    if (g_stSettings.m_bDisplayRemoteCodes)
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
          pFont->DrawText( 60, 60, 0xffffffff, wszText);
#else

          if (g_stSettings.m_bShowFreeMem)
            pFont->DrawText( 60, 60, 0xffffffff, wszText);
          else
            pFont->DrawText( 60, 40, 0xffffffff, wszText);
#endif

        }
        iShowRemoteCode--;
      }
    }

#ifndef _DEBUG
    if (g_stSettings.m_bShowFreeMem)
#endif

    {
      // in debug mode, show freememory
      CStdStringW wszText;
      wszText.Format(L"FPS %.0f FreeMem %d/%d", m_fFPS, stat.dwAvailPhys, stat.dwTotalPhys);

      CGUIFont* pFont = g_fontManager.GetFont("font13");
      if (pFont)
      {
        pFont->DrawText( 60, 40, 0xffffffff, wszText);
      }
    }
  }
  // Present the backbuffer contents to the display
  m_pd3dDevice->Present( NULL, NULL, NULL, NULL );
  g_graphicsContext.Unlock();

}

void CApplication::OnKey(CKey& key)
{
  // Turn the mouse off, as we've just got a keypress from controller or remote
  g_Mouse.SetInactive();
  CAction action;
  // a key has been pressed.
  // Reset the screensaver timer
  // but not for the analog thumbsticks
  if (key.GetButtonCode() != KEY_BUTTON_LEFT_THUMB_STICK &&
      key.GetButtonCode() != KEY_BUTTON_RIGHT_THUMB_STICK &&
      key.GetButtonCode() != KEY_BUTTON_LEFT_THUMB_BUTTON)
  {
    // reset harddisk spindown timer
    m_bSpinDown = false;
    m_bNetworkSpinDown = false;



    ResetScreenSaver();
    if (ResetScreenSaverWindow())
      return ;
  }

  // get the current active window
  int iWin = m_gWindowManager.GetActiveWindow();
  // change this if we have a dialog up
  if (m_gWindowManager.IsRouted())
  {
    iWin = m_gWindowManager.GetTopMostRoutedWindowID();
  }
  if (iWin == WINDOW_FULLSCREEN_VIDEO)
  {
    // current active window is full screen video.
    // check if OSD is visible
    CGUIWindowFullScreen *pFSWin = (CGUIWindowFullScreen *)m_gWindowManager.GetWindow(WINDOW_FULLSCREEN_VIDEO);
    if ( pFSWin->OSDVisible() )
    {
      // yes then use OSD section of keymap.xml to map key->action
      g_buttonTranslator.GetAction(WINDOW_OSD, key, action);
    }
    else if (g_application.m_pPlayer && g_application.m_pPlayer->IsInMenu())
    {
      // if player is in some sort of menu, (ie DVDMENU) map buttons differently
      g_buttonTranslator.GetAction(WINDOW_MEDIA_MENU, key, action);
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
      //check for key received over HttpApi first
      action.wID = 0;
      if (m_pWebServer && pXbmcHttp)
      {
        CKey keyHttp(pXbmcHttp->GetKey());
        if (keyHttp.GetButtonCode() != KEY_INVALID)
          action.wID = (WORD) keyHttp.GetButtonCode(); //Nad
      }
      ;
      if (action.wID == 0)
        // see if we've got an ascii key
        if (g_Keyboard.GetAscii() != 0)
          action.wID = (WORD)g_Keyboard.GetAscii() | KEY_ASCII;
        else
          action.wID = (WORD)g_Keyboard.GetKey() | KEY_VKEY;
    }
    else
      g_buttonTranslator.GetAction(iWin, key, action);
  }

  //  Play a sound based on the action
  g_audioManager.PlayActionSound(action);

  // special case for switching between GUI & fullscreen mode.
  if (action.wID == ACTION_SHOW_GUI)
  { // Switch to fullscreen mode if we can
    if (SwitchToFullScreen())
      return ;
  }

  // handle global functions - form is XBMC.Function()
  if (action.wID == ACTION_BUILT_IN_FUNCTION)
  {
    CUtil::ExecBuiltIn(action.strAction);
    return ;
  }
  // in normal case
  // just pass the action to the current window and let it handle it
  m_gWindowManager.OnAction(action);

  /* handle extra global presses */
  // stop : stops playing current audio song
  if (action.wID == ACTION_STOP)
  {
    StopPlaying();
  }

//HACK - We really should check if a window has handled on action
//the dvdplayer handles these itself
if (!m_strCurrentPlayer.Equals("dvdplayer"))
{
  // previous : play previous song from playlist
  if (action.wID == ACTION_PREV_ITEM)
  {
    g_playlistPlayer.PlayPrevious();
  }

  // next : play next song from playlist
  if (action.wID == ACTION_NEXT_ITEM)
  {
    g_playlistPlayer.PlayNext();
  }
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
      }
      if (action.wID == ACTION_PLAYER_FORWARD || action.wID == ACTION_PLAYER_REWIND)
      {
        if (m_strCurrentPlayer == "sid")
        {
          // sid uses these to track skip
          m_pPlayer->Seek(action.wID == ACTION_PLAYER_FORWARD);
        }
        else
        {
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
        }
      }
      else if (action.wID == ACTION_ANALOG_REWIND || action.wID == ACTION_ANALOG_FORWARD)
      {
        // calculate the speed based on the amount the button is held down
        int iPower = (int)(action.fAmount1 * 5.0f + 0.5f);
        // returns 0 -> 5
        int iSpeed = 1 << iPower;
        if (iSpeed != 1 && action.wID == ACTION_ANALOG_REWIND)
          iSpeed = -iSpeed;
        g_application.SetPlaySpeed(iSpeed);
      }
    }
    // allow play to unpause
    else
    {
      if (action.wID == ACTION_PLAYER_PLAY)
        m_pPlayer->Pause();
    }
  }
  if (action.wID == ACTION_MUTE)
  {
    if (g_stSettings.m_bMute == false)
      Mute();
    else  // already muted
      UnMute();

    if (m_pPlayer)
      m_pPlayer->SetVolume(g_stSettings.m_nVolumeLevel);
  }

  // Check for global volume control
  if (action.wID == ACTION_VOLUME_UP || action.wID == ACTION_VOLUME_DOWN)
  {
    if (g_stSettings.m_bMute == true)
    {
      if (action.wID == ACTION_VOLUME_UP)   // restore level only on volume up
        UnMute();
    }
    else // regular volume change
    {
      // increase or decrease the volume
      if (action.wID == ACTION_VOLUME_UP)
        g_stSettings.m_nVolumeLevel += (int)(action.fAmount1 * 100);
      else
        g_stSettings.m_nVolumeLevel -= (int)(action.fAmount1 * 100);

      // sanity check
      if (g_stSettings.m_nVolumeLevel >= VOLUME_MAXIMUM)
      {
        g_stSettings.m_nVolumeLevel = VOLUME_MAXIMUM;
      }
      if (g_stSettings.m_nVolumeLevel <= VOLUME_MINIMUM)
      {
        g_stSettings.m_nVolumeLevel = VOLUME_MINIMUM;
        Mute();   // activate mute when volume becomes 0
      }
    }

    // tell our hardware to update the volume level...
    if (m_pPlayer)
      m_pPlayer->SetVolume(g_stSettings.m_nVolumeLevel);
    // show visual feedback of volume change...
    if (!m_guiDialogVolumeBar.IsRunning())
      m_guiDialogVolumeBar.Show(m_gWindowManager.GetActiveWindow());
    else
      m_guiDialogVolumeBar.OnAction(action);
  }

}

void CApplication::SetKaiNotification(const CStdString& aCaption, const CStdString& aDescription, CGUIImage* aIcon/*=NULL*/)
{
  // queue toast notification
  if (g_guiSettings.GetBool("XLinkKai.EnableNotifications"))
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

  if (g_guiSettings.GetInt("LCD.Mode") == LCD_MODE_NONE)
    return ;
  long lTimeOut = 1000;
  if ( m_iPlaySpeed != 1)
    lTimeOut = 0;
  if ( ((long)GetTickCount() - lTickCount) >= lTimeOut)
  {
    CStdString strTime;
    CStdString strIcon;
    CStdString strLine;
    CStdString strProgressBar;
    if (IsPlayingVideo())
    {
      // line 0: play symbol current time/total time
      // line 1: movie filename / title
      // line 2: genre
      // line 3: year
      CStdString strTotalTime;
      unsigned int tmpvar = g_application.m_pPlayer->GetTotalTime();
      if (tmpvar != 0)
      {
        int ihour = tmpvar / 3600;
        int imin = (tmpvar - ihour * 3600) / 60;
        int isec = (tmpvar - ihour * 3600) % 60;
        strTotalTime.Format("/%2.2d:%2.2d:%2.2d", ihour, imin, isec);
      }
      else
      {
        strTotalTime = " ";
      }

      strTime = g_infoManager.GetVideoLabel(254); // time

      if (m_iPlaySpeed < 1)
        strIcon.Format("\3 %ix", m_iPlaySpeed);
      else if (m_iPlaySpeed > 1)
        strIcon.Format("\4 %ix", m_iPlaySpeed);
      else if (m_pPlayer->IsPaused())
        strIcon.Format("\7");
      else
        strIcon.Format("\5");
      strLine.Format("%s %s%s", strIcon.c_str(), strTime.c_str(), strTotalTime.c_str());
      g_lcd->SetLine(0, strLine);

      int iLine = 1;
      g_lcd->SetLine(iLine++, g_infoManager.GetVideoLabel(250));  // time

      if (iLine < 4 && g_infoManager.GetVideoLabel(251) != "")  // genre
        g_lcd->SetLine(iLine++, g_infoManager.GetVideoLabel(251));

      strProgressBar = g_lcd->GetProgressBar((double)g_application.m_pPlayer->GetTime() * 0.001f,
                                             (double)g_application.m_pPlayer->GetTotalTime());
      g_lcd->SetLine(iLine++, strProgressBar);
      /*
      if (iLine<4 && m_tagCurrentMovie.m_iYear>1900)
      {
       strLine.Format("%i", m_tagCurrentMovie.m_iYear);
       g_lcd->SetLine(iLine++,strLine);
      }
      */

      if (iLine < 4)
      {
        MEMORYSTATUS stat;
        GlobalMemoryStatus(&stat);
        DWORD dwMegFree = stat.dwAvailPhys / (1024 * 1024);
        strTime.Format("Freemem:%i meg", dwMegFree);
        g_lcd->SetLine(iLine++, strTime);

      }
      while (iLine < 4)
        g_lcd->SetLine(iLine++, "");

    }
    else if (IsPlayingAudio())
    {
      // Show:
      // line 0: play symbol current time/total time
      // line 1: song title
      // line 2: artist
      // line 3: release date
      strTime = g_infoManager.GetMusicLabel(205); // time
      CStdString strDuration = g_infoManager.GetMusicLabel(209);  // duration

      if (m_iPlaySpeed < 1)
        strIcon.Format("\3:%ix", m_iPlaySpeed);
      else if (m_iPlaySpeed > 1)
        strIcon.Format("\4:%ix", m_iPlaySpeed);
      else if (m_pPlayer->IsPaused())
        strIcon.Format("\7");
      else
        strIcon.Format("\5");
      if (strDuration.size())
        strLine.Format("%s %s/%s", strIcon.c_str(), strTime.c_str(), strDuration.c_str());
      else
        strLine.Format("%s %s", strIcon.c_str(), strTime.c_str());
      g_lcd->SetLine(0, strLine);
      int iLine = 1;
      strLine = g_infoManager.GetMusicLabel(200); // title
      if (iLine < 4 && strLine != "")
        g_lcd->SetLine(iLine++, strLine);
      strLine = g_infoManager.GetMusicLabel(202);  // artist
      if (iLine < 4 && strLine != "")
        g_lcd->SetLine(iLine++, strLine);
      strLine = g_infoManager.GetMusicLabel(201);   // album

      strProgressBar = g_lcd->GetProgressBar( (double)(g_application.m_pPlayer->GetTime() * 0.001f - g_infoManager.GetCurrentSongStart() / 75.0f),
                                              (double)g_application.m_pPlayer->GetTotalTime());

      g_lcd->SetLine(iLine++, strProgressBar);

      if (iLine < 4 && strLine != "")
      {
        CStdString strYear = g_infoManager.GetMusicLabel(204);  // year
        if (strYear.size())
        {
          CStdString strYearLine;
          strYearLine.Format("%s (%s)", strLine.c_str(), strYear.c_str());
          g_lcd->SetLine(iLine++, strYearLine);
        }
        else
        {
          g_lcd->SetLine(iLine++, strLine);
        }
      }
      while (iLine < 4)
        g_lcd->SetLine(iLine++, "");
    }
    else
    {
      if (g_guiSettings.GetInt("LCD.Mode") == LCD_MODE_NORMAL)
      {
        // line 0: XBMC running...
        // line 1: time/date
        // line 2: free memory (megs)
        // line 3: GUI resolution
        g_lcd->SetLine(0, "XBMC running...");
        CStdString strDateTime;
        CUtil::Unicode2Ansi(g_infoManager.GetTime() + L" " + g_infoManager.GetDate(true), strDateTime);
        g_lcd->SetLine(1, strDateTime);
        MEMORYSTATUS stat;
        GlobalMemoryStatus(&stat);
        DWORD dwMegFree = stat.dwAvailPhys / (1024 * 1024);
        strTime.Format("Freemem:%i meg", dwMegFree);
        g_lcd->SetLine(2, strTime);
        int iResolution = g_graphicsContext.GetVideoResolution();
        strTime.Format("%ix%i %s", g_settings.m_ResInfo[iResolution].iWidth, g_settings.m_ResInfo[iResolution].iHeight, g_settings.m_ResInfo[iResolution].strMode);
        g_lcd->SetLine(3, strTime);
      }
      if (g_guiSettings.GetInt("LCD.Mode") == LCD_MODE_NOTV)
      {
        // line 0: window name like   My music/songs
        // line 1: current control or selected item
        // line 2: time/date
        // line 3: free memory (megs)
        CStdString strTmp;
        int iWin = m_gWindowManager.GetActiveWindow();
        CGUIWindow* pWindow = m_gWindowManager.GetWindow(iWin);
        if (pWindow)
        {
          CStdString strLine;
          wstring wstrLine;
          wstrLine = g_localizeStrings.Get(iWin);
          CUtil::Unicode2Ansi(wstrLine, strLine);
          g_lcd->SetLine(0, strLine);

          int iControl = pWindow->GetFocusedControl();
          CGUIControl* pControl = (CGUIControl* )pWindow->GetControl(iControl);
          if (pControl)
          {
            if (pControl->GetControlType() == CGUIControl::GUICONTROL_BUTTON)
              g_lcd->SetLine(1, ((CGUIButtonControl*)pControl)->GetLabel());
            else if (pControl->GetControlType() == CGUIControl::GUICONTROL_SPIN)
            {
              CGUISpinControl* pSpinControl = (CGUISpinControl*)pControl;
              strTmp.Format("%i/%i", 1 + pSpinControl->GetValue(), pSpinControl->GetMaximum());
              g_lcd->SetLine(1, strTmp);
            }
            else if (pControl->GetControlType() == CGUIControl::GUICONTROL_LABEL)
            {
              CGUIListControl* pListControl = (CGUIListControl*)pControl;
              pListControl->GetSelectedItem(strTmp);
              g_lcd->SetLine(1, strTmp);
            }
            else if (pControl->GetControlType() == CGUIControl::GUICONTROL_THUMBNAIL)
            {
              CGUIThumbnailPanel* pThumbControl = (CGUIThumbnailPanel*)pControl;
              pThumbControl->GetSelectedItem(strTmp);
              g_lcd->SetLine(1, strTmp);
            }
            else if (pControl->GetControlType() == CGUIControl::GUICONTROL_LIST)
            {
              CGUIListControl* pListControl = (CGUIListControl*)pControl;
              pListControl->GetSelectedItem(strTmp);
              g_lcd->SetLine(1, strTmp);
            }
          }
          else
            g_lcd->SetLine(1, " ");
          CStdString strDateTime;
          CUtil::Unicode2Ansi(g_infoManager.GetTime() + L" " + g_infoManager.GetDate(true), strDateTime);
          g_lcd->SetLine(2, strDateTime);
          MEMORYSTATUS stat;
          GlobalMemoryStatus(&stat);
          DWORD dwMegFree = stat.dwAvailPhys / (1024 * 1024);
          strLine.Format("Freemem:%i meg", dwMegFree);
          g_lcd->SetLine(3, strLine);
        }
      }
    }
    lTickCount = GetTickCount();
  }
}

void CApplication::FrameMove()
{
  CKaiClient::GetInstance()->DoWork();
  if (m_guiDialogKaiToast.DoWork())
  {
    if (!m_guiDialogKaiToast.IsRunning())
    {
      m_guiDialogKaiToast.Show(m_gWindowManager.GetActiveWindow());
    }
  }

  // Update display of the dialog if
  // we are scanning for new music info
  m_guiDialogMusicScan.UpdateState();

  if (g_lcd)
    UpdateLCD();
  // read raw input from controller, remote control, mouse and keyboard
  ReadInput();
  // process mouse actions
  if (g_Mouse.IsActive())
  {
    // Reset the screensaver
    ResetScreenSaver();
    if (ResetScreenSaverWindow())
      return ;

    // call OnAction with ACTION_MOUSE
    CAction action;
    action.wID = ACTION_MOUSE;
    action.fAmount1 = (float) m_guiPointer.GetPosX();
    action.fAmount2 = (float) m_guiPointer.GetPosY();
    // send mouse event to the music + video overlays, if they're enabled
    if (g_graphicsContext.IsOverlayAllowed())
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
  // process the buttons received via the HttpApi
  if (m_pWebServer && pXbmcHttp)
  {
    CKey keyHttp(pXbmcHttp->GetKey());
    if (keyHttp.GetButtonCode() != KEY_INVALID)
    {
      OnKey(keyHttp);
      pXbmcHttp->ResetKey();
      return ;
    }
  };
  // process the keyboard buttons etc.
  BYTE vkey = g_Keyboard.GetKey();
  if (vkey)
  {
    // got a valid keypress - convert to a key code
    WORD wkeyID = (WORD)vkey | KEY_VKEY;
    //  CLog::DebugLog("Keyboard: time=%i key=%i", timeGetTime(), vkey);
    CKey key(wkeyID);
    OnKey(key);
    return ;
  }

  XBIR_REMOTE* pRemote = &m_DefaultIR_Remote;
  XBGAMEPAD* pGamepad = &m_DefaultGamepad;

  WORD wButtons = pGamepad->wButtons;
  WORD wRemotes = pRemote->wButtons;
  WORD wDpad = wButtons & (XINPUT_GAMEPAD_DPAD_UP | XINPUT_GAMEPAD_DPAD_DOWN | XINPUT_GAMEPAD_DPAD_LEFT | XINPUT_GAMEPAD_DPAD_RIGHT);

  BYTE bLeftTrigger = pGamepad->bAnalogButtons[XINPUT_GAMEPAD_LEFT_TRIGGER];
  BYTE bRightTrigger = pGamepad->bAnalogButtons[XINPUT_GAMEPAD_RIGHT_TRIGGER];

  // pass them through the delay
  // we don't pass the remote through, as delay is handled in the XBInputEx class.
  WORD wDir = m_ctrDpad.DpadInput(wDpad, 0 != bLeftTrigger, 0 != bRightTrigger);

  bool bGotKey = false;

  static lastAnalogKey = 0;
  bool bIsDown = false;

  // map all controller & remote actions to their keys
  if (pGamepad->fX1 || pGamepad->fY1)
  {
    bGotKey = true;
    CKey key(KEY_BUTTON_LEFT_THUMB_STICK, bLeftTrigger, bRightTrigger, pGamepad->fX1, pGamepad->fY1, pGamepad->fX2, pGamepad->fY2);
    OnKey(key);
  }
  if (pGamepad->fX2 || pGamepad->fY2)
  {
    bGotKey = true;
    CKey key(KEY_BUTTON_RIGHT_THUMB_STICK, bLeftTrigger, bRightTrigger, pGamepad->fX1, pGamepad->fY1, pGamepad->fX2, pGamepad->fY2);
    OnKey(key);
  }
  // direction specific keys (for defining different actions for each direction)
  // left thumb stick
  bIsDown = (pGamepad->fY1 > 0 && pGamepad->fX1 < pGamepad->fY1 && -pGamepad->fX1 < pGamepad->fY1);
  if (lastAnalogKey == KEY_BUTTON_LEFT_THUMB_STICK_UP || bIsDown)
  {
    bGotKey = true;
    lastAnalogKey = bIsDown ? KEY_BUTTON_LEFT_THUMB_STICK_UP : 0;
    CKey key(KEY_BUTTON_LEFT_THUMB_STICK_UP, bLeftTrigger, bRightTrigger, pGamepad->fX1, pGamepad->fY1, pGamepad->fX2, pGamepad->fY2);
    OnKey(key);
  }
  bIsDown = (pGamepad->fY1 < 0 && pGamepad->fX1 < -pGamepad->fY1 && -pGamepad->fX1 < -pGamepad->fY1);
  if (lastAnalogKey == KEY_BUTTON_LEFT_THUMB_STICK_DOWN || bIsDown)
  {
    bGotKey = true;
    lastAnalogKey = bIsDown ? KEY_BUTTON_LEFT_THUMB_STICK_DOWN : 0;
    CKey key(KEY_BUTTON_LEFT_THUMB_STICK_DOWN, bLeftTrigger, bRightTrigger, pGamepad->fX1, pGamepad->fY1, pGamepad->fX2, -pGamepad->fY2);
    OnKey(key);
  }
  bIsDown = (pGamepad->fX1 > 0 && pGamepad->fY1 < pGamepad->fX1 && -pGamepad->fY1 < pGamepad->fX1);
  if (lastAnalogKey == KEY_BUTTON_LEFT_THUMB_STICK_RIGHT || bIsDown)
  {
    bGotKey = true;
    lastAnalogKey = bIsDown ? KEY_BUTTON_LEFT_THUMB_STICK_RIGHT : 0;
    CKey key(KEY_BUTTON_LEFT_THUMB_STICK_RIGHT, bLeftTrigger, bRightTrigger, pGamepad->fX1, pGamepad->fY1, pGamepad->fX2, pGamepad->fY2);
    OnKey(key);
  }
  bIsDown = (pGamepad->fX1 < 0 && pGamepad->fY1 < -pGamepad->fX1 && -pGamepad->fY1 < -pGamepad->fX1);
  if (lastAnalogKey == KEY_BUTTON_LEFT_THUMB_STICK_LEFT || bIsDown)
  {
    bGotKey = true;
    lastAnalogKey = bIsDown ? KEY_BUTTON_LEFT_THUMB_STICK_LEFT : 0;
    CKey key(KEY_BUTTON_LEFT_THUMB_STICK_LEFT, bLeftTrigger, bRightTrigger, pGamepad->fX1, pGamepad->fY1, -pGamepad->fX2, pGamepad->fY2);
    OnKey(key);
  }
  // right thumb stick
  bIsDown = (pGamepad->fY2 > 0 && pGamepad->fX2 < pGamepad->fY2 && -pGamepad->fX2 < pGamepad->fY2);
  if (lastAnalogKey == KEY_BUTTON_RIGHT_THUMB_STICK_UP || bIsDown)
  {
    bGotKey = true;
    lastAnalogKey = bIsDown ? KEY_BUTTON_RIGHT_THUMB_STICK_UP : 0;
    CKey key(KEY_BUTTON_RIGHT_THUMB_STICK_UP, bLeftTrigger, bRightTrigger, pGamepad->fX1, pGamepad->fY1, pGamepad->fX2, pGamepad->fY2);
    OnKey(key);
  }
  bIsDown = (pGamepad->fY2 < 0 && pGamepad->fX2 < -pGamepad->fY2 && -pGamepad->fX2 < -pGamepad->fY2);
  if (lastAnalogKey == KEY_BUTTON_RIGHT_THUMB_STICK_DOWN || bIsDown)
  {
    bGotKey = true;
    lastAnalogKey = bIsDown ? KEY_BUTTON_RIGHT_THUMB_STICK_DOWN : 0;
    CKey key(KEY_BUTTON_RIGHT_THUMB_STICK_DOWN, bLeftTrigger, bRightTrigger, pGamepad->fX1, pGamepad->fY1, pGamepad->fX2, -pGamepad->fY2);
    OnKey(key);
  }
  bIsDown = (pGamepad->fX2 > 0 && pGamepad->fY2 < pGamepad->fX2 && -pGamepad->fY2 < pGamepad->fX2);
  if (lastAnalogKey == KEY_BUTTON_RIGHT_THUMB_STICK_RIGHT || bIsDown)
  {
    bGotKey = true;
    lastAnalogKey = bIsDown ? KEY_BUTTON_RIGHT_THUMB_STICK_RIGHT : 0;
    CKey key(KEY_BUTTON_RIGHT_THUMB_STICK_RIGHT, bLeftTrigger, bRightTrigger, pGamepad->fX1, pGamepad->fY1, pGamepad->fX2, pGamepad->fY2);
    OnKey(key);
  }
  bIsDown = (pGamepad->fX2 < 0 && pGamepad->fY2 < -pGamepad->fX2 && -pGamepad->fY2 < -pGamepad->fX2);
  if (lastAnalogKey == KEY_BUTTON_RIGHT_THUMB_STICK_LEFT || bIsDown)
  {
    bGotKey = true;
    lastAnalogKey = bIsDown ? KEY_BUTTON_RIGHT_THUMB_STICK_LEFT : 0;
    CKey key(KEY_BUTTON_RIGHT_THUMB_STICK_LEFT, bLeftTrigger, bRightTrigger, pGamepad->fX1, pGamepad->fY1, -pGamepad->fX2, pGamepad->fY2);
    OnKey(key);
  }
  // analog trigger detection
  // with code to make sure it resets on release
  if (bLeftTrigger || lastAnalogKey == KEY_BUTTON_LEFT_ANALOG_TRIGGER)
  {
    lastAnalogKey = bLeftTrigger ? KEY_BUTTON_LEFT_ANALOG_TRIGGER : 0;
    bGotKey = true;
    CKey key(KEY_BUTTON_LEFT_ANALOG_TRIGGER, bLeftTrigger, bRightTrigger, pGamepad->fX1, pGamepad->fY1, pGamepad->fX2, pGamepad->fY2);
    OnKey(key);
  }
  if (bRightTrigger || lastAnalogKey == KEY_BUTTON_RIGHT_ANALOG_TRIGGER)
  {
    lastAnalogKey = bRightTrigger ? KEY_BUTTON_RIGHT_ANALOG_TRIGGER : 0;
    bGotKey = true;
    CKey key(KEY_BUTTON_RIGHT_ANALOG_TRIGGER, bLeftTrigger, bRightTrigger, pGamepad->fX1, pGamepad->fY1, pGamepad->fX2, pGamepad->fY2);
    OnKey(key);
  }

  if ( wDir & DC_LEFTTRIGGER)
  {
    bGotKey = true;
    CKey key(KEY_BUTTON_LEFT_TRIGGER, bLeftTrigger, bRightTrigger, pGamepad->fX1, pGamepad->fY1, pGamepad->fX2, pGamepad->fY2);
    OnKey(key);
    return ;
  }
  if ( wDir & DC_RIGHTTRIGGER)
  {
    bGotKey = true;
    CKey key(KEY_BUTTON_RIGHT_TRIGGER, bLeftTrigger, bRightTrigger, pGamepad->fX1, pGamepad->fY1, pGamepad->fX2, pGamepad->fY2);
    OnKey(key);
    return ;
  }
  if ( wDir & DC_LEFT )
  {
    bGotKey = true;
    CKey key(KEY_BUTTON_DPAD_LEFT, bLeftTrigger, bRightTrigger, pGamepad->fX1, pGamepad->fY1, pGamepad->fX2, pGamepad->fY2);
    OnKey(key);
    return ;
  }
  if ( wDir & DC_RIGHT)
  {
    bGotKey = true;
    CKey key(KEY_BUTTON_DPAD_RIGHT, bLeftTrigger, bRightTrigger, pGamepad->fX1, pGamepad->fY1, pGamepad->fX2, pGamepad->fY2);
    OnKey(key);
    return ;
  }
  if ( wDir & DC_UP )
  {
    bGotKey = true;
    CKey key(KEY_BUTTON_DPAD_UP, bLeftTrigger, bRightTrigger, pGamepad->fX1, pGamepad->fY1, pGamepad->fX2, pGamepad->fY2);
    OnKey(key);
    return ;
  }
  if ( wDir & DC_DOWN )
  {
    bGotKey = true;
    CKey key(KEY_BUTTON_DPAD_DOWN, bLeftTrigger, bRightTrigger, pGamepad->fX1, pGamepad->fY1, pGamepad->fX2, pGamepad->fY2);
    OnKey(key);
    return ;
  }


  if ( pGamepad->wPressedButtons & XINPUT_GAMEPAD_BACK )
  {
    bGotKey = true;
    CKey key(KEY_BUTTON_BACK, bLeftTrigger, bRightTrigger, pGamepad->fX1, pGamepad->fY1, pGamepad->fX2, pGamepad->fY2);
    OnKey(key);
    return ;
  }
  if ( pGamepad->wPressedButtons & XINPUT_GAMEPAD_START)
  {
    bGotKey = true;
    CKey key(KEY_BUTTON_START, bLeftTrigger, bRightTrigger, pGamepad->fX1, pGamepad->fY1, pGamepad->fX2, pGamepad->fY2);
    OnKey(key);
    return ;
  }

  if ( pGamepad->wPressedButtons & XINPUT_GAMEPAD_LEFT_THUMB)
  {
    bGotKey = true;
    CKey key(KEY_BUTTON_LEFT_THUMB_BUTTON, bLeftTrigger, bRightTrigger, pGamepad->fX1, pGamepad->fY1, pGamepad->fX2, pGamepad->fY2);
    OnKey(key);
    return ;
  }

  if ( pGamepad->wPressedButtons & XINPUT_GAMEPAD_RIGHT_THUMB)
  {
    bGotKey = true;
    CKey key(KEY_BUTTON_RIGHT_THUMB_BUTTON, bLeftTrigger, bRightTrigger, pGamepad->fX1, pGamepad->fY1, pGamepad->fX2, pGamepad->fY2);
    OnKey(key);
    return ;
  }


  if (pGamepad->bPressedAnalogButtons[XINPUT_GAMEPAD_A])
  {
    bGotKey = true;
    CKey key(KEY_BUTTON_A, bLeftTrigger, bRightTrigger, pGamepad->fX1, pGamepad->fY1, pGamepad->fX2, pGamepad->fY2);
    OnKey(key);
    return ;
  }
  if (pGamepad->bPressedAnalogButtons[XINPUT_GAMEPAD_B])
  {
    bGotKey = true;
    CKey key(KEY_BUTTON_B, bLeftTrigger, bRightTrigger, pGamepad->fX1, pGamepad->fY1, pGamepad->fX2, pGamepad->fY2);
    OnKey(key);
    return ;
  }

  if (pGamepad->bPressedAnalogButtons[XINPUT_GAMEPAD_X])
  {
    bGotKey = true;
    CKey key(KEY_BUTTON_X, bLeftTrigger, bRightTrigger, pGamepad->fX1, pGamepad->fY1, pGamepad->fX2, pGamepad->fY2);
    OnKey(key);
    return ;
  }
  if (pGamepad->bPressedAnalogButtons[XINPUT_GAMEPAD_Y])
  {
    bGotKey = true;
    CKey key(KEY_BUTTON_Y, bLeftTrigger, bRightTrigger, pGamepad->fX1, pGamepad->fY1, pGamepad->fX2, pGamepad->fY2);
    OnKey(key);
    return ;
  }
  if (pGamepad->bPressedAnalogButtons[XINPUT_GAMEPAD_BLACK])
  {
    bGotKey = true;
    CKey key(KEY_BUTTON_BLACK, bLeftTrigger, bRightTrigger, pGamepad->fX1, pGamepad->fY1, pGamepad->fX2, pGamepad->fY2);
    OnKey(key);
    return ;
  }
  if (pGamepad->bPressedAnalogButtons[XINPUT_GAMEPAD_WHITE])
  {
    bGotKey = true;
    CKey key(KEY_BUTTON_WHITE, bLeftTrigger, bRightTrigger, pGamepad->fX1, pGamepad->fY1, pGamepad->fX2, pGamepad->fY2);
    OnKey(key);
    return ;
  }

  switch (wRemotes)
  {
    // 0 is invalid
  case 0:
    break;
    // Map all other keys unchanged
  default:
    {
      bGotKey = true;
      CKey key(wRemotes);
      OnKey(key);
      break;
    }
  }
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
    m_bStop = true;
    CLog::Log(LOGNOTICE, "stop all");

    CKaiClient::GetInstance()->RemoveObserver();

    StopServices();

    if (m_pPlayer)
    {
      CLog::Log(LOGNOTICE, "stop mplayer");
      delete m_pPlayer;
      m_pPlayer = NULL;
    }

    // if we have an active connection to iTunes, stop that too
    if (g_application.m_DAAPPtr)
    {
      CDAAPDirectory *objDAAP;

      objDAAP = new CDAAPDirectory();
      objDAAP->CloseDAAP();
    }

    if (m_guiDialogMusicScan.IsRunning())
      m_guiDialogMusicScan.StopScanning();

    //g_lcd->StopThread();
    CLog::Log(LOGNOTICE, "stop python");
    g_applicationMessenger.Cleanup();
    g_pythonParser.FreeResources();

    CLog::Log(LOGNOTICE, "unload skin");
    m_guiMusicOverlay.FreeResources();
    m_guiVideoOverlay.FreeResources();
    m_guiPointer.FreeResources();
    m_guiDialogVolumeBar.FreeResources();
    m_guiDialogKaiToast.FreeResources();
    m_guiDialogMusicScan.FreeResources();
    g_fontManager.Clear();
    m_gWindowManager.DeInitialize();
    g_audioManager.DeInitialize();
    g_TextureManager.Cleanup();

    CLog::Log(LOGNOTICE, "unload sections");
    CSectionLoader::UnloadAll();
    CLog::Log(LOGNOTICE, "destroy");
    Destroy();
    CLog::Log(LOGNOTICE, "stopped");
  }
  catch (...)
  {}
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
  if (item.IsInternetStream())
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

  CPlayList& playlist = (*pPlayList);

  // no songs in playlist just return
  if (playlist.size() == 0)
    return false;

  // how many songs are in the new playlist
  if (playlist.size() == 1)
  {
    // just 1 song? then play it (no need to have a playlist of 1 song)
    CPlayList::CPlayListItem item = playlist[0];
    return g_application.PlayFile(CFileItem(item));
  }

  // clear current playlist
  g_playlistPlayer.GetPlaylist(iPlaylist).Clear();

  // if autoshuffle playlist on load option is enabled
  //  then shuffle the playlist
  // (dont do this for shoutcast .pls files)
  if (playlist.size())
  {
    const CPlayList::CPlayListItem& playListItem = playlist[0];
    if (!playListItem.IsShoutCast() && g_guiSettings.GetBool("MusicLibrary.ShufflePlaylistsOnLoad"))
      pPlayList->Shuffle();
  }

  // add each item of the playlist to the playlistplayer
  for (int i = 0; i < (int)pPlayList->size(); ++i)
  {
    const CPlayList::CPlayListItem& playListItem = playlist[i];
    CStdString strLabel = playListItem.GetDescription();
    if (strLabel.size() == 0)
      strLabel = CUtil::GetTitleFromPath(playListItem.GetFileName());

    CPlayList::CPlayListItem playlistItem;
    playlistItem.SetDescription(playListItem.GetDescription());
    playlistItem.SetDuration(playListItem.GetDuration());
    playlistItem.SetFileName(playListItem.GetFileName());
    g_playlistPlayer.GetPlaylist( iPlaylist ).Add(playlistItem);
  }

  // if we got a playlist
  if (g_playlistPlayer.GetPlaylist( iPlaylist ).size() )
  {
    // then get 1st song
    CPlayList& playlist = g_playlistPlayer.GetPlaylist( iPlaylist );
    const CPlayList::CPlayListItem& item = playlist[0];

    // and start playing it
    g_playlistPlayer.SetCurrentPlaylist(iPlaylist);
    g_playlistPlayer.Reset();
    if (g_playlistPlayer.ShuffledPlay(iPlaylist))
    {
      // if shuffled dont start on first song
      g_playlistPlayer.SetCurrentSong(0);
      g_playlistPlayer.PlayNext();
    }
    else
      g_playlistPlayer.Play(0);
    return true;
  }
  return false;
}

bool CApplication::PlayFile(const CFileItem& item, bool bRestart)
{
  if (item.IsPlayList())
    return false;
  float AVDelay = 0;

  m_iPlaySpeed = 1;
  if (!bRestart)
  {
    OutputDebugString("new file set audiostream:0\n");
    // Switch to default options
    g_stSettings.m_defaultVideoSettings.m_AdjustFrameRate = g_guiSettings.GetBool("MyVideos.FrameRateConversions");
    g_stSettings.m_defaultVideoSettings.m_Deinterlace = g_guiSettings.GetBool("PostProcessing.DeInterlace");
    g_stSettings.m_defaultVideoSettings.m_FilmGrain = g_guiSettings.GetBool("Filters.Noise") ? g_guiSettings.GetInt("Filters.NoiseLevel") : 0;
    g_stSettings.m_defaultVideoSettings.m_ViewMode = g_guiSettings.GetInt("MyVideos.ViewMode");
    g_stSettings.m_defaultVideoSettings.m_Brightness = g_guiSettings.GetInt("MyVideos.Brightness");
    g_stSettings.m_defaultVideoSettings.m_Contrast = g_guiSettings.GetInt("MyVideos.Contrast");
    g_stSettings.m_defaultVideoSettings.m_Gamma = g_guiSettings.GetInt("MyVideos.Gamma");
    g_stSettings.m_currentVideoSettings = g_stSettings.m_defaultVideoSettings;
    // see if we have saved options in the database
    if (item.IsVideo())
    {
      CVideoDatabase dbs;
      // open the d/b and retrieve the bookmarks for the current movie
      dbs.Open();
      dbs.GetVideoSettings(item.m_strPath, g_stSettings.m_currentVideoSettings);
      dbs.Close();
    }
  }
  else
  {
    AVDelay = m_pPlayer->GetAVDelay();
  }

  CURL url(item.m_strPath);
  CStdString strNewPlayer = "mplayer";
  if ( url.GetProtocol() == "cdda")
  {
    strNewPlayer = "cdda";
  }
  else if (strcmp(g_stSettings.m_szExternalDVDPlayer, "dvdplayerbeta") == 0 &&
           (item.IsDVD() || item.IsDVDFile() || item.IsDVDImage()))
  {
    strNewPlayer = "dvdplayer";
  }
  else if (ModPlayer::IsSupportedFormat(url.GetFileType()))
  {
    strNewPlayer = "mod";
  }
  else if (url.GetFileType() == "sid")
  {
    strNewPlayer = "sid";
  }
  else if (url.GetFileType() == "mp3")
  {
    if (CUtil::FileExists("Q:\\system\\players\\paplayer\\in_mp3.dll"))  strNewPlayer = "paplayer";
  }
  // Check if we are moving from one cue sheet item to the next
  // need:
  // 1.  player to exist
  // 2.  current play time > endtime of current song.
  // 3.  next item's startoffset>0
  // 4.  next item start offset == current items end offset
  // 5.  current and next item based on same media file.
  if (m_pPlayer && (m_pPlayer->GetTime() > m_itemCurrentFile.m_lEndOffset*(__int64)1000 / 75))
  {
    if (item.m_lStartOffset > 0 && item.m_lStartOffset == m_itemCurrentFile.m_lEndOffset &&
        item.m_strPath == m_itemCurrentFile.m_strPath && m_pPlayer)
    { // this is the next cue sheet item, so we don't have to restart the player
      // just update our display etc.
      m_itemCurrentFile = item;
      g_infoManager.SetCurrentItem(m_itemCurrentFile);
      m_guiMusicOverlay.Update();
      m_guiVideoOverlay.Update();
      m_dwIdleTime = timeGetTime();
      return true;
    }
  }
  //We have to stop parsing a cdg before mplayer is deallocated
  m_CdgParser.Stop();
  // We should restart the player, unless the previous and next tracks are using the cdda player
  // (allows gapless cdda playback)
  if (m_pPlayer && !(m_strCurrentPlayer == strNewPlayer && (m_strCurrentPlayer == "cdda" || m_strCurrentPlayer == "dvdplayer")))
  {
    if (1 || m_strCurrentPlayer != strNewPlayer || !m_itemCurrentFile.IsAudio() )
    {
      delete m_pPlayer;
      m_pPlayer = NULL;
    }
  }

  m_itemCurrentFile = item;
  m_strCurrentPlayer = strNewPlayer;
  if (!m_pPlayer)
  {
    CPlayerCoreFactory factory;
    m_pPlayer = factory.CreatePlayer(strNewPlayer, *this);
  }

  bool bResult = m_pPlayer->OpenFile(m_itemCurrentFile, m_itemCurrentFile.m_lStartOffset * 1000 / 75);
  if (bResult)
  {
    if ( IsPlayingVideo())
    {
      //pause video until screen is setup
      m_pPlayer->Pause();
    }
    g_infoManager.SetCurrentItem(m_itemCurrentFile);
    m_guiMusicOverlay.Update();
    m_guiVideoOverlay.Update();

    if (m_itemCurrentFile.IsAudio() && !m_itemCurrentFile.IsInternetStream() && g_guiSettings.GetBool("Karaoke.Enabled"))
      m_CdgParser.Start(m_itemCurrentFile.m_strPath);

    m_dwIdleTime = timeGetTime();


    if (bRestart)
    {
      m_pPlayer->SetAVDelay(AVDelay);
    }

    // if file happens to contain video stream
    if ( IsPlayingVideo())
    {
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
      if (bCanSwitch) SwitchToFullScreen();
      //screen is setup, resume playing
      m_pPlayer->Pause();
    }
  }
  return bResult;
}

void CApplication::OnPlayBackEnded()
{
  //playback ended
  m_iPlaySpeed = 1;

  // informs python script currently running playback has ended
  // (does nothing if python is not loaded)
  g_pythonParser.OnPlayBackEnded();

  OutputDebugString("Playback has finished\n");
  CGUIMessage msg(GUI_MSG_PLAYBACK_ENDED, 0, 0, 0, 0, NULL);
  m_gWindowManager.SendThreadMessage(msg);
  StartLEDControl(false);
}

void CApplication::OnPlayBackStarted()
{
  // informs python script currently running playback has started
  // (does nothing if python is not loaded)
  g_pythonParser.OnPlayBackStarted();

  CGUIMessage msg(GUI_MSG_PLAYBACK_STARTED, 0, 0, 0, 0, NULL);
  m_gWindowManager.SendThreadMessage(msg);

  CheckNetworkHDSpinDown(true);

  StartLEDControl(true);
}

void CApplication::OnPlayBackStopped()
{
  // informs python script currently running playback has ended
  // (does nothing if python is not loaded)
  g_pythonParser.OnPlayBackStopped();

  m_CdgParser.Free();
  OutputDebugString("Playback was stopped\n");
  CGUIMessage msg( GUI_MSG_PLAYBACK_STOPPED, 0, 0, 0, 0, NULL );
  m_gWindowManager.SendMessage(msg);

  StartLEDControl(false);
}

bool CApplication::IsPlaying() const
{
  if (!m_pPlayer)
    return false;
  if (!m_pPlayer->IsPlaying())
    return false;
  return true;
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
    // turn off visualisation window when stopping
    if (iWin == WINDOW_VISUALISATION)
      m_gWindowManager.PreviousWindow();
    if ( IsPlayingVideo() )
    { // save our position for resuming at a later date
      g_stSettings.m_currentVideoSettings.m_ResumeTime = (int)(m_pPlayer->GetTime() * 75 / 1000); // need it in frames (75ths of a second)
    }
    m_pPlayer->CloseFile();
  }

  OnPlayBackStopped();
}


bool CApplication::NeedRenderFullScreen()
{
  if (m_gWindowManager.GetActiveWindow() == WINDOW_FULLSCREEN_VIDEO)
  {
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
    CGUIWindowFullScreen *pFSWin = (CGUIWindowFullScreen *)m_gWindowManager.GetWindow(WINDOW_FULLSCREEN_VIDEO);
    if (!pFSWin)
      return ;
    pFSWin->RenderFullScreen();
  }
}

void CApplication::ResetScreenSaver()
{
  if (m_bInactive)
  {
    m_dwSaverTick = timeGetTime(); // Start the timer going ...
  }
}

bool CApplication::ResetScreenSaverWindow()
{
  m_bInactive = false;  // reset the inactive flag as a key has been pressed
  // if Screen saver is active
  if (m_bScreenSave)
  {
    // disable screensaver
    m_bScreenSave = false;

    // if matrix trails screensaver is active
    int iWin = m_gWindowManager.GetActiveWindow();
    if (iWin == WINDOW_SCREENSAVER)
    {
      // then show previous window
      m_gWindowManager.PreviousWindow();
    }
    else
    {
      // Fade to dim or black screensaver is active
      // fade in
      float fFadeLevel = 1.0f;
      CStdString strScreenSaver = g_guiSettings.GetString("ScreenSaver.Mode");
      if (strScreenSaver == "Dim")
      {
        fFadeLevel = (float)g_guiSettings.GetInt("ScreenSaver.DimLevel") / 100;
      }
      else if (strScreenSaver == "Fade")
      {
        fFadeLevel = 0;
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
    }
    return true;
  }
  else
  {
    return false;
  }
}

void CApplication::CheckScreenSaver()
{
  if ( m_gWindowManager.IsRouted())
    return ;
  if (g_guiSettings.GetInt("LCD.Mode") == LCD_MODE_NOTV)
    return ;

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
      if ( (long)(timeGetTime() - m_dwSaverTick) >= (long)(g_guiSettings.GetInt("ScreenSaver.Time")*60*1000L) )
      {
        //yes, show the screensaver
        ActivateScreenSaver();
      }
    }
  }
}

void CApplication::ActivateScreenSaver()
{
  D3DGAMMARAMP Ramp;
  FLOAT fFadeLevel;

  m_bInactive = true;
  m_bScreenSave = true;
  m_dwSaverTick = timeGetTime();  // Save the current time for the shutdown timeout

  CStdString strScreenSaver = g_guiSettings.GetString("ScreenSaver.Mode");
  if (strScreenSaver == "Dim")
  {
    fFadeLevel = (FLOAT) g_guiSettings.GetInt("ScreenSaver.DimLevel") / 100; // 0.07f;
  }
  else if (strScreenSaver == "Black")
  {
    fFadeLevel = 0;
  }
  else if (strScreenSaver != "None")
  {
    if (!IsPlayingVideo())
    {
      m_gWindowManager.ActivateWindow(WINDOW_SCREENSAVER);
      return ;
    }
    else
    {
      fFadeLevel = (FLOAT) g_guiSettings.GetInt("ScreenSaver.DimLevel") / 100; // 0.07f;
    }
  }
  m_pd3dDevice->GetGammaRamp(&m_OldRamp); // Store the old gamma ramp
  // Fade to fFadeLevel
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
    if (g_guiSettings.GetBool("System.ShutDownWhilePlaying")) // shutdown if active
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
    if ( (long)(timeGetTime() - m_dwSaverTick) >= (long)(g_guiSettings.GetInt("System.ShutDownTime")*60*1000L) )
    {
      bool bShutDown = false;
      if (g_guiSettings.GetBool("System.ShutDownWhilePlaying")) // shutdown if active
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
  int iSpinDown = g_guiSettings.GetInt("System.RemotePlayHDSpinDown");
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
      iDuration = m_pPlayer->GetTotalTime();
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
        (iDuration > g_guiSettings.GetInt("System.RemotePlayHDSpinDownMinDuration")*60)
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
          CGUIWindowFullScreen *pFSWin = (CGUIWindowFullScreen *)m_gWindowManager.GetWindow(WINDOW_FULLSCREEN_VIDEO);
          m_bNetworkSpinDown = !pFSWin->OSDVisible();
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
    if ( (m_dwSpinDownTime != 0) && (dwTimeSpan >= ((DWORD)g_guiSettings.GetInt("System.RemotePlayHDSpinDownDelay")*1000UL)) )
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
      if (iMinSpinUp > g_guiSettings.GetInt("System.RemotePlayHDSpinDownDelay")*0.5f)
        iMinSpinUp = (int)(g_guiSettings.GetInt("System.RemotePlayHDSpinDownDelay")*0.5f);
      if (g_infoManager.GetPlayTimeRemaining() == iMinSpinUp)
      { // spin back up
        CIoSupport::SpindownHarddisk(false);
      }
    }
  }
}

void CApplication::CheckHDSpindown()
{
  if (!g_guiSettings.GetInt("System.HDSpinDownTime"))
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
    if ( (m_dwSpinDownTime != 0) && (dwTimeSpan >= ((DWORD)g_guiSettings.GetInt("System.HDSpinDownTime")*60UL*1000UL)) )
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
  m_guiDialogKaiToast.ResetAllControls();
  m_guiDialogMuteBug.ResetAllControls();

}

bool CApplication::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_DVDDRIVE_EJECTED_CD:
    {
      // Update general playlist: Remove DVD playlist items
      int nRemoved = g_playlistPlayer.RemoveDVDItems();
      if ( nRemoved > 0 )
      {
        CGUIMessage msg( GUI_MSG_PLAYLIST_CHANGED, 0, 0, 0, 0, NULL );
        m_gWindowManager.SendMessage( msg );
      }
    }
    break;

  case GUI_MSG_PLAYBACK_STOPPED:
  case GUI_MSG_PLAYBACK_ENDED:
    {
      m_dwIdleTime = timeGetTime();

      // reset our infoManager details
      g_infoManager.ResetCurrentItem();

      // reset our spindown
      m_bNetworkSpinDown = false;
      m_bSpinDown = false;

      if (message.GetMessage() == GUI_MSG_PLAYBACK_ENDED)
      {
        if (m_itemCurrentFile.IsVideo() && g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_VIDEO_TEMP)
        {
          // Video stacking playback ended
          CPlayList& playlist = g_playlistPlayer.GetPlaylist(PLAYLIST_VIDEO_TEMP);

          if (g_playlistPlayer.GetEntriesNotFound() < playlist.size())
          {
            g_playlistPlayer.PlayNext(true);
          }
          else if (m_pPlayer)
          {
            delete m_pPlayer;
            m_pPlayer = 0;
            m_itemCurrentFile.Clear();
          }
        }
        else
        {
          // Normal playback ended
          if (m_pPlayer)
          {
            CPlayList& playlist = g_playlistPlayer.GetPlaylist(g_playlistPlayer.GetCurrentPlaylist());

            if (g_playlistPlayer.GetEntriesNotFound() < playlist.size())
            {
              g_playlistPlayer.PlayNext(true);
            }
            else
            {
              delete m_pPlayer;
              m_pPlayer = 0;
              m_itemCurrentFile.Clear();

            }
          }
        }
      }
      else
      {
        if (m_pPlayer)
        {
          m_itemCurrentFile.Clear();
          delete m_pPlayer;
          m_pPlayer = 0;
        }
      }

      if (!m_pPlayer)
      {
        m_CdgParser.Free();
      }

      if (!IsPlayingVideo() && m_gWindowManager.GetActiveWindow() == WINDOW_FULLSCREEN_VIDEO)
      {
        m_gWindowManager.PreviousWindow();
      }

      if (!IsPlayingAudio() && g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_NONE && m_gWindowManager.GetActiveWindow() == WINDOW_VISUALISATION)
      {
        g_settings.Save();  // save vis settings
        m_gWindowManager.PreviousWindow();
      }

      // DVD ejected while playing in vis ?
      if (!IsPlayingAudio() && (m_itemCurrentFile.IsCDDA() || m_itemCurrentFile.IsDVD() || m_itemCurrentFile.IsISO9660()) && !CDetectDVDMedia::IsDiscInDrive() && m_gWindowManager.GetActiveWindow() == WINDOW_VISUALISATION)
      {
        // yes, disable vis
        g_settings.Save();    // save vis settings
        m_gWindowManager.PreviousWindow();
      }

    }
    break;

  case GUI_MSG_PLAYLISTPLAYER_STARTED:
  case GUI_MSG_PLAYLISTPLAYER_CHANGED:
    {
      if (message.GetParam1() == PLAYLIST_MUSIC || message.GetParam1() == PLAYLIST_MUSIC_TEMP)
      {
        CPlayList::CPlayListItem* pItem = (CPlayList::CPlayListItem*)message.GetLPVOID();
        if (pItem)
        {
          // only Increment Top 100 Counter, if we have not clicked from inside the Top 100 view
          if (g_stSettings.m_iMyMusicStartWindow == WINDOW_MUSIC_TOP100)
          {
            break;
          }
          else
          {
            // Can't write to the musicdatabase while scanning for music info
            if (!m_guiDialogMusicScan.IsRunning())
            {
              if (g_musicDatabase.Open())
              {
                g_musicDatabase.IncrTop100CounterByFileName(pItem->GetFileName());
                g_musicDatabase.Close();
              }
            }
          }
        }
      }
    }
    break;
  case GUI_MSG_PLAYLISTPLAYER_STOPPED:
    {
      if (message.GetParam1() == PLAYLIST_MUSIC || message.GetParam1() == PLAYLIST_MUSIC_TEMP)
      {
        if (m_gWindowManager.GetActiveWindow() == WINDOW_VISUALISATION)
          m_gWindowManager.PreviousWindow();
      }
      m_CdgParser.Free();
      SAFE_DELETE(m_pPlayer);
    }
    break;
  case GUI_MSG_FULLSCREEN:
    { // Switch to fullscreen, if we can
      SwitchToFullScreen();
    }
    break;
  case GUI_MSG_EXECUTE:
    { // user has asked for something to be executed
      CFileItem item(message.GetStringParam(), false);

      if (item.IsPythonScript())
      { // a python script
        g_pythonParser.evalFile(item.m_strPath.c_str());
      }
      else if (item.IsXBE())
      { // an XBE
        CUtil::RunXBE(item.m_strPath.c_str());
      }
      else if (item.IsAudio() || item.IsVideo())
      { // an audio or video file
        PlayFile(item);
        if (IsPlayingVideo() && m_gWindowManager.GetActiveWindow() != WINDOW_FULLSCREEN_VIDEO)
        {
          SwitchToFullScreen();
        }
      }
      else if (CUtil::IsBuiltIn(item.m_strPath))
      {
        CUtil::ExecBuiltIn(item.m_strPath);
      }
    }
  }
  return true;
}

void CApplication::Process()
{
  // checks whats in the DVD drive and tries to autostart the content (xbox games, dvd, cdda, avi files...)
  m_Autorun.HandleAutorun();

  // check if we need to load a new skin
  if (m_dwSkinTime && timeGetTime() >= m_dwSkinTime)
  {
    CGUIMessage msg(GUI_MSG_LOAD_SKIN, -1, m_gWindowManager.GetActiveWindow());
    g_graphicsContext.SendMessage(msg);
  }

  // dispatch the messages generated by python or other threads to the current window
  m_gWindowManager.DispatchThreadMessages();

  // process messages which have to be send to the gui
  // (this can only be done after m_gWindowManager.Render())
  g_applicationMessenger.ProcessWindowMessages();

  // process any Python scripts
  g_pythonParser.Process();

  // check if we need 2 spin down the harddisk
  CheckNetworkHDSpinDown();
  if (!m_bNetworkSpinDown)
    CheckHDSpindown();

  // check if we need to activate the screensaver (if enabled)
  if (g_guiSettings.GetString("ScreenSaver.Mode") != "None")
    CheckScreenSaver();

  // check if we need to shutdown (if enabled)
  if (g_guiSettings.GetInt("System.ShutDownTime"))
    CheckShutdown();

  // process messages, even if a movie is playing
  g_applicationMessenger.ProcessMessages();

  // check for memory unit changes
  UpdateMemoryUnits();

  // check if we can free unused memory
  g_audioManager.FreeUnused();
}
void CApplication::Restart(bool bSamePosition)
{
  // this function gets called when the user changes a setting (like noninterleaved)
  // and which means we gotta close & reopen the current playing file

  // first check if we're playing a file
  if ( !IsPlayingVideo() && !IsPlayingAudio())
    return ;

  // do we want to return to the current position in the file
  if (false == bSamePosition)
  {
    // no, then just reopen the file and start at the beginning
    PlayFile(m_itemCurrentFile, true);
    return ;
  }

  // else get current position
  int iPercentage = m_pPlayer->GetPercentage();

  // reopen the file
  if ( PlayFile(m_itemCurrentFile, true) )
  {
    // and seek to the position
    m_pPlayer->SeekPercentage(iPercentage);
  }
}

const CStdString& CApplication::CurrentFile()
{
  return m_itemCurrentFile.m_strPath;
}

const CFileItem& CApplication::CurrentFileItem()
{
  return m_itemCurrentFile;
}

void CApplication::Mute(void)
{
  g_stSettings.m_iPreMuteVolumeLevel = g_stSettings.m_nVolumeLevel;
  g_stSettings.m_bMute = true;
  g_stSettings.m_nVolumeLevel = VOLUME_MINIMUM;

  m_guiDialogMuteBug.Show(m_gWindowManager.GetActiveWindow());
}

void CApplication::UnMute(void)
{
  g_stSettings.m_bMute = false;
  g_stSettings.m_nVolumeLevel = g_stSettings.m_iPreMuteVolumeLevel;

  CGUIMessage msg(GUI_MSG_MUTE_OFF, 0, 0, 0, 0, NULL);
  m_gWindowManager.SendMessage(msg);
}

void CApplication::SetVolume(int iPercent)
{
  // convert the percentage to a mB (milliBell) value (*100 for dB)
  if (iPercent < 0)
    iPercent = 0;
  if (iPercent > 100)
    iPercent = 100;
  float fHardwareVolume = ((float)iPercent) / 100.0f * (VOLUME_MAXIMUM - VOLUME_MINIMUM) + VOLUME_MINIMUM;
  // update our settings
  g_stSettings.m_nVolumeLevel = (long)fHardwareVolume;
  // and tell our player to update the volume
  if (m_pPlayer)
    m_pPlayer->SetVolume(g_stSettings.m_nVolumeLevel);
}

int CApplication::GetVolume() const
{
  // converts the hardware volume (in mB) to a percentage
  return int(((float)(g_stSettings.m_nVolumeLevel - VOLUME_MINIMUM)) / (VOLUME_MAXIMUM - VOLUME_MINIMUM)*100.0f + 0.5f);
}

void CApplication::SetPlaySpeed(int iSpeed)
{
  if (!IsPlayingAudio() && !IsPlayingVideo())
    return ;
  if (m_iPlaySpeed == iSpeed)
    return ;
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
  // if (m_pAudioDecoder)
  {
    if (m_iPlaySpeed == 1)
    { // restore volume
      m_pPlayer->SetVolume(g_stSettings.m_nVolumeLevel);
      //   m_pAudioDecoder->Mute (false);
    }
    else
    { // mute volume
      m_pPlayer->SetVolume(VOLUME_MINIMUM);
      //   m_pAudioDecoder->Mute (true);
    }
  }
}

int CApplication::GetPlaySpeed() const
{
  return m_iPlaySpeed;
}

//
// Returns the total time in seconds of the current media.  Fractional
// portions of a second are possible - but not necessarily supported by the
// player class.  This returns a double to be consistent with GetTime() and
// SeekTime().
//
double CApplication::GetTotalTime() const
{
  double rc = 0.0;

  if (IsPlaying() && m_pPlayer)
  {
    rc = m_pPlayer->GetTotalTime();
  }

  return rc;
}

//
// Returns the current time in seconds of the currently playing media.
// Fractional portions of a second are possible.  This returns a double to
// consistent with GetTotalTime() and SeekTime().
//
double CApplication::GetTime() const
{
  double rc = 0.0;

  if (IsPlaying() && m_pPlayer)
  {
    rc = static_cast<double>(m_pPlayer->GetTime() * 0.001f);
  }

  return rc;
}

//
// Sets the current position of the currently playing media to the specified
// time in seconds.  Fractional portions of a second are valid.  The passed
// time is the time offset from the beginning of the file as opposed to a
// delta from the current position.  This method accepts a double to be
// consistent with GetTime() and GetTotalTime().
//
void CApplication::SeekTime( double dTime )
{
  if (IsPlaying() && m_pPlayer && (dTime >= 0.0))
  {
    // convert to milliseconds
    m_pPlayer->SeekTime( static_cast<__int64>( dTime * 1000.0 ) );
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
{ // don't switch if there is a dialog on screen or the slideshow is active
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

const CStdString& CApplication::GetCurrentPlayer()
{
  return m_strCurrentPlayer;
}

// when a scan is initiated, save current settings
// and enable tag reading and remote thums
void CApplication::SaveMusicScanSettings()
{
  CLog::Log(LOGINFO,"Music scan has started ... enabling Tag Reading, and Remote Thumbs");
  g_stSettings.m_bMyMusicIsScanning = true;
  g_stSettings.m_bMyMusicOldUseTags = g_guiSettings.GetBool("MyMusic.UseTags");
  g_stSettings.m_bMyMusicOldFindThumbs = g_guiSettings.GetBool("MusicLibrary.FindRemoteThumbs");
  g_settings.Save();
  
  g_guiSettings.SetBool("MyMusic.UseTags", true);
  g_guiSettings.SetBool("MusicLibrary.FindRemoteThumbs", true);
}

void CApplication::RestoreMusicScanSettings()
{
  g_guiSettings.SetBool("MyMusic.UseTags", g_stSettings.m_bMyMusicOldUseTags);
  g_guiSettings.SetBool("MusicLibrary.FindRemoteThumbs", g_stSettings.m_bMyMusicOldFindThumbs);
  g_stSettings.m_bMyMusicIsScanning = false;
  g_settings.Save();
}

