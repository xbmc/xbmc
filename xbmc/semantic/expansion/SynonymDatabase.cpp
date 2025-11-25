/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SynonymDatabase.h"

#include "semantic/SemanticDatabase.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <sstream>

using namespace KODI::SEMANTIC;

CSynonymDatabase::CSynonymDatabase() = default;

CSynonymDatabase::~CSynonymDatabase() = default;

bool CSynonymDatabase::Initialize(CSemanticDatabase* database)
{
  if (database == nullptr)
  {
    CLog::LogF(LOGERROR, "Cannot initialize with null database pointer");
    return false;
  }

  m_database = database;

  // Create table if it doesn't exist
  if (!TableExists())
  {
    if (!CreateTable())
    {
      CLog::LogF(LOGERROR, "Failed to create synonym table");
      return false;
    }
  }

  // Load default synonyms if table is empty
  int count = GetSynonymCount();
  if (count == 0)
  {
    CLog::LogF(LOGINFO, "Synonym table empty, loading default synonyms");
    LoadDefaultSynonyms();
  }

  CLog::LogF(LOGDEBUG, "SynonymDatabase initialized successfully ({} entries)", count);
  return true;
}

bool CSynonymDatabase::IsInitialized() const
{
  return m_database != nullptr;
}

std::vector<Synonym> CSynonymDatabase::GetSynonyms(const std::string& word,
                                                    int maxResults,
                                                    float minWeight)
{
  if (!IsInitialized())
  {
    CLog::LogF(LOGERROR, "Database not initialized");
    return {};
  }

  if (word.empty())
  {
    return {};
  }

  // Normalize word to lowercase for lookup
  std::string normalizedWord = word;
  StringUtils::ToLower(normalizedWord);

  // Check cache first
  std::vector<Synonym> cached;
  if (GetFromCache(normalizedWord, cached))
  {
    // Filter by weight and limit results
    std::vector<Synonym> filtered;
    for (const auto& syn : cached)
    {
      if (syn.weight >= minWeight && static_cast<int>(filtered.size()) < maxResults)
      {
        filtered.push_back(syn);
      }
    }
    return filtered;
  }

  // Query database
  std::string sql = StringUtils::Format(
      "SELECT synonym, weight, source FROM semantic_synonyms "
      "WHERE word = '{}' AND weight >= {} "
      "ORDER BY weight DESC LIMIT {}",
      normalizedWord, minWeight, maxResults);

  auto dataset = m_database->Query(sql);
  if (!dataset)
  {
    CLog::LogF(LOGERROR, "Failed to query synonyms for word '{}'", word);
    return {};
  }

  std::vector<Synonym> synonyms;
  while (!dataset->eof())
  {
    Synonym syn;
    syn.word = normalizedWord;
    syn.synonym = dataset->fv("synonym").get_asString();
    syn.weight = dataset->fv("weight").get_asFloat();
    syn.source = dataset->fv("source").get_asString();
    synonyms.push_back(syn);
    dataset->next();
  }

  // Add to cache
  AddToCache(normalizedWord, synonyms);

  return synonyms;
}

