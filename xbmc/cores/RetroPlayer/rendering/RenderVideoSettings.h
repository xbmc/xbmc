/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/GameSettings.h"

#include <string>

namespace KODI
{
namespace RETRO
{
/*!
 * \brief Video settings provided by the rendering system
 */
class CRenderVideoSettings
{
public:
  CRenderVideoSettings() { Reset(); }

  void Reset();

  bool operator==(const CRenderVideoSettings& rhs) const;
  bool operator!=(const CRenderVideoSettings& rhs) const { return !(*this == rhs); }
  bool operator<(const CRenderVideoSettings& rhs) const;
  bool operator>(const CRenderVideoSettings& rhs) const { return !(*this == rhs || *this < rhs); }

  /*!
   * \brief Get a string representation of the video filter parameters
   */
  std::string GetVideoFilter() const;
  void SetVideoFilter(const std::string& videoFilter);

  SCALINGMETHOD GetScalingMethod() const { return m_scalingMethod; }
  void SetScalingMethod(SCALINGMETHOD method) { m_scalingMethod = method; }

  STRETCHMODE GetRenderStretchMode() const { return m_stretchMode; }
  void SetRenderStretchMode(STRETCHMODE mode) { m_stretchMode = mode; }

  unsigned int GetRenderRotation() const { return m_rotationDegCCW; }
  void SetRenderRotation(unsigned int rotationDegCCW) { m_rotationDegCCW = rotationDegCCW; }

  std::string GetPixels() const { return m_pixelPath; }
  void SetPixels(const std::string& pixelPath) { m_pixelPath = pixelPath; }
  void ResetPixels();

private:
  SCALINGMETHOD m_scalingMethod;
  STRETCHMODE m_stretchMode;
  unsigned int m_rotationDegCCW;
  std::string m_pixelPath;
};
} // namespace RETRO
} // namespace KODI
