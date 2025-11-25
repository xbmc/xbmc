/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "HybridSearchEngine.h"

#include <string>
#include <unordered_map>
#include <vector>

class CVideoDatabase;
class CVideoInfoTag;

namespace KODI
{
namespace SEMANTIC
{

/*!
 * @brief Enriched search result with full media metadata
 *
 * Combines search result data (scores, snippets, timestamps) with
 * complete media information from Kodi's video database (title,
 * plot, ratings, artwork, etc.).
 */
struct EnrichedSearchResult
{
  // From HybridSearchResult - search metadata
  int64_t chunkId{-1};       //!< Chunk ID from database
  SemanticChunk chunk;       //!< Full chunk data
  float combinedScore{0.0f}; //!< Combined RRF score
  float keywordScore{0.0f};  //!< BM25 normalized score (0-1)
  float vectorScore{0.0f};   //!< Cosine similarity score (0-1)
  std::string snippet;       //!< Text snippet for display
  std::string formattedTimestamp; //!< Formatted timestamp (e.g., "1:23:45")

  // Enriched media info - common fields
  int mediaId{-1};            //!< Media ID in video database
  std::string mediaType;      //!< Media type (movie/episode/musicvideo)
  std::string title;          //!< Media title
  std::string originalTitle;  //!< Original title (if different)
  int year{0};                //!< Release/air year
  std::string plot;           //!< Plot summary
  std::string thumbnailPath;  //!< Thumbnail image path
  std::string fanartPath;     //!< Fanart image path
  std::string filePath;       //!< File path on disk
  float rating{0.0f};         //!< Rating (0-10 scale)
  std::string genre;          //!< Genre(s) joined with " / "
  int runtime{0};             //!< Runtime in minutes

  // Episode-specific fields
  std::string showTitle;     //!< TV show title (for episodes)
  int seasonNumber{0};       //!< Season number (for episodes)
  int episodeNumber{0};      //!< Episode number (for episodes)
  std::string episodeTitle;  //!< Episode title (for episodes)

  /*!
   * @brief Get display title for UI
   * @return Show title for episodes, regular title for movies/music videos
   */
  std::string GetDisplayTitle() const;

  /*!
   * @brief Get subtitle for UI (season/episode info or year)
   * @return Formatted subtitle string
   */
  std::string GetSubtitle() const;
};

/*!
 * @brief Result enricher that augments search results with media metadata
 *
 * Takes search results from HybridSearchEngine and joins them with full
 * media information from Kodi's video database. Caches media info to
 * avoid repeated database lookups.
 *
 * Example usage:
 * \code
 * CResultEnricher enricher;
 * enricher.Initialize(&videoDatabase);
 *
 * auto searchResults = hybridSearch.Search("detective solving mystery");
 * auto enrichedResults = enricher.EnrichBatch(searchResults);
 *
 * for (const auto& result : enrichedResults)
 * {
 *   std::cout << result.GetDisplayTitle() << " - " << result.snippet << std::endl;
 * }
 * \endcode
 */
class CResultEnricher
{
public:
  CResultEnricher();
  ~CResultEnricher();

  /*!
   * @brief Initialize enricher with video database
   * @param videoDatabase Pointer to video database (must remain valid)
   * @return true if initialization succeeded, false otherwise
   */
  bool Initialize(CVideoDatabase* videoDatabase);

  /*!
   * @brief Check if the enricher is initialized
   * @return true if initialized and ready to enrich results
   */
  bool IsInitialized() const;

  /*!
   * @brief Enrich a single search result with media metadata
   * @param result Search result from HybridSearchEngine
   * @return Enriched result with full media information
   */
  EnrichedSearchResult Enrich(const HybridSearchResult& result);

  /*!
   * @brief Enrich multiple results in batch (more efficient)
   * @param results Vector of search results from HybridSearchEngine
   * @return Vector of enriched results with full media information
   *
   * Batch processing is more efficient than individual calls because
   * it can cache media info across multiple results from the same media item.
   */
  std::vector<EnrichedSearchResult> EnrichBatch(const std::vector<HybridSearchResult>& results);

  /*!
   * @brief Format enriched result for list display
   * @param result Enriched search result
   * @return Formatted string with title, subtitle, snippet, and timestamp
   */
  std::string FormatForList(const EnrichedSearchResult& result);

  /*!
   * @brief Clear the media info cache
   *
   * Call this if the video database has been updated and cached
   * information may be stale.
   */
  void ClearCache();

private:
  CVideoDatabase* m_videoDb{nullptr};

  // Media info cache (mediaType:mediaId -> CVideoInfoTag)
  std::unordered_map<std::string, CVideoInfoTag> m_infoCache;

  /*!
   * @brief Generate cache key for media item
   * @param mediaId Media ID in video database
   * @param mediaType Media type string (movie/episode/musicvideo)
   * @return Cache key string
   */
  std::string MakeCacheKey(int mediaId, const std::string& mediaType) const;

  /*!
   * @brief Load movie info from video database
   * @param mediaId Movie ID
   * @param tag Output video info tag
   * @return true if info was loaded successfully
   */
  bool LoadMovieInfo(int mediaId, CVideoInfoTag& tag);

  /*!
   * @brief Load episode info from video database
   * @param mediaId Episode ID
   * @param tag Output video info tag
   * @return true if info was loaded successfully
   */
  bool LoadEpisodeInfo(int mediaId, CVideoInfoTag& tag);

  /*!
   * @brief Load music video info from video database
   * @param mediaId Music video ID
   * @param tag Output video info tag
   * @return true if info was loaded successfully
   */
  bool LoadMusicVideoInfo(int mediaId, CVideoInfoTag& tag);

  /*!
   * @brief Extract common fields from CVideoInfoTag
   * @param tag Video info tag from database
   * @param result Output enriched result
   */
  void ExtractCommonFields(const CVideoInfoTag& tag, EnrichedSearchResult& result);

  /*!
   * @brief Extract episode-specific fields from CVideoInfoTag
   * @param tag Video info tag from database
   * @param result Output enriched result
   */
  void ExtractEpisodeFields(const CVideoInfoTag& tag, EnrichedSearchResult& result);
};

} // namespace SEMANTIC
} // namespace KODI
