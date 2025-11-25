/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ContextProvider.h"

#include "semantic/SemanticDatabase.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <cmath>

using namespace KODI::SEMANTIC;

CContextProvider::CContextProvider() = default;

CContextProvider::~CContextProvider() = default;

bool CContextProvider::Initialize(CSemanticDatabase* database)
{
  if (!database)
  {
    CLog::Log(LOGERROR, "CContextProvider::{}: Invalid database pointer", __FUNCTION__);
    return false;
  }

  m_database = database;
  CLog::Log(LOGDEBUG, "CContextProvider::{}: Initialized successfully", __FUNCTION__);
  return true;
}

ContextWindow CContextProvider::GetContext(const HybridSearchResult& result, int64_t windowMs)
{
  return GetContextAt(result.chunk.mediaId, result.chunk.mediaType, result.chunk.startMs, windowMs);
}

ContextWindow CContextProvider::GetContextAt(int mediaId,
                                             const std::string& mediaType,
                                             int64_t timestampMs,
                                             int64_t windowMs)
{
  ContextWindow window;
  window.centerTimestampMs = timestampMs;

  if (!m_database)
  {
    CLog::Log(LOGERROR, "CContextProvider::{}: Database not initialized", __FUNCTION__);
    return window;
  }

  // Calculate window boundaries
  int64_t halfWindow = windowMs / 2;
  int64_t startMs = std::max(0LL, timestampMs - halfWindow);
  int64_t endMs = timestampMs + halfWindow;

  window.windowStartMs = startMs;
  window.windowEndMs = endMs;

  CLog::Log(LOGDEBUG,
            "CContextProvider::{}: Getting context for mediaId={} type='{}' at {}ms (window: "
            "{}-{}ms)",
            __FUNCTION__, mediaId, mediaType, timestampMs, startMs, endMs);

  // Get chunks from database
  auto rawChunks = m_database->GetContext(mediaId, mediaType, timestampMs, windowMs);

  if (rawChunks.empty())
  {
    CLog::Log(LOGWARNING, "CContextProvider::{}: No chunks found in context window", __FUNCTION__);
    return window;
  }

  // Build context window with enriched chunks
  window = BuildContextWindow(rawChunks, -1, timestampMs, windowMs);

  CLog::Log(LOGDEBUG, "CContextProvider::{}: Retrieved {} context chunks", __FUNCTION__,
            window.chunks.size());

  return window;
}

ContextWindow CContextProvider::ExpandBefore(const ContextWindow& current, int64_t additionalMs)
{
  if (current.chunks.empty())
  {
    CLog::Log(LOGWARNING, "CContextProvider::{}: Cannot expand empty context window",
              __FUNCTION__);
    return current;
  }

  // Calculate new window boundaries
  int64_t newStartMs = std::max(0LL, current.windowStartMs - additionalMs);
  int64_t newWindowMs = current.windowEndMs - newStartMs;

  CLog::Log(LOGDEBUG, "CContextProvider::{}: Expanding context before by {}ms (new start: {}ms)",
            __FUNCTION__, additionalMs, newStartMs);

  // Get context for expanded window
  return GetContextAt(current.chunks[0].chunk.mediaId, current.chunks[0].chunk.mediaType,
                      (newStartMs + current.windowEndMs) / 2, newWindowMs);
}

ContextWindow CContextProvider::ExpandAfter(const ContextWindow& current, int64_t additionalMs)
{
  if (current.chunks.empty())
  {
    CLog::Log(LOGWARNING, "CContextProvider::{}: Cannot expand empty context window",
              __FUNCTION__);
    return current;
  }

  // Calculate new window boundaries
  int64_t newEndMs = current.windowEndMs + additionalMs;
  int64_t newWindowMs = newEndMs - current.windowStartMs;

  CLog::Log(LOGDEBUG, "CContextProvider::{}: Expanding context after by {}ms (new end: {}ms)",
            __FUNCTION__, additionalMs, newEndMs);

  // Get context for expanded window
  return GetContextAt(current.chunks[0].chunk.mediaId, current.chunks[0].chunk.mediaType,
                      (current.windowStartMs + newEndMs) / 2, newWindowMs);
}

