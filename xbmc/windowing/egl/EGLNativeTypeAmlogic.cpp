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
#include "utils/SysfsUtils.h"

#include <stdio.h>
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
  std::string name;
  std::string modalias = "/sys/class/graphics/" + m_framebuffer_name + "/device/modalias";

  SysfsUtils::GetString(modalias, name);
  StringUtils::Trim(name);
  if (name == "platform:mesonfb")
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
  std::string mode;
  SysfsUtils::GetString("/sys/class/display/mode", mode);
  return aml_mode_to_resolution(mode.c_str(), res);
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
  std::string valstr;
  SysfsUtils::GetString("/sys/class/amhdmitx/amhdmitx0/disp_cap", valstr);
  std::vector<std::string> probe_str = StringUtils::Split(valstr, "\n");

  resolutions.clear();
  RESOLUTION_INFO res;
  for (std::vector<std::string>::const_iterator i = probe_str.begin(); i != probe_str.end(); ++i)
  {
    if(aml_mode_to_resolution(i->c_str(), &res))
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
    aml_mode_to_resolution("720p", res);
  }

  return true;
}

bool CEGLNativeTypeAmlogic::ShowWindow(bool show)
{
  std::string blank_framebuffer = "/sys/class/graphics/" + m_framebuffer_name + "/blank";
  SysfsUtils::SetInt(blank_framebuffer.c_str(), show ? 0 : 1);
  return true;
}

bool CEGLNativeTypeAmlogic::SetDisplayResolution(const char *resolution)
{
  std::string mode = resolution;
  // switch display resolution
  SysfsUtils::SetString("/sys/class/display/mode", mode.c_str());
  SetupVideoScaling(mode.c_str());

  return true;
}

void CEGLNativeTypeAmlogic::SetupVideoScaling(const char *mode)
{
  SysfsUtils::SetInt("/sys/class/graphics/fb0/blank",      1);
  SysfsUtils::SetInt("/sys/class/graphics/fb0/free_scale", 0);
  SysfsUtils::SetInt("/sys/class/graphics/fb1/free_scale", 0);
  SysfsUtils::SetInt("/sys/class/ppmgr/ppscaler",          0);

  if (strstr(mode, "1080"))
  {
    SysfsUtils::SetString("/sys/class/graphics/fb0/request2XScale", "8");
    SysfsUtils::SetString("/sys/class/graphics/fb1/scale_axis",     "1280 720 1920 1080");
    SysfsUtils::SetString("/sys/class/graphics/fb1/scale",          "0x10001");
  }
  else
  {
    SysfsUtils::SetString("/sys/class/graphics/fb0/request2XScale", "16 1280 720");
  }

  SysfsUtils::SetInt("/sys/class/graphics/fb0/blank", 0);
}

void CEGLNativeTypeAmlogic::EnableFreeScale()
{
  // enable OSD free scale using frame buffer size of 720p
  SysfsUtils::SetInt("/sys/class/graphics/fb0/free_scale", 0);
  SysfsUtils::SetInt("/sys/class/graphics/fb1/free_scale", 0);
  SysfsUtils::SetInt("/sys/class/graphics/fb0/scale_width",  1280);
  SysfsUtils::SetInt("/sys/class/graphics/fb0/scale_height", 720);
  SysfsUtils::SetInt("/sys/class/graphics/fb1/scale_width",  1280);
  SysfsUtils::SetInt("/sys/class/graphics/fb1/scale_height", 720);

  // enable video free scale (scaling to 1920x1080 with frame buffer size 1280x720)
  SysfsUtils::SetInt("/sys/class/ppmgr/ppscaler", 0);
  SysfsUtils::SetInt("/sys/class/video/disable_video", 1);
  SysfsUtils::SetInt("/sys/class/ppmgr/ppscaler", 1);
  SysfsUtils::SetString("/sys/class/ppmgr/ppscaler_rect", "0 0 1919 1079 0");
  SysfsUtils::SetString("/sys/class/ppmgr/disp", "1280 720");
  //
  SysfsUtils::SetInt("/sys/class/graphics/fb0/scale_width",  1280);
  SysfsUtils::SetInt("/sys/class/graphics/fb0/scale_height", 720);
  SysfsUtils::SetInt("/sys/class/graphics/fb1/scale_width",  1280);
  SysfsUtils::SetInt("/sys/class/graphics/fb1/scale_height", 720);
  //
  SysfsUtils::SetInt("/sys/class/video/disable_video", 2);
  SysfsUtils::SetString("/sys/class/display/axis", "0 0 1279 719 0 0 0 0");
  SysfsUtils::SetString("/sys/class/ppmgr/ppscaler_rect", "0 0 1279 719 1");
  //
  SysfsUtils::SetInt("/sys/class/graphics/fb0/free_scale", 1);
  SysfsUtils::SetInt("/sys/class/graphics/fb1/free_scale", 1);
  SysfsUtils::SetString("/sys/class/graphics/fb0/free_scale_axis", "0 0 1279 719");
}

void CEGLNativeTypeAmlogic::DisableFreeScale()
{
  // turn off frame buffer freescale
  SysfsUtils::SetInt("/sys/class/graphics/fb0/free_scale", 0);
  SysfsUtils::SetInt("/sys/class/graphics/fb1/free_scale", 0);
  SysfsUtils::SetString("/sys/class/graphics/fb0/free_scale_axis", "0 0 1279 719");

  SysfsUtils::SetInt("/sys/class/ppmgr/ppscaler", 0);
  SysfsUtils::SetInt("/sys/class/video/disable_video", 0);
  // now default video display to off
  SysfsUtils::SetInt("/sys/class/video/disable_video", 1);

  // revert display axis
  int fd0;
  std::string framebuffer = "/dev/" + m_framebuffer_name;

  if ((fd0 = open(framebuffer.c_str(), O_RDWR)) >= 0)
  {
    struct fb_var_screeninfo vinfo;
    if (ioctl(fd0, FBIOGET_VSCREENINFO, &vinfo) == 0)
    {
      char daxis_str[256] = {0};
      sprintf(daxis_str, "%d %d %d %d 0 0 0 0", 0, 0, vinfo.xres-1, vinfo.yres-1);
      SysfsUtils::SetString("/sys/class/display/axis", daxis_str);
    }
    close(fd0);
  }
}
