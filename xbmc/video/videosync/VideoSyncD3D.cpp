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

#if defined(TARGET_WINDOWS)

#pragma comment (lib,"d3d9.lib")
#pragma comment (lib,"DxErr.lib")
#include <Dxerr.h>
#include "utils/log.h"
#include "Utils/TimeUtils.h"
#include "Utils/MathUtils.h"
#include "windowing\WindowingFactory.h"
#include "video/videosync/VideoSyncD3D.h"
#include "guilib/GraphicContext.h"

void CVideoSyncD3D::OnDestroyDevice()
{
  if (!m_displayLost)
  {
    m_displayLost = true;
    m_lostEvent.Wait();
  }
}

void CVideoSyncD3D::OnResetDevice()
{
  m_displayReset = true;
}

void CVideoSyncD3D::RefreshChanged()
{
  m_displayReset = true;
}

bool CVideoSyncD3D::Setup(PUPDATECLOCK func)
{
  int ReturnV;
  CLog::Log(LOGDEBUG, "CVideoSyncD3D: Setting up Direct3d");
  CSingleLock lock(g_graphicsContext);
  g_Windowing.Register(this);
  m_displayLost = false;
  m_displayReset = false;
  m_lostEvent.Reset();
  UpdateClock = func;

  //get d3d device
  m_D3dDev = g_Windowing.Get3DDevice();
  //we need a high priority thread to get accurate timing
  if (!SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL))
    CLog::Log(LOGDEBUG, "CVideoSyncD3D: SetThreadPriority failed");

  D3DCAPS9 DevCaps;
  ReturnV = m_D3dDev->GetDeviceCaps(&DevCaps);

  if (ReturnV != D3D_OK)
  {
    CLog::Log(LOGDEBUG, "CVideoSyncD3D: GetDeviceCaps returned %s: %s",
      DXGetErrorString(ReturnV), DXGetErrorDescription(ReturnV));
    return false;
  }

  if ((DevCaps.Caps & D3DCAPS_READ_SCANLINE) != D3DCAPS_READ_SCANLINE)
  {
    CLog::Log(LOGDEBUG, "CVideoSyncD3D: Hardware does not support GetRasterStatus");
    return false;
  }

  D3DRASTER_STATUS RasterStatus;
  ReturnV = m_D3dDev->GetRasterStatus(0, &RasterStatus);
  if (ReturnV != D3D_OK)
  {
    CLog::Log(LOGDEBUG, "CVideoSyncD3D: GetRasterStatus returned returned %s: %s",
      DXGetErrorString(ReturnV), DXGetErrorDescription(ReturnV));
    return false;
  }

  D3DDISPLAYMODE DisplayMode;
  ReturnV = m_D3dDev->GetDisplayMode(0, &DisplayMode);
  if (ReturnV != D3D_OK)
  {
    CLog::Log(LOGDEBUG, "CVideoSyncD3D: GetDisplayMode returned returned %s: %s",
      DXGetErrorString(ReturnV), DXGetErrorDescription(ReturnV));
    return false;
  }
  m_height = DisplayMode.Height;

  return true;
}

void CVideoSyncD3D::Run(volatile bool& stop)
{
  D3DRASTER_STATUS RasterStatus;
  int64_t Now;
  int64_t LastVBlankTime;
  unsigned int LastLine;
  int NrVBlanks;
  double VBlankTime;
  int ReturnV;
  int64_t systemFrequency = CurrentHostFrequency();

  //get the scanline we're currently at
  m_D3dDev->GetRasterStatus(0, &RasterStatus);
  if (RasterStatus.InVBlank)
    LastLine = 0;
  else
    LastLine = RasterStatus.ScanLine;

  //init the vblanktime
  Now = CurrentHostCounter();
  LastVBlankTime = Now;
  m_lastUpdateTime = Now - systemFrequency;
  while (!stop && !m_displayLost && !m_displayReset)
  {
    //get the scanline we're currently at
    ReturnV = m_D3dDev->GetRasterStatus(0, &RasterStatus);
    if (ReturnV != D3D_OK)
    {
      CLog::Log(LOGDEBUG, "CVideoSyncD3D: GetRasterStatus returned returned %s: %s",
        DXGetErrorString(ReturnV), DXGetErrorDescription(ReturnV));
      return;
    }

    //if InVBlank is set, or the current scanline is lower than the previous scanline, a vblank happened
    if ((RasterStatus.InVBlank && LastLine > 0) || (RasterStatus.ScanLine < LastLine))
    {
      //calculate how many vblanks happened
      Now = CurrentHostCounter() - systemFrequency * RasterStatus.ScanLine / (m_height * MathUtils::round_int(m_fps));
      VBlankTime = (double)(Now - LastVBlankTime) / (double)systemFrequency;
      NrVBlanks = MathUtils::round_int(VBlankTime * m_fps);
      //update the vblank timestamp, update the clock and send a signal that we got a vblank
      UpdateClock(NrVBlanks, Now);

      //save the timestamp of this vblank so we can calculate how many vblanks happened next time
      LastVBlankTime = Now;
      //because we had a vblank, sleep until half the refreshrate period
      Now = CurrentHostCounter();

      if ((Now - m_lastUpdateTime) >= systemFrequency)
      {
        if (m_fps != GetFps())
          break;
      }

      int SleepTime = (int)((LastVBlankTime + (systemFrequency / MathUtils::round_int(m_fps) / 2) - Now) * 1000 / systemFrequency);
      if (SleepTime > 100) 
        SleepTime = 100; //failsafe
      if (SleepTime > 0)
        ::Sleep(SleepTime);
    }
    else
    {
      ::Sleep(1);
    }

    if (RasterStatus.InVBlank)
      LastLine = 0;
    else
      LastLine = RasterStatus.ScanLine;
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
  CSingleLock lock(g_graphicsContext);

  m_lostEvent.Set();
  g_Windowing.Unregister(this);
}

float CVideoSyncD3D::GetFps()
{
  D3DDISPLAYMODE DisplayMode;
  m_D3dDev->GetDisplayMode(0, &DisplayMode);
  m_fps = DisplayMode.RefreshRate;
  if (m_fps == 0)
    m_fps = 60;
  
  if (m_fps == 23 || m_fps == 29 || m_fps == 59)
    m_fps++;

  if (g_Windowing.Interlaced())
  {
    m_fps *= 2;
  }
  return m_fps;
}
#endif
