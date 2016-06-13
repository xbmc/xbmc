/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "WinSystemWin32.h"
#include "WinEventsWin32.h"
#include "resource.h"
#include "guilib/gui3d.h"
#include "messaging/ApplicationMessenger.h"
#include "settings/AdvancedSettings.h"
#include "settings/DisplaySettings.h"
#include "settings/Settings.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "utils/CharsetConverter.h"
#include "utils/SystemInfo.h"

#ifdef TARGET_WINDOWS
#include <tpcshrd.h>

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
  m_IsAlteringWindow = false;
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

  if(m_MonitorsInfo.empty())
  {
    CLog::Log(LOGERROR, "%s - no suitable monitor found, aborting...", __FUNCTION__);
    return false;
  }

  return true;
}

bool CWinSystemWin32::DestroyWindowSystem()
{
  RestoreDesktopResolution(m_nScreen);
  return true;
}

bool CWinSystemWin32::CreateNewWindow(const std::string& name, bool fullScreen, RESOLUTION_INFO& res, PHANDLE_EVENT_FUNC userFunction)
{
  m_hInstance = ( HINSTANCE )GetModuleHandle( NULL );

  if(m_hInstance == NULL)
    CLog::Log(LOGDEBUG, "%s : GetModuleHandle failed with %d", __FUNCTION__, GetLastError());

  m_nWidth  = res.iWidth;
  m_nHeight = res.iHeight;
  m_bFullScreen = fullScreen;
  m_nScreen = res.iScreen;

  m_hIcon = LoadIcon(m_hInstance, MAKEINTRESOURCE(IDI_MAIN_ICON));

  // Register the windows class
  WNDCLASS wndClass;
  wndClass.style = CS_OWNDC; // For OpenGL
  wndClass.lpfnWndProc = CWinEventsWin32::WndProc;
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
    CLog::Log(LOGERROR, "%s : RegisterClass failed with %d", __FUNCTION__, GetLastError());
    return false;
  }

  HWND hWnd = CreateWindow( name.c_str(), name.c_str(), fullScreen ? WS_POPUP : WS_OVERLAPPEDWINDOW,
    0, 0, m_nWidth, m_nHeight, 0,
    NULL, m_hInstance, userFunction );
  if( hWnd == NULL )
  {
    CLog::Log(LOGERROR, "%s : CreateWindow failed with %d", __FUNCTION__, GetLastError());
    return false;
  }

  const DWORD dwHwndTabletProperty =
      TABLET_DISABLE_PENBARRELFEEDBACK | // disables UI feedback on pen button down (circle)
      TABLET_DISABLE_FLICKS; // disables pen flicks (back, forward, drag down, drag up)

  SetProp(hWnd, MICROSOFT_TABLETPENSERVICE_PROPERTY, reinterpret_cast<HANDLE>(dwHwndTabletProperty));

  // setup our touch pointers
  HMODULE hUser32 = GetModuleHandleA( "user32" );
  if (hUser32)
  {
    PtrGetGestureInfo = (pGetGestureInfo) GetProcAddress( hUser32, "GetGestureInfo" );
    PtrSetGestureConfig = (pSetGestureConfig) GetProcAddress( hUser32, "SetGestureConfig" );
    PtrCloseGestureInfoHandle = (pCloseGestureInfoHandle) GetProcAddress( hUser32, "CloseGestureInfoHandle" );
  }

  m_hWnd = hWnd;
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
  if(!RegisterClassEx(&wcex))
  {
    CLog::Log(LOGERROR, "%s : RegisterClass failed with %d", __FUNCTION__, GetLastError());
    return false;
  }

  // We need as many blank windows as there are screens (minus 1)
  int BlankWindowsCount = m_MonitorsInfo.size() -1;

  for (int i=0; i < BlankWindowsCount; i++)
  {
    HWND hBlankWindow = CreateWindowEx(WS_EX_TOPMOST, "BlankWindowClass", "", WS_POPUP | WS_DISABLED,
    CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, NULL, NULL);

    if(hBlankWindow ==  NULL)
    {
      CLog::Log(LOGERROR, "%s : CreateWindowEx failed with %d", __FUNCTION__, GetLastError());
      return false;
    }

    m_hBlankWindows.push_back(hBlankWindow);
  }

  return true;
}

