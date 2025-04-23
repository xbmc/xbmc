/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "dbwrappers/Database.h"
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

// Maximum number of database connections in the pool
#define MAX_DB_CONNECTIONS 4

// Max time a connection can be idle before being recycled (ms)
#define MAX_CONNECTION_IDLE_TIME 60000 // 1 minute

// Pragmas to optimize SQLite for Android
#define ANDROID_SQLITE_OPTIMIZE_PRAGMAS \
  "PRAGMA synchronous=NORMAL;" \
  "PRAGMA journal_mode=WAL;" \
  "PRAGMA temp_store=MEMORY;" \
  "PRAGMA cache_size=2000;" \
  "PRAGMA auto_vacuum=FULL;"

class CAndroidDatabaseManager
{
public:
  /**
   * @brief Initialize the Android database manager
   */
  static void Initialize();

  /**
   * @brief Get a database connection from the pool
   * @param dbName Name of the database
   * @return Database connection or nullptr if not available
   */
  static std::shared_ptr<CDatabase> GetPooledConnection(const std::string& dbName);

  /**
   * @brief Return a connection to the pool for reuse
   * @param dbName Name of the database
   * @param connection Database connection to return
   */
  static void ReturnConnection(const std::string& dbName, std::shared_ptr<CDatabase> connection);

  /**
   * @brief Apply Android-specific optimizations to the database
   * @param db Database to optimize
   */
  static void OptimizeDatabase(CDatabase* db);
  
  /**
   * @brief Clean up unused database connections
   */
  static void CleanupIdleConnections();
  
  /**
   * @brief Execute SQL statement asynchronously
   * @param db Database name
   * @param sql SQL statement to execute
   * @param callback Optional callback when complete
   */
  static void ExecuteAsync(const std::string& dbName, const std::string& sql, 
                          std::function<void(bool)> callback = nullptr);

private:
  static std::mutex m_poolMutex;
  
  // Database connection pool - map of database names to vector of available connections
  static std::map<std::string, std::vector<std::pair<std::shared_ptr<CDatabase>, unsigned int>>> m_connectionPool;
};