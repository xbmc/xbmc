/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include <math.h>

#include "Vector.h"

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

float CVector::length() const
{
  return sqrt(pow(x, 2) + pow(y, 2));
}
