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
#include <algorithm>

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
#include "utils/MathUtils.h"
#include "DVDClock.h"
#include "cores/VideoPlayer/DVDCodecs/DVDCodecUtils.h"

#include <algorithm>

#define  DCIC_DEVICE    "/dev/mxc_dcic0"
#define  FB_DEVICE      "/dev/fb0"

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
  CSingleLock lock(m_critSection);

  m_fddcic = open(DCIC_DEVICE, O_RDWR);
  if (m_fddcic < 0)
  {
    m_frameTime = 0;
    return false;
  }

  Create();

  return true;
}

void CIMX::Deinitialize()
{
  CSingleLock lock(m_critSection);

  StopThread();
  m_VblankEvent.Set();

  if (m_fddcic > 0)
    close(m_fddcic);
}

bool CIMX::UpdateDCIC()
{
  struct fb_var_screeninfo screen_info;

  if (!m_change)
    return true;

  CSingleLock lock(m_critSection);

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
  m_change = false;
  m_lastSyncFlag = screen_info.sync;

  return true;
}

void CIMX::Process()
{
  ioctl(m_fddcic, DCIC_IOC_START_VSYNC, 0);
  while (!m_bStop)
  {
    if (m_change && !UpdateDCIC())
    {
      CLog::Log(LOGERROR, "CIMX::%s - Error occured. Exiting. Probably will need to reinitialize.", __FUNCTION__);
      break;
    }

    read(m_fddcic, &m_counter, sizeof(unsigned long));
    m_VblankEvent.Set();
  }
  ioctl(m_fddcic, DCIC_IOC_STOP_VSYNC, 0);
}

int CIMX::WaitVsync()
{
  int diff;

  if (!IsRunning())
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
    if (d != 0.0 && prev != DVD_NOPTS_VALUE)
      m_hgraph[MathUtils::round_int(d - prev)]++;
    prev = d;
  }

  unsigned int patternLength = 0;
  for (auto it = m_hgraph.begin(); it != m_hgraph.end();)
  {
    if (it->second > 1)
    {
      count += it->second;
      frameDuration += it->first * it->second;
      ++it;
    }
    else
      m_hgraph.erase(it++);
  }

  if (count)
    frameDuration /= count;

  double frameNorm = CDVDCodecUtils::NormalizeFrameduration(frameDuration, &hasMatch);

  if (hasMatch && !patternLength)
    m_patternLength = 1;
  else
    m_patternLength = patternLength;

  if (!m_hasPattern && hasMatch)
    m_frameDuration = frameNorm;

  if ((m_ts.size() == DIFFRINGSIZE && !m_hasPattern && hasMatch))
    m_hasPattern = true;

  if (m_hasPattern)
    m_ptscorrection = (m_ts.size() - 1) * m_frameDuration + m_ts.front() - m_ts.back();

  if (m_hasPattern && m_ts.size() == DIFFRINGSIZE && m_ptscorrection > m_frameDuration / 4)
  {
    m_hasPattern = false;
    m_frameDuration = DVD_NOPTS_VALUE;
  }

  return m_hgraph.size() <= 2;
  bool ret = m_hgraph.size() <= 2;
  if (!m_hasPattern && ret)
    m_frameDuration = frameNorm;
  return ret;
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
  m_ts.clear();
  m_frameDuration = 0.0;
  m_ptscorrection = 0.0;
  m_hasPattern = false;
}
