/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WinSystemWin32DX.h"

#include "commons/ilog.h"
#include "rendering/dx/RenderContext.h"
#include "settings/DisplaySettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/SystemInfo.h"
#include "utils/XTimeUtils.h"
#include "utils/log.h"
#include "windowing/GraphicContext.h"
#include "windowing/WindowSystemFactory.h"

#include "platform/win32/CharsetConverter.h"
#include "platform/win32/WIN32Util.h"

#include "system.h"

void CWinSystemWin32DX::Register()
{
  KODI::WINDOWING::CWindowSystemFactory::RegisterWindowSystem(CreateWinSystem);
}

std::unique_ptr<CWinSystemBase> CWinSystemWin32DX::CreateWinSystem()
{
  return std::make_unique<CWinSystemWin32DX>();
}

CWinSystemWin32DX::CWinSystemWin32DX() : CRenderSystemDX()
{
}

CWinSystemWin32DX::~CWinSystemWin32DX()
{
}

void CWinSystemWin32DX::PresentRenderImpl(bool rendered)
{
  if (rendered)
    m_deviceResources->Present();

  if (m_delayDispReset && m_dispResetTimer.IsTimePast())
  {
    m_delayDispReset = false;
    OnDisplayReset();
  }

  if (!rendered)
    KODI::TIME::Sleep(40);
}

bool CWinSystemWin32DX::CreateNewWindow(const std::string& name, bool fullScreen, RESOLUTION_INFO& res)
{
  const MONITOR_DETAILS* monitor = GetDisplayDetails(CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(CSettings::SETTING_VIDEOSCREEN_MONITOR));
  if (!monitor)
    return false;

  m_hMonitor = monitor->hMonitor;
  m_deviceResources = DX::DeviceResources::Get();
  // setting monitor before creating window for proper hooking into a driver
  m_deviceResources->SetMonitor(m_hMonitor);

  return CWinSystemWin32::CreateNewWindow(name, fullScreen, res) && m_deviceResources->HasValidDevice();
}

void CWinSystemWin32DX::SetWindow(HWND hWnd) const
{
  m_deviceResources->SetWindow(hWnd);
}

bool CWinSystemWin32DX::DestroyRenderSystem()
{
  CRenderSystemDX::DestroyRenderSystem();

  m_deviceResources->Release();
  m_deviceResources.reset();
  return true;
}

void CWinSystemWin32DX::SetDeviceFullScreen(bool fullScreen, RESOLUTION_INFO& res)
{
  if (m_deviceResources->SetFullScreen(fullScreen, res))
  {
    ResolutionChanged();
  }
}

bool CWinSystemWin32DX::ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop)
{
  CWinSystemWin32::ResizeWindow(newWidth, newHeight, newLeft, newTop);
  CRenderSystemDX::OnResize();

  return true;
}

void CWinSystemWin32DX::OnMove(int x, int y)
{
  // do not handle moving at window creation because MonitorFromWindow
  // returns default system monitor in case of m_hWnd is null
  if (!m_hWnd)
    return;

  HMONITOR newMonitor = MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST);
  if (newMonitor != m_hMonitor)
  {
    MONITOR_DETAILS* details = GetDisplayDetails(newMonitor);

    if (!details)
      return;

    CDisplaySettings::GetInstance().SetMonitor(KODI::PLATFORM::WINDOWS::FromW(details->MonitorNameW));
    m_deviceResources->SetMonitor(newMonitor);
    m_hMonitor = newMonitor;
  }
}

bool CWinSystemWin32DX::DPIChanged(WORD dpi, RECT windowRect) const
{
  // on Win10 FCU the OS keeps window size exactly the same size as it was
  if (CSysInfo::IsWindowsVersionAtLeast(CSysInfo::WindowsVersionWin10_1709))
    return true;

  m_deviceResources->SetDpi(dpi);
  if (!IsAlteringWindow())
    return __super::DPIChanged(dpi, windowRect);

  return true;
}

void CWinSystemWin32DX::ReleaseBackBuffer()
{
  m_deviceResources->ReleaseBackBuffer();
}

void CWinSystemWin32DX::CreateBackBuffer()
{
  m_deviceResources->CreateBackBuffer();
}

void CWinSystemWin32DX::ResizeDeviceBuffers()
{
  m_deviceResources->ResizeBuffers();
}

bool CWinSystemWin32DX::IsStereoEnabled()
{
  return m_deviceResources->IsStereoEnabled();
}

void CWinSystemWin32DX::OnScreenChange(HMONITOR monitor)
{
  m_deviceResources->SetMonitor(monitor);
}

bool CWinSystemWin32DX::ChangeResolution(const RESOLUTION_INFO &res, bool forceChange)
{
  bool changed = CWinSystemWin32::ChangeResolution(res, forceChange);
  // this is a try to fix FCU issue after changing resolution
  if (m_deviceResources && changed)
    m_deviceResources->ResizeBuffers();
  return changed;
}

void CWinSystemWin32DX::OnResize(int width, int height)
{
  if (!m_IsAlteringWindow)
    ReleaseBackBuffer();

  m_deviceResources->SetLogicalSize(static_cast<float>(width), static_cast<float>(height));

  if (!m_IsAlteringWindow)
    CreateBackBuffer();
}

bool CWinSystemWin32DX::SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays)
{
  bool const result = CWinSystemWin32::SetFullScreen(fullScreen, res, blankOtherDisplays);
  CRenderSystemDX::OnResize();
  return result;
}

bool CWinSystemWin32DX::IsHDRDisplay()
{
  return (CWIN32Util::GetWindowsHDRStatus() != HDR_STATUS::HDR_UNSUPPORTED);
}

HDR_STATUS CWinSystemWin32DX::GetOSHDRStatus()
{
  return CWIN32Util::GetWindowsHDRStatus();
}

HDR_STATUS CWinSystemWin32DX::ToggleHDR()
{
  return m_deviceResources->ToggleHDR();
}

bool CWinSystemWin32DX::IsHDROutput() const
{
  return m_deviceResources->IsHDROutput();
}

bool CWinSystemWin32DX::IsTransferPQ() const
{
  return m_deviceResources->IsTransferPQ();
}

void CWinSystemWin32DX::SetHdrMetaData(DXGI_HDR_METADATA_HDR10& hdr10) const
{
  m_deviceResources->SetHdrMetaData(hdr10);
}

void CWinSystemWin32DX::SetHdrColorSpace(const DXGI_COLOR_SPACE_TYPE colorSpace) const
{
  m_deviceResources->SetHdrColorSpace(colorSpace);
}

DEBUG_INFO_RENDER CWinSystemWin32DX::GetDebugInfo()
{
  return m_deviceResources->GetDebugInfo();
}
