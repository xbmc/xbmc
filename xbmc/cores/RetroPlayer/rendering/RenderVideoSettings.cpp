/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RenderVideoSettings.h"

#include "utils/log.h"

using namespace KODI;
using namespace RETRO;

#define VIDEO_FILTER_NEAREST "nearest"
#define VIDEO_FILTER_LINEAR "linear"

#define VIDEO_FILTER_DEFAULT VIDEO_FILTER_NEAREST

void CRenderVideoSettings::Reset()
{
  m_scalingMethod = SCALINGMETHOD::AUTO;
  m_stretchMode = STRETCHMODE::Normal;
  m_rotationDegCCW = 0;
  m_shaderPreset.clear();
  m_pixelPath.clear();
}

bool CRenderVideoSettings::operator==(const CRenderVideoSettings& rhs) const
{
  return m_scalingMethod == rhs.m_scalingMethod && m_stretchMode == rhs.m_stretchMode &&
         m_rotationDegCCW == rhs.m_rotationDegCCW && m_shaderPreset == rhs.m_shaderPreset &&
         m_pixelPath == rhs.m_pixelPath;
}

bool CRenderVideoSettings::operator<(const CRenderVideoSettings& rhs) const
{
  if (m_shaderPreset < rhs.m_shaderPreset)
    return true;
  if (m_shaderPreset > rhs.m_shaderPreset)
    return false;

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
  // Sanity check
  if (!m_shaderPreset.empty() && m_scalingMethod != SCALINGMETHOD::AUTO)
    CLog::LogF(LOGWARNING, "Shader preset selected but scaling method is not AUTO");

  if (UsesShaderPreset())
    return m_shaderPreset;

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
    ResetShaderPreset();
  }
  else if (videoFilter == VIDEO_FILTER_LINEAR)
  {
    m_scalingMethod = SCALINGMETHOD::LINEAR;
    ResetShaderPreset();
  }
  else
  {
    m_scalingMethod = SCALINGMETHOD::AUTO;

    // Not a known video filter, assume it's a shader preset path
    SetShaderPreset(videoFilter);
  }
}

void CRenderVideoSettings::ResetShaderPreset()
{
  m_shaderPreset.clear();
}

void CRenderVideoSettings::ResetPixels()
{
  m_pixelPath.clear();
}

bool CRenderVideoSettings::UsesShaderPreset() const
{
  return !m_shaderPreset.empty() && m_scalingMethod == SCALINGMETHOD::AUTO;
}
