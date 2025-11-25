/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IContentParser.h"
#include "semantic/SemanticTypes.h"

#include <cstdint>
#include <string>
#include <vector>

namespace KODI
{
namespace SEMANTIC
{

/*!
 * \brief Configuration parameters for chunk processing
 *
 * These parameters control how parsed entries are segmented into
 * appropriately-sized chunks for optimal FTS5 search performance.
 */
struct ChunkConfig
{
  int maxChunkWords{50};        //!< Target maximum words per chunk
  int minChunkWords{10};        //!< Minimum viable chunk size
  int overlapWords{5};          //!< Word overlap between chunks for context
  bool mergeShortEntries{true}; //!< Combine short adjacent entries
  int maxMergeGapMs{2000};      //!< Maximum time gap for merging entries (ms)
};

/*!
 * \brief Processes parsed content entries into optimized chunks for indexing
 *
 * The ChunkProcessor takes raw parsed entries (subtitles, transcriptions, metadata)
 * and segments them into appropriately-sized chunks for efficient FTS5 indexing.
 * It handles both merging short adjacent entries and splitting long entries to
 * maintain optimal chunk sizes for search performance.
 */
class CChunkProcessor
{
public:
  /*!
   * \brief Construct with default configuration
   */
  CChunkProcessor();

  /*!
   * \brief Construct with custom configuration
   * \param config Chunk processing configuration
   */
  explicit CChunkProcessor(const ChunkConfig& config);

  ~CChunkProcessor() = default;

  /*!
   * \brief Process parsed entries into optimized chunks
   * \param entries Vector of parsed entries with timing information
   * \param mediaId The media item ID
   * \param mediaType The media type (e.g., "movie", "episode")
   * \param sourceType The source type (subtitle, transcription, metadata)
   * \return Vector of semantic chunks ready for indexing
   */
  std::vector<SemanticChunk> Process(const std::vector<ParsedEntry>& entries,
                                     int mediaId,
                                     const std::string& mediaType,
                                     SourceType sourceType);

  /*!
   * \brief Process a single large text into chunks
   *
   * Useful for processing metadata like plot summaries that don't have
   * inherent timing information.
   *
   * \param text The text to process
   * \param mediaId The media item ID
   * \param mediaType The media type (e.g., "movie", "episode")
   * \param sourceType The source type (typically metadata)
   * \return Vector of semantic chunks ready for indexing
   */
  std::vector<SemanticChunk> ProcessText(const std::string& text,
                                         int mediaId,
                                         const std::string& mediaType,
                                         SourceType sourceType);

  /*!
   * \brief Configure chunk parameters
   * \param config New configuration to apply
   */
  void SetConfig(const ChunkConfig& config);

  /*!
   * \brief Get current configuration
   * \return Current chunk configuration
   */
  const ChunkConfig& GetConfig() const;

private:
  ChunkConfig m_config; //!< Current chunk processing configuration

  /*!
   * \brief Merge short adjacent entries into larger chunks
   *
   * Combines consecutive short entries (below maxChunkWords) that are
   * close together in time (within maxMergeGapMs) to create optimally-sized chunks.
   *
   * \param entries Vector of parsed entries to merge
   * \param mediaId The media item ID
   * \param mediaType The media type
   * \param sourceType The source type
   * \return Vector of merged semantic chunks
   */
  std::vector<SemanticChunk> MergeShortEntries(const std::vector<ParsedEntry>& entries,
                                                int mediaId,
                                                const std::string& mediaType,
                                                SourceType sourceType);

  /*!
   * \brief Split a long entry into multiple chunks
   *
   * Breaks up entries that exceed maxChunkWords by splitting on sentence
   * boundaries while estimating timing information proportionally.
   *
   * \param entry The parsed entry to split
   * \param mediaId The media item ID
   * \param mediaType The media type
   * \param sourceType The source type
   * \return Vector of split semantic chunks
   */
  std::vector<SemanticChunk> SplitLongEntry(const ParsedEntry& entry,
                                            int mediaId,
                                            const std::string& mediaType,
                                            SourceType sourceType);

  /*!
   * \brief Count words in text
   *
   * Uses simple whitespace-based word counting.
   *
   * \param text The text to count words in
   * \return Number of words
   */
  int CountWords(const std::string& text) const;

  /*!
   * \brief Split text into sentences
   *
   * Uses simple punctuation-based sentence splitting (., !, ?).
   *
   * \param text The text to split
   * \return Vector of sentences
   */
  std::vector<std::string> SplitIntoSentences(const std::string& text) const;

  /*!
   * \brief Generate unique content hash for deduplication
   *
   * Creates a SHA256 hash from normalized text and timing information
   * to enable content deduplication.
   *
   * \param text The chunk text
   * \param startMs The start time in milliseconds
   * \return SHA256 hash string
   */
  std::string GenerateContentHash(const std::string& text, int64_t startMs) const;
};

} // namespace SEMANTIC
} // namespace KODI
