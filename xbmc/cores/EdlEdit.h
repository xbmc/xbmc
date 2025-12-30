/*
 *  Copyright (C) 2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <chrono>
#include <string_view>

namespace EDL
{

constexpr int EDL_ACTION_NONE = -1;

enum class Action
{
  CUT = 0,
  MUTE = 1,
  SCENE = 2,
  COMM_BREAK = 3
};

constexpr std::string_view ActionToString(Action action) noexcept
{
  switch (action)
  {
    case Action::CUT:
      return "CUT";
    case Action::MUTE:
      return "MUTE";
    case Action::SCENE:
      return "SCENE";
    case Action::COMM_BREAK:
      return "COMM_BREAK";
    default:
      return "UNKNOWN";
  }
}

struct Edit
{
  std::chrono::milliseconds start{0};
  std::chrono::milliseconds end{0};
  Action action = Action::CUT;
};

} // namespace EDL