bool CWinSystemWin32::BlankNonActiveMonitors(bool bBlank)
{
  if(m_hBlankWindows.empty())
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
  RESOLUTION_INFO DesktopRes = CDisplaySettings::GetInstance().GetResolutionInfo(RES_DESKTOP);

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
  if (m_bFullScreen && bGaining)
  {
    SetWindowPos(m_hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOREDRAW);
  }
}

bool CWinSystemWin32::SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays)
{
  return SetFullScreenEx(fullScreen, res, blankOtherDisplays, false);
}

bool CWinSystemWin32::SetFullScreenEx(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays, bool forceResChange)
{
  m_IsAlteringWindow = true;

  CLog::Log(LOGDEBUG, "%s (%s) on screen %d with size %dx%d, refresh %f%s", __FUNCTION__, !fullScreen ? "windowed" : (CSettings::GetInstance().GetBool(CSettings::SETTING_VIDEOSCREEN_FAKEFULLSCREEN) ? "windowed fullscreen" : "true fullscreen"), res.iScreen, res.iWidth, res.iHeight, res.fRefreshRate, (res.dwFlags & D3DPRESENTFLAG_INTERLACED) ? "i" : "");

  bool forceResize = false;

  if (m_nScreen != res.iScreen)
  {
    forceResize = true;
    RestoreDesktopResolution(m_nScreen);
  }

  if(m_hWnd && !m_bFullScreen && fullScreen)
  {
    // save position of windowed mode
    WINDOWINFO wi;
    wi.cbSize = sizeof(WINDOWINFO);
    if(GetWindowInfo(m_hWnd, &wi))
    {
      m_nLeft = wi.rcClient.left;
      m_nTop = wi.rcClient.top;
      m_ValidWindowedPosition = true;
    }
    else
      CLog::Log(LOGERROR, "%s : GetWindowInfo failed with %d", __FUNCTION__, GetLastError());
  }

  m_bFullScreen = fullScreen;
  m_nScreen = res.iScreen;
  m_nWidth  = res.iWidth;
  m_nHeight = res.iHeight;
  m_bBlankOtherDisplay = blankOtherDisplays;

  if (fullScreen && CSettings::GetInstance().GetBool(CSettings::SETTING_VIDEOSCREEN_FAKEFULLSCREEN))
    ChangeResolution(res, forceResChange);

  ResizeInternal(forceResize);

  BlankNonActiveMonitors(m_bBlankOtherDisplay);

  m_IsAlteringWindow = false;

  return true;
}

void CWinSystemWin32::RestoreDesktopResolution(int screen)
{
  int resIdx = RES_DESKTOP;
  for (int idx = RES_DESKTOP; idx < RES_DESKTOP + GetNumScreens(); idx++)
  {
    if (CDisplaySettings::GetInstance().GetResolutionInfo(idx).iScreen == screen)
    {
      resIdx = idx;
      break;
    }
  }
  ChangeResolution(CDisplaySettings::GetInstance().GetResolutionInfo(resIdx));
}

