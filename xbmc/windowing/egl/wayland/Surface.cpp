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
#include "Surface.h"

namespace xw = xbmc::wayland;

boost::scoped_ptr<xw::WaylandSurfaceListener> xw::WaylandSurfaceListener::m_instance;

xw::WaylandSurfaceListener &
xw::WaylandSurfaceListener::GetInstance()
{
  if (!m_instance)
    m_instance.reset(new WaylandSurfaceListener());

  return *m_instance;
}

void
xw::WaylandSurfaceListener::SetHandler(const Handler &handler)
{
  m_handler = handler;
}

void
xw::WaylandSurfaceListener::SurfaceCreated(xw::Surface &surface)
{
  if (!m_handler.empty())
    m_handler(surface);
}

xw::Surface::Surface(IDllWaylandClient &clientLibrary,
                     struct wl_surface *surface) :
  m_clientLibrary(clientLibrary),
  m_surface(surface)
{
  WaylandSurfaceListener::GetInstance().SurfaceCreated(*this);
}

xw::Surface::~Surface()
{
  protocol::CallMethodOnWaylandObject(m_clientLibrary,
                                      m_surface,
                                      WL_SURFACE_DESTROY);
  protocol::DestroyWaylandObject(m_clientLibrary,
                                 m_surface);
}

struct wl_surface *
xw::Surface::GetWlSurface()
{
  return m_surface;
}

struct wl_callback *
xw::Surface::CreateFrameCallback()
{
  struct wl_callback *callback =
    protocol::CreateWaylandObject<struct wl_callback *,
                                  struct wl_surface *>(m_clientLibrary,
                                                       m_surface,
                                                       m_clientLibrary.Get_wl_callback_interface());
  protocol::CallMethodOnWaylandObject(m_clientLibrary,
                                      m_surface,
                                      WL_SURFACE_FRAME, callback);
  return callback;
}

void
xw::Surface::SetOpaqueRegion(struct wl_region *region)
{
  protocol::CallMethodOnWaylandObject(m_clientLibrary,
                                      m_surface,
                                      WL_SURFACE_SET_OPAQUE_REGION,
                                      region);
}

void
xw::Surface::Commit()
{
  protocol::CallMethodOnWaylandObject(m_clientLibrary,
                                      m_surface,
                                      WL_SURFACE_COMMIT);
}
