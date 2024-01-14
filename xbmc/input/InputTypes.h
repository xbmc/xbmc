/*
 *  Copyright (C) 2018-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

namespace KODI
{
namespace INPUT
{
/*!
 * \brief Cardinal directions, used for input device motions
 */
enum class CARDINAL_DIRECTION
{
  NONE = 0x0,
  UP = 0x1,
  DOWN = 0x2,
  RIGHT = 0x4,
  LEFT = 0x8,
};

/*!
 * \brief Cardinal and intercardinal directions, used for input device motions
 */
enum class INTERCARDINAL_DIRECTION
{
  NONE = static_cast<unsigned int>(CARDINAL_DIRECTION::NONE),
  UP = static_cast<unsigned int>(CARDINAL_DIRECTION::UP),
  DOWN = static_cast<unsigned int>(CARDINAL_DIRECTION::DOWN),
  RIGHT = static_cast<unsigned int>(CARDINAL_DIRECTION::RIGHT),
  LEFT = static_cast<unsigned int>(CARDINAL_DIRECTION::LEFT),
  RIGHTUP = RIGHT | UP,
  RIGHTDOWN = RIGHT | DOWN,
  LEFTUP = LEFT | UP,
  LEFTDOWN = LEFT | DOWN,
};
} // namespace INPUT
} // namespace KODI
