/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "GameClientTable.h"
#include "dbwrappers/Database.h"

namespace KODI
{
namespace GAME
{
/*!
 * \brief Base name of the database file, before the schema version suffix
 *
 * Kodi opens "Games" + the schema version, so this is the name every version
 * of the file shares. Changing it orphans the user's existing database.
 */
constexpr const char* GAME_DATABASE_NAME = "Games";

/*!
 * \ingroup games
 *
 * \brief The database of everything Kodi remembers about games
 *
 * The database owns the file and its schema version; each table owns its own
 * schema and queries and is reached through an accessor. Adding a table means
 * adding a class and a line to each of the three overrides below, rather than
 * adding another set of queries here.
 */
class CGameDatabase : public CDatabase
{
public:
  CGameDatabase();
  ~CGameDatabase() override;

  // Implementation of CDatabase
  bool Open() override;

  /*!
   * \brief The table remembering which emulator to open a game with
   */
  CGameClientTable& GameClients() { return m_gameClients; }

protected:
  // Implementation of CDatabase
  void CreateTables() override;
  void CreateAnalytics() override;
  void UpdateTables(int version) override;
  int GetSchemaVersion() const override { return 1; }
  const char* GetBaseDBName() const override { return GAME_DATABASE_NAME; }

private:
  // Tables
  CGameClientTable m_gameClients{*this};
};
} // namespace GAME
} // namespace KODI
