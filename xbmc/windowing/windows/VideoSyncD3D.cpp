/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/log.h"
#include "Utils/TimeUtils.h"
#include "Utils/MathUtils.h"
#include "rendering/dx/DeviceResources.h"
#include "rendering/dx/RenderContext.h"
#include "VideoSyncD3D.h"
#include "windowing/GraphicContext.h"
#include "utils/StringUtils.h"

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

bool CVideoSyncD3D::Setup(PUPDATECLOCK func)
{
  CLog::Log(LOGDEBUG, "CVideoSyncD3D: Setting up Direct3d");
  CSingleLock lock(CServiceBroker::GetWinSystem()->GetGfxContext());
  DX::Windowing()->Register(this);
  m_displayLost = false;
  m_displayReset = false;
  m_lostEvent.Reset();
  UpdateClock = func;

  // we need a high priority thread to get accurate timing
  if (!SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL))
    CLog::Log(LOGDEBUG, "CVideoSyncD3D: SetThreadPriority failed");

  return true;
}

void CVideoSyncD3D::Run(CEvent& stopEvent)
{
  int64_t Now;
  int64_t LastVBlankTime;
  int NrVBlanks;
  double VBlankTime;
  int64_t systemFrequency = CurrentHostFrequency();

  // init the vblanktime
  Now = CurrentHostCounter();
  LastVBlankTime = Now;
  m_lastUpdateTime = Now - systemFrequency;
  while (!stopEvent.Signaled() && !m_displayLost && !m_displayReset)
  {
    // sleep until vblank
    Microsoft::WRL::ComPtr<IDXGIOutput> pOutput;
    DX::DeviceResources::Get()->GetOutput(&pOutput);
    HRESULT hr = pOutput->WaitForVBlank();

    // calculate how many vblanks happened
    Now = CurrentHostCounter();
    VBlankTime = (double)(Now - LastVBlankTime) / (double)systemFrequency;
    NrVBlanks = MathUtils::round_int(VBlankTime * m_fps);

    // update the vblank timestamp, update the clock and send a signal that we got a vblank
    UpdateClock(NrVBlanks, Now, m_refClock);

    // save the timestamp of this vblank so we can calculate how many vblanks happened next time
    LastVBlankTime = Now;

    if ((Now - m_lastUpdateTime) >= systemFrequency)
    {
      float fps = m_fps;
      if (fps != GetFps())
        break;
    }

    // because we had a vblank, sleep until half the refreshrate period because i think WaitForVBlank block any rendering stuf
    // without sleeping we have freeze rendering
    int SleepTime = (int)((LastVBlankTime + (systemFrequency / MathUtils::round_int(m_fps) / 2) - Now) * 1000 / systemFrequency);
    if (SleepTime > 50)
      SleepTime = 50; //failsafe
    if (SleepTime > 0)
      ::Sleep(SleepTime);
  }

  m_lostEvent.Set();
  while (!stopEvent.Signaled() && m_displayLost && !m_displayReset)
  {
    Sleep(10);
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
  DXGI_MODE_DESC DisplayMode;
  DX::DeviceResources::Get()->GetDisplayMode(&DisplayMode);

  m_fps = (DisplayMode.RefreshRate.Denominator != 0) ? (float)DisplayMode.RefreshRate.Numerator / (float)DisplayMode.RefreshRate.Denominator : 0.0f;

  if (m_fps == 0.0)
    m_fps = 60.0f;

  if (m_fps == 23 || m_fps == 29 || m_fps == 59)
    m_fps++;

  if (DX::Windowing()->Interlaced())
  {
    m_fps *= 2;
  }
  return m_fps;
}

