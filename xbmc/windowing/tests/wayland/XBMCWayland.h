#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
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
#if defined(HAVE_WAYLAND_XBMC_PROTO)

#include <boost/noncopyable.hpp>

struct wl_surface;
struct xbmc_wayland;

namespace xbmc
{
namespace test
{
namespace wayland
{
class XBMCWayland :
  boost::noncopyable
{
public:

  XBMCWayland(struct xbmc_wayland *xbmcWayland);
  ~XBMCWayland();

  struct wl_surface * MostRecentSurface();

  void AddMode(int width,
               int height,
               uint32_t refresh,
               enum wl_output_mode mode);
  void MovePointerTo(struct wl_surface *surface,
                     wl_fixed_t x,
                     wl_fixed_t y);
  void SendButtonTo(struct wl_surface *surface,
                    uint32_t button,
                    uint32_t state);
  void SendAxisTo(struct wl_surface *,
                  uint32_t axis,
                  wl_fixed_t value);
  void SendKeyToKeyboard(struct wl_surface *surface,
                         uint32_t key,
                         enum wl_keyboard_key_state state);
  void SendModifiersToKeyboard(struct wl_surface *surface,
                               uint32_t depressed,
                               uint32_t latched,
                               uint32_t locked,
                               uint32_t group);
  void GiveSurfaceKeyboardFocus(struct wl_surface *surface);
  void PingSurface (struct wl_surface *surface,
                    uint32_t serial);

private:

  struct xbmc_wayland *m_xbmcWayland;
};
}
}
}

#endif
