/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "addons/Scraper.h"
#include "addons/addoninfo/AddonInfoBuilder.h"
#include "addons/addoninfo/AddonType.h"
#include "filesystem/SpecialProtocol.h"
#include "music/MusicDatabase.h"
#include "settings/AdvancedSettings.h"

#include <memory>
#include <string>

#include <gtest/gtest.h>

namespace
{
ADDON::ScraperPtr MakeScraper(const std::string& id, ADDON::AddonType type)
{
  const ADDON::AddonInfoPtr info = ADDON::CAddonInfoBuilder::Generate(id, type);
  return std::make_shared<ADDON::CScraper>(info, type);
}
} // unnamed namespace

class TestMusicDatabase : public ::testing::Test
{
protected:
  // Each test gets its own database file: a test that dies mid-transaction leaks a write lock.
  void Connect(const std::string& name)
  {
    DatabaseSettings settings;
    settings.type = "sqlite3";
    settings.name = name;
    settings.host = CSpecialProtocol::TranslatePath("special://temp/");

    ASSERT_EQ(CDatabase::ConnectionState::STATE_CONNECTED, database.Connect(name, settings, true));
  }

  void TearDown() override { database.Close(); }

  CMusicDatabase database;
};

TEST_F(TestMusicDatabase, ClearsAnArtistInfoProviderOverride)
{
  Connect("music_scraper_artist_clear");

  const int idArtist = database.AddArtist("Test Artist", "", "");
  ASSERT_GT(idArtist, 0);

  const ADDON::ScraperPtr scraper =
      MakeScraper("metadata.test.artists", ADDON::AddonType::SCRAPER_ARTISTS);
  ASSERT_TRUE(database.SetScraper(idArtist, ADDON::ContentType::ARTISTS, scraper));
  ASSERT_TRUE(database.ScraperInUse("metadata.test.artists"));

  EXPECT_TRUE(database.SetScraper(idArtist, ADDON::ContentType::ARTISTS, nullptr));
  EXPECT_FALSE(database.ScraperInUse("metadata.test.artists"));
}

TEST_F(TestMusicDatabase, ClearsAnAlbumInfoProviderOverride)
{
  Connect("music_scraper_album_clear");

  const int idAlbum = database.AddAlbum("Test Album", "", "", "Test Artist", "", "", "", "", false,
                                        "", "", "", false, AudioType::Type::Album);
  ASSERT_GT(idAlbum, 0);

  const ADDON::ScraperPtr scraper =
      MakeScraper("metadata.test.albums", ADDON::AddonType::SCRAPER_ALBUMS);
  ASSERT_TRUE(database.SetScraper(idAlbum, ADDON::ContentType::ALBUMS, scraper));
  ASSERT_TRUE(database.ScraperInUse("metadata.test.albums"));

  EXPECT_TRUE(database.SetScraper(idAlbum, ADDON::ContentType::ALBUMS, nullptr));
  EXPECT_FALSE(database.ScraperInUse("metadata.test.albums"));
}

TEST_F(TestMusicDatabase, ClearingWithoutAnOverrideLeavesTheItemOnTheDefault)
{
  Connect("music_scraper_noop_clear");

  const int idArtist = database.AddArtist("Test Artist", "", "");
  ASSERT_GT(idArtist, 0);

  EXPECT_TRUE(database.SetScraper(idArtist, ADDON::ContentType::ARTISTS, nullptr));
  EXPECT_FALSE(database.ScraperInUse("metadata.test.artists"));
}
