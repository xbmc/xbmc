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
#include "filesystem/File.h"
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
 * \brief An empty database named \p name of a test's own, connected for the life of the guard and
 * deleted after it. Connect() rather than Open(), so CDatabaseManager plays no part.
 */
template<typename DATABASE>
class CConnectedTestDatabase
{
public:
  explicit CConnectedTestDatabase(const std::string& name) : m_name(name)
  {
    EXPECT_EQ(CDatabase::ConnectionState::STATE_CONNECTED,
              m_db.Connect(m_name, TestDatabaseSettings(), true))
        << "could not create the test database " << m_name;
  }

  ~CConnectedTestDatabase()
  {
    m_db.Close();
    XFILE::CFile::Delete(TestDatabaseSettings().host + m_name + ".db");
  }

  CConnectedTestDatabase(const CConnectedTestDatabase&) = delete;
  CConnectedTestDatabase& operator=(const CConnectedTestDatabase&) = delete;

  DATABASE& Get() { return m_db; }

private:
  const std::string m_name;
  DATABASE m_db;
};

/*!
 * \brief Points a database slot of the advanced settings, which Open() connects to, at an empty
 * test database for the life of the guard, so code that opens the library itself can be tested.
 */
template<typename DATABASE>
class CScopedTestDatabase
{
public:
  CScopedTestDatabase(DatabaseSettings CAdvancedSettings::* slot, const std::string& baseName)
    : m_slot(slot),
      m_settings(TestDatabaseSettings()),
      m_restore(Target())
  {
    CDatabaseManager& manager{CServiceBroker::GetDatabaseManager()};
    if (!manager.CanOpen(m_db.GetBaseDBName()) && !manager.Initialize())
    {
      ADD_FAILURE() << "the database manager did not initialise";
      return;
    }

    m_settings.name = baseName;
    Target() = m_settings;
    m_settings.name += std::to_string(m_db.GetSchemaVersion());
    EXPECT_EQ(CDatabase::ConnectionState::STATE_CONNECTED,
              m_db.Connect(m_settings.name, m_settings, true));
  }

  ~CScopedTestDatabase()
  {
    Target() = m_restore;
    m_db.Close();
    XFILE::CFile::Delete(m_settings.host + m_settings.name + ".db");
  }

  CScopedTestDatabase(const CScopedTestDatabase&) = delete;
  CScopedTestDatabase& operator=(const CScopedTestDatabase&) = delete;

  DATABASE& Get() { return m_db; }

private:
  struct Database : DATABASE
  {
    using DATABASE::CDatabase::GetBaseDBName;
    using DATABASE::CDatabase::GetSchemaVersion;
  };

  DatabaseSettings& Target()
  {
    return (*CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()).*m_slot;
  }

  DatabaseSettings CAdvancedSettings::* m_slot;
  DatabaseSettings m_settings;
  DatabaseSettings m_restore;
  Database m_db;
};
