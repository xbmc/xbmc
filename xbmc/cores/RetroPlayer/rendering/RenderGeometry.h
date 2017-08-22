/*
 *      Copyright (C) 2017 Team Kodi
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

#include "guilib/Geometry.h"

namespace KODI
{
namespace RETRO
{
  class CRenderGeometry
  {
  public:
    CRenderGeometry() { Reset(); }
    CRenderGeometry(const CRect &dimensions);

    void Reset();

    bool operator==(const CRenderGeometry &rhs) const;
    bool operator!=(const CRenderGeometry &rhs) const { return !(*this == rhs); }
    bool operator<(const CRenderGeometry &rhs) const;
    bool operator>(const CRenderGeometry &rhs) const { return !(*this == rhs || *this < rhs); }

    CRect &Dimensions() { return m_dimensions; }
    const CRect &Dimensions() const { return m_dimensions; }

  private:
    CRect m_dimensions;
  };
}
}
