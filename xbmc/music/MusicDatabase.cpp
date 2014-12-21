/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "network/Network.h"
#include "threads/SystemClock.h"
#include "system.h"
#include "MusicDatabase.h"
#include "network/cddb.h"
#include "filesystem/DirectoryCache.h"
#include "filesystem/MusicDatabaseDirectory/DirectoryNode.h"
#include "filesystem/MusicDatabaseDirectory/QueryParams.h"
#include "filesystem/MusicDatabaseDirectory.h"
#include "filesystem/SpecialProtocol.h"
#include "GUIInfoManager.h"
#include "music/tags/MusicInfoTag.h"
#include "addons/AddonManager.h"
#include "addons/Scraper.h"
#include "addons/Addon.h"
#include "utils/URIUtils.h"
#include "Artist.h"
#include "Album.h"
#include "Song.h"
#include "guilib/GUIWindowManager.h"
#include "dialogs/GUIDialogOK.h"
#include "dialogs/GUIDialogProgress.h"
#include "dialogs/GUIDialogYesNo.h"
#include "dialogs/GUIDialogSelect.h"
#include "filesystem/File.h"
#include "profiles/ProfilesManager.h"
#include "settings/AdvancedSettings.h"
#include "FileItem.h"
#include "Application.h"
#ifdef HAS_KARAOKE
#include "karaoke/karaokelyricsfactory.h"
#endif
#include "storage/MediaManager.h"
#include "settings/MediaSettings.h"
#include "settings/Settings.h"
#include "utils/StringUtils.h"
#include "guilib/LocalizeStrings.h"
#include "utils/LegacyPathTranslation.h"
#include "utils/log.h"
#include "utils/TimeUtils.h"
#include "TextureCache.h"
#include "addons/AddonInstaller.h"
#include "utils/AutoPtrHandle.h"
#include "interfaces/AnnouncementManager.h"
#include "dbwrappers/dataset.h"
#include "utils/XMLUtils.h"
#include "URL.h"
#include "playlists/SmartPlayList.h"

using namespace std;
using namespace AUTOPTR;
using namespace XFILE;
using namespace MUSICDATABASEDIRECTORY;
using ADDON::AddonPtr;

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
  ANNOUNCEMENT::CAnnouncementManager::Get().Announce(ANNOUNCEMENT::AudioLibrary, "xbmc", "OnRemove", data);
}

static void AnnounceUpdate(const std::string& content, int id)
{
  CVariant data;
  data["type"] = content;
  data["id"] = id;
  if (g_application.IsMusicScanning())
    data["transaction"] = true;
  ANNOUNCEMENT::CAnnouncementManager::Get().Announce(ANNOUNCEMENT::AudioLibrary, "xbmc", "OnUpdate", data);
}

CMusicDatabase::CMusicDatabase(void)
{
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
              " lastScraped varchar(20) default NULL, "
              " dateAdded varchar (20) default NULL)");
  CLog::Log(LOGINFO, "create album table");
  m_pDS->exec("CREATE TABLE album (idAlbum integer primary key, "
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
  CLog::Log(LOGINFO, "create album_artist table");
  m_pDS->exec("CREATE TABLE album_artist (idArtist integer, idAlbum integer, strJoinPhrase text, boolFeatured integer, iOrder integer, strArtist text)");
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
              " rating char default '0', comment text)");
  CLog::Log(LOGINFO, "create song_artist table");
  m_pDS->exec("CREATE TABLE song_artist (idArtist integer, idSong integer, strJoinPhrase text, boolFeatured integer, iOrder integer, strArtist text)");
  CLog::Log(LOGINFO, "create song_genre table");
  m_pDS->exec("CREATE TABLE song_genre (idGenre integer, idSong integer, iOrder integer)");

  CLog::Log(LOGINFO, "create albuminfosong table");
  m_pDS->exec("CREATE TABLE albuminfosong (idAlbumInfoSong integer primary key, idAlbumInfo integer, iTrack integer, strTitle text, iDuration integer)");

  CLog::Log(LOGINFO, "create content table");
  m_pDS->exec("CREATE TABLE content (strPath text, strScraperPath text, strContent text, strSettings text)");
  CLog::Log(LOGINFO, "create discography table");
  m_pDS->exec("CREATE TABLE discography (idArtist integer, strAlbum text, strYear text)");

  CLog::Log(LOGINFO, "create karaokedata table");
  m_pDS->exec("CREATE TABLE karaokedata (iKaraNumber integer, idSong integer, iKaraDelay integer, strKaraEncoding text, "
              "strKaralyrics text, strKaraLyrFileCRC text)");

  CLog::Log(LOGINFO, "create art table");
  m_pDS->exec("CREATE TABLE art(art_id INTEGER PRIMARY KEY, media_id INTEGER, media_type TEXT, type TEXT, url TEXT)");

  // Add 'Karaoke' genre
  AddGenre( "Karaoke" );
}

void CMusicDatabase::CreateAnalytics()
{
  CLog::Log(LOGINFO, "%s - creating indices", __FUNCTION__);
  m_pDS->exec("CREATE INDEX idxAlbum ON album(strAlbum(255))");
  m_pDS->exec("CREATE INDEX idxAlbum_1 ON album(bCompilation)");
  m_pDS->exec("CREATE UNIQUE INDEX idxAlbum_2 ON album(strMusicBrainzAlbumID(36))");

  m_pDS->exec("CREATE UNIQUE INDEX idxAlbumArtist_1 ON album_artist ( idAlbum, idArtist )");
  m_pDS->exec("CREATE UNIQUE INDEX idxAlbumArtist_2 ON album_artist ( idArtist, idAlbum )");
  m_pDS->exec("CREATE INDEX idxAlbumArtist_3 ON album_artist ( boolFeatured )");

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

  m_pDS->exec("CREATE UNIQUE INDEX idxSongArtist_1 ON song_artist ( idSong, idArtist )");
  m_pDS->exec("CREATE UNIQUE INDEX idxSongArtist_2 ON song_artist ( idArtist, idSong )");
  m_pDS->exec("CREATE INDEX idxSongArtist_3 ON song_artist ( boolFeatured )");

  m_pDS->exec("CREATE UNIQUE INDEX idxSongGenre_1 ON song_genre ( idSong, idGenre )");
  m_pDS->exec("CREATE UNIQUE INDEX idxSongGenre_2 ON song_genre ( idGenre, idSong )");

  m_pDS->exec("CREATE INDEX idxAlbumInfoSong_1 ON albuminfosong ( idAlbumInfo )");

  m_pDS->exec("CREATE INDEX idxKaraNumber on karaokedata(iKaraNumber)");
  m_pDS->exec("CREATE INDEX idxKarSong on karaokedata(idSong)");

  m_pDS->exec("CREATE INDEX idxDiscography_1 ON discography ( idArtist )");

  m_pDS->exec("CREATE INDEX ix_art ON art(media_id, media_type(20), type(20))");

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
              "  DELETE FROM karaokedata WHERE karaokedata.idSong = old.idSong;"
              "  DELETE FROM art WHERE media_id=old.idSong AND media_type='song';"
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
              "        lastplayed, rating, comment, "
              "        song.idAlbum AS idAlbum, "
              "        strAlbum, "
              "        strPath, "
              "        iKaraNumber, iKaraDelay, strKaraEncoding,"
              "        album.bCompilation AS bCompilation,"
              "        album.strArtists AS strAlbumArtists "
              "FROM song"
              "  JOIN album ON"
              "    song.idAlbum=album.idAlbum"
              "  JOIN path ON"
              "    song.idPath=path.idPath"
              "  LEFT OUTER JOIN karaokedata ON"
              "    song.idSong=karaokedata.idSong");

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
              "        iRating, "
              "        bCompilation, "
              "        MIN(song.iTimesPlayed) AS iTimesPlayed "
              "FROM album"
              " LEFT OUTER JOIN song ON"
              "   album.idAlbum=song.idAlbum "
              "GROUP BY album.idAlbum");

  CLog::Log(LOGINFO, "create artist view");
  m_pDS->exec("CREATE VIEW artistview AS SELECT"
              "  idArtist, strArtist, "
              "  strMusicBrainzArtistID, "
              "  strBorn, strFormed, strGenres,"
              "  strMoods, strStyles, strInstruments, "
              "  strBiography, strDied, strDisbanded, "
              "  strYearsActive, strImage, strFanart "
              "FROM artist");

  CLog::Log(LOGINFO, "create albumartist view");
  m_pDS->exec("CREATE VIEW albumartistview AS SELECT"
              "  album_artist.idAlbum AS idAlbum, "
              "  album_artist.idArtist AS idArtist, "
              "  artist.strArtist AS strArtist, "
              "  artist.strMusicBrainzArtistID AS strMusicBrainzArtistID, "
              "  album_artist.boolFeatured AS boolFeatured, "
              "  album_artist.strJoinPhrase AS strJoinPhrase, "
              "  album_artist.iOrder AS iOrder "
              "FROM album_artist "
              "JOIN artist ON "
              "     album_artist.idArtist = artist.idArtist");

  CLog::Log(LOGINFO, "create songartist view");
  m_pDS->exec("CREATE VIEW songartistview AS SELECT"
              "  song_artist.idSong AS idSong, "
              "  song_artist.idArtist AS idArtist, "
              "  artist.strArtist AS strArtist, "
              "  artist.strMusicBrainzArtistID AS strMusicBrainzArtistID, "
              "  song_artist.boolFeatured AS boolFeatured, "
              "  song_artist.strJoinPhrase AS strJoinPhrase, "
              "  song_artist.iOrder AS iOrder "
              "FROM song_artist "
              "JOIN artist ON "
              "     song_artist.idArtist = artist.idArtist");
}

int CMusicDatabase::AddAlbumInfoSong(int idAlbum, const CSong& song)
{
  CStdString strSQL = PrepareSQL("SELECT idAlbumInfoSong FROM albuminfosong WHERE idAlbumInfo = %i and iTrack = %i", idAlbum, song.iTrack);
  int idAlbumInfoSong = (int)strtol(GetSingleValue(strSQL).c_str(), NULL, 10);
  if (idAlbumInfoSong > 0)
  {
    strSQL = PrepareSQL("UPDATE albuminfosong SET strTitle = '%s', iDuration = %i WHERE idAlbumInfoSong = %i", song.strTitle.c_str(), song.iDuration, idAlbumInfoSong);
    return ExecuteQuery(strSQL);
  }
  else
  {
    strSQL = PrepareSQL("INSERT INTO albuminfosong (idAlbumInfoSong,idAlbumInfo,iTrack,strTitle,iDuration) VALUES (NULL,%i,%i,'%s',%i)",
                        idAlbum,
                        song.iTrack,
                        song.strTitle.c_str(),
                        song.iDuration);
    return ExecuteQuery(strSQL);
  }
}

std::string GetArtistString(const VECARTISTCREDITS &credits)
{
  std::string artistString;
  for (VECARTISTCREDITS::const_iterator i = credits.begin(); i != credits.end(); ++i)
    artistString += i->GetArtist() + i->GetJoinPhrase();
  return artistString;
}

bool CMusicDatabase::AddAlbum(CAlbum& album)
{
  BeginTransaction();

  album.idAlbum = AddAlbum(album.strAlbum,
                           album.strMusicBrainzAlbumID,
                           GetArtistString(album.artistCredits),
                           album.GetGenreString(),
                           album.iYear,
                           album.bCompilation);

  // Add the album artists
  for (VECARTISTCREDITS::iterator artistCredit = album.artistCredits.begin(); artistCredit != album.artistCredits.end(); ++artistCredit)
  {
    artistCredit->idArtist = AddArtist(artistCredit->GetArtist(), artistCredit->GetMusicBrainzArtistID());
    AddAlbumArtist(artistCredit->idArtist,
                   album.idAlbum,
                   artistCredit->GetArtist(),
                   artistCredit->GetJoinPhrase(),
                   artistCredit == album.artistCredits.begin() ? false : true,
                   std::distance(album.artistCredits.begin(), artistCredit));
  }

  for (VECSONGS::iterator song = album.songs.begin(); song != album.songs.end(); ++song)
  {
    song->idAlbum = album.idAlbum;
    song->idSong = AddSong(song->idAlbum,
                           song->strTitle, song->strMusicBrainzTrackID,
                           song->strFileName, song->strComment,
                           song->strThumb,
                           GetArtistString(song->artistCredits), song->genre,
                           song->iTrack, song->iDuration, song->iYear,
                           song->iTimesPlayed, song->iStartOffset,
                           song->iEndOffset,
                           song->lastPlayed,
                           song->rating,
                           song->iKaraokeNumber);
    for (VECARTISTCREDITS::iterator artistCredit = song->artistCredits.begin(); artistCredit != song->artistCredits.end(); ++artistCredit)
    {
      artistCredit->idArtist = AddArtist(artistCredit->GetArtist(),
                                         artistCredit->GetMusicBrainzArtistID());
      AddSongArtist(artistCredit->idArtist,
                    song->idSong,
                    artistCredit->GetArtist(),
                    artistCredit->GetJoinPhrase(), // we don't have song artist breakdowns from scrapers, yet
                    artistCredit == song->artistCredits.begin() ? false : true,
                    std::distance(song->artistCredits.begin(), artistCredit));
    }
  }
  for (VECSONGS::const_iterator infoSong = album.infoSongs.begin(); infoSong != album.infoSongs.end(); ++infoSong)
    AddAlbumInfoSong(album.idAlbum, *infoSong);

  for (std::map<std::string, std::string>::const_iterator albumArt = album.art.begin();
                                                          albumArt != album.art.end();
                                                        ++albumArt)
    SetArtForItem(album.idAlbum, MediaTypeAlbum, albumArt->first, albumArt->second);

  CommitTransaction();
  return true;
}

bool CMusicDatabase::UpdateAlbum(CAlbum& album)
{
  BeginTransaction();

  UpdateAlbum(album.idAlbum,
              album.strAlbum, album.strMusicBrainzAlbumID,
              GetArtistString(album.artistCredits), album.GetGenreString(),
              StringUtils::Join(album.moods, g_advancedSettings.m_musicItemSeparator).c_str(),
              StringUtils::Join(album.styles, g_advancedSettings.m_musicItemSeparator).c_str(),
              StringUtils::Join(album.themes, g_advancedSettings.m_musicItemSeparator).c_str(),
              album.strReview,
              album.thumbURL.m_xml.c_str(),
              album.strLabel, album.strType,
              album.iRating, album.iYear, album.bCompilation);

  // Add the album artists
  DeleteAlbumArtistsByAlbum(album.idAlbum);
  for (VECARTISTCREDITS::iterator artistCredit = album.artistCredits.begin(); artistCredit != album.artistCredits.end(); ++artistCredit)
  {
    artistCredit->idArtist = AddArtist(artistCredit->GetArtist(),
                                       artistCredit->GetMusicBrainzArtistID());
    AddAlbumArtist(artistCredit->idArtist,
                   album.idAlbum,
                   artistCredit->GetArtist(),
                   artistCredit->GetJoinPhrase(),
                   artistCredit == album.artistCredits.begin() ? false : true,
                   std::distance(album.artistCredits.begin(), artistCredit));
  }

  for (VECSONGS::iterator song = album.songs.begin(); song != album.songs.end(); ++song)
  {
    UpdateSong(song->idSong,
               song->strTitle,
               song->strMusicBrainzTrackID,
               song->strFileName,
               song->strComment,
               song->strThumb,
               GetArtistString(song->artistCredits),
               song->genre,
               song->iTrack,
               song->iDuration,
               song->iYear,
               song->iTimesPlayed,
               song->iStartOffset,
               song->iEndOffset,
               song->lastPlayed,
               song->rating,
               song->iKaraokeNumber);
    DeleteSongArtistsBySong(song->idSong);
    for (VECARTISTCREDITS::iterator artistCredit = song->artistCredits.begin(); artistCredit != song->artistCredits.end(); ++artistCredit)
    {
      artistCredit->idArtist = AddArtist(artistCredit->GetArtist(),
                                         artistCredit->GetMusicBrainzArtistID());
      AddSongArtist(artistCredit->idArtist,
                    song->idSong,
                    artistCredit->GetArtist(),
                    artistCredit->GetJoinPhrase(),
                    artistCredit == song->artistCredits.begin() ? false : true,
                    std::distance(song->artistCredits.begin(), artistCredit));
    }
  }
  for (VECSONGS::const_iterator infoSong = album.infoSongs.begin(); infoSong != album.infoSongs.end(); ++infoSong)
    AddAlbumInfoSong(album.idAlbum, *infoSong);

  if (!album.art.empty())
    SetArtForItem(album.idAlbum, MediaTypeAlbum, album.art);

  CommitTransaction();
  return true;
}

