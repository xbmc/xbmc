/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ChunkProcessor.h"

#include "utils/Digest.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <regex>
#include <sstream>

using namespace KODI::SEMANTIC;
using namespace KODI::UTILITY;
using namespace KODI::UTILS;

CChunkProcessor::CChunkProcessor() = default;

CChunkProcessor::CChunkProcessor(const ChunkConfig& config) : m_config(config)
{
}

std::vector<SemanticChunk> CChunkProcessor::Process(const std::vector<ParsedEntry>& entries,
                                                     int mediaId,
                                                     const std::string& mediaType,
                                                     SourceType sourceType)
{
  if (entries.empty())
  {
    CLog::Log(LOGDEBUG, "CChunkProcessor: No entries to process");
    return {};
  }

  CLog::Log(LOGDEBUG, "CChunkProcessor: Processing {} entries for mediaId={} type={}",
            entries.size(), mediaId, mediaType);

  std::vector<SemanticChunk> chunks;

  if (m_config.mergeShortEntries)
  {
    // Use merge strategy for short entries
    chunks = MergeShortEntries(entries, mediaId, mediaType, sourceType);
  }
  else
  {
    // Process each entry individually
    for (const auto& entry : entries)
    {
      int wordCount = CountWords(entry.text);

      if (wordCount > m_config.maxChunkWords)
      {
        // Split long entries
        auto splitChunks = SplitLongEntry(entry, mediaId, mediaType, sourceType);
        chunks.insert(chunks.end(), splitChunks.begin(), splitChunks.end());
      }
      else if (wordCount >= m_config.minChunkWords)
      {
        // Create single chunk
        SemanticChunk chunk;
        chunk.mediaId = mediaId;
        chunk.mediaType = mediaType;
        chunk.sourceType = sourceType;
        chunk.startMs = static_cast<int>(entry.startMs);
        chunk.endMs = static_cast<int>(entry.endMs);
        chunk.text = entry.text;
        chunk.confidence = entry.confidence;
        chunk.language = ""; // Set by caller if needed
        chunks.push_back(chunk);
      }
      // Skip entries below minChunkWords
    }
  }

  CLog::Log(LOGDEBUG, "CChunkProcessor: Generated {} chunks from {} entries", chunks.size(),
            entries.size());

  return chunks;
}

std::vector<SemanticChunk> CChunkProcessor::ProcessText(const std::string& text,
                                                        int mediaId,
                                                        const std::string& mediaType,
                                                        SourceType sourceType)
{
  if (text.empty())
  {
    return {};
  }

  CLog::Log(LOGDEBUG, "CChunkProcessor: Processing text of {} chars for mediaId={}", text.length(),
            mediaId);

  int wordCount = CountWords(text);

  if (wordCount <= m_config.maxChunkWords)
  {
    // Text fits in single chunk
    SemanticChunk chunk;
    chunk.mediaId = mediaId;
    chunk.mediaType = mediaType;
    chunk.sourceType = sourceType;
    chunk.startMs = 0;
    chunk.endMs = 0;
    chunk.text = text;
    chunk.confidence = 1.0f;
    return {chunk};
  }

  // Split into multiple chunks using sentence boundaries
  std::vector<SemanticChunk> chunks;
  std::vector<std::string> sentences = SplitIntoSentences(text);

  std::string currentText;

  for (const auto& sentence : sentences)
  {
    int sentenceWords = CountWords(sentence);
    int currentWords = CountWords(currentText);

    if (currentWords + sentenceWords > m_config.maxChunkWords && !currentText.empty())
    {
      // Flush current chunk
      SemanticChunk chunk;
      chunk.mediaId = mediaId;
      chunk.mediaType = mediaType;
      chunk.sourceType = sourceType;
      chunk.startMs = 0;
      chunk.endMs = 0;
      chunk.text = currentText;
      chunk.confidence = 1.0f;
      chunks.push_back(chunk);

      currentText = sentence;
    }
    else
    {
      if (!currentText.empty())
        currentText += " ";
      currentText += sentence;
    }
  }

  // Flush final chunk
  if (!currentText.empty())
  {
    SemanticChunk chunk;
    chunk.mediaId = mediaId;
    chunk.mediaType = mediaType;
    chunk.sourceType = sourceType;
    chunk.startMs = 0;
    chunk.endMs = 0;
    chunk.text = currentText;
    chunk.confidence = 1.0f;
    chunks.push_back(chunk);
  }

  CLog::Log(LOGDEBUG, "CChunkProcessor: Generated {} chunks from text", chunks.size());

  return chunks;
}

