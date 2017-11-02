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

#include "RenderVideoSettings.h"

using namespace KODI;
using namespace RETRO;

void CRenderVideoSettings::Reset()
{
  m_scalingMethod = VS_SCALINGMETHOD_AUTO;
  m_viewMode = ViewModeNormal;
}

bool CRenderVideoSettings::operator==(const CRenderVideoSettings &rhs) const
{
  return m_scalingMethod == rhs.m_scalingMethod &&
         m_viewMode == rhs.m_viewMode;
}

bool CRenderVideoSettings::operator<(const CRenderVideoSettings &rhs) const
{
  if (m_scalingMethod < rhs.m_scalingMethod) return true;
  if (m_scalingMethod > rhs.m_scalingMethod) return false;

  if (m_viewMode < rhs.m_viewMode) return true;
  if (m_viewMode > rhs.m_viewMode) return false;

  return false;
}
