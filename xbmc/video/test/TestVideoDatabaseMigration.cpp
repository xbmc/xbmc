/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "dbwrappers/test/DatabaseTestUtils.h"
#include "filesystem/File.h"
#include "utils/StringUtils.h"
#include "video/VideoDatabase.h"
#include "video/VideoManagerTypes.h"

#include <string>

#include <gtest/gtest.h>

namespace
{
constexpr int ASSET_VERSION{1};
constexpr int ASSET_EXTRA{2};
constexpr int OWNER_SYSTEM{0};
constexpr int OWNER_USER{2};

constexpr int SHARED_TYPE{50000}; // user type used by versions and an extra, flagged as extras
constexpr int QUALITY_TYPE{50001}; // user type used only by versions, flagged as extras
constexpr int EXTRAS_TYPE{50002}; // user type used only by extras

void AddMovie(CDatabase& db, int idMovie, int idFile, int itemType, int idType)
{
  ASSERT_TRUE(db.ExecuteQuery(StringUtils::Format(
      "INSERT INTO files (idFile, idPath, strFilename) VALUES ({}, 1, 'movie{}.mkv')", idFile,
      idMovie)));
  ASSERT_TRUE(db.ExecuteQuery(
      StringUtils::Format("INSERT INTO movie (idMovie, idFile, c00) VALUES ({}, {}, 'Movie {}')",
                          idMovie, idFile, idMovie)));
  ASSERT_TRUE(db.ExecuteQuery(StringUtils::Format(
      "INSERT INTO videoversion (idFile, idMedia, media_type, itemType, idType) "
      "VALUES ({}, {}, 'movie', {}, {})",
      idFile, idMovie, itemType, idType)));
}

void AddAsset(CDatabase& db, int idMovie, int idFile, int itemType, int idType)
{
  ASSERT_TRUE(db.ExecuteQuery(StringUtils::Format(
      "INSERT INTO files (idFile, idPath, strFilename) VALUES ({}, 1, 'asset{}.mkv')", idFile,
      idFile)));
  ASSERT_TRUE(db.ExecuteQuery(StringUtils::Format(
      "INSERT INTO videoversion (idFile, idMedia, media_type, itemType, idType) "
      "VALUES ({}, {}, 'movie', {}, {})",
      idFile, idMovie, itemType, idType)));
}

int AssetItemType(CDatabase& db, int idFile)
{
  return db.GetSingleValueInt(
      StringUtils::Format("SELECT itemType FROM videoversion WHERE idFile = {}", idFile));
}

int AssetTypeId(CDatabase& db, int idFile)
{
  return db.GetSingleValueInt(
      StringUtils::Format("SELECT idType FROM videoversion WHERE idFile = {}", idFile));
}

int TypeItemType(CDatabase& db, int idType)
{
  return db.GetSingleValueInt(
      StringUtils::Format("SELECT itemType FROM videoversiontype WHERE id = {}", idType));
}
} // unnamed namespace

/*!
 * Upgrades from schema < 131 by Kodi 22 builds turned versions and their types into extras
 * (https://github.com/xbmc/xbmc/issues/29918). The repair has to restore them when such a database
 * is upgraded again.
 */
