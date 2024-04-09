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

void CSeatWebOS::InstallKeyboardRepeatInfo()
{
  // Since webOS 7 the compositor sends the following key repeat info:
  // Key repeat rate: 40 cps, delay 400 ms
  // Which is too fast for the long press detection
}

} // namespace KODI::WINDOWING::WAYLAND
