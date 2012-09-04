/*
 *      Copyright (C) 2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "system.h"

#if defined(TARGET_ANDROID)

#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/ioctl.h>

#include "WinEGLPlatformAndroid.h"
#include "android/activity/XBMCApp.h"
////////////////////////////////////////////////////////////////////////////////////////////
EGLNativeWindowType CWinEGLPlatformAndroid::InitWindowSystem(EGLNativeDisplayType nativeDisplay, int width, int height, int bpp)
{
  if (CXBMCApp::GetNativeWindow() == NULL)
    return 0;

  CWinEGLPlatformGeneric::InitWindowSystem(nativeDisplay, width, height, bpp);
  
  return getNativeWindow();
}

void CWinEGLPlatformAndroid::DestroyWindowSystem(EGLNativeWindowType native_window)
{
  CWinEGLPlatformGeneric::DestroyWindowSystem(native_window);
}

bool CWinEGLPlatformAndroid::ClampToGUIDisplayLimits(int &width, int &height)
{
  return false;
}

bool CWinEGLPlatformAndroid::CreateWindow()
{
  EGLint format;
  // EGL_NATIVE_VISUAL_ID is an attribute of the EGLConfig that is
  // guaranteed to be accepted by ANativeWindow_setBuffersGeometry().
  // As soon as we picked a EGLConfig, we can safely reconfigure the
  // ANativeWindow buffers to match, using EGL_NATIVE_VISUAL_ID.
  eglGetConfigAttrib(m_display, m_config, EGL_NATIVE_VISUAL_ID, &format);

  CXBMCApp::SetBuffersGeometry(0, 0, format);
  
  CWinEGLPlatformGeneric::CreateWindow();
  return true;
}

EGLNativeWindowType CWinEGLPlatformAndroid::getNativeWindow()
{
  return (EGLNativeWindowType)CXBMCApp::GetNativeWindow();
}

#endif