int CMusicDatabase::AddSong(const int idAlbum,
                            const CStdString& strTitle, const CStdString& strMusicBrainzTrackID,
                            const CStdString& strPathAndFileName, const CStdString& strComment, const CStdString& strThumb,
                            const std::string &artistString, const std::vector<std::string>& genres,
                            int iTrack, int iDuration, int iYear,
                            const int iTimesPlayed, int iStartOffset, int iEndOffset,
                            const CDateTime& dtLastPlayed, char rating, int iKaraokeNumber)
{
  int idSong = -1;
  CStdString strSQL;
  try
  {
    // We need at least the title
    if (strTitle.empty())
      return -1;

    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    CStdString strPath, strFileName;
    URIUtils::Split(strPathAndFileName, strPath, strFileName);
    int idPath = AddPath(strPath);

    bool bHasKaraoke = false;
#ifdef HAS_KARAOKE
    bHasKaraoke = CKaraokeLyricsFactory::HasLyrics(strPathAndFileName);
#endif

    if (!strMusicBrainzTrackID.empty())
      strSQL = PrepareSQL("SELECT * FROM song WHERE idAlbum = %i AND strMusicBrainzTrackID = '%s'",
                          idAlbum,
                          strMusicBrainzTrackID.c_str());
    else
      strSQL = PrepareSQL("SELECT * FROM song WHERE idAlbum=%i AND strFileName='%s' AND strTitle='%s' AND strMusicBrainzTrackID IS NULL",
                          idAlbum,
                          strFileName.c_str(),
                          strTitle.c_str());

    if (!m_pDS->query(strSQL.c_str()))
      return -1;

    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      strSQL=PrepareSQL("INSERT INTO song (idSong,idAlbum,idPath,strArtists,strGenres,strTitle,iTrack,iDuration,iYear,strFileName,strMusicBrainzTrackID,iTimesPlayed,iStartOffset,iEndOffset,lastplayed,rating,comment) values (NULL, %i, %i, '%s', '%s', '%s', %i, %i, %i, '%s'",
                    idAlbum,
                    idPath,
                    artistString.c_str(),
                    StringUtils::Join(genres, g_advancedSettings.m_musicItemSeparator).c_str(),
                    strTitle.c_str(),
                    iTrack, iDuration, iYear,
                    strFileName.c_str());

      if (strMusicBrainzTrackID.empty())
        strSQL += PrepareSQL(",NULL");
      else
        strSQL += PrepareSQL(",'%s'", strMusicBrainzTrackID.c_str());

      if (dtLastPlayed.IsValid())
        strSQL += PrepareSQL(",%i,%i,%i,'%s','%c','%s')",
                      iTimesPlayed, iStartOffset, iEndOffset, dtLastPlayed.GetAsDBDateTime().c_str(), rating, strComment.c_str());
      else
        strSQL += PrepareSQL(",%i,%i,%i,NULL,'%c','%s')",
                      iTimesPlayed, iStartOffset, iEndOffset, rating, strComment.c_str());
      m_pDS->exec(strSQL.c_str());
      idSong = (int)m_pDS->lastinsertid();
    }
    else
    {
      idSong = m_pDS->fv("idSong").get_asInt();
      m_pDS->close();
      UpdateSong(idSong, strTitle, strMusicBrainzTrackID, strPathAndFileName, strComment, strThumb, artistString, genres, iTrack, iDuration, iYear, iTimesPlayed, iStartOffset, iEndOffset, dtLastPlayed, rating,  iKaraokeNumber);
    }

    if (!strThumb.empty())
      SetArtForItem(idSong, MediaTypeSong, "thumb", strThumb);

    unsigned int index = 0;
    // If this is karaoke song, change the genre to 'Karaoke' (and add it if it's not there)
    if ( bHasKaraoke && g_advancedSettings.m_karaokeChangeGenreForKaraokeSongs )
    {
      int idGenre = AddGenre("Karaoke");
      AddSongGenre(idGenre, idSong, index);
      AddAlbumGenre(idGenre, idAlbum, index++);
    }
    for (vector<string>::const_iterator i = genres.begin(); i != genres.end(); ++i)
    {
      // index will be wrong for albums, but ordering is not all that relevant
      // for genres anyway
      int idGenre = AddGenre(*i);
      AddSongGenre(idGenre, idSong, index);
      AddAlbumGenre(idGenre, idAlbum, index++);
    }

    // Add karaoke information (if any)
    if (bHasKaraoke)
      AddKaraokeData(idSong, iKaraokeNumber);

    AnnounceUpdate(MediaTypeSong, idSong);
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

    CStdString strSQL=PrepareSQL("SELECT songview.*,songartistview.* FROM songview "
                                 " LEFT JOIN songartistview ON songview.idSong = songartistview.idSong "
                                 " WHERE songview.idSong = %i", idSong);

    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return false;
    }

    int songArtistOffset = song_enumCount;

    set<int> artistcredits;
    song = GetSongFromDataset(m_pDS.get()->get_sql_record());
    while (!m_pDS->eof())
    {
      const dbiplus::sql_record* const record = m_pDS.get()->get_sql_record();

      int idSongArtist = record->at(songArtistOffset + artistCredit_idArtist).get_asInt();
      if (artistcredits.find(idSongArtist) == artistcredits.end())
      {
        song.artistCredits.push_back(GetArtistCreditFromDataset(record, songArtistOffset));
        artistcredits.insert(idSongArtist);
      }

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

int CMusicDatabase::UpdateSong(int idSong, const CSong &song)
{
  return UpdateSong(idSong,
                    song.strTitle,
                    song.strMusicBrainzTrackID,
                    song.strFileName,
                    song.strComment,
                    song.strThumb,
                    StringUtils::Join(song.artist, g_advancedSettings.m_musicItemSeparator), // NOTE: Don't call this function internally!!!
                    song.genre,
                    song.iTrack,
                    song.iDuration,
                    song.iYear,
                    song.iTimesPlayed,
                    song.iStartOffset,
                    song.iEndOffset,
                    song.lastPlayed,
                    song.rating,
                    song.iKaraokeNumber);
}

int CMusicDatabase::UpdateSong(int idSong,
                               const CStdString& strTitle, const CStdString& strMusicBrainzTrackID,
                               const CStdString& strPathAndFileName, const CStdString& strComment, const CStdString& strThumb,
                               const std::string& artistString, const std::vector<std::string>& genres,
                               int iTrack, int iDuration, int iYear,
                               int iTimesPlayed, int iStartOffset, int iEndOffset,
                               const CDateTime& dtLastPlayed, char rating, int iKaraokeNumber)
{
  if (idSong < 0)
    return -1;

  CStdString strSQL;
  CStdString strPath, strFileName;
  URIUtils::Split(strPathAndFileName, strPath, strFileName);
  int idPath = AddPath(strPath);

  strSQL = PrepareSQL("UPDATE song SET idPath = %i, strArtists = '%s', strGenres = '%s', strTitle = '%s', iTrack = %i, iDuration = %i, iYear = %i, strFileName = '%s'",
                      idPath,
                      artistString.c_str(),
                      StringUtils::Join(genres, g_advancedSettings.m_musicItemSeparator).c_str(),
                      strTitle.c_str(),
                      iTrack, iDuration, iYear,
                      strFileName.c_str());
  if (strMusicBrainzTrackID.empty())
    strSQL += PrepareSQL(", strMusicBrainzTrackID = NULL");
  else
    strSQL += PrepareSQL(", strMusicBrainzTrackID = '%s'", strMusicBrainzTrackID.c_str());

  if (dtLastPlayed.IsValid())
    strSQL += PrepareSQL(", iTimesPlayed = %i, iStartOffset = %i, iEndOffset = %i, lastplayed = '%s', rating = '%c', comment = '%s'",
                         iTimesPlayed, iStartOffset, iEndOffset, dtLastPlayed.GetAsDBDateTime().c_str(), rating, strComment.c_str());
  else
    strSQL += PrepareSQL(", iTimesPlayed = %i, iStartOffset = %i, iEndOffset = %i, lastplayed = NULL, rating = '%c', comment = '%s'",
                         iTimesPlayed, iStartOffset, iEndOffset, rating, strComment.c_str());
  strSQL += PrepareSQL(" WHERE idSong = %i", idSong);

  bool status = ExecuteQuery(strSQL);
  if (status)
    AnnounceUpdate(MediaTypeSong, idSong);
  return idSong;
}

int CMusicDatabase::AddAlbum(const CStdString& strAlbum, const CStdString& strMusicBrainzAlbumID,
                             const CStdString& strArtist, const CStdString& strGenre, int year, bool bCompilation)
{
  CStdString strSQL;
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    if (!strMusicBrainzAlbumID.empty())
      strSQL = PrepareSQL("SELECT * FROM album WHERE strMusicBrainzAlbumID = '%s'",
                        strMusicBrainzAlbumID.c_str());
    else
      strSQL = PrepareSQL("SELECT * FROM album WHERE strArtists LIKE '%s' AND strAlbum LIKE '%s' AND strMusicBrainzAlbumID IS NULL",
                          strArtist.c_str(),
                          strAlbum.c_str());
    m_pDS->query(strSQL.c_str());

    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      // doesnt exists, add it
      if (strMusicBrainzAlbumID.empty())
        strSQL=PrepareSQL("insert into album (idAlbum, strAlbum, strMusicBrainzAlbumID, strArtists, strGenres, iYear, bCompilation) values( NULL, '%s', NULL, '%s', '%s', %i, %i)",
                          strAlbum.c_str(),
                          strArtist.c_str(),
                          strGenre.c_str(),
                          year,
                          bCompilation);
      else
        strSQL=PrepareSQL("insert into album (idAlbum, strAlbum, strMusicBrainzAlbumID, strArtists, strGenres, iYear, bCompilation) values( NULL, '%s', '%s', '%s', '%s', %i, %i)",
                          strAlbum.c_str(),
                          strMusicBrainzAlbumID.c_str(),
                          strArtist.c_str(),
                          strGenre.c_str(),
                          year,
                          bCompilation);
      m_pDS->exec(strSQL.c_str());

      return (int)m_pDS->lastinsertid();
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
      int idAlbum = m_pDS->fv("idAlbum").get_asInt();
      m_pDS->close();
      if (strMusicBrainzAlbumID.empty())
        strSQL=PrepareSQL("UPDATE album SET strGenres = '%s', iYear=%i, bCompilation=%i, lastScraped = NULL WHERE idAlbum=%i",
                          strGenre.c_str(),
                          year,
                          bCompilation,
                          idAlbum);
      else
        strSQL=PrepareSQL("UPDATE album SET strAlbum = '%s', strArtists = '%s', strGenres = '%s', iYear=%i, bCompilation=%i, lastScraped = NULL WHERE idAlbum=%i",
                          strAlbum.c_str(),
                          strArtist.c_str(),
                          strGenre.c_str(),
                          year,
                          bCompilation,
                          idAlbum);
      m_pDS->exec(strSQL.c_str());
      DeleteAlbumArtistsByAlbum(idAlbum);
      DeleteAlbumGenresByAlbum(idAlbum);
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
                                 const CStdString& strAlbum, const CStdString& strMusicBrainzAlbumID,
                                 const CStdString& strArtist, const CStdString& strGenre,
                                 const CStdString& strMoods, const CStdString& strStyles,
                                 const CStdString& strThemes, const CStdString& strReview,
                                 const CStdString& strImage, const CStdString& strLabel,
                                 const CStdString& strType,
                                 int iRating, int iYear, bool bCompilation)
{
  if (idAlbum < 0)
    return -1;

  CStdString strSQL;
  strSQL = PrepareSQL("UPDATE album SET "
                      " strAlbum = '%s', strArtists = '%s', strGenres = '%s', "
                      " strMoods = '%s', strStyles = '%s', strThemes = '%s', "
                      " strReview = '%s', strImage = '%s', strLabel = '%s', "
                      " strType = '%s', iRating = %i,"
                      " iYear = %i, bCompilation = %i, lastScraped = '%s'",
                      strAlbum.c_str(), strArtist.c_str(), strGenre.c_str(),
                      strMoods.c_str(), strStyles.c_str(), strThemes.c_str(),
                      strReview.c_str(), strImage.c_str(), strLabel.c_str(),
                      strType.c_str(), iRating,
                      iYear, bCompilation,
                      CDateTime::GetCurrentDateTime().GetAsDBDateTime().c_str());
  if (strMusicBrainzAlbumID.empty())
    strSQL += PrepareSQL(", strMusicBrainzAlbumID = NULL");
  else
    strSQL += PrepareSQL(", strMusicBrainzAlbumID = '%s'", strMusicBrainzAlbumID.c_str());

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

    CStdString sql;
    if (getSongs)
    {
      sql = PrepareSQL("SELECT albumview.*,albumartistview.*,songview.*,songartistview.*,albuminfosong.* "
                       "  FROM albumview "
                       "  LEFT JOIN albumartistview ON albumview.idAlbum = albumartistview.idAlbum "
                       "  JOIN songview ON albumview.idAlbum = songview.idAlbum "
                       "  LEFT JOIN songartistview ON songview.idSong = songartistview.idSong "
                       "  LEFT JOIN albuminfosong ON albumview.idAlbum = albuminfosong.idAlbumInfo "
                       "  WHERE albumview.idAlbum = %ld "
                       "  ORDER BY albumartistview.iOrder, songview.iTrack, songartistview.iOrder", idAlbum);
    }
    else
    {
      sql = PrepareSQL("SELECT albumview.*,albumartistview.* "
                       "  FROM albumview "
                       "  LEFT JOIN albumartistview ON albumview.idAlbum = albumartistview.idAlbum "
                       "  WHERE albumview.idAlbum = %ld "
                       "  ORDER BY albumartistview.iOrder", idAlbum);
    }

    CLog::Log(LOGDEBUG, "%s", sql.c_str());
    if (!m_pDS->query(sql.c_str())) return false;
    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      return false;
    }

    int albumArtistOffset = album_enumCount;
    int songOffset = albumArtistOffset + artistCredit_enumCount;
    int songArtistOffset = songOffset + song_enumCount;
    int infoSongOffset = songArtistOffset + artistCredit_enumCount;

    set<int> artistcredits;
    set<int> songs;
    set<pair<int, int> > songartistcredits;
    set<int> infosongs;
    album = GetAlbumFromDataset(m_pDS.get()->get_sql_record(), 0, true); // true to grab and parse the imageURL
    while (!m_pDS->eof())
    {
      const dbiplus::sql_record* const record = m_pDS->get_sql_record();

      // Because rows repeat in the joined query (cartesian join) we may see each
      // entity (album artist, song, song artist) multiple times in the result set.
      // Since there should never be a song with the same artist twice, or an album
      // with the same song (by id) listed twice, we key on the entity ID and only
      // create an entity for the first occurence of each entity in the data set.
      int idAlbumArtist = record->at(albumArtistOffset + artistCredit_idArtist).get_asInt();
      if (artistcredits.find(idAlbumArtist) == artistcredits.end())
      {
        album.artistCredits.push_back(GetArtistCreditFromDataset(record, albumArtistOffset));
        artistcredits.insert(idAlbumArtist);
      }

      if (getSongs)
      {
        int idSong = record->at(songOffset + song_idSong).get_asInt();
        if (songs.find(idSong) == songs.end())
        {
          album.songs.push_back(GetSongFromDataset(record, songOffset));
          songs.insert(idSong);
        }

        int idSongArtistSong = record->at(songArtistOffset + artistCredit_idEntity).get_asInt();
        int idSongArtistArtist = record->at(songArtistOffset + artistCredit_idArtist).get_asInt();
        if (songartistcredits.find(make_pair(idSongArtistSong, idSongArtistArtist)) == songartistcredits.end())
        {
          for (VECSONGS::iterator si = album.songs.begin(); si != album.songs.end(); ++si)
            if (si->idSong == idSongArtistSong)
              si->artistCredits.push_back(GetArtistCreditFromDataset(record, songArtistOffset));
          songartistcredits.insert(make_pair(idSongArtistSong, idSongArtistArtist));
        }

        int idAlbumInfoSong = m_pDS.get()->get_sql_record()->at(infoSongOffset + albumInfoSong_idAlbumInfoSong).get_asInt();
        if (infosongs.find(idAlbumInfoSong) == infosongs.end())
        {
          album.infoSongs.push_back(GetAlbumInfoSongFromDataset(record, infoSongOffset));
          infosongs.insert(idAlbumInfoSong);
        }
      }
      m_pDS->next();
    }
    m_pDS->close(); // cleanup recordset data
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
  CStdString strSQL = PrepareSQL("UPDATE album SET lastScraped = NULL WHERE idAlbum = %i", idAlbum);
  return ExecuteQuery(strSQL);
}

bool CMusicDatabase::HasAlbumBeenScraped(int idAlbum)
{
  CStdString strSQL = PrepareSQL("SELECT idAlbum FROM album WHERE idAlbum = %i AND lastScraped IS NULL", idAlbum);
  return GetSingleValue(strSQL).empty();
}

