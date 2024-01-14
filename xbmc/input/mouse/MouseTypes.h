/*
 *  Copyright (C) 2018-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "input/InputTypes.h"

#include <string>

namespace KODI
{
namespace MOUSE
{

/// \ingroup mouse
/// \{

/*!
 * \brief Buttons on a mouse
 */
enum class BUTTON_ID
{
  UNKNOWN,
  LEFT,
  RIGHT,
  MIDDLE,
  BUTTON4,
  BUTTON5,
  WHEEL_UP,
  WHEEL_DOWN,
  HORIZ_WHEEL_LEFT,
  HORIZ_WHEEL_RIGHT,
};

/*!
 * \brief Name of a mouse button
 *
 * Names are defined in the mouse's controller profile.
 */
using ButtonName = std::string;

/*!
 * \brief Directions of motion for a mouse pointer
 */
using POINTER_DIRECTION = INPUT::CARDINAL_DIRECTION;

/*!
 * \brief Name of the mouse pointer
 *
 * Names are defined in the mouse's controller profile.
 */
using PointerName = std::string;

/// \}

} // namespace MOUSE
} // namespace KODI
