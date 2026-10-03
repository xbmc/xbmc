/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DatabaseManager.h"
#include "GUIInfoManager.h"
#include "ServiceBroker.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "interfaces/AnnouncementManager.h"
#include "interfaces/json-rpc/AudioLibrary.h"
#include "music/MusicDatabase.h"
#include "utils/Variant.h"

#include <array>
#include <memory>
#include <string>

#include <gtest/gtest.h>

namespace
{
class TestGUI : public CGUIComponent
{
public:
  TestGUI() : CGUIComponent(false)
  {
    m_pWindowManager = std::make_unique<CGUIWindowManager>();
    m_guiInfoManager = std::make_unique<CGUIInfoManager>();
    CServiceBroker::RegisterGUI(this);
  }

  ~TestGUI() override { m_pWindowManager.reset(); }
};

class TestAudioLibrary : public testing::TestWithParam<bool>
{
protected:
  void SetUp() override
  {
    m_previousAnnouncements = CServiceBroker::GetAnnouncementManager();
    CServiceBroker::RegisterAnnouncementManager(
        std::make_shared<ANNOUNCEMENT::CAnnouncementManager>());
    if (!CServiceBroker::GetDatabaseManager().CanOpen("MyMusic"))
    {
      ASSERT_TRUE(CServiceBroker::GetDatabaseManager().Initialize());
    }
    ASSERT_TRUE(m_db.Open());
    m_lastScanned = m_db.GetLibraryLastUpdated();
    m_lastScannedIsNull =
        m_db.GetSingleValue("SELECT lastscanned IS NULL FROM versiontagscan LIMIT 1") == "1";
    m_artistId = m_db.AddArtist("SetArtistDetails test", "");
    ASSERT_GT(m_artistId, 0);
    ASSERT_TRUE(m_db.ExecuteQuery("INSERT INTO album (strAlbum) VALUES ('Test album')"));
    m_albumId = std::stoi(m_db.GetSingleValue("SELECT MAX(idAlbum) FROM album"));
    ASSERT_TRUE(m_db.ExecuteQuery(m_db.PrepareSQL(
        "INSERT INTO album_artist (idArtist, idAlbum) VALUES (%i, %i)", m_artistId, m_albumId)));
  }

  void TearDown() override
  {
    m_db.ExecuteQuery(m_db.PrepareSQL("DELETE FROM album WHERE idAlbum = %i", m_albumId));
    EXPECT_TRUE(m_db.ExecuteQuery(m_db.PrepareSQL(
        "DELETE FROM removed_link WHERE idArtist = %i AND idMedia = %i AND idRole = -1", m_artistId,
        m_albumId)));
    m_db.ExecuteQuery(m_db.PrepareSQL("DELETE FROM artist WHERE idArtist = %i", m_artistId));
    EXPECT_TRUE(m_db.ExecuteQuery(
        m_lastScannedIsNull ? "UPDATE versiontagscan SET lastscanned = NULL"
                            : m_db.PrepareSQL("UPDATE versiontagscan SET lastscanned = '%s'",
                                              m_lastScanned.c_str())));
    m_db.Close();
    CServiceBroker::RegisterAnnouncementManager(m_previousAnnouncements);
  }

  TestGUI m_gui;
  CMusicDatabase m_db;
  int m_artistId{-1};
  int m_albumId{-1};
  std::string m_lastScanned;
  bool m_lastScannedIsNull{true};
  std::shared_ptr<ANNOUNCEMENT::CAnnouncementManager> m_previousAnnouncements;
};
} // namespace