std::vector<int64_t> CContextProvider::GetSceneBoundaries(int mediaId,
                                                          const std::string& mediaType)
{
  std::vector<int64_t> boundaries;

  if (!m_database)
  {
    CLog::Log(LOGERROR, "CContextProvider::{}: Database not initialized", __FUNCTION__);
    return boundaries;
  }

  // Get all chunks for this media
  std::vector<SemanticChunk> chunks;
  if (!m_database->GetChunksForMedia(mediaId, mediaType, chunks))
  {
    CLog::Log(LOGWARNING, "CContextProvider::{}: Failed to get chunks for mediaId={} type='{}'",
              __FUNCTION__, mediaId, mediaType);
    return boundaries;
  }

  if (chunks.size() < 2)
  {
    return boundaries;
  }

  // Sort chunks by start time
  std::sort(chunks.begin(), chunks.end(),
            [](const SemanticChunk& a, const SemanticChunk& b) { return a.startMs < b.startMs; });

  // Detect scene boundaries by looking for gaps between chunks
  // A gap larger than 5 seconds might indicate a scene change
  const int64_t SCENE_BOUNDARY_GAP_MS = 5000;

  for (size_t i = 1; i < chunks.size(); ++i)
  {
    int64_t gap = chunks[i].startMs - chunks[i - 1].endMs;
    if (gap > SCENE_BOUNDARY_GAP_MS)
    {
      // Add boundary at the midpoint of the gap
      boundaries.push_back(chunks[i - 1].endMs + gap / 2);
    }
  }

  CLog::Log(LOGDEBUG, "CContextProvider::{}: Found {} scene boundaries for mediaId={} type='{}'",
            __FUNCTION__, boundaries.size(), mediaId, mediaType);

  return boundaries;
}

std::optional<SemanticChunk> CContextProvider::GetNextChunk(int mediaId,
                                                            const std::string& mediaType,
                                                            int64_t afterTimestampMs)
{
  if (!m_database)
  {
    CLog::Log(LOGERROR, "CContextProvider::{}: Database not initialized", __FUNCTION__);
    return std::nullopt;
  }

  // Get all chunks for this media
  std::vector<SemanticChunk> chunks;
  if (!m_database->GetChunksForMedia(mediaId, mediaType, chunks))
  {
    return std::nullopt;
  }

  // Sort by start time
  std::sort(chunks.begin(), chunks.end(),
            [](const SemanticChunk& a, const SemanticChunk& b) { return a.startMs < b.startMs; });

  // Find first chunk after the timestamp
  for (const auto& chunk : chunks)
  {
    if (chunk.startMs > afterTimestampMs)
    {
      CLog::Log(LOGDEBUG,
                "CContextProvider::{}: Found next chunk at {}ms (after {}ms, mediaId={} type='{}')",
                __FUNCTION__, chunk.startMs, afterTimestampMs, mediaId, mediaType);
      return chunk;
    }
  }

  CLog::Log(LOGDEBUG, "CContextProvider::{}: No next chunk found after {}ms (mediaId={} type='{}')",
            __FUNCTION__, afterTimestampMs, mediaId, mediaType);
  return std::nullopt;
}

std::optional<SemanticChunk> CContextProvider::GetPreviousChunk(int mediaId,
                                                                const std::string& mediaType,
                                                                int64_t beforeTimestampMs)
{
  if (!m_database)
  {
    CLog::Log(LOGERROR, "CContextProvider::{}: Database not initialized", __FUNCTION__);
    return std::nullopt;
  }

  // Get all chunks for this media
  std::vector<SemanticChunk> chunks;
  if (!m_database->GetChunksForMedia(mediaId, mediaType, chunks))
  {
    return std::nullopt;
  }

  // Sort by start time (descending)
  std::sort(chunks.begin(), chunks.end(),
            [](const SemanticChunk& a, const SemanticChunk& b) { return a.startMs > b.startMs; });

  // Find first chunk before the timestamp
  for (const auto& chunk : chunks)
  {
    if (chunk.startMs < beforeTimestampMs)
    {
      CLog::Log(
          LOGDEBUG,
          "CContextProvider::{}: Found previous chunk at {}ms (before {}ms, mediaId={} type='{}')",
          __FUNCTION__, chunk.startMs, beforeTimestampMs, mediaId, mediaType);
      return chunk;
    }
  }

  CLog::Log(LOGDEBUG,
            "CContextProvider::{}: No previous chunk found before {}ms (mediaId={} type='{}')",
            __FUNCTION__, beforeTimestampMs, mediaId, mediaType);
  return std::nullopt;
}

