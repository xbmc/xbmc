/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "cores/VideoSettings.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "settings/AdvancedSettings.h"
#include "video/VideoDatabase.h"

#include <string>

#include <gtest/gtest.h>

namespace
{
constexpr const char* DB_NAME = "TestVideoDatabase";

class TestVideoDatabase : public ::testing::Test
{
protected:
  void SetUp() override
  {
    m_settings.type = "sqlite3";
    m_settings.name = DB_NAME;
    m_settings.host = CSpecialProtocol::TranslatePath("special://temp/");
    ASSERT_EQ(CDatabase::ConnectionState::STATE_CONNECTED, m_db.Connect(DB_NAME, m_settings, true));
  }

  void TearDown() override
  {
    m_db.Close();
    XFILE::CFile::Delete(m_settings.host + DB_NAME + ".db");
  }

  DatabaseSettings m_settings;
  CVideoDatabase m_db;
};
} // namespace

TEST_F(TestVideoDatabase, InsertStoresOrientationAndCenterMixLevel)
{
  const int idFile = m_db.AddFile("C:/videos/rotated.mkv");
  ASSERT_GT(idFile, 0);

  CVideoSettings rotated;
  rotated.m_Orientation = 90;
  rotated.m_CenterMixLevel = 3;
  m_db.SetVideoSettings(idFile, rotated);

  CVideoSettings stored;
  ASSERT_TRUE(m_db.GetVideoSettings(idFile, stored));
  EXPECT_EQ(90, stored.m_Orientation);
  EXPECT_EQ(3, stored.m_CenterMixLevel);
}

TEST_F(TestVideoDatabase, UpdateStoresOrientationAndCenterMixLevel)
{
  const int idFile = m_db.AddFile("C:/videos/rotated.mkv");
  ASSERT_GT(idFile, 0);

  m_db.SetVideoSettings(idFile, CVideoSettings());

  CVideoSettings rotated;
  rotated.m_Orientation = 90;
  rotated.m_CenterMixLevel = 3;
  m_db.SetVideoSettings(idFile, rotated);

  CVideoSettings stored;
  ASSERT_TRUE(m_db.GetVideoSettings(idFile, stored));
  EXPECT_EQ(90, stored.m_Orientation);
  EXPECT_EQ(3, stored.m_CenterMixLevel);
}
