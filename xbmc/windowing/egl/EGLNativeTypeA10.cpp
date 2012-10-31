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
#include "system.h"
#include <EGL/egl.h>
#include "EGLNativeTypeA10.h"
#include "utils/log.h"
#include "guilib/gui3d.h"

#if defined(ALLWINNERA10) && !defined(TARGET_ANDROID)
#include <sys/ioctl.h>
extern "C" {
#include "drv_display_sun4i.h"
}
static fbdev_window g_fbwin;
#endif

CEGLNativeTypeA10::CEGLNativeTypeA10()
{
#if defined(ALLWINNERA10) && !defined(ANDROID)
  int fd = open("/dev/disp", O_RDWR);

  if (fd >= 0)
  {
    unsigned long       args[4] = { 0, 0, 0, 0 };
    __disp_layer_info_t layera;

    g_fbwin.width  = ioctl(fd, DISP_CMD_SCN_GET_WIDTH , args);
    g_fbwin.height = ioctl(fd, DISP_CMD_SCN_GET_HEIGHT, args);

    if ((g_fbwin.height > 720) && (getenv("A10AB") == NULL))
    {
      //set workmode scaler (system layer)
      args[0] = 0;
      args[1] = 0x64;
      args[2] = (unsigned long) (&layera);
      args[3] = 0;
      ioctl(fd, DISP_CMD_LAYER_GET_PARA, args);
      layera.mode = DISP_LAYER_WORK_MODE_SCALER;
      args[0] = 0;
      args[1] = 0x64;
      args[2] = (unsigned long) (&layera);
      args[3] = 0;
      ioctl(fd, DISP_CMD_LAYER_SET_PARA, args);
    }
    else
    {
      //set workmode normal (system layer)
      args[0] = 0;
      args[1] = 0x64;
      args[2] = (unsigned long) (&layera);
      args[3] = 0;
      ioctl(fd, DISP_CMD_LAYER_GET_PARA, args);
      layera.mode = DISP_LAYER_WORK_MODE_NORMAL;
      args[0] = 0;
      args[1] = 0x64;
      args[2] = (unsigned long) (&layera);
      args[3] = 0;
      ioctl(fd, DISP_CMD_LAYER_SET_PARA, args);

    }
    close(fd);
  }
  else {
    fprintf(stderr, "can not open /dev/disp (errno=%d)!\n", errno);
    CLog::Log(LOGERROR, "can not open /dev/disp (errno=%d)!\n", errno);
  }
#endif
}

CEGLNativeTypeA10::~CEGLNativeTypeA10()
{
} 

bool CEGLNativeTypeA10::CheckCompatibility()
{
#if defined(ALLWINNERA10) && !defined(TARGET_ANDROID)
  return true;
#endif
  return false;
}

void CEGLNativeTypeA10::Initialize()
{
  return;
}
void CEGLNativeTypeA10::Destroy()
{
  return;
}

bool CEGLNativeTypeA10::CreateNativeDisplay()
{
  m_nativeDisplay = EGL_DEFAULT_DISPLAY;
  return true;
}

bool CEGLNativeTypeA10::CreateNativeWindow()
{
#if defined(ALLWINNERA10) && !defined(TARGET_ANDROID)
  m_nativeWindow = &g_fbwin;
  return true;
#else
  return false;
#endif
}  

bool CEGLNativeTypeA10::GetNativeDisplay(XBNativeDisplayType **nativeDisplay) const
{
  if (!nativeDisplay)
    return false;
  *nativeDisplay = (XBNativeDisplayType*) &m_nativeDisplay;
  return true;
}

bool CEGLNativeTypeA10::GetNativeWindow(XBNativeWindowType **nativeWindow) const
{
  if (!nativeWindow)
    return false;
  *nativeWindow = (XBNativeWindowType*) &m_nativeWindow;
  return true;
}

bool CEGLNativeTypeA10::DestroyNativeDisplay()
{
  return true;
}

bool CEGLNativeTypeA10::DestroyNativeWindow()
{
  return true;
}

bool CEGLNativeTypeA10::GetNativeResolution(RESOLUTION_INFO *res) const
{
#if defined(ALLWINNERA10) && !defined(TARGET_ANDROID)
  res->iWidth = g_fbwin.width;
  res->iHeight= g_fbwin.height;

  res->fRefreshRate = 60;
  res->dwFlags= D3DPRESENTFLAG_PROGRESSIVE | D3DPRESENTFLAG_WIDESCREEN;
  res->iScreen       = 0;
  res->bFullScreen   = true;
  res->iSubtitles    = (int)(0.965 * res->iHeight);
  res->fPixelRatio   = 1.0f;
  res->iScreenWidth  = res->iWidth;
  res->iScreenHeight = res->iHeight;
  res->strMode.Format("%dx%d @ %.2f%s - Full Screen", res->iScreenWidth, res->iScreenHeight, res->fRefreshRate,
  res->dwFlags & D3DPRESENTFLAG_INTERLACED ? "i" : "");
  CLog::Log(LOGNOTICE,"Current resolution: %s\n",res->strMode.c_str());
  return true;
#else
  return false;
#endif
}

bool CEGLNativeTypeA10::SetNativeResolution(const RESOLUTION_INFO &res)
{
  return false;
}

bool CEGLNativeTypeA10::ProbeResolutions(std::vector<RESOLUTION_INFO> &resolutions)
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

bool CEGLNativeTypeA10::GetPreferredResolution(RESOLUTION_INFO *res) const
{
  return false;
}

bool CEGLNativeTypeA10::ShowWindow(bool show)
{
  return false;
}
