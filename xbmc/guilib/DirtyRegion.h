#pragma once
/*
 *      Copyright (C) 2005-2012 Team XBMC
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

#include "Geometry.h"
#include <vector>

class CDirtyRegion : public CRect
{
public:
  CDirtyRegion(const CRect &rect) : CRect(rect) { m_age = 0; }
  CDirtyRegion(float left, float top, float right, float bottom) : CRect(left, top, right, bottom) { m_age = 0; }
  CDirtyRegion() : CRect() { m_age = 0; }

  int UpdateAge() { return ++m_age; }
private:
  int m_age;
};

typedef std::vector<CDirtyRegion> CDirtyRegionList;
