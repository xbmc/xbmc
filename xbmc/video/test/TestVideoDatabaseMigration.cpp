/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "settings/AdvancedSettings.h"
#include "utils/StringUtils.h"
#include "video/VideoDatabase.h"
#include "video/VideoDatabaseMigration.h"
#include "video/VideoManagerTypes.h"

#include <string>

#include <gtest/gtest.h>

namespace
{
constexpr int ASSET_VERSION{1};
constexpr int ASSET_EXTRA{2};
constexpr int OWNER_SYSTEM{0};
constexpr int OWNER_USER{2};

constexpr int SYSTEM_TYPE{VIDEO_VERSION_ID_DEFAULT + 2};
constexpr int SHARED_TYPE{50000}; // user type used by versions and an extra, flagged as extras
constexpr int QUALITY_TYPE{50001}; // user type used only by versions, flagged as extras
constexpr int EXTRAS_TYPE{50002}; // user type used only by extras

//! The repair is for UpdateTables() only, the test calls it directly.
class CRepairableDatabase : public CVideoDatabase
{
public:
  void RepairAssetTypes(int fromVersion)
  {
    KODI::DATABASE::MIGRATION::RepairVideoAssetTypesV150(*this, *m_pDS, *m_pDS2, fromVersion);
  }
};

/*!
 * Upgrades from schema < 131 by Kodi 22 builds turned versions and their types into extras
 * (https://github.com/xbmc/xbmc/issues/29918). The tests damage a database the same way and
 * repair it.
 */
class TestVideoDatabaseMigration : public ::testing::Test
{
protected:
  void SetUp() override
  {
    m_settings.type = "sqlite3";
    m_settings.host = CSpecialProtocol::TranslatePath("special://temp/");
    m_name = StringUtils::Format("TestVideoDatabaseMigration{}",
                                 ::testing::UnitTest::GetInstance()->current_test_info()->name());
    XFILE::CFile::Delete(m_settings.host + m_name + ".db");
    ASSERT_EQ(CDatabase::ConnectionState::STATE_CONNECTED, m_db.Connect(m_name, m_settings, true));
    ASSERT_TRUE(m_db.ExecuteQuery("INSERT INTO path (idPath, strPath) VALUES (1, '/movies/')"));

    // The steps flagged every system type as extras
    ASSERT_TRUE(m_db.ExecuteQuery(StringUtils::Format(
        "UPDATE videoversiontype SET itemType = {} WHERE owner = {}", ASSET_EXTRA, OWNER_SYSTEM)));
  }

  void TearDown() override
  {
    m_db.Close();
    XFILE::CFile::Delete(m_settings.host + m_name + ".db");
  }

  void AddMovie(int idMovie, int idFile, int itemType, int idType)
  {
    ASSERT_TRUE(m_db.ExecuteQuery(
        StringUtils::Format("INSERT INTO movie (idMovie, idFile, c00) VALUES ({}, {}, 'Movie {}')",
                            idMovie, idFile, idMovie)));
    AddAsset(idMovie, idFile, itemType, idType);
  }

  void AddAsset(int idMovie, int idFile, int itemType, int idType)
  {
    ASSERT_TRUE(m_db.ExecuteQuery(StringUtils::Format(
        "INSERT INTO files (idFile, idPath, strFilename) VALUES ({}, 1, 'file{}.mkv')", idFile,
        idFile)));
    ASSERT_TRUE(m_db.ExecuteQuery(StringUtils::Format(
        "INSERT INTO videoversion (idFile, idMedia, media_type, itemType, idType) "
        "VALUES ({}, {}, 'movie', {}, {})",
        idFile, idMovie, itemType, idType)));
  }

  void AddUserType(int id, const std::string& name)
  {
    ASSERT_TRUE(m_db.ExecuteQuery(StringUtils::Format(
        "INSERT INTO videoversiontype (id, name, owner, itemType) VALUES ({}, '{}', {}, {})", id,
        name, OWNER_USER, ASSET_EXTRA)));
  }

  int AssetItemType(int idFile)
  {
    return m_db.GetSingleValueInt(
        StringUtils::Format("SELECT itemType FROM videoversion WHERE idFile = {}", idFile));
  }

  int AssetTypeId(int idFile)
  {
    return m_db.GetSingleValueInt(
        StringUtils::Format("SELECT idType FROM videoversion WHERE idFile = {}", idFile));
  }

  int TypeItemType(int idType)
  {
    return m_db.GetSingleValueInt(
        StringUtils::Format("SELECT itemType FROM videoversiontype WHERE id = {}", idType));
  }

  int TypeOwner(int idType)
  {
    return m_db.GetSingleValueInt(
        StringUtils::Format("SELECT owner FROM videoversiontype WHERE id = {}", idType));
  }

  //! Every asset has a type of its own item type, so movie_view lists it
  void ExpectEveryAssetMatchesItsType()
  {
    EXPECT_EQ(0, m_db.GetSingleValueInt("SELECT count(*) FROM videoversion vv "
                                        "LEFT JOIN videoversiontype vvt "
                                        "  ON vvt.id = vv.idType AND vvt.itemType = vv.itemType "
                                        "WHERE vvt.id IS NULL"));
  }

