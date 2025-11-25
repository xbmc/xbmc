/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "HybridSearchEngine.h"
#include "semantic/SemanticTypes.h"

#include <optional>
#include <string>
#include <vector>

namespace KODI
{
namespace SEMANTIC
{

// Forward declaration
class CSemanticDatabase;

/*!
 * @brief Individual chunk with context metadata
 */
struct ContextChunk
{
  SemanticChunk chunk;       //!< The semantic chunk data
  bool isMatch{false};       //!< True if this is the matched chunk
  float relevance{0.0f};     //!< How relevant to query (0-1)
  std::string formattedTime; //!< Formatted timestamp (e.g., "1:23:45")
};

/*!
 * @brief Context window around a search result or timestamp
 */
struct ContextWindow
{
  // The matched result
  int64_t matchedChunkId{-1};      //!< ID of the matched chunk
  int64_t centerTimestampMs{0};    //!< Center timestamp of the window

  // Context chunks (ordered by time)
  std::vector<ContextChunk> chunks; //!< All chunks in the window

  // Window boundaries
  int64_t windowStartMs{0};  //!< Start of the window
  int64_t windowEndMs{0};    //!< End of the window
  int64_t mediaDurationMs{0}; //!< Total media duration (if known)

  // Navigation
  bool hasEarlierContext{false}; //!< More context available before
  bool hasLaterContext{false};   //!< More context available after

  /*!
   * @brief Get formatted time range for the window
   * @return Time range string (e.g., "1:23:45 - 1:24:15")
   */
  std::string GetTimeRange() const;

  /*!
   * @brief Get full text of all chunks concatenated
   * @return Full text with timestamps
   */
  std::string GetFullText() const;
};

/*!
 * @brief Provider for rich context retrieval around search results
 *
 * This class provides advanced context retrieval capabilities for semantic
 * search results, enabling "see more context" functionality with:
 * - Time-based context windows
 * - Window expansion (load more before/after)
 * - Navigation helpers (next/previous chunk)
 * - Scene boundary detection
 * - Formatted output for display
 *
 * Features:
 * - Retrieves surrounding chunks by timestamp
 * - Calculates relevance based on distance from center
 * - Supports dynamic window expansion
 * - Provides navigation between significant chunks
 * - Formats timestamps for display
 *
 * Example usage:
 * \code
 * CContextProvider provider;
 * provider.Initialize(database);
 *
 * // Get context around a search result
 * auto context = provider.GetContext(searchResult, 60000); // ±30 seconds
 *
 * // Expand to see more
 * if (context.hasEarlierContext)
 * {
 *   auto expanded = provider.ExpandBefore(context, 30000);
 * }
 *
 * // Navigate to next chunk
 * auto nextChunk = provider.GetNextChunk(
 *     context.chunks[0].chunk.mediaId,
 *     context.chunks[0].chunk.mediaType,
 *     context.centerTimestampMs);
 * \endcode
 */
class CContextProvider
{
public:
  CContextProvider();
  ~CContextProvider();

  /*!
   * @brief Initialize the context provider
   * @param database Pointer to semantic database (must remain valid)
   * @return true if initialization succeeded, false otherwise
   */
  bool Initialize(CSemanticDatabase* database);

  /*!
   * @brief Get context around a search result
   * @param result The search result to get context for
   * @param windowMs Time window in milliseconds (default: 60 seconds = ±30s)
   * @return Context window with surrounding chunks
   */
  ContextWindow GetContext(const HybridSearchResult& result, int64_t windowMs = 60000);

  /*!
   * @brief Get context around a specific timestamp
   * @param mediaId The media item ID
   * @param mediaType The media type (movie, episode, musicvideo)
   * @param timestampMs The center timestamp in milliseconds
   * @param windowMs Time window in milliseconds (default: 60 seconds = ±30s)
   * @return Context window with surrounding chunks
   */
  ContextWindow GetContextAt(int mediaId,
                             const std::string& mediaType,
                             int64_t timestampMs,
                             int64_t windowMs = 60000);

  /*!
   * @brief Expand context window to include more before
   * @param current The current context window
   * @param additionalMs Additional time to add before (default: 30 seconds)
   * @return Expanded context window
   */
  ContextWindow ExpandBefore(const ContextWindow& current, int64_t additionalMs = 30000);

  /*!
   * @brief Expand context window to include more after
   * @param current The current context window
   * @param additionalMs Additional time to add after (default: 30 seconds)
   * @return Expanded context window
   */
  ContextWindow ExpandAfter(const ContextWindow& current, int64_t additionalMs = 30000);

  /*!
   * @brief Get chapter or scene boundaries (if available)
   * @param mediaId The media item ID
   * @param mediaType The media type
   * @return Vector of timestamps for scene boundaries
   *
   * Returns timestamps where significant scene changes occur.
   * Currently detects gaps in chunks (potential scene boundaries).
   */
  std::vector<int64_t> GetSceneBoundaries(int mediaId, const std::string& mediaType);

  /*!
   * @brief Navigate to next significant chunk
   * @param mediaId The media item ID
   * @param mediaType The media type
   * @param afterTimestampMs Get chunk after this timestamp
   * @return Next chunk if available, std::nullopt otherwise
   */
  std::optional<SemanticChunk> GetNextChunk(int mediaId,
                                            const std::string& mediaType,
                                            int64_t afterTimestampMs);

  /*!
   * @brief Navigate to previous significant chunk
   * @param mediaId The media item ID
   * @param mediaType The media type
   * @param beforeTimestampMs Get chunk before this timestamp
   * @return Previous chunk if available, std::nullopt otherwise
   */
  std::optional<SemanticChunk> GetPreviousChunk(int mediaId,
                                                const std::string& mediaType,
                                                int64_t beforeTimestampMs);

private:
  CSemanticDatabase* m_database{nullptr};

  /*!
   * @brief Format milliseconds to HH:MM:SS or MM:SS
   * @param ms Timestamp in milliseconds
   * @return Formatted string (e.g., "1:23:45" or "2:34")
   */
  std::string FormatTimestamp(int64_t ms);

  /*!
   * @brief Build context window from raw chunks
   * @param chunks Raw semantic chunks from database
   * @param matchedChunkId ID of the matched chunk (-1 if none)
   * @param centerMs Center timestamp of the window
   * @param windowMs Total window size in milliseconds
   * @return Constructed context window
   */
  ContextWindow BuildContextWindow(const std::vector<SemanticChunk>& chunks,
                                   int64_t matchedChunkId,
                                   int64_t centerMs,
                                   int64_t windowMs);
};

} // namespace SEMANTIC
} // namespace KODI
