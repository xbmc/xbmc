/*
 *  Copyright (C) 2005-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace KODI::SEMANTIC
{

/*!
 * \brief Represents a single parsed content entry with timing information
 */
struct ParsedEntry
{
  int64_t startMs; //!< Start time in milliseconds
  int64_t endMs; //!< End time in milliseconds
  std::string text; //!< The content text
  std::string speaker; //!< Optional speaker identification
  float confidence{1.0f}; //!< Confidence score (0.0-1.0)
};

/*!
 * \brief Interface for parsing various content formats
 *
 * This interface provides a common abstraction for parsing subtitle files,
 * transcription formats, and metadata into time-aligned text entries.
 * Implementations can handle formats like SRT, VTT, JSON transcriptions, etc.
 */
class IContentParser
{
public:
  virtual ~IContentParser() = default;

  /*!
   * \brief Parse content from a file path
   * \param path The file path to parse
   * \return Vector of parsed entries with timing information
   * \throws std::runtime_error if parsing fails
   */
  virtual std::vector<ParsedEntry> Parse(const std::string& path) = 0;

  /*!
   * \brief Check if parser can handle this file
   * \param path The file path to check
   * \return true if this parser can handle the file, false otherwise
   */
  virtual bool CanParse(const std::string& path) const = 0;

  /*!
   * \brief Get supported file extensions
   * \return Vector of supported extensions (without leading dot)
   */
  virtual std::vector<std::string> GetSupportedExtensions() const = 0;
};

} // namespace KODI::SEMANTIC
