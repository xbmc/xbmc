/*
 *      Copyright (C) 2017-present Team Kodi
 *      This file is part of Kodi - https://kodi.tv
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
#pragma once

#include "utils/Geometry.h"

struct RESOLUTION_INFO;

namespace KODI
{
namespace RETRO
{
  class IGUIRenderSettings;

  /*!
   * \brief Interface to expose rendering functions to GUI components
   */
  class IRenderManager
  {
  public:
    virtual ~IRenderManager() = default;

    /*!
     * \brief Render a fullscreen window
     *
     * \param bClear Whether the render area should be cleared
     * \param coordsRes Resolution that the window coordinates are in
     */
    virtual void RenderWindow(bool bClear, const RESOLUTION_INFO &coordsRes) = 0;

    /*!
     * \brief Render a game control
     *
     * \param bClear Whether the render area should be cleared
     * \param bUseAlpha Whether the graphics context's alpha should be used
     * \param renderRegion The region of the control being rendered
     * \param renderSettings The settings used to render the control
     */
    virtual void RenderControl(bool bClear, bool bUseAlpha, const CRect &renderRegion, const IGUIRenderSettings *renderSettings) = 0;

    /*!
     * \brief Clear the background of a fullscreen window
     */
    virtual void ClearBackground() = 0;
  };
}
}
