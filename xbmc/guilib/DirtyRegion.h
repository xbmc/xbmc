/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/Geometry.h"

#include <vector>

class CDirtyRegion : public CRect
{
public:
  explicit CDirtyRegion(const CRect &rect) : CRect(rect) { m_age = 0; }
  CDirtyRegion(float left, float top, float right, float bottom) : CRect(left, top, right, bottom) { m_age = 0; }
  CDirtyRegion() : CRect() { m_age = 0; }

  int UpdateAge() { return ++m_age; }
private:
  int m_age;
};

typedef std::vector<CDirtyRegion> CDirtyRegionList;
