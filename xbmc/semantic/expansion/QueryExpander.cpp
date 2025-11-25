/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "QueryExpander.h"

#include "SynonymDatabase.h"
#include "semantic/SemanticDatabase.h"
#include "semantic/embedding/EmbeddingEngine.h"
#include "semantic/embedding/Tokenizer.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <unordered_set>

using namespace KODI::SEMANTIC;

namespace
{

// Porter stemming algorithm - simplified version for English
// This removes common suffixes to get word stems
std::string PorterStem(const std::string& word)
{
  std::string stem = word;

  // Skip very short words
  if (stem.length() <= 3)
    return stem;

  // Step 1: Plural removal
  if (StringUtils::EndsWith(stem, "sses"))
  {
    stem = stem.substr(0, stem.length() - 2); // sses -> ss
  }
  else if (StringUtils::EndsWith(stem, "ies"))
  {
    stem = stem.substr(0, stem.length() - 3) + "i"; // ies -> i
  }
  else if (StringUtils::EndsWith(stem, "ss"))
  {
    // Keep ss
  }
  else if (StringUtils::EndsWith(stem, "s") && stem.length() > 3)
  {
    stem = stem.substr(0, stem.length() - 1); // s -> (empty)
  }

  // Step 2: -ing, -ed removal
  if (StringUtils::EndsWith(stem, "ing") && stem.length() > 5)
  {
    stem = stem.substr(0, stem.length() - 3);
  }
  else if (StringUtils::EndsWith(stem, "ed") && stem.length() > 4)
  {
    stem = stem.substr(0, stem.length() - 2);
  }

  // Step 3: -ly removal
  if (StringUtils::EndsWith(stem, "ly") && stem.length() > 4)
  {
    stem = stem.substr(0, stem.length() - 2);
  }

  return stem;
}

// Common English stop words to skip expansion
const std::unordered_set<std::string> STOP_WORDS = {
    "a",    "an",   "and",  "are",  "as",   "at",   "be",   "by",   "for",  "from",
    "has",  "he",   "in",   "is",   "it",   "its",  "of",   "on",   "that", "the",
    "to",   "was",  "will", "with", "the",  "this", "but",  "they", "have", "had",
    "what", "when", "where", "who", "which", "why",  "how",  "all",  "each", "every"};

bool IsStopWord(const std::string& word)
{
  std::string lower = word;
  StringUtils::ToLower(lower);
  return STOP_WORDS.find(lower) != STOP_WORDS.end();
}

} // anonymous namespace

CQueryExpander::CQueryExpander() = default;

CQueryExpander::~CQueryExpander() = default;

bool CQueryExpander::Initialize(CSemanticDatabase* database,
                                CEmbeddingEngine* embeddingEngine,
                                CSynonymDatabase* synonymDb)
{
  if (database == nullptr)
  {
    CLog::LogF(LOGERROR, "Cannot initialize with null database pointer");
    return false;
  }

  m_database = database;
  m_embeddingEngine = embeddingEngine;
  m_synonymDb = synonymDb;

  // Log which expansion strategies are available
  std::vector<std::string> available;
  available.push_back("stemming");

  if (m_synonymDb && m_synonymDb->IsInitialized())
  {
    available.push_back("synonym");
  }

  if (m_embeddingEngine && m_embeddingEngine->IsInitialized())
  {
    available.push_back("embedding");
  }

  CLog::LogF(LOGINFO, "QueryExpander initialized with strategies: {}",
             StringUtils::Join(available, ", "));

  return true;
}

bool CQueryExpander::IsInitialized() const
{
  return m_database != nullptr;
}

ExpansionResult CQueryExpander::ExpandQuery(const std::string& query, const ExpansionConfig& config)
{
  if (!IsInitialized())
  {
    CLog::LogF(LOGERROR, "Expander not initialized");
    return {};
  }

  if (query.empty())
  {
    return {};
  }

  m_totalQueries++;

  // Check cache first
  if (m_cacheEnabled)
  {
    ExpansionResult cached;
    if (GetFromCache(query, cached))
    {
      m_cacheHits++;
      cached.cached = true;
      return cached;
    }
  }

  ExpansionResult result;
  result.originalQuery = query;

  // Tokenize query into words
  std::vector<std::string> words = TokenizeQuery(query);

  // Expand each word
  for (const auto& word : words)
  {
    // Skip stop words and very short words
    if (IsStopWord(word) || word.length() < 2)
    {
      // Add original word with high weight
      ExpandedTerm term;
      term.term = word;
      term.weight = config.originalWeight;
      term.source = "original";
      term.isOriginal = true;
      result.terms.push_back(term);
      continue;
    }

    // Add original term first
    ExpandedTerm originalTerm;
    originalTerm.term = word;
    originalTerm.weight = config.originalWeight;
    originalTerm.source = "original";
    originalTerm.isOriginal = true;
    result.terms.push_back(originalTerm);

    // Expand this word
    std::vector<ExpandedTerm> expansions = ExpandWord(word, config);

    // Track expansions for this word
    std::vector<std::string> wordExpansions;
    for (const auto& exp : expansions)
    {
      if (!exp.isOriginal)
      {
        wordExpansions.push_back(exp.term);
      }
    }

    if (!wordExpansions.empty())
    {
      result.expansionMap[word] = wordExpansions;
      result.totalExpansions += static_cast<int>(wordExpansions.size());
    }

    // Limit expansions per word
    if (static_cast<int>(expansions.size()) > config.maxTermsPerWord)
    {
      expansions.resize(config.maxTermsPerWord);
    }

    // Add expansions (skip original as we already added it)
    for (const auto& exp : expansions)
    {
      if (!exp.isOriginal)
      {
        result.terms.push_back(exp);
      }
    }
  }

  // Deduplicate terms if enabled
  if (config.deduplicateTerms)
  {
    DeduplicateTerms(result.terms);
  }

  // Limit total terms
  if (static_cast<int>(result.terms.size()) > config.maxTotalTerms)
  {
    LimitTerms(result.terms, config.maxTotalTerms);
  }

  // Build expanded query string
  result.expandedQuery = BuildExpandedQuery(result.terms);

  // Add to cache
  if (m_cacheEnabled)
  {
    AddToCache(query, result);
  }

  CLog::LogF(LOGDEBUG, "Expanded query '{}' -> '{}' ({} expansions)", query, result.expandedQuery,
             result.totalExpansions);

  return result;
}

