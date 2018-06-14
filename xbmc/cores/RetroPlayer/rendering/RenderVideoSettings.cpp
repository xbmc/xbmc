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

#define VIDEO_FILTER_NEAREST  "nearest"
#define VIDEO_FILTER_LINEAR   "linear"

void CRenderVideoSettings::Reset()
{
  m_scalingMethod = SCALINGMETHOD::AUTO;
  m_viewMode = VIEWMODE::Normal;
  m_rotationDegCCW = 0;
}

bool CRenderVideoSettings::operator==(const CRenderVideoSettings &rhs) const
{
  return m_scalingMethod == rhs.m_scalingMethod &&
         m_viewMode == rhs.m_viewMode &&
         m_rotationDegCCW == rhs.m_rotationDegCCW;
}

bool CRenderVideoSettings::operator<(const CRenderVideoSettings &rhs) const
{
  if (m_scalingMethod < rhs.m_scalingMethod) return true;
  if (m_scalingMethod > rhs.m_scalingMethod) return false;

  if (m_viewMode < rhs.m_viewMode) return true;
  if (m_viewMode > rhs.m_viewMode) return false;

  if (m_rotationDegCCW < rhs.m_rotationDegCCW) return true;
  if (m_rotationDegCCW > rhs.m_rotationDegCCW) return false;

  return false;
}

std::string CRenderVideoSettings::GetVideoFilter() const
{
  switch (m_scalingMethod)
  {
  case SCALINGMETHOD::NEAREST:
    return VIDEO_FILTER_NEAREST;
  case SCALINGMETHOD::LINEAR:
    return VIDEO_FILTER_LINEAR;
  default:
    break;
  }

  return "";
}

void CRenderVideoSettings::SetVideoFilter(const std::string &videoFilter)
{
  if (videoFilter == VIDEO_FILTER_NEAREST)
  {
    m_scalingMethod = SCALINGMETHOD::NEAREST;
  }
  else if (videoFilter == VIDEO_FILTER_LINEAR)
  {
    m_scalingMethod = SCALINGMETHOD::LINEAR;
  }
  else
  {
    m_scalingMethod = SCALINGMETHOD::AUTO;
  }
}
