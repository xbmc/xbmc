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

#include <cstdlib>

#include "system.h"
#include <EGL/egl.h>

#include "EGLNativeTypeIMX.h"
#include <math.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#ifdef HAS_IMXVPU
#include <linux/mxcfb.h>
#endif
#include "utils/log.h"
#include "utils/RegExp.h"
#include "utils/StringUtils.h"
#include "utils/SysfsUtils.h"
#include "utils/Environment.h"
#include "guilib/gui3d.h"
#include "windowing/WindowingFactory.h"
#include "cores/AudioEngine/AEFactory.h"
#include <fstream>

CEGLNativeTypeIMX::CEGLNativeTypeIMX()
  : m_display(NULL)
  , m_window(NULL)
{
}

CEGLNativeTypeIMX::~CEGLNativeTypeIMX()
{
}

bool CEGLNativeTypeIMX::CheckCompatibility()
{
  std::ifstream file("/sys/class/graphics/fb0/fsl_disp_dev_property");
  return file;
}

void CEGLNativeTypeIMX::Initialize()
{
  int fd;

  // Check if we can change the framebuffer resolution
  fd = open("/sys/class/graphics/fb0/mode", O_RDWR);
  if (fd >= 0)
  {
    CLog::Log(LOGNOTICE, "%s - graphics sysfs is writable\n", __FUNCTION__);
    m_readonly = false;
  }
  else
  {
    CLog::Log(LOGNOTICE, "%s - graphics sysfs is read-only\n", __FUNCTION__);
    m_readonly = true;
  }
  close(fd);

  bool alphaBlending = false;
  std::string bpp;
  if (SysfsUtils::GetString("/sys/class/graphics/fb0/bits_per_pixel", bpp))
  {
    CLog::Log(LOGWARNING, "%s - determining current bits per pixel failed, assuming 16bpp\n", __FUNCTION__);
  }
  else
  {
    StringUtils::Trim(bpp);
    if (bpp == "32")
    {
      CLog::Log(LOGNOTICE, "%s - 32bpp: configure alpha blending\n", __FUNCTION__);
      alphaBlending = true;
    }
    else
    {
      CLog::Log(LOGNOTICE, "%s - %sbpp: configure color keying\n", __FUNCTION__, bpp.c_str());
    }
  }

  fd = open("/dev/fb0",O_RDWR);
  if (fd < 0)
  {
    CLog::Log(LOGERROR, "%s - Error while opening /dev/fb0.\n", __FUNCTION__);
    return;
  }
#ifdef HAS_IMXVPU
  struct mxcfb_color_key colorKey;
  struct mxcfb_gbl_alpha gbl_alpha;
  struct mxcfb_loc_alpha lalpha;
  memset(&lalpha, 0, sizeof(lalpha));

  // Configure local alpha
  lalpha.enable = alphaBlending?1:0;
  lalpha.alpha_in_pixel = 1;
  if (ioctl(fd, MXCFB_SET_LOC_ALPHA, &lalpha) < 0)
    CLog::Log(LOGERROR, "%s - Failed to setup alpha blending\n", __FUNCTION__);

  gbl_alpha.alpha = 255;
  gbl_alpha.enable = alphaBlending?0:1;
  if (ioctl(fd, MXCFB_SET_GBL_ALPHA, &gbl_alpha) < 0)
    CLog::Log(LOGERROR, "%s - Failed to setup global alpha\n", __FUNCTION__);

  colorKey.enable = alphaBlending?0:1;
  colorKey.color_key = (16 << 16)|(8 << 8)|16;
  if (ioctl(fd, MXCFB_SET_CLR_KEY, &colorKey) < 0)
    CLog::Log(LOGERROR, "%s - Failed to setup color keying\n", __FUNCTION__);
#endif
  // Unblank the fb
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
  // Force double-buffering
  CEnvironment::setenv("FB_MULTI_BUFFER", "2", 0);

#ifdef HAS_IMXVPU
  // EGL will be rendered on fb0
  m_display = fbGetDisplayByIndex(0);
  m_nativeDisplay = &m_display;
  return true;
#else
  return false;
#endif
}

bool CEGLNativeTypeIMX::CreateNativeWindow()
{
#ifdef HAS_IMXVPU
  m_window = fbCreateWindow(m_display, 0, 0, 0, 0);
  m_nativeWindow = &m_window;
  return true;
#else
  return false;
#endif
}

bool CEGLNativeTypeIMX::GetNativeDisplay(XBNativeDisplayType **nativeDisplay) const
{
  if (!nativeDisplay)
    return false;
  if (!m_nativeDisplay)
    return false;
  *nativeDisplay = (XBNativeDisplayType*)m_nativeDisplay;
  return true;
}

