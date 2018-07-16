/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
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
