/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "WinSystemWin32.h"
#include "WinEventsWin32.h"
#include "settings/Settings.h"
#include "resource.h"
#include "settings/GUISettings.h"
#include "settings/AdvancedSettings.h"
#include "utils/log.h"

#ifdef _WIN32
#include <tpcshrd.h>

HWND g_hWnd = NULL;

CWinSystemWin32::CWinSystemWin32()
: CWinSystemBase()
{
  m_eWindowSystem = WINDOW_SYSTEM_WIN32;
  m_hWnd = NULL;
  m_hInstance = NULL;
  m_hIcon = NULL;
  m_hDC = NULL;
  m_nPrimary = 0;
  PtrCloseGestureInfoHandle = NULL;
  PtrSetGestureConfig = NULL;
  PtrGetGestureInfo = NULL;
  m_ValidWindowedPosition = false;
}

CWinSystemWin32::~CWinSystemWin32()
{
  if (m_hIcon)
  {
    DestroyIcon(m_hIcon);
    m_hIcon = NULL;
  }
};

bool CWinSystemWin32::InitWindowSystem()
{
  if(!CWinSystemBase::InitWindowSystem())
    return false;

  return true;
}

bool CWinSystemWin32::DestroyWindowSystem()
{
  RestoreDesktopResolution(m_nScreen);
  return true;
}

bool CWinSystemWin32::IsSystemScreenSaverEnabled()
{
  // Check if system screen saver is enabled
  // We are checking registry due to bug with SPI_GETSCREENSAVEACTIVE
  HKEY hKeyScreenSaver = NULL;
  long lReturn = NULL;
  long lScreenSaver = NULL;
  DWORD dwData = NULL;
  bool result = false;

  lReturn = RegOpenKeyEx(HKEY_CURRENT_USER, TEXT("Control Panel\\Desktop"),0,KEY_QUERY_VALUE,&hKeyScreenSaver);
  if(lReturn == ERROR_SUCCESS)
  {
    lScreenSaver = RegQueryValueEx(hKeyScreenSaver,TEXT("SCRNSAVE.EXE"),NULL,NULL,NULL,&dwData);

    // ScreenSaver is active
    if(lScreenSaver == ERROR_SUCCESS)
       result = true;
  }
  RegCloseKey(hKeyScreenSaver);

  return result;
}

void CWinSystemWin32::EnableSystemScreenSaver(bool bEnable)
{
  SystemParametersInfo(SPI_SETSCREENSAVEACTIVE,bEnable,0,0);
  if(!bEnable)
    SetThreadExecutionState(ES_DISPLAY_REQUIRED|ES_CONTINUOUS);
}

