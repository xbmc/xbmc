/*
 *      Copyright (C) 2005-2017 Team Kodi
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
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
#include "CueInfoLoader.h"
#include "dbwrappers/dataset.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "dialogs/GUIDialogOK.h"
#include "dialogs/GUIDialogProgress.h"
#include "dialogs/GUIDialogSelect.h"
#include "FileItem.h"
#include "filesystem/DirectoryCache.h"
#include "filesystem/File.h"
#include "filesystem/MusicDatabaseDirectory/DirectoryNode.h"
#include "filesystem/MusicDatabaseDirectory/QueryParams.h"
#include "guiinfo/GUIInfoLabels.h"
#include "GUIInfoManager.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "interfaces/AnnouncementManager.h"
#include "messaging/helpers/DialogHelper.h"
#include "music/tags/MusicInfoTag.h"
#include "network/cddb.h"
#include "network/Network.h"
#include "playlists/SmartPlayList.h"
#include "profiles/ProfilesManager.h"
#include "settings/AdvancedSettings.h"
#include "settings/MediaSettings.h"
#include "settings/Settings.h"
#include "Song.h"
#include "storage/MediaManager.h"
#include "system.h"
#include "TextureCache.h"
#include "threads/SystemClock.h"
#include "URL.h"
#include "utils/FileUtils.h"
#include "utils/LegacyPathTranslation.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "TextureCache.h"
#include "interfaces/AnnouncementManager.h"
#include "dbwrappers/dataset.h"
#include "utils/XMLUtils.h"

#include <odb/odb_gen/ODBCue.h>
#include <odb/odb_gen/ODBCue_odb.h>
#include <odb/odb_gen/ODBAlbum.h>
#include <odb/odb_gen/ODBAlbum_odb.h>
#include <odb/odb_gen/ODBArtistDetail.h>
#include <odb/odb_gen/ODBArtistDetail_odb.h>
#include <odb/odb_gen/ODBArtistDiscography.h>
#include <odb/odb_gen/ODBArtistDiscography_odb.h>
#include <odb/odb_gen/ODBVersionTagScan.h>
#include <odb/odb_gen/ODBVersionTagScan_odb.h>

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
  ANNOUNCEMENT::CAnnouncementManager::GetInstance().Announce(ANNOUNCEMENT::AudioLibrary, "xbmc", "OnRemove", data);
}

static void AnnounceUpdate(const std::string& content, int id)
{
  CVariant data;
  data["type"] = content;
  data["id"] = id;
  if (g_application.IsMusicScanning())
    data["transaction"] = true;
  ANNOUNCEMENT::CAnnouncementManager::GetInstance().Announce(ANNOUNCEMENT::AudioLibrary, "xbmc", "OnUpdate", data);
}

CMusicDatabase::CMusicDatabase(void) : m_cdb(CCommonDatabase::GetInstance())
{
  m_translateBlankArtist = true;
}

CMusicDatabase::~CMusicDatabase(void)
{
  EmptyCache();
}

bool CMusicDatabase::Open()
{
  return CDatabase::Open(g_advancedSettings.m_databaseMusic);
}

void CMusicDatabase::CreateTables()
{
  CLog::Log(LOGINFO, "create artist table");
  m_pDS->exec("CREATE TABLE artist ( idArtist integer primary key, "
              " strArtist varchar(256), strMusicBrainzArtistID text, "
              " strBorn text, strFormed text, strGenres text, strMoods text, "
              " strStyles text, strInstruments text, strBiography text, "
              " strDied text, strDisbanded text, strYearsActive text, "
              " strImage text, strFanart text, "
              " lastScraped varchar(20) default NULL)");
  // Create missing artist tag artist [Missing].
  std::string strSQL = PrepareSQL("INSERT INTO artist (idArtist, strArtist, strMusicBrainzArtistID) VALUES( %i, '%s', '%s' )",
                                  BLANKARTIST_ID, BLANKARTIST_NAME.c_str(), BLANKARTIST_FAKEMUSICBRAINZID.c_str());
  m_pDS->exec(strSQL);

  CLog::Log(LOGINFO, "create album table");
  m_pDS->exec("CREATE TABLE album (idAlbum integer primary key, "
              " strAlbum varchar(256), strMusicBrainzAlbumID text, "
              " strArtists text, strGenres text, "
              " iYear integer, idThumb integer, "
              " bCompilation integer not null default '0', "
              " strMoods text, strStyles text, strThemes text, "
              " strReview text, strImage text, strLabel text, "
              " strType text, "
              " fRating FLOAT NOT NULL DEFAULT 0, "
              " iUserrating INTEGER NOT NULL DEFAULT 0, "
              " lastScraped varchar(20) default NULL, "
              " strReleaseType text, "
              " iVotes INTEGER NOT NULL DEFAULT 0)");
  CLog::Log(LOGINFO, "create audiobook table");
  m_pDS->exec("CREATE TABLE audiobook (idBook integer primary key, "
              " strBook varchar(256), strAuthor text,"
              " bookmark integer, file text,"
              " dateAdded varchar (20) default NULL)");
  CLog::Log(LOGINFO, "create album_artist table");
  m_pDS->exec("CREATE TABLE album_artist (idArtist integer, idAlbum integer, iOrder integer, strArtist text)");
  CLog::Log(LOGINFO, "create album_genre table");
  m_pDS->exec("CREATE TABLE album_genre (idGenre integer, idAlbum integer, iOrder integer)");

  CLog::Log(LOGINFO, "create genre table");
  m_pDS->exec("CREATE TABLE genre (idGenre integer primary key, strGenre varchar(256))");
  CLog::Log(LOGINFO, "create path table");
  m_pDS->exec("CREATE TABLE path (idPath integer primary key, strPath varchar(512), strHash text)");
  CLog::Log(LOGINFO, "create song table");
  m_pDS->exec("CREATE TABLE song (idSong integer primary key, "
              " idAlbum integer, idPath integer, "
              " strArtists text, strGenres text, strTitle varchar(512), "
              " iTrack integer, iDuration integer, iYear integer, "
              " dwFileNameCRC text, "
              " strFileName text, strMusicBrainzTrackID text, "
              " iTimesPlayed integer, iStartOffset integer, iEndOffset integer, "
              " idThumb integer, "
              " lastplayed varchar(20) default NULL, "
              " rating FLOAT NOT NULL DEFAULT 0, userrating INTEGER NOT NULL DEFAULT 0, "
              " comment text, mood text, dateAdded text, votes INTEGER NOT NULL DEFAULT 0)");
  CLog::Log(LOGINFO, "create song_artist table");
  m_pDS->exec("CREATE TABLE song_artist (idArtist integer, idSong integer, idRole integer, iOrder integer, strArtist text)");
  CLog::Log(LOGINFO, "create song_genre table");
  m_pDS->exec("CREATE TABLE song_genre (idGenre integer, idSong integer, iOrder integer)");

  CLog::Log(LOGINFO, "create role table");
  m_pDS->exec("CREATE TABLE role (idRole integer primary key, strRole text)");
  m_pDS->exec("INSERT INTO role(idRole, strRole) VALUES (1, 'Artist')");   //Default role

  CLog::Log(LOGINFO, "create albuminfosong table");
  m_pDS->exec("CREATE TABLE albuminfosong (idAlbumInfoSong integer primary key, idAlbumInfo integer, iTrack integer, strTitle text, iDuration integer)");

  CLog::Log(LOGINFO, "create content table");
  m_pDS->exec("CREATE TABLE content (strPath text, strScraperPath text, strContent text, strSettings text)");
  CLog::Log(LOGINFO, "create discography table");
  m_pDS->exec("CREATE TABLE discography (idArtist integer, strAlbum text, strYear text)");

  CLog::Log(LOGINFO, "create art table");
  m_pDS->exec("CREATE TABLE art(art_id INTEGER PRIMARY KEY, media_id INTEGER, media_type TEXT, type TEXT, url TEXT)");

  CLog::Log(LOGINFO, "create cue table");
  m_pDS->exec("CREATE TABLE cue (idPath integer, strFileName text, strCuesheet text)");

  CLog::Log(LOGINFO, "create versiontagscan table");
  m_pDS->exec("CREATE TABLE versiontagscan (idVersion integer, iNeedsScan integer)");
  m_pDS->exec(PrepareSQL("INSERT INTO versiontagscan (idVersion, iNeedsScan) values(%i, 0)", GetSchemaVersion()));
}

void CMusicDatabase::CreateAnalytics()
{
  CLog::Log(LOGINFO, "%s - creating indices", __FUNCTION__);
  m_pDS->exec("CREATE INDEX idxAlbum ON album(strAlbum(255))");
  m_pDS->exec("CREATE INDEX idxAlbum_1 ON album(bCompilation)");
  m_pDS->exec("CREATE UNIQUE INDEX idxAlbum_2 ON album(strMusicBrainzAlbumID(36))");

  m_pDS->exec("CREATE UNIQUE INDEX idxAlbumArtist_1 ON album_artist ( idAlbum, idArtist )");
  m_pDS->exec("CREATE UNIQUE INDEX idxAlbumArtist_2 ON album_artist ( idArtist, idAlbum )");

  m_pDS->exec("CREATE UNIQUE INDEX idxAlbumGenre_1 ON album_genre ( idAlbum, idGenre )");
  m_pDS->exec("CREATE UNIQUE INDEX idxAlbumGenre_2 ON album_genre ( idGenre, idAlbum )");

  m_pDS->exec("CREATE INDEX idxGenre ON genre(strGenre(255))");

  m_pDS->exec("CREATE INDEX idxArtist ON artist(strArtist(255))");
  m_pDS->exec("CREATE UNIQUE INDEX idxArtist1 ON artist(strMusicBrainzArtistID(36))");

  m_pDS->exec("CREATE INDEX idxPath ON path(strPath(255))");

  m_pDS->exec("CREATE INDEX idxSong ON song(strTitle(255))");
  m_pDS->exec("CREATE INDEX idxSong1 ON song(iTimesPlayed)");
  m_pDS->exec("CREATE INDEX idxSong2 ON song(lastplayed)");
  m_pDS->exec("CREATE INDEX idxSong3 ON song(idAlbum)");
  m_pDS->exec("CREATE INDEX idxSong6 ON song( idPath, strFileName(255) )");
  m_pDS->exec("CREATE UNIQUE INDEX idxSong7 ON song( idAlbum, strMusicBrainzTrackID(36) )");

  m_pDS->exec("CREATE UNIQUE INDEX idxSongArtist_1 ON song_artist ( idSong, idArtist, idRole )");
  m_pDS->exec("CREATE INDEX idxSongArtist_2 ON song_artist ( idSong, idRole )");
  m_pDS->exec("CREATE INDEX idxSongArtist_3 ON song_artist ( idArtist, idRole )");
  m_pDS->exec("CREATE INDEX idxSongArtist_4 ON song_artist ( idRole )");

  m_pDS->exec("CREATE UNIQUE INDEX idxSongGenre_1 ON song_genre ( idSong, idGenre )");
  m_pDS->exec("CREATE UNIQUE INDEX idxSongGenre_2 ON song_genre ( idGenre, idSong )");

  m_pDS->exec("CREATE INDEX idxRole on role(strRole(255))");

  m_pDS->exec("CREATE INDEX idxAlbumInfoSong_1 ON albuminfosong ( idAlbumInfo )");

  m_pDS->exec("CREATE INDEX idxDiscography_1 ON discography ( idArtist )");

  m_pDS->exec("CREATE INDEX ix_art ON art(media_id, media_type(20), type(20))");

  m_pDS->exec("CREATE UNIQUE INDEX idxCue ON cue(idPath, strFileName(255))");

  CLog::Log(LOGINFO, "create triggers");
  m_pDS->exec("CREATE TRIGGER tgrDeleteAlbum AFTER delete ON album FOR EACH ROW BEGIN"
              "  DELETE FROM song WHERE song.idAlbum = old.idAlbum;"
              "  DELETE FROM album_artist WHERE album_artist.idAlbum = old.idAlbum;"
              "  DELETE FROM album_genre WHERE album_genre.idAlbum = old.idAlbum;"
              "  DELETE FROM albuminfosong WHERE albuminfosong.idAlbumInfo=old.idAlbum;"
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
  m_pDS->exec("CREATE TRIGGER tgrDeletePath AFTER delete ON path FOR EACH ROW BEGIN"
              "  DELETE FROM cue WHERE cue.idPath = old.idPath;"
              " END");

  // we create views last to ensure all indexes are rolled in
  CreateViews();
}

void CMusicDatabase::CreateViews()
{
  CLog::Log(LOGINFO, "create song view");
  m_pDS->exec("CREATE VIEW songview AS SELECT "
              "        song.idSong AS idSong, "
              "        song.strArtists AS strArtists,"
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
              "        album.strArtists AS strAlbumArtists,"
              "        album.strReleaseType AS strAlbumReleaseType,"
              "        song.mood as mood,"
              "        song.dateAdded as dateAdded "
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
              "        album.strArtists AS strArtists, "
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
              "        (SELECT AVG(song.iTimesPlayed) FROM song WHERE song.idAlbum = album.idAlbum) AS iTimesPlayed, "
              "        strReleaseType, "
              "        (SELECT MAX(song.dateAdded) FROM song WHERE song.idAlbum = album.idAlbum) AS dateAdded, "
              "        (SELECT MAX(song.lastplayed) FROM song WHERE song.idAlbum = album.idAlbum) AS lastplayed "
              "FROM album"
              );

  CLog::Log(LOGINFO, "create artist view");
  m_pDS->exec("CREATE VIEW artistview AS SELECT"
              "  idArtist, strArtist, "
              "  strMusicBrainzArtistID, "
              "  strBorn, strFormed, strGenres,"
              "  strMoods, strStyles, strInstruments, "
              "  strBiography, strDied, strDisbanded, "
              "  strYearsActive, strImage, strFanart, "
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
              "  artist.strMusicBrainzArtistID AS strMusicBrainzArtistID, "
              "  song_artist.iOrder AS iOrder "
              "FROM song_artist "
              "JOIN artist ON "
              "     song_artist.idArtist = artist.idArtist "
              "JOIN role ON "
              "     song_artist.idRole = role.idRole");
}

void CMusicDatabase::SaveCuesheet(const std::string& fullSongPath, const std::string& strCuesheet)
{
  std::string strPath, strFileName;
  URIUtils::Split(fullSongPath, strPath, strFileName);
  
  std::shared_ptr<CODBFile> objFile(AddFileAndPath(strFileName, strPath));
  if (!objFile || !objFile->m_path.load())
    return;
  
  std::string strSQL;
  try
  {
    auto it = m_cueCache.find(fullSongPath);
    if (it != m_cueCache.end() && it->second == strCuesheet)
      return;

    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    
    ODBView_Cue_Path objCueView;
    if (m_cdb.getDB()->query_one<ODBView_Cue_Path>(odb::query<ODBView_Cue_Path>::CODBPath::idPath == objFile->m_path->m_idPath && odb::query<ODBView_Cue_Path>::CODBFile::filename == strFileName, objCueView))
    {
      if (strCuesheet.empty())
      {
        m_cdb.getDB()->erase(objCueView.cue);
      }
      else
      {
        objCueView.cue->m_cuesheet = strCuesheet;
        m_cdb.getDB()->update(objCueView.cue);
      }
    }
    else
    {
      if (strCuesheet.empty())
      {
        m_pDS->close();
        m_cueCache.insert(CueCache::value_type(fullSongPath, strCuesheet));
        return;
      }
      
      CODBCue objCue;
      objCue.m_cuesheet = strCuesheet;
      objCue.m_file = objFile;
      
      m_cdb.getDB()->persist(objCue);
    }
    
    if(odb_transaction)
      odb_transaction->commit();
    
    m_cueCache.insert(CueCache::value_type(fullSongPath, strCuesheet));
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "musicdatabase:exception to addcue (%s) - %s", strSQL.c_str(), e.what());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "musicdatabase:unable to addcue (%s)", strSQL.c_str());
  }
}

std::string CMusicDatabase::LoadCuesheet(const std::string& fullSongPath)
{
  auto it = m_cueCache.find(fullSongPath);
  if (it != m_cueCache.end())
    return it->second;

  std::string strCuesheet;

  std::string strPath, strFileName;
  URIUtils::Split(fullSongPath, strPath, strFileName);
  
  std::shared_ptr<CODBFile> objFile(AddFileAndPath(strFileName, strPath));
  if (!objFile || !objFile->m_path.load())
    return strCuesheet;

  std::string strSQL;
  try
  {
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    
    ODBView_Cue_Path objCueView;
    if (m_cdb.getDB()->query_one<ODBView_Cue_Path>(odb::query<ODBView_Cue_Path>::CODBPath::idPath == objFile->m_path->m_idPath && odb::query<ODBView_Cue_Path>::CODBFile::filename == strFileName, objCueView))
    {
      strCuesheet = objCueView.cue->m_cuesheet;
    }
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "musicdatabase:exception to loadcue (%s) - %s", strSQL.c_str(), e.what());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "musicdatabase:unable to loadcue (%s)", strSQL.c_str());
  }
  return strCuesheet;
}

bool CMusicDatabase::AddAlbum(CAlbum& album)
{
  try
  {
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    
    std::shared_ptr<CODBAlbum> objAlbum = AddAlbum(album.strAlbum,
                                                   album.strMusicBrainzAlbumID,
                                                   album.GetAlbumArtistString(),
                                                   album.GetGenreString(),
                                                   album.iYear,
                                                   album.strLabel, album.strType,
                                                   album.bCompilation, album.releaseType);
    if (!objAlbum)
      return false;
    
    album.idAlbum = objAlbum->m_idAlbum;
    
    if (!objAlbum->section_foreign.loaded())
      m_cdb.getDB()->load(*objAlbum, objAlbum->section_foreign);

    // Add the album artists
    if (album.artistCredits.empty())
    {
      std::shared_ptr<CODBPerson> objArtist = AddArtist(BLANKARTIST_NAME, "");
      AddAlbumArtist(objArtist, objAlbum, 0);
    }
    for (auto artistCredit = album.artistCredits.begin(); artistCredit != album.artistCredits.end(); ++artistCredit)
    {
      std::shared_ptr<CODBPerson> objArtist = AddArtist(artistCredit->GetArtist(), artistCredit->GetMusicBrainzArtistID());
      artistCredit->idArtist = objArtist->m_idPerson;
      AddAlbumArtist(objArtist,
                     objAlbum,
                     std::distance(album.artistCredits.begin(), artistCredit));
    }

    for (auto song = album.songs.begin(); song != album.songs.end(); ++song)
    {
      song->idAlbum = album.idAlbum;
      std::shared_ptr<CODBSong> objSong(AddSong(objAlbum,
                                                song->strTitle, song->strMusicBrainzTrackID,
                                                song->strFileName, song->strComment,
                                                song->strMood, song->strThumb,
                                                song->GetArtistString(), song->genre,
                                                song->iTrack, song->iDuration, song->iYear,
                                                song->iTimesPlayed, song->iStartOffset,
                                                song->iEndOffset,
                                                song->lastPlayed,
                                                song->rating,
                                                song->userrating,
                                                song->votes));
      
      if (!objSong)
        return false;

      song->idSong = objSong->m_idSong;

      if (song->artistCredits.empty())
      {
        std::shared_ptr<CODBPerson> objArtist = AddArtist(BLANKARTIST_NAME, "");
        AddSongArtist(objArtist, objSong, 0); // Song must have at least one artist so set artist to [Missing]
      }
      
      for (auto artistCredit = song->artistCredits.begin(); artistCredit != song->artistCredits.end(); ++artistCredit)
      {
        std::shared_ptr<CODBPerson> objArtist = AddArtist(artistCredit->GetArtist(),
                                                          artistCredit->GetMusicBrainzArtistID());
        artistCredit->idArtist = objArtist->m_idPerson;
        
        AddSongArtist(objArtist,
                      objSong,
                      std::distance(song->artistCredits.begin(), artistCredit));
      }
      // Having added artist credits (maybe with MBID) add the other contributing artists (no MBID)
      AddSongContributors(objSong, song->GetContributors());

      SaveCuesheet(song->strFileName, song->strCueSheet);
    }

    for (const auto &albumArt : album.art)
      SetArtForItem(album.idAlbum, MediaTypeAlbum, albumArt.first, albumArt.second);

    if(odb_transaction)
      odb_transaction->commit();
    
    return true;
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "musicdatabase:exception on AddAlbum - %s", e.what());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "musicdatabase:unable to AddAlbum");
  }
  return false;
}

bool CMusicDatabase::UpdateAlbum(CAlbum& album, bool OverrideTagData /* = true*/)
{
  try
  {
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    
    std::shared_ptr<CODBAlbum> objAlbum = UpdateAlbum(album.idAlbum,
                                                      album.strAlbum, album.strMusicBrainzAlbumID,
                                                      album.GetAlbumArtistString(), album.GetGenreString(),
                                                      StringUtils::Join(album.moods, g_advancedSettings.m_musicItemSeparator).c_str(),
                                                      StringUtils::Join(album.styles, g_advancedSettings.m_musicItemSeparator).c_str(),
                                                      StringUtils::Join(album.themes, g_advancedSettings.m_musicItemSeparator).c_str(),
                                                      album.strReview,
                                                      album.thumbURL.m_xml.c_str(),
                                                      album.strLabel, album.strType,
                                                      album.fRating, album.iUserrating, album.iVotes, album.iYear, album.bCompilation, album.releaseType);

    if (OverrideTagData)
    {
      // Replace the album artists
      objAlbum->m_artists.clear();
      if (album.artistCredits.empty())
      {
        std::shared_ptr<CODBPerson> objArtist = AddArtist(BLANKARTIST_NAME, "");
        AddAlbumArtist(objArtist, objAlbum, 0); // Album must have at least one artist so set artist to [Missing]
      }
      
      for (auto artistCredit = album.artistCredits.begin(); artistCredit != album.artistCredits.end(); ++artistCredit)
      {
        std::shared_ptr<CODBPerson> objArtist = AddArtist(artistCredit->GetArtist(), artistCredit->GetMusicBrainzArtistID());
        artistCredit->idArtist = objArtist->m_idPerson;
        AddAlbumArtist(objArtist,
                       objAlbum,
                       std::distance(album.artistCredits.begin(), artistCredit));
      }

      for (auto &song : album.songs)
      {
        std::shared_ptr<CODBSong> objSong(new CODBSong);
        if (!m_cdb.getDB()->query_one<CODBSong>(odb::query<CODBSong>::idSong == song.idSong, *objSong))
          continue;
        
        UpdateSong(objSong,
                   song.strTitle,
                   song.strMusicBrainzTrackID,
                   song.strFileName,
                   song.strComment,
                   song.strMood,
                   song.strThumb,
                   song.GetArtistString(),
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
                   song.votes);
        //Replace song artists and contributors
        DeleteSongArtistsBySong(objSong);
        
        if (song.artistCredits.empty())
        {
          std::shared_ptr<CODBPerson> objArtist = AddArtist(BLANKARTIST_NAME, "");
          AddSongArtist(objArtist, objSong, 0); // Song must have at least one artist so set artist to [Missing]
        }
        
        for (auto artistCredit = song.artistCredits.begin(); artistCredit != song.artistCredits.end(); ++artistCredit)
        {
          std::shared_ptr<CODBPerson> objArtist = AddArtist(artistCredit->GetArtist(),
                                                            artistCredit->GetMusicBrainzArtistID());
          artistCredit->idArtist = objArtist->m_idPerson;
          
          AddSongArtist(objArtist,
                        objSong,
                        std::distance(song.artistCredits.begin(), artistCredit));
        }
        // Having added artist credits (maybe with MBID) add the other contributing artists (MBID unknown)
        AddSongContributors(objSong, song.GetContributors());

        SaveCuesheet(song.strFileName, song.strCueSheet);
      }
    }

    if (!album.art.empty())
      SetArtForItem(album.idAlbum, MediaTypeAlbum, album.art);

    if(odb_transaction)
      odb_transaction->commit();
    
    return true;
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "musicdatabase:exception on UpdateAlbum - %s", e.what());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "musicdatabase:unable to UpdateAlbum");
  }
  return false;
}

