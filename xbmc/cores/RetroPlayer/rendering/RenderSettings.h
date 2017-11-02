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

#include "RenderGeometry.h"
#include "RenderVideoSettings.h"
#include "cores/IPlayer.h"

namespace KODI
{
namespace RETRO
{
  class CRenderSettings
  {
  public:
    CRenderSettings() { Reset(); }

    void Reset();

    bool operator==(const CRenderSettings &rhs) const;
    bool operator<(const CRenderSettings &rhs) const;

    CRenderGeometry &Geometry() { return m_geometry; }
    const CRenderGeometry &Geometry() const { return m_geometry; }

    CRenderVideoSettings &VideoSettings() { return m_videoSettings; }
    const CRenderVideoSettings &VideoSettings() const { return m_videoSettings; }

  private:
    CRenderGeometry m_geometry;
    CRenderVideoSettings m_videoSettings;
  };
}
}
