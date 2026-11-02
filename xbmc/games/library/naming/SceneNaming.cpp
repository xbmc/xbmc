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
// The territory each code is, as BCP 47 codes it. KS is South Korea, and EU
// UN M.49's Europe rather than BCP 47's European Union.
constexpr auto territories = std::to_array<std::pair<std::string_view, std::string_view>>({
    {"AU", "AU"},
    {"DE", "DE"},
    {"ES", "ES"},
    {"EU", "150"},
    {"FR", "FR"},
    {"IT", "IT"},
    {"JP", "JP"},
    {"KS", "KR"},
    {"NL", "NL"},
    {"US", "US"},
});
} // namespace

std::optional<GameRegion> CSceneNaming::Region(std::string_view code)
{
  for (const auto& [c, territory] : territories)
  {
    if (c == code)
      return GameRegion::Make(territory);
  }
  return std::nullopt;
}
