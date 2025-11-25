/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

namespace KODI
{
namespace SEMANTIC
{

/*!
 * @brief Source type for semantic chunks
 */
enum class SourceType
{
  SUBTITLE,       //!< Content from subtitle files
  TRANSCRIPTION,  //!< Content from audio transcription
  METADATA        //!< Content from media metadata
};

/*!
 * @brief Index status for media items
 */
enum class IndexStatus
{
  PENDING,      //!< Not yet processed
  IN_PROGRESS,  //!< Currently being processed
  COMPLETED,    //!< Successfully indexed
  FAILED        //!< Processing failed
};

/*!
 * @brief Convert SourceType to string for database storage
 */
inline const char* SourceTypeToString(SourceType type)
{
  switch (type)
  {
    case SourceType::SUBTITLE:
      return "subtitle";
    case SourceType::TRANSCRIPTION:
      return "transcription";
    case SourceType::METADATA:
      return "metadata";
    default:
      return "unknown";
  }
}

/*!
 * @brief Convert string to SourceType
 */
inline SourceType StringToSourceType(const std::string& str)
{
  if (str == "subtitle")
    return SourceType::SUBTITLE;
  if (str == "transcription")
    return SourceType::TRANSCRIPTION;
  if (str == "metadata")
    return SourceType::METADATA;
  return SourceType::SUBTITLE; // default
}

/*!
 * @brief Convert IndexStatus to string for database storage
 */
inline const char* IndexStatusToString(IndexStatus status)
{
  switch (status)
  {
    case IndexStatus::PENDING:
      return "pending";
    case IndexStatus::IN_PROGRESS:
      return "in_progress";
    case IndexStatus::COMPLETED:
      return "completed";
    case IndexStatus::FAILED:
      return "failed";
    default:
      return "pending";
  }
}

/*!
 * @brief Convert string to IndexStatus
 */
inline IndexStatus StringToIndexStatus(const std::string& str)
{
  if (str == "pending")
    return IndexStatus::PENDING;
  if (str == "in_progress")
    return IndexStatus::IN_PROGRESS;
  if (str == "completed")
    return IndexStatus::COMPLETED;
  if (str == "failed")
    return IndexStatus::FAILED;
  return IndexStatus::PENDING; // default
}

/*!
 * @brief Semantic chunk data structure
 */
struct SemanticChunk
{
  int chunkId{-1};
  int mediaId{-1};
  std::string mediaType;
  SourceType sourceType{SourceType::SUBTITLE};
  std::string sourcePath;
  int startMs{0};
  int endMs{0};
  std::string text;
  std::string language;
  float confidence{1.0f};
  std::string createdAt;
};

/*!
 * @brief Index state for media items
 */
struct SemanticIndexState
{
  int stateId{-1};
  int mediaId{-1};
  std::string mediaType;
  std::string mediaPath;
  IndexStatus subtitleStatus{IndexStatus::PENDING};
  IndexStatus transcriptionStatus{IndexStatus::PENDING};
  std::string transcriptionProvider;
  float transcriptionProgress{0.0f};
  IndexStatus metadataStatus{IndexStatus::PENDING};
  int priority{0};
  int chunkCount{0};
  std::string createdAt;
  std::string updatedAt;

  // Embedding state (added in schema version 2)
  IndexStatus embeddingStatus{IndexStatus::PENDING};
  float embeddingProgress{0.0f};
  std::string embeddingError;
  int embeddingsCount{0};
};

/*!
 * @brief Vector search result structure
 */
struct VectorSearchResult
{
  int64_t chunkId{-1};
  float distance{0.0f};  // Cosine distance (0 = identical, 2 = opposite)

  /*!
   * @brief Convert distance to similarity score (0-1 range)
   * @return Similarity score where 1.0 = identical, 0.0 = opposite
   */
  float similarity() const { return 1.0f - (distance / 2.0f); }
};

/*!
 * @brief Options for search queries
 */
struct SearchOptions
{
  int maxResults{50};           //!< Maximum number of results to return
  std::string mediaType;         //!< Filter by media type (empty = all)
  int mediaId{-1};               //!< Filter by specific media ID (-1 = all)
  SourceType sourceType{SourceType::SUBTITLE}; //!< Filter by source type
  bool filterBySource{false};    //!< Whether to filter by source type
  float minConfidence{0.0f};     //!< Minimum confidence threshold
};

/*!
 * @brief Search result with ranking score
 */
struct SearchResult
{
  SemanticChunk chunk;           //!< The matching chunk
  float score{0.0f};             //!< Relevance score (BM25)
  std::string snippet;           //!< Highlighted snippet
};

/*!
 * @brief Statistics about the semantic index
 */
struct IndexStats
{
  int totalMedia{0};             //!< Total media items in system
  int indexedMedia{0};           //!< Media items with completed indexing
  int totalChunks{0};            //!< Total number of chunks
  int totalWords{0};             //!< Total word count (approximate)
  int queuedJobs{0};             //!< Pending indexing jobs
};

} // namespace SEMANTIC
} // namespace KODI
