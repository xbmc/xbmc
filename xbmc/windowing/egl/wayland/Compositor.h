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

struct wl_compositor;
struct wl_surface;
struct wl_region;

class IDllWaylandClient;

namespace xbmc
{
namespace wayland
{
class Compositor :
  boost::noncopyable
{
public:

  Compositor(IDllWaylandClient &clientLibrary,
             struct wl_compositor *compositor);
  ~Compositor();

  struct wl_compositor * GetWlCompositor();
  
  /* Creates a "surface" on the compositor. This is not a renderable
   * surface immediately, a renderable "buffer" must be bound to it
   * (usually an EGL Window) */
  struct wl_surface * CreateSurface() const;
  
  /* Creates a "region" on the compositor side. Server side regions
   * are manipulated on the client side and then can be used to
   * affect rendering and input on the server side */
  struct wl_region * CreateRegion() const;

private:

  IDllWaylandClient &m_clientLibrary;
  struct wl_compositor *m_compositor;
};
}
}
