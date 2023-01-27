/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RenderTranslator.h"

#include <stdint.h>

using namespace KODI;
using namespace RETRO;

const char* CRenderTranslator::TranslatePixelFormat(AVPixelFormat format)
{
  switch (format)
  {
    case AV_PIX_FMT_0RGB32:
      return "0RGB32";
    case AV_PIX_FMT_RGBA:
      return "RGBA32";
    case AV_PIX_FMT_RGB565:
      return "RGB565";
    case AV_PIX_FMT_RGB555:
      return "RGB555";
    default:
      break;
  }

  return "unknown";
}

const char* CRenderTranslator::TranslateScalingMethod(SCALINGMETHOD scalingMethod)
{
  switch (scalingMethod)
  {
    case SCALINGMETHOD::NEAREST:
      return "nearest";
    case SCALINGMETHOD::LINEAR:
      return "linear";
    default:
      break;
  }

  return "";
}

unsigned int CRenderTranslator::TranslateWidthToBytes(unsigned int width, AVPixelFormat format)
{
  unsigned int bpp = 0;

  switch (format)
  {
    case AV_PIX_FMT_0RGB32:
    case AV_PIX_FMT_RGBA:
    {
      bpp = sizeof(uint32_t);
      break;
    }
    case AV_PIX_FMT_RGB555:
    {
      bpp = sizeof(uint16_t);
      break;
    }
    case AV_PIX_FMT_RGB565:
    {
      bpp = sizeof(uint16_t);
      break;
    }
    default:
      break;
  }

  return width * bpp;
}
