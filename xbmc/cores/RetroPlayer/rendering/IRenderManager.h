/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/RetroPlayer/rendering/RenderVideoSettings.h"
#include "utils/Geometry.h"

#include <cstdint>

struct RESOLUTION_INFO;

namespace KODI
{
namespace RETRO
{
class IGUIRenderSettings;

/*!
 * \brief What a render target last drew
 */
struct RenderTargetState
{
  bool drawn{false};
  uint64_t frame{0};
  CRenderVideoSettings settings;
  bool hdrWithVideoFilters{false};
};

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
   * \param videoLayer True for the pass under the GUI, which the game is drawn
   *        in when it's shown in HDR, false for the GUI's own pass
   */
  virtual void RenderWindow(bool bClear, const RESOLUTION_INFO& coordsRes, bool videoLayer) = 0;

  /*!
   * \brief Render a game control
   *
   * \param bClear Whether the render area should be cleared
   * \param bUseAlpha Whether the graphics context's alpha should be used
   * \param renderRegion The region of the control being rendered
   * \param renderSettings The settings used to render the control
   *
   * \return True if the game was drawn
   */
  virtual bool RenderControl(bool bClear,
                             bool bUseAlpha,
                             const CRect& renderRegion,
                             const IGUIRenderSettings* renderSettings) = 0;

  /*!
   * \brief Clear the background of a fullscreen window
   */
  virtual void ClearBackground() = 0;

  /*!
   * \brief Check if a render target needs to draw the game again
   *
   * A paused game may not need drawing again until its frame or its settings
   * change.
   *
   * \param renderSettings The settings of the control, or nullptr for the
   *        fullscreen window
   * \param[in,out] state What the render target last drew
   *
   * \return True if the game needs drawing
   */
  virtual bool NeedsDrawing(const IGUIRenderSettings* renderSettings, RenderTargetState& state) = 0;
};
} // namespace RETRO
} // namespace KODI
