/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GameClientTable.h"

#include "dbwrappers/Database.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

using namespace KODI;
using namespace GAME;

namespace
{
/*!
 * \brief How many folders above a game are asked for an emulator
 *
 * GetParentPath() stops handing back new paths at the root, so the walk ends
 * on its own for every path this has been given. The limit is there so that a
 * path form which never shrinks cannot spin forever.
 */
constexpr unsigned int MAX_FOLDER_DEPTH = 64;
} // namespace

CGameClientTable::CGameClientTable(CDatabase& database) : m_database(database)
{
}

CGameClientTable::~CGameClientTable() = default;

void CGameClientTable::Create()
{
  CLog::Log(LOGINFO, "GAME: Creating gameclient table");

  m_database.ExecuteQuery("CREATE TABLE gameclient ("
                          "idPath integer primary key,"
                          "path text,"
                          "gameClient text)");
}

void CGameClientTable::CreateAnalytics()
{
  CLog::Log(LOGINFO, "GAME: Creating gameclient index");

  // Unique: a path has one emulator, so storing a second replaces the first
  m_database.ExecuteQuery("CREATE UNIQUE INDEX idxGameClientPath ON gameclient(path)");
}

void CGameClientTable::UpdateTables(int version)
{
  // No schema changes yet
  (void)version;
}

bool CGameClientTable::SetGameClient(const std::string& path, const std::string& gameClient)
{
  if (path.empty())
    return false;

  // No emulator means the path is no longer remembered, rather than remembered
  // as nothing
  if (gameClient.empty())
    return ClearGameClient(path);

  const std::string sql =
      m_database.PrepareSQL("REPLACE INTO gameclient (path, gameClient) VALUES ('%s', '%s')",
                            path.c_str(), gameClient.c_str());

  if (!m_database.ExecuteQuery(sql))
  {
    CLog::Log(LOGERROR, "GAME: Failed to remember emulator {} for {}", gameClient, path);
    return false;
  }

  return true;
}

std::string CGameClientTable::GetGameClient(const std::string& path)
{
  if (path.empty())
    return "";

  const std::string sql =
      m_database.PrepareSQL("SELECT gameClient FROM gameclient WHERE path='%s'", path.c_str());

  return m_database.GetSingleValue(sql);
}

std::string CGameClientTable::GetGameClientForGame(const std::string& path)
{
  if (path.empty())
    return "";

  // The game's own emulator wins, which is what makes a per-game choice an
  // override of the folder it sits in
  std::string gameClient = GetGameClient(path);
  if (!gameClient.empty())
    return gameClient;

  // Otherwise the nearest folder above it that has one
  std::string currentPath = path;
  for (unsigned int i = 0; i < MAX_FOLDER_DEPTH; ++i)
  {
    std::string parentPath;
    if (!URIUtils::GetParentPath(currentPath, parentPath) || parentPath.empty() ||
        parentPath == currentPath)
      break;

    gameClient = GetGameClient(parentPath);
    if (!gameClient.empty())
      return gameClient;

    currentPath = std::move(parentPath);
  }

  return "";
}

bool CGameClientTable::ClearGameClient(const std::string& path)
{
  if (path.empty())
    return false;

  const std::string sql =
      m_database.PrepareSQL("DELETE FROM gameclient WHERE path='%s'", path.c_str());

  if (!m_database.ExecuteQuery(sql))
  {
    CLog::Log(LOGERROR, "GAME: Failed to forget the emulator for {}", path);
    return false;
  }

  return true;
}
