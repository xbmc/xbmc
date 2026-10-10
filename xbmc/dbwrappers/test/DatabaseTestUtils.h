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
