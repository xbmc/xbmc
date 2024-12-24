/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DisplayUtilsWin32.h"

#include "utils/SystemInfo.h"

// @todo: Remove this and "SDK_26100.h" when Windows SDK updated to 10.0.26100.0 in builders
#ifndef NTDDI_WIN11_GE // Windows SDK 10.0.26100.0 or newer
#include "SDK_26100.h"
#endif

extern HWND g_hWnd;

std::wstring CDisplayUtilsWin32::GetCurrentDisplayName()
{
  MONITORINFOEXW mi{};
  mi.cbSize = sizeof(mi);
  if (!GetMonitorInfoW(MonitorFromWindow(g_hWnd, MONITOR_DEFAULTTOPRIMARY), &mi))
    return {};
  return mi.szDevice;
}

std::vector<DISPLAYCONFIG_PATH_INFO> CDisplayUtilsWin32::GetDisplayConfigPaths()
{
  uint32_t pathCount{0};
  uint32_t modeCount{0};
  std::vector<DISPLAYCONFIG_PATH_INFO> paths;
  std::vector<DISPLAYCONFIG_MODE_INFO> modes;

  constexpr uint32_t flags{QDC_ONLY_ACTIVE_PATHS};
  LONG result{ERROR_SUCCESS};

  do
  {
    if (GetDisplayConfigBufferSizes(flags, &pathCount, &modeCount) != ERROR_SUCCESS)
      return {};

    paths.resize(pathCount);
    modes.resize(modeCount);

    result = QueryDisplayConfig(flags, &pathCount, paths.data(), &modeCount, modes.data(), nullptr);
  } while (result == ERROR_INSUFFICIENT_BUFFER);

  if (result != ERROR_SUCCESS)
    return {};

  paths.resize(pathCount);
  modes.resize(modeCount);

  return paths;
}

std::optional<CDisplayUtilsWin32::DisplayConfigId> CDisplayUtilsWin32::GetCurrentDisplayTargetId()
{
  std::wstring name{GetCurrentDisplayName()};
  if (name.empty())
    return std::nullopt;

  return GetDisplayTargetId(name);
}

std::optional<CDisplayUtilsWin32::DisplayConfigId> CDisplayUtilsWin32::GetDisplayTargetId(
    const std::wstring& gdiDeviceName)
{
  DISPLAYCONFIG_SOURCE_DEVICE_NAME source{};
  source.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME;
  source.header.size = sizeof(source);

  for (const auto& path : GetDisplayConfigPaths())
  {
    source.header.adapterId = path.sourceInfo.adapterId;
    source.header.id = path.sourceInfo.id;

    if (DisplayConfigGetDeviceInfo(&source.header) == ERROR_SUCCESS &&
        gdiDeviceName == source.viewGdiDeviceName)
      return DisplayConfigId{path.targetInfo.adapterId, path.targetInfo.id};
  }
  return std::nullopt;
}

HDR_STATUS CDisplayUtilsWin32::SetDisplayHDRStatus(const DisplayConfigId& identifier, bool enable)
{
  LONG result{ERROR_SUCCESS};
  HDR_STATUS status{enable ? HDR_STATUS::HDR_ON : HDR_STATUS::HDR_OFF};

  // Windows 11 24H2 or newer (SDK 10.0.26100.0)
  if (CSysInfo::IsWindowsVersionAtLeast(CSysInfo::WindowsVersionWin11_24H2))
  {
    DISPLAYCONFIG_SET_HDR_STATE setHdrState = {};
    setHdrState.header.type =
        static_cast<DISPLAYCONFIG_DEVICE_INFO_TYPE>(DISPLAYCONFIG_DEVICE_INFO_SET_HDR_STATE);
    setHdrState.header.size = sizeof(setHdrState);
    setHdrState.header.adapterId = identifier.adapterId;
    setHdrState.header.id = identifier.id;
    setHdrState.enableHdr = enable ? TRUE : FALSE;

    result = DisplayConfigSetDeviceInfo(&setHdrState.header);
  }
  else // older than Windows 11 24H2
  {
    DISPLAYCONFIG_SET_ADVANCED_COLOR_STATE setColorState = {};
    setColorState.header.type = DISPLAYCONFIG_DEVICE_INFO_SET_ADVANCED_COLOR_STATE;
    setColorState.header.size = sizeof(setColorState);
    setColorState.header.adapterId = identifier.adapterId;
    setColorState.header.id = identifier.id;
    setColorState.enableAdvancedColor = enable ? TRUE : FALSE;

    result = DisplayConfigSetDeviceInfo(&setColorState.header);
  }

  if (result != ERROR_SUCCESS)
    status = HDR_STATUS::HDR_TOGGLE_FAILED;

  return status;
}

