/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WinSystemWin10.h"

#include "ServiceBroker.h"
#include "WIN32Util.h"
#include "WinEventsWin10.h"
#include "application/Application.h"
#include "cores/AudioEngine/AESinkFactory.h"
#include "cores/AudioEngine/Sinks/AESinkWASAPI.h"
#include "cores/AudioEngine/Sinks/AESinkXAudio.h"
#include "rendering/dx/DirectXHelper.h"
#include "rendering/dx/RenderContext.h"
#include "rendering/dx/ScreenshotSurfaceWindows.h"
#include "settings/DisplaySettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/SystemInfo.h"
#include "utils/log.h"
#include "windowing/GraphicContext.h"
#include "windowing/windows/VideoSyncD3D.h"

#include "platform/win10/AsyncHelpers.h"
#include "platform/win32/CharsetConverter.h"

#include <cmath>
#include <mutex>

#pragma pack(push,8)

#include <tpcshrd.h>
#include <ppltasks.h>
#include <winrt/Windows.ApplicationModel.DataTransfer.h>
#include <winrt/Windows.Foundation.Metadata.h>
#include <winrt/Windows.Graphics.Display.h>
#include <winrt/Windows.Graphics.Display.Core.h>

using namespace winrt::Windows::ApplicationModel::DataTransfer;
using namespace winrt::Windows::Foundation::Metadata;
using namespace winrt::Windows::Graphics::Display;
using namespace winrt::Windows::Graphics::Display::Core;
using namespace winrt::Windows::UI::Core;
using namespace winrt::Windows::UI::ViewManagement;

using namespace std::chrono_literals;

CWinSystemWin10::CWinSystemWin10()
  : CWinSystemBase()
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
  CAESinkWASAPI::Register();
  CScreenshotSurfaceWindows::Register();
}

CWinSystemWin10::~CWinSystemWin10()
{
};

bool CWinSystemWin10::InitWindowSystem()
{
  m_coreWindow = CoreWindow::GetForCurrentThread();
  dynamic_cast<CWinEventsWin10&>(*m_winEvents).InitEventHandlers(m_coreWindow);

  if (!CWinSystemBase::InitWindowSystem())
    return false;

  if (m_displays.empty())
  {
    CLog::Log(LOGERROR, "{} - no suitable monitor found, aborting...", __FUNCTION__);
    return false;
  }

  return true;
}

bool CWinSystemWin10::DestroyWindowSystem()
{
  m_bWindowCreated = false;
  RestoreDesktopResolution();
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
  m_fRefreshRate = res.fRefreshRate;
  m_inFocus = true;
  m_bWindowCreated = true;
  m_state = state;

  m_coreWindow.Activate();

  AdjustWindow();
  // dispatch all events currently pending in the queue to show window's content
  // and hide UWP splash, without this the Kodi's splash will not be shown
  m_coreWindow.Dispatcher().ProcessEvents(CoreProcessEventsOption::ProcessOneAndAllPending);

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

void CWinSystemWin10::ForceFullScreen(const RESOLUTION_INFO& resInfo)
{
  ResizeWindow(resInfo.iScreenWidth, resInfo.iScreenHeight, 0, 0);
}

void CWinSystemWin10::AdjustWindow()
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

bool CWinSystemWin10::SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays)
{
  CWinSystemWin10::UpdateStates(fullScreen);
  WINDOW_STATE state = GetState(fullScreen);

  CLog::Log(LOGDEBUG, "{} ({}) with size {}x{}, refresh {:f}{}", __FUNCTION__,
            window_state_names[state], res.iWidth, res.iHeight, res.fRefreshRate,
            (res.dwFlags & D3DPRESENTFLAG_INTERLACED) ? "i" : "");

  bool forceChange = false;    // resolution/display is changed but window state isn't changed
  bool stereoChange = IsStereoEnabled() != (CServiceBroker::GetWinSystem()->GetGfxContext().GetStereoMode() == RENDER_STEREO_MODE_HARDWAREBASED);

  if ( m_nWidth != res.iWidth || m_nHeight != res.iHeight || m_fRefreshRate != res.fRefreshRate ||
    stereoChange || m_bFirstResChange)
  {
    forceChange = true;
  }

  if (state == m_state && !forceChange)
    return true;

  // entering to stereo mode, limit resolution to 1080p@23.976
  if (stereoChange && !IsStereoEnabled() && res.iWidth > 1280)
  {
    res = CDisplaySettings::GetInstance().GetResolutionInfo(CResolutionUtils::ChooseBestResolution(24.f / 1.001f, 1920, 1080, true));
  }

  if (m_state == WINDOW_STATE_WINDOWED)
  {
    if (m_coreWindow)
    {
      m_nLeft = m_coreWindow.Bounds().X;
      m_nTop = m_coreWindow.Bounds().Y;
      m_ValidWindowedPosition = true;
    }
  }

  m_IsAlteringWindow = true;
  ReleaseBackBuffer();

  m_bFirstResChange = false;
  m_bFullScreen = fullScreen;
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
    if (state == WINDOW_STATE_WINDOWED) // go to a windowed state
    {
      // need to restore resolution if it was changed to not native
      // because we do not support resolution change in windowed mode
      RestoreDesktopResolution();
    }
    else if (state == WINDOW_STATE_FULLSCREEN_WINDOW) // enter fullscreen window instead
    {
      ChangeResolution(res, stereoChange);
    }

    m_state = state;
    AdjustWindow();
  }
  else // we're in windowed state now
  {
    if (state == WINDOW_STATE_FULLSCREEN_WINDOW)
    {
      ChangeResolution(res, stereoChange);

      m_state = state;
      AdjustWindow();
    }
  }

  CreateBackBuffer();
  m_IsAlteringWindow = false;
  return true;
}

