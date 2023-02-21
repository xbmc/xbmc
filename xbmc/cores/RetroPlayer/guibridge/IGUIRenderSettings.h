/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/RetroPlayer/rendering/RenderSettings.h"
#include "utils/Geometry.h"

namespace KODI
{
namespace RETRO
{
/*!
 * \brief Interface to pass render settings from the GUI to the renderer
 */
class IGUIRenderSettings
{
public:
  virtual ~IGUIRenderSettings() = default;

  /*!
   * \brief Returns true if this render target has a video filter set
   */
  virtual bool HasVideoFilter() const { return true; }

  /*!
   * \brief Returns true if this render target has a stretch mode set
   */
  virtual bool HasStretchMode() const { return true; }

  /*!
   * \brief Returns true if this render target has a video rotation set
   */
  virtual bool HasRotation() const { return true; }

  /*!
   * \brief Returns true if this render target has a path to a savestate for
   *        showing pixel data
   */
  virtual bool HasPixels() const { return true; }

  /*!
   * \brief Get the settings used to render this target
   *
   * \return The render settings
   */
  virtual CRenderSettings GetSettings() const = 0;

  /*!
   * \brief Get the dimensions of this target
   *
   * Dimensions are ignored for fullscreen windows.
   *
   * \return The destination dimensions, or unused for fullscreen window
   */
  virtual CRect GetDimensions() const { return CRect{}; }
};
} // namespace RETRO
} // namespace KODI
