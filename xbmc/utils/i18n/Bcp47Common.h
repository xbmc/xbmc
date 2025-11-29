/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>
#include <vector>

namespace KODI::UTILS::I18N
{
struct Bcp47Extension
{
  char name;
  std::vector<std::string> segments;
  bool operator==(const Bcp47Extension& other) const = default;
};
} // namespace KODI::UTILS::I18N
