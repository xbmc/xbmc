/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

class CDatabase;

namespace KODI
{
namespace GAME
{
/*!
 * \ingroup games
 *
 * \brief The gameclient table, which remembers which emulator to open a game
 *        with
 *
 * One row says "this path uses this emulator". The path is either a game or a
 * folder holding games, which is what makes per-game overrides fall out for
 * free: a game is looked up first, and only if it has no emulator of its own
 * is the folder above it asked, and the folder above that.
 *
 * The table owns its schema and its queries. It reaches the database it belongs
 * to through CDatabase's public interface only, so nothing here depends on how
 * the database holds its connection.
 */
class CGameClientTable
{
public:
  /*!
   * \brief Create the table's interface to the database holding it
   *
   * \param database The database this table lives in, which must outlive this
   */
  explicit CGameClientTable(CDatabase& database);
  ~CGameClientTable();

  /*!
   * \brief Create the table
   */
  void Create();

  /*!
   * \brief Create the table's indices
   */
  void CreateAnalytics();

  /*!
   * \brief Bring the table up to the current schema version
   *
   * \param version The schema version being upgraded from
   */
  void UpdateTables(int version);

  /*!
   * \brief Remember the emulator to open a game or folder with
   *
   * \param path The game or folder
   * \param gameClient The add-on ID of the emulator
   *
   * \return True if the emulator was stored
   */
  bool SetGameClient(const std::string& path, const std::string& gameClient);

  /*!
   * \brief The emulator remembered for exactly this path
   *
   * \param path The game or folder
   *
   * \return The add-on ID, or empty if this path has none of its own
   */
  std::string GetGameClient(const std::string& path);

  /*!
   * \brief The emulator remembered for a game, or for the nearest folder above it
   *
   * \param path The game
   *
   * \return The add-on ID, or empty if neither the game nor any folder above
   *         it has one
   */
  std::string GetGameClientForGame(const std::string& path);

private:
  /*!
   * \brief Forget the emulator remembered for a game or folder
   *
   * Reached through SetGameClient() with no emulator, which is how the "None"
   * entry is expressed.
   *
   * \param path The game or folder
   *
   * \return True if the path is no longer remembered
   */
  bool ClearGameClient(const std::string& path);

  // Construction parameters
  CDatabase& m_database;
};
} // namespace GAME
} // namespace KODI
