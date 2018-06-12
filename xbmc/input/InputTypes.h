/*
 *      Copyright (C) 2018 Team Kodi
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
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
    NONE    = 0x0,
    UP      = 0x1,
    DOWN    = 0x2,
    RIGHT   = 0x4,
    LEFT    = 0x8,
  };

  /*!
   * \brief Cardinal and intercardinal directions, used for input device motions
   */
  enum class INTERCARDINAL_DIRECTION
  {
    NONE      = static_cast<unsigned int>(CARDINAL_DIRECTION::NONE),
    UP        = static_cast<unsigned int>(CARDINAL_DIRECTION::UP),
    DOWN      = static_cast<unsigned int>(CARDINAL_DIRECTION::DOWN),
    RIGHT     = static_cast<unsigned int>(CARDINAL_DIRECTION::RIGHT),
    LEFT      = static_cast<unsigned int>(CARDINAL_DIRECTION::LEFT),
    RIGHTUP   = RIGHT | UP,
    RIGHTDOWN = RIGHT | DOWN,
    LEFTUP    = LEFT  | UP,
    LEFTDOWN  = LEFT  | DOWN,
  };
}
}
