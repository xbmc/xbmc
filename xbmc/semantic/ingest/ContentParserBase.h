/*
 *  Copyright (C) 2005-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IContentParser.h"

#include <string>

namespace KODI::SEMANTIC
{

/*!
 * \brief Base class providing common utilities for content parsers
 *
 * This class provides helper methods for common parsing tasks such as
 * HTML tag removal, whitespace normalization, charset detection, and
 * filtering non-dialogue content.
 */
class ContentParserBase : public IContentParser
{
public:
  ~ContentParserBase() override = default;

protected:
  /*!
   * \brief Strip HTML/formatting tags from text
   * \param text The text to process (modified in-place)
   *
   * Removes HTML tags like <b>, <i>, <font>, etc. while preserving
   * the text content. Handles both subtitle formatting and HTML entities.
   */
  static void StripHTMLTags(std::string& text);

  /*!
   * \brief Normalize whitespace in text
   * \param text The text to process (modified in-place)
   *
   * Removes duplicate spaces/tabs, trims leading/trailing whitespace,
   * and normalizes line endings to \n.
   */
  static void NormalizeWhitespace(std::string& text);

  /*!
   * \brief Detect charset of content and convert to UTF-8
   * \param content The content to analyze and convert (modified in-place)
   * \return true if conversion was successful, false otherwise
   *
   * Attempts to detect if content is UTF-8, Latin-1, or other encodings
   * and converts to UTF-8. Handles BOMs and common subtitle encodings.
   */
  static bool DetectCharset(std::string& content);

  /*!
   * \brief Check if text is non-dialogue content
   * \param text The text to check
   * \return true if text is non-dialogue (sound effects, music, etc.)
   *
   * Filters out common non-dialogue indicators like [music], ♪, sound
   * effects in square brackets, etc.
   */
  static bool IsNonDialogue(const std::string& text);

  /*!
   * \brief Check if file extension is supported
   * \param path The file path
   * \param extensions Vector of supported extensions (without leading dot)
   * \return true if file extension matches any supported extension
   */
  static bool HasSupportedExtension(const std::string& path,
                                    const std::vector<std::string>& extensions);
};

} // namespace KODI::SEMANTIC