int CMusicDatabase::AddGenre(const CStdString& strGenre1)
{
  CStdString strSQL;
  try
  {
    CStdString strGenre = strGenre1;
    StringUtils::Trim(strGenre);

    if (strGenre.empty())
      strGenre=g_localizeStrings.Get(13205); // Unknown

    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;
    map <CStdString, int>::const_iterator it;

    it = m_genreCache.find(strGenre);
    if (it != m_genreCache.end())
      return it->second;


    strSQL=PrepareSQL("select * from genre where strGenre like '%s'", strGenre.c_str());
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      // doesnt exists, add it
      strSQL=PrepareSQL("insert into genre (idGenre, strGenre) values( NULL, '%s' )", strGenre.c_str());
      m_pDS->exec(strSQL.c_str());

      int idGenre = (int)m_pDS->lastinsertid();
      m_genreCache.insert(pair<CStdString, int>(strGenre1, idGenre));
      return idGenre;
    }
    else
    {
      int idGenre = m_pDS->fv("idGenre").get_asInt();
      m_genreCache.insert(pair<CStdString, int>(strGenre1, idGenre));
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
  UpdateArtist(artist.idArtist,
               artist.strArtist, artist.strMusicBrainzArtistID,
               artist.strBorn, artist.strFormed,
               StringUtils::Join(artist.genre, g_advancedSettings.m_musicItemSeparator),
               StringUtils::Join(artist.moods, g_advancedSettings.m_musicItemSeparator),
               StringUtils::Join(artist.styles, g_advancedSettings.m_musicItemSeparator),
               StringUtils::Join(artist.instruments, g_advancedSettings.m_musicItemSeparator),
               artist.strBiography, artist.strDied,
               artist.strDisbanded,
               StringUtils::Join(artist.yearsActive, g_advancedSettings.m_musicItemSeparator).c_str(),
               artist.thumbURL.m_xml.c_str(),
               artist.fanart.m_xml.c_str());

  DeleteArtistDiscography(artist.idArtist);
  for (std::vector<std::pair<CStdString,CStdString> >::const_iterator disc = artist.discography.begin();
                                                                      disc != artist.discography.end();
                                                                    ++disc)
  {
    AddArtistDiscography(artist.idArtist, disc->first, disc->second);
  }

  return true;
}

int CMusicDatabase::AddArtist(const CStdString& strArtist, const CStdString& strMusicBrainzArtistID)
{
  CStdString strSQL;
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    // 1) MusicBrainz
    if (!strMusicBrainzArtistID.empty())
    {
      // 1.a) Match on a MusicBrainz ID
      strSQL = PrepareSQL("SELECT * FROM artist WHERE strMusicBrainzArtistID = '%s'",
                          strMusicBrainzArtistID.c_str());
      m_pDS->query(strSQL.c_str());
      if (m_pDS->num_rows() > 0)
      {
        int idArtist = (int)m_pDS->fv("idArtist").get_asInt();
        m_pDS->close();
        return idArtist;
      }
      m_pDS->close();


      // 1.b) No match on MusicBrainz ID. Look for a previously added artist with no MusicBrainz ID
      //     and update that if it exists.
      strSQL = PrepareSQL("SELECT * FROM artist WHERE strArtist LIKE '%s' AND strMusicBrainzArtistID IS NULL", strArtist.c_str());
      m_pDS->query(strSQL.c_str());
      if (m_pDS->num_rows() > 0)
      {
        int idArtist = (int)m_pDS->fv("idArtist").get_asInt();
        m_pDS->close();
        // 1.b.a) We found an artist by name but with no MusicBrainz ID set, update it and assume it is our artist
        strSQL = PrepareSQL("UPDATE artist SET strArtist = '%s', strMusicBrainzArtistID = '%s' WHERE idArtist = %i",
                            strArtist.c_str(),
                            strMusicBrainzArtistID.c_str(),
                            idArtist);
        m_pDS->exec(strSQL.c_str());
        return idArtist;
      }

    // 2) No MusicBrainz - search for any artist (MB ID or non) with the same name.
    //    With MusicBrainz IDs this could return multiple artists and is non-determinstic
    //    Always pick the first artist ID returned by the DB to return.
    }
    else
    {
      strSQL = PrepareSQL("SELECT * FROM artist WHERE strArtist LIKE '%s'",
                          strArtist.c_str());

      m_pDS->query(strSQL.c_str());
      if (m_pDS->num_rows() > 0)
      {
        int idArtist = (int)m_pDS->fv("idArtist").get_asInt();
        m_pDS->close();
        return idArtist;
      }
      m_pDS->close();
    }

    // 3) No artist exists at all - add it
    if (strMusicBrainzArtistID.empty())
      strSQL = PrepareSQL("INSERT INTO artist (idArtist, strArtist, strMusicBrainzArtistID) VALUES( NULL, '%s', NULL )",
                          strArtist.c_str());
    else
      strSQL = PrepareSQL("INSERT INTO artist (idArtist, strArtist, strMusicBrainzArtistID) VALUES( NULL, '%s', '%s' )",
                          strArtist.c_str(),
                          strMusicBrainzArtistID.c_str());

    m_pDS->exec(strSQL.c_str());
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
                                  const CStdString& strArtist, const CStdString& strMusicBrainzArtistID,
                                  const CStdString& strBorn, const CStdString& strFormed,
                                  const CStdString& strGenres, const CStdString& strMoods,
                                  const CStdString& strStyles, const CStdString& strInstruments,
                                  const CStdString& strBiography, const CStdString& strDied,
                                  const CStdString& strDisbanded, const CStdString& strYearsActive,
                                  const CStdString& strImage, const CStdString& strFanart)
{
  CScraperUrl thumbURL;
  CFanart fanart;
  if (idArtist < 0)
    return -1;

  CStdString strSQL;
  strSQL = PrepareSQL("UPDATE artist SET "
                      " strArtist = '%s', "
                      " strBorn = '%s', strFormed = '%s', strGenres = '%s', "
                      " strMoods = '%s', strStyles = '%s', strInstruments = '%s', "
                      " strBiography = '%s', strDied = '%s', strDisbanded = '%s', "
                      " strYearsActive = '%s', strImage = '%s', strFanart = '%s', "
                      " lastScraped = '%s'",
                      strArtist.c_str(), /* strMusicBrainzArtistID.c_str(), */
                      strBorn.c_str(), strFormed.c_str(), strGenres.c_str(),
                      strMoods.c_str(), strStyles.c_str(), strInstruments.c_str(),
                      strBiography.c_str(), strDied.c_str(), strDisbanded.c_str(),
                      strYearsActive.c_str(), strImage.c_str(), strFanart.c_str(),
                      CDateTime::GetCurrentDateTime().GetAsDBDateTime().c_str());
  if (strMusicBrainzArtistID.empty())
    strSQL += PrepareSQL(", strMusicBrainzArtistID = NULL");
  else
    strSQL += PrepareSQL(", strMusicBrainzArtistID = '%s'", strMusicBrainzArtistID.c_str());

  strSQL += PrepareSQL(" WHERE idArtist = %i", idArtist);

  bool status = ExecuteQuery(strSQL);
  if (status)
    AnnounceUpdate(MediaTypeArtist, idArtist);
  return idArtist;
}

bool CMusicDatabase::GetArtist(int idArtist, CArtist &artist, bool fetchAll /* = false */)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    if (idArtist == -1)
      return false; // not in the database

    CStdString strSQL;
    if (fetchAll)
      strSQL = PrepareSQL("SELECT * FROM artistview LEFT JOIN discography ON artistview.idArtist = discography.idArtist WHERE artistview.idArtist = %i", idArtist);
    else
      strSQL = PrepareSQL("SELECT * FROM artistview WHERE artistview.idArtist = %i", idArtist);

    if (!m_pDS->query(strSQL.c_str())) return false;
    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      return false;
    }

    int discographyOffset = artist_enumCount;

    artist.discography.clear();
    artist = GetArtistFromDataset(m_pDS.get()->get_sql_record(), 0, fetchAll);
    if (fetchAll)
    {
      while (!m_pDS->eof())
      {
        const dbiplus::sql_record* const record = m_pDS.get()->get_sql_record();

        artist.discography.push_back(make_pair(record->at(discographyOffset + 1).get_asString(), record->at(discographyOffset + 2).get_asString()));
        m_pDS->next();
      }
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

bool CMusicDatabase::HasArtistBeenScraped(int idArtist)
{
  CStdString strSQL = PrepareSQL("SELECT idArtist FROM artist WHERE idArtist = %i AND lastScraped IS NULL", idArtist);
  return GetSingleValue(strSQL).empty();
}

bool CMusicDatabase::ClearArtistLastScrapedTime(int idArtist)
{
  CStdString strSQL = PrepareSQL("UPDATE artist SET lastScraped = NULL WHERE idArtist = %i", idArtist);
  return ExecuteQuery(strSQL);
}

int CMusicDatabase::AddArtistDiscography(int idArtist, const CStdString& strAlbum, const CStdString& strYear)
{
  CStdString strSQL=PrepareSQL("INSERT INTO discography (idArtist, strAlbum, strYear) values(%i, '%s', '%s')",
                               idArtist,
                               strAlbum.c_str(),
                               strYear.c_str());
  return ExecuteQuery(strSQL);
}

bool CMusicDatabase::DeleteArtistDiscography(int idArtist)
{
  CStdString strSQL = PrepareSQL("DELETE FROM discography WHERE idArtist = %i", idArtist);
  return ExecuteQuery(strSQL);
}

bool CMusicDatabase::AddSongArtist(int idArtist, int idSong, std::string strArtist, std::string joinPhrase, bool featured, int iOrder)
{
  CStdString strSQL;
  strSQL=PrepareSQL("replace into song_artist (idArtist, idSong, strArtist, strJoinPhrase, boolFeatured, iOrder) values(%i,%i,'%s','%s',%i,%i)",
                    idArtist, idSong, strArtist.c_str(), joinPhrase.c_str(), featured == true ? 1 : 0, iOrder);
  return ExecuteQuery(strSQL);
};

bool CMusicDatabase::DeleteSongArtistsBySong(int idSong)
{
  return ExecuteQuery(PrepareSQL("DELETE FROM song_artist WHERE idSong = %i", idSong));
}

bool CMusicDatabase::AddAlbumArtist(int idArtist, int idAlbum, std::string strArtist, std::string joinPhrase, bool featured, int iOrder)
{
  CStdString strSQL;
  strSQL=PrepareSQL("replace into album_artist (idArtist, idAlbum, strArtist, strJoinPhrase, boolFeatured, iOrder) values(%i,%i,'%s','%s',%i,%i)",
                    idArtist, idAlbum, strArtist.c_str(), joinPhrase.c_str(), featured == true ? 1 : 0, iOrder);
  return ExecuteQuery(strSQL);
};

bool CMusicDatabase::DeleteAlbumArtistsByAlbum(int idAlbum)
{
  return ExecuteQuery(PrepareSQL("DELETE FROM album_artist WHERE idAlbum = %i", idAlbum));
}

bool CMusicDatabase::AddSongGenre(int idGenre, int idSong, int iOrder)
{
  if (idGenre == -1 || idSong == -1)
    return true;

  CStdString strSQL;
  strSQL=PrepareSQL("replace into song_genre (idGenre, idSong, iOrder) values(%i,%i,%i)",
                    idGenre, idSong, iOrder);
  return ExecuteQuery(strSQL);
};

bool CMusicDatabase::DeleteSongGenresBySong(int idSong)
{
  return ExecuteQuery(PrepareSQL("DELETE FROM song_genre WHERE idSong = %i", idSong));
}

bool CMusicDatabase::AddAlbumGenre(int idGenre, int idAlbum, int iOrder)
{
  if (idGenre == -1 || idAlbum == -1)
    return true;
  
  CStdString strSQL;
  strSQL=PrepareSQL("replace into album_genre (idGenre, idAlbum, iOrder) values(%i,%i,%i)",
                    idGenre, idAlbum, iOrder);
  return ExecuteQuery(strSQL);
};

bool CMusicDatabase::DeleteAlbumGenresByAlbum(int idAlbum)
{
  return ExecuteQuery(PrepareSQL("DELETE FROM album_genre WHERE idAlbum = %i", idAlbum));
}

bool CMusicDatabase::GetAlbumsByArtist(int idArtist, bool includeFeatured, std::vector<int> &albums)
{
  try 
  {
    CStdString strSQL, strPrepSQL;

    strPrepSQL = "select idAlbum from album_artist where idArtist=%i";
    if (includeFeatured == false)
      strPrepSQL += " AND boolFeatured = 0";
    
    strSQL=PrepareSQL(strPrepSQL, idArtist);
    if (!m_pDS->query(strSQL.c_str())) 
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

bool CMusicDatabase::GetArtistsByAlbum(int idAlbum, bool includeFeatured, std::vector<int> &artists)
{
  try 
  {
    CStdString strSQL, strPrepSQL;

    strPrepSQL = "select idArtist from album_artist where idAlbum=%i";
    if (includeFeatured == false)
      strPrepSQL += " AND boolFeatured = 0";

    strSQL=PrepareSQL(strPrepSQL, idAlbum);
    if (!m_pDS->query(strSQL.c_str())) 
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
    CLog::Log(LOGERROR, "%s(%i) failed", __FUNCTION__, idAlbum);
  }
  return false;
}

bool CMusicDatabase::GetSongsByArtist(int idArtist, bool includeFeatured, std::vector<int> &songs)
{
  try 
  {
    CStdString strSQL, strPrepSQL;
    
    strPrepSQL = "select idSong from song_artist where idArtist=%i";
    if (includeFeatured == false)
      strPrepSQL += " AND boolFeatured = 0";

    strSQL=PrepareSQL(strPrepSQL, idArtist);
    if (!m_pDS->query(strSQL.c_str())) 
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

bool CMusicDatabase::GetArtistsBySong(int idSong, bool includeFeatured, std::vector<int> &artists)
{
  try 
  {
    CStdString strSQL, strPrepSQL;
    
    strPrepSQL = "select idArtist from song_artist where idSong=%i";
    if (includeFeatured == false)
      strPrepSQL += " AND boolFeatured = 0";
    
    strSQL=PrepareSQL(strPrepSQL, idSong);
    if (!m_pDS->query(strSQL.c_str())) 
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

bool CMusicDatabase::GetGenresByAlbum(int idAlbum, std::vector<int>& genres)
{
  try
  {
    CStdString strSQL = PrepareSQL("select idGenre from album_genre where idAlbum = %i ORDER BY iOrder ASC", idAlbum);
    if (!m_pDS->query(strSQL.c_str()))
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
    CLog::Log(LOGERROR, "%s(%i) failed", __FUNCTION__, idAlbum);
  }
  return false;
}

bool CMusicDatabase::GetGenresBySong(int idSong, std::vector<int>& genres)
{
  try
  {
    CStdString strSQL = PrepareSQL("select idGenre from song_genre where idSong = %i ORDER BY iOrder ASC", idSong);
    if (!m_pDS->query(strSQL.c_str()))
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

int CMusicDatabase::AddPath(const CStdString& strPath1)
{
  CStdString strSQL;
  try
  {
    CStdString strPath(strPath1);
    if (!URIUtils::HasSlashAtEnd(strPath))
      URIUtils::AddSlashAtEnd(strPath);

    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    map <CStdString, int>::const_iterator it;

    it = m_pathCache.find(strPath);
    if (it != m_pathCache.end())
      return it->second;

    strSQL=PrepareSQL( "select * from path where strPath='%s'", strPath.c_str());
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      // doesnt exists, add it
      strSQL=PrepareSQL("insert into path (idPath, strPath) values( NULL, '%s' )", strPath.c_str());
      m_pDS->exec(strSQL.c_str());

      int idPath = (int)m_pDS->lastinsertid();
      m_pathCache.insert(pair<CStdString, int>(strPath, idPath));
      return idPath;
    }
    else
    {
      int idPath = m_pDS->fv("idPath").get_asInt();
      m_pathCache.insert(pair<CStdString, int>(strPath, idPath));
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
  // get the full artist string
  song.artist = StringUtils::Split(record->at(offset + song_strArtists).get_asString(), g_advancedSettings.m_musicItemSeparator);
  // and the full genre string
  song.genre = StringUtils::Split(record->at(offset + song_strGenres).get_asString(), g_advancedSettings.m_musicItemSeparator);
  // and the rest...
  song.strAlbum = record->at(offset + song_strAlbum).get_asString();
  song.idAlbum = record->at(offset + song_idAlbum).get_asInt();
  song.iTrack = record->at(offset + song_iTrack).get_asInt() ;
  song.iDuration = record->at(offset + song_iDuration).get_asInt() ;
  song.iYear = record->at(offset + song_iYear).get_asInt() ;
  song.strTitle = record->at(offset + song_strTitle).get_asString();
  song.iTimesPlayed = record->at(offset + song_iTimesPlayed).get_asInt();
  song.lastPlayed.SetFromDBDateTime(record->at(offset + song_lastplayed).get_asString());
  song.iStartOffset = record->at(offset + song_iStartOffset).get_asInt();
  song.iEndOffset = record->at(offset + song_iEndOffset).get_asInt();
  song.strMusicBrainzTrackID = record->at(offset + song_strMusicBrainzTrackID).get_asString();
  song.rating = record->at(offset + song_rating).get_asChar();
  song.strComment = record->at(offset + song_comment).get_asString();
  song.iKaraokeNumber = record->at(offset + song_iKarNumber).get_asInt();
  song.strKaraokeLyrEncoding = record->at(offset + song_strKarEncoding).get_asString();
  song.iKaraokeDelay = record->at(offset + song_iKarDelay).get_asInt();
  song.bCompilation = record->at(offset + song_bCompilation).get_asInt() == 1;
  song.albumArtist = StringUtils::Split(record->at(offset + song_strAlbumArtists).get_asString(), g_advancedSettings.m_musicItemSeparator);

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
  // get the full artist string
  item->GetMusicInfoTag()->SetArtist(StringUtils::Split(record->at(song_strArtists).get_asString(), g_advancedSettings.m_musicItemSeparator));
  // and the full genre string
  item->GetMusicInfoTag()->SetGenre(record->at(song_strGenres).get_asString());
  // and the rest...
  item->GetMusicInfoTag()->SetAlbum(record->at(song_strAlbum).get_asString());
  item->GetMusicInfoTag()->SetAlbumId(record->at(song_idAlbum).get_asInt());
  item->GetMusicInfoTag()->SetTrackAndDiscNumber(record->at(song_iTrack).get_asInt());
  item->GetMusicInfoTag()->SetDuration(record->at(song_iDuration).get_asInt());
  item->GetMusicInfoTag()->SetDatabaseId(record->at(song_idSong).get_asInt(), MediaTypeSong);
  SYSTEMTIME stTime;
  stTime.wYear = (WORD)record->at(song_iYear).get_asInt();
  item->GetMusicInfoTag()->SetReleaseDate(stTime);
  item->GetMusicInfoTag()->SetTitle(record->at(song_strTitle).get_asString());
  item->SetLabel(record->at(song_strTitle).get_asString());
  item->m_lStartOffset = record->at(song_iStartOffset).get_asInt();
  item->SetProperty("item_start", item->m_lStartOffset);
  item->m_lEndOffset = record->at(song_iEndOffset).get_asInt();
  item->GetMusicInfoTag()->SetMusicBrainzTrackID(record->at(song_strMusicBrainzTrackID).get_asString());
  item->GetMusicInfoTag()->SetRating(record->at(song_rating).get_asChar());
  item->GetMusicInfoTag()->SetComment(record->at(song_comment).get_asString());
  item->GetMusicInfoTag()->SetPlayCount(record->at(song_iTimesPlayed).get_asInt());
  item->GetMusicInfoTag()->SetLastPlayed(record->at(song_lastplayed).get_asString());
  CStdString strRealPath = URIUtils::AddFileToFolder(record->at(song_strPath).get_asString(), record->at(song_strFileName).get_asString());
  item->GetMusicInfoTag()->SetURL(strRealPath);
  item->GetMusicInfoTag()->SetCompilation(record->at(song_bCompilation).get_asInt() == 1);
  item->GetMusicInfoTag()->SetAlbumArtist(record->at(song_strAlbumArtists).get_asString());
  item->GetMusicInfoTag()->SetLoaded(true);
  // Get filename with full path
  if (!baseUrl.IsValid())
    item->SetPath(strRealPath);
  else
  {
    CMusicDbUrl itemUrl = baseUrl;
    CStdString strFileName = record->at(song_strFileName).get_asString();
    CStdString strExt = URIUtils::GetExtension(strFileName);
    CStdString path = StringUtils::Format("%i%s", record->at(song_idSong).get_asInt(), strExt.c_str());
    itemUrl.AppendPath(path);
    item->SetPath(itemUrl.ToString());
  }
}

CAlbum CMusicDatabase::GetAlbumFromDataset(dbiplus::Dataset* pDS, int offset /* = 0 */, bool imageURL /* = false*/)
{
  return GetAlbumFromDataset(pDS->get_sql_record(), offset, imageURL);
}

CAlbum CMusicDatabase::GetAlbumFromDataset(const dbiplus::sql_record* const record, int offset /* = 0 */, bool imageURL /* = false*/)
{
  CAlbum album;
  album.idAlbum = record->at(offset + album_idAlbum).get_asInt();
  album.strAlbum = record->at(offset + album_strAlbum).get_asString();
  if (album.strAlbum.empty())
    album.strAlbum = g_localizeStrings.Get(1050);
  album.strMusicBrainzAlbumID = record->at(offset + album_strMusicBrainzAlbumID).get_asString();
  album.artist = StringUtils::Split(record->at(offset + album_strArtists).get_asString(), g_advancedSettings.m_musicItemSeparator);
  album.genre = StringUtils::Split(record->at(offset + album_strGenres).get_asString(), g_advancedSettings.m_musicItemSeparator);
  album.iYear = record->at(offset + album_iYear).get_asInt();
  if (imageURL)
    album.thumbURL.ParseString(record->at(offset + album_strThumbURL).get_asString());
  album.iRating = record->at(offset + album_iRating).get_asInt();
  album.iYear = record->at(offset + album_iYear).get_asInt();
  album.strReview = record->at(offset + album_strReview).get_asString();
  album.styles = StringUtils::Split(record->at(offset + album_strStyles).get_asString(), g_advancedSettings.m_musicItemSeparator);
  album.moods = StringUtils::Split(record->at(offset + album_strMoods).get_asString(), g_advancedSettings.m_musicItemSeparator);
  album.themes = StringUtils::Split(record->at(offset + album_strThemes).get_asString(), g_advancedSettings.m_musicItemSeparator);
  album.strLabel = record->at(offset + album_strLabel).get_asString();
  album.strType = record->at(offset + album_strType).get_asString();
  album.bCompilation = record->at(offset + album_bCompilation).get_asInt() == 1;
  album.iTimesPlayed = record->at(offset + album_iTimesPlayed).get_asInt();
  return album;
}

CArtistCredit CMusicDatabase::GetArtistCreditFromDataset(const dbiplus::sql_record* const record, int offset /* = 0 */)
{
  CArtistCredit artistCredit;
  artistCredit.idArtist = record->at(offset + artistCredit_idArtist).get_asInt();
  artistCredit.m_strArtist = record->at(offset + artistCredit_strArtist).get_asString();
  artistCredit.m_strMusicBrainzArtistID = record->at(offset + artistCredit_strMusicBrainzArtistID).get_asString();
  artistCredit.m_boolFeatured = record->at(offset + artistCredit_bFeatured).get_asBool();
  artistCredit.m_strJoinPhrase = record->at(offset + artistCredit_strJoinPhrase).get_asString();
  return artistCredit;
}

CArtist CMusicDatabase::GetArtistFromDataset(dbiplus::Dataset* pDS, int offset /* = 0 */, bool needThumb /* = true */)
{
  return GetArtistFromDataset(pDS->get_sql_record(), offset, needThumb);
}

CArtist CMusicDatabase::GetArtistFromDataset(const dbiplus::sql_record* const record, int offset /* = 0 */, bool needThumb /* = true */)
{
  CArtist artist;
  artist.idArtist = record->at(offset + artist_idArtist).get_asInt();
  artist.strArtist = record->at(offset + artist_strArtist).get_asString();
  artist.strMusicBrainzArtistID = record->at(offset + artist_strMusicBrainzArtistID).get_asString();
  artist.genre = StringUtils::Split(record->at(offset + artist_strGenres).get_asString(), g_advancedSettings.m_musicItemSeparator);
  artist.strBiography = record->at(offset + artist_strBiography).get_asString();
  artist.styles = StringUtils::Split(record->at(offset + artist_strStyles).get_asString(), g_advancedSettings.m_musicItemSeparator);
  artist.moods = StringUtils::Split(record->at(offset + artist_strMoods).get_asString(), g_advancedSettings.m_musicItemSeparator);
  artist.strBorn = record->at(offset + artist_strBorn).get_asString();
  artist.strFormed = record->at(offset + artist_strFormed).get_asString();
  artist.strDied = record->at(offset + artist_strDied).get_asString();
  artist.strDisbanded = record->at(offset + artist_strDisbanded).get_asString();
  artist.yearsActive = StringUtils::Split(record->at(offset + artist_strYearsActive).get_asString(), g_advancedSettings.m_musicItemSeparator);
  artist.instruments = StringUtils::Split(record->at(offset + artist_strInstruments).get_asString(), g_advancedSettings.m_musicItemSeparator);

  if (needThumb)
  {
    artist.fanart.m_xml = record->at(artist_strFanart).get_asString();
    artist.fanart.Unpack();
    artist.thumbURL.ParseString(record->at(artist_strImage).get_asString());
  }

  return artist;
}

CSong CMusicDatabase::GetAlbumInfoSongFromDataset(const dbiplus::sql_record* const record, int offset /* = 0 */)
{
  CSong song;
  song.iTrack = record->at(offset + albumInfoSong_iTrack).get_asInt();
  song.iDuration = record->at(offset + albumInfoSong_iDuration).get_asInt();
  song.strTitle = record->at(offset + albumInfoSong_strTitle).get_asString();
  return song;
}

bool CMusicDatabase::GetSongByFileName(const CStdString& strFileNameAndPath, CSong& song, int startOffset)
{
  song.Clear();
  CURL url(strFileNameAndPath);

  if (url.IsProtocol("musicdb"))
  {
    CStdString strFile = URIUtils::GetFileName(strFileNameAndPath);
    URIUtils::RemoveExtension(strFile);
    return GetSong(atol(strFile.c_str()), song);
  }

  if (NULL == m_pDB.get()) return false;
  if (NULL == m_pDS.get()) return false;

  CStdString strPath, strFileName;
  URIUtils::Split(strFileNameAndPath, strPath, strFileName);
  URIUtils::AddSlashAtEnd(strPath);

  CStdString strSQL = PrepareSQL("select idSong from songview "
                                 "where strFileName='%s' and strPath='%s'",
                                 strFileName.c_str(), strPath.c_str());
  if (startOffset)
    strSQL += PrepareSQL(" AND iStartOffset=%i", startOffset);

  int idSong = (int)strtol(GetSingleValue(strSQL).c_str(), NULL, 10);
  if (idSong > 0)
    return GetSong(idSong, song);

  return false;
}

int CMusicDatabase::GetAlbumIdByPath(const CStdString& strPath)
{
  try
  {
    CStdString strSQL=PrepareSQL("select distinct idAlbum from song join path on song.idPath = path.idPath where path.strPath='%s'", strPath.c_str());
    m_pDS->query(strSQL.c_str());
    if (m_pDS->eof())
      return -1;

    int idAlbum = m_pDS->fv(0).get_asInt();
    m_pDS->close();

    return idAlbum;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%s) failed", __FUNCTION__, strPath.c_str());
  }

  return false;
}

int CMusicDatabase::GetSongByArtistAndAlbumAndTitle(const CStdString& strArtist, const CStdString& strAlbum, const CStdString& strTitle)
{
  try
  {
    CStdString strSQL=PrepareSQL("select idSong from songview "
                                "where strArtists like '%s' and strAlbum like '%s' and "
                                "strTitle like '%s'",strArtist.c_str(),strAlbum.c_str(),strTitle.c_str());

    if (!m_pDS->query(strSQL.c_str())) return false;
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

bool CMusicDatabase::SearchArtists(const CStdString& search, CFileItemList &artists)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strVariousArtists = g_localizeStrings.Get(340).c_str();
    CStdString strSQL;
    if (search.size() >= MIN_FULL_SEARCH_LENGTH)
      strSQL=PrepareSQL("select * from artist "
                                "where (strArtist like '%s%%' or strArtist like '%% %s%%') and strArtist <> '%s' "
                                , search.c_str(), search.c_str(), strVariousArtists.c_str() );
    else
      strSQL=PrepareSQL("select * from artist "
                                "where strArtist like '%s%%' and strArtist <> '%s' "
                                , search.c_str(), strVariousArtists.c_str() );

    if (!m_pDS->query(strSQL.c_str())) return false;
    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      return false;
    }

    CStdString artistLabel(g_localizeStrings.Get(557)); // Artist
    while (!m_pDS->eof())
    {
      CStdString path = StringUtils::Format("musicdb://artists/%i/", m_pDS->fv(0).get_asInt());
      CFileItemPtr pItem(new CFileItem(path, true));
      CStdString label = StringUtils::Format("[%s] %s", artistLabel.c_str(), m_pDS->fv(1).get_asString().c_str());
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

bool CMusicDatabase::GetTop100(const CStdString& strBaseDir, CFileItemList& items)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CMusicDbUrl baseUrl;
    if (!strBaseDir.empty() && !baseUrl.FromString(strBaseDir))
      return false;

    CStdString strSQL="select * from songview "
                      "where iTimesPlayed>0 "
                      "order by iTimesPlayed desc "
                      "limit 100";

    CLog::Log(LOGDEBUG, "%s query: %s", __FUNCTION__, strSQL.c_str());
    if (!m_pDS->query(strSQL.c_str())) return false;
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

    // NOTE: The song.idAlbum is needed for the group by, as for some reason group by albumview.idAlbum doesn't work
    //       consistently - possibly an SQLite bug, as it works fine in SQLiteSpy (v3.3.17)
    CStdString strSQL = "select albumview.* from albumview "
                    "where albumview.iTimesPlayed>0 and albumview.strAlbum != '' "
                    "order by albumview.iTimesPlayed desc "
                    "limit 100 ";

    CLog::Log(LOGDEBUG, "%s query: %s", __FUNCTION__, strSQL.c_str());
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return true;
    }
    while (!m_pDS->eof())
    {
      albums.push_back(GetAlbumFromDataset(m_pDS.get()));
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

bool CMusicDatabase::GetTop100AlbumSongs(const CStdString& strBaseDir, CFileItemList& items)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CMusicDbUrl baseUrl;
    if (!strBaseDir.empty() && baseUrl.FromString(strBaseDir))
      return false;

    CStdString strSQL = StringUtils::Format("select * from songview join albumview on (songview.idAlbum = albumview.idAlbum) where albumview.idAlbum in (select song.idAlbum from song where song.iTimesPlayed>0 group by idAlbum order by sum(song.iTimesPlayed) desc limit 100) order by albumview.idAlbum in (select song.idAlbum from song where song.iTimesPlayed>0 group by idAlbum order by sum(song.iTimesPlayed) desc limit 100)");
    CLog::Log(LOGDEBUG,"GetTop100AlbumSongs() query: %s", strSQL.c_str());
    if (!m_pDS->query(strSQL.c_str())) return false;

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

    CStdString strSQL = StringUtils::Format("select distinct albumview.* from song join albumview on albumview.idAlbum=song.idAlbum where song.lastplayed IS NOT NULL order by song.lastplayed desc limit %i", RECENTLY_PLAYED_LIMIT);
    CLog::Log(LOGDEBUG, "%s query: %s", __FUNCTION__, strSQL.c_str());
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return true;
    }
    while (!m_pDS->eof())
    {
      albums.push_back(GetAlbumFromDataset(m_pDS.get()));
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

bool CMusicDatabase::GetRecentlyPlayedAlbumSongs(const CStdString& strBaseDir, CFileItemList& items)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CMusicDbUrl baseUrl;
    if (!strBaseDir.empty() && !baseUrl.FromString(strBaseDir))
      return false;

    CStdString strSQL = StringUtils::Format("select * from songview join albumview on (songview.idAlbum = albumview.idAlbum) where albumview.idAlbum in (select distinct albumview.idAlbum from albumview join song on albumview.idAlbum=song.idAlbum where song.lastplayed IS NOT NULL order by song.lastplayed desc limit %i)", g_advancedSettings.m_iMusicLibraryRecentlyAddedItems);
    CLog::Log(LOGDEBUG,"GetRecentlyPlayedAlbumSongs() query: %s", strSQL.c_str());
    if (!m_pDS->query(strSQL.c_str())) return false;

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

bool CMusicDatabase::GetRecentlyAddedAlbums(VECALBUMS& albums, unsigned int limit)
{
  try
  {
    albums.erase(albums.begin(), albums.end());
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL = StringUtils::Format("select * from albumview where strAlbum != '' order by idAlbum desc limit %u", limit ? limit : g_advancedSettings.m_iMusicLibraryRecentlyAddedItems);

    CLog::Log(LOGDEBUG, "%s query: %s", __FUNCTION__, strSQL.c_str());
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return true;
    }

    while (!m_pDS->eof())
    {
      albums.push_back(GetAlbumFromDataset(m_pDS.get()));
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

bool CMusicDatabase::GetRecentlyAddedAlbumSongs(const CStdString& strBaseDir, CFileItemList& items, unsigned int limit)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CMusicDbUrl baseUrl;
    if (!strBaseDir.empty() && !baseUrl.FromString(strBaseDir))
      return false;

    CStdString strSQL;
    strSQL = PrepareSQL("SELECT songview.* FROM (SELECT idAlbum FROM albumview ORDER BY idAlbum DESC LIMIT %u) AS recentalbums JOIN songview ON songview.idAlbum=recentalbums.idAlbum", limit ? limit : g_advancedSettings.m_iMusicLibraryRecentlyAddedItems);
    CLog::Log(LOGDEBUG,"GetRecentlyAddedAlbumSongs() query: %s", strSQL.c_str());
    if (!m_pDS->query(strSQL.c_str())) return false;

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

void CMusicDatabase::IncrementPlayCount(const CFileItem& item)
{
  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    int idSong = GetSongIDFromPath(item.GetPath());

    CStdString sql=PrepareSQL("UPDATE song SET iTimesPlayed=iTimesPlayed+1, lastplayed=CURRENT_TIMESTAMP where idSong=%i", idSong);
    m_pDS->exec(sql.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%s) failed", __FUNCTION__, item.GetPath().c_str());
  }
}

bool CMusicDatabase::GetSongsByPath(const CStdString& strPath1, MAPSONGS& songs, bool bAppendToMap)
{
  CStdString strPath(strPath1);
  try
  {
    if (!URIUtils::HasSlashAtEnd(strPath))
      URIUtils::AddSlashAtEnd(strPath);

    if (!bAppendToMap)
      songs.clear();

    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL=PrepareSQL("select * from songview where strPath='%s'", strPath.c_str() );
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return false;
    }
    while (!m_pDS->eof())
    {
      CSong song = GetSongFromDataset();
      songs.insert(make_pair(song.strFileName, song));
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
  m_artistCache.erase(m_artistCache.begin(), m_artistCache.end());
  m_genreCache.erase(m_genreCache.begin(), m_genreCache.end());
  m_pathCache.erase(m_pathCache.begin(), m_pathCache.end());
  m_albumCache.erase(m_albumCache.begin(), m_albumCache.end());
  m_thumbCache.erase(m_thumbCache.begin(), m_thumbCache.end());
}

bool CMusicDatabase::Search(const CStdString& search, CFileItemList &items)
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

bool CMusicDatabase::SearchSongs(const CStdString& search, CFileItemList &items)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CMusicDbUrl baseUrl;
    if (!baseUrl.FromString("musicdb://songs/"))
      return false;

    CStdString strSQL;
    if (search.size() >= MIN_FULL_SEARCH_LENGTH)
      strSQL=PrepareSQL("select * from songview where strTitle like '%s%%' or strTitle like '%% %s%%' limit 1000", search.c_str(), search.c_str());
    else
      strSQL=PrepareSQL("select * from songview where strTitle like '%s%%' limit 1000", search.c_str());

    if (!m_pDS->query(strSQL.c_str())) return false;
    if (m_pDS->num_rows() == 0) return false;

    CStdString songLabel = g_localizeStrings.Get(179); // Song
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

bool CMusicDatabase::SearchAlbums(const CStdString& search, CFileItemList &albums)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL;
    if (search.size() >= MIN_FULL_SEARCH_LENGTH)
      strSQL=PrepareSQL("select * from albumview where strAlbum like '%s%%' or strAlbum like '%% %s%%'", search.c_str(), search.c_str());
    else
      strSQL=PrepareSQL("select * from albumview where strAlbum like '%s%%'", search.c_str());

    if (!m_pDS->query(strSQL.c_str())) return false;

    CStdString albumLabel(g_localizeStrings.Get(558)); // Album
    while (!m_pDS->eof())
    {
      CAlbum album = GetAlbumFromDataset(m_pDS.get());
      CStdString path = StringUtils::Format("musicdb://albums/%ld/", album.idAlbum);
      CFileItemPtr pItem(new CFileItem(path, album));
      CStdString label = StringUtils::Format("[%s] %s", albumLabel.c_str(), album.strAlbum.c_str());
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

bool CMusicDatabase::CleanupSongsByIds(const CStdString &strSongIds)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    // ok, now find all idSong's
    CStdString strSQL=PrepareSQL("select * from song join path on song.idPath = path.idPath where song.idSong in %s", strSongIds.c_str());
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return true;
    }
    vector<std::string> songsToDelete;
    while (!m_pDS->eof())
    { // get the full song path
      CStdString strFileName = URIUtils::AddFileToFolder(m_pDS->fv("path.strPath").get_asString(), m_pDS->fv("song.strFileName").get_asString());

      //  Special case for streams inside an ogg file. (oggstream)
      //  The last dir in the path is the ogg file that
      //  contains the stream, so test if its there
      if (URIUtils::HasExtension(strFileName, ".oggstream|.nsfstream"))
      {
        strFileName = URIUtils::GetDirectory(strFileName);
        // we are dropping back to a file, so remove the slash at end
        URIUtils::RemoveSlashAtEnd(strFileName);
      }

      if (!CFile::Exists(strFileName))
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
      m_pDS->exec(strSQL.c_str());
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

bool CMusicDatabase::CleanupSongs()
{
  try
  {
    // run through all songs and get all unique path ids
    int iLIMIT = 1000;
    for (int i=0;;i+=iLIMIT)
    {
      CStdString strSQL=PrepareSQL("select song.idSong from song order by song.idSong limit %i offset %i",iLIMIT,i);
      if (!m_pDS->query(strSQL.c_str())) return false;
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
    CStdString strSQL = "select * from album where album.idAlbum not in (select idAlbum from song)";
    if (!m_pDS->query(strSQL.c_str())) return false;
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
    m_pDS->exec(strSQL.c_str());
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
    CStdString sql = "select * from path where idPath not in (select idPath from song)";
    if (!m_pDS->query(sql.c_str())) return false;
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
      CStdString path = m_pDS->fv("strPath").get_asString();
      CStdString sql = PrepareSQL("select count(idPath) from songpaths where SUBSTR(strPath,1,%i)='%s'", StringUtils::utf8_strlen(path.c_str()), path.c_str());
      if (m_pDS2->query(sql.c_str()) && m_pDS2->num_rows() == 1 && m_pDS2->fv(0).get_asInt() == 0)
        pathIds.push_back(m_pDS->fv("idPath").get_asString()); // nothing found, so delete
      m_pDS2->close();
      m_pDS->next();
    }
    m_pDS->close();

    if (!pathIds.empty())
    {
      // do the deletion, and drop our temp table
      std::string deleteSQL = "DELETE FROM path WHERE idPath IN (" + StringUtils::Join(pathIds, ",") + ")";
      m_pDS->exec(deleteSQL.c_str());
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

bool CMusicDatabase::InsideScannedPath(const CStdString& path)
{
  CStdString sql = PrepareSQL("select idPath from path where SUBSTR(strPath,1,%i)='%s' LIMIT 1", path.size(), path.c_str());
  return !GetSingleValue(sql).empty();
}

bool CMusicDatabase::CleanupArtists()
{
  try
  {
    // (nested queries by Bobbin007)
    // must be executed AFTER the song, album and their artist link tables are cleaned.
    // don't delete the "Various Artists" string

    // Create temp table to avoid 1442 trigger hell on mysql
    m_pDS->exec("CREATE TEMPORARY TABLE tmp_delartists (idArtist integer)");
    m_pDS->exec("INSERT INTO tmp_delartists select idArtist from song_artist");
    m_pDS->exec("INSERT INTO tmp_delartists select idArtist from album_artist");
    m_pDS->exec("delete from artist where idArtist not in (select idArtist from tmp_delartists)");
    m_pDS->exec("DROP TABLE tmp_delartists");
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
    // Cleanup orphaned genres (ie those that don't belong to a song or an album entry)
    // (nested queries by Bobbin007)
    // Must be executed AFTER the song, song_genre, album and album_genre tables have been cleaned.
    CStdString strSQL = "delete from genre where idGenre not in (select idGenre from song_genre) and";
    strSQL += " idGenre not in (select idGenre from album_genre)";
    m_pDS->exec(strSQL.c_str());
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "Exception in CMusicDatabase::CleanupGenres() or was aborted");
  }
  return false;
}

bool CMusicDatabase::CleanupOrphanedItems()
{
  // paths aren't cleaned up here - they're cleaned up in RemoveSongsFromPath()
  if (NULL == m_pDB.get()) return false;
  if (NULL == m_pDS.get()) return false;
  if (!CleanupAlbums()) return false;
  if (!CleanupArtists()) return false;
  if (!CleanupGenres()) return false;
  return true;
}

int CMusicDatabase::Cleanup(bool bShowProgress /* = true */)
{
  if (NULL == m_pDB.get()) return ERROR_DATABASE;
  if (NULL == m_pDS.get()) return ERROR_DATABASE;

  int ret = ERROR_OK;
  CGUIDialogProgress* pDlgProgress = NULL;
  unsigned int time = XbmcThreads::SystemClockMillis();
  CLog::Log(LOGNOTICE, "%s: Starting musicdatabase cleanup ..", __FUNCTION__);
  ANNOUNCEMENT::CAnnouncementManager::Get().Announce(ANNOUNCEMENT::AudioLibrary, "xbmc", "OnCleanStarted");

  // first cleanup any songs with invalid paths
  if (bShowProgress)
  {
    pDlgProgress = (CGUIDialogProgress*)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
    if (pDlgProgress)
    {
      pDlgProgress->SetHeading(700);
      pDlgProgress->SetLine(0, "");
      pDlgProgress->SetLine(1, 318);
      pDlgProgress->SetLine(2, 330);
      pDlgProgress->SetPercentage(0);
      pDlgProgress->StartModal();
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
    pDlgProgress->SetLine(1, 326);
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
    pDlgProgress->SetLine(1, 324);
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
    pDlgProgress->SetLine(1, 320);
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
    pDlgProgress->SetLine(1, 322);
    pDlgProgress->SetPercentage(80);
    pDlgProgress->Progress();
  }
  if (!CleanupGenres())
  {
    ret = ERROR_REORG_GENRE;
    goto error;
  }
  // commit transaction
  if (pDlgProgress)
  {
    pDlgProgress->SetLine(1, 328);
    pDlgProgress->SetPercentage(90);
    pDlgProgress->Progress();
  }
  if (!CommitTransaction())
  {
    ret = ERROR_WRITING_CHANGES;
    goto error;
  }
  // and compress the database
  if (pDlgProgress)
  {
    pDlgProgress->SetLine(1, 331);
    pDlgProgress->SetPercentage(100);
    pDlgProgress->Progress();
    pDlgProgress->Close();
  }
  time = XbmcThreads::SystemClockMillis() - time;
  CLog::Log(LOGNOTICE, "%s: Cleaning musicdatabase done. Operation took %s", __FUNCTION__, StringUtils::SecondsToTimeString(time / 1000).c_str());
  ANNOUNCEMENT::CAnnouncementManager::Get().Announce(ANNOUNCEMENT::AudioLibrary, "xbmc", "OnCleanFinished");

  if (!Compress(false))
  {
    return ERROR_COMPRESSING;
  }
  return ERROR_OK;

error:
  RollbackTransaction();
  ANNOUNCEMENT::CAnnouncementManager::Get().Announce(ANNOUNCEMENT::AudioLibrary, "xbmc", "OnCleanFinished");
  return ret;
}

bool CMusicDatabase::LookupCDDBInfo(bool bRequery/*=false*/)
{
#ifdef HAS_DVD_DRIVE
  if (!CSettings::Get().GetBool("audiocds.usecddb"))
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
    CStdString strFile = StringUtils::Format("%x.cddb", pCdInfo->GetCddbDiscId());
    CFile::Delete(URIUtils::AddFileToFolder(CProfilesManager::Get().GetCDDBFolder(), strFile));
  }

  // Prepare cddb
  Xcddb cddb;
  cddb.setCacheDir(CProfilesManager::Get().GetCDDBFolder());

  // Do we have to look for cddb information
  if (pCdInfo->HasCDDBInfo() && !cddb.isCDCached(pCdInfo))
  {
    CGUIDialogProgress* pDialogProgress = (CGUIDialogProgress*)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
    CGUIDialogSelect *pDlgSelect = (CGUIDialogSelect*)g_windowManager.GetWindow(WINDOW_DIALOG_SELECT);

    if (!pDialogProgress) return false;
    if (!pDlgSelect) return false;

    // Show progress dialog if we have to connect to freedb.org
    pDialogProgress->SetHeading(255); //CDDB
    pDialogProgress->SetLine(0, ""); // Querying freedb for CDDB info
    pDialogProgress->SetLine(1, 256);
    pDialogProgress->SetLine(2, "");
    pDialogProgress->ShowProgressBar(false);
    pDialogProgress->StartModal();

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
        pDlgSelect->SetHeading(255);
        int i = 1;
        while (1)
        {
          CStdString strTitle = cddb.getInexactTitle(i);
          if (strTitle == "") break;

          CStdString strArtist = cddb.getInexactArtist(i);
          if (!strArtist.empty())
            strTitle += " - " + strArtist;

          pDlgSelect->Add(strTitle);
          i++;
        }
        pDlgSelect->DoModal();

        // Has the user selected a match...
        int iSelectedCD = pDlgSelect->GetSelectedLabel();
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
        CStdString strErrorText = StringUtils::Format("[%d] %s", cddb.getLastError(), cddb.getLastErrorText());
        CGUIDialogOK::ShowAndGetInput(255, 257, strErrorText, 0);
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
  if (!CDirectory::GetDirectory(CProfilesManager::Get().GetCDDBFolder(), items, ".cddb", DIR_FLAG_NO_FILE_DIRS))
  {
    CGUIDialogOK::ShowAndGetInput(313, 426, 0, 0);
    return ;
  }
  // Show a selectdialog that the user can select the album to delete
  CGUIDialogSelect *pDlg = (CGUIDialogSelect*)g_windowManager.GetWindow(WINDOW_DIALOG_SELECT);
  if (pDlg)
  {
    pDlg->SetHeading(g_localizeStrings.Get(181).c_str());
    pDlg->Reset();

    map<ULONG, CStdString> mapCDDBIds;
    for (int i = 0; i < items.Size(); ++i)
    {
      if (items[i]->m_bIsFolder)
        continue;

      CStdString strFile = URIUtils::GetFileName(items[i]->GetPath());
      strFile.erase(strFile.size() - 5, 5);
      ULONG lDiscId = strtoul(strFile.c_str(), NULL, 16);
      Xcddb cddb;
      cddb.setCacheDir(CProfilesManager::Get().GetCDDBFolder());

      if (!cddb.queryCache(lDiscId))
        continue;

      CStdString strDiskTitle, strDiskArtist;
      cddb.getDiskTitle(strDiskTitle);
      cddb.getDiskArtist(strDiskArtist);

      CStdString str;
      if (strDiskArtist.empty())
        str = strDiskTitle;
      else
        str = strDiskTitle + " - " + strDiskArtist;

      pDlg->Add(str);
      mapCDDBIds.insert(pair<ULONG, CStdString>(lDiscId, str));
    }

    pDlg->Sort();
    pDlg->DoModal();

    // and wait till user selects one
    int iSelectedAlbum = pDlg->GetSelectedLabel();
    if (iSelectedAlbum < 0)
    {
      mapCDDBIds.erase(mapCDDBIds.begin(), mapCDDBIds.end());
      return ;
    }

    CStdString strSelectedAlbum = pDlg->GetSelectedLabelText();
    map<ULONG, CStdString>::iterator it;
    for (it = mapCDDBIds.begin();it != mapCDDBIds.end();it++)
    {
      if (it->second == strSelectedAlbum)
      {
        CStdString strFile = StringUtils::Format("%x.cddb", (unsigned int) it->first);
        CFile::Delete(URIUtils::AddFileToFolder(CProfilesManager::Get().GetCDDBFolder(), strFile));
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
    CGUIDialogOK::ShowAndGetInput(189, 14057, 0, 0);
    return;
  }

  if (CGUIDialogYesNo::ShowAndGetInput(313, 333, 0, 0))
  {
    CMusicDatabase musicdatabase;
    if (musicdatabase.Open())
    {
      int iReturnString = musicdatabase.Cleanup();
      musicdatabase.Close();

      if (iReturnString != ERROR_OK)
      {
        CGUIDialogOK::ShowAndGetInput(313, iReturnString, 0, 0);
      }
    }
  }
}

bool CMusicDatabase::GetGenresNav(const CStdString& strBaseDir, CFileItemList& items, const Filter &filter /* = Filter() */, bool countOnly /* = false */)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    // get primary genres for songs - could be simplified to just SELECT * FROM genre?
    CStdString strSQL = "SELECT %s FROM genre ";

    Filter extFilter = filter;
    CMusicDbUrl musicUrl;
    SortDescription sorting;
    if (!musicUrl.FromString(strBaseDir) || !GetFilter(musicUrl, extFilter, sorting))
      return false;

    // if there are extra WHERE conditions we might need access
    // to songview or albumview for these conditions
    if (extFilter.where.size() > 0)
    {
      if (extFilter.where.find("artistview") != string::npos)
        extFilter.AppendJoin("JOIN song_genre ON song_genre.idGenre = genre.idGenre JOIN songview ON songview.idSong = song_genre.idSong "
                             "JOIN song_artist ON song_artist.idSong = songview.idSong JOIN artistview ON artistview.idArtist = song_artist.idArtist");
      else if (extFilter.where.find("songview") != string::npos)
        extFilter.AppendJoin("JOIN song_genre ON song_genre.idGenre = genre.idGenre JOIN songview ON songview.idSong = song_genre.idSong");
      else if (extFilter.where.find("albumview") != string::npos)
        extFilter.AppendJoin("JOIN album_genre ON album_genre.idGenre = genre.idGenre JOIN albumview ON albumview.idAlbum = album_genre.idAlbum");

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

    if (!m_pDS->query(strSQL.c_str()))
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
      CStdString strDir = StringUtils::Format("%i/", m_pDS->fv("genre.idGenre").get_asInt());
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

bool CMusicDatabase::GetYearsNav(const CStdString& strBaseDir, CFileItemList& items, const Filter &filter /* = Filter() */)
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
    CStdString strSQL = "SELECT DISTINCT albumview.iYear FROM albumview ";
    extFilter.AppendWhere("albumview.iYear <> 0");

    if (!BuildSQL(strSQL, extFilter, strSQL))
      return false;

    // run query
    CLog::Log(LOGDEBUG, "%s query: %s", __FUNCTION__, strSQL.c_str());
    if (!m_pDS->query(strSQL.c_str()))
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
      stTime.wYear = (WORD)m_pDS->fv(0).get_asInt();
      pItem->GetMusicInfoTag()->SetReleaseDate(stTime);

      CMusicDbUrl itemUrl = musicUrl;
      CStdString strDir = StringUtils::Format("%i/", m_pDS->fv(0).get_asInt());
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

bool CMusicDatabase::GetAlbumsByYear(const CStdString& strBaseDir, CFileItemList& items, int year)
{
  CMusicDbUrl musicUrl;
  if (!musicUrl.FromString(strBaseDir))
    return false;

  musicUrl.AddOption("year", year);
  musicUrl.AddOption("show_singles", true); // allow singles to be listed
  
  Filter filter;
  return GetAlbumsByWhere(musicUrl.ToString(), filter, items);
}

bool CMusicDatabase::GetCommonNav(const CStdString &strBaseDir, const CStdString &table, const CStdString &labelField, CFileItemList &items, const Filter &filter /* = Filter() */, bool countOnly /* = false */)
{
  if (NULL == m_pDB.get()) return false;
  if (NULL == m_pDS.get()) return false;

  if (table.empty() || labelField.empty())
    return false;
  
  try
  {
    Filter extFilter = filter;
    CStdString strSQL = "SELECT %s FROM " + table + " ";
    extFilter.AppendGroup(labelField);
    extFilter.AppendWhere(labelField + " != ''");
    
    if (countOnly)
    {
      extFilter.fields = "COUNT(DISTINCT " + labelField + ")";
      extFilter.group.clear();
      extFilter.order.clear();
    }
    
    CMusicDbUrl musicUrl;
    if (!BuildSQL(strBaseDir, strSQL, extFilter, strSQL, musicUrl))
      return false;
    
    strSQL = PrepareSQL(strSQL, !extFilter.fields.empty() ? extFilter.fields.c_str() : labelField.c_str());
    
    // run query
    CLog::Log(LOGDEBUG, "%s query: %s", __FUNCTION__, strSQL.c_str());
    if (!m_pDS->query(strSQL.c_str()))
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
      string labelValue = m_pDS->fv(labelField).get_asString();
      CFileItemPtr pItem(new CFileItem(labelValue));
      
      CMusicDbUrl itemUrl = musicUrl;
      CStdString strDir = StringUtils::Format("%s/", labelValue.c_str());
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

bool CMusicDatabase::GetAlbumTypesNav(const CStdString &strBaseDir, CFileItemList &items, const Filter &filter /* = Filter() */, bool countOnly /* = false */)
{
  return GetCommonNav(strBaseDir, "albumview", "albumview.strType", items, filter, countOnly);
}

bool CMusicDatabase::GetMusicLabelsNav(const CStdString &strBaseDir, CFileItemList &items, const Filter &filter /* = Filter() */, bool countOnly /* = false */)
{
  return GetCommonNav(strBaseDir, "albumview", "albumview.strLabel", items, filter, countOnly);
}

bool CMusicDatabase::GetArtistsNav(const CStdString& strBaseDir, CFileItemList& items, bool albumArtistsOnly /* = false */, int idGenre /* = -1 */, int idAlbum /* = -1 */, int idSong /* = -1 */, const Filter &filter /* = Filter() */, const SortDescription &sortDescription /* = SortDescription() */, bool countOnly /* = false */)
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

bool CMusicDatabase::GetArtistsByWhere(const CStdString& strBaseDir, const Filter &filter, CFileItemList& items, const SortDescription &sortDescription /* = SortDescription() */, bool countOnly /* = false */)
{
  if (NULL == m_pDB.get()) return false;
  if (NULL == m_pDS.get()) return false;

  try
  {
    int total = -1;

    CStdString strSQL = "SELECT %s FROM artistview ";

    Filter extFilter = filter;
    CMusicDbUrl musicUrl;
    SortDescription sorting = sortDescription;
    if (!musicUrl.FromString(strBaseDir) || !GetFilter(musicUrl, extFilter, sorting))
      return false;

    // if there are extra WHERE conditions we might need access
    // to songview or albumview for these conditions
    if (extFilter.where.size() > 0)
    {
      bool extended = false;
      if (extFilter.where.find("songview") != string::npos)
      {
        extended = true;
        extFilter.AppendJoin("JOIN song_artist ON song_artist.idArtist = artistview.idArtist JOIN songview ON songview.idSong = song_artist.idSong");
      }
      else if (extFilter.where.find("albumview") != string::npos)
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

    // Apply the limiting directly here if there's no special sorting but limiting
    if (extFilter.limit.empty() &&
        sortDescription.sortBy == SortByNone &&
       (sortDescription.limitStart > 0 || sortDescription.limitEnd > 0))
    {
      total = (int)strtol(GetSingleValue(PrepareSQL(strSQL, "COUNT(1)") + strSQLExtra, m_pDS).c_str(), NULL, 10);
      strSQLExtra += DatabaseUtils::BuildLimitClause(sortDescription.limitEnd, sortDescription.limitStart);
    }

    strSQL = PrepareSQL(strSQL.c_str(), !extFilter.fields.empty() && extFilter.fields.compare("*") != 0 ? extFilter.fields.c_str() : "artistview.*") + strSQLExtra;

    // run query
    CLog::Log(LOGDEBUG, "%s query: %s", __FUNCTION__, strSQL.c_str());
    if (!m_pDS->query(strSQL.c_str())) return false;
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

    // store the total value of items as a property
    if (total < iRowsFound)
      total = iRowsFound;
    items.SetProperty("total", total);
    
    DatabaseResults results;
    results.reserve(iRowsFound);
    if (!SortUtils::SortFromDataset(sortDescription, MediaTypeArtist, m_pDS, results))
      return false;

    // get data from returned rows
    items.Reserve(results.size());
    const dbiplus::query_data &data = m_pDS->get_result_set().records;
    for (DatabaseResults::const_iterator it = results.begin(); it != results.end(); it++)
    {
      unsigned int targetRow = (unsigned int)it->at(FieldRow).asInteger();
      const dbiplus::sql_record* const record = data.at(targetRow);
      
      try
      {
        CArtist artist = GetArtistFromDataset(record, false);
        CFileItemPtr pItem(new CFileItem(artist));

        CMusicDbUrl itemUrl = musicUrl;
        CStdString path = StringUtils::Format("%ld/", artist.idArtist);
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

    CStdString strSQL = PrepareSQL("select albumview.* from song join albumview on song.idAlbum = albumview.idAlbum where song.idSong='%i'", idSong);
    if (!m_pDS->query(strSQL.c_str())) return false;
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

bool CMusicDatabase::GetAlbumsNav(const CStdString& strBaseDir, CFileItemList& items, int idGenre /* = -1 */, int idArtist /* = -1 */, const Filter &filter /* = Filter() */, const SortDescription &sortDescription /* = SortDescription() */, bool countOnly /* = false */)
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

bool CMusicDatabase::GetAlbumsByWhere(const CStdString &baseDir, const Filter &filter, CFileItemList &items, const SortDescription &sortDescription /* = SortDescription() */, bool countOnly /* = false */)
{
  if (m_pDB.get() == NULL || m_pDS.get() == NULL)
    return false;

  try
  {
    int total = -1;

    CStdString strSQL = "SELECT %s FROM albumview ";

    Filter extFilter = filter;
    CMusicDbUrl musicUrl;
    SortDescription sorting = sortDescription;
    if (!musicUrl.FromString(baseDir) || !GetFilter(musicUrl, extFilter, sorting))
      return false;

    // if there are extra WHERE conditions we might need access
    // to songview for these conditions
    if (extFilter.where.find("songview") != string::npos)
    {
      extFilter.AppendJoin("JOIN songview ON songview.idAlbum = albumview.idAlbum");
      extFilter.AppendGroup("albumview.idAlbum");
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

    strSQL = PrepareSQL(strSQL, !filter.fields.empty() && filter.fields.compare("*") != 0 ? filter.fields.c_str() : "albumview.*") + strSQLExtra;

    CLog::Log(LOGDEBUG, "%s query: %s", __FUNCTION__, strSQL.c_str());
    // run query
    unsigned int time = XbmcThreads::SystemClockMillis();
    if (!m_pDS->query(strSQL.c_str()))
      return false;
    CLog::Log(LOGDEBUG, "%s - query took %i ms",
              __FUNCTION__, XbmcThreads::SystemClockMillis() - time); time = XbmcThreads::SystemClockMillis();

    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound <= 0)
    {
      m_pDS->close();
      return true;
    }

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
    if (!SortUtils::SortFromDataset(sortDescription, MediaTypeAlbum, m_pDS, results))
      return false;

    // get data from returned rows
    items.Reserve(results.size());
    const dbiplus::query_data &data = m_pDS->get_result_set().records;
    for (DatabaseResults::const_iterator it = results.begin(); it != results.end(); it++)
    {
      unsigned int targetRow = (unsigned int)it->at(FieldRow).asInteger();
      const dbiplus::sql_record* const record = data.at(targetRow);
      
      try
      {
        CMusicDbUrl itemUrl = musicUrl;
        CStdString path = StringUtils::Format("%i/", record->at(album_idAlbum).get_asInt());
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
    return true;
  }
  catch (...)
  {
    m_pDS->close();
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, filter.where.c_str());
  }
  return false;
}

bool CMusicDatabase::GetSongsByWhere(const CStdString &baseDir, const Filter &filter, CFileItemList &items, const SortDescription &sortDescription /* = SortDescription() */)
{
  if (m_pDB.get() == NULL || m_pDS.get() == NULL)
    return false;

  try
  {
    unsigned int time = XbmcThreads::SystemClockMillis();
    int total = -1;

    CStdString strSQL = "SELECT %s FROM songview ";

    Filter extFilter = filter;
    CMusicDbUrl musicUrl;
    SortDescription sorting = sortDescription;
    if (!musicUrl.FromString(baseDir) || !GetFilter(musicUrl, extFilter, sorting))
      return false;

    // if there are extra WHERE conditions we might need access
    // to songview for these conditions
    if (extFilter.where.find("albumview") != string::npos)
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
    if (!m_pDS->query(strSQL.c_str()))
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
    for (DatabaseResults::const_iterator it = results.begin(); it != results.end(); it++)
    {
      unsigned int targetRow = (unsigned int)it->at(FieldRow).asInteger();
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

bool CMusicDatabase::GetSongsByYear(const CStdString& baseDir, CFileItemList& items, int year)
{
  CMusicDbUrl musicUrl;
  if (!musicUrl.FromString(baseDir))
    return false;

  musicUrl.AddOption("year", year);
  
  Filter filter;
  return GetSongsByWhere(baseDir, filter, items);
}

bool CMusicDatabase::GetSongsNav(const CStdString& strBaseDir, CFileItemList& items, int idGenre, int idArtist, int idAlbum, const SortDescription &sortDescription /* = SortDescription() */)
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
  return GetSongsByWhere(musicUrl.ToString(), filter, items, sortDescription);
}

void CMusicDatabase::UpdateTables(int version)
{
  if (version < 19)
  {
    int len = g_advancedSettings.m_musicItemSeparator.size() + 1;
    CStdString sql = PrepareSQL("UPDATE song SET strExtraArtists=SUBSTR(strExtraArtists,%i), strExtraGenres=SUBSTR(strExtraGenres,%i)", len, len);
    m_pDS->exec(sql.c_str());
    sql = PrepareSQL("UPDATE album SET strExtraArtists=SUBSTR(strExtraArtists,%i), strExtraGenres=SUBSTR(strExtraGenres,%i)", len, len);
    m_pDS->exec(sql.c_str());
  }

  if (version < 21)
  {
    m_pDS->exec("CREATE TABLE album_artist ( idArtist integer, idAlbum integer, boolFeatured integer, iOrder integer )\n");
    m_pDS->exec("INSERT INTO album_artist (idArtist, idAlbum, boolFeatured, iOrder) SELECT idArtist, idAlbum, 1, iPosition FROM exartistalbum");
    m_pDS->exec("REPLACE INTO album_artist (idArtist, idAlbum, boolFeatured, iOrder) SELECT idArtist, idAlbum, 0, 0 FROM album");

    CStdString strSQL;
    strSQL=PrepareSQL("SELECT album.idAlbum AS idAlbum, strExtraArtists,"
                      "  album.idArtist AS idArtist, strArtist FROM album "
                      "  LEFT OUTER JOIN artist ON album.idArtist=artist.idArtist");
    m_pDS->query(strSQL.c_str());

    VECALBUMS albums;
    while (!m_pDS->eof())
    {
      CAlbum album;
      album.idAlbum = m_pDS->fv("idAlbum").get_asInt();
      album.artist.push_back(m_pDS->fv("strArtist").get_asString());
      if (!m_pDS->fv("strExtraArtists").get_asString().empty())
      {
        std::vector<std::string> extraArtists = StringUtils::Split(m_pDS->fv("strExtraArtists").get_asString(), g_advancedSettings.m_musicItemSeparator);
        album.artist.insert(album.artist.end(), extraArtists.begin(), extraArtists.end());
      }
      albums.push_back(album);
      m_pDS->next();
    }
    m_pDS->close();
    m_pDS->exec("CREATE TABLE album_new ( idAlbum integer primary key, strAlbum varchar(256), strArtists text, idGenre integer, strExtraGenres text, iYear integer, idThumb integer)");
    m_pDS->exec("INSERT INTO album_new ( idAlbum, strAlbum, idGenre, strExtraGenres, iYear, idThumb ) SELECT idAlbum, strAlbum, idGenre, strExtraGenres, iYear, idThumb FROM album");

    for (VECALBUMS::iterator it = albums.begin(); it != albums.end(); ++it)
    {
      CStdString strSQL;
      strSQL = PrepareSQL("UPDATE album_new SET strArtists='%s' WHERE idAlbum=%i", StringUtils::Join(it->artist, g_advancedSettings.m_musicItemSeparator).c_str(), it->idAlbum);
      m_pDS->exec(strSQL);
    }

    m_pDS->exec("DROP TABLE album");
    m_pDS->exec("ALTER TABLE album_new RENAME TO album");
    m_pDS->exec("DROP TABLE IF EXISTS exartistalbum");
  }

  if (version < 22)
  {
    m_pDS->exec("CREATE TABLE song_artist ( idArtist integer, idSong integer, boolFeatured integer, iOrder integer )\n");
    m_pDS->exec("INSERT INTO song_artist (idArtist, idSong, boolFeatured, iOrder) SELECT idArtist, idSong, 1, iPosition FROM exartistsong");
    m_pDS->exec("REPLACE INTO song_artist (idArtist, idSong, boolFeatured, iOrder) SELECT idArtist, idSong, 0, 0 FROM song");

    CStdString strSQL;
    strSQL=PrepareSQL("SELECT song.idSong AS idSong, strExtraArtists,"
                      "  song.idArtist AS idArtist, strArtist FROM song "
                      "  LEFT OUTER JOIN artist ON song.idArtist=artist.idArtist");
    m_pDS->query(strSQL.c_str());

    VECSONGS songs;
    while (!m_pDS->eof())
    {
      CSong song;
      song.idSong = m_pDS->fv("idSong").get_asInt();
      song.artist.push_back(m_pDS->fv("strArtist").get_asString());
      if (!m_pDS->fv("strExtraArtists").get_asString().empty())
      {
        std::vector<std::string> extraArtists = StringUtils::Split(m_pDS->fv("strExtraArtists").get_asString(), g_advancedSettings.m_musicItemSeparator);
        song.artist.insert(song.artist.end(), extraArtists.begin(), extraArtists.end());
      }
      songs.push_back(song);
      m_pDS->next();
    }
    m_pDS->close();
    m_pDS->exec("CREATE TABLE song_new ( idSong integer primary key, idAlbum integer, idPath integer, strArtists text, idGenre integer, strExtraGenres text, strTitle varchar(512), iTrack integer, iDuration integer, iYear integer, dwFileNameCRC text, strFileName text, strMusicBrainzTrackID text, strMusicBrainzArtistID text, strMusicBrainzAlbumID text, strMusicBrainzAlbumArtistID text, strMusicBrainzTRMID text, iTimesPlayed integer, iStartOffset integer, iEndOffset integer, idThumb integer, lastplayed varchar(20) default NULL, rating char default '0', comment text)");
    m_pDS->exec("INSERT INTO song_new ( idSong, idAlbum, idPath, idGenre, strExtraGenres, strTitle, iTrack, iDuration, iYear, dwFileNameCRC, strFileName, strMusicBrainzTrackID, strMusicBrainzArtistID, strMusicBrainzAlbumID, strMusicBrainzAlbumArtistID, strMusicBrainzTRMID, iTimesPlayed, iStartOffset, iEndOffset, idThumb, lastplayed, rating, comment ) SELECT idSong, idAlbum, idPath, idGenre, strExtraGenres, strTitle, iTrack, iDuration, iYear, dwFileNameCRC, strFileName, strMusicBrainzTrackID, strMusicBrainzArtistID, strMusicBrainzAlbumID, strMusicBrainzAlbumArtistID, strMusicBrainzTRMID, iTimesPlayed, iStartOffset, iEndOffset, idThumb, lastplayed, rating, comment FROM song");

    for (VECSONGS::iterator it = songs.begin(); it != songs.end(); ++it)
    {
      CStdString strSQL;
      strSQL = PrepareSQL("UPDATE song_new SET strArtists='%s' WHERE idSong=%i", StringUtils::Join(it->artist, g_advancedSettings.m_musicItemSeparator).c_str(), it->idSong);
      m_pDS->exec(strSQL);
    }

    m_pDS->exec("DROP TABLE song");
    m_pDS->exec("ALTER TABLE song_new RENAME TO song");
    m_pDS->exec("DROP TABLE IF EXISTS exartistsong");
  }

  if (version < 23)
  {
    m_pDS->exec("CREATE TABLE album_genre ( idGenre integer, idAlbum integer, iOrder integer )\n");
    m_pDS->exec("INSERT INTO album_genre ( idGenre, idAlbum, iOrder) SELECT idGenre, idAlbum, iPosition FROM exgenrealbum");
    m_pDS->exec("REPLACE INTO album_genre ( idGenre, idAlbum, iOrder) SELECT idGenre, idAlbum, 0 FROM album");

    CStdString strSQL;
    strSQL=PrepareSQL("SELECT album.idAlbum AS idAlbum, strExtraGenres,"
                      "  album.idGenre AS idGenre, strGenre FROM album "
                      "  JOIN genre ON album.idGenre=genre.idGenre");
    m_pDS->query(strSQL.c_str());

    VECALBUMS albums;
    while (!m_pDS->eof())
    {
      CAlbum album;
      album.idAlbum = m_pDS->fv("idAlbum").get_asInt();
      album.genre.push_back(m_pDS->fv("strGenre").get_asString());
      if (!m_pDS->fv("strExtraGenres").get_asString().empty())
      {
        std::vector<std::string> extraGenres = StringUtils::Split(m_pDS->fv("strExtraGenres").get_asString(), g_advancedSettings.m_musicItemSeparator);
        album.genre.insert(album.genre.end(), extraGenres.begin(), extraGenres.end());
      }
      albums.push_back(album);
      m_pDS->next();
    }
    m_pDS->close();
    m_pDS->exec("CREATE TABLE album_new ( idAlbum integer primary key, strAlbum varchar(256), strArtists text, strGenres text, iYear integer, idThumb integer)");
    m_pDS->exec("INSERT INTO album_new ( idAlbum, strAlbum, strArtists, iYear, idThumb) SELECT idAlbum, strAlbum, strArtists, iYear, idThumb FROM album");

    for (VECALBUMS::iterator it = albums.begin(); it != albums.end(); ++it)
    {
      CStdString strSQL;
      strSQL = PrepareSQL("UPDATE album_new SET strGenres='%s' WHERE idAlbum=%i", StringUtils::Join(it->genre, g_advancedSettings.m_musicItemSeparator).c_str(), it->idAlbum);
      m_pDS->exec(strSQL);
    }

    m_pDS->exec("DROP TABLE album");
    m_pDS->exec("ALTER TABLE album_new RENAME TO album");
    m_pDS->exec("DROP TABLE IF EXISTS exgenrealbum");
  }

  if (version < 24)
  {
    m_pDS->exec("CREATE TABLE song_genre ( idGenre integer, idSong integer, iOrder integer )\n");
    m_pDS->exec("INSERT INTO song_genre ( idGenre, idSong, iOrder) SELECT idGenre, idSong, iPosition FROM exgenresong");
    m_pDS->exec("REPLACE INTO song_genre ( idGenre, idSong, iOrder) SELECT idGenre, idSong, 0 FROM song");

    CStdString strSQL;
    strSQL=PrepareSQL("SELECT song.idSong AS idSong, strExtraGenres,"
                      "  song.idGenre AS idGenre, strGenre FROM song "
                      "  JOIN genre ON song.idGenre=genre.idGenre");
    m_pDS->query(strSQL.c_str());

    VECSONGS songs;
    while (!m_pDS->eof())
    {
      CSong song;
      song.idSong = m_pDS->fv("idSong").get_asInt();
      song.genre.push_back(m_pDS->fv("strGenre").get_asString());
      if (!m_pDS->fv("strExtraGenres").get_asString().empty())
      {
        std::vector<std::string> extraGenres = StringUtils::Split(m_pDS->fv("strExtraGenres").get_asString(), g_advancedSettings.m_musicItemSeparator);
        song.genre.insert(song.genre.end(), extraGenres.begin(), extraGenres.end());
      }
      songs.push_back(song);
      m_pDS->next();
    }
    m_pDS->close();
    m_pDS->exec("CREATE TABLE song_new ( idSong integer primary key, idAlbum integer, idPath integer, strArtists text, strGenres text, strTitle varchar(512), iTrack integer, iDuration integer, iYear integer, dwFileNameCRC text, strFileName text, strMusicBrainzTrackID text, strMusicBrainzArtistID text, strMusicBrainzAlbumID text, strMusicBrainzAlbumArtistID text, strMusicBrainzTRMID text, iTimesPlayed integer, iStartOffset integer, iEndOffset integer, idThumb integer, lastplayed varchar(20) default NULL, rating char default '0', comment text)\n");
    m_pDS->exec("INSERT INTO song_new ( idSong, idAlbum, idPath, strArtists, strTitle, iTrack, iDuration, iYear, dwFileNameCRC, strFileName, strMusicBrainzTrackID, strMusicBrainzArtistID, strMusicBrainzAlbumID, strMusicBrainzAlbumArtistID, strMusicBrainzTRMID, iTimesPlayed, iStartOffset, iEndOffset, idThumb, lastplayed, rating, comment) SELECT idSong, idAlbum, idPath, strArtists, strTitle, iTrack, iDuration, iYear, dwFileNameCRC, strFileName, strMusicBrainzTrackID, strMusicBrainzArtistID, strMusicBrainzAlbumID, strMusicBrainzAlbumArtistID, strMusicBrainzTRMID, iTimesPlayed, iStartOffset, iEndOffset, idThumb, lastplayed, rating, comment FROM song");

    for (VECSONGS::iterator it = songs.begin(); it != songs.end(); ++it)
    {
      CStdString strSQL;
      strSQL = PrepareSQL("UPDATE song_new SET strGenres='%s' WHERE idSong=%i", StringUtils::Join(it->genre, g_advancedSettings.m_musicItemSeparator).c_str(), it->idSong);
      m_pDS->exec(strSQL);
    }

    m_pDS->exec("DROP TABLE song");
    m_pDS->exec("ALTER TABLE song_new RENAME TO song");
    m_pDS->exec("DROP TABLE IF EXISTS exgenresong");
  }

  if (version < 25)
  {
    m_pDS->exec("ALTER TABLE album ADD bCompilation integer not null default '0'");
  }

  if (version < 26)
  { // add art table
    m_pDS->exec("CREATE TABLE art(art_id INTEGER PRIMARY KEY, media_id INTEGER, media_type TEXT, type TEXT, url TEXT)");
  }

  if (version < 27)
  {
    m_pDS->exec("DROP TABLE IF EXISTS thumb");

    CMediaSettings::Get().SetMusicNeedsUpdate(27);
    CSettings::Get().Save();
  }

  if (version < 29)
  { // update old art URLs
    m_pDS->query("select art_id,url from art where url like 'image://%%'");
    vector< pair<int, string> > art;
    while (!m_pDS->eof())
    {
      art.push_back(make_pair(m_pDS->fv(0).get_asInt(), CURL(m_pDS->fv(1).get_asString()).Get()));
      m_pDS->next();
    }
    m_pDS->close();
    for (vector< pair<int, string> >::iterator i = art.begin(); i != art.end(); ++i)
      m_pDS->exec(PrepareSQL("update art set url='%s' where art_id=%d", i->second.c_str(), i->first));
  }
  if (version < 30)
  { // update URL encoded paths
    m_pDS->query("select idSong, strFileName from song");
    vector< pair<int, string> > files;
    while (!m_pDS->eof())
    {
      files.push_back(make_pair(m_pDS->fv(0).get_asInt(), m_pDS->fv(1).get_asString()));
      m_pDS->next();
    }
    m_pDS->close();

    for (vector< pair<int, string> >::iterator i = files.begin(); i != files.end(); ++i)
    {
      std::string filename = i->second;
      if (URIUtils::UpdateUrlEncoding(filename))
        m_pDS->exec(PrepareSQL("UPDATE song SET strFileName='%s' WHERE idSong=%d", filename.c_str(), i->first));
    }
  }

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

  if (version < 35)
  {
    m_pDS->exec("ALTER TABLE album_artist ADD strJoinPhrase text\n");
    m_pDS->exec("ALTER TABLE song_artist ADD strJoinPhrase text\n");
    CMediaSettings::Get().SetMusicNeedsUpdate(35);
    CSettings::Get().Save();
  }

  if (version < 36)
  {
    // translate legacy musicdb:// paths
    if (m_pDS->query("SELECT strPath FROM content"))
    {
      vector<string> contentPaths;
      while (!m_pDS->eof())
      {
        contentPaths.push_back(m_pDS->fv(0).get_asString());
        m_pDS->next();
      }
      m_pDS->close();

      for (vector<string>::const_iterator it = contentPaths.begin(); it != contentPaths.end(); it++)
      {
        std::string originalPath = *it;
        std::string path = CLegacyPathTranslation::TranslateMusicDbPath(originalPath);
        m_pDS->exec(PrepareSQL("UPDATE content SET strPath='%s' WHERE strPath='%s'", path.c_str(), originalPath.c_str()).c_str());
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
    CStdString sql = "select idArtist,strArtist from artist";
    m_pDS->query(sql.c_str());
    while (!m_pDS->eof())
    {
      m_pDS2->exec(PrepareSQL("UPDATE song_artist SET strArtist='%s' where idArtist=%i", m_pDS->fv(1).get_asString().c_str(), m_pDS->fv(0).get_asInt()));
      m_pDS2->exec(PrepareSQL("UPDATE album_artist SET strArtist='%s' where idArtist=%i", m_pDS->fv(1).get_asString().c_str(), m_pDS->fv(0).get_asInt()));
      m_pDS->next();
    }
    // drop the last separator if more than one
    m_pDS->exec("UPDATE song_artist SET strJoinPhrase = '' WHERE 100*idSong+iOrder IN (SELECT id FROM (SELECT 100*idSong+max(iOrder) AS id FROM song_artist GROUP BY idSong) AS sub)");
    m_pDS->exec("UPDATE album_artist SET strJoinPhrase = '' WHERE 100*idAlbum+iOrder IN (SELECT id FROM (SELECT 100*idAlbum+max(iOrder) AS id FROM album_artist GROUP BY idAlbum) AS sub)");
  }
  if (version < 48)
  { // null out columns that are no longer used
    m_pDS->exec("UPDATE song SET dwFileNameCRC=NULL, idThumb=NULL");
    m_pDS->exec("UPDATE karaokedata SET strKaraLyrFileCRC=NULL");
    m_pDS->exec("UPDATE album SET idThumb=NULL");
  }
}

int CMusicDatabase::GetSchemaVersion() const
{
  return 48;
}

unsigned int CMusicDatabase::GetSongIDs(const Filter &filter, vector<pair<int,int> > &songIDs)
{
  try
  {
    if (NULL == m_pDB.get()) return 0;
    if (NULL == m_pDS.get()) return 0;

    CStdString strSQL = "select idSong from songview ";
    if (!CDatabase::BuildSQL(strSQL, filter, strSQL))
      return false;

    if (!m_pDS->query(strSQL.c_str())) return 0;
    songIDs.clear();
    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      return 0;
    }
    songIDs.reserve(m_pDS->num_rows());
    while (!m_pDS->eof())
    {
      songIDs.push_back(make_pair<int,int>(1,m_pDS->fv(song_idSong).get_asInt()));
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

    CStdString strSQL = "select count(idSong) as NumSongs from songview ";
    if (!CDatabase::BuildSQL(strSQL, filter, strSQL))
      return false;

    if (!m_pDS->query(strSQL.c_str())) return false;
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

bool CMusicDatabase::GetAlbumPath(int idAlbum, std::string& path)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS2.get()) return false;

    path.clear();

    std::string strSQL=PrepareSQL("select strPath from song join path on song.idPath = path.idPath where song.idAlbum=%ld", idAlbum);
    if (!m_pDS2->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS2->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS2->close();
      return false;
    }

    // if this returns more than one path, we just grab the first one.  It's just for determining where to obtain + place
    // a local thumbnail
    path = m_pDS2->fv("strPath").get_asString();

    m_pDS2->close(); // cleanup recordset data
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%i) failed", __FUNCTION__, idAlbum);
  }

  return false;
}

bool CMusicDatabase::SaveAlbumThumb(int idAlbum, const CStdString& strThumb)
{
  SetArtForItem(idAlbum, MediaTypeAlbum, "thumb", strThumb);
  // TODO: We should prompt the user to update the art for songs
  CStdString sql = PrepareSQL("UPDATE art"
                              " SET url='-'"
                              " WHERE media_type='song'"
                              " AND type='thumb'"
                              " AND media_id IN"
                              " (SELECT idSong FROM song WHERE idAlbum=%ld)", idAlbum);
  ExecuteQuery(sql);
  return true;
}

bool CMusicDatabase::GetArtistPath(int idArtist, std::string &basePath)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS2.get()) return false;

    // find all albums from this artist, and all the paths to the songs from those albums
    std::string strSQL=PrepareSQL("SELECT strPath"
                                 "  FROM album_artist"
                                 "  JOIN song "
                                 "    ON album_artist.idAlbum = song.idAlbum"
                                 "  JOIN path"
                                 "    ON song.idPath = path.idPath"
                                 " WHERE album_artist.idArtist = %i"
                                 " GROUP BY song.idPath", idArtist);

    // run query
    if (!m_pDS2->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS2->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS2->close();
      return false;
    }

    // special case for single path - assume that we're in an artist/album/songs filesystem
    if (iRowsFound == 1)
    {
      URIUtils::GetParentPath(m_pDS2->fv("strPath").get_asString(), basePath);
      m_pDS2->close();
      return true;
    }

    // find the common path (if any) to these albums
    basePath.clear();
    while (!m_pDS2->eof())
    {
      std::string path = m_pDS2->fv("strPath").get_asString();
      if (basePath.empty())
        basePath = path;
      else
        URIUtils::GetCommonPath(basePath,path);

      m_pDS2->next();
    }

    // cleanup
    m_pDS2->close();
    return true;

  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}

int CMusicDatabase::GetArtistByName(const CStdString& strArtist)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL=PrepareSQL("select idArtist from artist where artist.strArtist like '%s'", strArtist.c_str());

    // run query
    if (!m_pDS->query(strSQL.c_str())) return false;
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

int CMusicDatabase::GetAlbumByName(const CStdString& strAlbum, const CStdString& strArtist)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL;
    if (strArtist.empty())
      strSQL=PrepareSQL("SELECT idAlbum FROM album WHERE album.strAlbum LIKE '%s'", strAlbum.c_str());
    else
      strSQL=PrepareSQL("SELECT album.idAlbum FROM album WHERE album.strAlbum LIKE '%s' AND album.strArtists LIKE '%s'", strAlbum.c_str(),strArtist.c_str());
    // run query
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound != 1)
    {
      m_pDS->close();
      return -1;
    }
    return m_pDS->fv("album.idAlbum").get_asInt();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return -1;
}

int CMusicDatabase::GetAlbumByName(const CStdString& strAlbum, const std::vector<std::string>& artist)
{
  return GetAlbumByName(strAlbum, StringUtils::Join(artist, g_advancedSettings.m_musicItemSeparator));
}

CStdString CMusicDatabase::GetGenreById(int id)
{
  return GetSingleValue("genre", "strGenre", PrepareSQL("idGenre=%i", id));
}

CStdString CMusicDatabase::GetArtistById(int id)
{
  return GetSingleValue("artist", "strArtist", PrepareSQL("idArtist=%i", id));
}

CStdString CMusicDatabase::GetAlbumById(int id)
{
  return GetSingleValue("album", "strAlbum", PrepareSQL("idAlbum=%i", id));
}

int CMusicDatabase::GetGenreByName(const CStdString& strGenre)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL;
    strSQL=PrepareSQL("select idGenre from genre where genre.strGenre like '%s'", strGenre.c_str());
    // run query
    if (!m_pDS->query(strSQL.c_str())) return false;
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

bool CMusicDatabase::GetRandomSong(CFileItem* item, int& idSong, const Filter &filter)
{
  try
  {
    idSong = -1;

    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    // We don't use PrepareSQL here, as the WHERE clause is already formatted
    CStdString strSQL = PrepareSQL("select %s from songview ", !filter.fields.empty() ? filter.fields.c_str() : "*");
    Filter extFilter = filter;
    extFilter.AppendOrder(PrepareSQL("RANDOM()"));
    extFilter.limit = "1";

    if (!CDatabase::BuildSQL(strSQL, extFilter, strSQL))
      return false;

    CLog::Log(LOGDEBUG, "%s query = %s", __FUNCTION__, strSQL.c_str());
    // run query
    if (!m_pDS->query(strSQL.c_str()))
      return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound != 1)
    {
      m_pDS->close();
      return false;
    }
    GetFileItemFromDataset(item, CMusicDbUrl());
    idSong = m_pDS->fv("songview.idSong").get_asInt();
    m_pDS->close();
    return true;
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "%s(%s) failed", __FUNCTION__, filter.where.c_str());
  }
  return false;
}

bool CMusicDatabase::GetCompilationAlbums(const CStdString& strBaseDir, CFileItemList& items)
{
  CMusicDbUrl musicUrl;
  if (!musicUrl.FromString(strBaseDir))
    return false;

  musicUrl.AddOption("compilation", true);
  
  Filter filter;
  return GetAlbumsByWhere(musicUrl.ToString(), filter, items);
}

bool CMusicDatabase::GetCompilationSongs(const CStdString& strBaseDir, CFileItemList& items)
{
  CMusicDbUrl musicUrl;
  if (!musicUrl.FromString(strBaseDir))
    return false;

  musicUrl.AddOption("compilation", true);

  Filter filter;
  return GetSongsByWhere(musicUrl.ToString(), filter, items);
}

int CMusicDatabase::GetCompilationAlbumsCount()
{
  return strtol(GetSingleValue("album", "count(idAlbum)", "bCompilation = 1").c_str(), NULL, 10);
}

bool CMusicDatabase::SetPathHash(const CStdString &path, const CStdString &hash)
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

    CStdString strSQL=PrepareSQL("update path set strHash='%s' where idPath=%ld", hash.c_str(), idPath);
    m_pDS->exec(strSQL.c_str());

    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s, %s) failed", __FUNCTION__, path.c_str(), hash.c_str());
  }

  return false;
}

bool CMusicDatabase::GetPathHash(const CStdString &path, CStdString &hash)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL=PrepareSQL("select strHash from path where strPath='%s'", path.c_str());
    m_pDS->query(strSQL.c_str());
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

bool CMusicDatabase::RemoveSongsFromPath(const CStdString &path1, MAPSONGS& songs, bool exact)
{
  // We need to remove all songs from this path, as their tags are going
  // to be re-read.  We need to remove all songs from the song table + all links to them
  // from the song link tables (as otherwise if a song is added back
  // to the table with the same idSong, these tables can't be cleaned up properly later)

  // TODO: SQLite probably doesn't allow this, but can we rely on that??

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
  CStdString path(path1);
  try
  {
    if (!URIUtils::HasSlashAtEnd(path))
      URIUtils::AddSlashAtEnd(path);

    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString where;
    if (exact)
      where = PrepareSQL(" where strPath='%s'", path.c_str());
    else
      where = PrepareSQL(" where SUBSTR(strPath,1,%i)='%s'", StringUtils::utf8_strlen(path.c_str()), path.c_str());
    CStdString sql = "select * from songview" + where;
    if (!m_pDS->query(sql.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound > 0)
    {
      std::vector<std::string> songIds;
      while (!m_pDS->eof())
      {
        CSong song = GetSongFromDataset();
        song.strThumb = GetArtForItem(song.idSong, MediaTypeSong, "thumb");
        songs.insert(make_pair(song.strFileName, song));
        songIds.push_back(PrepareSQL("%i", song.idSong));
        m_pDS->next();
      }
      m_pDS->close();

      //TODO: move this below the m_pDS->exec block, once UPnP doesn't rely on this anymore
      for (MAPSONGS::iterator songit = songs.begin(); songit != songs.end(); ++songit)
        AnnounceRemove(MediaTypeSong, songit->second.idSong);

      // and delete all songs, and anything linked to them
      sql = "delete from song where idSong in (" + StringUtils::Join(songIds, ",") + ")";
      m_pDS->exec(sql.c_str());
    }
    // and remove the path as well (it'll be re-added later on with the new hash if it's non-empty)
    sql = "delete from path" + where;
    m_pDS->exec(sql.c_str());
    return iRowsFound > 0;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, path.c_str());
  }
  return false;
}

bool CMusicDatabase::GetPaths(set<string> &paths)
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

bool CMusicDatabase::SetSongRating(const CStdString &filePath, char rating)
{
  try
  {
    if (filePath.empty()) return false;
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    int songID = GetSongIDFromPath(filePath);
    if (-1 == songID) return false;

    CStdString sql = PrepareSQL("update song set rating='%c' where idSong = %i", rating, songID);
    m_pDS->exec(sql.c_str());
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s,%c) failed", __FUNCTION__, filePath.c_str(), rating);
  }
  return false;
}

int CMusicDatabase::GetSongIDFromPath(const CStdString &filePath)
{
  // grab the where string to identify the song id
  CURL url(filePath);
  if (url.IsProtocol("musicdb"))
  {
    CStdString strFile=URIUtils::GetFileName(filePath);
    URIUtils::RemoveExtension(strFile);
    return atol(strFile.c_str());
  }
  // hit the db
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    CStdString strPath, strFileName;
    URIUtils::Split(filePath, strPath, strFileName);
    URIUtils::AddSlashAtEnd(strPath);

    CStdString sql = PrepareSQL("select idSong from song join path on song.idPath = path.idPath where song.strFileName='%s' and path.strPath='%s'", strFileName.c_str(), strPath.c_str());
    if (!m_pDS->query(sql.c_str())) return -1;

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
    g_infoManager.SetLibraryBool(LIBRARY_HAS_MUSIC, GetSongsCount() > 0);
    return true;
  }
  return false;
}

bool CMusicDatabase::SetScraperForPath(const CStdString& strPath, const ADDON::ScraperPtr& scraper)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    // wipe old settings
    CStdString strSQL = PrepareSQL("delete from content where strPath='%s'",strPath.c_str());
    m_pDS->exec(strSQL.c_str());

    // insert new settings
    strSQL = PrepareSQL("insert into content (strPath, strScraperPath, strContent, strSettings) values ('%s','%s','%s','%s')",
      strPath.c_str(), scraper->ID().c_str(), ADDON::TranslateContent(scraper->Content()).c_str(), scraper->GetPathSettings().c_str());
    m_pDS->exec(strSQL.c_str());

    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s - (%s) failed", __FUNCTION__, strPath.c_str());
  }
  return false;
}

bool CMusicDatabase::GetScraperForPath(const CStdString& strPath, ADDON::ScraperPtr& info, const ADDON::TYPE &type)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL = PrepareSQL("select * from content where strPath='%s'",strPath.c_str());
    m_pDS->query(strSQL.c_str());
    if (m_pDS->eof()) // no info set for path - fallback logic commencing
    {
      CQueryParams params;
      CDirectoryNode::GetDatabaseInfo(strPath, params);
      if (params.GetGenreId() != -1) // check genre
      {
        strSQL = PrepareSQL("select * from content where strPath='musicdb://genres/%i/'",params.GetGenreId());
        m_pDS->query(strSQL.c_str());
      }
      if (m_pDS->eof() && params.GetAlbumId() != -1) // check album
      {
        strSQL = PrepareSQL("select * from content where strPath='musicdb://albums/%i/'",params.GetAlbumId());
        m_pDS->query(strSQL.c_str());
        if (m_pDS->eof()) // general albums setting
        {
          strSQL = PrepareSQL("select * from content where strPath='musicdb://albums/'");
          m_pDS->query(strSQL.c_str());
        }
      }
      if (m_pDS->eof() && params.GetArtistId() != -1) // check artist
      {
        strSQL = PrepareSQL("select * from content where strPath='musicdb://artists/%i/'",params.GetArtistId());
        m_pDS->query(strSQL.c_str());

        if (m_pDS->eof()) // general artist setting
        {
          strSQL = PrepareSQL("select * from content where strPath='musicdb://artists/'");
          m_pDS->query(strSQL.c_str());
        }
      }
    }

    if (!m_pDS->eof())
    { // try and ascertain scraper for this path
      CONTENT_TYPE content = ADDON::TranslateContent(m_pDS->fv("content.strContent").get_asString());
      CStdString scraperUUID = m_pDS->fv("content.strScraperPath").get_asString();

      if (content != CONTENT_NONE)
      { // content set, use pre configured or default scraper
        ADDON::AddonPtr addon;
        if (!scraperUUID.empty() && ADDON::CAddonMgr::Get().GetAddon(scraperUUID, addon) && addon)
        {
          info = std::dynamic_pointer_cast<ADDON::CScraper>(addon->Clone());
          if (!info)
            return false;
          // store this path's settings
          info->SetPathSettings(content, m_pDS->fv("content.strSettings").get_asString());
        }
      }
      else
      { // use default scraper of the requested type
        ADDON::AddonPtr defaultScraper;
        if (ADDON::CAddonMgr::Get().GetDefault(type, defaultScraper))
        {
          info = std::dynamic_pointer_cast<ADDON::CScraper>(defaultScraper->Clone());
        }
      }
    }
    m_pDS->close();

    if (!info)
    { // use default music scraper instead
      ADDON::AddonPtr addon;
      if(ADDON::CAddonMgr::Get().GetDefault(type, addon))
      {
        info = std::dynamic_pointer_cast<ADDON::CScraper>(addon);
        return info != NULL;
      }
      else
        return false;
    }

    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s -(%s) failed", __FUNCTION__, strPath.c_str());
  }
  return false;
}

bool CMusicDatabase::ScraperInUse(const CStdString &scraperID) const
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString sql = PrepareSQL("select count(1) from content where strScraperPath='%s'",scraperID.c_str());
    if (!m_pDS->query(sql.c_str()) || m_pDS->num_rows() == 0)
      return false;
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

bool CMusicDatabase::GetItems(const CStdString &strBaseDir, CFileItemList &items, const Filter &filter /* = Filter() */, const SortDescription &sortDescription /* = SortDescription() */)
{
  CMusicDbUrl musicUrl;
  if (!musicUrl.FromString(strBaseDir))
    return false;

  return GetItems(strBaseDir, musicUrl.GetType(), items, filter, sortDescription);
}

bool CMusicDatabase::GetItems(const CStdString &strBaseDir, const CStdString &itemType, CFileItemList &items, const Filter &filter /* = Filter() */, const SortDescription &sortDescription /* = SortDescription() */)
{
  if (itemType.Equals("genres"))
    return GetGenresNav(strBaseDir, items, filter);
  else if (itemType.Equals("years"))
    return GetYearsNav(strBaseDir, items, filter);
  else if (itemType.Equals("artists"))
    return GetArtistsNav(strBaseDir, items, !CSettings::Get().GetBool("musiclibrary.showcompilationartists"), -1, -1, -1, filter, sortDescription);
  else if (itemType.Equals("albums"))
    return GetAlbumsByWhere(strBaseDir, filter, items, sortDescription);
  else if (itemType.Equals("songs"))
    return GetSongsByWhere(strBaseDir, filter, items, sortDescription);

  return false;
}

CStdString CMusicDatabase::GetItemById(const CStdString &itemType, int id)
{
  if (itemType.Equals("genres"))
    return GetGenreById(id);
  else if (itemType.Equals("years"))
    return StringUtils::Format("%d", id);
  else if (itemType.Equals("artists"))
    return GetArtistById(id);
  else if (itemType.Equals("albums"))
    return GetAlbumById(id);

  return "";
}

void CMusicDatabase::ExportToXML(const CStdString &xmlFile, bool singleFiles, bool images, bool overwrite)
{
  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;
    if (NULL == m_pDS2.get()) return;

    // find all albums
    vector<int> albumIds;
    CStdString sql = "select idAlbum FROM album WHERE lastScraped IS NOT NULL";
    m_pDS->query(sql.c_str());

    int total = m_pDS->num_rows();
    int current = 0;

    albumIds.reserve(total);
    while (!m_pDS->eof())
    {
      albumIds.push_back(m_pDS->fv("idAlbum").get_asInt());
      m_pDS->next();
    }
    m_pDS->close();

    CGUIDialogProgress *progress = (CGUIDialogProgress *)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
    if (progress)
    {
      progress->SetHeading(20196);
      progress->SetLine(0, 650);
      progress->SetLine(1, "");
      progress->SetLine(2, "");
      progress->SetPercentage(0);
      progress->StartModal();
      progress->ShowProgressBar(true);
    }

    // create our xml document
    CXBMCTinyXML xmlDoc;
    TiXmlDeclaration decl("1.0", "UTF-8", "yes");
    xmlDoc.InsertEndChild(decl);
    TiXmlNode *pMain = NULL;
    if (singleFiles)
      pMain = &xmlDoc;
    else
    {
      TiXmlElement xmlMainElement("musicdb");
      pMain = xmlDoc.InsertEndChild(xmlMainElement);
    }
    for (vector<int>::iterator albumId = albumIds.begin(); albumId != albumIds.end(); ++albumId)
    {
      CAlbum album;
      GetAlbum(*albumId, album);
      CStdString strPath;
      GetAlbumPath(*albumId, strPath);
      album.Save(pMain, "album", strPath);
      if (singleFiles)
      {
        if (!CDirectory::Exists(strPath))
          CLog::Log(LOGDEBUG, "%s - Not exporting item %s as it does not exist", __FUNCTION__, strPath.c_str());
        else
        {
          CStdString nfoFile = URIUtils::AddFileToFolder(strPath, "album.nfo");
          if (overwrite || !CFile::Exists(nfoFile))
          {
            if (!xmlDoc.SaveFile(nfoFile))
              CLog::Log(LOGERROR, "%s: Album nfo export failed! ('%s')", __FUNCTION__, nfoFile.c_str());
          }

          if (images)
          {
            string thumb = GetArtForItem(album.idAlbum, MediaTypeAlbum, "thumb");
            if (!thumb.empty() && (overwrite || !CFile::Exists(URIUtils::AddFileToFolder(strPath,"folder.jpg"))))
              CTextureCache::Get().Export(thumb, URIUtils::AddFileToFolder(strPath,"folder.jpg"));
          }
          xmlDoc.Clear();
          TiXmlDeclaration decl("1.0", "UTF-8", "yes");
          xmlDoc.InsertEndChild(decl);
        }
      }

      if ((current % 50) == 0 && progress)
      {
        progress->SetLine(1, album.strAlbum);
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
    vector<int> artistIds;
    CStdString artistSQL = "SELECT idArtist FROM artist where lastScraped IS NOT NULL";
    m_pDS->query(artistSQL.c_str());
    total = m_pDS->num_rows();
    current = 0;
    artistIds.reserve(total);
    while (!m_pDS->eof())
    {
      artistIds.push_back(m_pDS->fv("idArtist").get_asInt());
      m_pDS->next();
    }
    m_pDS->close();

    for (vector<int>::iterator artistId = artistIds.begin(); artistId != artistIds.end(); ++artistId)
    {
      CArtist artist;
      GetArtist(*artistId, artist);
      CStdString strPath;
      GetArtistPath(artist.idArtist,strPath);
      artist.Save(pMain, "artist", strPath);

      map<string, string> artwork;
      if (GetArtForItem(artist.idArtist, MediaTypeArtist, artwork) && !singleFiles)
      { // append to the XML
        TiXmlElement additionalNode("art");
        for (map<string, string>::const_iterator i = artwork.begin(); i != artwork.end(); ++i)
          XMLUtils::SetString(&additionalNode, i->first.c_str(), i->second);
        pMain->LastChild()->InsertEndChild(additionalNode);
      }
      if (singleFiles)
      {
        if (!CDirectory::Exists(strPath))
          CLog::Log(LOGDEBUG, "%s - Not exporting item %s as it does not exist", __FUNCTION__, strPath.c_str());
        else
        {
          CStdString nfoFile = URIUtils::AddFileToFolder(strPath, "artist.nfo");
          if (overwrite || !CFile::Exists(nfoFile))
          {
            if (!xmlDoc.SaveFile(nfoFile))
              CLog::Log(LOGERROR, "%s: Artist nfo export failed! ('%s')", __FUNCTION__, nfoFile.c_str());
          }

          if (images && !artwork.empty())
          {
            CStdString savedThumb = URIUtils::AddFileToFolder(strPath,"folder.jpg");
            CStdString savedFanart = URIUtils::AddFileToFolder(strPath,"fanart.jpg");
            if (artwork.find("thumb") != artwork.end() && (overwrite || !CFile::Exists(savedThumb)))
              CTextureCache::Get().Export(artwork["thumb"], savedThumb);
            if (artwork.find("fanart") != artwork.end() && (overwrite || !CFile::Exists(savedFanart)))
              CTextureCache::Get().Export(artwork["fanart"], savedFanart);
          }
          xmlDoc.Clear();
          TiXmlDeclaration decl("1.0", "UTF-8", "yes");
          xmlDoc.InsertEndChild(decl);
        }
      }

      if ((current % 50) == 0 && progress)
      {
        progress->SetLine(1, artist.strArtist);
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

    if (progress)
      progress->Close();

    xmlDoc.SaveFile(xmlFile);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
}

void CMusicDatabase::ImportFromXML(const CStdString &xmlFile)
{
  CGUIDialogProgress *progress = (CGUIDialogProgress *)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
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
      progress->SetHeading(20197);
      progress->SetLine(0, 649);
      progress->SetLine(1, 330);
      progress->SetLine(2, "");
      progress->SetPercentage(0);
      progress->StartModal();
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
      CStdString strTitle;
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
        int idAlbum = GetAlbumByName(importedAlbum.strAlbum, importedAlbum.artist);
        if (idAlbum > -1)
        {
          CAlbum album;
          GetAlbum(idAlbum, album, true);
          album.MergeScrapedAlbum(importedAlbum, true);
          UpdateAlbum(album);
        }

        current++;
      }
      entry = entry ->NextSiblingElement();
      if (progress && total)
      {
        progress->SetPercentage(current * 100 / total);
        progress->SetLine(2, strTitle);
        progress->Progress();
        if (progress->IsCanceled())
        {
          progress->Close();
          RollbackTransaction();
          return;
        }
      }
    }
    CommitTransaction();

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

void CMusicDatabase::AddKaraokeData(int idSong, int iKaraokeNumber)
{
  try
  {
    CStdString strSQL;

    // If song.iKaraokeNumber is non-zero, we already have it in the database. Just replace the song ID.
    if (iKaraokeNumber > 0)
    {
      CStdString strSQL = PrepareSQL("UPDATE karaokedata SET idSong=%i WHERE iKaraNumber=%i", idSong, iKaraokeNumber);
      m_pDS->exec(strSQL.c_str());
      return;
    }

    // Get the maximum number allocated
    strSQL=PrepareSQL( "SELECT MAX(iKaraNumber) FROM karaokedata" );
    if (!m_pDS->query(strSQL.c_str())) return;

    int iKaraokeNumber = g_advancedSettings.m_karaokeStartIndex;

    if ( m_pDS->num_rows() == 1 )
      iKaraokeNumber = m_pDS->fv("MAX(iKaraNumber)").get_asInt() + 1;

    // Add the data
    strSQL=PrepareSQL( "INSERT INTO karaokedata (iKaraNumber, idSong, iKaraDelay, strKaraEncoding, strKaralyrics) "
        "VALUES( %i, %i, 0, NULL, NULL)", iKaraokeNumber, idSong );

    m_pDS->exec(strSQL.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s -(%i, %i) failed", __FUNCTION__, idSong, iKaraokeNumber);
  }
}

bool CMusicDatabase::GetSongByKaraokeNumber(int number, CSong & song)
{
  try
  {
    // Get info from karaoke db
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL=PrepareSQL("SELECT * FROM karaokedata where iKaraNumber=%ld", number);

    if (!m_pDS->query(strSQL.c_str())) return false;
    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      return false;
    }

    int idSong = m_pDS->fv("karaokedata.idSong").get_asInt();
    m_pDS->close();

    return GetSong( idSong, song );
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%i) failed", __FUNCTION__, number);
  }

  return false;
}

void CMusicDatabase::ExportKaraokeInfo(const CStdString & outFile, bool asHTML)
{
  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    // find all karaoke songs
    CStdString sql = "SELECT * FROM songview WHERE iKaraNumber > 0 ORDER BY strFileName";

    m_pDS->query(sql.c_str());

    int total = m_pDS->num_rows();
    int current = 0;

    if ( total == 0 )
    {
      m_pDS->close();
      return;
    }

    // Write the document
    XFILE::CFile file;

    if ( !file.OpenForWrite( outFile, true ) )
      return;

    CGUIDialogProgress *progress = (CGUIDialogProgress *)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
    if (progress)
    {
      progress->SetHeading(asHTML ? 22034 : 22035);
      progress->SetLine(0, 650);
      progress->SetLine(1, "");
      progress->SetLine(2, "");
      progress->SetPercentage(0);
      progress->StartModal();
      progress->ShowProgressBar(true);
    }

    CStdString outdoc;
    if ( asHTML )
    {
      outdoc = "<html><head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\"></meta></head>\n"
          "<body>\n<table>\n";

      if (file.Write(outdoc, outdoc.size()) != outdoc.size())
        return; // error
    }

    while (!m_pDS->eof())
    {
      CSong song = GetSongFromDataset();
      CStdString songnum = StringUtils::Format("%06ld", song.iKaraokeNumber);

      if ( asHTML )
        outdoc = "<tr><td>" + songnum + "</td><td>" + (CStdString)StringUtils::Join(song.artist, g_advancedSettings.m_musicItemSeparator) + "</td><td>" + song.strTitle + "</td></tr>\r\n";
      else
        outdoc = songnum + '\t' + (CStdString)StringUtils::Join(song.artist, g_advancedSettings.m_musicItemSeparator) + '\t' + song.strTitle + '\t' + song.strFileName + "\r\n";

      if (file.Write(outdoc, outdoc.size()) != outdoc.size())
        return; // error

      if ((current % 50) == 0 && progress)
      {
        progress->SetPercentage(current * 100 / total);
        progress->Progress();
        if (progress->IsCanceled())
        {
          progress->Close();
          m_pDS->close();
          return;
        }
      }
      m_pDS->next();
      current++;
    }

    m_pDS->close();

    if ( asHTML )
    {
      outdoc = "</table>\n</body>\n</html>\n";
      if (file.Write(outdoc, outdoc.size()) != outdoc.size())
        return; // error
    }

    file.Close();

    if (progress)
      progress->Close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
}

void CMusicDatabase::ImportKaraokeInfo(const CStdString & inputFile)
{
  CGUIDialogProgress *progress = (CGUIDialogProgress *)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);

  try
  {
    if (NULL == m_pDB.get()) return;

    XFILE::CFile file;
    XFILE::auto_buffer buf;

    if (file.LoadFile(inputFile, buf) <= 0)
    {
      CLog::Log(LOGERROR, "%s: Cannot read karaoke import file \"%s\"", __FUNCTION__, inputFile.c_str());
      return;
    }

    // Null-terminate content
    buf.resize(buf.size() + 1);
    buf.get()[buf.size() - 1] = 0;

    file.Close();

    if (progress)
    {
      progress->SetHeading( 22036 );
      progress->SetLine(0, 649);
      progress->SetLine(1, "");
      progress->SetLine(2, "");
      progress->SetPercentage(0);
      progress->StartModal();
      progress->ShowProgressBar(true);
    }

    if (NULL == m_pDS.get()) return;
    BeginTransaction();

    //
    // A simple state machine to parse the file
    //
    char * linestart = buf.get();
    unsigned int offset = 0, lastpercentage = 0;

    for (char * p = buf.get(); *p; p++, offset++)
    {
      // Skip \r
      if ( *p == 0x0D )
      {
        *p = '\0';
        continue;
      }

      // Line number
      if ( *p == 0x0A )
      {
        *p = '\0';

        unsigned int tabs = 0;
        char * songpath, *artist = 0, *title = 0;
        for ( songpath = linestart; *songpath; songpath++ )
        {
          if ( *songpath == '\t' )
          {
            tabs++;
            *songpath = '\0';

            switch( tabs )
            {
              case 1: // the number end
                artist = songpath + 1;
                break; 

              case 2: // the artist end
                title = songpath + 1;
                break; 

              case 3: // the title end
                break;
            }
          }
        }

        int num = atoi( linestart );
        if ( num <= 0 || tabs < 3 || *artist == '\0' || *title == '\0' )
        {
          CLog::Log( LOGERROR, "Karaoke import: error in line %s", linestart );
          linestart = p + 1;
          continue;
        }

        linestart = p + 1;
        CStdString strSQL=PrepareSQL("select idSong from songview "
                     "where strArtists like '%s' and strTitle like '%s'", artist, title );

        if ( !m_pDS->query(strSQL.c_str()) )
        {
            RollbackTransaction();
            if (progress)
              progress->Close();
            m_pDS->close();
            return;
        }

        int iRowsFound = m_pDS->num_rows();
        if (iRowsFound == 0)
        {
          CLog::Log( LOGERROR, "Karaoke import: song %s by %s #%d is not found in the database, skipped", 
               title, artist, num );
          continue;
        }

        int lResult = m_pDS->fv(0).get_asInt();
        strSQL = PrepareSQL("UPDATE karaokedata SET iKaraNumber=%i WHERE idSong=%i", num, lResult );
        m_pDS->exec(strSQL.c_str());

        if ( progress && (offset * 100 / buf.size()) != lastpercentage )
        {
          lastpercentage = offset * 100 / buf.size();
          progress->SetPercentage( lastpercentage);
          progress->Progress();
          if ( progress->IsCanceled() )
          {
            RollbackTransaction();
            progress->Close();
            m_pDS->close();
            return;
          }
        }
      }
    }

    CommitTransaction();
    CLog::Log( LOGNOTICE, "Karaoke import: file '%s' was imported successfully", inputFile.c_str() );
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
    RollbackTransaction();
  }

  if (progress)
    progress->Close();
}

bool CMusicDatabase::SetKaraokeSongDelay(int idSong, int delay)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL = PrepareSQL("UPDATE karaokedata SET iKaraDelay=%i WHERE idSong=%i", delay, idSong);
    m_pDS->exec(strSQL.c_str());

    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }

  return false;
}

int CMusicDatabase::GetKaraokeSongsCount()
{
  try
  {
    if (NULL == m_pDB.get()) return 0;
    if (NULL == m_pDS.get()) return 0;

    if (!m_pDS->query( "select count(idSong) as NumSongs from karaokedata")) return 0;
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
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return 0;
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
  item.SetProperty("album_artist", StringUtils::Join(album.artist, g_advancedSettings.m_musicItemSeparator));
  item.SetProperty("album_artist_array", album.artist);
  item.SetProperty("album_genre", StringUtils::Join(album.genre, g_advancedSettings.m_musicItemSeparator));
  item.SetProperty("album_genre_array", album.genre);
  item.SetProperty("album_title", album.strAlbum);
  if (album.iRating > 0)
    item.SetProperty("album_rating", album.iRating);
}

void CMusicDatabase::SetPropertiesForFileItem(CFileItem& item)
{
  if (!item.HasMusicInfoTag())
    return;
  int idArtist = GetArtistByName(StringUtils::Join(item.GetMusicInfoTag()->GetArtist(), g_advancedSettings.m_musicItemSeparator));
  if (idArtist > -1)
  {
    CArtist artist;
    if (GetArtist(idArtist, artist))
      SetPropertiesFromArtist(item,artist);
  }
  int idAlbum = item.GetMusicInfoTag()->GetAlbumId();
  if (idAlbum <= 0)
    idAlbum = GetAlbumByName(item.GetMusicInfoTag()->GetAlbum(),
                             item.GetMusicInfoTag()->GetArtist());
  if (idAlbum > -1)
  {
    CAlbum album;
    if (GetAlbum(idAlbum, album, false))
      SetPropertiesFromAlbum(item,album);
  }
}

void CMusicDatabase::SetArtForItem(int mediaId, const string &mediaType, const map<string, string> &art)
{
  for (map<string, string>::const_iterator i = art.begin(); i != art.end(); ++i)
    SetArtForItem(mediaId, mediaType, i->first, i->second);
}

void CMusicDatabase::SetArtForItem(int mediaId, const string &mediaType, const string &artType, const string &url)
{
  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    // don't set <foo>.<bar> art types - these are derivative types from parent items
    if (artType.find('.') != string::npos)
      return;

    CStdString sql = PrepareSQL("SELECT art_id FROM art WHERE media_id=%i AND media_type='%s' AND type='%s'", mediaId, mediaType.c_str(), artType.c_str());
    m_pDS->query(sql.c_str());
    if (!m_pDS->eof())
    { // update
      int artId = m_pDS->fv(0).get_asInt();
      m_pDS->close();
      sql = PrepareSQL("UPDATE art SET url='%s' where art_id=%d", url.c_str(), artId);
      m_pDS->exec(sql.c_str());
    }
    else
    { // insert
      m_pDS->close();
      sql = PrepareSQL("INSERT INTO art(media_id, media_type, type, url) VALUES (%d, '%s', '%s', '%s')", mediaId, mediaType.c_str(), artType.c_str(), url.c_str());
      m_pDS->exec(sql.c_str());
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%d, '%s', '%s', '%s') failed", __FUNCTION__, mediaId, mediaType.c_str(), artType.c_str(), url.c_str());
  }
}

bool CMusicDatabase::GetArtForItem(int mediaId, const string &mediaType, map<string, string> &art)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS2.get()) return false; // using dataset 2 as we're likely called in loops on dataset 1

    CStdString sql = PrepareSQL("SELECT type,url FROM art WHERE media_id=%i AND media_type='%s'", mediaId, mediaType.c_str());
    m_pDS2->query(sql.c_str());
    while (!m_pDS2->eof())
    {
      art.insert(make_pair(m_pDS2->fv(0).get_asString(), m_pDS2->fv(1).get_asString()));
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

string CMusicDatabase::GetArtForItem(int mediaId, const string &mediaType, const string &artType)
{
  std::string query = PrepareSQL("SELECT url FROM art WHERE media_id=%i AND media_type='%s' AND type='%s'", mediaId, mediaType.c_str(), artType.c_str());
  return GetSingleValue(query, m_pDS2);
}

bool CMusicDatabase::GetArtistArtForItem(int mediaId, const std::string &mediaType, std::map<std::string, std::string> &art)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS2.get()) return false; // using dataset 2 as we're likely called in loops on dataset 1

    CStdString sql = PrepareSQL("SELECT type,url FROM art WHERE media_id=(SELECT idArtist from %s_artist WHERE id%s=%i AND iOrder=0) AND media_type='artist'", mediaType.c_str(), mediaType.c_str(), mediaId);
    m_pDS2->query(sql.c_str());
    while (!m_pDS2->eof())
    {
      art.insert(make_pair(m_pDS2->fv(0).get_asString(), m_pDS2->fv(1).get_asString()));
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

string CMusicDatabase::GetArtistArtForItem(int mediaId, const string &mediaType, const string &artType)
{
  std::string query = PrepareSQL("SELECT url FROM art WHERE media_id=(SELECT idArtist from %s_artist WHERE id%s=%i AND iOrder=0) AND media_type='artist' AND type='%s'", mediaType.c_str(), mediaType.c_str(), mediaId, artType.c_str());
  return GetSingleValue(query, m_pDS2);
}

bool CMusicDatabase::GetFilter(CDbUrl &musicUrl, Filter &filter, SortDescription &sorting)
{
  if (!musicUrl.IsValid())
    return false;

  std::string type = musicUrl.GetType();
  const CUrlOptions::UrlOptions& options = musicUrl.GetOptions();
  CUrlOptions::UrlOptions::const_iterator option;

  if (type == "artists")
  {
    int idArtist = -1, idGenre = -1, idAlbum = -1, idSong = -1;
    bool albumArtistsOnly = false;

    option = options.find("artistid");
    if (option != options.end())
      idArtist = (int)option->second.asInteger();

    option = options.find("genreid");
    if (option != options.end())
      idGenre = (int)option->second.asInteger();
    else
    {
      option = options.find("genre");
      if (option != options.end())
        idGenre = GetGenreByName(option->second.asString());
    }

    option = options.find("albumid");
    if (option != options.end())
      idAlbum = (int)option->second.asInteger();
    else
    {
      option = options.find("album");
      if (option != options.end())
        idAlbum = GetAlbumByName(option->second.asString());
    }

    option = options.find("songid");
    if (option != options.end())
      idSong = (int)option->second.asInteger();

    option = options.find("albumartistsonly");
    if (option != options.end())
      albumArtistsOnly = option->second.asBoolean();

    CStdString strSQL = "(artistview.idArtist IN ";
    if (idArtist > 0)
      strSQL += PrepareSQL("(%d)", idArtist);
    else if (idAlbum > 0)
      strSQL += PrepareSQL("(SELECT album_artist.idArtist FROM album_artist WHERE album_artist.idAlbum = %i)", idAlbum);
    else if (idSong > 0)
      strSQL += PrepareSQL("(SELECT song_artist.idArtist FROM song_artist WHERE song_artist.idSong = %i)", idSong);
    else if (idGenre > 0)
    { // same statements as below, but limit to the specified genre
      // in this case we show the whole lot always - there is no limitation to just album artists
      if (!albumArtistsOnly)  // show all artists in this case (ie those linked to a song)
        strSQL+=PrepareSQL("(SELECT song_artist.idArtist FROM song_artist" // All artists linked to extra genres
                           " JOIN song_genre ON song_artist.idSong = song_genre.idSong"
                           " WHERE song_genre.idGenre = %i)"
                           " OR idArtist IN ", idGenre);
      // and add any artists linked to an album (may be different from above due to album artist tag)
      strSQL += PrepareSQL("(SELECT album_artist.idArtist FROM album_artist" // All album artists linked to extra genres
                           " JOIN album_genre ON album_artist.idAlbum = album_genre.idAlbum"
                           " WHERE album_genre.idGenre = %i)", idGenre);
    }
    else
    {
      if (!albumArtistsOnly)  // show all artists in this case (ie those linked to a song)
        strSQL += "(SELECT song_artist.idArtist FROM song_artist)"
                  " OR artistview.idArtist IN ";

      // and always show any artists linked to an album (may be different from above due to album artist tag)
      strSQL +=   "(SELECT album_artist.idArtist FROM album_artist"; // All artists linked to an album
      if (albumArtistsOnly)
        strSQL += " JOIN album ON album.idAlbum = album_artist.idAlbum WHERE album.bCompilation = 0 ";            // then exclude those that have no extra artists
      strSQL +=   ")";
    }

    // remove the null string
    strSQL += ") and artistview.strArtist != ''";

    // and the various artist entry if applicable
    if (!albumArtistsOnly)
    {
      CStdString strVariousArtists = g_localizeStrings.Get(340);
      strSQL += PrepareSQL(" and artistview.strArtist <> '%s'", strVariousArtists.c_str());
    }

    filter.AppendWhere(strSQL);
  }
  else if (type == "albums")
  {
    option = options.find("year");
    if (option != options.end())
      filter.AppendWhere(PrepareSQL("albumview.iYear = %i", (int)option->second.asInteger()));
    
    option = options.find("compilation");
    if (option != options.end())
      filter.AppendWhere(PrepareSQL("albumview.bCompilation = %i", option->second.asBoolean() ? 1 : 0));

    option = options.find("genreid");
    if (option != options.end())
      filter.AppendWhere(PrepareSQL("albumview.idAlbum IN (SELECT song.idAlbum FROM song JOIN song_genre ON song.idSong = song_genre.idSong WHERE song_genre.idGenre = %i)", (int)option->second.asInteger()));

    option = options.find("genre");
    if (option != options.end())
      filter.AppendWhere(PrepareSQL("albumview.idAlbum IN (SELECT song.idAlbum FROM song JOIN song_genre ON song.idSong = song_genre.idSong JOIN genre ON genre.idGenre = song_genre.idGenre WHERE genre.strGenre like '%s')", option->second.asString().c_str()));

    option = options.find("artistid");
    if (option != options.end())
    {
      filter.AppendJoin("JOIN song ON song.idAlbum = albumview.idAlbum "
                        "JOIN song_artist ON song.idSong = song_artist.idSong "
                        "JOIN album_artist ON albumview.idAlbum = album_artist.idAlbum");
      filter.AppendWhere(PrepareSQL("      song_artist.idArtist = %i" // All albums linked to this artist via songs
                                    " OR  album_artist.idArtist = %i", // All albums where album artists fit
                                    (int)option->second.asInteger(), (int)option->second.asInteger()));
      filter.AppendGroup("albumview.idAlbum");
    }
    else
    {
      option = options.find("artist");
      if (option != options.end())
        filter.AppendWhere(PrepareSQL("albumview.idAlbum IN (SELECT song.idAlbum FROM song JOIN song_artist ON song.idSong = song_artist.idSong JOIN artist ON artist.idArtist = song_artist.idArtist WHERE artist.strArtist like '%s')" // All albums linked to this artist via songs
                                      " OR albumview.idAlbum IN (SELECT album_artist.idAlbum FROM album_artist JOIN artist ON artist.idArtist = album_artist.idArtist WHERE artist.strArtist like '%s')", // All albums where album artists fit
                                      option->second.asString().c_str(), option->second.asString().c_str()));
      // no artist given, so exclude any single albums (aka empty tagged albums)
      else
      {
        option = options.find("show_singles");
        if (option == options.end() || !option->second.asBoolean())
          filter.AppendWhere("albumview.strAlbum <> ''");
      }
    }
  }
  else if (type == "songs" || type == "singles")
  {
    option = options.find("singles");
    if (option != options.end())
      filter.AppendWhere(PrepareSQL("songview.idAlbum %sIN (SELECT idAlbum FROM album WHERE strAlbum = '')", option->second.asBoolean() ? "" : "NOT "));

    option = options.find("year");
    if (option != options.end())
      filter.AppendWhere(PrepareSQL("songview.iYear = %i", (int)option->second.asInteger()));
    
    option = options.find("compilation");
    if (option != options.end())
      filter.AppendWhere(PrepareSQL("songview.bCompilation = %i", option->second.asBoolean() ? 1 : 0));

    option = options.find("albumid");
    if (option != options.end())
      filter.AppendWhere(PrepareSQL("songview.idAlbum = %i", (int)option->second.asInteger()));
    
    option = options.find("album");
    if (option != options.end())
      filter.AppendWhere(PrepareSQL("songview.strAlbum like '%s'", option->second.asString().c_str()));

    option = options.find("genreid");
    if (option != options.end())
      filter.AppendWhere(PrepareSQL("songview.idSong IN (SELECT song_genre.idSong FROM song_genre WHERE song_genre.idGenre = %i)", (int)option->second.asInteger()));

    option = options.find("genre");
    if (option != options.end())
      filter.AppendWhere(PrepareSQL("songview.idSong IN (SELECT song_genre.idSong FROM song_genre JOIN genre ON genre.idGenre = song_genre.idGenre WHERE genre.strGenre like '%s')", option->second.asString().c_str()));

    option = options.find("artistid");
    if (option != options.end())
      filter.AppendWhere(PrepareSQL("songview.idSong IN (SELECT song_artist.idSong FROM song_artist WHERE song_artist.idArtist = %i)" // song artists
                                    " OR songview.idSong IN (SELECT song.idSong FROM song JOIN album_artist ON song.idAlbum=album_artist.idAlbum WHERE album_artist.idArtist = %i)", // album artists
                                    (int)option->second.asInteger(), (int)option->second.asInteger()));

    option = options.find("artist");
    if (option != options.end())
      filter.AppendWhere(PrepareSQL("songview.idSong IN (SELECT song_artist.idSong FROM song_artist JOIN artist ON artist.idArtist = song_artist.idArtist WHERE artist.strArtist like '%s')" // song artists
                                    " OR songview.idSong IN (SELECT song.idSong FROM song JOIN album_artist ON song.idAlbum=album_artist.idAlbum JOIN artist ON artist.idArtist = album_artist.idArtist WHERE artist.strArtist like '%s')", // album artists
                                    option->second.asString().c_str(), option->second.asString().c_str()));
  }

  option = options.find("xsp");
  if (option != options.end())
  {
    CSmartPlaylist xsp;
    if (!xsp.LoadFromJson(option->second.asString()))
      return false;

    // check if the filter playlist matches the item type
    if (xsp.GetType()  == type ||
       (xsp.GetGroup() == type && !xsp.IsGroupMixed()))
    {
      std::set<std::string> playlists;
      filter.AppendWhere(xsp.GetWhereClause(*this, playlists));

      if (xsp.GetLimit() > 0)
        sorting.limitEnd = xsp.GetLimit();
      if (xsp.GetOrder() != SortByNone)
        sorting.sortBy = xsp.GetOrder();
      sorting.sortOrder = xsp.GetOrderAscending() ? SortOrderAscending : SortOrderDescending;
      if (CSettings::Get().GetBool("filelists.ignorethewhensorting"))
        sorting.sortAttributes = SortAttributeIgnoreArticle;
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
