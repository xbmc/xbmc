/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Util.h"

#include <map>
#include <string>

#include <wayland-cursor.hpp>

namespace
{
/*
 *  List from gdkcursor-wayland.c
 *
 *  GDK - The GIMP Drawing Kit
 *  Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *  See LICENSES/README.md for more information.
 */
static std::map<std::string, std::string> CursorFallbackNameMap =
{
  { "default",      "left_ptr" },
  { "help",         "question_arrow" },
  { "context-menu", "left_ptr" },
  { "pointer",      "hand" },
  { "progress",     "left_ptr_watch" },
  { "wait",         "watch" },
  { "cell",         "crosshair" },
  { "crosshair",    "cross" },
  { "text",         "xterm" },
  { "vertical-text","xterm" },
  { "alias",        "dnd-link" },
  { "copy",         "dnd-copy" },
  { "move",         "dnd-move" },
  { "no-drop",      "dnd-none" },
  { "dnd-ask",      "dnd-copy" }, // not CSS, but we want to guarantee it anyway
  { "not-allowed",  "crossed_circle" },
  { "grab",         "hand2" },
  { "grabbing",     "hand2" },
  { "all-scroll",   "left_ptr" },
  { "col-resize",   "h_double_arrow" },
  { "row-resize",   "v_double_arrow" },
  { "n-resize",     "top_side" },
  { "e-resize",     "right_side" },
  { "s-resize",     "bottom_side" },
  { "w-resize",     "left_side" },
  { "ne-resize",    "top_right_corner" },
  { "nw-resize",    "top_left_corner" },
  { "se-resize",    "bottom_right_corner" },
  { "sw-resize",    "bottom_left_corner" },
  { "ew-resize",    "h_double_arrow" },
  { "ns-resize",    "v_double_arrow" },
  { "nesw-resize",  "fd_double_arrow" },
  { "nwse-resize",  "bd_double_arrow" },
  { "zoom-in",      "left_ptr" },
  { "zoom-out",     "left_ptr" }
};

}

using namespace KODI::WINDOWING::WAYLAND;

wayland::cursor_t CCursorUtil::LoadFromTheme(wayland::cursor_theme_t const& theme, std::string const& name)
{
  try
  {
    return theme.get_cursor(name);
  }
  catch (std::exception const&)
  {
    auto i = CursorFallbackNameMap.find(name);
    if (i == CursorFallbackNameMap.end())
    {
      throw;
    }
    else
    {
      return theme.get_cursor(i->second);
    }
  }
}
