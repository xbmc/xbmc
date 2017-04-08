/*
 *      Copyright (C) 2005-2013 Team XBMC
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


#include "IMX.h"
#include <linux/mxcfb.h>

/*
 *  official imx6 -SR tree
 *  https://github.com/SolidRun/linux-fslc.git
 *
 */

#include <linux/mxc_dcic.h>
#include <sys/ioctl.h>

#include "guilib/GraphicContext.h"
#include "windowing/WindowingFactory.h"
#include "utils/log.h"
#include "guilib/GraphicContext.h"
#include "cores/VideoPlayer/DVDCodecs/DVDCodecUtils.h"

#define  DCIC_DEVICE    "/dev/mxc_dcic0"

CIMX::CIMX(void) : CThread("CIMX")
  , m_change(true)
  , m_lastSyncFlag(0)
{
  m_frameTime = (double)1300 / g_graphicsContext.GetFPS();
  g_Windowing.Register(this);
}

CIMX::~CIMX(void)
{
  g_Windowing.Unregister(this);
  Deinitialize();
}

bool CIMX::Initialize()
{
  m_change = true;
  CSingleLock lock(m_critSection);

  m_fddcic = open(DCIC_DEVICE, O_RDWR);
  if (m_fddcic < 0)
  {
    m_frameTime = (int) 1000 / 25;
    return false;
  }

  Create();
  return true;
}

void CIMX::Deinitialize()
{
  StopThread(false);
  CSingleLock lock(m_critSection);
  StopThread();

  if (m_fddcic > 0)
    ioctl(m_fddcic, DCIC_IOC_STOP_VSYNC, 0);

  if (m_fddcic > 0)
    close(m_fddcic);
  m_fddcic = 0;
}

bool CIMX::UpdateDCIC()
{
  struct fb_var_screeninfo screen_info;

  if (!m_change)
    return true;

  CSingleLock lock(m_critSection);
  m_change = false;

  int fb0 = open(FB_DEVICE, O_RDONLY | O_NONBLOCK);
  if (fb0 < 0)
    return false;

  int ret = ioctl(fb0, FBIOGET_VSCREENINFO, &screen_info);
  close(fb0);

  if (ret < 0)
    return false;
  else if (m_lastSyncFlag == screen_info.sync)
    return true;

  CLog::Log(LOGDEBUG, "CIMX::%s - Setting up screen_info parameters", __FUNCTION__);
  ioctl(m_fddcic, DCIC_IOC_STOP_VSYNC, 0);

  if (ioctl(m_fddcic, DCIC_IOC_CONFIG_DCIC, &screen_info.sync) < 0)
    return false;

  if (IsRunning())
    ioctl(m_fddcic, DCIC_IOC_START_VSYNC, 0);

  m_VblankEvent.Reset();
  m_lastSyncFlag = screen_info.sync;

  return true;
}

void CIMX::Process()
{
  ioctl(m_fddcic, DCIC_IOC_START_VSYNC, 0);
  while (!m_bStop)
  {
    read(m_fddcic, &m_counter, sizeof(unsigned long));
    m_VblankEvent.Set();
  }
  ioctl(m_fddcic, DCIC_IOC_STOP_VSYNC, 0);
}

int CIMX::WaitVsync()
{
  int diff;

  if (m_fddcic < 1)
    Initialize();

  if (!m_VblankEvent.WaitMSec(m_frameTime))
    m_counter++;

  diff = m_counter - m_counterLast;
  m_counterLast = m_counter;

  return std::max(0, diff);
}

void CIMX::OnResetDisplay()
{
  m_frameTime = (double)1300 / g_graphicsContext.GetFPS();
  m_change = true;
  UpdateDCIC();
}

bool CIMX::IsBlank()
{
  unsigned long curBlank;
  int fd = open(FB_DEVICE, O_RDONLY | O_NONBLOCK);
  bool ret = ioctl(fd, MXCFB_GET_FB_BLANK, &curBlank) || curBlank != FB_BLANK_UNBLANK;
  close(fd);
  return ret;
}

bool CIMXFps::Recalc()
{
  double prev = DVD_NOPTS_VALUE;
  unsigned int count = 0;
  double frameDuration = 0.0;
  bool hasMatch;

  std::sort(m_ts.begin(), m_ts.end());

  m_hgraph.clear();
  for (auto d : m_ts)
  {
    if (prev != DVD_NOPTS_VALUE)
    {
      frameDuration = CDVDCodecUtils::NormalizeFrameduration((d - prev), &hasMatch);
      if (fabs(frameDuration - rint(frameDuration)) < 0.01)
        frameDuration = rint(frameDuration);

      m_hgraph[(unsigned long)(frameDuration * 100)]++;
    }
    prev = d;
  }

  for (auto it = m_hgraph.begin(); it != m_hgraph.end();)
  {
    if (it->second > 1)
    {
      double duration = CDVDCodecUtils::NormalizeFrameduration((double)it->first / 100, &hasMatch);

      ++it;
    }
    else
    {
      for (auto iti = m_hgraph.begin(); it != iti; iti++)
      {
        if (!iti->first)
          continue;
        int dv = it->first / iti->first;
        if (dv * iti->first == it->first)
        {
          m_hgraph[it->first] += dv;
          break;
        }
      }
      m_hgraph.erase(it++);
    }
  }

  frameDuration = 0.0;
  for (auto h : m_hgraph)
  {
    count += h.second;
    frameDuration += h.first * h.second;
  }

  if (count)
    frameDuration /= (100 * count);

  frameDuration = CDVDCodecUtils::NormalizeFrameduration(frameDuration, &hasMatch);

  if (hasMatch)
    m_frameDuration = frameDuration;
  return true;
}

void CIMXFps::Add(double tm)
{
  m_ts.push_back(tm);
  if (m_ts.size() > DIFFRINGSIZE)
    m_ts.pop_front();
  Recalc();
}

void CIMXFps::Flush()
{
  m_frameDuration = DVD_NOPTS_VALUE;
  m_ts.clear();
}
