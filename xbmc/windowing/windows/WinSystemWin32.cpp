/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WinSystemWin32.h"

#include "ServiceBroker.h"
#include "VideoSyncD3D.h"
#include "WIN32Util.h"
#include "WinEventsWin32.h"
#include "application/Application.h"
#include "cores/AudioEngine/AESinkFactory.h"
#include "cores/AudioEngine/Sinks/AESinkDirectSound.h"
#include "cores/AudioEngine/Sinks/AESinkWASAPI.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "messaging/ApplicationMessenger.h"
#include "platform/Environment.h"
#include "rendering/dx/ScreenshotSurfaceWindows.h"
#include "resource.h"
#include "settings/AdvancedSettings.h"
#include "settings/DisplaySettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"
#include "utils/SystemInfo.h"
#include "utils/log.h"
#include "windowing/GraphicContext.h"
#include "windowing/windows/Win32DPMSSupport.h"

#include "platform/win32/CharsetConverter.h"
#include "platform/win32/input/IRServerSuite.h"

#include <algorithm>
#include <cmath>
#include <mutex>

#include <tpcshrd.h>

using namespace std::chrono_literals;

const char* CWinSystemWin32::SETTING_WINDOW_TOP = "window.top";
const char* CWinSystemWin32::SETTING_WINDOW_LEFT = "window.left";

CWinSystemWin32::CWinSystemWin32()
  : CWinSystemBase()
  , m_hWnd(nullptr)
  , m_hMonitor(nullptr)
  , m_hInstance(nullptr)
  , m_hIcon(nullptr)
  , m_ValidWindowedPosition(false)
  , m_IsAlteringWindow(false)
  , m_delayDispReset(false)
  , m_state(WINDOW_STATE_WINDOWED)
  , m_fullscreenState(WINDOW_FULLSCREEN_STATE_FULLSCREEN_WINDOW)
  , m_windowState(WINDOW_WINDOW_STATE_WINDOWED)
  , m_windowStyle(WINDOWED_STYLE)
  , m_windowExStyle(WINDOWED_EX_STYLE)
  , m_inFocus(false)
  , m_bMinimized(false)
{
  std::string cacert = CEnvironment::getenv("SSL_CERT_FILE");
  if (cacert.empty() || !XFILE::CFile::Exists(cacert))
  {
    cacert = CSpecialProtocol::TranslatePath("special://xbmc/system/certs/cacert.pem");
    if (XFILE::CFile::Exists(cacert))
      CEnvironment::setenv("SSL_CERT_FILE", cacert.c_str(), 1);
  }

  m_winEvents.reset(new CWinEventsWin32());
  AE::CAESinkFactory::ClearSinks();
  CAESinkDirectSound::Register();
  CAESinkWASAPI::Register();
  CScreenshotSurfaceWindows::Register();

  if (CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_bScanIRServer)
  {
    m_irss.reset(new CIRServerSuite());
    m_irss->Initialize();
  }
  m_dpms = std::make_shared<CWin32DPMSSupport>();
}