const MONITOR_DETAILS* CWinSystemWin32::GetMonitor(int screen) const
{
  for (unsigned int monitor = 0; monitor < m_MonitorsInfo.size(); monitor++)
    if (m_MonitorsInfo[monitor].ScreenNumber == screen)
      return &m_MonitorsInfo[monitor];

  // What to do if monitor is not found? Not sure... use the primary screen as a default value.
  if (m_nPrimary >= 0 && static_cast<size_t>(m_nPrimary) < m_MonitorsInfo.size())
  {
    CLog::LogFunction(LOGDEBUG, __FUNCTION__, "no monitor found for screen %i, "
                      "will use primary screen %i", screen, m_nPrimary);
    return &m_MonitorsInfo[m_nPrimary];
  }
  else
  {
    CLog::LogFunction(LOGERROR, __FUNCTION__, "no monitor found for screen %i", screen);
    return nullptr;
  }
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
  const MONITOR_DETAILS* details = GetMonitor(screen);

  if (!details)
  {
    CLog::LogFunction(LOGERROR, __FUNCTION__, "no monitor found for screen %i", screen);
  }

  DEVMODEW sDevMode;
  ZeroMemory(&sDevMode, sizeof(sDevMode));
  sDevMode.dmSize = sizeof(sDevMode);
  if(!EnumDisplaySettingsW(details->DeviceNameW.c_str(), ENUM_CURRENT_SETTINGS, &sDevMode))
    CLog::Log(LOGERROR, "%s : EnumDisplaySettings failed with %d", __FUNCTION__, GetLastError());

  RECT rc;
  rc.left = sDevMode.dmPosition.x;
  rc.right = sDevMode.dmPosition.x + sDevMode.dmPelsWidth;
  rc.top = sDevMode.dmPosition.y;
  rc.bottom = sDevMode.dmPosition.y + sDevMode.dmPelsHeight;

  return rc;
}

bool CWinSystemWin32::ResizeInternal(bool forceRefresh)
{
  if (m_hWnd == NULL)
    return false;
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
  wi.cbSize = sizeof (WINDOWINFO);
  if(!GetWindowInfo(m_hWnd, &wi))
  {
    CLog::Log(LOGERROR, "%s : GetWindowInfo failed with %d", __FUNCTION__, GetLastError());
    return false;
  }
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

    //! @todo Probably only need this if switching screens
    ValidateRect(NULL, NULL);
  }
  return true;
}

