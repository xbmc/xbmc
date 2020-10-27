/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#if defined(TARGET_LINUX) || defined(TARGET_FREEBSD)

#if defined(WL_EGL_PLATFORM)
#undef WL_EGL_PLATFORM
#define KODI_WL_EGL_PLATFORM
#endif

#if defined(__GBM__)
#undef __GBM__
#define KODI__GBM__
#endif

#if defined(USE_X11)
#undef USE_X11
#define KODI_USE_X11
#endif

#define EGL_NO_X11
#define MESA_EGL_NO_X11_HEADERS
#endif

#include <EGL/egl.h>

#if defined(TARGET_LINUX) || defined(TARGET_FREEBSD)

#if defined(KODI_WL_EGL_PLATFORM)
#undef KODI_WL_EGL_PLATFORM
#define WL_EGL_PLATFORM
#endif

#if defined(KODI__GBM__)
#undef KODI__GBM__
#define __GBM__
#endif

#if defined(KODI_USE_X11)
#undef KODI_USE_X11
#define USE_X11
#endif

#endif
