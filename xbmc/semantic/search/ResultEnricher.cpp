/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ResultEnricher.h"

#include "utils/StringUtils.h"
#include "utils/log.h"
#include "video/VideoDatabase.h"
#include "video/VideoInfoTag.h"

using namespace KODI::SEMANTIC;

// ============================================================================
// EnrichedSearchResult Display Helpers
// ============================================================================

std::string EnrichedSearchResult::GetDisplayTitle() const
{
  // For episodes, show the series title
  if (mediaType == "episode" && !showTitle.empty())
    return showTitle;

  // For movies and music videos, show the regular title
  return title;
}

std::string EnrichedSearchResult::GetSubtitle() const
{
  // For episodes, format as "S02E05 - Episode Title"
  if (mediaType == "episode")
  {
    return StringUtils::Format("S{:02d}E{:02d} - {}", seasonNumber, episodeNumber,
                               episodeTitle.c_str());
  }

  // For movies/music videos, show the year
  if (year > 0)
    return std::to_string(year);

  return "";
}

// ============================================================================
// CResultEnricher Implementation
// ============================================================================

CResultEnricher::CResultEnricher() = default;

CResultEnricher::~CResultEnricher() = default;

bool CResultEnricher::Initialize(CVideoDatabase* videoDatabase)
{
  if (videoDatabase == nullptr)
  {
    CLog::LogF(LOGERROR, "Cannot initialize with null video database pointer");
    return false;
  }

  m_videoDb = videoDatabase;
  CLog::LogF(LOGDEBUG, "Result enricher initialized successfully");
  return true;
}

bool CResultEnricher::IsInitialized() const
{
  return m_videoDb != nullptr;
}

EnrichedSearchResult CResultEnricher::Enrich(const HybridSearchResult& searchResult)
{
  EnrichedSearchResult result;

  // Copy search result fields
  result.chunkId = searchResult.chunkId;
  result.chunk = searchResult.chunk;
  result.combinedScore = searchResult.combinedScore;
  result.keywordScore = searchResult.keywordScore;
  result.vectorScore = searchResult.vectorScore;
  result.snippet = searchResult.snippet;
  result.formattedTimestamp = searchResult.formattedTimestamp;

  result.mediaId = searchResult.chunk.mediaId;
  result.mediaType = searchResult.chunk.mediaType;

  if (!IsInitialized())
  {
    CLog::LogF(LOGWARNING, "Enricher not initialized, returning basic result");
    return result;
  }

  // Check cache first
  std::string cacheKey = MakeCacheKey(result.mediaId, result.mediaType);
  auto it = m_infoCache.find(cacheKey);

  CVideoInfoTag tag;
  if (it != m_infoCache.end())
  {
    // Use cached info
    tag = it->second;
    CLog::LogF(LOGDEBUG, "Using cached media info for {} id={}", result.mediaType, result.mediaId);
  }
  else
  {
    // Load from database
    bool loaded = false;
    if (result.mediaType == "movie")
    {
      loaded = LoadMovieInfo(result.mediaId, tag);
    }
    else if (result.mediaType == "episode")
    {
      loaded = LoadEpisodeInfo(result.mediaId, tag);
    }
    else if (result.mediaType == "musicvideo")
    {
      loaded = LoadMusicVideoInfo(result.mediaId, tag);
    }
    else
    {
      CLog::LogF(LOGWARNING, "Unknown media type: {}", result.mediaType);
    }

    if (loaded)
    {
      // Cache the loaded info
      m_infoCache[cacheKey] = tag;
      CLog::LogF(LOGDEBUG, "Loaded and cached media info for {} id={}", result.mediaType,
                 result.mediaId);
    }
    else
    {
      CLog::LogF(LOGWARNING, "Failed to load media info for {} id={}", result.mediaType,
                 result.mediaId);
    }
  }

  // Extract fields from tag
  ExtractCommonFields(tag, result);
  if (result.mediaType == "episode")
  {
    ExtractEpisodeFields(tag, result);
  }

  return result;
}