  void ExpectTheSystemTypesAreTypesOfVersions()
  {
    EXPECT_EQ(0, m_db.GetSingleValueInt(StringUtils::Format(
                     "SELECT count(*) FROM videoversiontype WHERE owner = {} AND itemType <> {}",
                     OWNER_SYSTEM, ASSET_VERSION)));
  }

  //! The assets of a database upgraded by Kodi 22 from 123 to 130, or damaged before 134
  void AddDamagedAssets()
  {
    AddUserType(SHARED_TYPE, "Shared");
    AddUserType(QUALITY_TYPE, "Quality");
    AddUserType(EXTRAS_TYPE, "My extras");

    // Upgraded from < v123: the default version became an extra
    AddMovie(1, 100, ASSET_EXTRA, VIDEO_VERSION_ID_DEFAULT);
    // Default version, alternate version and extra sharing a type
    AddMovie(2, 200, ASSET_VERSION, SHARED_TYPE);
    AddAsset(2, 201, ASSET_VERSION, SHARED_TYPE);
    AddAsset(2, 202, ASSET_EXTRA, SHARED_TYPE);
    // Alternate version on a type only versions use; extras on a system type and a user type of
    // extras
    AddMovie(3, 300, ASSET_VERSION, VIDEO_VERSION_ID_DEFAULT);
    AddAsset(3, 301, ASSET_VERSION, QUALITY_TYPE);
    AddAsset(3, 302, ASSET_EXTRA, SYSTEM_TYPE);
    AddAsset(3, 303, ASSET_EXTRA, EXTRAS_TYPE);
  }

  void ExpectTheDamagedAssetsAreRepaired()
  {
    EXPECT_EQ(3,
              m_db.GetSingleValueInt("SELECT count(*) FROM movie_view WHERE isDefaultVersion = 1"));
    ExpectEveryAssetMatchesItsType();
    ExpectTheSystemTypesAreTypesOfVersions();

    EXPECT_EQ(ASSET_VERSION, AssetItemType(100));

    // The shared type is kept by the versions; the extra gets an extras type of the same name
    EXPECT_EQ(ASSET_VERSION, TypeItemType(SHARED_TYPE));
    EXPECT_EQ(SHARED_TYPE, AssetTypeId(200));
    EXPECT_EQ(SHARED_TYPE, AssetTypeId(201));
    const int sharedExtrasType{AssetTypeId(202)};
    EXPECT_NE(SHARED_TYPE, sharedExtrasType);
    EXPECT_EQ(ASSET_EXTRA, TypeItemType(sharedExtrasType));
    EXPECT_EQ("Shared", m_db.GetSingleValue(StringUtils::Format(
                            "SELECT name FROM videoversiontype WHERE id = {}", sharedExtrasType)));

    EXPECT_EQ(ASSET_VERSION, TypeItemType(QUALITY_TYPE));
    EXPECT_EQ(QUALITY_TYPE, AssetTypeId(301));

    const int systemCopyType{AssetTypeId(302)};
    EXPECT_EQ(OWNER_USER, TypeOwner(systemCopyType));
    EXPECT_EQ(ASSET_EXTRA, TypeItemType(systemCopyType));

    EXPECT_EQ(EXTRAS_TYPE, AssetTypeId(303));
    EXPECT_EQ(ASSET_EXTRA, TypeItemType(EXTRAS_TYPE));
  }

  DatabaseSettings m_settings;
  std::string m_name;
  CRepairableDatabase m_db;
};
} // unnamed namespace

TEST_F(TestVideoDatabaseMigration, AnUpgradeFromBefore123GetsItsVersionsBack)
{
  AddMovie(1, 100, ASSET_EXTRA, VIDEO_VERSION_ID_DEFAULT);
  AddMovie(2, 200, ASSET_EXTRA, VIDEO_VERSION_ID_DEFAULT);

  m_db.RepairAssetTypes(121);

  EXPECT_EQ(2,
            m_db.GetSingleValueInt("SELECT count(*) FROM movie_view WHERE isDefaultVersion = 1"));
  EXPECT_EQ(ASSET_VERSION, AssetItemType(100));
  EXPECT_EQ(ASSET_VERSION, AssetItemType(200));
  ExpectEveryAssetMatchesItsType();
  ExpectTheSystemTypesAreTypesOfVersions();
}

TEST_F(TestVideoDatabaseMigration, AnUpgradeFrom123To130IsRepaired)
{
  AddDamagedAssets();

  m_db.RepairAssetTypes(125);

  ExpectTheDamagedAssetsAreRepaired();
}

TEST_F(TestVideoDatabaseMigration, DamageFromAnEarlierUpgradeIsRepaired)
{
  AddDamagedAssets();

  m_db.RepairAssetTypes(149);

  ExpectTheDamagedAssetsAreRepaired();
}

TEST_F(TestVideoDatabaseMigration, AnUpgradeFrom131To133IsLeftAlone)
{
  AddDamagedAssets();

  m_db.RepairAssetTypes(132);

  EXPECT_EQ(ASSET_EXTRA, AssetItemType(100));
  EXPECT_EQ(ASSET_EXTRA, TypeItemType(VIDEO_VERSION_ID_DEFAULT));
  EXPECT_EQ(SHARED_TYPE, AssetTypeId(202));
}