std::vector<ExpandedTerm> CQueryExpander::ExpandWord(const std::string& word,
                                                      const ExpansionConfig& config)
{
  std::vector<ExpandedTerm> expansions;

  // Normalize word
  std::string normalized = NormalizeWord(word);
  if (normalized.empty())
  {
    return expansions;
  }

  // Apply each enabled strategy
  if (HasFlag(config.strategies, ExpansionStrategy::Synonym))
  {
    auto synExpansions = ExpandSynonym(normalized, config);
    expansions.insert(expansions.end(), synExpansions.begin(), synExpansions.end());
  }

  if (HasFlag(config.strategies, ExpansionStrategy::Embedding))
  {
    auto embExpansions = ExpandEmbedding(normalized, config);
    expansions.insert(expansions.end(), embExpansions.begin(), embExpansions.end());
  }

  if (HasFlag(config.strategies, ExpansionStrategy::Stemming))
  {
    auto stemExpansions = ExpandStemming(normalized, config);
    expansions.insert(expansions.end(), stemExpansions.begin(), stemExpansions.end());
  }

  // Sort by weight (highest first)
  std::sort(expansions.begin(), expansions.end(),
            [](const ExpandedTerm& a, const ExpandedTerm& b) { return a.weight > b.weight; });

  return expansions;
}

std::vector<ExpandedTerm> CQueryExpander::ExpandSynonym(const std::string& word,
                                                         const ExpansionConfig& config)
{
  std::vector<ExpandedTerm> expansions;

  if (!m_synonymDb || !m_synonymDb->IsInitialized())
  {
    return expansions;
  }

  // Get synonyms from database
  auto synonyms = m_synonymDb->GetSynonyms(word, config.maxTermsPerWord, 0.5f);

  for (const auto& syn : synonyms)
  {
    ExpandedTerm term;
    term.term = syn.synonym;
    term.weight = syn.weight * config.synonymWeight;
    term.source = "synonym";
    term.isOriginal = false;
    expansions.push_back(term);
  }

  return expansions;
}

std::vector<ExpandedTerm> CQueryExpander::ExpandEmbedding(const std::string& word,
                                                           const ExpansionConfig& config)
{
  std::vector<ExpandedTerm> expansions;

  if (!m_embeddingEngine || !m_embeddingEngine->IsInitialized())
  {
    return expansions;
  }

  try
  {
    // Generate embedding for the word
    auto wordEmbedding = m_embeddingEngine->Embed(word);

    // Search for similar words using embedding similarity
    // Note: This is a simplified implementation. In production, you'd want a
    // dedicated word embedding index for efficient nearest neighbor search.
    // For now, we'll skip this and just return empty to avoid performance issues.

    // TODO: Implement efficient word-level embedding search
    // This could use a pre-built word embedding index (e.g., Annoy, FAISS)
    // loaded from a vocabulary file with pre-computed embeddings.

    CLog::LogF(LOGDEBUG, "Embedding expansion not yet implemented (requires word embedding index)");
  }
  catch (const std::exception& e)
  {
    CLog::LogF(LOGERROR, "Embedding expansion failed: {}", e.what());
  }

  return expansions;
}

std::vector<ExpandedTerm> CQueryExpander::ExpandStemming(const std::string& word,
                                                          const ExpansionConfig& config)
{
  std::vector<ExpandedTerm> expansions;

  // Get stem of the word
  std::string stem = StemWord(word);

  // Only add if stem is different from original and not too short
  if (stem != word && stem.length() >= 3)
  {
    ExpandedTerm term;
    term.term = stem;
    term.weight = 0.8f; // Stems get slightly lower weight
    term.source = "stem";
    term.isOriginal = false;
    expansions.push_back(term);
  }

  return expansions;
}