bool CWinSystemWin10::DPIChanged(WORD dpi, RECT windowRect) const
{
  (void)dpi;
  return true;
}

void CWinSystemWin10::RestoreDesktopResolution()
{
  CLog::Log(LOGDEBUG, __FUNCTION__": restoring default desktop resolution");
  ChangeResolution(CDisplaySettings::GetInstance().GetResolutionInfo(RES_DESKTOP));
}

const MONITOR_DETAILS* CWinSystemWin10::GetDefaultMonitor() const
{
  if (m_displays.empty())
    return nullptr;

  return &m_displays.front();
}

bool CWinSystemWin10::ChangeResolution(const RESOLUTION_INFO& res, bool forceChange /*= false*/)
{
  const MONITOR_DETAILS* details = GetDefaultMonitor();

  if (!details)
    return false;

  if (ApiInformation::IsTypePresent(L"Windows.Graphics.Display.Core.HdmiDisplayInformation"))
  {
    bool changed = false;
    auto hdmiInfo = HdmiDisplayInformation::GetForCurrentView();
    const bool needHDR = DX::DeviceResources::Get()->IsHDROutput();

    if (hdmiInfo != nullptr)
    {
      // default mode not in list of supported display modes
      // TO DO: is still necessary? (or now all modes are listed?)
      if (!needHDR && res.iScreenWidth == details->ScreenWidth &&
          res.iScreenHeight == details->ScreenHeight &&
          fabs(res.fRefreshRate - details->RefreshRate) <= 0.00001)
      {
        Wait(hdmiInfo.SetDefaultDisplayModeAsync());
        changed = true;
      }
      else
      {
        bool needStereo = CServiceBroker::GetWinSystem()->GetGfxContext().GetStereoMode() == RENDER_STEREO_MODE_HARDWAREBASED;
        auto hdmiModes = hdmiInfo.GetSupportedDisplayModes();

        // For backward compatibility (also old Xbox models) only match color space for HDR modes
        // and keep SDR modes selection as it is (any color space). Assumes SDR modes listed first.
        // TO DO: for HDR modes make use of IsSmpte2084Supported() but has issues with current code.
        // TO DO: for SDR implement preference for BT.709 color space but also fallback to sRGB.
        HdmiDisplayMode selected = nullptr;
        for (const auto& mode : hdmiModes)
        {
          if ((!needHDR || (needHDR && mode.ColorSpace() == HdmiDisplayColorSpace::BT2020)) &&
              res.iScreenWidth == mode.ResolutionWidthInRawPixels() &&
              res.iScreenHeight == mode.ResolutionHeightInRawPixels() &&
              fabs(res.fRefreshRate - mode.RefreshRate()) <= 0.00001)
          {
            selected = mode;
            if (needStereo == mode.StereoEnabled())
              break;
          }
        }

        if (selected != nullptr)
        {
          changed = Wait(hdmiInfo.RequestSetCurrentDisplayModeAsync(
              selected, needHDR ? HdmiDisplayHdrOption::Eotf2084 : HdmiDisplayHdrOption::None));
        }
      }
    }

    // changing display mode doesn't fire CoreWindow::SizeChanged event
    if (changed && m_bWindowCreated)
    {
      // dispatch all events currently pending in the queue to change window's content
      m_coreWindow.Dispatcher().ProcessEvents(CoreProcessEventsOption::ProcessOneAndAllPending);

      float dpi = DisplayInformation::GetForCurrentView().LogicalDpi();
      float dipsW = DX::ConvertPixelsToDips(m_nWidth, dpi);
      float dipsH = DX::ConvertPixelsToDips(m_nHeight, dpi);

      dynamic_cast<CWinEventsWin10&>(*m_winEvents).OnResize(dipsW, dipsH);
    }
    return changed;
  }

  CLog::LogF(LOGDEBUG, "Not supported.");
  return false;
}

