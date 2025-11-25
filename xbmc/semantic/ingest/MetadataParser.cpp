/*
 *  Copyright (C) 2005-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "MetadataParser.h"

#include "filesystem/File.h"
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML.h"
#include "utils/XMLUtils.h"
#include "video/VideoInfoTag.h"

#include <stdexcept>

using namespace KODI::SEMANTIC;

std::vector<ParsedEntry> CMetadataParser::Parse(const std::string& path)
{
  if (!CanParse(path))
  {
    throw std::runtime_error("MetadataParser: Unsupported file type: " + path);
  }

  // Read file contents
  XFILE::CFile file;
  if (!file.Open(path))
  {
    throw std::runtime_error("MetadataParser: Failed to open file: " + path);
  }

  std::string content;
  const int64_t fileSize = file.GetLength();
  if (fileSize <= 0 || fileSize > 10 * 1024 * 1024) // 10MB limit
  {
    file.Close();
    throw std::runtime_error("MetadataParser: Invalid file size: " + path);
  }

  content.resize(static_cast<size_t>(fileSize));
  ssize_t bytesRead = file.Read(content.data(), fileSize);
  file.Close();

  if (bytesRead != fileSize)
  {
    throw std::runtime_error("MetadataParser: Failed to read file: " + path);
  }

  // Handle charset conversion
  DetectCharset(content);

  return ParseNFO(content);
}

bool CMetadataParser::CanParse(const std::string& path) const
{
  return HasSupportedExtension(path, GetSupportedExtensions());
}

std::vector<std::string> CMetadataParser::GetSupportedExtensions() const
{
  return {"nfo"};
}

std::vector<ParsedEntry> CMetadataParser::ParseFromVideoInfo(const CVideoInfoTag& tag)
{
  std::vector<ParsedEntry> entries;

  // Extract plot (highest priority)
  if (!tag.m_strPlot.empty())
  {
    entries.push_back(CreateMetadataEntry(tag.m_strPlot));
  }

  // Extract tagline
  if (!tag.m_strTagLine.empty())
  {
    entries.push_back(CreateMetadataEntry(tag.m_strTagLine));
  }

  // Extract plot outline
  if (!tag.m_strPlotOutline.empty())
  {
    entries.push_back(CreateMetadataEntry(tag.m_strPlotOutline));
  }

  // Extract genres
  if (!tag.m_genre.empty())
  {
    std::string genreText = StringUtils::Join(tag.m_genre, ", ");
    if (!genreText.empty())
    {
      entries.push_back(CreateMetadataEntry(genreText));
    }
  }

  // Extract tags
  if (!tag.m_tags.empty())
  {
    std::string tagsText = StringUtils::Join(tag.m_tags, ", ");
    if (!tagsText.empty())
    {
      entries.push_back(CreateMetadataEntry(tagsText));
    }
  }

  return entries;
}

std::vector<ParsedEntry> CMetadataParser::ParseNFO(const std::string& content)
{
  std::vector<ParsedEntry> entries;

  // Parse XML
  CXBMCTinyXML xmlDoc;
  if (!xmlDoc.Parse(content))
  {
    throw std::runtime_error("MetadataParser: Failed to parse XML");
  }

  const TiXmlElement* root = xmlDoc.RootElement();
  if (!root)
  {
    throw std::runtime_error("MetadataParser: No root element in XML");
  }

  // Extract plot
  std::string plot = ExtractPlot(root);
  if (!plot.empty())
  {
    entries.push_back(CreateMetadataEntry(plot));
  }

  // Extract tagline
  std::string tagline = ExtractTagline(root);
  if (!tagline.empty())
  {
    entries.push_back(CreateMetadataEntry(tagline));
  }

  // Extract outline
  std::string outline = ExtractOutline(root);
  if (!outline.empty())
  {
    entries.push_back(CreateMetadataEntry(outline));
  }

  // Extract genres
  std::string genres = ExtractGenres(root);
  if (!genres.empty())
  {
    entries.push_back(CreateMetadataEntry(genres));
  }

  // Extract tags
  std::string tags = ExtractTags(root);
  if (!tags.empty())
  {
    entries.push_back(CreateMetadataEntry(tags));
  }

  return entries;
}

std::string CMetadataParser::ExtractPlot(const TiXmlElement* movie)
{
  if (!movie)
    return "";

  std::string plot;
  XMLUtils::GetString(movie, "plot", plot);

  if (!plot.empty())
  {
    // Strip HTML tags that might be in the plot
    StripHTMLTags(plot);
    NormalizeWhitespace(plot);
  }

  return plot;
}

std::string CMetadataParser::ExtractTagline(const TiXmlElement* movie)
{
  if (!movie)
    return "";

  std::string tagline;
  XMLUtils::GetString(movie, "tagline", tagline);

  if (!tagline.empty())
  {
    StripHTMLTags(tagline);
    NormalizeWhitespace(tagline);
  }

  return tagline;
}

std::string CMetadataParser::ExtractOutline(const TiXmlElement* movie)
{
  if (!movie)
    return "";

  std::string outline;
  XMLUtils::GetString(movie, "outline", outline);

  if (!outline.empty())
  {
    StripHTMLTags(outline);
    NormalizeWhitespace(outline);
  }

  return outline;
}

std::string CMetadataParser::ExtractGenres(const TiXmlElement* movie)
{
  if (!movie)
    return "";

  std::vector<std::string> genres;
  XMLUtils::GetStringArray(movie, "genre", genres);

  if (genres.empty())
    return "";

  // Concatenate genres with comma separator
  std::string genreText = StringUtils::Join(genres, ", ");
  NormalizeWhitespace(genreText);

  return genreText;
}

std::string CMetadataParser::ExtractTags(const TiXmlElement* movie)
{
  if (!movie)
    return "";

  std::vector<std::string> tags;
  XMLUtils::GetStringArray(movie, "tag", tags);

  if (tags.empty())
    return "";

  // Concatenate tags with comma separator
  std::string tagsText = StringUtils::Join(tags, ", ");
  NormalizeWhitespace(tagsText);

  return tagsText;
}

ParsedEntry CMetadataParser::CreateMetadataEntry(const std::string& text)
{
  if (text.empty())
    return {};

  ParsedEntry entry;
  entry.startMs = 0; // No timestamp for metadata
  entry.endMs = 0;
  entry.text = text;
  entry.speaker = ""; // No speaker for metadata
  entry.confidence = 1.0f; // High confidence for metadata

  return entry;
}
