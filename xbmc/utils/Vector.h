/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

class CVector
{
public:
  CVector() = default;
  constexpr CVector(float xCoord, float yCoord):x(xCoord), y(yCoord) {}

  constexpr CVector operator+(const CVector &other) const
  {
    return CVector(x + other.x, y + other.y);
  }

  constexpr CVector operator-(const CVector &other) const
  {
    return CVector(x - other.x, y - other.y);
  }

  CVector& operator+=(const CVector &other);
  CVector& operator-=(const CVector &other);

  constexpr float scalar(const CVector &other) const
  {
    return x * other.x + y * other.y;
  }

  float length() const;

  float x = 0;
  float y = 0;
};