void CChunkProcessor::SetConfig(const ChunkConfig& config)
{
  m_config = config;
}

const ChunkConfig& CChunkProcessor::GetConfig() const
{
  return m_config;
}

std::vector<SemanticChunk> CChunkProcessor::MergeShortEntries(
    const std::vector<ParsedEntry>& entries,
    int mediaId,
    const std::string& mediaType,
    SourceType sourceType)
{
  std::vector<SemanticChunk> chunks;

  std::string accumulatedText;
  int64_t startMs = -1;
  int64_t endMs = -1;
  float minConfidence = 1.0f;

  for (size_t i = 0; i < entries.size(); ++i)
  {
    const auto& entry = entries[i];
    int wordCount = CountWords(entry.text);

    // Check if we can merge with accumulated text
    bool canMerge = !accumulatedText.empty() &&
                    (entry.startMs - endMs) <= m_config.maxMergeGapMs;

    int accumulatedWords = CountWords(accumulatedText);

    if (canMerge && (accumulatedWords + wordCount) <= m_config.maxChunkWords)
    {
      // Merge with previous
      accumulatedText += " " + entry.text;
      endMs = entry.endMs;
      minConfidence = std::min(minConfidence, entry.confidence);
    }
    else
    {
      // Flush accumulated text if it meets minimum size
      if (!accumulatedText.empty() && accumulatedWords >= m_config.minChunkWords)
      {
        SemanticChunk chunk;
        chunk.mediaId = mediaId;
        chunk.mediaType = mediaType;
        chunk.sourceType = sourceType;
        chunk.startMs = static_cast<int>(startMs);
        chunk.endMs = static_cast<int>(endMs);
        chunk.text = accumulatedText;
        chunk.confidence = minConfidence;
        chunk.language = ""; // Set by caller if needed
        chunks.push_back(chunk);
      }

      // Check if current entry needs splitting
      if (wordCount > m_config.maxChunkWords)
      {
        // Split this long entry
        auto splitChunks = SplitLongEntry(entry, mediaId, mediaType, sourceType);
        chunks.insert(chunks.end(), splitChunks.begin(), splitChunks.end());

        // Reset accumulation
        accumulatedText.clear();
        startMs = -1;
        endMs = -1;
        minConfidence = 1.0f;
      }
      else
      {
        // Start new accumulation with current entry
        accumulatedText = entry.text;
        startMs = entry.startMs;
        endMs = entry.endMs;
        minConfidence = entry.confidence;
      }
    }
  }

  // Flush final accumulated text
  if (!accumulatedText.empty() && CountWords(accumulatedText) >= m_config.minChunkWords)
  {
    SemanticChunk chunk;
    chunk.mediaId = mediaId;
    chunk.mediaType = mediaType;
    chunk.sourceType = sourceType;
    chunk.startMs = static_cast<int>(startMs);
    chunk.endMs = static_cast<int>(endMs);
    chunk.text = accumulatedText;
    chunk.confidence = minConfidence;
    chunk.language = "";
    chunks.push_back(chunk);
  }

  return chunks;
}