TEST(TestVideoDatabaseMigration, VersionsTurnedIntoExtrasAreRepaired)
{
  constexpr const char* BASE_NAME{"MyVideosAssetTypes"};
  constexpr int PREVIOUS{151};

  const DatabaseSettings settings{TestDatabaseSettings()};
  const std::string oldName{StringUtils::Format("{}{}", BASE_NAME, PREVIOUS)};
  XFILE::CFile::Delete(settings.host + oldName + ".db");

  int current{0};
  {
    CVideoDatabase old;
    ASSERT_EQ(CDatabase::ConnectionState::STATE_CONNECTED, old.Connect(oldName, settings, true));
    current = old.GetSingleValueInt("SELECT idVersion FROM version");
    old.DropAnalytics();

    ASSERT_TRUE(old.ExecuteQuery("INSERT INTO path (idPath, strPath) VALUES (1, '/movies/')"));
    ASSERT_TRUE(old.ExecuteQuery(StringUtils::Format(
        "UPDATE videoversiontype SET itemType = {} WHERE owner = {}", ASSET_EXTRA, OWNER_SYSTEM)));
    ASSERT_TRUE(old.ExecuteQuery(StringUtils::Format(
        "INSERT INTO videoversiontype (id, name, owner, itemType) VALUES "
        "({}, 'Shared', {}, {}), ({}, 'Quality', {}, {}), ({}, 'My extras', {}, {})",
        SHARED_TYPE, OWNER_USER, ASSET_EXTRA, QUALITY_TYPE, OWNER_USER, ASSET_EXTRA, EXTRAS_TYPE,
        OWNER_USER, ASSET_EXTRA)));

    // Upgraded from < v123: the default version became an extra
    AddMovie(old, 1, 100, ASSET_EXTRA, VIDEO_VERSION_ID_DEFAULT);
    // Default version, alternate version and extra sharing a type
    AddMovie(old, 2, 200, ASSET_VERSION, SHARED_TYPE);
    AddAsset(old, 2, 201, ASSET_VERSION, SHARED_TYPE);
    AddAsset(old, 2, 202, ASSET_EXTRA, SHARED_TYPE);
    // Alternate version on a type only versions use; extras on a system type and a user type
    AddMovie(old, 3, 300, ASSET_VERSION, VIDEO_VERSION_ID_DEFAULT);
    AddAsset(old, 3, 301, ASSET_VERSION, QUALITY_TYPE);
    AddAsset(old, 3, 302, ASSET_EXTRA, VIDEO_VERSION_ID_DEFAULT + 2);
    AddAsset(old, 3, 303, ASSET_EXTRA, EXTRAS_TYPE);

    ASSERT_TRUE(
        old.ExecuteQuery(StringUtils::Format("UPDATE version SET idVersion = {}", PREVIOUS)));
    old.Close();
  }

  const std::string newName{StringUtils::Format("{}{}", BASE_NAME, current)};
  XFILE::CFile::Delete(settings.host + newName + ".db");
  ASSERT_TRUE(UpgradeThroughManager(&CAdvancedSettings::m_databaseVideo, BASE_NAME));

  CVideoDatabase db;
  ASSERT_EQ(CDatabase::ConnectionState::STATE_CONNECTED, db.Connect(newName, settings, false));
  EXPECT_EQ(current, db.GetSingleValueInt("SELECT idVersion FROM version"));

  // Every movie is listed with its default version, and no asset is hidden by a type mismatch
  EXPECT_EQ(3, db.GetSingleValueInt("SELECT count(*) FROM movie_view WHERE isDefaultVersion = 1"));
  EXPECT_EQ(0, db.GetSingleValueInt("SELECT count(*) FROM videoversion vv "
                                    "LEFT JOIN videoversiontype vvt "
                                    "  ON vvt.id = vv.idType AND vvt.itemType = vv.itemType "
                                    "WHERE vvt.id IS NULL"));
  EXPECT_EQ(0, db.GetSingleValueInt(StringUtils::Format(
                   "SELECT count(*) FROM videoversiontype WHERE owner = {} AND itemType <> {}",
                   OWNER_SYSTEM, ASSET_VERSION)));

  EXPECT_EQ(ASSET_VERSION, AssetItemType(db, 100));

  // The shared type is kept by the versions; the extra gets an extras type of the same name
  EXPECT_EQ(ASSET_VERSION, TypeItemType(db, SHARED_TYPE));
  EXPECT_EQ(SHARED_TYPE, AssetTypeId(db, 200));
  EXPECT_EQ(SHARED_TYPE, AssetTypeId(db, 201));
  const int sharedExtrasType{AssetTypeId(db, 202)};
  EXPECT_NE(SHARED_TYPE, sharedExtrasType);
  EXPECT_EQ(ASSET_EXTRA, TypeItemType(db, sharedExtrasType));
  EXPECT_EQ("Shared", db.GetSingleValue(StringUtils::Format(
                          "SELECT name FROM videoversiontype WHERE id = {}", sharedExtrasType)));

  EXPECT_EQ(ASSET_VERSION, TypeItemType(db, QUALITY_TYPE));
  EXPECT_EQ(QUALITY_TYPE, AssetTypeId(db, 301));

  const int systemExtrasType{AssetTypeId(db, 302)};
  EXPECT_EQ(OWNER_USER, db.GetSingleValueInt(StringUtils::Format(
                            "SELECT owner FROM videoversiontype WHERE id = {}", systemExtrasType)));
  EXPECT_EQ(ASSET_EXTRA, TypeItemType(db, systemExtrasType));

  EXPECT_EQ(EXTRAS_TYPE, AssetTypeId(db, 303));
  EXPECT_EQ(ASSET_EXTRA, TypeItemType(db, EXTRAS_TYPE));

  db.Close();
  XFILE::CFile::Delete(settings.host + newName + ".db");
  XFILE::CFile::Delete(settings.host + oldName + ".db");
}
