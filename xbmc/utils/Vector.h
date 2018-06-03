#pragma once
/*
 *      Copyright (C) 2012-present Team Kodi
 *      This file is part of Kodi - https://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */

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
