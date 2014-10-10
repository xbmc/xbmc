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

class IDllWaylandClient;

struct wl_region;

namespace xbmc
{
namespace wayland
{
class Region :
  boost::noncopyable
{
public:

  Region(IDllWaylandClient &clientLibrary,
         struct wl_region *);
  ~Region();
  
  struct wl_region * GetWlRegion();

  void AddRectangle(int32_t x,
                    int32_t y,
                    int32_t width,
                    int32_t height);

private:

  IDllWaylandClient &m_clientLibrary;
  struct wl_region *m_region;
};
}
}
