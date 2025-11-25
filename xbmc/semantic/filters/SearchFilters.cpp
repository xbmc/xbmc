/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SearchFilters.h"

#include "utils/JSONVariantParser.h"
#include "utils/JSONVariantWriter.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"

#include <algorithm>

using namespace KODI::SEMANTIC;

CSearchFilters::CSearchFilters()
{
  // Initialize with defaults (all filters off)
  Clear();
}

void CSearchFilters::SetMediaType(MediaTypeFilter type)
{
  m_mediaType = type;
}

std::string CSearchFilters::GetMediaTypeString() const
{
  switch (m_mediaType)
  {
    case MediaTypeFilter::All:
      return "";
    case MediaTypeFilter::Movies:
      return "movie";
    case MediaTypeFilter::TVShows:
      return "episode";
    case MediaTypeFilter::MusicVideos:
      return "musicvideo";
    default:
      return "";
  }
}

void CSearchFilters::AddGenre(const std::string& genre)
{
  if (!genre.empty())
  {
    m_genres.insert(genre);
  }
}

void CSearchFilters::RemoveGenre(const std::string& genre)
{
  m_genres.erase(genre);
}

void CSearchFilters::ClearGenres()
{
  m_genres.clear();
}

bool CSearchFilters::HasGenre(const std::string& genre) const
{
  return m_genres.find(genre) != m_genres.end();
}

void CSearchFilters::SetYearRange(int minYear, int maxYear)
{
  m_yearRange.minYear = minYear;
  m_yearRange.maxYear = maxYear;
}

void CSearchFilters::SetMinYear(int year)
{
  m_yearRange.minYear = year;
}

void CSearchFilters::SetMaxYear(int year)
{
  m_yearRange.maxYear = year;
}

void CSearchFilters::ClearYearRange()
{
  m_yearRange.minYear = -1;
  m_yearRange.maxYear = -1;
}

void CSearchFilters::SetRating(RatingFilter rating)
{
  m_rating = rating;
}

std::string CSearchFilters::GetRatingString() const
{
  switch (m_rating)
  {
    case RatingFilter::All:
      return "All Ratings";
    case RatingFilter::G:
      return "G";
    case RatingFilter::PG:
      return "PG";
    case RatingFilter::PG13:
      return "PG-13";
    case RatingFilter::R:
      return "R";
    case RatingFilter::NC17:
      return "NC-17";
    case RatingFilter::Unrated:
      return "Unrated";
    default:
      return "All Ratings";
  }
}

void CSearchFilters::SetDuration(DurationFilter duration)
{
  m_duration = duration;
}

std::string CSearchFilters::GetDurationString() const
{
  switch (m_duration)
  {
    case DurationFilter::All:
      return "All Durations";
    case DurationFilter::Short:
      return "Short (<30min)";
    case DurationFilter::Medium:
      return "Medium (30-90min)";
    case DurationFilter::Long:
      return "Long (>90min)";
    default:
      return "All Durations";
  }
}

std::pair<int, int> CSearchFilters::GetDurationMinutesRange() const
{
  switch (m_duration)
  {
    case DurationFilter::Short:
      return {0, 30};
    case DurationFilter::Medium:
      return {30, 90};
    case DurationFilter::Long:
      return {90, 999999};
    case DurationFilter::All:
    default:
      return {0, 999999};
  }
}

void CSearchFilters::SetSourceFilter(const SourceFilter& filter)
{
  m_sourceFilter = filter;
}

void CSearchFilters::SetIncludeSubtitles(bool include)
{
  m_sourceFilter.includeSubtitles = include;
}

void CSearchFilters::SetIncludeTranscription(bool include)
{
  m_sourceFilter.includeTranscription = include;
}

void CSearchFilters::SetIncludeMetadata(bool include)
{
  m_sourceFilter.includeMetadata = include;
}

bool CSearchFilters::IsActive() const
{
  return GetActiveFilterCount() > 0;
}

int CSearchFilters::GetActiveFilterCount() const
{
  int count = 0;

  if (m_mediaType != MediaTypeFilter::All)
    count++;

  if (!m_genres.empty())
    count++;

  if (m_yearRange.IsActive())
    count++;

  if (m_rating != RatingFilter::All)
    count++;

  if (m_duration != DurationFilter::All)
    count++;

  if (!m_sourceFilter.IsAll())
    count++;

  return count;
}

void CSearchFilters::Clear()
{
  m_mediaType = MediaTypeFilter::All;
  m_genres.clear();
  m_yearRange.minYear = -1;
  m_yearRange.maxYear = -1;
  m_rating = RatingFilter::All;
  m_duration = DurationFilter::All;
  m_sourceFilter.includeSubtitles = true;
  m_sourceFilter.includeTranscription = true;
  m_sourceFilter.includeMetadata = true;
}

std::string CSearchFilters::Serialize() const
{
  CVariant data(CVariant::VariantTypeObject);

  // Media type
  data["mediaType"] = static_cast<int>(m_mediaType);

  // Genres
  CVariant genresArray(CVariant::VariantTypeArray);
  for (const auto& genre : m_genres)
  {
    genresArray.push_back(genre);
  }
  data["genres"] = genresArray;

  // Year range
  data["minYear"] = m_yearRange.minYear;
  data["maxYear"] = m_yearRange.maxYear;

  // Rating
  data["rating"] = static_cast<int>(m_rating);

  // Duration
  data["duration"] = static_cast<int>(m_duration);

  // Source filter
  data["includeSubtitles"] = m_sourceFilter.includeSubtitles;
  data["includeTranscription"] = m_sourceFilter.includeTranscription;
  data["includeMetadata"] = m_sourceFilter.includeMetadata;

  std::string serialized;
  CJSONVariantWriter::Write(data, serialized, true);
  return serialized;
}

