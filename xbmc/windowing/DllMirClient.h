#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#if (defined HAVE_CONFIG_H) && (!defined TARGET_WINDOWS)
  #include "config.h"
#endif

#include <cstdarg>

#include "utils/log.h"
#include "DynamicDll.h"

class IDllMirClient
{
public:

  typedef struct MirConnection MirConnection;
  typedef struct MirSurface MirSurface;
  typedef struct MirDisplayInfo MirDisplayInfo;
  typedef struct MirSurfaceParameters MirSurfaceParameters;
  typedef struct MirEventDelegate MirEventDelegate;
  typedef unsigned int MirPixelFormat;
  typedef unsigned int MirBufferUsage;
  typedef void * MirEGLNativeDisplayType;
  typedef void * MirEGLNativeWindowType;

  virtual MirConnection * mir_connect_sync(const char *, const char *) = 0;
  virtual int mir_connection_is_valid(MirConnection *) = 0;
  virtual void mir_connection_release(MirConnection *) = 0;
  virtual void mir_connection_get_display_info(MirConnection *,
                                               MirDisplayInfo *) = 0;
  virtual MirSurface * mir_connection_create_surface_sync(MirConnection *,
                                                          MirSurfaceParameters *) = 0;
  virtual MirEGLNativeDisplayType mir_connection_get_egl_native_display(MirConnection *) = 0;
  virtual MirEGLNativeWindowType mir_surface_get_egl_native_window(MirSurface *) = 0;
  virtual void mir_surface_set_event_handler(MirSurface *, MirEventDelegate *) = 0;
  virtual void mir_surface_release_sync(MirSurface *) = 0;

  virtual ~IDllMirClient() {}
};

class DllMirClient : public DllDynamic, public IDllMirClient
{
  DECLARE_DLL_WRAPPER(DllMirClient, DLL_PATH_MIR_CLIENT)
  
  DEFINE_METHOD2(MirConnection *, mir_connect_sync, (const char *p1, const char *p2));
  DEFINE_METHOD1(int, mir_connection_is_valid, (MirConnection *p1));
  DEFINE_METHOD1(void, mir_connection_release, (MirConnection *p1));
  DEFINE_METHOD2(void, mir_connection_get_display_info, (MirConnection *p1, MirDisplayInfo *p2));
  DEFINE_METHOD2(MirSurface *, mir_connection_create_surface_sync, (MirConnection *p1, MirSurfaceParameters *p2));
  DEFINE_METHOD1(MirEGLNativeDisplayType, mir_connection_get_egl_native_display, (MirConnection *p1));
  DEFINE_METHOD1(MirEGLNativeWindowType, mir_surface_get_egl_native_window, (MirSurface *p1));
  DEFINE_METHOD1(void, mir_surface_release_sync, (MirSurface *p1));
  DEFINE_METHOD2(void, mir_surface_set_event_handler, (MirSurface *p1, MirEventDelegate *p2));

  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD(mir_connect_sync)
    RESOLVE_METHOD(mir_connection_is_valid)
    RESOLVE_METHOD(mir_connection_release)
    RESOLVE_METHOD(mir_connection_get_display_info)
    RESOLVE_METHOD(mir_connection_create_surface_sync)
    RESOLVE_METHOD(mir_connection_get_egl_native_display)
    RESOLVE_METHOD(mir_surface_get_egl_native_window)
    RESOLVE_METHOD(mir_surface_release_sync)
    RESOLVE_METHOD(mir_surface_set_event_handler)
  END_METHOD_RESOLVE()
};
