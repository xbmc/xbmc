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
#include <wayland-client.h>

#include "windowing/DllWaylandClient.h"
#include "windowing/WaylandProtocol.h"
#include "Compositor.h"

namespace xw = xbmc::wayland;

xw::Compositor::Compositor(IDllWaylandClient &clientLibrary,
                           struct wl_compositor *compositor) :
  m_clientLibrary(clientLibrary),
  m_compositor(compositor)
{
}

xw::Compositor::~Compositor()
{
  protocol::DestroyWaylandObject(m_clientLibrary,
                                 m_compositor);
}

struct wl_compositor *
xw::Compositor::GetWlCompositor()
{
  return m_compositor;
}

struct wl_surface *
xw::Compositor::CreateSurface() const
{
  struct wl_surface *surface =
    protocol::CreateWaylandObject<struct wl_surface *,
                                  struct wl_compositor *>(m_clientLibrary,
                                                          m_compositor,
                                                          m_clientLibrary.Get_wl_surface_interface());
  protocol::CallMethodOnWaylandObject(m_clientLibrary,
                                      m_compositor,
                                      WL_COMPOSITOR_CREATE_SURFACE,
                                      surface);
  return surface;
}

struct wl_region *
xw::Compositor::CreateRegion() const
{
  struct wl_region *region =
    protocol::CreateWaylandObject<struct wl_region *,
                                  struct wl_compositor *>(m_clientLibrary,
                                                          m_compositor,
                                                          m_clientLibrary.Get_wl_region_interface ());
  protocol::CallMethodOnWaylandObject(m_clientLibrary,
                                      m_compositor,
                                      WL_COMPOSITOR_CREATE_REGION,
                                      region);
  return region;
}