bool CSearchFilters::Deserialize(const std::string& data)
{
  if (data.empty())
    return false;

  CVariant variant;
  if (!CJSONVariantParser::Parse(data, variant))
  {
    CLog::Log(LOGERROR, "CSearchFilters: Failed to parse JSON data");
    return false;
  }

  try
  {
    // Media type
    if (variant.isMember("mediaType"))
      m_mediaType = static_cast<MediaTypeFilter>(variant["mediaType"].asInteger());

    // Genres
    if (variant.isMember("genres") && variant["genres"].isArray())
    {
      m_genres.clear();
      for (unsigned int i = 0; i < variant["genres"].size(); ++i)
      {
        m_genres.insert(variant["genres"][i].asString());
      }
    }

    // Year range
    if (variant.isMember("minYear"))
      m_yearRange.minYear = variant["minYear"].asInteger();
    if (variant.isMember("maxYear"))
      m_yearRange.maxYear = variant["maxYear"].asInteger();

    // Rating
    if (variant.isMember("rating"))
      m_rating = static_cast<RatingFilter>(variant["rating"].asInteger());

    // Duration
    if (variant.isMember("duration"))
      m_duration = static_cast<DurationFilter>(variant["duration"].asInteger());

    // Source filter
    if (variant.isMember("includeSubtitles"))
      m_sourceFilter.includeSubtitles = variant["includeSubtitles"].asBoolean();
    if (variant.isMember("includeTranscription"))
      m_sourceFilter.includeTranscription = variant["includeTranscription"].asBoolean();
    if (variant.isMember("includeMetadata"))
      m_sourceFilter.includeMetadata = variant["includeMetadata"].asBoolean();

    return true;
  }
  catch (const std::exception& ex)
  {
    CLog::Log(LOGERROR, "CSearchFilters: Exception during deserialization: {}", ex.what());
    return false;
  }
}

std::string CSearchFilters::GetActiveFiltersDescription() const
{
  if (!IsActive())
    return "No filters active";

  std::vector<std::string> parts;

  // Media type
  if (m_mediaType != MediaTypeFilter::All)
  {
    switch (m_mediaType)
    {
      case MediaTypeFilter::Movies:
        parts.push_back("Movies");
        break;
      case MediaTypeFilter::TVShows:
        parts.push_back("TV Shows");
        break;
      case MediaTypeFilter::MusicVideos:
        parts.push_back("Music Videos");
        break;
      default:
        break;
    }
  }

  // Genres
  if (!m_genres.empty())
  {
    if (m_genres.size() == 1)
    {
      parts.push_back(*m_genres.begin());
    }
    else
    {
      parts.push_back(StringUtils::Format("{} genres", m_genres.size()));
    }
  }

  // Year range
  if (m_yearRange.IsActive())
  {
    parts.push_back(m_yearRange.ToString());
  }

  // Rating
  if (m_rating != RatingFilter::All)
  {
    parts.push_back(GetRatingString());
  }

  // Duration
  if (m_duration != DurationFilter::All)
  {
    parts.push_back(GetDurationString());
  }

  // Source filter
  if (!m_sourceFilter.IsAll())
  {
    std::vector<std::string> sources;
    if (m_sourceFilter.includeSubtitles)
      sources.push_back("Subtitles");
    if (m_sourceFilter.includeTranscription)
      sources.push_back("Transcription");
    if (m_sourceFilter.includeMetadata)
      sources.push_back("Metadata");

    if (!sources.empty())
    {
      parts.push_back(StringUtils::Join(sources, ", "));
    }
  }

  return StringUtils::Join(parts, " • ");
}

std::vector<std::string> CSearchFilters::GetActiveFilterBadges() const
{
  std::vector<std::string> badges;

  // Media type
  if (m_mediaType != MediaTypeFilter::All)
  {
    switch (m_mediaType)
    {
      case MediaTypeFilter::Movies:
        badges.push_back("Movies");
        break;
      case MediaTypeFilter::TVShows:
        badges.push_back("TV Shows");
        break;
      case MediaTypeFilter::MusicVideos:
        badges.push_back("Music Videos");
        break;
      default:
        break;
    }
  }

  // Genres (show each genre separately)
  for (const auto& genre : m_genres)
  {
    badges.push_back(genre);
  }

  // Year range
  if (m_yearRange.IsActive())
  {
    badges.push_back(m_yearRange.ToString());
  }

  // Rating
  if (m_rating != RatingFilter::All)
  {
    badges.push_back(GetRatingString());
  }

  // Duration
  if (m_duration != DurationFilter::All)
  {
    badges.push_back(GetDurationString());
  }

  // Source filter (if not all)
  if (!m_sourceFilter.IsAll())
  {
    std::vector<std::string> sources;
    if (m_sourceFilter.includeSubtitles)
      sources.push_back("Subs");
    if (m_sourceFilter.includeTranscription)
      sources.push_back("Audio");
    if (m_sourceFilter.includeMetadata)
      sources.push_back("Meta");

    if (!sources.empty())
    {
      badges.push_back("Sources: " + StringUtils::Join(sources, "/"));
    }
  }

  return badges;
}
