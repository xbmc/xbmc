/*
 *  Copyright (C) 2005-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIInfoUtils.h"

#include "utils/StreamUtils.h"
#include "utils/StringUtils.h"

using namespace KODI::GUILIB::GUIINFO;

std::optional<std::string> CGUIInfoUtils::FormatAudioChannels(const std::string& format,
                                                              int channels)
{
  if (channels > 0)
  {
    if (format.empty())
    {
      return std::to_string(channels);
    }
    else if (StringUtils::EqualsNoCase(format, "defaultlayout"))
    {
      return StreamUtils::GetLayout(static_cast<unsigned int>(channels));
    }
  }

  return std::nullopt;
}
