#pragma once

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
#include <boost/noncopyable.hpp>

#include <wayland-client.h>

class IDllWaylandClient;

namespace xbmc
{
namespace wayland
{
class IPointerReceiver
{
public:

  virtual ~IPointerReceiver() {}
  virtual void Motion(uint32_t time,
                      const float &x,
                      const float &y) = 0;
  virtual void Button(uint32_t serial,
                      uint32_t time,
                      uint32_t button,
                      enum wl_pointer_button_state state) = 0;
  virtual void Axis(uint32_t time,
                    uint32_t axis,
                    float value) = 0;
  virtual void Enter(struct wl_surface *surface,
                     double surfaceX,
                     double surfaceY) = 0;
};

/* Wrapper class for a pointer object. Generally there is one pointer
 * per seat.
 * 
 * Events are forwarded from the internal proxy to an IPointerReceiver
 * for further processing. The responsibility of this class is only
 * to receive the events and represent them in a sensible way */
class Pointer
{
public:

  Pointer(IDllWaylandClient &,
          struct wl_pointer *,
          IPointerReceiver &);
  ~Pointer();

  struct wl_pointer * GetWlPointer();

  /* This method changes the cursor to have the contents of
   * an arbitrary surface on the server side. It can also hide
   * the cursor if NULL is passed as "surface" */
  void SetCursor(uint32_t serial,
                 struct wl_surface *surface,
                 int32_t hotspot_x,
                 int32_t hotspot_y);

  static void HandleEnterCallback(void *,
                                  struct wl_pointer *,
                                  uint32_t,
                                  struct wl_surface *,
                                  wl_fixed_t, 
                                  wl_fixed_t);
  static void HandleLeaveCallback(void *,
                                  struct wl_pointer *,
                                  uint32_t,
                                  struct wl_surface *);
  static void HandleMotionCallback(void *,
                                   struct wl_pointer *,
                                   uint32_t,
                                   wl_fixed_t,
                                   wl_fixed_t);
  static void HandleButtonCallback(void *,
                                   struct wl_pointer *,
                                   uint32_t,
                                   uint32_t,
                                   uint32_t,
                                   uint32_t);
  static void HandleAxisCallback(void *,
                                 struct wl_pointer *,
                                 uint32_t,
                                 uint32_t,
                                 wl_fixed_t);

private:

  void HandleEnter(uint32_t serial,
                   struct wl_surface *surface,
                   wl_fixed_t surfaceXFixed,
                   wl_fixed_t surfaceYFixed);
  void HandleLeave(uint32_t serial,
                   struct wl_surface *surface);
  void HandleMotion(uint32_t time,
                    wl_fixed_t surfaceXFixed,
                    wl_fixed_t surfaceYFixed);
  void HandleButton(uint32_t serial,
                    uint32_t time,
                    uint32_t button,
                    uint32_t state);
  void HandleAxis(uint32_t time,
                  uint32_t axis,
                  wl_fixed_t value);

  static const struct wl_pointer_listener m_listener;

  IDllWaylandClient &m_clientLibrary;
  struct wl_pointer *m_pointer;
  IPointerReceiver &m_receiver;
};
}
}
