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

#include "RenderTranslator.h"

using namespace KODI;
using namespace RETRO;

const char *CRenderTranslator::TranslatePixelFormat(AVPixelFormat format)
{
  switch (format)
  {
  case AV_PIX_FMT_0RGB32:
    return "0RGB32";
  case AV_PIX_FMT_RGB565:
    return "RGB565";
  case AV_PIX_FMT_RGB555:
    return "RGB555";
  default:
    break;
  }

  return "unknown";
}

const char *CRenderTranslator::TranslateScalingMethod(ESCALINGMETHOD scalingMethod)
{
  switch (scalingMethod)
  {
  case VS_SCALINGMETHOD_NEAREST:
    return "nearest";
  case VS_SCALINGMETHOD_LINEAR:
    return "linear";
  default:
    break;
  }

  return "";
}
