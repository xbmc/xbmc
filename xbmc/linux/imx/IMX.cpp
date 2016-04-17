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

#define  DCIC_DEVICE    "/dev/mxc_dcic0"
#define  FB_DEVICE      "/dev/fb0"

CIMX::CIMX(void) : CThread("CIMX"), m_change(true), m_frameTime(1000)
{
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

  m_VblankEvent.WaitMSec(m_frameTime);

  diff = m_counter - m_counterLast;
  m_counterLast = m_counter;

  return std::max(0, diff);
}

void CIMX::OnResetDisplay()
{
  m_change = true;
}
