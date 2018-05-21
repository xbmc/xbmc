/*
 *      Copyright (C) 2017-present Team Kodi
 *      http://kodi.tv
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

#include "cores/IPlayer.h"

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
    static const char *TranslateScalingMethod(ESCALINGMETHOD scalingMethod);
  };
}
}
