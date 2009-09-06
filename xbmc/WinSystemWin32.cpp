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

#include "stdafx.h"
#include "WinSystemWin32.h"
#include "WinEventsWin32.h"
#include "Settings.h"
#include "resource.h"

#ifdef _WIN32

HWND g_hWnd = NULL;

CWinSystemWin32::CWinSystemWin32()
: CWinSystemBase()
{
  m_eWindowSystem = WINDOW_SYSTEM_WIN32;
  m_hWnd = NULL;
  m_hBlankWindow = NULL;
  m_hInstance = NULL;
  m_hIcon = NULL;
  m_hDC = NULL;
  m_nMonitorsCount = 0;
  m_nPrimary = 0;
  m_nSecondary = 0;
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
  return true;
}

bool CWinSystemWin32::CreateNewWindow(const CStdString& name, int width, int height, bool fullScreen, PHANDLE_EVENT_FUNC userFunction)
{
  m_hInstance = ( HINSTANCE )GetModuleHandle( NULL );

  m_nWidth = width;
  m_nHeight = height;
  m_bFullScreen = fullScreen;

  m_hIcon = LoadIcon(m_hInstance, MAKEINTRESOURCE(IDI_MAIN_ICON));

  // Register the windows class
  WNDCLASS wndClass;
  wndClass.style = CS_DBLCLKS;
  wndClass.lpfnWndProc = CWinEvents::WndProc;
  wndClass.cbClsExtra = 0;
  wndClass.cbWndExtra = 0;
  wndClass.style = CS_OWNDC;
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

  HWND hWnd = CreateWindow( name.c_str(), name.c_str(), 0,
    0, 0, m_nWidth, m_nHeight, 0,
    NULL, m_hInstance, userFunction );
  if( hWnd == NULL )
  {
    return false;
  }

  m_hWnd = hWnd;
  g_hWnd = hWnd;
  m_hDC = GetDC(m_hWnd);

  m_bWindowCreated = true;

  CreateBlankWindow();

  ResizeInternal();
  CenterWindow();

  // Show the window
  ShowWindow( m_hWnd, SW_SHOWDEFAULT );
  UpdateWindow( m_hWnd );

  return true;
}

bool CWinSystemWin32::CreateBlankWindow()
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

  m_hBlankWindow = CreateWindowEx(WS_EX_TOPMOST, "BlankWindowClass", "", WS_POPUP | WS_DISABLED, 
    CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, NULL, NULL);

  if(m_hBlankWindow == NULL)
    return false;

  return true;
}

bool CWinSystemWin32::BlankNonActiveMonitor(bool bBlank)
{
  if(m_hBlankWindow == NULL)
    return false;

  if(bBlank == false)
  {
    ShowWindow(m_hBlankWindow, SW_HIDE);
    return true;
  }

  int nMonitorToBlank;

  if(m_nScreen == 0)
    nMonitorToBlank = m_nSecondary;
  else
    nMonitorToBlank = m_nPrimary;

  RECT rBounds;
  MONITOR_DETAILS details;
  details = m_MonitorsInfo[
    nMonitorToBlank];
  CopyRect(&rBounds, &details.MonitorRC);

  // finally, move and resize the window
  SetWindowPos(m_hBlankWindow, NULL, rBounds.left, rBounds.top,
    rBounds.right - rBounds.left, rBounds.bottom - rBounds.top,
    SWP_NOACTIVATE);

  ShowWindow(m_hBlankWindow, SW_SHOW | SW_SHOWNOACTIVATE);

  if(m_hWnd)
    SetForegroundWindow(m_hWnd);

  return true;
}

