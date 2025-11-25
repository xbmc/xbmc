/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>
#include <string>
#include <vector>

namespace KODI
{
namespace SEMANTIC
{

// Forward declaration
class CSemanticDatabase;

/*!
 * @brief Synonym entry with weight
 */
struct Synonym
{
  std::string word;         //!< Original word
  std::string synonym;      //!< Synonym term
  float weight{1.0f};       //!< Synonym weight/confidence (0-1)
  std::string source;       //!< Source of synonym (e.g., "wordnet", "custom")
};

/*!
 * @brief Database manager for query expansion synonyms
 *
 * This class manages a synonym database stored in SQLite for query expansion.
 * It supports WordNet-style synonym relationships with weights and custom additions.
 *
 * The synonym database stores bidirectional relationships:
 * - "car" -> "automobile", "vehicle"
 * - "automobile" -> "car", "vehicle"
 *
 * Features:
 * - WordNet-based synonym loading
 * - Custom synonym additions
 * - Weighted synonyms for relevance control
 * - Efficient lookup via database indexes
 * - Bidirectional synonym relationships
 * - Synonym caching for performance
 *
 * Database schema:
 * \code
 * CREATE TABLE semantic_synonyms (
 *   id INTEGER PRIMARY KEY AUTOINCREMENT,
 *   word TEXT NOT NULL,
 *   synonym TEXT NOT NULL,
 *   weight REAL DEFAULT 1.0,
 *   source TEXT DEFAULT 'wordnet',
 *   created_at TEXT DEFAULT (datetime('now')),
 *   UNIQUE(word, synonym)
 * );
 * CREATE INDEX idx_synonyms_word ON semantic_synonyms(word);
 * \endcode
 *
 * Example usage:
 * \code
 * CSynonymDatabase synDb;
 * synDb.Initialize(database);
 *
 * // Get synonyms for a word
 * auto synonyms = synDb.GetSynonyms("car");
 * // synonyms = ["automobile" (0.9), "vehicle" (0.8)]
 *
 * // Add custom synonym
 * synDb.AddSynonym("kodi", "xbmc", 1.0f, "custom");
 * \endcode
 */
class CSynonymDatabase
{
public:
  CSynonymDatabase();
  ~CSynonymDatabase();

  /*!
   * @brief Initialize the synonym database
   * @param database Pointer to semantic database (must remain valid)
   * @return true if initialization succeeded, false otherwise
   */
  bool Initialize(CSemanticDatabase* database);

  /*!
   * @brief Check if the database is initialized
   * @return true if initialized and ready
   */
  bool IsInitialized() const;

  /*!
   * @brief Get synonyms for a word
   * @param word The word to look up
   * @param maxResults Maximum number of synonyms to return (default: 5)
   * @param minWeight Minimum weight threshold (default: 0.5)
   * @return Vector of synonym entries sorted by weight (highest first)
   */
  std::vector<Synonym> GetSynonyms(const std::string& word,
                                   int maxResults = 5,
                                   float minWeight = 0.5f);

  /*!
   * @brief Add a synonym relationship
   * @param word The original word
   * @param synonym The synonym term
   * @param weight Synonym weight/confidence (0-1, default: 1.0)
   * @param source Source identifier (default: "custom")
   * @param bidirectional If true, also add reverse relationship (default: true)
   * @return true if added successfully, false otherwise
   *
   * If bidirectional is true, this creates both:
   * - word -> synonym
   * - synonym -> word
   */
  bool AddSynonym(const std::string& word,
                  const std::string& synonym,
                  float weight = 1.0f,
                  const std::string& source = "custom",
                  bool bidirectional = true);

  /*!
   * @brief Remove a synonym relationship
   * @param word The original word
   * @param synonym The synonym to remove
   * @param bidirectional If true, also remove reverse relationship (default: true)
   * @return true if removed successfully, false otherwise
   */
  bool RemoveSynonym(const std::string& word, const std::string& synonym, bool bidirectional = true);

  /*!
   * @brief Load synonyms from a WordNet-style text file
   * @param filePath Path to the synonym file
   * @param source Source identifier (default: "wordnet")
   * @return Number of synonym relationships loaded, -1 on error
   *
   * File format (one per line):
   * word:synonym1,synonym2,synonym3
   * car:automobile,vehicle,auto
   */
  int LoadFromFile(const std::string& filePath, const std::string& source = "wordnet");

  /*!
   * @brief Load default English synonyms from built-in data
   * @return Number of synonym relationships loaded, -1 on error
   *
   * Loads a curated set of common English synonyms for media-related terms.
   */
  int LoadDefaultSynonyms();

  /*!
   * @brief Clear all synonyms from the database
   * @param source Optional source filter (empty = clear all)
   * @return true if cleared successfully, false otherwise
   */
  bool ClearSynonyms(const std::string& source = "");

  /*!
   * @brief Get count of synonym entries in database
   * @param source Optional source filter (empty = count all)
   * @return Number of synonym entries, -1 on error
   */
  int GetSynonymCount(const std::string& source = "");

  /*!
   * @brief Check if synonym table exists
   * @return true if table exists, false otherwise
   */
  bool TableExists();

  /*!
   * @brief Create the synonym table if it doesn't exist
   * @return true if table created or already exists, false on error
   */
  bool CreateTable();

private:
  CSemanticDatabase* m_database{nullptr};

  // Synonym cache for performance
  struct SynonymCache
  {
    std::string word;
    std::vector<Synonym> synonyms;
    std::chrono::steady_clock::time_point timestamp;
  };

  mutable std::vector<SynonymCache> m_cache;
  static constexpr size_t MAX_CACHE_SIZE = 100;
  static constexpr int CACHE_TTL_SECONDS = 300; // 5 minutes

  // Cache helpers
  bool GetFromCache(const std::string& word, std::vector<Synonym>& synonyms) const;
  void AddToCache(const std::string& word, const std::vector<Synonym>& synonyms);
  void ClearCache();
};

} // namespace SEMANTIC
} // namespace KODI