TEST_P(TestAudioLibrary, SetArtistDetailsPreservesDiscographyAndVideoLinks)
{
  struct Song
  {
    const char* title;
    const char* mbid;
    const char* video;
  };
  const std::array songs{
      Song{"One", "", "https://example.com/shared"},
      Song{"Someone", "", ""},
      Song{"Another song", "", "https://example.com/shared"},
      Song{"One", "", "https://example.com/shared"},
      Song{"One", "", ""},
      Song{"MBID song", "recording-id", "https://example.com/mbid"},
      Song{"MBID song", "recording-id", ""},
  };
  for (size_t i = 0; i < songs.size(); ++i)
  {
    const auto& song = songs[i];
    ASSERT_TRUE(m_db.ExecuteQuery(m_db.PrepareSQL(
        "INSERT INTO song (idAlbum, iTrack, strTitle, strMusicBrainzTrackID, strVideoURL) "
        "VALUES (%i, %i, '%s', NULLIF('%s', ''), NULLIF('%s', ''))",
        m_albumId, static_cast<int>(i), song.title, song.mbid, song.video)));
    if (*song.video)
    {
      ASSERT_TRUE(m_db.ExecuteQuery(
          m_db.PrepareSQL("INSERT INTO art (media_id, media_type, type, url) "
                          "SELECT idSong, 'song', 'videothumb', 'thumb%i' FROM song "
                          "WHERE idAlbum = %i AND iTrack = %i",
                          static_cast<int>(i), m_albumId, static_cast<int>(i))));
    }
  }
  if (GetParam())
  {
    ASSERT_TRUE(m_db.ExecuteQuery(m_db.PrepareSQL(
        "INSERT INTO discography VALUES (%i, 'Scraped album', '2026', 'release-id')", m_artistId)));
  }
  m_db.SetArtForItem(m_artistId, "artist", "fanart", "existing-fanart");
  m_db.SetArtForItem(m_artistId, "artist", "thumb", "old-thumb");
  m_db.SetArtForItem(m_artistId, "artist", "banner", "old-banner");
  ASSERT_TRUE(m_db.ExecuteQuery("UPDATE versiontagscan SET lastscanned = '2000-01-01 00:00:00'"));

  CVariant params(CVariant::VariantTypeObject);
  params["artistid"] = m_artistId;
  params["description"] = "Updated biography";
  params["art"]["thumb"] = "new-thumb";
  params["art"]["banner"] = CVariant(CVariant::VariantTypeNull);
  CVariant result;
  ASSERT_EQ(JSONRPC::ACK, JSONRPC::CAudioLibrary::SetArtistDetails(
                              "AudioLibrary.SetArtistDetails", nullptr, nullptr, params, result));

  EXPECT_EQ("Updated biography",
            m_db.GetSingleValue(m_db.PrepareSQL(
                "SELECT strBiography FROM artist WHERE idArtist = %i", m_artistId)));
  EXPECT_EQ(GetParam() ? "1" : "0",
            m_db.GetSingleValue(m_db.PrepareSQL(
                "SELECT COUNT(*) FROM discography WHERE idArtist = %i", m_artistId)));
  if (GetParam())
  {
    const std::string where = m_db.PrepareSQL("idArtist = %i", m_artistId);
    EXPECT_EQ("Scraped album", m_db.GetSingleValue("discography", "strAlbum", where));
    EXPECT_EQ("2026", m_db.GetSingleValue("discography", "strYear", where));
    EXPECT_EQ("release-id", m_db.GetSingleValue("discography", "strReleaseGroupMBID", where));
  }
  for (size_t i = 0; i < songs.size(); ++i)
  {
    SCOPED_TRACE(i);
    EXPECT_EQ(songs[i].video, m_db.GetSingleValue(m_db.PrepareSQL(
                                  "SELECT strVideoURL FROM song WHERE idAlbum = %i AND iTrack = %i",
                                  m_albumId, static_cast<int>(i))));
    EXPECT_EQ(
        *songs[i].video ? "thumb" + std::to_string(i) : "",
        m_db.GetSingleValue(m_db.PrepareSQL(
            "SELECT url FROM art JOIN song ON media_id = idSong "
            "WHERE media_type = 'song' AND type = 'videothumb' AND idAlbum = %i AND iTrack = %i",
            m_albumId, static_cast<int>(i))));
  }
  EXPECT_EQ("existing-fanart", m_db.GetArtForItem(m_artistId, "artist", "fanart"));
  EXPECT_EQ("new-thumb", m_db.GetArtForItem(m_artistId, "artist", "thumb"));
  EXPECT_TRUE(m_db.GetArtForItem(m_artistId, "artist", "banner").empty());
  EXPECT_NE("2000-01-01 00:00:00", m_db.GetLibraryLastUpdated());
}

INSTANTIATE_TEST_SUITE_P(WithAndWithoutDiscography, TestAudioLibrary, testing::Bool());
