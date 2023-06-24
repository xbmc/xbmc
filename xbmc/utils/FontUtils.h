/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <set>
#include <stdint.h>
#include <string>
#include <vector>

namespace UTILS
{
namespace FONT
{
constexpr const char* SUPPORTED_EXTENSIONS_MASK = ".ttf|.ttc|.otf";

// The default application font
constexpr const char* FONT_DEFAULT_FILENAME = "arial.ttf";

namespace FONTPATH
{
// Directory where Kodi bundled fonts files are located
constexpr const char* SYSTEM = "special://xbmc/media/Fonts/";
// Directory where user defined fonts are located
constexpr const char* USER = "special://home/media/Fonts/";
// Temporary font path (where MKV fonts are extracted and temporarily stored)
constexpr const char* TEMP = "special://temp/fonts/";

/*!
 *  \brief Provided a font filename returns the complete path for the font in
 *  the system font folder (if it exists) or an empty string otherwise
 *  \param filename The font file name
 *  \return The path for the font or an empty string if the path does not exist
 */
std::string GetSystemFontPath(const std::string& filename);
}; // namespace FONTPATH

/*!
 *  \brief Get the font family name from a font file,
 *         in case of font collection (.ttc) will take the family name of all fonts
 *  \param buffer The font data
 *  \param familyNames The font family names
 *  \return True if success, otherwise false when an error occurs
 */
bool GetFontFamilyNames(const std::vector<uint8_t>& buffer, std::set<std::string>& familyNames);

/*!
 *  \brief Get the font family name from a font file,
 *         in case of font collection (.ttc) will take the family name of all fonts
 *  \param filepath The path where read the font data
 *  \param familyNames The font family names
 *  \return True if success, otherwise false when an error occurs
 */
bool GetFontFamilyNames(const std::string& filepath, std::set<std::string>& familyNames);

/*!
 *  \brief Get the font family name from a font file,
 *         in case of font collection (.ttc) will take the first available
 *  \param buffer The font data
 *  \return The font family name, otherwise empty if fails
 */
std::string GetFontFamily(std::vector<uint8_t>& buffer);

/*!
 *  \brief Get the font family name from a font file,
 *         in case of font collection (.ttc) will take the first available
 *  \param filepath The path where read the font data
 *  \return The font family name, otherwise empty if fails
 */
std::string GetFontFamily(const std::string& filepath);

/*!
 *  \brief Check if a filename have a supported font extension.
 *  \param filepath The font file path
 *  \return True if it has a supported extension, otherwise false
 */
bool IsSupportedFontExtension(const std::string& filepath);

/*!
 *  \brief Removes all temporary fonts, e.g.those extract from MKV containers
 *  that are only available during playback
 */
void ClearTemporaryFonts();

} // namespace FONT
} // namespace UTILS
