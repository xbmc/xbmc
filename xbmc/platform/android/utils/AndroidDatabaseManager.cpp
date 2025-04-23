/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AndroidDatabaseManager.h"
#include "ServiceBroker.h"
#include "utils/log.h"
#include "threads/Thread.h"
#include "utils/JobManager.h"

#include <algorithm>

std::mutex CAndroidDatabaseManager::m_poolMutex;
std::map<std::string, std::vector<std::pair<std::shared_ptr<CDatabase>, unsigned int>>> CAndroidDatabaseManager::m_connectionPool;

class CAsyncDatabaseJob : public CJob
{
public:
  CAsyncDatabaseJob(const std::string& dbName, const std::string& sql, std::function<void(bool)> callback)
    : m_dbName(dbName)
    , m_sql(sql)
    , m_callback(callback)
  {}

  ~CAsyncDatabaseJob() override = default;

  bool DoWork() override
  {
    bool success = false;
    
    std::shared_ptr<CDatabase> db = CAndroidDatabaseManager::GetPooledConnection(m_dbName);
    if (db && db->IsOpen())
    {
      try
      {
        db->ExecuteSQL(m_sql);
        success = true;
      }
      catch (...)
      {
        CLog::Log(LOGERROR, "CAsyncDatabaseJob: Failed to execute SQL: %s", m_sql.c_str());
        success = false;
      }
      
      CAndroidDatabaseManager::ReturnConnection(m_dbName, db);
    }
    
    if (m_callback)
    {
      m_callback(success);
    }
    
    return success;
  }

private:
  std::string m_dbName;
  std::string m_sql;
  std::function<void(bool)> m_callback;
};

void CAndroidDatabaseManager::Initialize()
{
  // Preallocate some space in the connection pool map to avoid reallocations
  m_connectionPool.reserve(5);
  
  CLog::Log(LOGINFO, "CAndroidDatabaseManager: Initialized database connection pool");
}

std::shared_ptr<CDatabase> CAndroidDatabaseManager::GetPooledConnection(const std::string& dbName)
{
  std::unique_lock<std::mutex> lock(m_poolMutex);
  
  // Get the current time for tracking connection usage
  unsigned int currentTime = CThread::GetTickCount();
  
  // Check if we have a connection pool for this database
  auto poolIt = m_connectionPool.find(dbName);
  if (poolIt != m_connectionPool.end() && !poolIt->second.empty())
  {
    // We have connections in the pool, use the first one
    auto connectionPair = poolIt->second.back();
    poolIt->second.pop_back();
    
    std::shared_ptr<CDatabase> connection = connectionPair.first;
    
    // Check if the connection is still valid
    if (connection && connection->IsOpen())
    {
      CLog::Log(LOGDEBUG, "CAndroidDatabaseManager: Reusing pooled connection for database %s", dbName.c_str());
      return connection;
    }
    
    // Connection is not valid, discard it and create a new one
    CLog::Log(LOGDEBUG, "CAndroidDatabaseManager: Discarding invalid pooled connection for database %s", dbName.c_str());
  }
  
  // Create a new database connection
  std::shared_ptr<CDatabase> newDb = std::make_shared<CDatabase>();
  if (newDb->Open(dbName))
  {
    OptimizeDatabase(newDb.get());
    CLog::Log(LOGDEBUG, "CAndroidDatabaseManager: Created new connection for database %s", dbName.c_str());
    return newDb;
  }
  
  CLog::Log(LOGERROR, "CAndroidDatabaseManager: Failed to create connection for database %s", dbName.c_str());
  return nullptr;
}

void CAndroidDatabaseManager::ReturnConnection(const std::string& dbName, std::shared_ptr<CDatabase> connection)
{
  if (!connection)
    return;
  
  std::unique_lock<std::mutex> lock(m_poolMutex);
  
  // Get the current time for tracking connection usage
  unsigned int currentTime = CThread::GetTickCount();
  
  // Check if we have reached the maximum number of connections for this database
  auto& connections = m_connectionPool[dbName];
  if (connections.size() >= MAX_DB_CONNECTIONS)
  {
    // Pool is full, discard this connection
    CLog::Log(LOGDEBUG, "CAndroidDatabaseManager: Connection pool full for database %s, discarding connection", dbName.c_str());
    return;
  }
  
  // Add the connection back to the pool with the current timestamp
  connections.push_back(std::make_pair(connection, currentTime));
  CLog::Log(LOGDEBUG, "CAndroidDatabaseManager: Returned connection to pool for database %s", dbName.c_str());
}

