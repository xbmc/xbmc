#pragma once

/*
 *      Copyright (C) 2011-2013 Team XBMC
 *      http://www.xbmc.org
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

struct wl_display;
struct wl_callback;

typedef struct wl_display * EGLNativeDisplayType;

namespace xbmc
{
namespace wayland
{
class Display :
  boost::noncopyable
{
  public:

    Display(IDllWaylandClient &clientLibrary);
    ~Display();

    struct wl_display * GetWlDisplay();
    EGLNativeDisplayType* GetEGLNativeDisplay();
    struct wl_callback * Sync();

  private:

    IDllWaylandClient &m_clientLibrary;
    struct wl_display *m_display;
};
}
}
