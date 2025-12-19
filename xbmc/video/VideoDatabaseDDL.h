/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "dbwrappers/Database.h"
#include "dbwrappers/dataset.h"

#include <string>

namespace KODI::DATABASE
{
class CVideoDatabaseDDL
{
public:
  CVideoDatabaseDDL() = delete;

  /*!
   * \brief Create the tables of the video database
   * \param[in] db the database
   */
  static void CreateTables(CDatabase& db);

  /*!
   * \brief Create the indexes, triggers and views of the video database
   * \param[in] db the database
   */
  static void CreateAnalytics(CDatabase& db);

private:
  static void CreateLinkIndex(CDatabase& db, const std::string& table);
  static void CreateForeignLinkIndex(CDatabase& db,
                                     const std::string& table,
                                     const std::string& foreignkey);
  static void CreateIndices(CDatabase& db);
  static void CreateTriggers(CDatabase& db);
  static void CreateViews(CDatabase& db);
  static void InitializeVideoVersionTypeTable(CDatabase& db);
};
} // namespace KODI::DATABASE
