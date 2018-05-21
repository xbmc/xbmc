/*
 *      Copyright (C) 2012-present Team Kodi
 *      http://kodi.tv
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

#include <math.h>

#include "Vector.h"

CVector::CVector()
{
  reset();
}

CVector::CVector(float xCoord, float yCoord)
  : x(xCoord),
    y(yCoord)
{ }

void CVector::reset()
{
  x = 0.0f;
  y = 0.0f;
}

const CVector CVector::operator+(const CVector &other) const
{
  return CVector(x + other.x, y + other.y);
}

const CVector CVector::operator-(const CVector &other) const
{
  return CVector(x - other.x, y - other.y);
}

CVector& CVector::operator+=(const CVector &other)
{
  x += other.x;
  y += other.y;

  return *this;
}

CVector& CVector::operator-=(const CVector &other)
{
  x -= other.x;
  y -= other.y;

  return *this;
}

float CVector::scalar(const CVector &other) const
{
  return x * other.x + y * other.y;
}

float CVector::length() const
{
  return sqrt(pow(x, 2) + pow(y, 2));
}
