/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "filesystem/SpecialProtocol.h"
#include "utils/StringUtils.h"
#include "video/VideoDatabase.h"

#include <string>
#include <utility>

#include <gtest/gtest.h>

/*!
 * \brief A fixture holding a private video database in special://temp/.
 *
 * Connect() rather than Open(), so this brings up the video schema alone rather than every
 * database Kodi has by way of CDatabaseManager. Each test gets a database of its own, named
 * after it, so no test reads another's rows.
 */
class VideoDatabaseTestBase : public ::testing::Test
{
protected:
  VideoDatabaseTestBase(std::string namePrefix, std::string pathPrefix)
    : m_namePrefix(std::move(namePrefix)),
      m_pathPrefix(std::move(pathPrefix))
  {
  }

  void SetUp() override
  {
    DatabaseSettings settings;
    settings.type = "sqlite3";
    settings.host = CSpecialProtocol::TranslatePath("special://temp/");

    const std::string name{StringUtils::Format(
        "{}{}", m_namePrefix, ::testing::UnitTest::GetInstance()->current_test_info()->name())};

    ASSERT_EQ(CDatabase::ConnectionState::STATE_CONNECTED, m_db.Connect(name, settings, true))
        << "could not create a video database for this test";
  }

  void TearDown() override { m_db.Close(); }

  int AddTestFile(const std::string& name)
  {
    const int idFile{m_db.AddFile(m_pathPrefix + name, m_pathPrefix)};
    EXPECT_GE(idFile, 0);
    return idFile;
  }

  CVideoDatabase m_db;

private:
  const std::string m_namePrefix;
  const std::string m_pathPrefix;
};