std::shared_ptr<CODBSong> CMusicDatabase::AddSong(const std::shared_ptr<CODBAlbum> objAlbum,
                            const std::string& strTitle, const std::string& strMusicBrainzTrackID,
                            const std::string& strPathAndFileName, const std::string& strComment,
                            const std::string& strMood, const std::string& strThumb,
                            const std::string &artistString, const std::vector<std::string>& genres,
                            int iTrack, int iDuration, int iYear,
                            const int iTimesPlayed, int iStartOffset, int iEndOffset,
                            const CDateTime& dtLastPlayed, float rating, int userrating, int votes)
{
  try
  {
    if (!objAlbum)
      return nullptr;
    
    // We need at least the title
    if (strTitle.empty())
      return nullptr;

    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    typedef odb::query<CODBSong> query;

    std::string strPath, strFileName;
    URIUtils::Split(strPathAndFileName, strPath, strFileName);
    
    query songQuery;

    if (!strMusicBrainzTrackID.empty())
      songQuery = query::album->idAlbum == objAlbum->m_idAlbum && query::musicBrainzTrackID == strMusicBrainzTrackID;
    else
      songQuery = query::album->idAlbum == objAlbum->m_idAlbum
              && query::file->filename == strFileName
              && query::title == strTitle
              && query::track == iTrack
              && query::musicBrainzTrackID == "";

    std::shared_ptr<CODBSong> objSong(new CODBSong);
    if (!m_cdb.getDB()->query_one<CODBSong>(songQuery, *objSong))
    {
      objSong->m_album = objAlbum;
      
      std::shared_ptr<CODBFile> objFile(AddFileAndPath(strFileName, strPath));
      if (objFile == nullptr)
        return nullptr;
      
      objFile->m_playCount = iTimesPlayed;
      if (dtLastPlayed.IsValid())
      {
        objFile->m_lastPlayed.setDateTime(dtLastPlayed.GetAsULongLong(), dtLastPlayed.GetAsDBDateTime());
      }
      m_cdb.getDB()->update(objFile);
      
      objSong->m_file = objFile;
      
      objSong->m_artistsString = artistString;
      objSong->m_genresString = StringUtils::Join(genres, g_advancedSettings.m_musicItemSeparator);
      objSong->m_title = strTitle;
      objSong->m_track = iTrack;
      objSong->m_duration = iDuration;
      objSong->m_year = iYear;
      objSong->m_musicBrainzTrackID = strMusicBrainzTrackID;
      objSong->m_startOffset = iStartOffset;
      objSong->m_endOffset = iEndOffset;
      objSong->m_rating = rating;
      objSong->m_userrating = userrating;
      objSong->m_votes = votes;
      objSong->m_comment = strComment;
      objSong->m_mood = strMood;

      m_cdb.getDB()->persist(objSong);
    }
    else
    {
      UpdateSong( objSong, strTitle, strMusicBrainzTrackID, strPathAndFileName, strComment, strMood, strThumb,
                  artistString, genres, iTrack, iDuration, iYear, iTimesPlayed, iStartOffset, iEndOffset, 
                  dtLastPlayed, rating, userrating, votes);
    }
    
    if (!objSong->section_foreign.loaded())
      m_cdb.getDB()->load(*objSong, objSong->section_foreign);

    if (!strThumb.empty())
      SetArtForItem(objSong->m_idSong, MediaTypeSong, "thumb", strThumb);

    SetODBDetailsGenres(objSong, genres);
    SetODBDetailsGenres(objAlbum, genres);
    
    std::shared_ptr<CODBFile> objFile(objSong->m_file.get_eager());

    UpdateFileDateAdded(objFile, strPathAndFileName);
    
    if(odb_transaction)
      odb_transaction->commit();

    AnnounceUpdate(MediaTypeSong, objSong->m_idSong);
    
    return objSong;
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "musicdatabase:exception on addsong - %s", e.what());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "musicdatabase:unable to addsong");
  }
  return nullptr;
}

bool CMusicDatabase::GetSong(int idSong, CSong& song)
{
  try
  {
    song.Clear();

    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    
    std::shared_ptr<CODBSong> objSong(new CODBSong);
    if (!m_cdb.getDB()->query_one<CODBSong>(odb::query<CODBSong>::idSong == idSong, *objSong))
    {
      return false;
    }
    
    song = GetSongFromODBObject(objSong);

    return true;
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "%s(%i) failed - %s", __FUNCTION__, idSong, e.what());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%i) failed", __FUNCTION__, idSong);
  }

  return false;
}


bool CMusicDatabase::UpdateSong(int idSong, const CSong &song)
{
  try
  {
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    
    std::shared_ptr<CODBSong> objSong(new CODBSong);
    if (!m_cdb.getDB()->query_one<CODBSong>(odb::query<CODBSong>::idSong == idSong, *objSong))
      return false;
    
    objSong = UpdateSong(objSong, song);
    
    if(odb_transaction)
      odb_transaction->commit();
    
    if (objSong)
      return true;
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "%s exception - %s", __FUNCTION__, e.what());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  
  return false;
}

std::shared_ptr<CODBSong> CMusicDatabase::UpdateSong(std::shared_ptr<CODBSong> objSong, const CSong &song)
{
  return UpdateSong(objSong,
                    song.strTitle,
                    song.strMusicBrainzTrackID,
                    song.strFileName,
                    song.strComment,
                    song.strMood,
                    song.strThumb,
                    song.GetArtistString(), // NOTE: Don't call this function internally!!!
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
                    song.votes);
}

std::shared_ptr<CODBSong> CMusicDatabase::UpdateSong(std::shared_ptr<CODBSong> objSong,
                                                     const std::string& strTitle, const std::string& strMusicBrainzTrackID,
                                                     const std::string& strPathAndFileName, const std::string& strComment,
                                                     const std::string& strMood, const std::string& strThumb,
                                                     const std::string& artistString, const std::vector<std::string>& genres,
                                                     int iTrack, int iDuration, int iYear,
                                                     int iTimesPlayed, int iStartOffset, int iEndOffset,
                                                     const CDateTime& dtLastPlayed, float rating, int userrating, int votes)
{
  try
  {
    if (!objSong)
      return nullptr;
    
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    
    if (!objSong->section_foreign.loaded())
      m_cdb.getDB()->load(*objSong, objSong->section_foreign);
    
    std::string strPath, strFileName;
    URIUtils::Split(strPathAndFileName, strPath, strFileName);
    
    std::shared_ptr<CODBFile> objFile(AddFileAndPath(strFileName, strPath));
    if (objFile == nullptr)
      return nullptr;
    
    objFile->m_playCount = iTimesPlayed;
    if (dtLastPlayed.IsValid())
    {
      objFile->m_lastPlayed.setDateTime(dtLastPlayed.GetAsULongLong(), dtLastPlayed.GetAsDBDateTime());
    }
    m_cdb.getDB()->update(*objFile);
    
    objSong->m_file = objFile;
    
    objSong->m_artistsString = artistString;
    objSong->m_genresString = StringUtils::Join(genres, g_advancedSettings.m_musicItemSeparator);
    objSong->m_title = strTitle;
    objSong->m_track = iTrack;
    objSong->m_duration = iDuration;
    objSong->m_year = iYear;
    objSong->m_musicBrainzTrackID = strMusicBrainzTrackID;
    objSong->m_startOffset = iStartOffset;
    objSong->m_endOffset = iEndOffset;
    objSong->m_rating = rating;
    objSong->m_userrating = userrating;
    objSong->m_votes = votes;
    objSong->m_comment = strComment;
    objSong->m_mood = strMood;
    
    m_cdb.getDB()->update(*objSong);
    m_cdb.getDB()->update(*objSong, objSong->section_foreign);

    UpdateFileDateAdded(objFile, strPathAndFileName);
    
    if(odb_transaction)
      odb_transaction->commit();
    
    AnnounceUpdate(MediaTypeSong, objSong->m_idSong);
    return objSong;
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "musicdatabase:exception on UpdateSong - %s", e.what());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "musicdatabase:unable to UpdateSong");
  }
  return nullptr;
}

std::shared_ptr<CODBAlbum> CMusicDatabase::AddAlbum(const std::string& strAlbum, const std::string& strMusicBrainzAlbumID,
                                                    const std::string& strArtist, const std::string& strGenre, int year,
                                                    const std::string& strRecordLabel, const std::string& strType,
                                                    bool bCompilation, CAlbum::ReleaseType releaseType)
{
  try
  {
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    typedef odb::query<ODBView_Album> query;
    query queryAlbum;

    if (!strMusicBrainzAlbumID.empty())
      queryAlbum = query::CODBAlbum::musicBrainzAlbumID == strMusicBrainzAlbumID;
    else
      queryAlbum = query::CODBAlbum::artistsString.like(strArtist) && query::CODBAlbum::album.like(strAlbum) && query::CODBAlbum::musicBrainzAlbumID == "";

    ODBView_Album objAlbumView;
    if (!m_cdb.getDB()->query_one<ODBView_Album>(queryAlbum, objAlbumView))
    {
      objAlbumView.album = std::shared_ptr<CODBAlbum>(new CODBAlbum);
      
      objAlbumView.album->m_album = strAlbum;
      objAlbumView.album->m_musicBrainzAlbumID = strMusicBrainzAlbumID;
      objAlbumView.album->m_artistsString = strArtist;
      objAlbumView.album->m_genresString = strGenre;
      objAlbumView.album->m_year = year;
      objAlbumView.album->m_label = strRecordLabel;
      objAlbumView.album->m_type = strType;
      objAlbumView.album->m_compilation = bCompilation;
      objAlbumView.album->m_releaseType = CAlbum::ReleaseTypeToString(releaseType);
      
      m_cdb.getDB()->persist(objAlbumView.album);
    }
    else
    {
      /* Exists in our database and being re-scanned from tags, so we should update it as the details
         may have changed.

         Note that for multi-folder albums this will mean the last folder scanned will have the information
         stored for it.  Most values here should be the same across all songs anyway, but it does mean
         that if there's any inconsistencies then only the last folders information will be taken.

         We make sure we clear out the link tables (album artists, album genres) and we reset
         the last scraped time to make sure that online metadata is re-fetched. */
      
      objAlbumView.album->m_album = strAlbum;
      objAlbumView.album->m_musicBrainzAlbumID = strMusicBrainzAlbumID;
      objAlbumView.album->m_artistsString = strArtist;
      objAlbumView.album->m_genresString = strGenre;
      objAlbumView.album->m_year = year;
      objAlbumView.album->m_label = strRecordLabel;
      objAlbumView.album->m_type = strType;
      objAlbumView.album->m_compilation = bCompilation;
      objAlbumView.album->m_releaseType = CAlbum::ReleaseTypeToString(releaseType);
      
      objAlbumView.album->m_artists.clear();
      objAlbumView.album->m_genres.clear();
      //TODO: We may need to cleanup at some point to remove unassigned artists and genre?
      
      m_cdb.getDB()->update(objAlbumView.album);
      m_cdb.getDB()->update(*(objAlbumView.album), objAlbumView.album->section_foreign);
    }
    
    if(odb_transaction)
      odb_transaction->commit();
    
    return objAlbumView.album;
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "%s exception - %s", __FUNCTION__, e.what());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }

  return nullptr;
}

std::shared_ptr<CODBAlbum> CMusicDatabase::UpdateAlbum(int idAlbum,
                                                       const std::string& strAlbum, const std::string& strMusicBrainzAlbumID,
                                                       const std::string& strArtist, const std::string& strGenre,
                                                       const std::string& strMoods, const std::string& strStyles,
                                                       const std::string& strThemes, const std::string& strReview,
                                                       const std::string& strImage, const std::string& strLabel,
                                                       const std::string& strType,
                                                       float fRating, int iUserrating, int iVotes, int iYear, bool bCompilation,
                                                       CAlbum::ReleaseType releaseType)
{
  if (idAlbum < 0)
    return nullptr;
  
  try
  {
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    
    std::shared_ptr<CODBAlbum> objAlbum(new CODBAlbum);
    if (!m_cdb.getDB()->query_one<CODBAlbum>(odb::query<CODBAlbum>::idAlbum == idAlbum, *objAlbum))
      return nullptr;
    
    objAlbum->m_album = strAlbum;
    objAlbum->m_musicBrainzAlbumID = strMusicBrainzAlbumID;
    objAlbum->m_artistsString = strArtist;
    objAlbum->m_genresString = strGenre;
    objAlbum->m_year = iYear;
    objAlbum->m_label = strLabel;
    objAlbum->m_type = strType;
    objAlbum->m_compilation = bCompilation;
    objAlbum->m_releaseType = CAlbum::ReleaseTypeToString(releaseType);
    objAlbum->m_moods = strMoods;
    objAlbum->m_styles = strStyles;
    objAlbum->m_themes = strThemes;
    objAlbum->m_review = strReview;
    objAlbum->m_image = strImage;
    objAlbum->m_rating = fRating;
    objAlbum->m_userrating = iUserrating;
    objAlbum->m_votes = iVotes;
    
    m_cdb.getDB()->update(objAlbum);
    
    if(odb_transaction)
      odb_transaction->commit();
    
    AnnounceUpdate(MediaTypeAlbum, idAlbum);
    
    return objAlbum;
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "%s exception - %s", __FUNCTION__, e.what());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  
  return nullptr;
}

std::shared_ptr<CODBAlbum> CMusicDatabase::GetODBAlbum(int idAlbum)
{
  try
  {
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    
    std::shared_ptr<CODBAlbum> objAlbum(new CODBAlbum);
    if (!m_cdb.getDB()->query_one<CODBAlbum>(odb::query<CODBAlbum>::idAlbum == idAlbum, *objAlbum))
      return nullptr;
    
    return objAlbum;
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "%s exception - %s", __FUNCTION__, e.what());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  
  return nullptr;
}

bool CMusicDatabase::GetAlbum(int idAlbum, CAlbum& album, bool getSongs /* = true */)
{
  try
  {
    if (idAlbum == -1)
      return false; // not in the database
    
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    
    //Get album data
    odb::result<ODBView_Album> res(m_cdb.getDB()->query<ODBView_Album>(odb::query<ODBView_Album>::CODBAlbum::idAlbum == idAlbum));
    if(res.begin() == res.end())
      return false;
    
    std::shared_ptr<CODBAlbum> objAlbum(res.begin()->album);
    
    if (!objAlbum->section_foreign.loaded())
      m_cdb.getDB()->load(*objAlbum, objAlbum->section_foreign);
    
    album = GetAlbumFromODBObject(objAlbum);
    
    for (auto artist : objAlbum->m_artists)
    {
      if (artist.load())
        album.artistCredits.push_back(GetArtistCreditFromODBObject(artist.get_eager()));
    }
    
    //Get song data
    if (getSongs)
    {
      odb::result<ODBView_Song> songRes(m_cdb.getDB()->query<ODBView_Song>(odb::query<ODBView_Song>::CODBAlbum::idAlbum == idAlbum));
      
      std::set<int> songs;
      for (auto song : songRes)
      {
        std::shared_ptr<CODBSong> objSong(song.song);
        
        if (!objSong->section_foreign.loaded())
          m_cdb.getDB()->load(*objSong, objSong->section_foreign);
        
        if (songs.find(objSong->m_idSong) == songs.end())
        {
          album.songs.emplace_back(GetSongFromODBObject(objSong));
          songs.insert(objSong->m_idSong);
        }
        
        for (auto artist : objSong->m_artists)
        {
          if (!artist.load() || artist->m_role.load())
            continue;
          
          if ( artist->m_role->m_name == "artist")
          {
            album.songs.back().artistCredits.emplace_back(GetArtistCreditFromODBObject(artist.get_eager()));
          }
          else
          {
            album.songs.back().AppendArtistRole(GetArtistRoleFromODBObject(artist.get_eager()));
          }
        }
      }
    }
    
    if(odb_transaction)
      odb_transaction->commit();

    return true;
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "%s(%i) exception - %s", __FUNCTION__, idAlbum, e.what());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%i) failed", __FUNCTION__, idAlbum);
  }

  return false;
}

bool CMusicDatabase::ClearAlbumLastScrapedTime(int idAlbum)
{
  try
  {
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    
    CODBAlbum objAlbum;
    if (!m_cdb.getDB()->query_one<CODBAlbum>(odb::query<CODBAlbum>::idAlbum == idAlbum, objAlbum))
      return false;
    
    objAlbum.m_lastScraped.clear();
    
    if(odb_transaction)
      odb_transaction->commit();
    
    return true;
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "%s exception - %s", __FUNCTION__, e.what());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}

bool CMusicDatabase::HasAlbumBeenScraped(int idAlbum)
{
  try
  {
    CODBAlbum objAlbum;
    if (!m_cdb.getDB()->query_one<CODBAlbum>(odb::query<CODBAlbum>::idAlbum == idAlbum, objAlbum))
      return false;
    
    return objAlbum.m_lastScraped.m_ulong_date != 0;
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "%s exception - %s", __FUNCTION__, e.what());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}

template <typename T>
void CMusicDatabase::SetODBDetailsGenres(T odbObject, std::vector<std::string> genres)
{
  if (!genres.empty())
  {
    for (std::vector<odb::lazy_shared_ptr<CODBGenre> >::iterator i = odbObject->m_genres.begin(); i != odbObject->m_genres.end();)
    {
      if ((*i).load())
      {
        (*i)->m_synced = false;
        i++;
      }
      else
        //Object could not be loaded, removing it
        i = odbObject->m_genres.erase(i);
    }
    
    for (auto& i: genres)
    {
      bool already_exists = false;
      for (auto& j: odbObject->m_genres)
      {
        if (j->m_name == i)
        {
          j->m_synced = true;
          already_exists = true;
          break;
        }
      }
      
      if (already_exists)
        continue;
      
      std::shared_ptr<CODBGenre> genre(new CODBGenre);
      if (!m_cdb.getDB()->query_one<CODBGenre>(odb::query<CODBGenre>::name == i, *genre))
      {
        genre->m_name = i;
        genre->m_type = MediaTypeMusic;
        m_cdb.getDB()->persist(genre);
      }
      genre->m_synced = true;
      odbObject->m_genres.push_back(genre);
    }
    
    //Cleanup
    for (std::vector<odb::lazy_shared_ptr<CODBGenre> >::iterator i = odbObject->m_genres.begin(); i != odbObject->m_genres.end();)
    {
      if (!(*i)->m_synced)
      {
        i = odbObject->m_genres.erase(i);
      }
      else
        i++;
    }
  }
}

bool CMusicDatabase::UpdateArtist(const CArtist& artist)
{
  try
  {
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    
    std::shared_ptr<CODBPerson> objPerson(new CODBPerson);
    if (!m_cdb.getDB()->query_one<CODBPerson>(odb::query<CODBPerson>::idPerson == artist.idArtist, *objPerson))
      return false;
    
    UpdateArtist(objPerson,
                 artist.strArtist, artist.strMusicBrainzArtistID,
                 artist.strBorn, artist.strFormed,
                 artist.genre,
                 StringUtils::Join(artist.moods, g_advancedSettings.m_musicItemSeparator),
                 StringUtils::Join(artist.styles, g_advancedSettings.m_musicItemSeparator),
                 StringUtils::Join(artist.instruments, g_advancedSettings.m_musicItemSeparator),
                 artist.strBiography, artist.strDied,
                 artist.strDisbanded,
                 StringUtils::Join(artist.yearsActive, g_advancedSettings.m_musicItemSeparator).c_str(),
                 artist.thumbURL.m_xml.c_str(),
                 artist.fanart.m_xml.c_str());

    DeleteArtistDiscography(objPerson->m_idPerson);
    for (const auto &disc : artist.discography)
    {
      AddArtistDiscography(objPerson, disc.first, disc.second);
    }
    
    if(odb_transaction)
      odb_transaction->commit();

    return true;
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "%s exception - %s", __FUNCTION__, e.what());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}

std::shared_ptr<CODBPerson> CMusicDatabase::AddArtist(const std::string& strArtist, const std::string& strMusicBrainzArtistID)
{
  try
  {
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    
    typedef odb::query<ODBView_Artist_Details> query;

    // 1) MusicBrainz
    if (!strMusicBrainzArtistID.empty())
    {
      // 1.a) Match on a MusicBrainz ID
      ODBView_Artist_Details objArtistDetail;
      if (m_cdb.getDB()->query_one<ODBView_Artist_Details>(query::CODBArtistDetail::musicBrainzArtistId == strMusicBrainzArtistID, objArtistDetail))
      {
        if (objArtistDetail.person->m_name.compare(strMusicBrainzArtistID) == 0)
        {
          objArtistDetail.person->m_name = strArtist;
          m_cdb.getDB()->update(objArtistDetail.person);
        }
        
        if(odb_transaction)
          odb_transaction->commit();
        
        return objArtistDetail.person;
      }


      // 1.b) No match on MusicBrainz ID. Look for a previously added artist with no MusicBrainz ID
      //     and update that if it exists.
      if (m_cdb.getDB()->query_one<ODBView_Artist_Details>(query::CODBPerson::name == strArtist, objArtistDetail))
      {
        // 1.b.a) We found an artist by name but with no MusicBrainz ID set, update it and assume it is our artist
        if (objArtistDetail.details)
        {
          objArtistDetail.details->m_musicBrainzArtistId = strMusicBrainzArtistID;
          m_cdb.getDB()->update(objArtistDetail.details);
        }
        else
        {
          objArtistDetail.details = std::shared_ptr<CODBArtistDetail>(new CODBArtistDetail);
          objArtistDetail.details->m_person = objArtistDetail.person;
          objArtistDetail.details->m_musicBrainzArtistId = strMusicBrainzArtistID;
          m_cdb.getDB()->persist(objArtistDetail.details);
        }
        
        return objArtistDetail.person;
      }

    // 2) No MusicBrainz - search for any artist (MB ID or non) with the same name.
    //    With MusicBrainz IDs this could return multiple artists and is non-deterministic
    //    Always pick the first artist ID returned by the DB to return.
    }
    else
    {
      ODBView_Artist_Details objArtistDetail;
      if (m_cdb.getDB()->query_one<ODBView_Artist_Details>(query::CODBPerson::name == strArtist, objArtistDetail))
      {
        return objArtistDetail.person;
      }
    }

    // 3) No artist exists at all - add it
    std::shared_ptr<CODBPerson> objPerson(new CODBPerson);
    objPerson->m_name = strArtist;
    m_cdb.getDB()->persist(objPerson);
    
    CODBArtistDetail objDetails;
    objDetails.m_person = objPerson;
    objDetails.m_musicBrainzArtistId = strMusicBrainzArtistID;
    m_cdb.getDB()->persist(objDetails);
    
    if(odb_transaction)
      odb_transaction->commit();
    
    return objPerson;
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "musicdatabase:exception on addartist - %s", e.what());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "musicdatabase:unable to addartist");
  }

  return nullptr;
}

