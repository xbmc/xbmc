/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace KODI
{
namespace GAME
{
enum class DiscSlotType
{
  Unknown,
  Disc,
  Removed,
};

struct DiscSlot
{
  DiscSlotType type{DiscSlotType::Unknown};
  std::string fileName;
  std::string label;

  bool operator==(const DiscSlot&) const = default;
};

struct GameClientDiscState
{
  std::vector<DiscSlot> slots;
  int32_t selectedSlot{-1};
  bool trayEjected{false};

  bool operator==(const GameClientDiscState&) const = default;
};
} // namespace GAME
} // namespace KODI
