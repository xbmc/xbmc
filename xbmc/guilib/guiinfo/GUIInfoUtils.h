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

namespace KODI::UTILS
{
class CLanguageTag;
}

namespace KODI::GUILIB::GUIINFO
{

class CGUIInfoUtils
{
public:
  CGUIInfoUtils() = delete;

  static std::optional<std::string> FormatAudioChannels(const std::string& format, int channels);

  /*!
   * \brief The name of a language, for a caller that has to show one either way.
   * \note Where showing nothing is the better answer, ask CLanguageTag::GetEnglishName instead
   *       and omit what it does not name.
   * \note Do not use for a value that leaves Kodi, which must not be localized.
   * \param[in] language The language of a stream.
   * \return The English name of the language, or a localized "Unknown" when it has no name.
   */
  static std::string FormatLanguage(const UTILS::CLanguageTag& language);
};

} // namespace KODI::GUILIB::GUIINFO
