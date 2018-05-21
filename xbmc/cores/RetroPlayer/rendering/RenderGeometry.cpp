/*
 *      Copyright (C) 2017-present Team Kodi
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

#include "RenderGeometry.h"
#include "settings/GameSettings.h"
#include "settings/MediaSettings.h"

using namespace KODI;
using namespace RETRO;

CRenderGeometry::CRenderGeometry(const CRect &dimensions) :
  m_dimensions(dimensions)
{
}

void CRenderGeometry::Reset()
{
}

bool CRenderGeometry::operator==(const CRenderGeometry &rhs) const
{
  return m_dimensions == rhs.m_dimensions;
}

bool CRenderGeometry::operator<(const CRenderGeometry &rhs) const
{
  return m_dimensions.Area() < rhs.m_dimensions.Area();
}