CWinSystemWin32::~CWinSystemWin32()
{
  if (m_hIcon)
  {
    DestroyIcon(m_hIcon);
    m_hIcon = nullptr;
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
  if (m_hMonitor)
  {
    MONITOR_DETAILS* details = GetDisplayDetails(m_hMonitor);
    if (details)
      RestoreDesktopResolution(details);
  }
  return true;
}

bool CWinSystemWin32::CreateNewWindow(const std::string& name, bool fullScreen, RESOLUTION_INFO& res)
{
  using KODI::PLATFORM::WINDOWS::ToW;
  auto nameW = ToW(name);

  m_hInstance = static_cast<HINSTANCE>(GetModuleHandle(nullptr));
  if(m_hInstance == nullptr)
    CLog::LogF(LOGDEBUG, " GetModuleHandle failed with {}", GetLastError());

  UpdateStates(fullScreen);
  // initialize the state
  WINDOW_STATE state = GetState(fullScreen);

  m_nWidth  = res.iWidth;
  m_nHeight = res.iHeight;
  m_bFullScreen = fullScreen;
  m_fRefreshRate = res.fRefreshRate;

  m_hIcon = LoadIcon(m_hInstance, MAKEINTRESOURCE(IDI_MAIN_ICON));

  // Register the windows class
  WNDCLASSEX wndClass = {};
  wndClass.cbSize = sizeof(wndClass);
  wndClass.style = CS_HREDRAW | CS_VREDRAW;
  wndClass.lpfnWndProc = CWinEventsWin32::WndProc;
  wndClass.cbClsExtra = 0;
  wndClass.cbWndExtra = 0;
  wndClass.hInstance = m_hInstance;
  wndClass.hIcon = m_hIcon;
  wndClass.hCursor = LoadCursor(nullptr, IDC_ARROW );
  wndClass.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
  wndClass.lpszMenuName = nullptr;
  wndClass.lpszClassName = nameW.c_str();

  if( !RegisterClassExW( &wndClass ) )
  {
    CLog::LogF(LOGERROR, " RegisterClassExW failed with {}", GetLastError());
    return false;
  }

  // put the window at desired display
  RECT screenRect = ScreenRect(m_hMonitor);
  m_nLeft = screenRect.left;
  m_nTop = screenRect.top;

  if (state == WINDOW_STATE_WINDOWED)
  {
    const auto settings = CServiceBroker::GetSettingsComponent()->GetSettings();
    const int top = settings->GetInt(SETTING_WINDOW_TOP);
    const int left = settings->GetInt(SETTING_WINDOW_LEFT);
    const RECT vsRect = GetVirtualScreenRect();

    // we check that window is inside of virtual screen rect (sum of all monitors)
    // top 0 left 0 is a special position that centers the window on the screen
    if ((top != 0 || left != 0) && top >= vsRect.top && top + m_nHeight <= vsRect.bottom &&
        left >= vsRect.left && left + m_nWidth <= vsRect.right)
    {
      // restore previous window position
      m_nLeft = left;
      m_nTop = top;
    }
    else
    {
      // Windowed mode: position and size in settings and most places in Kodi
      // are for the client part of the window.
      RECT rcWorkArea = GetScreenWorkArea(m_hMonitor);

      RECT rcNcArea = GetNcAreaOffsets(m_windowStyle, false, m_windowExStyle);
      int maxClientWidth = (rcWorkArea.right - rcNcArea.right) - (rcWorkArea.left - rcNcArea.left);
      int maxClientHeight = (rcWorkArea.bottom - rcNcArea.bottom) - (rcWorkArea.top - rcNcArea.top);

      m_nWidth = std::min(m_nWidth, maxClientWidth);
      m_nHeight = std::min(m_nHeight, maxClientHeight);
      CWinSystemBase::SetWindowResolution(m_nWidth, m_nHeight);

      // center window on desktop
      m_nLeft = rcWorkArea.left - rcNcArea.left + (maxClientWidth - m_nWidth) / 2;
      m_nTop = rcWorkArea.top - rcNcArea.top + (maxClientHeight - m_nHeight) / 2;
    }
    m_ValidWindowedPosition = true;
  }

  HWND hWnd = CreateWindowExW(
    m_windowExStyle,
    nameW.c_str(),
    nameW.c_str(),
    m_windowStyle,
    m_nLeft,
    m_nTop,
    m_nWidth,
    m_nHeight,
    nullptr,
    nullptr,
    m_hInstance,
    nullptr
  );

  if( hWnd == nullptr )
  {
    CLog::LogF(LOGERROR, " CreateWindow failed with {}", GetLastError());
    return false;
  }

  m_inFocus = true;

  DWORD dwHwndTabletProperty =
      TABLET_DISABLE_PENBARRELFEEDBACK | // disables UI feedback on pen button down (circle)
      TABLET_DISABLE_FLICKS; // disables pen flicks (back, forward, drag down, drag up)

  SetProp(hWnd, MICROSOFT_TABLETPENSERVICE_PROPERTY, &dwHwndTabletProperty);

  m_hWnd = hWnd;
  m_bWindowCreated = true;

  CreateBlankWindows();

  m_state = state;
  AdjustWindow(true);

  // Show the window
  ShowWindow( m_hWnd, SW_SHOWDEFAULT );
  UpdateWindow( m_hWnd );

  // Configure the tray icon.
  m_trayIcon.cbSize = sizeof(m_trayIcon);
  m_trayIcon.hWnd = m_hWnd;
  m_trayIcon.hIcon = m_hIcon;
  wcsncpy(m_trayIcon.szTip, nameW.c_str(), sizeof(m_trayIcon.szTip) / sizeof(WCHAR));
  m_trayIcon.uCallbackMessage = TRAY_ICON_NOTIFY;
  m_trayIcon.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;

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
  wcex.hInstance= nullptr;
  wcex.hIcon= nullptr;
  wcex.hCursor= nullptr;
  wcex.hbrBackground= static_cast<HBRUSH>(CreateSolidBrush(RGB(0, 0, 0)));
  wcex.lpszMenuName= nullptr;
  wcex.lpszClassName= L"BlankWindowClass";
  wcex.hIconSm= nullptr;

  // Now we can go ahead and register our new window class
  if(!RegisterClassEx(&wcex))
  {
    CLog::LogF(LOGERROR, "RegisterClass failed with {}", GetLastError());
    return false;
  }

  // We need as many blank windows as there are screens (minus 1)
  for (size_t i = 0; i < m_displays.size() - 1; i++)
  {
    HWND hBlankWindow = CreateWindowEx(WS_EX_TOPMOST, L"BlankWindowClass", L"", WS_POPUP | WS_DISABLED,
    CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, nullptr, nullptr, nullptr, nullptr);

    if (hBlankWindow == nullptr)
    {
      CLog::LogF(LOGERROR, "CreateWindowEx failed with {}", GetLastError());
      return false;
    }

    m_hBlankWindows.push_back(hBlankWindow);
  }

  return true;
}

bool CWinSystemWin32::BlankNonActiveMonitors(bool bBlank)
{
  if (m_hBlankWindows.empty())
    return false;

  if (bBlank == false)
  {
    for (unsigned int i=0; i < m_hBlankWindows.size(); i++)
      ShowWindow(m_hBlankWindows[i], SW_HIDE);
    return true;
  }

  // Move a blank window in front of every display, except the current display.
  for (size_t i = 0, j = 0; i < m_displays.size(); ++i)
  {
    MONITOR_DETAILS& details = m_displays[i];
    if (details.hMonitor == m_hMonitor)
      continue;

    RECT rBounds = ScreenRect(details.hMonitor);
    // move and resize the window
    SetWindowPos(m_hBlankWindows[j], nullptr, rBounds.left, rBounds.top,
      rBounds.right - rBounds.left, rBounds.bottom - rBounds.top,
      SWP_NOACTIVATE);

    ShowWindow(m_hBlankWindows[j], SW_SHOW | SW_SHOWNOACTIVATE);
    j++;
  }

  if (m_hWnd)
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

  SetWindowPos(m_hWnd, nullptr, rc.left, rc.top, 0, 0, SWP_NOSIZE);

  return true;
}

bool CWinSystemWin32::ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop)
{
  m_nWidth = newWidth;
  m_nHeight = newHeight;

  if (newLeft > 0)
    m_nLeft = newLeft;

  if (newTop > 0)
    m_nTop = newTop;

  AdjustWindow();

  return true;
}

void CWinSystemWin32::FinishWindowResize(int newWidth, int newHeight)
{
  m_nWidth = newWidth;
  m_nHeight = newHeight;
}

void CWinSystemWin32::ForceFullScreen(const RESOLUTION_INFO& resInfo)
{
  ResizeWindow(resInfo.iScreenWidth, resInfo.iScreenHeight, 0, 0);
}

