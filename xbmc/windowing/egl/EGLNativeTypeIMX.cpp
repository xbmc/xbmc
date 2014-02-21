/*
 *      Copyright (C) 2011-2013 Team XBMC
 *      http://www.xbmc.org
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

#include <linux/mxcfb.h>
#include "system.h"
#include <EGL/egl.h>

#include "EGLNativeTypeIMX.h"
#include <math.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "guilib/gui3d.h"

CEGLNativeTypeIMX::CEGLNativeTypeIMX()
{
}

CEGLNativeTypeIMX::~CEGLNativeTypeIMX()
{
} 

bool CEGLNativeTypeIMX::CheckCompatibility()
{
  char name[256] = {0};
  get_sysfs_str("/sys/class/graphics/fb0/device/modalias", name, 255);
  CStdString strName = name;
  StringUtils::Trim(strName);
  if (strName == "platform:mxc_sdc_fb")
    return true;
  return false;
}

void CEGLNativeTypeIMX::Initialize()
{  
  struct mxcfb_gbl_alpha alpha;
  int fd;

  
  fd = open("/dev/fb0",O_RDWR);
  if (fd < 0)
  {
    CLog::Log(LOGERROR, "%s - Error while opening /dev/fb0.\n", __FUNCTION__);
    return;
  }
  // Store screen info
  if (ioctl(fd, FBIOGET_VSCREENINFO, &m_screeninfo) != 0)
  {
    CLog::Log(LOGERROR, "%s - Error while querying frame buffer.\n", __FUNCTION__);
    return;
  }
      
  // Unblank the fbs
  if (ioctl(fd, FBIOBLANK, 0) < 0)
  {
    CLog::Log(LOGERROR, "%s - Error while unblanking fb0.\n", __FUNCTION__);
  }
  
  close(fd);
  
  return;
}

void CEGLNativeTypeIMX::Destroy()
{
  struct fb_fix_screeninfo fixed_info;
  void *fb_buffer;
  int fd;

  fd = open("/dev/fb0",O_RDWR);
  if (fd < 0)
  {
    CLog::Log(LOGERROR, "%s - Error while opening /dev/fb0.\n", __FUNCTION__);
    return;
  }   
  
  ioctl( fd, FBIOGET_FSCREENINFO, &fixed_info);  
  // Black fb0
  fb_buffer = mmap(NULL, fixed_info.smem_len, PROT_WRITE, MAP_SHARED, fd, 0);
  if (fb_buffer == MAP_FAILED)
  {
    CLog::Log(LOGERROR, "%s - fb mmap failed %s.\n", __FUNCTION__, strerror(errno));
  }
  else 
  {
    memset(fb_buffer, 0x0, fixed_info.smem_len);
    munmap(fb_buffer, fixed_info.smem_len);
  }
 
  close(fd); 
  
  return;
}

bool CEGLNativeTypeIMX::CreateNativeDisplay()
{
  // EGL will be rendered on fb0
  m_display = fbGetDisplayByIndex(0);
  m_nativeDisplay = &m_display;
  return true;
}

bool CEGLNativeTypeIMX::CreateNativeWindow()
{
  m_window = fbCreateWindow(m_display, 0, 0, m_screeninfo.xres, m_screeninfo.yres);
  m_nativeWindow = &m_window;
  return true;
}  

bool CEGLNativeTypeIMX::GetNativeDisplay(XBNativeDisplayType **nativeDisplay) const
{
  if (!nativeDisplay)
    return false;
  *nativeDisplay = (XBNativeDisplayType*)m_nativeDisplay;
  return true;
}

bool CEGLNativeTypeIMX::GetNativeWindow(XBNativeWindowType **nativeWindow) const
{
  if (!nativeWindow)
    return false;
  *nativeWindow = (XBNativeWindowType*)m_nativeWindow;
  return true;
}

bool CEGLNativeTypeIMX::DestroyNativeDisplay()
{
  return true;
}

bool CEGLNativeTypeIMX::DestroyNativeWindow()
{
  return true;
}

bool CEGLNativeTypeIMX::GetNativeResolution(RESOLUTION_INFO *res) const
{
  double drate = 0, hrate = 0, vrate = 0;
  if (!res)
    return false;

  drate = 1e12 / m_screeninfo.pixclock;
  hrate = drate / (m_screeninfo.left_margin + m_screeninfo.xres +  m_screeninfo.right_margin + m_screeninfo.hsync_len);
  vrate = hrate / (m_screeninfo.upper_margin + m_screeninfo.yres + m_screeninfo.lower_margin + m_screeninfo.vsync_len);

  res->iWidth = m_screeninfo.xres;
  res->iHeight = m_screeninfo.yres;
  res->iScreenWidth  = res->iWidth;
  res->iScreenHeight = res->iHeight;
  res->fRefreshRate = lrint(vrate);
  res->dwFlags= D3DPRESENTFLAG_PROGRESSIVE;
  res->iScreen       = 0;
  res->bFullScreen   = true;
  res->iSubtitles    = (int)(0.965 * res->iHeight);
  res->fPixelRatio   = 1.0f;
  res->strMode = StringUtils::Format("%dx%d @ %.2f%s - Full Screen", res->iScreenWidth, res->iScreenHeight, res->fRefreshRate,
  res->dwFlags & D3DPRESENTFLAG_INTERLACED ? "i" : "");

  return res->iWidth > 0 && res->iHeight> 0;
}

bool CEGLNativeTypeIMX::SetNativeResolution(const RESOLUTION_INFO &res)
{
  return false;
}

bool CEGLNativeTypeIMX::ProbeResolutions(std::vector<RESOLUTION_INFO> &resolutions)
{
  RESOLUTION_INFO res;
  bool ret = false;
  ret = GetNativeResolution(&res);
  if (ret && res.iWidth > 1 && res.iHeight > 1)
  {
    resolutions.push_back(res);
    return true;
  }
  return false;
}

bool CEGLNativeTypeIMX::GetPreferredResolution(RESOLUTION_INFO *res) const
{
  return false;
}

bool CEGLNativeTypeIMX::ShowWindow(bool show)
{
  // CLog::Log(LOGERROR, "%s - call CEGLNativeTypeIMX::ShowWindow with %d.\n", __FUNCTION__, show);
  return false;
}

int CEGLNativeTypeIMX::get_sysfs_str(const char *path, char *valstr, const int size) const
{
  int fd = open(path, O_RDONLY);
  if (fd >= 0)
  {
    int len = read(fd, valstr, size - 1);
    if (len != -1 )
      valstr[len] = '\0';
    close(fd);
  }
  else
  {
    sprintf(valstr, "%s", "fail");
    return -1;
  }
  return 0;
}
