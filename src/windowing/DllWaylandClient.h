#pragma once

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

#if (defined HAVE_CONFIG_H) && (!defined TARGET_WINDOWS)
  #include "config.h"
#endif

#include <cstdarg>

#include "utils/log.h"
#include "DynamicDll.h"

struct wl_proxy;
struct wl_interface;

struct wl_display;
struct wl_registry;
struct wl_callback;
struct wl_compositor;
struct wl_shell;
struct wl_shell_surface;
struct wl_surface;
struct wl_seat;
struct wl_pointer;
struct wl_keyboard;
struct wl_output;
struct wl_region;

extern const struct wl_interface wl_display_interface;
extern const struct wl_interface wl_registry_interface;
extern const struct wl_interface wl_callback_interface;
extern const struct wl_interface wl_compositor_interface;
extern const struct wl_interface wl_shell_interface;
extern const struct wl_interface wl_shell_surface_interface;
extern const struct wl_interface wl_surface_interface;
extern const struct wl_interface wl_seat_interface;
extern const struct wl_interface wl_pointer_interface;
extern const struct wl_interface wl_keyboard_interface;
extern const struct wl_interface wl_output_interface;
extern const struct wl_interface wl_region_interface;

class IDllWaylandClient
{
public:
  typedef void(*wl_proxy_marshal_func)(struct wl_proxy *,
                                       uint32_t,
                                       ...);
  typedef void(*wl_proxy_listener_func)(void);
  typedef int(*wl_display_read_events_func)(struct wl_display *);
  typedef int(*wl_display_prepare_read_func)(struct wl_display *);

  virtual struct wl_interface ** Get_wl_display_interface() = 0;
  virtual struct wl_interface ** Get_wl_registry_interface() = 0;
  virtual struct wl_interface ** Get_wl_callback_interface() = 0;
  virtual struct wl_interface ** Get_wl_compositor_interface() = 0;
  virtual struct wl_interface ** Get_wl_shell_interface() = 0;
  virtual struct wl_interface ** Get_wl_shell_surface_interface() = 0;
  virtual struct wl_interface ** Get_wl_surface_interface() = 0;
  virtual struct wl_interface ** Get_wl_seat_interface() = 0;
  virtual struct wl_interface ** Get_wl_pointer_interface() = 0;
  virtual struct wl_interface ** Get_wl_keyboard_interface() = 0;
  virtual struct wl_interface ** Get_wl_output_interface() = 0;
  virtual struct wl_interface ** Get_wl_region_interface() = 0;

  virtual struct wl_display * wl_display_connect(const char *) = 0;
  virtual void wl_display_disconnect(struct wl_display *) = 0;
  virtual int wl_display_get_fd(struct wl_display *) = 0;
  virtual wl_display_prepare_read_func wl_display_prepare_read_proc() = 0;
  virtual wl_display_read_events_func wl_display_read_events_proc() = 0;
  virtual int wl_display_dispatch_pending(struct wl_display *) = 0;
  virtual int wl_display_dispatch(struct wl_display *) = 0;
  virtual int wl_display_flush(struct wl_display *) = 0;
  
  virtual wl_proxy_marshal_func wl_proxy_marshaller() = 0;

  virtual struct wl_proxy * wl_proxy_create(struct wl_proxy *,
                                            const struct wl_interface *) = 0;
  virtual void wl_proxy_destroy(struct wl_proxy *) = 0;
  virtual int wl_proxy_add_listener(struct wl_proxy *,
                                    wl_proxy_listener_func *,
                                    void *) = 0;

  virtual ~IDllWaylandClient() {}
};

class DllWaylandClient : public DllDynamic, public IDllWaylandClient
{
  DECLARE_DLL_WRAPPER(DllWaylandClient, DLL_PATH_WAYLAND_CLIENT)
  
