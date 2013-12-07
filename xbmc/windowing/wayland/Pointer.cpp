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
#include <sstream>
#include <iostream>
#include <stdexcept>

#include <wayland-client.h>

#include "windowing/DllWaylandClient.h"
#include "windowing/WaylandProtocol.h"
#include "Pointer.h"

namespace xw = xbmc::wayland;

const struct wl_pointer_listener xw::Pointer::m_listener =
{
  Pointer::HandleEnterCallback,
  Pointer::HandleLeaveCallback,
  Pointer::HandleMotionCallback,
  Pointer::HandleButtonCallback,
  Pointer::HandleAxisCallback
};

xw::Pointer::Pointer(IDllWaylandClient &clientLibrary,
                     struct wl_pointer *pointer,
                     IPointerReceiver &receiver) :
  m_clientLibrary(clientLibrary),
  m_pointer(pointer),
  m_receiver(receiver)
{
  protocol::AddListenerOnWaylandObject(m_clientLibrary,
                                       pointer,
                                       &m_listener,
                                       this);
}

xw::Pointer::~Pointer()
{
  protocol::DestroyWaylandObject(m_clientLibrary,
                                 m_pointer);
}

void xw::Pointer::SetCursor(uint32_t serial,
                            struct wl_surface *surface,
                            int32_t hotspot_x,
                            int32_t hotspot_y)
{
  protocol::CallMethodOnWaylandObject(m_clientLibrary,
                                      m_pointer,
                                      WL_POINTER_SET_CURSOR,
                                      serial,
                                      surface,
                                      hotspot_x,
                                      hotspot_y);
}

void xw::Pointer::HandleEnterCallback(void *data,
                                      struct wl_pointer *pointer,
                                      uint32_t serial,
                                      struct wl_surface *surface,
                                      wl_fixed_t x,
                                      wl_fixed_t y)
{
  static_cast<Pointer *>(data)->HandleEnter(serial,
                                            surface,
                                            x,
                                            y);
}

void xw::Pointer::HandleLeaveCallback(void *data,
                                      struct wl_pointer *pointer,
                                      uint32_t serial,
                                      struct wl_surface *surface)
{
  static_cast<Pointer *>(data)->HandleLeave(serial, surface);
}

void xw::Pointer::HandleMotionCallback(void *data,
                                       struct wl_pointer *pointer,
                                       uint32_t time,
                                       wl_fixed_t x,
                                       wl_fixed_t y)
{
  static_cast<Pointer *>(data)->HandleMotion(time,
                                             x,
                                             y);
}

void xw::Pointer::HandleButtonCallback(void *data,
                                       struct wl_pointer * pointer,
                                       uint32_t serial,
                                       uint32_t time,
                                       uint32_t button,
                                       uint32_t state)
{
  static_cast<Pointer *>(data)->HandleButton(serial,
                                             time,
                                             button,
                                             state);
}

void xw::Pointer::HandleAxisCallback(void *data,
                                     struct wl_pointer *pointer,
                                     uint32_t time,
                                     uint32_t axis,
                                     wl_fixed_t value)
{
  static_cast<Pointer *>(data)->HandleAxis(time,
                                           axis,
                                           value);
}

void xw::Pointer::HandleEnter(uint32_t serial,
                              struct wl_surface *surface,
                              wl_fixed_t surfaceXFixed,
                              wl_fixed_t surfaceYFixed)
{
  m_receiver.Enter(surface,
                   wl_fixed_to_double(surfaceXFixed),
                   wl_fixed_to_double(surfaceYFixed));
}

void xw::Pointer::HandleLeave(uint32_t serial,
                              struct wl_surface *surface)
{
}

void xw::Pointer::HandleMotion(uint32_t time,
                               wl_fixed_t surfaceXFixed,
                               wl_fixed_t surfaceYFixed)
{
  m_receiver.Motion(time,
                    wl_fixed_to_double(surfaceXFixed),
                    wl_fixed_to_double(surfaceYFixed));
}

void xw::Pointer::HandleButton(uint32_t serial,
                               uint32_t time,
                               uint32_t button,
                               uint32_t state)
{
  m_receiver.Button(serial,
                    time,
                    button,
                    static_cast<enum wl_pointer_button_state>(state));
}

void xw::Pointer::HandleAxis(uint32_t time,
                             uint32_t axis,
                             wl_fixed_t value)
{
  m_receiver.Axis(time,
                  axis,
                  wl_fixed_to_double(value));
}
