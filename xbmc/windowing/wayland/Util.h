/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <cstdint>
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
  bool operator()(wayland::proxy_t const& p1, wayland::proxy_t const& p2) const
  {
    return reinterpret_cast<std::uintptr_t>(p1.c_ptr()) < reinterpret_cast<std::uintptr_t>(p2.c_ptr());
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
