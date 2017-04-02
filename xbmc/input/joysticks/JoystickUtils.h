/*
 *      Copyright (C) 2014-2017 Team Kodi
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

#include "JoystickTypes.h"

/// \ingroup joystick
/// \{

namespace KODI
{
namespace JOYSTICK
{

inline HAT_DIRECTION& operator|=(HAT_DIRECTION& lhs, HAT_DIRECTION rhs)
{
  return lhs = static_cast<HAT_DIRECTION>(static_cast<int>(lhs) | static_cast<int>(rhs));
}

inline HAT_STATE& operator|=(HAT_STATE& lhs, HAT_STATE rhs)
{
  return lhs = static_cast<HAT_STATE>(static_cast<int>(lhs) | static_cast<int>(rhs));
}

inline bool operator&(HAT_STATE lhs, HAT_DIRECTION rhs)
{
  return (static_cast<int>(lhs) & static_cast<int>(rhs)) ? true : false;
}

inline SEMIAXIS_DIRECTION operator*(SEMIAXIS_DIRECTION lhs, int rhs)
{
  return static_cast<SEMIAXIS_DIRECTION>(static_cast<int>(lhs) * rhs);
}

inline float operator*(float lhs, SEMIAXIS_DIRECTION rhs)
{
  return lhs * static_cast<int>(rhs);
}

}
}

/// \}
