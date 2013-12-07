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
#include "system.h"

#if defined(HAVE_WAYLAND_XBMC_PROTO)

#include <wayland-client.h>
#include <wayland-client-protocol.h>
#include "xbmc_wayland_test_client_protocol.h"

#include "XBMCWayland.h"

namespace xtw = xbmc::test::wayland;

xtw::XBMCWayland::XBMCWayland(struct xbmc_wayland *xbmcWayland) :
  m_xbmcWayland(xbmcWayland)
{
}

xtw::XBMCWayland::~XBMCWayland()
{
  xbmc_wayland_destroy(m_xbmcWayland);
}

void
xtw::XBMCWayland::AddMode(int width,
                          int height,
                          uint32_t refresh,
                          enum wl_output_mode flags)
{
  xbmc_wayland_add_mode(m_xbmcWayland,
                        width,
                        height,
                        refresh,
                        static_cast<uint32_t>(flags));
}

void
xtw::XBMCWayland::MovePointerTo(struct wl_surface *surface,
                                wl_fixed_t x,
                                wl_fixed_t y)
{
  xbmc_wayland_move_pointer_to_on_surface(m_xbmcWayland,
                                          surface,
                                          x,
                                          y);
}

void
xtw::XBMCWayland::SendButtonTo(struct wl_surface *surface,
                               uint32_t button,
                               uint32_t state)
{
  xbmc_wayland_send_button_to_surface(m_xbmcWayland,
                                      surface,
                                      button,
                                      state);
}

void
xtw::XBMCWayland::SendAxisTo(struct wl_surface *surface,
                             uint32_t axis,
                             wl_fixed_t value)
{
  xbmc_wayland_send_axis_to_surface(m_xbmcWayland,
                                    surface,
                                    axis,
                                    value);
}

void
xtw::XBMCWayland::SendKeyToKeyboard(struct wl_surface *surface,
                                    uint32_t key,
                                    enum wl_keyboard_key_state state)
{
  xbmc_wayland_send_key_to_keyboard(m_xbmcWayland,
                                    surface,
                                    key,
                                    state);
}

void
xtw::XBMCWayland::SendModifiersToKeyboard(struct wl_surface *surface,
                                          uint32_t depressed,
                                          uint32_t latched,
                                          uint32_t locked,
                                          uint32_t group)
{
  xbmc_wayland_send_modifiers_to_keyboard(m_xbmcWayland,
                                          surface,
                                          depressed,
                                          latched,
                                          locked,
                                          group);
}

void
xtw::XBMCWayland::GiveSurfaceKeyboardFocus(struct wl_surface *surface)
{
  xbmc_wayland_give_surface_keyboard_focus(m_xbmcWayland,
                                           surface);
}

void
xtw::XBMCWayland::PingSurface(struct wl_surface *surface,
                              uint32_t serial)
{
  xbmc_wayland_ping_surface(m_xbmcWayland, surface, serial);
}

#endif
