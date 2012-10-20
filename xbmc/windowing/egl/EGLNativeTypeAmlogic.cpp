/*
 *      Copyright (C) 2011-2012 Team XBMC
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
#include <EGL/egl.h>
#include "EGLNativeTypeAmlogic.h"
#include <stdlib.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include "utils/StringUtils.h"
#include "guilib/gui3d.h"
#define m_framebuffer_name "fb0"

CEGLNativeTypeAmlogic::CEGLNativeTypeAmlogic()
{
}

CEGLNativeTypeAmlogic::~CEGLNativeTypeAmlogic()
{
}

bool CEGLNativeTypeAmlogic::CheckCompatibility()
{
  char name[256] = {0};
  get_sysfs_str("/sys/class/graphics/fb0/device/modalias", name, 255);
  CStdString strName = name;
  strName.Trim();
  if (strName == "platform:mesonfb")
    return true;
  return false;
}

void CEGLNativeTypeAmlogic::Initialize()
{
  SetCpuMinLimit(true);
  return;
}
void CEGLNativeTypeAmlogic::Destroy()
{
  SetCpuMinLimit(false);
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
  free(m_nativeWindow);
  return true;
}

bool CEGLNativeTypeAmlogic::GetNativeResolution(RESOLUTION_INFO *res) const
{
  char mode[256] = {0};
  get_sysfs_str("/sys/class/display/mode", mode, 255);
  return ModeToResolution(mode, res);
}

bool CEGLNativeTypeAmlogic::SetNativeResolution(const RESOLUTION_INFO &res)
{
  if (res.iScreenWidth == 1920 && res.iScreenHeight == 1080)
  {
    if (res.dwFlags & D3DPRESENTFLAG_INTERLACED)
    {
      if ((int)res.fRefreshRate == 60)
        SetDisplayResolution("1080i");
      else
        SetDisplayResolution("1080i50hz");
    }
    else
    {
      if ((int)res.fRefreshRate == 60)
        SetDisplayResolution("1080p");
      else
        SetDisplayResolution("1080p50hz");
    }
  }
  else if (res.iScreenWidth == 1280 && res.iScreenHeight == 720)
  {
    if ((int)res.fRefreshRate == 60)
      SetDisplayResolution("720p");
    else
      SetDisplayResolution("720p50hz");
  }
  else if (res.iScreenWidth == 720  && res.iScreenHeight == 480)
  {
    SetDisplayResolution("480p");
  }
  return true;
}

bool CEGLNativeTypeAmlogic::ProbeResolutions(std::vector<RESOLUTION_INFO> &resolutions)
{
  char valstr[256] = {0};
  get_sysfs_str("/sys/class/amhdmitx/amhdmitx0/disp_cap", valstr, 255);
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
  res->iWidth = 1280;
  res->iHeight= 720;
  res->fRefreshRate = 60;
  res->dwFlags= D3DPRESENTFLAG_PROGRESSIVE;
  res->iScreen       = 0;
  res->bFullScreen   = true;
  res->iSubtitles    = (int)(0.965 * res->iHeight);
  res->fPixelRatio   = 1.0f;
  res->iScreenWidth  = res->iWidth;
  res->iScreenHeight = res->iHeight;
  res->strMode.Format("%dx%d @ %.2f%s - Full Screen", res->iScreenWidth, res->iScreenHeight, res->fRefreshRate,
     res->dwFlags & D3DPRESENTFLAG_INTERLACED ? "i" : "");
  return true;
}

bool CEGLNativeTypeAmlogic::ShowWindow(bool show)
{
  std::string blank_framebuffer = "/sys/class/graphics/fb0/blank";
  set_sysfs_int(blank_framebuffer.c_str(), show ? 0 : 1);
  return true;
}

int CEGLNativeTypeAmlogic::get_sysfs_str(const char *path, char *valstr, const int size) const
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

int CEGLNativeTypeAmlogic::set_sysfs_str(const char *path, const char *val) const
{
  int fd = open(path, O_CREAT | O_RDWR | O_TRUNC, 0644);
  if (fd >= 0)
  {
    write(fd, val, strlen(val));
    close(fd);
    return 0;
  }
  return -1;
}

int CEGLNativeTypeAmlogic::set_sysfs_int(const char *path, const int val) const
{
  char bcmd[16];
  int fd = open(path, O_CREAT | O_RDWR | O_TRUNC, 0644);
  if (fd >= 0)
  {
    sprintf(bcmd, "%d", val);
    write(fd, bcmd, strlen(bcmd));
    close(fd);
    return 0;
  }
  return -1;
}

int CEGLNativeTypeAmlogic::get_sysfs_int(const char *path) const
{
  int val = 0;
  char bcmd[16];
  int fd = open(path, O_RDONLY);
  if (fd >= 0)
  {
    read(fd, bcmd, sizeof(bcmd));
    val = strtol(bcmd, NULL, 16);
    close(fd);
  }
  return val;
}

bool CEGLNativeTypeAmlogic::SetDisplayResolution(const char *resolution)
{
  CStdString modestr = resolution;
  // switch display resolution
  set_sysfs_str("/sys/class/display/mode", modestr.c_str());
  usleep(250 * 1000);

  // setup gui freescale depending on display resolution
  DisableFreeScale();
  if (modestr.Left(4).Equals("1080"))
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
  fromMode.Trim();
  // strips, for example, 720p* to 720p
  if (fromMode.Right(1) == "*")
    fromMode = fromMode.Left(std::max(0, (int)fromMode.size() - 1));

  if (fromMode.Equals("720p"))
  {
    res->iWidth = 1280;
    res->iHeight= 720;
    res->iScreenWidth  = res->iWidth;
    res->iScreenHeight = res->iHeight;
    res->fRefreshRate = 60;
    res->dwFlags= D3DPRESENTFLAG_PROGRESSIVE;
  }
  else if (fromMode.Equals("720p50hz"))
  {
    res->iWidth = 1280;
    res->iHeight= 720;
    res->iScreenWidth  = res->iWidth;
    res->iScreenHeight = res->iHeight;
    res->fRefreshRate = 50;
    res->dwFlags= D3DPRESENTFLAG_PROGRESSIVE;
  }
  else if (fromMode.Equals("1080p"))
  {
    res->iWidth = 1280;
    res->iHeight= 720;
    res->iScreenWidth  = 1920;
    res->iScreenHeight = 1080;
    res->fRefreshRate = 60;
    res->dwFlags= D3DPRESENTFLAG_PROGRESSIVE;
  }
  else if (fromMode.Equals("1080p50hz"))
  {
    res->iScreenWidth = 1920;
    res->iScreenHeight= 1080;
    res->iWidth = 1280;
    res->iHeight= 720;
    res->fRefreshRate = 50;
    res->dwFlags= D3DPRESENTFLAG_PROGRESSIVE;
  }
  else if (fromMode.Equals("1080i"))
  {
    res->iScreenWidth = 1920;
    res->iScreenHeight= 1080;
    res->iWidth = 1280;
    res->iHeight= 720;
    res->fRefreshRate = 60;
    res->dwFlags= D3DPRESENTFLAG_INTERLACED;
  }
  else if (fromMode.Equals("1080i50hz"))
  {
    res->iWidth = 1920;
    res->iHeight= 1080;
    res->iScreenWidth = 1280;
    res->iScreenHeight= 720;
    res->fRefreshRate = 50;
    res->dwFlags= D3DPRESENTFLAG_INTERLACED;
  }

  res->iScreen       = 0;
  res->bFullScreen   = true;
  res->iSubtitles    = (int)(0.965 * res->iHeight);
  res->fPixelRatio   = 1.0f;
  res->strMode.Format("%dx%d @ %.2f%s - Full Screen", res->iScreenWidth, res->iScreenHeight, res->fRefreshRate,
    res->dwFlags & D3DPRESENTFLAG_INTERLACED ? "i" : "");

  return res->iWidth > 0 && res->iHeight> 0;
}

void CEGLNativeTypeAmlogic::EnableFreeScale()
{
  // remove default OSD and video path (default_osd default)
  set_sysfs_str("/sys/class/vfm/map", "rm all");
  usleep(60 * 1000);

  // add OSD path
  set_sysfs_str("/sys/class/vfm/map", "add osdpath osd amvideo");
  // enable OSD free scale using frame buffer size of 720p
  set_sysfs_int("/sys/class/graphics/fb0/free_scale", 0);
  set_sysfs_int("/sys/class/graphics/fb1/free_scale", 0);
  set_sysfs_int("/sys/class/graphics/fb0/scale_width",  1280);
  set_sysfs_int("/sys/class/graphics/fb0/scale_height", 720);
  set_sysfs_int("/sys/class/graphics/fb1/scale_width",  1280);
  set_sysfs_int("/sys/class/graphics/fb1/scale_height", 720);
  set_sysfs_int("/sys/class/graphics/fb0/free_scale", 1);
  set_sysfs_int("/sys/class/graphics/fb1/free_scale", 1);
  usleep(60 * 1000);
  // remove OSD path
  set_sysfs_int("/sys/class/graphics/fb0/free_scale", 0);
  set_sysfs_int("/sys/class/graphics/fb1/free_scale", 0);
  set_sysfs_str("/sys/class/vfm/map", "rm osdpath");
  usleep(60 * 1000);
  // add video path
  set_sysfs_str("/sys/class/vfm/map", "add videopath decoder ppmgr amvideo");
  // enable video free scale (scaling to 1920x1080 with frame buffer size 1280x720)
  set_sysfs_int("/sys/class/ppmgr/ppscaler", 0);
  set_sysfs_int("/sys/class/video/disable_video", 1);
  set_sysfs_int("/sys/class/ppmgr/ppscaler", 1);
  set_sysfs_str("/sys/class/ppmgr/ppscaler_rect", "0 0 1919 1079 0");
  set_sysfs_str("/sys/class/ppmgr/disp", "1280 720");
  usleep(60 * 1000);
  //
  set_sysfs_int("/sys/class/graphics/fb0/free_scale", 0);
  set_sysfs_int("/sys/class/graphics/fb1/free_scale", 0);
  set_sysfs_int("/sys/class/graphics/fb0/scale_width",  1280);
  set_sysfs_int("/sys/class/graphics/fb0/scale_height", 720);
  set_sysfs_int("/sys/class/graphics/fb1/scale_width",  1280);
  set_sysfs_int("/sys/class/graphics/fb1/scale_height", 720);
  set_sysfs_int("/sys/class/graphics/fb0/free_scale", 1);
  set_sysfs_int("/sys/class/graphics/fb1/free_scale", 1);
  usleep(60 * 1000);
  //
  set_sysfs_int("/sys/class/video/disable_video", 2);
  set_sysfs_str("/sys/class/display/axis", "0 0 1279 719 0 0 0 0");
  set_sysfs_str("/sys/class/ppmgr/ppscaler_rect", "0 0 1279 719 1");
}

void CEGLNativeTypeAmlogic::DisableFreeScale()
{
  // turn off frame buffer freescale
  set_sysfs_int("/sys/class/graphics/fb0/free_scale", 0);
  set_sysfs_int("/sys/class/graphics/fb1/free_scale", 0);
  // revert to default video paths
  set_sysfs_str("/sys/class/vfm/map", "rm all");
  set_sysfs_str("/sys/class/vfm/map", "add default_osd osd amvideo");
  set_sysfs_str("/sys/class/vfm/map", "add default decoder ppmgr amvideo");
  // disable post processing scaler and disable_video special mode
  set_sysfs_int("/sys/class/ppmgr/ppscaler", 0);
  set_sysfs_int("/sys/class/video/disable_video", 0);

  // revert display axis
  int fd0;
  std::string framebuffer = "/dev/fbo";

  if ((fd0 = open(framebuffer.c_str(), O_RDWR)) >= 0)
  {
    struct fb_var_screeninfo vinfo;
    if (ioctl(fd0, FBIOGET_VSCREENINFO, &vinfo) == 0)
    {
      char daxis_str[255] = {0};
      sprintf(daxis_str, "%d %d %d %d 0 0 0 0", 0, 0, vinfo.xres, vinfo.yres);
      set_sysfs_str("/sys/class/display/axis", daxis_str);
    }
    close(fd0);
  }
}

void CEGLNativeTypeAmlogic::SetCpuMinLimit(bool limit)
{
  // when playing hw decoded audio, we cannot drop below 600MHz
  // or risk hw audio issues. AML code does a 2X scaling based off
  // /sys/class/audiodsp/codec_mips but tests show that this is
  // seems risky so we just clamp to 600Mhz to be safe.

  // only adjust if we are running "ondemand"
  char scaling_governor[256] = {0};
  get_sysfs_str("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor", scaling_governor, 255);
  if (strncmp(scaling_governor, "ondemand", 255))
    return;

  int freq;
  if (limit)
    freq = 600000;
  else
    freq = get_sysfs_int("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_min_freq");
  set_sysfs_int("/sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq", freq);
}
