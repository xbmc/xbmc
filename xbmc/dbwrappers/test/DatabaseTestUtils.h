/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DatabaseManager.h"
#include "ServiceBroker.h"
#include "dbwrappers/Database.h"
#include "filesystem/SpecialProtocol.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"

#include <string>

#include <gtest/gtest.h>

//! \brief Where a test database lives: an sqlite file in special://temp/.
inline DatabaseSettings TestDatabaseSettings()
{
  DatabaseSettings settings;
  settings.type = "sqlite3";
  settings.host = CSpecialProtocol::TranslatePath("special://temp/");
  return settings;
}

/*!
 * \brief Upgrades the test database named \p baseName as Kodi does at startup: points \p slot of
 * the advanced settings at it and reinitialises CDatabaseManager, then puts both back.
 * \return whether the database manager initialised
 */
inline bool UpgradeThroughManager(DatabaseSettings CAdvancedSettings::* slot,
                                  const std::string& baseName)
{
  const auto advancedSettings{CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()};
  DatabaseSettings& target{(*advancedSettings).*slot};
  const DatabaseSettings restore{target};

  const DatabaseSettings test{TestDatabaseSettings()};
  target.type = test.type;
  target.host = test.host;
  target.name = baseName;

  CDatabaseManager& manager{CServiceBroker::GetDatabaseManager()};
  manager.Deinitialize();
  const bool initialized{manager.Initialize()};

  target = restore;
  manager.Deinitialize();

  return initialized;
}

/*!
 * \brief Restates the tables the 152 video upgrade changes as 150 defined them, so that upgrade can
 * run over a database Connect() built at the current schema.
 *
 * Restated rather than derived from the current tables, so a column added to the 152 upgrade and
 * not taken back out here cannot abort the upgrade on a duplicate.
 */
inline void RestateVideo150(CDatabase& db)
{
  ASSERT_TRUE(db.ExecuteQuery("DROP TABLE contentgeometry"));
  ASSERT_TRUE(db.ExecuteQuery("DROP TABLE settings"));
  ASSERT_TRUE(db.ExecuteQuery(
      "CREATE TABLE settings ( idFile integer, Deinterlace bool,"
      "ViewMode integer,ZoomAmount float, PixelRatio float, VerticalShift float, AudioStream "
      "integer, SubtitleStream integer,"
      "SubtitleDelay float, SubtitlesOn bool, Brightness float, Contrast float, Gamma float,"
      "VolumeAmplification float, AudioDelay float, ResumeTime integer,"
      "Sharpness float, NoiseReduction float, NonLinStretch bool, PostProcess bool,"
      "ScalingMethod integer, DeinterlaceMode integer, StereoMode integer, StereoInvert bool, "
      "VideoStream integer,"
      "TonemapMethod integer, TonemapParam float, Orientation integer, CenterMixLevel integer)"));
}
