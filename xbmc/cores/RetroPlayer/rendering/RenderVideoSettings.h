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

    bool operator==(const CRenderVideoSettings &rhs) const;
    bool operator!=(const CRenderVideoSettings &rhs) const { return !(*this == rhs); }
    bool operator<(const CRenderVideoSettings &rhs) const;
    bool operator>(const CRenderVideoSettings &rhs) const { return !(*this == rhs || *this < rhs); }

    /*!
     * \brief Get a string representation of the video filter parameters
     */
    std::string GetVideoFilter() const;
    void SetVideoFilter(const std::string &videoFilter);

    SCALINGMETHOD GetScalingMethod() const { return m_scalingMethod; }
    void SetScalingMethod(SCALINGMETHOD method) { m_scalingMethod = method; }

    VIEWMODE GetRenderViewMode() const { return m_viewMode; }
    void SetRenderViewMode(VIEWMODE mode) { m_viewMode = mode; }

    unsigned int GetRenderRotation() const { return m_rotationDegCCW; }
    void SetRenderRotation(unsigned int rotationDegCCW) { m_rotationDegCCW = rotationDegCCW; }

  private:
    SCALINGMETHOD m_scalingMethod;
    VIEWMODE m_viewMode;
    unsigned int m_rotationDegCCW;
  };
}
}
