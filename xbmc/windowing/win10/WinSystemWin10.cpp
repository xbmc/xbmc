/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://kodi.tv
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

#include "Application.h"
#include "cores/AudioEngine/AESinkFactory.h"
#include "cores/AudioEngine/Sinks/AESinkXAudio.h"
#include "cores/AudioEngine/Sinks/AESinkWASAPI.h"
#include "guilib/gui3d.h"
#include "windowing/GraphicContext.h"
#include "messaging/ApplicationMessenger.h"
#include "platform/win10/AsyncHelpers.h"
#include "platform/win10/powermanagement/Win10PowerSyscall.h"
#include "platform/win32/CharsetConverter.h"
#include "rendering/dx/DirectXHelper.h"
#include "rendering/dx/RenderContext.h"
#include "ServiceBroker.h"
#include "settings/AdvancedSettings.h"
#include "settings/DisplaySettings.h"
#include "settings/Settings.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "utils/SystemInfo.h"
#include "windowing/windows/VideoSyncD3D.h"
#include "WinEventsWin10.h"
#include "WinSystemWin10.h"

#pragma pack(push,8)

#include <tpcshrd.h>
#include <ppltasks.h>
#include <winrt/Windows.ApplicationModel.DataTransfer.h>
#include <winrt/Windows.Foundation.Metadata.h>
#include <winrt/Windows.Graphics.Display.h>
#include <winrt/Windows.Graphics.Display.Core.h>

using namespace winrt::Windows::Foundation::Metadata;
using namespace winrt::Windows::Graphics::Display;
using namespace winrt::Windows::Graphics::Display::Core;
using namespace winrt::Windows::UI::Core;
using namespace winrt::Windows::UI::ViewManagement;

CWinSystemWin10::CWinSystemWin10()
  : CWinSystemBase()
  , m_nPrimary(0)
  , m_ValidWindowedPosition(false)
  , m_IsAlteringWindow(false)
  , m_delayDispReset(false)
  , m_state(WINDOW_STATE_WINDOWED)
  , m_fullscreenState(WINDOW_FULLSCREEN_STATE_FULLSCREEN_WINDOW)
  , m_windowState(WINDOW_WINDOW_STATE_WINDOWED)
  , m_inFocus(false)
  , m_bMinimized(false)
{
  m_winEvents.reset(new CWinEventsWin10());

  AE::CAESinkFactory::ClearSinks();
  CAESinkXAudio::Register();
  if (CSysInfo::GetWindowsDeviceFamily() == CSysInfo::WindowsDeviceFamily::Desktop)
  {
    CAESinkWASAPI::Register();
  }
  CPowerSyscall::Register();
}

CWinSystemWin10::~CWinSystemWin10()
{
};

bool CWinSystemWin10::InitWindowSystem()
{
  m_coreWindow = winrt::Windows::UI::Core::CoreWindow::GetForCurrentThread();
  dynamic_cast<CWinEventsWin10&>(*m_winEvents).InitEventHandlers(m_coreWindow);

  if (!CWinSystemBase::InitWindowSystem())
    return false;

  if (m_MonitorsInfo.empty())
  {
    CLog::Log(LOGERROR, "%s - no suitable monitor found, aborting...", __FUNCTION__);
    return false;
  }

  return true;
}

bool CWinSystemWin10::DestroyWindowSystem()
{
  m_bWindowCreated = false;
  RestoreDesktopResolution(m_nScreen);
  return true;
}

bool CWinSystemWin10::CanDoWindowed()
{
  return CSysInfo::GetWindowsDeviceFamily() == CSysInfo::Desktop;
}

bool CWinSystemWin10::CreateNewWindow(const std::string& name, bool fullScreen, RESOLUTION_INFO& res)
{
  UpdateStates(fullScreen);
  // initialize the state
  WINDOW_STATE state = GetState(fullScreen);

  m_nWidth = res.iWidth;
  m_nHeight = res.iHeight;
  m_bFullScreen = fullScreen;
  m_nScreen = res.iScreen;
  m_fRefreshRate = res.fRefreshRate;
  m_inFocus = true;
  m_bWindowCreated = true;
  m_state = state;

  m_coreWindow.Activate();

  AdjustWindow();
  // dispatch all events currently pending in the queue to show window's content
  // and hide UWP splash, without this the Kodi's splash will not be shown
  m_coreWindow.Dispatcher().ProcessEvents(winrt::Windows::UI::Core::CoreProcessEventsOption::ProcessOneAndAllPending);

  // in some cases CoreWindow::SizeChanged isn't fired
  // it causes mismatch between window actual size and UI
  winrt::Rect winRect = m_coreWindow.Bounds();
  dynamic_cast<CWinEventsWin10&>(*m_winEvents).OnResize(winRect.Width, winRect.Height);

  return true;
}

