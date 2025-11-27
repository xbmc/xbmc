/*
 *  Copyright (C) 2005-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "ContentParserBase.h"

#include <string>
#include <vector>

class CVideoInfoTag;
class TiXmlElement;

namespace KODI::SEMANTIC
{

/*!
 * \brief Parser for extracting searchable metadata from NFO files and video info
 *
 * This parser handles Kodi's NFO (XML) metadata files and can extract searchable
 * text from plot summaries, taglines, outlines, genres, and user tags. It also
 * supports parsing directly from CVideoInfoTag objects for already-loaded metadata.
 *
 * Supported metadata fields:
 * - <plot> - Main synopsis (highest priority)
 * - <tagline> - Marketing tagline
 * - <outline> - Brief description
 * - <genre> - Genre tags (concatenated)
 * - <tag> - User-defined tags
 */
class CMetadataParser : public ContentParserBase
{
public:
  ~CMetadataParser() override = default;

  /*!
   * \brief Parse metadata from an NFO file
   * \param path The file path to the NFO file
   * \return Vector of parsed entries (no timestamps)
   * \throws std::runtime_error if file cannot be read or parsed
   */
  std::vector<ParsedEntry> Parse(const std::string& path) override;

  /*!
   * \brief Check if parser can handle this file
   * \param path The file path to check
   * \return true if file has .nfo extension
   */
  bool CanParse(const std::string& path) const override;

  /*!
   * \brief Get supported file extensions
   * \return Vector containing "nfo"
   */
  std::vector<std::string> GetSupportedExtensions() const override;

  /*!
   * \brief Parse metadata from VideoInfoTag directly (no file needed)
   * \param tag The VideoInfoTag containing metadata
   * \return Vector of parsed entries (no timestamps)
   *
   * This method extracts searchable text from already-loaded video metadata,
   * avoiding the need to re-read and parse NFO files. Useful when metadata
   * is already available in memory.
   */
  std::vector<ParsedEntry> ParseFromVideoInfo(const CVideoInfoTag& tag);

private:
  /*!
   * \brief Parse NFO XML content
   * \param content The XML content to parse
   * \return Vector of parsed entries
   */
  std::vector<ParsedEntry> ParseNFO(const std::string& content);

  /*!
   * \brief Extract plot text from XML
   * \param movie The XML element containing video metadata
   * \return Plot text or empty string
   */
  std::string ExtractPlot(const TiXmlElement* movie);

  /*!
   * \brief Extract tagline text from XML
   * \param movie The XML element containing video metadata
   * \return Tagline text or empty string
   */
  std::string ExtractTagline(const TiXmlElement* movie);

  /*!
   * \brief Extract outline text from XML
   * \param movie The XML element containing video metadata
   * \return Outline text or empty string
   */
  std::string ExtractOutline(const TiXmlElement* movie);

  /*!
   * \brief Extract and concatenate genre tags from XML
   * \param movie The XML element containing video metadata
   * \return Concatenated genre string or empty string
   */
  std::string ExtractGenres(const TiXmlElement* movie);

  /*!
   * \brief Extract and concatenate user tags from XML
   * \param movie The XML element containing video metadata
   * \return Concatenated tags string or empty string
   */
  std::string ExtractTags(const TiXmlElement* movie);

  /*!
   * \brief Create a ParsedEntry for metadata text
   * \param text The metadata text
   * \return ParsedEntry with no timestamp and confidence 1.0
   */
  ParsedEntry CreateMetadataEntry(const std::string& text);
};

} // namespace KODI::SEMANTIC