bool CEGLNativeTypeIMX::GetNativeWindow(XBNativeWindowType **nativeWindow) const
{
  if (!nativeWindow)
    return false;
  if (!m_nativeWindow || !m_window)
    return false;
  *nativeWindow = (XBNativeWindowType*)m_nativeWindow;
  return true;
}

bool CEGLNativeTypeIMX::DestroyNativeDisplay()
{
#ifdef HAS_IMXVPU
  if (m_display)
    fbDestroyDisplay(m_display);
  m_display =  NULL;
  return true;
#else
  return false;
#endif
}

bool CEGLNativeTypeIMX::DestroyNativeWindow()
{
#ifdef HAS_IMXVPU
  if (m_window)
    fbDestroyWindow(m_window);
  m_window =  NULL;
  return true;
#else
  return false;
#endif
}

bool CEGLNativeTypeIMX::GetNativeResolution(RESOLUTION_INFO *res) const
{
  std::string mode;
  SysfsUtils::GetString("/sys/class/graphics/fb0/mode", mode);
  return ModeToResolution(mode, res);
}

bool CEGLNativeTypeIMX::SetNativeResolution(const RESOLUTION_INFO &res)
{
  if (m_readonly)
    return false;

  std::string mode;
  SysfsUtils::GetString("/sys/class/graphics/fb0/mode", mode);
  if (res.strId == mode)
    return false;

  DestroyNativeWindow();
  DestroyNativeDisplay();

  SysfsUtils::SetString("/sys/class/graphics/fb0/mode", res.strId + "\n");

  CreateNativeDisplay();

  CLog::Log(LOGDEBUG, "%s: %s",__FUNCTION__, res.strId.c_str());

  // Reset AE
  CAEFactory::DeviceChange();

  return true;
}

bool CEGLNativeTypeIMX::ProbeResolutions(std::vector<RESOLUTION_INFO> &resolutions)
{
  if (m_readonly)
    return false;

  std::string valstr;
  SysfsUtils::GetString("/sys/class/graphics/fb0/modes", valstr);
  std::vector<std::string> probe_str;
  probe_str = StringUtils::Split(valstr, "\n");

  resolutions.clear();
  RESOLUTION_INFO res;
  for (size_t i = 0; i < probe_str.size(); i++)
  {
    if(!StringUtils::StartsWith(probe_str[i], "S:"))
      continue;
    if(ModeToResolution(probe_str[i], &res))
      resolutions.push_back(res);
  }
  return resolutions.size() > 0;
}

bool CEGLNativeTypeIMX::GetPreferredResolution(RESOLUTION_INFO *res) const
{
  return GetNativeResolution(res);
}

bool CEGLNativeTypeIMX::ShowWindow(bool show)
{
  // Force vsync by default
  eglSwapInterval(g_Windowing.GetEGLDisplay(), 1);
  EGLint result = eglGetError();
  if(result != EGL_SUCCESS)
    CLog::Log(LOGERROR, "EGL error in %s: %x",__FUNCTION__, result);

  return false;
}

bool CEGLNativeTypeIMX::ModeToResolution(std::string mode, RESOLUTION_INFO *res) const
{
  if (!res)
    return false;

  res->iWidth = 0;
  res->iHeight= 0;

  if(mode.empty())
    return false;

  std::string fromMode = StringUtils::Mid(mode, 2);
  StringUtils::Trim(fromMode);

  CRegExp split(true);
  split.RegComp("([0-9]+)x([0-9]+)([pi])-([0-9]+)");
  if (split.RegFind(fromMode) < 0)
    return false;

  int w = atoi(split.GetMatch(1).c_str());
  int h = atoi(split.GetMatch(2).c_str());
  std::string p = split.GetMatch(3);
  int r = atoi(split.GetMatch(4).c_str());

  res->iWidth = w;
  res->iHeight= h;
  res->iScreenWidth = w;
  res->iScreenHeight= h;
  res->fRefreshRate = r;
  res->dwFlags = p[0] == 'p' ? D3DPRESENTFLAG_PROGRESSIVE : D3DPRESENTFLAG_INTERLACED;

  res->iScreen       = 0;
  res->bFullScreen   = true;
  res->iSubtitles    = (int)(0.965 * res->iHeight);
  res->fPixelRatio   = 1.0f;
  res->strMode       = StringUtils::Format("%dx%d @ %.2f%s - Full Screen", res->iScreenWidth, res->iScreenHeight, res->fRefreshRate,
                                           res->dwFlags & D3DPRESENTFLAG_INTERLACED ? "i" : "");
  res->strId         = mode;

  return res->iWidth > 0 && res->iHeight> 0;
}

