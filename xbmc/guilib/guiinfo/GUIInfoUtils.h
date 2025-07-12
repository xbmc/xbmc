/*
 *  Copyright (C) 2005-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <optional>
#include <string>

namespace KODI::GUILIB::GUIINFO
{

class CGUIInfoUtils
{
public:
  CGUIInfoUtils() = delete;

  static std::optional<std::string> FormatAudioChannels(const std::string& format, int channels);
};

} // namespace KODI::GUILIB::GUIINFO