  DEFINE_GLOBAL_PTR(struct wl_interface *, wl_display_interface);
  DEFINE_GLOBAL_PTR(struct wl_interface *, wl_registry_interface);
  DEFINE_GLOBAL_PTR(struct wl_interface *, wl_callback_interface);
  DEFINE_GLOBAL_PTR(struct wl_interface *, wl_compositor_interface);
  DEFINE_GLOBAL_PTR(struct wl_interface *, wl_shell_interface);
  DEFINE_GLOBAL_PTR(struct wl_interface *, wl_shell_surface_interface);
  DEFINE_GLOBAL_PTR(struct wl_interface *, wl_surface_interface);
  DEFINE_GLOBAL_PTR(struct wl_interface *, wl_seat_interface);
  DEFINE_GLOBAL_PTR(struct wl_interface *, wl_pointer_interface);
  DEFINE_GLOBAL_PTR(struct wl_interface *, wl_keyboard_interface);
  DEFINE_GLOBAL_PTR(struct wl_interface *, wl_output_interface);
  DEFINE_GLOBAL_PTR(struct wl_interface *, wl_region_interface);
  
  DEFINE_METHOD1(struct wl_display *, wl_display_connect, (const char *p1));
  DEFINE_METHOD1(void, wl_display_disconnect, (struct wl_display *p1));
  DEFINE_METHOD1(int, wl_display_get_fd, (struct wl_display *p1));
  DEFINE_METHOD_FP(int, wl_display_prepare_read, (struct wl_display *p1));
  DEFINE_METHOD_FP(int, wl_display_read_events, (struct wl_display *p1));
  DEFINE_METHOD1(int, wl_display_dispatch_pending, (struct wl_display *p1));
  DEFINE_METHOD1(int, wl_display_dispatch, (struct wl_display *p1));
  DEFINE_METHOD1(int, wl_display_flush, (struct wl_display *p1));
  
  /* We need to resolve wl_proxy_marshal as a function pointer as it
   * takes varargs */
  DEFINE_METHOD_FP(void,
                   wl_proxy_marshal,
                   (struct wl_proxy *p1, uint32_t p2, ...));

  DEFINE_METHOD2(struct wl_proxy *,
                 wl_proxy_create,
                 (struct wl_proxy *p1, const struct wl_interface *p2));
  DEFINE_METHOD1(void, wl_proxy_destroy, (struct wl_proxy *p1));
  DEFINE_METHOD3(int,
                 wl_proxy_add_listener,
                 (struct wl_proxy *p1,
                  wl_proxy_listener_func *p2,
                  void *p3));
  
  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD(wl_display_interface)
    RESOLVE_METHOD(wl_registry_interface)
    RESOLVE_METHOD(wl_callback_interface)
    RESOLVE_METHOD(wl_compositor_interface)
    RESOLVE_METHOD(wl_shell_interface)
    RESOLVE_METHOD(wl_shell_surface_interface)
    RESOLVE_METHOD(wl_surface_interface)
    RESOLVE_METHOD(wl_seat_interface)
    RESOLVE_METHOD(wl_pointer_interface)
    RESOLVE_METHOD(wl_keyboard_interface)
    RESOLVE_METHOD(wl_output_interface)
    RESOLVE_METHOD(wl_region_interface)
  
    RESOLVE_METHOD(wl_display_connect)
    RESOLVE_METHOD(wl_display_disconnect)
    RESOLVE_METHOD(wl_display_get_fd)
    RESOLVE_METHOD_OPTIONAL_FP(wl_display_prepare_read)
    RESOLVE_METHOD_OPTIONAL_FP(wl_display_read_events)
    RESOLVE_METHOD(wl_display_dispatch_pending)
    RESOLVE_METHOD(wl_display_dispatch)
    RESOLVE_METHOD(wl_display_flush)
    RESOLVE_METHOD_FP(wl_proxy_marshal)
    RESOLVE_METHOD(wl_proxy_create)
    RESOLVE_METHOD(wl_proxy_destroy)
    RESOLVE_METHOD(wl_proxy_add_listener)
  END_METHOD_RESOLVE()
  
public:

  /* This overload returns the function pointer to wl_proxy_marshal
   * so that clients can call it directly */
  wl_proxy_marshal_func wl_proxy_marshaller()
  {
    return DllWaylandClient::wl_proxy_marshal;
  }
  
  wl_display_prepare_read_func wl_display_prepare_read_proc()
  {
    return DllWaylandClient::wl_display_prepare_read;
  }
  
  wl_display_read_events_func wl_display_read_events_proc()
  {
    return DllWaylandClient::wl_display_read_events;
  }
};
