/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <stdint.h>
#include <string>
#include <vector>

namespace UTILS
{
namespace FONT
{
constexpr const char* SUPPORTED_EXTENSIONS_MASK = ".ttf|.otf";

// The default application font
constexpr const char* FONT_DEFAULT_FILENAME = "arial.ttf";

// Prefix used to store temporary font files in the user fonts folder
constexpr const char* TEMP_FONT_FILENAME_PREFIX = "tmp.font.";

/*!
 *  \brief Get the font family name from a font file
 *  \param buffer The font data
 *  \return The font family name, otherwise empty if fails
 */
std::string GetFontFamily(std::vector<uint8_t>& buffer);

/*!
 *  \brief Get the font family name from a font file
 *  \param filepath The path where read the font data
 *  \return The font family name, otherwise empty if fails
 */
std::string GetFontFamily(const std::string& filepath);

/*!
 *  \brief Check if a file is a temporary font file. Temporary fonts are
 *         extracted by the demuxer from MKV files and stored in the user
 *         fonts folder.
 *  \param filepath The font file path
 *  \return True if it is a temporary file, otherwise false
 */
bool IsTemporaryFontFile(const std::string& filepath);

/*!
 *  \brief Check if a filename have a supported font extension.
 *  \param filepath The font file path
 *  \return True if it has a supported extension, otherwise false
 */
bool IsSupportedFontExtension(const std::string& filepath);
} // namespace FONT
} // namespace UTILS
