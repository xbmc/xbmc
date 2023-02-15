/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RenderVideoSettings.h"

using namespace KODI;
using namespace RETRO;

#define VIDEO_FILTER_NEAREST "nearest"
#define VIDEO_FILTER_LINEAR "linear"

void CRenderVideoSettings::Reset()
{
  m_scalingMethod = SCALINGMETHOD::AUTO;
  m_stretchMode = STRETCHMODE::Normal;
  m_rotationDegCCW = 0;
  m_pixelPath.clear();
}

bool CRenderVideoSettings::operator==(const CRenderVideoSettings& rhs) const
{
  return m_scalingMethod == rhs.m_scalingMethod && m_stretchMode == rhs.m_stretchMode &&
         m_rotationDegCCW == rhs.m_rotationDegCCW && m_pixelPath == rhs.m_pixelPath;
}

bool CRenderVideoSettings::operator<(const CRenderVideoSettings& rhs) const
{
  if (m_scalingMethod < rhs.m_scalingMethod)
    return true;
  if (m_scalingMethod > rhs.m_scalingMethod)
    return false;

  if (m_stretchMode < rhs.m_stretchMode)
    return true;
  if (m_stretchMode > rhs.m_stretchMode)
    return false;

  if (m_rotationDegCCW < rhs.m_rotationDegCCW)
    return true;
  if (m_rotationDegCCW > rhs.m_rotationDegCCW)
    return false;

  if (m_pixelPath < rhs.m_pixelPath)
    return true;
  if (m_pixelPath > rhs.m_pixelPath)
    return false;

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

void CRenderVideoSettings::SetVideoFilter(const std::string& videoFilter)
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
void CRenderVideoSettings::ResetPixels()
{
  m_pixelPath.clear();
}
