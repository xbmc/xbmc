#pragma once
/*
 *      Copyright (C) 2012 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

class CVector
{
public:
  CVector();
  CVector(float xCoord, float yCoord);
  virtual ~CVector() { }
  
  virtual void reset();
  
  const CVector operator+(const CVector &other) const;
  const CVector operator-(const CVector &other) const;
  CVector& operator+=(const CVector &other);
  CVector& operator-=(const CVector &other);
  
  float scalar(const CVector &other) const;
  float length() const;
  
  float x;
  float y;
};