std::vector<EnrichedSearchResult> CResultEnricher::EnrichBatch(
    const std::vector<HybridSearchResult>& results)
{
  std::vector<EnrichedSearchResult> enrichedResults;
  enrichedResults.reserve(results.size());

  CLog::LogF(LOGDEBUG, "Enriching batch of {} results", results.size());

  for (const auto& result : results)
  {
    enrichedResults.push_back(Enrich(result));
  }

  CLog::LogF(LOGDEBUG, "Batch enrichment complete, cache size: {}", m_infoCache.size());
  return enrichedResults;
}

std::string CResultEnricher::FormatForList(const EnrichedSearchResult& result)
{
  std::string formatted;

  // Line 1: Display title with subtitle
  formatted = result.GetDisplayTitle();

  std::string subtitle = result.GetSubtitle();
  if (!subtitle.empty())
  {
    formatted += " (" + subtitle + ")";
  }

  formatted += "\n";

  // Line 2: Snippet
  formatted += result.snippet;
  formatted += "\n";

  // Line 3: Timestamp indicator
  formatted += "→ " + result.formattedTimestamp;

  return formatted;
}

void CResultEnricher::ClearCache()
{
  CLog::LogF(LOGDEBUG, "Clearing media info cache ({} entries)", m_infoCache.size());
  m_infoCache.clear();
}

// ============================================================================
// Private Helper Methods
// ============================================================================

std::string CResultEnricher::MakeCacheKey(int mediaId, const std::string& mediaType) const
{
  return mediaType + ":" + std::to_string(mediaId);
}

bool CResultEnricher::LoadMovieInfo(int mediaId, CVideoInfoTag& tag)
{
  if (!m_videoDb)
    return false;

  // GetMovieInfo signature: (path, tag, idMovie, idVersion, idFile, getDetails)
  // Pass empty path, use mediaId, and get all details
  return m_videoDb->GetMovieInfo("", tag, mediaId, -1, -1, VideoDbDetailsAll);
}

bool CResultEnricher::LoadEpisodeInfo(int mediaId, CVideoInfoTag& tag)
{
  if (!m_videoDb)
    return false;

  // GetEpisodeInfo signature: (path, tag, idEpisode, getDetails)
  return m_videoDb->GetEpisodeInfo("", tag, mediaId, VideoDbDetailsAll);
}

bool CResultEnricher::LoadMusicVideoInfo(int mediaId, CVideoInfoTag& tag)
{
  if (!m_videoDb)
    return false;

  // GetMusicVideoInfo signature: (path, tag, idMVideo, getDetails)
  return m_videoDb->GetMusicVideoInfo("", tag, mediaId, VideoDbDetailsAll);
}

void CResultEnricher::ExtractCommonFields(const CVideoInfoTag& tag, EnrichedSearchResult& result)
{
  // Basic text fields
  result.title = tag.m_strTitle;
  result.originalTitle = tag.m_strOriginalTitle;
  result.year = tag.GetYear();
  result.plot = tag.m_strPlot;
  result.filePath = tag.m_strFileNameAndPath;

  // Rating
  CRating rating = tag.GetRating();
  result.rating = rating.rating;

  // Runtime (convert from seconds to minutes)
  unsigned int durationSecs = tag.GetDuration();
  result.runtime = durationSecs / 60;

  // Genre (join multiple genres with " / ")
  if (!tag.m_genre.empty())
  {
    result.genre = StringUtils::Join(tag.m_genre, " / ");
  }

  // Thumbnail path
  // CScraperUrl stores URLs in a vector, get the first one
  const auto& thumbUrl = tag.m_strPictureURL.GetFirstUrlByType();
  result.thumbnailPath = thumbUrl.m_url;

  // If no general thumb, try explicit "thumb" type
  if (result.thumbnailPath.empty())
  {
    const auto& thumbUrl2 = tag.m_strPictureURL.GetFirstUrlByType("thumb");
    result.thumbnailPath = thumbUrl2.m_url;
  }

  // Fanart path
  result.fanartPath = tag.m_fanart.GetImageURL();
}

void CResultEnricher::ExtractEpisodeFields(const CVideoInfoTag& tag, EnrichedSearchResult& result)
{
  result.showTitle = tag.m_strShowTitle;
  result.seasonNumber = tag.m_iSeason;
  result.episodeNumber = tag.m_iEpisode;
  result.episodeTitle = tag.m_strTitle; // Episode title is in m_strTitle
}