std::shared_ptr<CODBPerson> CMusicDatabase::UpdateArtist(std::shared_ptr<CODBPerson> objArtist,
                                  const std::string& strArtist, const std::string& strMusicBrainzArtistID,
                                  const std::string& strBorn, const std::string& strFormed,
                                  const std::vector<std::string>& genres, const std::string& strMoods,
                                  const std::string& strStyles, const std::string& strInstruments,
                                  const std::string& strBiography, const std::string& strDied,
                                  const std::string& strDisbanded, const std::string& strYearsActive,
                                  const std::string& strImage, const std::string& strFanart)
{
  CScraperUrl thumbURL;
  CFanart fanart;
  if (!objArtist)
    return nullptr;
  
  try
  {
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    
    objArtist->m_name = strArtist;
    m_cdb.getDB()->update(objArtist);
    
    std::shared_ptr<CODBArtistDetail> objDetails(new CODBArtistDetail);
    bool exists = false;
    if (m_cdb.getDB()->query_one<CODBArtistDetail>(odb::query<CODBArtistDetail>::person->idPerson == objArtist->m_idPerson, *objDetails))
    {
      exists = true;
    }
    
    objDetails->m_musicBrainzArtistId = strMusicBrainzArtistID;
    objDetails->m_born = strBorn;
    objDetails->m_formed = strFormed;
    SetODBDetailsGenres(objDetails, genres);
    objDetails->m_moods = strMoods;
    objDetails->m_styles = strStyles;
    objDetails->m_instruments  = strInstruments;
    objDetails->m_biography = strBiography;
    objDetails->m_died = strDied;
    objDetails->m_disbanded = strDisbanded;
    objDetails->m_yearsActive = strYearsActive;
    objDetails->m_image = strImage;
    objDetails->m_fanart = strFanart;
    CDateTime current = CDateTime::GetCurrentDateTime();
    objDetails->m_lastScraped.setDateTime(current.GetAsULongLong(), current.GetAsDBDateTime());
    
    if (exists)
      m_cdb.getDB()->update(objDetails);
    else
      m_cdb.getDB()->persist(objDetails);
    
    AnnounceUpdate(MediaTypeArtist, objArtist->m_idPerson);
    
    if(odb_transaction)
      odb_transaction->commit();
    
    return objArtist;
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "%s exception - %s", __FUNCTION__, e.what());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  
  return nullptr;
  
}

bool CMusicDatabase::GetArtist(int idArtist, CArtist &artist, bool fetchAll /* = false */)
{
  try
  {
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    
    std::shared_ptr<CODBPerson> objArtist(new CODBPerson);
    
    if (!m_cdb.getDB()->query_one<CODBPerson>(odb::query<CODBPerson>::idPerson == idArtist, *objArtist))
      return false;
    
    artist.discography.clear();
    artist = GetArtistFromODBObject(objArtist, fetchAll);
    
    
    if (fetchAll)
    {
      //Also fetch the discography
      odb::result<CODBArtistDiscography> res(m_cdb.getDB()->query<CODBArtistDiscography>(odb::query<CODBArtistDiscography>::artist->idPerson == objArtist->m_idPerson));
      for (auto objDiscography : res)
      {
        artist.discography.emplace_back(objDiscography.m_album, std::to_string(objDiscography.m_year));
      }
    }

    if(odb_transaction)
      odb_transaction->commit();
    
    return true;
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "%s(%i) exception - %s", __FUNCTION__, idArtist, e.what());
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
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    
    std::shared_ptr<CODBPerson> objPerson(new CODBPerson);
    if (m_cdb.getDB()->query_one<CODBPerson>(odb::query<CODBPerson>::idPerson == idArtist, *objPerson))
      return true;
    
    return false;
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "%s(%i) exception - %s", __FUNCTION__, idArtist, e.what());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%i) failed", __FUNCTION__, idArtist);
  }

  return false;
}

bool CMusicDatabase::HasArtistBeenScraped(int idArtist)
{
  try
  {
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    
    std::shared_ptr<ODBView_Artist_Details> objDetails(new ODBView_Artist_Details);
    if (m_cdb.getDB()->query_one<ODBView_Artist_Details>(odb::query<ODBView_Artist_Details>::CODBPerson::idPerson == idArtist, *objDetails))
      return objDetails->details->m_lastScraped.m_ulong_date != 0;
    return false;
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "%s exception - %s", __FUNCTION__, e.what());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  
  return false;
}

bool CMusicDatabase::ClearArtistLastScrapedTime(int idArtist)
{
  try
  {
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    
    std::shared_ptr<ODBView_Artist_Details> objDetails(new ODBView_Artist_Details);
    if (m_cdb.getDB()->query_one<ODBView_Artist_Details>(odb::query<ODBView_Artist_Details>::CODBPerson::idPerson == idArtist, *objDetails))
    {
      objDetails->details->m_lastScraped.clear();
      m_cdb.getDB()->update(objDetails->details);
      
      if(odb_transaction)
        odb_transaction->commit();
      
      return true;
    }
    return false;
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "%s exception - %s", __FUNCTION__, e.what());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  
  return false;
}

bool CMusicDatabase::AddArtistDiscography(std::shared_ptr<CODBPerson> objPerson, const std::string& strAlbum, const std::string& strYear)
{
  try
  {
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    typedef odb::query<CODBArtistDiscography> query;
    
    std::shared_ptr<CODBArtistDiscography> objDiscography(new CODBArtistDiscography);
    if (m_cdb.getDB()->query_one<CODBArtistDiscography>(query::artist->idPerson == objPerson->m_idPerson
                                                        && query::album == strAlbum
                                                        && query::year == std::stoi(strYear)
                                                        , *objDiscography))
    {
      return true;
    }
    
    objDiscography->m_artist = objPerson;
    objDiscography->m_album = strAlbum;
    objDiscography->m_year = std::stoi(strYear);
    
    m_cdb.getDB()->persist(objDiscography);
    
    if(odb_transaction)
      odb_transaction->commit();
    
    return true;
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "%s exception - %s", __FUNCTION__, e.what());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  
  return false;
}

bool CMusicDatabase::DeleteArtistDiscography(int idArtist)
{
  try
  {
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    
    m_cdb.getDB()->erase_query<CODBArtistDiscography>(odb::query<CODBArtistDiscography>::artist->idPerson == idArtist);
    
    if(odb_transaction)
      odb_transaction->commit();
    
    return true;
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "%s exception - %s", __FUNCTION__, e.what());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}

bool CMusicDatabase::AddSongArtist(std::shared_ptr<CODBPerson> artist, std::shared_ptr<CODBSong> song, int iOrder, const std::string& strRole /* = "artist" */)
{
  try
  {
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    
    //Make sure the foreing values are loaded
    if (!song->section_foreign.loaded())
      m_cdb.getDB()->load(*song, song->section_foreign);
    
    //Check if the artist is already assigned
    for (auto& songArtist : song->m_artists)
    {
      if (songArtist->m_person.load() && songArtist->m_person->m_idPerson == artist->m_idPerson)
        return true;
    }
    
    std::shared_ptr<CODBPersonLink> objPersonLink(new CODBPersonLink);
    objPersonLink->m_person = artist;
    objPersonLink->m_castOrder = iOrder;
    
    std::shared_ptr<CODBRole> objRole(AddRole(strRole));
    if (objRole)
      objPersonLink->m_role = objRole;
    m_cdb.getDB()->persist(objPersonLink);
    
    song->m_artists.push_back(objPersonLink);
    
    m_cdb.getDB()->update(*song);
    m_cdb.getDB()->update(*song, song->section_foreign);
    
    if (odb_transaction)
      odb_transaction->commit();
    
    return true;
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "%s exception - %s", __FUNCTION__, e.what());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  
  return false;
}

std::shared_ptr<CODBPerson> CMusicDatabase::AddSongContributor(std::shared_ptr<CODBSong> objSong, const std::string& strRole, const std::string& strArtist)
{
  if (strArtist.empty())
    return nullptr;

  std::string strSQL;
  try
  {
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    
    if (!objSong->section_foreign.loaded())
      m_cdb.getDB()->load(*objSong, objSong->section_foreign);
    
    std::shared_ptr<CODBPerson> objArtist;
    // Add artist. As we only have name (no MBID) first try to identify artist from song
    // as they may have already been added with a different role (including MBID).
    for (auto& artist : objSong->m_artists)
    {
      if (!artist.load() || !artist->m_person.load())
        continue;
      
      if (artist->m_person->m_name == strArtist)
      {
        objArtist = artist->m_person.get_eager();
      }
    }
    
    if (!objArtist)
      objArtist = AddArtist(strArtist, "");

    // Add to song
    AddSongArtist(objArtist, objSong, 0, strRole);
    
    if(odb_transaction)
      odb_transaction->commit();

    return objArtist;
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "musicdatabase:exception no AddSongContributor (%s) - %s", strSQL.c_str(), e.what());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "musicdatabase:unable to AddSongContributor (%s)", strSQL.c_str());
  }

  return nullptr;
}

void CMusicDatabase::AddSongContributors(std::shared_ptr<CODBSong> objSong, const VECMUSICROLES& contributors)
{
  for (const auto &credit : contributors)
  {
    AddSongContributor(objSong, credit.GetRoleDesc(), credit.GetArtist());
  }
}

int CMusicDatabase::GetRoleByName(const std::string& strRole)
{
  try
  {
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    
    std::shared_ptr<CODBRole> objRole(new CODBRole);
    if (!m_cdb.getDB()->query_one<CODBRole>(odb::query<CODBRole>::name == strRole, *objRole))
      return -1;
    
    return objRole->m_idRole;
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "%s exception - %s", __FUNCTION__, e.what());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return -1;
  
}

std::shared_ptr<CODBRole> CMusicDatabase::AddRole(const std::string& strRole)
{
  try
  {
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    
    std::shared_ptr<CODBRole> objRole(new CODBRole);
    if (m_cdb.getDB()->query_one<CODBRole>(odb::query<CODBRole>::name == strRole, *objRole))
    {
      return objRole;
    }
    
    objRole->m_name = strRole;
    m_cdb.getDB()->persist(objRole);
    
    if(odb_transaction)
      odb_transaction->commit();
    
    return objRole;
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "%s exception - %s", __FUNCTION__, e.what());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  
  return nullptr;
}

bool CMusicDatabase::GetRolesByArtist(int idArtist, CFileItem* item)
{
  try
  {
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    
    CVariant artistRoles(CVariant::VariantTypeArray);
    
    odb::result<ODBView_Artist_Roles> res(m_cdb.getDB()->query<ODBView_Artist_Roles>(odb::query<ODBView_Artist_Roles>::CODBPerson::idPerson == idArtist));
    for (auto objRole : res)
    {
      CVariant roleObj;
      roleObj["role"] = objRole.role->m_name;
      roleObj["roleid"] = (int)objRole.role->m_idRole;
      artistRoles.push_back(roleObj);
    }
    
    item->SetProperty("roles", artistRoles);
    return true;
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "%s(%i) exception - %s", __FUNCTION__, idArtist, e.what());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%i) failed", __FUNCTION__, idArtist);
  }
  return false;
}

bool CMusicDatabase::DeleteSongArtistsBySong(std::shared_ptr<CODBSong> objSong)
{
  objSong->m_artists.clear();
  return true;
}

bool CMusicDatabase::AddAlbumArtist(std::shared_ptr<CODBPerson> artist, std::shared_ptr<CODBAlbum> album, int iOrder)
{
  try
  {
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    
    //Make sure the foreing values are loaded
    if (!album->section_foreign.loaded())
      m_cdb.getDB()->load(*album, album->section_foreign);
    
    //Check if the artist is already assigned
    for (auto& albumArtist : album->m_artists)
    {
      if (albumArtist.load() && albumArtist->m_person.load() && albumArtist->m_person->m_idPerson == artist->m_idPerson)
        return true;
    }
    
    std::shared_ptr<CODBPersonLink> objPersonLink(new CODBPersonLink);
    objPersonLink->m_person = artist;
    objPersonLink->m_castOrder = iOrder;
    
    std::shared_ptr<CODBRole> objRole(AddRole("artist"));
    if (objRole)
      objPersonLink->m_role = objRole;
    m_cdb.getDB()->persist(objPersonLink);
    
    album->m_artists.push_back(objPersonLink);
    m_cdb.getDB()->update(*album, album->section_foreign);
    
    if(odb_transaction)
      odb_transaction->commit();
    
    return true;
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "%s exception - %s", __FUNCTION__, e.what());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}

bool CMusicDatabase::GetAlbumsByArtist(int idArtist, std::vector<int> &albums)
{
  try 
  {
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    
    odb::result<ODBView_Albums_by_Artist> res(m_cdb.getDB()->query<ODBView_Albums_by_Artist>(odb::query<ODBView_Albums_by_Artist>::CODBPerson::idPerson == idArtist));
    if (res.begin() == res.end())
      return false;
    
    for (auto objAlbum : res)
    {
      albums.push_back(objAlbum.album->m_idAlbum);
    }

    return true;
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "%s(%i) exception - %s", __FUNCTION__, idArtist, e.what());
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
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    
    odb::result<ODBView_Artists_by_Album> res(m_cdb.getDB()->query<ODBView_Artists_by_Album>(odb::query<ODBView_Artists_by_Album>::CODBAlbum::idAlbum == idAlbum));
    if (res.begin() == res.end())
      return false;
    
    // Get album artist credits
    VECARTISTCREDITS artistCredits;
    for (auto artist : res)
    {
      artistCredits.emplace_back(GetArtistCreditFromODBObject(artist.artist));
    }
   
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
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "%s(%i) exception - %s", __FUNCTION__, idAlbum, e.what());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%i) failed", __FUNCTION__, idAlbum);
  }
  return false;
}

bool CMusicDatabase::GetArtistsBySong(int idSong, std::vector<int> &artists)
{
  try 
  {
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    
    odb::result<ODBView_Song_Artists_Link> res(m_cdb.getDB()->query<ODBView_Song_Artists_Link>(odb::query<ODBView_Song_Artists_Link>::CODBSong::idSong == idSong));
    if (res.begin() == res.end())
      return false;
    
    for (auto artist : res)
    {
      if (artist.artist->m_person.load())
        artists.push_back(artist.artist->m_person->m_idPerson);
    }

    return true;
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "%s(%i) exception - %s", __FUNCTION__, idSong, e.what());
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
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    
    odb::result<ODBView_Artist_Genres> res(m_cdb.getDB()->query<ODBView_Artist_Genres>(odb::query<ODBView_Artist_Genres>::CODBPerson::idPerson == idArtist));
    if (res.begin() == res.end())
      return true;

    CVariant artistSongGenres(CVariant::VariantTypeArray);

    for (auto genre : res)
    {
      CVariant genreObj;
      genreObj["title"] = genre.genre->m_name;
      genreObj["genreid"] = (int)genre.genre->m_idGenre;
      artistSongGenres.push_back(genreObj);
      m_pDS->next();
    }
    m_pDS->close();

    item->SetProperty("songgenres", artistSongGenres);
    return true;
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "%s(%i) exception - %s", __FUNCTION__, idArtist, e.what());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%i) failed", __FUNCTION__, idArtist);
  }
  return false;
}

bool CMusicDatabase::GetGenresByAlbum(int idAlbum, std::vector<int>& genres)
{
  try
  {
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    
    std::shared_ptr<CODBAlbum> objAlbum(new CODBAlbum);
    if (!m_cdb.getDB()->query_one<CODBAlbum>(odb::query<CODBAlbum>::idAlbum == idAlbum, *objAlbum))
      return false;
    
    if (!objAlbum->section_foreign.loaded())
      m_cdb.getDB()->load(*objAlbum, objAlbum->section_foreign);
    
    for (auto genre : objAlbum->m_genres)
    {
      if (genre.load())
        genres.push_back(genre->m_idGenre);
    }

    return true;
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "%s(%i) exception - %s", __FUNCTION__, idAlbum, e.what());
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
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    
    std::shared_ptr<CODBSong> objSong(new CODBSong);
    if (!m_cdb.getDB()->query_one<CODBSong>(odb::query<CODBSong>::idSong == idSong, *objSong))
      return false;
    
    if (!objSong->section_foreign.loaded())
      m_cdb.getDB()->load(*objSong, objSong->section_foreign);
    
    for (auto genre : objSong->m_genres)
    {
      if (genre.load())
        genres.push_back(genre->m_idGenre);
    }

    return true;
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "%s(%i) exception - %s", __FUNCTION__, idSong, e.what());
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
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    
    odb::result<ODBView_Albums_by_Artist> res(m_cdb.getDB()->query<ODBView_Albums_by_Artist>(odb::query<ODBView_Albums_by_Artist>::CODBPerson::idPerson == idArtist));
    bool IsAlbumArtistObj = (res.begin() != res.end());
    item->SetProperty("isalbumartist", IsAlbumArtistObj);
    return true;
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "%s(%i) exception - %s", __FUNCTION__, idArtist, e.what());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%i) failed", __FUNCTION__, idArtist);
  }
  return false;
}


std::shared_ptr<CODBPath> CMusicDatabase::AddPath(const std::string& strPath1)
{
  try
  {
    std::string strPath(strPath1);
    if (!URIUtils::HasSlashAtEnd(strPath))
      URIUtils::AddSlashAtEnd(strPath);
    
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());

    auto it = m_pathCache.find(strPath);
    if (it != m_pathCache.end())
    {
      return it->second;
    }
    
    std::shared_ptr<CODBPath> objPath(new CODBPath);
    if (!m_cdb.getDB()->query_one<CODBPath>(odb::query<CODBPath>::path == strPath, *objPath))
    {
      objPath->m_path = strPath;
      
      CDateTime dateAdded = CDateTime::GetCurrentDateTime();
      objPath->m_dateAdded = CODBDate(dateAdded.GetAsULongLong(), dateAdded.GetAsDBDateTime());
      
      m_cdb.getDB()->persist(objPath);
      
      if(odb_transaction)
        odb_transaction->commit();
      
      m_pathCache.insert(std::pair<std::string, std::shared_ptr<CODBPath>>(strPath, objPath));
    }
    
    return objPath;
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "musicdatabase:exception on addpath - %s", e.what());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "musicdatabase:unable to addpath");
  }

  return nullptr;
}

std::shared_ptr<CODBFile> CMusicDatabase::AddFileAndPath(const std::string& strFileName, const std::string& strPath)
{
  std::string strSQL = "";
  try
  {
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    typedef odb::query<CODBFile> query;
    
    std::shared_ptr<CODBPath> path = AddPath(strPath);
    if (!path)
      return nullptr;
    
    std::shared_ptr<CODBFile> file(new CODBFile);
    if (m_cdb.getDB()->query_one<CODBFile>(query::filename == strFileName && query::path->idPath == path->m_idPath, *file))
    {
      return file;
    }
    
    file->m_path = path;
    file->m_filename = strFileName;
    m_cdb.getDB()->persist(file);
    
    if(odb_transaction)
      odb_transaction->commit();
    
    return file;
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "%s exception on addfile (%s) - %s", __FUNCTION__, strSQL.c_str(), e.what());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s unable to addfile (%s)", __FUNCTION__, strSQL.c_str());
  }
  return nullptr;
}

CSong CMusicDatabase::GetSongFromODBObject(std::shared_ptr<CODBSong> objSong)
{
  if (!objSong->section_foreign.loaded())
    m_cdb.getDB()->load(*objSong, objSong->section_foreign);
  
  CSong song;
  song.idSong = objSong->m_idSong;
  
  song.strArtistDesc = objSong->m_artistsString;
  for (auto& artist : objSong->m_artists)
  {
    if (!artist.load() || !artist->m_person.load())
      continue;
    
    CODBArtistDetail objArtistDetail;
    if (!m_cdb.getDB()->query_one<CODBArtistDetail>(odb::query<CODBArtistDetail>::person->idPerson == artist->m_person->m_idPerson, objArtistDetail))
      continue;
    
    if (artist->m_role.load() && artist->m_role->m_name == "artist")
      song.artistCredits.emplace_back(artist->m_person->m_name);
    else
      song.AppendArtistRole(GetArtistRoleFromODBObject(artist.get_eager()));
  }
    
  for (auto& genre : objSong->m_genres)
  {
    if (genre.load())
      song.genre.push_back(genre->m_name);
  }
  
  if (objSong->m_album.load())
  {
    song.strAlbum = objSong->m_album->m_album;
    song.idAlbum = objSong->m_album->m_idAlbum;
    song.bCompilation = objSong->m_album->m_compilation;
  }
  
  if (objSong->m_file.load())
  {
    song.iTimesPlayed = objSong->m_file->m_playCount;
    song.lastPlayed.SetFromULongLong(objSong->m_file->m_lastPlayed.m_ulong_date);
    song.dateAdded.SetFromULongLong(objSong->m_file->m_dateAdded.m_ulong_date);
    
    // Get filename with full path
    if (objSong->m_file->m_path.load())
    {
      song.strFileName = URIUtils::AddFileToFolder(objSong->m_file->m_path->m_path, objSong->m_file->m_filename);
    }
  }
  
  // and the rest...
  song.iTrack = objSong->m_track;
  song.iDuration = objSong->m_duration;
  song.iYear = objSong->m_year;
  song.strTitle = objSong->m_title;
  song.iStartOffset = objSong->m_startOffset;
  song.iEndOffset = objSong->m_endOffset;
  song.strMusicBrainzTrackID = objSong->m_musicBrainzTrackID;
  song.rating = objSong->m_rating;
  song.userrating = objSong->m_userrating;
  song.votes = objSong->m_votes;
  song.strComment = objSong->m_comment;
  song.strMood = objSong->m_mood;

  return song;
}