void CWinSystemWin32::AdjustWindow(bool forceResize)
{
  CLog::LogF(LOGDEBUG, "adjusting window if required.");

  HWND windowAfter;
  RECT rc;

  if (m_state == WINDOW_STATE_FULLSCREEN_WINDOW || m_state == WINDOW_STATE_FULLSCREEN)
  {
    windowAfter = HWND_TOP;
    rc = ScreenRect(m_hMonitor);
  }
  else // m_state == WINDOW_STATE_WINDOWED
  {
    windowAfter = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_alwaysOnTop ? HWND_TOPMOST : HWND_NOTOPMOST;

    if (!m_ValidWindowedPosition)
    {
      const auto settings = CServiceBroker::GetSettingsComponent()->GetSettings();
      const int top = settings->GetInt(SETTING_WINDOW_TOP);
      const int left = settings->GetInt(SETTING_WINDOW_LEFT);
      const RECT vsRect = GetVirtualScreenRect();

      // we check that window is inside of virtual screen rect (sum of all monitors)
      // top 0 left 0 is a special position that centers the window on the screen
      if ((top != 0 || left != 0) && top >= vsRect.top && top + m_nHeight <= vsRect.bottom &&
          left >= vsRect.left && left + m_nWidth <= vsRect.right)
      {
        // restore previous window position
        m_nTop = top;
        m_nLeft = left;
      }
      else
      {
        // Windowed mode: position and size in settings and most places in Kodi
        // are for the client part of the window.
        RECT rcWorkArea = GetScreenWorkArea(m_hMonitor);

        RECT rcNcArea = GetNcAreaOffsets(m_windowStyle, false, m_windowExStyle);
        int maxClientWidth =
            (rcWorkArea.right - rcNcArea.right) - (rcWorkArea.left - rcNcArea.left);
        int maxClientHeight =
            (rcWorkArea.bottom - rcNcArea.bottom) - (rcWorkArea.top - rcNcArea.top);

        m_nWidth = std::min(m_nWidth, maxClientWidth);
        m_nHeight = std::min(m_nHeight, maxClientHeight);
        CWinSystemBase::SetWindowResolution(m_nWidth, m_nHeight);

        // center window on desktop
        m_nLeft = rcWorkArea.left - rcNcArea.left + (maxClientWidth - m_nWidth) / 2;
        m_nTop = rcWorkArea.top - rcNcArea.top + (maxClientHeight - m_nHeight) / 2;
      }
      m_ValidWindowedPosition = true;
    }

    rc.left = m_nLeft;
    rc.right = m_nLeft + m_nWidth;
    rc.top = m_nTop;
    rc.bottom = m_nTop + m_nHeight;

    HMONITOR hMon = MonitorFromRect(&rc, MONITOR_DEFAULTTONULL);
    HMONITOR hMon2 = MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTOPRIMARY);

    if (!m_ValidWindowedPosition || hMon == nullptr || hMon != hMon2)
    {
      // centering window at desktop
      RECT newScreenRect = ScreenRect(hMon2);
      rc.left = m_nLeft = newScreenRect.left + ((newScreenRect.right - newScreenRect.left) / 2) - (m_nWidth / 2);
      rc.top = m_nTop = newScreenRect.top + ((newScreenRect.bottom - newScreenRect.top) / 2) - (m_nHeight / 2);
      rc.right = m_nLeft + m_nWidth;
      rc.bottom = m_nTop + m_nHeight;
      m_ValidWindowedPosition = true;
    }
    AdjustWindowRectEx(&rc, m_windowStyle, false, m_windowExStyle);
  }

  WINDOWINFO wi;
  wi.cbSize = sizeof(WINDOWINFO);
  if (!GetWindowInfo(m_hWnd, &wi))
  {
    CLog::LogF(LOGERROR, "GetWindowInfo failed with {}", GetLastError());
    return;
  }
  RECT wr = wi.rcWindow;

  if ( wr.bottom - wr.top == rc.bottom - rc.top
    && wr.right - wr.left == rc.right - rc.left
    && (wi.dwStyle & WS_CAPTION) == (m_windowStyle & WS_CAPTION)
    && !forceResize)
  {
    return;
  }

  //Sets the window style
  SetLastError(0);
  SetWindowLongPtr( m_hWnd, GWL_STYLE, m_windowStyle );

  //Sets the window ex style
  SetLastError(0);
  SetWindowLongPtr( m_hWnd, GWL_EXSTYLE, m_windowExStyle );

  // resize window
  CLog::LogF(LOGDEBUG, "resizing due to size change ({},{},{},{}{})->({},{},{},{}{})", wr.left,
             wr.top, wr.right, wr.bottom, (wi.dwStyle & WS_CAPTION) ? "" : " fullscreen", rc.left,
             rc.top, rc.right, rc.bottom, (m_windowStyle & WS_CAPTION) ? "" : " fullscreen");
  SetWindowPos(
    m_hWnd,
    windowAfter,
    rc.left,
    rc.top,
    rc.right - rc.left,
    rc.bottom - rc.top,
    SWP_SHOWWINDOW | SWP_DRAWFRAME
  );
}

void CWinSystemWin32::CenterCursor() const
{
  RECT rect;
  POINT point = {};

  //Gets the client rect, then translates it to screen coordinates
  //so that SetCursorPos isn't called with relative x and y values
  GetClientRect(m_hWnd, &rect);
  ClientToScreen(m_hWnd, &point);

  rect.left += point.x;
  rect.right += point.x;
  rect.top += point.y;
  rect.bottom += point.y;

  int x = rect.left + (rect.right - rect.left) / 2;
  int y = rect.top + (rect.bottom - rect.top) / 2;

  SetCursorPos(x, y);
}

