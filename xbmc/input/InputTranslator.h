/*
 *  Copyright (C) 2018-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "InputTypes.h"

namespace KODI
{
namespace INPUT
{
class CInputTranslator
{
public:
  /*!
   * \brief Get the closest cardinal direction to the given vector
   *
   * This function assumes a right-handed cartesian coordinate system; positive
   * X is right, positive Y is up.
   *
   * Ties are resolved in the clockwise direction: (0.5, 0.5) will resolve to
   * RIGHT.
   *
   * \param x  The x component of the vector
   * \param y  The y component of the vector
   *
   * \return The closest cardinal direction (up, down, right or left), or
   *         CARDINAL_DIRECTION::NONE if x and y are both 0
   */
  static CARDINAL_DIRECTION VectorToCardinalDirection(float x, float y);

  /*!
   * \brief Get the closest cardinal or intercardinal direction to the given
   *        vector
   *
   * This function assumes a right-handed cartesian coordinate system; positive
   * X is right, positive Y is up.
   *
   * Ties are resolved in the clockwise direction.
   *
   * \param x  The x component of the vector
   * \param y  The y component of the vector
   *
   * \return The closest intercardinal direction, or
   *         INTERCARDINAL_DIRECTION::NONE if x and y are both 0
   */
  static INTERCARDINAL_DIRECTION VectorToIntercardinalDirection(float x, float y);
};
} // namespace INPUT
} // namespace KODI