bool CWinSystemWin32::CreateNewWindow(const CStdString& name, bool fullScreen, RESOLUTION_INFO& res, PHANDLE_EVENT_FUNC userFunction)
{
  m_hInstance = ( HINSTANCE )GetModuleHandle( NULL );

  m_nWidth  = res.iWidth;
  m_nHeight = res.iHeight;
  m_bFullScreen = fullScreen;
  m_nScreen = res.iScreen;

  m_hIcon = LoadIcon(m_hInstance, MAKEINTRESOURCE(IDI_MAIN_ICON));

  // Register the windows class
  WNDCLASS wndClass;
  wndClass.style = CS_OWNDC; // For OpenGL
  wndClass.lpfnWndProc = CWinEvents::WndProc;
  wndClass.cbClsExtra = 0;
  wndClass.cbWndExtra = 0;
  wndClass.hInstance = m_hInstance;
  wndClass.hIcon = m_hIcon;
  wndClass.hCursor = LoadCursor( NULL, IDC_ARROW );
  wndClass.hbrBackground = ( HBRUSH )GetStockObject( BLACK_BRUSH );
  wndClass.lpszMenuName = NULL;
  wndClass.lpszClassName = name.c_str();

  if( !RegisterClass( &wndClass ) )
  {
    return false;
  }

  HWND hWnd = CreateWindow( name.c_str(), name.c_str(), fullScreen ? WS_POPUP : WS_OVERLAPPEDWINDOW,
    0, 0, m_nWidth, m_nHeight, 0,
    NULL, m_hInstance, userFunction );
  if( hWnd == NULL )
  {
    return false;
  }

  const DWORD dwHwndTabletProperty = 
      TABLET_DISABLE_PENBARRELFEEDBACK | // disables UI feedback on pen button down (circle)
      TABLET_DISABLE_FLICKS; // disables pen flicks (back, forward, drag down, drag up)
  
  SetProp(hWnd, MICROSOFT_TABLETPENSERVICE_PROPERTY, reinterpret_cast<HANDLE>(dwHwndTabletProperty));

  // setup our touch pointers
  PtrGetGestureInfo = (pGetGestureInfo) GetProcAddress( GetModuleHandle( TEXT( "user32" ) ), "GetGestureInfo" );
  PtrSetGestureConfig = (pSetGestureConfig) GetProcAddress( GetModuleHandle( TEXT( "user32" ) ), "SetGestureConfig" );
  PtrCloseGestureInfoHandle = (pCloseGestureInfoHandle) GetProcAddress( GetModuleHandle( TEXT( "user32" ) ), "CloseGestureInfoHandle" );

  m_hWnd = hWnd;
  g_hWnd = hWnd;
  m_hDC = GetDC(m_hWnd);

  m_bWindowCreated = true;

  CreateBlankWindows();

  ResizeInternal(true);

  // Show the window
  ShowWindow( m_hWnd, SW_SHOWDEFAULT );
  UpdateWindow( m_hWnd );

  return true;
}

bool CWinSystemWin32::CreateBlankWindows()
{
  WNDCLASSEX wcex;

  wcex.cbSize = sizeof(WNDCLASSEX);
  wcex.style= CS_HREDRAW | CS_VREDRAW;
  wcex.lpfnWndProc= DefWindowProc;
  wcex.cbClsExtra= 0;
  wcex.cbWndExtra= 0;
  wcex.hInstance= NULL;
  wcex.hIcon= 0;
  wcex.hCursor= NULL;
  wcex.hbrBackground= (HBRUSH)CreateSolidBrush(RGB(0, 0, 0));
  wcex.lpszMenuName= 0;
  wcex.lpszClassName= "BlankWindowClass";
  wcex.hIconSm= 0;

  // Now we can go ahead and register our new window class
  int reg = RegisterClassEx(&wcex);

  // We need as many blank windows as there are screens (minus 1)
  int BlankWindowsCount = m_MonitorsInfo.size() -1;
  
  m_hBlankWindows.reserve(BlankWindowsCount);

  for (int i=0; i < BlankWindowsCount; i++)
  {
    HWND hBlankWindow = CreateWindowEx(WS_EX_TOPMOST, "BlankWindowClass", "", WS_POPUP | WS_DISABLED,
    CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, NULL, NULL);

    if(hBlankWindow ==  NULL)
      return false;

    m_hBlankWindows.push_back(hBlankWindow);
  }

  return true;
}

bool CWinSystemWin32::BlankNonActiveMonitors(bool bBlank)
{
  if(m_hBlankWindows.size() == 0)
    return false;

  if(bBlank == false)
  {
    for (unsigned int i=0; i < m_hBlankWindows.size(); i++)
      ShowWindow(m_hBlankWindows[i], SW_HIDE);
    return true;
  }

  // Move a blank window in front of every screen, except the current XBMC screen.
  int screen = 0;
  if (screen == m_nScreen)
    screen++;

  for (unsigned int i=0; i < m_hBlankWindows.size(); i++)
  {
    RECT rBounds = ScreenRect(screen);
    // move and resize the window
    SetWindowPos(m_hBlankWindows[i], NULL, rBounds.left, rBounds.top,
      rBounds.right - rBounds.left, rBounds.bottom - rBounds.top,
      SWP_NOACTIVATE);

    ShowWindow(m_hBlankWindows[i], SW_SHOW | SW_SHOWNOACTIVATE);

    screen++;
    if (screen == m_nScreen)
      screen++;
  }

  if(m_hWnd)
    SetForegroundWindow(m_hWnd);

  return true;
}

