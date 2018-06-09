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

#include "cores/GameSettings.h"

#include "libavutil/pixfmt.h"

namespace KODI
{
namespace RETRO
{
  class CRenderTranslator
  {
  public:
    /*!
     * \brief Translate a pixel format to a string suitable for logging
     */
    static const char *TranslatePixelFormat(AVPixelFormat format);

    /*!
     * \brief Translate a scaling method to a string suitable for logging
     */
    static const char *TranslateScalingMethod(SCALINGMETHOD scalingMethod);

    /*!
     * \brief Translate a width in pixels to a width in bytes
     *
     * \param width The width in pixels
     * \param format The pixel format
     *
     * \return The width in bytes, or 0 if unknown
     */
    static unsigned int TranslateWidthToBytes(unsigned int width, AVPixelFormat format);
  };
}
}