bool CWinSystemWin32::ChangeResolution(const RESOLUTION_INFO& res, bool forceChange /*= false*/)
{
  const MONITOR_DETAILS* details = GetMonitor(res.iScreen);

  if (!details)
    return false;

  DEVMODEW sDevMode;
  ZeroMemory(&sDevMode, sizeof(sDevMode));
  sDevMode.dmSize = sizeof(sDevMode);

  // If we can't read the current resolution or any detail of the resolution is different than res
  if (!EnumDisplaySettingsW(details->DeviceNameW.c_str(), ENUM_CURRENT_SETTINGS, &sDevMode) ||
      sDevMode.dmPelsWidth != res.iWidth || sDevMode.dmPelsHeight != res.iHeight ||
      sDevMode.dmDisplayFrequency != (int)res.fRefreshRate ||
      ((sDevMode.dmDisplayFlags & DM_INTERLACED) && !(res.dwFlags & D3DPRESENTFLAG_INTERLACED)) ||
      (!(sDevMode.dmDisplayFlags & DM_INTERLACED) && (res.dwFlags & D3DPRESENTFLAG_INTERLACED)) 
      || forceChange)
  {
    ZeroMemory(&sDevMode, sizeof(sDevMode));
    sDevMode.dmSize = sizeof(sDevMode);
    sDevMode.dmDriverExtra = 0;
    sDevMode.dmPelsWidth = res.iWidth;
    sDevMode.dmPelsHeight = res.iHeight;
    sDevMode.dmDisplayFrequency = (int)res.fRefreshRate;
    sDevMode.dmDisplayFlags = (res.dwFlags & D3DPRESENTFLAG_INTERLACED) ? DM_INTERLACED : 0;
    sDevMode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY | DM_DISPLAYFLAGS;

    LONG rc = DISP_CHANGE_SUCCESSFUL;
    bool bResChanged = false;

    // Windows 8 refresh rate workaround for 24.0, 48.0 and 60.0 Hz
    if (CSysInfo::IsWindowsVersionAtLeast(CSysInfo::WindowsVersionWin8) && (res.fRefreshRate == 24.0 || res.fRefreshRate == 48.0 || res.fRefreshRate == 60.0))
    {
      CLog::Log(LOGDEBUG, "%s : Using Windows 8+ workaround for refresh rate %d Hz", __FUNCTION__, (int)res.fRefreshRate);

      // Get current resolution stored in registry
      DEVMODEW sDevModeRegistry;
      ZeroMemory(&sDevModeRegistry, sizeof(sDevModeRegistry));
      sDevModeRegistry.dmSize = sizeof(sDevModeRegistry);
      if (EnumDisplaySettingsW(details->DeviceNameW.c_str(), ENUM_REGISTRY_SETTINGS, &sDevModeRegistry))
      {
        // Set requested mode in registry without actually changing resolution
        rc = ChangeDisplaySettingsExW(details->DeviceNameW.c_str(), &sDevMode, NULL, CDS_UPDATEREGISTRY | CDS_NORESET, NULL);
        if (rc == DISP_CHANGE_SUCCESSFUL)
        {
          // Change resolution based on registry setting
          rc = ChangeDisplaySettingsExW(details->DeviceNameW.c_str(), NULL, NULL, CDS_FULLSCREEN, NULL);
          if (rc == DISP_CHANGE_SUCCESSFUL)
            bResChanged = true;
          else
            CLog::Log(LOGERROR, "%s : ChangeDisplaySettingsEx (W8+ change resolution) failed with %d, using fallback", __FUNCTION__, rc);

          // Restore registry with original values
          sDevModeRegistry.dmSize = sizeof(sDevModeRegistry);
          sDevModeRegistry.dmDriverExtra = 0;
          sDevModeRegistry.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY | DM_DISPLAYFLAGS;
          rc = ChangeDisplaySettingsExW(details->DeviceNameW.c_str(), &sDevModeRegistry, NULL, CDS_UPDATEREGISTRY | CDS_NORESET, NULL);
          if (rc != DISP_CHANGE_SUCCESSFUL)
            CLog::Log(LOGERROR, "%s : ChangeDisplaySettingsEx (W8+ restore registry) failed with %d", __FUNCTION__, rc);
        }
        else
          CLog::Log(LOGERROR, "%s : ChangeDisplaySettingsEx (W8+ set registry) failed with %d, using fallback", __FUNCTION__, rc);
      }
      else
        CLog::Log(LOGERROR, "%s : Unable to retrieve registry settings for Windows 8+ workaround, using fallback", __FUNCTION__);
    }

    // Standard resolution change/fallback for Windows 8+ workaround
    if (!bResChanged)
    {
      // CDS_FULLSCREEN is for temporary fullscreen mode and prevents icons and windows from moving
      // to fit within the new dimensions of the desktop
      rc = ChangeDisplaySettingsExW(details->DeviceNameW.c_str(), &sDevMode, NULL, CDS_FULLSCREEN, NULL);
      if (rc == DISP_CHANGE_SUCCESSFUL)
        bResChanged = true;
      else
        CLog::Log(LOGERROR, "%s : ChangeDisplaySettingsEx failed with %d", __FUNCTION__, rc);
    }
    
    if (bResChanged) 
      ResolutionChanged();

    return bResChanged;
  }

  // nothing to do, return success
  return true;
}