bool CWinSystemWin32::CenterWindow()
{
  RESOLUTION_INFO DesktopRes = g_settings.m_ResInfo[RES_DESKTOP];

  m_nLeft = (DesktopRes.iWidth / 2) - (m_nWidth / 2);
  m_nTop = (DesktopRes.iHeight / 2) - (m_nHeight / 2);

  RECT rc;
  rc.left = m_nLeft;
  rc.top = m_nTop;
  rc.right = rc.left + m_nWidth;
  rc.bottom = rc.top + m_nHeight;
  AdjustWindowRect( &rc, WS_OVERLAPPEDWINDOW, false );

  SetWindowPos(m_hWnd, 0, rc.left, rc.top, 0, 0, SWP_NOSIZE);

  return true;
}

bool CWinSystemWin32::ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop)
{
  m_nWidth = newWidth;
  m_nHeight = newHeight;

  if(newLeft > 0)
    m_nLeft = newLeft;

  if(newTop > 0)
    m_nTop = newTop;

  ResizeInternal();

  return true;
}

void CWinSystemWin32::NotifyAppFocusChange(bool bGaining)
{
  if (m_bFullScreen && bGaining) //bump ourselves to top
    SetWindowPos(m_hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOREDRAW);
}

bool CWinSystemWin32::SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays)
{
  CLog::Log(LOGDEBUG, "%s (%s) on screen %d with size %dx%d, refresh %f%s", __FUNCTION__, !fullScreen ? "windowed" : (g_guiSettings.GetBool("videoscreen.fakefullscreen") ? "windowed fullscreen" : "true fullscreen"), res.iScreen, res.iWidth, res.iHeight, res.fRefreshRate, (res.dwFlags & D3DPRESENTFLAG_INTERLACED) ? "i" : "");

  bool forceResize = false;

  if (m_nScreen != res.iScreen)
  {
    forceResize = true;
    RestoreDesktopResolution(m_nScreen);
  }

  if(!m_bFullScreen && fullScreen)
  {
    // save position of windowed mode
    WINDOWINFO wi;
    GetWindowInfo(m_hWnd, &wi);
    m_nLeft = wi.rcClient.left;
    m_nTop = wi.rcClient.top;
    m_ValidWindowedPosition = true;
  }

  m_bFullScreen = fullScreen;
  m_nScreen = res.iScreen;
  m_nWidth  = res.iWidth;
  m_nHeight = res.iHeight;
  m_bBlankOtherDisplay = blankOtherDisplays;

  if (fullScreen && g_guiSettings.GetBool("videoscreen.fakefullscreen"))
    ChangeResolution(res);

  ResizeInternal(forceResize);

  BlankNonActiveMonitors(m_bBlankOtherDisplay);

  return true;
}

void CWinSystemWin32::RestoreDesktopResolution(int screen)
{
  int resIdx = RES_DESKTOP;
  for (int idx = RES_DESKTOP; idx < RES_DESKTOP + GetNumScreens(); idx++)
  {
    if (g_settings.m_ResInfo[idx].iScreen == screen)
    {
      resIdx = idx;
      break;
    }
  }
  ChangeResolution(g_settings.m_ResInfo[resIdx]);
}

const MONITOR_DETAILS &CWinSystemWin32::GetMonitor(int screen) const
{
  for (unsigned int monitor = 0; monitor < m_MonitorsInfo.size(); monitor++)
    if (m_MonitorsInfo[monitor].ScreenNumber == screen)
      return m_MonitorsInfo[monitor];

  // What to do if monitor is not found? Not sure... use the primary screen as a default value.
  return m_MonitorsInfo[m_nPrimary];
}

int CWinSystemWin32::GetCurrentScreen()
{
  HMONITOR hMonitor = MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTOPRIMARY);
  for (unsigned int monitor = 0; monitor < m_MonitorsInfo.size(); monitor++)
    if (m_MonitorsInfo[monitor].hMonitor == hMonitor)
      return m_MonitorsInfo[monitor].ScreenNumber;
  // primary as fallback - v. strange if this ever happens
  return 0;
}