bool CWinSystemWin10::CenterWindow()
{
  RESOLUTION_INFO DesktopRes = CDisplaySettings::GetInstance().GetResolutionInfo(RES_DESKTOP);

  m_nLeft = (DesktopRes.iWidth / 2) - (m_nWidth / 2);
  m_nTop = (DesktopRes.iHeight / 2) - (m_nHeight / 2);

  RECT rc;
  rc.left = m_nLeft;
  rc.top = m_nTop;
  rc.right = rc.left + m_nWidth;
  rc.bottom = rc.top + m_nHeight;

  // @todo center the window

  return true;
}

bool CWinSystemWin10::ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop)
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

void CWinSystemWin10::FinishWindowResize(int newWidth, int newHeight)
{
  m_nWidth = newWidth;
  m_nHeight = newHeight;

  float dpi = DX::DeviceResources::Get()->GetDpi();
  int dipsWidth = round(DX::ConvertPixelsToDips(m_nWidth, dpi));
  int dipsHeight = round(DX::ConvertPixelsToDips(m_nHeight, dpi));

  ApplicationView::PreferredLaunchViewSize(winrt::Windows::Foundation::Size(dipsWidth, dipsHeight));
  ApplicationView::PreferredLaunchWindowingMode(ApplicationViewWindowingMode::PreferredLaunchViewSize);
}

void CWinSystemWin10::AdjustWindow(bool forceResize)
{
  CLog::Log(LOGDEBUG, __FUNCTION__": adjusting window if required.");

  auto appView = ApplicationView::GetForCurrentView();
  bool isInFullscreen = appView.IsFullScreenMode();

  if (m_state == WINDOW_STATE_FULLSCREEN_WINDOW || m_state == WINDOW_STATE_FULLSCREEN)
  {
    if (!isInFullscreen)
    {
      if (appView.TryEnterFullScreenMode())
        ApplicationView::PreferredLaunchWindowingMode(ApplicationViewWindowingMode::FullScreen);
    }
  }
  else // m_state == WINDOW_STATE_WINDOWED
  {
    if (isInFullscreen)
    {
      appView.ExitFullScreenMode();
    }

    int viewWidth = appView.VisibleBounds().Width;
    int viewHeight = appView.VisibleBounds().Height;

    float dpi = DX::DeviceResources::Get()->GetDpi();
    int dipsWidth = round(DX::ConvertPixelsToDips(m_nWidth, dpi));
    int dipsHeight = round(DX::ConvertPixelsToDips(m_nHeight, dpi));

    if (viewHeight != dipsHeight || viewWidth != dipsWidth)
    {
      if (!appView.TryResizeView(winrt::Windows::Foundation::Size(dipsWidth, dipsHeight)))
      {
        CLog::LogF(LOGDEBUG, __FUNCTION__, "resizing ApplicationView failed.");
      }
    }

    ApplicationView::PreferredLaunchViewSize(winrt::Windows::Foundation::Size(dipsWidth, dipsHeight));
    ApplicationView::PreferredLaunchWindowingMode(ApplicationViewWindowingMode::PreferredLaunchViewSize);
  }
}

void CWinSystemWin10::CenterCursor() const
{
}

