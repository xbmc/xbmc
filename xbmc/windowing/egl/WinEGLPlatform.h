#pragma once
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

#ifdef HAS_EGL

#include "system.h"

#if   defined(TARGET_RASPBERRY_PI)
  #include "xbmc/windowing/egl/WinEGLPlatformRaspberryPI.h"
  class CWinEGLPlatformRaspberryPI;
    #define CWinEGLPlatform CWinEGLPlatformRaspberryPI
#elif   defined(TARGET_ANDROID)
  #include "xbmc/windowing/egl/WinEGLPlatformAndroid.h"
  class CWinEGLPlatformAndroid;
  #define CWinEGLPlatform CWinEGLPlatformAndroid

#else
  #include "xbmc/windowing/egl/WinEGLPlatformGeneric.h"
  class CWinEGLPlatformGeneric;
  #define CWinEGLPlatform CWinEGLPlatformGeneric

#endif

#endif