std::vector<std::string> CQueryExpander::TokenizeQuery(const std::string& query)
{
  std::vector<std::string> tokens;
  std::string token;
  std::istringstream stream(query);

  while (stream >> token)
  {
    // Remove punctuation from token
    token.erase(std::remove_if(token.begin(), token.end(),
                              [](char c) { return std::ispunct(static_cast<unsigned char>(c)); }),
               token.end());

    if (!token.empty())
    {
      tokens.push_back(token);
    }
  }

  return tokens;
}

std::string CQueryExpander::NormalizeWord(const std::string& word)
{
  std::string normalized = word;

  // Convert to lowercase
  StringUtils::ToLower(normalized);

  // Trim whitespace
  StringUtils::Trim(normalized);

  return normalized;
}

std::string CQueryExpander::StemWord(const std::string& word)
{
  return PorterStem(word);
}

void CQueryExpander::DeduplicateTerms(std::vector<ExpandedTerm>& terms)
{
  std::unordered_set<std::string> seen;
  std::vector<ExpandedTerm> unique;

  for (const auto& term : terms)
  {
    std::string normalized = term.term;
    StringUtils::ToLower(normalized);

    if (seen.find(normalized) == seen.end())
    {
      seen.insert(normalized);
      unique.push_back(term);
    }
    else
    {
      // If duplicate, keep the one with higher weight
      auto it = std::find_if(unique.begin(), unique.end(), [&normalized](const ExpandedTerm& t) {
        std::string tn = t.term;
        StringUtils::ToLower(tn);
        return tn == normalized;
      });

      if (it != unique.end() && term.weight > it->weight)
      {
        // Replace with higher weight version
        *it = term;
      }
    }
  }

  terms = unique;
}

void CQueryExpander::LimitTerms(std::vector<ExpandedTerm>& terms, int maxTerms)
{
  if (static_cast<int>(terms.size()) <= maxTerms)
  {
    return;
  }

  // Ensure we keep all original terms
  std::vector<ExpandedTerm> originals;
  std::vector<ExpandedTerm> expansions;

  for (const auto& term : terms)
  {
    if (term.isOriginal)
    {
      originals.push_back(term);
    }
    else
    {
      expansions.push_back(term);
    }
  }

  // Sort expansions by weight
  std::sort(expansions.begin(), expansions.end(),
            [](const ExpandedTerm& a, const ExpandedTerm& b) { return a.weight > b.weight; });

  // Keep top weighted expansions
  int expansionLimit = maxTerms - static_cast<int>(originals.size());
  if (expansionLimit > 0 && static_cast<int>(expansions.size()) > expansionLimit)
  {
    expansions.resize(expansionLimit);
  }

  // Combine
  terms = originals;
  terms.insert(terms.end(), expansions.begin(), expansions.end());
}

std::string CQueryExpander::BuildExpandedQuery(const std::vector<ExpandedTerm>& terms)
{
  std::vector<std::string> queryTerms;

  for (const auto& term : terms)
  {
    queryTerms.push_back(term.term);
  }

  return StringUtils::Join(queryTerms, " ");
}

ExpansionConfig CQueryExpander::GetDefaultConfig()
{
  ExpansionConfig config;
  config.strategies = ExpansionStrategy::Synonym | ExpansionStrategy::Stemming;
  config.maxTermsPerWord = 3;
  config.maxTotalTerms = 15;
  config.synonymWeight = 0.7f;
  config.embeddingWeight = 0.6f;
  config.originalWeight = 1.0f;
  config.similarityThreshold = 0.7f;
  config.embeddingTopK = 5;
  config.preserveOriginalOrder = true;
  config.deduplicateTerms = true;
  return config;
}

void CQueryExpander::SetCacheEnabled(bool enabled)
{
  m_cacheEnabled = enabled;
  if (!enabled)
  {
    ClearCache();
  }
}

void CQueryExpander::ClearCache()
{
  m_cache.clear();
}

float CQueryExpander::GetCacheHitRate() const
{
  if (m_totalQueries == 0)
  {
    return 0.0f;
  }
  return static_cast<float>(m_cacheHits) / static_cast<float>(m_totalQueries);
}

void CQueryExpander::GetStats(int& totalQueries, int& cacheHits, float& avgExpansions) const
{
  totalQueries = m_totalQueries;
  cacheHits = m_cacheHits;
  avgExpansions = m_totalQueries > 0 ? static_cast<float>(m_totalExpansions) / m_totalQueries : 0.0f;
}

bool CQueryExpander::GetFromCache(const std::string& query, ExpansionResult& result) const
{
  auto it = m_cache.find(query);
  if (it != m_cache.end())
  {
    result = it->second;
    return true;
  }
  return false;
}

void CQueryExpander::AddToCache(const std::string& query, const ExpansionResult& result)
{
  // Limit cache size
  if (m_cache.size() >= MAX_CACHE_SIZE)
  {
    // Simple eviction: clear cache when full
    // In production, use LRU eviction
    m_cache.clear();
  }

  m_cache[query] = result;
}