bool CWinSystemWin10::SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays)
{
  CWinSystemWin10::UpdateStates(fullScreen);
  WINDOW_STATE state = GetState(fullScreen);

  CLog::Log(LOGDEBUG, "%s (%s) on screen %d with size %dx%d, refresh %f%s", __FUNCTION__, window_state_names[state]
    , res.iScreen, res.iWidth, res.iHeight, res.fRefreshRate, (res.dwFlags & D3DPRESENTFLAG_INTERLACED) ? "i" : "");

  bool forceChange = false;    // resolution/display is changed but window state isn't changed
  bool changeScreen = false;   // display is changed
  bool stereoChange = IsStereoEnabled() != (CServiceBroker::GetWinSystem()->GetGfxContext().GetStereoMode() == RENDER_STEREO_MODE_HARDWAREBASED);

  if ( m_nWidth != res.iWidth
    || m_nHeight != res.iHeight
    || m_fRefreshRate != res.fRefreshRate
    || m_nScreen != res.iScreen
    || stereoChange)
  {
    if (m_nScreen != res.iScreen)
      changeScreen = true;

    forceChange = true;
  }

  if (state == m_state && !forceChange)
    return true;

  // entering to stereo mode, limit resolution to 1080p@23.976
  if (stereoChange && !IsStereoEnabled() && res.iWidth > 1280)
  {
    res = CDisplaySettings::GetInstance().GetResolutionInfo(CResolutionUtils::ChooseBestResolution(24.f / 1.001f, 1920, true));
  }

  if (m_state == WINDOW_STATE_WINDOWED)
  {
    // @todo get window size
    //if (GetWindowInfo(m_hWnd, &wi))
    {
      //m_nLeft = wi.rcClient.left;
      //m_nTop = wi.rcClient.top;
      m_ValidWindowedPosition = true;
    }
  }

  m_IsAlteringWindow = true;
  ReleaseBackBuffer();

  if (changeScreen)
  {
    // before we changing display we have to leave exclusive mode on "old" display
    //if (m_state == WINDOW_STATE_FULLSCREEN)
    //  SetDeviceFullScreen(false, res);

    // restoring native resolution on "old" display
    RestoreDesktopResolution(m_nScreen);
  }

  m_bFullScreen = fullScreen;
  m_nScreen = res.iScreen;
  m_nWidth = res.iWidth;
  m_nHeight = res.iHeight;
  m_bBlankOtherDisplay = blankOtherDisplays;
  m_fRefreshRate = res.fRefreshRate;

  if (state == WINDOW_STATE_FULLSCREEN)
  {
    // isn't allowed in UWP
  }
  else if (m_state == WINDOW_STATE_FULLSCREEN || m_state == WINDOW_STATE_FULLSCREEN_WINDOW) // we're in fullscreen state now
  {
    // guess we are leaving exclusive mode, this will not an effect if we already not in
    //SetDeviceFullScreen(false, res);

    if (state == WINDOW_STATE_WINDOWED) // go to a windowed state
    {
      // need to restore resoultion if it was changed to not native
      // because we do not support resolution change in windowed mode
      if (changeScreen)
        RestoreDesktopResolution(m_nScreen);
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
  m_IsAlteringWindow = false;
  return true;
}

bool CWinSystemWin10::DPIChanged(WORD dpi, RECT windowRect) const
{
  (void)dpi;
  return true;
}

void CWinSystemWin10::RestoreDesktopResolution(int screen)
{
  CLog::Log(LOGDEBUG, __FUNCTION__": restoring desktop resolution for screen %i, ", screen);
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

const MONITOR_DETAILS* CWinSystemWin10::GetMonitor(int screen) const
{
  for (unsigned int monitor = 0; monitor < m_MonitorsInfo.size(); monitor++)
    if (m_MonitorsInfo[monitor].ScreenNumber == screen)
      return &m_MonitorsInfo[monitor];

  // What to do if monitor is not found? Not sure... use the primary screen as a default value.
  if (m_nPrimary >= 0 && static_cast<size_t>(m_nPrimary) < m_MonitorsInfo.size())
  {
    CLog::Log(LOGDEBUG, __FUNCTION__, "no monitor found for screen %i, "
      "will use primary screen %i", screen, m_nPrimary);
    return &m_MonitorsInfo[m_nPrimary];
  }
  else
  {
    CLog::LogFunction(LOGERROR, __FUNCTION__, "no monitor found for screen %i", screen);
    return nullptr;
  }
}

int CWinSystemWin10::GetCurrentScreen()
{
  CLog::Log(LOGDEBUG, "%s is not implemented", __FUNCTION__);
  // fallback to default
  return 0;
}

RECT CWinSystemWin10::ScreenRect(int screen) const
{
  const MONITOR_DETAILS* details = GetMonitor(screen);

  if (!details)
  {
    CLog::LogFunction(LOGERROR, __FUNCTION__, "no monitor found for screen %i", screen);
  }

  RECT rc = { 0, 0, details->ScreenWidth, details->ScreenHeight };
  return rc;
}

bool CWinSystemWin10::ChangeResolution(const RESOLUTION_INFO& res, bool forceChange /*= false*/)
{
  const MONITOR_DETAILS* details = GetMonitor(res.iScreen);

  if (!details)
    return false;

  if (ApiInformation::IsTypePresent(L"Windows.Graphics.Display.Core.HdmiDisplayInformation"))
  {
    bool changed = false;
    auto hdmiInfo = HdmiDisplayInformation::GetForCurrentView();
    if (hdmiInfo != nullptr)
    {
      // default mode not in list of supported display modes
      if (res.iScreenWidth == details->ScreenWidth && res.iScreenHeight == details->ScreenHeight
        && fabs(res.fRefreshRate - details->RefreshRate) <= 0.00001)
      {
        Wait(hdmiInfo.SetDefaultDisplayModeAsync());
        changed = true;
      }
      else
      {
        bool needStereo = CServiceBroker::GetWinSystem()->GetGfxContext().GetStereoMode() == RENDER_STEREO_MODE_HARDWAREBASED;
        auto hdmiModes = hdmiInfo.GetSupportedDisplayModes();

        HdmiDisplayMode selected = nullptr;
        for (auto& mode : hdmiModes)
        {
          if (res.iScreenWidth == mode.ResolutionWidthInRawPixels() && res.iScreenHeight == mode.ResolutionHeightInRawPixels()
            && fabs(res.fRefreshRate - mode.RefreshRate()) <= 0.00001)
          {
            selected = mode;
            if (needStereo == mode.StereoEnabled())
              break;
          }
        }

        if (selected != nullptr)
        {
          changed = Wait(hdmiInfo.RequestSetCurrentDisplayModeAsync(selected));
        }
      }
    }

    // changing display mode doesn't fire CoreWindow::SizeChanged event
    if (changed && m_bWindowCreated)
    {
      float dpi = DisplayInformation::GetForCurrentView().LogicalDpi();
      float dipsW = DX::ConvertPixelsToDips(m_nWidth, dpi);
      float dipsH = DX::ConvertPixelsToDips(m_nHeight, dpi);

      dynamic_cast<CWinEventsWin10&>(*m_winEvents).OnResize(dipsW, dipsH);
    }
    return changed;
  }

  CLog::LogFunction(LOGDEBUG, __FUNCTION__, "Not supported.");
  return false;
}

void CWinSystemWin10::UpdateResolutions()
{
  m_MonitorsInfo.clear();
  CWinSystemBase::UpdateResolutions();

  UpdateResolutionsInternal();

  if (m_MonitorsInfo.empty())
    return;

  float refreshRate = 0;
  int w = 0;
  int h = 0;
  uint32_t dwFlags;

  // Primary
  m_MonitorsInfo[m_nPrimary].ScreenNumber = 0;
  w = m_MonitorsInfo[m_nPrimary].ScreenWidth;
  h = m_MonitorsInfo[m_nPrimary].ScreenHeight;
  if ((m_MonitorsInfo[m_nPrimary].RefreshRate == 59) || (m_MonitorsInfo[m_nPrimary].RefreshRate == 29) || (m_MonitorsInfo[m_nPrimary].RefreshRate == 23))
    refreshRate = (float)(m_MonitorsInfo[m_nPrimary].RefreshRate + 1) / 1.001f;
  else
    refreshRate = (float)m_MonitorsInfo[m_nPrimary].RefreshRate;
  dwFlags = m_MonitorsInfo[m_nPrimary].Interlaced ? D3DPRESENTFLAG_INTERLACED : 0;

  UpdateDesktopResolution(CDisplaySettings::GetInstance().GetResolutionInfo(RES_DESKTOP), 0, w, h, refreshRate, dwFlags);
  CLog::Log(LOGNOTICE, "Primary mode: %s", CDisplaySettings::GetInstance().GetResolutionInfo(RES_DESKTOP).strMode.c_str());

  if (ApiInformation::IsTypePresent(L"Windows.Graphics.Display.Core.HdmiDisplayInformation"))
  {
    auto hdmiInfo = HdmiDisplayInformation::GetForCurrentView();
    if (hdmiInfo != nullptr)
    {
      auto hdmiModes = hdmiInfo.GetSupportedDisplayModes();
      for (auto& mode : hdmiModes)
      {
        RESOLUTION_INFO res;
        UpdateDesktopResolution(res, 0, mode.ResolutionWidthInRawPixels(), mode.ResolutionHeightInRawPixels(), mode.RefreshRate(), 0);
        AddResolution(res);
        CLog::Log(LOGNOTICE, "Additional mode: %s %s", res.strMode.c_str(), mode.Is2086MetadataSupported() ? "(HDR)" : "");
      }
    }
  }

  // Desktop resolution of the other screens
  if (m_MonitorsInfo.size() >= 2)
  {
    int xbmcmonitor = 1;  // The screen number+1 showed in the GUI display settings

    for (unsigned int monitor = 0; monitor < m_MonitorsInfo.size(); monitor++)
    {
      if (monitor != m_nPrimary)
      {
        m_MonitorsInfo[monitor].ScreenNumber = xbmcmonitor;
        w = m_MonitorsInfo[monitor].ScreenWidth;
        h = m_MonitorsInfo[monitor].ScreenHeight;
        if ((m_MonitorsInfo[monitor].RefreshRate == 59) || (m_MonitorsInfo[monitor].RefreshRate == 29) || (m_MonitorsInfo[monitor].RefreshRate == 23))
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
}

void CWinSystemWin10::AddResolution(const RESOLUTION_INFO &res)
{
  for (unsigned int i = 0; i < CDisplaySettings::GetInstance().ResolutionInfoSize(); i++)
  {
    if (CDisplaySettings::GetInstance().GetResolutionInfo(i).iScreen == res.iScreen &&
      CDisplaySettings::GetInstance().GetResolutionInfo(i).iWidth == res.iWidth &&
      CDisplaySettings::GetInstance().GetResolutionInfo(i).iHeight == res.iHeight &&
      CDisplaySettings::GetInstance().GetResolutionInfo(i).iScreenWidth == res.iScreenWidth &&
      CDisplaySettings::GetInstance().GetResolutionInfo(i).iScreenHeight == res.iScreenHeight &&
      CDisplaySettings::GetInstance().GetResolutionInfo(i).fRefreshRate == res.fRefreshRate &&
      CDisplaySettings::GetInstance().GetResolutionInfo(i).dwFlags == res.dwFlags)
      return; // already have this resolution
  }

  CDisplaySettings::GetInstance().AddResolutionInfo(res);
}

bool CWinSystemWin10::UpdateResolutionsInternal()
{
  CLog::Log(LOGNOTICE, "Win10 UWP Found screen. Need to update code to use DirectX!");

  auto dispatcher = m_coreWindow.Dispatcher();
  winrt::Windows::UI::Core::DispatchedHandler handler([this]()
  {
    MONITOR_DETAILS md = {};

    auto displayInfo = DisplayInformation::GetForCurrentView();
    bool flipResolution = false;
    switch (displayInfo.NativeOrientation())
    {
    case DisplayOrientations::Landscape:
      switch (displayInfo.CurrentOrientation())
      {
      case DisplayOrientations::Portrait:
      case DisplayOrientations::PortraitFlipped:
        flipResolution = true;
        break;
      }
      break;
    case DisplayOrientations::Portrait:
      switch (displayInfo.CurrentOrientation())
      {
      case DisplayOrientations::Landscape:
      case DisplayOrientations::LandscapeFlipped:
        flipResolution = true;
        break;
      }
      break;
    }
    md.ScreenWidth = flipResolution ? displayInfo.ScreenHeightInRawPixels() : displayInfo.ScreenWidthInRawPixels();
    md.ScreenHeight = flipResolution ? displayInfo.ScreenWidthInRawPixels() : displayInfo.ScreenHeightInRawPixels();

    if (ApiInformation::IsTypePresent(L"Windows.Graphics.Display.Core.HdmiDisplayInformation"))
    {
      auto hdmiInfo = HdmiDisplayInformation::GetForCurrentView();
      if (hdmiInfo != nullptr)
      {
        auto currentMode = hdmiInfo.GetCurrentDisplayMode();
        md.RefreshRate = currentMode.RefreshRate();
        md.Bpp = currentMode.BitsPerPixel();
      }
      else
      {
        md.RefreshRate = 60.0;
        md.Bpp = 24;
      }
    }
    else
    {
      // note that refresh rate information is not available on Win10 UWP
      md.RefreshRate = 60.0;
      md.Bpp = 24;
    }
    md.Interlaced = false;

    m_MonitorsInfo.push_back(md);
  });

  if (dispatcher.HasThreadAccess())
    handler();
  else
    Wait(dispatcher.RunAsync(CoreDispatcherPriority::High, handler));

  return true;
}

void CWinSystemWin10::ShowOSMouse(bool show)
{
  if (!m_coreWindow)
    return;

  winrt::Windows::UI::Core::DispatchedHandler handler([this, show]()
  {
    CoreCursor cursor = nullptr;
    if (show)
      CoreCursor cursor(CoreCursorType::Arrow, 1);
    m_coreWindow.PointerCursor(cursor);
  });

  if (m_coreWindow.Dispatcher().HasThreadAccess())
    handler();
  else
    m_coreWindow.Dispatcher().RunAsync(CoreDispatcherPriority::Normal, handler);
}

bool CWinSystemWin10::Minimize()
{
  CLog::Log(LOGDEBUG, "%s is not implemented", __FUNCTION__);
  return true;
}
bool CWinSystemWin10::Restore()
{
  CLog::Log(LOGDEBUG, "%s is not implemented", __FUNCTION__);
  return true;
}
bool CWinSystemWin10::Hide()
{
  CLog::Log(LOGDEBUG, "%s is not implemented", __FUNCTION__);
  return true;
}
bool CWinSystemWin10::Show(bool raise)
{
  CLog::Log(LOGDEBUG, "%s is not implemented", __FUNCTION__);
  return true;
}

void CWinSystemWin10::Register(IDispResource *resource)
{
  CSingleLock lock(m_resourceSection);
  m_resources.push_back(resource);
}

void CWinSystemWin10::Unregister(IDispResource* resource)
{
  CSingleLock lock(m_resourceSection);
  std::vector<IDispResource*>::iterator i = find(m_resources.begin(), m_resources.end(), resource);
  if (i != m_resources.end())
    m_resources.erase(i);
}

void CWinSystemWin10::OnDisplayLost()
{
  CLog::Log(LOGDEBUG, "%s - notify display lost event", __FUNCTION__);

  {
    CSingleLock lock(m_resourceSection);
    for (std::vector<IDispResource *>::iterator i = m_resources.begin(); i != m_resources.end(); ++i)
      (*i)->OnLostDisplay();
  }
}

void CWinSystemWin10::OnDisplayReset()
{
  if (!m_delayDispReset)
  {
    CLog::Log(LOGDEBUG, "%s - notify display reset event", __FUNCTION__);
    CSingleLock lock(m_resourceSection);
    for (std::vector<IDispResource *>::iterator i = m_resources.begin(); i != m_resources.end(); ++i)
      (*i)->OnResetDisplay();
  }
}

void CWinSystemWin10::OnDisplayBack()
{
  int delay = CServiceBroker::GetSettings().GetInt("videoscreen.delayrefreshchange");
  if (delay > 0)
  {
    m_delayDispReset = true;
    m_dispResetTimer.Set(delay * 100);
  }
  OnDisplayReset();
}

void CWinSystemWin10::ResolutionChanged()
{
  OnDisplayLost();
  OnDisplayBack();
}

std::unique_ptr<CVideoSync> CWinSystemWin10::GetVideoSync(void *clock)
{
  std::unique_ptr<CVideoSync> pVSync(new CVideoSyncD3D(clock));
  return pVSync;
}

std::string CWinSystemWin10::GetClipboardText()
{
  std::wstring unicode_text;
  std::string utf8_text;

  auto contentView = winrt::Windows::ApplicationModel::DataTransfer::Clipboard::GetContent();
  if (contentView.Contains(winrt::Windows::ApplicationModel::DataTransfer::StandardDataFormats::Text()))
  {
    auto text = Wait(contentView.GetTextAsync());
    unicode_text.append(text.c_str());
  }

  return KODI::PLATFORM::WINDOWS::FromW(unicode_text);
}

bool CWinSystemWin10::UseLimitedColor()
{
  return CServiceBroker::GetSettings().GetBool(CSettings::SETTING_VIDEOSCREEN_LIMITEDRANGE);
}

void CWinSystemWin10::NotifyAppFocusChange(bool bGaining)
{
  m_inFocus = bGaining;
}

void CWinSystemWin10::UpdateStates(bool fullScreen)
{
  m_fullscreenState = WINDOW_FULLSCREEN_STATE_FULLSCREEN_WINDOW; // currently only this allowed
  m_windowState = WINDOW_WINDOW_STATE_WINDOWED; // currently only this allowed
}

WINDOW_STATE CWinSystemWin10::GetState(bool fullScreen) const
{
  return static_cast<WINDOW_STATE>(fullScreen ? m_fullscreenState : m_windowState);
}

bool CWinSystemWin10::MessagePump()
{
  return m_winEvents->MessagePump();
}

#pragma pack(pop)