RECT CWinSystemWin32::ScreenRect(int screen)
{
  const MONITOR_DETAILS &details = GetMonitor(screen);

  DEVMODE sDevMode;
  ZeroMemory(&sDevMode, sizeof(DEVMODE));
  sDevMode.dmSize = sizeof(DEVMODE);
  EnumDisplaySettings(details.DeviceName, ENUM_CURRENT_SETTINGS, &sDevMode);

  RECT rc;
  rc.left = sDevMode.dmPosition.x;
  rc.right = sDevMode.dmPosition.x + sDevMode.dmPelsWidth;
  rc.top = sDevMode.dmPosition.y;
  rc.bottom = sDevMode.dmPosition.y + sDevMode.dmPelsHeight;

  return rc;
}

bool CWinSystemWin32::ResizeInternal(bool forceRefresh)
{
  DWORD dwStyle = WS_CLIPCHILDREN;
  HWND windowAfter;
  RECT rc;

  if(m_bFullScreen)
  {
    dwStyle |= WS_POPUP;
    windowAfter = HWND_TOP;
    rc = ScreenRect(m_nScreen);
  }
  else
  {
    dwStyle |= WS_OVERLAPPEDWINDOW;
    windowAfter = g_advancedSettings.m_alwaysOnTop ? HWND_TOPMOST : HWND_NOTOPMOST;

    rc.left = m_nLeft;
    rc.right = m_nLeft + m_nWidth;
    rc.top = m_nTop;
    rc.bottom = m_nTop + m_nHeight;
    
    HMONITOR hMon = MonitorFromRect(&rc, MONITOR_DEFAULTTONULL);
    HMONITOR hMon2 = MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTOPRIMARY);

    // hasn't been windowed yet, or windowed position would not fullscreen to the same screen we were fullscreen on?
    // -> center on the screen that we were fullscreen on
    if(!m_ValidWindowedPosition || hMon == NULL || hMon != hMon2)
    {
      RECT newScreenRect = ScreenRect(GetCurrentScreen());
      rc.left = m_nLeft = newScreenRect.left + ((newScreenRect.right - newScreenRect.left) / 2) - (m_nWidth / 2);
      rc.top  = m_nTop  =  newScreenRect.top + ((newScreenRect.bottom - newScreenRect.top) / 2) - (m_nHeight / 2);
      rc.right = m_nLeft + m_nWidth;
      rc.bottom = m_nTop + m_nHeight;
    }

    AdjustWindowRect( &rc, WS_OVERLAPPEDWINDOW, false );
  }

  WINDOWINFO wi;
  GetWindowInfo(m_hWnd, &wi);
  RECT wr = wi.rcWindow;

  if (forceRefresh || wr.bottom  - wr.top != rc.bottom - rc.top || wr.right - wr.left != rc.right - rc.left ||
                     (wi.dwStyle & WS_CAPTION) != (dwStyle & WS_CAPTION))
  {
    CLog::Log(LOGDEBUG, "%s - resizing due to size change (%d,%d,%d,%d%s)->(%d,%d,%d,%d%s)",__FUNCTION__,wr.left, wr.top, wr.right, wr.bottom, (wi.dwStyle & WS_CAPTION) ? "" : " fullscreen",
                                                                                                         rc.left, rc.top, rc.right, rc.bottom, (dwStyle & WS_CAPTION) ? "" : " fullscreen");
    SetWindowRgn(m_hWnd, 0, false);
    SetWindowLong(m_hWnd, GWL_STYLE, dwStyle);

    // The SWP_DRAWFRAME is here because, perversely, without it win7 draws a
    // white frame plus titlebar around the xbmc splash
    SetWindowPos(m_hWnd, windowAfter, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, SWP_SHOWWINDOW|SWP_DRAWFRAME);

    // TODO: Probably only need this if switching screens
    ValidateRect(NULL, NULL);
  }
  return true;
}