std::string CContextProvider::FormatTimestamp(int64_t ms)
{
  int hours = static_cast<int>(ms / 3600000);
  int minutes = static_cast<int>((ms % 3600000) / 60000);
  int seconds = static_cast<int>((ms % 60000) / 1000);

  if (hours > 0)
  {
    return StringUtils::Format("%d:%02d:%02d", hours, minutes, seconds);
  }
  else
  {
    return StringUtils::Format("%d:%02d", minutes, seconds);
  }
}

ContextWindow CContextProvider::BuildContextWindow(const std::vector<SemanticChunk>& chunks,
                                                   int64_t matchedChunkId,
                                                   int64_t centerMs,
                                                   int64_t windowMs)
{
  ContextWindow window;
  window.matchedChunkId = matchedChunkId;
  window.centerTimestampMs = centerMs;

  int64_t halfWindow = windowMs / 2;
  window.windowStartMs = std::max(0LL, centerMs - halfWindow);
  window.windowEndMs = centerMs + halfWindow;

  // Convert raw chunks to context chunks
  for (const auto& chunk : chunks)
  {
    ContextChunk ctx;
    ctx.chunk = chunk;

    // Determine if this is the matched chunk
    ctx.isMatch = (matchedChunkId != -1 && chunk.chunkId == matchedChunkId) ||
                  (chunk.startMs <= centerMs && chunk.endMs >= centerMs);

    if (ctx.isMatch && matchedChunkId == -1)
    {
      window.matchedChunkId = chunk.chunkId;
    }

    // Format timestamp for display
    ctx.formattedTime = FormatTimestamp(chunk.startMs);

    // Calculate relevance based on distance from center
    int64_t chunkCenter = (chunk.startMs + chunk.endMs) / 2;
    int64_t distance = std::abs(chunkCenter - centerMs);

    if (halfWindow > 0)
    {
      ctx.relevance = 1.0f - (static_cast<float>(distance) / static_cast<float>(halfWindow));
      ctx.relevance = std::max(0.0f, std::min(1.0f, ctx.relevance));
    }
    else
    {
      ctx.relevance = 1.0f;
    }

    window.chunks.push_back(ctx);
  }

  // Sort chunks by timestamp
  std::sort(window.chunks.begin(), window.chunks.end(),
            [](const ContextChunk& a, const ContextChunk& b) {
              return a.chunk.startMs < b.chunk.startMs;
            });

  // Determine if more context is available
  if (!window.chunks.empty())
  {
    window.hasEarlierContext = (window.chunks.front().chunk.startMs > 0);
    window.hasLaterContext = true; // Assume more unless we know duration

    // If we have chunks at the very start or end, we might not have more context
    if (window.windowStartMs <= 1000) // Within 1 second of start
    {
      window.hasEarlierContext = false;
    }
  }

  return window;
}

// ContextWindow helper methods

std::string ContextWindow::GetTimeRange() const
{
  auto formatTime = [](int64_t ms) {
    int hours = static_cast<int>(ms / 3600000);
    int minutes = static_cast<int>((ms % 3600000) / 60000);
    int seconds = static_cast<int>((ms % 60000) / 1000);

    if (hours > 0)
    {
      return StringUtils::Format("%d:%02d:%02d", hours, minutes, seconds);
    }
    else
    {
      return StringUtils::Format("%d:%02d", minutes, seconds);
    }
  };

  return formatTime(windowStartMs) + " - " + formatTime(windowEndMs);
}

std::string ContextWindow::GetFullText() const
{
  std::string text;

  for (const auto& ctx : chunks)
  {
    if (!text.empty())
    {
      text += "\n";
    }

    // Add timestamp and text
    text += "[" + ctx.formattedTime + "] " + ctx.chunk.text;

    // Optionally mark the matched chunk
    if (ctx.isMatch)
    {
      text += " **";
    }
  }

  return text;
}
