/*
 *      Copyright (C) 2011-2013 Team XBMC
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

#include "EGLNativeTypeAmlogic.h"
#include "guilib/gui3d.h"
#include "utils/AMLUtils.h"
#include "utils/StringUtils.h"

#include <stdlib.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <EGL/egl.h>

CEGLNativeTypeAmlogic::CEGLNativeTypeAmlogic()
{
  const char *env_framebuffer = getenv("FRAMEBUFFER");

  // default to framebuffer 0
  m_framebuffer_name = "fb0";
  if (env_framebuffer)
  {
    std::string framebuffer(env_framebuffer);
    std::string::size_type start = framebuffer.find("fb");
    m_framebuffer_name = framebuffer.substr(start);
  }
  m_nativeWindow = NULL;
}

CEGLNativeTypeAmlogic::~CEGLNativeTypeAmlogic()
{
}

bool CEGLNativeTypeAmlogic::CheckCompatibility()
{
  char name[256] = {0};
  std::string modalias = "/sys/class/graphics/" + m_framebuffer_name + "/device/modalias";

  aml_get_sysfs_str(modalias.c_str(), name, 255);
  CStdString strName = name;
  StringUtils::Trim(strName);
  if (strName == "platform:mesonfb")
    return true;
  return false;
}

void CEGLNativeTypeAmlogic::Initialize()
{
  aml_permissions();
  aml_cpufreq_min(true);
  aml_cpufreq_max(true);
  return;
}
void CEGLNativeTypeAmlogic::Destroy()
{
  aml_cpufreq_min(false);
  aml_cpufreq_max(false);
  return;
}

bool CEGLNativeTypeAmlogic::CreateNativeDisplay()
{
  m_nativeDisplay = EGL_DEFAULT_DISPLAY;
  return true;
}

bool CEGLNativeTypeAmlogic::CreateNativeWindow()
{
#if defined(_FBDEV_WINDOW_H_)
  fbdev_window *nativeWindow = new fbdev_window;
  if (!nativeWindow)
    return false;

  nativeWindow->width = 1280;
  nativeWindow->height = 720;
  m_nativeWindow = nativeWindow;
  return true;
#else
  return false;
#endif
}

bool CEGLNativeTypeAmlogic::GetNativeDisplay(XBNativeDisplayType **nativeDisplay) const
{
  if (!nativeDisplay)
    return false;
  *nativeDisplay = (XBNativeDisplayType*) &m_nativeDisplay;
  return true;
}

bool CEGLNativeTypeAmlogic::GetNativeWindow(XBNativeWindowType **nativeWindow) const
{
  if (!nativeWindow)
    return false;
  *nativeWindow = (XBNativeWindowType*) &m_nativeWindow;
  return true;
}

bool CEGLNativeTypeAmlogic::DestroyNativeDisplay()
{
  return true;
}

bool CEGLNativeTypeAmlogic::DestroyNativeWindow()
{
#if defined(_FBDEV_WINDOW_H_)
  delete (fbdev_window*)m_nativeWindow, m_nativeWindow = NULL;
#endif
  return true;
}

bool CEGLNativeTypeAmlogic::GetNativeResolution(RESOLUTION_INFO *res) const
{
  char mode[256] = {0};
  aml_get_sysfs_str("/sys/class/display/mode", mode, 255);
  return ModeToResolution(mode, res);
}

bool CEGLNativeTypeAmlogic::SetNativeResolution(const RESOLUTION_INFO &res)
{
  switch((int)(0.5 + res.fRefreshRate))
  {
    default:
    case 60:
      switch(res.iScreenWidth)
      {
        default:
        case 1280:
          SetDisplayResolution("720p");
          break;
        case 1920:
          if (res.dwFlags & D3DPRESENTFLAG_INTERLACED)
            SetDisplayResolution("1080i");
          else
            SetDisplayResolution("1080p");
          break;
      }
      break;
    case 50:
      switch(res.iScreenWidth)
      {
        default:
        case 1280:
          SetDisplayResolution("720p50hz");
          break;
        case 1920:
          if (res.dwFlags & D3DPRESENTFLAG_INTERLACED)
            SetDisplayResolution("1080i50hz");
          else
            SetDisplayResolution("1080p50hz");
          break;
      }
      break;
    case 30:
      SetDisplayResolution("1080p30hz");
      break;
    case 24:
      SetDisplayResolution("1080p24hz");
      break;
  }

  return true;
}

bool CEGLNativeTypeAmlogic::ProbeResolutions(std::vector<RESOLUTION_INFO> &resolutions)
{
  char valstr[256] = {0};
  aml_get_sysfs_str("/sys/class/amhdmitx/amhdmitx0/disp_cap", valstr, 255);
  std::vector<CStdString> probe_str;
  StringUtils::SplitString(valstr, "\n", probe_str);

  resolutions.clear();
  RESOLUTION_INFO res;
  for (size_t i = 0; i < probe_str.size(); i++)
  {
    if(ModeToResolution(probe_str[i].c_str(), &res))
      resolutions.push_back(res);
  }
  return resolutions.size() > 0;

}

bool CEGLNativeTypeAmlogic::GetPreferredResolution(RESOLUTION_INFO *res) const
{
  // check display/mode, it gets defaulted at boot
  if (!GetNativeResolution(res))
  {
    // punt to 720p if we get nothing
    ModeToResolution("720p", res);
  }

  return true;
}

bool CEGLNativeTypeAmlogic::ShowWindow(bool show)
{
  std::string blank_framebuffer = "/sys/class/graphics/" + m_framebuffer_name + "/blank";
  aml_set_sysfs_int(blank_framebuffer.c_str(), show ? 0 : 1);
  return true;
}

bool CEGLNativeTypeAmlogic::SetDisplayResolution(const char *resolution)
{
  CStdString modestr = resolution;
  // switch display resolution
  aml_set_sysfs_str("/sys/class/display/mode", modestr.c_str());

  // setup gui freescale depending on display resolution
  DisableFreeScale();
  if (StringUtils::StartsWith(modestr, "1080"))
  {
    EnableFreeScale();
  }

  return true;
}

bool CEGLNativeTypeAmlogic::ModeToResolution(const char *mode, RESOLUTION_INFO *res) const
{
  if (!res)
    return false;

  res->iWidth = 0;
  res->iHeight= 0;

  if(!mode)
    return false;

  CStdString fromMode = mode;
  StringUtils::Trim(fromMode);
  // strips, for example, 720p* to 720p
  // the * indicate the 'native' mode of the display
  if (StringUtils::EndsWith(fromMode, "*"))
    fromMode.erase(fromMode.size() - 1);

  if (fromMode.Equals("720p"))
  {
    res->iWidth = 1280;
    res->iHeight= 720;
    res->iScreenWidth = 1280;
    res->iScreenHeight= 720;
    res->fRefreshRate = 60;
    res->dwFlags = D3DPRESENTFLAG_PROGRESSIVE;
  }
  else if (fromMode.Equals("720p50hz"))
  {
    res->iWidth = 1280;
    res->iHeight= 720;
    res->iScreenWidth = 1280;
    res->iScreenHeight= 720;
    res->fRefreshRate = 50;
    res->dwFlags = D3DPRESENTFLAG_PROGRESSIVE;
  }
  else if (fromMode.Equals("1080p"))
  {
    res->iWidth = 1280;
    res->iHeight= 720;
    res->iScreenWidth = 1920;
    res->iScreenHeight= 1080;
    res->fRefreshRate = 60;
    res->dwFlags = D3DPRESENTFLAG_PROGRESSIVE;
  }
  else if (fromMode.Equals("1080p24hz"))
  {
    res->iWidth = 1280;
    res->iHeight= 720;
    res->iScreenWidth = 1920;
    res->iScreenHeight= 1080;
    res->fRefreshRate = 24;
    res->dwFlags = D3DPRESENTFLAG_PROGRESSIVE;
  }
  else if (fromMode.Equals("1080p30hz"))
  {
    res->iWidth = 1280;
    res->iHeight= 720;
    res->iScreenWidth = 1920;
    res->iScreenHeight= 1080;
    res->fRefreshRate = 30;
    res->dwFlags = D3DPRESENTFLAG_PROGRESSIVE;
  }
  else if (fromMode.Equals("1080p50hz"))
  {
    res->iWidth = 1280;
    res->iHeight= 720;
    res->iScreenWidth = 1920;
    res->iScreenHeight= 1080;
    res->fRefreshRate = 50;
    res->dwFlags = D3DPRESENTFLAG_PROGRESSIVE;
  }
  else if (fromMode.Equals("1080i"))
  {
    res->iWidth = 1280;
    res->iHeight= 720;
    res->iScreenWidth = 1920;
    res->iScreenHeight= 1080;
    res->fRefreshRate = 60;
    res->dwFlags = D3DPRESENTFLAG_INTERLACED;
  }
  else if (fromMode.Equals("1080i50hz"))
  {
    res->iWidth = 1280;
    res->iHeight= 720;
    res->iScreenWidth = 1920;
    res->iScreenHeight= 1080;
    res->fRefreshRate = 50;
    res->dwFlags = D3DPRESENTFLAG_INTERLACED;
  }
  else
  {
    return false;
  }


  res->iScreen       = 0;
  res->bFullScreen   = true;
  res->iSubtitles    = (int)(0.965 * res->iHeight);
  res->fPixelRatio   = 1.0f;
  res->strMode       = StringUtils::Format("%dx%d @ %.2f%s - Full Screen", res->iScreenWidth, res->iScreenHeight, res->fRefreshRate,
    res->dwFlags & D3DPRESENTFLAG_INTERLACED ? "i" : "");

  return res->iWidth > 0 && res->iHeight> 0;
}

void CEGLNativeTypeAmlogic::EnableFreeScale()
{
  // enable OSD free scale using frame buffer size of 720p
  aml_set_sysfs_int("/sys/class/graphics/fb0/free_scale", 0);
  aml_set_sysfs_int("/sys/class/graphics/fb1/free_scale", 0);
  aml_set_sysfs_int("/sys/class/graphics/fb0/scale_width",  1280);
  aml_set_sysfs_int("/sys/class/graphics/fb0/scale_height", 720);
  aml_set_sysfs_int("/sys/class/graphics/fb1/scale_width",  1280);
  aml_set_sysfs_int("/sys/class/graphics/fb1/scale_height", 720);

  // enable video free scale (scaling to 1920x1080 with frame buffer size 1280x720)
  aml_set_sysfs_int("/sys/class/ppmgr/ppscaler", 0);
  aml_set_sysfs_int("/sys/class/video/disable_video", 1);
  aml_set_sysfs_int("/sys/class/ppmgr/ppscaler", 1);
  aml_set_sysfs_str("/sys/class/ppmgr/ppscaler_rect", "0 0 1919 1079 0");
  aml_set_sysfs_str("/sys/class/ppmgr/disp", "1280 720");
  //
  aml_set_sysfs_int("/sys/class/graphics/fb0/scale_width",  1280);
  aml_set_sysfs_int("/sys/class/graphics/fb0/scale_height", 720);
  aml_set_sysfs_int("/sys/class/graphics/fb1/scale_width",  1280);
  aml_set_sysfs_int("/sys/class/graphics/fb1/scale_height", 720);
  //
  aml_set_sysfs_int("/sys/class/video/disable_video", 2);
  aml_set_sysfs_str("/sys/class/display/axis", "0 0 1279 719 0 0 0 0");
  aml_set_sysfs_str("/sys/class/ppmgr/ppscaler_rect", "0 0 1279 719 1");
  //
  aml_set_sysfs_int("/sys/class/graphics/fb0/free_scale", 1);
  aml_set_sysfs_int("/sys/class/graphics/fb1/free_scale", 1);
  aml_set_sysfs_str("/sys/class/graphics/fb0/free_scale_axis", "0 0 1279 719");
}

void CEGLNativeTypeAmlogic::DisableFreeScale()
{
  // turn off frame buffer freescale
  aml_set_sysfs_int("/sys/class/graphics/fb0/free_scale", 0);
  aml_set_sysfs_int("/sys/class/graphics/fb1/free_scale", 0);
  aml_set_sysfs_str("/sys/class/graphics/fb0/free_scale_axis", "0 0 1279 719");

  aml_set_sysfs_int("/sys/class/ppmgr/ppscaler", 0);
  aml_set_sysfs_int("/sys/class/video/disable_video", 0);
  // now default video display to off
  aml_set_sysfs_int("/sys/class/video/disable_video", 1);

  // revert display axis
  int fd0;
  std::string framebuffer = "/dev/" + m_framebuffer_name;

  if ((fd0 = open(framebuffer.c_str(), O_RDWR)) >= 0)
  {
    struct fb_var_screeninfo vinfo;
    if (ioctl(fd0, FBIOGET_VSCREENINFO, &vinfo) == 0)
    {
      char daxis_str[255] = {0};
      sprintf(daxis_str, "%d %d %d %d 0 0 0 0", 0, 0, vinfo.xres-1, vinfo.yres-1);
      aml_set_sysfs_str("/sys/class/display/axis", daxis_str);
    }
    close(fd0);
  }
}
