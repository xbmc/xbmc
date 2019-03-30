/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "MusicDatabase.h"

#include "addons/Addon.h"
#include "addons/AddonManager.h"
#include "addons/AddonSystemSettings.h"
#include "addons/Scraper.h"
#include "Album.h"
#include "Application.h"
#include "ServiceBroker.h"
#include "Artist.h"
#include "dbwrappers/dataset.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "dialogs/GUIDialogProgress.h"
#include "dialogs/GUIDialogSelect.h"
#include "events/EventLog.h"
#include "events/NotificationEvent.h"
#include "FileItem.h"
#include "filesystem/Directory.h"
#include "filesystem/DirectoryCache.h"
#include "filesystem/File.h"
#include "filesystem/MusicDatabaseDirectory/DirectoryNode.h"
#include "guilib/GUIComponent.h"
#include "guilib/guiinfo/GUIInfoLabels.h"
#include "GUIInfoManager.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "interfaces/AnnouncementManager.h"
#include "LangInfo.h"
#include "messaging/helpers/DialogHelper.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "music/tags/MusicInfoTag.h"
#include "network/cddb.h"
#include "network/Network.h"
#include "playlists/SmartPlayList.h"
#include "profiles/ProfileManager.h"
#include "settings/AdvancedSettings.h"
#include "settings/MediaSourceSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "Song.h"
#include "storage/MediaManager.h"
#include "TextureCache.h"
#include "threads/SystemClock.h"
#include "URL.h"
#include "Util.h"
#include "utils/FileUtils.h"
#include "utils/LegacyPathTranslation.h"
#include "utils/log.h"
#include "utils/MathUtils.h"
#include "utils/Random.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/XMLUtils.h"
#include <inttypes.h>

using namespace XFILE;
using namespace MUSICDATABASEDIRECTORY;
using namespace KODI::MESSAGING;
using namespace MUSIC_INFO;

using ADDON::AddonPtr;
using KODI::MESSAGING::HELPERS::DialogResponse;

#define RECENTLY_PLAYED_LIMIT 25
#define MIN_FULL_SEARCH_LENGTH 3

#ifdef HAS_DVD_DRIVE
using namespace CDDB;
using namespace MEDIA_DETECT;
#endif

static void AnnounceRemove(const std::string& content, int id)
{
  CVariant data;
  data["type"] = content;
  data["id"] = id;
  if (g_application.IsMusicScanning())
    data["transaction"] = true;
  CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::AudioLibrary, "xbmc", "OnRemove", data);
}

static void AnnounceUpdate(const std::string& content, int id, bool added = false)
{
  CVariant data;
  data["type"] = content;
  data["id"] = id;
  if (g_application.IsMusicScanning())
    data["transaction"] = true;
  if (added)
    data["added"] = true;
  CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::AudioLibrary, "xbmc", "OnUpdate", data);
}

CMusicDatabase::CMusicDatabase(void)
{
  m_translateBlankArtist = true;
}

CMusicDatabase::~CMusicDatabase(void)
{
  EmptyCache();
}

bool CMusicDatabase::Open()
{
  return CDatabase::Open(CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_databaseMusic);
}

void CMusicDatabase::CreateTables()
{
  CLog::Log(LOGINFO, "create artist table");
  m_pDS->exec("CREATE TABLE artist ( idArtist integer primary key, "
              " strArtist varchar(256), strMusicBrainzArtistID text, "
              " strSortName text, "
              " strType text, strGender text, strDisambiguation text, "
              " strBorn text, strFormed text, strGenres text, strMoods text, "
              " strStyles text, strInstruments text, strBiography text, "
              " strDied text, strDisbanded text, strYearsActive text, "
              " strImage text, strFanart text, "
              " lastScraped varchar(20) default NULL, "
              " bScrapedMBID INTEGER NOT NULL DEFAULT 0, "
              " idInfoSetting INTEGER NOT NULL DEFAULT 0)");
  // Create missing artist tag artist [Missing].
  std::string strSQL = PrepareSQL("INSERT INTO artist (idArtist, strArtist, strSortName, strMusicBrainzArtistID) "
     "VALUES( %i, '%s', '%s', '%s' )",
    BLANKARTIST_ID, BLANKARTIST_NAME.c_str(),
    BLANKARTIST_NAME.c_str(), BLANKARTIST_FAKEMUSICBRAINZID.c_str());
  m_pDS->exec(strSQL);

  CLog::Log(LOGINFO, "create album table");
  m_pDS->exec("CREATE TABLE album (idAlbum integer primary key, "
              " strAlbum varchar(256), strMusicBrainzAlbumID text, "
              " strReleaseGroupMBID text, "
              " strArtistDisp text, strArtistSort text, strGenres text, "
              " iYear integer, "
              " bCompilation integer not null default '0', "
              " strMoods text, strStyles text, strThemes text, "
              " strReview text, strImage text, strLabel text, "
              " strType text, "
              " fRating FLOAT NOT NULL DEFAULT 0, "
              " iVotes INTEGER NOT NULL DEFAULT 0, "
              " iUserrating INTEGER NOT NULL DEFAULT 0, "
              " lastScraped varchar(20) default NULL, "
              " bScrapedMBID INTEGER NOT NULL DEFAULT 0, "
              " strReleaseType text, "
              " idInfoSetting INTEGER NOT NULL DEFAULT 0)");

  CLog::Log(LOGINFO, "create audiobook table");
  m_pDS->exec("CREATE TABLE audiobook (idBook integer primary key, "
              " strBook varchar(256), strAuthor text,"
              " bookmark integer, file text,"
              " dateAdded varchar (20) default NULL)");

  CLog::Log(LOGINFO, "create album_artist table");
  m_pDS->exec("CREATE TABLE album_artist (idArtist integer, idAlbum integer, iOrder integer, strArtist text)");

  CLog::Log(LOGINFO, "create album_source table");
  m_pDS->exec("CREATE TABLE album_source (idSource INTEGER, idAlbum INTEGER)");

  CLog::Log(LOGINFO, "create genre table");
  m_pDS->exec("CREATE TABLE genre (idGenre integer primary key, strGenre varchar(256))");

  CLog::Log(LOGINFO, "create path table");
  m_pDS->exec("CREATE TABLE path (idPath integer primary key, strPath varchar(512), strHash text)");

  CLog::Log(LOGINFO, "create source table");
  m_pDS->exec("CREATE TABLE source (idSource INTEGER PRIMARY KEY, strName TEXT, strMultipath TEXT)");

  CLog::Log(LOGINFO, "create source_path table");
  m_pDS->exec("CREATE TABLE source_path (idSource INTEGER, idPath INTEGER, strPath varchar(512))");

  CLog::Log(LOGINFO, "create song table");
  m_pDS->exec("CREATE TABLE song (idSong integer primary key, "
              " idAlbum integer, idPath integer, "
              " strArtistDisp text, strArtistSort text, strGenres text, strTitle varchar(512), "
              " iTrack integer, iDuration integer, iYear integer, "
              " strFileName text, strMusicBrainzTrackID text, "
              " iTimesPlayed integer, iStartOffset integer, iEndOffset integer, "
              " lastplayed varchar(20) default NULL, "
              " rating FLOAT NOT NULL DEFAULT 0, votes INTEGER NOT NULL DEFAULT 0, "
              " userrating INTEGER NOT NULL DEFAULT 0, "
              " comment text, mood text, strReplayGain text, dateAdded text)");
  CLog::Log(LOGINFO, "create song_artist table");
  m_pDS->exec("CREATE TABLE song_artist (idArtist integer, idSong integer, idRole integer, iOrder integer, strArtist text)");
  CLog::Log(LOGINFO, "create song_genre table");
  m_pDS->exec("CREATE TABLE song_genre (idGenre integer, idSong integer, iOrder integer)");

  CLog::Log(LOGINFO, "create role table");
  m_pDS->exec("CREATE TABLE role (idRole integer primary key, strRole text)");
  m_pDS->exec("INSERT INTO role(idRole, strRole) VALUES (1, 'Artist')");   //Default role

  CLog::Log(LOGINFO, "create infosetting table");
  m_pDS->exec("CREATE TABLE infosetting (idSetting INTEGER PRIMARY KEY, strScraperPath TEXT, strSettings TEXT)");

  CLog::Log(LOGINFO, "create discography table");
  m_pDS->exec("CREATE TABLE discography (idArtist integer, strAlbum text, strYear text)");

  CLog::Log(LOGINFO, "create art table");
  m_pDS->exec("CREATE TABLE art(art_id INTEGER PRIMARY KEY, media_id INTEGER, media_type TEXT, type TEXT, url TEXT)");

  CLog::Log(LOGINFO, "create versiontagscan table");
  m_pDS->exec("CREATE TABLE versiontagscan (idVersion INTEGER, iNeedsScan INTEGER, lastscanned VARCHAR(20))");
  m_pDS->exec(PrepareSQL("INSERT INTO versiontagscan (idVersion, iNeedsScan) values(%i, 0)", GetSchemaVersion()));
}

void CMusicDatabase::CreateAnalytics()
{
  CLog::Log(LOGINFO, "%s - creating indices", __FUNCTION__);
  m_pDS->exec("CREATE INDEX idxAlbum ON album(strAlbum(255))");
  m_pDS->exec("CREATE INDEX idxAlbum_1 ON album(bCompilation)");
  m_pDS->exec("CREATE UNIQUE INDEX idxAlbum_2 ON album(strMusicBrainzAlbumID(36))");
  m_pDS->exec("CREATE INDEX idxAlbum_3 ON album(idInfoSetting)");

  m_pDS->exec("CREATE UNIQUE INDEX idxAlbumArtist_1 ON album_artist ( idAlbum, idArtist )");
  m_pDS->exec("CREATE UNIQUE INDEX idxAlbumArtist_2 ON album_artist ( idArtist, idAlbum )");

  m_pDS->exec("CREATE INDEX idxGenre ON genre(strGenre(255))");

  m_pDS->exec("CREATE INDEX idxArtist ON artist(strArtist(255))");
  m_pDS->exec("CREATE UNIQUE INDEX idxArtist1 ON artist(strMusicBrainzArtistID(36))");
  m_pDS->exec("CREATE INDEX idxArtist_2 ON artist(idInfoSetting)");

  m_pDS->exec("CREATE INDEX idxPath ON path(strPath(255))");

  m_pDS->exec("CREATE INDEX idxSource_1 ON source(strName(255))");
  m_pDS->exec("CREATE INDEX idxSource_2 ON source(strMultipath(255))");

  m_pDS->exec("CREATE UNIQUE INDEX idxSourcePath_1 ON source_path ( idSource, idPath)");

  m_pDS->exec("CREATE UNIQUE INDEX idxAlbumSource_1 ON album_source ( idSource, idAlbum )");
  m_pDS->exec("CREATE UNIQUE INDEX idxAlbumSource_2 ON album_source ( idAlbum, idSource )");

  m_pDS->exec("CREATE INDEX idxSong ON song(strTitle(255))");
  m_pDS->exec("CREATE INDEX idxSong1 ON song(iTimesPlayed)");
  m_pDS->exec("CREATE INDEX idxSong2 ON song(lastplayed)");
  m_pDS->exec("CREATE INDEX idxSong3 ON song(idAlbum)");
  m_pDS->exec("CREATE INDEX idxSong6 ON song( idPath, strFileName(255) )");
  //Musicbrainz Track ID is not unique on an album, recordings are sometimes repeated e.g. "[silence]" or on a disc set
  m_pDS->exec("CREATE UNIQUE INDEX idxSong7 ON song( idAlbum, iTrack, strMusicBrainzTrackID(36) )");

  m_pDS->exec("CREATE UNIQUE INDEX idxSongArtist_1 ON song_artist ( idSong, idArtist, idRole )");
  m_pDS->exec("CREATE INDEX idxSongArtist_2 ON song_artist ( idSong, idRole )");
  m_pDS->exec("CREATE INDEX idxSongArtist_3 ON song_artist ( idArtist, idRole )");
  m_pDS->exec("CREATE INDEX idxSongArtist_4 ON song_artist ( idRole )");

  m_pDS->exec("CREATE UNIQUE INDEX idxSongGenre_1 ON song_genre ( idSong, idGenre )");
  m_pDS->exec("CREATE UNIQUE INDEX idxSongGenre_2 ON song_genre ( idGenre, idSong )");

  m_pDS->exec("CREATE INDEX idxRole on role(strRole(255))");

  m_pDS->exec("CREATE INDEX idxDiscography_1 ON discography ( idArtist )");

  m_pDS->exec("CREATE INDEX ix_art ON art(media_id, media_type(20), type(20))");

  CLog::Log(LOGINFO, "create triggers");
  m_pDS->exec("CREATE TRIGGER tgrDeleteAlbum AFTER delete ON album FOR EACH ROW BEGIN"
              "  DELETE FROM song WHERE song.idAlbum = old.idAlbum;"
              "  DELETE FROM album_artist WHERE album_artist.idAlbum = old.idAlbum;"
              "  DELETE FROM album_source WHERE album_source.idAlbum = old.idAlbum;"
              "  DELETE FROM art WHERE media_id=old.idAlbum AND media_type='album';"
              " END");
  m_pDS->exec("CREATE TRIGGER tgrDeleteArtist AFTER delete ON artist FOR EACH ROW BEGIN"
              "  DELETE FROM album_artist WHERE album_artist.idArtist = old.idArtist;"
              "  DELETE FROM song_artist WHERE song_artist.idArtist = old.idArtist;"
              "  DELETE FROM discography WHERE discography.idArtist = old.idArtist;"
              "  DELETE FROM art WHERE media_id=old.idArtist AND media_type='artist';"
              " END");
  m_pDS->exec("CREATE TRIGGER tgrDeleteSong AFTER delete ON song FOR EACH ROW BEGIN"
              "  DELETE FROM song_artist WHERE song_artist.idSong = old.idSong;"
              "  DELETE FROM song_genre WHERE song_genre.idSong = old.idSong;"
              "  DELETE FROM art WHERE media_id=old.idSong AND media_type='song';"
              " END");
  m_pDS->exec("CREATE TRIGGER tgrDeleteSource AFTER delete ON source FOR EACH ROW BEGIN"
              "  DELETE FROM source_path WHERE source_path.idSource = old.idSource;"
              "  DELETE FROM album_source WHERE album_source.idSource = old.idSource;"
              " END");
  
  // we create views last to ensure all indexes are rolled in
  CreateViews();

}

void CMusicDatabase::CreateViews()
{
  CLog::Log(LOGINFO, "create song view");
  m_pDS->exec("CREATE VIEW songview AS SELECT "
              "        song.idSong AS idSong, "
              "        song.strArtistDisp AS strArtists,"
              "        song.strArtistSort AS strArtistSort,"
              "        song.strGenres AS strGenres,"
              "        strTitle, "
              "        iTrack, iDuration, "
              "        song.iYear AS iYear, "
              "        strFileName, "
              "        strMusicBrainzTrackID, "
              "        iTimesPlayed, iStartOffset, iEndOffset, "
              "        lastplayed, "
              "        song.rating, "
              "        song.userrating, "
              "        song.votes, "
              "        comment, "
              "        song.idAlbum AS idAlbum, "
              "        strAlbum, "
              "        strPath, "
              "        album.bCompilation AS bCompilation,"
              "        album.strArtistDisp AS strAlbumArtists,"
              "        album.strArtistSort AS strAlbumArtistSort,"
              "        album.strReleaseType AS strAlbumReleaseType,"
              "        song.mood as mood,"
              "        song.dateAdded as dateAdded, "
              "        song.strReplayGain "
              "FROM song"
              "  JOIN album ON"
              "    song.idAlbum=album.idAlbum"
              "  JOIN path ON"
              "    song.idPath=path.idPath");

  CLog::Log(LOGINFO, "create album view");
  m_pDS->exec("CREATE VIEW albumview AS SELECT "
              "        album.idAlbum AS idAlbum, "
              "        strAlbum, "
              "        strMusicBrainzAlbumID, "
              "        strReleaseGroupMBID, "
              "        album.strArtistDisp AS strArtists, "
              "        album.strArtistSort AS strArtistSort, "
              "        album.strGenres AS strGenres, "
              "        album.iYear AS iYear, "
              "        album.strMoods AS strMoods, "
              "        album.strStyles AS strStyles, "
              "        strThemes, "
              "        strReview, "
              "        strLabel, "
              "        strType, "
              "        album.strImage as strImage, "
              "        album.fRating, "
              "        album.iUserrating, "
              "        album.iVotes, "
              "        bCompilation, "
              "        bScrapedMBID,"
              "        lastScraped,"
              "        (SELECT ROUND(AVG(song.iTimesPlayed)) FROM song WHERE song.idAlbum = album.idAlbum) AS iTimesPlayed, "
              "        strReleaseType, "
              "        (SELECT MAX(song.dateAdded) FROM song WHERE song.idAlbum = album.idAlbum) AS dateAdded, "
              "        (SELECT MAX(song.lastplayed) FROM song WHERE song.idAlbum = album.idAlbum) AS lastplayed "
              "FROM album"
              );

  CLog::Log(LOGINFO, "create artist view");
  m_pDS->exec("CREATE VIEW artistview AS SELECT"
              "  idArtist, strArtist, strSortName, "
              "  strMusicBrainzArtistID, "
              "  strType, strGender, strDisambiguation, "
              "  strBorn, strFormed, strGenres,"
              "  strMoods, strStyles, strInstruments, "
              "  strBiography, strDied, strDisbanded, "
              "  strYearsActive, strImage, strFanart, "
              "  bScrapedMBID, lastScraped, "
              "  (SELECT MAX(song.dateAdded) FROM song_artist INNER JOIN song ON song.idSong = song_artist.idSong "
              "  WHERE song_artist.idArtist = artist.idArtist) AS dateAdded "
              "FROM artist");

  CLog::Log(LOGINFO, "create albumartist view");
  m_pDS->exec("CREATE VIEW albumartistview AS SELECT"
              "  album_artist.idAlbum AS idAlbum, "
              "  album_artist.idArtist AS idArtist, "
              "  0 AS idRole, "
              "  'AlbumArtist' AS strRole, "
              "  artist.strArtist AS strArtist, "
              "  artist.strSortName AS strSortName,"
              "  artist.strMusicBrainzArtistID AS strMusicBrainzArtistID, "
              "  album_artist.iOrder AS iOrder "
              "FROM album_artist "
              "JOIN artist ON "
              "     album_artist.idArtist = artist.idArtist");

  CLog::Log(LOGINFO, "create songartist view");
  m_pDS->exec("CREATE VIEW songartistview AS SELECT"
              "  song_artist.idSong AS idSong, "
              "  song_artist.idArtist AS idArtist, "
              "  song_artist.idRole AS idRole, "
              "  role.strRole AS strRole, "
              "  artist.strArtist AS strArtist, "
              "  artist.strSortName AS strSortName,"
              "  artist.strMusicBrainzArtistID AS strMusicBrainzArtistID, "
              "  song_artist.iOrder AS iOrder "
              "FROM song_artist "
              "JOIN artist ON "
              "     song_artist.idArtist = artist.idArtist "
              "JOIN role ON "
              "     song_artist.idRole = role.idRole");
}

void CMusicDatabase::SplitPath(const std::string& strFileNameAndPath, std::string& strPath, std::string& strFileName)
{
  URIUtils::Split(strFileNameAndPath, strPath, strFileName);
  // Keep protocol options as part of the path
  if (URIUtils::IsURL(strFileNameAndPath))
  {
    CURL url(strFileNameAndPath);
    if (!url.GetProtocolOptions().empty())
      strPath += "|" + url.GetProtocolOptions();
  }
}

bool CMusicDatabase::AddAlbum(CAlbum& album, int idSource)
{
  BeginTransaction();
  SetLibraryLastUpdated();

  album.idAlbum = AddAlbum(album.strAlbum,
                           album.strMusicBrainzAlbumID,
                           album.strReleaseGroupMBID,
                           album.GetAlbumArtistString(),
                           album.GetAlbumArtistSort(),
                           album.GetGenreString(),
                           album.iYear,
                           album.strLabel, album.strType,
                           album.bCompilation, album.releaseType);

  // Add the album artists
  if (album.artistCredits.empty())
    AddAlbumArtist(BLANKARTIST_ID, album.idAlbum, BLANKARTIST_NAME, 0); // Album must have at least one artist so set artist to [Missing]
  for (auto artistCredit = album.artistCredits.begin(); artistCredit != album.artistCredits.end(); ++artistCredit)
  {
    artistCredit->idArtist = AddArtist(artistCredit->GetArtist(), artistCredit->GetMusicBrainzArtistID(), artistCredit->GetSortName());
    AddAlbumArtist(artistCredit->idArtist,
                   album.idAlbum,
                   artistCredit->GetArtist(),
                   std::distance(album.artistCredits.begin(), artistCredit));
  }

  for (auto song = album.songs.begin(); song != album.songs.end(); ++song)
  {
    song->idAlbum = album.idAlbum;

    song->idSong = AddSong(song->idAlbum,
                           song->strTitle, song->strMusicBrainzTrackID,
                           song->strFileName, song->strComment,
                           song->strMood, song->strThumb,
                           song->GetArtistString(),
                           song->GetArtistSort(),
                           song->genre,
                           song->iTrack, song->iDuration, song->iYear,
                           song->iTimesPlayed, song->iStartOffset,
                           song->iEndOffset,
                           song->lastPlayed,
                           song->rating,
                           song->userrating,
                           song->votes,
                           song->replayGain);

    if (song->artistCredits.empty())
      AddSongArtist(BLANKARTIST_ID, song->idSong, ROLE_ARTIST, BLANKARTIST_NAME, 0); // Song must have at least one artist so set artist to [Missing]

    for (auto artistCredit = song->artistCredits.begin(); artistCredit != song->artistCredits.end(); ++artistCredit)
    {
      artistCredit->idArtist = AddArtist(artistCredit->GetArtist(),
                                         artistCredit->GetMusicBrainzArtistID(),
                                         artistCredit->GetSortName());
      AddSongArtist(artistCredit->idArtist,
                    song->idSong,
                    ROLE_ARTIST,
                    artistCredit->GetArtist(), // we don't have song artist breakdowns from scrapers, yet
                    std::distance(song->artistCredits.begin(), artistCredit));
    }
    // Having added artist credits (maybe with MBID) add the other contributing artists (no MBID)
    // and use COMPOSERSORT tag data to provide sort names for artists that are composers
    AddSongContributors(song->idSong, song->GetContributors(), song->GetComposerSort());
  }

  // Add album sources
  if (idSource > 0)
    AddAlbumSource(album.idAlbum, idSource);
  else
  {
    // Use album path, or failing that song paths to determine sources for the album
    AddAlbumSources(album.idAlbum, album.strPath);
  }

  for (const auto &albumArt : album.art)
    SetArtForItem(album.idAlbum, MediaTypeAlbum, albumArt.first, albumArt.second);

  CommitTransaction();
  return true;
}

bool CMusicDatabase::UpdateAlbum(CAlbum& album)
{
  BeginTransaction();
  SetLibraryLastUpdated();

  const std::string itemSeparator = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_musicItemSeparator;

  UpdateAlbum(album.idAlbum,
              album.strAlbum, album.strMusicBrainzAlbumID,
              album.strReleaseGroupMBID,
              album.GetAlbumArtistString(), album.GetAlbumArtistSort(),
              album.GetGenreString(),
              StringUtils::Join(album.moods, itemSeparator),
              StringUtils::Join(album.styles, itemSeparator),
              StringUtils::Join(album.themes, itemSeparator),
              album.strReview,
              album.thumbURL.m_xml.c_str(),
              album.strLabel, album.strType,
              album.fRating, album.iUserrating, album.iVotes, album.iYear, album.bCompilation, album.releaseType,
              album.bScrapedMBID);

  if (!album.bArtistSongMerge)
  {
    // Album artist(s) already exist and names are not changing, but may have scraped Musicbrainz ids to add
    for (const auto &artistCredit : album.artistCredits)
      UpdateArtistScrapedMBID(artistCredit.GetArtistId(), artistCredit.GetMusicBrainzArtistID());
  }
  else
  {
    // Replace the album artists with those scraped or set by JSON
    DeleteAlbumArtistsByAlbum(album.idAlbum);
    if (album.artistCredits.empty())
      AddAlbumArtist(BLANKARTIST_ID, album.idAlbum, BLANKARTIST_NAME, 0); // Album must have at least one artist so set artist to [Missing]
    for (auto artistCredit = album.artistCredits.begin(); artistCredit != album.artistCredits.end(); ++artistCredit)
    {
      artistCredit->idArtist = AddArtist(artistCredit->GetArtist(),
        artistCredit->GetMusicBrainzArtistID(), artistCredit->GetSortName(), true);
      AddAlbumArtist(artistCredit->idArtist,
        album.idAlbum,
        artistCredit->GetArtist(),
        std::distance(album.artistCredits.begin(), artistCredit));
    }
    /* Replace the songs with those scraped or imported, but if new songs is empty
       (such as when called from JSON) do not remove the original ones
       Also updates nested data e.g. song artists, song genres and contributors
    */
    for (auto &song : album.songs)
      UpdateSong(song);
  }

  if (!album.art.empty())
    SetArtForItem(album.idAlbum, MediaTypeAlbum, album.art);

  CommitTransaction();
  return true;
}

int CMusicDatabase::AddSong(const int idAlbum,
                            const std::string& strTitle, const std::string& strMusicBrainzTrackID,
                            const std::string& strPathAndFileName, const std::string& strComment,
                            const std::string& strMood, const std::string& strThumb,
                            const std::string &artistDisp, const std::string &artistSort,
                            const std::vector<std::string>& genres,
                            int iTrack, int iDuration, int iYear,
                            const int iTimesPlayed, int iStartOffset, int iEndOffset,
                            const CDateTime& dtLastPlayed, float rating, int userrating, int votes,
                            const ReplayGain& replayGain)
{
  int idSong = -1;
  std::string strSQL;
  try
  {
    // We need at least the title
    if (strTitle.empty())
      return -1;

    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    std::string strPath, strFileName;
    SplitPath(strPathAndFileName, strPath, strFileName);
    int idPath = AddPath(strPath);

    if (!strMusicBrainzTrackID.empty())
      strSQL = PrepareSQL("SELECT idSong FROM song WHERE idAlbum = %i AND iTrack=%i AND strMusicBrainzTrackID = '%s'",
                          idAlbum,
                          iTrack,
                          strMusicBrainzTrackID.c_str());
    else
      strSQL = PrepareSQL("SELECT idSong FROM song WHERE idAlbum=%i AND strFileName='%s' AND strTitle='%s' AND iTrack=%i AND strMusicBrainzTrackID IS NULL",
                          idAlbum,
                          strFileName.c_str(),
                          strTitle.c_str(),
                          iTrack);

    if (!m_pDS->query(strSQL))
      return -1;

    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      strSQL=PrepareSQL("INSERT INTO song ("
                                          "idSong,idAlbum,idPath,strArtistDisp,"
                                          "strTitle,iTrack,iDuration,iYear,strFileName,"
                                          "strMusicBrainzTrackID, strArtistSort, "
                                          "iTimesPlayed,iStartOffset, "
                                          "iEndOffset,lastplayed,rating,userrating,votes,comment,mood,strReplayGain"
                        ") values (NULL, %i, %i, '%s', '%s', %i, %i, %i, '%s'",
                    idAlbum,
                    idPath,
                    artistDisp.c_str(),
                    strTitle.c_str(),
                    iTrack, iDuration, iYear,
                    strFileName.c_str());

      if (strMusicBrainzTrackID.empty())
        strSQL += PrepareSQL(",NULL");
      else
        strSQL += PrepareSQL(",'%s'", strMusicBrainzTrackID.c_str());
      if (artistSort.empty())
        strSQL += PrepareSQL(",NULL");
      else
        strSQL += PrepareSQL(",'%s'", artistSort.c_str());

      if (dtLastPlayed.IsValid())
        strSQL += PrepareSQL(",%i,%i,%i,'%s', %.1f, %i, %i, '%s','%s', '%s')",
                      iTimesPlayed, iStartOffset, iEndOffset, dtLastPlayed.GetAsDBDateTime().c_str(), rating, userrating, votes,
                      strComment.c_str(), strMood.c_str(), replayGain.Get().c_str());
      else
        strSQL += PrepareSQL(",%i,%i,%i,NULL, %.1f, %i, %i,'%s', '%s', '%s')",
                      iTimesPlayed, iStartOffset, iEndOffset, rating, userrating, votes, strComment.c_str(), strMood.c_str(), replayGain.Get().c_str());
      m_pDS->exec(strSQL);
      idSong = (int)m_pDS->lastinsertid();
    }
    else
    {
      idSong = m_pDS->fv("idSong").get_asInt();
      m_pDS->close();
      UpdateSong( idSong, strTitle, strMusicBrainzTrackID, strPathAndFileName, strComment, strMood, strThumb,
                  artistDisp, artistSort, genres, iTrack, iDuration, iYear, iTimesPlayed, iStartOffset, iEndOffset,
                  dtLastPlayed, rating, userrating, votes, replayGain);
    }

    if (!strThumb.empty())
      SetArtForItem(idSong, MediaTypeSong, "thumb", strThumb);

    // Song genres added, and genre string updated to use the standardised genre names
    AddSongGenres(idSong, genres);

    UpdateFileDateAdded(idSong, strPathAndFileName);

    AnnounceUpdate(MediaTypeSong, idSong, true);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "musicdatabase:unable to addsong (%s)", strSQL.c_str());
  }
  return idSong;
}

bool CMusicDatabase::GetSong(int idSong, CSong& song)
{
  try
  {
    song.Clear();

    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    std::string strSQL=PrepareSQL("SELECT songview.*,songartistview.* FROM songview "
                                 " JOIN songartistview ON songview.idSong = songartistview.idSong "
                                 " WHERE songview.idSong = %i "
                                 " ORDER BY songartistview.idRole, songartistview.iOrder", idSong);

    if (!m_pDS->query(strSQL)) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return false;
    }

    int songArtistOffset = song_enumCount;

    song = GetSongFromDataset(m_pDS.get()->get_sql_record());
    while (!m_pDS->eof())
    {
      const dbiplus::sql_record* const record = m_pDS.get()->get_sql_record();

      int idSongArtistRole = record->at(songArtistOffset + artistCredit_idRole).get_asInt();
      if (idSongArtistRole == ROLE_ARTIST)
        song.artistCredits.emplace_back(GetArtistCreditFromDataset(record, songArtistOffset));
      else
        song.AppendArtistRole(GetArtistRoleFromDataset(record, songArtistOffset));

      m_pDS->next();
    }
    m_pDS->close(); // cleanup recordset data
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%i) failed", __FUNCTION__, idSong);
  }

  return false;
}

bool CMusicDatabase::UpdateSong(CSong& song, bool bArtists /*= true*/)
{
  int result = UpdateSong(song.idSong,
                    song.strTitle,
                    song.strMusicBrainzTrackID,
                    song.strFileName,
                    song.strComment,
                    song.strMood,
                    song.strThumb,
                    song.GetArtistString(),
                    song.GetArtistSort(),
                    song.genre,
                    song.iTrack,
                    song.iDuration,
                    song.iYear,
                    song.iTimesPlayed,
                    song.iStartOffset,
                    song.iEndOffset,
                    song.lastPlayed,
                    song.rating,
                    song.userrating,
                    song.votes,
                    song.replayGain);
  if (result < 0)
    return false;

  // Replace Song genres and update genre string using the standardised genre names
  AddSongGenres(song.idSong, song.genre);
  if (bArtists)
  {
    //Replace song artists and contributors
    DeleteSongArtistsBySong(song.idSong);
    if (song.artistCredits.empty())
      AddSongArtist(BLANKARTIST_ID, song.idSong, ROLE_ARTIST, BLANKARTIST_NAME, 0); // Song must have at least one artist so set artist to [Missing]
    for (auto artistCredit = song.artistCredits.begin(); artistCredit != song.artistCredits.end(); ++artistCredit)
    {
      artistCredit->idArtist = AddArtist(artistCredit->GetArtist(),
        artistCredit->GetMusicBrainzArtistID(), artistCredit->GetSortName());
      AddSongArtist(artistCredit->idArtist,
        song.idSong,
        ROLE_ARTIST,
        artistCredit->GetArtist(),
        std::distance(song.artistCredits.begin(), artistCredit));
    }
    // Having added artist credits (maybe with MBID) add the other contributing artists (MBID unknown)
    // and use COMPOSERSORT tag data to provide sort names for artists that are composers
    AddSongContributors(song.idSong, song.GetContributors(), song.GetComposerSort());
  }

  return true;
}

int CMusicDatabase::UpdateSong(int idSong,
                               const std::string& strTitle, const std::string& strMusicBrainzTrackID,
                               const std::string& strPathAndFileName, const std::string& strComment,
                               const std::string& strMood, const std::string& strThumb,
                               const std::string &artistDisp, const std::string &artistSort,
                               const std::vector<std::string>& genres,
                               int iTrack, int iDuration, int iYear,
                               int iTimesPlayed, int iStartOffset, int iEndOffset,
                               const CDateTime& dtLastPlayed, float rating, int userrating, int votes,
                               const ReplayGain& replayGain)
{
  if (idSong < 0)
    return -1;

  std::string strSQL;
  std::string strPath, strFileName;
  SplitPath(strPathAndFileName, strPath, strFileName);
  int idPath = AddPath(strPath);

  strSQL = PrepareSQL("UPDATE song SET idPath = %i, strArtistDisp = '%s', strGenres = '%s', "
      " strTitle = '%s', iTrack = %i, iDuration = %i, iYear = %i, strFileName = '%s'",
      idPath,
      artistDisp.c_str(),
      StringUtils::Join(genres, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_musicItemSeparator).c_str(),
      strTitle.c_str(),
      iTrack, iDuration, iYear,
      strFileName.c_str());
  if (strMusicBrainzTrackID.empty())
    strSQL += PrepareSQL(", strMusicBrainzTrackID = NULL");
  else
    strSQL += PrepareSQL(", strMusicBrainzTrackID = '%s'", strMusicBrainzTrackID.c_str());
  if (artistSort.empty())
    strSQL += PrepareSQL(", strArtistSort = NULL");
  else
    strSQL += PrepareSQL(", strArtistSort = '%s'", artistSort.c_str());


  if (dtLastPlayed.IsValid())
    strSQL += PrepareSQL(", iTimesPlayed = %i, iStartOffset = %i, iEndOffset = %i, lastplayed = '%s', rating = %.1f, userrating = %i, votes = %i, comment = '%s', mood = '%s', strReplayGain = '%s'",
                         iTimesPlayed, iStartOffset, iEndOffset, dtLastPlayed.GetAsDBDateTime().c_str(), rating, userrating, votes, strComment.c_str(), strMood.c_str(), replayGain.Get().c_str());
  else
    strSQL += PrepareSQL(", iTimesPlayed = %i, iStartOffset = %i, iEndOffset = %i, lastplayed = NULL, rating = %.1f, userrating = %i, votes = %i, comment = '%s', mood = '%s', strReplayGain = '%s'",
                         iTimesPlayed, iStartOffset, iEndOffset, rating, userrating, votes, strComment.c_str(), strMood.c_str(), replayGain.Get().c_str());
  strSQL += PrepareSQL(" WHERE idSong = %i", idSong);

  bool status = ExecuteQuery(strSQL);

  UpdateFileDateAdded(idSong, strPathAndFileName);

  if (status)
    AnnounceUpdate(MediaTypeSong, idSong);
  return idSong;
}

int CMusicDatabase::AddAlbum(const std::string& strAlbum, const std::string& strMusicBrainzAlbumID,
                             const std::string& strReleaseGroupMBID,
                             const std::string& strArtist, const std::string& strArtistSort,
                             const std::string& strGenre, int year,
                             const std::string& strRecordLabel, const std::string& strType,
                             bool bCompilation, CAlbum::ReleaseType releaseType)
{
  std::string strSQL;
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    if (!strMusicBrainzAlbumID.empty())
      strSQL = PrepareSQL("SELECT * FROM album WHERE strMusicBrainzAlbumID = '%s'",
                        strMusicBrainzAlbumID.c_str());
    else
      strSQL = PrepareSQL("SELECT * FROM album WHERE strArtistDisp LIKE '%s' AND strAlbum LIKE '%s' AND strMusicBrainzAlbumID IS NULL",
                          strArtist.c_str(),
                          strAlbum.c_str());
    m_pDS->query(strSQL);

    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      // doesnt exists, add it
      strSQL = PrepareSQL("INSERT INTO album (idAlbum, strAlbum, strArtistDisp, strGenres, iYear, "
        "strLabel, strType, bCompilation, strReleaseType, strMusicBrainzAlbumID, strReleaseGroupMBID, strArtistSort) "
        "values(NULL, '%s', '%s', '%s', %i, '%s', '%s', %i, '%s'",
        strAlbum.c_str(),
        strArtist.c_str(),
        strGenre.c_str(),
        year,
        strRecordLabel.c_str(),
        strType.c_str(),
        bCompilation,
        CAlbum::ReleaseTypeToString(releaseType).c_str());

      if (strMusicBrainzAlbumID.empty())
        strSQL += PrepareSQL(", NULL");
      else
        strSQL += PrepareSQL(",'%s'", strMusicBrainzAlbumID.c_str());
      if (strReleaseGroupMBID.empty())
        strSQL += PrepareSQL(", NULL");
      else
        strSQL += PrepareSQL(",'%s'", strReleaseGroupMBID.c_str());
      if (strArtistSort.empty())
        strSQL += PrepareSQL(", NULL");
      else
        strSQL += PrepareSQL(", '%s'", strArtistSort.c_str());
      strSQL += ")";
      m_pDS->exec(strSQL);

      return (int)m_pDS->lastinsertid();
    }
    else
    {
      /* Exists in our database and being re-scanned from tags, so we should update it as the details
         may have changed.

         Note that for multi-folder albums this will mean the last folder scanned will have the information
         stored for it.  Most values here should be the same across all songs anyway, but it does mean
         that if there's any inconsistencies then only the last folders information will be taken.

         We make sure we clear out the link tables (album artists, album sources) and we reset
         the last scraped time to make sure that online metadata is re-fetched. */
      int idAlbum = m_pDS->fv("idAlbum").get_asInt();
      m_pDS->close();

      strSQL = "UPDATE album SET ";
      if (!strMusicBrainzAlbumID.empty())
        strSQL += PrepareSQL("strAlbum = '%s', strArtistDisp = '%s', ", strAlbum.c_str(), strArtist.c_str());
      if (strReleaseGroupMBID.empty())
        strSQL += PrepareSQL(" strReleaseGroupMBID = NULL,");
      else
        strSQL += PrepareSQL(" strReleaseGroupMBID ='%s', ", strReleaseGroupMBID.c_str());
      if (strArtistSort.empty())
        strSQL += PrepareSQL(" strArtistSort = NULL");
      else
        strSQL += PrepareSQL(" strArtistSort = '%s'", strArtistSort.c_str());

      strSQL += PrepareSQL(", strGenres = '%s', iYear=%i, strLabel = '%s', strType = '%s', "
        "bCompilation=%i, strReleaseType = '%s', lastScraped = NULL "
        "WHERE idAlbum=%i",
        strGenre.c_str(),
        year,
        strRecordLabel.c_str(),
        strType.c_str(),
        bCompilation,
        CAlbum::ReleaseTypeToString(releaseType).c_str(),
        idAlbum);
      m_pDS->exec(strSQL);
      DeleteAlbumArtistsByAlbum(idAlbum);
      DeleteAlbumSources(idAlbum);
      return idAlbum;
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed with query (%s)", __FUNCTION__, strSQL.c_str());
  }

  return -1;
}

int  CMusicDatabase::UpdateAlbum(int idAlbum,
                                 const std::string& strAlbum, const std::string& strMusicBrainzAlbumID,
                                 const std::string& strReleaseGroupMBID,
                                 const std::string& strArtist, const std::string& strArtistSort,
                                 const std::string& strGenre,
                                 const std::string& strMoods, const std::string& strStyles,
                                 const std::string& strThemes, const std::string& strReview,
                                 const std::string& strImage, const std::string& strLabel,
                                 const std::string& strType,
                                 float fRating, int iUserrating, int iVotes, int iYear, bool bCompilation,
                                 CAlbum::ReleaseType releaseType,
                                 bool bScrapedMBID)
{
  if (idAlbum < 0)
    return -1;

  std::string strSQL;
  strSQL = PrepareSQL("UPDATE album SET "
                      " strAlbum = '%s', strArtistDisp = '%s', strGenres = '%s', "
                      " strMoods = '%s', strStyles = '%s', strThemes = '%s', "
                      " strReview = '%s', strImage = '%s', strLabel = '%s', "
                      " strType = '%s', fRating = %f, iUserrating = %i, iVotes = %i,"
                      " iYear = %i, bCompilation = %i, strReleaseType = '%s', "
                      " lastScraped = '%s', bScrapedMBID = %i",
                      strAlbum.c_str(), strArtist.c_str(), strGenre.c_str(),
                      strMoods.c_str(), strStyles.c_str(), strThemes.c_str(),
                      strReview.c_str(), strImage.c_str(), strLabel.c_str(),
                      strType.c_str(), fRating, iUserrating, iVotes,
                      iYear, bCompilation,
                      CAlbum::ReleaseTypeToString(releaseType).c_str(),
                      CDateTime::GetCurrentDateTime().GetAsDBDateTime().c_str(),
                      bScrapedMBID);
  if (strMusicBrainzAlbumID.empty())
    strSQL += PrepareSQL(", strMusicBrainzAlbumID = NULL");
  else
    strSQL += PrepareSQL(", strMusicBrainzAlbumID = '%s'", strMusicBrainzAlbumID.c_str());
  if (strReleaseGroupMBID.empty())
    strSQL += PrepareSQL(", strReleaseGroupMBID = NULL");
  else
    strSQL += PrepareSQL(", strReleaseGroupMBID = '%s'", strReleaseGroupMBID.c_str());
  if (strArtistSort.empty())
    strSQL += PrepareSQL(", strArtistSort = NULL");
  else
    strSQL += PrepareSQL(", strArtistSort = '%s'", strArtistSort.c_str());

  strSQL += PrepareSQL(" WHERE idAlbum = %i", idAlbum);

  bool status = ExecuteQuery(strSQL);
  if (status)
    AnnounceUpdate(MediaTypeAlbum, idAlbum);
  return idAlbum;
}

bool CMusicDatabase::GetAlbum(int idAlbum, CAlbum& album, bool getSongs /* = true */)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    if (idAlbum == -1)
      return false; // not in the database

    //Get album, song and album song info data using separate queries/datasets because we can have
    //multiple roles per artist for songs and that makes a single combined join impractical
    //Get album data
    std::string sql;
    sql = PrepareSQL("SELECT albumview.*,albumartistview.* "
      " FROM albumview "
      " JOIN albumartistview ON albumview.idAlbum = albumartistview.idAlbum "
      " WHERE albumview.idAlbum = %ld "
      " ORDER BY albumartistview.iOrder", idAlbum);

    CLog::Log(LOGDEBUG, "%s", sql.c_str());
    if (!m_pDS->query(sql)) return false;
    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      return false;
    }

    int albumArtistOffset = album_enumCount;

    album = GetAlbumFromDataset(m_pDS.get()->get_sql_record(), 0, true); // true to grab and parse the imageURL
    while (!m_pDS->eof())
    {
      const dbiplus::sql_record* const record = m_pDS->get_sql_record();

      // Album artists always have role = 0 (idRole and strRole columns are in albumartistview to match columns of songartistview)
      // so there is only one row in the result set for each artist credit.
      album.artistCredits.push_back(GetArtistCreditFromDataset(record, albumArtistOffset));

      m_pDS->next();
    }
    m_pDS->close(); // cleanup recordset data

    //Get song data
    if (getSongs)
    {
      sql = PrepareSQL("SELECT songview.*, songartistview.*"
        " FROM songview "
        " JOIN songartistview ON songview.idSong = songartistview.idSong "
        " WHERE songview.idAlbum = %ld "
        " ORDER BY songview.iTrack, songartistview.idRole, songartistview.iOrder", idAlbum);

      CLog::Log(LOGDEBUG, "%s", sql.c_str());
      if (!m_pDS->query(sql)) return false;
      if (m_pDS->num_rows() == 0)  //Album with no songs
      {
        m_pDS->close();
        return false;
      }

      int songArtistOffset = song_enumCount;
      std::set<int> songs;
      while (!m_pDS->eof())
      {
        const dbiplus::sql_record* const record = m_pDS->get_sql_record();

        int idSong = record->at(song_idSong).get_asInt();  //Same as songartist.idSong by join
        if (songs.find(idSong) == songs.end())
        {
          album.songs.emplace_back(GetSongFromDataset(record));
          songs.insert(idSong);
        }

        int idSongArtistRole = record->at(songArtistOffset + artistCredit_idRole).get_asInt();
        //By query order song is the last one appended to the album song vector.
        if (idSongArtistRole == ROLE_ARTIST)
          album.songs.back().artistCredits.emplace_back(GetArtistCreditFromDataset(record, songArtistOffset));
        else
          album.songs.back().AppendArtistRole(GetArtistRoleFromDataset(record, songArtistOffset));

        m_pDS->next();
      }
      m_pDS->close(); // cleanup recordset data
    }

    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%i) failed", __FUNCTION__, idAlbum);
  }

  return false;
}

bool CMusicDatabase::ClearAlbumLastScrapedTime(int idAlbum)
{
  std::string strSQL = PrepareSQL("UPDATE album SET lastScraped = NULL WHERE idAlbum = %i", idAlbum);
  return ExecuteQuery(strSQL);
}

bool CMusicDatabase::HasAlbumBeenScraped(int idAlbum)
{
  std::string strSQL = PrepareSQL("SELECT idAlbum FROM album WHERE idAlbum = %i AND lastScraped IS NULL", idAlbum);
  return GetSingleValue(strSQL).empty();
}

int CMusicDatabase::AddGenre(std::string& strGenre)
{
  std::string strSQL;
  try
  {
    StringUtils::Trim(strGenre);

    if (strGenre.empty())
      strGenre=g_localizeStrings.Get(13205); // Unknown

    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    auto it = m_genreCache.find(strGenre);
    if (it != m_genreCache.end())
      return it->second;


    strSQL=PrepareSQL("SELECT idGenre, strGenre FROM genre WHERE strGenre LIKE '%s'", strGenre.c_str());
    m_pDS->query(strSQL);
    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      // doesnt exists, add it
      strSQL=PrepareSQL("INSERT INTO genre (idGenre, strGenre) values( NULL, '%s' )", strGenre.c_str());
      m_pDS->exec(strSQL);

      int idGenre = (int)m_pDS->lastinsertid();
      m_genreCache.insert(std::pair<std::string, int>(strGenre, idGenre));
      return idGenre;
    }
    else
    {
      int idGenre = m_pDS->fv("idGenre").get_asInt();
      strGenre = m_pDS->fv("strGenre").get_asString();
      m_genreCache.insert(std::pair<std::string, int>(strGenre, idGenre));
      m_pDS->close();
      return idGenre;
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "musicdatabase:unable to addgenre (%s)", strSQL.c_str());
  }

  return -1;
}

bool CMusicDatabase::UpdateArtist(const CArtist& artist)
{
  SetLibraryLastUpdated();

  const std::string itemSeparator = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_musicItemSeparator;

  UpdateArtist(artist.idArtist,
               artist.strArtist, artist.strSortName,
               artist.strMusicBrainzArtistID, artist.bScrapedMBID,
               artist.strType, artist.strGender, artist.strDisambiguation,
               artist.strBorn, artist.strFormed,
               StringUtils::Join(artist.genre, itemSeparator),
               StringUtils::Join(artist.moods, itemSeparator),
               StringUtils::Join(artist.styles, itemSeparator),
               StringUtils::Join(artist.instruments, itemSeparator),
               artist.strBiography, artist.strDied,
               artist.strDisbanded,
               StringUtils::Join(artist.yearsActive, itemSeparator).c_str(),
               artist.thumbURL.m_xml.c_str(),
               artist.fanart.m_xml.c_str());

  DeleteArtistDiscography(artist.idArtist);
  for (const auto &disc : artist.discography)
  {
    AddArtistDiscography(artist.idArtist, disc.first, disc.second);
  }

  // Set current artwork (held in art table)
  if (!artist.art.empty())
    SetArtForItem(artist.idArtist, MediaTypeArtist, artist.art);

  return true;
}

int CMusicDatabase::AddArtist(const std::string& strArtist, const std::string& strMusicBrainzArtistID, const std::string& strSortName, bool bScrapedMBID /* = false*/)
{
  std::string strSQL;
  int idArtist = AddArtist(strArtist, strMusicBrainzArtistID, bScrapedMBID);
  if (idArtist < 0 || strSortName.empty())
    return idArtist;

  /* Artist sort name always taken as the first value provided that is different from name, so only
     update when current sort name is blank. If a new sortname the same as name is provided then
     clear any sortname currently held.
  */

  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    strSQL = PrepareSQL("SELECT strArtist, strSortName FROM artist WHERE idArtist = %i", idArtist);
    m_pDS->query(strSQL);
    if (m_pDS->num_rows() != 1)
    {
      m_pDS->close();
      return -1;
    }
    std::string strArtistName, strArtistSort;
    strArtistName = m_pDS->fv("strArtist").get_asString();
    strArtistSort = m_pDS->fv("strSortName").get_asString();
    m_pDS->close();

    if (!strArtistSort.empty())
    {
      if (strSortName.compare(strArtistName) == 0)
        m_pDS->exec(PrepareSQL("UPDATE artist SET strSortName = NULL WHERE idArtist = %i", idArtist));
    }
    else if (strSortName.compare(strArtistName) != 0)
        m_pDS->exec(PrepareSQL("UPDATE artist SET strSortName = '%s' WHERE idArtist = %i", strSortName.c_str(), idArtist));

    return idArtist;
  }

  catch (...)
  {
    CLog::Log(LOGERROR, "musicdatabase:unable to addartist with sortname (%s)", strSQL.c_str());
  }

  return -1;
}

int CMusicDatabase::AddArtist(const std::string& strArtist, const std::string& strMusicBrainzArtistID, bool bScrapedMBID /* = false*/)
{
  std::string strSQL;
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    // 1) MusicBrainz
    if (!strMusicBrainzArtistID.empty())
    {
      // 1.a) Match on a MusicBrainz ID
      strSQL = PrepareSQL("SELECT idArtist, strArtist FROM artist WHERE strMusicBrainzArtistID = '%s'",
        strMusicBrainzArtistID.c_str());
      m_pDS->query(strSQL);
      if (m_pDS->num_rows() > 0)
      {
        int idArtist = m_pDS->fv("idArtist").get_asInt();
        bool update = m_pDS->fv("strArtist").get_asString().compare(strMusicBrainzArtistID) == 0;
        m_pDS->close();
        if (update)
        {
          strSQL = PrepareSQL("UPDATE artist SET strArtist = '%s' WHERE idArtist = %i", strArtist.c_str(), idArtist);
          m_pDS->exec(strSQL);
          m_pDS->close();
        }
        return idArtist;
      }
      m_pDS->close();


      // 1.b) No match on MusicBrainz ID. Look for a previously added artist with no MusicBrainz ID
      //     and update that if it exists.
      strSQL = PrepareSQL("SELECT idArtist FROM artist WHERE strArtist LIKE '%s' AND strMusicBrainzArtistID IS NULL", strArtist.c_str());
      m_pDS->query(strSQL);
      if (m_pDS->num_rows() > 0)
      {
        int idArtist = m_pDS->fv("idArtist").get_asInt();
        m_pDS->close();
        // 1.b.a) We found an artist by name but with no MusicBrainz ID set, update it and assume it is our artist, flag when mbid scraped
        strSQL = PrepareSQL("UPDATE artist SET strArtist = '%s', strMusicBrainzArtistID = '%s', bScrapedMBID = %i WHERE idArtist = %i",
          strArtist.c_str(),
          strMusicBrainzArtistID.c_str(),
          bScrapedMBID,
          idArtist);
        m_pDS->exec(strSQL);
        return idArtist;
      }

      // 2) No MusicBrainz - search for any artist (MB ID or non) with the same name.
      //    With MusicBrainz IDs this could return multiple artists and is non-determinstic
      //    Always pick the first artist ID returned by the DB to return.
    }
    else
    {
      strSQL = PrepareSQL("SELECT idArtist FROM artist WHERE strArtist LIKE '%s'",
        strArtist.c_str());

      m_pDS->query(strSQL);
      if (m_pDS->num_rows() > 0)
      {
        int idArtist = m_pDS->fv("idArtist").get_asInt();
        m_pDS->close();
        return idArtist;
      }
      m_pDS->close();
    }

    // 3) No artist exists at all - add it, flagging when has scraped mbid
    if (strMusicBrainzArtistID.empty())
      strSQL = PrepareSQL("INSERT INTO artist (idArtist, strArtist, strMusicBrainzArtistID) VALUES( NULL, '%s', NULL )",
        strArtist.c_str());
    else
      strSQL = PrepareSQL("INSERT INTO artist (idArtist, strArtist, strMusicBrainzArtistID, bScrapedMBID) VALUES( NULL, '%s', '%s', %i )",
        strArtist.c_str(),
        strMusicBrainzArtistID.c_str(),
        bScrapedMBID);

    m_pDS->exec(strSQL);
    int idArtist = (int)m_pDS->lastinsertid();
    return idArtist;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "musicdatabase:unable to addartist (%s)", strSQL.c_str());
  }

  return -1;
}

int  CMusicDatabase::UpdateArtist(int idArtist,
                                  const std::string& strArtist, const std::string& strSortName,
                                  const std::string& strMusicBrainzArtistID, const bool bScrapedMBID,
                                  const std::string& strType, const std::string& strGender,
                                  const std::string& strDisambiguation,
                                  const std::string& strBorn, const std::string& strFormed,
                                  const std::string& strGenres, const std::string& strMoods,
                                  const std::string& strStyles, const std::string& strInstruments,
                                  const std::string& strBiography, const std::string& strDied,
                                  const std::string& strDisbanded, const std::string& strYearsActive,
                                  const std::string& strImage, const std::string& strFanart)
{
  CScraperUrl thumbURL;
  CFanart fanart;
  if (idArtist < 0)
    return -1;

  std::string strSQL;
  strSQL = PrepareSQL("UPDATE artist SET "
                      " strArtist = '%s', "
                      " strType = '%s', strGender = '%s', strDisambiguation = '%s', "
                      " strBorn = '%s', strFormed = '%s', strGenres = '%s', "
                      " strMoods = '%s', strStyles = '%s', strInstruments = '%s', "
                      " strBiography = '%s', strDied = '%s', strDisbanded = '%s', "
                      " strYearsActive = '%s', strImage = '%s', strFanart = '%s', "
                      " lastScraped = '%s', bScrapedMBID = %i",
                      strArtist.c_str(),
                      /* strSortName.c_str(),*/
                      /* strMusicBrainzArtistID.c_str(), */
                      strType.c_str(), strGender.c_str(), strDisambiguation.c_str(),
                      strBorn.c_str(), strFormed.c_str(), strGenres.c_str(),
                      strMoods.c_str(), strStyles.c_str(), strInstruments.c_str(),
                      strBiography.c_str(), strDied.c_str(), strDisbanded.c_str(),
                      strYearsActive.c_str(), strImage.c_str(), strFanart.c_str(),
                      CDateTime::GetCurrentDateTime().GetAsDBDateTime().c_str(),
                      bScrapedMBID);
  if (strMusicBrainzArtistID.empty())
    strSQL += PrepareSQL(", strMusicBrainzArtistID = NULL");
  else
    strSQL += PrepareSQL(", strMusicBrainzArtistID = '%s'", strMusicBrainzArtistID.c_str());
  if (strSortName.empty())
    strSQL += PrepareSQL(", strSortName = NULL");
  else
    strSQL += PrepareSQL(", strSortName = '%s'", strSortName.c_str());

  strSQL += PrepareSQL(" WHERE idArtist = %i", idArtist);

  bool status = ExecuteQuery(strSQL);
  if (status)
    AnnounceUpdate(MediaTypeArtist, idArtist);
  return idArtist;
}

bool CMusicDatabase::UpdateArtistScrapedMBID(int idArtist, const std::string& strMusicBrainzArtistID)
{
  if (strMusicBrainzArtistID.empty() || idArtist < 0)
    return false;

  // Set scraped artist Musicbrainz ID for a previously added artist with no MusicBrainz ID
  std::string strSQL;
  strSQL = PrepareSQL("UPDATE artist SET strMusicBrainzArtistID = '%s', bScrapedMBID = 1 "
    "WHERE idArtist = %i AND strMusicBrainzArtistID IS NULL",
    strMusicBrainzArtistID.c_str(), idArtist);

  bool status = ExecuteQuery(strSQL);
  if (status)
  {
    AnnounceUpdate(MediaTypeArtist, idArtist);
    return true;
  }
  return false;
}

bool CMusicDatabase::GetArtist(int idArtist, CArtist &artist, bool fetchAll /* = false */)
{
  try
  {
    unsigned int time = XbmcThreads::SystemClockMillis();
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    if (idArtist == -1)
      return false; // not in the database

    std::string strSQL;
    if (fetchAll)
      strSQL = PrepareSQL("SELECT * FROM artistview LEFT JOIN discography ON artistview.idArtist = discography.idArtist WHERE artistview.idArtist = %i", idArtist);
    else
      // Same fields as artistview, but don't fetch dateadded when value not 
      // needed. MySQL very slow for view with subquery column with aggregate
      //! @todo replace with artistview once dateadded is column of artist table
      strSQL = PrepareSQL("SELECT "
        "idArtist, strArtist, strSortName, "
        "strMusicBrainzArtistID, "
        "strType, strGender, strDisambiguation, "
        "strBorn, strFormed, strGenres,"
        "strMoods, strStyles, strInstruments, "
        "strBiography, strDied, strDisbanded, "
        "strYearsActive, strImage, strFanart, "
        "bScrapedMBID, lastScraped, "
        "'' AS dateAdded "
        "FROM artist WHERE idArtist = %i", idArtist);

    if (!m_pDS->query(strSQL)) return false;
    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      return false;
    }

    int discographyOffset = artist_enumCount;

    artist.discography.clear();
    artist = GetArtistFromDataset(m_pDS.get()->get_sql_record(), 0, true); // inc scraped art URLs
    if (fetchAll)
    {
      while (!m_pDS->eof())
      {
        const dbiplus::sql_record* const record = m_pDS.get()->get_sql_record();

        artist.discography.emplace_back(record->at(discographyOffset + 1).get_asString(), record->at(discographyOffset + 2).get_asString());
        m_pDS->next();
      }
    }
    m_pDS->close(); // cleanup recordset data
    CLog::Log(LOGDEBUG, LOGDATABASE, "{0}({1}) - took {2} ms", __FUNCTION__, strSQL, XbmcThreads::SystemClockMillis() - time);
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%i) failed", __FUNCTION__, idArtist);
  }

  return false;
}

bool CMusicDatabase::GetArtistExists(int idArtist)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    std::string strSQL = PrepareSQL("SELECT 1 FROM artist WHERE artist.idArtist = %i LIMIT 1", idArtist);

    if (!m_pDS->query(strSQL)) return false;
    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      return false;
    }
    m_pDS->close(); // cleanup recordset data
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%i) failed", __FUNCTION__, idArtist);
  }

  return false;
}

int CMusicDatabase::GetLastArtist()
{
  std::string strSQL = "SELECT MAX(idArtist) FROM artist";
  std::string lastArtist = GetSingleValue(strSQL);
  if (lastArtist.empty())
      return -1;

  return static_cast<int>(strtol(lastArtist.c_str(), NULL, 10));
}

bool CMusicDatabase::HasArtistBeenScraped(int idArtist)
{
  std::string strSQL = PrepareSQL("SELECT idArtist FROM artist WHERE idArtist = %i AND lastScraped IS NULL", idArtist);
  return GetSingleValue(strSQL).empty();
}

bool CMusicDatabase::ClearArtistLastScrapedTime(int idArtist)
{
  std::string strSQL = PrepareSQL("UPDATE artist SET lastScraped = NULL WHERE idArtist = %i", idArtist);
  return ExecuteQuery(strSQL);
}

int CMusicDatabase::AddArtistDiscography(int idArtist, const std::string& strAlbum, const std::string& strYear)
{
  std::string strSQL=PrepareSQL("INSERT INTO discography (idArtist, strAlbum, strYear) values(%i, '%s', '%s')",
                               idArtist,
                               strAlbum.c_str(),
                               strYear.c_str());
  return ExecuteQuery(strSQL);
}

bool CMusicDatabase::DeleteArtistDiscography(int idArtist)
{
  std::string strSQL = PrepareSQL("DELETE FROM discography WHERE idArtist = %i", idArtist);
  return ExecuteQuery(strSQL);
}

bool CMusicDatabase::GetArtistDiscography(int idArtist, CFileItemList& items)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    // Combine entries from discography and album tables
    // When title in both, album entry will be before disco entry
    // Use PrepareSQL to adjust CAST syntax to AS UNSIGNED INTEGER for MySQL
    std::string strSQL;
    strSQL = PrepareSQL("SELECT strAlbum, "
      "CAST(discography.strYear AS INTEGER) AS iYear, -1 AS idAlbum "
      "FROM discography "
      "WHERE discography.idArtist = %i "
      "UNION "
      "SELECT strAlbum, iYear, album.idAlbum "
      "FROM album JOIN album_artist ON album_artist.idAlbum = album.idAlbum "
      "WHERE album_artist.idArtist = %i "
      "ORDER BY iYear, strAlbum, idAlbum DESC",
      idArtist, idArtist);

    if (!m_pDS->query(strSQL))
      return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return true;
    }

    std::string strAlbum;
    std::string strLastAlbum;
    int iLastID = -1;
    while (!m_pDS->eof())
    {
      int idAlbum = m_pDS->fv("idAlbum").get_asInt();
      strAlbum = m_pDS->fv("strAlbum").get_asString();
      if (!strAlbum.empty())
      {
        if (strAlbum.compare(strLastAlbum) != 0)
        { // Save new title (from album or discography)
          CFileItemPtr pItem(new CFileItem(strAlbum));
          pItem->SetLabel2(m_pDS->fv("iYear").get_asString());
          pItem->GetMusicInfoTag()->SetDatabaseId(idAlbum, MediaTypeAlbum);

          items.Add(pItem);
          strLastAlbum = strAlbum;
          iLastID = idAlbum;
        }
        else if (idAlbum > 0 && iLastID < 0)
        { // Amend previously saved discography item to set album ID
          items[items.Size() - 1]->GetMusicInfoTag()->SetDatabaseId(idAlbum, MediaTypeAlbum);
        }
      }
      m_pDS->next();
    }

    // cleanup
    m_pDS->close();

    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}

int CMusicDatabase::AddRole(const std::string &strRole)
{
  int idRole = -1;
  std::string strSQL;

  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;
    strSQL = PrepareSQL("SELECT idRole FROM role WHERE strRole LIKE '%s'", strRole.c_str());
    m_pDS->query(strSQL);
    if (m_pDS->num_rows() > 0)
      idRole = m_pDS->fv("idRole").get_asInt();
    m_pDS->close();

    if (idRole < 0)
    {
      strSQL = PrepareSQL("INSERT INTO role (strRole) VALUES ('%s')", strRole.c_str());
      m_pDS->exec(strSQL);
      idRole = static_cast<int>(m_pDS->lastinsertid());
      m_pDS->close();
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "musicdatabase:unable to AddRole (%s)", strSQL.c_str());
  }
  return idRole;
}

bool CMusicDatabase::AddSongArtist(int idArtist, int idSong, const std::string& strRole, const std::string& strArtist, int iOrder)
{
  int idRole = AddRole(strRole);
  return AddSongArtist(idArtist, idSong, idRole, strArtist, iOrder);
}

bool CMusicDatabase::AddSongArtist(int idArtist, int idSong, int idRole, const std::string& strArtist, int iOrder)
{
  std::string strSQL;
  strSQL = PrepareSQL("replace into song_artist (idArtist, idSong, idRole, strArtist, iOrder) values(%i,%i,%i,'%s',%i)",
    idArtist, idSong, idRole, strArtist.c_str(), iOrder);
  return ExecuteQuery(strSQL);
}

int CMusicDatabase::AddSongContributor(int idSong, const std::string& strRole, const std::string& strArtist, const std::string &strSort)
{
  if (strArtist.empty())
    return -1;

  std::string strSQL;
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    int idArtist = -1;
    // Add artist. As we only have name (no MBID) first try to identify artist from song
    // as they may have already been added with a different role (including MBID).
    strSQL = PrepareSQL("SELECT idArtist FROM song_artist WHERE idSong = %i AND strArtist LIKE '%s' ", idSong, strArtist.c_str());
    m_pDS->query(strSQL);
    if (m_pDS->num_rows() > 0)
      idArtist = m_pDS->fv("idArtist").get_asInt();
    m_pDS->close();

    if (idArtist < 0)
      idArtist = AddArtist(strArtist, "", strSort);

    // Add to song_artist table
    AddSongArtist(idArtist, idSong, strRole, strArtist, 0);

    return idArtist;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "musicdatabase:unable to AddSongContributor (%s)", strSQL.c_str());
  }

  return -1;
}

void CMusicDatabase::AddSongContributors(int idSong, const VECMUSICROLES& contributors, const std::string &strSort)
{
  std::vector<std::string> composerSort;
  size_t countComposer = 0;
  if (!strSort.empty())
  {
    composerSort = StringUtils::Split(strSort, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_musicItemSeparator);
  }

  for (const auto &credit : contributors)
  {
    std::string strSortName;
    //Identify composer sort name if we have it
    if (countComposer < composerSort.size())
    {
      if (credit.GetRoleDesc().compare("Composer") == 0)
      {
        strSortName = composerSort[countComposer];
        countComposer++;
      }
    }
    AddSongContributor(idSong, credit.GetRoleDesc(), credit.GetArtist(), strSortName);
  }
}

int CMusicDatabase::GetRoleByName(const std::string& strRole)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    std::string strSQL;
    strSQL = PrepareSQL("SELECT idRole FROM role WHERE strRole like '%s'", strRole.c_str());
    // run query
    if (!m_pDS->query(strSQL)) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound != 1)
    {
      m_pDS->close();
      return -1;
    }
    return m_pDS->fv("idRole").get_asInt();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return -1;

}

bool CMusicDatabase::GetRolesByArtist(int idArtist, CFileItem* item)
{
  try
  {
    std::string strSQL = PrepareSQL("SELECT DISTINCT song_artist.idRole, Role.strRole FROM song_artist JOIN role ON "
                                    " song_artist.idRole = role.idRole WHERE idArtist = %i ORDER BY song_artist.idRole ASC", idArtist);
    if (!m_pDS->query(strSQL))
      return false;
    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      return true;
    }

    CVariant artistRoles(CVariant::VariantTypeArray);

    while (!m_pDS->eof())
    {
      CVariant roleObj;
      roleObj["role"] = m_pDS->fv("strRole").get_asString();
      roleObj["roleid"] = m_pDS->fv("idrole").get_asInt();
      artistRoles.push_back(roleObj);
      m_pDS->next();
    }
    m_pDS->close();

    item->SetProperty("roles", artistRoles);
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%i) failed", __FUNCTION__, idArtist);
  }
  return false;
}

bool CMusicDatabase::DeleteSongArtistsBySong(int idSong)
{
  return ExecuteQuery(PrepareSQL("DELETE FROM song_artist WHERE idSong = %i", idSong));
}

bool CMusicDatabase::AddAlbumArtist(int idArtist, int idAlbum, std::string strArtist, int iOrder)
{
  std::string strSQL;
  strSQL = PrepareSQL("replace into album_artist (idArtist, idAlbum, strArtist, iOrder) values(%i,%i,'%s',%i)",
    idArtist, idAlbum, strArtist.c_str(), iOrder);
  return ExecuteQuery(strSQL);
}

bool CMusicDatabase::DeleteAlbumArtistsByAlbum(int idAlbum)
{
  return ExecuteQuery(PrepareSQL("DELETE FROM album_artist WHERE idAlbum = %i", idAlbum));
}

bool CMusicDatabase::AddSongGenres(int idSong, const std::vector<std::string>& genres)
{
  if (idSong == -1)
    return true;

  std::string strSQL;
  try
  {
    // Clear current entries for song
    strSQL = PrepareSQL("DELETE FROM song_genre WHERE idSong = %i", idSong);
    if (!ExecuteQuery(strSQL))
      return false;
    unsigned int index = 0;
    std::vector<std::string> modgenres = genres;
    for (auto &strGenre : modgenres)
    {
      int idGenre = AddGenre(strGenre); // Genre string trimed and matched case insensitively
      strSQL = PrepareSQL("INSERT INTO song_genre (idGenre, idSong, iOrder) VALUES(%i,%i,%i)",
        idGenre, idSong, index++);
      if (!ExecuteQuery(strSQL))
        return false;
    }
    // Update concatenated genre string from the standardised genre values
    std::string strGenres = StringUtils::Join(modgenres, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_musicItemSeparator);
    strSQL = PrepareSQL("UPDATE song SET strGenres = '%s' WHERE idSong = %i", strGenres.c_str(), idSong);
    if (!ExecuteQuery(strSQL))
      return false;

    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%i) %s failed", __FUNCTION__, idSong, strSQL.c_str());
  }
  return false;
}

bool CMusicDatabase::GetAlbumsByArtist(int idArtist, std::vector<int> &albums)
{
  try
  {
    std::string strSQL;
    strSQL = PrepareSQL("SELECT idAlbum  FROM album_artist WHERE idArtist = %i", idArtist);
    if (!m_pDS->query(strSQL))
      return false;
    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      return false;
    }

    while (!m_pDS->eof())
    {
      albums.push_back(m_pDS->fv("idAlbum").get_asInt());
      m_pDS->next();
    }
    m_pDS->close();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%i) failed", __FUNCTION__, idArtist);
  }
  return false;
}

bool CMusicDatabase::GetArtistsByAlbum(int idAlbum, CFileItem* item)
{
  try
  {
    std::string strSQL;

    strSQL = PrepareSQL("SELECT * FROM albumartistview WHERE idAlbum = %i", idAlbum);

    if (!m_pDS->query(strSQL))
      return false;
    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      return false;
    }

    // Get album artist credits
    VECARTISTCREDITS artistCredits;
    while (!m_pDS->eof())
    {
      artistCredits.emplace_back(GetArtistCreditFromDataset(m_pDS->get_sql_record(), 0));
      m_pDS->next();
    }
    m_pDS->close();

    // Populate item with song albumartist credits
    std::vector<std::string> musicBrainzID;
    std::vector<std::string> albumartists;
    CVariant artistidObj(CVariant::VariantTypeArray);
    for (const auto &artistCredit : artistCredits)
    {
      artistidObj.push_back(artistCredit.GetArtistId());
      albumartists.emplace_back(artistCredit.GetArtist());
      if (!artistCredit.GetMusicBrainzArtistID().empty())
        musicBrainzID.emplace_back(artistCredit.GetMusicBrainzArtistID());
    }
    item->GetMusicInfoTag()->SetAlbumArtist(albumartists);
    item->GetMusicInfoTag()->SetMusicBrainzAlbumArtistID(musicBrainzID);
    // Add song albumartistIds as separate property as not part of CMusicInfoTag
    item->SetProperty("albumartistid", artistidObj);

    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%i) failed", __FUNCTION__, idAlbum);
  }
  return false;
}

bool CMusicDatabase::GetSongsByArtist(int idArtist, std::vector<int> &songs)
{
  try
  {
    std::string strSQL;
    //Restrict to Artists only, no other roles
    strSQL = PrepareSQL("SELECT idSong FROM song_artist WHERE idArtist = %i AND idRole = 1", idArtist);
    if (!m_pDS->query(strSQL))
      return false;
    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      return false;
    }

    while (!m_pDS->eof())
    {
      songs.push_back(m_pDS->fv("idSong").get_asInt());
      m_pDS->next();
    }
    m_pDS->close();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%i) failed", __FUNCTION__, idArtist);
  }
  return false;
};

bool CMusicDatabase::GetArtistsBySong(int idSong, std::vector<int> &artists)
{
  try
  {
    std::string strSQL;
    //Restrict to Artists only, no other roles
    strSQL = PrepareSQL("SELECT idArtist FROM song_artist WHERE idSong = %i AND idRole = 1", idSong);
    if (!m_pDS->query(strSQL))
      return false;
    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      return false;
    }

    while (!m_pDS->eof())
    {
      artists.push_back(m_pDS->fv("idArtist").get_asInt());
      m_pDS->next();
    }
    m_pDS->close();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%i) failed", __FUNCTION__, idSong);
  }
  return false;
}

bool CMusicDatabase::GetGenresByArtist(int idArtist, CFileItem* item)
{
  try
  {
    std::string strSQL;
    strSQL = PrepareSQL("SELECT DISTINCT song_genre.idGenre, genre.strGenre FROM "
      "album_artist JOIN song ON album_artist.idAlbum = song.idAlbum "
      "JOIN song_genre ON song.idSong = song_genre.idSong "
      "JOIN genre ON song_genre.idGenre = genre.idGenre "
      "WHERE album_artist.idArtist = %i "
      "ORDER BY song_genre.idGenre", idArtist);
    if (!m_pDS->query(strSQL))
      return false;
    if (m_pDS->num_rows() == 0)
    {
      // Artist does have any song genres via albums may not be an album artist.
      // Check via songs artist to fetch song genres from compilations or where they are guest artist
      m_pDS->close();
      strSQL = PrepareSQL("SELECT DISTINCT song_genre.idGenre, genre.strGenre FROM "
        "song_artist JOIN song_genre ON song_artist.idSong = song_genre.idSong "
        "JOIN genre ON song_genre.idGenre = genre.idGenre "
        "WHERE song_artist.idArtist = %i "
        "ORDER BY song_genre.idGenre", idArtist);
      if (!m_pDS->query(strSQL))
        return false;
      if (m_pDS->num_rows() == 0)
      {
        //No song genres, but query sucessfull
        m_pDS->close();
        return true;
      }
    }

    CVariant artistSongGenres(CVariant::VariantTypeArray);

    while (!m_pDS->eof())
    {
      CVariant genreObj;
      genreObj["title"] = m_pDS->fv("strGenre").get_asString();
      genreObj["genreid"] = m_pDS->fv("idGenre").get_asInt();
      artistSongGenres.push_back(genreObj);
      m_pDS->next();
    }
    m_pDS->close();

    item->SetProperty("songgenres", artistSongGenres);
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%i) failed", __FUNCTION__, idArtist);
  }
  return false;
}

bool CMusicDatabase::GetGenresByAlbum(int idAlbum, CFileItem* item)
{
  try
  {
    std::string strSQL;
    strSQL = PrepareSQL("SELECT DISTINCT song_genre.idGenre, genre.strGenre FROM "
      "song JOIN song_genre ON song.idSong = song_genre.idSong "
      "JOIN genre ON song_genre.idGenre = genre.idGenre "
      "WHERE song.idAlbum = %i "
      "ORDER BY song_genre.idSong, song_genre.iOrder", idAlbum);
    if (!m_pDS->query(strSQL))
      return false;
    if (m_pDS->num_rows() == 0)
    {
      //No song genres, but query sucessfull
      m_pDS->close();
      return true;
    }

    CVariant albumSongGenres(CVariant::VariantTypeArray);

    while (!m_pDS->eof())
    {
      CVariant genreObj;
      genreObj["title"] = m_pDS->fv("strGenre").get_asString();
      genreObj["genreid"] = m_pDS->fv("idGenre").get_asInt();
      albumSongGenres.push_back(genreObj);
      m_pDS->next();
    }
    m_pDS->close();

    item->SetProperty("songgenres", albumSongGenres);
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%i) failed", __FUNCTION__, idAlbum);
  }
  return false;
}

bool CMusicDatabase::GetGenresBySong(int idSong, std::vector<int>& genres)
{
  try
  {
    std::string strSQL = PrepareSQL("select idGenre from song_genre where idSong = %i ORDER BY iOrder ASC", idSong);
    if (!m_pDS->query(strSQL))
      return false;
    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      return true;
    }

    while (!m_pDS->eof())
    {
      genres.push_back(m_pDS->fv("idGenre").get_asInt());
      m_pDS->next();
    }
    m_pDS->close();

    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%i) failed", __FUNCTION__, idSong);
  }
  return false;
}

bool CMusicDatabase::GetIsAlbumArtist(int idArtist, CFileItem* item)
{
  try
  {
    int countalbum = strtol(GetSingleValue("album_artist", "count(idArtist)", PrepareSQL("idArtist=%i", idArtist)).c_str(), NULL, 10);
    CVariant IsAlbumArtistObj(CVariant::VariantTypeBoolean);
    IsAlbumArtistObj = (countalbum > 0);
    item->SetProperty("isalbumartist", IsAlbumArtistObj);
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%i) failed", __FUNCTION__, idArtist);
  }
  return false;
}


int CMusicDatabase::AddPath(const std::string& strPath1)
{
  std::string strSQL;
  try
  {
    std::string strPath(strPath1);
    if (!URIUtils::HasSlashAtEnd(strPath))
      URIUtils::AddSlashAtEnd(strPath);

    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    auto it = m_pathCache.find(strPath);
    if (it != m_pathCache.end())
      return it->second;

    strSQL=PrepareSQL( "select * from path where strPath='%s'", strPath.c_str());
    m_pDS->query(strSQL);
    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      // doesnt exists, add it
      strSQL=PrepareSQL("insert into path (idPath, strPath) values( NULL, '%s' )", strPath.c_str());
      m_pDS->exec(strSQL);

      int idPath = (int)m_pDS->lastinsertid();
      m_pathCache.insert(std::pair<std::string, int>(strPath, idPath));
      return idPath;
    }
    else
    {
      int idPath = m_pDS->fv("idPath").get_asInt();
      m_pathCache.insert(std::pair<std::string, int>(strPath, idPath));
      m_pDS->close();
      return idPath;
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "musicdatabase:unable to addpath (%s)", strSQL.c_str());
  }

  return -1;
}

CSong CMusicDatabase::GetSongFromDataset()
{
  return GetSongFromDataset(m_pDS->get_sql_record());
}

CSong CMusicDatabase::GetSongFromDataset(const dbiplus::sql_record* const record, int offset /* = 0 */)
{
  CSong song;
  song.idSong = record->at(offset + song_idSong).get_asInt();
  // Note this function does not populate artist credits, this must be done separately.
  // However artist names are held as a descriptive string
  song.strArtistDesc = record->at(offset + song_strArtists).get_asString();
  song.strArtistSort = record->at(offset + song_strArtistSort).get_asString();
  // Get the full genre string
  song.genre = StringUtils::Split(record->at(offset + song_strGenres).get_asString(), CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_musicItemSeparator);
  // and the rest...
  song.strAlbum = record->at(offset + song_strAlbum).get_asString();
  song.idAlbum = record->at(offset + song_idAlbum).get_asInt();
  song.iTrack = record->at(offset + song_iTrack).get_asInt() ;
  song.iDuration = record->at(offset + song_iDuration).get_asInt() ;
  song.iYear = record->at(offset + song_iYear).get_asInt() ;
  song.strTitle = record->at(offset + song_strTitle).get_asString();
  song.iTimesPlayed = record->at(offset + song_iTimesPlayed).get_asInt();
  song.lastPlayed.SetFromDBDateTime(record->at(offset + song_lastplayed).get_asString());
  song.dateAdded.SetFromDBDateTime(record->at(offset + song_dateAdded).get_asString());
  song.iStartOffset = record->at(offset + song_iStartOffset).get_asInt();
  song.iEndOffset = record->at(offset + song_iEndOffset).get_asInt();
  song.strMusicBrainzTrackID = record->at(offset + song_strMusicBrainzTrackID).get_asString();
  song.rating = record->at(offset + song_rating).get_asFloat();
  song.userrating = record->at(offset + song_userrating).get_asInt();
  song.votes = record->at(offset + song_votes).get_asInt();
  song.strComment = record->at(offset + song_comment).get_asString();
  song.strMood = record->at(offset + song_mood).get_asString();
  song.bCompilation = record->at(offset + song_bCompilation).get_asInt() == 1;
  // Replay gain data (needed for songs from cuesheets, both separate .cue files and embedded metadata)
  song.replayGain.Set(record->at(offset + song_strReplayGain).get_asString());
  // Get filename with full path
  song.strFileName = URIUtils::AddFileToFolder(record->at(offset + song_strPath).get_asString(), record->at(offset + song_strFileName).get_asString());
  return song;
}

void CMusicDatabase::GetFileItemFromDataset(CFileItem* item, const CMusicDbUrl &baseUrl)
{
  GetFileItemFromDataset(m_pDS->get_sql_record(), item, baseUrl);
}

void CMusicDatabase::GetFileItemFromDataset(const dbiplus::sql_record* const record, CFileItem* item, const CMusicDbUrl &baseUrl)
{
  // get the artist string from songview (not the song_artist and artist tables)
  item->GetMusicInfoTag()->SetArtistDesc(record->at(song_strArtists).get_asString());
  // get the artist sort name string from songview (not the song_artist and artist tables)
  item->GetMusicInfoTag()->SetArtistSort(record->at(song_strArtistSort).get_asString());
  // and the full genre string
  item->GetMusicInfoTag()->SetGenre(record->at(song_strGenres).get_asString());
  // and the rest...
  item->GetMusicInfoTag()->SetAlbum(record->at(song_strAlbum).get_asString());
  item->GetMusicInfoTag()->SetAlbumId(record->at(song_idAlbum).get_asInt());
  item->GetMusicInfoTag()->SetTrackAndDiscNumber(record->at(song_iTrack).get_asInt());
  item->GetMusicInfoTag()->SetDuration(record->at(song_iDuration).get_asInt());
  item->GetMusicInfoTag()->SetDatabaseId(record->at(song_idSong).get_asInt(), MediaTypeSong);
  SYSTEMTIME stTime;
  stTime.wYear = static_cast<unsigned short>(record->at(song_iYear).get_asInt());
  item->GetMusicInfoTag()->SetReleaseDate(stTime);
  item->GetMusicInfoTag()->SetTitle(record->at(song_strTitle).get_asString());
  item->SetLabel(record->at(song_strTitle).get_asString());
  item->m_lStartOffset = record->at(song_iStartOffset).get_asInt64();
  item->SetProperty("item_start", item->m_lStartOffset);
  item->m_lEndOffset = record->at(song_iEndOffset).get_asInt64();
  item->GetMusicInfoTag()->SetMusicBrainzTrackID(record->at(song_strMusicBrainzTrackID).get_asString());
  item->GetMusicInfoTag()->SetRating(record->at(song_rating).get_asFloat());
  item->GetMusicInfoTag()->SetUserrating(record->at(song_userrating).get_asInt());
  item->GetMusicInfoTag()->SetVotes(record->at(song_votes).get_asInt());
  item->GetMusicInfoTag()->SetComment(record->at(song_comment).get_asString());
  item->GetMusicInfoTag()->SetMood(record->at(song_mood).get_asString());
  item->GetMusicInfoTag()->SetPlayCount(record->at(song_iTimesPlayed).get_asInt());
  item->GetMusicInfoTag()->SetLastPlayed(record->at(song_lastplayed).get_asString());
  item->GetMusicInfoTag()->SetDateAdded(record->at(song_dateAdded).get_asString());
  std::string strRealPath = URIUtils::AddFileToFolder(record->at(song_strPath).get_asString(), record->at(song_strFileName).get_asString());
  item->GetMusicInfoTag()->SetURL(strRealPath);
  item->GetMusicInfoTag()->SetCompilation(record->at(song_bCompilation).get_asInt() == 1);
  // get the album artist string from songview (not the album_artist and artist tables)
  item->GetMusicInfoTag()->SetAlbumArtist(record->at(song_strAlbumArtists).get_asString());
  item->GetMusicInfoTag()->SetAlbumReleaseType(CAlbum::ReleaseTypeFromString(record->at(song_strAlbumReleaseType).get_asString()));
  // Replay gain data (needed for songs from cuesheets, both separate .cue files and embedded metadata)
  ReplayGain replaygain;
  replaygain.Set(record->at(song_strReplayGain).get_asString());
  item->GetMusicInfoTag()->SetReplayGain(replaygain);

  item->GetMusicInfoTag()->SetLoaded(true);
  // Get filename with full path
  if (!baseUrl.IsValid())
    item->SetPath(strRealPath);
  else
  {
    CMusicDbUrl itemUrl = baseUrl;
    std::string strFileName = record->at(song_strFileName).get_asString();
    std::string strExt = URIUtils::GetExtension(strFileName);
    std::string path = StringUtils::Format("%i%s", record->at(song_idSong).get_asInt(), strExt.c_str());
    itemUrl.AppendPath(path);
    item->SetPath(itemUrl.ToString());
    item->SetDynPath(strRealPath);
  }
}

void CMusicDatabase::GetFileItemFromArtistCredits(VECARTISTCREDITS& artistCredits, CFileItem* item)
{
  // Populate fileitem with artists from vector of artist credits
  std::vector<std::string> musicBrainzID;
  std::vector<std::string> songartists;
  CVariant artistidObj(CVariant::VariantTypeArray);

  // When "missing tag" artist, it is the only artist when present.
  if (artistCredits.begin()->GetArtistId() == BLANKARTIST_ID)
  {
    artistidObj.push_back((int)BLANKARTIST_ID);
    songartists.push_back(StringUtils::Empty);
  }
  else
  {
    for (const auto &artistCredit : artistCredits)
    {
      artistidObj.push_back(artistCredit.GetArtistId());
      songartists.push_back(artistCredit.GetArtist());
      if (!artistCredit.GetMusicBrainzArtistID().empty())
        musicBrainzID.push_back(artistCredit.GetMusicBrainzArtistID());
    }
  }
  item->GetMusicInfoTag()->SetArtist(songartists); // Also sets ArtistDesc if empty from song.strArtist field
  item->GetMusicInfoTag()->SetMusicBrainzArtistID(musicBrainzID);
  // Add album artistIds as separate property as not part of CMusicInfoTag
  item->SetProperty("artistid", artistidObj);
}

CAlbum CMusicDatabase::GetAlbumFromDataset(dbiplus::Dataset* pDS, int offset /* = 0 */, bool imageURL /* = false*/)
{
  return GetAlbumFromDataset(pDS->get_sql_record(), offset, imageURL);
}

CAlbum CMusicDatabase::GetAlbumFromDataset(const dbiplus::sql_record* const record, int offset /* = 0 */, bool imageURL /* = false*/)
{
  const std::string itemSeparator = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_musicItemSeparator;

  CAlbum album;
  album.idAlbum = record->at(offset + album_idAlbum).get_asInt();
  album.strAlbum = record->at(offset + album_strAlbum).get_asString();
  if (album.strAlbum.empty())
    album.strAlbum = g_localizeStrings.Get(1050);
  album.strMusicBrainzAlbumID = record->at(offset + album_strMusicBrainzAlbumID).get_asString();
  album.strReleaseGroupMBID = record->at(offset + album_strReleaseGroupMBID).get_asString();
  album.strArtistDesc = record->at(offset + album_strArtists).get_asString();
  album.strArtistSort = record->at(offset + album_strArtistSort).get_asString();
  album.genre = StringUtils::Split(record->at(offset + album_strGenres).get_asString(), itemSeparator);
  album.iYear = record->at(offset + album_iYear).get_asInt();
  if (imageURL)
    album.thumbURL.ParseString(record->at(offset + album_strThumbURL).get_asString());
  album.fRating = record->at(offset + album_fRating).get_asFloat();
  album.iUserrating = record->at(offset + album_iUserrating).get_asInt();
  album.iVotes = record->at(offset + album_iVotes).get_asInt();
  album.iYear = record->at(offset + album_iYear).get_asInt();
  album.strReview = record->at(offset + album_strReview).get_asString();
  album.styles = StringUtils::Split(record->at(offset + album_strStyles).get_asString(), itemSeparator);
  album.moods = StringUtils::Split(record->at(offset + album_strMoods).get_asString(), itemSeparator);
  album.themes = StringUtils::Split(record->at(offset + album_strThemes).get_asString(), itemSeparator);
  album.strLabel = record->at(offset + album_strLabel).get_asString();
  album.strType = record->at(offset + album_strType).get_asString();
  album.bCompilation = record->at(offset + album_bCompilation).get_asInt() == 1;
  album.bScrapedMBID = record->at(offset + album_bScrapedMBID).get_asInt() == 1;
  album.strLastScraped = record->at(offset + album_lastScraped).get_asString();
  album.iTimesPlayed = record->at(offset + album_iTimesPlayed).get_asInt();
  album.SetReleaseType(record->at(offset + album_strReleaseType).get_asString());
  album.SetDateAdded(record->at(offset + album_dtDateAdded).get_asString());
  album.SetLastPlayed(record->at(offset + album_dtLastPlayed).get_asString());
  return album;
}

CArtistCredit CMusicDatabase::GetArtistCreditFromDataset(const dbiplus::sql_record* const record, int offset /* = 0 */)
{
  CArtistCredit artistCredit;
  artistCredit.idArtist = record->at(offset + artistCredit_idArtist).get_asInt();
  if (artistCredit.idArtist == BLANKARTIST_ID)
    artistCredit.m_strArtist = StringUtils::Empty;
  else
  {
    artistCredit.m_strArtist = record->at(offset + artistCredit_strArtist).get_asString();
    artistCredit.m_strMusicBrainzArtistID = record->at(offset + artistCredit_strMusicBrainzArtistID).get_asString();
  }
  return artistCredit;
}

CMusicRole CMusicDatabase::GetArtistRoleFromDataset(const dbiplus::sql_record* const record, int offset /* = 0 */)
{
  CMusicRole ArtistRole(record->at(offset + artistCredit_idRole).get_asInt(),
                        record->at(offset + artistCredit_strRole).get_asString(),
                        record->at(offset + artistCredit_strArtist).get_asString(),
                        record->at(offset + artistCredit_idArtist).get_asInt());
  return ArtistRole;
}

CArtist CMusicDatabase::GetArtistFromDataset(dbiplus::Dataset* pDS, int offset /* = 0 */, bool needThumb /* = true */)
{
  return GetArtistFromDataset(pDS->get_sql_record(), offset, needThumb);
}

CArtist CMusicDatabase::GetArtistFromDataset(const dbiplus::sql_record* const record, int offset /* = 0 */, bool needThumb /* = true */)
{
  const std::string itemSeparator = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_musicItemSeparator;

  CArtist artist;
  artist.idArtist = record->at(offset + artist_idArtist).get_asInt();
  if (artist.idArtist == BLANKARTIST_ID && m_translateBlankArtist)
    artist.strArtist = g_localizeStrings.Get(38042);  //Missing artist tag in current language
  else
    artist.strArtist = record->at(offset + artist_strArtist).get_asString();
  artist.strSortName = record->at(offset + artist_strSortName).get_asString();
  artist.strMusicBrainzArtistID = record->at(offset + artist_strMusicBrainzArtistID).get_asString();
  artist.strType = record->at(offset + artist_strType).get_asString();
  artist.strGender = record->at(offset + artist_strGender).get_asString();
  artist.strDisambiguation = record->at(offset + artist_strDisambiguation).get_asString();
  artist.genre = StringUtils::Split(record->at(offset + artist_strGenres).get_asString(), itemSeparator);
  artist.strBiography = record->at(offset + artist_strBiography).get_asString();
  artist.styles = StringUtils::Split(record->at(offset + artist_strStyles).get_asString(), itemSeparator);
  artist.moods = StringUtils::Split(record->at(offset + artist_strMoods).get_asString(), itemSeparator);
  artist.strBorn = record->at(offset + artist_strBorn).get_asString();
  artist.strFormed = record->at(offset + artist_strFormed).get_asString();
  artist.strDied = record->at(offset + artist_strDied).get_asString();
  artist.strDisbanded = record->at(offset + artist_strDisbanded).get_asString();
  artist.yearsActive = StringUtils::Split(record->at(offset + artist_strYearsActive).get_asString(), itemSeparator);
  artist.instruments = StringUtils::Split(record->at(offset + artist_strInstruments).get_asString(), itemSeparator);
  artist.bScrapedMBID = record->at(offset + artist_bScrapedMBID).get_asInt() == 1;
  artist.strLastScraped = record->at(offset + artist_lastScraped).get_asString();
  artist.SetDateAdded(record->at(offset + artist_dtDateAdded).get_asString());

  if (needThumb)
  {
    artist.fanart.m_xml = record->at(artist_strFanart).get_asString();
    artist.fanart.Unpack();
    artist.thumbURL.ParseString(record->at(artist_strImage).get_asString());
  }

  return artist;
}

bool CMusicDatabase::GetSongByFileName(const std::string& strFileNameAndPath, CSong& song, int64_t startOffset)
{
  song.Clear();
  CURL url(strFileNameAndPath);

  if (url.IsProtocol("musicdb"))
  {
    std::string strFile = URIUtils::GetFileName(strFileNameAndPath);
    URIUtils::RemoveExtension(strFile);
    return GetSong(atol(strFile.c_str()), song);
  }

  if (NULL == m_pDB.get()) return false;
  if (NULL == m_pDS.get()) return false;

  std::string strPath, strFileName;
  SplitPath(strFileNameAndPath, strPath, strFileName);
  URIUtils::AddSlashAtEnd(strPath);

  std::string strSQL = PrepareSQL("select idSong from songview "
                                 "where strFileName='%s' and strPath='%s'",
                                 strFileName.c_str(), strPath.c_str());
  if (startOffset)
    strSQL += PrepareSQL(" AND iStartOffset=%" PRIi64, startOffset);

  int idSong = (int)strtol(GetSingleValue(strSQL).c_str(), NULL, 10);
  if (idSong > 0)
    return GetSong(idSong, song);

  return false;
}

int CMusicDatabase::GetAlbumIdByPath(const std::string& strPath)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    std::string strSQL = PrepareSQL("SELECT DISTINCT idAlbum FROM song JOIN path ON song.idPath = path.idPath WHERE path.strPath='%s'", strPath.c_str());
    // run query
    if (!m_pDS->query(strSQL)) return false;
    int iRowsFound = m_pDS->num_rows();

    int idAlbum = -1; // If no album is found, or more than one album is found then -1 is returned
    if (iRowsFound == 1)
      idAlbum = m_pDS->fv(0).get_asInt();

    m_pDS->close();

    return idAlbum;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%s) failed", __FUNCTION__, strPath.c_str());
  }

  return -1;
}

int CMusicDatabase::GetSongByArtistAndAlbumAndTitle(const std::string& strArtist, const std::string& strAlbum, const std::string& strTitle)
{
  try
  {
    std::string strSQL=PrepareSQL("select idSong from songview "
                                "where strArtists like '%s' and strAlbum like '%s' and "
                                "strTitle like '%s'",strArtist.c_str(),strAlbum.c_str(),strTitle.c_str());

    if (!m_pDS->query(strSQL)) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return -1;
    }
    int lResult = m_pDS->fv(0).get_asInt();
    m_pDS->close(); // cleanup recordset data
    return lResult;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s,%s,%s) failed", __FUNCTION__, strArtist.c_str(),strAlbum.c_str(),strTitle.c_str());
  }

  return -1;
}

bool CMusicDatabase::SearchArtists(const std::string& search, CFileItemList &artists)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    std::string strVariousArtists = g_localizeStrings.Get(340).c_str();
    std::string strSQL;
    if (search.size() >= MIN_FULL_SEARCH_LENGTH)
      strSQL=PrepareSQL("select * from artist "
                                "where (strArtist like '%s%%' or strArtist like '%% %s%%') and strArtist <> '%s' "
                                , search.c_str(), search.c_str(), strVariousArtists.c_str() );
    else
      strSQL=PrepareSQL("select * from artist "
                                "where strArtist like '%s%%' and strArtist <> '%s' "
                                , search.c_str(), strVariousArtists.c_str() );

    if (!m_pDS->query(strSQL)) return false;
    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      return false;
    }

    std::string artistLabel(g_localizeStrings.Get(557)); // Artist
    while (!m_pDS->eof())
    {
      std::string path = StringUtils::Format("musicdb://artists/%i/", m_pDS->fv(0).get_asInt());
      CFileItemPtr pItem(new CFileItem(path, true));
      std::string label = StringUtils::Format("[%s] %s", artistLabel.c_str(), m_pDS->fv(1).get_asString().c_str());
      pItem->SetLabel(label);
      label = StringUtils::Format("A %s", m_pDS->fv(1).get_asString().c_str()); // sort label is stored in the title tag
      pItem->GetMusicInfoTag()->SetTitle(label);
      pItem->GetMusicInfoTag()->SetDatabaseId(m_pDS->fv(0).get_asInt(), MediaTypeArtist);
      artists.Add(pItem);
      m_pDS->next();
    }

    m_pDS->close(); // cleanup recordset data
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }

  return false;
}

bool CMusicDatabase::GetTop100(const std::string& strBaseDir, CFileItemList& items)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CMusicDbUrl baseUrl;
    if (!strBaseDir.empty() && !baseUrl.FromString(strBaseDir))
      return false;

    std::string strSQL="select * from songview "
                      "where iTimesPlayed>0 "
                      "order by iTimesPlayed desc "
                      "limit 100";

    CLog::Log(LOGDEBUG, "%s query: %s", __FUNCTION__, strSQL.c_str());
    if (!m_pDS->query(strSQL)) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return true;
    }
    items.Reserve(iRowsFound);
    while (!m_pDS->eof())
    {
      CFileItemPtr item(new CFileItem);
      GetFileItemFromDataset(item.get(), baseUrl);
      items.Add(item);
      m_pDS->next();
    }

    m_pDS->close(); // cleanup recordset data
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }

  return false;
}

bool CMusicDatabase::GetTop100Albums(VECALBUMS& albums)
{
  try
  {
    albums.erase(albums.begin(), albums.end());
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    // Get data from album and album_artist tables to fully populate albums
    std::string strSQL = "SELECT albumview.*, albumartistview.* FROM albumview "
      "JOIN albumartistview ON albumview.idAlbum = albumartistview.idAlbum "
      "WHERE albumartistview.idAlbum in "
      "(SELECT albumview.idAlbum FROM albumview "
      "WHERE albumview.strAlbum != '' AND albumview.iTimesPlayed>0 "
      "ORDER BY albumview.iTimesPlayed DESC LIMIT 100) "
      "ORDER BY albumview.iTimesPlayed DESC, albumartistview.iOrder";

    CLog::Log(LOGDEBUG, "%s query: %s", __FUNCTION__, strSQL.c_str());
    if (!m_pDS->query(strSQL)) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return true;
    }

    int albumArtistOffset = album_enumCount;
    int albumId = -1;
    while (!m_pDS->eof())
    {
      const dbiplus::sql_record* const record = m_pDS->get_sql_record();

      if (albumId != record->at(album_idAlbum).get_asInt())
      { // New album
        albumId = record->at(album_idAlbum).get_asInt();
        albums.push_back(GetAlbumFromDataset(record));
      }
      // Get album artists
      albums.back().artistCredits.push_back(GetArtistCreditFromDataset(record, albumArtistOffset));

      m_pDS->next();
    }

    m_pDS->close(); // cleanup recordset data
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }

  return false;
}

bool CMusicDatabase::GetTop100AlbumSongs(const std::string& strBaseDir, CFileItemList& items)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CMusicDbUrl baseUrl;
    if (!strBaseDir.empty() && baseUrl.FromString(strBaseDir))
      return false;

    std::string strSQL = StringUtils::Format("SELECT songview.*, albumview.* FROM songview JOIN albumview ON (songview.idAlbum = albumview.idAlbum) JOIN (SELECT song.idAlbum, SUM(song.iTimesPlayed) AS iTimesPlayedSum FROM song WHERE song.iTimesPlayed > 0 GROUP BY idAlbum ORDER BY iTimesPlayedSum DESC LIMIT 100) AS _albumlimit ON (songview.idAlbum = _albumlimit.idAlbum) ORDER BY _albumlimit.iTimesPlayedSum DESC");
    CLog::Log(LOGDEBUG,"GetTop100AlbumSongs() query: %s", strSQL.c_str());
    if (!m_pDS->query(strSQL)) return false;

    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return true;
    }

    // get data from returned rows
    items.Reserve(iRowsFound);
    while (!m_pDS->eof())
    {
      CFileItemPtr item(new CFileItem);
      GetFileItemFromDataset(item.get(), baseUrl);
      items.Add(item);
      m_pDS->next();
    }

    // cleanup
    m_pDS->close();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}

bool CMusicDatabase::GetRecentlyPlayedAlbums(VECALBUMS& albums)
{
  try
  {
    albums.erase(albums.begin(), albums.end());
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    // Get data from album and album_artist tables to fully populate albums
    std::string strSQL = PrepareSQL("SELECT albumview.*, albumartistview.* FROM "
      "(SELECT idAlbum FROM albumview WHERE albumview.lastplayed IS NOT NULL "
      "AND albumview.strReleaseType = '%s' "
      "ORDER BY albumview.lastplayed DESC LIMIT %u) as playedalbums "
      "JOIN albumview ON albumview.idAlbum = playedalbums.idAlbum "
      "JOIN albumartistview ON albumview.idAlbum = albumartistview.idAlbum "
      "ORDER BY albumview.lastplayed DESC, albumartistview.iorder ",
      CAlbum::ReleaseTypeToString(CAlbum::Album).c_str(), RECENTLY_PLAYED_LIMIT);

    CLog::Log(LOGDEBUG, "%s query: %s", __FUNCTION__, strSQL.c_str());
    if (!m_pDS->query(strSQL)) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return true;
    }

    int albumArtistOffset = album_enumCount;
    int albumId = -1;
    while (!m_pDS->eof())
    {
      const dbiplus::sql_record* const record = m_pDS->get_sql_record();

      if (albumId != record->at(album_idAlbum).get_asInt())
      { // New album
        albumId = record->at(album_idAlbum).get_asInt();
        albums.push_back(GetAlbumFromDataset(record));
      }
      // Get album artists
      albums.back().artistCredits.push_back(GetArtistCreditFromDataset(record, albumArtistOffset));

      m_pDS->next();
    }
    m_pDS->close(); // cleanup recordset data
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }

  return false;
}

bool CMusicDatabase::GetRecentlyPlayedAlbumSongs(const std::string& strBaseDir, CFileItemList& items)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CMusicDbUrl baseUrl;
    if (!strBaseDir.empty() && !baseUrl.FromString(strBaseDir))
      return false;

    std::string strSQL = PrepareSQL("SELECT songview.*, songartistview.* FROM "
      "(SELECT idAlbum, lastPlayed FROM albumview WHERE albumview.lastplayed IS NOT NULL "
      "ORDER BY albumview.lastplayed DESC LIMIT %u) as playedalbums "
      "JOIN songview ON songview.idAlbum = playedalbums.idAlbum "
      "JOIN songartistview ON songview.idSong = songartistview.idSong "
      "ORDER BY playedalbums.lastplayed DESC,songartistview.idsong, songartistview.idRole, songartistview.iOrder",
      CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_iMusicLibraryRecentlyAddedItems);
    CLog::Log(LOGDEBUG,"GetRecentlyPlayedAlbumSongs() query: %s", strSQL.c_str());
    if (!m_pDS->query(strSQL)) return false;

    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return true;
    }

    // Needs a separate query to determine number of songs to set items size.
    // Get songs from returned rows. Join means there is a row for every song artist
    // Gather artist credits, rather than append to item as go along, so can return array of artistIDs too
    int songArtistOffset = song_enumCount;
    int songId = -1;
    VECARTISTCREDITS artistCredits;
    while (!m_pDS->eof())
    {
      const dbiplus::sql_record* const record = m_pDS->get_sql_record();

      int idSongArtistRole = record->at(songArtistOffset + artistCredit_idRole).get_asInt();
      if (songId != record->at(song_idSong).get_asInt())
      { //New song
        if (songId > 0 && !artistCredits.empty())
        {
          //Store artist credits for previous song
          GetFileItemFromArtistCredits(artistCredits, items[items.Size() - 1].get());
          artistCredits.clear();
        }
        songId = record->at(song_idSong).get_asInt();
        CFileItemPtr item(new CFileItem);
        GetFileItemFromDataset(record, item.get(), baseUrl);
        items.Add(item);
      }
      // Get song artist credits and contributors
      if (idSongArtistRole == ROLE_ARTIST)
        artistCredits.push_back(GetArtistCreditFromDataset(record, songArtistOffset));
      else
        items[items.Size() - 1]->GetMusicInfoTag()->AppendArtistRole(GetArtistRoleFromDataset(record, songArtistOffset));

      m_pDS->next();
    }
    if (!artistCredits.empty())
    {
      //Store artist credits for final song
      GetFileItemFromArtistCredits(artistCredits, items[items.Size() - 1].get());
      artistCredits.clear();
    }

    // cleanup
    m_pDS->close();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}

bool CMusicDatabase::GetRecentlyAddedAlbums(VECALBUMS& albums, unsigned int limit)
{
  try
  {
    albums.erase(albums.begin(), albums.end());
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    // Get data from album and album_artist tables to fully populate albums
    // Use idAlbum to determine the recently added albums
    // (not "dateAdded" as this is file time stamp and nothing to do with when albums added to library)
    std::string strSQL = PrepareSQL("SELECT albumview.*, albumartistview.* FROM "
      "(SELECT idAlbum FROM album WHERE strAlbum != '' ORDER BY idAlbum DESC LIMIT %u) AS recentalbums "
      "JOIN albumview ON albumview.idAlbum = recentalbums.idAlbum "
      "JOIN albumartistview ON albumview.idAlbum = albumartistview.idAlbum "
      "ORDER BY albumview.idAlbum desc, albumartistview.iOrder ",
       limit ? limit : CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_iMusicLibraryRecentlyAddedItems);

    CLog::Log(LOGDEBUG, "%s query: %s", __FUNCTION__, strSQL.c_str());
    if (!m_pDS->query(strSQL)) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return true;
    }

    int albumArtistOffset = album_enumCount;
    int albumId = -1;
    while (!m_pDS->eof())
    {
      const dbiplus::sql_record* const record = m_pDS->get_sql_record();

      if (albumId != record->at(album_idAlbum).get_asInt())
      { // New album
        albumId = record->at(album_idAlbum).get_asInt();
        albums.push_back(GetAlbumFromDataset(record));
      }
      // Get album artists
      albums.back().artistCredits.push_back(GetArtistCreditFromDataset(record, albumArtistOffset));

      m_pDS->next();
    }
    m_pDS->close(); // cleanup recordset data
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }

  return false;
}

bool CMusicDatabase::GetRecentlyAddedAlbumSongs(const std::string& strBaseDir, CFileItemList& items, unsigned int limit)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CMusicDbUrl baseUrl;
    if (!strBaseDir.empty() && !baseUrl.FromString(strBaseDir))
      return false;

    // Get data from song and song_artist tables to fully populate songs
    // Use idAlbum to determine the recently added albums
    // (not "dateAdded" as this is file time stamp and nothing to do with when albums added to library)
    std::string strSQL;
    strSQL = PrepareSQL("SELECT songview.*, songartistview.* FROM "
        "(SELECT idAlbum FROM album ORDER BY idAlbum DESC LIMIT %u) AS recentalbums "
        "JOIN songview ON songview.idAlbum = recentalbums.idAlbum "
        "JOIN songartistview ON songview.idSong = songartistview.idSong "
        "ORDER BY songview.idAlbum DESC, songview.idSong, songartistview.idRole, songartistview.iOrder ",
        limit ? limit : CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_iMusicLibraryRecentlyAddedItems);
    CLog::Log(LOGDEBUG,"GetRecentlyAddedAlbumSongs() query: %s", strSQL.c_str());
    if (!m_pDS->query(strSQL)) return false;

    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return true;
    }

    // Needs a separate query to determine number of songs to set items size.
    // Get songs from returned rows. Join means there is a row for every song artist
    int songArtistOffset = song_enumCount;
    int songId = -1;
    VECARTISTCREDITS artistCredits;
    while (!m_pDS->eof())
    {
      const dbiplus::sql_record* const record = m_pDS->get_sql_record();

      int idSongArtistRole = record->at(songArtistOffset + artistCredit_idRole).get_asInt();
      if (songId != record->at(song_idSong).get_asInt())
      { //New song
        if (songId > 0 && !artistCredits.empty())
        {
          //Store artist credits for previous song
          GetFileItemFromArtistCredits(artistCredits, items[items.Size() - 1].get());
          artistCredits.clear();
        }
        songId = record->at(song_idSong).get_asInt();
        CFileItemPtr item(new CFileItem);
        GetFileItemFromDataset(record, item.get(), baseUrl);
        items.Add(item);
      }
      // Get song artist credits and contributors
      if (idSongArtistRole == ROLE_ARTIST)
        artistCredits.push_back(GetArtistCreditFromDataset(record, songArtistOffset));
      else
        items[items.Size() - 1]->GetMusicInfoTag()->AppendArtistRole(GetArtistRoleFromDataset(record, songArtistOffset));

      m_pDS->next();
    }
    if (!artistCredits.empty())
    {
      //Store artist credits for final song
      GetFileItemFromArtistCredits(artistCredits, items[items.Size() - 1].get());
      artistCredits.clear();
    }

    // cleanup
    m_pDS->close();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}

void CMusicDatabase::IncrementPlayCount(const CFileItem& item)
{
  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    int idSong = GetSongIDFromPath(item.GetPath());

    std::string sql=PrepareSQL("UPDATE song SET iTimesPlayed=iTimesPlayed+1, lastplayed=CURRENT_TIMESTAMP where idSong=%i", idSong);
    m_pDS->exec(sql);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%s) failed", __FUNCTION__, item.GetPath().c_str());
  }
}

bool CMusicDatabase::GetSongsByPath(const std::string& strPath1, MAPSONGS& songs, bool bAppendToMap)
{
  std::string strPath(strPath1);
  try
  {
    if (!URIUtils::HasSlashAtEnd(strPath))
      URIUtils::AddSlashAtEnd(strPath);

    if (!bAppendToMap)
      songs.clear();

    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    std::string strSQL=PrepareSQL("SELECT * FROM songview WHERE strPath='%s'", strPath.c_str());
    if (!m_pDS->query(strSQL)) return false;
    CLog::Log(LOGDEBUG, "%s query: %s", __FUNCTION__, strSQL.c_str());
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return false;
    }
    while (!m_pDS->eof())
    {
      CSong song = GetSongFromDataset();
      // For songs from cue sheets strFileName is not unique, so only 1st song gets added to song map
      songs.insert(std::make_pair(song.strFileName, song));
      m_pDS->next();
    }
    m_pDS->close(); // cleanup recordset data
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%s) failed", __FUNCTION__, strPath.c_str());
  }

  return false;
}

void CMusicDatabase::EmptyCache()
{
  m_genreCache.erase(m_genreCache.begin(), m_genreCache.end());
  m_pathCache.erase(m_pathCache.begin(), m_pathCache.end());
}

bool CMusicDatabase::Search(const std::string& search, CFileItemList &items)
{
  unsigned int time = XbmcThreads::SystemClockMillis();
  // first grab all the artists that match
  SearchArtists(search, items);
  CLog::Log(LOGDEBUG, "%s Artist search in %i ms",
            __FUNCTION__, XbmcThreads::SystemClockMillis() - time); time = XbmcThreads::SystemClockMillis();

  // then albums that match
  SearchAlbums(search, items);
  CLog::Log(LOGDEBUG, "%s Album search in %i ms",
            __FUNCTION__, XbmcThreads::SystemClockMillis() - time); time = XbmcThreads::SystemClockMillis();

  // and finally songs
  SearchSongs(search, items);
  CLog::Log(LOGDEBUG, "%s Songs search in %i ms",
            __FUNCTION__, XbmcThreads::SystemClockMillis() - time); time = XbmcThreads::SystemClockMillis();
  return true;
}

bool CMusicDatabase::SearchSongs(const std::string& search, CFileItemList &items)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CMusicDbUrl baseUrl;
    if (!baseUrl.FromString("musicdb://songs/"))
      return false;

    std::string strSQL;
    if (search.size() >= MIN_FULL_SEARCH_LENGTH)
      strSQL=PrepareSQL("select * from songview where strTitle like '%s%%' or strTitle like '%% %s%%' limit 1000", search.c_str(), search.c_str());
    else
      strSQL=PrepareSQL("select * from songview where strTitle like '%s%%' limit 1000", search.c_str());

    if (!m_pDS->query(strSQL)) return false;
    if (m_pDS->num_rows() == 0) return false;

    std::string songLabel = g_localizeStrings.Get(179); // Song
    while (!m_pDS->eof())
    {
      CFileItemPtr item(new CFileItem);
      GetFileItemFromDataset(item.get(), baseUrl);
      items.Add(item);
      m_pDS->next();
    }

    m_pDS->close();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }

  return false;
}

bool CMusicDatabase::SearchAlbums(const std::string& search, CFileItemList &albums)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    std::string strSQL;
    if (search.size() >= MIN_FULL_SEARCH_LENGTH)
      strSQL=PrepareSQL("select * from albumview where strAlbum like '%s%%' or strAlbum like '%% %s%%'", search.c_str(), search.c_str());
    else
      strSQL=PrepareSQL("select * from albumview where strAlbum like '%s%%'", search.c_str());

    if (!m_pDS->query(strSQL)) return false;

    std::string albumLabel(g_localizeStrings.Get(558)); // Album
    while (!m_pDS->eof())
    {
      CAlbum album = GetAlbumFromDataset(m_pDS.get());
      std::string path = StringUtils::Format("musicdb://albums/%ld/", album.idAlbum);
      CFileItemPtr pItem(new CFileItem(path, album));
      std::string label = StringUtils::Format("[%s] %s", albumLabel.c_str(), album.strAlbum.c_str());
      pItem->SetLabel(label);
      label = StringUtils::Format("B %s", album.strAlbum.c_str()); // sort label is stored in the title tag
      pItem->GetMusicInfoTag()->SetTitle(label);
      albums.Add(pItem);
      m_pDS->next();
    }
    m_pDS->close(); // cleanup recordset data
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}

bool CMusicDatabase::CleanupSongsByIds(const std::string &strSongIds)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    // ok, now find all idSong's
    std::string strSQL=PrepareSQL("select * from song join path on song.idPath = path.idPath where song.idSong in %s", strSongIds.c_str());
    if (!m_pDS->query(strSQL)) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return true;
    }
    std::vector<std::string> songsToDelete;
    while (!m_pDS->eof())
    { // get the full song path
      std::string strFileName = URIUtils::AddFileToFolder(m_pDS->fv("path.strPath").get_asString(), m_pDS->fv("song.strFileName").get_asString());

      //  Special case for streams inside an ogg file. (oggstream)
      //  The last dir in the path is the ogg file that
      //  contains the stream, so test if its there
      if (URIUtils::HasExtension(strFileName, ".oggstream|.nsfstream"))
      {
        strFileName = URIUtils::GetDirectory(strFileName);
        // we are dropping back to a file, so remove the slash at end
        URIUtils::RemoveSlashAtEnd(strFileName);
      }

      if (!CFile::Exists(strFileName, false))
      { // file no longer exists, so add to deletion list
        songsToDelete.push_back(m_pDS->fv("song.idSong").get_asString());
      }
      m_pDS->next();
    }
    m_pDS->close();

    if (!songsToDelete.empty())
    {
      std::string strSongsToDelete = "(" + StringUtils::Join(songsToDelete, ",") + ")";
      // ok, now delete these songs + all references to them from the linked tables
      strSQL = "delete from song where idSong in " + strSongsToDelete;
      m_pDS->exec(strSQL);
      m_pDS->close();
    }
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "Exception in CMusicDatabase::CleanupSongsFromPaths()");
  }
  return false;
}

bool CMusicDatabase::CleanupSongs(CGUIDialogProgress* progressDialog /*= nullptr*/)
{
  try
  {

    int total;
    // Count total number of songs
    total = (int)strtol(GetSingleValue("SELECT COUNT(1) FROM song", m_pDS).c_str(), nullptr, 10);
    // No songs to clean
    if (total == 0)
      return true;

    // run through all songs and get all unique path ids
    int iLIMIT = 1000;
    for (int i=0;;i+=iLIMIT)
    {
      std::string strSQL=PrepareSQL("select song.idSong from song order by song.idSong limit %i offset %i",iLIMIT,i);
      if (!m_pDS->query(strSQL)) return false;
      int iRowsFound = m_pDS->num_rows();
      // keep going until no rows are left!
      if (iRowsFound == 0)
      {
        m_pDS->close();
        return true;
      }

      std::vector<std::string> songIds;
      while (!m_pDS->eof())
      {
        songIds.push_back(m_pDS->fv("song.idSong").get_asString());
        m_pDS->next();
      }
      m_pDS->close();
      std::string strSongIds = "(" + StringUtils::Join(songIds, ",") + ")";
      CLog::Log(LOGDEBUG,"Checking songs from song ID list: %s",strSongIds.c_str());
      if (progressDialog)
      {
        int percentage = i * 100 / total;
        if (percentage > progressDialog->GetPercentage())
        {
          progressDialog->SetPercentage(percentage);
          progressDialog->Progress();
        }
        if (progressDialog->IsCanceled())
        {
          m_pDS->close();
          return false;
        }
      }
      if (!CleanupSongsByIds(strSongIds)) return false;
    }
    return true;
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "Exception in CMusicDatabase::CleanupSongs()");
  }
  return false;
}

bool CMusicDatabase::CleanupAlbums()
{
  try
  {
    // This must be run AFTER songs have been cleaned up
    // delete albums with no reference to songs
    std::string strSQL = "select * from album where album.idAlbum not in (select idAlbum from song)";
    if (!m_pDS->query(strSQL)) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return true;
    }

    std::vector<std::string> albumIds;
    while (!m_pDS->eof())
    {
      albumIds.push_back(m_pDS->fv("album.idAlbum").get_asString());
      m_pDS->next();
    }
    m_pDS->close();

    std::string strAlbumIds = "(" + StringUtils::Join(albumIds, ",") + ")";
    // ok, now we can delete them and the references in the linked tables
    strSQL = "delete from album where idAlbum in " + strAlbumIds;
    m_pDS->exec(strSQL);
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "Exception in CMusicDatabase::CleanupAlbums()");
  }
  return false;
}

bool CMusicDatabase::CleanupPaths()
{
  try
  {
    // needs to be done AFTER the songs and albums have been cleaned up.
    // we can happily delete any path that has no reference to a song
    // but we must keep all paths that have been scanned that may contain songs in subpaths

    // first create a temporary table of song paths
    m_pDS->exec("CREATE TEMPORARY TABLE songpaths (idPath integer, strPath varchar(512))\n");
    m_pDS->exec("INSERT INTO songpaths select idPath,strPath from path where idPath in (select idPath from song)\n");

    // grab all paths that aren't immediately connected with a song
    std::string sql = "select * from path where idPath not in (select idPath from song)";
    if (!m_pDS->query(sql)) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return true;
    }
    // and construct a list to delete
    std::vector<std::string> pathIds;
    while (!m_pDS->eof())
    {
      // anything that isn't a parent path of a song path is to be deleted
      std::string path = m_pDS->fv("strPath").get_asString();
      std::string sql = PrepareSQL("select count(idPath) from songpaths where SUBSTR(strPath,1,%i)='%s'", StringUtils::utf8_strlen(path.c_str()), path.c_str());
      if (m_pDS2->query(sql) && m_pDS2->num_rows() == 1 && m_pDS2->fv(0).get_asInt() == 0)
        pathIds.push_back(m_pDS->fv("idPath").get_asString()); // nothing found, so delete
      m_pDS2->close();
      m_pDS->next();
    }
    m_pDS->close();

    if (!pathIds.empty())
    {
      // do the deletion, and drop our temp table
      std::string deleteSQL = "DELETE FROM path WHERE idPath IN (" + StringUtils::Join(pathIds, ",") + ")";
      m_pDS->exec(deleteSQL);
    }
    m_pDS->exec("drop table songpaths");
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "Exception in CMusicDatabase::CleanupPaths() or was aborted");
  }
  return false;
}

bool CMusicDatabase::InsideScannedPath(const std::string& path)
{
  std::string sql = PrepareSQL("select idPath from path where SUBSTR(strPath,1,%i)='%s' LIMIT 1", path.size(), path.c_str());
  return !GetSingleValue(sql).empty();
}

bool CMusicDatabase::CleanupArtists()
{
  try
  {
    // (nested queries by Bobbin007)
    // must be executed AFTER the song, album and their artist link tables are cleaned.
    // Don't delete [Missing] the missing artist tag artist

    // Create temp table to avoid 1442 trigger hell on mysql
    m_pDS->exec("CREATE TEMPORARY TABLE tmp_delartists (idArtist integer)");
    m_pDS->exec("INSERT INTO tmp_delartists select idArtist from song_artist");
    m_pDS->exec("INSERT INTO tmp_delartists select idArtist from album_artist");
    m_pDS->exec(PrepareSQL("INSERT INTO tmp_delartists VALUES(%i)", BLANKARTIST_ID));
    // tmp_delartists contains duplicate ids, and on a large library with small changes can be very large.
    // To avoid MySQL hanging or timeout create a table of unique ids with primary key
    m_pDS->exec("CREATE TEMPORARY TABLE tmp_keep (idArtist INTEGER PRIMARY KEY)");
    m_pDS->exec("INSERT INTO tmp_keep SELECT DISTINCT idArtist from tmp_delartists");
    m_pDS->exec("DELETE FROM artist WHERE idArtist NOT IN (SELECT idArtist FROM tmp_keep)");
    // Tidy up temp tables
    m_pDS->exec("DROP TABLE tmp_delartists");
    m_pDS->exec("DROP TABLE tmp_keep");

    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "Exception in CMusicDatabase::CleanupArtists() or was aborted");
  }
  return false;
}

bool CMusicDatabase::CleanupGenres()
{
  try
  {
    // Cleanup orphaned song genres (ie those that don't belong to a song entry)
    // (nested queries by Bobbin007)
    // Must be executed AFTER the song, and song_genre have been cleaned.
    std::string strSQL = "DELETE FROM genre WHERE idGenre NOT IN (SELECT idGenre FROM song_genre)";
    m_pDS->exec(strSQL);
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "Exception in CMusicDatabase::CleanupGenres() or was aborted");
  }
  return false;
}

bool CMusicDatabase::CleanupInfoSettings()
{
  try
  {
    // Cleanup orphaned info settings (ie those that don't belong to an album or artist entry)
    // Must be executed AFTER the album and artist tables have been cleaned.
    std::string strSQL = "DELETE FROM infosetting WHERE idSetting NOT IN (SELECT idInfoSetting FROM artist) "
      "AND idSetting NOT IN (SELECT idInfoSetting FROM album)";
    m_pDS->exec(strSQL);
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "Exception in CMusicDatabase::CleanupInfoSettings() or was aborted");
  }
  return false;
}

bool CMusicDatabase::CleanupRoles()
{
  try
  {
    // Cleanup orphaned roles (ie those that don't belong to a song entry)
    // Must be executed AFTER the song, and song_artist tables have been cleaned.
    // Do not remove default role (ROLE_ARTIST)
    std::string strSQL = "DELETE FROM role WHERE idRole > 1 AND idRole NOT IN (SELECT idRole FROM song_artist)";
    m_pDS->exec(strSQL);
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "Exception in CMusicDatabase::CleanupRoles() or was aborted");
  }
  return false;
}

bool CMusicDatabase::CleanupOrphanedItems()
{
  // paths aren't cleaned up here - they're cleaned up in RemoveSongsFromPath()
  if (NULL == m_pDB.get()) return false;
  if (NULL == m_pDS.get()) return false;
  SetLibraryLastUpdated();
  if (!CleanupAlbums()) return false;
  if (!CleanupArtists()) return false;
  if (!CleanupGenres()) return false;
  if (!CleanupRoles()) return false;
  if (!CleanupInfoSettings()) return false;
  return true;
}

int CMusicDatabase::Cleanup(CGUIDialogProgress* progressDialog /*= nullptr*/)
{
  if (NULL == m_pDB.get()) return ERROR_DATABASE;
  if (NULL == m_pDS.get()) return ERROR_DATABASE;

  int ret = ERROR_OK;
  unsigned int time = XbmcThreads::SystemClockMillis();
  CLog::Log(LOGNOTICE, "%s: Starting musicdatabase cleanup ..", __FUNCTION__);
  CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::AudioLibrary, "xbmc", "OnCleanStarted");

  // first cleanup any songs with invalid paths
  if (progressDialog)
  {
    progressDialog->SetLine(1, CVariant{318});
    progressDialog->SetLine(2, CVariant{330});
    progressDialog->SetPercentage(0);
    progressDialog->Progress();
  }
  if (!CleanupSongs(progressDialog))
  {
    ret = ERROR_REORG_SONGS;
    goto error;
  }
  // then the albums that are not linked to a song or to album, or whose path is removed
  if (progressDialog)
  {
    progressDialog->SetLine(1, CVariant{326});
    progressDialog->SetPercentage(20);
    progressDialog->Progress();
    if (progressDialog->IsCanceled())
    {
      ret = ERROR_CANCEL;
      goto error;
    }
  }
  if (!CleanupAlbums())
  {
    ret = ERROR_REORG_ALBUM;
    goto error;
  }
  // now the paths
  if (progressDialog)
  {
    progressDialog->SetLine(1, CVariant{324});
    progressDialog->SetPercentage(40);
    progressDialog->Progress();
    if (progressDialog->IsCanceled())
    {
      ret = ERROR_CANCEL;
      goto error;
    }
  }
  if (!CleanupPaths())
  {
    ret = ERROR_REORG_PATH;
    goto error;
  }
  // and finally artists + genres
  if (progressDialog)
  {
    progressDialog->SetLine(1, CVariant{320});
    progressDialog->SetPercentage(60);
    progressDialog->Progress();
    if (progressDialog->IsCanceled())
    {
      ret = ERROR_CANCEL;
      goto error;
    }
  }
  if (!CleanupArtists())
  {
    ret = ERROR_REORG_ARTIST;
    goto error;
  }
  //Genres, roles and info settings progess in one step
  if (progressDialog)
  {
    progressDialog->SetLine(1, CVariant{322});
    progressDialog->SetPercentage(80);
    progressDialog->Progress();
    if (progressDialog->IsCanceled())
    {
      ret = ERROR_CANCEL;
      goto error;
    }
  }
  if (!CleanupGenres())
  {
    ret = ERROR_REORG_OTHER;
    goto error;
  }
  if (!CleanupRoles())
  {
    ret = ERROR_REORG_OTHER;
    goto error;
  }
  if (!CleanupInfoSettings())
  {
    ret = ERROR_REORG_OTHER;
    goto error;
  }
  // commit transaction
  if (progressDialog)
  {
    progressDialog->SetLine(1, CVariant{328});
    progressDialog->SetPercentage(90);
    progressDialog->Progress();
    if (progressDialog->IsCanceled())
    {
      ret = ERROR_CANCEL;
      goto error;
    }
  }
  if (!CommitTransaction())
  {
    ret = ERROR_WRITING_CHANGES;
    goto error;
  }
  // and compress the database
  if (progressDialog)
  {
    progressDialog->SetLine(1, CVariant{331});
    progressDialog->SetPercentage(100);
    progressDialog->Close();
  }
  time = XbmcThreads::SystemClockMillis() - time;
  CLog::Log(LOGNOTICE, "%s: Cleaning musicdatabase done. Operation took %s", __FUNCTION__, StringUtils::SecondsToTimeString(time / 1000).c_str());
  CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::AudioLibrary, "xbmc", "OnCleanFinished");

  if (!Compress(false))
  {
    return ERROR_COMPRESSING;
  }
  return ERROR_OK;

error:
  RollbackTransaction();
  CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::AudioLibrary, "xbmc", "OnCleanFinished");
  return ret;
}

bool CMusicDatabase::LookupCDDBInfo(bool bRequery/*=false*/)
{
#ifdef HAS_DVD_DRIVE
  if (!CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_AUDIOCDS_USECDDB))
    return false;

  // check network connectivity
  if (!CServiceBroker::GetNetwork().IsAvailable())
    return false;

  // Get information for the inserted disc
  CCdInfo* pCdInfo = g_mediaManager.GetCdInfo();
  if (pCdInfo == NULL)
    return false;

  // If the disc has no tracks, we are finished here.
  int nTracks = pCdInfo->GetTrackCount();
  if (nTracks <= 0)
    return false;

  //  Delete old info if any
  if (bRequery)
  {
    std::string strFile = StringUtils::Format("%x.cddb", pCdInfo->GetCddbDiscId());
    CFile::Delete(URIUtils::AddFileToFolder(m_profileManager.GetCDDBFolder(), strFile));
  }

  // Prepare cddb
  Xcddb cddb;
  cddb.setCacheDir(m_profileManager.GetCDDBFolder());

  // Do we have to look for cddb information
  if (pCdInfo->HasCDDBInfo() && !cddb.isCDCached(pCdInfo))
  {
    CGUIDialogProgress* pDialogProgress = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogProgress>(WINDOW_DIALOG_PROGRESS);
    CGUIDialogSelect *pDlgSelect = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(WINDOW_DIALOG_SELECT);

    if (!pDialogProgress) return false;
    if (!pDlgSelect) return false;

    // Show progress dialog if we have to connect to freedb.org
    pDialogProgress->SetHeading(CVariant{255}); //CDDB
    pDialogProgress->SetLine(0, CVariant{""}); // Querying freedb for CDDB info
    pDialogProgress->SetLine(1, CVariant{256});
    pDialogProgress->SetLine(2, CVariant{""});
    pDialogProgress->ShowProgressBar(false);
    pDialogProgress->Open();

    // get cddb information
    if (!cddb.queryCDinfo(pCdInfo))
    {
      pDialogProgress->Close();
      int lasterror = cddb.getLastError();

      // Have we found more then on match in cddb for this disc,...
      if (lasterror == E_WAIT_FOR_INPUT)
      {
        // ...yes, show the matches found in a select dialog
        // and let the user choose an entry.
        pDlgSelect->Reset();
        pDlgSelect->SetHeading(CVariant{255});
        int i = 1;
        while (1)
        {
          std::string strTitle = cddb.getInexactTitle(i);
          if (strTitle == "") break;

          std::string strArtist = cddb.getInexactArtist(i);
          if (!strArtist.empty())
            strTitle += " - " + strArtist;

          pDlgSelect->Add(strTitle);
          i++;
        }
        pDlgSelect->Open();

        // Has the user selected a match...
        int iSelectedCD = pDlgSelect->GetSelectedItem();
        if (iSelectedCD >= 0)
        {
          // ...query cddb for the inexact match
          if (!cddb.queryCDinfo(pCdInfo, 1 + iSelectedCD))
            pCdInfo->SetNoCDDBInfo();
        }
        else
          pCdInfo->SetNoCDDBInfo();
      }
      else if (lasterror == E_NO_MATCH_FOUND)
      {
        pCdInfo->SetNoCDDBInfo();
      }
      else
      {
        pCdInfo->SetNoCDDBInfo();
        // ..no, an error occured, display it to the user
        std::string strErrorText = StringUtils::Format("[%d] %s", cddb.getLastError(), cddb.getLastErrorText());
        HELPERS::ShowOKDialogLines(CVariant{255}, CVariant{257}, CVariant{std::move(strErrorText)}, CVariant{0});
      }
    } // if ( !cddb.queryCDinfo( pCdInfo ) )
    else
      pDialogProgress->Close();
  }

  // Filling the file items with cddb info happens in CMusicInfoTagLoaderCDDA

  return pCdInfo->HasCDDBInfo();
#else
  return false;
#endif
}

void CMusicDatabase::DeleteCDDBInfo()
{
#ifdef HAS_DVD_DRIVE
  CFileItemList items;
  if (!CDirectory::GetDirectory(m_profileManager.GetCDDBFolder(), items, ".cddb", DIR_FLAG_NO_FILE_DIRS))
  {
    HELPERS::ShowOKDialogText(CVariant{313}, CVariant{426});
    return ;
  }
  // Show a selectdialog that the user can select the album to delete
  CGUIDialogSelect *pDlg = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(WINDOW_DIALOG_SELECT);
  if (pDlg)
  {
    pDlg->SetHeading(CVariant{g_localizeStrings.Get(181)});
    pDlg->Reset();

    std::map<uint32_t, std::string> mapCDDBIds;
    for (int i = 0; i < items.Size(); ++i)
    {
      if (items[i]->m_bIsFolder)
        continue;

      std::string strFile = URIUtils::GetFileName(items[i]->GetPath());
      strFile.erase(strFile.size() - 5, 5);
      uint32_t lDiscId = strtoul(strFile.c_str(), NULL, 16);
      Xcddb cddb;
      cddb.setCacheDir(m_profileManager.GetCDDBFolder());

      if (!cddb.queryCache(lDiscId))
        continue;

      std::string strDiskTitle, strDiskArtist;
      cddb.getDiskTitle(strDiskTitle);
      cddb.getDiskArtist(strDiskArtist);

      std::string str;
      if (strDiskArtist.empty())
        str = strDiskTitle;
      else
        str = strDiskTitle + " - " + strDiskArtist;

      pDlg->Add(str);
      mapCDDBIds.insert(std::pair<uint32_t, std::string>(lDiscId, str));
    }

    pDlg->Sort();
    pDlg->Open();

    // and wait till user selects one
    int iSelectedAlbum = pDlg->GetSelectedItem();
    if (iSelectedAlbum < 0)
    {
      mapCDDBIds.erase(mapCDDBIds.begin(), mapCDDBIds.end());
      return ;
    }

    std::string strSelectedAlbum = pDlg->GetSelectedFileItem()->GetLabel();
    for (const auto &i : mapCDDBIds)
    {
      if (i.second == strSelectedAlbum)
      {
        std::string strFile = StringUtils::Format("%x.cddb", (unsigned int) i.first);
        CFile::Delete(URIUtils::AddFileToFolder(m_profileManager.GetCDDBFolder(), strFile));
        break;
      }
    }
    mapCDDBIds.erase(mapCDDBIds.begin(), mapCDDBIds.end());
  }
#endif
}

void CMusicDatabase::Clean()
{
  // If we are scanning for music info in the background,
  // other writing access to the database is prohibited.
  if (g_application.IsMusicScanning())
  {
    HELPERS::ShowOKDialogText(CVariant{189}, CVariant{14057});
    return;
  }

  if (HELPERS::ShowYesNoDialogText(CVariant{313}, CVariant{333}) == DialogResponse::YES)
  {
    CMusicDatabase musicdatabase;
    if (musicdatabase.Open())
    {
      int iReturnString = musicdatabase.Cleanup();
      musicdatabase.Close();

      if (iReturnString != ERROR_OK)
      {
        HELPERS::ShowOKDialogText(CVariant{313}, CVariant{iReturnString});
      }
    }
  }
}

bool CMusicDatabase::GetGenresNav(const std::string& strBaseDir, CFileItemList& items, const Filter &filter /* = Filter() */, bool countOnly /* = false */)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    // get primary genres for songs - could be simplified to just SELECT * FROM genre?
    std::string strSQL = "SELECT %s FROM genre ";

    Filter extFilter = filter;
    CMusicDbUrl musicUrl;
    SortDescription sorting;
    if (!musicUrl.FromString(strBaseDir) || !GetFilter(musicUrl, extFilter, sorting))
      return false;

    // if there are extra WHERE conditions we might need access
    // to songview or albumview for these conditions
    if (!extFilter.where.empty())
    {
      if (extFilter.where.find("artistview") != std::string::npos)
      {
        extFilter.AppendJoin("JOIN song_genre ON song_genre.idGenre = genre.idGenre"); 
        extFilter.AppendJoin("JOIN songview ON songview.idSong = song_genre.idSong"); 
        extFilter.AppendJoin("JOIN song_artist ON song_artist.idSong = songview.idSong"); 
        extFilter.AppendJoin("JOIN artistview ON artistview.idArtist = song_artist.idArtist");
      }
      else if (extFilter.where.find("songview") != std::string::npos)
      {
        extFilter.AppendJoin("JOIN song_genre ON song_genre.idGenre = genre.idGenre"); 
        extFilter.AppendJoin("JOIN songview ON songview.idSong = song_genre.idSong");
      }
      else if (extFilter.where.find("albumview") != std::string::npos)
      {
        extFilter.AppendJoin("JOIN song_genre ON song_genre.idGenre = genre.idGenre");
        extFilter.AppendJoin("JOIN song ON song.idSong = song_genre.idSong");
        extFilter.AppendJoin("JOIN albumview ON albumview.idAlbum = song.idAlbum");
      }
      extFilter.AppendGroup("genre.idGenre");
    }
    extFilter.AppendWhere("genre.strGenre != ''");

    if (countOnly)
    {
      extFilter.fields = "COUNT(DISTINCT genre.idGenre)";
      extFilter.group.clear();
      extFilter.order.clear();
    }

    std::string strSQLExtra;
    if (!BuildSQL(strSQLExtra, extFilter, strSQLExtra))
      return false;

    strSQL = PrepareSQL(strSQL.c_str(), !extFilter.fields.empty() && extFilter.fields.compare("*") != 0 ? extFilter.fields.c_str() : "genre.*") + strSQLExtra;

    // run query
    CLog::Log(LOGDEBUG, "%s query: %s", __FUNCTION__, strSQL.c_str());

    if (!m_pDS->query(strSQL))
      return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return true;
    }

    if (countOnly)
    {
      CFileItemPtr pItem(new CFileItem());
      pItem->SetProperty("total", iRowsFound == 1 ? m_pDS->fv(0).get_asInt() : iRowsFound);
      items.Add(pItem);

      m_pDS->close();
      return true;
    }

    // get data from returned rows
    while (!m_pDS->eof())
    {
      CFileItemPtr pItem(new CFileItem(m_pDS->fv("genre.strGenre").get_asString()));
      pItem->GetMusicInfoTag()->SetGenre(m_pDS->fv("genre.strGenre").get_asString());
      pItem->GetMusicInfoTag()->SetDatabaseId(m_pDS->fv("genre.idGenre").get_asInt(), "genre");

      CMusicDbUrl itemUrl = musicUrl;
      std::string strDir = StringUtils::Format("%i/", m_pDS->fv("genre.idGenre").get_asInt());
      itemUrl.AppendPath(strDir);
      pItem->SetPath(itemUrl.ToString());

      pItem->m_bIsFolder = true;
      items.Add(pItem);

      m_pDS->next();
    }

    // cleanup
    m_pDS->close();

    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}

bool CMusicDatabase::GetSourcesNav(const std::string& strBaseDir, CFileItemList& items, const Filter &filter /*= Filter()*/, bool countOnly /*= false*/)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    // Get sources for selection list when add/edit filter or smartplaylist rule
    std::string strSQL = "SELECT %s FROM source ";

    Filter extFilter = filter;
    CMusicDbUrl musicUrl;
    SortDescription sorting;
    if (!musicUrl.FromString(strBaseDir) || !GetFilter(musicUrl, extFilter, sorting))
      return false;

    // if there are extra WHERE conditions we might need access
    // to songview or albumview for these conditions
    if (!extFilter.where.empty())
    {
      if (extFilter.where.find("artistview") != std::string::npos)
      {
        extFilter.AppendJoin("JOIN album_source ON album_source.idSource = source.idSource");
        extFilter.AppendJoin("JOIN album_artist ON album_artist.idAlbum = album_source.idAlbum");
        extFilter.AppendJoin("JOIN artistview ON artistview.idArtist = album_artist.idArtist");
      }
      else if (extFilter.where.find("songview") != std::string::npos)
      {
        extFilter.AppendJoin("JOIN album_source ON album_source.idSource = source.idSource");
        extFilter.AppendJoin("JOIN songview ON songview.idAlbum = album_source .idAlbum");
      }
      else if (extFilter.where.find("albumview") != std::string::npos)
      {
        extFilter.AppendJoin("JOIN album_source ON album_source.idSource = source.idSource");
        extFilter.AppendJoin("JOIN albumview ON albumview.idAlbum = album_source .idAlbum");
      }
      extFilter.AppendGroup("source.idSource");
    }
    else
    { // Get only sources that have been scanned into music library
      extFilter.AppendJoin("JOIN album_source ON album_source.idSource = source.idSource");
      extFilter.AppendGroup("source.idSource");
    }

    if (countOnly)
    {
      extFilter.fields = "COUNT(DISTINCT source.idSource)";
      extFilter.group.clear();
      extFilter.order.clear();
    }

    std::string strSQLExtra;
    if (!BuildSQL(strSQLExtra, extFilter, strSQLExtra))
      return false;

    strSQL = PrepareSQL(strSQL.c_str(), !extFilter.fields.empty() && extFilter.fields.compare("*") != 0 ? extFilter.fields.c_str() : "source.*") + strSQLExtra;

    // run query
    CLog::Log(LOGDEBUG, "%s query: %s", __FUNCTION__, strSQL.c_str());

    if (!m_pDS->query(strSQL))
      return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return true;
    }

    if (countOnly)
    {
      CFileItemPtr pItem(new CFileItem());
      pItem->SetProperty("total", iRowsFound == 1 ? m_pDS->fv(0).get_asInt() : iRowsFound);
      items.Add(pItem);

      m_pDS->close();
      return true;
    }

    // get data from returned rows
    while (!m_pDS->eof())
    {
      CFileItemPtr pItem(new CFileItem(m_pDS->fv("source.strName").get_asString()));
      pItem->GetMusicInfoTag()->SetTitle(m_pDS->fv("source.strName").get_asString());
      pItem->GetMusicInfoTag()->SetDatabaseId(m_pDS->fv("source.idSource").get_asInt(), "source");

      CMusicDbUrl itemUrl = musicUrl;
      std::string strDir = StringUtils::Format("%i/", m_pDS->fv("source.idSource").get_asInt());
      itemUrl.AppendPath(strDir);
      itemUrl.AddOption("sourceid", m_pDS->fv("source.idSource").get_asInt());
      pItem->SetPath(itemUrl.ToString());

      pItem->m_bIsFolder = true;
      items.Add(pItem);

      m_pDS->next();
    }

    // cleanup
    m_pDS->close();

    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}

bool CMusicDatabase::GetYearsNav(const std::string& strBaseDir, CFileItemList& items, const Filter &filter /* = Filter() */)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    Filter extFilter = filter;
    CMusicDbUrl musicUrl;
    SortDescription sorting;
    if (!musicUrl.FromString(strBaseDir) || !GetFilter(musicUrl, extFilter, sorting))
      return false;

    // get years from album list
    std::string strSQL = "SELECT DISTINCT albumview.iYear FROM albumview ";
    extFilter.AppendWhere("albumview.iYear <> 0");

    if (!BuildSQL(strSQL, extFilter, strSQL))
      return false;

    // run query
    CLog::Log(LOGDEBUG, "%s query: %s", __FUNCTION__, strSQL.c_str());
    if (!m_pDS->query(strSQL))
      return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return true;
    }

    // get data from returned rows
    while (!m_pDS->eof())
    {
      CFileItemPtr pItem(new CFileItem(m_pDS->fv(0).get_asString()));
      SYSTEMTIME stTime;
      stTime.wYear = static_cast<unsigned short>(m_pDS->fv(0).get_asInt());
      pItem->GetMusicInfoTag()->SetReleaseDate(stTime);
      pItem->GetMusicInfoTag()->SetDatabaseId(-1, "year");

      CMusicDbUrl itemUrl = musicUrl;
      std::string strDir = StringUtils::Format("%i/", m_pDS->fv(0).get_asInt());
      itemUrl.AppendPath(strDir);
      pItem->SetPath(itemUrl.ToString());

      pItem->m_bIsFolder = true;
      items.Add(pItem);

      m_pDS->next();
    }

    // cleanup
    m_pDS->close();

    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}

bool CMusicDatabase::GetRolesNav(const std::string& strBaseDir, CFileItemList& items, const Filter &filter /* = Filter() */)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    Filter extFilter = filter;
    CMusicDbUrl musicUrl;
    SortDescription sorting;
    if (!musicUrl.FromString(strBaseDir) || !GetFilter(musicUrl, extFilter, sorting))
      return false;

    // get roles with artists having that role
    std::string strSQL = "SELECT DISTINCT role.idRole, role.strRole FROM role "
                         "JOIN song_artist ON song_artist.idRole = role.idRole ";

    if (!BuildSQL(strSQL, extFilter, strSQL))
      return false;

    // run query
    CLog::Log(LOGDEBUG, "%s query: %s", __FUNCTION__, strSQL.c_str());
    if (!m_pDS->query(strSQL))
      return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return true;
    }

    // get data from returned rows
    while (!m_pDS->eof())
    {
      std::string labelValue = m_pDS->fv("role.strRole").get_asString();
      CFileItemPtr pItem(new CFileItem(labelValue));
      pItem->GetMusicInfoTag()->SetTitle(labelValue);
      pItem->GetMusicInfoTag()->SetDatabaseId(m_pDS->fv("role.idRole").get_asInt(), "role");
      CMusicDbUrl itemUrl = musicUrl;
      std::string strDir = StringUtils::Format("%i/", m_pDS->fv("role.idRole").get_asInt());
      itemUrl.AppendPath(strDir);
      itemUrl.AddOption("roleid", m_pDS->fv("role.idRole").get_asInt());
      pItem->SetPath(itemUrl.ToString());

      pItem->m_bIsFolder = true;
      items.Add(pItem);

      m_pDS->next();
    }

    // cleanup
    m_pDS->close();

    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}

bool CMusicDatabase::GetAlbumsByYear(const std::string& strBaseDir, CFileItemList& items, int year)
{
  CMusicDbUrl musicUrl;
  if (!musicUrl.FromString(strBaseDir))
    return false;

  musicUrl.AddOption("year", year);
  musicUrl.AddOption("show_singles", true); // allow singles to be listed

  Filter filter;
  return GetAlbumsByWhere(musicUrl.ToString(), filter, items);
}

bool CMusicDatabase::GetCommonNav(const std::string &strBaseDir, const std::string &table, const std::string &labelField, CFileItemList &items, const Filter &filter /* = Filter() */, bool countOnly /* = false */)
{
  if (NULL == m_pDB.get()) return false;
  if (NULL == m_pDS.get()) return false;

  if (table.empty() || labelField.empty())
    return false;

  try
  {
    Filter extFilter = filter;
    std::string strSQL = "SELECT %s FROM " + table + " ";
    extFilter.AppendGroup(labelField);
    extFilter.AppendWhere(labelField + " != ''");

    if (countOnly)
    {
      extFilter.fields = "COUNT(DISTINCT " + labelField + ")";
      extFilter.group.clear();
      extFilter.order.clear();
    }

    // Do prepare before add where as it could contain a LIKE statement with wild card that upsets format
    // e.g. LIKE '%symphony%' would be taken as a %s format argument
    strSQL = PrepareSQL(strSQL, !extFilter.fields.empty() ? extFilter.fields.c_str() : labelField.c_str());

    CMusicDbUrl musicUrl;
    if (!BuildSQL(strBaseDir, strSQL, extFilter, strSQL, musicUrl))
      return false;

    // run query
    CLog::Log(LOGDEBUG, "%s query: %s", __FUNCTION__, strSQL.c_str());
    if (!m_pDS->query(strSQL))
      return false;

    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound <= 0)
    {
      m_pDS->close();
      return false;
    }

    if (countOnly)
    {
      CFileItemPtr pItem(new CFileItem());
      pItem->SetProperty("total", iRowsFound == 1 ? m_pDS->fv(0).get_asInt() : iRowsFound);
      items.Add(pItem);

      m_pDS->close();
      return true;
    }

    // get data from returned rows
    while (!m_pDS->eof())
    {
      std::string labelValue = m_pDS->fv(labelField.c_str()).get_asString();
      CFileItemPtr pItem(new CFileItem(labelValue));

      CMusicDbUrl itemUrl = musicUrl;
      std::string strDir = StringUtils::Format("%s/", labelValue.c_str());
      itemUrl.AppendPath(strDir);
      pItem->SetPath(itemUrl.ToString());

      pItem->m_bIsFolder = true;
      items.Add(pItem);

      m_pDS->next();
    }

    // cleanup
    m_pDS->close();

    return true;
  }
  catch (...)
  {
    m_pDS->close();
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }

  return false;
}

bool CMusicDatabase::GetAlbumTypesNav(const std::string &strBaseDir, CFileItemList &items, const Filter &filter /* = Filter() */, bool countOnly /* = false */)
{
  return GetCommonNav(strBaseDir, "albumview", "albumview.strType", items, filter, countOnly);
}

bool CMusicDatabase::GetMusicLabelsNav(const std::string &strBaseDir, CFileItemList &items, const Filter &filter /* = Filter() */, bool countOnly /* = false */)
{
  return GetCommonNav(strBaseDir, "albumview", "albumview.strLabel", items, filter, countOnly);
}

bool CMusicDatabase::GetArtistsNav(const std::string& strBaseDir, CFileItemList& items, bool albumArtistsOnly /* = false */, int idGenre /* = -1 */, int idAlbum /* = -1 */, int idSong /* = -1 */, const Filter &filter /* = Filter() */, const SortDescription &sortDescription /* = SortDescription() */, bool countOnly /* = false */)
{
  if (NULL == m_pDB.get()) return false;
  if (NULL == m_pDS.get()) return false;
  try
  {
    CMusicDbUrl musicUrl;
    if (!musicUrl.FromString(strBaseDir))
      return false;

    if (idGenre > 0)
      musicUrl.AddOption("genreid", idGenre);
    else if (idAlbum > 0)
      musicUrl.AddOption("albumid", idAlbum);
    else if (idSong > 0)
      musicUrl.AddOption("songid", idSong);

    // Override albumArtistsOnly parameter (usually externally set to SETTING_MUSICLIBRARY_SHOWCOMPILATIONARTISTS)
    // when local option already present in music URL thus allowing it to be an option in custom nodes
    if (!musicUrl.HasOption("albumartistsonly"))
      musicUrl.AddOption("albumartistsonly", albumArtistsOnly);

    bool result = GetArtistsByWhere(musicUrl.ToString(), filter, items, sortDescription, countOnly);

    return result;
  }
  catch (...)
  {
    m_pDS->close();
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}

bool CMusicDatabase::GetArtistsByWhere(const std::string& strBaseDir, const Filter &filter, CFileItemList& items, const SortDescription &sortDescription /* = SortDescription() */, bool countOnly /* = false */)
{
  if (NULL == m_pDB.get()) return false;
  if (NULL == m_pDS.get()) return false;

  try
  {
    unsigned int querytime = 0;
    unsigned int time = XbmcThreads::SystemClockMillis();
    int total = -1;

    std::string strSQL = "SELECT %s FROM artistview ";

    Filter extFilter = filter;
    CMusicDbUrl musicUrl;
    SortDescription sorting = sortDescription;
    if (!musicUrl.FromString(strBaseDir) || !GetFilter(musicUrl, extFilter, sorting))
      return false;

    // if there are extra WHERE conditions we might need access
    // to songview or albumview for these conditions
    if (!extFilter.where.empty())
    {
      bool extended = false;
      if (extFilter.where.find("songview") != std::string::npos)
      {
        extended = true;
        extFilter.AppendJoin("JOIN song_artist ON song_artist.idArtist = artistview.idArtist JOIN songview ON songview.idSong = song_artist.idSong");
      }
      else if (extFilter.where.find("albumview") != std::string::npos)
      {
        extended = true;
        extFilter.AppendJoin("JOIN album_artist ON album_artist.idArtist = artistview.idArtist JOIN albumview ON albumview.idAlbum = album_artist.idAlbum");
      }

      if (extended)
        extFilter.AppendGroup("artistview.idArtist");
    }

    if (countOnly)
    {
      extFilter.fields = "COUNT(DISTINCT artistview.idArtist)";
      extFilter.group.clear();
      extFilter.order.clear();
    }

    std::string strSQLExtra;
    if (!BuildSQL(strSQLExtra, extFilter, strSQLExtra))
      return false;

    // Apply limits and sort order directly in SQL when random sort or none
    bool limitedInSQL = extFilter.limit.empty() &&
      (sortDescription.sortBy == SortByNone || sortDescription.sortBy == SortByRandom) &&
      (sortDescription.limitStart > 0 || sortDescription.limitEnd > 0);
    if (limitedInSQL)
    {
      total = (int)strtol(GetSingleValue(PrepareSQL(strSQL, "COUNT(1)") + strSQLExtra, m_pDS).c_str(), NULL, 10);
      if (sortDescription.sortBy == SortByRandom)
        strSQLExtra += PrepareSQL(" ORDER BY RANDOM()");
      strSQLExtra += DatabaseUtils::BuildLimitClause(sortDescription.limitEnd, sortDescription.limitStart);
    }

    strSQL = PrepareSQL(strSQL.c_str(), !extFilter.fields.empty() && extFilter.fields.compare("*") != 0 ? extFilter.fields.c_str() : "artistview.*") + strSQLExtra;

    // run query
    CLog::Log(LOGDEBUG, "%s query: %s", __FUNCTION__, strSQL.c_str());
    querytime = XbmcThreads::SystemClockMillis();
    if (!m_pDS->query(strSQL)) 
      return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return true;
    }
    querytime = XbmcThreads::SystemClockMillis() - querytime;

    if (countOnly)
    {
      CFileItemPtr pItem(new CFileItem());
      pItem->SetProperty("total", iRowsFound == 1 ? m_pDS->fv(0).get_asInt() : iRowsFound);
      items.Add(pItem);

      m_pDS->close();
      return true;
    }

    // store the total value of items as a property
    if (total < iRowsFound)
      total = iRowsFound;
    items.SetProperty("total", total);

    DatabaseResults results;
    results.reserve(iRowsFound);

    // Random order with limits already applied in SQL, just fetch results from dataset
    sorting = sortDescription;
    if (limitedInSQL && sortDescription.sortBy == SortByRandom)
      sorting.sortBy = SortByNone;
    if (!SortUtils::SortFromDataset(sorting, MediaTypeArtist, m_pDS, results))
      return false;
    
    // get data from returned rows
    items.Reserve(results.size());
    const dbiplus::query_data &data = m_pDS->get_result_set().records;
    for (const auto &i : results)
    {
      unsigned int targetRow = (unsigned int)i.at(FieldRow).asInteger();
      const dbiplus::sql_record* const record = data.at(targetRow);

      try
      {
        CArtist artist = GetArtistFromDataset(record, false);
        CFileItemPtr pItem(new CFileItem(artist));

        CMusicDbUrl itemUrl = musicUrl;
        std::string path = StringUtils::Format("%ld/", artist.idArtist);
        itemUrl.AppendPath(path);
        pItem->SetPath(itemUrl.ToString());

        pItem->GetMusicInfoTag()->SetDatabaseId(artist.idArtist, MediaTypeArtist);
        pItem->SetIconImage("DefaultArtist.png");

        SetPropertiesFromArtist(*pItem, artist);
        items.Add(pItem);
      }
      catch (...)
      {
        m_pDS->close();
        CLog::Log(LOGERROR, "%s - out of memory getting listing (got %i)", __FUNCTION__, items.Size());
      }
    }

    // cleanup
    m_pDS->close();

    CLog::Log(LOGDEBUG, "{0}: Time to fill list with artists {1}ms query took {2}ms",
      __FUNCTION__, XbmcThreads::SystemClockMillis() - time, querytime);
    return true;
  }
  catch (...)
  {
    m_pDS->close();
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}

bool CMusicDatabase::GetAlbumFromSong(int idSong, CAlbum &album)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    std::string strSQL = PrepareSQL("select albumview.* from song join albumview on song.idAlbum = albumview.idAlbum where song.idSong='%i'", idSong);
    if (!m_pDS->query(strSQL)) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound != 1)
    {
      m_pDS->close();
      return false;
    }

    album = GetAlbumFromDataset(m_pDS.get());

    m_pDS->close();
    return true;

  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}

bool CMusicDatabase::GetAlbumsNav(const std::string& strBaseDir, CFileItemList& items, int idGenre /* = -1 */, int idArtist /* = -1 */, const Filter &filter /* = Filter() */, const SortDescription &sortDescription /* = SortDescription() */, bool countOnly /* = false */)
{
  CMusicDbUrl musicUrl;
  if (!musicUrl.FromString(strBaseDir))
    return false;

  // where clause
  if (idGenre > 0)
    musicUrl.AddOption("genreid", idGenre);

  if (idArtist > 0)
    musicUrl.AddOption("artistid", idArtist);

  return GetAlbumsByWhere(musicUrl.ToString(), filter, items, sortDescription, countOnly);
}

bool CMusicDatabase::GetAlbumsByWhere(const std::string &baseDir, const Filter &filter, CFileItemList &items, const SortDescription &sortDescription /* = SortDescription() */, bool countOnly /* = false */)
{
  if (m_pDB.get() == NULL || m_pDS.get() == NULL)
    return false;

  try
  {
    unsigned int querytime = 0;
    unsigned int time = XbmcThreads::SystemClockMillis();
    int total = -1;

    std::string strSQL = "SELECT %s FROM albumview ";

    Filter extFilter = filter;
    CMusicDbUrl musicUrl;
    SortDescription sorting = sortDescription;
    if (!musicUrl.FromString(baseDir) || !GetFilter(musicUrl, extFilter, sorting))
      return false;

    // if there are extra WHERE conditions we might need access
    // to songview for these conditions
    if (extFilter.where.find("songview") != std::string::npos)
    {
      extFilter.AppendJoin("JOIN songview ON songview.idAlbum = albumview.idAlbum");
      extFilter.AppendGroup("albumview.idAlbum");
    }

    std::string strSQLExtra;
    if (!BuildSQL(strSQLExtra, extFilter, strSQLExtra))
      return false;

    // Apply limits and sort order directly in SQL when random sort or none
    bool limitedInSQL = extFilter.limit.empty() &&
      (sortDescription.sortBy == SortByNone || sortDescription.sortBy == SortByRandom) &&
      (sortDescription.limitStart > 0 || sortDescription.limitEnd > 0);
    if (limitedInSQL)
    {
      total = (int)strtol(GetSingleValue(PrepareSQL(strSQL, "COUNT(1)") + strSQLExtra, m_pDS).c_str(), NULL, 10);
      if (sortDescription.sortBy == SortByRandom)
        strSQLExtra += PrepareSQL(" ORDER BY RANDOM()");
      strSQLExtra += DatabaseUtils::BuildLimitClause(sortDescription.limitEnd, sortDescription.limitStart);
    }

    strSQL = PrepareSQL(strSQL, !filter.fields.empty() && filter.fields.compare("*") != 0 ? filter.fields.c_str() : "albumview.*") + strSQLExtra;

    // run query
    CLog::Log(LOGDEBUG, "%s query: %s", __FUNCTION__, strSQL.c_str());
    querytime = XbmcThreads::SystemClockMillis();
    if (!m_pDS->query(strSQL)) 
      return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return true;
    }
    querytime = XbmcThreads::SystemClockMillis() - querytime;

    // store the total value of items as a property
    if (total < iRowsFound)
      total = iRowsFound;
    items.SetProperty("total", total);

    if (countOnly)
    {
      CFileItemPtr pItem(new CFileItem());
      pItem->SetProperty("total", total);
      items.Add(pItem);

      m_pDS->close();
      return true;
    }

    DatabaseResults results;
    results.reserve(iRowsFound);

    // Random order with limits already applied in SQL, just fetch results from dataset
    sorting = sortDescription;
    if (limitedInSQL && sortDescription.sortBy == SortByRandom)
      sorting.sortBy = SortByNone;
    if (!SortUtils::SortFromDataset(sorting, MediaTypeAlbum, m_pDS, results))
      return false;

    // get data from returned rows
    items.Reserve(results.size());
    const dbiplus::query_data &data = m_pDS->get_result_set().records;
    for (const auto &i : results)
    {
      unsigned int targetRow = (unsigned int)i.at(FieldRow).asInteger();
      const dbiplus::sql_record* const record = data.at(targetRow);

      try
      {
        CMusicDbUrl itemUrl = musicUrl;
        std::string path = StringUtils::Format("%i/", record->at(album_idAlbum).get_asInt());
        itemUrl.AppendPath(path);

        CFileItemPtr pItem(new CFileItem(itemUrl.ToString(), GetAlbumFromDataset(record)));
        pItem->SetIconImage("DefaultAlbumCover.png");
        items.Add(pItem);
      }
      catch (...)
      {
        m_pDS->close();
        CLog::Log(LOGERROR, "%s - out of memory getting listing (got %i)", __FUNCTION__, items.Size());
      }
    }

    // cleanup
    m_pDS->close();
    CLog::Log(LOGDEBUG, "{0}: Time to fill list with albums {1}ms query took {2}ms",
      __FUNCTION__, XbmcThreads::SystemClockMillis() - time, querytime);
    return true;
  }
  catch (...)
  {
    m_pDS->close();
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, filter.where.c_str());
  }
  return false;
}

bool CMusicDatabase::GetSongsFullByWhere(const std::string &baseDir, const Filter &filter, CFileItemList &items, const SortDescription &sortDescription /* = SortDescription() */, bool artistData /* = false*/)
{
  if (m_pDB.get() == NULL || m_pDS.get() == NULL)
    return false;

  try
  {
    unsigned int time = XbmcThreads::SystemClockMillis();
    int total = -1;

    Filter extFilter = filter;
    CMusicDbUrl musicUrl;
    SortDescription sorting = sortDescription;
    if (!musicUrl.FromString(baseDir) || !GetFilter(musicUrl, extFilter, sorting))
      return false;

    // if there are extra WHERE conditions we might need access
    // to songview for these conditions
    if (extFilter.where.find("albumview") != std::string::npos)
    {
      extFilter.AppendJoin("JOIN albumview ON albumview.idAlbum = songview.idAlbum");
      extFilter.AppendGroup("songview.idSong");
    }

    std::string strSQLExtra;
    if (!BuildSQL(strSQLExtra, extFilter, strSQLExtra))
      return false;

    // Count number of songs that satisfy selection criteria
    total = (int)strtol(GetSingleValue("SELECT COUNT(1) FROM songview " + strSQLExtra, m_pDS).c_str(), NULL, 10);

    // Apply any limiting directly in SQL if there is either no special sorting or random sort
    // When limited, random sort is also applied in SQL
    bool limitedInSQL = extFilter.limit.empty() &&
      (sortDescription.sortBy == SortByNone || sortDescription.sortBy == SortByRandom) &&
      (sortDescription.limitStart > 0 || sortDescription.limitEnd > 0);
    if (limitedInSQL)
    {
      if (sortDescription.sortBy == SortByRandom)
        strSQLExtra += PrepareSQL(" ORDER BY RANDOM()");
      strSQLExtra += DatabaseUtils::BuildLimitClause(sortDescription.limitEnd, sortDescription.limitStart);
    }

    std::string strSQL;
    if (artistData)
    { // Get data from song and song_artist tables to fully populate songs with artists
      // All songs now have at least one artist so inner join sufficient
      // Need guaranteed ordering for dataset processing to extract songs
      if (limitedInSQL)
        //Apply where clause, limits and random order to songview, then join as multiple records in result set per song
        strSQL = "SELECT sv.*, songartistview.* "
          "FROM (SELECT songview.* FROM songview " + strSQLExtra + ") AS sv "
          "JOIN songartistview ON songartistview.idsong = sv.idsong ";
      else
        strSQL = "SELECT songview.*, songartistview.* "
          "FROM songview JOIN songartistview ON songartistview.idsong = songview.idsong " + strSQLExtra;
      strSQL += " ORDER BY songartistview.idsong, songartistview.idRole, songartistview.iOrder";
    }
    else
      strSQL = "SELECT songview.* FROM songview " + strSQLExtra;

    CLog::Log(LOGDEBUG, "%s query = %s", __FUNCTION__, strSQL.c_str());
    // run query
    if (!m_pDS->query(strSQL))
      return false;

    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return true;
    }

    // Store the total number of songs as a property
    items.SetProperty("total", total);

    DatabaseResults results;
    results.reserve(iRowsFound);
    // Avoid sorting with limits when have join with songartistview
    // Limit when SortByNone already applied in SQL,
    // apply sort later to fileitems list rather than dataset
    sorting = sortDescription;
    if (artistData && sortDescription.sortBy != SortByNone)
      sorting.sortBy = SortByNone;
    if (!SortUtils::SortFromDataset(sorting, MediaTypeSong, m_pDS, results))
      return false;

    // Get songs from returned rows. If join songartistview then there is a row for every artist
    items.Reserve(total);
    int songArtistOffset = song_enumCount;
    int songId = -1;
    VECARTISTCREDITS artistCredits;
    const dbiplus::query_data &data = m_pDS->get_result_set().records;
    int count = 0;
    for (const auto &i : results)
    {
      unsigned int targetRow = (unsigned int)i.at(FieldRow).asInteger();
      const dbiplus::sql_record* const record = data.at(targetRow);

      try
      {
        if (songId != record->at(song_idSong).get_asInt())
        { //New song
          if (songId > 0 && !artistCredits.empty())
          {
            //Store artist credits for previous song
            GetFileItemFromArtistCredits(artistCredits, items[items.Size()-1].get());
            artistCredits.clear();
          }
          songId = record->at(song_idSong).get_asInt();
          CFileItemPtr item(new CFileItem);
          GetFileItemFromDataset(record, item.get(), musicUrl);
          // HACK for sorting by database returned order
          item->m_iprogramCount = ++count;
          items.Add(item);
        }
        // Get song artist credits and contributors
        if (artistData)
        {
          int idSongArtistRole = record->at(songArtistOffset + artistCredit_idRole).get_asInt();
          if (idSongArtistRole == ROLE_ARTIST)
            artistCredits.push_back(GetArtistCreditFromDataset(record, songArtistOffset));
          else
            items[items.Size() - 1]->GetMusicInfoTag()->AppendArtistRole(GetArtistRoleFromDataset(record, songArtistOffset));
        }
      }
      catch (...)
      {
        m_pDS->close();
        CLog::Log(LOGERROR, "%s: out of memory loading query: %s", __FUNCTION__, filter.where.c_str());
        return (items.Size() > 0);
      }
    }
    if (!artistCredits.empty())
    {
      //Store artist credits for final song
      GetFileItemFromArtistCredits(artistCredits, items[items.Size() - 1].get());
      artistCredits.clear();
    }
    // cleanup
    m_pDS->close();

    // Finally do any sorting in items list we have not been able to do before in SQL or dataset,
    // that is when have join with songartistview and sorting other than random with limit
    if (artistData && sortDescription.sortBy != SortByNone && !(limitedInSQL && sortDescription.sortBy == SortByRandom))
      items.Sort(sortDescription);

    CLog::Log(LOGDEBUG, "%s(%s) - took %d ms", __FUNCTION__, filter.where.c_str(), XbmcThreads::SystemClockMillis() - time);
    return true;
  }
  catch (...)
  {
    // cleanup
    m_pDS->close();
    CLog::Log(LOGERROR, "%s(%s) failed", __FUNCTION__, filter.where.c_str());
  }
  return false;
}

bool CMusicDatabase::GetSongsByWhere(const std::string &baseDir, const Filter &filter, CFileItemList &items, const SortDescription &sortDescription /* = SortDescription() */)
{
  if (m_pDB.get() == NULL || m_pDS.get() == NULL)
    return false;

  try
  {
    int total = -1;

    std::string strSQL = "SELECT %s FROM songview ";

    Filter extFilter = filter;
    CMusicDbUrl musicUrl;
    SortDescription sorting = sortDescription;
    if (!musicUrl.FromString(baseDir) || !GetFilter(musicUrl, extFilter, sorting))
      return false;

    // if there are extra WHERE conditions we might need access
    // to songview for these conditions
    if (extFilter.where.find("albumview") != std::string::npos)
    {
      extFilter.AppendJoin("JOIN albumview ON albumview.idAlbum = songview.idAlbum");
      extFilter.AppendGroup("songview.idSong");
    }

    std::string strSQLExtra;
    if (!BuildSQL(strSQLExtra, extFilter, strSQLExtra))
      return false;

    // Apply the limiting directly here if there's no special sorting but limiting
    if (extFilter.limit.empty() &&
        sortDescription.sortBy == SortByNone &&
       (sortDescription.limitStart > 0 || sortDescription.limitEnd > 0))
    {
      total = (int)strtol(GetSingleValue(PrepareSQL(strSQL, "COUNT(1)") + strSQLExtra, m_pDS).c_str(), NULL, 10);
      strSQLExtra += DatabaseUtils::BuildLimitClause(sortDescription.limitEnd, sortDescription.limitStart);
    }

    strSQL = PrepareSQL(strSQL, !filter.fields.empty() && filter.fields.compare("*") != 0 ? filter.fields.c_str() : "songview.*") + strSQLExtra;

    CLog::Log(LOGDEBUG, "%s query = %s", __FUNCTION__, strSQL.c_str());
    // run query
    if (!m_pDS->query(strSQL))
      return false;

    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return true;
    }

    // store the total value of items as a property
    if (total < iRowsFound)
      total = iRowsFound;
    items.SetProperty("total", total);

    DatabaseResults results;
    results.reserve(iRowsFound);
    if (!SortUtils::SortFromDataset(sortDescription, MediaTypeSong, m_pDS, results))
      return false;

    // get data from returned rows
    items.Reserve(results.size());
    const dbiplus::query_data &data = m_pDS->get_result_set().records;
    int count = 0;
    for (const auto &i : results)
    {
      unsigned int targetRow = (unsigned int)i.at(FieldRow).asInteger();
      const dbiplus::sql_record* const record = data.at(targetRow);

      try
      {
        CFileItemPtr item(new CFileItem);
        GetFileItemFromDataset(record, item.get(), musicUrl);
        // HACK for sorting by database returned order
        item->m_iprogramCount = ++count;
        items.Add(item);
      }
      catch (...)
      {
        m_pDS->close();
        CLog::Log(LOGERROR, "%s: out of memory loading query: %s", __FUNCTION__, filter.where.c_str());
        return (items.Size() > 0);
      }
    }

    // cleanup
    m_pDS->close();
    return true;
  }
  catch (...)
  {
    // cleanup
    m_pDS->close();
    CLog::Log(LOGERROR, "%s(%s) failed", __FUNCTION__, filter.where.c_str());
  }
  return false;
}

bool CMusicDatabase::GetSongsByYear(const std::string& baseDir, CFileItemList& items, int year)
{
  CMusicDbUrl musicUrl;
  if (!musicUrl.FromString(baseDir))
    return false;

  musicUrl.AddOption("year", year);

  Filter filter;
  return GetSongsFullByWhere(baseDir, filter, items, SortDescription(), true);
}

bool CMusicDatabase::GetSongsNav(const std::string& strBaseDir, CFileItemList& items, int idGenre, int idArtist, int idAlbum, const SortDescription &sortDescription /* = SortDescription() */)
{
  CMusicDbUrl musicUrl;
  if (!musicUrl.FromString(strBaseDir))
    return false;

  if (idAlbum > 0)
    musicUrl.AddOption("albumid", idAlbum);

  if (idGenre > 0)
    musicUrl.AddOption("genreid", idGenre);

  if (idArtist > 0)
    musicUrl.AddOption("artistid", idArtist);

  Filter filter;
  return GetSongsFullByWhere(musicUrl.ToString(), filter, items, sortDescription, true);
}

typedef struct
{
  std::string fieldJSON;  // Field name in JSON schema
  std::string formatJSON; // Format in JSON schema
  bool bSimple;           // Fetch field directly to JSON output
  std::string fieldDB;    // Name of field in db query
  std::string SQL;        // SQL for scalar subqueries or field alias
} translateJSONField;

static const translateJSONField JSONtoDBArtist[] = {
  // Table and single value join fields
  { "artist",                    "string", true,  "strArtist",              "" }, // Label field at top
  { "sortname",                  "string", true,  "strSortname",            "" },
  { "instrument",                 "array", true,  "strInstruments",         "" },
  { "description",               "string", true,  "strBiography",           "" },
  { "genre",                      "array", true,  "strGenres",              "" },
  { "mood",                       "array", true,  "strMoods",               "" },
  { "style",                      "array", true,  "strStyles",              "" },
  { "yearsactive",                "array", true,  "strYearsActive",         "" },
  { "born",                      "string", true,  "strBorn",                "" },
  { "formed",                    "string", true,  "strFormed",              "" },
  { "died",                      "string", true,  "strDied",                "" },
  { "disbanded",                 "string", true,  "strDisbanded",           "" },
  { "type",                      "string", true,  "strType",                "" },
  { "gender",                    "string", true,  "strGender",              "" },
  { "disambiguation",            "string", true,  "strDisambiguation",      "" },
  { "musicbrainzartistid",        "array", true,  "strMusicBrainzArtistId", "" }, // Array in schema, but only ever one element

  // Scalar subquery fields
  { "dateadded",                 "string", true,  "dateAdded",              "(SELECT MAX(song.dateAdded) FROM song_artist JOIN song ON song.idSong = song_artist.idSong WHERE song_artist.idArtist = artist.idArtist) AS dateAdded" },
  { "",                          "string", true,  "artistsortname",         "(CASE WHEN strSortName IS NOT NULL THEN strSortname ELSE strArtist END) AS artistsortname" },
  // JOIN fields (multivalue), same order as _JoinToArtistFields
  { "",                                "", false, "isSong",                 "" },
  { "sourceid",                  "string", false, "idSourceAlbum",          "album_source.idSource AS idSourceAlbum" },
  { "",                          "string", false, "idSourceSong",           "album_source.idSource AS idSourceSong" },
  { "songgenres",                 "array", false, "idSongGenreAlbum",       "song_genre.idGenre AS idSongGenreAlbum" },
  { "",                           "array", false, "idSongGenreSong",        "song_genre.idGenre AS idSongGenreSong" },
  { "",                                "", false, "strSongGenreAlbum",      "genre.strGenre AS strSongGenreAlbum" },
  { "",                                "", false, "strSongGenreSong",       "genre.strGenre AS strSongGenreSong" },
  { "art",                             "", false, "idArt",                  "art.art_id AS idArt" },
  { "",                                "", false, "artType",                "art.type AS artType" },
  { "",                                "", false, "artURL",                 "art.url AS artURL" },
  { "",                                "", false, "idRole",                 "song_artist.idRole" },
  { "roles",                           "", false, "strRole",                "role.strRole" },
  { "",                                "", false, "iOrderRole",             "song_artist.iOrder AS iOrderRole" },
  // Derived from joined tables
  { "isalbumartist",               "bool", false, "",                       "" },
  { "thumbnail",                 "string", false, "",                       "" },
  { "fanart",                    "string", false, "",                       "" }

  /*
  Sources and genre are related via album, and so the dataset only contains source
  and genre pairs that exist, rather than all the genres being repeated for every
  source. We can not only look at genres for the first source, and genre can be
  out of order.
  */

};

static const size_t NUM_ARTIST_FIELDS = sizeof(JSONtoDBArtist) / sizeof(translateJSONField);

bool CMusicDatabase::GetArtistsByWhereJSON(const std::set<std::string>& fields, const std::string &baseDir,
  CVariant& result, int& total, const SortDescription &sortDescription /* = SortDescription() */)
{
  if (NULL == m_pDB.get()) return false;
  if (NULL == m_pDS.get()) return false;

  try
  {
    total = -1;

    Filter extFilter;
    CMusicDbUrl musicUrl;
    SortDescription sorting = sortDescription;
    //! @todo: replace GetFilter to avoid exists as well as JOIn to albm_artist and song_artist tables
    if (!musicUrl.FromString(baseDir) || !GetFilter(musicUrl, extFilter, sorting))
      return false;

    // Replace view names in filter with table names
    StringUtils::Replace(extFilter.where, "artistview", "artist");
    StringUtils::Replace(extFilter.where, "albumview", "album");

    std::string strSQLExtra;
    if (!BuildSQL(strSQLExtra, extFilter, strSQLExtra))
      return false;

    // Count number of artists that satisfy selection criteria 
    //(includes xsp limits from filter, but not sort limits)
    total = static_cast<int>(strtol(GetSingleValue("SELECT COUNT(1) FROM artist " + strSQLExtra, m_pDS).c_str(), NULL, 10));

    // Process albumartistsonly option
    const CUrlOptions::UrlOptions& options = musicUrl.GetOptions();
    bool albumArtistsOnly(false);
    auto option = options.find("albumartistsonly");
    if (option != options.end())
      albumArtistsOnly = option->second.asBoolean();
    // Process role options
    int roleidfilter = 1; // Default restrict song_artist to "artists" only, no other roles.
    option = options.find("roleid");
    if (option != options.end())
      roleidfilter = static_cast<int>(option->second.asInteger());
    else
    {
      option = options.find("role");
      if (option != options.end())
      {
        if (option->second.asString() == "all" || option->second.asString() == "%")
          roleidfilter = -1000; //All roles
        else
          roleidfilter = GetRoleByName(option->second.asString());
      }
    }

    //! @todo: use SortAttributeUseArtistSortName and remove articles
    std::vector<std::string> orderfields;
    std::string DESC;
    if (sortDescription.sortOrder == SortOrderDescending)
      DESC = " DESC";
    if (sortDescription.sortBy == SortByRandom)
      orderfields.emplace_back(PrepareSQL("RANDOM()")); // Adjust syntax
    else if (sortDescription.sortBy == SortByArtist)
      orderfields.emplace_back("strArtist");
    else if (sortDescription.sortBy == SortByDateAdded)
      orderfields.emplace_back("dateAdded");

    // Always sort by id to define order when other fields same
    if (sortDescription.sortBy != SortByRandom)
      orderfields.emplace_back("artist.idArtist");

    // Fill inline view filter order fields, and build sort scalar subquery SQL
    std::string artistsortSQL;
    for (auto& name : orderfields)
    {
      //Add field for adjusted name sorting using sort name and ignoring articles
      if (name.compare("strArtist") == 0)
      {
        artistsortSQL = SortnameBuildSQL("artistsortname", sortDescription.sortAttributes,
          "strArtist", "strSortName");
        if (!artistsortSQL.empty())
          name = "artistsortname";
        // Natural number case insensitve sort
        extFilter.AppendOrder(AlphanumericSortSQL(name, sortDescription.sortOrder));
      }
      else
        extFilter.AppendOrder(name + DESC);
    }

    std::string strSQL;

    // Setup fields to query, and album field number mapping
    // Find first join field (isSong) in JSONtoDBArtist for offset 
    int index_firstjoin = -1;
    for (unsigned int i = 0; i < NUM_ARTIST_FIELDS; i++)
    {
      if (JSONtoDBArtist[i].fieldDB == "isSong")
      {
        index_firstjoin = i;
        break;
      }
    }
    Filter joinFilter;
    Filter albumArtistFilter;
    Filter songArtistFilter;
    DatasetLayout joinLayout(static_cast<size_t>(joinToArtist_enumCount));
    extFilter.AppendField("artist.idArtist");  // ID "artistid" in JSON
    std::vector<int> dbfieldindex;
    // JSON "label" field is strArtist which is also output as "artist", query field once output twice
    extFilter.AppendField(JSONtoDBArtist[0].fieldDB);
    dbfieldindex.emplace_back(0); // Output "artist"

    // Check each otional artist db field that could be retrieved (not "artist")
    // and fields in sort order to query in inline view but not output
    for (unsigned int i = 1; i < NUM_ARTIST_FIELDS; i++)
    {
      bool foundOrderby(false);
      bool foundJSON = fields.find(JSONtoDBArtist[i].fieldJSON) != fields.end();
      if (!foundJSON)
        foundOrderby = std::find(orderfields.begin(), orderfields.end(), JSONtoDBArtist[i].fieldDB) != orderfields.end();
      if (foundOrderby || foundJSON)
      {
        if (JSONtoDBArtist[i].bSimple)
        {
          // Store indexes of requested artist table and scalar subquery fields 
          // to be output, and -1 when not output to JSON
          if (!foundJSON)
            dbfieldindex.emplace_back(-1);
          else
            dbfieldindex.emplace_back(i);
          // Field from scaler subquery
          if (!JSONtoDBArtist[i].SQL.empty())
          {
            if (JSONtoDBArtist[i].fieldDB == "artistsortname")
              extFilter.AppendField(artistsortSQL);
            else
              extFilter.AppendField(PrepareSQL(JSONtoDBArtist[i].SQL));
          }
          else
            // Field from artist table
            extFilter.AppendField(JSONtoDBArtist[i].fieldDB);
        }
        else
        {
          // Field from join or derived from joined fields
          joinLayout.SetField(i - index_firstjoin, JSONtoDBArtist[i].fieldDB, true);
        }
      }
    }

    // Build JOIN, WHERE, ORDER BY and LIMIT for inline view
    strSQLExtra = "";
    if (!BuildSQL(strSQLExtra, extFilter, strSQLExtra))
      return false;

    // Add any LIMIT clause to strSQLExtra
    if (extFilter.limit.empty() &&
      (sortDescription.limitStart > 0 || sortDescription.limitEnd > 0))
    {
      strSQLExtra += DatabaseUtils::BuildLimitClause(sortDescription.limitEnd, sortDescription.limitStart);
    }

    // Setup multivalue JOINs, GROUP BY and ORDER BY
    bool bJoinAlbumArtist(false);
    bool bJoinSongArtist(false);
    if (sortDescription.sortBy != SortByRandom)
    {
      // Repeat inline view order (that always includes idArtist) on join query
      std::string order = extFilter.order;
      StringUtils::Replace(order, "artist.", "a1.");
      joinFilter.AppendOrder(order);
    }
    else
      joinFilter.AppendOrder("a1.idArtist");
    joinFilter.AppendGroup("a1.idArtist"); 
    // Album artists and song artists
    if (joinLayout.GetFetch(joinToArtist_isalbumartist) ||
      joinLayout.GetFetch(joinToArtist_idSourceAlbum) ||
      joinLayout.GetFetch(joinToArtist_idSongGenreAlbum) ||
      joinLayout.GetFetch(joinToArtist_strRole))
    {
      bJoinAlbumArtist = true;
      albumArtistFilter.AppendGroup("album_artist.idArtist");
      albumArtistFilter.AppendField("album_artist.idArtist AS id");
      if (!albumArtistsOnly || joinLayout.GetFetch(joinToArtist_strRole))
      {
        bJoinSongArtist = true;
        songArtistFilter.AppendGroup("song_artist.idArtist");
        songArtistFilter.AppendField("song_artist.idArtist AS id");
        songArtistFilter.AppendField("1 AS isSong");
        albumArtistFilter.AppendField("0 AS isSong");
        joinLayout.SetField(joinToArtist_isSong, JSONtoDBArtist[index_firstjoin + joinToArtist_isSong].fieldDB);
        joinFilter.AppendOrder(JSONtoDBArtist[index_firstjoin + joinToArtist_isSong].fieldDB);
      }
    }

    // Sources
    if (joinLayout.GetFetch(joinToArtist_idSourceAlbum))
    { // Left join as source may have been removed but leaving lib entries      
      albumArtistFilter.AppendJoin("LEFT JOIN album_source ON album_source.idAlbum = album_artist.idAlbum");
      albumArtistFilter.AppendGroup("album_source.idSource");
      albumArtistFilter.AppendField(JSONtoDBArtist[index_firstjoin + joinToArtist_idSourceAlbum].SQL);
      joinFilter.AppendGroup(JSONtoDBArtist[index_firstjoin + joinToArtist_idSourceAlbum].fieldDB);
      joinFilter.AppendOrder(JSONtoDBArtist[index_firstjoin + joinToArtist_idSourceAlbum].fieldDB);
      if (bJoinSongArtist)
      {
        songArtistFilter.AppendJoin("JOIN song ON song.idSong = song_artist.idSong");
        songArtistFilter.AppendJoin("LEFT JOIN album_source ON album_source.idAlbum = song.idAlbum");
        songArtistFilter.AppendGroup("album_source.idSource");
        songArtistFilter.AppendField("-1 AS " + JSONtoDBArtist[index_firstjoin + joinToArtist_idSourceAlbum].fieldDB);
        songArtistFilter.AppendField(JSONtoDBArtist[index_firstjoin + joinToArtist_idSourceSong].SQL);
        albumArtistFilter.AppendField("-1 AS " + JSONtoDBArtist[index_firstjoin + joinToArtist_idSourceSong].fieldDB);
        joinLayout.SetField(joinToArtist_idSourceSong, JSONtoDBArtist[index_firstjoin + joinToArtist_idSourceSong].fieldDB);
        joinFilter.AppendGroup(JSONtoDBArtist[index_firstjoin + joinToArtist_idSourceSong].fieldDB);
        joinFilter.AppendOrder(JSONtoDBArtist[index_firstjoin + joinToArtist_idSourceSong].fieldDB);
      }
      else
      {
        joinLayout.SetField(joinToArtist_idSourceAlbum, JSONtoDBArtist[index_firstjoin + joinToArtist_idSourceAlbum].SQL, true);
      }
    }

    // Songgenres - id and genres always both
    if (joinLayout.GetFetch(joinToArtist_idSongGenreAlbum))
    { // All albums have songs, but left join genre as songs may not have genre
      albumArtistFilter.AppendJoin("JOIN song ON song.idAlbum = album_artist.idAlbum");
      albumArtistFilter.AppendJoin("LEFT JOIN song_genre ON song_genre.idSong = song.idSong");
      albumArtistFilter.AppendJoin("LEFT JOIN genre ON genre.idGenre = song_genre.idGenre");
      albumArtistFilter.AppendGroup("genre.idGenre");
      albumArtistFilter.AppendField(JSONtoDBArtist[index_firstjoin + joinToArtist_idSongGenreAlbum].SQL);
      albumArtistFilter.AppendField(JSONtoDBArtist[index_firstjoin + joinToArtist_strSongGenreAlbum].SQL);
      joinLayout.SetField(joinToArtist_strSongGenreAlbum, JSONtoDBArtist[index_firstjoin + joinToArtist_strSongGenreAlbum].fieldDB);
      joinFilter.AppendGroup(JSONtoDBArtist[index_firstjoin + joinToArtist_idSongGenreAlbum].fieldDB);
      joinFilter.AppendOrder(JSONtoDBArtist[index_firstjoin + joinToArtist_idSongGenreAlbum].fieldDB);
      if (bJoinSongArtist)
      { // Left join genre as songs may not have genre
        songArtistFilter.AppendJoin("LEFT JOIN song_genre ON song_genre.idSong = song_artist.idSong");
        songArtistFilter.AppendJoin("LEFT JOIN genre ON genre.idGenre = song_genre.idGenre");
        songArtistFilter.AppendGroup("genre.idGenre");
        songArtistFilter.AppendField("-1 AS " + JSONtoDBArtist[index_firstjoin + joinToArtist_idSongGenreAlbum].fieldDB);
        songArtistFilter.AppendField("'' AS " + JSONtoDBArtist[index_firstjoin + joinToArtist_strSongGenreAlbum].fieldDB);
        songArtistFilter.AppendField(JSONtoDBArtist[index_firstjoin + joinToArtist_idSongGenreSong].SQL);
        songArtistFilter.AppendField(JSONtoDBArtist[index_firstjoin + joinToArtist_strSongGenreSong].SQL);
        albumArtistFilter.AppendField("-1 AS " + JSONtoDBArtist[index_firstjoin + joinToArtist_idSongGenreSong].fieldDB);
        albumArtistFilter.AppendField("'' AS " + JSONtoDBArtist[index_firstjoin + joinToArtist_strSongGenreSong].fieldDB);
        joinLayout.SetField(joinToArtist_idSongGenreSong, JSONtoDBArtist[index_firstjoin + joinToArtist_idSongGenreSong].fieldDB);
        joinLayout.SetField(joinToArtist_strSongGenreSong, JSONtoDBArtist[index_firstjoin + joinToArtist_strSongGenreSong].fieldDB);
        joinFilter.AppendGroup(JSONtoDBArtist[index_firstjoin + joinToArtist_idSongGenreSong].fieldDB);
        joinFilter.AppendOrder(JSONtoDBArtist[index_firstjoin + joinToArtist_idSongGenreSong].fieldDB);
      }
      else
      {  // Define field alias names in join layout
        joinLayout.SetField(joinToArtist_idSongGenreAlbum, JSONtoDBArtist[index_firstjoin + joinToArtist_idSongGenreAlbum].SQL, true);
        joinLayout.SetField(joinToArtist_strSongGenreAlbum, JSONtoDBArtist[index_firstjoin + joinToArtist_strSongGenreAlbum].SQL);
      }
    }

    // Roles
    if (roleidfilter == 1 && !joinLayout.GetFetch(joinToArtist_strRole))
      // Only looking at album and song artists not other roles (default), 
      // so filter dataset rows likewise. 
      songArtistFilter.AppendWhere("song_artist.idRole = 1");
    else if (joinLayout.GetFetch(joinToArtist_strRole) ||  // "roles" field
             (bJoinSongArtist &&
             (joinLayout.GetFetch(joinToArtist_idSourceAlbum) ||
             joinLayout.GetFetch(joinToArtist_idSongGenreAlbum))))
    { // Rows from many roles so fetch roleid for "roles", source and genre processing
      songArtistFilter.AppendField(JSONtoDBArtist[index_firstjoin + joinToArtist_idRole].SQL);
      songArtistFilter.AppendGroup(JSONtoDBArtist[index_firstjoin + joinToArtist_idRole].SQL);
      // Add fake column to album_artist query
      albumArtistFilter.AppendField("-1 AS " + JSONtoDBArtist[index_firstjoin + joinToArtist_idRole].fieldDB);
      joinLayout.SetField(joinToArtist_idRole, JSONtoDBArtist[index_firstjoin + joinToArtist_idRole].fieldDB);
      joinFilter.AppendGroup(JSONtoDBArtist[index_firstjoin + joinToArtist_idRole].fieldDB);
      joinFilter.AppendOrder(JSONtoDBArtist[index_firstjoin + joinToArtist_idRole].fieldDB);
    }
    if (joinLayout.GetFetch(joinToArtist_strRole))
    { // Fetch role desc
      songArtistFilter.AppendJoin("JOIN role ON role.idRole = song_artist.idRole");
      songArtistFilter.AppendField(JSONtoDBArtist[index_firstjoin + joinToArtist_strRole].SQL);
      // Add fake column to album_artist query
      albumArtistFilter.AppendField("'albumartist' AS " + JSONtoDBArtist[index_firstjoin + joinToArtist_strRole].fieldDB);
    }

    // Build source, genre and roles part of query
    if (bJoinAlbumArtist)
    {
      if (bJoinSongArtist)
      {
        // Combine song and album artist filter as UNION and add to join filter as an inline view
        std::string strAlbumSQL;
        if (!BuildSQL(strAlbumSQL, albumArtistFilter, strAlbumSQL))
          return false;
        strAlbumSQL = "SELECT " + albumArtistFilter.fields + " FROM album_artist " + strAlbumSQL;
        std::string strSongSQL;
        if (!BuildSQL(strSongSQL, songArtistFilter, strSongSQL))
          return false;
        strSongSQL = "SELECT " + songArtistFilter.fields + " FROM song_artist " + strSongSQL;

        joinFilter.AppendJoin("JOIN (" + strAlbumSQL + " UNION " + strSongSQL + ") AS albumSong ON id = a1.idArtist");
      }
      else
      { //Only join album_artist, so move filter elements to join filter
        joinFilter.AppendJoin("JOIN album_artist ON album_artist.idArtist = a1.idArtist");
        joinFilter.AppendJoin(albumArtistFilter.join);
      }
    }

    //Art
    bool bJoinArt(false);
    bJoinArt = joinLayout.GetOutput(joinToArtist_idArt) ||
      joinLayout.GetOutput(joinToArtist_thumbnail) ||
      joinLayout.GetOutput(joinToArtist_fanart);
    if (bJoinArt)
    { // Left join as artist may not have any art
      joinFilter.AppendJoin("LEFT JOIN art ON art.media_id = a1.idArtist AND art.media_type = 'artist'");
      joinLayout.SetField(joinToArtist_idArt, JSONtoDBArtist[index_firstjoin + joinToArtist_idArt].SQL, 
        joinLayout.GetOutput(joinToArtist_idArt));
      joinLayout.SetField(joinToArtist_artType, JSONtoDBArtist[index_firstjoin + joinToArtist_artType].SQL);
      joinLayout.SetField(joinToArtist_artURL, JSONtoDBArtist[index_firstjoin + joinToArtist_artURL].SQL);
      joinFilter.AppendGroup("art.art_id");
      joinFilter.AppendOrder("arttype");
      if (!joinLayout.GetOutput(joinToArtist_idArt))
      {
        if (!joinLayout.GetOutput(joinToArtist_thumbnail))
          // Fanart only
          joinFilter.AppendJoin("AND art.type = 'fanart'");
        else if (!joinLayout.GetOutput(joinToArtist_fanart))
          // Thumb only
          joinFilter.AppendJoin("AND art.type = 'thumb'");
      }
    }

    // Build JOIN part of query (if we have one)
    std::string strSQLJoin;
    if (joinLayout.HasFilterFields())
      if (!BuildSQL(strSQLJoin, joinFilter, strSQLJoin))
        return false;

    // Adjust where in the results record the join fields are allowing for the
    // inline view fields (Quicker than finding field by name every time)
    // idArtist + other artist fields    
    joinLayout.AdjustRecordNumbers(1 + dbfieldindex.size());

    // Build full query
    // When have multiple value joins e.g. song genres, use inline view
    // SELECT a1.*, <join fields> FROM 
    //   (SELECT <artist fields> FROM artist <where> + <order by> +  <limits> ) AS a1 
    //   <joins> <group by> <order by> + <joins order by>
    // Don't use prepareSQL - confuses  arttype = 'thumb' filter 

    strSQL = "SELECT " + extFilter.fields + " FROM artist " + strSQLExtra;
    if (joinLayout.HasFilterFields())
    {
      strSQL = "(" + strSQL + ") AS a1 ";
      strSQL = "SELECT a1.*, " + joinLayout.GetFields() + " FROM " + strSQL + strSQLJoin;
    }

    CLog::Log(LOGDEBUG, "%s query: %s", __FUNCTION__, strSQL.c_str());
    // run query
    unsigned int time = XbmcThreads::SystemClockMillis();
    if (!m_pDS->query(strSQL))
      return false;
    CLog::Log(LOGDEBUG, "%s - query took %i ms",
      __FUNCTION__, XbmcThreads::SystemClockMillis() - time); time = XbmcThreads::SystemClockMillis();

    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound <= 0)
    {
      m_pDS->close();
      return true;
    }

    DatabaseResults dbResults;
    dbResults.reserve(iRowsFound);

    // Get artists from returned rows. Joins means there can be many rows per artist
    int artistId = -1;
    int sourceId = -1;
    int genreId = -1;
    int roleId = -1;
    int artId = -1;
    std::vector<int> genreidlist;
    std::vector<int> sourceidlist;
    std::vector<int> roleidlist;
    bool bArtDone(false);
    bool bHaveArtist(false);
    bool bIsAlbumArtist(true);
    bool bGenreFoundViaAlbum(false);
    CVariant artistObj;
    while (!m_pDS->eof() || bHaveArtist)
    {
      const dbiplus::sql_record* const record = m_pDS->get_sql_record();

      if (m_pDS->eof() || artistId != record->at(0).get_asInt())
      {
        // Store previous or last artist
        if (bHaveArtist)
        {
          // Convert any empty MBid array into an array with one empty element [""]
          // to match the number of artist ID (way other mbid arrays handled)
          if (artistObj.isMember("musicbrainzartistid") && artistObj["musicbrainzartistid"].empty())
            artistObj["musicbrainzartistid"].append("");

          result["artists"].append(artistObj);
          bHaveArtist = false;
          artistObj.clear();
        }
        if (artistObj.empty())
        {
          // Initialise fields, ensure those with possible null values are set to correct empty variant type
          if (joinLayout.GetOutput(joinToArtist_idSourceAlbum))
            artistObj["sourceid"] = CVariant(CVariant::VariantTypeArray);
          if (joinLayout.GetOutput(joinToArtist_idSongGenreAlbum))
            artistObj["songgenres"] = CVariant(CVariant::VariantTypeArray);
          if (joinLayout.GetOutput(joinToArtist_idArt))
            artistObj["art"] = CVariant(CVariant::VariantTypeObject);
          if (joinLayout.GetOutput(joinToArtist_thumbnail))
            artistObj["thumbnail"] = "";
          if (joinLayout.GetOutput(joinToArtist_fanart))
            artistObj["fanart"] = "";

          sourceId = -1;
          roleId = -1;
          genreId = -1;
          artId = -1;
          genreidlist.clear();
          bGenreFoundViaAlbum = false;
          sourceidlist.clear();
          roleidlist.clear();
          bArtDone = false;
        }
        if (m_pDS->eof())
          continue;  // Having saved the last artist stop

        // New artist
        artistId = record->at(0).get_asInt();
        bHaveArtist = true;
        artistObj["artistid"] = artistId;
        artistObj["label"] = record->at(1).get_asString();
        artistObj["artist"] = record->at(1).get_asString(); // Always have "artist"
        bIsAlbumArtist = bJoinAlbumArtist;  //Album artist by default
        if (bJoinSongArtist)
        {
          bIsAlbumArtist = !record->at(joinLayout.GetRecNo(joinToArtist_isSong)).get_asBool();
          if (joinLayout.GetOutput(joinToArtist_isalbumartist))
            artistObj["isalbumartist"] = bIsAlbumArtist;
        }
        for (size_t i = 0; i < dbfieldindex.size(); i++)
          if (dbfieldindex[i] > -1)
          {
            if (JSONtoDBArtist[dbfieldindex[i]].formatJSON == "integer")
              artistObj[JSONtoDBArtist[dbfieldindex[i]].fieldJSON] = record->at(1 + i).get_asInt();
            else if (JSONtoDBArtist[dbfieldindex[i]].formatJSON == "float")
              artistObj[JSONtoDBArtist[dbfieldindex[i]].fieldJSON] = record->at(1 + i).get_asFloat();
            else if (JSONtoDBArtist[dbfieldindex[i]].formatJSON == "array")
              artistObj[JSONtoDBArtist[dbfieldindex[i]].fieldJSON] =
              StringUtils::Split(record->at(1 + i).get_asString(), CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_musicItemSeparator);
            else if (JSONtoDBArtist[dbfieldindex[i]].formatJSON == "boolean")
              artistObj[JSONtoDBArtist[dbfieldindex[i]].fieldJSON] = record->at(1 + i).get_asBool();
            else
              artistObj[JSONtoDBArtist[dbfieldindex[i]].fieldJSON] = record->at(1 + i).get_asString();
          }
      }
      if (bJoinAlbumArtist)
      {
        bool bAlbumArtistRow(true);
        int idRoleRow = -1;
        if (bJoinSongArtist)
        {
          bAlbumArtistRow = !record->at(joinLayout.GetRecNo(joinToArtist_isSong)).get_asBool();
          if (joinLayout.GetRecNo(joinToArtist_idRole) > -1 &&
            !record->at(joinLayout.GetRecNo(joinToArtist_idRole)).get_isNull())
          {
            idRoleRow = record->at(joinLayout.GetRecNo(joinToArtist_idRole)).get_asInt();
          }
        }

        // Sources - gathered via both album_artist and song_artist (with role = 1)       
        if (joinLayout.GetFetch(joinToArtist_idSourceAlbum))
        {
          if ((bAlbumArtistRow && joinLayout.GetRecNo(joinToArtist_idSourceAlbum) > -1 &&
            !record->at(joinLayout.GetRecNo(joinToArtist_idSourceAlbum)).get_isNull() &&
            sourceId != record->at(joinLayout.GetRecNo(joinToArtist_idSourceAlbum)).get_asInt()) ||
            (!bAlbumArtistRow && joinLayout.GetRecNo(joinToArtist_idSourceSong) > -1 &&
              !record->at(joinLayout.GetRecNo(joinToArtist_idSourceSong)).get_isNull() &&
              sourceId != record->at(joinLayout.GetRecNo(joinToArtist_idSourceSong)).get_asInt()))
          {
            bArtDone = bArtDone || (sourceId > 0);  // Not first source, skip art repeats
            bool found(false);
            sourceId = record->at(joinLayout.GetRecNo(joinToArtist_idSourceAlbum)).get_asInt();
            if (!bAlbumArtistRow)
            {
              // Skip other roles (when fetching them)
              if (idRoleRow > 1)
              {
                found = true;
              }
              else
              {
                sourceId = record->at(joinLayout.GetRecNo(joinToArtist_idSourceSong)).get_asInt();
                // Song artist row may repeat sources found via album artist
                // Already have that source? 
                for (const auto& i : sourceidlist)
                  if (i == sourceId)
                  {
                    found = true;
                    break;
                  }
              }
            }
            if (!found)
            {
              sourceidlist.emplace_back(sourceId);
              artistObj["sourceid"].append(sourceId);
            }
          }
        }
        // Songgenres - via album artist takes precedence
        /*
        Sources and genre are related via album, and so the dataset only contains source
        and genre pairs that exist, rather than all the genres being repeated for every
        source. We can not only look at genres for the first source, and genre can be
        found out of order.
        Also song artist row may repeat genres found via album artist
        */
        if (joinLayout.GetFetch(joinToArtist_idSongGenreAlbum))
        {
          std::string strGenre;
          bool newgenre(false);
          if (bAlbumArtistRow && joinLayout.GetRecNo(joinToArtist_idSongGenreAlbum) > -1 &&
            !record->at(joinLayout.GetRecNo(joinToArtist_idSongGenreAlbum)).get_isNull() &&
            genreId != record->at(joinLayout.GetRecNo(joinToArtist_idSongGenreAlbum)).get_asInt())
          {
            bArtDone = bArtDone || (genreId > 0);  // Not first genre, skip art repeats
            newgenre = true;
            genreId = record->at(joinLayout.GetRecNo(joinToArtist_idSongGenreAlbum)).get_asInt();
            strGenre = record->at(joinLayout.GetRecNo(joinToArtist_strSongGenreAlbum)).get_asString();
          }
          else if (!bAlbumArtistRow && !bGenreFoundViaAlbum &&
            joinLayout.GetRecNo(joinToArtist_idSongGenreSong) > -1 &&
            !record->at(joinLayout.GetRecNo(joinToArtist_idSongGenreSong)).get_isNull() &&
            genreId != record->at(joinLayout.GetRecNo(joinToArtist_idSongGenreSong)).get_asInt())
          {
            bArtDone = bArtDone || (genreId > 0);  // Not first genre, skip art repeats
            newgenre = idRoleRow <= 1; // Skip other roles (when fetching them)
            genreId = record->at(joinLayout.GetRecNo(joinToArtist_idSongGenreSong)).get_asInt();
            strGenre = record->at(joinLayout.GetRecNo(joinToArtist_strSongGenreSong)).get_asString();
          }
          bool found(false);
          if (newgenre)
          {
            // Already have that genre? 
            for (const auto& i : genreidlist)
              if (i == genreId)
              {
                found = true;
                break;
              }
            if (!found)
            {
              bGenreFoundViaAlbum = bGenreFoundViaAlbum || bAlbumArtistRow;
              genreidlist.emplace_back(genreId);
              CVariant genreObj;
              genreObj["genreid"] = genreId;
              genreObj["title"] = strGenre;
              artistObj["songgenres"].append(genreObj);
            }
          }
        }
        // Roles - gathered via song_artist roleid rows 
        if (joinLayout.GetFetch(joinToArtist_idRole))
        {
          if (!bAlbumArtistRow && roleId != idRoleRow)
          {
            bArtDone = bArtDone || (roleId > 0);  // Not first role, skip art repeats
            roleId = idRoleRow;
            if (joinLayout.GetOutput(joinToArtist_strRole))
            {
              // Already have that role? 
              bool found(false);
              for (const auto& i : roleidlist)
                if (i == roleId)
                {
                  found = true;
                  break;
                }
              if (!found)
              {
                roleidlist.emplace_back(roleId);
                CVariant roleObj;
                roleObj["roleid"] = roleId;
                roleObj["role"] = record->at(joinLayout.GetRecNo(joinToArtist_strRole)).get_asString();
                artistObj["roles"].append(roleObj);
              }
            }
          }
        }
      }
      // Art
      if (bJoinArt && !bArtDone && 
        !record->at(joinLayout.GetRecNo(joinToArtist_idArt)).get_isNull() &&
        record->at(joinLayout.GetRecNo(joinToArtist_idArt)).get_asInt() > 0 && 
        artId != record->at(joinLayout.GetRecNo(joinToArtist_idArt)).get_asInt())
      {
        artId = record->at(joinLayout.GetRecNo(joinToArtist_idArt)).get_asInt();
        if (joinLayout.GetOutput(joinToArtist_idArt))
        {
          artistObj["art"][record->at(joinLayout.GetRecNo(joinToArtist_artType)).get_asString()] =
            CTextureUtils::GetWrappedImageURL(record->at(joinLayout.GetRecNo(joinToArtist_artURL)).get_asString());
        }
        if (joinLayout.GetOutput(joinToArtist_thumbnail) &&
          record->at(joinLayout.GetRecNo(joinToArtist_artType)).get_asString() == "thumb")
        {
          artistObj["thumbnail"] = CTextureUtils::GetWrappedImageURL(record->at(joinLayout.GetRecNo(joinToArtist_artURL)).get_asString());
        }
        if (joinLayout.GetOutput(joinToArtist_fanart) &&
          record->at(joinLayout.GetRecNo(joinToArtist_artType)).get_asString() == "fanart")
        {
          artistObj["fanart"] = CTextureUtils::GetWrappedImageURL(record->at(joinLayout.GetRecNo(joinToArtist_artURL)).get_asString());
        }
      }

      m_pDS->next();
    }
    m_pDS->close(); // cleanup recordset data

    // Ensure random order of output when results set is sorted to process multi-value joins
    if (sortDescription.sortBy == SortByRandom && joinLayout.HasFilterFields())
      KODI::UTILS::RandomShuffle(result["artists"].begin_array(), result["artists"].end_array());

    return true;
  }
  catch (...)
  {
    m_pDS->close();
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}

static const translateJSONField JSONtoDBAlbum[] = {
  // albumview (inc scalar subquery fields use in filter rules)
  { "title",                     "string", true,  "strAlbum",               "" },  // Label field at top
  { "description",               "string", true,  "strReview",              "" },
  { "genre",                      "array", true,  "strGenres",              "" },
  { "theme",                      "array", true,  "strThemes",              "" },
  { "mood",                       "array", true,  "strMoods",               "" },
  { "style",                      "array", true,  "strStyles",              "" },
  { "type",                      "string", true,  "strType",                "" },
  { "albumlabel",                "string", true,  "strLabel",               "" },
  { "rating",                     "float", true,  "fRating",                "" },
  { "votes",                    "integer", true,  "iVotes",                 "" },
  { "userrating",              "unsigned", true,  "iUserrating",            "" },
  { "year",                     "integer", true,  "iYear",                  "" },
  { "musicbrainzalbumid",        "string", true,  "strMusicBrainzAlbumID",  "" },
  { "displayartist",             "string", true,  "strArtists",             "" }, //strArtistDisp in album table
  { "compilation",              "boolean", true,  "bCompilation",           "" },
  { "releasetype",               "string", true,  "strReleaseType",         "" },
  { "sortartist",                "string", true,  "strArtistSort",          "" },
  { "musicbrainzreleasegroupid", "string", true,  "strReleaseGroupMBID",    "" },
  { "playcount",                "integer", true,  "iTimesPlayed",           "" },  // Scalar subquery in view
  { "dateadded",                 "string", true,  "dateAdded",              "" },  // Scalar subquery in view
  { "lastplayed",                "string", true,  "lastPlayed",             "" },  // Scalar subquery in view
  // Scalar subquery fields
  { "sourceid",                  "string", true,  "sourceid",               "(SELECT GROUP_CONCAT(album_source.idSource SEPARATOR '; ')  FROM album_source WHERE album_source.idAlbum = albumview.idAlbum) AS sources" },
  // Single value JOIN fields
  { "thumbnail",                  "image", true,  "thumbnail",              "art.url AS thumbnail" }, // or (SELECT art.url FROM art WHERE art.media_id = album.idAlbum AND art.media_type = "album" AND art.type = "thumb") as url
  // JOIN fields (multivalue), same order as _JoinToAlbumFields
  { "artistid",                   "array", false, "idArtist",               "album_artist.idArtist AS idArtist" },
  { "artist",                     "array", false, "strArtist",              "artist.strArtist AS strArtist" },
  { "musicbrainzalbumartistid",   "array", false, "strArtistMBID",          "artist.strMusicBrainzArtistID AS strArtistMBID" },
  { "songgenres",                 "array", false, "idSongGenre",            "song_genre.idGenre AS idSongGenre" },
  { "",                                "", false, "strSongGenre",           "genre.strGenre AS strSongGenre" },
  { "",                                "", true, "artistsortname",          "CASE WHEN strArtistSort IS NOT NULL THEN strArtistSort ELSE strArtists END AS artistsortname"}
  /*
   Album "fanart" and "art" fields of JSON schema are fetched using thumbloader
   and separate queries to allow for fallback strategy.

   Using albmview, rather than album table, as view has scalar subqueries for
   playcount, dateadded and lastplayed already defined. Needed as MySQL does
   not support use of scalar subquery field alias names in where clauses (they
   have to be repeated) and these fields can be used by filter rules. 
   Using this view is no slower than the album table as these scalar fields are
   only calculated (slowing query) when field is in field list.
  */
};

static const size_t NUM_ALBUM_FIELDS = sizeof(JSONtoDBAlbum) / sizeof(translateJSONField);

bool CMusicDatabase::GetAlbumsByWhereJSON(const std::set<std::string>& fields, const std::string &baseDir,  
  CVariant& result, int& total, const SortDescription &sortDescription /* = SortDescription() */)
{
  
  if (NULL == m_pDB.get()) return false;
  if (NULL == m_pDS.get()) return false;

  try
  {
    total = -1;

    Filter extFilter;
    CMusicDbUrl musicUrl;
    SortDescription sorting = sortDescription;
    if (!musicUrl.FromString(baseDir) || !GetFilter(musicUrl, extFilter, sorting))
      return false;

    // Replace view names in filter with table names
    StringUtils::Replace(extFilter.where, "artistview", "artist");

    std::string strSQLExtra;
    if (!BuildSQL(strSQLExtra, extFilter, strSQLExtra))
      return false;

    // Count number of albums that satisfy selection criteria 
    // (includes xsp limits from filter, but not sort limits)
    // Use albumview as filter rules in where clause may use scalar query fields
    total = static_cast<int>(strtol(GetSingleValue("SELECT COUNT(1) FROM albumview " + strSQLExtra, m_pDS).c_str(), nullptr, 10));

    //! @todo: use SortAttributeUseArtistSortName and remove articles
    std::vector<std::string> orderfields;
    std::string DESC;
    if (sortDescription.sortOrder == SortOrderDescending)
      DESC = " DESC";
    if (sortDescription.sortBy == SortByRandom)
      orderfields.emplace_back(PrepareSQL("RANDOM()")); //Adjust styntax
    else if (sortDescription.sortBy == SortByAlbum ||
      sortDescription.sortBy == SortByLabel ||
      sortDescription.sortBy == SortByTitle)
    {
      orderfields.emplace_back("strAlbum");
      orderfields.emplace_back("strArtists");
    }
    else if (sortDescription.sortBy == SortByAlbumType)
    {
      orderfields.emplace_back("strType");
      orderfields.emplace_back("strAlbum");
      orderfields.emplace_back("strArtists");
    }
    else if (sortDescription.sortBy == SortByArtist)
    {
      orderfields.emplace_back("strArtists");
      orderfields.emplace_back("strAlbum");
    }
    else if (sortDescription.sortBy == SortByArtistThenYear)
    {
      orderfields.emplace_back("strArtists");
      orderfields.emplace_back("iYear");
      orderfields.emplace_back("strAlbum");
    }
    else if (sortDescription.sortBy == SortByYear)
    {
      orderfields.emplace_back("iYear");
      orderfields.emplace_back("strAlbum");
    }
    else if (sortDescription.sortBy == SortByGenre)
    {
      orderfields.emplace_back("strGenres");
      orderfields.emplace_back("strAlbum");
      orderfields.emplace_back("strArtists");
    }
    else if (sortDescription.sortBy == SortByDateAdded)
    {
      orderfields.emplace_back("dateAdded");
    }
    else if (sortDescription.sortBy == SortByPlaycount)
    {
      orderfields.emplace_back("iTimesPlayed");
      orderfields.emplace_back("strAlbum");
      orderfields.emplace_back("strArtists");
    }
    else if (sortDescription.sortBy == SortByLastPlayed)
    {
      orderfields.emplace_back("lastPlayed");
      orderfields.emplace_back("strAlbum");
      orderfields.emplace_back("strArtists");
    }
    else if (sortDescription.sortBy == SortByRating)
    {
      orderfields.emplace_back("fRating");
      orderfields.emplace_back("strAlbum");
      orderfields.emplace_back("strArtists");
    }
    else if (sortDescription.sortBy == SortByVotes)
    {
      orderfields.emplace_back("iVotes");
      orderfields.emplace_back("strAlbum");
      orderfields.emplace_back("strArtists");
    }
    else if (sortDescription.sortBy == SortByUserRating)
    {
      orderfields.emplace_back("iUserrating");
      orderfields.emplace_back("strAlbum");
      orderfields.emplace_back("strArtists");
    }
    // Always sort by id to define order when other fields same
    if (sortDescription.sortBy != SortByRandom)
      orderfields.emplace_back("albumview.idAlbum");

    // Fill inline view filter order fields, and build sort scalar subquery SQL
    std::string artistsortSQL;
    for (auto& name : orderfields)
    {
      //Add field for adjusted name sorting using sort name and ignoring articles
      if (name.compare("strArtists") == 0)
      {
        artistsortSQL = SortnameBuildSQL("artistsortname", sortDescription.sortAttributes,
          "strArtists", "strArtistSort");
        if (!artistsortSQL.empty())
          name = "artistsortname";
        // Natural number case insensitve sort
        extFilter.AppendOrder(AlphanumericSortSQL(name, sortDescription.sortOrder));
      }
      else if (name.compare("strAlbum") == 0 || 
               name.compare("strType") == 0 ||
               name.compare("strGenres") == 0)
        // Natural number case insensitve sort
        extFilter.AppendOrder(AlphanumericSortSQL(name, sortDescription.sortOrder));
      else
        extFilter.AppendOrder(name + DESC);
    }
    
    std::string strSQL;

    // Setup fields to query, and album field number mapping
    // Find idArtist in JSONtoDBAlbum, offset of first join field 
    int index_idArtist = -1;
    for (unsigned int i = 0; i < NUM_ALBUM_FIELDS; i++)
    {
      if (JSONtoDBAlbum[i].fieldDB == "idArtist")
      {
        index_idArtist = i;
        break;
      }
    }   
    Filter joinFilter;
    DatasetLayout joinLayout(static_cast<size_t>(joinToAlbum_enumCount));
    extFilter.AppendField("albumview.idAlbum");  // ID "albumid" in JSON
    std::vector<int> dbfieldindex;
    // JSON "label" field is strAlbum which may also be requested as "title", query field once output twice
    extFilter.AppendField(JSONtoDBAlbum[0].fieldDB);
    if (fields.find(JSONtoDBAlbum[0].fieldJSON) != fields.end())
      dbfieldindex.emplace_back(0); // Output "title"
    else
      dbfieldindex.emplace_back(-1); // fetch but not outout

    // Check each optional album db field that could be retrieved (not label)
    // and fields in sort order to query in inline view but not output
    for (unsigned int i = 1; i < NUM_ALBUM_FIELDS; i++)
    {
      bool foundOrderby(false);
      bool foundJSON = fields.find(JSONtoDBAlbum[i].fieldJSON) != fields.end();
      if (!foundJSON)
        foundOrderby = std::find(orderfields.begin(), orderfields.end(), JSONtoDBAlbum[i].fieldDB) != orderfields.end();
      if (foundOrderby || foundJSON)
      {
        if (JSONtoDBAlbum[i].bSimple)
        {
          // Store indexes of requested album table and scalar subquery fields 
          // to be output, and -1 when not output to JSON
          if (!foundJSON)
            dbfieldindex.emplace_back(-1);
          else
            dbfieldindex.emplace_back(i);
          // Field from scaler subquery
          if (!JSONtoDBAlbum[i].SQL.empty())
          { 
            if (JSONtoDBAlbum[i].fieldDB == "artistsortname")
              extFilter.AppendField(artistsortSQL);
            else
              extFilter.AppendField(PrepareSQL(JSONtoDBAlbum[i].SQL));
          }
          else
            // Field from album table
            extFilter.AppendField(JSONtoDBAlbum[i].fieldDB);
        }
        else
        {  // Field from join
          joinLayout.SetField(i - index_idArtist, JSONtoDBAlbum[i].SQL, true);
        }
      }
    }

    // JOIN art tables if needed (fields output and/or in sort)
    if (extFilter.fields.find("art.") != std::string::npos)
    { // Left join as not all albums have art, but only have one thumb at most
      extFilter.AppendJoin("LEFT JOIN art ON art.media_id = idAlbum "
        "AND art.media_type = 'album' AND art.type = 'thumb'");
    }

    // Build JOIN, WHERE, ORDER BY and LIMIT for inline view
    strSQLExtra = "";
    if (!BuildSQL(strSQLExtra, extFilter, strSQLExtra))
      return false;

    // Add any LIMIT clause to strSQLExtra
    if (extFilter.limit.empty() &&
      (sortDescription.limitStart > 0 || sortDescription.limitEnd > 0))
    {
      strSQLExtra += DatabaseUtils::BuildLimitClause(sortDescription.limitEnd, sortDescription.limitStart);
    }

    // Setup multivalue JOINs, GROUP BY and ORDER BY
    bool bJoinAlbumArtist(false);
    if (sortDescription.sortBy != SortByRandom)
    {
      // Repeat inline view order (that always includes idAlbum) on join query
      std::string order = extFilter.order;
      StringUtils::Replace(order, "albumview.", "a1.");
      joinFilter.AppendOrder(order);
    }
    else
      joinFilter.AppendOrder("a1.idAlbum");
    joinFilter.AppendGroup("a1.idAlbum");
    // Album artists
    if (joinLayout.GetFetch(joinToAlbum_idArtist) ||
        joinLayout.GetFetch(joinToAlbum_strArtist) ||
        joinLayout.GetFetch(joinToAlbum_strArtistMBID))
    { // All albums have at least one artist so inner join sufficient
      bJoinAlbumArtist = true;
      joinFilter.AppendJoin("JOIN album_artist ON album_artist.idAlbum = a1.idAlbum");
      joinFilter.AppendGroup("album_artist.idArtist");
      joinFilter.AppendOrder("album_artist.iOrder");
      // Ensure idArtist is queried 
      if (!joinLayout.GetFetch(joinToAlbum_idArtist))
        joinLayout.SetField(joinToAlbum_idArtist, JSONtoDBAlbum[index_idArtist + joinToAlbum_idArtist].SQL);
    }
    // artist table needed for strArtist or MBID 
    // (album_artist.strArtist can be an alias or spelling variation) 
    if (joinLayout.GetFetch(joinToAlbum_strArtist) || joinLayout.GetFetch(joinToAlbum_strArtistMBID))
      joinFilter.AppendJoin("JOIN artist ON artist.idArtist = album_artist.idArtist");

    // Songgenres - id and genres always both
    if (joinLayout.GetFetch(joinToAlbum_idSongGenre))
    { // All albums have songs, but left join genre as songs may not have genre
      joinFilter.AppendJoin("JOIN song ON song.idAlbum = a1.idAlbum");
      joinFilter.AppendJoin("LEFT JOIN song_genre ON song.idSong = song_genre.idSong");
      joinFilter.AppendJoin("LEFT JOIN genre ON song_genre.idGenre = genre.idGenre");
      joinFilter.AppendGroup("genre.idGenre");
      joinFilter.AppendOrder("song_genre.iOrder");
      joinLayout.SetField(joinToAlbum_strSongGenre, JSONtoDBAlbum[index_idArtist + joinToAlbum_strSongGenre].SQL);
    }

    // Build JOIN part of query (if we have one)
    std::string strSQLJoin;
    if (joinLayout.HasFilterFields())
      if (!BuildSQL(strSQLJoin, joinFilter, strSQLJoin))
        return false;
        
    // Adjust where in the results record the join fields are allowing for the
    // inline view fields (Quicker than finding field by name every time)
    // idAlbum + other album fields    
    joinLayout.AdjustRecordNumbers(1 + dbfieldindex.size());
    
    // Build full query
    // When have multiple value joins (artists or song genres) use inline view
    // SELECT a1.*, <join fields> FROM 
    //   (SELECT <album fields> FROM albumview <where> + <order by> +  <limits> ) AS a1 
    //   <joins> <group by> <order by> <joins order by>
    // Don't use prepareSQL - confuses  releasetype = 'album' filter and group_concat separator

    strSQL = "SELECT " + extFilter.fields + " FROM albumview " + strSQLExtra;
    if (joinLayout.HasFilterFields())
    {
      strSQL = "(" + strSQL + ") AS a1 ";
      strSQL = "SELECT a1.*, " + joinLayout.GetFields() + " FROM " + strSQL + strSQLJoin;
    }
        
    CLog::Log(LOGDEBUG, "%s query: %s", __FUNCTION__, strSQL.c_str());
    // run query
    unsigned int time = XbmcThreads::SystemClockMillis();
    if (!m_pDS->query(strSQL))
      return false;
    CLog::Log(LOGDEBUG, "%s - query took %i ms",
      __FUNCTION__, XbmcThreads::SystemClockMillis() - time); time = XbmcThreads::SystemClockMillis();

    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound <= 0)
    {
      m_pDS->close();
      return true;
    }

    DatabaseResults dbResults;
    dbResults.reserve(iRowsFound);

    // Get albums from returned rows. Joins means there can be many rows per album
    int albumId = -1;
    int artistId = -1;
    bool bSongGenreDone(false);
    CVariant albumObj;
    while (!m_pDS->eof() || !albumObj.empty())
    {
      const dbiplus::sql_record* const record = m_pDS->get_sql_record();

      if (m_pDS->eof() || albumId != record->at(0).get_asInt())
      { 
        // Store previous or last album
        if (!albumObj.empty())
        {
          // Ensure albums with null songgenres get empty array
          if (joinLayout.GetOutput(joinToAlbum_idSongGenre) && !albumObj.isMember("songgenres"))
            albumObj["songgenres"] = CVariant(CVariant::VariantTypeArray);

          // Split sources string into int array
          if (albumObj.isMember("sourceid"))
          {
            std::vector<std::string> sources = StringUtils::Split(albumObj["sourceid"].asString(), ";");
            albumObj["sourceid"] = CVariant(CVariant::VariantTypeArray);
            for (size_t i = 0; i < sources.size(); i++)
              albumObj["sourceid"].append(atoi(sources[i].c_str()));
          }

          result["albums"].append(albumObj);

          albumObj.clear();          
          artistId = -1;
          bSongGenreDone = false;
        }
        if (m_pDS->eof())
          continue; // Having saved last album stop

        // New album
        albumId = record->at(0).get_asInt();
        albumObj["albumid"] = albumId;
        albumObj["label"] = record->at(1).get_asString();
        for (size_t i = 0; i < dbfieldindex.size(); i++)
          if (dbfieldindex[i] > -1)
          {
            if (JSONtoDBAlbum[dbfieldindex[i]].formatJSON == "integer")
              albumObj[JSONtoDBAlbum[dbfieldindex[i]].fieldJSON] = record->at(1 + i).get_asInt();
            else if (JSONtoDBAlbum[dbfieldindex[i]].formatJSON == "unsigned")
              albumObj[JSONtoDBAlbum[dbfieldindex[i]].fieldJSON] = std::max(record->at(1 + i).get_asInt(), 0);
            else if (JSONtoDBAlbum[dbfieldindex[i]].formatJSON == "float")
              albumObj[JSONtoDBAlbum[dbfieldindex[i]].fieldJSON] = std::max(record->at(1 + i).get_asFloat(), 0.f);
            else if (JSONtoDBAlbum[dbfieldindex[i]].formatJSON == "array")
              albumObj[JSONtoDBAlbum[dbfieldindex[i]].fieldJSON] = StringUtils::Split(record->at(1 + i).get_asString(),
                CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_musicItemSeparator);
            else if (JSONtoDBAlbum[dbfieldindex[i]].formatJSON == "boolean")
              albumObj[JSONtoDBAlbum[dbfieldindex[i]].fieldJSON] = record->at(1 + i).get_asBool();
            else if (JSONtoDBAlbum[dbfieldindex[i]].formatJSON == "image")
            {
              std::string url = record->at(1 + i).get_asString();
              if (!url.empty())
                url = CTextureUtils::GetWrappedImageURL(url);
              albumObj[JSONtoDBAlbum[dbfieldindex[i]].fieldJSON] = url;
            }
            else
              albumObj[JSONtoDBAlbum[dbfieldindex[i]].fieldJSON] = record->at(1 + i).get_asString();
          }
      }
      if (bJoinAlbumArtist && joinLayout.GetRecNo(joinToAlbum_idArtist) > -1)
      {
        if (artistId != record->at(joinLayout.GetRecNo(joinToAlbum_idArtist)).get_asInt())
        {
          bSongGenreDone = (artistId > 0);  // Not first artist, skip genre
          artistId = record->at(joinLayout.GetRecNo(joinToAlbum_idArtist)).get_asInt();
          if (joinLayout.GetOutput(joinToAlbum_idArtist))
            albumObj["artistid"].append(artistId);
          if (artistId == BLANKARTIST_ID)
          {
            if (joinLayout.GetOutput(joinToAlbum_strArtist))
              albumObj["artist"].append(StringUtils::Empty);
            if (joinLayout.GetOutput(joinToAlbum_strArtistMBID))
              albumObj["musicbrainzalbumartistid"].append(StringUtils::Empty);
          }
          else
          {
            if (joinLayout.GetOutput(joinToAlbum_strArtist) && joinLayout.GetRecNo(joinToAlbum_strArtist) > -1)
              albumObj["artist"].append(record->at(joinLayout.GetRecNo(joinToAlbum_strArtist)).get_asString());
            if (joinLayout.GetOutput(joinToAlbum_strArtistMBID) && joinLayout.GetRecNo(joinToAlbum_strArtistMBID) > -1)
              albumObj["musicbrainzalbumartistid"].append(record->at(joinLayout.GetRecNo(joinToAlbum_strArtistMBID)).get_asString());
          }
        }        
      }
      if (!bSongGenreDone && joinLayout.GetRecNo(joinToAlbum_idSongGenre) > -1 &&
          joinLayout.GetRecNo(joinToAlbum_strSongGenre) > -1 &&
          !record->at(joinLayout.GetRecNo(joinToAlbum_idSongGenre)).get_isNull())
      {
        CVariant genreObj;
        genreObj["genreid"] = record->at(joinLayout.GetRecNo(joinToAlbum_idSongGenre)).get_asInt();
        genreObj["title"] = record->at(joinLayout.GetRecNo(joinToAlbum_strSongGenre)).get_asString();
        albumObj["songgenres"].append(genreObj);
      }
      m_pDS->next();
    }
    m_pDS->close(); // cleanup recordset data

    // Ensure random order of output when results set is sorted to process multi-value joins
    if (sortDescription.sortBy == SortByRandom && joinLayout.HasFilterFields())
      KODI::UTILS::RandomShuffle(result["albums"].begin_array(), result["albums"].end_array());

    return true;
  }
  catch (...)
  {
    m_pDS->close();
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}

static const translateJSONField JSONtoDBSong[] = {
  // table and single value join fields
  { "title",                     "string", true,  "strTitle",               "" }, // Label field at top
  { "albumid",                  "integer", true,  "song.idAlbum",           "" },
  { "",                                "", true,  "song.iTrack",            "" },
  { "displayartist",             "string", true,  "song.strArtistDisp",     "" },
  { "sortartist",                "string", true,  "song.strArtistSort",     "" },
  { "genre",                      "array", true,  "song.strGenres",         "" },
  { "duration",                 "integer", true,  "iDuration",              "" },
  { "comment",                   "string", true,  "comment",                "" },
  { "year",                     "integer", true,  "song.iYear",             "" },
  { "",                          "string", true,  "strFileName",            "" },
  { "musicbrainztrackid",        "string", true,  "strMusicBrainzTrackID",  "" },
  { "playcount",                "integer", true,  "iTimesPlayed",           "" },
  { "lastplayed",                "string", true,  "lastPlayed",             "" },
  { "rating",                     "float", true,  "rating",                 "" },
  { "votes",                    "integer", true,  "votes",                  "" },
  { "userrating",              "unsigned", true,  "song.userrating",        "" },
  { "mood",                       "array", true,  "mood",                   "" },
  { "dateadded",                 "string", true,  "dateAdded",              "" },
  { "file",                      "string", true,  "strPathFile",            "CONCAT(path.strPath, strFilename) AS strPathFile" }, 
  { "",                          "string", true,  "strPath",                "path.strPath AS strPath" },
  { "album",                     "string", true,  "strAlbum",               "album.strAlbum AS strAlbum" },
  { "albumreleasetype",          "string", true,  "strAlbumReleaseType",    "album.strReleaseType AS strAlbumReleaseType" },
  { "musicbrainzalbumid",        "string", true,  "strMusicBrainzAlbumID",  "album.strMusicBrainzAlbumID AS strMusicBrainzAlbumID" },

  // JOIN fields (multivalue), same order as _JoinToSongFields 
  { "albumartistid",              "array", false, "idAlbumArtist",          "album_artist.idArtist AS idAlbumArtist" },
  { "albumartist",                "array", false, "strAlbumArtist",         "albumartist.strArtist AS strAlbumArtist" },
  { "musicbrainzalbumartistid",   "array", false, "strAlbumArtistMBID",     "albumartist.strMusicBrainzArtistID AS strAlbumArtistMBID" },
  { "",                                "", false, "iOrderAlbumArtist",      "album_artist.iOrder AS iOrderAlbumArtist" },
  { "artistid",                   "array", false, "idArtist",               "song_artist.idArtist AS idArtist" },
  { "artist",                     "array", false, "strArtist",              "songartist.strArtist AS strArtist" },
  { "musicbrainzartistid",        "array", false, "strArtistMBID",          "songartist.strMusicBrainzArtistID AS strArtistMBID" },
  { "",                                "", false, "iOrderArtist",           "song_artist.iOrder AS iOrderArtist" },
  { "",                                "", false, "idRole",                 "song_artist.idRole" },
  { "",                                "", false, "strRole",                "role.strRole" },
  { "",                                "", false, "iOrderRole",             "song_artist.iOrder AS iOrderRole" },
  { "genreid",                    "array", false, "idGenre",                "song_genre.idGenre AS idGenre" }, // Not GROUP_CONCAT as can't control order
  { "",                                "", false, "iOrderGenre",            "song_genre.idOrder AS iOrderGenre" },

  { "contributors",               "array", false, "Role_All",               "song_artist.idRole AS Role_All" },
  { "displaycomposer",           "string", false, "Role_Composer",          "song_artist.idRole AS Role_Composer" },
  { "displayconductor",          "string", false, "Role_Conductor",         "song_artist.idRole AS Role_Conductor" },
  { "displayorchestra",          "string", false, "Role_Orchestra",         "song_artist.idRole AS Role_Orchestra" },
  { "displaylyricist",           "string", false, "Role_Lyricist",          "song_artist.idRole AS Role_Lyricist" },
  
  // Scalar subquery fields
  { "track",                    "integer", true,  "track",                  "(iTrack & 0xffff) AS track" },
  { "disc",                     "integer", true,  "disc",                   "(iTrack >> 16) AS disc" },
  { "sourceid",                  "string", true,  "sourceid",               "(SELECT GROUP_CONCAT(album_source.idSource SEPARATOR '; ') FROM album_source WHERE album_source.idAlbum = song.idAlbum) AS sources" },
  { "",                                "", true,  "artistsortname",         "CASE WHEN song.strArtistSort IS NOT NULL THEN song.strArtistSort ELSE song.strArtistDisp END AS artistsortname"}
  /* 
  Song "thumbnail", "fanart" and "art" fields of JSON schema are fetched using
  thumbloader and separate queries to allow for fallback strategy
  "lyrics"?? Can be set for an item (by addons) but not held in db so
  AudioLibrary.GetSongs() never fills this field despite being in schema

   FROM ( SELECT * FROM song 
     JOIN album ON album.idAlbum = song.idAlbum
     JOIN path ON path.idPath = song.idPath) AS sv
   JOIN album_artist ON album_artist.idAlbum = song.idAlbum
   JOIN artist AS albumartist ON albumartist.idArtist = album_artist.idArtist
   JOIN song_artist ON song_artist.idSong = song.idSong
   JOIN artist AS artistsong ON artistsong.idArtist  = song_artist.idArtist
   JOIN role ON song_artist.idRole = role.idRole
   LEFT JOIN song_genre ON song.idSong = song_genre.idSong

  */
};

static const size_t NUM_SONG_FIELDS = sizeof(JSONtoDBSong) / sizeof(translateJSONField);

bool CMusicDatabase::GetSongsByWhereJSON(const std::set<std::string>& fields, const std::string &baseDir,
  CVariant& result, int& total, const SortDescription &sortDescription /* = SortDescription() */)
{

  if (NULL == m_pDB.get()) return false;
  if (NULL == m_pDS.get()) return false;

  try
  {
    total = -1;

    Filter extFilter;
    CMusicDbUrl musicUrl;
    SortDescription sorting = sortDescription;
    if (!musicUrl.FromString(baseDir) || !GetFilter(musicUrl, extFilter, sorting))
      return false;

    std::string strSQLExtra;
    if (!BuildSQL(strSQLExtra, extFilter, strSQLExtra))
      return false;

    // Count number of songs that satisfy selection criteria 
    // (includes xsp limits from filter, but not sort limits)
    // Use songview as filter rules in where clause may use album and path JOIN fields
    total = static_cast<int>(strtol(GetSingleValue("SELECT COUNT(1) FROM songview " + strSQLExtra, m_pDS).c_str(), nullptr, 10));

    // Replace view names in filter with table names
    StringUtils::Replace(extFilter.where, "artistview", "artist");
    StringUtils::Replace(extFilter.where, "albumview", "album");
    StringUtils::Replace(extFilter.where, "songview.strPath", "strPath");
    StringUtils::Replace(extFilter.where, "songview.strAlbum", "strAlbum");
    StringUtils::Replace(extFilter.where, "songview", "song");
    StringUtils::Replace(extFilter.where, "songartistview", "song_artist");

    // JOIN album and path tables needed by filter rules in where clause
    if (extFilter.where.find("album.") != std::string::npos ||
      extFilter.where.find("strAlbum") != std::string::npos)
    { // All songs have one album so inner join sufficient
      extFilter.AppendJoin("JOIN album ON album.idAlbum = song.idAlbum");
    }
    if (extFilter.where.find("strPath") != std::string::npos)
    { // All songs have one path so inner join sufficient
      extFilter.AppendJoin("JOIN path ON path.idPath = song.idPath");
    }

    //! @todo: use SortAttributeUseArtistSortName and remove articles
    std::vector<std::string> orderfields;
    std::string DESC;
    if (sortDescription.sortOrder == SortOrderDescending)
      DESC = " DESC";
    if (sortDescription.sortBy == SortByRandom)
      orderfields.emplace_back(PrepareSQL("RANDOM()")); //Adjust styntax
    else if (sortDescription.sortBy == SortByLabel)
    {
      orderfields.emplace_back("song.iTrack");
      orderfields.emplace_back("strTitle");
    }
    else if (sortDescription.sortBy == SortByTrackNumber)
      orderfields.emplace_back("song.iTrack");
    else if (sortDescription.sortBy == SortByTitle)
      orderfields.emplace_back("strTitle");
    else if (sortDescription.sortBy == SortByAlbum)
    {
      orderfields.emplace_back("strAlbum");
      orderfields.emplace_back("song.strArtistDisp");
      orderfields.emplace_back("song.iTrack");
    }    
    else if (sortDescription.sortBy == SortByArtist)
    {
      orderfields.emplace_back("song.strArtistDisp");
      orderfields.emplace_back("strAlbum");
      orderfields.emplace_back("song.iTrack");
    }
    else if (sortDescription.sortBy == SortByArtistThenYear)
    {
      orderfields.emplace_back("song.strArtistDisp");
      orderfields.emplace_back("iYear");
      orderfields.emplace_back("strAlbum");
      orderfields.emplace_back("song.iTrack");
    }
    else if (sortDescription.sortBy == SortByYear)
    {
      orderfields.emplace_back("song.iYear");
      orderfields.emplace_back("strAlbum");
      orderfields.emplace_back("song.iTrack");
      orderfields.emplace_back("strTitle");
    }
    else if (sortDescription.sortBy == SortByGenre)
    {
      orderfields.emplace_back("song.strGenres");
      orderfields.emplace_back("strTitle");
      orderfields.emplace_back("song.strArtistDisp");
    }
    else if (sortDescription.sortBy == SortByDateAdded)
      orderfields.emplace_back("dateAdded");
    else if (sortDescription.sortBy == SortByPlaycount)
    {
      orderfields.emplace_back("iTimesPlayed");
      orderfields.emplace_back("song.iTrack");
      orderfields.emplace_back("strTitle");
    }
    else if (sortDescription.sortBy == SortByLastPlayed)
    {
      orderfields.emplace_back("lastPlayed");
      orderfields.emplace_back("song.iTrack");
      orderfields.emplace_back("strTitle");
    }
    else if (sortDescription.sortBy == SortByRating)
    {
      orderfields.emplace_back("fRating");
      orderfields.emplace_back("song.iTrack");
      orderfields.emplace_back("strTitle");
    }
    else if (sortDescription.sortBy == SortByVotes)
    {
      orderfields.emplace_back("iVotes");
      orderfields.emplace_back("song.iTrack");
      orderfields.emplace_back("strTitle");
    }
    else if (sortDescription.sortBy == SortByUserRating)
    {
      orderfields.emplace_back("userrating");
      orderfields.emplace_back("song.iTrack");
      orderfields.emplace_back("strTitle");
    }
    else if (sortDescription.sortBy == SortByFile)
      orderfields.emplace_back("strPathFile");
    else if (sortDescription.sortBy == SortByTime)
      orderfields.emplace_back("iDuration");

    // Always sort by id to define order when other fields same
    if (sortDescription.sortBy != SortByRandom)      
      orderfields.emplace_back("song.idSong");

    // Fill inline view filter order fields, and build sort scalar subquery SQL
    std::string artistsortSQL;
    for (auto& name : orderfields)
    {
      //Add field for adjusted name sorting using sort name and ignoring articles
      if (name.compare("song.strArtistDisp") == 0)
      {
        artistsortSQL = SortnameBuildSQL("artistsortname", sortDescription.sortAttributes, 
          "song.strArtistDisp", "song.strArtistSort");
        if (!artistsortSQL.empty())
          name = "artistsortname";
        // Natural number case insensitve sort
        extFilter.AppendOrder(AlphanumericSortSQL(name, sortDescription.sortOrder));
      }
      else if (name.compare("strTitle") == 0  || 
               name.compare("strAlbum") == 0 || 
               name.compare("song.strGenres") == 0)
        // Natural number case insensitve sort
        extFilter.AppendOrder(AlphanumericSortSQL(name, sortDescription.sortOrder));
      else

      extFilter.AppendOrder(name + DESC);
    }

    std::string strSQL;

    // Setup fields to query, and song field number mapping
    // Find idAlbumArtist in JSONtoDBSong, offset of first join field 
    int index_idAlbumArtist = -1;
    for (unsigned int i = 0; i < NUM_SONG_FIELDS; i++)
    {
      if (JSONtoDBSong[i].fieldDB == "idAlbumArtist")
      {
        index_idAlbumArtist = i;
        break;
      }
    }    
    Filter joinFilter;
    DatasetLayout joinLayout(static_cast<size_t>(joinToSongs_enumCount));
    extFilter.AppendField("song.idSong");  // ID "songid" in JSON
    std::vector<int> dbfieldindex;
    // JSON "label" field is strTitle which may also be requested as "title", query field once output twice
    extFilter.AppendField(JSONtoDBSong[0].fieldDB);
    if (fields.find(JSONtoDBSong[0].fieldJSON) != fields.end())
      dbfieldindex.emplace_back(0); // Output "title"
    else
      dbfieldindex.emplace_back(-1); // Fetch but not output
    std::vector<std::string> rolefieldlist;
    std::vector<int> roleidlist;
    // Check each optional db field that could be retrieved (not label)
    // and fields in sort order to query in inline view but not output
    for (unsigned int i = 1; i < NUM_SONG_FIELDS; i++)
    {
      bool foundOrderby(false);
      bool foundJSON = fields.find(JSONtoDBSong[i].fieldJSON) != fields.end();
      if (!foundJSON)
        foundOrderby = std::find(orderfields.begin(), orderfields.end(), JSONtoDBSong[i].fieldDB) != orderfields.end();
      if (foundOrderby || foundJSON)
      {
        if (JSONtoDBSong[i].bSimple)
        {
          // Store indexes of requested album table and scalar subquery fields 
          // to be output, and -1 when not output to JSON
          if (!foundJSON)
            dbfieldindex.emplace_back(-1);
          else
            dbfieldindex.emplace_back(i);
          // Field from scaler subquery
          if (!JSONtoDBSong[i].SQL.empty())
          { 
            if (JSONtoDBSong[i].fieldDB == "artistsortname")
              extFilter.AppendField(artistsortSQL);
            else
              extFilter.AppendField(PrepareSQL(JSONtoDBSong[i].SQL));
          }
          else
            // Field from song table
            extFilter.AppendField(JSONtoDBSong[i].fieldDB);
        }
        else
        {  // Field from join
          if (!StringUtils::StartsWith(JSONtoDBSong[i].fieldDB, "Role_"))
          {
            joinLayout.SetField(i - index_idAlbumArtist, JSONtoDBSong[i].SQL, true);
          }
          else
          { // "contributors", "displaycomposer" etc.
            rolefieldlist.emplace_back(JSONtoDBSong[i].fieldJSON);
          }
        }
      }
    }
    // Build matching list of role id for "displaycomposer", "displayconductor", 
    // "displayorchestra", "displaylyricist"
    for (const auto& name : rolefieldlist)
    {
      int idRole = -1;
      if (StringUtils::StartsWith(name, "display"))
        idRole = GetRoleByName(name.substr(7));
      roleidlist.emplace_back(idRole);
    }

    // JOIN album and path tables needed for field output and/or in sort
    // if not already there for filter
    if ((extFilter.fields.find("album.") != std::string::npos ||
         extFilter.fields.find("strAlbum") != std::string::npos) &&
        extFilter.join.find("JOIN album") == std::string::npos)
    { // All songs have one album so inner join sufficient
      extFilter.AppendJoin("JOIN album ON album.idAlbum = song.idAlbum");
    }
    if (extFilter.fields.find("path.") != std::string::npos &&
        extFilter.join.find("JOIN path") == std::string::npos)
    { // All songs have one path so inner join sufficient
      extFilter.AppendJoin("JOIN path ON path.idPath = song.idPath");
    }
    
    // Build JOIN, WHERE, ORDER BY and LIMIT for inline view
    strSQLExtra = "";
    if (!BuildSQL(strSQLExtra, extFilter, strSQLExtra))
      return false;

    // Add any LIMIT clause to strSQLExtra
    if (extFilter.limit.empty() &&
      (sortDescription.limitStart > 0 || sortDescription.limitEnd > 0))
    {
      strSQLExtra += DatabaseUtils::BuildLimitClause(sortDescription.limitEnd, sortDescription.limitStart);
    }
    
    // Setup multivalue JOINs, GROUP BY and ORDER BY
    bool bJoinSongArtist(false);
    bool bJoinAlbumArtist(false);
    bool bJoinRole(false);
    if (sortDescription.sortBy != SortByRandom)
    {
      // Repeat inline view order (that always includes idSong) on join query
      std::string order = extFilter.order;
      order = extFilter.order;
      StringUtils::Replace(order, "album.", "sv.");
      StringUtils::Replace(order, "song.", "sv.");
      joinFilter.AppendOrder(order);
    }
    else
      joinFilter.AppendOrder("sv.idSong");
    joinFilter.AppendGroup("sv.idSong");
    
    // Album artists
    if (joinLayout.GetFetch(joinToSongs_idAlbumArtist) ||
        joinLayout.GetFetch(joinToSongs_strAlbumArtist) ||
        joinLayout.GetFetch(joinToSongs_strAlbumArtistMBID))
    { // All songs have at least one album artist so inner join sufficient
      bJoinAlbumArtist = true;
      joinFilter.AppendJoin("JOIN album_artist ON album_artist.idAlbum = sv.idAlbum");
      joinFilter.AppendGroup("album_artist.idArtist");
      joinFilter.AppendOrder("album_artist.iOrder");
      // Ensure idAlbumArtist is queried for processing repeats
      if (!joinLayout.GetFetch(joinToSongs_idAlbumArtist))
      {
        joinLayout.SetField(joinToSongs_idAlbumArtist, 
          JSONtoDBSong[index_idAlbumArtist + joinToSongs_idAlbumArtist].SQL);
      }      
      // Ensure song.IdAlbum is field of the inline view for join
      if (fields.find("albumid") == fields.end())
      {
        extFilter.AppendField("song.idAlbum"); //Prefer lookup JSONtoDBSong[XXX].dbField);
        dbfieldindex.emplace_back(-1);
      }
      // artist table needed for strArtist or MBID 
      // (album_artist.strArtist can be an alias or spelling variation) 
      if (joinLayout.GetFetch(joinToSongs_strAlbumArtistMBID) || joinLayout.GetFetch(joinToSongs_strAlbumArtist))
        joinFilter.AppendJoin("JOIN artist AS albumartist ON albumartist.idArtist = album_artist.idArtist");
    }

    /*
     Song artists
     JSON schema "artist", "artistid", "musicbrainzartistid", "contributors",
     "displaycomposer", "displayconductor", "displayorchestra", "displaylyricist",
    */
    if (joinLayout.GetFetch(joinToSongs_idArtist) ||
        joinLayout.GetFetch(joinToSongs_strArtist) ||
        joinLayout.GetFetch(joinToSongs_strArtistMBID) ||
        !rolefieldlist.empty())
    { // All songs have at least one artist (idRole = 1) so inner join sufficient
      bJoinSongArtist = true;
      if (rolefieldlist.empty())
      { // song artists only, no other roles needed
        joinFilter.AppendJoin("JOIN song_artist ON song_artist.idSong = sv.idSong AND song_artist.idRole = 1");
        joinFilter.AppendGroup("song_artist.idArtist");
        joinFilter.AppendOrder("song_artist.iOrder");
      }
      else 
      {
        // Ensure idRole is queried
        if (!joinLayout.GetFetch(joinToSongs_idRole))
        {
          joinLayout.SetField(joinToSongs_idRole,
            JSONtoDBSong[index_idAlbumArtist + joinToSongs_idRole].SQL);
        }
        // Ensure strArtist is queried
        if (!joinLayout.GetFetch(joinToSongs_strArtist))
        {
          joinLayout.SetField(joinToSongs_strArtist,
            JSONtoDBSong[index_idAlbumArtist + joinToSongs_strArtist].SQL);
        }
        if (fields.find("contributors") != fields.end())
        { // all roles
          bJoinRole = true;
          // Ensure strRole is queried from role table
          joinLayout.SetField(joinToSongs_strRole, "role.strRole");
          joinFilter.AppendJoin("JOIN song_artist ON song_artist.idSong = sv.idSong");
          joinFilter.AppendJoin("JOIN role ON song_artist.idRole = role.idRole");
          joinFilter.AppendGroup("song_artist.idArtist, song_artist.idRole");
          joinFilter.AppendOrder("song_artist.idRole, song_artist.iOrder, song_artist.idArtist");
        }
        else
        { // Get just roles for  "displaycomposer", "displayconductor" etc.
          std::string where;
          for (size_t i = 0; i < roleidlist.size(); i++)
          {          
            int idRole = roleidlist[i];
            if (idRole <= 1)
              continue;
            if (where.empty())
              // Always get song artists too (role = 1) so can do inner join
              where = PrepareSQL("song_artist.idRole = 1 OR song_artist.idRole = %i", idRole);
            else
              where += PrepareSQL(" OR song_artist.idRole = %i", idRole);
          }
          where = " (" + where + ")";
          joinFilter.AppendJoin("JOIN song_artist ON song_artist.idSong = sv.idSong AND " + where);
          joinFilter.AppendGroup("song_artist.idArtist, song_artist.idRole");
          joinFilter.AppendOrder("song_artist.idRole, song_artist.iOrder, song_artist.idArtist");
        }
      }
      // Ensure idArtist is queried for processing repeats
      if (!joinLayout.GetFetch(joinToSongs_idArtist))
      {
        joinLayout.SetField(joinToSongs_idArtist,
          JSONtoDBSong[index_idAlbumArtist + joinToSongs_idArtist].SQL);
      }
      // artist table needed for strArtist or MBID 
      // (song_artist.strArtist can be an alias or spelling variation) 
      if (joinLayout.GetFetch(joinToSongs_strArtistMBID) || joinLayout.GetFetch(joinToSongs_strArtist))
        joinFilter.AppendJoin("JOIN artist AS songartist ON songartist.idArtist = song_artist.idArtist");
    }    

    // Genre ids
    if (joinLayout.GetFetch(joinToSongs_idGenre))
    { // song genre ids (strGenre demormalised in song table)
      // Left join as songs may not have genre      
      joinFilter.AppendJoin("LEFT JOIN song_genre ON song_genre.idSong = sv.idSong");
      joinFilter.AppendGroup("song_genre.idGenre");
      joinFilter.AppendOrder("song_genre.iOrder");
    }

    // Build JOIN part of query (if we have one)
    std::string strSQLJoin;
    if (joinLayout.HasFilterFields())
      if (!BuildSQL(strSQLJoin, joinFilter, strSQLJoin))
        return false;

    // Adjust where in the results record the join fields are allowing for the
    // inline view fields (Quicker than finding field by name every time)
    // idSong + other song fields
    joinLayout.AdjustRecordNumbers(1 + dbfieldindex.size());

    // Build full query
    // When have multiple value joins use inline view
    // SELECT sv.*, <join fields> FROM 
    //   (SELECT <song fields> FROM song <JOIN album> <where> + <order by> +  <limits> ) AS sv 
    //   <joins> <group by>
    //   <order by> + <joins order by>
    // Don't use prepareSQL - confuses  releasetype = 'album' filter and group_concat separator
    strSQL = "SELECT " + extFilter.fields + " FROM song " + strSQLExtra;
    if (joinLayout.HasFilterFields())
    {
      strSQL = "("+ strSQL + ") AS sv ";
      strSQL = "SELECT sv.*, " + joinLayout.GetFields() + " FROM " + strSQL + strSQLJoin;
    }
    
    CLog::Log(LOGDEBUG, "%s query: %s", __FUNCTION__, strSQL.c_str());

    // Run query
    unsigned int time = XbmcThreads::SystemClockMillis();
    if (!m_pDS->query(strSQL))
      return false;
    CLog::Log(LOGDEBUG, "%s - query took %i ms",
      __FUNCTION__, XbmcThreads::SystemClockMillis() - time); time = XbmcThreads::SystemClockMillis();

    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound <= 0)
    {
      m_pDS->close();
      return true;
    }

    DatabaseResults dbResults;
    dbResults.reserve(iRowsFound);

    // Get song from returned rows. Joins mean there can be many rows per song
    int songId = -1;
    int albumartistId = -1;
    int artistId = -1;
    int roleId = -1;
    bool bSongGenreDone(false);
    bool bSongArtistDone(false);
    bool bHaveSong(false);
    CVariant songObj;
    while (!m_pDS->eof() || bHaveSong)
    {
      const dbiplus::sql_record* const record = m_pDS->get_sql_record();

      if (m_pDS->eof() || songId != record->at(0).get_asInt())
      {
        // Store previous or last song
        if (bHaveSong)
        {
          // Check empty role fields get returned, and format
          for (const auto& displayXXX : rolefieldlist)
          {
            if (!StringUtils::StartsWith(displayXXX, "display"))
            {
              // "contributors"
              if (!songObj.isMember(displayXXX))
                songObj[displayXXX] = CVariant(CVariant::VariantTypeArray);
            }
            else if (songObj.isMember(displayXXX) && songObj[displayXXX].isArray())
            {
              // Convert "displaycomposer", "displayconductor", "displayorchestra",
              // and "displaylyricist" arrays into strings
              std::vector<std::string> names;
              for (CVariant::const_iterator_array field = songObj[displayXXX].begin_array();
                field != songObj[displayXXX].end_array(); field++)
                names.emplace_back(field->asString());

              std::string role = StringUtils::Join(names, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_musicItemSeparator);
              songObj[displayXXX] = role;
            }
            else
              songObj[displayXXX] = "";
          }

          result["songs"].append(songObj);
          bHaveSong = false;
          songObj.clear();
        }
        if (songObj.empty())
        {
          // Initialise fields, ensure those with possible null values are set to correct empty variant type
          if (joinLayout.GetOutput(joinToSongs_idGenre))
            songObj["genreid"] = CVariant(CVariant::VariantTypeArray); //"genre" set [] by split of array

          albumartistId = -1;
          artistId = -1;
          roleId = -1;
          bSongGenreDone = false;
          bSongArtistDone = false;
        }
        if (m_pDS->eof())
          continue;  // Having saved the last song stop

        // New song
        songId = record->at(0).get_asInt();
        bHaveSong = true;
        songObj["songid"] = songId;
        songObj["label"] = record->at(1).get_asString();
        for (size_t i = 0; i < dbfieldindex.size(); i++)
          if (dbfieldindex[i] > -1)
          {
            if (JSONtoDBSong[dbfieldindex[i]].formatJSON == "integer")
              songObj[JSONtoDBSong[dbfieldindex[i]].fieldJSON] = record->at(1 + i).get_asInt();
            else if (JSONtoDBSong[dbfieldindex[i]].formatJSON == "unsigned")
              songObj[JSONtoDBSong[dbfieldindex[i]].fieldJSON] = std::max(record->at(1 + i).get_asInt(), 0);
            else if (JSONtoDBSong[dbfieldindex[i]].formatJSON == "float")
              songObj[JSONtoDBSong[dbfieldindex[i]].fieldJSON] = std::max(record->at(1 + i).get_asFloat(), 0.f);
            else if (JSONtoDBSong[dbfieldindex[i]].formatJSON == "array")
              songObj[JSONtoDBSong[dbfieldindex[i]].fieldJSON] = StringUtils::Split(record->at(1 + i).get_asString(), CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_musicItemSeparator);
            else if (JSONtoDBSong[dbfieldindex[i]].formatJSON == "boolean")
              songObj[JSONtoDBSong[dbfieldindex[i]].fieldJSON] = record->at(1 + i).get_asBool();
            else
              songObj[JSONtoDBSong[dbfieldindex[i]].fieldJSON] = record->at(1 + i).get_asString();
          }

        // Split sources string into int array
        if (songObj.isMember("sourceid"))
        {
          std::vector<std::string> sources = StringUtils::Split(songObj["sourceid"].asString(), ";");
          songObj["sourceid"] = CVariant(CVariant::VariantTypeArray);
          for (size_t i = 0; i < sources.size(); i++)
            songObj["sourceid"].append(atoi(sources[i].c_str()));
        }
      }

      if (bJoinAlbumArtist)
      {
        if (albumartistId != record->at(joinLayout.GetRecNo(joinToSongs_idAlbumArtist)).get_asInt())
        {
          bSongGenreDone = bSongGenreDone || (albumartistId > 0);  // Not first album artist, skip genre
          bSongArtistDone = bSongArtistDone || (albumartistId > 0);  // Not first album artist, skip song artists
          albumartistId = record->at(joinLayout.GetRecNo(joinToSongs_idAlbumArtist)).get_asInt();
          if (joinLayout.GetOutput(joinToSongs_idAlbumArtist))
            songObj["albumartistid"].append(albumartistId);
          if (albumartistId == BLANKARTIST_ID)
          {
            if (joinLayout.GetOutput(joinToSongs_strAlbumArtist))
              songObj["albumartist"].append(StringUtils::Empty);
            if (joinLayout.GetOutput(joinToSongs_strAlbumArtistMBID))
              songObj["musicbrainzalbumartistid"].append(StringUtils::Empty);
          }
          else
          {
            if (joinLayout.GetOutput(joinToSongs_idAlbumArtist))
              songObj["albumartistid"].append(albumartistId);
            if (joinLayout.GetOutput(joinToSongs_strAlbumArtist))
              songObj["albumartist"].append(record->at(joinLayout.GetRecNo(joinToSongs_strAlbumArtist)).get_asString());
            if (joinLayout.GetOutput(joinToSongs_strAlbumArtistMBID))
              songObj["musicbrainzalbumartistid"].append(record->at(joinLayout.GetRecNo(joinToSongs_strAlbumArtistMBID)).get_asString());
          }
        }
      }
      if (bJoinSongArtist && !bSongArtistDone)
      {
        if (artistId != record->at(joinLayout.GetRecNo(joinToSongs_idArtist)).get_asInt())
        {
          bSongGenreDone = bSongGenreDone || (artistId > 0);  // Not first artist, skip genre
          roleId = -1; // Allow for many artists same role
          artistId = record->at(joinLayout.GetRecNo(joinToSongs_idArtist)).get_asInt();
          if (joinLayout.GetRecNo(joinToSongs_idRole) < 0 ||
              record->at(joinLayout.GetRecNo(joinToSongs_idRole)).get_asInt() == 1)
          {
            if (joinLayout.GetOutput(joinToSongs_idArtist))
              songObj["artistid"].append(artistId);
            if (artistId == BLANKARTIST_ID)
            {
              if (joinLayout.GetOutput(joinToSongs_strArtist))
                songObj["artist"].append(StringUtils::Empty);
              if (joinLayout.GetOutput(joinToSongs_strArtistMBID))
                songObj["musicbrainzartistid"].append(StringUtils::Empty);
            }
            else
            {
              if (joinLayout.GetOutput(joinToSongs_strArtist))
                songObj["artist"].append(record->at(joinLayout.GetRecNo(joinToSongs_strArtist)).get_asString());
              if (joinLayout.GetOutput(joinToSongs_strArtistMBID))
                songObj["musicbrainzartistid"].append(record->at(joinLayout.GetRecNo(joinToSongs_strArtistMBID)).get_asString());
            }
          }
        }
        if (joinLayout.GetRecNo(joinToSongs_idRole) > 0 &&
            roleId != record->at(joinLayout.GetRecNo(joinToSongs_idRole)).get_asInt())
        {
          bSongGenreDone = bSongGenreDone || (roleId > 0);  // Not first role, skip genre
          roleId = record->at(joinLayout.GetRecNo(joinToSongs_idRole)).get_asInt();
          if (roleId > 1)
          {
            if (bJoinRole)
            {  //Contributors
               CVariant contributor;
               contributor["name"] = record->at(joinLayout.GetRecNo(joinToSongs_strArtist)).get_asString();
               contributor["role"] = record->at(joinLayout.GetRecNo(joinToSongs_strRole)).get_asString();
               contributor["roleid"] = roleId;
               contributor["artistid"] = record->at(joinLayout.GetRecNo(joinToSongs_idArtist)).get_asInt();
               songObj["contributors"].append(contributor);               
            }
            // "displaycomposer", "displayconductor" etc.
            for (size_t i = 0; i < roleidlist.size(); i++)
            {
              if (roleidlist[i] == roleId)
              {
                songObj[rolefieldlist[i]].append(record->at(joinLayout.GetRecNo(joinToSongs_strArtist)).get_asString());
                continue;
              }
            }
          }
        }
      }
      if (!bSongGenreDone && joinLayout.GetRecNo(joinToSongs_idGenre) > -1 &&
          !record->at(joinLayout.GetRecNo(joinToSongs_idGenre)).get_isNull())
      {
        songObj["genreid"].append(record->at(joinLayout.GetRecNo(joinToSongs_idGenre)).get_asInt());
      }
      m_pDS->next();
    }
    m_pDS->close(); // cleanup recordset data

    // Ensure random order of output when results set is sorted to process multi-value joins
    if (sortDescription.sortBy == SortByRandom && joinLayout.HasFilterFields())
      KODI::UTILS::RandomShuffle(result["songs"].begin_array(), result["songs"].end_array());

    return true;
  }
  catch (...)
  {
    m_pDS->close();
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}

std::string CMusicDatabase::GetIgnoreArticleSQL(const std::string& strField)
{
  /* 
  Make SQL clause from ignore article list.
  Group tokens the same length together, for example :
    WHEN strArtist LIKE 'the ' OR strArtist LIKE 'the.' strArtist LIKE 'the_' ESCAPE '_'
    THEN SUBSTR(strArtist, 5)
    WHEN strArtist LIKE 'an ' OR strArtist LIKE 'an.' strArtist LIKE 'an_' ESCAPE '_'
    THEN SUBSTR(strArtist, 4)
  */
  std::set<std::string> sortTokens = g_langInfo.GetSortTokens();
  std::string sortclause;
  size_t tokenlength = 0;
  std::string strWhen;
  for (const auto& token : sortTokens)
  {
    if (token.length() != tokenlength)
    {
      if (!strWhen.empty())
      {
        if (!sortclause.empty())
           sortclause += " ";
        std::string strThen = PrepareSQL(" THEN SUBSTR(%s, %i)", strField.c_str(), tokenlength + 1);
        sortclause += "WHEN " + strWhen + strThen;
        strWhen.clear();
      }
      tokenlength = token.length();
    }
    std::string tokenclause = token;
    //Escape any ' or % in the token
    StringUtils::Replace(tokenclause, "'", "''");
    StringUtils::Replace(tokenclause, "%", "%%");
    // Single %, _ and ' so avoid using PrepareSQL
    tokenclause = strField + " LIKE '" + tokenclause + "%'";
    if (token.find("_") != std::string::npos)
       tokenclause += " ESCAPE '_'";
    if (!strWhen.empty())
       strWhen += " OR ";
    strWhen += tokenclause;
  }
  if (!strWhen.empty())
  {
    if (!sortclause.empty())
       sortclause += " ";
    std::string strThen = PrepareSQL(" THEN SUBSTR(%s, %i)", strField.c_str(), tokenlength + 1);
    sortclause += "WHEN " + strWhen + strThen;
  }
  return sortclause;
}

std::string CMusicDatabase::SortnameBuildSQL(const std::string& strAlias, const SortAttribute& sortAttributes, const std::string& strField, const std::string& strSortField)
{
  /*
  Build SQL for sort name scalar subquery from sort attributes and ignore article list.
  For example :
  CASE WHEN strArtistSort IS NOT NULL THEN strArtistSort 
  WHEN strField LIKE 'the ' OR strField LIKE 'the_' ESCAPE '_' THEN SUBSTR(strArtist, 5)
  WHEN strField LIKE 'LIKE 'an.' strField LIKE 'an_' ESCAPE '_' THEN SUBSTR(strArtist, 4)
  ELSE strField
  END AS strAlias
  */

  std::string artistsortSQL;
  if (sortAttributes & SortAttributeUseArtistSortName)
    artistsortSQL = PrepareSQL("WHEN %s IS NOT NULL THEN %s ", strSortField.c_str(), strSortField.c_str());
  if (sortAttributes & SortAttributeIgnoreArticle)
  {
    if (!artistsortSQL.empty())
      artistsortSQL += " ";
    // Make SQL from ignore article list, grouping tokens the same length together
    artistsortSQL += GetIgnoreArticleSQL(strField);
  }
  if (!artistsortSQL.empty())
  {
    artistsortSQL = "CASE " + artistsortSQL;  // Not prepare as may contain ' and % etc.
    artistsortSQL += PrepareSQL(" ELSE %s END AS %s", strField.c_str(), strAlias.c_str());
  }

  return artistsortSQL;
}

std::string CMusicDatabase::AlphanumericSortSQL(const std::string& strField, const SortOrder& sortOrder)
{
  /*
  Make sort of initial numbers natural, and case insensitive in SQLite.
  Collation NOCASE ould be more efficient done in table create.
  MySQL uses case insensitive utf8_general_ci collation defined for tables.
  Use PrepareSQL to adjust syntax removing NOCASE and add AS UNSIGNED INTEGER
  */
  std::string DESC;
  if (sortOrder == SortOrderDescending)
    DESC = " DESC";
  return PrepareSQL("CASE WHEN CAST(%s AS INTEGER) = 0 "
    "THEN 100000000 ELSE CAST(%s AS INTEGER) END%s, "
    "%s COLLATE NOCASE%s",
    strField.c_str(), strField.c_str(), DESC.c_str(), strField.c_str(), DESC.c_str());
}

void CMusicDatabase::UpdateTables(int version)
{
  CLog::Log(LOGINFO, "%s - updating tables", __FUNCTION__);
  if (version < 34)
  {
    m_pDS->exec("ALTER TABLE artist ADD strMusicBrainzArtistID text\n");
    m_pDS->exec("ALTER TABLE album ADD strMusicBrainzAlbumID text\n");
    m_pDS->exec("CREATE TABLE song_new ( idSong integer primary key, idAlbum integer, idPath integer, strArtists text, strGenres text, strTitle varchar(512), iTrack integer, iDuration integer, iYear integer, dwFileNameCRC text, strFileName text, strMusicBrainzTrackID text, iTimesPlayed integer, iStartOffset integer, iEndOffset integer, idThumb integer, lastplayed varchar(20) default NULL, rating char default '0', comment text)\n");
    m_pDS->exec("INSERT INTO song_new ( idSong, idAlbum, idPath, strArtists, strTitle, iTrack, iDuration, iYear, dwFileNameCRC, strFileName, strMusicBrainzTrackID, iTimesPlayed, iStartOffset, iEndOffset, idThumb, lastplayed, rating, comment) SELECT idSong, idAlbum, idPath, strArtists, strTitle, iTrack, iDuration, iYear, dwFileNameCRC, strFileName, strMusicBrainzTrackID, iTimesPlayed, iStartOffset, iEndOffset, idThumb, lastplayed, rating, comment FROM song");

    m_pDS->exec("DROP TABLE song");
    m_pDS->exec("ALTER TABLE song_new RENAME TO song");

    m_pDS->exec("UPDATE song SET strMusicBrainzTrackID = NULL");
  }

  if (version < 36)
  {
    // translate legacy musicdb:// paths
    if (m_pDS->query("SELECT strPath FROM content"))
    {
      std::vector<std::string> contentPaths;
      while (!m_pDS->eof())
      {
        contentPaths.push_back(m_pDS->fv(0).get_asString());
        m_pDS->next();
      }
      m_pDS->close();

      for (const auto &originalPath : contentPaths)
      {
        std::string path = CLegacyPathTranslation::TranslateMusicDbPath(originalPath);
        m_pDS->exec(PrepareSQL("UPDATE content SET strPath='%s' WHERE strPath='%s'", path.c_str(), originalPath.c_str()));
      }
    }
  }

  if (version < 39)
  {
    m_pDS->exec("CREATE TABLE album_new "
                "(idAlbum integer primary key, "
                " strAlbum varchar(256), strMusicBrainzAlbumID text, "
                " strArtists text, strGenres text, "
                " iYear integer, idThumb integer, "
                " bCompilation integer not null default '0', "
                " strMoods text, strStyles text, strThemes text, "
                " strReview text, strImage text, strLabel text, "
                " strType text, "
                " iRating integer, "
                " lastScraped varchar(20) default NULL, "
                " dateAdded varchar (20) default NULL)");
    m_pDS->exec("INSERT INTO album_new "
                "(idAlbum, "
                " strAlbum, strMusicBrainzAlbumID, "
                " strArtists, strGenres, "
                " iYear, idThumb, "
                " bCompilation, "
                " strMoods, strStyles, strThemes, "
                " strReview, strImage, strLabel, "
                " strType, "
                " iRating) "
                " SELECT "
                " album.idAlbum, "
                " strAlbum, strMusicBrainzAlbumID, "
                " strArtists, strGenres, "
                " album.iYear, idThumb, "
                " bCompilation, "
                " strMoods, strStyles, strThemes, "
                " strReview, strImage, strLabel, "
                " strType, iRating "
                " FROM album LEFT JOIN albuminfo ON album.idAlbum = albuminfo.idAlbum");
    m_pDS->exec("UPDATE albuminfosong SET idAlbumInfo = (SELECT idAlbum FROM albuminfo WHERE albuminfo.idAlbumInfo = albuminfosong.idAlbumInfo)");
    m_pDS->exec(PrepareSQL("UPDATE album_new SET lastScraped='%s' WHERE idAlbum IN (SELECT idAlbum FROM albuminfo)", CDateTime::GetCurrentDateTime().GetAsDBDateTime().c_str()));
    m_pDS->exec("DROP TABLE album");
    m_pDS->exec("DROP TABLE albuminfo");
    m_pDS->exec("ALTER TABLE album_new RENAME TO album");
  }
  if (version < 40)
  {
    m_pDS->exec("CREATE TABLE artist_new ( idArtist integer primary key, "
                " strArtist varchar(256), strMusicBrainzArtistID text, "
                " strBorn text, strFormed text, strGenres text, strMoods text, "
                " strStyles text, strInstruments text, strBiography text, "
                " strDied text, strDisbanded text, strYearsActive text, "
                " strImage text, strFanart text, "
                " lastScraped varchar(20) default NULL, "
                " dateAdded varchar (20) default NULL)");
    m_pDS->exec("INSERT INTO artist_new "
                "(idArtist, strArtist, strMusicBrainzArtistID, "
                " strBorn, strFormed, strGenres, strMoods, "
                " strStyles , strInstruments , strBiography , "
                " strDied, strDisbanded, strYearsActive, "
                " strImage, strFanart) "
                " SELECT "
                " artist.idArtist, "
                " strArtist, strMusicBrainzArtistID, "
                " strBorn, strFormed, strGenres, strMoods, "
                " strStyles, strInstruments, strBiography, "
                " strDied, strDisbanded, strYearsActive, "
                " strImage, strFanart "
                " FROM artist "
                " LEFT JOIN artistinfo ON artist.idArtist = artistinfo.idArtist");
    m_pDS->exec(PrepareSQL("UPDATE artist_new SET lastScraped='%s' WHERE idArtist IN (SELECT idArtist FROM artistinfo)", CDateTime::GetCurrentDateTime().GetAsDBDateTime().c_str()));
    m_pDS->exec("DROP TABLE artist");
    m_pDS->exec("DROP TABLE artistinfo");
    m_pDS->exec("ALTER TABLE artist_new RENAME TO artist");
  }
  if (version < 42)
  {
    m_pDS->exec("ALTER TABLE album_artist ADD strArtist text\n");
    m_pDS->exec("ALTER TABLE song_artist ADD strArtist text\n");
    // populate these
    std::string sql = "select idArtist,strArtist from artist";
    m_pDS->query(sql);
    while (!m_pDS->eof())
    {
      m_pDS2->exec(PrepareSQL("UPDATE song_artist SET strArtist='%s' where idArtist=%i", m_pDS->fv(1).get_asString().c_str(), m_pDS->fv(0).get_asInt()));
      m_pDS2->exec(PrepareSQL("UPDATE album_artist SET strArtist='%s' where idArtist=%i", m_pDS->fv(1).get_asString().c_str(), m_pDS->fv(0).get_asInt()));
      m_pDS->next();
    }
  }
  if (version < 48)
  { // null out columns that are no longer used
    m_pDS->exec("UPDATE song SET dwFileNameCRC=NULL, idThumb=NULL");
    m_pDS->exec("UPDATE album SET idThumb=NULL");
  }
  if (version < 49)
  {
    m_pDS->exec("CREATE TABLE cue (idPath integer, strFileName text, strCuesheet text)");
  }
  if (version < 50)
  {
    // add a new column strReleaseType for albums
    m_pDS->exec("ALTER TABLE album ADD strReleaseType text\n");

    // set strReleaseType based on album name
    m_pDS->exec(PrepareSQL("UPDATE album SET strReleaseType = '%s' WHERE strAlbum IS NOT NULL AND strAlbum <> ''", CAlbum::ReleaseTypeToString(CAlbum::Album).c_str()));
    m_pDS->exec(PrepareSQL("UPDATE album SET strReleaseType = '%s' WHERE strAlbum IS NULL OR strAlbum = ''", CAlbum::ReleaseTypeToString(CAlbum::Single).c_str()));
  }
  if (version < 51)
  {
    m_pDS->exec("ALTER TABLE song ADD mood text\n");
  }
  if (version < 53)
  {
    m_pDS->exec("ALTER TABLE song ADD dateAdded text");
  }
  if (version < 54)
  {
      //Remove dateAdded from artist table
      m_pDS->exec("CREATE TABLE artist_new ( idArtist integer primary key, "
              " strArtist varchar(256), strMusicBrainzArtistID text, "
              " strBorn text, strFormed text, strGenres text, strMoods text, "
              " strStyles text, strInstruments text, strBiography text, "
              " strDied text, strDisbanded text, strYearsActive text, "
              " strImage text, strFanart text, "
              " lastScraped varchar(20) default NULL)");
      m_pDS->exec("INSERT INTO artist_new "
          "(idArtist, strArtist, strMusicBrainzArtistID, "
          " strBorn, strFormed, strGenres, strMoods, "
          " strStyles , strInstruments , strBiography , "
          " strDied, strDisbanded, strYearsActive, "
          " strImage, strFanart, lastScraped) "
          " SELECT "
          " idArtist, "
          " strArtist, strMusicBrainzArtistID, "
          " strBorn, strFormed, strGenres, strMoods, "
          " strStyles, strInstruments, strBiography, "
          " strDied, strDisbanded, strYearsActive, "
          " strImage, strFanart, lastScraped "
          " FROM artist");
      m_pDS->exec("DROP TABLE artist");
      m_pDS->exec("ALTER TABLE artist_new RENAME TO artist");

      //Remove dateAdded from album table
      m_pDS->exec("CREATE TABLE album_new (idAlbum integer primary key, "
              " strAlbum varchar(256), strMusicBrainzAlbumID text, "
              " strArtists text, strGenres text, "
              " iYear integer, idThumb integer, "
              " bCompilation integer not null default '0', "
              " strMoods text, strStyles text, strThemes text, "
              " strReview text, strImage text, strLabel text, "
              " strType text, "
              " iRating integer, "
              " lastScraped varchar(20) default NULL, "
              " strReleaseType text)");
      m_pDS->exec("INSERT INTO album_new "
          "(idAlbum, "
          " strAlbum, strMusicBrainzAlbumID, "
          " strArtists, strGenres, "
          " iYear, idThumb, "
          " bCompilation, "
          " strMoods, strStyles, strThemes, "
          " strReview, strImage, strLabel, "
          " strType, iRating, lastScraped, "
          " strReleaseType) "
          " SELECT "
          " album.idAlbum, "
          " strAlbum, strMusicBrainzAlbumID, "
          " strArtists, strGenres, "
          " iYear, idThumb, "
          " bCompilation, "
          " strMoods, strStyles, strThemes, "
          " strReview, strImage, strLabel, "
          " strType, iRating, lastScraped, "
          " strReleaseType"
          " FROM album");
      m_pDS->exec("DROP TABLE album");
      m_pDS->exec("ALTER TABLE album_new RENAME TO album");
   }
   if (version < 55)
   {
     m_pDS->exec("DROP TABLE karaokedata");
   }
   if (version < 57)
   {
     m_pDS->exec("ALTER TABLE song ADD userrating INTEGER NOT NULL DEFAULT 0");
     m_pDS->exec("UPDATE song SET rating = 0 WHERE rating < 0 or rating IS NULL");
     m_pDS->exec("UPDATE song SET userrating = rating * 2");
     m_pDS->exec("UPDATE song SET rating = 0");
     m_pDS->exec("CREATE TABLE song_new (idSong INTEGER PRIMARY KEY, "
       " idAlbum INTEGER, idPath INTEGER, "
       " strArtists TEXT, strGenres TEXT, strTitle VARCHAR(512), "
       " iTrack INTEGER, iDuration INTEGER, iYear INTEGER, "
       " dwFileNameCRC TEXT, "
       " strFileName TEXT, strMusicBrainzTrackID TEXT, "
       " iTimesPlayed INTEGER, iStartOffset INTEGER, iEndOffset INTEGER, "
       " idThumb INTEGER, "
       " lastplayed VARCHAR(20) DEFAULT NULL, "
       " rating FLOAT DEFAULT 0, "
       " userrating INTEGER DEFAULT 0, "
       " comment TEXT, mood TEXT, dateAdded TEXT)");
     m_pDS->exec("INSERT INTO song_new "
       "(idSong, "
       " idAlbum, idPath, "
       " strArtists, strGenres, strTitle, "
       " iTrack, iDuration, iYear, "
       " dwFileNameCRC, "
       " strFileName, strMusicBrainzTrackID, "
       " iTimesPlayed, iStartOffset, iEndOffset, "
       " idThumb, "
       " lastplayed,"
       " rating, userrating, "
       " comment, mood, dateAdded)"
       " SELECT "
       " idSong, "
       " idAlbum, idPath, "
       " strArtists, strGenres, strTitle, "
       " iTrack, iDuration, iYear, "
       " dwFileNameCRC, "
       " strFileName, strMusicBrainzTrackID, "
       " iTimesPlayed, iStartOffset, iEndOffset, "
       " idThumb, "
       " lastplayed,"
       " rating, "
       " userrating, "
       " comment, mood, dateAdded"
       " FROM song");
     m_pDS->exec("DROP TABLE song");
     m_pDS->exec("ALTER TABLE song_new RENAME TO song");

     m_pDS->exec("ALTER TABLE album ADD iUserrating INTEGER NOT NULL DEFAULT 0");
     m_pDS->exec("UPDATE album SET iRating = 0 WHERE iRating < 0 or iRating IS NULL");
     m_pDS->exec("CREATE TABLE album_new (idAlbum INTEGER PRIMARY KEY, "
       " strAlbum VARCHAR(256), strMusicBrainzAlbumID TEXT, "
       " strArtists TEXT, strGenres TEXT, "
       " iYear INTEGER, idThumb INTEGER, "
       " bCompilation INTEGER NOT NULL DEFAULT '0', "
       " strMoods TEXT, strStyles TEXT, strThemes TEXT, "
       " strReview TEXT, strImage TEXT, strLabel TEXT, "
       " strType TEXT, "
       " fRating FLOAT NOT NULL DEFAULT 0, "
       " iUserrating INTEGER NOT NULL DEFAULT 0, "
       " lastScraped VARCHAR(20) DEFAULT NULL, "
       " strReleaseType TEXT)");
     m_pDS->exec("INSERT INTO album_new "
       "(idAlbum, "
       " strAlbum, strMusicBrainzAlbumID, "
       " strArtists, strGenres, "
       " iYear, idThumb, "
       " bCompilation, "
       " strMoods, strStyles, strThemes, "
       " strReview, strImage, strLabel, "
       " strType, "
       " fRating, "
       " iUserrating, "
       " lastScraped, "
       " strReleaseType)"
       " SELECT "
       " idAlbum, "
       " strAlbum, strMusicBrainzAlbumID, "
       " strArtists, strGenres, "
       " iYear, idThumb, "
       " bCompilation, "
       " strMoods, strStyles, strThemes, "
       " strReview, strImage, strLabel, "
       " strType, "
       " iRating, "
       " iUserrating, "
       " lastScraped, "
       " strReleaseType"
       " FROM album");
     m_pDS->exec("DROP TABLE album");
     m_pDS->exec("ALTER TABLE album_new RENAME TO album");

     m_pDS->exec("ALTER TABLE album ADD iVotes INTEGER NOT NULL DEFAULT 0");
     m_pDS->exec("ALTER TABLE song ADD votes INTEGER NOT NULL DEFAULT 0");
   }
  if (version < 58)
  {
     m_pDS->exec("UPDATE album SET fRating = fRating * 2");
  }
  if (version < 59)
  {
    m_pDS->exec("CREATE TABLE role (idRole integer primary key, strRole text)");
    m_pDS->exec("INSERT INTO role(idRole, strRole) VALUES (1, 'Artist')"); //Default Role

    //Remove strJoinPhrase, boolFeatured from song_artist table and add idRole
    m_pDS->exec("CREATE TABLE song_artist_new (idArtist integer, idSong integer, idRole integer, iOrder integer, strArtist text)");
    m_pDS->exec("INSERT INTO song_artist_new (idArtist, idSong, idRole, iOrder, strArtist) "
      "SELECT idArtist, idSong, 1 as idRole, iOrder, strArtist FROM song_artist");
    m_pDS->exec("DROP TABLE song_artist");
    m_pDS->exec("ALTER TABLE song_artist_new RENAME TO song_artist");

    //Remove strJoinPhrase, boolFeatured from album_artist table
    m_pDS->exec("CREATE TABLE album_artist_new (idArtist integer, idAlbum integer, iOrder integer, strArtist text)");
    m_pDS->exec("INSERT INTO album_artist_new (idArtist, idAlbum, iOrder, strArtist) "
      "SELECT idArtist, idAlbum, iOrder, strArtist FROM album_artist");
    m_pDS->exec("DROP TABLE album_artist");
    m_pDS->exec("ALTER TABLE album_artist_new RENAME TO album_artist");
  }
  if (version < 60)
  {
    // From now on artist ID = 1 will be an artificial artist [Missing] use for songs that
    // do not have an artist tag to ensure all songs in the library have at least one artist.
    std::string strSQL;
    if (GetArtistExists(BLANKARTIST_ID))
    {
      // When BLANKARTIST_ID (=1) is already in use, move the record
      try
      { //No mbid index yet, so can have record for artist twice even with mbid
        strSQL = PrepareSQL("INSERT INTO artist SELECT null, "
          "strArtist, strMusicBrainzArtistID, "
          "strBorn, strFormed, strGenres, strMoods, "
          "strStyles, strInstruments, strBiography, "
          "strDied, strDisbanded, strYearsActive, "
          "strImage, strFanart, lastScraped "
          "FROM artist WHERE artist.idArtist = %i", BLANKARTIST_ID);
        m_pDS->exec(strSQL);
        int idArtist = (int)m_pDS->lastinsertid();
        //No triggers, so can delete artist without effecting other tables.
        strSQL = PrepareSQL("DELETE FROM artist WHERE artist.idArtist = %i", BLANKARTIST_ID);
        m_pDS->exec(strSQL);

        // Update related tables with the new artist ID
        // Indices have been dropped making transactions very slow, so create appropriate temp indices
        m_pDS->exec("CREATE INDEX idxSongArtist2 ON song_artist ( idArtist )");
        m_pDS->exec("CREATE INDEX idxAlbumArtist2 ON album_artist ( idArtist )");
        m_pDS->exec("CREATE INDEX idxDiscography ON discography ( idArtist )");
        m_pDS->exec("CREATE INDEX ix_art ON art ( media_id, media_type(20) )");
        strSQL = PrepareSQL("UPDATE song_artist SET idArtist = %i WHERE idArtist = %i", idArtist, BLANKARTIST_ID);
        m_pDS->exec(strSQL);
        strSQL = PrepareSQL("UPDATE album_artist SET idArtist = %i WHERE idArtist = %i", idArtist, BLANKARTIST_ID);
        m_pDS->exec(strSQL);
        strSQL = PrepareSQL("UPDATE art SET media_id = %i WHERE media_id = %i AND media_type='artist'", idArtist, BLANKARTIST_ID);
        m_pDS->exec(strSQL);
        strSQL = PrepareSQL("UPDATE discography SET idArtist = %i WHERE idArtist = %i", idArtist, BLANKARTIST_ID);
        m_pDS->exec(strSQL);
        // Drop temp indices
        m_pDS->exec("DROP INDEX idxSongArtist2 ON song_artist");
        m_pDS->exec("DROP INDEX idxAlbumArtist2 ON album_artist");
        m_pDS->exec("DROP INDEX idxDiscography ON discography");
        m_pDS->exec("DROP INDEX ix_art ON art");
      }
      catch (...)
      {
        CLog::Log(LOGERROR, "Moving existing artist to add missing tag artist has failed");
      }
    }

    // Create missing artist tag artist [Missing].
    // Fake MusicbrainzId assures uniqueness and avoids updates from scanned songs
    strSQL = PrepareSQL("INSERT INTO artist (idArtist, strArtist, strMusicBrainzArtistID) VALUES( %i, '%s', '%s' )",
      BLANKARTIST_ID, BLANKARTIST_NAME.c_str(), BLANKARTIST_FAKEMUSICBRAINZID.c_str());
    m_pDS->exec(strSQL);

    // Indices have been dropped making transactions very slow, so create temp index
    m_pDS->exec("CREATE INDEX idxSongArtist1 ON song_artist ( idSong, idRole )");
    m_pDS->exec("CREATE INDEX idxAlbumArtist1 ON album_artist ( idAlbum )");

    // Ensure all songs have at least one artist, set those without to [Missing]
    strSQL = "SELECT count(idSong) FROM song "
             "WHERE NOT EXISTS(SELECT idSong FROM song_artist "
             "WHERE song_artist.idsong = song.idsong AND song_artist.idRole = 1)";
    int numsongs = strtol(GetSingleValue(strSQL).c_str(), NULL, 10);
    if (numsongs > 0)
    {
      CLog::Log(LOGDEBUG, "%i songs have no artist, setting artist to [Missing]", numsongs);
      // Insert song_artist records for songs that don't have any
      try
      {
        strSQL = PrepareSQL("INSERT INTO song_artist(idArtist, idSong, idRole, strArtist, iOrder) "
          "SELECT %i, idSong, %i, '%s', 0 FROM song "
          "WHERE NOT EXISTS(SELECT idSong FROM song_artist "
          "WHERE song_artist.idsong = song.idsong AND song_artist.idRole = %i)",
          BLANKARTIST_ID, ROLE_ARTIST, BLANKARTIST_NAME.c_str(), ROLE_ARTIST);
        ExecuteQuery(strSQL);
      }
      catch (...)
      {
        CLog::Log(LOGERROR, "Setting missing artist for songs without an artist has failed");
      }
    }

    // Ensure all albums have at least one artist, set those without to [Missing]
    strSQL = "SELECT count(idAlbum) FROM album "
      "WHERE NOT EXISTS(SELECT idAlbum FROM album_artist "
      "WHERE album_artist.idAlbum = album.idAlbum)";
    int numalbums = strtol(GetSingleValue(strSQL).c_str(), NULL, 10);
    if (numalbums > 0)
    {
      CLog::Log(LOGDEBUG, "%i albums have no artist, setting artist to [Missing]", numalbums);
      // Insert album_artist records for albums that don't have any
      try
      {
        strSQL = PrepareSQL("INSERT INTO album_artist(idArtist, idAlbum, strArtist, iOrder) "
          "SELECT %i, idAlbum, '%s', 0 FROM album "
          "WHERE NOT EXISTS(SELECT idAlbum FROM album_artist "
          "WHERE album_artist.idAlbum = album.idAlbum)",
          BLANKARTIST_ID, BLANKARTIST_NAME.c_str());
        ExecuteQuery(strSQL);
      }
      catch (...)
      {
        CLog::Log(LOGERROR, "Setting artist missing for albums without an artist has failed");
      }
    }
    //Remove temp indices, full analytics for database created later
    m_pDS->exec("DROP INDEX idxSongArtist1 ON song_artist");
    m_pDS->exec("DROP INDEX idxAlbumArtist1 ON album_artist");
  }
  if (version < 61)
  {
    // Create versiontagscan table
    m_pDS->exec("CREATE TABLE versiontagscan (idVersion integer, iNeedsScan integer)");
    m_pDS->exec("INSERT INTO versiontagscan (idVersion, iNeedsScan) values(0, 0)");
  }
  if (version < 62)
  {
    CLog::Log(LOGINFO, "create audiobook table");
    m_pDS->exec("CREATE TABLE audiobook (idBook integer primary key, "
        " strBook varchar(256), strAuthor text,"
        " bookmark integer, file text,"
        " dateAdded varchar (20) default NULL)");
  }
  if (version < 63)
  {
    // Add strSortName to Artist table
    m_pDS->exec("ALTER TABLE artist ADD strSortName text\n");

    //Remove idThumb (column unused since v47), rename strArtists and add strArtistSort to album table
    m_pDS->exec("CREATE TABLE album_new (idAlbum integer primary key, "
      " strAlbum varchar(256), strMusicBrainzAlbumID text, "
      " strArtistDisp text, strArtistSort text, strGenres text, "
      " iYear integer, bCompilation integer not null default '0', "
      " strMoods text, strStyles text, strThemes text, "
      " strReview text, strImage text, strLabel text, "
      " strType text, "
      " fRating FLOAT NOT NULL DEFAULT 0, "
      " iUserrating INTEGER NOT NULL DEFAULT 0, "
      " lastScraped varchar(20) default NULL, "
      " strReleaseType text, "
      " iVotes INTEGER NOT NULL DEFAULT 0)");
    m_pDS->exec("INSERT INTO album_new "
      "(idAlbum, "
      " strAlbum, strMusicBrainzAlbumID, "
      " strArtistDisp, strArtistSort, strGenres, "
      " iYear, bCompilation, "
      " strMoods, strStyles, strThemes, "
      " strReview, strImage, strLabel, "
      " strType, "
      " fRating, iUserrating, iVotes, "
      " lastScraped, "
      " strReleaseType)"
      " SELECT "
      " idAlbum, "
      " strAlbum, strMusicBrainzAlbumID, "
      " strArtists, NULL, strGenres, "
      " iYear, bCompilation, "
      " strMoods, strStyles, strThemes, "
      " strReview, strImage, strLabel, "
      " strType, "
      " fRating, iUserrating, iVotes, "
      " lastScraped, "
      " strReleaseType"
      " FROM album");
    m_pDS->exec("DROP TABLE album");
    m_pDS->exec("ALTER TABLE album_new RENAME TO album");

    //Remove dwFileNameCRC, idThumb (columns unused since v47), rename strArtists and add strArtistSort to song table
    m_pDS->exec("CREATE TABLE song_new (idSong INTEGER PRIMARY KEY, "
      " idAlbum INTEGER, idPath INTEGER, "
      " strArtistDisp TEXT, strArtistSort TEXT, strGenres TEXT, strTitle VARCHAR(512), "
      " iTrack INTEGER, iDuration INTEGER, iYear INTEGER, "
      " strFileName TEXT, strMusicBrainzTrackID TEXT, "
      " iTimesPlayed INTEGER, iStartOffset INTEGER, iEndOffset INTEGER, "
      " lastplayed VARCHAR(20) DEFAULT NULL, "
      " rating FLOAT NOT NULL DEFAULT 0, votes INTEGER NOT NULL DEFAULT 0, "
      " userrating INTEGER NOT NULL DEFAULT 0, "
      " comment TEXT, mood TEXT, dateAdded TEXT)");
    m_pDS->exec("INSERT INTO song_new "
      "(idSong, "
      " idAlbum, idPath, "
      " strArtistDisp, strArtistSort, strGenres, strTitle, "
      " iTrack, iDuration, iYear, "
      " strFileName, strMusicBrainzTrackID, "
      " iTimesPlayed, iStartOffset, iEndOffset, "
      " lastplayed,"
      " rating, userrating, votes, "
      " comment, mood, dateAdded)"
      " SELECT "
      " idSong, "
      " idAlbum, idPath, "
      " strArtists, NULL, strGenres, strTitle, "
      " iTrack, iDuration, iYear, "
      " strFileName, strMusicBrainzTrackID, "
      " iTimesPlayed, iStartOffset, iEndOffset, "
      " lastplayed,"
      " rating, userrating, votes, "
      " comment, mood, dateAdded"
      " FROM song");
    m_pDS->exec("DROP TABLE song");
    m_pDS->exec("ALTER TABLE song_new RENAME TO song");
  }
  if (version < 65)
  {
    // Remove cue table
    m_pDS->exec("DROP TABLE cue");
    // Add strReplayGain to song table
    m_pDS->exec("ALTER TABLE song ADD strReplayGain TEXT\n");
  }
  if (version < 66)
  {
    // Add a new columns strReleaseGroupMBID, bScrapedMBID for albums
    m_pDS->exec("ALTER TABLE album ADD bScrapedMBID INTEGER NOT NULL DEFAULT 0\n");
    m_pDS->exec("ALTER TABLE album ADD strReleaseGroupMBID TEXT \n");
    // Add a new column bScrapedMBID for artists
    m_pDS->exec("ALTER TABLE artist ADD bScrapedMBID INTEGER NOT NULL DEFAULT 0\n");
  }
  if (version < 67)
  {
    // Add infosetting table
    m_pDS->exec("CREATE TABLE infosetting (idSetting INTEGER PRIMARY KEY, strScraperPath TEXT, strSettings TEXT)");
    // Add a new column for setting to album and artist tables
    m_pDS->exec("ALTER TABLE artist ADD idInfoSetting INTEGER NOT NULL DEFAULT 0\n");
    m_pDS->exec("ALTER TABLE album ADD idInfoSetting INTEGER NOT NULL DEFAULT 0\n");

    // Attempt to get album and artist specific scraper settings from the content table, extracting ids from path
    m_pDS->exec("CREATE TABLE content_temp(id INTEGER PRIMARY KEY, idItem INTEGER, strContent text, "
      "strScraperPath text, strSettings text)");
    try
    {
      m_pDS->exec("INSERT INTO content_temp(idItem, strContent, strScraperPath, strSettings) "
        "SELECT SUBSTR(strPath, 19, LENGTH(strPath) - 19) + 0 AS idItem, strContent, strScraperPath, strSettings "
        "FROM content WHERE strContent = 'artists' AND strPath LIKE 'musicdb://artists/_%/' ORDER BY idItem"
        );
    }
    catch (...)
    {
      CLog::Log(LOGERROR, "Migrating specific artist scraper settings has failed, settings not transfered");
    }
    try
    {
      m_pDS->exec("INSERT INTO content_temp (idItem, strContent, strScraperPath, strSettings ) "
        "SELECT SUBSTR(strPath, 18, LENGTH(strPath) - 18) + 0 AS idItem, strContent, strScraperPath, strSettings "
        "FROM content WHERE strContent = 'albums' AND strPath LIKE 'musicdb://albums/_%/' ORDER BY idItem"
      );
    }
    catch (...)
    {
      CLog::Log(LOGERROR, "Migrating specific album scraper settings has failed, settings not transfered");
    }
    try
    {
      m_pDS->exec("INSERT INTO infosetting(idSetting, strScraperPath, strSettings) "
        "SELECT id, strScraperPath, strSettings FROM content_temp");
      m_pDS->exec("UPDATE artist SET idInfoSetting = "
        "(SELECT id FROM content_temp WHERE strContent = 'artists' AND idItem = idArtist) "
        "WHERE EXISTS(SELECT 1 FROM content_temp WHERE strContent = 'artists' AND idItem = idArtist) ");
      m_pDS->exec("UPDATE album SET idInfoSetting = "
        "(SELECT id FROM content_temp WHERE strContent = 'albums' AND idItem = idAlbum) "
        "WHERE EXISTS(SELECT 1 FROM content_temp WHERE strContent = 'albums' AND idItem = idAlbum) ");
    }
    catch (...)
    {
      CLog::Log(LOGERROR, "Migrating album and artist scraper settings has failed, settings not transfered");
    }
    m_pDS->exec("DROP TABLE content_temp");

    // Remove content table
    m_pDS->exec("DROP TABLE content");
    // Remove albuminfosong table
    m_pDS->exec("DROP TABLE albuminfosong");
  }
  if (version < 68)
  {
    // Add a new columns strType, strGender, strDisambiguation for artists
    m_pDS->exec("ALTER TABLE artist ADD strType TEXT \n");
    m_pDS->exec("ALTER TABLE artist ADD strGender TEXT \n");
    m_pDS->exec("ALTER TABLE artist ADD strDisambiguation TEXT \n");
  }
  if (version < 69)
  {
    // Remove album_genre table
    m_pDS->exec("DROP TABLE album_genre");
  }
  if (version < 70)
  {
    // Update all songs iStartOffset and iEndOffset to milliseconds instead of frames (* 1000 / 75)
    m_pDS->exec("UPDATE song SET iStartOffset = iStartOffset * 40 / 3, iEndOffset = iEndOffset * 40 / 3 \n");
  }
  if (version < 71)
  {
    // Add lastscanned to versiontagscan table
    m_pDS->exec("ALTER TABLE versiontagscan ADD lastscanned VARCHAR(20)\n");
    CDateTime dateAdded = CDateTime::GetCurrentDateTime();
    m_pDS->exec(PrepareSQL("UPDATE versiontagscan SET lastscanned = '%s'", dateAdded.GetAsDBDateTime().c_str()));
  }
  if (version < 72)
  {
    // Create source table
    m_pDS->exec("CREATE TABLE source (idSource INTEGER PRIMARY KEY, strName TEXT, strMultipath TEXT)");
    // Create source_path table
    m_pDS->exec("CREATE TABLE source_path (idSource INTEGER, idPath INTEGER, strPath varchar(512))");
    // Create album_source table
    m_pDS->exec("CREATE TABLE album_source (idSource INTEGER, idAlbum INTEGER)");
    // Populate source and source_path tables from sources.xml
    // Filling album_source needs to be done after indexes are created or it is
    // very slow. It could be populated during CreateAnalytics but it is checked
    // and filled as part of scanning anyway so simply force full rescan.
    MigrateSources();
  }

  // Set the verion of tag scanning required.
  // Not every schema change requires the tags to be rescanned, set to the highest schema version
  // that needs this. Forced rescanning (of music files that have not changed since they were
  // previously scanned) also accommodates any changes to the way tags are processed
  // e.g. read tags that were not processed by previous versions.
  // The original db version when the tags were scanned, and the minimal db version needed are
  // later used to determine if a forced rescan should be prompted

  // The last schema change needing forced rescanning was 60.
  // Mostly because of the new tags processed by v17 rather than a schema change.
  SetMusicNeedsTagScan(60);

  // After all updates, store the original db version.
  // This indicates the version of tag processing that was used to populate db
  SetMusicTagScanVersion(version);
}

int CMusicDatabase::GetSchemaVersion() const
{
  return 72;
}

int CMusicDatabase::GetMusicNeedsTagScan()
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    std::string sql = "SELECT * FROM versiontagscan";
    if (!m_pDS->query(sql)) return -1;

    if (m_pDS->num_rows() != 1)
    {
      m_pDS->close();
      return -1;
    }

    int idVersion = m_pDS->fv("idVersion").get_asInt();
    int iNeedsScan = m_pDS->fv("iNeedsScan").get_asInt();
    m_pDS->close();
    if (idVersion < iNeedsScan)
      return idVersion;
    else
      return 0;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return -1;
}

void CMusicDatabase::SetMusicNeedsTagScan(int version)
{
  m_pDS->exec(PrepareSQL("UPDATE versiontagscan SET iNeedsScan=%i", version));
}

void CMusicDatabase::SetMusicTagScanVersion(int version /* = 0 */)
{
  if (version == 0)
    m_pDS->exec(PrepareSQL("UPDATE versiontagscan SET idVersion=%i", GetSchemaVersion()));
  else
    m_pDS->exec(PrepareSQL("UPDATE versiontagscan SET idVersion=%i", version));
}

std::string CMusicDatabase::GetLibraryLastUpdated()
{
  return GetSingleValue("SELECT lastscanned FROM versiontagscan LIMIT 1");
}

void CMusicDatabase::SetLibraryLastUpdated()
{
  CDateTime dateUpdated = CDateTime::GetCurrentDateTime();
  m_pDS->exec(PrepareSQL("UPDATE versiontagscan SET lastscanned = '%s'", dateUpdated.GetAsDBDateTime().c_str()));
}


unsigned int CMusicDatabase::GetRandomSongIDs(const Filter &filter, std::vector<std::pair<int,int> > &songIDs)
{
  try
  {
    if (NULL == m_pDB.get()) return 0;
    if (NULL == m_pDS.get()) return 0;

    std::string strSQL = "SELECT idSong FROM songview ";
    if (!CDatabase::BuildSQL(strSQL, filter, strSQL))
      return false;
    strSQL += PrepareSQL(" ORDER BY RANDOM()");

    if (!m_pDS->query(strSQL)) return 0;
    songIDs.clear();
    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      return 0;
    }
    songIDs.reserve(m_pDS->num_rows());
    while (!m_pDS->eof())
    {
      songIDs.push_back(std::make_pair<int,int>(1, m_pDS->fv(song_idSong).get_asInt()));
      m_pDS->next();
    }    // cleanup
    m_pDS->close();
    return songIDs.size();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%s) failed", __FUNCTION__, filter.where.c_str());
  }
  return 0;
}

int CMusicDatabase::GetSongsCount(const Filter &filter)
{
  try
  {
    if (NULL == m_pDB.get()) return 0;
    if (NULL == m_pDS.get()) return 0;

    std::string strSQL = "select count(idSong) as NumSongs from songview ";
    if (!CDatabase::BuildSQL(strSQL, filter, strSQL))
      return false;

    if (!m_pDS->query(strSQL)) return false;
    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      return 0;
    }

    int iNumSongs = m_pDS->fv("NumSongs").get_asInt();
    // cleanup
    m_pDS->close();
    return iNumSongs;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%s) failed", __FUNCTION__, filter.where.c_str());
  }
  return 0;
}

bool CMusicDatabase::GetAlbumPath(int idAlbum, std::string& basePath)
{
  basePath.clear();
  std::vector<std::pair<std::string, int>> paths;
  if (!GetAlbumPaths(idAlbum, paths))
    return false;

  for (const auto& pathpair : paths)
  {
    if (basePath.empty())
      basePath = pathpair.first.c_str();
    else
      URIUtils::GetCommonPath(basePath, pathpair.first.c_str());
  }
  return true;
}

bool CMusicDatabase::GetAlbumPaths(int idAlbum, std::vector<std::pair<std::string, int>>& paths)
{
  paths.clear();
  std::string strSQL;
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS2.get()) return false;

    // Get the unique paths of songs on the album, providing there are no songs from
    // other albums with the same path. This returns
    // a) <album> if is contains all the songs and no others, or
    // b) <album>/cd1, <album>/cd2 etc. for disc sets
    // but does *not* return any path when albums are mixed together. That could be because of
    // deliberate file organisation, or (more likely) because of a tagging error in album name
    // or Musicbrainzalbumid. Thus it avoids finding somme generic music path.
    strSQL = PrepareSQL("SELECT DISTINCT strPath, song.idPath FROM song "
      "JOIN path ON song.idPath = path.idPath "
      "WHERE song.idAlbum = %ld "
      "AND (SELECT COUNT(DISTINCT(idAlbum)) FROM song AS song2 "
      "WHERE idPath = song.idPath) = 1", idAlbum);

    if (!m_pDS2->query(strSQL))
      return false;
    if (m_pDS2->num_rows() == 0)
    {
      // Album does not have a unique path, files are mixed
      m_pDS2->close();
      return false;
    }

    while (!m_pDS2->eof())
    {
      paths.emplace_back(m_pDS2->fv("strPath").get_asString(), m_pDS2->fv("song.idPath").get_asInt());
      m_pDS2->next();
    }
    // Cleanup recordset data
    m_pDS2->close();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CMusicDatabase::%s - failed to execute %s", __FUNCTION__, strSQL.c_str());
  }

  return false;
}

int CMusicDatabase::GetDiscnumberForPathID(int idPath)
{
  std::string strSQL;
  int result = -1;
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS2.get()) return -1;

    strSQL = PrepareSQL("SELECT DISTINCT(song.iTrack >> 16) AS discnum FROM song "
      "WHERE idPath = %i", idPath);

    if (!m_pDS2->query(strSQL))
      return -1;
    if (m_pDS2->num_rows() == 1)
    { // Songs with this path have a unique disc number
      result = m_pDS2->fv("discnum").get_asInt();
    }
    // Cleanup recordset data
    m_pDS2->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CMusicDatabase::%s - failed to execute %s", __FUNCTION__, strSQL.c_str());
  }
  return result;
}

// Get old "artist path" - where artist.nfo and art was located v17 and below.
// It is the path common to all albums by an (album) artist, but ensure it is unique
// to that artist and not shared with other artists. Previously this caused incorrect nfo
// and art to be applied to multiple artists.
bool CMusicDatabase::GetOldArtistPath(int idArtist, std::string &basePath)
{
  basePath.clear();
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS2.get()) return false;

    // find all albums from this artist, and all the paths to the songs from those albums
    std::string strSQL = PrepareSQL("SELECT strPath FROM album_artist "
      "JOIN song ON album_artist.idAlbum = song.idAlbum "
      "JOIN path ON song.idPath = path.idPath "
      "WHERE album_artist.idArtist = %ld "
      "GROUP BY song.idPath",
      idArtist);

    // run query
    if (!m_pDS2->query(strSQL)) return false;
    int iRowsFound = m_pDS2->num_rows();
    if (iRowsFound == 0)
    {
      // Artist is not an album artist, no path to find
      m_pDS2->close();
      return false;
    }
    else if (iRowsFound == 1)
    {
      // Special case for single path - assume that we're in an artist/album/songs filesystem
      URIUtils::GetParentPath(m_pDS2->fv("strPath").get_asString(), basePath);
      m_pDS2->close();
    }
    else
    {
      // find the common path (if any) to these albums
      while (!m_pDS2->eof())
      {
        std::string path = m_pDS2->fv("strPath").get_asString();
        if (basePath.empty())
          basePath = path;
        else
          URIUtils::GetCommonPath(basePath, path);

        m_pDS2->next();
      }
      m_pDS2->close();
    }

    // Check any path found is unique to that album artist, and do *not* return any path
    // that is shared with other album artists. That could be because of collaborations
    // i.e. albums with more than one album artist, or because there are albums by the
    // artist on multiple music sources, or elsewhere in the folder hierarchy.
    // Avoid returning some generic music path.
    if (!basePath.empty())
    {
      strSQL = PrepareSQL("SELECT COUNT(album_artist.idArtist) FROM album_artist "
        "JOIN song ON album_artist.idAlbum = song.idAlbum "
        "JOIN path ON song.idPath = path.idPath "
        "WHERE album_artist.idArtist <> %ld "
        "AND strPath LIKE '%s%%'",
        idArtist, basePath.c_str());
      std::string strValue = GetSingleValue(strSQL, m_pDS2);
      if (!strValue.empty())
      {
        int countartists = static_cast<int>(strtol(strValue.c_str(), NULL, 10));
        if (countartists == 0)
          return true;
      }
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  basePath.clear();
  return false;
}

bool CMusicDatabase::GetArtistPath(const CArtist& artist, std::string &path)
{
   // Get path for artist in the artists folder
  path = CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(CSettings::SETTING_MUSICLIBRARY_ARTISTSFOLDER);
  if (path.empty())
    return false; // No Artists folder not set;
  // Get unique artist folder name
  std::string strFolder;
  if (GetArtistFolderName(artist, strFolder))
  {
      path = URIUtils::AddFileToFolder(path, strFolder);
      return true;
  }
  path.clear();
  return false;
}

bool CMusicDatabase::GetAlbumFolder(const CAlbum& album, const std::string &strAlbumPath, std::string &strFolder)
{
  strFolder.clear();
  // Get a name for the album folder that is unique for the artist to use when
  // exporting albums to separate nfo files in a folder under an artist folder

  // When given an album path (common to all the music files containing *only*
  // that album) check if that folder name is *unique* looking at folders on
  // all levels of the music file paths for the artist
  if (!strAlbumPath.empty())
  {
    // Get last folder from full path
    std::vector<std::string> folders = URIUtils::SplitPath(strAlbumPath);
    if (!folders.empty())
    {
      strFolder = folders.back();
      // The same folder name could be used on different paths for albums by the
      // same first artist. The albums could be totally different or also have
      // the same name (but different mbid). Be over cautious and look for the
      // name any where in the music file paths
      std::string strSQL = PrepareSQL("SELECT DISTINCT album_artist.idAlbum FROM album_artist "
        "JOIN song ON album_artist.idAlbum = song.idAlbum "
        "JOIN path on path.idPath = song.idPath "
        "WHERE album_artist.iOrder = 0 "
        "AND album_artist.idArtist = %ld "
        "AND path.strPath LIKE '%%\\%s\\%%'",
        album.artistCredits[0].GetArtistId(), strFolder.c_str());

      if (!m_pDS2->query(strSQL))
        return false;
      int iRowsFound = m_pDS2->num_rows();
      m_pDS2->close();
      if (iRowsFound == 1)
        return true;
    }
  }
  // Create a valid unique folder name from album title
  // @todo: Does UFT8 matter or need normalizing?
  // @todo: Simplify punctuation removing unicode appostraphes, "..." etc.?
  strFolder = CUtil::MakeLegalFileName(album.strAlbum, LEGAL_WIN32_COMPAT);
  StringUtils::Replace(strFolder, " _ ", "_");

  // Check <first albumartist name>/<albumname> is unique e.g. 2 x Bruckner Symphony No. 3
  // To have duplicate albumartist/album names at least one will have mbid, so append start of mbid to folder.
  // This will not handle names that only differ by reserved chars e.g. "a>album" and "a?name"
  // will be unique in db, but produce same folder name "a_name", but that kind of album and artist naming is very unlikely
  std::string strSQL = PrepareSQL("SELECT COUNT(album_artist.idAlbum) FROM album_artist "
    "JOIN album ON album_artist.idAlbum = album.idAlbum "
    "WHERE album_artist.iOrder = 0 "
    "AND album_artist.idArtist = %ld "
    "AND album.strAlbum LIKE '%s'  ",
    album.artistCredits[0].GetArtistId(), album.strAlbum.c_str());
  std::string strValue = GetSingleValue(strSQL, m_pDS2);
  if (strValue.empty())
    return false;
  int countalbum = static_cast<int>(strtol(strValue.c_str(), NULL, 10));
  if (countalbum > 1 && !album.strMusicBrainzAlbumID.empty())
  { // Only one of the duplicate albums can be without mbid
    strFolder += "_" + album.strMusicBrainzAlbumID.substr(0, 4);
  }
  return !strFolder.empty();
}

bool CMusicDatabase::GetArtistFolderName(const CArtist &artist, std::string &strFolder)
{
  return GetArtistFolderName(artist.strArtist, artist.strMusicBrainzArtistID, strFolder);
}

bool CMusicDatabase::GetArtistFolderName(const std::string &strArtist, const std::string &strMusicBrainzArtistID,
  std::string &strFolder)
{
  // Create a valid unique folder name for artist
  // @todo: Does UFT8 matter or need normalizing?
  // @todo: Simplify punctuation removing unicode appostraphes, "..." etc.?
  strFolder = CUtil::MakeLegalFileName(strArtist, LEGAL_WIN32_COMPAT);
  StringUtils::Replace(strFolder, " _ ", "_");

  // Ensure <artist name> is unique e.g. 2 x John Williams.
  // To have duplicate artist names there must both have mbids, so append start of mbid to folder.
  // This will not handle names that only differ by reserved chars e.g. "a>name" and "a?name"
  // will be unique in db, but produce same folder name "a_name", but that kind of artist naming is very unlikely
  std::string strSQL = PrepareSQL("SELECT COUNT(1) FROM artist WHERE strArtist LIKE '%s'", strArtist.c_str());
  std::string strValue = GetSingleValue(strSQL, m_pDS2);
  if (strValue.empty())
    return false;
  int countartist = static_cast<int>(strtol(strValue.c_str(), NULL, 10));
  if (countartist > 1)
    strFolder += "_" + strMusicBrainzArtistID.substr(0, 4);
  return !strFolder.empty();
}

int CMusicDatabase::AddSource(const std::string& strName, const std::string& strMultipath, const std::vector<std::string>& vecPaths, int id /*= -1*/)
{
  int idSource = -1;
  std::string strSQL;
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    // Check if source name already exists
    idSource = GetSourceByName(strName);
    if (idSource < 0)
    {
      BeginTransaction();
      // Add new source and source paths
      if (id > 0)
        strSQL = PrepareSQL("INSERT INTO source (idSource, strName, strMultipath) VALUES(%i, '%s', '%s')",
          id, strName.c_str(), strMultipath.c_str());
      else
        strSQL = PrepareSQL("INSERT INTO source (idSource, strName, strMultipath) VALUES(NULL, '%s', '%s')",
          strName.c_str(), strMultipath.c_str());
      m_pDS->exec(strSQL);

      idSource = static_cast<int>(m_pDS->lastinsertid());

      int idPath = 1;
      for (const auto& path : vecPaths)
      {
        strSQL = PrepareSQL("INSERT INTO source_path (idSource, idPath, strPath) values(%i,%i,'%s')",
          idSource, idPath, path.c_str());
        m_pDS->exec(strSQL);
        ++idPath;
      }

      // Find albums by song path, building WHERE for multiple source paths
      // (providing source has a path)
      if (vecPaths.size() > 0)
      {
        std::vector<int> albumIds;
        Filter extFilter;
        strSQL = "SELECT DISTINCT idAlbum FROM song ";
        extFilter.AppendJoin("JOIN path ON song.idPath = path.idPath");
        for (const auto& path : vecPaths)
          extFilter.AppendWhere(PrepareSQL("path.strPath LIKE '%s%%%%'", path.c_str()), false);
        if (!BuildSQL(strSQL, extFilter, strSQL))
          return -1;

        if (!m_pDS->query(strSQL))
          return -1;

        while (!m_pDS->eof())
        {
          albumIds.push_back(m_pDS->fv("idAlbum").get_asInt());
          m_pDS->next();
        }
        m_pDS->close();

        // Add album_source for related albums
        for (auto idAlbum : albumIds)
        {
          strSQL = PrepareSQL("INSERT INTO album_source (idSource, idAlbum) VALUES('%i', '%i')",
            idSource, idAlbum);
          m_pDS->exec(strSQL);
        }
      }
      CommitTransaction();
    }
    return idSource;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed with query (%s)", __FUNCTION__, strSQL.c_str());
    RollbackTransaction();
  }

  return -1;
}

int CMusicDatabase::UpdateSource(const std::string& strOldName, const std::string& strName, const std::string& strMultipath, const std::vector<std::string>& vecPaths)
{
  int idSource = -1;
  std::string strSourceMultipath;
  std::string strSQL;
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    // Get details of named old source 
    if (!strOldName.empty())
    {
      strSQL = PrepareSQL("SELECT idSource, strMultipath FROM source WHERE strName LIKE '%s'", 
        strOldName.c_str());
      if (!m_pDS->query(strSQL))
        return -1;
      if (m_pDS->num_rows() > 0)
      {
        idSource = m_pDS->fv("idSource").get_asInt();
        strSourceMultipath = m_pDS->fv("strMultipath").get_asString();
      }
      m_pDS->close();
    }
    if (idSource < 0)
    {
      // Source not found, add new one
      return AddSource(strName, strMultipath, vecPaths);
    }

    // Nothing changed? (that we hold in db, other source details could be modified)
    bool pathschanged = strMultipath.compare(strSourceMultipath) != 0;
    if (!pathschanged && strOldName.compare(strName) == 0)
      return idSource;

    if (!pathschanged)
    {
      // Name changed? Could be that none of the values held in db changed
      if (strOldName.compare(strName) != 0)
      {
        strSQL = PrepareSQL("UPDATE source SET strName = '%s' WHERE idSource = %i",
          strName.c_str(), idSource);
        m_pDS->exec(strSQL);
      }
      return idSource;
    }
    else
    {
      // Change paths (and name) by deleting and re-adding, but keep same ID
      strSQL = PrepareSQL("DELETE FROM source WHERE idSource = %i", idSource);
      m_pDS->exec(strSQL);
      return AddSource(strName, strMultipath, vecPaths, idSource);
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed with query (%s)", __FUNCTION__, strSQL.c_str());
    RollbackTransaction();
  }

  return -1;
}

bool CMusicDatabase::RemoveSource(const std::string& strName)
{
  // Related album_source and source_path rows removed by trigger
  return ExecuteQuery(PrepareSQL("DELETE FROM source WHERE strName ='%s'", strName.c_str()));
}

int CMusicDatabase::GetSourceFromPath(const std::string& strPath1)
{
  std::string strSQL;
  int idSource = -1;
  try
  {
    std::string strPath(strPath1);
    if (!URIUtils::HasSlashAtEnd(strPath))
      URIUtils::AddSlashAtEnd(strPath);

    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    // Check if path is a source matching on multipath
    strSQL = PrepareSQL("SELECT idSource FROM source WHERE strMultipath = '%s'", strPath.c_str());
    if (!m_pDS->query(strSQL))
      return -1;
    if (m_pDS->num_rows() > 0)
      idSource = m_pDS->fv("idSource").get_asInt();
    m_pDS->close();
    if (idSource > 0)
      return idSource;

    // Check if path is a source path (of many) or a subfolder of a single source
    strSQL = PrepareSQL("SELECT DISTINCT idSource FROM source_path "
      "WHERE SUBSTR('%s', 1, LENGTH(strPath)) = strPath", strPath.c_str());
    if (!m_pDS->query(strSQL))
      return -1;
    if (m_pDS->num_rows() == 1)
      idSource = m_pDS->fv("idSource").get_asInt();
    m_pDS->close();
    return idSource;

  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s path: %s (%s) failed", __FUNCTION__, strSQL.c_str(), strPath1.c_str());
  }

  return -1;
}

bool CMusicDatabase::AddAlbumSource(int idAlbum, int idSource)
{
  std::string strSQL;
  strSQL = PrepareSQL("INSERT INTO album_source (idAlbum, idSource) values(%i, %i)",
    idAlbum, idSource);
  return ExecuteQuery(strSQL);
}

bool CMusicDatabase::AddAlbumSources(int idAlbum, const std::string& strPath)
{
  std::string strSQL;
  std::vector<int> sourceIds;
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    if (!strPath.empty())
    {
      // Find sources related to album using album path       
      strSQL = PrepareSQL("SELECT DISTINCT idSource FROM source_path "
        "WHERE SUBSTR('%s', 1, LENGTH(strPath)) = strPath", strPath.c_str());
      if (!m_pDS->query(strSQL))
        return false;
      while (!m_pDS->eof())
      {
        sourceIds.push_back(m_pDS->fv("idSource").get_asInt());
        m_pDS->next();
      }
      m_pDS->close();
    }
    else
    { 
      // Find sources using song paths, check each source path individually
      if (NULL == m_pDS2.get()) return false;
      strSQL = "SELECT idSource, strPath FROM source_path";
      if (!m_pDS->query(strSQL))
        return false;
      while (!m_pDS->eof())
      {
        std::string sourcepath = m_pDS->fv("strPath").get_asString();
        strSQL = PrepareSQL("SELECT 1 FROM song "
          "JOIN path ON song.idPath = path.idPath "
          "WHERE song.idAlbum = %i AND path.strPath LIKE '%s%%%%'", sourcepath.c_str());
        if (!m_pDS2->query(strSQL))
          return false;
        if (m_pDS2->num_rows() > 0)
          sourceIds.push_back(m_pDS->fv("idSource").get_asInt());
        m_pDS2->close();

        m_pDS->next();
      }
      m_pDS->close();
    }

    //Add album sources
    for (auto idSource : sourceIds)
    {
      AddAlbumSource(idAlbum, idSource);
    }
    
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s path: %s (%s) failed", __FUNCTION__, strSQL.c_str(), strPath.c_str());
  }

  return false;
}

bool CMusicDatabase::DeleteAlbumSources(int idAlbum)
{
  return ExecuteQuery(PrepareSQL("DELETE FROM album_source WHERE idAlbum = %i", idAlbum));
}

bool CMusicDatabase::CheckSources(VECSOURCES& sources)
{
  if (sources.empty())
  {
    // Source table empty too?
    return GetSingleValue("SELECT 1 FROM source LIMIT 1").empty();
  }

  // Check number of entries matches
  size_t total = static_cast<size_t>(strtol(GetSingleValue("SELECT COUNT(1) FROM source").c_str(), NULL, 10));
  if (total != sources.size())
    return false;

  // Check individual sources match
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    std::string strSQL;
    for (const auto& source : sources)
    {
      // Check each source by name
      strSQL = PrepareSQL("SELECT idSource, strMultipath FROM source "
        "WHERE strName LIKE '%s'", source.strName.c_str());
      m_pDS->query(strSQL);
      if (!m_pDS->query(strSQL))
        return false;
      if (m_pDS->num_rows() != 1)
      {
        // Missing source, or name duplication
        m_pDS->close();
        return false;
      }
      else
      {
        // Check details. Encoded URLs of source.strPath matched to strMultipath
        // field, no need to look at individual paths of source_path table
        if (source.strPath.compare(m_pDS->fv("strMultipath").get_asString()) != 0)
        {
          // Paths not match
          m_pDS->close();
          return false;
        }
        m_pDS->close();
      }
    }
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}

bool CMusicDatabase::MigrateSources()
{
  //Fetch music sources from xml
  VECSOURCES sources(*CMediaSourceSettings::GetInstance().GetSources("music"));

  std::string strSQL;
  try
  {   
    // Fill source and source paths tables
    for (const auto& source : sources)
    {
      // AddSource(source.strName, source.strPath, source.vecPaths);
      // Add new source
      strSQL = PrepareSQL("INSERT INTO source (idSource, strName, strMultipath) VALUES(NULL, '%s', '%s')",
        source.strName.c_str(), source.strPath.c_str());
      m_pDS->exec(strSQL);
      int idSource = static_cast<int>(m_pDS->lastinsertid());

      // Add new source paths
      int idPath = 1;
      for (const auto& path : source.vecPaths)
      {
        strSQL = PrepareSQL("INSERT INTO source_path (idSource, idPath, strPath) values(%i,%i,'%s')",
          idSource, idPath, path.c_str());
        m_pDS->exec(strSQL);
        ++idPath;
      }  
    }

    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strSQL.c_str());
  }
  return false;
}

 bool CMusicDatabase::UpdateSources()
{
  //Check library and xml sources match
  VECSOURCES sources(*CMediaSourceSettings::GetInstance().GetSources("music"));
  if (CheckSources(sources))
    return true;
  
  try
  {  
    // Empty sources table (related link tables removed by trigger);
    ExecuteQuery("DELETE FROM source");
  
    // Fill source table, and album sources
    for (const auto& source : sources)
      AddSource(source.strName, source.strPath, source.vecPaths);

    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}

bool CMusicDatabase::GetSources(CFileItemList& items)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    // Get music sources and individual source paths (may not be scanned or have albums etc.)
    std::string strSQL = "SELECT source.idSource, source.strName, source.strMultipath, source_path.strPath "
      "FROM source JOIN source_path ON source.idSource = source_path.idSource "
      "ORDER BY source.idSource, source_path.idPath";

    CLog::Log(LOGDEBUG, "%s query: %s", __FUNCTION__, strSQL.c_str());
    if (!m_pDS->query(strSQL))
      return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return true;
    }

    // Get data from returned rows
    // Item has source ID in MusicInfotag, multipath in path, and indiviual paths in property
    CVariant sourcePaths(CVariant::VariantTypeArray);
    int idSource = -1;
    while (!m_pDS->eof())
    {
      if (idSource != m_pDS->fv("source.idSource").get_asInt())
      { // New source
        if (idSource > 0 && !sourcePaths.empty())
        {
          //Store paths for previous source in item list
          items[items.Size() - 1].get()->SetProperty("paths", sourcePaths);
          sourcePaths.clear();
        }
        idSource = m_pDS->fv("source.idSource").get_asInt();
        CFileItemPtr pItem(new CFileItem(m_pDS->fv("source.strName").get_asString()));
        pItem->GetMusicInfoTag()->SetDatabaseId(idSource, "source");
        // Set tag URL for "file" property in AudioLibary processing
        pItem->GetMusicInfoTag()->SetURL(m_pDS->fv("source.strMultipath").get_asString());
        // Set item path as source URL encoded multipath too
        pItem->SetPath(m_pDS->fv("source.strMultiPath").get_asString());

        pItem->m_bIsFolder = true;
        items.Add(pItem);
      }
      // Get path data
      sourcePaths.push_back(m_pDS->fv("source_path.strPath").get_asString());

      m_pDS->next();
    }
    if (!sourcePaths.empty())
    {
      //Store paths for final source
      items[items.Size() - 1].get()->SetProperty("paths", sourcePaths);
      sourcePaths.clear();
    }

    // cleanup
    m_pDS->close();

    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}

bool CMusicDatabase::GetSourcesByArtist(int idArtist, CFileItem* item)
{
  if (NULL == m_pDB.get()) return false;
  if (NULL == m_pDS.get()) return false;

  try
  {
    std::string strSQL;
    strSQL = PrepareSQL("SELECT DISTINCT album_source.idSource FROM artist "
      "JOIN album_artist ON album_artist.idArtist = artist.idArtist "
      "JOIN album_source ON album_source.idAlbum = album_artist.idAlbum "
      "WHERE artist.idArtist = %i "
      "ORDER BY album_source.idSource", idArtist);
    if (!m_pDS->query(strSQL))
      return false;
    if (m_pDS->num_rows() == 0)
    {
      // Artist does have any source via albums may not be an album artist.
      // Check via songs fetch sources from compilations or where they are guest artist
      m_pDS->close();
      strSQL = PrepareSQL("SELECT DISTINCT album_source.idSource, FROM song_artist "
        "JOIN song ON song_artist.idSong = song.idSong "
        "JOIN album_source ON album_source.idAlbum = song.idAlbum "
        "WHERE song_artist.idArtist = %i AND song_artist.idRole = 1 "
        "ORDER BY album_source.idSource", idArtist);
      if (!m_pDS->query(strSQL))
        return false;
      if (m_pDS->num_rows() == 0)
      {
        //No sources, but query sucessfull
        m_pDS->close();
        return true;
      }
    }

    CVariant artistSources(CVariant::VariantTypeArray);
    while (!m_pDS->eof())
    {      
      artistSources.push_back(m_pDS->fv("idSource").get_asInt());
      m_pDS->next();
    }
    m_pDS->close();

    item->SetProperty("sourceid", artistSources);
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%i) failed", __FUNCTION__, idArtist);
  }
  return false;
}

bool CMusicDatabase::GetSourcesByAlbum(int idAlbum, CFileItem* item)
{
  if (NULL == m_pDB.get()) return false;
  if (NULL == m_pDS.get()) return false;

  try
  {
    std::string strSQL;
    strSQL = PrepareSQL("SELECT idSource FROM album_source "
      "WHERE album_source.idAlbum = %i "
      "ORDER BY idSource", idAlbum);
    if (!m_pDS->query(strSQL))
      return false;
    CVariant albumSources(CVariant::VariantTypeArray);
    if (m_pDS->num_rows() > 0)
    {
      while (!m_pDS->eof())
      {
        albumSources.push_back(m_pDS->fv("idSource").get_asInt());
        m_pDS->next();
      }
      m_pDS->close();
    }
    else 
    {
      //! @todo: handle singles, or don't waste time checking songs
      // Album does have any sources, may be a single??
      // Check via song paths, check each source path individually
      // usually fewer source paths than songs
      m_pDS->close();
     
      if (NULL == m_pDS2.get()) return false;
      strSQL = "SELECT idSource, strPath FROM source_path";
      if (!m_pDS->query(strSQL))
        return false;
      while (!m_pDS->eof())
      {
        std::string sourcepath = m_pDS->fv("strPath").get_asString();
        strSQL = PrepareSQL("SELECT 1 FROM song "
          "JOIN path ON song.idPath = path.idPath "
          "WHERE song.idAlbum = %i AND path.strPath LIKE '%s%%%%'", idAlbum, sourcepath.c_str());
        if (!m_pDS2->query(strSQL))
          return false;
        if (m_pDS2->num_rows() > 0)
          albumSources.push_back(m_pDS->fv("idSource").get_asInt());
        m_pDS2->close();

        m_pDS->next();
      }
      m_pDS->close();
    }

    
    item->SetProperty("sourceid", albumSources);
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%i) failed", __FUNCTION__, idAlbum);
  }
  return false;
}

bool CMusicDatabase::GetSourcesBySong(int idSong, const std::string& strPath1, CFileItem* item)
{
  if (NULL == m_pDB.get()) return false;
  if (NULL == m_pDS.get()) return false;

  try
  {
    std::string strSQL;
    strSQL = PrepareSQL("SELECT idSource FROM song "
      "JOIN album_source ON album_source.idAlbum = song.idAlbum "
      "WHERE song.idSong = %i "
      "ORDER BY idSource", idSong);
    if (!m_pDS->query(strSQL))
      return false;
    if (m_pDS->num_rows() == 0 && !strPath1.empty())
    {
      // Check via song path instead
      m_pDS->close();
      std::string strPath(strPath1);
      if (!URIUtils::HasSlashAtEnd(strPath))
        URIUtils::AddSlashAtEnd(strPath);

      strSQL = PrepareSQL("SELECT DISTINCT idSource FROM source_path "
        "WHERE SUBSTR('%s', 1, LENGTH(strPath)) = strPath", strPath.c_str());
      if (!m_pDS->query(strSQL))
        return false;
    }
    CVariant songSources(CVariant::VariantTypeArray);
    while (!m_pDS->eof())
    {
      songSources.push_back(m_pDS->fv("idSource").get_asInt());
      m_pDS->next();
    }
    m_pDS->close();

    item->SetProperty("sourceid", songSources);
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%i) failed", __FUNCTION__, idSong);
  }
  return false;
}

int CMusicDatabase::GetSourceByName(const std::string& strSource)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    std::string strSQL;
    strSQL = PrepareSQL("SELECT idSource FROM source WHERE strName LIKE '%s'", strSource.c_str());
    // run query
    if (!m_pDS->query(strSQL)) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound != 1)
    {
      m_pDS->close();
      return -1;
    }
    return m_pDS->fv("idSource").get_asInt();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return -1;
}

std::string CMusicDatabase::GetSourceById(int id)
{
  return GetSingleValue("source", "strName", PrepareSQL("idSource = %i", id));
}

int CMusicDatabase::GetArtistByName(const std::string& strArtist)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    std::string strSQL=PrepareSQL("select idArtist from artist where artist.strArtist like '%s'", strArtist.c_str());

    // run query
    if (!m_pDS->query(strSQL)) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound != 1)
    {
      m_pDS->close();
      return -1;
    }
    int lResult = m_pDS->fv("artist.idArtist").get_asInt();
    m_pDS->close();
    return lResult;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return -1;
}

int CMusicDatabase::GetArtistByMatch(const CArtist& artist)
{
  std::string strSQL;
  try
  {
    if (NULL == m_pDB.get() || NULL == m_pDS.get())
      return false;
    // Match on MusicBrainz ID, definitively unique
    if (!artist.strMusicBrainzArtistID.empty())
      strSQL = PrepareSQL("SELECT idArtist FROM artist WHERE strMusicBrainzArtistID = '%s'",
        artist.strMusicBrainzArtistID.c_str());
    else
    // No MusicBrainz ID, artist by name with no mbid
    strSQL = PrepareSQL("SELECT idArtist FROM artist WHERE strArtist LIKE '%s' AND strMusicBrainzArtistID IS NULL",
      artist.strArtist.c_str());
    if (!m_pDS->query(strSQL)) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound != 1)
    {
      m_pDS->close();
      // Match on artist name, relax mbid restriction
      return GetArtistByName(artist.strArtist);
    }
    int lResult = m_pDS->fv("idArtist").get_asInt();
    m_pDS->close();
    return lResult;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CMusicDatabase::%s - failed to execute %", __FUNCTION__, strSQL.c_str());
  }
  return -1;
}

bool CMusicDatabase::GetArtistFromSong(int idSong, CArtist &artist)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    std::string strSQL = PrepareSQL(
      "SELECT artistview.* FROM song_artist "
      "JOIN artistview ON song_artist.idArtist = artistview.idArtist "
      "WHERE song_artist.idSong= %i AND song_artist.idRole = 1 AND song_artist.iOrder = 0",
      idSong);
    if (!m_pDS->query(strSQL)) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound != 1)
    {
      m_pDS->close();
      return false;
    }

    artist = GetArtistFromDataset(m_pDS.get());

    m_pDS->close();
    return true;

  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}

bool CMusicDatabase::IsSongArtist(int idSong, int idArtist)
{
  std::string strSQL = PrepareSQL(
    "SELECT 1 FROM song_artist "
    "WHERE song_artist.idSong= %i AND "
    "song_artist.idArtist = %i AND song_artist.idRole = 1",
    idSong, idArtist);
  return GetSingleValue(strSQL).empty();
}

bool CMusicDatabase::IsSongAlbumArtist(int idSong, int idArtist)
{
  std::string strSQL = PrepareSQL(
    "SELECT 1 FROM song JOIN album_artist ON song.idAlbum = album_artist.idAlbum "
    "WHERE song.idSong = %i AND album_artist.idArtist = %i",
    idSong, idArtist);
  return GetSingleValue(strSQL).empty();
}

int CMusicDatabase::GetAlbumByName(const std::string& strAlbum, const std::string& strArtist)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    std::string strSQL;
    if (strArtist.empty())
      strSQL=PrepareSQL("SELECT idAlbum FROM album WHERE album.strAlbum LIKE '%s'", strAlbum.c_str());
    else
      strSQL=PrepareSQL("SELECT idAlbum FROM album WHERE album.strAlbum LIKE '%s' AND album.strArtistDisp LIKE '%s'", strAlbum.c_str(),strArtist.c_str());
    // run query
    if (!m_pDS->query(strSQL)) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound != 1)
    {
      m_pDS->close();
      return -1;
    }
    return m_pDS->fv("idAlbum").get_asInt();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return -1;
}

int CMusicDatabase::GetAlbumByName(const std::string& strAlbum, const std::vector<std::string>& artist)
{
  return GetAlbumByName(strAlbum, StringUtils::Join(artist, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_musicItemSeparator));
}

int CMusicDatabase::GetAlbumByMatch(const CAlbum &album)
{
  std::string strSQL;
  try
  {
    if (NULL == m_pDB.get() || NULL == m_pDS.get())
      return false;
    // Match on MusicBrainz ID, definitively unique
    if (!album.strMusicBrainzAlbumID.empty())
      strSQL = PrepareSQL("SELECT idAlbum FROM album WHERE strMusicBrainzAlbumID = '%s'", album.strMusicBrainzAlbumID.c_str());
    else
      // No mbid, match on album title and album artist descriptive string, ignore those with mbid
      strSQL = PrepareSQL("SELECT idAlbum FROM album WHERE strArtistDisp LIKE '%s' AND strAlbum LIKE '%s' AND strMusicBrainzAlbumID IS NULL",
        album.GetAlbumArtistString().c_str(),
        album.strAlbum.c_str());
    m_pDS->query(strSQL);
    if (!m_pDS->query(strSQL)) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound != 1)
    {
      m_pDS->close();
      // Match on album title and album artist descriptive string, relax mbid restriction
      return GetAlbumByName(album.strAlbum, album.GetAlbumArtistString());
    }
    int lResult = m_pDS->fv("idAlbum").get_asInt();
    m_pDS->close();
    return lResult;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CMusicDatabase::%s - failed to execute %", __FUNCTION__, strSQL.c_str());
  }
  return -1;
}

std::string CMusicDatabase::GetGenreById(int id)
{
  return GetSingleValue("genre", "strGenre", PrepareSQL("idGenre=%i", id));
}

std::string CMusicDatabase::GetArtistById(int id)
{
  return GetSingleValue("artist", "strArtist", PrepareSQL("idArtist=%i", id));
}

std::string CMusicDatabase::GetRoleById(int id)
{
  return GetSingleValue("role", "strRole", PrepareSQL("idRole=%i", id));
}

bool CMusicDatabase::UpdateArtistSortNames(int idArtist /*=-1*/)
{
  // Propagate artist sort names into concatenated artist sort name string for songs and albums
  std::string strSQL;
  // MySQL syntax for GROUP_CONCAT with order is different from that in SQLite (not handled by PrepareSQL)
  bool bisMySQL = StringUtils::EqualsNoCase(CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_databaseMusic.type, "mysql");

  BeginMultipleExecute();
  if (bisMySQL)
    strSQL = "UPDATE album SET strArtistSort =  "
      "(SELECT GROUP_CONCAT("
      "CASE WHEN artist.strSortName IS NULL THEN artist.strArtist "
      "ELSE artist.strSortName END "
      "ORDER BY album_artist.idAlbum, album_artist.iOrder "
      "SEPARATOR '; ') as val "
      "FROM album_artist JOIN artist on artist.idArtist = album_artist.idArtist "
      "WHERE album_artist.idAlbum = album.idAlbum GROUP BY idAlbum) "
      "WHERE album.strArtistSort = '' OR album.strArtistSort is NULL";
  else
    strSQL = "UPDATE album SET strArtistSort = "
      "(SELECT GROUP_CONCAT(val, '; ') "
      "FROM(SELECT album_artist.idAlbum, "
      "CASE WHEN artist.strSortName IS NULL THEN artist.strArtist "
      "ELSE artist.strSortName END as val "
      "FROM album_artist JOIN artist on artist.idArtist = album_artist.idArtist "
      "WHERE album_artist.idAlbum = album.idAlbum "
      "ORDER BY album_artist.idAlbum, album_artist.iOrder) GROUP BY idAlbum) "
      "WHERE album.strArtistSort = '' OR album.strArtistSort is NULL";
  if (idArtist > 0)
    strSQL += PrepareSQL(" AND EXISTS (SELECT 1 FROM album_artist WHERE album_artist.idArtist = %ld "
      "AND album_artist.idAlbum = album.idAlbum)", idArtist);
  ExecuteQuery(strSQL);
  CLog::Log(LOGDEBUG, "%s query: %s", __FUNCTION__, strSQL.c_str());


  if (bisMySQL)
    strSQL = "UPDATE song SET strArtistSort = "
      "(SELECT GROUP_CONCAT("
      "CASE WHEN artist.strSortName IS NULL THEN artist.strArtist "
      "ELSE artist.strSortName END "
      "ORDER BY song_artist.idSong, song_artist.iOrder "
      "SEPARATOR '; ') as val "
      "FROM song_artist JOIN artist on artist.idArtist = song_artist.idArtist "
      "WHERE song_artist.idSong = song.idSong AND song_artist.idRole = 1 GROUP BY idSong) "
      "WHERE song.strArtistSort = ''  OR song.strArtistSort is NULL";
  else
    strSQL = "UPDATE song SET strArtistSort = "
      "(SELECT GROUP_CONCAT(val, '; ') "
      "FROM(SELECT song_artist.idSong, "
      "CASE WHEN artist.strSortName IS NULL THEN artist.strArtist "
      "ELSE artist.strSortName END as val "
      "FROM song_artist JOIN artist on artist.idArtist = song_artist.idArtist "
      "WHERE song_artist.idSong = song.idSong AND song_artist.idRole = 1 "
      "ORDER BY song_artist.idSong, song_artist.iOrder) GROUP BY idSong) "
      "WHERE song.strArtistSort = ''  OR song.strArtistSort is NULL ";
  if (idArtist > 0)
    strSQL += PrepareSQL(" AND EXISTS (SELECT 1 FROM song_artist WHERE song_artist.idArtist = %ld "
      "AND song_artist.idSong = song.idSong AND song_artist.idRole = 1)", idArtist);
  ExecuteQuery(strSQL);
  CLog::Log(LOGDEBUG, "%s query: %s", __FUNCTION__, strSQL.c_str());

  //Restore nulls where strArtistSort = strArtistDisp
  strSQL = "UPDATE album SET strArtistSort = Null WHERE strArtistSort = strArtistDisp";
  if (idArtist > 0)
    strSQL += PrepareSQL(" AND EXISTS (SELECT 1 FROM album_artist WHERE album_artist.idArtist = %ld "
      "AND album_artist.idAlbum = album.idAlbum)", idArtist);
  ExecuteQuery(strSQL);
  CLog::Log(LOGDEBUG, "%s query: %s", __FUNCTION__, strSQL.c_str());
  strSQL = "UPDATE song SET strArtistSort = Null WHERE strArtistSort = strArtistDisp";
  if (idArtist > 0)
    strSQL += PrepareSQL(" AND EXISTS (SELECT 1 FROM song_artist WHERE song_artist.idArtist = %ld "
        "AND song_artist.idSong = song.idSong AND song_artist.idRole = 1)", idArtist);
  ExecuteQuery(strSQL);
  CLog::Log(LOGDEBUG, "%s query: %s", __FUNCTION__, strSQL.c_str());

  if (CommitMultipleExecute())
    return true;
  else
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  return false;
}

std::string CMusicDatabase::GetAlbumById(int id)
{
  return GetSingleValue("album", "strAlbum", PrepareSQL("idAlbum=%i", id));
}

int CMusicDatabase::GetGenreByName(const std::string& strGenre)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    std::string strSQL;
    strSQL=PrepareSQL("select idGenre from genre where genre.strGenre like '%s'", strGenre.c_str());
    // run query
    if (!m_pDS->query(strSQL)) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound != 1)
    {
      m_pDS->close();
      return -1;
    }
    return m_pDS->fv("genre.idGenre").get_asInt();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return -1;
}

bool CMusicDatabase::GetGenresJSON(CFileItemList& items, bool bSources)
{
  std::string strSQL;
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    strSQL = "SELECT %s FROM genre ";
    Filter extFilter;
    extFilter.AppendField("genre.idGenre");
    extFilter.AppendField("genre.strGenre");
    if (bSources)
    {
      strSQL = "SELECT DISTINCT %s FROM genre ";
      extFilter.AppendField("album_source.idSource");
      extFilter.AppendJoin("JOIN song_genre ON song_genre.idGenre = genre.idGenre");
      extFilter.AppendJoin("JOIN song ON song.idSong = song_genre.idSong");
      extFilter.AppendJoin("JOIN album ON album.idAlbum = song.idAlbum");
      extFilter.AppendJoin("LEFT JOIN album_source on album_source.idAlbum = album.idAlbum");
      extFilter.AppendOrder("genre.strGenre");
      extFilter.AppendOrder("album_source.idSource");
    }   
    extFilter.AppendWhere("genre.strGenre != ''");

    std::string strSQLExtra;
    if (!BuildSQL(strSQLExtra, extFilter, strSQLExtra))
      return false;
   
    strSQL = PrepareSQL(strSQL.c_str(), extFilter.fields.c_str()) + strSQLExtra;

    // run query
    CLog::Log(LOGDEBUG, "%s query: %s", __FUNCTION__, strSQL.c_str());

    if (!m_pDS->query(strSQL))
      return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return true;
    }

    if (!bSources)
      items.Reserve(iRowsFound);

    // Get data from returned rows
    // Item has genre name and ID in MusicInfotag, VFS path, and sources in property
    CVariant genreSources(CVariant::VariantTypeArray);
    int idGenre = -1;
    while (!m_pDS->eof())
    {
      if (idGenre != m_pDS->fv("genre.idGenre").get_asInt())
      { // New genre
        if (idGenre > 0 && bSources)
        {
          //Store sources for previous genre in item list
          items[items.Size() - 1].get()->SetProperty("sourceid", genreSources);
          genreSources.clear();
        }
        idGenre = m_pDS->fv("genre.idGenre").get_asInt();       
        std::string strGenre = m_pDS->fv("genre.strGenre").get_asString();
        CFileItemPtr pItem(new CFileItem(strGenre));
        pItem->GetMusicInfoTag()->SetTitle(strGenre);
        pItem->GetMusicInfoTag()->SetGenre(strGenre);
        pItem->GetMusicInfoTag()->SetDatabaseId(idGenre, "genre");
        pItem->SetPath(StringUtils::Format("musicdb://genres/%i/", idGenre));
        pItem->m_bIsFolder = true;
        items.Add(pItem);
      }
      // Get source data
      if (bSources)
      {
        int sourceid = m_pDS->fv("album_source.idSource").get_asInt();
        if (sourceid > 0)
          genreSources.push_back(sourceid);
      }
      m_pDS->next();
    }
    if (bSources)
    {
      //Store sources for final genre
      items[items.Size() - 1].get()->SetProperty("sourceid", genreSources);
    }

    // cleanup
    m_pDS->close();

    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strSQL.c_str());
  }
  return false;
}

bool CMusicDatabase::GetCompilationAlbums(const std::string& strBaseDir, CFileItemList& items)
{
  CMusicDbUrl musicUrl;
  if (!musicUrl.FromString(strBaseDir))
    return false;

  musicUrl.AddOption("compilation", true);

  Filter filter;
  return GetAlbumsByWhere(musicUrl.ToString(), filter, items);
}

bool CMusicDatabase::GetCompilationSongs(const std::string& strBaseDir, CFileItemList& items)
{
  CMusicDbUrl musicUrl;
  if (!musicUrl.FromString(strBaseDir))
    return false;

  musicUrl.AddOption("compilation", true);

  Filter filter;
  return GetSongsFullByWhere(musicUrl.ToString(), filter, items, SortDescription(), true);
}

int CMusicDatabase::GetCompilationAlbumsCount()
{
  return strtol(GetSingleValue("album", "count(idAlbum)", "bCompilation = 1").c_str(), NULL, 10);
}

int CMusicDatabase::GetSinglesCount()
{
  CDatabase::Filter filter(PrepareSQL("songview.idAlbum IN (SELECT idAlbum FROM album WHERE strReleaseType = '%s')", CAlbum::ReleaseTypeToString(CAlbum::Single).c_str()));
  return GetSongsCount(filter);
}

int CMusicDatabase::GetArtistCountForRole(int role)
{
  std::string strSQL = PrepareSQL("SELECT COUNT(DISTINCT idartist) FROM song_artist WHERE song_artist.idRole = %i", role);
  return strtol(GetSingleValue(strSQL).c_str(), NULL, 10);
}

int CMusicDatabase::GetArtistCountForRole(const std::string& strRole)
{
  std::string strSQL = PrepareSQL("SELECT COUNT(DISTINCT idartist) FROM song_artist JOIN role ON song_artist.idRole = role.idRole WHERE role.strRole LIKE '%s'", strRole.c_str());
  return strtol(GetSingleValue(strSQL).c_str(), NULL, 10);
}

bool CMusicDatabase::SetPathHash(const std::string &path, const std::string &hash)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    if (hash.empty())
    { // this is an empty folder - we need only add it to the path table
      // if the path actually exists
      if (!CDirectory::Exists(path))
        return false;
    }
    int idPath = AddPath(path);
    if (idPath < 0) return false;

    std::string strSQL=PrepareSQL("update path set strHash='%s' where idPath=%ld", hash.c_str(), idPath);
    m_pDS->exec(strSQL);

    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s, %s) failed", __FUNCTION__, path.c_str(), hash.c_str());
  }

  return false;
}

bool CMusicDatabase::GetPathHash(const std::string &path, std::string &hash)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    std::string strSQL=PrepareSQL("select strHash from path where strPath='%s'", path.c_str());
    m_pDS->query(strSQL);
    if (m_pDS->num_rows() == 0)
      return false;
    hash = m_pDS->fv("strHash").get_asString();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, path.c_str());
  }

  return false;
}

bool CMusicDatabase::RemoveSongsFromPath(const std::string &path1, MAPSONGS& songs, bool exact)
{
  // We need to remove all songs from this path, as their tags are going
  // to be re-read.  We need to remove all songs from the song table + all links to them
  // from the song link tables (as otherwise if a song is added back
  // to the table with the same idSong, these tables can't be cleaned up properly later)

  //! @todo SQLite probably doesn't allow this, but can we rely on that??

  // We don't need to remove orphaned albums at this point as in AddAlbum() we check
  // first whether the album has already been read during this scan, and if it hasn't
  // we check whether it's in the table and update accordingly at that point, removing the entries from
  // the album link tables.  The only failure point for this is albums
  // that span multiple folders, where just the files in one folder have been changed.  In this case
  // any linked fields that are only in the files that haven't changed will be removed.  Clearly
  // the primary albumartist still matches (as that's what we looked up based on) so is this really
  // an issue?  I don't think it is, as those artists will still have links to the album via the songs
  // which is generally what we rely on, so the only failure point is albumartist lookup.  In this
  // case, it will return only things in the album_artist table from the newly updated songs (and
  // only if they have additional artists).  I think the effect of this is minimal at best, as ALL
  // songs in the album should have the same albumartist!

  // we also remove the path at this point as it will be added later on if the
  // path still exists.
  // After scanning we then remove the orphaned artists, genres and thumbs.

  // Note: when used to remove all songs from a path and its subpath (exact=false), this
  // does miss archived songs.
  std::string path(path1);
  SetLibraryLastUpdated();
  try
  {
    if (!URIUtils::HasSlashAtEnd(path))
      URIUtils::AddSlashAtEnd(path);

    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    std::string where;
    if (exact)
      where = PrepareSQL(" where strPath='%s'", path.c_str());
    else
      where = PrepareSQL(" where SUBSTR(strPath,1,%i)='%s'", StringUtils::utf8_strlen(path.c_str()), path.c_str());
    std::string sql = "select * from songview" + where;
    if (!m_pDS->query(sql)) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound > 0)
    {
      std::vector<std::string> songIds;
      while (!m_pDS->eof())
      {
        CSong song = GetSongFromDataset();
        song.strThumb = GetArtForItem(song.idSong, MediaTypeSong, "thumb");
        songs.insert(std::make_pair(song.strFileName, song));
        songIds.push_back(PrepareSQL("%i", song.idSong));
        m_pDS->next();
      }
      m_pDS->close();

      //! @todo move this below the m_pDS->exec block, once UPnP doesn't rely on this anymore
      for (const auto &song : songs)
        AnnounceRemove(MediaTypeSong, song.second.idSong);

      // and delete all songs, and anything linked to them
      sql = "delete from song where idSong in (" + StringUtils::Join(songIds, ",") + ")";
      m_pDS->exec(sql);
    }
    // and remove the path as well (it'll be re-added later on with the new hash if it's non-empty)
    sql = "delete from path" + where;
    m_pDS->exec(sql);
    return iRowsFound > 0;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, path.c_str());
  }
  return false;
}

bool CMusicDatabase::GetPaths(std::set<std::string> &paths)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    paths.clear();

    // find all paths
    if (!m_pDS->query("select strPath from path")) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return true;
    }
    while (!m_pDS->eof())
    {
      paths.insert(m_pDS->fv("strPath").get_asString());
      m_pDS->next();
    }
    m_pDS->close();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}

bool CMusicDatabase::SetSongUserrating(const std::string &filePath, int userrating)
{
  try
  {
    if (filePath.empty()) return false;
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    int songID = GetSongIDFromPath(filePath);
    if (-1 == songID) return false;

    return SetSongUserrating(songID, userrating);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s,%i) failed", __FUNCTION__, filePath.c_str(), userrating);
  }
  return false;
}

bool CMusicDatabase::SetSongUserrating(int idSong, int userrating)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    std::string sql = PrepareSQL("UPDATE song SET userrating='%i' WHERE idSong = %i", userrating, idSong);
    m_pDS->exec(sql);
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%i,%i) failed", __FUNCTION__, idSong, userrating);
  }
  return false;
}

bool CMusicDatabase::SetAlbumUserrating(const int idAlbum, int userrating)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    if (-1 == idAlbum) return false;

    std::string sql = PrepareSQL("UPDATE album SET iUserrating='%i' WHERE idAlbum = %i", userrating, idAlbum);
    m_pDS->exec(sql);
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%i,%i) failed", __FUNCTION__, idAlbum, userrating);
  }
  return false;
}

bool CMusicDatabase::SetSongVotes(const std::string &filePath, int votes)
{
  try
  {
    if (filePath.empty()) return false;
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    int songID = GetSongIDFromPath(filePath);
    if (-1 == songID) return false;

    std::string sql = PrepareSQL("UPDATE song SET votes='%i' WHERE idSong = %i", votes, songID);
    m_pDS->exec(sql);
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s,%i) failed", __FUNCTION__, filePath.c_str(), votes);
  }
  return false;
}

int CMusicDatabase::GetSongIDFromPath(const std::string &filePath)
{
  // grab the where string to identify the song id
  CURL url(filePath);
  if (url.IsProtocol("musicdb"))
  {
    std::string strFile=URIUtils::GetFileName(filePath);
    URIUtils::RemoveExtension(strFile);
    return atol(strFile.c_str());
  }
  // hit the db
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    std::string strPath, strFileName;
    SplitPath(filePath, strPath, strFileName);
    URIUtils::AddSlashAtEnd(strPath);

    std::string sql = PrepareSQL("select idSong from song join path on song.idPath = path.idPath where song.strFileName='%s' and path.strPath='%s'", strFileName.c_str(), strPath.c_str());
    if (!m_pDS->query(sql)) return -1;

    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      return -1;
    }

    int songID = m_pDS->fv("idSong").get_asInt();
    m_pDS->close();
    return songID;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, filePath.c_str());
  }
  return -1;
}

bool CMusicDatabase::CommitTransaction()
{
  if (CDatabase::CommitTransaction())
  { // number of items in the db has likely changed, so reset the infomanager cache
    CGUIComponent* gui = CServiceBroker::GetGUI();
    if (gui)
    {
      gui->GetInfoManager().GetInfoProviders().GetLibraryInfoProvider().SetLibraryBool(LIBRARY_HAS_MUSIC, GetSongsCount() > 0);
      return true;
    }
  }
  return false;
}

bool CMusicDatabase::SetScraperAll(const std::string & strBaseDir, const ADDON::ScraperPtr scraper)
{
  if (NULL == m_pDB.get()) return false;
  if (NULL == m_pDS.get()) return false;
  std::string strSQL;
  int idSetting = -1;
  try
  {
    CONTENT_TYPE content = CONTENT_NONE;

    // Build where clause from virtual path
    Filter extFilter;
    CMusicDbUrl musicUrl;
    SortDescription sorting;
    if (!musicUrl.FromString(strBaseDir) || !GetFilter(musicUrl, extFilter, sorting))
      return false;

    std::string itemType = musicUrl.GetType();
    if (StringUtils::EqualsNoCase(itemType, "artists"))
    {
      content = CONTENT_ARTISTS;
    }
    else if (StringUtils::EqualsNoCase(itemType, "albums"))
    {
      content = CONTENT_ALBUMS;
    }
    else
      return false;  //Only artists and albums have info settings

    std::string strSQLWhere;
    if (!BuildSQL(strSQLWhere, extFilter, strSQLWhere))
      return false;

    // Replace view names with table names
    StringUtils::Replace(strSQLWhere, "artistview", "artist");
    StringUtils::Replace(strSQLWhere, "albumview", "album");

    BeginTransaction();
    // Clear current scraper settings (0 => default scraper used)
    if (content == CONTENT_ARTISTS)
      strSQL = "UPDATE artist SET idInfoSetting = %i ";
    else
      strSQL = "UPDATE album SET idInfoSetting = %i ";
    strSQL = PrepareSQL(strSQL, 0) + strSQLWhere;
    m_pDS->exec(strSQL);

    //Remove orphaned settings
    CleanupInfoSettings();

    if (scraper)
    {
      // Add new info setting
      strSQL = "INSERT INTO infosetting (strScraperPath, strSettings) values ('%s','%s')";
      strSQL = PrepareSQL(strSQL, scraper->ID().c_str(), scraper->GetPathSettings().c_str());
      m_pDS->exec(strSQL);
      idSetting = static_cast<int>(m_pDS->lastinsertid());

      if (content == CONTENT_ARTISTS)
        strSQL = "UPDATE artist SET idInfoSetting = %i ";
      else
        strSQL = "UPDATE album SET idInfoSetting = %i ";
      strSQL = PrepareSQL(strSQL, idSetting) + strSQLWhere;
      m_pDS->exec(strSQL);
    }
    CommitTransaction();
    return true;
  }
  catch (...)
  {
    RollbackTransaction();
    CLog::Log(LOGERROR, "%s - (%s, %s) failed", __FUNCTION__, strBaseDir.c_str(), strSQL.c_str());
  }
  return false;
}

bool CMusicDatabase::SetScraper(int id, const CONTENT_TYPE &content, const ADDON::ScraperPtr scraper)
{
  if (NULL == m_pDB.get()) return false;
  if (NULL == m_pDS.get()) return false;
  std::string strSQL;
  int idSetting = -1;
  try
  {
    BeginTransaction();
    // Fetch current info settings for item, 0 => default is used
    if (content == CONTENT_ARTISTS)
      strSQL = "SELECT idInfoSetting FROM artist WHERE idArtist = %i";
    else
      strSQL = "SELECT idInfoSetting FROM album WHERE idAlbum = %i";
    strSQL = PrepareSQL(strSQL, id);
    m_pDS->query(strSQL);
    if (m_pDS->num_rows() > 0)
      idSetting = m_pDS->fv("idInfoSetting").get_asInt();
    m_pDS->close();

    if (idSetting < 1)
    { // Add new info setting
      strSQL = "INSERT INTO infosetting (strScraperPath, strSettings) values ('%s','%s')";
      strSQL = PrepareSQL(strSQL, scraper->ID().c_str(), scraper->GetPathSettings().c_str());
      m_pDS->exec(strSQL);
      idSetting = static_cast<int>(m_pDS->lastinsertid());

      if (content == CONTENT_ARTISTS)
        strSQL = "UPDATE artist SET idInfoSetting = %i WHERE idArtist = %i";
      else
        strSQL = "UPDATE album SET idInfoSetting = %i WHERE idAlbum = %i";
      strSQL = PrepareSQL(strSQL, idSetting, id);
      m_pDS->exec(strSQL);
    }
    else
    {  // Update info setting
      strSQL = "UPDATE infosetting SET strScraperPath = '%s', strSettings = '%s' WHERE idSetting = %i";
      strSQL = PrepareSQL(strSQL, scraper->ID().c_str(), scraper->GetPathSettings().c_str(), idSetting);
      m_pDS->exec(strSQL);
    }
    CommitTransaction();
    return true;
  }
  catch (...)
  {
    RollbackTransaction();
    CLog::Log(LOGERROR, "%s - (%i, %s) failed", __FUNCTION__, id, strSQL.c_str());
  }
  return false;
}

bool CMusicDatabase::GetScraper(int id, const CONTENT_TYPE &content, ADDON::ScraperPtr& scraper)
{
  std::string scraperUUID;
  std::string strSettings;
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    std::string strSQL;
    strSQL = "SELECT strScraperPath, strSettings FROM infosetting JOIN ";
    if (content == CONTENT_ARTISTS)
      strSQL = strSQL + "artist ON artist.idInfoSetting = infosetting.idSetting WHERE artist.idArtist = %i";
    else
      strSQL = strSQL + "album ON album.idInfoSetting = infosetting.idSetting WHERE album.idAlbum = %i";
    strSQL = PrepareSQL(strSQL, id);
    m_pDS->query(strSQL);
    if (!m_pDS->eof())
    { // try and ascertain scraper
      scraperUUID = m_pDS->fv("strScraperPath").get_asString();
      strSettings = m_pDS->fv("strSettings").get_asString();

      // Use pre configured or default scraper
      ADDON::AddonPtr addon;
      if (!scraperUUID.empty() && CServiceBroker::GetAddonMgr().GetAddon(scraperUUID, addon) && addon)
      {
        scraper = std::dynamic_pointer_cast<ADDON::CScraper>(addon);
        if (scraper)
          // Set settings
          scraper->SetPathSettings(content, strSettings);
      }
    }
    m_pDS->close();

    if (!scraper)
    { // use default music scraper instead
      ADDON::AddonPtr addon;
      if(ADDON::CAddonSystemSettings::GetInstance().GetActive(ADDON::ScraperTypeFromContent(content), addon))
      {
        scraper = std::dynamic_pointer_cast<ADDON::CScraper>(addon);
        return scraper != NULL;
      }
      else
        return false;
    }

    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s -(%i, %s %s) failed", __FUNCTION__, id, scraperUUID.c_str(), strSettings.c_str());
  }
  return false;
}

bool CMusicDatabase::ScraperInUse(const std::string &scraperID) const
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    std::string sql = PrepareSQL("SELECT COUNT(1) FROM infosetting WHERE strScraperPath='%s'",scraperID.c_str());
    if (!m_pDS->query(sql) || m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      return false;
    }
    bool found = m_pDS->fv(0).get_asInt() > 0;
    m_pDS->close();
    return found;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%s) failed", __FUNCTION__, scraperID.c_str());
  }
  return false;
}

bool CMusicDatabase::GetItems(const std::string &strBaseDir, CFileItemList &items, const Filter &filter /* = Filter() */, const SortDescription &sortDescription /* = SortDescription() */)
{
  CMusicDbUrl musicUrl;
  if (!musicUrl.FromString(strBaseDir))
    return false;

  return GetItems(strBaseDir, musicUrl.GetType(), items, filter, sortDescription);
}

bool CMusicDatabase::GetItems(const std::string &strBaseDir, const std::string &itemType, CFileItemList &items, const Filter &filter /* = Filter() */, const SortDescription &sortDescription /* = SortDescription() */)
{
  if (StringUtils::EqualsNoCase(itemType, "genres"))
    return GetGenresNav(strBaseDir, items, filter);
  else if (StringUtils::EqualsNoCase(itemType, "sources"))
    return GetSourcesNav(strBaseDir, items, filter);
  else if (StringUtils::EqualsNoCase(itemType, "years"))
    return GetYearsNav(strBaseDir, items, filter);
  else if (StringUtils::EqualsNoCase(itemType, "roles"))
    return GetRolesNav(strBaseDir, items, filter);
  else if (StringUtils::EqualsNoCase(itemType, "artists"))
    return GetArtistsNav(strBaseDir, items, !CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_MUSICLIBRARY_SHOWCOMPILATIONARTISTS), -1, -1, -1, filter, sortDescription);
  else if (StringUtils::EqualsNoCase(itemType, "albums"))
    return GetAlbumsByWhere(strBaseDir, filter, items, sortDescription);
  else if (StringUtils::EqualsNoCase(itemType, "songs"))
    return GetSongsFullByWhere(strBaseDir, filter, items, sortDescription, true);

  return false;
}

std::string CMusicDatabase::GetItemById(const std::string &itemType, int id)
{
  if (StringUtils::EqualsNoCase(itemType, "genres"))
    return GetGenreById(id);
  else if (StringUtils::EqualsNoCase(itemType, "sources"))
    return GetSourceById(id);
  else if (StringUtils::EqualsNoCase(itemType, "years"))
    return StringUtils::Format("%d", id);
  else if (StringUtils::EqualsNoCase(itemType, "artists"))
    return GetArtistById(id);
  else if (StringUtils::EqualsNoCase(itemType, "albums"))
    return GetAlbumById(id);
  else if (StringUtils::EqualsNoCase(itemType, "roles"))
    return GetRoleById(id);

  return "";
}

void CMusicDatabase::ExportToXML(const CLibExportSettings& settings,  CGUIDialogProgress* progressDialog /*= nullptr*/)
{
  if (!settings.IsItemExported(ELIBEXPORT_ALBUMARTISTS) &&
      !settings.IsItemExported(ELIBEXPORT_SONGARTISTS) &&
      !settings.IsItemExported(ELIBEXPORT_OTHERARTISTS) &&
      !settings.IsItemExported(ELIBEXPORT_ALBUMS) && 
      !settings.IsItemExported(ELIBEXPORT_SONGS))
    return;

  // Exporting albums either art or NFO (or both) selected
  if ((settings.IsToLibFolders() || settings.IsSeparateFiles()) &&
       settings.m_skipnfo && !settings.m_artwork &&
       settings.IsItemExported(ELIBEXPORT_ALBUMS))
    return;

  std::string strFolder;
  if (settings.IsSingleFile() || settings.IsSeparateFiles())
  {
    // Exporting to single file or separate files in a specified location
    if (settings.m_strPath.empty())
      return;

    strFolder = settings.m_strPath;
    if (!URIUtils::HasSlashAtEnd(strFolder))
      URIUtils::AddSlashAtEnd(strFolder);
    strFolder = URIUtils::GetDirectory(strFolder);
    if (strFolder.empty())
      return;
  }
  else if (settings.IsArtistFoldersOnly() || (settings.IsToLibFolders() && settings.IsArtists()))
  {
    // Exporting artist folders only, or artist NFO or art to library folders
    // need Artist Information Folder defined. 
    // (Album NFO and art goes to music folders)
    strFolder = CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(CSettings::SETTING_MUSICLIBRARY_ARTISTSFOLDER);
    if (strFolder.empty())
      return;
  }

  //
  bool artistfoldersonly; 
  artistfoldersonly = settings.IsArtistFoldersOnly() ||
                      ((settings.IsToLibFolders() || settings.IsSeparateFiles()) && 
                        settings.m_skipnfo && !settings.m_artwork);

  int iFailCount = 0;
  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;
    if (NULL == m_pDS2.get()) return;

    // Create our xml document
    CXBMCTinyXML xmlDoc;
    TiXmlDeclaration decl("1.0", "UTF-8", "yes");
    xmlDoc.InsertEndChild(decl);
    TiXmlNode *pMain = NULL;
    if ((settings.IsToLibFolders() || settings.IsSeparateFiles()) && !artistfoldersonly)
      pMain = &xmlDoc;
    else if (settings.IsSingleFile())
    {
      TiXmlElement xmlMainElement("musicdb");
      pMain = xmlDoc.InsertEndChild(xmlMainElement);
    }

    if (settings.IsItemExported(ELIBEXPORT_ALBUMS) && !artistfoldersonly)
    {
      // Find albums to export
      std::vector<int> albumIds;
      std::string strSQL = PrepareSQL("SELECT idAlbum FROM album WHERE strReleaseType = '%s' ",
        CAlbum::ReleaseTypeToString(CAlbum::Album).c_str());
      if (!settings.m_unscraped)
        strSQL += "AND lastScraped IS NOT NULL";
      CLog::Log(LOGDEBUG, "CMusicDatabase::%s - %s", __FUNCTION__, strSQL.c_str());
      m_pDS->query(strSQL);

      int total = m_pDS->num_rows();
      int current = 0;

      albumIds.reserve(total);
      while (!m_pDS->eof())
      {
        albumIds.push_back(m_pDS->fv("idAlbum").get_asInt());
        m_pDS->next();
      }
      m_pDS->close();

      for (const auto &albumId : albumIds)
      {
        CAlbum album;
        GetAlbum(albumId, album);
        std::string strAlbumPath;
        std::string strPath;
        // Get album path, empty unless all album songs are under a unique folder, and
        // there are no songs from another album in the same folder.
        if (!GetAlbumPath(albumId, strAlbumPath))
          strAlbumPath.clear();
        if (settings.IsSingleFile())
        {
          // Save album to xml, including album path
          album.Save(pMain, "album", strAlbumPath);
        }
        else
        { // Separate files and artwork
          bool pathfound = false;
          if (settings.IsToLibFolders())
          { // Save album.nfo and artwork with music files.
            // Most albums are under a unique folder, but if songs from various albums are mixed then
            // avoid overwriting by not allow NFO and art to be exported
            if (strAlbumPath.empty())
              CLog::Log(LOGDEBUG, "CMusicDatabase::%s - Not exporting album %s as unique path not found",
                __FUNCTION__, album.strAlbum.c_str());
            else if (!CDirectory::Exists(strAlbumPath))
              CLog::Log(LOGDEBUG, "CMusicDatabase::%s - Not exporting album %s as found path %s does not exist",
                __FUNCTION__, album.strAlbum.c_str(), strAlbumPath.c_str());
            else
            {
              strPath = strAlbumPath;
              pathfound = true;
            }
          }
          else
          { // Save album.nfo and artwork to subfolder on export path
            // strPath = strFolder/<albumartist name>/<albumname>
            // where <albumname> is either the same name as the album folder
            // containing the music files (if unique) or is created using the album name
            std::string strAlbumArtist;
            pathfound = GetArtistFolderName(album.GetAlbumArtist()[0], album.GetMusicBrainzAlbumArtistID()[0], strAlbumArtist);
            if (pathfound)
            {
              strPath = URIUtils::AddFileToFolder(strFolder, strAlbumArtist);
              pathfound = CDirectory::Exists(strPath);
              if (!pathfound)
                pathfound = CDirectory::Create(strPath);
            }
            if (!pathfound)
              CLog::Log(LOGDEBUG, "CMusicDatabase::%s - Not exporting album %s as could not create %s",
                __FUNCTION__, album.strAlbum.c_str(), strPath.c_str());
            else
            {
              std::string strAlbumFolder;
              pathfound = GetAlbumFolder(album, strAlbumPath, strAlbumFolder);
              if (pathfound)
              {
                strPath = URIUtils::AddFileToFolder(strPath, strAlbumFolder);
                pathfound = CDirectory::Exists(strPath);
                if (!pathfound)
                  pathfound = CDirectory::Create(strPath);
              }
              if (!pathfound)
                CLog::Log(LOGDEBUG, "CMusicDatabase::%s - Not exporting album %s as could not create %s",
                  __FUNCTION__, album.strAlbum.c_str(), strPath.c_str());
            }
          }
          if (pathfound)
          {
            if (!settings.m_skipnfo)
            {
              // Save album to NFO, including album path
              album.Save(pMain, "album", strAlbumPath);
              std::string nfoFile = URIUtils::AddFileToFolder(strPath, "album.nfo");
              if (settings.m_overwrite || !CFile::Exists(nfoFile))
              {
                if (!xmlDoc.SaveFile(nfoFile))
                {
                  CLog::Log(LOGERROR, "CMusicDatabase::%s: Album nfo export failed! ('%s')", __FUNCTION__, nfoFile.c_str());
                  CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Error, g_localizeStrings.Get(20302), nfoFile);
                  iFailCount++;
                }
              }
            }
            if (settings.m_artwork)
            {
              // Save art in album folder
              // Note thumb resoluton may be lower than original when overwriting
              std::map<std::string, std::string> artwork;
              std::string savedArtfile;
              if (GetArtForItem(album.idAlbum, MediaTypeAlbum, artwork))
              {
                for (const auto &art : artwork)
                {
                  if (art.first == "thumb")
                    savedArtfile = URIUtils::AddFileToFolder(strPath, "folder");
                  else
                    savedArtfile = URIUtils::AddFileToFolder(strPath, art.first);
                  CTextureCache::GetInstance().Export(art.second, savedArtfile, settings.m_overwrite);
                }
              }
            }
            xmlDoc.Clear();
            TiXmlDeclaration decl("1.0", "UTF-8", "yes");
            xmlDoc.InsertEndChild(decl);
          }
        }

        if ((current % 50) == 0 && progressDialog)
        {
          progressDialog->SetLine(1, CVariant{ album.strAlbum });
          progressDialog->SetPercentage(current * 100 / total);
          if (progressDialog->IsCanceled())
            return;
        }
        current++;
      }
    }

    // Export song playback history to single file only
    if (settings.IsSingleFile() && settings.IsItemExported(ELIBEXPORT_SONGS))
    {
      if (!ExportSongHistory(pMain, progressDialog))
        return;
    }

    if ((settings.IsArtists() || artistfoldersonly) && !strFolder.empty())
    {
      // Find artists to export
      std::vector<int> artistIds;
      std::string sql;
      Filter filter;

      if (settings.IsItemExported(ELIBEXPORT_ALBUMARTISTS))
        filter.AppendWhere("EXISTS(SELECT 1 FROM album_artist WHERE album_artist.idArtist = artist.idArtist)", false);
      if (settings.IsItemExported(ELIBEXPORT_SONGARTISTS))
      {
        if (settings.IsItemExported(ELIBEXPORT_OTHERARTISTS))
          filter.AppendWhere("EXISTS (SELECT 1 FROM song_artist WHERE song_artist.idArtist = artist.idArtist )", false);
        else
          filter.AppendWhere("EXISTS (SELECT 1 FROM song_artist WHERE song_artist.idArtist = artist.idArtist AND song_artist.idRole = 1)", false);
      }
      else if (settings.IsItemExported(ELIBEXPORT_OTHERARTISTS))
        filter.AppendWhere("EXISTS (SELECT 1 FROM song_artist WHERE song_artist.idArtist = artist.idArtist AND song_artist.idRole > 1)", false);

      if (!settings.m_unscraped && !artistfoldersonly)
        filter.AppendWhere("lastScraped IS NOT NULL", true);

      std::string strSQL = "SELECT idArtist FROM artist";
      BuildSQL(strSQL, filter, strSQL);
      CLog::Log(LOGDEBUG, "CMusicDatabase::%s - %s", __FUNCTION__, strSQL.c_str());

      m_pDS->query(strSQL);
      int total = m_pDS->num_rows();
      int current = 0;
      artistIds.reserve(total);
      while (!m_pDS->eof())
      {
        artistIds.push_back(m_pDS->fv("idArtist").get_asInt());
        m_pDS->next();
      }
      m_pDS->close();

      for (const auto &artistId : artistIds)
      {
        CArtist artist;
        GetArtist(artistId, artist, !artistfoldersonly); // include discography when not folders only
        std::string strPath;
        std::map<std::string, std::string> artwork;
        if (settings.IsSingleFile())
        {
          // Save artist to xml, and old path (common to music files) if it has one
          GetOldArtistPath(artist.idArtist, strPath);
          artist.Save(pMain, "artist", strPath);

          if (GetArtForItem(artist.idArtist, MediaTypeArtist, artwork))
          { // append to the XML
            TiXmlElement additionalNode("art");
            for (const auto &i : artwork)
              XMLUtils::SetString(&additionalNode, i.first.c_str(), i.second);
            pMain->LastChild()->InsertEndChild(additionalNode);
          }
        }
        else
        { // Separate files: artist.nfo and artwork in strFolder/<artist name>
          // Get unique folder allowing for duplicate names e.g. 2 x John Williams
          bool pathfound = GetArtistFolderName(artist, strPath);
          if (pathfound)
          {
            strPath = URIUtils::AddFileToFolder(strFolder, strPath);
            pathfound = CDirectory::Exists(strPath);
            if (!pathfound)
              pathfound = CDirectory::Create(strPath);
          }
          if (!pathfound)
            CLog::Log(LOGDEBUG, "CMusicDatabase::%s - Not exporting artist %s as could not create %s",
              __FUNCTION__, artist.strArtist.c_str(), strPath.c_str());
          else
          {
            if (!artistfoldersonly)
            {
              if (!settings.m_skipnfo)
              {
                artist.Save(pMain, "artist", strPath);
                std::string nfoFile = URIUtils::AddFileToFolder(strPath, "artist.nfo");
                if (settings.m_overwrite || !CFile::Exists(nfoFile))
                {
                  if (!xmlDoc.SaveFile(nfoFile))
                  {
                    CLog::Log(LOGERROR, "CMusicDatabase::%s: Artist nfo export failed! ('%s')", __FUNCTION__, nfoFile.c_str());
                    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Error, g_localizeStrings.Get(20302), nfoFile);
                    iFailCount++;
                  }
                }
              }
              if (settings.m_artwork)
              {
                std::string savedArtfile;
                if (GetArtForItem(artist.idArtist, MediaTypeArtist, artwork))
                {
                  for (const auto &art : artwork)
                  {
                    if (art.first == "thumb")
                      savedArtfile = URIUtils::AddFileToFolder(strPath, "folder");
                    else
                      savedArtfile = URIUtils::AddFileToFolder(strPath, art.first);
                    CTextureCache::GetInstance().Export(art.second, savedArtfile, settings.m_overwrite);
                  }
                }
              }
              xmlDoc.Clear();
              TiXmlDeclaration decl("1.0", "UTF-8", "yes");
              xmlDoc.InsertEndChild(decl);
            }
          }
        }
        if ((current % 50) == 0 && progressDialog)
        {
          progressDialog->SetLine(1, CVariant{ artist.strArtist });
          progressDialog->SetPercentage(current * 100 / total);
          if (progressDialog->IsCanceled())
            return;
        }
        current++;
      }
    }

    if (settings.IsSingleFile())
    {
      std::string xmlFile = URIUtils::AddFileToFolder(strFolder, "kodi_musicdb" + CDateTime::GetCurrentDateTime().GetAsDBDate() + ".xml");
      if (CFile::Exists(xmlFile))
        xmlFile = URIUtils::AddFileToFolder(strFolder, "kodi_musicdb" + CDateTime::GetCurrentDateTime().GetAsSaveString() + ".xml");
      xmlDoc.SaveFile(xmlFile);

      CVariant data;
      data["file"] = xmlFile;
      if (iFailCount > 0)
        data["failcount"] = iFailCount;
      CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::AudioLibrary, "xbmc", "OnExport", data);
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CMusicDatabase::%s failed", __FUNCTION__);
    iFailCount++;
  }

  if (progressDialog)
    progressDialog->Close();

  if (iFailCount > 0 && progressDialog)
    HELPERS::ShowOKDialogLines(CVariant{20196}, CVariant{StringUtils::Format(g_localizeStrings.Get(15011).c_str(), iFailCount)});
}

bool CMusicDatabase::ExportSongHistory(TiXmlNode* pNode, CGUIDialogProgress* progressDialog)
{
  try
  {
    // Export songs with some playback history
    std::string strSQL = "SELECT idSong, song.idAlbum, "
      "strAlbum, strMusicBrainzAlbumID, album.strArtistDisp AS strAlbumArtistDisp, "
      "song.strArtistDisp, strTitle, iTrack, strFileName, strMusicBrainzTrackID, "
      "iTimesPlayed, lastplayed, song.rating, song.votes, song.userrating "
      "FROM song JOIN album on album.idAlbum = song.idAlbum "
      "WHERE iTimesPlayed > 0 OR rating > 0 or userrating > 0";

    CLog::Log(LOGDEBUG, "{0} - {1}", __FUNCTION__, strSQL.c_str());
    m_pDS->query(strSQL);

    int total = m_pDS->num_rows();
    int current = 0;
    while (!m_pDS->eof())
    {
      TiXmlElement songElement("song");
      TiXmlNode* song = pNode->InsertEndChild(songElement);

      XMLUtils::SetInt(song, "idsong", m_pDS->fv("idSong").get_asInt());
      XMLUtils::SetString(song, "artistdesc", m_pDS->fv("strArtistDisp").get_asString());
      XMLUtils::SetString(song, "title", m_pDS->fv("strTitle").get_asString());
      XMLUtils::SetInt(song, "track", m_pDS->fv("iTrack").get_asInt());
      XMLUtils::SetString(song, "filename", m_pDS->fv("strFilename").get_asString());
      XMLUtils::SetString(song, "musicbrainztrackid", m_pDS->fv("strMusicBrainzTrackID").get_asString());
      XMLUtils::SetInt(song, "idalbum", m_pDS->fv("idAlbum").get_asInt());
      XMLUtils::SetString(song, "albumtitle", m_pDS->fv("strAlbum").get_asString());
      XMLUtils::SetString(song, "musicbrainzalbumid", m_pDS->fv("strMusicBrainzAlbumID").get_asString());
      XMLUtils::SetString(song, "albumartistdesc", m_pDS->fv("strAlbumArtistDisp").get_asString());
      XMLUtils::SetInt(song, "timesplayed", m_pDS->fv("iTimesplayed").get_asInt());
      XMLUtils::SetString(song, "lastplayed", m_pDS->fv("lastplayed").get_asString());
      auto* rating = XMLUtils::SetString(song, "rating", StringUtils::FormatNumber(m_pDS->fv("rating").get_asFloat()));
      if (rating)
        rating->ToElement()->SetAttribute("max", 10);
      XMLUtils::SetInt(song, "votes", m_pDS->fv("votes").get_asInt());
      auto* userrating = XMLUtils::SetInt(song, "userrating", m_pDS->fv("userrating").get_asInt());
      if (userrating)
        userrating->ToElement()->SetAttribute("max", 10);

      if ((current % 100) == 0 && progressDialog)
      {
        progressDialog->SetLine(1, CVariant{ m_pDS->fv("strAlbum").get_asString() });
        progressDialog->SetPercentage(current * 100 / total);
        if (progressDialog->IsCanceled())
        {
          m_pDS->close();
          return false;
        }
      }
      current++;

      m_pDS->next();
    }
    m_pDS->close();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "{0} failed", __FUNCTION__);
  }
  return false;
}

void CMusicDatabase::ImportFromXML(const std::string& xmlFile, CGUIDialogProgress* progressDialog)
{  
  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    CXBMCTinyXML xmlDoc;
    if (!xmlDoc.LoadFile(xmlFile) && progressDialog)
    {
      HELPERS::ShowOKDialogLines(CVariant{ 20197 }, CVariant{ 38354 }); //"Unable to read xml file"
      return;
    }

    TiXmlElement *root = xmlDoc.RootElement();
    if (!root) return;

    TiXmlElement *entry = root->FirstChildElement();
    int current = 0;
    int total = 0;
    int songtotal = 0;
    // Count the number of artists, albums and songs
    while (entry)
    {
      if (strnicmp(entry->Value(), "artist", 6)==0 ||
          strnicmp(entry->Value(), "album", 5)==0)
        total++;
      else if (strnicmp(entry->Value(), "song", 4) == 0)
        songtotal++;

      entry = entry->NextSiblingElement();
    }

    BeginTransaction();
    entry = root->FirstChildElement();
    while (entry)
    {
      std::string strTitle;
      if (strnicmp(entry->Value(), "artist", 6) == 0)
      {
        CArtist importedArtist;
        importedArtist.Load(entry);
        strTitle = importedArtist.strArtist;

        // Match by mbid first (that is definatively unique), then name (no mbid), finally by just name
        int idArtist = GetArtistByMatch(importedArtist);
        if (idArtist > -1)
        {
          CArtist artist;
          GetArtist(idArtist, artist, true); // include discography
          artist.MergeScrapedArtist(importedArtist, true);
          UpdateArtist(artist);
        }
        else
          CLog::Log(LOGDEBUG, "%s - Not import additional artist data as %s not found", __FUNCTION__, importedArtist.strArtist.c_str());
        current++;
      }
      else if (strnicmp(entry->Value(), "album", 5) == 0)
      {
        CAlbum importedAlbum;
        importedAlbum.Load(entry);
        strTitle = importedAlbum.strAlbum;
        // Match by mbid first (that is definatively unique), then title and artist desc (no mbid), finally by just name and artist
        int idAlbum = GetAlbumByMatch(importedAlbum);
        if (idAlbum > -1)
        {
          CAlbum album;
          GetAlbum(idAlbum, album, true);
          album.MergeScrapedAlbum(importedAlbum, true);
          UpdateAlbum(album); //Will replace song artists if present in xml
        }
        else
          CLog::Log(LOGDEBUG, "%s - Not import additional album data as %s not found", __FUNCTION__, importedAlbum.strAlbum.c_str());

        current++;
      }
      entry = entry ->NextSiblingElement();
      if (progressDialog && total)
      {
        progressDialog->SetPercentage(current * 100 / total);
        progressDialog->SetLine(2, CVariant{std::move(strTitle)});
        progressDialog->Progress();
        if (progressDialog->IsCanceled())
        {
          RollbackTransaction();
          return;
        }
      }
    }
    CommitTransaction();

    // Import song playback history <song> entries found
    if (songtotal > 0)
      if (!ImportSongHistory(xmlFile, songtotal, progressDialog))
        return;

    CGUIComponent* gui = CServiceBroker::GetGUI();
    if (gui)
      gui->GetInfoManager().GetInfoProviders().GetLibraryInfoProvider().ResetLibraryBools();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
    RollbackTransaction();
  }
  if (progressDialog)
    progressDialog->Close();
}

bool CMusicDatabase::ImportSongHistory(const std::string& xmlFile, const int total,  CGUIDialogProgress* progressDialog)
{  
  bool bHistSongExists = false;
  try
  {
    CXBMCTinyXML xmlDoc;
    if (!xmlDoc.LoadFile(xmlFile))
      return false;

    TiXmlElement* root = xmlDoc.RootElement();
    if (!root) 
      return false;

    TiXmlElement* entry = root->FirstChildElement();
    int current = 0;
    
    if (progressDialog)
    {
      progressDialog->SetLine(1, CVariant{38350}); //"Importing song playback history"
      progressDialog->SetLine(2, CVariant{ "" });
    }
   
    // As can be many songs do in db, not song at a time which would be slow
    // Convert xml entries into a SQL bulk insert statement
    std::string strSQL;    
    entry = root->FirstChildElement();
    while (entry)
    {
      std::string strArtistDisp;
      std::string strTitle;
      int iTrack;
      std::string strFilename;
      std::string strMusicBrainzTrackID;
      std::string strAlbum;
      std::string strMusicBrainzAlbumID;
      std::string strAlbumArtistDisp;
      int iTimesplayed;
      std::string lastplayed;
      int iUserrating = 0;
      float fRating = 0.0;
      int iVotes;
      std::string strSQLSong;
      if (strnicmp(entry->Value(), "song", 4) == 0)
      {      
        XMLUtils::GetString(entry, "artistdesc", strArtistDisp);
        XMLUtils::GetString(entry, "title", strTitle);
        XMLUtils::GetInt(entry, "track", iTrack);
        XMLUtils::GetString(entry, "filename", strFilename);
        XMLUtils::GetString(entry, "musicbrainztrackid", strMusicBrainzTrackID);
        XMLUtils::GetString(entry, "albumtitle", strAlbum);
        XMLUtils::GetString(entry, "musicbrainzalbumid", strMusicBrainzAlbumID);
        XMLUtils::GetString(entry, "albumartistdesc", strAlbumArtistDisp);
        XMLUtils::GetInt(entry, "timesplayed", iTimesplayed);
        XMLUtils::GetString(entry, "lastplayed", lastplayed);
        const TiXmlElement* rElement = entry->FirstChildElement("rating");
        if (rElement)
        {
          float rating = 0;
          float max_rating = 10;
          XMLUtils::GetFloat(entry, "rating", rating);
          if (rElement->QueryFloatAttribute("max", &max_rating) == TIXML_SUCCESS && max_rating >= 1)
            rating *= (10.f / max_rating); // Normalise the value to between 0 and 10
          if (rating > 10.f)
            rating = 10.f;
          fRating = rating;
        }        
        XMLUtils::GetInt(entry, "votes", iVotes);
        const TiXmlElement* userrating = entry->FirstChildElement("userrating");
        if (userrating)
        {
          float rating = 0;
          float max_rating = 10;
          XMLUtils::GetFloat(entry, "userrating", rating);
          if (userrating->QueryFloatAttribute("max", &max_rating) == TIXML_SUCCESS && max_rating >= 1)
            rating *= (10.f / max_rating); // Normalise the value to between 0 and 10
          if (rating > 10.f)
            rating = 10.f;
          iUserrating = MathUtils::round_int(rating);
        }

        strSQLSong = PrepareSQL("(%d, %d, ", current + 1, iTrack);
        strSQLSong += PrepareSQL("'%s', '%s', '%s', ", strArtistDisp.c_str(), strTitle.c_str(), strFilename.c_str());
        if (strMusicBrainzTrackID.empty())
          strSQLSong += PrepareSQL("NULL, ");
        else
          strSQLSong += PrepareSQL("'%s', ", strMusicBrainzTrackID.c_str());
        strSQLSong += PrepareSQL("'%s', '%s', ", strAlbum.c_str(), strAlbumArtistDisp.c_str());
        if (strMusicBrainzAlbumID.empty())
          strSQLSong += PrepareSQL("NULL, ");
        else
          strSQLSong += PrepareSQL("'%s', ", strMusicBrainzAlbumID.c_str());
        strSQLSong += PrepareSQL("%d, ", iTimesplayed);
        if (lastplayed.empty())
          strSQLSong += PrepareSQL("NULL, ");
        else
          strSQLSong += PrepareSQL("'%s', ", lastplayed.c_str());
        strSQLSong += PrepareSQL("%.1f, %d, %d, -1, -1)", fRating, iVotes, iUserrating);

        if (current > 0)
          strSQLSong = ", " + strSQLSong;
        strSQL += strSQLSong;
        current++;
      }

      entry = entry->NextSiblingElement();

      if ((current % 100) == 0 && progressDialog)
      {
        progressDialog->SetPercentage(current * 100 / total);
        progressDialog->SetLine(3, CVariant{ std::move(strTitle) });
        progressDialog->Progress();
        if (progressDialog->IsCanceled())
          return false;
      }
    }

    CLog::Log(LOGINFO, "{0}: Create temporary HistSong table and insert {1} records", __FUNCTION__, total);
    /* Can not use CREATE TEMPORARY TABLE as MySQL does not support updates of
       song table using correlated subqueries to a temp table. An updatable join
       to temp table would work in MySQL but SQLite not support updatable joins.
    */
    m_pDS->exec("CREATE TABLE HistSong ("
      "idSongSrc INTEGER primary key, "
      "strAlbum varchar(256), "
      "strMusicBrainzAlbumID text, "
      "strAlbumArtistDisp text, "
      "strArtistDisp text, strTitle varchar(512), "
      "iTrack INTEGER, strFileName text, strMusicBrainzTrackID text, "
      "iTimesPlayed INTEGER, lastplayed varchar(20) default NULL, "
      "rating FLOAT NOT NULL DEFAULT 0, votes INTEGER NOT NULL DEFAULT 0, "
      "userrating INTEGER NOT NULL DEFAULT 0, "
      "idAlbum INTEGER, idSong INTEGER)");
    bHistSongExists = true;

    strSQL = "INSERT INTO HistSong (idSongSrc, iTrack, strArtistDisp, strTitle, "
      "strFileName, strMusicBrainzTrackID, "
      "strAlbum, strAlbumArtistDisp, strMusicBrainzAlbumID, "
      " iTimesPlayed, lastplayed, rating, votes, userrating, idAlbum, idSong) VALUES " + strSQL;
    m_pDS->exec(strSQL);

    if (progressDialog)
    {
      progressDialog->SetLine(2, CVariant{38351}); //"Matching data" 
      progressDialog->SetLine(3, CVariant{ "" });
      progressDialog->Progress();
      if (progressDialog->IsCanceled())
      {
        m_pDS->exec("DROP TABLE HistSong");
        return false;
      }
    }

    BeginTransaction();
    // Match albums first on mbid then artist string and album title, setting idAlbum 
    strSQL = "UPDATE HistSong "
      "SET idAlbum = (SELECT album.idAlbum FROM album "
      "WHERE album.strMusicBrainzAlbumID = HistSong.strMusicBrainzAlbumID) "
      "WHERE EXISTS(SELECT 1 FROM album "
      "WHERE album.strMusicBrainzAlbumID = HistSong.strMusicBrainzAlbumID) AND idAlbum < 0";
    m_pDS->exec(strSQL);

    strSQL = "UPDATE HistSong "
      "SET idAlbum = (SELECT album.idAlbum FROM album "
      "WHERE HistSong.strAlbumArtistDisp = album.strArtistDisp AND HistSong.strAlbum = album.strAlbum) "
      "WHERE EXISTS(SELECT 1 FROM album "
      "WHERE HistSong.strAlbumArtistDisp = album.strArtistDisp AND HistSong.strAlbum = album.strAlbum)"
      "AND idAlbum < 0";
    m_pDS->exec(strSQL);
    if (progressDialog)
    {
      progressDialog->Progress();
      if (progressDialog->IsCanceled())
      {
        RollbackTransaction();
        m_pDS->exec("DROP TABLE HistSong");
        return false;
      }
    }

    // Match songs on first on idAlbum, track and mbid, then idAlbum, track and title, setting idSong
    strSQL = "UPDATE HistSong "
      "SET idSong = (SELECT idsong FROM song "
      "WHERE HistSong.idAlbum = song.idAlbum AND "
      "HistSong.iTrack = song.iTrack AND "
      "HistSong.strMusicBrainzTrackID = song.strMusicBrainzTrackID) "
      "WHERE EXISTS(SELECT 1 FROM song "
      "WHERE HistSong.idAlbum = song.idAlbum AND "
      "HistSong.iTrack = song.iTrack AND "
      "HistSong.strMusicBrainzTrackID = song.strMusicBrainzTrackID) AND idSong < 0";
    m_pDS->exec(strSQL);

    strSQL = "UPDATE HistSong "
      "SET idSong = (SELECT idsong FROM song "
      "WHERE HistSong.idAlbum = song.idAlbum AND "
      "HistSong.iTrack = song.iTrack AND HistSong.strTitle = song.strTitle) "
      "WHERE EXISTS(SELECT 1 FROM song "
      "WHERE HistSong.idAlbum = song.idAlbum AND "
      "HistSong.iTrack = song.iTrack AND HistSong.strTitle = song.strTitle) AND idSong < 0";
    m_pDS->exec(strSQL);
    CommitTransaction();
    if (progressDialog)
    {
      progressDialog->Progress();
      if (progressDialog->IsCanceled())
      {
        m_pDS->exec("DROP TABLE HistSong");
        return false;
      }
    }

    // Create an index to speed up the updates
    m_pDS->exec("CREATE INDEX idxHistSong ON HistSong(idSong)");

    // Log how many songs matched
    int unmatched = static_cast<int>(strtol(GetSingleValue("SELECT COUNT(1) FROM HistSong WHERE idSong < 0", m_pDS).c_str(), nullptr, 10));
    CLog::Log(LOGINFO, "{0}: Importing song history {1} of {2} songs matched", __FUNCTION__, total - unmatched,  total);

    if (progressDialog)
    {
      progressDialog->SetLine(2, CVariant{38352}); //"Updating song playback history"
      progressDialog->Progress();
      if (progressDialog->IsCanceled())
      {
        m_pDS->exec("DROP TABLE HistSong"); // Drops index too
        return false;
      }
    }

    /* Update song table using the song ids we have matched. 
      Use correlated subqueries as SQLite does not support updatable joins.
      MySQL requires HistSong table not to be defined temporary for this. 
    */
    BeginTransaction();
    // Times played and last played date(when count is greater)
    strSQL = "UPDATE song SET iTimesPlayed = "
      "(SELECT iTimesPlayed FROM HistSong WHERE HistSong.idSong = song.idSong), "
      "lastplayed = "
      "(SELECT lastplayed FROM HistSong WHERE HistSong.idSong = song.idSong) "
      "WHERE  EXISTS(SELECT 1 FROM HistSong WHERE "
      "HistSong.idSong = song.idSong AND HistSong.iTimesPlayed > song.iTimesPlayed)";
    m_pDS->exec(strSQL);
    
    // User rating
    strSQL = "UPDATE song SET userrating = "
      "(SELECT userrating FROM HistSong WHERE HistSong.idSong = song.idSong) "
      "WHERE  EXISTS(SELECT 1 FROM HistSong WHERE "
      "HistSong.idSong = song.idSong AND HistSong.userrating > 0)";
    m_pDS->exec(strSQL);
    
    // Rating and votes
    strSQL = "UPDATE song SET rating = "
      "(SELECT rating FROM HistSong WHERE HistSong.idSong = song.idSong), "
      "votes = "
      "(SELECT votes FROM HistSong WHERE HistSong.idSong = song.idSong) "
      "WHERE  EXISTS(SELECT 1 FROM HistSong WHERE "
      "HistSong.idSong = song.idSong AND HistSong.rating > 0)";
    m_pDS->exec(strSQL);

    if (progressDialog)
    {
      progressDialog->Progress();
      if (progressDialog->IsCanceled())
      {
        RollbackTransaction();
        m_pDS->exec("DROP TABLE HistSong");
        return false;
      }
    }
    CommitTransaction();

    // Tidy up temp table (index also removed)
    m_pDS->exec("DROP TABLE HistSong");
    // Compact db to recover space as had to add/drop actual table
    if (progressDialog)
    {
      progressDialog->SetLine(2, CVariant{ 331 });
      progressDialog->Progress();
    }
    Compress(false);    

    // Write event log entry
    // "Importing song history {1} of {2} songs matched", total - unmatched, total)
    std::string strLine = StringUtils::Format(g_localizeStrings.Get(38353).c_str(), total - unmatched, total);
    CServiceBroker::GetEventLog().Add(
      EventPtr(new CNotificationEvent(20197, strLine, EventLevel::Information)));

    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
    RollbackTransaction();
    if (bHistSongExists)
      m_pDS->exec("DROP TABLE HistSong");
  }
  return false;
}

void CMusicDatabase::SetPropertiesFromArtist(CFileItem& item, const CArtist& artist)
{
  const std::string itemSeparator = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_musicItemSeparator;

  item.SetProperty("artist_sortname", artist.strSortName);
  item.SetProperty("artist_type", artist.strType);
  item.SetProperty("artist_gender", artist.strGender);
  item.SetProperty("artist_disambiguation", artist.strDisambiguation);
  item.SetProperty("artist_instrument", StringUtils::Join(artist.instruments, itemSeparator));
  item.SetProperty("artist_instrument_array", artist.instruments);
  item.SetProperty("artist_style", StringUtils::Join(artist.styles, itemSeparator));
  item.SetProperty("artist_style_array", artist.styles);
  item.SetProperty("artist_mood", StringUtils::Join(artist.moods, itemSeparator));
  item.SetProperty("artist_mood_array", artist.moods);
  item.SetProperty("artist_born", artist.strBorn);
  item.SetProperty("artist_formed", artist.strFormed);
  item.SetProperty("artist_description", artist.strBiography);
  item.SetProperty("artist_genre", StringUtils::Join(artist.genre, itemSeparator));
  item.SetProperty("artist_genre_array", artist.genre);
  item.SetProperty("artist_died", artist.strDied);
  item.SetProperty("artist_disbanded", artist.strDisbanded);
  item.SetProperty("artist_yearsactive", StringUtils::Join(artist.yearsActive, itemSeparator));
  item.SetProperty("artist_yearsactive_array", artist.yearsActive);
}

void CMusicDatabase::SetPropertiesFromAlbum(CFileItem& item, const CAlbum& album)
{
  const std::string itemSeparator = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_musicItemSeparator;

  item.SetProperty("album_description", album.strReview);
  item.SetProperty("album_theme", StringUtils::Join(album.themes, itemSeparator));
  item.SetProperty("album_theme_array", album.themes);
  item.SetProperty("album_mood", StringUtils::Join(album.moods, itemSeparator));
  item.SetProperty("album_mood_array", album.moods);
  item.SetProperty("album_style", StringUtils::Join(album.styles, itemSeparator));
  item.SetProperty("album_style_array", album.styles);
  item.SetProperty("album_type", album.strType);
  item.SetProperty("album_label", album.strLabel);
  item.SetProperty("album_artist", album.GetAlbumArtistString());
  item.SetProperty("album_artist_array", album.GetAlbumArtist());
  item.SetProperty("album_genre", StringUtils::Join(album.genre, itemSeparator));
  item.SetProperty("album_genre_array", album.genre);
  item.SetProperty("album_title", album.strAlbum);
  if (album.fRating > 0)
    item.SetProperty("album_rating", StringUtils::FormatNumber(album.fRating));
  if (album.iUserrating > 0)
    item.SetProperty("album_userrating", album.iUserrating);
  if (album.iVotes > 0)
    item.SetProperty("album_votes", album.iVotes);
  item.SetProperty("album_releasetype", CAlbum::ReleaseTypeToString(album.releaseType));
}

void CMusicDatabase::SetPropertiesForFileItem(CFileItem& item)
{
  if (!item.HasMusicInfoTag())
    return;
  // May already have song artist ids as item property set when data read from
  // db, but check property is valid array (scripts could set item properties
  // incorrectly), otherwise try to fetch artist by name.
  int idArtist = -1;
  if (item.HasProperty("artistid") && item.GetProperty("artistid").isArray())
  {
    CVariant::const_iterator_array varid = item.GetProperty("artistid").begin_array();
    idArtist = varid->asInteger();
  }
  else
    idArtist = GetArtistByName(item.GetMusicInfoTag()->GetArtistString());
  if (idArtist > -1)
  {
    CArtist artist;
    if (GetArtist(idArtist, artist))
      SetPropertiesFromArtist(item,artist);
  }
  int idAlbum = item.GetMusicInfoTag()->GetAlbumId();
  if (idAlbum <= 0)
    idAlbum = GetAlbumByName(item.GetMusicInfoTag()->GetAlbum(),
                             item.GetMusicInfoTag()->GetArtistString());
  if (idAlbum > -1)
  {
    CAlbum album;
    if (GetAlbum(idAlbum, album, false))
      SetPropertiesFromAlbum(item,album);
  }
}

void CMusicDatabase::SetArtForItem(int mediaId, const std::string &mediaType, const std::map<std::string, std::string> &art)
{
  for (const auto &i : art)
    SetArtForItem(mediaId, mediaType, i.first, i.second);
}

void CMusicDatabase::SetArtForItem(int mediaId, const std::string &mediaType, const std::string &artType, const std::string &url)
{
  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    // don't set <foo>.<bar> art types - these are derivative types from parent items
    if (artType.find('.') != std::string::npos)
      return;

    std::string sql = PrepareSQL("SELECT art_id FROM art WHERE media_id=%i AND media_type='%s' AND type='%s'", mediaId, mediaType.c_str(), artType.c_str());
    m_pDS->query(sql);
    if (!m_pDS->eof())
    { // update
      int artId = m_pDS->fv(0).get_asInt();
      m_pDS->close();
      sql = PrepareSQL("UPDATE art SET url='%s' where art_id=%d", url.c_str(), artId);
      m_pDS->exec(sql);
    }
    else
    { // insert
      m_pDS->close();
      sql = PrepareSQL("INSERT INTO art(media_id, media_type, type, url) VALUES (%d, '%s', '%s', '%s')", mediaId, mediaType.c_str(), artType.c_str(), url.c_str());
      m_pDS->exec(sql);
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%d, '%s', '%s', '%s') failed", __FUNCTION__, mediaId, mediaType.c_str(), artType.c_str(), url.c_str());
  }
}

bool CMusicDatabase::GetArtForItem(int songId, int albumId, int artistId, bool bPrimaryArtist, std::vector<ArtForThumbLoader> &art)
{
  std::string strSQL;
  try
  {
    if (!(songId > 0 || albumId > 0 || artistId > 0)) return false;
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS2.get()) return false; // using dataset 2 as we're likely called in loops on dataset 1

    Filter filter;
    if (songId > 0)
      filter.AppendWhere(PrepareSQL("media_id = %i AND media_type ='%s'", songId, MediaTypeSong));
    if (albumId > 0)
      filter.AppendWhere(PrepareSQL("media_id = %i AND media_type ='%s'", albumId, MediaTypeAlbum), false);
    if (artistId > 0)
      filter.AppendWhere(PrepareSQL("media_id = %i AND media_type ='%s'", artistId, MediaTypeArtist), false);

    strSQL = "SELECT DISTINCT art_id, media_id, media_type, type, '' as prefix, url, 0 as iorder FROM art";
    if (!BuildSQL(strSQL, filter, strSQL))
      return false;

    if (!(artistId > 0))
    {
      // Artist ID unknown, so lookup album artist for albums and songs
      std::string strSQL2;
      if (albumId > 0)
      {
        //Album ID known, so use it to look up album artist(s)
        strSQL2 = PrepareSQL(
          "SELECT art_id, media_id, media_type, type, 'albumartist' as prefix, "
          "url, album_artist.iOrder as iorder FROM art "
          "JOIN album_artist ON art.media_id = album_artist.idArtist AND art.media_type ='%s' "
          "WHERE album_artist.idAlbum = %i ",
          MediaTypeArtist, albumId);
        if (bPrimaryArtist)
          strSQL2 += "AND album_artist.iOrder = 0";

        strSQL = strSQL + " UNION " + strSQL2;
      }
      if (songId > 0)
      {
        if (albumId < 0)
        {
          //Album ID unknown, so get from song to look up album artist(s)
          strSQL2 = PrepareSQL(
            "SELECT art_id, media_id, media_type, type, 'albumartist' as prefix, "
            "url, album_artist.iOrder as iorder FROM art "
            "JOIN album_artist ON art.media_id = album_artist.idArtist AND art.media_type ='%s' "
            "JOIN song ON song.idAlbum = album_artist.idAlbum  "
            "WHERE song.idSong = %i ",
            MediaTypeArtist, songId);
          if (bPrimaryArtist)
            strSQL2 += "AND album_artist.iOrder = 0";

          strSQL = strSQL + " UNION " + strSQL2;
        }

        // Artist ID unknown, so lookup artist for songs (could be different from album artist)
        strSQL2 = PrepareSQL(
          "SELECT art_id, media_id, media_type, type, 'artist' as prefix, "
          "url, song_artist.iOrder as iorder FROM art "
          "JOIN song_artist on art.media_id = song_artist.idArtist AND art.media_type = '%s' "
          "WHERE song_artist.idsong = %i AND song_artist.idRole = %i ",
          MediaTypeArtist, songId, ROLE_ARTIST);
        if (bPrimaryArtist)
          strSQL2 += "AND song_artist.iOrder = 0";

        strSQL = strSQL +  " UNION " + strSQL2;
      }
    }
    if (songId > 0 && albumId < 0)
    {
      //Album ID unknown, so get from song to look up album art
      std::string strSQL2;
      strSQL2 = PrepareSQL(
        "SELECT art_id, media_id, media_type, type, '' as prefix, "
        "url, 0 as iorder FROM art "
        "JOIN song ON art.media_id = song.idAlbum AND art.media_type ='%s' "
        "WHERE song.idSong = %i ",
        MediaTypeAlbum, songId);
      strSQL = strSQL + " UNION " + strSQL2;
    }

    m_pDS2->query(strSQL);
    while (!m_pDS2->eof())
    {
      ArtForThumbLoader artitem;
      artitem.artType = m_pDS2->fv("type").get_asString();
      artitem.mediaType = m_pDS2->fv("media_type").get_asString();
      artitem.prefix = m_pDS2->fv("prefix").get_asString();
      artitem.url = m_pDS2->fv("url").get_asString();
      int iOrder = m_pDS2->fv("iorder").get_asInt();
      // Add order to prefix for multiple artist art for songs and albums e.g. "albumartist2"
      if (iOrder > 0)
        artitem.prefix += m_pDS2->fv("iorder").get_asString();

      art.emplace_back(artitem);
      m_pDS2->next();
    }
    m_pDS2->close();
    return !art.empty();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%s) failed", __FUNCTION__, strSQL.c_str());
  }
  return false;
}

bool CMusicDatabase::GetArtForItem(int mediaId, const std::string &mediaType, std::map<std::string, std::string> &art)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS2.get()) return false; // using dataset 2 as we're likely called in loops on dataset 1

    std::string sql = PrepareSQL("SELECT type,url FROM art WHERE media_id=%i AND media_type='%s'", mediaId, mediaType.c_str());
    m_pDS2->query(sql);
    while (!m_pDS2->eof())
    {
      art.insert(std::make_pair(m_pDS2->fv(0).get_asString(), m_pDS2->fv(1).get_asString()));
      m_pDS2->next();
    }
    m_pDS2->close();
    return !art.empty();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%d) failed", __FUNCTION__, mediaId);
  }
  return false;
}

std::string CMusicDatabase::GetArtForItem(int mediaId, const std::string &mediaType, const std::string &artType)
{
  std::string query = PrepareSQL("SELECT url FROM art WHERE media_id=%i AND media_type='%s' AND type='%s'", mediaId, mediaType.c_str(), artType.c_str());
  return GetSingleValue(query, m_pDS2);
}

bool CMusicDatabase::RemoveArtForItem(int mediaId, const MediaType & mediaType, const std::string & artType)
{
  return ExecuteQuery(PrepareSQL("DELETE FROM art WHERE media_id=%i AND media_type='%s' AND type='%s'", mediaId, mediaType.c_str(), artType.c_str()));
}

bool CMusicDatabase::RemoveArtForItem(int mediaId, const MediaType & mediaType, const std::set<std::string>& artTypes)
{
  bool result = true;
  for (const auto &i : artTypes)
    result &= RemoveArtForItem(mediaId, mediaType, i);

  return result;
}

bool CMusicDatabase::GetArtTypes(const MediaType &mediaType, std::vector<std::string> &artTypes)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    std::string strSQL = PrepareSQL("SELECT DISTINCT type FROM art WHERE media_type='%s'", mediaType.c_str());

    if (!m_pDS->query(strSQL)) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return false;
    }

    while (!m_pDS->eof())
    {
      artTypes.emplace_back(m_pDS->fv(0).get_asString());
      m_pDS->next();
    }
    m_pDS->close();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%s) failed", __FUNCTION__, mediaType.c_str());
  }
  return false;
}

std::vector<std::string> CMusicDatabase::GetAvailableArtTypesForItem(int mediaId,
  const MediaType& mediaType)
{
  std::vector<std::string> result;
  if (mediaType == MediaTypeArtist)
  {
    CArtist artist;
    if (GetArtist(mediaId, artist))
    {
      //! @todo artwork: fanart stored separately, doesn't need to be
      if (artist.fanart.GetNumFanarts())
        result.push_back("fanart");

      // all other images
      for (const auto& urlEntry : artist.thumbURL.m_url)
      {
        std::string artType = urlEntry.m_aspect;
        if (artType.empty())
          artType = "thumb";
        if (std::find(result.begin(), result.end(), artType) == result.end())
          result.push_back(artType);
      }
    }
  }
  else if (mediaType == MediaTypeAlbum)
  {
    CAlbum album;
    if (GetAlbum(mediaId, album))
    {
      for (const auto& urlEntry : album.thumbURL.m_url)
      {
        std::string artType = urlEntry.m_aspect;
        if (artType.empty())
          artType = "thumb";
        if (std::find(result.begin(), result.end(), artType) == result.end())
          result.push_back(artType);
      }
    }
  }
  return result;
}

bool CMusicDatabase::GetFilter(CDbUrl &musicUrl, Filter &filter, SortDescription &sorting)
{
  if (!musicUrl.IsValid())
    return false;

  std::string type = musicUrl.GetType();
  const CUrlOptions::UrlOptions& options = musicUrl.GetOptions();

  // Check for playlist rules first, they may contain role criteria
  bool hasRoleRules = false;
  auto option = options.find("xsp");
  if (option != options.end())
  {
    CSmartPlaylist xsp;
    if (!xsp.LoadFromJson(option->second.asString()))
      return false;

    std::set<std::string> playlists;
    std::string xspWhere;
    xspWhere = xsp.GetWhereClause(*this, playlists);
    hasRoleRules = xsp.GetType() == "artists" && xspWhere.find("song_artist.idRole = role.idRole") != xspWhere.npos;

    // check if the filter playlist matches the item type
    if (xsp.GetType() == type ||
      (xsp.GetGroup() == type && !xsp.IsGroupMixed()))
    {
      filter.AppendWhere(xspWhere);

      if (xsp.GetLimit() > 0)
        sorting.limitEnd = xsp.GetLimit();
      if (xsp.GetOrder() != SortByNone)
        sorting.sortBy = xsp.GetOrder();
      sorting.sortOrder = xsp.GetOrderAscending() ? SortOrderAscending : SortOrderDescending;
      if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_FILELISTS_IGNORETHEWHENSORTING))
        sorting.sortAttributes = SortAttributeIgnoreArticle;
    }
  }

  //Process role options, common to artist and album type filtering
  int idRole = 1; // Default restrict song_artist to "artists" only, no other roles.
  option = options.find("roleid");
  if (option != options.end())
    idRole = static_cast<int>(option->second.asInteger());
  else
  {
    option = options.find("role");
    if (option != options.end())
    {
      if (option->second.asString() == "all" || option->second.asString() == "%")
        idRole = -1000; //All roles
      else
        idRole = GetRoleByName(option->second.asString());
    }
  }
  if (hasRoleRules)
  {
    // Get Role from role rule(s) here.
    // But that requires much change, so for now get all roles as better than none
    idRole = -1000; //All roles
  }

  std::string strRoleSQL; //Role < 0 means all roles, otherwise filter by role
  if(idRole > 0) strRoleSQL = PrepareSQL(" AND song_artist.idRole = %i ", idRole);

  int idArtist = -1, idGenre = -1, idAlbum = -1, idSong = -1;
  int idSource = -1;
  bool albumArtistsOnly = false;
  std::string artistname;

  // Process albumartistsonly option
  option = options.find("albumartistsonly");
  if (option != options.end())
    albumArtistsOnly = option->second.asBoolean();

  // Process genre option
  option = options.find("genreid");
  if (option != options.end())
    idGenre = static_cast<int>(option->second.asInteger());
  else
  {
    option = options.find("genre");
    if (option != options.end())
      idGenre = GetGenreByName(option->second.asString());
  }

  // Process source option
  option = options.find("sourceid");
  if (option != options.end())
    idSource = static_cast<int>(option->second.asInteger());
  else
  {
    option = options.find("source");
    if (option != options.end())
      idSource = GetSourceByName(option->second.asString());
  }

  // Process album option
  option = options.find("albumid");
  if (option != options.end())
    idAlbum = static_cast<int>(option->second.asInteger());
  else
  {
    option = options.find("album");
    if (option != options.end())
      idAlbum = GetAlbumByName(option->second.asString());
  }

  // Process artist option
  option = options.find("artistid");
  if (option != options.end())
    idArtist = static_cast<int>(option->second.asInteger());
  else
  {
    option = options.find("artist");
    if (option != options.end())
    {
      idArtist = GetArtistByName(option->second.asString());
      if (idArtist == -1)
      {// not found with that name, or more than one found as artist name is not unique
        artistname = option->second.asString();
      }
    }
  }

  //  Process song option
  option = options.find("songid");
  if (option != options.end())
    idSong = static_cast<int>(option->second.asInteger());

  if (type == "artists")
  {
    if (!hasRoleRules)
    { // Not an "artists" smart playlist with roles rules, so get filter from options
      if (idArtist > 0)
        filter.AppendWhere(PrepareSQL("artistview.idArtist = %d", idArtist));
      else if (idAlbum > 0)
        filter.AppendWhere(PrepareSQL("artistview.idArtist IN (SELECT album_artist.idArtist FROM album_artist "
          "WHERE album_artist.idAlbum = %i)", idAlbum));
      else if (idSong > 0)
      {
        filter.AppendWhere(PrepareSQL("artistview.idArtist IN (SELECT song_artist.idArtist FROM song_artist "
          "WHERE song_artist.idSong = %i %s)", idSong, strRoleSQL.c_str()));
      }
      else
      { /*
        Process idRole, idGenre, idSource and albumArtistsOnly options

        For artists these rules are combined because they apply via album and song
        and so we need to ensure all criteria are met via the same album or song.
        1) Some artists may be only album artists, so for all artists (with linked 
           albums or songs) we need to check both album_artist and song_artist tables.
        2) Role is determined from song_artist table, so even if looking for album artists
           only we find those that also have a specific role e.g. which album artist is a
           composer of songs in that album, from entries in the song_artist table.
        a) Role < -1 is used to indicate that all roles are wanted.
        b) When not album artists only and a specific role wanted then only the song_artist
           table is checked.
        c) When album artists only and role = 1 (an "artist") then only the album_artist
           table is checked.      
        */        
        std::string albumArtistSQL, songArtistSQL;
        ExistsSubQuery albumArtistSub("album_artist", "album_artist.idArtist = artistview.idArtist");
        // Prepare album artist subquery SQL
        if (idSource > 0)
        {
          if (idRole == 1 && idGenre < 0)
          {
            albumArtistSub.AppendJoin("JOIN album_source ON album_source.idAlbum = album_artist.idAlbum");
            albumArtistSub.AppendWhere(PrepareSQL("album_source.idSource = %i", idSource));
          }
          else
          {
            albumArtistSub.AppendWhere(PrepareSQL("EXISTS(SELECT 1 FROM album_source "
              "WHERE album_source.idSource = %i "
              "AND album_source.idAlbum = album_artist.idAlbum)", idSource));
          }       
        }
        if (idRole <= 1 && idGenre > 0)
        { // Check genre of songs of album using nested subquery
          std::string strGenre = PrepareSQL("EXISTS(SELECT 1 FROM song "
            "JOIN song_genre ON song_genre.idSong = song.idSong "
            "WHERE song.idAlbum = album_artist.idAlbum AND song_genre.idGenre = %i)", idGenre);
          albumArtistSub.AppendWhere(strGenre);
        }

        // Prepare song artist subquery SQL
        ExistsSubQuery songArtistSub("song_artist", "song_artist.idArtist = artistview.idArtist");
        if (idRole > 0)
          songArtistSub.AppendWhere(PrepareSQL("song_artist.idRole = %i", idRole));
        if (idSource > 0 && idGenre > 0 && !albumArtistsOnly && idRole >= 1)
        {
          songArtistSub.AppendWhere(PrepareSQL("EXISTS(SELECT 1 FROM song "
            "JOIN song_genre ON song_genre.idSong = song.idSong "
            "WHERE song.idSong = song_artist.idSong "
            "AND song_genre.idGenre = %i "
            "AND EXISTS(SELECT 1 FROM album_source "
            "WHERE album_source.idSource = %i "
            "AND album_source.idAlbum = song.idAlbum))", idGenre, idSource));
        }
        else 
        {
          if (idGenre > 0)
          {
            songArtistSub.AppendJoin("JOIN song_genre ON song_genre.idSong = song_artist.idSong");
            songArtistSub.AppendWhere(PrepareSQL("song_genre.idGenre = %i", idGenre));
          }
          if (idSource > 0 && !albumArtistsOnly)
          {
            songArtistSub.AppendJoin("JOIN song ON song.idSong = song_artist.idSong");
            songArtistSub.AppendJoin("JOIN album_source ON album_source.idAlbum = song.idAlbum");
            songArtistSub.AppendWhere(PrepareSQL("album_source.idSource = %i", idSource));
          }
          if (idRole > 1 && albumArtistsOnly)
          { // Album artists only with role, check AND in album_artist for album of song
            // using nested subquery correlated with album_artist
            songArtistSub.AppendJoin("JOIN song ON song.idSong = song_artist.idSong");
            songArtistSub.param = "song_artist.idArtist = album_artist.idArtist";
            songArtistSub.AppendWhere("song.idAlbum = album_artist.idAlbum");
          }         
        }

        // Build filter clause from subqueries
        if (idRole > 1 && albumArtistsOnly)
        { // Album artists only with role, check AND in album_artist for album of song
          // using nested subquery correlated with album_artist
          songArtistSub.BuildSQL(songArtistSQL);
          albumArtistSub.AppendWhere(songArtistSQL);
          albumArtistSub.BuildSQL(albumArtistSQL);
          filter.AppendWhere(albumArtistSQL);
        }
        else
        {
          songArtistSub.BuildSQL(songArtistSQL);
          albumArtistSub.BuildSQL(albumArtistSQL);
          if (idRole < 0 || (idRole == 1 && !albumArtistsOnly))
          { // Artist contributing to songs, any role, check OR album artist too
            // as artists can be just album artists but not song artists
            filter.AppendWhere(songArtistSQL + " OR " + albumArtistSQL);
          }
          else if (idRole > 1)
          {
            // Artist contributes that role (not albmartistsonly as already handled)
            filter.AppendWhere(songArtistSQL);
          }
          else // idRole = 1 and albumArtistsOnly
          { // Only look at album artists, not albums where artist features on songs
            filter.AppendWhere(albumArtistSQL);
          }
        }
      }
    }
    // remove the null string
    filter.AppendWhere("artistview.strArtist != ''");
  }
  else if (type == "albums")
  {
    option = options.find("year");
    if (option != options.end())
      filter.AppendWhere(PrepareSQL("albumview.iYear = %i", static_cast<int>(option->second.asInteger())));

    option = options.find("compilation");
    if (option != options.end())
      filter.AppendWhere(PrepareSQL("albumview.bCompilation = %i", option->second.asBoolean() ? 1 : 0));

    if (idSource > 0)
      filter.AppendWhere(PrepareSQL("EXISTS(SELECT 1 FROM album_source "
        "WHERE album_source.idAlbum = albumview.idAlbum AND album_source.idSource = %i)", idSource));

    // Process artist, role and genre options together as song subquery to filter those
    // albums that have songs with both that artist and genre
    std::string albumArtistSQL, songArtistSQL, genreSQL;
    ExistsSubQuery genreSub("song", "song.idAlbum = album_artist.idAlbum");
    genreSub.AppendJoin("JOIN song_genre ON song_genre.idSong = song.idSong");
    genreSub.AppendWhere(PrepareSQL("song_genre.idGenre = %i", idGenre));
    ExistsSubQuery albumArtistSub("album_artist", "album_artist.idAlbum = albumview.idAlbum");
    ExistsSubQuery songArtistSub("song_artist", "song.idAlbum = albumview.idAlbum");
    songArtistSub.AppendJoin("JOIN song ON song.idSong = song_artist.idSong");

    if (idArtist > 0)
    {
      songArtistSub.AppendWhere(PrepareSQL("song_artist.idArtist = %i", idArtist));
      albumArtistSub.AppendWhere(PrepareSQL("album_artist.idArtist = %i", idArtist));
    }
    else if (!artistname.empty())
    { // Artist name is not unique, so could get albums or songs from more than one.
      songArtistSub.AppendJoin("JOIN artist ON artist.idArtist = song_artist.idArtist");
      songArtistSub.AppendWhere(PrepareSQL("artist.strArtist like '%s'", artistname.c_str()));

      albumArtistSub.AppendJoin("JOIN artist ON artist.idArtist = song_artist.idArtist");
      albumArtistSub.AppendWhere(PrepareSQL("artist.strArtist like '%s'", artistname.c_str()));
    }
    if (idRole > 0)
      songArtistSub.AppendWhere(PrepareSQL("song_artist.idRole = %i", idRole));
    if (idGenre > 0)
    {
      songArtistSub.AppendJoin("JOIN song_genre ON song_genre.idSong = song.idSong");
      songArtistSub.AppendWhere(PrepareSQL("song_genre.idGenre = %i", idGenre));
    }

    if (idArtist > 0 || !artistname.empty())
    {
      if (idRole <= 1 && idGenre > 0)
      { // Check genre of songs of album using nested subquery
        genreSub.BuildSQL(genreSQL);
        albumArtistSub.AppendWhere(genreSQL);
      }
      if (idRole > 1 && albumArtistsOnly)
      {  // Album artists only with role, check AND in album_artist for same song
         // using nested subquery correlated with album_artist
         songArtistSub.param = "song.idAlbum = album_artist.idAlbum";
         songArtistSub.BuildSQL(songArtistSQL);
         albumArtistSub.AppendWhere(songArtistSQL);
         albumArtistSub.BuildSQL(albumArtistSQL);
         filter.AppendWhere(albumArtistSQL);
      }
      else
      {
        songArtistSub.BuildSQL(songArtistSQL);
        albumArtistSub.BuildSQL(albumArtistSQL);
        if (idRole < 0 || (idRole == 1 && !albumArtistsOnly))
        { // Artist contributing to songs, any role, check OR album artist too
          // as artists can be just album artists but not song artists
          filter.AppendWhere(songArtistSQL + " OR " + albumArtistSQL);
        }
        else if (idRole > 1)
        { // Albums with songs where artist contributes that role (not albmartistsonly as already handled)
          filter.AppendWhere(songArtistSQL);
        }
        else // idRole = 1 and albumArtistsOnly
        { // Only look at album artists, not albums where artist features on songs
          // This may want to be a separate option so you can choose to see all the albums where that artist
          // appears on one or more songs without having to list all song artists in the artists node.
          filter.AppendWhere(albumArtistSQL);
        }
      }
    }
    else
    { // No artist given
      if (idGenre > 0)
      { // Have genre option but not artist
        genreSub.param = "song.idAlbum = albumview.idAlbum";
        genreSub.BuildSQL(genreSQL);
        filter.AppendWhere(genreSQL);
      }
      // Exclude any single albums (aka empty tagged albums)
      // This causes "albums"  media filter artist selection to only offer album artists
      option = options.find("show_singles");
      if (option == options.end() || !option->second.asBoolean())
        filter.AppendWhere(PrepareSQL("albumview.strReleaseType = '%s'", CAlbum::ReleaseTypeToString(CAlbum::Album).c_str()));
    }
  }
  else if (type == "songs" || type == "singles")
  {
    option = options.find("singles");
    if (option != options.end())
      filter.AppendWhere(PrepareSQL("songview.idAlbum %sIN (SELECT idAlbum FROM album WHERE strReleaseType = '%s')",
                                    option->second.asBoolean() ? "" : "NOT ",
                                    CAlbum::ReleaseTypeToString(CAlbum::Single).c_str()));

    option = options.find("year");
    if (option != options.end())
      filter.AppendWhere(PrepareSQL("songview.iYear = %i", static_cast<int>(option->second.asInteger())));

    option = options.find("compilation");
    if (option != options.end())
      filter.AppendWhere(PrepareSQL("songview.bCompilation = %i", option->second.asBoolean() ? 1 : 0));

    if (idSong > 0)
      filter.AppendWhere(PrepareSQL("songview.idSong = %i", idSong));

    if (idAlbum > 0)
      filter.AppendWhere(PrepareSQL("songview.idAlbum = %i", idAlbum));

    if (idGenre > 0)
      filter.AppendWhere(PrepareSQL("songview.idSong IN (SELECT song_genre.idSong FROM song_genre WHERE song_genre.idGenre = %i)", idGenre));

    if (idSource > 0)
      filter.AppendWhere(PrepareSQL("EXISTS(SELECT 1 FROM album_source "
        "WHERE album_source.idAlbum = songview.idAlbum AND album_source.idSource = %i)", idSource));

    std::string songArtistClause, albumArtistClause;
    if (idArtist > 0)
    {
      songArtistClause = PrepareSQL("EXISTS (SELECT 1 FROM song_artist "
        "WHERE song_artist.idSong = songview.idSong AND song_artist.idArtist = %i %s)",
        idArtist, strRoleSQL.c_str());
      albumArtistClause = PrepareSQL("EXISTS (SELECT 1 FROM album_artist "
        "WHERE album_artist.idAlbum = songview.idAlbum AND album_artist.idArtist = %i)",
        idArtist);
    }
    else if (!artistname.empty())
    {  // Artist name is not unique, so could get songs from more than one.
      songArtistClause = PrepareSQL("EXISTS (SELECT 1 FROM song_artist JOIN artist ON artist.idArtist = song_artist.idArtist "
        "WHERE song_artist.idSong = songview.idSong AND artist.strArtist like '%s' %s)",
        artistname.c_str(), strRoleSQL.c_str());
      albumArtistClause = PrepareSQL("EXISTS (SELECT 1 FROM album_artist JOIN artist ON artist.idArtist = album_artist.idArtist "
        "WHERE album_artist.idAlbum = songview.idAlbum AND artist.strArtist like '%s')",
        artistname.c_str());
    }

    // Process artist name or id option
    if (!songArtistClause.empty())
    {
      if (idRole < 0) // Artist contributes to songs, any roles OR is album artist
        filter.AppendWhere("(" + songArtistClause + " OR " + albumArtistClause + ")");
      else if (idRole > 1)
      {
        if (albumArtistsOnly)  //Album artists only with role, check AND in album_artist for same song
          filter.AppendWhere("(" + songArtistClause + " AND " + albumArtistClause + ")");
        else // songs where artist contributes that role.
          filter.AppendWhere(songArtistClause);
      }
      else
      {
        if (albumArtistsOnly) // Only look at album artists, not where artist features on songs
          filter.AppendWhere(albumArtistClause);
        else // Artist is song artist or album artist
          filter.AppendWhere("(" + songArtistClause + " OR " + albumArtistClause + ")");
      }
    }
  }

  option = options.find("filter");
  if (option != options.end())
  {
    CSmartPlaylist xspFilter;
    if (!xspFilter.LoadFromJson(option->second.asString()))
      return false;

    // check if the filter playlist matches the item type
    if (xspFilter.GetType() == type)
    {
      std::set<std::string> playlists;
      filter.AppendWhere(xspFilter.GetWhereClause(*this, playlists));
    }
    // remove the filter if it doesn't match the item type
    else
      musicUrl.RemoveOption("filter");
  }

  return true;
}

void CMusicDatabase::UpdateFileDateAdded(int songId, const std::string& strFileNameAndPath)
{
  if (songId < 0 || strFileNameAndPath.empty())
    return;

  CDateTime dateAdded;
  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    // 1 preferring to use the files mtime(if it's valid) and only using the file's ctime if the mtime isn't valid
    if (CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_iMusicLibraryDateAdded == 1)
      dateAdded = CFileUtils::GetModificationDate(strFileNameAndPath, false);
    //2 using the newer datetime of the file's mtime and ctime
    else if (CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_iMusicLibraryDateAdded == 2)
      dateAdded = CFileUtils::GetModificationDate(strFileNameAndPath, true);
    //0 using the current datetime if non of the above matches or one returns an invalid datetime
    if (!dateAdded.IsValid())
      dateAdded = CDateTime::GetCurrentDateTime();

    m_pDS->exec(PrepareSQL("UPDATE song SET dateAdded='%s' WHERE idSong=%d", dateAdded.GetAsDBDateTime().c_str(), songId));
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s, %s) failed", __FUNCTION__, CURL::GetRedacted(strFileNameAndPath).c_str(), dateAdded.GetAsDBDateTime().c_str());
  }
}

bool CMusicDatabase::AddAudioBook(const CFileItem& item)
{
  auto const& artists = item.GetMusicInfoTag()->GetArtist();
  std::string strSQL = PrepareSQL(
    "INSERT INTO audiobook (idBook,strBook,strAuthor,bookmark,file,dateAdded) "
    "VALUES (NULL,'%s','%s',%i,'%s','%s')",
    item.GetMusicInfoTag()->GetAlbum().c_str(),
    artists.empty() ? "" : artists[0].c_str(),
    0,
    item.GetDynPath().c_str(),
    CDateTime::GetCurrentDateTime().GetAsDBDateTime().c_str()
  );
  return ExecuteQuery(strSQL);
}

bool CMusicDatabase::SetResumeBookmarkForAudioBook(const CFileItem& item, int bookmark)
{
  std::string strSQL = PrepareSQL("select bookmark from audiobook where file='%s'",
                                 item.GetDynPath().c_str());
  if (!m_pDS->query(strSQL.c_str()) || m_pDS->num_rows() == 0)
  {
    if (!AddAudioBook(item))
      return false;
  }

  strSQL = PrepareSQL("UPDATE audiobook SET bookmark=%i WHERE file='%s'",
                      bookmark, item.GetDynPath().c_str());

  return ExecuteQuery(strSQL);
}

bool CMusicDatabase::GetResumeBookmarkForAudioBook(const CFileItem& item, int& bookmark)
{
  std::string strSQL = PrepareSQL("SELECT bookmark FROM audiobook WHERE file='%s'",
                                 item.GetDynPath().c_str());
  if (!m_pDS->query(strSQL.c_str()) || m_pDS->num_rows() == 0)
    return false;

  bookmark = m_pDS->fv(0).get_asInt();
  return true;
}