bool CWinSystemWin32::ChangeResolution(RESOLUTION_INFO res)
{
  const MONITOR_DETAILS &details = GetMonitor(res.iScreen);

  DEVMODE sDevMode;
  ZeroMemory(&sDevMode, sizeof(DEVMODE));
  sDevMode.dmSize = sizeof(DEVMODE);

  // If we can't read the current resolution or any detail of the resolution is different than res
  if (!EnumDisplaySettings(details.DeviceName, ENUM_CURRENT_SETTINGS, &sDevMode) ||
      sDevMode.dmPelsWidth != res.iWidth || sDevMode.dmPelsHeight != res.iHeight ||
      sDevMode.dmDisplayFrequency != (int)res.fRefreshRate ||
      ((sDevMode.dmDisplayFlags & DM_INTERLACED) && !(res.dwFlags & D3DPRESENTFLAG_INTERLACED)) || 
      (!(sDevMode.dmDisplayFlags & DM_INTERLACED) && (res.dwFlags & D3DPRESENTFLAG_INTERLACED)) )
  {
    ZeroMemory(&sDevMode, sizeof(DEVMODE));
    sDevMode.dmSize = sizeof(DEVMODE);
    sDevMode.dmDriverExtra = 0;
    sDevMode.dmPelsWidth = res.iWidth;
    sDevMode.dmPelsHeight = res.iHeight;
    sDevMode.dmDisplayFrequency = (int)res.fRefreshRate;
    sDevMode.dmDisplayFlags = (res.dwFlags & D3DPRESENTFLAG_INTERLACED) ? DM_INTERLACED : 0;
    sDevMode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY | DM_DISPLAYFLAGS;

    // CDS_FULLSCREEN is for temporary fullscreen mode and prevents icons and windows from moving
    // to fit within the new dimensions of the desktop
    LONG rc = ChangeDisplaySettingsEx(details.DeviceName, &sDevMode, NULL, CDS_FULLSCREEN, NULL);
    if (rc != DISP_CHANGE_SUCCESSFUL)
    {
      CLog::Log(LOGERROR, "%s: error, code %d", __FUNCTION__, rc);
      return false;
    }
    else
    {
      return true;
    }
  }
  // nothing to do, return success
  return true;
}