void CAndroidDatabaseManager::OptimizeDatabase(CDatabase* db)
{
  if (!db || !db->IsOpen())
    return;
  
  // Apply Android-specific optimizations to the database
  try
  {
    db->ExecuteSQL(ANDROID_SQLITE_OPTIMIZE_PRAGMAS);
    
    // Add indexes for frequently accessed columns
    // For the Texture database
    if (db->GetDatabaseName() == "Textures")
    {
      // Check if the index already exists before creating
      if (!db->IndexExists("ix_textures_lastused"))
      {
        db->ExecuteSQL("CREATE INDEX IF NOT EXISTS ix_textures_lastused ON texture(lastusetime);");
      }
      if (!db->IndexExists("ix_textures_url"))
      {
        db->ExecuteSQL("CREATE INDEX IF NOT EXISTS ix_textures_url ON texture(url);");
      }
    }
    // For the music database
    else if (db->GetDatabaseName().find("MyMusic") != std::string::npos)
    {
      // Check if the index already exists before creating
      if (!db->IndexExists("ix_song_artist"))
      {
        db->ExecuteSQL("CREATE INDEX IF NOT EXISTS ix_song_artist ON song(idArtist);");
      }
      if (!db->IndexExists("ix_album_artist"))
      {
        db->ExecuteSQL("CREATE INDEX IF NOT EXISTS ix_album_artist ON album(idArtist);");
      }
    }
    // For the video database
    else if (db->GetDatabaseName().find("MyVideos") != std::string::npos)
    {
      // Check if the index already exists before creating
      if (!db->IndexExists("ix_files_path"))
      {
        db->ExecuteSQL("CREATE INDEX IF NOT EXISTS ix_files_path ON files(strPath);");
      }
      if (!db->IndexExists("ix_movie_title"))
      {
        db->ExecuteSQL("CREATE INDEX IF NOT EXISTS ix_movie_title ON movie(c00);");
      }
    }
    
    CLog::Log(LOGINFO, "CAndroidDatabaseManager: Applied Android-specific optimizations to database %s", db->GetDatabaseName().c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CAndroidDatabaseManager: Failed to apply optimizations to database %s", db->GetDatabaseName().c_str());
  }
}

void CAndroidDatabaseManager::CleanupIdleConnections()
{
  std::unique_lock<std::mutex> lock(m_poolMutex);
  
  unsigned int currentTime = CThread::GetTickCount();
  int removedCount = 0;
  
  // Iterate through all pools
  for (auto& poolPair : m_connectionPool)
  {
    auto& connections = poolPair.second;
    
    // Remove connections that have been idle for too long
    connections.erase(std::remove_if(connections.begin(), connections.end(), 
                                   [&currentTime, &removedCount](const std::pair<std::shared_ptr<CDatabase>, unsigned int>& pair) {
                                     bool remove = (currentTime - pair.second) > MAX_CONNECTION_IDLE_TIME;
                                     if (remove) removedCount++;
                                     return remove;
                                   }), 
                    connections.end());
  }
  
  if (removedCount > 0)
  {
    CLog::Log(LOGINFO, "CAndroidDatabaseManager: Cleaned up %d idle database connections", removedCount);
  }
}

void CAndroidDatabaseManager::ExecuteAsync(const std::string& dbName, const std::string& sql, std::function<void(bool)> callback)
{
  // Create a new job for asynchronous SQL execution
  std::shared_ptr<CAsyncDatabaseJob> job = std::make_shared<CAsyncDatabaseJob>(dbName, sql, callback);
  CServiceBroker::GetJobManager()->AddJob(job, nullptr);
}