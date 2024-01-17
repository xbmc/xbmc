/*
 *  Copyright (C) 2013-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/Vector.h"

/*!
 * \ingroup touch
 * \brief A class representing a touch consisting of an x and y coordinate and
 *        a time
 */
class Touch : public CVector
{
public:
  /*!
   * \brief Checks if the touch is valid i.e. if the x/y coordinates and the
   *        time are >= 0
   *
   * \return True if the touch is valid otherwise false
   */
  bool valid() const { return x >= 0.0f && y >= 0.0f && time >= 0; }

  /*!
   * \brief Copies the x/y coordinates and the time from the given touch
   *
   * \param other Touch to copy x/y coordinates and time from
   */
  void copy(const Touch& other)
  {
    x = other.x;
    y = other.y;
    time = other.time;
  }

  int64_t time = -1; // in nanoseconds
};

/*!
 * \ingroup touch
 * \brief A class representing a touch pointer interaction consisting of an
 *        down touch, the last touch and the current touch.
 */
class Pointer
{
public:
  Pointer() { reset(); }

  /*!
   * \brief Resets the pointer and all its touches
   */
  void reset()
  {
    down = {};
    last = {};
    moving = false;
    size = 0.0f;
  }

  /*!
   * \brief Checks if the "down" touch is valid
   *
   * \return True if the "down" touch is valid otherwise false
   */
  bool valid() const { return down.valid(); }

  /*!
   * \brief Calculates the velocity in x/y direction using the "down" and
   *        either the "last" or "current" touch
   *
   * \param velocityX   Placeholder for velocity in x direction
   * \param velocityY   Placeholder for velocity in y direction
   * \param fromLast    Whether to calculate the velocity with the "last" or
   *                    the "current" touch
   *
   * \return True if the velocity is valid otherwise false
   */
  bool velocity(float& velocityX, float& velocityY, bool fromLast = true) const
  {
    int64_t fromTime = last.time;
    float fromX = last.x;
    float fromY = last.y;
    if (!fromLast)
    {
      fromTime = down.time;
      fromX = down.x;
      fromY = down.y;
    }

    velocityX = 0.0f; // number of pixels per second
    velocityY = 0.0f; // number of pixels per second

    int64_t timeDiff = current.time - fromTime;
    if (timeDiff <= 0)
      return false;

    velocityX = ((current.x - fromX) * 1000000000) / timeDiff;
    velocityY = ((current.y - fromY) * 1000000000) / timeDiff;
    return true;
  }

  Touch down;
  Touch last;
  Touch current;
  bool moving;
  float size;
};