HDR_STATUS CDisplayUtilsWin32::GetDisplayHDRStatus(const DisplayConfigId& identifier)
{
  bool hdrSupported{false};
  bool hdrEnabled{false};

  // Windows 11 24H2 or newer (SDK 10.0.26100.0)
  if (CSysInfo::IsWindowsVersionAtLeast(CSysInfo::WindowsVersionWin11_24H2))
  {
    DISPLAYCONFIG_GET_ADVANCED_COLOR_INFO_2 getColorInfo2 = {};
    getColorInfo2.header.type = static_cast<DISPLAYCONFIG_DEVICE_INFO_TYPE>(
        DISPLAYCONFIG_DEVICE_INFO_GET_ADVANCED_COLOR_INFO_2);
    getColorInfo2.header.size = sizeof(getColorInfo2);
    getColorInfo2.header.adapterId = identifier.adapterId;
    getColorInfo2.header.id = identifier.id;

    if (ERROR_SUCCESS == DisplayConfigGetDeviceInfo(&getColorInfo2.header))
    {
      if (getColorInfo2.activeColorMode == DISPLAYCONFIG_ADVANCED_COLOR_MODE_HDR)
        hdrEnabled = true;

      if (getColorInfo2.highDynamicRangeSupported == TRUE)
        hdrSupported = true;
    }
  }
  else // older than Windows 11 24H2
  {
    DISPLAYCONFIG_GET_ADVANCED_COLOR_INFO getColorInfo = {};
    getColorInfo.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_ADVANCED_COLOR_INFO;
    getColorInfo.header.size = sizeof(getColorInfo);
    getColorInfo.header.adapterId = identifier.adapterId;
    getColorInfo.header.id = identifier.id;

    if (ERROR_SUCCESS == DisplayConfigGetDeviceInfo(&getColorInfo.header))
    {
      // DISPLAYCONFIG_DEVICE_INFO_GET_ADVANCED_COLOR_INFO documentation is lacking and field
      // names are confusing. Through experimentation and deductions from equivalent WinRT API:
      //
      // SDR screen, advanced color not supported (Win 10, Win 11 < 22H2)
      // > advancedColorSupported = 0 and wideColorEnforced = 0
      // SDR screen, advanced color is supported (Win 11 >= 22H2)
      // > advancedColorSupported = 1 and wideColorEnforced = 1
      // HDR screen
      // > advancedColorSupported = 1 and wideColorEnforced = 0
      //
      // advancedColorForceDisabled: maybe equivalent of advancedColorLimitedByPolicy?
      //
      // advancedColorEnabled = 1:
      // For HDR screens means HDR is on
      // For SDR screens means ACM (Automatic Color Management, Win 11 >= 22H2) is on

      if (getColorInfo.advancedColorSupported && !getColorInfo.wideColorEnforced)
        hdrSupported = true;

      if (hdrSupported && getColorInfo.advancedColorEnabled)
        hdrEnabled = true;
    }
  }

  HDR_STATUS status{HDR_STATUS::HDR_UNSUPPORTED};

  if (hdrSupported)
    status = hdrEnabled ? HDR_STATUS::HDR_ON : HDR_STATUS::HDR_OFF;

  return status;
}
