/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "semantic/SemanticTypes.h"

#include <set>
#include <string>
#include <vector>

namespace KODI
{
namespace SEMANTIC
{

/*!
 * @brief Media type filter options
 */
enum class MediaTypeFilter
{
  All,          //!< No filter, show all types
  Movies,       //!< Movies only
  TVShows,      //!< TV episodes only
  MusicVideos   //!< Music videos only
};

/*!
 * @brief Content rating filter
 */
enum class RatingFilter
{
  All,     //!< No rating filter
  G,       //!< General audiences
  PG,      //!< Parental guidance
  PG13,    //!< Parents strongly cautioned
  R,       //!< Restricted
  NC17,    //!< Adults only
  Unrated  //!< Unrated content
};

/*!
 * @brief Duration filter options
 */
enum class DurationFilter
{
  All,       //!< No duration filter
  Short,     //!< Less than 30 minutes
  Medium,    //!< 30-90 minutes
  Long       //!< More than 90 minutes
};

/*!
 * @brief Content source filter options
 */
struct SourceFilter
{
  bool includeSubtitles{true};      //!< Include subtitle content
  bool includeTranscription{true};  //!< Include transcription content
  bool includeMetadata{true};       //!< Include metadata content

  /*!
   * @brief Check if all sources are included (effectively no filter)
   */
  bool IsAll() const
  {
    return includeSubtitles && includeTranscription && includeMetadata;
  }

  /*!
   * @brief Check if any sources are enabled
   */
  bool HasAny() const
  {
    return includeSubtitles || includeTranscription || includeMetadata;
  }
};

/*!
 * @brief Year range filter
 */
struct YearRange
{
  int minYear{-1};  //!< Minimum year (-1 = no min)
  int maxYear{-1};  //!< Maximum year (-1 = no max)

  /*!
   * @brief Check if year range is active
   */
  bool IsActive() const { return minYear > 0 || maxYear > 0; }

  /*!
   * @brief Check if a year falls within the range
   */
  bool Contains(int year) const
  {
    if (!IsActive())
      return true;

    if (minYear > 0 && year < minYear)
      return false;
    if (maxYear > 0 && year > maxYear)
      return false;

    return true;
  }

  /*!
   * @brief Format year range as string
   */
  std::string ToString() const
  {
    if (!IsActive())
      return "All Years";

    if (minYear > 0 && maxYear > 0)
      return std::to_string(minYear) + " - " + std::to_string(maxYear);
    else if (minYear > 0)
      return std::to_string(minYear) + "+";
    else if (maxYear > 0)
      return "Up to " + std::to_string(maxYear);

    return "All Years";
  }
};

/*!
 * @brief Comprehensive search filter container
 *
 * This class manages all filter state for semantic search, including:
 * - Media type filtering (movies, TV shows, music videos)
 * - Genre filtering with multi-select
 * - Year range filtering
 * - Content rating filtering
 * - Duration filtering
 * - Content source filtering (subtitle, transcription, metadata)
 *
 * Features:
 * - AND combination between filter types
 * - OR combination within multi-select filters (genres)
 * - Active filter tracking and counting
 * - Filter clearing and reset
 * - Serialization for persistence
 *
 * Example usage:
 * \code
 * CSearchFilters filters;
 * filters.SetMediaType(MediaTypeFilter::Movies);
 * filters.AddGenre("Action");
 * filters.AddGenre("Thriller");
 * filters.SetYearRange(2020, 2024);
 *
 * if (filters.IsActive())
 * {
 *   auto results = searchEngine->Search(query, filters);
 * }
 * \endcode
 */
class CSearchFilters
{
public:
  CSearchFilters();
  ~CSearchFilters() = default;

  // Media type filter
  void SetMediaType(MediaTypeFilter type);
  MediaTypeFilter GetMediaType() const { return m_mediaType; }
  std::string GetMediaTypeString() const;

  // Genre filter (multi-select)
  void AddGenre(const std::string& genre);
  void RemoveGenre(const std::string& genre);
  void ClearGenres();
  bool HasGenre(const std::string& genre) const;
  const std::set<std::string>& GetGenres() const { return m_genres; }
  bool IsGenreFilterActive() const { return !m_genres.empty(); }

  // Year range filter
  void SetYearRange(int minYear, int maxYear);
  void SetMinYear(int year);
  void SetMaxYear(int year);
  void ClearYearRange();
  const YearRange& GetYearRange() const { return m_yearRange; }

  // Rating filter
  void SetRating(RatingFilter rating);
  RatingFilter GetRating() const { return m_rating; }
  std::string GetRatingString() const;

  // Duration filter
  void SetDuration(DurationFilter duration);
  DurationFilter GetDuration() const { return m_duration; }
  std::string GetDurationString() const;
  std::pair<int, int> GetDurationMinutesRange() const;

  // Source filter
  void SetSourceFilter(const SourceFilter& filter);
  const SourceFilter& GetSourceFilter() const { return m_sourceFilter; }
  void SetIncludeSubtitles(bool include);
  void SetIncludeTranscription(bool include);
  void SetIncludeMetadata(bool include);

  // Filter state queries
  bool IsActive() const;
  int GetActiveFilterCount() const;
  void Clear();

  // Serialization for persistence
  std::string Serialize() const;
  bool Deserialize(const std::string& data);

  // Filter display helpers
  std::string GetActiveFiltersDescription() const;
  std::vector<std::string> GetActiveFilterBadges() const;

private:
  MediaTypeFilter m_mediaType{MediaTypeFilter::All};
  std::set<std::string> m_genres;
  YearRange m_yearRange;
  RatingFilter m_rating{RatingFilter::All};
  DurationFilter m_duration{DurationFilter::All};
  SourceFilter m_sourceFilter;
};

} // namespace SEMANTIC
} // namespace KODI
