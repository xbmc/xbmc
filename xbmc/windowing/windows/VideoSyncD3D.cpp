/*
 *      Copyright (C) 2005-2014 Team XBMC
 *      http://xbmc.org
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

#include "system.h"

#include "utils/log.h"
#include "Utils/TimeUtils.h"
#include "Utils/MathUtils.h"
#include "windowing\WindowingFactory.h"
#include "VideoSyncD3D.h"
#include "guilib/GraphicContext.h"
#include "platform/win32/dxerr.h"
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
  CSingleLock lock(g_graphicsContext);
  g_Windowing.Register(this);
  m_displayLost = false;
  m_displayReset = false;
  m_lostEvent.Reset();
  UpdateClock = func;

  // we need a high priority thread to get accurate timing
  if (!SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL))
    CLog::Log(LOGDEBUG, "CVideoSyncD3D: SetThreadPriority failed");

  return true;
}

void CVideoSyncD3D::Run(std::atomic<bool>& stop)
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
  while (!stop && !m_displayLost && !m_displayReset)
  {
    // sleep until vblank
    HRESULT hr = g_Windowing.GetCurrentOutput()->WaitForVBlank();

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
  while (!stop && m_displayLost && !m_displayReset)
  {
    Sleep(10);
  }
}

void CVideoSyncD3D::Cleanup()
{
  CLog::Log(LOGDEBUG, "CVideoSyncD3D: Cleaning up Direct3d");

  m_lostEvent.Set();
  g_Windowing.Unregister(this);
}

float CVideoSyncD3D::GetFps()
{
  DXGI_MODE_DESC DisplayMode;
  g_Windowing.GetDisplayMode(&DisplayMode, true);

  m_fps = (DisplayMode.RefreshRate.Denominator != 0) ? (float)DisplayMode.RefreshRate.Numerator / (float)DisplayMode.RefreshRate.Denominator : 0.0f;

  if (m_fps == 0.0)
    m_fps = 60.0f;
  
  if (m_fps == 23 || m_fps == 29 || m_fps == 59)
    m_fps++;

  if (g_Windowing.Interlaced())
  {
    m_fps *= 2;
  }
  return m_fps;
}

std::string CVideoSyncD3D::GetErrorDescription(HRESULT hr)
{
  WCHAR buff[1024];
  DXGetErrorDescription(hr, buff, 1024);
  std::wstring error(DXGetErrorString(hr));
  std::wstring descr(buff);
  return StringUtils::Format("%ls: %ls", error.c_str(), descr.c_str());
}

