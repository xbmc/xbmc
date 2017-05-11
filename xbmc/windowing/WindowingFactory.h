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

#ifndef WINDOWING_FACTORY_H
#define WINDOWING_FACTORY_H

#include "system.h"

#if   defined(TARGET_WINDOWS) && defined(HAS_GL)
#include "windows/WinSystemWin32GL.h"

#elif defined(TARGET_WINDOWS) && defined(HAS_DX)
#include "windows/WinSystemWin32DX.h"

#elif defined(TARGET_RASPBERRY_PI) && defined(HAS_GLES)
#include "rpi/WinSystemRpiGLESContext.h"

#elif defined(TARGET_LINUX) && defined(HAS_LIBAMCODEC) && defined(HAS_GLES)
#include "amlogic/WinSystemAmlogicGLESContext.h"

#elif defined(TARGET_ANDROID) && defined(HAS_GLES)
#include "android/WinSystemAndroidGLESContext.h"

#elif defined(TARGET_LINUX) && defined(HAVE_MIR) && defined(HAS_GL)
#include "mir/WinSystemMirGLContext.h"

#elif defined(TARGET_LINUX) && defined(HAVE_MIR) && defined(HAS_GLES)
#include "mir/WinSystemMirGLESContext.h"

#elif defined(TARGET_LINUX)   && defined(HAVE_X11)   && defined(HAS_GLES)
#include "X11/WinSystemX11GLESContext.h"

#elif defined(TARGET_LINUX)   && defined(HAVE_X11)   && defined(HAS_GL)
#include "X11/WinSystemX11GLContext.h"

#elif defined(TARGET_LINUX)   && defined(HAS_GLES) && defined(HAS_EGL) && !defined(HAVE_X11)
#include "egl/WinSystemEGL.h"

#elif defined(TARGET_FREEBSD)   && defined(HAS_GL)   && defined(HAVE_X11)
#include "X11/WinSystemX11GL.h"

#elif defined(TARGET_FREEBSD) && defined(HAS_GLES) && defined(HAS_EGL)
#include "egl/WinSystemGLES.h"

#elif defined(TARGET_DARWIN_OSX)
#include "osx/WinSystemOSXGL.h"

#elif defined(TARGET_DARWIN_IOS)
#include "osx/WinSystemIOS.h"

#endif

#endif // WINDOWING_FACTORY_H