std::vector<SemanticChunk> CChunkProcessor::SplitLongEntry(const ParsedEntry& entry,
                                                           int mediaId,
                                                           const std::string& mediaType,
                                                           SourceType sourceType)
{
  std::vector<SemanticChunk> chunks;

  std::vector<std::string> sentences = SplitIntoSentences(entry.text);

  if (sentences.empty())
  {
    return chunks;
  }

  std::string currentText;
  int64_t chunkStartMs = entry.startMs;
  int64_t entryDurationMs = entry.endMs - entry.startMs;
  int totalWords = CountWords(entry.text);
  int processedWords = 0;

  for (size_t i = 0; i < sentences.size(); ++i)
  {
    const auto& sentence = sentences[i];
    int sentenceWords = CountWords(sentence);
    int currentWords = CountWords(currentText);

    if ((currentWords + sentenceWords) > m_config.maxChunkWords && !currentText.empty())
    {
      // Create chunk from current text
      SemanticChunk chunk;
      chunk.mediaId = mediaId;
      chunk.mediaType = mediaType;
      chunk.sourceType = sourceType;
      chunk.startMs = static_cast<int>(chunkStartMs);

      // Estimate end time based on word progress
      float progress = static_cast<float>(processedWords) / static_cast<float>(totalWords);
      chunk.endMs = static_cast<int>(entry.startMs + static_cast<int64_t>(progress * entryDurationMs));

      chunk.text = currentText;
      chunk.confidence = entry.confidence;
      chunk.language = "";
      chunks.push_back(chunk);

      // Start new chunk with overlap if configured
      if (m_config.overlapWords > 0)
      {
        // Keep last few words for context
        std::vector<std::string> words;
        std::istringstream iss(currentText);
        std::string word;
        while (iss >> word)
        {
          words.push_back(word);
        }

        int overlapStart = std::max(0, static_cast<int>(words.size()) - m_config.overlapWords);
        currentText.clear();
        for (size_t j = overlapStart; j < words.size(); ++j)
        {
          if (!currentText.empty())
            currentText += " ";
          currentText += words[j];
        }
      }
      else
      {
        currentText.clear();
      }

      chunkStartMs = chunk.endMs;
    }

    if (!currentText.empty())
      currentText += " ";
    currentText += sentence;

    processedWords += sentenceWords;
  }

  // Create final chunk
  if (!currentText.empty())
  {
    SemanticChunk chunk;
    chunk.mediaId = mediaId;
    chunk.mediaType = mediaType;
    chunk.sourceType = sourceType;
    chunk.startMs = static_cast<int>(chunkStartMs);
    chunk.endMs = static_cast<int>(entry.endMs);
    chunk.text = currentText;
    chunk.confidence = entry.confidence;
    chunk.language = "";
    chunks.push_back(chunk);
  }

  return chunks;
}

int CChunkProcessor::CountWords(const std::string& text) const
{
  if (text.empty())
  {
    return 0;
  }

  // Simple whitespace-based word counting
  std::istringstream iss(text);
  std::string word;
  int count = 0;

  while (iss >> word)
  {
    ++count;
  }

  return count;
}

std::vector<std::string> CChunkProcessor::SplitIntoSentences(const std::string& text) const
{
  std::vector<std::string> sentences;

  if (text.empty())
  {
    return sentences;
  }

  // Simple sentence splitting on . ! ? followed by space or end
  // Pattern: capture everything up to and including sentence-ending punctuation
  std::regex sentenceRegex(R"([^.!?]+[.!?]+\s*)");

  auto begin = std::sregex_iterator(text.begin(), text.end(), sentenceRegex);
  auto end = std::sregex_iterator();

  for (auto it = begin; it != end; ++it)
  {
    std::string sentence = it->str();
    StringUtils::Trim(sentence);
    if (!sentence.empty())
    {
      sentences.push_back(sentence);
    }
  }

  // Handle text without sentence markers or leftover text
  if (sentences.empty() && !text.empty())
  {
    sentences.push_back(text);
  }
  else
  {
    // Check if there's any unmatched text at the end
    std::string reconstructed;
    for (const auto& s : sentences)
    {
      reconstructed += s + " ";
    }
    StringUtils::Trim(reconstructed);

    if (reconstructed.length() < text.length())
    {
      // There's leftover text, add it as final sentence
      std::string leftover = text.substr(reconstructed.length());
      StringUtils::Trim(leftover);
      if (!leftover.empty())
      {
        sentences.push_back(leftover);
      }
    }
  }

  return sentences;
}

std::string CChunkProcessor::GenerateContentHash(const std::string& text, int64_t startMs) const
{
  // Normalize text for consistent hashing
  std::string normalized = StringUtils::ToLower(text);
  StringUtils::RemoveDuplicatedSpacesAndTabs(normalized);
  StringUtils::Trim(normalized);

  // Combine normalized text with timestamp for uniqueness
  std::string hashInput = normalized + "|" + std::to_string(startMs);

  // Generate SHA256 hash
  return CDigest::Calculate(CDigest::Type::SHA256, hashInput);
}
