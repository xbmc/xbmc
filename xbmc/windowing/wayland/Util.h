/*
 *      Copyright (C) 2017 Team XBMC
 *      http://kodi.tv
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

#pragma once

#include <string>

#include <wayland-client.hpp>
#include <wayland-cursor.hpp>

namespace KODI
{
namespace WINDOWING
{
namespace WAYLAND
{

struct WaylandCPtrCompare
{
  bool operator()(wayland::proxy_t const& p1, wayland::proxy_t const& p2)
  {
    return p1.c_ptr() < p2.c_ptr();
  }
};

class CCursorUtil
{
public:
  /**
   * Load a cursor from a theme with automatic fallback
   *
   * Modern cursor themes use CSS names for the cursors as defined in
   * the XDG cursor-spec (draft at the moment), but older themes
   * might still use the cursor names that were popular with X11.
   * This function tries to load a cursor by the given CSS name and
   * automatically falls back to the corresponding X11 name if the
   * load fails.
   *
   * \param theme cursor theme to load from
   * \param name CSS cursor name to load
   * \return requested cursor
   */
  static wayland::cursor_t LoadFromTheme(wayland::cursor_theme_t const& theme, std::string const& name);
};

}
}
}
