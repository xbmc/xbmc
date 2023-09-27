/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoSyncD3D.h"

#include "Utils/MathUtils.h"
#include "Utils/TimeUtils.h"
#include "cores/VideoPlayer/VideoReferenceClock.h"
#include "rendering/dx/DeviceResources.h"
#include "rendering/dx/RenderContext.h"
#include "utils/StringUtils.h"
#include "utils/XTimeUtils.h"
#include "utils/log.h"
#include "windowing/GraphicContext.h"

#include <mutex>

#ifdef TARGET_WINDOWS_STORE
#include <winrt/Windows.Graphics.Display.Core.h>
#endif

using namespace std::chrono_literals;

void CVideoSyncD3D::OnLostDisplay()
{
  if (!m_displayLost)
  {
    m_displayLost = true;
    m_lostEvent.Wait();
  }
}

void CVideoSyncD3D::OnResetDisplay()
{
  m_displayReset = true;
}

void CVideoSyncD3D::RefreshChanged()
{
  m_displayReset = true;
}

bool CVideoSyncD3D::Setup()
{
  CLog::Log(LOGDEBUG, "CVideoSyncD3D: Setting up Direct3d");
  std::unique_lock<CCriticalSection> lock(CServiceBroker::GetWinSystem()->GetGfxContext());
  DX::Windowing()->Register(this);
  m_displayLost = false;
  m_displayReset = false;
  m_lostEvent.Reset();

  // we need a high priority thread to get accurate timing
  if (!SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL))
    CLog::Log(LOGDEBUG, "CVideoSyncD3D: SetThreadPriority failed");

  CreateDXGIFactory1(IID_PPV_ARGS(m_factory.ReleaseAndGetAddressOf()));

  Microsoft::WRL::ComPtr<IDXGIOutput> pOutput;
  DX::DeviceResources::Get()->GetCachedOutputAndDesc(&pOutput, &m_outputDesc);

  return true;
}

void CVideoSyncD3D::Run(CEvent& stopEvent)
{
  int64_t Now;
  int64_t LastVBlankTime;
  int NrVBlanks;
  double VBlankTime;
  const int64_t systemFrequency = CurrentHostFrequency();
  bool validVBlank{true};

  // init the vblanktime
  Now = CurrentHostCounter();
  LastVBlankTime = Now;

  while (!stopEvent.Signaled() && !m_displayLost && !m_displayReset)
  {
    // sleep until vblank
    Microsoft::WRL::ComPtr<IDXGIOutput> pOutput;
    DX::DeviceResources::Get()->GetCachedOutputAndDesc(pOutput.ReleaseAndGetAddressOf(),
                                                       &m_outputDesc);

    const int64_t WaitForVBlankStartTime = CurrentHostCounter();
    const HRESULT hr = pOutput ? pOutput->WaitForVBlank() : E_INVALIDARG;
    const int64_t WaitForVBlankElapsedTime = CurrentHostCounter() - WaitForVBlankStartTime;

    // WaitForVBlank() can return very quickly due to errors or screen sleeping
    if (!SUCCEEDED(hr) || WaitForVBlankElapsedTime - (systemFrequency / 1000) <= 0)
    {
      if (SUCCEEDED(hr) && validVBlank)
        CLog::LogF(LOGWARNING, "failed to detect vblank - screen asleep?");

      if (!SUCCEEDED(hr))
        CLog::LogF(LOGERROR, "error waiting for vblank, {}", DX::GetErrorDescription(hr));

      validVBlank = false;

      // Wait a while, until vblank may have come back. No need for accurate sleep.
      ::Sleep(250);
      continue;
    }
    else if (!validVBlank)
    {
      CLog::LogF(LOGWARNING, "vblank detected - resuming reference clock updates");
      validVBlank = true;
    }

    // calculate how many vblanks happened
    Now = CurrentHostCounter();
    VBlankTime = (double)(Now - LastVBlankTime) / (double)systemFrequency;
    NrVBlanks = MathUtils::round_int(VBlankTime * m_fps);

    // update the vblank timestamp, update the clock and send a signal that we got a vblank
    m_refClock->UpdateClock(NrVBlanks, Now);

    // save the timestamp of this vblank so we can calculate how many vblanks happened next time
    LastVBlankTime = Now;

    if (!m_factory->IsCurrent())
    {
      CreateDXGIFactory1(IID_PPV_ARGS(m_factory.ReleaseAndGetAddressOf()));

      float fps = m_fps;
      if (fps != GetFps())
        break;
    }
  }

  m_lostEvent.Set();
  while (!stopEvent.Signaled() && m_displayLost && !m_displayReset)
  {
    KODI::TIME::Sleep(10ms);
  }
}

void CVideoSyncD3D::Cleanup()
{
  CLog::Log(LOGDEBUG, "CVideoSyncD3D: Cleaning up Direct3d");

  m_lostEvent.Set();
  DX::Windowing()->Unregister(this);
}

float CVideoSyncD3D::GetFps()
{
#ifdef TARGET_WINDOWS_DESKTOP
  DEVMODEW sDevMode = {};
  sDevMode.dmSize = sizeof(sDevMode);

  if (EnumDisplaySettingsW(m_outputDesc.DeviceName, ENUM_CURRENT_SETTINGS, &sDevMode))
  {
    if ((sDevMode.dmDisplayFrequency + 1) % 24 == 0 || (sDevMode.dmDisplayFrequency + 1) % 30 == 0)
      m_fps = static_cast<float>(sDevMode.dmDisplayFrequency + 1) / 1.001f;
    else
      m_fps = static_cast<float>(sDevMode.dmDisplayFrequency);

    if (sDevMode.dmDisplayFlags & DM_INTERLACED)
      m_fps *= 2;
  }
#else
  using namespace winrt::Windows::Graphics::Display::Core;

  auto hdmiInfo = HdmiDisplayInformation::GetForCurrentView();
  if (hdmiInfo) // Xbox only
  {
    auto currentMode = hdmiInfo.GetCurrentDisplayMode();
    m_fps = static_cast<float>(currentMode.RefreshRate());
  }
#endif

  if (m_fps == 0.0)
    m_fps = 60.0f;

  return m_fps;
}
