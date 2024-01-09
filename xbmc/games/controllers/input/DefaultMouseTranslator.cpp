/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DefaultMouseTranslator.h"

using namespace KODI;
using namespace GAME;

namespace
{
// Mouse buttons
constexpr auto DEFAULT_MOUSE_FEATURE_LEFT = "left";
constexpr auto DEFAULT_MOUSE_FEATURE_RIGHT = "right";
constexpr auto DEFAULT_MOUSE_FEATURE_MIDDLE = "middle";
constexpr auto DEFAULT_MOUSE_FEATURE_BUTTON4 = "button4";
constexpr auto DEFAULT_MOUSE_FEATURE_BUTTON5 = "button5";
constexpr auto DEFAULT_MOUSE_FEATURE_WHEEL_UP = "wheelup";
constexpr auto DEFAULT_MOUSE_FEATURE_WHEEL_DOWN = "wheeldown";
constexpr auto DEFAULT_MOUSE_FEATURE_HORIZ_WHEEL_LEFT = "horizwheelleft";
constexpr auto DEFAULT_MOUSE_FEATURE_HORIZ_WHEEL_RIGHT = "horizwheelright";

// Mouse pointers
constexpr auto DEFAULT_MOUSE_FEATURE_POINTER = "pointer";
} // namespace

const char* CDefaultMouseTranslator::TranslateMouseButton(MOUSE::BUTTON_ID button)
{
  switch (button)
  {
    case MOUSE::BUTTON_ID::LEFT:
      return DEFAULT_MOUSE_FEATURE_LEFT;
    case MOUSE::BUTTON_ID::RIGHT:
      return DEFAULT_MOUSE_FEATURE_RIGHT;
    case MOUSE::BUTTON_ID::MIDDLE:
      return DEFAULT_MOUSE_FEATURE_MIDDLE;
    case MOUSE::BUTTON_ID::BUTTON4:
      return DEFAULT_MOUSE_FEATURE_BUTTON4;
    case MOUSE::BUTTON_ID::BUTTON5:
      return DEFAULT_MOUSE_FEATURE_BUTTON5;
    case MOUSE::BUTTON_ID::WHEEL_UP:
      return DEFAULT_MOUSE_FEATURE_WHEEL_UP;
    case MOUSE::BUTTON_ID::WHEEL_DOWN:
      return DEFAULT_MOUSE_FEATURE_WHEEL_DOWN;
    case MOUSE::BUTTON_ID::HORIZ_WHEEL_LEFT:
      return DEFAULT_MOUSE_FEATURE_HORIZ_WHEEL_LEFT;
    case MOUSE::BUTTON_ID::HORIZ_WHEEL_RIGHT:
      return DEFAULT_MOUSE_FEATURE_HORIZ_WHEEL_RIGHT;
    default:
      break;
  }

  return "";
}

const char* CDefaultMouseTranslator::TranslateMousePointer(MOUSE::POINTER_DIRECTION direction)
{
  switch (direction)
  {
    case MOUSE::POINTER_DIRECTION::UP:
    case MOUSE::POINTER_DIRECTION::DOWN:
    case MOUSE::POINTER_DIRECTION::RIGHT:
    case MOUSE::POINTER_DIRECTION::LEFT:
      return DEFAULT_MOUSE_FEATURE_POINTER;
    default:
      break;
  }

  return "";
}