void CMusicDatabase::GetFileItemFromODBObject(std::shared_ptr<CODBSong> objSong, CFileItem* item, const CMusicDbUrl &baseUrl)
{
  if (!objSong)
    return;
  
  if (!objSong->section_foreign.loaded())
    m_cdb.getDB()->load(*objSong, objSong->section_foreign);
  
  // get the artist string from songview (not the song_artist and artist tables)
  item->GetMusicInfoTag()->SetArtistDesc(objSong->m_artistsString);
  // and the full genre string
  item->GetMusicInfoTag()->SetGenre(objSong->m_genresString);
  // and the rest...
  
  item->GetMusicInfoTag()->SetTrackAndDiscNumber(objSong->m_track);
  item->GetMusicInfoTag()->SetDuration(objSong->m_duration);
  item->GetMusicInfoTag()->SetDatabaseId(objSong->m_idSong, MediaTypeSong);
  SYSTEMTIME stTime;
  stTime.wYear = (WORD)objSong->m_year;
  item->GetMusicInfoTag()->SetReleaseDate(stTime);
  item->GetMusicInfoTag()->SetTitle(objSong->m_title);
  item->SetLabel(objSong->m_title);
  item->m_lStartOffset = objSong->m_startOffset;
  item->SetProperty("item_start", item->m_lStartOffset);
  item->m_lEndOffset = objSong->m_endOffset;
  item->GetMusicInfoTag()->SetMusicBrainzTrackID(objSong->m_musicBrainzTrackID);
  item->GetMusicInfoTag()->SetRating(objSong->m_rating);
  item->GetMusicInfoTag()->SetUserrating(objSong->m_userrating);
  item->GetMusicInfoTag()->SetVotes(objSong->m_votes);
  item->GetMusicInfoTag()->SetComment(objSong->m_comment);
  item->GetMusicInfoTag()->SetMood(objSong->m_mood);
  
  if (objSong->m_album.load())
  {
    item->GetMusicInfoTag()->SetAlbum(objSong->m_album->m_album);
    item->GetMusicInfoTag()->SetAlbumId(objSong->m_album->m_idAlbum);
    item->GetMusicInfoTag()->SetCompilation(objSong->m_album->m_compilation);
    
    // get the album artist string from songview (not the album_artist and artist tables)
    item->GetMusicInfoTag()->SetAlbumArtist(objSong->m_album->m_artistsString);
    item->GetMusicInfoTag()->SetAlbumReleaseType(CAlbum::ReleaseTypeFromString(objSong->m_album->m_releaseType));
    item->GetMusicInfoTag()->SetLoaded(true);
  }
  
  if (objSong->m_file.load() && objSong->m_file->m_path.load())
  {
    item->GetMusicInfoTag()->SetPlayCount(objSong->m_file->m_playCount);
    item->GetMusicInfoTag()->SetLastPlayed(objSong->m_file->m_lastPlayed.m_date);
    item->GetMusicInfoTag()->SetDateAdded(objSong->m_file->m_dateAdded.m_date);
    std::string strRealPath = URIUtils::AddFileToFolder(objSong->m_file->m_path->m_path, objSong->m_file->m_filename);
    item->GetMusicInfoTag()->SetURL(strRealPath);
    
    // Get filename with full path
    if (!baseUrl.IsValid())
      item->SetPath(strRealPath);
    else
    {
      CMusicDbUrl itemUrl = baseUrl;
      std::string strFileName = objSong->m_file->m_filename;
      std::string strExt = URIUtils::GetExtension(strFileName);
      std::string path = StringUtils::Format("%lu%s", objSong->m_idSong, strExt.c_str());
      itemUrl.AppendPath(path);
      item->SetPath(itemUrl.ToString());
    }
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

CAlbum CMusicDatabase::GetAlbumFromODBObject(std::shared_ptr<CODBAlbum> objAlbum, bool imageURL /* = false */)
{
  CAlbum album;
  
  if (!objAlbum->section_foreign.loaded())
    m_cdb.getDB()->load(*objAlbum, objAlbum->section_foreign);
  
  album.idAlbum = objAlbum->m_idAlbum;
  album.strAlbum = objAlbum->m_album;
  if (album.strAlbum.empty())
    album.strAlbum = g_localizeStrings.Get(1050);
  album.strMusicBrainzAlbumID = objAlbum->m_musicBrainzAlbumID;
  album.strArtistDesc = objAlbum->m_artistsString;
  for (auto genre : objAlbum->m_genres)
  {
    if (genre.load())
      album.genre.push_back(genre->m_name);
  }
  album.iYear = objAlbum->m_year;
  if (imageURL)
    album.thumbURL.ParseString(objAlbum->m_image);
  album.fRating = objAlbum->m_rating;
  album.iUserrating = objAlbum->m_userrating;
  album.iVotes = objAlbum->m_votes;
  album.strReview = objAlbum->m_review;
  album.styles = StringUtils::Split(objAlbum->m_styles, g_advancedSettings.m_musicItemSeparator);
  album.moods = StringUtils::Split(objAlbum->m_moods, g_advancedSettings.m_musicItemSeparator);
  album.themes = StringUtils::Split(objAlbum->m_themes, g_advancedSettings.m_musicItemSeparator);
  album.strLabel = objAlbum->m_label;
  album.strType = objAlbum->m_type;
  album.bCompilation = objAlbum->m_compilation;
  album.SetReleaseType(objAlbum->m_releaseType);
  
  ODBView_Album_File_Details objDetails;
  if (m_cdb.getDB()->query_one<ODBView_Album_File_Details>(odb::query<ODBView_Album_File_Details>::CODBAlbum::idAlbum == objAlbum->m_idAlbum, objDetails))
  {
    album.iTimesPlayed = objDetails.watchedCount;
    //TODO: We could assign the ulonglong directly
    CDateTime dateAdded;
    dateAdded.SetFromULongLong(objDetails.dateAddedULong);
    album.SetDateAdded(dateAdded.GetAsDBDateTime());
    
    CDateTime lastPlayed;
    lastPlayed.SetFromULongLong(objDetails.lastPlayedULong);
    album.SetLastPlayed(lastPlayed.GetAsDBDateTime());
  }

  return album;
}

CArtistCredit CMusicDatabase::GetArtistCreditFromODBObject(std::shared_ptr<CODBPersonLink> objPersonLinks)
{
  CArtistCredit artistCredit;
  
  if (!objPersonLinks->m_person.load())
    return artistCredit;
  
  artistCredit.idArtist = objPersonLinks->m_person->m_idPerson;
  artistCredit.m_strArtist = objPersonLinks->m_person->m_name;
  
  CODBArtistDetail objDetail;
  if (m_cdb.getDB()->query_one<CODBArtistDetail>(odb::query<CODBArtistDetail>::person->idPerson == objPersonLinks->m_person->m_idPerson, objDetail))
    artistCredit.m_strMusicBrainzArtistID = objDetail.m_musicBrainzArtistId;
  
  return artistCredit;
}

CMusicRole CMusicDatabase::GetArtistRoleFromODBObject(std::shared_ptr<CODBPersonLink> objArtist)
{
  if (!objArtist->m_role.load() || !objArtist->m_person.load())
    return CMusicRole();

  CMusicRole ArtistRole(objArtist->m_idPersonLink,
                        objArtist->m_role->m_name,
                        objArtist->m_person->m_name,
                        objArtist->m_person->m_idPerson);
  return ArtistRole;
}

CArtist CMusicDatabase::GetArtistFromODBObject(std::shared_ptr<CODBPerson> objArtist, bool needThumb /* = true */)
{
  CArtist artist;
  
  artist.idArtist = objArtist->m_idPerson;
  
  if (objArtist->m_name == BLANKARTIST_NAME && m_translateBlankArtist)
    artist.strArtist = g_localizeStrings.Get(38042);  //Missing artist tag in current language
  else
    artist.strArtist = objArtist->m_name;
  
  std::shared_ptr<CODBArtistDetail> objDetails(new CODBArtistDetail);
  if (!m_cdb.getDB()->query_one<CODBArtistDetail>(odb::query<CODBArtistDetail>::person->idPerson == objArtist->m_idPerson, *objDetails))
    return artist;
  
  if (!objDetails->section_foreign.loaded())
    m_cdb.getDB()->load(*objDetails, objDetails->section_foreign);
  
  artist.strMusicBrainzArtistID = objDetails->m_musicBrainzArtistId;
  for (auto genre : objDetails->m_genres)
  {
    if (!genre.load())
      continue;
    
    artist.genre.push_back(genre->m_name);
  }
  artist.strBiography = objDetails->m_biography;
  artist.styles = StringUtils::Split(objDetails->m_styles, g_advancedSettings.m_musicItemSeparator);
  artist.moods = StringUtils::Split(objDetails->m_moods, g_advancedSettings.m_musicItemSeparator);
  artist.strBorn = objDetails->m_born;
  artist.strFormed = objDetails->m_formed;
  artist.strDied = objDetails->m_died;
  artist.strDisbanded = objDetails->m_disbanded;
  artist.yearsActive = StringUtils::Split(objDetails->m_yearsActive, g_advancedSettings.m_musicItemSeparator);
  artist.instruments = StringUtils::Split(objDetails->m_instruments, g_advancedSettings.m_musicItemSeparator);
  artist.SetDateAdded(objDetails->m_lastScraped.m_date);
  
  if (needThumb)
  {
    artist.fanart.m_xml = objDetails->m_fanart;
    artist.fanart.Unpack();
    
    if (objArtist->m_art.load())
    {
      artist.thumbURL.ParseString(objArtist->m_art->m_url);
    }
  }
  
  return artist;
}

bool CMusicDatabase::GetSongByFileName(const std::string& strFileNameAndPath, CSong& song, int startOffset)
{
  song.Clear();
  CURL url(strFileNameAndPath);
  
  if (url.IsProtocol("musicdb"))
  {
    std::string strFile = URIUtils::GetFileName(strFileNameAndPath);
    URIUtils::RemoveExtension(strFile);
    return GetSong(atol(strFile.c_str()), song);
  }
  
  try
  {
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    
    std::string strPath, strFileName;
    URIUtils::Split(strFileNameAndPath, strPath, strFileName);
    URIUtils::AddSlashAtEnd(strPath);
    
    typedef odb::query<ODBView_Song> query;
    
    query objQuery = query::CODBFile::filename == strFileName && query::CODBPath::path == strPath;
    
    if (startOffset)
      objQuery = objQuery && query::CODBSong::startOffset == startOffset;
    
    std::shared_ptr<ODBView_Song> objSong(new ODBView_Song);
    if (!m_cdb.getDB()->query_one<ODBView_Song>(objQuery, *objSong))
      return false;
    
    song = GetSongFromODBObject(objSong->song);
    
    return true;
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "%s exception - %s", __FUNCTION__, e.what());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  
  return false;
}

int CMusicDatabase::GetAlbumIdByPath(const std::string& strPath)
{
  try
  {
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    
    
    odb::result<ODBView_Album_File_Paths> res(m_cdb.getDB()->query<ODBView_Album_File_Paths>(odb::query<ODBView_Album_File_Paths>::CODBPath::path == strPath));
    
    int idAlbum = -1; // If no album is found, or more than one album is found then -1 is returned
    if (res.begin() != res.end() && ++(res.begin()) == res.end())
      idAlbum = res.begin()->album->m_idAlbum;

    return idAlbum;
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "%s(%s) exception - %s", __FUNCTION__, strPath.c_str(), e.what());
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
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    typedef odb::query<ODBView_Song_Album_Artist> query;
    
    odb::result<ODBView_Song_Album_Artist> res(m_cdb.getDB()->query<ODBView_Song_Album_Artist>(query::CODBPerson::name.like(strArtist)
                                                                                               && query::CODBAlbum::album.like(strAlbum)
                                                                                               && query::CODBSong::title.like(strTitle)));
    
    if (res.begin() == res.end())
      return -1;
    
    return res.begin()->song->m_idSong;
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "%s (%s,%s,%s) exception - %s", __FUNCTION__, strArtist.c_str(),strAlbum.c_str(),strTitle.c_str(), e.what());
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
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    std::string strVariousArtists = g_localizeStrings.Get(340).c_str();
    
    typedef odb::query<ODBView_Song_Artists_Link> query;
    query objQuery;
    
    if (search.size() >= MIN_FULL_SEARCH_LENGTH)
    {
      objQuery = (query::CODBPerson::name.like(search+"%") || query::CODBPerson::name.like("% "+search+"%")) && query::CODBPerson::name != strVariousArtists;
    }
    else
    {
      objQuery = query::CODBPerson::name.like(search+"%") && query::CODBPerson::name != strVariousArtists;
    }
    
    odb::result<ODBView_Song_Artists_Link> res(m_cdb.getDB()->query<ODBView_Song_Artists_Link>(objQuery));
    
    if (res.begin() == res.end())
      return false;
    
    std::string artistLabel(g_localizeStrings.Get(557)); // Artist
    
    for (auto artist : res)
    {
      if (!artist.artist->m_person.load())
        continue;
      
      std::shared_ptr<CODBPerson> objArtist(artist.artist->m_person.get_eager());
      std::string path = StringUtils::Format("musicdb://artists/%lu/", objArtist->m_idPerson);
      CFileItemPtr pItem(new CFileItem(path, true));
      std::string label = StringUtils::Format("[%s] %s", artistLabel.c_str(), objArtist->m_name.c_str());
      pItem->SetLabel(label);
      label = StringUtils::Format("A %s", objArtist->m_name.c_str()); // sort label is stored in the title tag
      pItem->GetMusicInfoTag()->SetTitle(label);
      pItem->GetMusicInfoTag()->SetDatabaseId(objArtist->m_idPerson, MediaTypeArtist);
      artists.Add(pItem);
      m_pDS->next();
    }

    return true;
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "%s exception - %s", __FUNCTION__, e.what());
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
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    
    CMusicDbUrl baseUrl;
    if (!strBaseDir.empty() && !baseUrl.FromString(strBaseDir))
      return false;
    
    odb::query<ODBView_Song> query = odb::query<ODBView_Song>::CODBFile::playCount > 0;
    query += "ORDER BY "+odb::query<ODBView_Song>::CODBFile::playCount+" DESC LIMIT 100";
    
    odb::result<ODBView_Song> res(m_cdb.getDB()->query<ODBView_Song>(query));
    if (res.begin() == res.end())
      return true;
    
    for (auto song : res)
    {
      CFileItemPtr item(new CFileItem);
      GetFileItemFromODBObject(song.song, item.get(), baseUrl);
      items.Add(item);
    }

    return true;
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "%s exception - %s", __FUNCTION__, e.what());
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
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    
    typedef odb::query<ODBView_Album_File_Paths> query;
    
    //TODO: Needs to be verified that this query works
    odb::result<ODBView_Album_File_Paths> res(m_cdb.getDB()->query<ODBView_Album_File_Paths>("ORDER BY COUNT(" + query::CODBFile::playCount + ") DESC GROUP BY "+query::CODBAlbum::idAlbum+" LIMIT 100"));
    if (res.begin() == res.end())
      return true;
    
    for (auto resAlbum : res)
    {
      std::shared_ptr<CODBAlbum> objAlbum(resAlbum.album);
      if (!objAlbum->section_foreign.loaded())
        m_cdb.getDB()->load(*objAlbum, objAlbum->section_foreign);
      
      albums.push_back(GetAlbumFromODBObject(objAlbum));
      
      for (auto artist : objAlbum->m_artists)
      {
        if (artist.load())
          albums.back().artistCredits.push_back(GetArtistCreditFromODBObject(artist.get_eager()));
      }
    }
    
    return true;
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "%s exception - %s", __FUNCTION__, e.what());
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
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    
    CMusicDbUrl baseUrl;
    if (!strBaseDir.empty() && baseUrl.FromString(strBaseDir))
      return false;
    
    typedef odb::query<ODBView_Album_File_Paths> query;
    
    //TODO: Needs to be verified that this query works
    odb::result<ODBView_Album_File_Paths> res(m_cdb.getDB()->query<ODBView_Album_File_Paths>("ORDER BY COUNT(" + query::CODBFile::playCount + ") DESC GROUP BY "+query::CODBAlbum::idAlbum+" LIMIT 100"));
    if (res.begin() == res.end())
      return true;
    
    for (auto resAlbum : res)
    {
      odb::result<ODBView_Song> resSongs(m_cdb.getDB()->query<ODBView_Song>(odb::query<ODBView_Song>::CODBAlbum::idAlbum == resAlbum.album->m_idAlbum));
      if (res.begin() == res.end())
        continue;
      
      for (auto resSong : resSongs)
      {
        CFileItemPtr item(new CFileItem);
        GetFileItemFromODBObject(resSong.song, item.get(), baseUrl);
        items.Add(item);
      }
    }

    return true;
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "%s exception - %s", __FUNCTION__, e.what());
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
    
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
  
    typedef odb::query<ODBView_Album_File_Paths> query;
    query objQuery = query::CODBAlbum::releaseType == CAlbum::ReleaseTypeToString(CAlbum::Album);
    objQuery += "ORDER BY " + query::CODBFile::lastPlayed.ulong_date + " DESC LIMIT "+std::to_string(RECENTLY_PLAYED_LIMIT);
    
    //TODO: Needs to be verified that this query works
    odb::result<ODBView_Album_File_Paths> res(m_cdb.getDB()->query<ODBView_Album_File_Paths>(objQuery));
    if (res.begin() == res.end())
      return true;
    
    for (auto resAlbum : res)
    {
      std::shared_ptr<CODBAlbum> objAlbum(resAlbum.album);
      
      if (!objAlbum->section_foreign.loaded())
        m_cdb.getDB()->load(*objAlbum, objAlbum->section_foreign);
      
      albums.push_back(GetAlbumFromODBObject(objAlbum));
      
      for (auto artist : objAlbum->m_artists)
      {
        if (artist.load())
          albums.back().artistCredits.push_back(GetArtistCreditFromODBObject(artist.get_eager()));
      }
    }
    
    return true;
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "%s exception - %s", __FUNCTION__, e.what());
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
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    
    CMusicDbUrl baseUrl;
    if (!strBaseDir.empty() && baseUrl.FromString(strBaseDir))
      return false;
    
    typedef odb::query<ODBView_Album_File_Paths> query;
    
    query objQuery = query::CODBFile::lastPlayed.date != "";
    objQuery += "ORDER BY MAX(" + query::CODBFile::lastPlayed.ulong_date + ") DESC GROUP BY "+query::CODBAlbum::idAlbum+" LIMIT 100";
    
    //TODO: Needs to be verified that this query works
    odb::result<ODBView_Album_File_Paths> res(m_cdb.getDB()->query<ODBView_Album_File_Paths>(objQuery));
    if (res.begin() == res.end())
      return true;
    
    VECARTISTCREDITS artistCredits;
    
    for (auto resAlbum : res)
    {
      odb::result<ODBView_Song> resSongs(m_cdb.getDB()->query<ODBView_Song>(odb::query<ODBView_Song>::CODBAlbum::idAlbum == resAlbum.album->m_idAlbum));
      if (resSongs.begin() == resSongs.end())
        continue;
      
      for (auto resSong : resSongs)
      {
        std::shared_ptr<CODBSong> objSong(resSong.song);
        CFileItemPtr item(new CFileItem);
        GetFileItemFromODBObject(objSong, item.get(), baseUrl);
        items.Add(item);
        
        for (auto artist : objSong->m_artists)
        {
          if (artist.load() && artist->m_role.load())
          {
            if (artist->m_role->m_name == "artist")
              artistCredits.push_back(GetArtistCreditFromODBObject(artist.get_eager()));
            else
              items[items.Size() - 1]->GetMusicInfoTag()->AppendArtistRole(GetArtistRoleFromODBObject(artist.get_eager()));
          }
        }
        
        GetFileItemFromArtistCredits(artistCredits, items[items.Size() - 1].get());
        artistCredits.clear();
      }
    }

    return true;
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "%s exception - %s", __FUNCTION__, e.what());
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
    
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    
    typedef odb::query<ODBView_Album_File_Paths> query;
    query objQuery = query::CODBAlbum::releaseType == CAlbum::ReleaseTypeToString(CAlbum::Album);
    objQuery += "ORDER BY " + query::CODBFile::dateAdded.ulong_date + " DESC LIMIT "+std::to_string(limit ? limit : g_advancedSettings.m_iMusicLibraryRecentlyAddedItems);
    
    //TODO: Needs to be verified that this query works
    odb::result<ODBView_Album_File_Paths> res(m_cdb.getDB()->query<ODBView_Album_File_Paths>(objQuery));
    if (res.begin() == res.end())
      return true;
    
    for (auto resAlbum : res)
    {
      std::shared_ptr<CODBAlbum> objAlbum(resAlbum.album);
      
      if (!objAlbum->section_foreign.loaded())
        m_cdb.getDB()->load(*objAlbum, objAlbum->section_foreign);
      
      albums.push_back(GetAlbumFromODBObject(objAlbum));
      
      for (auto artist : objAlbum->m_artists)
      {
        if (artist.load())
          albums.back().artistCredits.push_back(GetArtistCreditFromODBObject(artist.get_eager()));
      }
    }
    
    return true;
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "%s exception - %s", __FUNCTION__, e.what());
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
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    
    CMusicDbUrl baseUrl;
    if (!strBaseDir.empty() && baseUrl.FromString(strBaseDir))
      return false;
    
    typedef odb::query<ODBView_Album_File_Paths> query;
    
    query objQuery = query::CODBFile::lastPlayed.date != "";
    objQuery += "ORDER BY MAX(" + query::CODBFile::dateAdded.ulong_date + ") DESC GROUP BY "+query::CODBAlbum::idAlbum+" LIMIT 100";
    
    //TODO: Needs to be verified that this query works
    odb::result<ODBView_Album_File_Paths> res(m_cdb.getDB()->query<ODBView_Album_File_Paths>(objQuery));
    if (res.begin() == res.end())
      return true;
    
    VECARTISTCREDITS artistCredits;
    
    for (auto resAlbum : res)
    {
      odb::result<ODBView_Song> resSongs(m_cdb.getDB()->query<ODBView_Song>(odb::query<ODBView_Song>::CODBAlbum::idAlbum == resAlbum.album->m_idAlbum));
      if (resSongs.begin() == resSongs.end())
        continue;
      
      for (auto resSong : resSongs)
      {
        std::shared_ptr<CODBSong> objSong(resSong.song);
        CFileItemPtr item(new CFileItem);
        GetFileItemFromODBObject(objSong, item.get(), baseUrl);
        items.Add(item);
        
        for (auto artist : objSong->m_artists)
        {
          if (artist.load() && artist->m_role.load())
          {
            if (artist->m_role->m_name == "artist")
              artistCredits.push_back(GetArtistCreditFromODBObject(artist.get_eager()));
            else
              items[items.Size() - 1]->GetMusicInfoTag()->AppendArtistRole(GetArtistRoleFromODBObject(artist.get_eager()));
          }
        }
        
        GetFileItemFromArtistCredits(artistCredits, items[items.Size() - 1].get());
        artistCredits.clear();
      }
    }
    
    return true;
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "%s exception - %s", __FUNCTION__, e.what());
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
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());

    std::shared_ptr<CODBSong> objSong(GetSongObjFromPath(item.GetPath()));
    
    if (!objSong->section_foreign.loaded())
      m_cdb.getDB()->load(*objSong, objSong->section_foreign);
    
    if (objSong->m_file.load())
    {
      objSong->m_file->m_playCount += 1;
      m_cdb.getDB()->update(*(objSong->m_file));
    }
    
    if(odb_transaction)
      odb_transaction->commit();
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "%s(%s) exception - %s", __FUNCTION__, item.GetPath().c_str(), e.what());
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
    
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    
    odb::result<ODBView_Song> res(m_cdb.getDB()->query<ODBView_Song>(odb::query<ODBView_Song>::CODBPath::path == strPath));
    if (res.begin() == res.end())
      return false;
    
    for ( auto song : res )
    {
      std::shared_ptr<CODBSong> objSong(song.song);
      
      if (!objSong->section_foreign.loaded())
        m_cdb.getDB()->load(*objSong, objSong->section_foreign);
      
      if (objSong->m_file.load())
        songs.insert(std::make_pair(objSong->m_file->m_filename, GetSongFromODBObject(objSong)));
    }
    
    return true;
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "%s(%s) failed", __FUNCTION__, strPath.c_str());
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
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    
    CMusicDbUrl baseUrl;
    if (!baseUrl.FromString("musicdb://songs/"))
      return false;
    
    odb::query<ODBView_Song> query = odb::query<ODBView_Song>::CODBSong::title.like(search+"%");
    
    if (search.size() >= MIN_FULL_SEARCH_LENGTH)
      query = query || odb::query<ODBView_Song>::CODBSong::title.like("% "+search+"%");
    
    query = query + "LIMIT 1000";
    
    odb::result<ODBView_Song> res(m_cdb.getDB()->query<ODBView_Song>(query));
    
    if (res.begin() == res.end())
      return false;
    
    std::string songLabel = g_localizeStrings.Get(179); // Song
    
    for (auto song : res)
    {
      CFileItemPtr item(new CFileItem);
      GetFileItemFromODBObject(song.song, item.get(), baseUrl);
      items.Add(item);
    }

    return true;
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "%s exception - %s", __FUNCTION__, e.what());
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
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    
    
    odb::query<ODBView_Album> query = odb::query<ODBView_Album>::CODBAlbum::album.like(search+"%");
    
    if (search.size() >= MIN_FULL_SEARCH_LENGTH)
      query = query || odb::query<ODBView_Album>::CODBAlbum::album.like("% "+search+"%");
    
    query = query + "LIMIT 1000";
    
    odb::result<ODBView_Album> res(m_cdb.getDB()->query<ODBView_Album>(query));
    
    if (res.begin() == res.end())
      return false;
    
    std::string albumLabel(g_localizeStrings.Get(558)); // Album
    
    for (auto objAlbum : res)
    {
      CAlbum album = GetAlbumFromODBObject(objAlbum.album);
      std::string path = StringUtils::Format("musicdb://albums/%ld/", album.idAlbum);
      CFileItemPtr pItem(new CFileItem(path, album));
      std::string label = StringUtils::Format("[%s] %s", albumLabel.c_str(), album.strAlbum.c_str());
      pItem->SetLabel(label);
      label = StringUtils::Format("B %s", album.strAlbum.c_str()); // sort label is stored in the title tag
      pItem->GetMusicInfoTag()->SetTitle(label);
      albums.Add(pItem);
    }

    return true;
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "%s exception - %s", __FUNCTION__, e.what());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}

