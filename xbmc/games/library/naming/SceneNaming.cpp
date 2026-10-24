/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SceneNaming.h"

#include <array>
#include <string_view>
#include <utility>

using namespace KODI;
using namespace GAME;

namespace
{
// KS is South Korea
constexpr auto regions = std::to_array<std::pair<std::string_view, std::string_view>>({
    {"AU", "Australia"},
    {"DE", "Germany"},
    {"ES", "Spain"},
    {"EU", "Europe"},
    {"FR", "France"},
    {"IT", "Italy"},
    {"JP", "Japan"},
    {"KS", "Korea"},
    {"NL", "Netherlands"},
    {"US", "USA"},
});
} // namespace

std::string_view CSceneNaming::RegionName(std::string_view code)
{
  for (const auto& [c, name] : regions)
  {
    if (c == code)
      return name;
  }
  return {};
}
