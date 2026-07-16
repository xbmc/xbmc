/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "input/keyboard/XBMC_keysym.h"

#include <cstdint>
#include <optional>

class CEvdevKeyMapping
{
public:
  static std::optional<XBMCKey> XBMCKeyForEvdevCode(uint32_t code);
};
