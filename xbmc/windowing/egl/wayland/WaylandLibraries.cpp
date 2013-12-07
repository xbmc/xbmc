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
#include <stdexcept>

#include <wayland-client.h>
#include "WaylandLibraries.h"

namespace xw = xbmc::wayland;

void
xw::LoadLibrary(DllDynamic &dll)
{
  if (!dll.Load())
  {
    std::stringstream ss;
    ss << "Failed to load library "
       << dll.GetFile().c_str();

    throw std::runtime_error(ss.str());
  }
}

IDllWaylandClient &
xw::Libraries::ClientLibrary()
{
  return m_clientLibrary.Get();
}

IDllWaylandEGL &
xw::Libraries::EGLLibrary()
{
  return m_eglLibrary.Get();
}

IDllXKBCommon &
xw::Libraries::XKBCommonLibrary()
{
  return m_xkbCommonLibrary.Get();
}