bool CSynonymDatabase::AddSynonym(const std::string& word,
                                  const std::string& synonym,
                                  float weight,
                                  const std::string& source,
                                  bool bidirectional)
{
  if (!IsInitialized())
  {
    CLog::LogF(LOGERROR, "Database not initialized");
    return false;
  }

  if (word.empty() || synonym.empty())
  {
    CLog::LogF(LOGERROR, "Word and synonym cannot be empty");
    return false;
  }

  // Normalize to lowercase
  std::string normalizedWord = word;
  std::string normalizedSynonym = synonym;
  StringUtils::ToLower(normalizedWord);
  StringUtils::ToLower(normalizedSynonym);

  // Don't add self-synonyms
  if (normalizedWord == normalizedSynonym)
  {
    return true;
  }

  // Clamp weight to valid range
  weight = std::max(0.0f, std::min(1.0f, weight));

  // Insert or replace synonym
  std::string sql = StringUtils::Format(
      "INSERT OR REPLACE INTO semantic_synonyms (word, synonym, weight, source) "
      "VALUES ('{}', '{}', {}, '{}')",
      normalizedWord, normalizedSynonym, weight, source);

  if (!m_database->ExecuteSQLQuery(sql))
  {
    CLog::LogF(LOGERROR, "Failed to add synonym '{}' -> '{}'", word, synonym);
    return false;
  }

  // Add bidirectional relationship
  if (bidirectional)
  {
    sql = StringUtils::Format(
        "INSERT OR REPLACE INTO semantic_synonyms (word, synonym, weight, source) "
        "VALUES ('{}', '{}', {}, '{}')",
        normalizedSynonym, normalizedWord, weight, source);

    if (!m_database->ExecuteSQLQuery(sql))
    {
      CLog::LogF(LOGWARNING, "Failed to add reverse synonym '{}' -> '{}'", synonym, word);
    }
  }

  // Clear cache for affected words
  ClearCache();

  return true;
}

bool CSynonymDatabase::RemoveSynonym(const std::string& word,
                                     const std::string& synonym,
                                     bool bidirectional)
{
  if (!IsInitialized())
  {
    CLog::LogF(LOGERROR, "Database not initialized");
    return false;
  }

  // Normalize to lowercase
  std::string normalizedWord = word;
  std::string normalizedSynonym = synonym;
  StringUtils::ToLower(normalizedWord);
  StringUtils::ToLower(normalizedSynonym);

  std::string sql = StringUtils::Format("DELETE FROM semantic_synonyms WHERE word = '{}' AND synonym = '{}'",
                                        normalizedWord, normalizedSynonym);

  if (!m_database->ExecuteSQLQuery(sql))
  {
    CLog::LogF(LOGERROR, "Failed to remove synonym '{}' -> '{}'", word, synonym);
    return false;
  }

  // Remove bidirectional relationship
  if (bidirectional)
  {
    sql = StringUtils::Format("DELETE FROM semantic_synonyms WHERE word = '{}' AND synonym = '{}'",
                             normalizedSynonym, normalizedWord);
    m_database->ExecuteSQLQuery(sql);
  }

  // Clear cache
  ClearCache();

  return true;
}

int CSynonymDatabase::LoadFromFile(const std::string& filePath, const std::string& source)
{
  if (!IsInitialized())
  {
    CLog::LogF(LOGERROR, "Database not initialized");
    return -1;
  }

  std::ifstream file(filePath);
  if (!file.is_open())
  {
    CLog::LogF(LOGERROR, "Failed to open synonym file: {}", filePath);
    return -1;
  }

  int loaded = 0;
  std::string line;

  while (std::getline(file, line))
  {
    // Skip empty lines and comments
    StringUtils::Trim(line);
    if (line.empty() || line[0] == '#')
      continue;

    // Parse format: word:synonym1,synonym2,synonym3
    size_t colonPos = line.find(':');
    if (colonPos == std::string::npos)
      continue;

    std::string word = line.substr(0, colonPos);
    std::string synonymList = line.substr(colonPos + 1);

    StringUtils::Trim(word);
    if (word.empty())
      continue;

    // Split synonyms by comma
    std::vector<std::string> synonyms = StringUtils::Split(synonymList, ',');
    for (const auto& synonym : synonyms)
    {
      std::string trimmedSynonym = synonym;
      StringUtils::Trim(trimmedSynonym);
      if (!trimmedSynonym.empty())
      {
        if (AddSynonym(word, trimmedSynonym, 0.8f, source, false))
        {
          loaded++;
        }
      }
    }
  }

  CLog::LogF(LOGINFO, "Loaded {} synonyms from file: {}", loaded, filePath);
  return loaded;
}