bool CMusicDatabase::CleanupSongs()
{
  try
  {
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    
    int iLIMIT = 1000;
    for (int i=0;;i+=iLIMIT)
    {
      odb::result<CODBSong> res(m_cdb.getDB()->query<CODBSong>("LIMIT "+std::to_string(iLIMIT)+" OFFSET "+std::to_string(i)));

      if (res.begin() == res.end())
        break;
      
      for (auto objSong : res)
      {
        if (!objSong.section_foreign.loaded())
          m_cdb.getDB()->load(objSong, objSong.section_foreign);
        
        if (!objSong.m_file.load() || !objSong.m_file->m_path.load())
        {
          m_cdb.getDB()->erase(objSong);
          continue;
        }
        
        // get the full song path
        std::string strFileName = URIUtils::AddFileToFolder(objSong.m_file->m_path->m_path, objSong.m_file->m_filename);
        
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
        { // file no longer exists, so delete it
          m_cdb.getDB()->erase(objSong);
        }
      }
    }
    
    if(odb_transaction)
      odb_transaction->commit();

    return true;
  }
  catch(std::exception& e)
  {
    CLog::Log(LOGERROR, "Exception in CMusicDatabase::CleanupSongs() - %s", e.what());
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
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    
    // This must be run AFTER songs have been cleaned up
    // delete albums with no reference to songs
    
    odb::result<CODBAlbum> res(m_cdb.getDB()->query<CODBAlbum>());
    
    for (auto resAlbum : res)
    {
      //Check if it has at leas one song
      odb::result<ODBView_Song> resSongs(m_cdb.getDB()->query<ODBView_Song>(odb::query<ODBView_Song>::CODBAlbum::idAlbum == resAlbum.m_idAlbum));
      if (res.begin() == res.end())
        m_cdb.getDB()->erase(resAlbum);
    }
    
    if(odb_transaction)
      odb_transaction->commit();
    
    return true;
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "Exception in CMusicDatabase::CleanupAlbums() - %s", e.what());
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
    //TODO: As paths are not merged together, they have to be cleaned up centrally somewhere
    
    // needs to be done AFTER the songs and albums have been cleaned up.
    // we can happily delete any path that has no reference to a song
    // but we must keep all paths that have been scanned that may contain songs in subpaths

    // first create a temporary table of song paths
    /*m_pDS->exec("CREATE TEMPORARY TABLE songpaths (idPath integer, strPath varchar(512))\n");
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
    m_pDS->exec("drop table songpaths");*/
    return true;
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "Exception in CMusicDatabase::CleanupPaths() - %s", e.what());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "Exception in CMusicDatabase::CleanupPaths() or was aborted");
  }
  return false;
}

bool CMusicDatabase::InsideScannedPath(const std::string& path)
{
  try
  {
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    
    std::shared_ptr<CODBPath> objPath(new CODBPath);
    odb::query<CODBPath> query = "WHERE SUBSTR("+odb::query<CODBPath>::path+",1,"+std::to_string(path.size())+") = " + path;
    if (m_cdb.getDB()->query_one<CODBPath>(query))
      return true;
    
    return false;
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "%s exception - %s", __FUNCTION__, e.what());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  
  return false;
}

bool CMusicDatabase::CleanupArtists()
{
  try
  {
    //TODO: Needs to be done centrally, as they are used by all types of content
    // (nested queries by Bobbin007)
    // must be executed AFTER the song, album and their artist link tables are cleaned.
    // Don't delete [Missing] the missing artist tag artist

    // Create temp table to avoid 1442 trigger hell on mysql
    /*m_pDS->exec("CREATE TEMPORARY TABLE tmp_delartists (idArtist integer)");
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
    m_pDS->exec("DROP TABLE tmp_keep");*/

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
    //TODO: Needs to be done centrally, as they are used by all types of content
    // Cleanup orphaned genres (ie those that don't belong to a song or an album entry)
    // (nested queries by Bobbin007)
    // Must be executed AFTER the song, song_genre, album and album_genre tables have been cleaned.
    /*std::string strSQL = "delete from genre where idGenre not in (select idGenre from song_genre) and";
    strSQL += " idGenre not in (select idGenre from album_genre)";
    m_pDS->exec(strSQL);*/
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "Exception in CMusicDatabase::CleanupGenres() or was aborted");
  }
  return false;
}

bool CMusicDatabase::CleanupRoles()
{
  try
  {
    //TODO: Needs to be done centrally, as they are used by all types of content
    // Cleanup orphaned roles (ie those that don't belong to a song entry)
    // Must be executed AFTER the song, and song_artist tables have been cleaned.
    // Do not remove default role (ROLE_ARTIST)
    /*std::string strSQL = "DELETE FROM role WHERE idRole > 1 AND idRole NOT IN (SELECT idRole FROM song_artist)";
    m_pDS->exec(strSQL);*/
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
  if (!CleanupAlbums()) return false;
  if (!CleanupArtists()) return false;
  if (!CleanupGenres()) return false;
  if (!CleanupRoles()) return false;
  return true;
}

int CMusicDatabase::Cleanup(bool bShowProgress /* = true */)
{
  std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
  
  int ret = ERROR_OK;
  CGUIDialogProgress* pDlgProgress = NULL;
  unsigned int time = XbmcThreads::SystemClockMillis();
  CLog::Log(LOGNOTICE, "%s: Starting musicdatabase cleanup ..", __FUNCTION__);
  ANNOUNCEMENT::CAnnouncementManager::GetInstance().Announce(ANNOUNCEMENT::AudioLibrary, "xbmc", "OnCleanStarted");

  // first cleanup any songs with invalid paths
  if (bShowProgress)
  {
    pDlgProgress = g_windowManager.GetWindow<CGUIDialogProgress>();
    if (pDlgProgress)
    {
      pDlgProgress->SetHeading(CVariant{700});
      pDlgProgress->SetLine(0, CVariant{""});
      pDlgProgress->SetLine(1, CVariant{318});
      pDlgProgress->SetLine(2, CVariant{330});
      pDlgProgress->SetPercentage(0);
      pDlgProgress->Open();
      pDlgProgress->ShowProgressBar(true);
    }
  }
  if (!CleanupSongs())
  {
    ret = ERROR_REORG_SONGS;
    goto error;
  }
  // then the albums that are not linked to a song or to album, or whose path is removed
  if (pDlgProgress)
  {
    pDlgProgress->SetLine(1, CVariant{326});
    pDlgProgress->SetPercentage(20);
    pDlgProgress->Progress();
  }
  if (!CleanupAlbums())
  {
    ret = ERROR_REORG_ALBUM;
    goto error;
  }
  // now the paths
  if (pDlgProgress)
  {
    pDlgProgress->SetLine(1, CVariant{324});
    pDlgProgress->SetPercentage(40);
    pDlgProgress->Progress();
  }
  if (!CleanupPaths())
  {
    ret = ERROR_REORG_PATH;
    goto error;
  }
  // and finally artists + genres
  if (pDlgProgress)
  {
    pDlgProgress->SetLine(1, CVariant{320});
    pDlgProgress->SetPercentage(60);
    pDlgProgress->Progress();
  }
  if (!CleanupArtists())
  {
    ret = ERROR_REORG_ARTIST;
    goto error;
  }
  if (pDlgProgress)
  {
    pDlgProgress->SetLine(1, CVariant{322});
    pDlgProgress->SetPercentage(80);
    pDlgProgress->Progress();
  }
  if (!CleanupGenres())
  {
    ret = ERROR_REORG_GENRE;
    goto error;
  }
  if (!CleanupRoles())
  {
    ret = ERROR_REORG_ROLE;
    goto error;
  }
  // commit transaction
  if (pDlgProgress)
  {
    pDlgProgress->SetLine(1, CVariant{328});
    pDlgProgress->SetPercentage(90);
    pDlgProgress->Progress();
  }
  
  odb_transaction->commit();
  
  // and compress the database
  if (pDlgProgress)
  {
    pDlgProgress->SetLine(1, CVariant{331});
    pDlgProgress->SetPercentage(100);
    pDlgProgress->Progress();
    pDlgProgress->Close();
  }
  time = XbmcThreads::SystemClockMillis() - time;
  CLog::Log(LOGNOTICE, "%s: Cleaning musicdatabase done. Operation took %s", __FUNCTION__, StringUtils::SecondsToTimeString(time / 1000).c_str());
  ANNOUNCEMENT::CAnnouncementManager::GetInstance().Announce(ANNOUNCEMENT::AudioLibrary, "xbmc", "OnCleanFinished");

  return ERROR_OK;

error:
  ANNOUNCEMENT::CAnnouncementManager::GetInstance().Announce(ANNOUNCEMENT::AudioLibrary, "xbmc", "OnCleanFinished");
  return ret;
}

bool CMusicDatabase::LookupCDDBInfo(bool bRequery/*=false*/)
{
#ifdef HAS_DVD_DRIVE
  if (!CServiceBroker::GetSettings().GetBool(CSettings::SETTING_AUDIOCDS_USECDDB))
    return false;

  // check network connectivity
  if (!g_application.getNetwork().IsAvailable())
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
    CFile::Delete(URIUtils::AddFileToFolder(CProfilesManager::GetInstance().GetCDDBFolder(), strFile));
  }

  // Prepare cddb
  Xcddb cddb;
  cddb.setCacheDir(CProfilesManager::GetInstance().GetCDDBFolder());

  // Do we have to look for cddb information
  if (pCdInfo->HasCDDBInfo() && !cddb.isCDCached(pCdInfo))
  {
    CGUIDialogProgress* pDialogProgress = g_windowManager.GetWindow<CGUIDialogProgress>();
    CGUIDialogSelect *pDlgSelect = g_windowManager.GetWindow<CGUIDialogSelect>();

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
        CGUIDialogOK::ShowAndGetInput(CVariant{255}, CVariant{257}, CVariant{std::move(strErrorText)}, CVariant{0});
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
  if (!CDirectory::GetDirectory(CProfilesManager::GetInstance().GetCDDBFolder(), items, ".cddb", DIR_FLAG_NO_FILE_DIRS))
  {
    CGUIDialogOK::ShowAndGetInput(CVariant{313}, CVariant{426});
    return ;
  }
  // Show a selectdialog that the user can select the album to delete
  CGUIDialogSelect *pDlg = g_windowManager.GetWindow<CGUIDialogSelect>();
  if (pDlg)
  {
    pDlg->SetHeading(CVariant{g_localizeStrings.Get(181)});
    pDlg->Reset();

    std::map<ULONG, std::string> mapCDDBIds;
    for (int i = 0; i < items.Size(); ++i)
    {
      if (items[i]->m_bIsFolder)
        continue;

      std::string strFile = URIUtils::GetFileName(items[i]->GetPath());
      strFile.erase(strFile.size() - 5, 5);
      ULONG lDiscId = strtoul(strFile.c_str(), NULL, 16);
      Xcddb cddb;
      cddb.setCacheDir(CProfilesManager::GetInstance().GetCDDBFolder());

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
      mapCDDBIds.insert(std::pair<ULONG, std::string>(lDiscId, str));
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
        CFile::Delete(URIUtils::AddFileToFolder(CProfilesManager::GetInstance().GetCDDBFolder(), strFile));
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
    CGUIDialogOK::ShowAndGetInput(CVariant{189}, CVariant{14057});
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
        CGUIDialogOK::ShowAndGetInput(CVariant{313}, CVariant{iReturnString});
      }
    }
  }
}

bool CMusicDatabase::GetGenresNav(const std::string& strBaseDir, CFileItemList& items, const Filter &filter /* = Filter() */, bool countOnly /* = false */)
{
  try
  {
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    
    typedef odb::query<ODBView_Music_Genres> query;
    
    query objQuery;
    
    Filter extFilter = filter;
    CMusicDbUrl musicUrl;
    SortDescription sorting;
    if (!musicUrl.FromString(strBaseDir))
      return false;
    
    objQuery = GetODBFilterGenres<query>(musicUrl, extFilter, sorting);
    
    odb::result<ODBView_Music_Genres> res(m_cdb.getDB()->query<ODBView_Music_Genres>(objQuery));
    
    if (res.begin() == res.end())
      return true;
    
    unsigned int total = 0;
    
    for (auto objRes : res)
    {
      total++;
      
      if (countOnly)
        continue;
      
      CFileItemPtr pItem(new CFileItem(objRes.genre->m_name));
      pItem->GetMusicInfoTag()->SetGenre(objRes.genre->m_name);
      pItem->GetMusicInfoTag()->SetDatabaseId(objRes.genre->m_idGenre, "genre");
      
      CMusicDbUrl itemUrl = musicUrl;
      std::string strDir = StringUtils::Format("%lu/", objRes.genre->m_idGenre);
      itemUrl.AppendPath(strDir);
      pItem->SetPath(itemUrl.ToString());
      
      pItem->m_bIsFolder = true;
      items.Add(pItem);
    }
    
    if (countOnly)
    {
      CFileItemPtr pItem(new CFileItem());
      pItem->SetProperty("total", total);
      items.Add(pItem);
    }
    
    return true;
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "%s exception - %s", __FUNCTION__, e.what());
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
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    
    //TODO: Maybe need to add in the filter, but not yet sure for what values
    CMusicDbUrl musicUrl;
    if (!musicUrl.FromString(strBaseDir))
      return false;
    
    odb::result<ODBView_Album_Years> res(m_cdb.getDB()->query<ODBView_Album_Years>(odb::query<ODBView_Album_Years>::year != 0));
    
    if (res.begin() == res.end())
      return true;
    
    for (auto objRes : res)
    {
      CFileItemPtr pItem(new CFileItem(std::to_string(objRes.year)));
      SYSTEMTIME stTime;
      stTime.wYear = (WORD)objRes.year;
      pItem->GetMusicInfoTag()->SetReleaseDate(stTime);
      
      CMusicDbUrl itemUrl = musicUrl;
      std::string strDir = StringUtils::Format("%i/", objRes.year);
      itemUrl.AppendPath(strDir);
      pItem->SetPath(itemUrl.ToString());
      
      pItem->m_bIsFolder = true;
      items.Add(pItem);
    }

    return true;
  }
  catch (std::exception&e )
  {
    CLog::Log(LOGERROR, "%s exception - %s", __FUNCTION__, e.what());
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
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    
    CMusicDbUrl musicUrl;
    if (!musicUrl.FromString(strBaseDir))
      return false;
    
    //TODO: Maybe need to add in the filter, but not yet sure for what values
    
    odb::result<ODBView_Music_Roles> res(m_cdb.getDB()->query<ODBView_Music_Roles>());
    
    if (res.begin() == res.end())
      return true;
    
    for (auto objRes : res)
    {
      std::string labelValue = objRes.role->m_name;
      CFileItemPtr pItem(new CFileItem(labelValue));
      pItem->GetMusicInfoTag()->SetTitle(labelValue);
      pItem->GetMusicInfoTag()->SetDatabaseId(objRes.role->m_idRole, "role");
      CMusicDbUrl itemUrl = musicUrl;
      std::string strDir = StringUtils::Format("%lu/", objRes.role->m_idRole);
      itemUrl.AppendPath(strDir);
      itemUrl.AddOption("roleid", (int)objRes.role->m_idRole);
      pItem->SetPath(itemUrl.ToString());
      
      pItem->m_bIsFolder = true;
      items.Add(pItem);
    }

    return true;
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "%s exception - %s", __FUNCTION__, e.what());
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
  if (table.empty() || labelField.empty())
    return false;
  
  try
  {
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    
    //TODO: Maybe this can be replaced with a template, so we do not have to match the strings
    // But that leaves the problem on how to access the defined field from the result object via a template
    
    if (table == "albumview")
    {
      typedef odb::query<CODBAlbum> query;
      query objQuery;
      
      if (labelField == "albumview.strType")
      {
        objQuery = "GROUP BY "+query::type;
      }
      else if (labelField == "albumview.strLabel")
      {
        objQuery = "GROUP BY "+query::label;
      }
      else
      {
        CLog::Log(LOGERROR, "%s unkonwn %s field: %s", __FUNCTION__, table.c_str(), labelField.c_str());
        return false;
      }
      
      odb::result<CODBAlbum> res(m_cdb.getDB()->query<CODBAlbum>(objQuery));
      
      if (res.begin() == res.end())
        return false;
      
      unsigned int total = 0;
      for (auto objRes : res)
      {
        total++;
        
        if (countOnly)
          continue;
        
        std::string labelValue = "";
        
        if (labelField == "albumview.strType")
        {
          labelValue = objRes.m_type;
        }
        else if (labelField == "albumview.strLabel")
        {
          labelValue = objRes.m_label;
        }
        
        CFileItemPtr pItem(new CFileItem(labelValue));
        
        CMusicDbUrl musicUrl;
        musicUrl.FromString(strBaseDir);
        
        CMusicDbUrl itemUrl = musicUrl;
        std::string strDir = StringUtils::Format("%s/", labelValue.c_str());
        itemUrl.AppendPath(strDir);
        pItem->SetPath(itemUrl.ToString());
        
        pItem->m_bIsFolder = true;
        items.Add(pItem);
        
      }
      
      if (countOnly)
      {
        CFileItemPtr pItem(new CFileItem());
        pItem->SetProperty("total", total);
        items.Add(pItem);
      }
    }
    else
    {
      CLog::Log(LOGERROR, "%s unkonwn table: %s", __FUNCTION__, table.c_str());
      return false;
    }
    
    return true;
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "%s exception - %s", __FUNCTION__, e.what());
  }
  catch (...)
  {
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
    unsigned int time = XbmcThreads::SystemClockMillis();

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
    CLog::Log(LOGDEBUG,"Time to retrieve artists from dataset = %i", XbmcThreads::SystemClockMillis() - time);

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
  try
  {
    int total = 0;

    Filter extFilter = filter;
    CMusicDbUrl musicUrl;
    SortDescription sorting = sortDescription;
    if (!musicUrl.FromString(strBaseDir))
      return false;
    
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    typedef odb::query<ODBView_Song_Artists> query;
    
    query objFilterQuery = GetODBFilterArtists<query>(musicUrl, extFilter, sorting);
    
    //TODO: This needs to be done differently
    if (extFilter.where.find("albumview") != std::string::npos)
    {
      objFilterQuery = objFilterQuery && query::CODBPerson::idPerson == query::albumArist::idPerson;
    }
    
    objFilterQuery = objFilterQuery + SortUtils::SortODBArtistsQuery<query>(sortDescription);
    odb::result<ODBView_Song_Artists> res(m_cdb.getDB()->query<ODBView_Song_Artists>(objFilterQuery));
    if (res.begin() == res.end())
      return true;
    
    for (auto resObj : res)
    {
      total++;
      if (countOnly)
        continue;
      
      CArtist artist = GetArtistFromODBObject(resObj.artist);
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
    
    if (countOnly)
    {
      CFileItemPtr pItem(new CFileItem());
      pItem->SetProperty("total", total);
      items.Add(pItem);
      return true;
    }
    
    // store the total value of items as a property
    items.SetProperty("total", total);
    
    return true;
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "%s exception - %s", __FUNCTION__, e.what());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}

bool CMusicDatabase::GetAlbumFromSong(int idSong, CAlbum &album)
{
  try
  {
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    
    CODBSong objSong;
    if (!m_cdb.getDB()->query_one<CODBSong>(odb::query<CODBSong>::idSong == idSong, objSong))
      return false;
    
    if (!objSong.section_foreign.loaded())
      m_cdb.getDB()->load(objSong, objSong.section_foreign);
    
    if (!objSong.m_album.load())
      return false;
    
    album = GetAlbumFromODBObject(objSong.m_album.get_eager());
    
    return true;
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "%s exception - %s", __FUNCTION__, e.what());
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
  try
  {
    int total = 0;

    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    typedef odb::query<ODBView_Album> query;
    
    query objQuery;

    Filter extFilter = filter;
    CMusicDbUrl musicUrl;
    SortDescription sorting = sortDescription;
    if (!musicUrl.FromString(baseDir))
      return false;
    
    objQuery = GetODBFilterAlbums<query>(musicUrl, extFilter, sorting);
    
    objQuery = objQuery + SortUtils::SortODBAlbumQuery<query>(sortDescription);
    odb::result<ODBView_Album> res(m_cdb.getDB()->query<ODBView_Album>(objQuery));
    if (res.begin() == res.end())
      return true;
    
    for (auto resObj : res)
    {
      total++;
      if (countOnly)
        continue;
      
      CMusicDbUrl itemUrl = musicUrl;
      std::string path = StringUtils::Format("%lu/", resObj.album->m_idAlbum);
      itemUrl.AppendPath(path);
      
      CFileItemPtr pItem(new CFileItem(itemUrl.ToString(), GetAlbumFromODBObject(resObj.album)));
      pItem->SetIconImage("DefaultAlbumCover.png");
      items.Add(pItem);
    }
    
    if (countOnly)
    {
      CFileItemPtr pItem(new CFileItem());
      pItem->SetProperty("total", total);
      items.Add(pItem);
      return true;
    }

    // store the total value of items as a property
    items.SetProperty("total", total);

    return true;
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "%s (%s) exception - %s", __FUNCTION__, filter.where.c_str(), e.what());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, filter.where.c_str());
  }
  return false;
}

