/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PlatformLinux.h"

// clang-format off
#if defined(HAS_GLES)
#if defined(HAVE_WAYLAND)
#include "windowing/wayland/WinSystemWaylandEGLContextGLES.h"
#endif
#if defined(HAVE_X11)
#include "windowing/X11/WinSystemX11GLESContext.h"
#endif
#if defined(HAVE_GBM)
#include "windowing/gbm/WinSystemGbmGLESContext.h"
#endif
#endif

#if defined(HAS_GL)
#if defined(HAVE_WAYLAND)
#include "windowing/wayland/WinSystemWaylandEGLContextGL.h"
#endif
#if defined(HAVE_X11)
#include "windowing/X11/WinSystemX11GLContext.h"
#endif
#if defined(HAVE_GBM)
#include "windowing/gbm/WinSystemGbmGLContext.h"
#endif
#endif
// clang-format on

#include <cstdlib>

CPlatform* CPlatform::CreateInstance()
{
  return new CPlatformLinux();
}

bool CPlatformLinux::Init()
{
  if (!CPlatformPosix::Init())
    return false;

  setenv("OS", "Linux", true); // for python scripts that check the OS

#if defined(HAS_GLES)
#if defined(HAVE_WAYLAND)
  KODI::WINDOWING::WAYLAND::CWinSystemWaylandEGLContextGLES::Register();
#endif
#if defined(HAVE_X11)
  KODI::WINDOWING::X11::CWinSystemX11GLESContext::Register();
#endif
#if defined(HAVE_GBM)
  KODI::WINDOWING::GBM::CWinSystemGbmGLESContext::Register();
#endif
#endif

#if defined(HAS_GL)
#if defined(HAVE_WAYLAND)
  KODI::WINDOWING::WAYLAND::CWinSystemWaylandEGLContextGL::Register();
#endif
#if defined(HAVE_X11)
  KODI::WINDOWING::X11::CWinSystemX11GLContext::Register();
#endif
#if defined(HAVE_GBM)
  KODI::WINDOWING::GBM::CWinSystemGbmGLContext::Register();
#endif
#endif

  return true;
}