int CSynonymDatabase::LoadDefaultSynonyms()
{
  if (!IsInitialized())
  {
    CLog::LogF(LOGERROR, "Database not initialized");
    return -1;
  }

  // Media-related synonyms for better search recall
  struct SynonymPair
  {
    const char* word;
    const char* synonym;
    float weight;
  };

  // Curated list of common media/entertainment synonyms
  static const SynonymPair defaultSynonyms[] = {
      // Movie synonyms
      {"movie", "film", 1.0f},
      {"movie", "picture", 0.8f},
      {"movie", "motion picture", 0.9f},
      {"film", "movie", 1.0f},
      {"film", "picture", 0.8f},

      // TV/Show synonyms
      {"show", "series", 0.9f},
      {"show", "program", 0.8f},
      {"show", "episode", 0.7f},
      {"series", "show", 0.9f},
      {"episode", "show", 0.7f},

      // Action/Scene synonyms
      {"scene", "sequence", 0.8f},
      {"scene", "clip", 0.7f},
      {"fight", "battle", 0.9f},
      {"chase", "pursuit", 0.8f},

      // Character/Person synonyms
      {"character", "person", 0.7f},
      {"character", "role", 0.8f},
      {"actor", "performer", 0.8f},
      {"actress", "performer", 0.8f},
      {"hero", "protagonist", 0.9f},
      {"villain", "antagonist", 0.9f},

      // Emotion synonyms
      {"happy", "joyful", 0.9f},
      {"sad", "melancholy", 0.8f},
      {"scared", "frightened", 0.9f},
      {"angry", "furious", 0.8f},
      {"funny", "humorous", 0.9f},
      {"funny", "comedic", 0.8f},

      // Genre synonyms
      {"scary", "horror", 0.9f},
      {"scary", "frightening", 0.8f},
      {"romantic", "love", 0.8f},
      {"action", "adventure", 0.7f},
      {"thriller", "suspense", 0.9f},

      // Quality synonyms
      {"good", "excellent", 0.7f},
      {"bad", "terrible", 0.7f},
      {"beautiful", "gorgeous", 0.9f},
      {"ugly", "hideous", 0.8f},
      {"big", "large", 0.9f},
      {"small", "tiny", 0.8f},

      // Location synonyms
      {"place", "location", 0.9f},
      {"city", "town", 0.7f},
      {"house", "home", 0.8f},
      {"building", "structure", 0.7f},

      // Object synonyms
      {"car", "automobile", 0.9f},
      {"car", "vehicle", 0.8f},
      {"gun", "weapon", 0.8f},
      {"phone", "telephone", 0.9f},

      // Action verbs
      {"run", "sprint", 0.8f},
      {"walk", "stroll", 0.7f},
      {"jump", "leap", 0.8f},
      {"talk", "speak", 0.9f},
      {"fight", "combat", 0.8f},
      {"kill", "murder", 0.7f},
      {"die", "perish", 0.8f},

      // Time synonyms
      {"night", "evening", 0.7f},
      {"day", "daytime", 0.8f},
      {"morning", "dawn", 0.7f},

      // Music synonyms
      {"song", "track", 0.8f},
      {"song", "music", 0.7f},
      {"music", "audio", 0.6f},

      // Kodi-specific
      {"kodi", "xbmc", 0.9f},
  };

  int loaded = 0;
  for (const auto& pair : defaultSynonyms)
  {
    if (AddSynonym(pair.word, pair.synonym, pair.weight, "default", true))
    {
      loaded++;
    }
  }

  CLog::LogF(LOGINFO, "Loaded {} default synonyms", loaded);
  return loaded;
}

bool CSynonymDatabase::ClearSynonyms(const std::string& source)
{
  if (!IsInitialized())
  {
    CLog::LogF(LOGERROR, "Database not initialized");
    return false;
  }

  std::string sql;
  if (source.empty())
  {
    sql = "DELETE FROM semantic_synonyms";
  }
  else
  {
    sql = StringUtils::Format("DELETE FROM semantic_synonyms WHERE source = '{}'", source);
  }

  if (!m_database->ExecuteSQLQuery(sql))
  {
    CLog::LogF(LOGERROR, "Failed to clear synonyms");
    return false;
  }

  ClearCache();
  return true;
}

