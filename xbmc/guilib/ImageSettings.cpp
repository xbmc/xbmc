/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ImageSettings.h"

using namespace KODI;
using namespace GUILIB;

namespace
{
constexpr auto IMAGE_FILTER_UNKNOWN = "unknown";
constexpr auto IMAGE_FILTER_LINEAR = "linear";
constexpr auto IMAGE_FILTER_NEAREST = "nearest";
} // namespace

std::string ImageSettings::TranslateImageFilter(IMAGE_FILTER filter)
{
  switch (filter)
  {
    case IMAGE_FILTER::LINEAR:
      return IMAGE_FILTER_LINEAR;
    case IMAGE_FILTER::NEAREST:
      return IMAGE_FILTER_NEAREST;
    default:
      break;
  }

  return IMAGE_FILTER_UNKNOWN;
}

IMAGE_FILTER ImageSettings::TranslateImageFilter(const std::string& filter)
{
  if (filter == IMAGE_FILTER_LINEAR)
    return IMAGE_FILTER::LINEAR;
  else if (filter == IMAGE_FILTER_NEAREST)
    return IMAGE_FILTER::NEAREST;

  return IMAGE_FILTER::UNKNOWN;
}