bool CWinSystemWin32::SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays)
{
  // Initialisation not finished, pretend the function succeeded
  if (!m_hWnd)
    return true;

  CWinSystemWin32::UpdateStates(fullScreen);
  WINDOW_STATE state = GetState(fullScreen);

  CLog::LogF(LOGDEBUG, "({}) with size {}x{}, refresh {:f}{}", window_state_names[state],
             res.iWidth, res.iHeight, res.fRefreshRate,
             (res.dwFlags & D3DPRESENTFLAG_INTERLACED) ? "i" : "");

  // oldMonitor may be NULL if it's powered off or not available due windows settings
  MONITOR_DETAILS* oldMonitor = GetDisplayDetails(m_hMonitor);
  MONITOR_DETAILS* newMonitor = GetDisplayDetails(res.strOutput);

  bool forceChange = false;    // resolution/display is changed but window state isn't changed
  bool changeScreen = false;   // display is changed
  bool stereoChange = IsStereoEnabled() != (CServiceBroker::GetWinSystem()->GetGfxContext().GetStereoMode() == RENDER_STEREO_MODE_HARDWAREBASED);

  if (m_nWidth != res.iWidth || m_nHeight != res.iHeight || m_fRefreshRate != res.fRefreshRate ||
      !oldMonitor || oldMonitor->hMonitor != newMonitor->hMonitor || stereoChange ||
      m_bFirstResChange)
  {
    if (!oldMonitor || oldMonitor->hMonitor != newMonitor->hMonitor)
      changeScreen = true;
    forceChange = true;
  }

  if (state == m_state && !forceChange)
  {
    m_bBlankOtherDisplay = blankOtherDisplays;
    BlankNonActiveMonitors(m_bBlankOtherDisplay);
    return true;
  }

  // entering to stereo mode, limit resolution to 1080p@23.976
  if (stereoChange && !IsStereoEnabled() && res.iWidth > 1280)
  {
    res = CDisplaySettings::GetInstance().GetResolutionInfo(
        CResolutionUtils::ChooseBestResolution(24.f / 1.001f, 1920, 1080, true));
  }

  if (m_state == WINDOW_STATE_WINDOWED)
  {
    WINDOWINFO wi = {};
    wi.cbSize = sizeof(WINDOWINFO);
    if (GetWindowInfo(m_hWnd, &wi) && wi.rcClient.top > 0)
    {
      m_nLeft = wi.rcClient.left;
      m_nTop = wi.rcClient.top;
      m_ValidWindowedPosition = true;
    }
  }

  m_IsAlteringWindow = true;
  ReleaseBackBuffer();

  if (changeScreen)
  {
    // before we changing display we have to leave exclusive mode on "old" display
    if (m_state == WINDOW_STATE_FULLSCREEN)
      SetDeviceFullScreen(false, res);

    // restoring native resolution on "old" display
    RestoreDesktopResolution(oldMonitor);

    // notify about screen change (it may require recreate rendering device)
    m_fRefreshRate = res.fRefreshRate; // use desired refresh for driver hook
    OnScreenChange(newMonitor->hMonitor);
  }

  m_bFirstResChange = false;
  m_bFullScreen = fullScreen;
  m_hMonitor = newMonitor->hMonitor;
  m_nWidth = res.iWidth;
  m_nHeight = res.iHeight;
  m_bBlankOtherDisplay = blankOtherDisplays;
  m_fRefreshRate = res.fRefreshRate;

  if (state == WINDOW_STATE_FULLSCREEN)
  {
    SetForegroundWindowInternal(m_hWnd);

    m_state = state;
    AdjustWindow(changeScreen);

    // enter in exclusive mode, this will change resolution if we already in
    SetDeviceFullScreen(true, res);
  }
  else if (m_state == WINDOW_STATE_FULLSCREEN || m_state == WINDOW_STATE_FULLSCREEN_WINDOW) // we're in fullscreen state now
  {
    // guess we are leaving exclusive mode, this will not an effect if we already not in
    SetDeviceFullScreen(false, res);

    if (state == WINDOW_STATE_WINDOWED) // go to a windowed state
    {
      // need to restore resolution if it was changed to not native
      // because we do not support resolution change in windowed mode
      RestoreDesktopResolution(newMonitor);
    }
    else if (state == WINDOW_STATE_FULLSCREEN_WINDOW) // enter fullscreen window instead
    {
      ChangeResolution(res, stereoChange);
    }

    m_state = state;
    AdjustWindow(changeScreen);
  }
  else // we're in windowed state now
  {
    if (state == WINDOW_STATE_FULLSCREEN_WINDOW)
    {
      ChangeResolution(res, stereoChange);

      m_state = state;
      AdjustWindow(changeScreen);
    }
  }

  if (changeScreen)
    CenterCursor();

  CreateBackBuffer();

  BlankNonActiveMonitors(m_bBlankOtherDisplay);
  m_IsAlteringWindow = false;

  return true;
}

bool CWinSystemWin32::DPIChanged(WORD dpi, RECT windowRect) const
{
  (void)dpi;
  RECT resizeRect = windowRect;
  HMONITOR hMon = MonitorFromRect(&resizeRect, MONITOR_DEFAULTTONULL);
  if (hMon == nullptr)
  {
    hMon = MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTOPRIMARY);
  }

  if (hMon)
  {
    MONITORINFOEX monitorInfo;
    monitorInfo.cbSize = sizeof(MONITORINFOEX);
    GetMonitorInfoW(hMon, &monitorInfo);

    if (m_state == WINDOW_STATE_FULLSCREEN_WINDOW ||
        m_state == WINDOW_STATE_FULLSCREEN)
    {
      resizeRect = monitorInfo.rcMonitor; // the whole screen
    }
    else
    {
      RECT wr = monitorInfo.rcWork; // it excludes task bar
      long wrWidth = wr.right - wr.left;
      long wrHeight = wr.bottom - wr.top;
      long resizeWidth = resizeRect.right - resizeRect.left;
      long resizeHeight = resizeRect.bottom - resizeRect.top;

      if (resizeWidth > wrWidth)
      {
        resizeRect.right = resizeRect.left + wrWidth;
      }

      // make sure suggested windows size is not taller or wider than working area of new monitor (considers the toolbar)
      if (resizeHeight > wrHeight)
      {
        resizeRect.bottom = resizeRect.top + wrHeight;
      }
    }
  }

  // resize the window to the suggested size. Will generate a WM_SIZE event
  SetWindowPos(m_hWnd,
    nullptr,
    resizeRect.left,
    resizeRect.top,
    resizeRect.right - resizeRect.left,
    resizeRect.bottom - resizeRect.top,
    SWP_NOZORDER | SWP_NOACTIVATE);

  return true;
}

void CWinSystemWin32::SetMinimized(bool minimized)
{
  const auto advancedSettings = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings();

  if (advancedSettings->m_minimizeToTray)
  {
    if (minimized)
    {
      Shell_NotifyIcon(NIM_ADD, &m_trayIcon);
      ShowWindow(m_hWnd, SW_HIDE);
    }
    else
    {
      Shell_NotifyIcon(NIM_DELETE, &m_trayIcon);
      ShowWindow(m_hWnd, SW_RESTORE);
    }
  }

  m_bMinimized = minimized;
}

std::vector<std::string> CWinSystemWin32::GetConnectedOutputs()
{
  std::vector<std::string> outputs;

  for (auto& display : m_displays)
  {
    outputs.emplace_back(KODI::PLATFORM::WINDOWS::FromW(display.MonitorNameW));
  }

  return outputs;
}

void CWinSystemWin32::RestoreDesktopResolution(MONITOR_DETAILS* details)
{
  if (!details)
    return;

  RESOLUTION_INFO info;
  info.iWidth = details->ScreenWidth;
  info.iHeight = details->ScreenHeight;
  if ((details->RefreshRate + 1) % 24 == 0 || (details->RefreshRate + 1) % 30 == 0)
    info.fRefreshRate = static_cast<float>(details->RefreshRate + 1) / 1.001f;
  else
    info.fRefreshRate = static_cast<float>(details->RefreshRate);
  info.strOutput = KODI::PLATFORM::WINDOWS::FromW(details->DeviceNameW);
  info.dwFlags = details->Interlaced ? D3DPRESENTFLAG_INTERLACED : 0;

  CLog::LogF(LOGDEBUG, "restoring desktop resolution for '{}'.", KODI::PLATFORM::WINDOWS::FromW(details->MonitorNameW));
  ChangeResolution(info);
}