bool CWinSystemWin32::CenterWindow()
{
  RESOLUTION_INFO DesktopRes = g_settings.m_ResInfo[RES_DESKTOP];

  m_nLeft = (DesktopRes.iWidth / 2) - (m_nWidth / 2);
  m_nTop = (DesktopRes.iHeight / 2) - (m_nHeight / 2);

  SetWindowPos(m_hWnd, 0, m_nLeft, m_nTop, 0, 0, SWP_NOSIZE);

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

bool CWinSystemWin32::SetFullScreen(bool fullScreen, int screen, int width, int height, bool blankOtherDisplays, bool alwaysOnTop)
{
  m_bFullScreen = fullScreen;
  m_nScreen = screen;
  m_nWidth = width;
  m_nHeight = height;
  m_bBlankOtherDisplay = blankOtherDisplays;

  ResizeInternal();

  BlankNonActiveMonitor(m_bBlankOtherDisplay);
 
  return true;
}

bool CWinSystemWin32::ResizeInternal()
{
  RECT rc;

  int monitorId;
  if(m_nScreen == 0)
    monitorId = m_nPrimary;
  else
    monitorId = m_nSecondary;

  CopyRect(&rc, &(m_MonitorsInfo[monitorId].MonitorRC));

  DWORD dwStyle = WS_CLIPCHILDREN;
  HWND windowAfter;

  if(m_bFullScreen)
  {
    dwStyle |= WS_POPUP;
    windowAfter = HWND_TOPMOST;
    // save position of window mode
    RECT oldRC;
    GetWindowRect(m_hWnd, &oldRC);
    m_nLeft = oldRC.left;
    m_nTop = oldRC.top;
  }
  else
  {
    dwStyle |= WS_OVERLAPPED | WS_BORDER | WS_CAPTION |
      WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_THICKFRAME;
    windowAfter = HWND_NOTOPMOST;

    if(m_nTop <= 0 || m_nLeft <= 0)
      CenterWindow();

    rc.left = m_nLeft;
    rc.right = m_nLeft + m_nWidth;
    rc.top = m_nTop;
    rc.bottom = m_nTop + m_nHeight;
    AdjustWindowRect( &rc, WS_OVERLAPPEDWINDOW, false );
  }

  SetWindowRgn(m_hWnd, 0, false);
  SetWindowLong(m_hWnd, GWL_STYLE, dwStyle);

  SetWindowPos(m_hWnd, windowAfter, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, SWP_SHOWWINDOW);
  return true;
}

void CWinSystemWin32::UpdateResolutions()
{

  CWinSystemBase::UpdateResolutions();

  UpdateResolutionsInternal();

  if(m_nMonitorsCount < 1)
    return;

  float refreshRate = 0;
  int w = 0;
  int h = 0;

  // Primary
  w = m_MonitorsInfo[m_nPrimary].ScreenWidth;
  h = m_MonitorsInfo[m_nPrimary].ScreenHeight;
  if( (m_MonitorsInfo[m_nPrimary].RefreshRate == 59) || (m_MonitorsInfo[m_nPrimary].RefreshRate == 29) || (m_MonitorsInfo[m_nPrimary].RefreshRate == 23) )
    refreshRate = (float)(m_MonitorsInfo[m_nPrimary].RefreshRate + 1) / 1.001f;
  else
    refreshRate = (float)m_MonitorsInfo[m_nPrimary].RefreshRate;
   
  UpdateDesktopResolution(g_settings.m_ResInfo[RES_DESKTOP], 0, w, h, refreshRate);
   

  // Secondary
  w = m_MonitorsInfo[m_nSecondary].ScreenWidth;
  h = m_MonitorsInfo[m_nSecondary].ScreenHeight;
  if( (m_MonitorsInfo[m_nSecondary].RefreshRate == 59) || (m_MonitorsInfo[m_nSecondary].RefreshRate == 29) || (m_MonitorsInfo[m_nSecondary].RefreshRate == 23) )
    refreshRate = (float)(m_MonitorsInfo[m_nSecondary].RefreshRate + 1) / 1.001f;
  else
    refreshRate = (float)m_MonitorsInfo[m_nSecondary].RefreshRate;

  RESOLUTION_INFO res;
  UpdateDesktopResolution(res, 1, w, h, refreshRate);
  g_graphicsContext.ResetOverscan(res);
  g_settings.m_ResInfo.push_back(res);
}


bool CWinSystemWin32::UpdateResolutionsInternal()
{
  memset(m_MonitorsInfo, sizeof(MONITOR_DETAILS) * MAX_MONITORS_NUM, 0);
  m_nMonitorsCount = 0;

  DISPLAY_DEVICE dd;
  dd.cb = sizeof(dd);
  DWORD dev = 0; // device index
  int id = 1; // monitor number, as used by Display Properties > Settings

  while (EnumDisplayDevices(0, dev, &dd, 0) && id - 1 < MAX_MONITORS_NUM)
  {
    if (!(dd.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER))
    {
      // ignore virtual mirror displays

      // get information about the monitor attached to this display adapter. dual head cards
      // and laptop video cards can have multiple monitors attached

      DISPLAY_DEVICE ddMon;
      ZeroMemory(&ddMon, sizeof(ddMon));
      ddMon.cb = sizeof(ddMon);
      DWORD devMon = 0;

      // please note that this enumeration may not return the correct monitor if multiple monitors
      // are attached. this is because not all display drivers return the ACTIVE flag for the monitor
      // that is actually active
      while (EnumDisplayDevices(dd.DeviceName, devMon, &ddMon, 0))
      {
        if (ddMon.StateFlags & DISPLAY_DEVICE_ACTIVE)
          break;

        devMon++;
      }

      if (!*ddMon.DeviceString)
      {
        EnumDisplayDevices(dd.DeviceName, 0, &ddMon, 0);
        if (!*ddMon.DeviceString)
          lstrcpy(ddMon.DeviceString, _T("Default Monitor"));
      }

      // get information about the display's position and the current display mode
      DEVMODE dm;
      ZeroMemory(&dm, sizeof(dm));
      dm.dmSize = sizeof(dm);
      if (EnumDisplaySettingsEx(dd.DeviceName, ENUM_CURRENT_SETTINGS, &dm, 0) == FALSE)
        EnumDisplaySettingsEx(dd.DeviceName, ENUM_REGISTRY_SETTINGS, &dm, 0);

      // get the monitor handle and workspace
      HMONITOR hm = 0;
      MONITORINFO mi;
      ZeroMemory(&mi, sizeof(mi));
      mi.cbSize = sizeof(mi);
      if (dd.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP)
      {
        // display is enabled. only enabled displays have a monitor handle
        POINT pt = { dm.dmPosition.x, dm.dmPosition.y };
        hm = MonitorFromPoint(pt, MONITOR_DEFAULTTONULL);
        if (hm)
          GetMonitorInfo(hm, &mi);
        else
        {
          dev++;
          continue;
        }
      }

      if (dd.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE)
      {
        m_MonitorsInfo[id - 1].type = MONITOR_TYPE_PRIMARY;
        m_nPrimary = id - 1;
      }
      else if (dd.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP)
      {
        m_MonitorsInfo[id - 1].type = MONITOR_TYPE_SECONDARY;
        m_nSecondary = id - 1;
      }
      else
      {
        dev++;
        continue;
      }

      strcpy(m_MonitorsInfo[id - 1].MonitorName, ddMon.DeviceString);
      strcpy(m_MonitorsInfo[id - 1].CardName, dd.DeviceString);
      strcpy(m_MonitorsInfo[id - 1].DeviceName, dd.DeviceName);


      // width x height @ x,y - bpp - refresh rate
      // note that refresh rate information is not available on Win9x
      m_MonitorsInfo[id - 1].ScreenWidth = dm.dmPelsWidth;
      m_MonitorsInfo[id - 1].ScreenHeight = dm.dmPelsHeight;
      CopyRect(&(m_MonitorsInfo[id - 1].MonitorRC), &mi.rcMonitor);

      m_MonitorsInfo[id - 1].hMonitor = hm;
      m_MonitorsInfo[id - 1].RefreshRate = dm.dmDisplayFrequency;
      m_MonitorsInfo[id - 1].Bpp = dm.dmBitsPerPel;

      id++;
    }

    dev++;
  }

  m_nMonitorsCount = id - 1;

  return 0;
}


#endif
