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
#include "Region.h"

namespace xw = xbmc::wayland;

xw::Region::Region(IDllWaylandClient &clientLibrary,
                   struct wl_region *region) :
  m_clientLibrary(clientLibrary),
  m_region(region)
{
}

xw::Region::~Region()
{
  protocol::CallMethodOnWaylandObject(m_clientLibrary,
                                      m_region,
                                      WL_REGION_DESTROY);
  protocol::DestroyWaylandObject(m_clientLibrary,
                                 m_region);
}

struct wl_region *
xw::Region::GetWlRegion()
{
  return m_region;
}

void
xw::Region::AddRectangle(int32_t x,
                         int32_t y,
                         int32_t width,
                         int32_t height)
{
  protocol::CallMethodOnWaylandObject(m_clientLibrary,
                                      m_region,
                                      WL_REGION_ADD,
                                      x,
                                      y,
                                      width,
                                      height);
}