MONITOR_DETAILS* CWinSystemWin32::GetDisplayDetails(const std::string& name)
{
  using KODI::PLATFORM::WINDOWS::ToW;

  if (!name.empty() && name != "Default")
  {
    std::wstring nameW = ToW(name);
    auto it = std::find_if(m_displays.begin(), m_displays.end(), [&nameW](MONITOR_DETAILS& m)
    {
      if (nameW[0] == '\\') // name is device name
        return m.DeviceNameW == nameW;
      else if (m.MonitorNameW == nameW)
        return true;
      else if (m.DeviceStringW == nameW)
        return true;
      return false;
    });
    if (it != m_displays.end())
      return &(*it);
  }

  // fallback to primary
  auto it = std::find_if(m_displays.begin(), m_displays.end(), [](MONITOR_DETAILS& m)
  {
    return m.IsPrimary;
  });
  if (it != m_displays.end())
    return &(*it);

  // nothing found
  return nullptr;
}

MONITOR_DETAILS* CWinSystemWin32::GetDisplayDetails(HMONITOR handle)
{
  auto it = std::find_if(m_displays.begin(), m_displays.end(), [&handle](MONITOR_DETAILS& m)
  {
    return m.hMonitor == handle;
  });
  if (it != m_displays.end())
    return &(*it);

  return nullptr;
}

RECT CWinSystemWin32::ScreenRect(HMONITOR handle)
{
  const MONITOR_DETAILS* details = GetDisplayDetails(handle);
  if (!details)
  {
    CLog::LogF(LOGERROR, "no monitor found for handle");
    return RECT();
  }

  DEVMODEW sDevMode = {};
  sDevMode.dmSize = sizeof(sDevMode);
  if(!EnumDisplaySettingsW(details->DeviceNameW.c_str(), ENUM_CURRENT_SETTINGS, &sDevMode))
    CLog::LogF(LOGERROR, " EnumDisplaySettings failed with {}", GetLastError());

  RECT rc;
  rc.left = sDevMode.dmPosition.x;
  rc.right = sDevMode.dmPosition.x + sDevMode.dmPelsWidth;
  rc.top = sDevMode.dmPosition.y;
  rc.bottom = sDevMode.dmPosition.y + sDevMode.dmPelsHeight;

  return rc;
}

void CWinSystemWin32::GetConnectedDisplays(std::vector<MONITOR_DETAILS>& outputs)
{
  using KODI::PLATFORM::WINDOWS::FromW;

  DISPLAY_DEVICEW ddAdapter = {};
  ddAdapter.cb = sizeof(ddAdapter);

  for (DWORD adapter = 0; EnumDisplayDevicesW(nullptr, adapter, &ddAdapter, 0); ++adapter)
  {
    // Exclude displays that are not part of the windows desktop. Using them is too different: no windows,
    // direct access with GDI CreateDC() or DirectDraw for example. So it may be possible to play video, but GUI?
    if ((ddAdapter.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER)
      || !(ddAdapter.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP))
      continue;

    DISPLAY_DEVICEW ddMon = {};
    ddMon.cb = sizeof(ddMon);
    bool foundScreen = false;

    DWORD screen = 0;
    // Just look for the first active output, we're actually only interested in the information at the adapter level.
    for (; EnumDisplayDevicesW(ddAdapter.DeviceName, screen, &ddMon, 0); ++screen)
    {
      if (ddMon.StateFlags & (DISPLAY_DEVICE_ACTIVE | DISPLAY_DEVICE_ATTACHED))
      {
        foundScreen = true;
        break;
      }
    }

    // Remoting returns no screens. Handle with a dummy screen.
    if (!foundScreen && screen == 0)
    {
      lstrcpyW(ddMon.DeviceString, L"Dummy Monitor"); // safe: large static array
      foundScreen = true;
    }

    if (foundScreen)
    {
      // get information about the display's current position and display mode
      DEVMODEW dm = {};
      dm.dmSize = sizeof(dm);
      if (EnumDisplaySettingsExW(ddAdapter.DeviceName, ENUM_CURRENT_SETTINGS, &dm, 0) == FALSE)
        EnumDisplaySettingsExW(ddAdapter.DeviceName, ENUM_REGISTRY_SETTINGS, &dm, 0);

      POINT pt = { dm.dmPosition.x, dm.dmPosition.y };
      HMONITOR hm = MonitorFromPoint(pt, MONITOR_DEFAULTTONULL);

      MONITOR_DETAILS md = {};
      uint8_t num = 1;
      do
      {
        // `Monitor #N`
        md.DeviceStringW = std::wstring(ddMon.DeviceString) + L" #" + std::to_wstring(num++);
      } while (std::any_of(outputs.begin(), outputs.end(), [&](MONITOR_DETAILS& m) {
        return m.DeviceStringW == md.DeviceStringW;
      }));

      std::wstring displayName = CWIN32Util::GetDisplayFriendlyName(ddAdapter.DeviceName);
      if (displayName.empty())
        displayName = std::wstring(ddMon.DeviceString);
      num = 1;
      do
      {
        // `Pretty Monitor Name #N`
        md.MonitorNameW = displayName + L" #" + std::to_wstring(num++);
      } while (std::any_of(outputs.begin(), outputs.end(),
                           [&](MONITOR_DETAILS& m) { return m.MonitorNameW == md.MonitorNameW; }));

      md.CardNameW = ddAdapter.DeviceString;
      md.DeviceNameW = ddAdapter.DeviceName;
      md.ScreenWidth = dm.dmPelsWidth;
      md.ScreenHeight = dm.dmPelsHeight;
      md.hMonitor = hm;
      md.RefreshRate = dm.dmDisplayFrequency;
      md.Bpp = dm.dmBitsPerPel;
      md.Interlaced = (dm.dmDisplayFlags & DM_INTERLACED) ? true : false;

      MONITORINFO mi = {};
      mi.cbSize = sizeof(mi);
      if (GetMonitorInfoW(hm, &mi) && (mi.dwFlags & MONITORINFOF_PRIMARY))
        md.IsPrimary = true;

      outputs.push_back(md);
    }
  }
}