bool CMusicDatabase::GetAlbumsByWhere(const std::string &baseDir, const Filter &filter, VECALBUMS& albums, int& total, const SortDescription &sortDescription /* = SortDescription() */, bool countOnly /* = false */)
{
  albums.erase(albums.begin(), albums.end());
  
  try
  {
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    typedef odb::query<ODBView_Album> query;
    
    query objQuery;
    
    Filter extFilter = filter;
    CMusicDbUrl musicUrl;
    SortDescription sorting = sortDescription;
    if (!musicUrl.FromString(baseDir))
      return false;
    
    objQuery = GetODBFilterAlbums<query>(musicUrl, extFilter, sorting);
    
    odb::result<ODBView_Album> res(m_cdb.getDB()->query<ODBView_Album>(objQuery));
    if (res.begin() == res.end())
      return true;
   
    for (auto resObj : res)
    {
      std::shared_ptr<CODBAlbum> objAlbum = resObj.album;
      
      albums.emplace_back(GetAlbumFromODBObject(objAlbum));
      // Get artists
      if (!objAlbum->section_foreign.loaded())
        m_cdb.getDB()->load(*objAlbum, objAlbum->section_foreign);
      
      for (auto objPerson : objAlbum->m_artists)
      {
        albums.back().artistCredits.emplace_back(GetArtistCreditFromODBObject(objPerson.get_eager()));
      }
    }
    
    // Apply any limits to sorted albums
    if (sortDescription.sortBy != SortByNone && (sortDescription.limitStart > 0 || sortDescription.limitEnd > 0))
    {
      int limitEnd = sortDescription.limitEnd;
      if (sortDescription.limitStart > 0 && (size_t)sortDescription.limitStart < albums.size())
      {
        albums.erase(albums.begin(), albums.begin() + sortDescription.limitStart);
        limitEnd = sortDescription.limitEnd - sortDescription.limitStart;
      }
      if (limitEnd > 0 && (size_t)limitEnd < albums.size())
        albums.erase(albums.begin() + limitEnd, albums.end());
    }
    
    return true;
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "%s (%s) exception - %s", __FUNCTION__, filter.where.c_str(), e.what());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, filter.where.c_str());
  }
  return false;
}

bool CMusicDatabase::GetSongsFullByWhere(const std::string &baseDir, const Filter &filter, CFileItemList &items, const SortDescription &sortDescription /* = SortDescription() */, bool artistData /* = false*/, bool cueSheetData /* = true*/)
{
  try
  {
    unsigned int time = XbmcThreads::SystemClockMillis();
    int total = 0;
    
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    typedef odb::query<ODBView_Song> query;
    
    query objQuery;
    
    Filter extFilter = filter;
    CMusicDbUrl musicUrl;
    SortDescription sorting = sortDescription;
    if (!musicUrl.FromString(baseDir))
      return false;
    
    objQuery = GetODBFilterSongs<query>(musicUrl, extFilter, sorting);
    objQuery = objQuery + SortUtils::SortODBSongQuery<query>(sortDescription);
    
    odb::result<ODBView_Song> res(m_cdb.getDB()->query<ODBView_Song>(objQuery));
    if (res.begin() == res.end())
      return true;
    
    VECARTISTCREDITS artistCredits;
    
    for (auto resSong : res)
    {
      std::shared_ptr<CODBSong> objSong(resSong.song);
      CFileItemPtr item(new CFileItem);
      GetFileItemFromODBObject(objSong, item.get(), musicUrl);
      items.Add(item);
      
      for (auto artist : objSong->m_artists)
      {
        if (artist.load() && artist->m_role.load())
        {
          if (artist->m_role->m_name == "artist")
            artistCredits.push_back(GetArtistCreditFromODBObject(artist.get_eager()));
          else
            items[items.Size() - 1]->GetMusicInfoTag()->AppendArtistRole(GetArtistRoleFromODBObject(artist.get_eager()));
        }
      }
      
      GetFileItemFromArtistCredits(artistCredits, items[items.Size() - 1].get());
      artistCredits.clear();
    }
    
    // Store the total number of songs as a property
    items.SetProperty("total", total);
    
    if (cueSheetData)
    { // Load some info from embedded cuesheet if present (now only ReplayGain)
      CueInfoLoader cueLoader;
      for (int i = 0; i < items.Size(); ++i)
        cueLoader.Load(LoadCuesheet(items[i]->GetMusicInfoTag()->GetURL()), items[i]);
    }
    
    CLog::Log(LOGDEBUG, "%s(%s) - took %d ms", __FUNCTION__, filter.where.c_str(), XbmcThreads::SystemClockMillis() - time);
    return true;
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "%s(%s) exception - %s", __FUNCTION__, filter.where.c_str(), e.what());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%s) failed", __FUNCTION__, filter.where.c_str());
  }
  return false;
}

bool CMusicDatabase::GetSongsByWhere(const std::string &baseDir, const Filter &filter, CFileItemList &items, const SortDescription &sortDescription /* = SortDescription() */)
{
  try
  {
    unsigned int time = XbmcThreads::SystemClockMillis();
    int total = 0;
    
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    typedef odb::query<ODBView_Song> query;
    
    query objQuery;
    
    Filter extFilter = filter;
    CMusicDbUrl musicUrl;
    SortDescription sorting = sortDescription;
    if (!musicUrl.FromString(baseDir))
      return false;
    
    objQuery = GetODBFilterSongs<query>(musicUrl, extFilter, sorting);
    objQuery = objQuery + SortUtils::SortODBSongQuery<query>(sortDescription);
    
    odb::result<ODBView_Song> res(m_cdb.getDB()->query<ODBView_Song>(objQuery));
    if (res.begin() == res.end())
      return true;
    
    VECARTISTCREDITS artistCredits;
    
    for (auto resSong : res)
    {
      std::shared_ptr<CODBSong> objSong(resSong.song);
      CFileItemPtr item(new CFileItem);
      GetFileItemFromODBObject(objSong, item.get(), musicUrl);
      items.Add(item);
      
      for (auto artist : objSong->m_artists)
      {
        if (artist.load() && artist->m_role.load())
        {
          if (artist->m_role->m_name == "artist")
            artistCredits.push_back(GetArtistCreditFromODBObject(artist.get_eager()));
          else
            items[items.Size() - 1]->GetMusicInfoTag()->AppendArtistRole(GetArtistRoleFromODBObject(artist.get_eager()));
        }
      }
      
      GetFileItemFromArtistCredits(artistCredits, items[items.Size() - 1].get());
      artistCredits.clear();
    }
    
    // Store the total number of songs as a property
    items.SetProperty("total", total);
    
    // Load some info from embedded cuesheet if present (now only ReplayGain)
    CueInfoLoader cueLoader;
    for (int i = 0; i < items.Size(); ++i)
      cueLoader.Load(LoadCuesheet(items[i]->GetMusicInfoTag()->GetURL()), items[i]);
    
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

  // Set the version of tag scanning required. 
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
  return 62;
}

int CMusicDatabase::GetMusicNeedsTagScan()
{
  try
  {
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    
    CODBVersionTagScan objVersionTagScan;
    if (!m_cdb.getDB()->query_one<CODBVersionTagScan>(odb::query<CODBVersionTagScan>(), objVersionTagScan))
      return -1;
    
    if (objVersionTagScan.m_idVersion < objVersionTagScan.m_NeedsScan)
      return objVersionTagScan.m_idVersion;
    else
      return 0;
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "%s exception - %s", __FUNCTION__, e.what());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return -1;
}

void CMusicDatabase::SetMusicNeedsTagScan(int version)
{
  try
  {
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    
    CODBVersionTagScan objVersionTagScan;
    bool exists = false;
    if (m_cdb.getDB()->query_one<CODBVersionTagScan>(odb::query<CODBVersionTagScan>(), objVersionTagScan))
      exists = true;
    
    objVersionTagScan.m_NeedsScan = version;
    if (exists)
      m_cdb.getDB()->update(objVersionTagScan);
    else
    {
      objVersionTagScan.m_idVersion = 0;
      m_cdb.getDB()->persist(objVersionTagScan);
    }
    
    if(odb_transaction)
      odb_transaction->commit();
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "%s exception - %s", __FUNCTION__, e.what());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
}

void CMusicDatabase::SetMusicTagScanVersion(int version /* = 0 */)
{
  try
  {
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    
    CODBVersionTagScan objVersionTagScan;
    bool exists = false;
    if (m_cdb.getDB()->query_one<CODBVersionTagScan>(odb::query<CODBVersionTagScan>(), objVersionTagScan))
      exists = true;;
    
    if (version == 0)
      objVersionTagScan.m_idVersion = GetSchemaVersion();
    else
      objVersionTagScan.m_idVersion = version;
    
    if ( exists)
      m_cdb.getDB()->update(objVersionTagScan);
    else
    {
      objVersionTagScan.m_NeedsScan = 0;
      m_cdb.getDB()->persist(objVersionTagScan);
    }
    
    if(odb_transaction)
      odb_transaction->commit();
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "%s exception - %s", __FUNCTION__, e.what());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
}

unsigned int CMusicDatabase::GetSongIDs(odb::query<ODBView_Song>& query, std::vector<std::pair<int,int> > &songIDs)
{
  try
  {
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    
    odb::result<ODBView_Song> res(m_cdb.getDB()->query<ODBView_Song>(query));
    
    if (res.begin() == res.end())
      return 0;
    
    for (auto objSong : res)
    {
      songIDs.push_back(std::make_pair<int,int>(1, static_cast<int>(objSong.song->m_idSong)));
    }

    return songIDs.size();
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "%s exception - %s", __FUNCTION__, e.what());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return 0;
}

int CMusicDatabase::GetSongsCount(odb::query<ODBView_Song_Count> query)
{
  try
  {
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    
    ODBView_Song_Count objCount;
    
    if (!m_cdb.getDB()->query_one<ODBView_Song_Count>(query, objCount))
      return 0;
    
    return objCount.count;
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "%s exception - %s", __FUNCTION__, e.what());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return 0;
}

bool CMusicDatabase::GetAlbumPath(int idAlbum, std::string& path)
{
  try
  {
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    typedef odb::query<ODBView_Song_Album_File_Path> query;
    
    path.clear();
    
    odb::result<ODBView_Song_Album_File_Path> res(m_cdb.getDB()->query<ODBView_Song_Album_File_Path>(query::CODBAlbum::idAlbum == idAlbum));
    for (odb::result<ODBView_Song_Album_File_Path>::iterator i = res.begin(); i != res.end(); i++)
    {
      // if this returns more than one path, we just grab the first one.  It's just for determining where to obtain + place
      // a local thumbnail
      path = i->path->m_path;
      return true;
    }
    
    return false;
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "%s(%i) exception - %s", __FUNCTION__, idAlbum, e.what());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%i) failed", __FUNCTION__, idAlbum);
  }

  return false;
}

bool CMusicDatabase::SaveAlbumThumb(int idAlbum, const std::string& strThumb)
{
  try
  {
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    
    SetArtForItem(idAlbum, MediaTypeAlbum, "thumb", strThumb);
    
    odb::result<ODBView_Song_Album_Art> res(m_cdb.getDB()->query<ODBView_Song_Album_Art>(odb::query<ODBView_Song_Album_Art>::CODBAlbum::idAlbum == idAlbum
                                                                                         && odb::query<ODBView_Song_Album_Art>::CODBArt::type == "thumb"));
    for (odb::result<ODBView_Song_Album_Art>::iterator i = res.begin(); i != res.end(); i++)
    {
      i->art->m_url = "-";
      m_cdb.getDB()->update(i->art);
    }
    
    if(odb_transaction)
      odb_transaction->commit();
    
    return true;
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "%s exception - %s", __FUNCTION__, e.what());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  
  return false;
}

bool CMusicDatabase::GetArtistPath(int idArtist, std::string &basePath)
{
  try
  {
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    
    odb::result<ODBView_Song_Artist_Paths> res(m_cdb.getDB()->query<ODBView_Song_Artist_Paths>(odb::query<ODBView_Song_Artist_Paths>::CODBPerson::idPerson == idArtist));
    
    if (res.begin() == res.end())
      return true;
    
    if ( ++(res.begin()) == res.end())
    {
      URIUtils::GetParentPath(res.begin()->path->m_path, basePath);
      return true;
    }
    
    // find the common path (if any) to these albums
    basePath.clear();
    
    for (auto resObj : res)
    {
      if (basePath.empty())
        basePath = resObj.path->m_path;
      else
        URIUtils::GetCommonPath(basePath, resObj.path->m_path);
    }

    return true;
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "%s exception - %s", __FUNCTION__, e.what());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}

int CMusicDatabase::GetArtistByName(const std::string& strArtist)
{
  try
  {
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    
    //TODO: We may have to only search in music artists here instead of all
    odb::result<CODBPerson> res(m_cdb.getDB()->query<CODBPerson>(odb::query<CODBPerson>::name.like(strArtist)));
    
    if (res.begin() == res.end())
      return -1;
    
    return res.begin()->m_idPerson;
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "%s exception - %s", __FUNCTION__, e.what());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return -1;
}

int CMusicDatabase::GetAlbumByName(const std::string& strAlbum, const std::string& strArtist)
{
  try
  {
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    
    odb::result<CODBAlbum> res(m_cdb.getDB()->query<CODBAlbum>(odb::query<CODBAlbum>::album.like(strArtist)));
    
    if (res.begin() == res.end())
      return -1;
    
    return res.begin()->m_idAlbum;
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "%s exception - %s", __FUNCTION__, e.what());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return -1;
}

int CMusicDatabase::GetAlbumByName(const std::string& strAlbum, const std::vector<std::string>& artist)
{
  return GetAlbumByName(strAlbum, StringUtils::Join(artist, g_advancedSettings.m_musicItemSeparator));
}

std::string CMusicDatabase::GetGenreById(int id)
{
  try
  {
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    
    odb::result<CODBGenre> res(m_cdb.getDB()->query<CODBGenre>(odb::query<CODBGenre>::idGenre == id));
    
    if (res.begin() == res.end())
      return "";
    
    return res.begin()->m_name;
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "%s exception - %s", __FUNCTION__, e.what());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return "";
}

std::string CMusicDatabase::GetArtistById(int id)
{
  try
  {
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    
    odb::result<CODBPerson> res(m_cdb.getDB()->query<CODBPerson>(odb::query<CODBPerson>::idPerson == id));
    
    if (res.begin() == res.end())
      return "";
    
    return res.begin()->m_name;
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "%s exception - %s", __FUNCTION__, e.what());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return "";
}

std::string CMusicDatabase::GetRoleById(int id)
{
  try
  {
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    
    odb::result<CODBRole> res(m_cdb.getDB()->query<CODBRole>(odb::query<CODBRole>::idRole == id));
    
    if (res.begin() == res.end())
      return "";
    
    return res.begin()->m_name;
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "%s exception - %s", __FUNCTION__, e.what());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return "";
}

std::string CMusicDatabase::GetAlbumById(int id)
{
  try
  {
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    
    odb::result<CODBAlbum> res(m_cdb.getDB()->query<CODBAlbum>(odb::query<CODBAlbum>::idAlbum == id));
    
    if (res.begin() == res.end())
      return "";
    
    return res.begin()->m_album;
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "%s exception - %s", __FUNCTION__, e.what());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return "";
}

int CMusicDatabase::GetGenreByName(const std::string& strGenre)
{
  try
  {
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    
    odb::result<CODBGenre> res(m_cdb.getDB()->query<CODBGenre>(odb::query<CODBGenre>::name == strGenre));
    
    if (res.begin() == res.end())
      return -1;
    
    return res.begin()->m_idGenre;
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "%s exception - %s", __FUNCTION__, e.what());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return -1;
}

bool CMusicDatabase::GetRandomSong(CFileItem* item, int& idSong, odb::query<ODBView_Song> objQuery)
{
  try
  {
    idSong = -1;
    
    // Get a random song that matches filter criteria (which may exclude some songs)

    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    
    // Try to avoid Random(), as it is slow with a large number of tracks
    
    //First get get total number of songs
    ODBView_Song_Count objCount;
    if (!m_cdb.getDB()->query_one<ODBView_Song_Count>(objQuery, objCount))
      return false;
    
    //Then generate a number in range of the total songs
    unsigned int total = objCount.count;
    unsigned int randVal = rand() % total;
    
    ODBView_Song objSong;
    objQuery = objQuery + "LIMIT 1 OFFSET "+std::to_string(randVal);
    if (!m_cdb.getDB()->query_one<ODBView_Song>(objQuery, objSong))
      return false;
    
    
    std::string baseDir = StringUtils::Format("musicdb://songs/?songid=%lu", objSong.song->m_idSong);
    CMusicDbUrl musicUrl;
    if (!musicUrl.FromString(baseDir))
      return false;
   
    GetFileItemFromODBObject(objSong.song, item, musicUrl);
    return true;
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "%s exception - %s", __FUNCTION__, e.what());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
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
  try
  {
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    
    ODBView_Album_Count objCount;
    if (!m_cdb.getDB()->query_one<ODBView_Album_Count>(odb::query<ODBView_Album_Count>(), objCount))
      return 0;
    
    return objCount.count;
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "%s exception - %s", __FUNCTION__, e.what());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return 0;
}

int CMusicDatabase::GetSinglesCount()
{
  odb::query<ODBView_Song_Count> query = odb::query<ODBView_Song_Count>::CODBAlbum::releaseType == CAlbum::ReleaseTypeToString(CAlbum::Single);
  return GetSongsCount(query);
}

int CMusicDatabase::GetArtistCountForRole(int role)
{
  try
  {
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    
    ODBView_Person_Count objCount;
    if (!m_cdb.getDB()->query_one<ODBView_Person_Count>(odb::query<ODBView_Person_Count>::CODBRole::idRole == role, objCount))
      return 0;
    
    return objCount.count;
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "%s exception - %s", __FUNCTION__, e.what());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return 0;
}

int CMusicDatabase::GetArtistCountForRole(const std::string& strRole)
{
  try
  {
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    
    ODBView_Person_Count objCount;
    if (!m_cdb.getDB()->query_one<ODBView_Person_Count>(odb::query<ODBView_Person_Count>::CODBRole::name == strRole, objCount))
      return 0;
    
    return objCount.count;
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "%s exception - %s", __FUNCTION__, e.what());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return 0;
}

bool CMusicDatabase::SetPathHash(const std::string &path, const std::string &hash)
{
  try
  {
    if (hash.empty())
    { // this is an empty folder - we need only add it to the path table
      // if the path actually exists
      if (!CDirectory::Exists(path))
        return false;
    }
    
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    
    std::shared_ptr<CODBPath> objPath = AddPath(path);
    if (!objPath) return false;
    
    objPath->m_hash = hash;
    
    m_cdb.getDB()->update(objPath);
    
    if(odb_transaction)
      odb_transaction->commit();
    
    return true;
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "%s (%s, %s) exception - %s", __FUNCTION__, path.c_str(), hash.c_str(), e.what());
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
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    
    std::shared_ptr<CODBPath> objPath = AddPath(path);
    if (!objPath) return false;
    
    hash = objPath->m_hash;
    
    if(odb_transaction)
      odb_transaction->commit();
    
    return true;
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "%s (%s) exception - %s", __FUNCTION__, path.c_str(), e.what());
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
  try
  {
    if (!URIUtils::HasSlashAtEnd(path))
      URIUtils::AddSlashAtEnd(path);
    
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    typedef odb::query<ODBView_Song> query;
    query objQuery;
    
    if (exact)
      objQuery = query::CODBPath::path == path;
    else
      objQuery = objQuery + "SUBSTR("+query::CODBPath::path+",1,"+std::to_string(StringUtils::utf8_strlen(path.c_str()))+") = "+path;
    
    odb::result<ODBView_Song> res(m_cdb.getDB()->query<ODBView_Song>(objQuery));
    if (res.begin() == res.end())
      return false;
    
    for (auto resObj : res)
    {
      CSong song = GetSongFromODBObject(resObj.song);
      song.strThumb = GetArtForItem(song.idSong, MediaTypeSong, "thumb");
      songs.insert(std::make_pair(song.strFileName, song));
      
      AnnounceRemove(MediaTypeSong, song.idSong);
      
      m_cdb.getDB()->erase(resObj.song);
    }
    
    // and remove the path as well (it'll be re-added later on with the new hash if it's non-empty)
    odb::query<CODBPath> objQueryPath;
    if (exact)
      objQueryPath = odb::query<CODBPath>::path == path;
    else
      objQueryPath = objQueryPath + "SUBSTR("+odb::query<CODBPath>::path+",1,"+std::to_string(StringUtils::utf8_strlen(path.c_str()))+") = "+path;
    
    m_cdb.getDB()->erase_query<CODBPath>(objQueryPath);
    
    if(odb_transaction)
      odb_transaction->commit();
    
    return true;
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "%s (%s) exception - %s", __FUNCTION__, path.c_str(), e.what());
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
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    
    odb::result<ODBView_Song_Paths> res(m_cdb.getDB()->query<ODBView_Song_Paths>());
    
    if (res.begin() == res.end())
      return true;
    
    for (auto resObj : res)
    {
      paths.insert(resObj.path->m_path);
    }
    
    return true;
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "%s exception - %s", __FUNCTION__, e.what());
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
    if (filePath.empty())
      return false;
    
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    
    std::shared_ptr<CODBSong> objSong(GetSongObjFromPath(filePath));
    if (!objSong)
      return false;
    
    objSong->m_userrating = userrating;
    m_cdb.getDB()->update(objSong);
    
    if(odb_transaction)
      odb_transaction->commit();
    
    return true;
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "%s (%s,%i) exception - %s", __FUNCTION__, filePath.c_str(), userrating, e.what());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s,%i) failed", __FUNCTION__, filePath.c_str(), userrating);
  }
  return false;
}

bool CMusicDatabase::SetAlbumUserrating(const int idAlbum, int userrating)
{
  try
  {
    if (-1 == idAlbum)
      return false;
    
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    
    std::shared_ptr<CODBAlbum> objAlbum = GetODBAlbum(idAlbum);
    if (!objAlbum)
      return false;
    
    objAlbum->m_userrating = userrating;
    m_cdb.getDB()->update(objAlbum);
    
    if(odb_transaction)
      odb_transaction->commit();
    
    return true;
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "%s (%i,%i) exception - %s", __FUNCTION__, idAlbum, userrating, e.what());
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
    
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    
    std::shared_ptr<CODBSong> objSong(GetSongObjFromPath(filePath));
    if (!objSong)
      return false;
    
    objSong->m_votes = votes;
    m_cdb.getDB()->update(objSong);
    
    if(odb_transaction)
      odb_transaction->commit();
    
    return true;
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "%s (%s,%i) exception - %s", __FUNCTION__, filePath.c_str(), votes, e.what());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s,%i) failed", __FUNCTION__, filePath.c_str(), votes);
  }
  return false;
}

std::shared_ptr<CODBSong> CMusicDatabase::GetSongObjFromPath(const std::string &filePath)
{
  try
  {
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    
    // grab the where string to identify the song id
    CURL url(filePath);
    if (url.IsProtocol("musicdb"))
    {
      std::string strFile=URIUtils::GetFileName(filePath);
      URIUtils::RemoveExtension(strFile);
      
      std::shared_ptr<CODBSong> objSong(new CODBSong);
      if (!m_cdb.getDB()->query_one<CODBSong>(odb::query<CODBSong>::idSong == atol(strFile.c_str()), *objSong))
        return nullptr;
      
      return objSong;
    }
    
    // hit the db
    std::string strPath, strFileName;
    URIUtils::Split(filePath, strPath, strFileName);
    URIUtils::AddSlashAtEnd(strPath);
    
    typedef odb::query<ODBView_Song_Album_File_Path> query;
    
    std::shared_ptr<ODBView_Song_Album_File_Path> objSong(new ODBView_Song_Album_File_Path);
    if (m_cdb.getDB()->query_one<ODBView_Song_Album_File_Path>(query::CODBFile::filename == strFileName
                                                                && query::CODBPath::path == strPath, *objSong))
    {
      return objSong->song;
    }
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "%s (%s) exception - %s", __FUNCTION__, filePath.c_str(), e.what());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, filePath.c_str());
  }
  return nullptr;
}