void CWinSystemWin32::UpdateResolutions()
{

  CWinSystemBase::UpdateResolutions();

  UpdateResolutionsInternal();

  if(m_MonitorsInfo.size() < 1)
    return;

  float refreshRate = 0;
  int w = 0;
  int h = 0;
  uint32_t dwFlags;

  // Primary
  m_MonitorsInfo[m_nPrimary].ScreenNumber = 0;
  w = m_MonitorsInfo[m_nPrimary].ScreenWidth;
  h = m_MonitorsInfo[m_nPrimary].ScreenHeight;
  if( (m_MonitorsInfo[m_nPrimary].RefreshRate == 59) || (m_MonitorsInfo[m_nPrimary].RefreshRate == 29) || (m_MonitorsInfo[m_nPrimary].RefreshRate == 23) )
    refreshRate = (float)(m_MonitorsInfo[m_nPrimary].RefreshRate + 1) / 1.001f;
  else
    refreshRate = (float)m_MonitorsInfo[m_nPrimary].RefreshRate;
  dwFlags = m_MonitorsInfo[m_nPrimary].Interlaced ? D3DPRESENTFLAG_INTERLACED : 0;

  UpdateDesktopResolution(g_settings.m_ResInfo[RES_DESKTOP], 0, w, h, refreshRate, dwFlags);
  CLog::Log(LOGNOTICE, "Primary mode: %s", g_settings.m_ResInfo[RES_DESKTOP].strMode.c_str());

  // Desktop resolution of the other screens
  if(m_MonitorsInfo.size() >= 2)
  {
    int xbmcmonitor = 1;  // The screen number+1 showed in the GUI display settings

    for (unsigned int monitor = 0; monitor < m_MonitorsInfo.size(); monitor++)
    {
      if (monitor != m_nPrimary)
      {
        m_MonitorsInfo[monitor].ScreenNumber = xbmcmonitor;
        w = m_MonitorsInfo[monitor].ScreenWidth;
        h = m_MonitorsInfo[monitor].ScreenHeight;
        if( (m_MonitorsInfo[monitor].RefreshRate == 59) || (m_MonitorsInfo[monitor].RefreshRate == 29) || (m_MonitorsInfo[monitor].RefreshRate == 23) )
          refreshRate = (float)(m_MonitorsInfo[monitor].RefreshRate + 1) / 1.001f;
        else
          refreshRate = (float)m_MonitorsInfo[monitor].RefreshRate;
        dwFlags = m_MonitorsInfo[monitor].Interlaced ? D3DPRESENTFLAG_INTERLACED : 0;

        RESOLUTION_INFO res;
        UpdateDesktopResolution(res, xbmcmonitor++, w, h, refreshRate, dwFlags);
        g_settings.m_ResInfo.push_back(res);
        CLog::Log(LOGNOTICE, "Secondary mode: %s", res.strMode.c_str());
      }
    }
  }

  // The rest of the resolutions. The order is not important.
  for (unsigned int monitor = 0; monitor < m_MonitorsInfo.size(); monitor++)
  {
    for(int mode = 0;; mode++)
    {
      DEVMODE devmode;
      ZeroMemory(&devmode, sizeof(devmode));
      devmode.dmSize = sizeof(devmode);
      if(EnumDisplaySettings(m_MonitorsInfo[monitor].DeviceName, mode, &devmode) == 0)
        break;
      if(devmode.dmBitsPerPel != 32)
        continue;

      float refreshRate;
      if(devmode.dmDisplayFrequency == 59 || devmode.dmDisplayFrequency == 29 || devmode.dmDisplayFrequency == 23)
        refreshRate = (float)(devmode.dmDisplayFrequency + 1) / 1.001f;
      else
        refreshRate = (float)(devmode.dmDisplayFrequency);
      dwFlags = (devmode.dmDisplayFlags & DM_INTERLACED) ? D3DPRESENTFLAG_INTERLACED : 0;

      RESOLUTION_INFO res;
      UpdateDesktopResolution(res, m_MonitorsInfo[monitor].ScreenNumber, devmode.dmPelsWidth, devmode.dmPelsHeight, refreshRate, dwFlags);
      AddResolution(res);
      CLog::Log(LOGNOTICE, "Additional mode: %s", res.strMode.c_str());
    }
  }
}

void CWinSystemWin32::AddResolution(const RESOLUTION_INFO &res)
{
  for (unsigned int i = 0; i < g_settings.m_ResInfo.size(); i++)
    if (g_settings.m_ResInfo[i].iScreen      == res.iScreen &&
        g_settings.m_ResInfo[i].iWidth       == res.iWidth &&
        g_settings.m_ResInfo[i].iHeight      == res.iHeight &&
        g_settings.m_ResInfo[i].fRefreshRate == res.fRefreshRate &&
        g_settings.m_ResInfo[i].dwFlags      == res.dwFlags)
      return; // already have this resolution

  g_settings.m_ResInfo.push_back(res);
}

