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
#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>

struct wl_surface;
struct wl_callback;
struct wl_region;

class IDllWaylandClient;

namespace xbmc
{
namespace wayland
{
class Surface :
  boost::noncopyable
{
public:

  Surface(IDllWaylandClient &clientLibrary,
          struct wl_surface *surface);
  ~Surface();

  struct wl_surface * GetWlSurface();
  struct wl_callback * CreateFrameCallback();
  void SetOpaqueRegion(struct wl_region *region);
  void Commit();

private:

  IDllWaylandClient &m_clientLibrary;
  struct wl_surface *m_surface;
};

/* This is effectively just a seam for testing purposes so that
 * we can listen for extra objects that the core implementation might
 * not necessarily be interested in. It isn't possible to get any
 * notification from within weston that a surface was created so
 * we need to rely on the client side in order to do that */
class WaylandSurfaceListener
{
public:

  typedef boost::function<void(Surface &)> Handler;
  
  void SetHandler(const Handler &);
  void SurfaceCreated(Surface &);

  static WaylandSurfaceListener & GetInstance();
private:

  Handler m_handler;
  
  static boost::scoped_ptr<WaylandSurfaceListener> m_instance;
};
}
}