void CWinSystemWin32::UpdateResolutions()
{

  CWinSystemBase::UpdateResolutions();

  UpdateResolutionsInternal();

  if(m_MonitorsInfo.empty())
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

  UpdateDesktopResolution(CDisplaySettings::GetInstance().GetResolutionInfo(RES_DESKTOP), 0, w, h, refreshRate, dwFlags);
  CLog::Log(LOGNOTICE, "Primary mode: %s", CDisplaySettings::GetInstance().GetResolutionInfo(RES_DESKTOP).strMode.c_str());

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
        CDisplaySettings::GetInstance().AddResolutionInfo(res);
        CLog::Log(LOGNOTICE, "Secondary mode: %s", res.strMode.c_str());
      }
    }
  }

  // The rest of the resolutions. The order is not important.
  for (unsigned int monitor = 0; monitor < m_MonitorsInfo.size(); monitor++)
  {
    for(int mode = 0;; mode++)
    {
      DEVMODEW devmode;
      ZeroMemory(&devmode, sizeof(devmode));
      devmode.dmSize = sizeof(devmode);
      if(EnumDisplaySettingsW(m_MonitorsInfo[monitor].DeviceNameW.c_str(), mode, &devmode) == 0)
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
  for (unsigned int i = 0; i < CDisplaySettings::GetInstance().ResolutionInfoSize(); i++)
  {
    if (CDisplaySettings::GetInstance().GetResolutionInfo(i).iScreen      == res.iScreen &&
        CDisplaySettings::GetInstance().GetResolutionInfo(i).iWidth       == res.iWidth &&
        CDisplaySettings::GetInstance().GetResolutionInfo(i).iHeight      == res.iHeight &&
        CDisplaySettings::GetInstance().GetResolutionInfo(i).iScreenWidth == res.iScreenWidth &&
        CDisplaySettings::GetInstance().GetResolutionInfo(i).iScreenHeight== res.iScreenHeight &&
        CDisplaySettings::GetInstance().GetResolutionInfo(i).fRefreshRate == res.fRefreshRate &&
        CDisplaySettings::GetInstance().GetResolutionInfo(i).dwFlags      == res.dwFlags)
      return; // already have this resolution
  }

  CDisplaySettings::GetInstance().AddResolutionInfo(res);
}

bool CWinSystemWin32::UpdateResolutionsInternal()
{

  DISPLAY_DEVICEW ddAdapter;
  ZeroMemory(&ddAdapter, sizeof(ddAdapter));
  ddAdapter.cb = sizeof(ddAdapter);
  DWORD adapter = 0;

  while (EnumDisplayDevicesW(NULL, adapter, &ddAdapter, 0))
  {
    // Exclude displays that are not part of the windows desktop. Using them is too different: no windows,
    // direct access with GDI CreateDC() or DirectDraw for example. So it may be possible to play video, but GUI?
    if (!(ddAdapter.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER) && (ddAdapter.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP))
    {
      DISPLAY_DEVICEW ddMon;
      ZeroMemory(&ddMon, sizeof(ddMon));
      ddMon.cb = sizeof(ddMon);
      bool foundScreen = false;
      DWORD screen = 0;

      // Just look for the first active output, we're actually only interested in the information at the adapter level.
      while (EnumDisplayDevicesW(ddAdapter.DeviceName, screen, &ddMon, 0))
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
        lstrcpyW(ddMon.DeviceString, L"Dummy Monitor"); // safe: large static array
        foundScreen = true;
      }

      if (foundScreen)
      {
        std::string monitorStr, adapterStr;
        g_charsetConverter.wToUTF8(ddMon.DeviceString, monitorStr);
        g_charsetConverter.wToUTF8(ddAdapter.DeviceString, adapterStr);
        CLog::Log(LOGNOTICE, "Found screen: %s on %s, adapter %d.", monitorStr.c_str(), adapterStr.c_str(), adapter);

        // get information about the display's current position and display mode
        //! @todo for Windows 7/Server 2008 and up, Microsoft recommends QueryDisplayConfig() instead, the API used by the control panel.
        DEVMODEW dm;
        ZeroMemory(&dm, sizeof(dm));
        dm.dmSize = sizeof(dm);
        if (EnumDisplaySettingsExW(ddAdapter.DeviceName, ENUM_CURRENT_SETTINGS, &dm, 0) == FALSE)
          EnumDisplaySettingsExW(ddAdapter.DeviceName, ENUM_REGISTRY_SETTINGS, &dm, 0);

        // get the monitor handle and workspace
        HMONITOR hm = 0;
        POINT pt = { dm.dmPosition.x, dm.dmPosition.y };
        hm = MonitorFromPoint(pt, MONITOR_DEFAULTTONULL);

        MONITOR_DETAILS md = {};

        md.MonitorNameW = ddMon.DeviceString;
        md.CardNameW = ddAdapter.DeviceString;
        md.DeviceNameW = ddAdapter.DeviceName;

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
    SetForegroundWindow(m_hWnd);
    SetFocus(m_hWnd);
  }
  return true;
}

void CWinSystemWin32::Register(IDispResource *resource)
{
  CSingleLock lock(m_resourceSection);
  m_resources.push_back(resource);
}

void CWinSystemWin32::Unregister(IDispResource* resource)
{
  CSingleLock lock(m_resourceSection);
  std::vector<IDispResource*>::iterator i = find(m_resources.begin(), m_resources.end(), resource);
  if (i != m_resources.end())
    m_resources.erase(i);
}

void CWinSystemWin32::OnDisplayLost()
{
  CLog::Log(LOGDEBUG, "%s - notify display lost event", __FUNCTION__);

  // make sure renderer has no invalid references
  KODI::MESSAGING::CApplicationMessenger::GetInstance().SendMsg(TMSG_RENDERER_FLUSH);

  {
    CSingleLock lock(m_resourceSection);
    for (std::vector<IDispResource *>::iterator i = m_resources.begin(); i != m_resources.end(); ++i)
      (*i)->OnLostDisplay();
  }
}

void CWinSystemWin32::OnDisplayReset()
{
  if (!m_delayDispReset)
  {
    CLog::Log(LOGDEBUG, "%s - notify display reset event", __FUNCTION__);
    CSingleLock lock(m_resourceSection);
    for (std::vector<IDispResource *>::iterator i = m_resources.begin(); i != m_resources.end(); ++i)
      (*i)->OnResetDisplay();
  }
}

void CWinSystemWin32::OnDisplayBack()
{
  int delay = CSettings::GetInstance().GetInt("videoscreen.delayrefreshchange");
  if (delay > 0)
  {
    m_delayDispReset = true;
    m_dispResetTimer.Set(delay * 100);
  }
  OnDisplayReset();
}

void CWinSystemWin32::ResolutionChanged()
{
  OnDisplayLost();
  OnDisplayBack();
}

void CWinSystemWin32::SetForegroundWindowInternal(HWND hWnd)
{
  if (!IsWindow(hWnd)) return;

  // if the window isn't focused, bring it to front or SetFullScreen will fail
  BYTE keyState[256] = { 0 };
  // to unlock SetForegroundWindow we need to imitate Alt pressing
  if (GetKeyboardState((LPBYTE)&keyState) && !(keyState[VK_MENU] & 0x80))
    keybd_event(VK_MENU, 0, KEYEVENTF_EXTENDEDKEY | 0, 0);

  BOOL res = SetForegroundWindow(hWnd);

  if (GetKeyboardState((LPBYTE)&keyState) && !(keyState[VK_MENU] & 0x80))
    keybd_event(VK_MENU, 0, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);

  if (!res)
  {
    //relation time of SetForegroundWindow lock
    DWORD lockTimeOut = 0;
    HWND  hCurrWnd = GetForegroundWindow();
    DWORD dwThisTID = GetCurrentThreadId(),
          dwCurrTID = GetWindowThreadProcessId(hCurrWnd, 0);

    // we need to bypass some limitations from Microsoft
    if (dwThisTID != dwCurrTID)
    {
      AttachThreadInput(dwThisTID, dwCurrTID, TRUE);
      SystemParametersInfo(SPI_GETFOREGROUNDLOCKTIMEOUT, 0, &lockTimeOut, 0);
      SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, 0, SPIF_SENDWININICHANGE | SPIF_UPDATEINIFILE);
      AllowSetForegroundWindow(ASFW_ANY);
    }

    SetForegroundWindow(hWnd);

    if (dwThisTID != dwCurrTID)
    {
      SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, (PVOID)lockTimeOut, SPIF_SENDWININICHANGE | SPIF_UPDATEINIFILE);
      AttachThreadInput(dwThisTID, dwCurrTID, FALSE);
    }
  }
}

#endif
