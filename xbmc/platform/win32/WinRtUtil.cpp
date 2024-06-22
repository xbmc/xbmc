/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WinRtUtil.h"

#include "rendering/dx/DirectXHelper.h"
#include "utils/SystemInfo.h"
#include "utils/log.h"

#ifdef TARGET_WINDOWS_DESKTOP
#include <Windows.Graphics.Display.Interop.h>

#else
#include <winrt/Windows.Devices.Display.Core.h>

using namespace winrt::Windows::Devices::Display::Core;
#endif

#include <winrt/Windows.Graphics.Display.h>

using namespace winrt::Windows::Graphics::Display;

#ifdef TARGET_WINDOWS_DESKTOP
extern HWND g_hWnd;
#endif

static constexpr std::string_view AdvancedColorKindToString(AdvancedColorKind kind)
{
  switch (kind)
  {
    case AdvancedColorKind::StandardDynamicRange:
      return "SDR";
    case AdvancedColorKind::WideColorGamut:
      return "WCG";
    case AdvancedColorKind::HighDynamicRange:
      return "HDR";
    default:
      return "unknown";
  }
}

static void LogAdvancedColorInfo(AdvancedColorInfo colorInfo)
{
  std::string availableKinds;
  static constexpr std::array<AdvancedColorKind, 3> kinds{AdvancedColorKind::StandardDynamicRange,
                                                          AdvancedColorKind::WideColorGamut,
                                                          AdvancedColorKind::HighDynamicRange};

  for (const AdvancedColorKind& kind : kinds)
  {
    if (colorInfo.IsAdvancedColorKindAvailable(kind))
      availableKinds.append(AdvancedColorKindToString(kind)).append(" ");
  }

  CLog::LogF(LOGDEBUG, "Current advanced color kind: {}, supported kinds: {}",
             AdvancedColorKindToString(colorInfo.CurrentAdvancedColorKind()), availableKinds);
}

HDR_STATUS CWinRtUtil::GetWindowsHDRStatus()
{
  DisplayInformation displayInformation{nullptr};

#ifdef TARGET_WINDOWS_STORE
  displayInformation = DisplayInformation::GetForCurrentView();

  if (!displayInformation)
  {
    CLog::LogF(LOGERROR, "unable to retrieve DisplayInformation.");
    return HDR_STATUS::HDR_UNKNOWN;
  }
#else
  auto factory{
      winrt::try_get_activation_factory<DisplayInformation, IDisplayInformationStaticsInterop>()};

  if (!factory)
  {
    CLog::LogF(LOGERROR,
               "unable to activate IDisplayInformationStaticsInterop. Windows version too low?");
    return HDR_STATUS::HDR_UNKNOWN;
  }

  HMONITOR hm{};
  if (g_hWnd)
  {
    RECT rect{};

    if (FALSE == GetWindowRect(g_hWnd, &rect))
    {
      CLog::LogF(LOGERROR, "unable to retrieve window rect, error {}",
                 DX::GetErrorDescription(GetLastError()));
      return HDR_STATUS::HDR_UNKNOWN;
    }

    hm = MonitorFromRect(&rect, MONITOR_DEFAULTTONULL);

    if (hm == NULL)
    {
      CLog::LogF(LOGERROR, "unable to retrieve monitor intersecting with application window");
      return HDR_STATUS::HDR_UNKNOWN;
    }
  }
  else
  {
    POINT point{};

    hm = MonitorFromPoint(point, MONITOR_DEFAULTTONULL);

    if (hm == NULL)
    {
      CLog::LogF(LOGERROR, "unable to retrieve primary monitor");
      return HDR_STATUS::HDR_UNKNOWN;
    }
  }

  HRESULT hr = factory->GetForMonitor(hm, winrt::guid_of<DisplayInformation>(),
                                      winrt::put_abi(displayInformation));
  if (FAILED(hr))
  {
    CLog::LogF(LOGERROR, "unable to retrieve DisplayInformation for window, error {}",
               DX::GetErrorDescription(hr));
    return HDR_STATUS::HDR_UNKNOWN;
  }
#endif

  AdvancedColorInfo colorInfo{displayInformation.GetAdvancedColorInfo()};

  if (!colorInfo)
  {
    CLog::LogF(LOGERROR, "unable to retrieve advanced color info");
    return HDR_STATUS::HDR_UNKNOWN;
  }

  LogAdvancedColorInfo(colorInfo);

  if (colorInfo.CurrentAdvancedColorKind() == AdvancedColorKind::HighDynamicRange)
    return HDR_STATUS::HDR_ON;

  // IsAdvancedColorKindAvailable works for desktop Windows 11 22H2+ and Xbox
  if (colorInfo.IsAdvancedColorKindAvailable(AdvancedColorKind::HighDynamicRange))
    return HDR_STATUS::HDR_OFF;
  else
    return HDR_STATUS::HDR_UNSUPPORTED;
}

HDR_STATUS CWinRtUtil::GetWindowsHDRStatusUWP()
{
#ifdef TARGET_WINDOWS_STORE
  // Legacy detection, useful for UWP desktop at this point
  DisplayInformation displayInformation = DisplayInformation::GetForCurrentView();

  if (!displayInformation)
  {
    CLog::LogF(LOGERROR, "unable to retrieve DisplayInformation.");
    return HDR_STATUS::HDR_UNKNOWN;
  }

  AdvancedColorInfo colorInfo{displayInformation.GetAdvancedColorInfo()};

  if (!colorInfo)
  {
    CLog::LogF(LOGERROR, "unable to retrieve advanced color info");
    return HDR_STATUS::HDR_UNKNOWN;
  }

  if (colorInfo.CurrentAdvancedColorKind() == AdvancedColorKind::HighDynamicRange)
    return HDR_STATUS::HDR_ON;

  // Detects any hdr-capable screen connected to the system, not necessarily the one that contains
  // the Kodi window
  bool hdrSupported{false};
  auto displayManager = DisplayManager::Create(DisplayManagerOptions::None);

  if (displayManager)
  {
    auto targets = displayManager.GetCurrentTargets();

    for (const auto& target : targets)
    {
      if (target.IsConnected())
      {
        auto displayMonitor = target.TryGetMonitor();
        if (displayMonitor.MaxLuminanceInNits() >= 400.0f)
        {
          hdrSupported = true;
          break;
        }
      }
    }
    displayManager.Close();
  }

  if (hdrSupported)
    return HDR_STATUS::HDR_OFF;
#endif // TARGET_WINDOWS_STORE

  return HDR_STATUS::HDR_UNSUPPORTED;
}
