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
     * This function assumes a right-handed cartesian coordinate system; postive
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
     * This function assumes a right-handed cartesian coordinate system; postive
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
}
}