bool CMusicDatabase::SetScraperForPath(const std::string& strPath, const ADDON::ScraperPtr& scraper)
{
  try
  {
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    
    CODBPath objPath;
    
    bool exists = false;
    if (m_cdb.getDB()->query_one<CODBPath>(odb::query<CODBPath>::path == strPath, objPath))
    {
      exists = true;
    }
    
    objPath.m_path = strPath;
    objPath.m_scraper = scraper->ID();
    objPath.m_content = ADDON::TranslateContent(scraper->Content());
    objPath.m_settings = scraper->GetPathSettings();
    
    if (exists)
      m_cdb.getDB()->update(objPath);
    else
      m_cdb.getDB()->persist(objPath);
    
    return true;
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "%s - (%s) exception - %s", __FUNCTION__, strPath.c_str(), e.what());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s - (%s) failed", __FUNCTION__, strPath.c_str());
  }
  return false;
}

bool CMusicDatabase::GetScraperForPath(const std::string& strPath, ADDON::ScraperPtr& info, const ADDON::TYPE &type)
{
  try
  {
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    
    CODBPath objPath;
    
    bool found = false;
    if (!m_cdb.getDB()->query_one<CODBPath>(odb::query<CODBPath>::path == strPath, objPath))
    {
      CQueryParams params;
      CDirectoryNode::GetDatabaseInfo(strPath, params);
      
      if (params.GetGenreId() != -1) // check genre
      {
        if (m_cdb.getDB()->query_one<CODBPath>(odb::query<CODBPath>::path == "musicdb://genres/"+std::to_string(params.GetGenreId())+"/", objPath))
          found = true;
      }
      if (!found && params.GetAlbumId() != -1) // check album
      {
        if (!m_cdb.getDB()->query_one<CODBPath>(odb::query<CODBPath>::path == "musicdb://albums/"+std::to_string(params.GetAlbumId())+"/", objPath))
        {
          // general albums setting
          if (m_cdb.getDB()->query_one<CODBPath>(odb::query<CODBPath>::path == "musicdb://albums/", objPath))
            found = true;
        }
        else
          found = true;
      }
      if (!found && params.GetArtistId() != -1) // check artist
      {
        if (!m_cdb.getDB()->query_one<CODBPath>(odb::query<CODBPath>::path == "musicdb://artists/"+std::to_string(params.GetArtistId())+"/", objPath))
        {
          // general artist setting
          if (m_cdb.getDB()->query_one<CODBPath>(odb::query<CODBPath>::path == "musicdb://artists/", objPath))
            found = true;
        }
        else
          found = true;
      }
    }
    else
      found = true;

    if (found)
    { // try and ascertain scraper for this path
      CONTENT_TYPE content = ADDON::TranslateContent(objPath.m_content);
      std::string scraperUUID = m_pDS->fv("content.strScraperPath").get_asString();

      if (content != CONTENT_NONE)
      { // content set, use pre configured or default scraper
        ADDON::AddonPtr addon;
        if (!scraperUUID.empty() && ADDON::CAddonMgr::GetInstance().GetAddon(scraperUUID, addon) && addon)
        {
          info = std::dynamic_pointer_cast<ADDON::CScraper>(addon);
          if (!info)
            return false;
          // store this path's settings
          info->SetPathSettings(content, objPath.m_settings);
        }
      }
      else
      { // use default scraper of the requested type
        ADDON::AddonPtr defaultScraper;
        if (ADDON::CAddonSystemSettings::GetInstance().GetActive(type, defaultScraper))
        {
          info = std::dynamic_pointer_cast<ADDON::CScraper>(defaultScraper);
        }
      }
    }
    m_pDS->close();

    if (!info)
    { // use default music scraper instead
      ADDON::AddonPtr addon;
      if(ADDON::CAddonSystemSettings::GetInstance().GetActive(type, addon))
      {
        info = std::dynamic_pointer_cast<ADDON::CScraper>(addon);
        return info != NULL;
      }
      else
        return false;
    }

    return true;
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "%s -(%s) exception - %s", __FUNCTION__, strPath.c_str(), e.what());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s -(%s) failed", __FUNCTION__, strPath.c_str());
  }
  return false;
}

bool CMusicDatabase::ScraperInUse(const std::string &scraperID)
{
  try
  {
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    
    odb::result<CODBPath> res(m_cdb.getDB()->query<CODBPath>(odb::query<CODBPath>::scraper == scraperID));
    
    if (res.begin() == res.end())
      return false;
    
    return true;
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "%s(%s) exception - %s", __FUNCTION__, scraperID.c_str(), e.what());
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
  else if (StringUtils::EqualsNoCase(itemType, "years"))
    return GetYearsNav(strBaseDir, items, filter);
  else if (StringUtils::EqualsNoCase(itemType, "roles"))
    return GetRolesNav(strBaseDir, items, filter);
  else if (StringUtils::EqualsNoCase(itemType, "artists"))
    return GetArtistsNav(strBaseDir, items, !CServiceBroker::GetSettings().GetBool(CSettings::SETTING_MUSICLIBRARY_SHOWCOMPILATIONARTISTS), -1, -1, -1, filter, sortDescription);
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

void CMusicDatabase::ExportToXML(const std::string &xmlFile, bool singleFile, bool images, bool overwrite)
{
  //TODO: Needs to be converted to ODB
  int iFailCount = 0;
  CGUIDialogProgress *progress=NULL;
  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;
    if (NULL == m_pDS2.get()) return;

    // find all albums
    std::vector<int> albumIds;
    std::string sql = "select idAlbum FROM album WHERE lastScraped IS NOT NULL";
    m_pDS->query(sql);

    int total = m_pDS->num_rows();
    int current = 0;

    albumIds.reserve(total);
    while (!m_pDS->eof())
    {
      albumIds.push_back(m_pDS->fv("idAlbum").get_asInt());
      m_pDS->next();
    }
    m_pDS->close();

    progress = g_windowManager.GetWindow<CGUIDialogProgress>();
    if (progress)
    {
      progress->SetHeading(CVariant{20196});
      progress->SetLine(0, CVariant{650});
      progress->SetLine(1, CVariant{""});
      progress->SetLine(2, CVariant{""});
      progress->SetPercentage(0);
      progress->Open();
      progress->ShowProgressBar(true);
    }

    // create our xml document
    CXBMCTinyXML xmlDoc;
    TiXmlDeclaration decl("1.0", "UTF-8", "yes");
    xmlDoc.InsertEndChild(decl);
    TiXmlNode *pMain = NULL;
    if (!singleFile)
      pMain = &xmlDoc;
    else
    {
      TiXmlElement xmlMainElement("musicdb");
      pMain = xmlDoc.InsertEndChild(xmlMainElement);
    }
    for (const auto &albumId : albumIds)
    {
      CAlbum album;
      GetAlbum(albumId, album);
      std::string strPath;
      GetAlbumPath(albumId, strPath);
      album.Save(pMain, "album", strPath);
      if (!singleFile)
      {
        if (!CDirectory::Exists(strPath))
          CLog::Log(LOGDEBUG, "%s - Not exporting item %s as it does not exist", __FUNCTION__, strPath.c_str());
        else
        {
          std::string nfoFile = URIUtils::AddFileToFolder(strPath, "album.nfo");
          if (overwrite || !CFile::Exists(nfoFile))
          {
            if (!xmlDoc.SaveFile(nfoFile))
            {
              CLog::Log(LOGERROR, "%s: Album nfo export failed! ('%s')", __FUNCTION__, nfoFile.c_str());
              CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Error, g_localizeStrings.Get(20302), nfoFile);
              iFailCount++;
            }
          }

          if (images)
          {
            std::string thumb = GetArtForItem(album.idAlbum, MediaTypeAlbum, "thumb");
            std::string imagePath = URIUtils::AddFileToFolder(strPath, "folder.jpg");
            if (!thumb.empty() && (overwrite || !CFile::Exists(imagePath)))
              CTextureCache::GetInstance().Export(thumb, imagePath);
          }
          xmlDoc.Clear();
          TiXmlDeclaration decl("1.0", "UTF-8", "yes");
          xmlDoc.InsertEndChild(decl);
        }
      }

      if ((current % 50) == 0 && progress)
      {
        progress->SetLine(1, CVariant{album.strAlbum});
        progress->SetPercentage(current * 100 / total);
        progress->Progress();
        if (progress->IsCanceled())
        {
          progress->Close();
          m_pDS->close();
          return;
        }
      }
      current++;
    }

    // find all artists
    std::vector<int> artistIds;
    std::string artistSQL = "SELECT idArtist FROM artist where lastScraped IS NOT NULL";
    m_pDS->query(artistSQL);
    total = m_pDS->num_rows();
    current = 0;
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
      GetArtist(artistId, artist);
      std::string strPath;
      GetArtistPath(artist.idArtist,strPath);
      artist.Save(pMain, "artist", strPath);

      std::map<std::string, std::string> artwork;
      if (GetArtForItem(artist.idArtist, MediaTypeArtist, artwork) && singleFile)
      { // append to the XML
        TiXmlElement additionalNode("art");
        for (const auto &i : artwork)
          XMLUtils::SetString(&additionalNode, i.first.c_str(), i.second);
        pMain->LastChild()->InsertEndChild(additionalNode);
      }
      if (!singleFile)
      {
        if (!CDirectory::Exists(strPath))
          CLog::Log(LOGDEBUG, "%s - Not exporting item %s as it does not exist", __FUNCTION__, strPath.c_str());
        else
        {
          std::string nfoFile = URIUtils::AddFileToFolder(strPath, "artist.nfo");
          if (overwrite || !CFile::Exists(nfoFile))
          {
            if (!xmlDoc.SaveFile(nfoFile))
            {
              CLog::Log(LOGERROR, "%s: Artist nfo export failed! ('%s')", __FUNCTION__, nfoFile.c_str());
              CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Error, g_localizeStrings.Get(20302), nfoFile);
              iFailCount++;
            }
          }

          if (images && !artwork.empty())
          {
            std::string savedThumb = URIUtils::AddFileToFolder(strPath,"folder.jpg");
            std::string savedFanart = URIUtils::AddFileToFolder(strPath,"fanart.jpg");
            if (artwork.find("thumb") != artwork.end() && (overwrite || !CFile::Exists(savedThumb)))
              CTextureCache::GetInstance().Export(artwork["thumb"], savedThumb);
            if (artwork.find("fanart") != artwork.end() && (overwrite || !CFile::Exists(savedFanart)))
              CTextureCache::GetInstance().Export(artwork["fanart"], savedFanart);
          }
          xmlDoc.Clear();
          TiXmlDeclaration decl("1.0", "UTF-8", "yes");
          xmlDoc.InsertEndChild(decl);
        }
      }

      if ((current % 50) == 0 && progress)
      {
        progress->SetLine(1, CVariant{artist.strArtist});
        progress->SetPercentage(current * 100 / total);
        progress->Progress();
        if (progress->IsCanceled())
        {
          progress->Close();
          m_pDS->close();
          return;
        }
      }
      current++;
    }

    xmlDoc.SaveFile(xmlFile);

    CVariant data;
    if (singleFile)
    {
      data["file"] = xmlFile;
      if (iFailCount > 0)
        data["failcount"] = iFailCount;
    }
    ANNOUNCEMENT::CAnnouncementManager::GetInstance().Announce(ANNOUNCEMENT::AudioLibrary, "xbmc", "OnExport", data);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
    iFailCount++;
  }

  if (progress)
    progress->Close();

  if (iFailCount > 0)
    CGUIDialogOK::ShowAndGetInput(CVariant{20196}, CVariant{StringUtils::Format(g_localizeStrings.Get(15011).c_str(), iFailCount)});
}

void CMusicDatabase::ImportFromXML(const std::string &xmlFile)
{
  CGUIDialogProgress *progress = g_windowManager.GetWindow<CGUIDialogProgress>();
  //TODO: Needs to be converted to OBD
  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    CXBMCTinyXML xmlDoc;
    if (!xmlDoc.LoadFile(xmlFile))
      return;

    TiXmlElement *root = xmlDoc.RootElement();
    if (!root) return;

    if (progress)
    {
      progress->SetHeading(CVariant{20197});
      progress->SetLine(0, CVariant{649});
      progress->SetLine(1, CVariant{330});
      progress->SetLine(2, CVariant{""});
      progress->SetPercentage(0);
      progress->Open();
      progress->ShowProgressBar(true);
    }

    TiXmlElement *entry = root->FirstChildElement();
    int current = 0;
    int total = 0;
    // first count the number of items...
    while (entry)
    {
      if (strnicmp(entry->Value(), "artist", 6)==0 ||
          strnicmp(entry->Value(), "album", 5)==0)
        total++;
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
        int idArtist = GetArtistByName(importedArtist.strArtist);
        if (idArtist > -1)
        {
          CArtist artist;
          GetArtist(idArtist, artist);
          artist.MergeScrapedArtist(importedArtist, true);
          UpdateArtist(artist);
        }

        current++;
      }
      else if (strnicmp(entry->Value(), "album", 5) == 0)
      {
        CAlbum importedAlbum;
        importedAlbum.Load(entry);
        strTitle = importedAlbum.strAlbum;
        int idAlbum = GetAlbumByName(importedAlbum.strAlbum, importedAlbum.GetAlbumArtistString());
        if (idAlbum > -1)
        {
          CAlbum album;
          GetAlbum(idAlbum, album, true);
          album.MergeScrapedAlbum(importedAlbum, true);
          UpdateAlbum(album); //Will replace song artists if present in xml
        }

        current++;
      }
      entry = entry ->NextSiblingElement();
      if (progress && total)
      {
        progress->SetPercentage(current * 100 / total);
        progress->SetLine(2, CVariant{std::move(strTitle)});
        progress->Progress();
        if (progress->IsCanceled())
        {
          progress->Close();
          RollbackTransaction();
          return;
        }
      }
    }
    g_infoManager.SetLibraryBool(LIBRARY_HAS_MUSIC, GetSongsCount() > 0);

    g_infoManager.ResetLibraryBools();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
    RollbackTransaction();
  }
  if (progress)
    progress->Close();
}

void CMusicDatabase::SetPropertiesFromArtist(CFileItem& item, const CArtist& artist)
{
  item.SetProperty("artist_instrument", StringUtils::Join(artist.instruments, g_advancedSettings.m_musicItemSeparator));
  item.SetProperty("artist_instrument_array", artist.instruments);
  item.SetProperty("artist_style", StringUtils::Join(artist.styles, g_advancedSettings.m_musicItemSeparator));
  item.SetProperty("artist_style_array", artist.styles);
  item.SetProperty("artist_mood", StringUtils::Join(artist.moods, g_advancedSettings.m_musicItemSeparator));
  item.SetProperty("artist_mood_array", artist.moods);
  item.SetProperty("artist_born", artist.strBorn);
  item.SetProperty("artist_formed", artist.strFormed);
  item.SetProperty("artist_description", artist.strBiography);
  item.SetProperty("artist_genre", StringUtils::Join(artist.genre, g_advancedSettings.m_musicItemSeparator));
  item.SetProperty("artist_genre_array", artist.genre);
  item.SetProperty("artist_died", artist.strDied);
  item.SetProperty("artist_disbanded", artist.strDisbanded);
  item.SetProperty("artist_yearsactive", StringUtils::Join(artist.yearsActive, g_advancedSettings.m_musicItemSeparator));
  item.SetProperty("artist_yearsactive_array", artist.yearsActive);
}

void CMusicDatabase::SetPropertiesFromAlbum(CFileItem& item, const CAlbum& album)
{
  item.SetProperty("album_description", album.strReview);
  item.SetProperty("album_theme", StringUtils::Join(album.themes, g_advancedSettings.m_musicItemSeparator));
  item.SetProperty("album_theme_array", album.themes);
  item.SetProperty("album_mood", StringUtils::Join(album.moods, g_advancedSettings.m_musicItemSeparator));
  item.SetProperty("album_mood_array", album.moods);
  item.SetProperty("album_style", StringUtils::Join(album.styles, g_advancedSettings.m_musicItemSeparator));
  item.SetProperty("album_style_array", album.styles);
  item.SetProperty("album_type", album.strType);
  item.SetProperty("album_label", album.strLabel);
  item.SetProperty("album_artist", album.GetAlbumArtistString());
  item.SetProperty("album_artist_array", album.GetAlbumArtist());
  item.SetProperty("album_genre", StringUtils::Join(album.genre, g_advancedSettings.m_musicItemSeparator));
  item.SetProperty("album_genre_array", album.genre);
  item.SetProperty("album_title", album.strAlbum);
  if (album.fRating > 0)
    item.SetProperty("album_rating", album.fRating);
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
  int idArtist = GetArtistByName(item.GetMusicInfoTag()->GetArtistString());
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
  //TODO: Seems unused 
  for (const auto &i : art)
    SetArtForItem(mediaId, mediaType, i.first, i.second);
}

void CMusicDatabase::SetArtForItem(int mediaId, const std::string &mediaType, const std::string &artType, const std::string &url)
{
  try
  {
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());

    // don't set <foo>.<bar> art types - these are derivative types from parent items
    if (artType.find('.') != std::string::npos)
      return;
    
    if (mediaType == MediaTypeAlbum)
    {
      ODBView_Album_Art objArtView;
      if (m_cdb.getDB()->query_one<ODBView_Album_Art>(odb::query<ODBView_Album_Art>::CODBAlbum::idAlbum == mediaId &&
                                                      odb::query<ODBView_Album_Art>::CODBArt::type == artType
                                                      , objArtView))
      {
        objArtView.art->m_url = url;
        m_cdb.getDB()->update(objArtView.art);
      }
      else
      {
        CODBAlbum objAlbum;
        if (m_cdb.getDB()->query_one<CODBAlbum>(odb::query<CODBAlbum>::idAlbum == mediaId, objAlbum))
        {
          std::shared_ptr<CODBArt> art(new CODBArt);
          art->m_url = url;
          art->m_media_type = mediaType;
          art->m_type = artType;
          m_cdb.getDB()->persist(art);
          
          if (!objAlbum.section_foreign.loaded())
            m_cdb.getDB()->load(objAlbum, objAlbum.section_foreign);
          objAlbum.m_artwork.push_back(art);
          m_cdb.getDB()->update(objAlbum);
          m_cdb.getDB()->update(objAlbum, objAlbum.section_foreign);
        }
      }
    }
    else if (mediaType == MediaTypeSong)
    {
      ODBView_Song_Art objArtView;
      if (m_cdb.getDB()->query_one<ODBView_Song_Art>(odb::query<ODBView_Song_Art>::CODBSong::idSong == mediaId &&
                                                     odb::query<ODBView_Song_Art>::CODBArt::type == artType
                                                     , objArtView))
      {
        objArtView.art->m_url = url;
        m_cdb.getDB()->update(objArtView.art);
      }
      else
      {
        CODBSong objSong;
        if (m_cdb.getDB()->query_one<CODBSong>(odb::query<CODBSong>::idSong == mediaId, objSong))
        {
          std::shared_ptr<CODBArt> art(new CODBArt);
          art->m_url = url;
          art->m_media_type = mediaType;
          art->m_type = artType;
          m_cdb.getDB()->persist(art);
          
          if (!objSong.section_foreign.loaded())
            m_cdb.getDB()->load(objSong, objSong.section_foreign);
          objSong.m_artwork.push_back(art);
          m_cdb.getDB()->update(objSong);
          m_cdb.getDB()->update(objSong, objSong.section_foreign);
        }
      }
    }
    else if (mediaType == MediaTypeArtist)
    {
      CODBPerson objPerson;
      if (m_cdb.getDB()->query_one<CODBPerson>(odb::query<CODBPerson>::idPerson == mediaId, objPerson))
      {
        if (objPerson.m_art.load())
        {
          objPerson.m_art->m_url = url;
          m_cdb.getDB()->update(*(objPerson.m_art));
        }
        else
        {
          std::shared_ptr<CODBArt> art(new CODBArt);
          art->m_url = url;
          art->m_media_type = mediaType;
          art->m_type = artType;
          m_cdb.getDB()->persist(art);
          
          objPerson.m_art = art;
          m_cdb.getDB()->update(objPerson);
        }
      }
    }
    else
    {
      CLog::Log(LOGERROR, "%s unkonwn mediaType - %s", __FUNCTION__, mediaType.c_str());
    }
    
    if(odb_transaction)
      odb_transaction->commit();
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "%s(%d, '%s', '%s', '%s') exception - %s", __FUNCTION__, mediaId, mediaType.c_str(), artType.c_str(), url.c_str(), e.what());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%d, '%s', '%s', '%s') failed", __FUNCTION__, mediaId, mediaType.c_str(), artType.c_str(), url.c_str());
  }
}

bool CMusicDatabase::GetArtForItem(int mediaId, const std::string &mediaType, std::map<std::string, std::string> &art)
{
  try
  {
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    
    if (mediaType == MediaTypeAlbum)
    {
      typedef odb::query<CODBAlbum> query;
      CODBAlbum objAlbum;
      if (m_cdb.getDB()->query_one<CODBAlbum>(query::idAlbum == mediaId, objAlbum))
      {
        if (!objAlbum.section_foreign.loaded())
          m_cdb.getDB()->load(objAlbum, objAlbum.section_foreign);
        for (auto& i: objAlbum.m_artwork)
        {
          if (i.load())
          {
            art.insert(make_pair(i->m_type, i->m_url));
          }
        }
      }
    }
    else if (mediaType == MediaTypeSong)
    {
      typedef odb::query<CODBSong> query;
      CODBSong objSong;
      if (m_cdb.getDB()->query_one<CODBSong>(query::idSong == mediaId, objSong))
      {
        if (!objSong.section_foreign.loaded())
          m_cdb.getDB()->load(objSong, objSong.section_foreign);
        for (auto& i: objSong.m_artwork)
        {
          if (i.load())
          {
            art.insert(make_pair(i->m_type, i->m_url));
          }
        }
      }
    }
    else if (mediaType == MediaTypeArtist)
    {
      typedef odb::query<CODBPerson> query;
      CODBPerson objPerson;
      if (m_cdb.getDB()->query_one<CODBPerson>(query::idPerson == mediaId, objPerson))
      {
        if (objPerson.m_art.load())
          art.insert(make_pair(objPerson.m_art->m_type, objPerson.m_art->m_url));
      }
    }
    else
    {
      CLog::Log(LOGERROR, "%s(%d) map unknown media type - %s", __FUNCTION__, mediaId, mediaType.c_str());
    }

    return !art.empty();
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "%s(%d) failed - %s", __FUNCTION__, mediaId, e.what());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%d) failed", __FUNCTION__, mediaId);
  }
  return false;
}