void CWinSystemWin10::UpdateResolutions()
{
  m_displays.clear();

  CWinSystemBase::UpdateResolutions();
  GetConnectedDisplays(m_displays);

  const MONITOR_DETAILS* details = GetDefaultMonitor();
  if (!details)
    return;

  float refreshRate;
  int w = details->ScreenWidth;
  int h = details->ScreenHeight;
  uint32_t dwFlags = details->Interlaced ? D3DPRESENTFLAG_INTERLACED : 0;;

  if (details->RefreshRate == 59 || details->RefreshRate == 29 || details->RefreshRate == 23)
    refreshRate = static_cast<float>(details->RefreshRate + 1) / 1.001f;
  else
    refreshRate = static_cast<float>(details->RefreshRate);

  RESOLUTION_INFO& primary_info = CDisplaySettings::GetInstance().GetResolutionInfo(RES_DESKTOP);
  UpdateDesktopResolution(primary_info, "Default", w, h, refreshRate, dwFlags);
  CLog::Log(LOGINFO, "Primary mode: {}", primary_info.strMode);

  // erase previous stored modes
  CDisplaySettings::GetInstance().ClearCustomResolutions();

  if (ApiInformation::IsTypePresent(L"Windows.Graphics.Display.Core.HdmiDisplayInformation"))
  {
    auto hdmiInfo = HdmiDisplayInformation::GetForCurrentView();
    if (hdmiInfo != nullptr)
    {
      auto hdmiModes = hdmiInfo.GetSupportedDisplayModes();
      for (const auto& mode : hdmiModes)
      {
        RESOLUTION_INFO res;
        res.iWidth = mode.ResolutionWidthInRawPixels();
        res.iHeight = mode.ResolutionHeightInRawPixels();
        res.bFullScreen = true;
        res.dwFlags = 0;
        res.fRefreshRate = mode.RefreshRate();
        res.fPixelRatio = 1.0f;
        res.iScreenWidth = res.iWidth;
        res.iScreenHeight = res.iHeight;
        res.iSubtitles = res.iHeight;
        res.strMode = StringUtils::Format("Default: {}x{} @ {:.2f}Hz", res.iWidth, res.iHeight,
                                          res.fRefreshRate);
        GetGfxContext().ResetOverscan(res);

        if (AddResolution(res))
          CLog::Log(LOGINFO, "Additional mode: {} {}", res.strMode,
                    mode.Is2086MetadataSupported() ? "(HDR)" : "");
      }
    }
  }

  CDisplaySettings::GetInstance().ApplyCalibrations();
}

bool CWinSystemWin10::AddResolution(const RESOLUTION_INFO &res)
{
  for (unsigned int i = RES_CUSTOM; i < CDisplaySettings::GetInstance().ResolutionInfoSize(); i++)
  {
    RESOLUTION_INFO& info = CDisplaySettings::GetInstance().GetResolutionInfo(i);
    if ( info.iWidth == res.iWidth
      && info.iHeight == res.iHeight
      && info.iScreenWidth == res.iScreenWidth
      && info.iScreenHeight == res.iScreenHeight
      && info.fRefreshRate == res.fRefreshRate
      && info.dwFlags == res.dwFlags)
      return false; // already have this resolution
  }

  CDisplaySettings::GetInstance().AddResolutionInfo(res);
  return true;
}

void CWinSystemWin10::GetConnectedDisplays(std::vector<MONITOR_DETAILS>& outputs)
{
  auto dispatcher = m_coreWindow.Dispatcher();
  DispatchedHandler handler([&]()
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
        // On Xbox, 4K resolutions only are reported by HdmiDisplayInformation API
        // so ScreenHeight & ScreenWidth are updated with info provided here
        md.ScreenHeight = currentMode.ResolutionHeightInRawPixels();
        md.ScreenWidth = currentMode.ResolutionWidthInRawPixels();
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

    outputs.push_back(md);
  });

  if (dispatcher.HasThreadAccess())
    handler();
  else
    Wait(dispatcher.RunAsync(CoreDispatcherPriority::High, handler));
}