int CSynonymDatabase::GetSynonymCount(const std::string& source)
{
  if (!IsInitialized())
  {
    CLog::LogF(LOGERROR, "Database not initialized");
    return -1;
  }

  std::string sql;
  if (source.empty())
  {
    sql = "SELECT COUNT(*) as count FROM semantic_synonyms";
  }
  else
  {
    sql = StringUtils::Format("SELECT COUNT(*) as count FROM semantic_synonyms WHERE source = '{}'",
                             source);
  }

  auto dataset = m_database->Query(sql);
  if (!dataset || dataset->eof())
  {
    return -1;
  }

  return dataset->fv("count").get_asInt();
}

bool CSynonymDatabase::TableExists()
{
  if (!IsInitialized())
  {
    return false;
  }

  auto dataset = m_database->Query(
      "SELECT name FROM sqlite_master WHERE type='table' AND name='semantic_synonyms'");

  return dataset && !dataset->eof();
}

bool CSynonymDatabase::CreateTable()
{
  if (!IsInitialized())
  {
    CLog::LogF(LOGERROR, "Database not initialized");
    return false;
  }

  CLog::LogF(LOGINFO, "Creating semantic_synonyms table");

  std::string sql = "CREATE TABLE IF NOT EXISTS semantic_synonyms ("
                    "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
                    "  word TEXT NOT NULL,"
                    "  synonym TEXT NOT NULL,"
                    "  weight REAL DEFAULT 1.0,"
                    "  source TEXT DEFAULT 'wordnet',"
                    "  created_at TEXT DEFAULT (datetime('now')),"
                    "  UNIQUE(word, synonym)"
                    ")";

  if (!m_database->ExecuteSQLQuery(sql))
  {
    CLog::LogF(LOGERROR, "Failed to create semantic_synonyms table");
    return false;
  }

  // Create index for fast word lookup
  sql = "CREATE INDEX IF NOT EXISTS idx_synonyms_word ON semantic_synonyms(word)";
  if (!m_database->ExecuteSQLQuery(sql))
  {
    CLog::LogF(LOGWARNING, "Failed to create index on semantic_synonyms");
  }

  // Create index for source filtering
  sql = "CREATE INDEX IF NOT EXISTS idx_synonyms_source ON semantic_synonyms(source)";
  if (!m_database->ExecuteSQLQuery(sql))
  {
    CLog::LogF(LOGWARNING, "Failed to create source index on semantic_synonyms");
  }

  return true;
}

bool CSynonymDatabase::GetFromCache(const std::string& word,
                                    std::vector<Synonym>& synonyms) const
{
  auto now = std::chrono::steady_clock::now();

  for (const auto& entry : m_cache)
  {
    if (entry.word == word)
    {
      // Check if cache entry is still valid
      auto age = std::chrono::duration_cast<std::chrono::seconds>(now - entry.timestamp).count();
      if (age < CACHE_TTL_SECONDS)
      {
        synonyms = entry.synonyms;
        return true;
      }
    }
  }

  return false;
}

void CSynonymDatabase::AddToCache(const std::string& word, const std::vector<Synonym>& synonyms)
{
  // Remove old entry for this word if it exists
  m_cache.erase(std::remove_if(m_cache.begin(), m_cache.end(),
                               [&word](const SynonymCache& entry) { return entry.word == word; }),
               m_cache.end());

  // Add new entry
  SynonymCache entry;
  entry.word = word;
  entry.synonyms = synonyms;
  entry.timestamp = std::chrono::steady_clock::now();
  m_cache.push_back(entry);

  // Limit cache size (LRU eviction - remove oldest)
  if (m_cache.size() > MAX_CACHE_SIZE)
  {
    m_cache.erase(m_cache.begin());
  }
}

void CSynonymDatabase::ClearCache()
{
  m_cache.clear();
}
