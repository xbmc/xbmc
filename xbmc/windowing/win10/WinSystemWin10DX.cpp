/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WinSystemWin10DX.h"

#include "input/touch/generic/GenericTouchActionHandler.h"
#include "input/touch/generic/GenericTouchInputHandler.h"
#include "rendering/dx/DirectXHelper.h"
#include "utils/XTimeUtils.h"
#include "utils/log.h"

std::unique_ptr<CWinSystemBase> CWinSystemBase::CreateWinSystem()
{
  return std::make_unique<CWinSystemWin10DX>();
}

CWinSystemWin10DX::CWinSystemWin10DX() : CRenderSystemDX()
{
}

CWinSystemWin10DX::~CWinSystemWin10DX()
{
}

void CWinSystemWin10DX::PresentRenderImpl(bool rendered)
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

bool CWinSystemWin10DX::CreateNewWindow(const std::string& name, bool fullScreen, RESOLUTION_INFO& res)
{
  const MONITOR_DETAILS* monitor = GetDefaultMonitor();
  if (!monitor)
    return false;

  m_deviceResources = DX::DeviceResources::Get();
  m_deviceResources->SetWindow(m_coreWindow);

  if (CWinSystemWin10::CreateNewWindow(name, fullScreen, res) && m_deviceResources->HasValidDevice())
  {
    CGenericTouchInputHandler::GetInstance().RegisterHandler(&CGenericTouchActionHandler::GetInstance());
    CGenericTouchInputHandler::GetInstance().SetScreenDPI(m_deviceResources->GetDpi());
    return true;
  }
  return false;
}

bool CWinSystemWin10DX::DestroyRenderSystem()
{
  CRenderSystemDX::DestroyRenderSystem();

  m_deviceResources->Release();
  m_deviceResources.reset();
  return true;
}

void CWinSystemWin10DX::ShowSplash(const std::string & message)
{
  CRenderSystemBase::ShowSplash(message);

  // this will prevent killing the app by watchdog timeout during loading
  if (m_coreWindow != nullptr)
    m_coreWindow.Dispatcher().ProcessEvents(winrt::Windows::UI::Core::CoreProcessEventsOption::ProcessAllIfPresent);
}

void CWinSystemWin10DX::SetDeviceFullScreen(bool fullScreen, RESOLUTION_INFO& res)
{
  m_deviceResources->SetFullScreen(fullScreen, res);
}

bool CWinSystemWin10DX::ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop)
{
  CWinSystemWin10::ResizeWindow(newWidth, newHeight, newLeft, newTop);
  CRenderSystemDX::OnResize();

  return true;
}

void CWinSystemWin10DX::OnMove(int x, int y)
{
  m_deviceResources->SetWindowPos(m_coreWindow.Bounds());
}

bool CWinSystemWin10DX::DPIChanged(WORD dpi, RECT windowRect) const
{
  m_deviceResources->SetDpi(dpi);
  if (!IsAlteringWindow())
    return CWinSystemWin10::DPIChanged(dpi, windowRect);

  return true;
}

void CWinSystemWin10DX::ReleaseBackBuffer()
{
  m_deviceResources->ReleaseBackBuffer();
}

void CWinSystemWin10DX::CreateBackBuffer()
{
  m_deviceResources->CreateBackBuffer();
}

void CWinSystemWin10DX::ResizeDeviceBuffers()
{
  m_deviceResources->ResizeBuffers();
}

bool CWinSystemWin10DX::IsStereoEnabled()
{
  return m_deviceResources->IsStereoEnabled();
}

void CWinSystemWin10DX::OnResize(int width, int height)
{
  if (!m_deviceResources)
    return;

  if (!m_IsAlteringWindow)
    ReleaseBackBuffer();

  m_deviceResources->SetLogicalSize(width, height);

  if (!m_IsAlteringWindow)
    CreateBackBuffer();
}

bool CWinSystemWin10DX::SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays)
{
  bool const result = CWinSystemWin10::SetFullScreen(fullScreen, res, blankOtherDisplays);
  CRenderSystemDX::OnResize();
  return result;
}

void CWinSystemWin10DX::UninitHooks()
{
}

void CWinSystemWin10DX::InitHooks(IDXGIOutput* pOutput)
{
}
