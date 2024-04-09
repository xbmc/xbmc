/*
*  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SeatWebOS.h"

namespace KODI::WINDOWING::WAYLAND
{

void CSeatWebOS::SetCursor(std::uint32_t serial,
                           wayland::surface_t const& surface,
                           std::int32_t hotspotX,
                           std::int32_t hotspotY)
{
  // set_cursor on webOS completely breaks pointer input
}

} // namespace KODI::WINDOWING::WAYLAND