bool CWinSystemWin32::UpdateResolutionsInternal()
{
  
  DISPLAY_DEVICE ddAdapter;
  ZeroMemory(&ddAdapter, sizeof(ddAdapter));
  ddAdapter.cb = sizeof(ddAdapter);
  DWORD adapter = 0;

  while (EnumDisplayDevices(NULL, adapter, &ddAdapter, 0))
  {
    // Exclude displays that are not part of the windows desktop. Using them is too different: no windows,
    // direct access with GDI CreateDC() or DirectDraw for example. So it may be possible to play video, but GUI?
    if (!(ddAdapter.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER) && (ddAdapter.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP))
    {
      DISPLAY_DEVICE ddMon;
      ZeroMemory(&ddMon, sizeof(ddMon));
      ddMon.cb = sizeof(ddMon);
      bool foundScreen = false;
      DWORD screen = 0;

      // Just look for the first active output, we're actually only interested in the information at the adapter level.
      while (EnumDisplayDevices(ddAdapter.DeviceName, screen, &ddMon, 0))
      {
        if (ddMon.StateFlags & (DISPLAY_DEVICE_ACTIVE | DISPLAY_DEVICE_ATTACHED))
        {
          foundScreen = true;
          break;
        }
        ZeroMemory(&ddMon, sizeof(ddMon));
        ddMon.cb = sizeof(ddMon);
        screen++;
      }
      // Remoting returns no screens. Handle with a dummy screen.
      if (!foundScreen && screen == 0)
      {
        lstrcpy(ddMon.DeviceString, _T("Dummy Monitor")); // safe: large static array
        foundScreen = true;
      }

      if (foundScreen)
      {
        CLog::Log(LOGNOTICE, "Found screen: %s on %s, adapter %d.", ddMon.DeviceString, ddAdapter.DeviceString, adapter);

        // get information about the display's current position and display mode
        // TODO: for Windows 7/Server 2008 and up, Microsoft recommends QueryDisplayConfig() instead, the API used by the control panel.
        DEVMODE dm;
        ZeroMemory(&dm, sizeof(dm));
        dm.dmSize = sizeof(dm);
        if (EnumDisplaySettingsEx(ddAdapter.DeviceName, ENUM_CURRENT_SETTINGS, &dm, 0) == FALSE)
          EnumDisplaySettingsEx(ddAdapter.DeviceName, ENUM_REGISTRY_SETTINGS, &dm, 0);

        // get the monitor handle and workspace
        HMONITOR hm = 0;
        POINT pt = { dm.dmPosition.x, dm.dmPosition.y };
        hm = MonitorFromPoint(pt, MONITOR_DEFAULTTONULL);

        MONITOR_DETAILS md;
        memset(&md, 0, sizeof(MONITOR_DETAILS));

        strcpy(md.MonitorName, ddMon.DeviceString);
        strcpy(md.CardName, ddAdapter.DeviceString);
        strcpy(md.DeviceName, ddAdapter.DeviceName);

        // width x height @ x,y - bpp - refresh rate
        // note that refresh rate information is not available on Win9x
        md.ScreenWidth = dm.dmPelsWidth;
        md.ScreenHeight = dm.dmPelsHeight;
        md.hMonitor = hm;
        md.RefreshRate = dm.dmDisplayFrequency;
        md.Bpp = dm.dmBitsPerPel;
        md.Interlaced = (dm.dmDisplayFlags & DM_INTERLACED) ? true : false;

        m_MonitorsInfo.push_back(md);

        // Careful, some adapters don't end up in the vector (mirroring, no active output, etc.)
        if (ddAdapter.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE)
          m_nPrimary = m_MonitorsInfo.size() -1;

      }
    }
    ZeroMemory(&ddAdapter, sizeof(ddAdapter));
    ddAdapter.cb = sizeof(ddAdapter);
    adapter++;
  }
  return 0;
}

void CWinSystemWin32::ShowOSMouse(bool show)
{
  static int counter = 0;
  if ((counter < 0 && show) || (counter >= 0 && !show))
    counter = ShowCursor(show);
}

bool CWinSystemWin32::Minimize()
{
  ShowWindow(m_hWnd, SW_MINIMIZE);
  return true;
}
bool CWinSystemWin32::Restore()
{
  ShowWindow(m_hWnd, SW_RESTORE);
  return true;
}
bool CWinSystemWin32::Hide()
{
  ShowWindow(m_hWnd, SW_HIDE);
  return true;
}
bool CWinSystemWin32::Show(bool raise)
{
  HWND windowAfter = HWND_BOTTOM;
  if (raise)
  {
    if (m_bFullScreen)
      windowAfter = HWND_TOP;
    else
      windowAfter = g_advancedSettings.m_alwaysOnTop ? HWND_TOPMOST : HWND_NOTOPMOST;
  }

  SetWindowPos(m_hWnd, windowAfter, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_SHOWWINDOW);
  UpdateWindow(m_hWnd);
  if (raise)
  {
    SetForegroundWindow(g_hWnd);
    SetFocus(g_hWnd);
  }
  return true;
}

#endif