std::string CMusicDatabase::GetArtForItem(int mediaId, const std::string &mediaType, const std::string &artType)
{
  try
  {
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    
    if (mediaType == MediaTypeAlbum)
    {
      typedef odb::query<ODBView_Album_Art> query;
      ODBView_Album_Art objViewArt;
      if (m_cdb.getDB()->query_one<ODBView_Album_Art>(query::CODBAlbum::idAlbum == mediaId
                                                      && query::CODBArt::media_type == mediaType
                                                      && query::CODBArt::type == artType, objViewArt))
      {
        return objViewArt.art->m_url;
      }
    }
    else if (mediaType == MediaTypeSong)
    {
      typedef odb::query<ODBView_Song_Art> query;
      ODBView_Song_Art objViewArt;
      if (m_cdb.getDB()->query_one<ODBView_Song_Art>(query::CODBSong::idSong == mediaId
                                                      && query::CODBArt::media_type == mediaType
                                                      && query::CODBArt::type == artType, objViewArt))
      {
        return objViewArt.art->m_url;
      }
    }
    else if (mediaType == MediaTypeArtist)
    {
      typedef odb::query<CODBPerson> query;
      CODBPerson objPerson;
      if (m_cdb.getDB()->query_one<CODBPerson>(query::idPerson == mediaId, objPerson))
      {
        if (objPerson.m_art.load())
          return objPerson.m_art->m_url;
      }
    }
    else
    {
      CLog::Log(LOGERROR, "%s(%d) unknown media type - %s", __FUNCTION__, mediaId, mediaType.c_str());
    }
    
    return "";
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "%s(%d) failed - %s", __FUNCTION__, mediaId, e.what());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%d) failed", __FUNCTION__, mediaId);
  }
  return "";
}

bool CMusicDatabase::GetArtistArtForItem(int mediaId, const std::string &mediaType, std::map<std::string, std::string> &art)
{
  try
  {
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    
    if (mediaType == MediaTypeAlbum)
    {
      typedef odb::query<ODBView_Album_Artist_Art> query;
      odb::result<ODBView_Album_Artist_Art> res(m_cdb.getDB()->query<ODBView_Album_Artist_Art>(query::CODBAlbum::idAlbum == mediaId));
      for (auto objRes : res)
      {
        art.insert(std::make_pair(objRes.art->m_type, objRes.art->m_url));
      }
    }
    else
    {
      typedef odb::query<ODBView_Song_Artist_Art> query;
      odb::result<ODBView_Song_Artist_Art> res(m_cdb.getDB()->query<ODBView_Song_Artist_Art>(query::CODBSong::idSong == mediaId
                                                                                             && query::CODBRole::name == "artist"));
      for (auto objRes : res)
      {
        art.insert(std::make_pair(objRes.art->m_type, objRes.art->m_url));
      }
    }

    return !art.empty();
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "%s(%d) exception - %s", __FUNCTION__, mediaId, e.what());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%d) failed", __FUNCTION__, mediaId);
  }
  return false;
}

std::string CMusicDatabase::GetArtistArtForItem(int mediaId, const std::string &mediaType, const std::string &artType)
{
  try
  {
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());
    
    if (mediaType == MediaTypeAlbum)
    {
      typedef odb::query<ODBView_Album_Artist_Art> query;
      odb::result<ODBView_Album_Artist_Art> res(m_cdb.getDB()->query<ODBView_Album_Artist_Art>(query::CODBAlbum::idAlbum == mediaId
                                                                                               && query::CODBArt::type == artType));
      if (res.begin() != res.end())
        return res.begin()->art->m_url;
    }
    else
    {
      typedef odb::query<ODBView_Song_Artist_Art> query;
      odb::result<ODBView_Song_Artist_Art> res(m_cdb.getDB()->query<ODBView_Song_Artist_Art>(query::CODBSong::idSong == mediaId
                                                                                             && query::CODBRole::name == "artist"
                                                                                             && query::CODBArt::type == artType));
      if (res.begin() != res.end())
        return res.begin()->art->m_url;
    }
    
    return "";
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "%s(%d) exception - %s", __FUNCTION__, mediaId, e.what());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%d) failed", __FUNCTION__, mediaId);
  }
  return "";
}

template <typename T>
T CMusicDatabase::GetODBFilterGenres(CDbUrl &musicUrl, Filter &filter, SortDescription &sorting)
{
  typedef odb::query<ODBView_Music_Genres> query;
  query objQuery;
  
  if (!musicUrl.IsValid())
    return objQuery;
  
  std::string type = musicUrl.GetType();
  const CUrlOptions::UrlOptions& options = musicUrl.GetOptions();
  
  // Check for playlist rules first
  auto option = options.find("xsp");
  if (option != options.end())
  {
    CSmartPlaylist xsp;
    if (!xsp.LoadFromJson(option->second.asString()))
      return objQuery;

    // check if the filter playlist matches the item type
    if (xsp.GetType() == type ||
        (xsp.GetGroup() == type && !xsp.IsGroupMixed()))
    {
      if (xsp.GetLimit() > 0)
        sorting.limitEnd = xsp.GetLimit();
      if (xsp.GetOrder() != SortByNone)
        sorting.sortBy = xsp.GetOrder();
      sorting.sortOrder = xsp.GetOrderAscending() ? SortOrderAscending : SortOrderDescending;
      if (CServiceBroker::GetSettings().GetBool(CSettings::SETTING_FILELISTS_IGNORETHEWHENSORTING))
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

  if(idRole > 0)
  {
    objQuery = objQuery && query::CODBRole::idRole == idRole;
  }
  
  return objQuery;
}

template <typename T>
T CMusicDatabase::GetODBFilterArtists(CDbUrl &musicUrl, Filter &filter, SortDescription &sorting)
{
  if (!musicUrl.IsValid())
    return T();
  
  std::string type = musicUrl.GetType();
  const CUrlOptions::UrlOptions& options = musicUrl.GetOptions();
  
  T objQuery;
  
  // Check for playlist rules first, they may contain role criteria
  bool hasRoleRules = false;
  auto option = options.find("xsp");
  if (option != options.end())
  {
    CSmartPlaylist xsp;
    if (!xsp.LoadFromJson(option->second.asString()))
      return objQuery;
    
    std::set<std::string> playlists;
    objQuery = xsp.GetArtistWhereClause(playlists);
    hasRoleRules = xsp.GetType() == "artists" && xsp.GetHasRoleRules();
    
    // check if the filter playlist matches the item type
    if (xsp.GetType() == type ||
        (xsp.GetGroup() == type && !xsp.IsGroupMixed()))
    {
      if (xsp.GetLimit() > 0)
        sorting.limitEnd = xsp.GetLimit();
      if (xsp.GetOrder() != SortByNone)
        sorting.sortBy = xsp.GetOrder();
      sorting.sortOrder = xsp.GetOrderAscending() ? SortOrderAscending : SortOrderDescending;
      if (CServiceBroker::GetSettings().GetBool(CSettings::SETTING_FILELISTS_IGNORETHEWHENSORTING))
        sorting.sortAttributes = SortAttributeIgnoreArticle;
    }
  }
  
  //Process role options, common to artist and album type filtering
  int idRole = GetRoleByName("artist"); // Default restrict song_artist to "artists" only, no other roles.
  int idAristRole = idRole;
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
  
  int idArtist = -1, idGenre = -1, idAlbum = -1, idSong = -1;
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
        objQuery = objQuery && T::CODBPerson::idPerson == idArtist;
      else if (idAlbum > 0)
        objQuery = objQuery && T::CODBAlbum::idAlbum == idAlbum;
      else if (idSong > 0)
      {
        objQuery = objQuery && T::CODBSong::idSong == idSong && T::CODBRole::idRole == idRole;
      }
      else
      { // Artists can be only album artists, so for all artists (with linked albums or songs)
        // we need to check both album_artist and song_artist tables.
        // Role is determined from song_artist table, so even if looking for album artists only
        // we can check those that have a specific role e.g. which album artist is a composer
        // of songs in that album, from entries in the song_artist table.
        // Role < -1 is used to indicate that all roles are wanted.
        // When not album artists only and a specific role wanted then only the song_artist table is checked.
        // When album artists only and role = 1 (an "artist") then only the album_artist table is checked.
        
        T objQeryAlbumArtistSub = T::CODBPerson::idPerson == T::albumArist::idPerson;
        T objQerySongArtistSub;
        
        if (idRole > 0)
        {
          objQerySongArtistSub = objQerySongArtistSub && T::CODBRole::idRole == idRole;
        }
        if (idGenre > 0)
        {
          objQerySongArtistSub = objQerySongArtistSub && T::genre::idGenre == idGenre;
        }
        if (idRole <= 1 && idGenre > 0)
        {// Check genre of songs of album using nested subquery
          objQeryAlbumArtistSub = objQeryAlbumArtistSub && T::genre::idGenre == idGenre;
        }
        if (idRole > 1 && albumArtistsOnly)
        { // Album artists only with role, check AND in album_artist for album of song
          // using nested subquery correlated with album_artist
          objQuery = objQuery && objQeryAlbumArtistSub;
        }
        else
        {
          if (idRole < 0 || (idRole == idAristRole && !albumArtistsOnly))
          { // Artist contributing to songs, any role, check OR album artist too
            // as artists can be just album artists but not song artists
            objQuery = objQuery && (objQerySongArtistSub || objQeryAlbumArtistSub);
          }
          else if (idRole > 1)
          {
            // Artist contributes that role (not albmartistsonly as already handled)
            objQuery = objQuery && objQerySongArtistSub;
          }
          else // idRole = 1 and albumArtistsOnly
          { // Only look at album artists, not albums where artist features on songs
            objQuery = objQuery && objQeryAlbumArtistSub;
          }
        }
      }
    }
    // remove the null string
    objQuery = objQuery && T::CODBPerson::name != "";
    
    // and the various artist entry if applicable
    if (!albumArtistsOnly)
    {
      std::string strVariousArtists = g_localizeStrings.Get(340);
      objQuery = objQuery && T::CODBPerson::name != strVariousArtists;
    }
  }
  
  option = options.find("filter");
  if (option != options.end())
  {
    CSmartPlaylist xspFilter;
    if (!xspFilter.LoadFromJson(option->second.asString()))
      return objQuery;
    
    // check if the filter playlist matches the item type
    if (xspFilter.GetType() == type)
    {
      std::set<std::string> playlists;
      objQuery = xspFilter.GetArtistWhereClause(playlists);
    }
    // remove the filter if it doesn't match the item type
    else
      musicUrl.RemoveOption("filter");
  }
  
  return objQuery;
}

template <typename T>
T CMusicDatabase::GetODBFilterSongs(CDbUrl &musicUrl, Filter &filter, SortDescription &sorting)
{
  if (!musicUrl.IsValid())
    return T();
    
  std::string type = musicUrl.GetType();
  const CUrlOptions::UrlOptions& options = musicUrl.GetOptions();
    
  T objQuery;
  
  // Check for playlist rules first, they may contain role criteria
  bool hasRoleRules = false;
  auto option = options.find("xsp");
  if (option != options.end())
  {
    CSmartPlaylist xsp;
    if (!xsp.LoadFromJson(option->second.asString()))
      return objQuery;
    
    std::set<std::string> playlists;
    objQuery = xsp.GetSongWhereClause(playlists);
    hasRoleRules = xsp.GetType() == "artists" && xsp.GetHasRoleRules();
    
    // check if the filter playlist matches the item type
    if (xsp.GetType() == type ||
        (xsp.GetGroup() == type && !xsp.IsGroupMixed()))
    {
      if (xsp.GetLimit() > 0)
        sorting.limitEnd = xsp.GetLimit();
      if (xsp.GetOrder() != SortByNone)
        sorting.sortBy = xsp.GetOrder();
      sorting.sortOrder = xsp.GetOrderAscending() ? SortOrderAscending : SortOrderDescending;
      if (CServiceBroker::GetSettings().GetBool(CSettings::SETTING_FILELISTS_IGNORETHEWHENSORTING))
        sorting.sortAttributes = SortAttributeIgnoreArticle;
    }
  }
  
  //Process role options, common to artist and album type filtering
  int idRole = GetRoleByName("artist"); // Default restrict song_artist to "artists" only, no other roles.
  int idAristRole = idRole;
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
  
  T objRoleQuery; //Role < 0 means all roles, otherwise filter by role
  if(idRole > 0) objRoleQuery = T::CODBRole::idRole == idRole;
  
  int idArtist = -1, idGenre = -1, idAlbum = -1, idSong = -1;
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
  
  if (type == "songs" || type == "singles")
  {
    option = options.find("singles");
    if (option != options.end())
    {
      T tmpQuery = T::CODBAlbum::releaseType == CAlbum::ReleaseTypeToString(CAlbum::Single);
      
      if (option->second.asBoolean())
      {
        tmpQuery = !tmpQuery;
      }
      
      objQuery = objQuery && tmpQuery;
    }
    
    option = options.find("year");
    if (option != options.end())
      objQuery = objQuery && T::CODBSong::year == static_cast<int>(option->second.asInteger());
    
    option = options.find("compilation");
    if (option != options.end())
      objQuery = objQuery && T::CODBAlbum::compilation == option->second.asBoolean();
    
    if (idSong > 0)
      objQuery = objQuery && T::CODBSong::idSong == idSong;
    
    if (idAlbum > 0)
       objQuery = objQuery && T::CODBAlbum::idAlbum == idAlbum;
    
    if (idGenre > 0)
      objQuery = objQuery && T::CODBGenre::idGenre == idGenre;
    
    T songArtistClause;
    T albumArtistClause = T::CODBPerson::idPerson == T::albumArist::idPerson;
    if (idArtist > 0)
    {
      songArtistClause = songArtistClause && T::CODBPerson::idPerson == idArtist && objRoleQuery;
      
      albumArtistClause = albumArtistClause && T::CODBPerson::idPerson == idArtist;
    }
    else if (!artistname.empty())
    {  // Artist name is not unique, so could get songs from more than one.
      songArtistClause = songArtistClause && T::CODBPerson::name == artistname && objRoleQuery;
      
      albumArtistClause = albumArtistClause && T::CODBPerson::name == artistname;
    }
    
    // Process artist name or id option
    if (!songArtistClause.empty())
    {
      if (idRole < 0) // Artist contributes to songs, any roles OR is album artist
        objQuery = objQuery && (songArtistClause || albumArtistClause);
      else if (idRole > 1)
      {
        if (albumArtistsOnly)  //Album artists only with role, check AND in album_artist for same song
          objQuery = objQuery && (songArtistClause && albumArtistClause);
        else // songs where artist contributes that role.
          objQuery = objQuery && songArtistClause;
      }
      else
      {
        if (albumArtistsOnly) // Only look at album artists, not where artist features on songs
          objQuery = objQuery && albumArtistClause;
        else // Artist is song artist or album artist
          objQuery = objQuery && (songArtistClause || albumArtistClause);
      }
    }
  }
  
  option = options.find("filter");
  if (option != options.end())
  {
    CSmartPlaylist xspFilter;
    if (!xspFilter.LoadFromJson(option->second.asString()))
      return objQuery;
    
    // check if the filter playlist matches the item type
    if (xspFilter.GetType() == type)
    {
      std::set<std::string> playlists;
      objQuery = xspFilter.GetSongWhereClause(playlists);
    }
    // remove the filter if it doesn't match the item type
    else
      musicUrl.RemoveOption("filter");
  }
  
  return objQuery;
}

template <typename T>
T CMusicDatabase::GetODBFilterAlbums(CDbUrl &musicUrl, Filter &filter, SortDescription &sorting)
{
  if (!musicUrl.IsValid())
    return T();
  
  std::string type = musicUrl.GetType();
  const CUrlOptions::UrlOptions& options = musicUrl.GetOptions();
  
  T objQuery;
  
  // Check for playlist rules first, they may contain role criteria
  bool hasRoleRules = false;
  auto option = options.find("xsp");
  if (option != options.end())
  {
    CSmartPlaylist xsp;
    if (!xsp.LoadFromJson(option->second.asString()))
      return objQuery;
    
    std::set<std::string> playlists;
    objQuery = xsp.GetAlbumWhereClause(playlists);
    hasRoleRules = xsp.GetType() == "artists" && xsp.GetHasRoleRules();
    
    // check if the filter playlist matches the item type
    if (xsp.GetType() == type ||
        (xsp.GetGroup() == type && !xsp.IsGroupMixed()))
    {
      if (xsp.GetLimit() > 0)
        sorting.limitEnd = xsp.GetLimit();
      if (xsp.GetOrder() != SortByNone)
        sorting.sortBy = xsp.GetOrder();
      sorting.sortOrder = xsp.GetOrderAscending() ? SortOrderAscending : SortOrderDescending;
      if (CServiceBroker::GetSettings().GetBool(CSettings::SETTING_FILELISTS_IGNORETHEWHENSORTING))
        sorting.sortAttributes = SortAttributeIgnoreArticle;
    }
  }
  
  //Process role options, common to artist and album type filtering
  int idRole = GetRoleByName("artist"); // Default restrict song_artist to "artists" only, no other roles.
  int idAristRole = idRole;
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
  
  int idArtist = -1, idGenre = -1, idAlbum = -1, idSong = -1;
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
  
  if (type == "albums")
  {
    option = options.find("year");
    if (option != options.end())
      objQuery = objQuery && T::CODBAlbum::year == static_cast<int>(option->second.asInteger());
    
    option = options.find("compilation");
    if (option != options.end())
      objQuery = objQuery && T::CODBAlbum::compilation == option->second.asBoolean();
    
    // Process artist, role and genre options together as song subquery to filter those
    // albums that have songs with both that artist and genre
    
    T objQeryAlbumArtistSub;
    T objQerySongArtistSub = T::CODBPerson::idPerson == T::songArist::idPerson;
    T objQeryGenreSub = T::CODBGenre::idGenre == idGenre;
    
    if (idArtist > 0)
    {
      objQeryAlbumArtistSub = objQeryAlbumArtistSub && T::CODBPerson::idPerson == idArtist;
      objQerySongArtistSub = objQerySongArtistSub && T::songArist::idPerson == idArtist;
    }
    else if (!artistname.empty())
    { // Artist name is not unique, so could get albums or songs from more than one.
      objQeryAlbumArtistSub = objQeryAlbumArtistSub && T::CODBPerson::name == artistname;
      objQerySongArtistSub = objQerySongArtistSub && T::songArist::name == artistname;
    }
    if (idRole > 0)
    {
      objQerySongArtistSub = objQerySongArtistSub && T::CODBRole::idRole == idRole;
    }
    if (idGenre > 0)
    {
      objQerySongArtistSub = objQerySongArtistSub && T::CODBGenre::idGenre == idGenre;
    }
    
    if (idArtist > 0 || !artistname.empty())
    {
      if (idRole <= 1 && idGenre > 0)
      { // Check genre of songs of album using nested subquery
        objQeryAlbumArtistSub = objQeryAlbumArtistSub && objQeryGenreSub;
      }
      if (idRole > 1 && albumArtistsOnly)
      {  // Album artists only with role, check AND in album_artist for same song
        objQuery = objQuery && objQeryAlbumArtistSub;//TODO:  && objQerySongArtistSub;
      }
      else
      {
        if (idRole < 0 || (idRole == idAristRole && !albumArtistsOnly))
        { // Artist contributing to songs, any role, check OR album artist too
          // as artists can be just album artists but not song artists
          objQuery = objQuery && (objQeryAlbumArtistSub /* TODO: || objQerySongArtistSub */);
        }
        else if (idRole > 1)
        { // Albums with songs where artist contributes that role (not albmartistsonly as already handled)
          objQuery = objQuery && objQerySongArtistSub;
        }
        else // idRole = 1 and albumArtistsOnly
        { // Only look at album artists, not albums where artist features on songs
          // This may want to be a separate option so you can choose to see all the albums where that artist
          // appears on one or more songs without having to list all song artists in the artists node.
          objQuery = objQuery && objQeryAlbumArtistSub;
        }
      }
    }
    else
    { // No artist given
      if (idGenre > 0)
      { // Have genre option but not artist
        objQuery = objQuery && objQeryGenreSub;
      }
      // Exclude any single albums (aka empty tagged albums)
      // This causes "albums"  media filter artist selection to only offer album artists
      option = options.find("show_singles");
      if (option == options.end() || !option->second.asBoolean())
        objQuery = objQuery && T::CODBAlbum::releaseType == CAlbum::ReleaseTypeToString(CAlbum::Album);
    }
  }
  
  option = options.find("filter");
  if (option != options.end())
  {
    CSmartPlaylist xspFilter;
    if (!xspFilter.LoadFromJson(option->second.asString()))
      return objQuery;
    
    // check if the filter playlist matches the item type
    if (xspFilter.GetType() == type)
    {
      std::set<std::string> playlists;
      objQuery = xspFilter.GetAlbumWhereClause(playlists);
    }
    // remove the filter if it doesn't match the item type
    else
      musicUrl.RemoveOption("filter");
  }
  
  return objQuery;
}

void CMusicDatabase::UpdateFileDateAdded(const std::shared_ptr<CODBFile> objFile, const std::string& strFileNameAndPath)
{
  if (!objFile|| strFileNameAndPath.empty())
    return;

  CDateTime dateAdded;
  try
  {
    std::shared_ptr<odb::transaction> odb_transaction (m_cdb.getTransaction());

    // 1 preferring to use the files mtime(if it's valid) and only using the file's ctime if the mtime isn't valid
    if (g_advancedSettings.m_iMusicLibraryDateAdded == 1)
      dateAdded = CFileUtils::GetModificationDate(strFileNameAndPath, false);
    //2 using the newer datetime of the file's mtime and ctime
    else if (g_advancedSettings.m_iMusicLibraryDateAdded == 2)
      dateAdded = CFileUtils::GetModificationDate(strFileNameAndPath, true);
    //0 using the current datetime if non of the above matches or one returns an invalid datetime
    if (!dateAdded.IsValid())
      dateAdded = CDateTime::GetCurrentDateTime();
    
    objFile->m_dateAdded.setDateTime(dateAdded.GetAsULongLong(), dateAdded.GetAsDBDateTime());
    m_cdb.getDB()->update(objFile);
    
    if(odb_transaction)
      odb_transaction->commit();
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "%s (%s, %s) exception - %s", __FUNCTION__, CURL::GetRedacted(strFileNameAndPath).c_str(), dateAdded.GetAsDBDateTime().c_str(), e.what());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s, %s) failed", __FUNCTION__, CURL::GetRedacted(strFileNameAndPath).c_str(), dateAdded.GetAsDBDateTime().c_str());
  }
}

bool CMusicDatabase::AddAudioBook(const CFileItem& item)
{
  // TODO
  std::string strSQL = PrepareSQL("INSERT INTO audiobook (idBook,strBook,strAuthor,bookmark,file,dateAdded) VALUES (NULL,'%s','%s',%i,'%s','%s')",
                                 item.GetMusicInfoTag()->GetAlbum().c_str(),
                                 item.GetMusicInfoTag()->GetArtist()[0].c_str(), 0,
                                 item.GetPath().c_str(),
                                 CDateTime::GetCurrentDateTime().GetAsDBDateTime().c_str());
  return ExecuteQuery(strSQL);
}

bool CMusicDatabase::SetResumeBookmarkForAudioBook(const CFileItem& item, int bookmark)
{
  // TODO
  std::string strSQL = PrepareSQL("select bookmark from audiobook where file='%s'",
                                 item.GetPath().c_str());
  if (!m_pDS->query(strSQL.c_str()) || m_pDS->num_rows() == 0)
  {
    if (!AddAudioBook(item))
      return false;
  }

  strSQL = PrepareSQL("UPDATE audiobook SET bookmark=%i WHERE file='%s'",
                      bookmark, item.GetPath().c_str());

  return ExecuteQuery(strSQL);
}

bool CMusicDatabase::GetResumeBookmarkForAudioBook(const std::string& path, int& bookmark)
{
  // TODO
  std::string strSQL = PrepareSQL("SELECT bookmark FROM audiobook WHERE file='%s'",
                                 path.c_str());
  if (!m_pDS->query(strSQL.c_str()) || m_pDS->num_rows() == 0)
    return false;

  bookmark = m_pDS->fv(0).get_asInt();
  return true;
}