bool CWinSystemWin32::ChangeResolution(const RESOLUTION_INFO& res, bool forceChange /*= false*/)
{
  using KODI::PLATFORM::WINDOWS::ToW;
  std::wstring outputW = ToW(res.strOutput);

  DEVMODEW sDevMode = {};
  sDevMode.dmSize = sizeof(sDevMode);

  // If we can't read the current resolution or any detail of the resolution is different than res
  if (!EnumDisplaySettingsW(outputW.c_str(), ENUM_CURRENT_SETTINGS, &sDevMode) ||
      sDevMode.dmPelsWidth != res.iWidth || sDevMode.dmPelsHeight != res.iHeight ||
      sDevMode.dmDisplayFrequency != static_cast<int>(res.fRefreshRate) ||
      ((sDevMode.dmDisplayFlags & DM_INTERLACED) && !(res.dwFlags & D3DPRESENTFLAG_INTERLACED)) ||
      (!(sDevMode.dmDisplayFlags & DM_INTERLACED) && (res.dwFlags & D3DPRESENTFLAG_INTERLACED))
      || forceChange)
  {
    ZeroMemory(&sDevMode, sizeof(sDevMode));
    sDevMode.dmSize = sizeof(sDevMode);
    sDevMode.dmDriverExtra = 0;
    sDevMode.dmPelsWidth = res.iWidth;
    sDevMode.dmPelsHeight = res.iHeight;
    sDevMode.dmDisplayFrequency = static_cast<int>(res.fRefreshRate);
    sDevMode.dmDisplayFlags = (res.dwFlags & D3DPRESENTFLAG_INTERLACED) ? DM_INTERLACED : 0;
    sDevMode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY | DM_DISPLAYFLAGS;

    LONG rc;
    bool bResChanged = false;

    // Windows 8 refresh rate workaround for 24.0, 48.0 and 60.0 Hz
    if (res.fRefreshRate == 24.0 || res.fRefreshRate == 48.0 || res.fRefreshRate == 60.0)
    {
      CLog::LogF(LOGDEBUG, "Using Windows 8+ workaround for refresh rate {} Hz",
                 static_cast<int>(res.fRefreshRate));

      // Get current resolution stored in registry
      DEVMODEW sDevModeRegistry = {};
      sDevModeRegistry.dmSize = sizeof(sDevModeRegistry);
      if (EnumDisplaySettingsW(outputW.c_str(), ENUM_REGISTRY_SETTINGS, &sDevModeRegistry))
      {
        // Set requested mode in registry without actually changing resolution
        rc = ChangeDisplaySettingsExW(outputW.c_str(), &sDevMode, nullptr, CDS_UPDATEREGISTRY | CDS_NORESET, nullptr);
        if (rc == DISP_CHANGE_SUCCESSFUL)
        {
          // Change resolution based on registry setting
          rc = ChangeDisplaySettingsExW(outputW.c_str(), nullptr, nullptr, CDS_FULLSCREEN, nullptr);
          if (rc == DISP_CHANGE_SUCCESSFUL)
            bResChanged = true;
          else
            CLog::LogF(
                LOGERROR,
                "ChangeDisplaySettingsEx (W8+ change resolution) failed with {}, using fallback",
                rc);

          // Restore registry with original values
          sDevModeRegistry.dmSize = sizeof(sDevModeRegistry);
          sDevModeRegistry.dmDriverExtra = 0;
          sDevModeRegistry.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY | DM_DISPLAYFLAGS;
          rc = ChangeDisplaySettingsExW(outputW.c_str(), &sDevModeRegistry, nullptr, CDS_UPDATEREGISTRY | CDS_NORESET, nullptr);
          if (rc != DISP_CHANGE_SUCCESSFUL)
            CLog::LogF(LOGERROR, "ChangeDisplaySettingsEx (W8+ restore registry) failed with {}",
                       rc);
        }
        else
          CLog::LogF(LOGERROR,
                     "ChangeDisplaySettingsEx (W8+ set registry) failed with {}, using fallback",
                     rc);
      }
      else
        CLog::LogF(LOGERROR, "Unable to retrieve registry settings for Windows 8+ workaround, using fallback");
    }

    // Standard resolution change/fallback for Windows 8+ workaround
    if (!bResChanged)
    {
      // CDS_FULLSCREEN is for temporary fullscreen mode and prevents icons and windows from moving
      // to fit within the new dimensions of the desktop
      rc = ChangeDisplaySettingsExW(outputW.c_str(), &sDevMode, nullptr, CDS_FULLSCREEN, nullptr);
      if (rc == DISP_CHANGE_SUCCESSFUL)
        bResChanged = true;
      else
        CLog::LogF(LOGERROR, "ChangeDisplaySettingsEx failed with {}", rc);
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
  using KODI::PLATFORM::WINDOWS::FromW;

  m_displays.clear();

  CWinSystemBase::UpdateResolutions();
  GetConnectedDisplays(m_displays);

  const auto settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  std::string settingsMonitor = settings->GetString(CSettings::SETTING_VIDEOSCREEN_MONITOR);
  MONITOR_DETAILS* details = GetDisplayDetails(settingsMonitor);
  if (!details)
    return;
  // Migrate the setting to EDID-based name for connected and recognized screens.
  // Other screens may be temporarily turned off, ignore so they're used once available again
  if (settingsMonitor != FromW(details->MonitorNameW) &&
      (settingsMonitor == FromW(details->DeviceStringW) ||
       settingsMonitor == FromW(details->DeviceNameW)))
    CDisplaySettings::GetInstance().SetMonitor(FromW(details->MonitorNameW));

  float refreshRate;
  int w = details->ScreenWidth;
  int h = details->ScreenHeight;
  if ((details->RefreshRate + 1) % 24 == 0 || (details->RefreshRate + 1) % 30 == 0)
    refreshRate = static_cast<float>(details->RefreshRate + 1) / 1.001f;
  else
    refreshRate = static_cast<float>(details->RefreshRate);

  std::string strOutput = FromW(details->DeviceNameW);
  std::string monitorName = FromW(details->MonitorNameW);

  uint32_t dwFlags = details->Interlaced ? D3DPRESENTFLAG_INTERLACED : 0;

  RESOLUTION_INFO& info = CDisplaySettings::GetInstance().GetResolutionInfo(RES_DESKTOP);
  UpdateDesktopResolution(info, monitorName, w, h, refreshRate, dwFlags);
  info.strOutput = strOutput;

  CLog::Log(LOGINFO, "Primary mode: {}", info.strMode);

  // erase previous stored modes
  CDisplaySettings::GetInstance().ClearCustomResolutions();

  for(int mode = 0;; mode++)
  {
    DEVMODEW devmode = {};
    devmode.dmSize = sizeof(devmode);
    if(EnumDisplaySettingsW(details->DeviceNameW.c_str(), mode, &devmode) == 0)
      break;
    if(devmode.dmBitsPerPel != 32)
      continue;

    float refresh;
    if ((devmode.dmDisplayFrequency + 1) % 24 == 0 || (devmode.dmDisplayFrequency + 1) % 30 == 0)
      refresh = static_cast<float>(devmode.dmDisplayFrequency + 1) / 1.001f;
    else
      refresh = static_cast<float>(devmode.dmDisplayFrequency);
    dwFlags = (devmode.dmDisplayFlags & DM_INTERLACED) ? D3DPRESENTFLAG_INTERLACED : 0;

    RESOLUTION_INFO res;
    res.iWidth = devmode.dmPelsWidth;
    res.iHeight = devmode.dmPelsHeight;
    res.bFullScreen = true;
    res.dwFlags = dwFlags;
    res.fRefreshRate = refresh;
    res.fPixelRatio = 1.0f;
    res.iScreenWidth = res.iWidth;
    res.iScreenHeight = res.iHeight;
    res.iSubtitles = res.iHeight;
    res.strMode = StringUtils::Format("{}: {}x{} @ {:.2f}Hz", monitorName, res.iWidth, res.iHeight,
                                      res.fRefreshRate);
    GetGfxContext().ResetOverscan(res);
    res.strOutput = strOutput;

    if (AddResolution(res))
      CLog::Log(LOGINFO, "Additional mode: {}", res.strMode);
  }

  CDisplaySettings::GetInstance().ApplyCalibrations();
}

bool CWinSystemWin32::AddResolution(const RESOLUTION_INFO &res)
{
  for (unsigned int i = RES_CUSTOM; i < CDisplaySettings::GetInstance().ResolutionInfoSize(); i++)
  {
    RESOLUTION_INFO& info = CDisplaySettings::GetInstance().GetResolutionInfo(i);
    if (info.iWidth        == res.iWidth
     && info.iHeight       == res.iHeight
     && info.iScreenWidth  == res.iScreenWidth
     && info.iScreenHeight == res.iScreenHeight
     && info.fRefreshRate  == res.fRefreshRate
     && info.dwFlags       == res.dwFlags)
      return false; // already have this resolution
  }

  CDisplaySettings::GetInstance().AddResolutionInfo(res);
  return true;
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
      windowAfter = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_alwaysOnTop ? HWND_TOPMOST : HWND_NOTOPMOST;
  }

  SetWindowPos(m_hWnd, windowAfter, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW | SWP_ASYNCWINDOWPOS);
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
  std::unique_lock<CCriticalSection> lock(m_resourceSection);
  m_resources.push_back(resource);
}

void CWinSystemWin32::Unregister(IDispResource* resource)
{
  std::unique_lock<CCriticalSection> lock(m_resourceSection);
  std::vector<IDispResource*>::iterator i = find(m_resources.begin(), m_resources.end(), resource);
  if (i != m_resources.end())
    m_resources.erase(i);
}

void CWinSystemWin32::OnDisplayLost()
{
  CLog::LogF(LOGDEBUG, "notify display lost event");

  {
    std::unique_lock<CCriticalSection> lock(m_resourceSection);
    for (std::vector<IDispResource *>::iterator i = m_resources.begin(); i != m_resources.end(); ++i)
      (*i)->OnLostDisplay();
  }
}

void CWinSystemWin32::OnDisplayReset()
{
  if (!m_delayDispReset)
  {
    CLog::LogF(LOGDEBUG, "notify display reset event");
    std::unique_lock<CCriticalSection> lock(m_resourceSection);
    for (std::vector<IDispResource *>::iterator i = m_resources.begin(); i != m_resources.end(); ++i)
      (*i)->OnResetDisplay();
  }
}

void CWinSystemWin32::OnDisplayBack()
{
  auto delay =
      std::chrono::milliseconds(CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(
                                    "videoscreen.delayrefreshchange") *
                                100);
  if (delay > 0ms)
  {
    m_delayDispReset = true;
    m_dispResetTimer.Set(delay);
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
  BYTE keyState[256] = {};
  // to unlock SetForegroundWindow we need to imitate Alt pressing
  if (GetKeyboardState(reinterpret_cast<LPBYTE>(&keyState)) && !(keyState[VK_MENU] & 0x80))
    keybd_event(VK_MENU, 0, KEYEVENTF_EXTENDEDKEY | 0, 0);

  BOOL res = SetForegroundWindow(hWnd);

  if (GetKeyboardState(reinterpret_cast<LPBYTE>(&keyState)) && !(keyState[VK_MENU] & 0x80))
    keybd_event(VK_MENU, 0, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);

  if (!res)
  {
    //relation time of SetForegroundWindow lock
    DWORD lockTimeOut = 0;
    HWND  hCurrWnd = GetForegroundWindow();
    DWORD dwThisTID = GetCurrentThreadId(),
          dwCurrTID = GetWindowThreadProcessId(hCurrWnd, nullptr);

    // we need to bypass some limitations from Microsoft
    if (dwThisTID != dwCurrTID)
    {
      AttachThreadInput(dwThisTID, dwCurrTID, TRUE);
      SystemParametersInfo(SPI_GETFOREGROUNDLOCKTIMEOUT, 0, &lockTimeOut, 0);
      SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, nullptr, SPIF_SENDWININICHANGE | SPIF_UPDATEINIFILE);
      AllowSetForegroundWindow(ASFW_ANY);
    }

    SetForegroundWindow(hWnd);

    if (dwThisTID != dwCurrTID)
    {
      SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, &lockTimeOut, SPIF_SENDWININICHANGE | SPIF_UPDATEINIFILE);
      AttachThreadInput(dwThisTID, dwCurrTID, FALSE);
    }
  }
}