void CWinSystemWin10::ShowOSMouse(bool show)
{
  if (!m_coreWindow)
    return;

  DispatchedHandler handler([this, show]()
  {
    CoreCursor cursor = nullptr;
    if (show)
      cursor = CoreCursor(CoreCursorType::Arrow, 1);
    m_coreWindow.PointerCursor(cursor);
  });

  if (m_coreWindow.Dispatcher().HasThreadAccess())
    handler();
  else
    m_coreWindow.Dispatcher().RunAsync(CoreDispatcherPriority::Normal, handler);
}

bool CWinSystemWin10::Minimize()
{
  CLog::Log(LOGDEBUG, "{} is not implemented", __FUNCTION__);
  return true;
}
bool CWinSystemWin10::Restore()
{
  CLog::Log(LOGDEBUG, "{} is not implemented", __FUNCTION__);
  return true;
}
bool CWinSystemWin10::Hide()
{
  CLog::Log(LOGDEBUG, "{} is not implemented", __FUNCTION__);
  return true;
}
bool CWinSystemWin10::Show(bool raise)
{
  CLog::Log(LOGDEBUG, "{} is not implemented", __FUNCTION__);
  return true;
}

void CWinSystemWin10::Register(IDispResource *resource)
{
  std::unique_lock<CCriticalSection> lock(m_resourceSection);
  m_resources.push_back(resource);
}

void CWinSystemWin10::Unregister(IDispResource* resource)
{
  std::unique_lock<CCriticalSection> lock(m_resourceSection);
  std::vector<IDispResource*>::iterator i = find(m_resources.begin(), m_resources.end(), resource);
  if (i != m_resources.end())
    m_resources.erase(i);
}

void CWinSystemWin10::OnDisplayLost()
{
  CLog::Log(LOGDEBUG, "{} - notify display lost event", __FUNCTION__);

  {
    std::unique_lock<CCriticalSection> lock(m_resourceSection);
    for (std::vector<IDispResource *>::iterator i = m_resources.begin(); i != m_resources.end(); ++i)
      (*i)->OnLostDisplay();
  }
}

void CWinSystemWin10::OnDisplayReset()
{
  if (!m_delayDispReset)
  {
    CLog::Log(LOGDEBUG, "{} - notify display reset event", __FUNCTION__);
    std::unique_lock<CCriticalSection> lock(m_resourceSection);
    for (std::vector<IDispResource *>::iterator i = m_resources.begin(); i != m_resources.end(); ++i)
      (*i)->OnResetDisplay();
  }
}

void CWinSystemWin10::OnDisplayBack()
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

void CWinSystemWin10::ResolutionChanged()
{
  OnDisplayLost();
  OnDisplayBack();
}

std::unique_ptr<CVideoSync> CWinSystemWin10::GetVideoSync(CVideoReferenceClock* clock)
{
  std::unique_ptr<CVideoSync> pVSync(new CVideoSyncD3D(clock));
  return pVSync;
}

std::string CWinSystemWin10::GetClipboardText()
{
  std::wstring unicode_text;

  auto contentView = Clipboard::GetContent();
  if (contentView.Contains(StandardDataFormats::Text()))
  {
    auto text = Wait(contentView.GetTextAsync());
    unicode_text.append(text.c_str());
  }

  return KODI::PLATFORM::WINDOWS::FromW(unicode_text);
}

bool CWinSystemWin10::UseLimitedColor()
{
  return CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_VIDEOSCREEN_LIMITEDRANGE);
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

/*!
 * \brief Max luminance for GUI SDR content in HDR mode.
 * \return Max luminance in nits, lower than 10000.
*/
float CWinSystemWin10::GetGuiSdrPeakLuminance() const
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
bool CWinSystemWin10::HasSystemSdrPeakLuminance()
{
  if (m_uiThreadId == GetCurrentThreadId())
  {
    const bool hasSystemSdrPeakLum = CWIN32Util::GetSystemSdrWhiteLevel(std::wstring(), nullptr);
    m_cachedHasSystemSdrPeakLum = hasSystemSdrPeakLum;
    return hasSystemSdrPeakLum;
  }

  return m_cachedHasSystemSdrPeakLum;
}

/*!
 * \brief Cache the system HDR/SDR balance for use during rendering, instead of querying the API
 for each frame.
*/
void CWinSystemWin10::CacheSystemSdrPeakLuminance()
{
  m_validSystemSdrPeakLuminance =
      CWIN32Util::GetSystemSdrWhiteLevel(std::wstring(), &m_systemSdrPeakLuminance);
}

#pragma pack(pop)
