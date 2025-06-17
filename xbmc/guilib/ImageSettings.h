/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

namespace KODI
{
namespace GUILIB
{

enum class IMAGE_FILTER
{
  UNKNOWN,
  LINEAR,
  NEAREST,
};

class ImageSettings
{
public:
  static std::string TranslateImageFilter(IMAGE_FILTER filter);
  static IMAGE_FILTER TranslateImageFilter(const std::string& filter);
};

} // namespace GUILIB
} // namespace KODI