std::unique_ptr<CVideoSync> CWinSystemWin32::GetVideoSync(CVideoReferenceClock* clock)
{
  std::unique_ptr<CVideoSync> pVSync(new CVideoSyncD3D(clock));
  return pVSync;
}

std::string CWinSystemWin32::GetClipboardText()
{
  std::wstring unicode_text;
  std::string utf8_text;

  if (OpenClipboard(nullptr))
  {
    HGLOBAL hglb = GetClipboardData(CF_UNICODETEXT);
    if (hglb != nullptr)
    {
      LPWSTR lpwstr = static_cast<LPWSTR>(GlobalLock(hglb));
      if (lpwstr != nullptr)
      {
        unicode_text = lpwstr;
        GlobalUnlock(hglb);
      }
    }
    CloseClipboard();
  }

  return KODI::PLATFORM::WINDOWS::FromW(unicode_text);
}

bool CWinSystemWin32::UseLimitedColor()
{
  return CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_VIDEOSCREEN_LIMITEDRANGE);
}

void CWinSystemWin32::NotifyAppFocusChange(bool bGaining)
{
  if (m_state == WINDOW_STATE_FULLSCREEN && !m_IsAlteringWindow)
  {
    m_IsAlteringWindow = true;
    ReleaseBackBuffer();

    if (bGaining)
      SetWindowPos(m_hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOREDRAW);

    RESOLUTION_INFO res = {};
    const RESOLUTION resolution = CServiceBroker::GetWinSystem()->GetGfxContext().GetVideoResolution();
    if (bGaining && resolution > RES_INVALID)
      res = CDisplaySettings::GetInstance().GetResolutionInfo(resolution);

    SetDeviceFullScreen(bGaining, res);

    if (!bGaining)
      SetWindowPos(m_hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOREDRAW);

    CreateBackBuffer();
    m_IsAlteringWindow = false;
  }
  m_inFocus = bGaining;
}

