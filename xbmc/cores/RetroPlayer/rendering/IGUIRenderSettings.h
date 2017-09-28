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

#include "RenderSettings.h"

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
     * \brief Returns true if this render target has a scaling method set
     */
    virtual bool HasScalingMethod() const { return true; }

    /*!
     * \brief Returns true if this render target has a view mode set
     */
    virtual bool HasViewMode() const { return true; }

    /*!
     * \brief Get the settings used to render this target
     *
     * \return The render settings
     */
    virtual CRenderSettings GetSettings() const = 0;
  };
}
}