void CWinSystemWin32::UpdateStates(bool fullScreen)
{
  m_fullscreenState = CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_VIDEOSCREEN_FAKEFULLSCREEN)
    ? WINDOW_FULLSCREEN_STATE_FULLSCREEN_WINDOW
    : WINDOW_FULLSCREEN_STATE_FULLSCREEN;
  m_windowState = WINDOW_WINDOW_STATE_WINDOWED; // currently only this allowed

  // set the appropriate window style
  if (fullScreen)
  {
    m_windowStyle = FULLSCREEN_WINDOW_STYLE;
    m_windowExStyle = FULLSCREEN_WINDOW_EX_STYLE;
  }
  else
  {
    m_windowStyle = WINDOWED_STYLE;
    m_windowExStyle = WINDOWED_EX_STYLE;
  }
}

WINDOW_STATE CWinSystemWin32::GetState(bool fullScreen) const
{
  return static_cast<WINDOW_STATE>(fullScreen ? m_fullscreenState : m_windowState);
}

bool CWinSystemWin32::MessagePump()
{
  return m_winEvents->MessagePump();
}

void CWinSystemWin32::SetTogglingHDR(bool toggling)
{
  if (toggling)
    SetTimer(m_hWnd, ID_TIMER_HDR, 6000U, nullptr);

  m_IsTogglingHDR = toggling;
}

RECT CWinSystemWin32::GetVirtualScreenRect()
{
  RECT rect = {};
  rect.left = GetSystemMetrics(SM_XVIRTUALSCREEN);
  rect.right = GetSystemMetrics(SM_CXVIRTUALSCREEN) + rect.left;
  rect.top = GetSystemMetrics(SM_YVIRTUALSCREEN);
  rect.bottom = GetSystemMetrics(SM_CYVIRTUALSCREEN) + rect.top;

  return rect;
}

/*!
 * \brief Max luminance for GUI SDR content in HDR mode.
 * \return Max luminance in nits, lower than 10000.
*/
float CWinSystemWin32::GetGuiSdrPeakLuminance() const
{
  const auto settings = CServiceBroker::GetSettingsComponent()->GetSettings();

  // use cached system value as this is called for each frame
  if (settings->GetBool(CSettings::SETTING_VIDEOSCREEN_USESYSTEMSDRPEAKLUMINANCE) &&
      m_validSystemSdrPeakLuminance)
    return m_systemSdrPeakLuminance;

  // Max nits for 100% UI setting = 1000 nits, < 10000 nits, min 80 nits for 0%
  const int guiSdrPeak = settings->GetInt(CSettings::SETTING_VIDEOSCREEN_GUISDRPEAKLUMINANCE);
  return (80.0f * std::pow(std::exp(1.0f), 0.025257f * guiSdrPeak));
}

/*!
 * \brief Test support of the OS for a SDR max luminance in HDR mode setting
 * \return true when the OS supports that setting, false otherwise
*/
bool CWinSystemWin32::HasSystemSdrPeakLuminance()
{
  MONITOR_DETAILS* md = GetDisplayDetails(m_hMonitor);
  if (md)
    return CWIN32Util::GetSystemSdrWhiteLevel(md->DeviceNameW, nullptr);

  return false;
}

/*!
 * \brief Cache the system HDR/SDR balance for use during rendering, instead of querying the API
 for each frame.
*/
void CWinSystemWin32::CacheSystemSdrPeakLuminance()
{
  m_validSystemSdrPeakLuminance = false;

  MONITOR_DETAILS* md = GetDisplayDetails(m_hMonitor);
  if (md)
  {
    m_validSystemSdrPeakLuminance =
        CWIN32Util::GetSystemSdrWhiteLevel(md->DeviceNameW, &m_systemSdrPeakLuminance);
  }
}

RECT CWinSystemWin32::GetScreenWorkArea(HMONITOR handle) const
{
  MONITORINFO monitorInfo{};
  monitorInfo.cbSize = sizeof(MONITORINFO);
  if (!GetMonitorInfoW(handle, &monitorInfo))
  {
    CLog::LogF(LOGERROR, "GetMonitorInfoW failed with {}", GetLastError());
    return RECT();
  }
  return monitorInfo.rcWork;
}

RECT CWinSystemWin32::GetNcAreaOffsets(DWORD dwStyle, BOOL bMenu, DWORD dwExStyle) const
{
  RECT rcNcArea{};
  SetRectEmpty(&rcNcArea);

  if (!AdjustWindowRectEx(&rcNcArea, dwStyle, false, dwExStyle))
  {
    CLog::LogF(LOGERROR, "AdjustWindowRectEx failed with {}", GetLastError());
    return RECT();
  }
  return rcNcArea;
}
