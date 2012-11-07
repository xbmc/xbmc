/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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
#include "settings/GUISettings.h"
#include "settings/AdvancedSettings.h"
#include "FileItem.h"
#include "Application.h"
#ifdef HAS_KARAOKE
#include "karaoke/karaokelyricsfactory.h"
#endif
#include "storage/MediaManager.h"
#include "settings/Settings.h"
#include "utils/StringUtils.h"
#include "guilib/LocalizeStrings.h"
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
#endif

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

bool CMusicDatabase::CreateTables()
{
  BeginTransaction();
  try
  {
    CDatabase::CreateTables();

    CLog::Log(LOGINFO, "create artist table");
    m_pDS->exec("CREATE TABLE artist ( idArtist integer primary key, strArtist varchar(256))\n");
    CLog::Log(LOGINFO, "create album table");
    m_pDS->exec("CREATE TABLE album ( idAlbum integer primary key, strAlbum varchar(256), strArtists text, strGenres text, iYear integer, idThumb integer, bCompilation integer not null default '0' )\n");
    CLog::Log(LOGINFO, "create album_artist table");
    m_pDS->exec("CREATE TABLE album_artist ( idArtist integer, idAlbum integer, boolFeatured integer, iOrder integer )\n");
    CLog::Log(LOGINFO, "create album_genre table");
    m_pDS->exec("CREATE TABLE album_genre ( idGenre integer, idAlbum integer, iOrder integer )\n");

    CLog::Log(LOGINFO, "create genre table");
    m_pDS->exec("CREATE TABLE genre ( idGenre integer primary key, strGenre varchar(256))\n");
    CLog::Log(LOGINFO, "create path table");
    m_pDS->exec("CREATE TABLE path ( idPath integer primary key, strPath varchar(512), strHash text)\n");
    CLog::Log(LOGINFO, "create song table");
    m_pDS->exec("CREATE TABLE song ( idSong integer primary key, idAlbum integer, idPath integer, strArtists text, strGenres text, strTitle varchar(512), iTrack integer, iDuration integer, iYear integer, dwFileNameCRC text, strFileName text, strMusicBrainzTrackID text, strMusicBrainzArtistID text, strMusicBrainzAlbumID text, strMusicBrainzAlbumArtistID text, strMusicBrainzTRMID text, iTimesPlayed integer, iStartOffset integer, iEndOffset integer, idThumb integer, lastplayed varchar(20) default NULL, rating char default '0', comment text)\n");
    CLog::Log(LOGINFO, "create song_artist table");
    m_pDS->exec("CREATE TABLE song_artist ( idArtist integer, idSong integer, boolFeatured integer, iOrder integer )\n");
    CLog::Log(LOGINFO, "create song_genre table");
    m_pDS->exec("CREATE TABLE song_genre ( idGenre integer, idSong integer, iOrder integer )\n");

    CLog::Log(LOGINFO, "create albuminfo table");
    m_pDS->exec("CREATE TABLE albuminfo ( idAlbumInfo integer primary key, idAlbum integer, iYear integer, strMoods text, strStyles text, strThemes text, strReview text, strImage text, strLabel text, strType text, iRating integer)\n");
    CLog::Log(LOGINFO, "create albuminfosong table");
    m_pDS->exec("CREATE TABLE albuminfosong ( idAlbumInfoSong integer primary key, idAlbumInfo integer, iTrack integer, strTitle text, iDuration integer)\n");
    CLog::Log(LOGINFO, "create artistnfo table");
    m_pDS->exec("CREATE TABLE artistinfo ( idArtistInfo integer primary key, idArtist integer, strBorn text, strFormed text, strGenres text, strMoods text, strStyles text, strInstruments text, strBiography text, strDied text, strDisbanded text, strYearsActive text, strImage text, strFanart text)\n");
    CLog::Log(LOGINFO, "create content table");
    m_pDS->exec("CREATE TABLE content (strPath text, strScraperPath text, strContent text, strSettings text)\n");
    CLog::Log(LOGINFO, "create discography table");
    m_pDS->exec("CREATE TABLE discography (idArtist integer, strAlbum text, strYear text)\n");

    CLog::Log(LOGINFO, "create karaokedata table");
    m_pDS->exec("CREATE TABLE karaokedata ( iKaraNumber integer, idSong integer, iKaraDelay integer, strKaraEncoding text, "
                "strKaralyrics text, strKaraLyrFileCRC text )\n");

    CLog::Log(LOGINFO, "create album index");
    m_pDS->exec("CREATE INDEX idxAlbum ON album(strAlbum)");
    CLog::Log(LOGINFO, "create album compilation index");
    m_pDS->exec("CREATE INDEX idxAlbum_1 ON album(bCompilation)");

    CLog::Log(LOGINFO, "create album_artist indexes");
    m_pDS->exec("CREATE UNIQUE INDEX idxAlbumArtist_1 ON album_artist ( idAlbum, idArtist )\n");
    m_pDS->exec("CREATE UNIQUE INDEX idxAlbumArtist_2 ON album_artist ( idArtist, idAlbum )\n");
    m_pDS->exec("CREATE INDEX idxAlbumArtist_3 ON album_artist ( boolFeatured )\n");

    CLog::Log(LOGINFO, "create album_genre indexes");
    m_pDS->exec("CREATE UNIQUE INDEX idxAlbumGenre_1 ON album_genre ( idAlbum, idGenre )\n");
    m_pDS->exec("CREATE UNIQUE INDEX idxAlbumGenre_2 ON album_genre ( idGenre, idAlbum )\n");

    CLog::Log(LOGINFO, "create genre index");
    m_pDS->exec("CREATE INDEX idxGenre ON genre(strGenre)");
    CLog::Log(LOGINFO, "create artist index");
    m_pDS->exec("CREATE INDEX idxArtist ON artist(strArtist)");
    CLog::Log(LOGINFO, "create path index");
    m_pDS->exec("CREATE INDEX idxPath ON path(strPath)");

    CLog::Log(LOGINFO, "create song index");
    m_pDS->exec("CREATE INDEX idxSong ON song(strTitle)");
    CLog::Log(LOGINFO, "create song index1");
    m_pDS->exec("CREATE INDEX idxSong1 ON song(iTimesPlayed)");
    CLog::Log(LOGINFO, "create song index2");
    m_pDS->exec("CREATE INDEX idxSong2 ON song(lastplayed)");
    CLog::Log(LOGINFO, "create song index3");
    m_pDS->exec("CREATE INDEX idxSong3 ON song(idAlbum)");
    CLog::Log(LOGINFO, "create song index6");
    m_pDS->exec("CREATE INDEX idxSong6 ON song(idPath)");

    CLog::Log(LOGINFO, "create song_artist indexes");
    m_pDS->exec("CREATE UNIQUE INDEX idxSongArtist_1 ON song_artist ( idSong, idArtist )\n");
    m_pDS->exec("CREATE UNIQUE INDEX idxSongArtist_2 ON song_artist ( idArtist, idSong )\n");
    m_pDS->exec("CREATE INDEX idxSongArtist_3 ON song_artist ( boolFeatured )\n");

    CLog::Log(LOGINFO, "create song_genre indexes");
    m_pDS->exec("CREATE UNIQUE INDEX idxSongGenre_1 ON song_genre ( idSong, idGenre )\n");
    m_pDS->exec("CREATE UNIQUE INDEX idxSongGenre_2 ON song_genre ( idGenre, idSong )\n");

    //m_pDS->exec("CREATE INDEX idxSong ON song(dwFileNameCRC)");
    CLog::Log(LOGINFO, "create artistinfo index");
    m_pDS->exec("CREATE INDEX idxArtistInfo on artistinfo(idArtist)");
    CLog::Log(LOGINFO, "create albuminfo index");
    m_pDS->exec("CREATE INDEX idxAlbumInfo on albuminfo(idAlbum)");

    CLog::Log(LOGINFO, "create karaokedata index");
    m_pDS->exec("CREATE INDEX idxKaraNumber on karaokedata(iKaraNumber)");
    m_pDS->exec("CREATE INDEX idxKarSong on karaokedata(idSong)");

    // Trigger
    CLog::Log(LOGINFO, "create albuminfo trigger");
    m_pDS->exec("CREATE TRIGGER tgrAlbumInfo AFTER delete ON albuminfo FOR EACH ROW BEGIN delete from albuminfosong where albuminfosong.idAlbumInfo=old.idAlbumInfo; END");

    CLog::Log(LOGINFO, "create art table, index and triggers");
    m_pDS->exec("CREATE TABLE art(art_id INTEGER PRIMARY KEY, media_id INTEGER, media_type TEXT, type TEXT, url TEXT)");
    m_pDS->exec("CREATE INDEX ix_art ON art(media_id, media_type(20), type(20))");
    m_pDS->exec("CREATE TRIGGER delete_song AFTER DELETE ON song FOR EACH ROW BEGIN DELETE FROM art WHERE media_id=old.idSong AND media_type='song'; END");
    m_pDS->exec("CREATE TRIGGER delete_album AFTER DELETE ON album FOR EACH ROW BEGIN DELETE FROM art WHERE media_id=old.idAlbum AND media_type='album'; END");
    m_pDS->exec("CREATE TRIGGER delete_artist AFTER DELETE ON artist FOR EACH ROW BEGIN DELETE FROM art WHERE media_id=old.idArtist AND media_type='artist'; END");

    // we create views last to ensure all indexes are rolled in
    CreateViews();

    // Add 'Karaoke' genre
    AddGenre( "Karaoke" );
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s unable to create tables:%i", __FUNCTION__, (int)GetLastError());
    RollbackTransaction();
    return false;
  }
  CommitTransaction();
  return true;
}

void CMusicDatabase::CreateViews()
{
  CLog::Log(LOGINFO, "create song view");
  m_pDS->exec("DROP VIEW IF EXISTS songview");
  m_pDS->exec("CREATE VIEW songview AS SELECT "
              "  song.idSong AS idSong, "
              "  song.strArtists AS strArtists,"
              "  song.strGenres AS strGenres,"
              "  strTitle, iTrack, iDuration,"
              "  song.iYear AS iYear, dwFileNameCRC, strFileName, strMusicBrainzTrackID,"
              "  strMusicBrainzArtistID, strMusicBrainzAlbumID, strMusicBrainzAlbumArtistID,"
              "  strMusicBrainzTRMID, iTimesPlayed, iStartOffset, iEndOffset, lastplayed,"
              "  rating, comment, song.idAlbum AS idAlbum, strAlbum, strPath,"
              "  iKaraNumber, iKaraDelay, strKaraEncoding,"
              "  album.bCompilation AS bCompilation "
              "FROM song"
              "  JOIN album ON"
              "    song.idAlbum=album.idAlbum"
              "  JOIN path ON"
              "    song.idPath=path.idPath"
              "  LEFT OUTER JOIN karaokedata ON"
              "    song.idSong=karaokedata.idSong");

  CLog::Log(LOGINFO, "create album view");
  m_pDS->exec("DROP VIEW IF EXISTS albumview");
  m_pDS->exec("CREATE VIEW albumview AS SELECT"
              "  album.idAlbum AS idAlbum, strAlbum, "
              "  album.strArtists AS strArtists,"
              "  album.strGenres AS strGenres, "
              "  album.iYear AS iYear,"
              "  idAlbumInfo, strMoods, strStyles, strThemes,"
              "  strReview, strLabel, strType, strImage, iRating, "
              "  bCompilation, "
              "  sum(song.iTimesPlayed) AS iTimesPlayed "
              "FROM album "
              "  LEFT OUTER JOIN albuminfo ON"
              "    album.idAlbum=albuminfo.idAlbum"
              "  LEFT OUTER JOIN song ON"
              "    album.idAlbum=song.idAlbum "
              "GROUP BY album.idAlbum");

  CLog::Log(LOGINFO, "create artist view");
  m_pDS->exec("DROP VIEW IF EXISTS artistview");
  m_pDS->exec("CREATE VIEW artistview AS SELECT"
              "  artist.idArtist AS idArtist, strArtist, "
              "  strBorn, strFormed, strGenres,"
              "  strMoods, strStyles, strInstruments, "
              "  strBiography, strDied, strDisbanded, "
              "  strYearsActive, strImage, strFanart "
              "FROM artist "
              "  LEFT OUTER JOIN artistinfo ON"
              "    artist.idArtist = artistinfo.idArtist");
}

int CMusicDatabase::AddAlbum(const CAlbum &album, vector<int> &songIDs)
{
  // add the album
  int idAlbum = AddAlbum(album.strAlbum, StringUtils::Join(album.artist, g_advancedSettings.m_musicItemSeparator), StringUtils::Join(album.genre, g_advancedSettings.m_musicItemSeparator), album.iYear, album.bCompilation);

  SetArtForItem(idAlbum, "album", album.art);

  // add the songs
  for (VECSONGS::const_iterator i = album.songs.begin(); i != album.songs.end(); ++i)
    songIDs.push_back(AddSong(*i, false, idAlbum));

  return idAlbum;
}

int CMusicDatabase::AddSong(const CSong& song, bool bCheck, int idAlbum)
{
  int idSong = -1;
  CStdString strSQL;
  try
  {
    // We need at least the title
    if (song.strTitle.IsEmpty())
      return -1;

    CStdString strPath, strFileName;
    URIUtils::Split(song.strFileName, strPath, strFileName);

    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    int idPath = AddPath(strPath);
    if (idAlbum < 0)
    {
      if (!song.albumArtist.empty())  // have an album artist
        idAlbum = AddAlbum(song.strAlbum, StringUtils::Join(song.albumArtist, g_advancedSettings.m_musicItemSeparator), StringUtils::Join(song.genre, g_advancedSettings.m_musicItemSeparator), song.iYear, song.bCompilation);
      else
        idAlbum = AddAlbum(song.strAlbum, StringUtils::Join(song.artist, g_advancedSettings.m_musicItemSeparator), StringUtils::Join(song.genre, g_advancedSettings.m_musicItemSeparator), song.iYear, song.bCompilation);
    }

    DWORD crc = ComputeCRC(song.strFileName);

    bool bInsert = true;
    bool bHasKaraoke = false;
#ifdef HAS_KARAOKE
    bHasKaraoke = CKaraokeLyricsFactory::HasLyrics( song.strFileName );
#endif

    if (bCheck)
    {
      strSQL=PrepareSQL("select * from song where idAlbum=%i and dwFileNameCRC='%ul' and strTitle='%s'",
                    idAlbum, crc, song.strTitle.c_str());

      if (!m_pDS->query(strSQL.c_str()))
        return -1;

      if (m_pDS->num_rows() != 0)
      {
        idSong = m_pDS->fv("idSong").get_asInt();
        bInsert = false;
      }
      m_pDS->close();
    }
    if (bInsert)
    {
      CStdString strSQL1;

      CStdString strIdSong;
      if (song.idSong < 0)
        strIdSong = "NULL";
      else
        strIdSong.Format("%d", song.idSong);

      // we use replace because it can handle both inserting a new song
      // and replacing an existing song's record if the given idSong already exists
      strSQL=PrepareSQL("replace into song (idSong,idAlbum,idPath,strArtists,strGenres,strTitle,iTrack,iDuration,iYear,dwFileNameCRC,strFileName,strMusicBrainzTrackID,strMusicBrainzArtistID,strMusicBrainzAlbumID,strMusicBrainzAlbumArtistID,strMusicBrainzTRMID,iTimesPlayed,iStartOffset,iEndOffset,lastplayed,rating,comment) values (%s,%i,%i,'%s','%s','%s',%i,%i,%i,'%ul','%s','%s','%s','%s','%s','%s'",
                    strIdSong.c_str(),
                    idAlbum, idPath, 
                    StringUtils::Join(song.artist, g_advancedSettings.m_musicItemSeparator).c_str(),
                    StringUtils::Join(song.genre, g_advancedSettings.m_musicItemSeparator).c_str(),
                    song.strTitle.c_str(),
                    song.iTrack, song.iDuration, song.iYear,
                    crc, strFileName.c_str(),
                    song.strMusicBrainzTrackID.c_str(),
                    song.strMusicBrainzArtistID.c_str(),
                    song.strMusicBrainzAlbumID.c_str(),
                    song.strMusicBrainzAlbumArtistID.c_str(),
                    song.strMusicBrainzTRMID.c_str());

      if (song.lastPlayed.IsValid())
        strSQL1=PrepareSQL(",%i,%i,%i,'%s','%c','%s')",
                      song.iTimesPlayed, song.iStartOffset, song.iEndOffset, song.lastPlayed.GetAsDBDateTime().c_str(), song.rating, song.strComment.c_str());
      else
        strSQL1=PrepareSQL(",%i,%i,%i,NULL,'%c','%s')",
                      song.iTimesPlayed, song.iStartOffset, song.iEndOffset, song.rating, song.strComment.c_str());
      strSQL+=strSQL1;

      m_pDS->exec(strSQL.c_str());

      if (song.idSong < 0)
        idSong = (int)m_pDS->lastinsertid();
      else
        idSong = song.idSong;
    }

    if (!song.strThumb.empty())
      SetArtForItem(idSong, "song", "thumb", song.strThumb);

    for (unsigned int index = 0; index < song.albumArtist.size(); index++)
    {
      int idAlbumArtist = AddArtist(song.albumArtist[index]);
      AddAlbumArtist(idAlbumArtist, idAlbum, index > 0 ? true : false, index);
    }

    for (unsigned int index = 0; index < song.artist.size(); index++)
    {
      int idArtist = AddArtist(song.artist[index]);
      AddSongArtist(idArtist, idSong, index > 0 ? true : false, index);
    }

    unsigned int index = 0;
    // If this is karaoke song, change the genre to 'Karaoke' (and add it if it's not there)
    if ( bHasKaraoke && g_advancedSettings.m_karaokeChangeGenreForKaraokeSongs )
    {
      int idGenre = AddGenre("Karaoke");
      AddSongGenre(idGenre, idSong, index);
      AddAlbumGenre(idGenre, idAlbum, index++);
    }
    for (vector<string>::const_iterator i = song.genre.begin(); i != song.genre.end(); ++i)
    {
      // index will be wrong for albums, but ordering is not all that relevant
      // for genres anyway
      int idGenre = AddGenre(*i);
      AddSongGenre(idGenre, idSong, index);
      AddAlbumGenre(idGenre, idAlbum, index++);
    }

    // Add karaoke information (if any)
    if ( bHasKaraoke )
      AddKaraokeData(idSong, song );

    AnnounceUpdate("song", idSong);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "musicdatabase:unable to addsong (%s)", strSQL.c_str());
  }
  return idSong;
}

int CMusicDatabase::UpdateSong(const CSong& song, int idSong /* = -1 */)
{
  CStdString sql;
  if (idSong < 0)
    idSong = song.idSong;

  if (idSong < 0)
    return -1;

  // delete linked songs
  // we don't delete from the song table here because
  // AddSong will update the existing record
  sql.Format("delete from song_artist where idSong=%d", idSong);
  ExecuteQuery(sql);
  sql.Format("delete from song_genre where idSong=%d", idSong);
  ExecuteQuery(sql);
  sql.Format("delete from karaokedata where idSong=%d", idSong);
  ExecuteQuery(sql);

  CSong newSong = song;
  // Make sure newSong.idSong has a valid value (> 0)
  newSong.idSong = idSong;
  // re-add the song
  newSong.idSong = AddSong(newSong, false);
  if (newSong.idSong < 0)
    return -1;

  return newSong.idSong;
}

int CMusicDatabase::AddAlbum(const CStdString& strAlbum1, const CStdString &strArtist, const CStdString& strGenre, int year, bool bCompilation)
{
  CStdString strSQL;
  try
  {
    CStdString strAlbum=strAlbum1;
    strAlbum.Trim();

    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    map <CStdString, CAlbum>::const_iterator it;

    it = m_albumCache.find(strAlbum + strArtist);
    if (it != m_albumCache.end())
      return it->second.idAlbum;

    strSQL=PrepareSQL("select * from album where strArtists='%s' and strAlbum like '%s'", strArtist.c_str(), strAlbum.c_str());
    m_pDS->query(strSQL.c_str());

    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      // doesnt exists, add it
      strSQL=PrepareSQL("insert into album (idAlbum, strAlbum, strArtists, strGenres, iYear, bCompilation) values( NULL, '%s', '%s', '%s', %i, %i)", strAlbum.c_str(), strArtist.c_str(), strGenre.c_str(), year, bCompilation);
      m_pDS->exec(strSQL.c_str());

      CAlbum album;
      album.idAlbum = (int)m_pDS->lastinsertid();
      album.strAlbum = strAlbum;
      album.artist = StringUtils::Split(strArtist, g_advancedSettings.m_musicItemSeparator);
      m_albumCache.insert(pair<CStdString, CAlbum>(album.strAlbum + strArtist, album));
      return album.idAlbum;
    }
    else
    {
      // exists in our database and not scanned during this scan, so we should update it as the details
      // may have changed (there's a reason we're rescanning, afterall!)
      CAlbum album;
      album.idAlbum = m_pDS->fv("idAlbum").get_asInt();
      album.strAlbum = strAlbum;
      album.artist = StringUtils::Split(strArtist, g_advancedSettings.m_musicItemSeparator);
      m_albumCache.insert(pair<CStdString, CAlbum>(album.strAlbum + strArtist, album));
      m_pDS->close();
      strSQL=PrepareSQL("update album set strGenres='%s', iYear=%i where idAlbum=%i", strGenre.c_str(), year, album.idAlbum);
      m_pDS->exec(strSQL.c_str());
      // and clear the link tables - these are updated in AddSong()
      strSQL=PrepareSQL("delete from album_artist where idAlbum=%i", album.idAlbum);
      m_pDS->exec(strSQL.c_str());
      strSQL=PrepareSQL("delete from album_genre where idAlbum=%i", album.idAlbum);
      m_pDS->exec(strSQL.c_str());
      return album.idAlbum;
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed with query (%s)", __FUNCTION__, strSQL.c_str());
  }

  return -1;
}

int CMusicDatabase::AddGenre(const CStdString& strGenre1)
{
  CStdString strSQL;
  try
  {
    CStdString strGenre = strGenre1;
    strGenre.Trim();

    if (strGenre.IsEmpty())
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

int CMusicDatabase::AddArtist(const CStdString& strArtist1)
{
  CStdString strSQL;
  try
  {
    CStdString strArtist = strArtist1;
    strArtist.Trim();

    if (strArtist.IsEmpty())
      strArtist=g_localizeStrings.Get(13205); // Unknown

    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    map <CStdString, int>::const_iterator it;

    it = m_artistCache.find(strArtist);
    if (it != m_artistCache.end())
      return it->second;//.idArtist;

    strSQL=PrepareSQL("select * from artist where strArtist like '%s'", strArtist.c_str());
    m_pDS->query(strSQL.c_str());

    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      // doesnt exists, add it
      strSQL=PrepareSQL("insert into artist (idArtist, strArtist) values( NULL, '%s' )", strArtist.c_str());
      m_pDS->exec(strSQL.c_str());
      int idArtist = (int)m_pDS->lastinsertid();
      m_artistCache.insert(pair<CStdString, int>(strArtist1, idArtist));
      return idArtist;
    }
    else
    {
      int idArtist = (int)m_pDS->fv("idArtist").get_asInt();
      m_artistCache.insert(pair<CStdString, int>(strArtist1, idArtist));
      m_pDS->close();
      return idArtist;
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "musicdatabase:unable to addartist (%s)", strSQL.c_str());
  }

  return -1;
}

bool CMusicDatabase::AddSongArtist(int idArtist, int idSong, bool featured, int iOrder)
{
  CStdString strSQL;
  strSQL=PrepareSQL("replace into song_artist (idArtist, idSong, boolFeatured, iOrder) values(%i,%i,%i,%i)",
                    idArtist, idSong, featured == true ? 1 : 0, iOrder);
  return ExecuteQuery(strSQL);
};

bool CMusicDatabase::AddAlbumArtist(int idArtist, int idAlbum, bool featured, int iOrder)
{
  CStdString strSQL;
  strSQL=PrepareSQL("replace into album_artist (idArtist, idAlbum, boolFeatured, iOrder) values(%i,%i,%i,%i)",
                    idArtist, idAlbum, featured == true ? 1 : 0, iOrder);
  return ExecuteQuery(strSQL);
};

bool CMusicDatabase::AddSongGenre(int idGenre, int idSong, int iOrder)
{
  if (idGenre == -1 || idSong == -1)
    return true;

  CStdString strSQL;
  strSQL=PrepareSQL("replace into song_genre (idGenre, idSong, iOrder) values(%i,%i,%i)",
                    idGenre, idSong, iOrder);
  return ExecuteQuery(strSQL);};

bool CMusicDatabase::AddAlbumGenre(int idGenre, int idAlbum, int iOrder)
{
  if (idGenre == -1 || idAlbum == -1)
    return true;
  
  CStdString strSQL;
  strSQL=PrepareSQL("replace into album_genre (idGenre, idAlbum, iOrder) values(%i,%i,%i)",
                    idGenre, idAlbum, iOrder);
  return ExecuteQuery(strSQL);
};

bool CMusicDatabase::GetAlbumsByArtist(int idArtist, bool includeFeatured, std::vector<long> &albums)
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

bool CMusicDatabase::GetArtistsByAlbum(int idAlbum, bool includeFeatured, std::vector<long> &artists)
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

bool CMusicDatabase::GetSongsByArtist(int idArtist, bool includeFeatured, std::vector<long> &songs)
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

bool CMusicDatabase::GetArtistsBySong(int idSong, bool includeFeatured, std::vector<long> &artists)
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
};

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

CSong CMusicDatabase::GetSongFromDataset(bool bWithMusicDbPath/*=false*/)
{
  CSong song;
  song.idSong = m_pDS->fv(song_idSong).get_asInt();
  // get the full artist string
  song.artist = StringUtils::Split(m_pDS->fv(song_strArtists).get_asString(), g_advancedSettings.m_musicItemSeparator);
  // and the full genre string
  song.genre = StringUtils::Split(m_pDS->fv(song_strGenres).get_asString(), g_advancedSettings.m_musicItemSeparator);
  // and the rest...
  song.strAlbum = m_pDS->fv(song_strAlbum).get_asString();
  song.iAlbumId = m_pDS->fv(song_idAlbum).get_asInt();
  song.iTrack = m_pDS->fv(song_iTrack).get_asInt() ;
  song.iDuration = m_pDS->fv(song_iDuration).get_asInt() ;
  song.iYear = m_pDS->fv(song_iYear).get_asInt() ;
  song.strTitle = m_pDS->fv(song_strTitle).get_asString();
  song.iTimesPlayed = m_pDS->fv(song_iTimesPlayed).get_asInt();
  song.lastPlayed.SetFromDBDateTime(m_pDS->fv(song_lastplayed).get_asString());
  song.iStartOffset = m_pDS->fv(song_iStartOffset).get_asInt();
  song.iEndOffset = m_pDS->fv(song_iEndOffset).get_asInt();
  song.strMusicBrainzTrackID = m_pDS->fv(song_strMusicBrainzTrackID).get_asString();
  song.strMusicBrainzArtistID = m_pDS->fv(song_strMusicBrainzArtistID).get_asString();
  song.strMusicBrainzAlbumID = m_pDS->fv(song_strMusicBrainzAlbumID).get_asString();
  song.strMusicBrainzAlbumArtistID = m_pDS->fv(song_strMusicBrainzAlbumArtistID).get_asString();
  song.strMusicBrainzTRMID = m_pDS->fv(song_strMusicBrainzTRMID).get_asString();
  song.rating = m_pDS->fv(song_rating).get_asChar();
  song.strComment = m_pDS->fv(song_comment).get_asString();
  song.iKaraokeNumber = m_pDS->fv(song_iKarNumber).get_asInt();
  song.strKaraokeLyrEncoding = m_pDS->fv(song_strKarEncoding).get_asString();
  song.iKaraokeDelay = m_pDS->fv(song_iKarDelay).get_asInt();
  song.bCompilation = m_pDS->fv(song_bCompilation).get_asInt() == 1;

  // Get filename with full path
  if (!bWithMusicDbPath)
    URIUtils::AddFileToFolder(m_pDS->fv(song_strPath).get_asString(), m_pDS->fv(song_strFileName).get_asString(), song.strFileName);
  else
  {
    CStdString strFileName = m_pDS->fv(song_strFileName).get_asString();
    CStdString strExt = URIUtils::GetExtension(strFileName);
    song.strFileName.Format("musicdb://3/%ld/%ld%s", m_pDS->fv(song_idAlbum).get_asInt(), m_pDS->fv(song_idSong).get_asInt(), strExt.c_str());
  }

  return song;
}

void CMusicDatabase::GetFileItemFromDataset(CFileItem* item, const CStdString& strMusicDBbasePath)
{
  return GetFileItemFromDataset(m_pDS->get_sql_record(), item, strMusicDBbasePath);
}

void CMusicDatabase::GetFileItemFromDataset(const dbiplus::sql_record* const record, CFileItem* item, const CStdString& strMusicDBbasePath)
{
  // get the full artist string
  item->GetMusicInfoTag()->SetArtist(StringUtils::Split(record->at(song_strArtists).get_asString(), g_advancedSettings.m_musicItemSeparator));
  // and the full genre string
  item->GetMusicInfoTag()->SetGenre(record->at(song_strGenres).get_asString());
  // and the rest...
  item->GetMusicInfoTag()->SetAlbum(record->at(song_strAlbum).get_asString());
  item->GetMusicInfoTag()->SetAlbumId(record->at(song_idAlbum).get_asInt());
  item->GetMusicInfoTag()->SetTrackAndDiskNumber(record->at(song_iTrack).get_asInt());
  item->GetMusicInfoTag()->SetDuration(record->at(song_iDuration).get_asInt());
  item->GetMusicInfoTag()->SetDatabaseId(record->at(song_idSong).get_asInt(), "song");
  SYSTEMTIME stTime;
  stTime.wYear = (WORD)record->at(song_iYear).get_asInt();
  item->GetMusicInfoTag()->SetReleaseDate(stTime);
  item->GetMusicInfoTag()->SetTitle(record->at(song_strTitle).get_asString());
  item->SetLabel(record->at(song_strTitle).get_asString());
  item->m_lStartOffset = record->at(song_iStartOffset).get_asInt();
  item->SetProperty("item_start", item->m_lStartOffset);
  item->m_lEndOffset = record->at(song_iEndOffset).get_asInt();
  item->GetMusicInfoTag()->SetMusicBrainzTrackID(record->at(song_strMusicBrainzTrackID).get_asString());
  item->GetMusicInfoTag()->SetMusicBrainzArtistID(record->at(song_strMusicBrainzArtistID).get_asString());
  item->GetMusicInfoTag()->SetMusicBrainzAlbumID(record->at(song_strMusicBrainzAlbumID).get_asString());
  item->GetMusicInfoTag()->SetMusicBrainzAlbumArtistID(record->at(song_strMusicBrainzAlbumArtistID).get_asString());
  item->GetMusicInfoTag()->SetMusicBrainzTRMID(record->at(song_strMusicBrainzTRMID).get_asString());
  item->GetMusicInfoTag()->SetRating(record->at(song_rating).get_asChar());
  item->GetMusicInfoTag()->SetComment(record->at(song_comment).get_asString());
  item->GetMusicInfoTag()->SetPlayCount(record->at(song_iTimesPlayed).get_asInt());
  item->GetMusicInfoTag()->SetLastPlayed(record->at(song_lastplayed).get_asString());
  CStdString strRealPath;
  URIUtils::AddFileToFolder(record->at(song_strPath).get_asString(), record->at(song_strFileName).get_asString(), strRealPath);
  item->GetMusicInfoTag()->SetURL(strRealPath);
  item->GetMusicInfoTag()->SetCompilation(m_pDS->fv(song_bCompilation).get_asInt() == 1);
  item->GetMusicInfoTag()->SetLoaded(true);
  // Get filename with full path
  if (strMusicDBbasePath.IsEmpty())
    item->SetPath(strRealPath);
  else
  {
    CMusicDbUrl itemUrl;
    if (!itemUrl.FromString(strMusicDBbasePath))
      return;
    
    CStdString strFileName = record->at(song_strFileName).get_asString();
    CStdString strExt = URIUtils::GetExtension(strFileName);
    CStdString path; path.Format("%ld%s", record->at(song_idSong).get_asInt(), strExt.c_str());
    itemUrl.AppendPath(path);
    item->SetPath(itemUrl.ToString());
  }
}

CAlbum CMusicDatabase::GetAlbumFromDataset(dbiplus::Dataset* pDS, bool imageURL /* = false*/)
{
  return GetAlbumFromDataset(pDS->get_sql_record(), imageURL);
}

CAlbum CMusicDatabase::GetAlbumFromDataset(const dbiplus::sql_record* const record, bool imageURL /* = false*/)
{
  CAlbum album;
  album.idAlbum = record->at(album_idAlbum).get_asInt();
  album.strAlbum = record->at(album_strAlbum).get_asString();
  if (album.strAlbum.IsEmpty())
    album.strAlbum = g_localizeStrings.Get(1050);
  album.artist = StringUtils::Split(record->at(album_strArtists).get_asString(), g_advancedSettings.m_musicItemSeparator);
  album.genre = StringUtils::Split(record->at(album_strGenres).get_asString(), g_advancedSettings.m_musicItemSeparator);
  album.iYear = record->at(album_iYear).get_asInt();
  if (imageURL)
    album.thumbURL.ParseString(record->at(album_strThumbURL).get_asString());
  album.iRating = record->at(album_iRating).get_asInt();
  album.iYear = record->at(album_iYear).get_asInt();
  album.strReview = record->at(album_strReview).get_asString();
  album.styles = StringUtils::Split(record->at(album_strStyles).get_asString(), g_advancedSettings.m_musicItemSeparator);
  album.moods = StringUtils::Split(record->at(album_strMoods).get_asString(), g_advancedSettings.m_musicItemSeparator);
  album.themes = StringUtils::Split(record->at(album_strThemes).get_asString(), g_advancedSettings.m_musicItemSeparator);
  album.strLabel = record->at(album_strLabel).get_asString();
  album.strType = record->at(album_strType).get_asString();
  album.bCompilation = record->at(album_bCompilation).get_asInt() == 1;
  album.iTimesPlayed = record->at(album_iTimesPlayed).get_asInt();
  return album;
}

CArtist CMusicDatabase::GetArtistFromDataset(dbiplus::Dataset* pDS, bool needThumb)
{
  return GetArtistFromDataset(pDS->get_sql_record(), needThumb);
}

CArtist CMusicDatabase::GetArtistFromDataset(const dbiplus::sql_record* const record, bool needThumb /* = true */)
{
  CArtist artist;
  artist.idArtist = record->at(artist_idArtist).get_asInt();
  artist.strArtist = record->at(artist_strArtist).get_asString();
  artist.genre = StringUtils::Split(record->at(artist_strGenres).get_asString(), g_advancedSettings.m_musicItemSeparator);
  artist.strBiography = record->at(artist_strBiography).get_asString();
  artist.styles = StringUtils::Split(record->at(artist_strStyles).get_asString(), g_advancedSettings.m_musicItemSeparator);
  artist.moods = StringUtils::Split(record->at(artist_strMoods).get_asString(), g_advancedSettings.m_musicItemSeparator);
  artist.strBorn = record->at(artist_strBorn).get_asString();
  artist.strFormed = record->at(artist_strFormed).get_asString();
  artist.strDied = record->at(artist_strDied).get_asString();
  artist.strDisbanded = record->at(artist_strDisbanded).get_asString();
  artist.yearsActive = StringUtils::Split(record->at(artist_strYearsActive).get_asString(), g_advancedSettings.m_musicItemSeparator);
  artist.instruments = StringUtils::Split(record->at(artist_strInstruments).get_asString(), g_advancedSettings.m_musicItemSeparator);

  if (needThumb)
  {
    artist.fanart.m_xml = record->at(artist_strFanart).get_asString();
    artist.fanart.Unpack();
    artist.thumbURL.ParseString(record->at(artist_strImage).get_asString());
  }

  return artist;
}

bool CMusicDatabase::GetSongByFileName(const CStdString& strFileName, CSong& song, int startOffset)
{
  try
  {
    song.Clear();
    CURL url(strFileName);

    if (url.GetProtocol()=="musicdb")
    {
      CStdString strFile = URIUtils::GetFileName(strFileName);
      URIUtils::RemoveExtension(strFile);
      return GetSongById(atol(strFile.c_str()), song);
    }

    CStdString strPath;
    URIUtils::GetDirectory(strFileName, strPath);
    URIUtils::AddSlashAtEnd(strPath);

    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    DWORD crc = ComputeCRC(strFileName);

    CStdString strSQL=PrepareSQL("select * from songview "
                                "where dwFileNameCRC='%ul' and strPath='%s'"
                                , crc,
                                strPath.c_str());
    if (startOffset)
      strSQL += PrepareSQL(" AND iStartOffset=%i", startOffset);

    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return false;
    }
    song = GetSongFromDataset();
    m_pDS->close(); // cleanup recordset data
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%s) failed", __FUNCTION__, strFileName.c_str());
  }

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
                                "where strArtist like '%s' and strAlbum like '%s' and "
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

bool CMusicDatabase::GetSongById(int idSong, CSong& song)
{
  try
  {
    song.Clear();

    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL=PrepareSQL("select * from songview "
                                "where idSong=%i"
                                , idSong);

    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return false;
    }
    song = GetSongFromDataset();
    m_pDS->close(); // cleanup recordset data
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%i) failed", __FUNCTION__, idSong);
  }

  return false;
}

bool CMusicDatabase::SearchArtists(const CStdString& search, CFileItemList &artists)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    // Exclude "Various Artists"
    int idVariousArtist = AddArtist(g_localizeStrings.Get(340));

    CStdString strSQL;
    if (search.GetLength() >= MIN_FULL_SEARCH_LENGTH)
      strSQL=PrepareSQL("select * from artist "
                                "where (strArtist like '%s%%' or strArtist like '%% %s%%') and idArtist <> %i "
                                , search.c_str(), search.c_str(), idVariousArtist );
    else
      strSQL=PrepareSQL("select * from artist "
                                "where strArtist like '%s%%' and idArtist <> %i "
                                , search.c_str(), idVariousArtist );

    if (!m_pDS->query(strSQL.c_str())) return false;
    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      return false;
    }

    CStdString artistLabel(g_localizeStrings.Get(557)); // Artist
    while (!m_pDS->eof())
    {
      CStdString path;
      path.Format("musicdb://2/%ld/", m_pDS->fv(0).get_asInt());
      CFileItemPtr pItem(new CFileItem(path, true));
      CStdString label;
      label.Format("[%s] %s", artistLabel.c_str(), m_pDS->fv(1).get_asString());
      pItem->SetLabel(label);
      label.Format("A %s", m_pDS->fv(1).get_asString()); // sort label is stored in the title tag
      pItem->GetMusicInfoTag()->SetTitle(label);
      pItem->GetMusicInfoTag()->SetDatabaseId(m_pDS->fv(0).get_asInt(), "artist");
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

bool CMusicDatabase::GetAlbumInfo(int idAlbum, CAlbum &info, VECSONGS* songs)
{
  try
  {
    if (idAlbum == -1)
      return false; // not in the database

    CStdString strSQL=PrepareSQL("select * from albumview where idAlbum = %ld", idAlbum);

    if (!m_pDS2->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS2->num_rows();
    if (iRowsFound != 0)
    {
      info = GetAlbumFromDataset(m_pDS2.get(), true); // true to grab the thumburl rather than the thumb
      int idAlbumInfo = m_pDS2->fv(album_idAlbumInfo).get_asInt();
      m_pDS2->close(); // cleanup recordset data

      if (songs)
        GetAlbumInfoSongs(idAlbumInfo, *songs);

      return true;
    }
    m_pDS2->close();
    return false;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%i) failed", __FUNCTION__, idAlbum);
  }

  return false;
}

bool CMusicDatabase::HasAlbumInfo(int idAlbum)
{
  try
  {
    if (idAlbum == -1)
      return false; // not in the database

    CStdString strSQL=PrepareSQL("select * from albuminfo where idAlbum = %ld", idAlbum);

    if (!m_pDS2->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS2->num_rows();
    m_pDS2->close();
    return iRowsFound > 0;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%i) failed", __FUNCTION__, idAlbum);
  }

  return false;
}

bool CMusicDatabase::DeleteAlbumInfo(int idAlbum)
{
  try
  {
    if (idAlbum == -1)
      return false; // not in the database

    CStdString strSQL = PrepareSQL("delete from albuminfo where idAlbum=%i",idAlbum);

    if (!m_pDS2->exec(strSQL.c_str()))
      return false;

    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s - (%i) failed", __FUNCTION__, idAlbum);
  }

  return false;
}

bool CMusicDatabase::GetArtistInfo(int idArtist, CArtist &info, bool needAll)
{
  try
  {
    if (idArtist == -1)
      return false; // not in the database

    CStdString strSQL=PrepareSQL("SELECT artist.idArtist AS idArtist, strArtist, "
                                 "  strBorn, strFormed, strGenres,"
                                 "  strMoods, strStyles, strInstruments, "
                                 "  strBiography, strDied, strDisbanded, "
                                 "  strYearsActive, strImage, strFanart "
                                 "  FROM artist "
                                 "  JOIN artistinfo "
                                 "    ON artist.idArtist = artistinfo.idArtist "
                                 "  WHERE artistinfo.idArtist = %i"
                                 , idArtist);

    if (!m_pDS2->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS2->num_rows();
    if (iRowsFound != 0)
    {
      info = GetArtistFromDataset(m_pDS2.get(),needAll);
      if (needAll)
      {
        strSQL=PrepareSQL("select * from discography where idArtist=%i",idArtist);
        m_pDS2->query(strSQL.c_str());
        while (!m_pDS2->eof())
        {
          info.discography.push_back(make_pair(m_pDS2->fv("strAlbum").get_asString(),m_pDS2->fv("strYear").get_asString()));
          m_pDS2->next();
        }
      }
      m_pDS2->close(); // cleanup recordset data
      return true;
    }
    m_pDS2->close();
    return false;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s - (%i) failed", __FUNCTION__, idArtist);
  }

  return false;
}

bool CMusicDatabase::DeleteArtistInfo(int idArtist)
{
  try
  {
    if (idArtist == -1)
      return false; // not in the database

    CStdString strSQL = PrepareSQL("delete from artistinfo where idArtist=%i",idArtist);

    if (!m_pDS2->exec(strSQL.c_str()))
      return false;

    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s - (%i) failed", __FUNCTION__, idArtist);
  }

  return false;
}

bool CMusicDatabase::GetAlbumInfoSongs(int idAlbumInfo, VECSONGS& songs)
{
  try
  {
    CStdString strSQL=PrepareSQL("select * from albuminfosong "
                                "where idAlbumInfo=%i "
                                "order by iTrack", idAlbumInfo);

    if (!m_pDS2->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS2->num_rows();
    if (iRowsFound == 0) return false;
    while (!m_pDS2->eof())
    {
      CSong song;
      song.iTrack = m_pDS2->fv("iTrack").get_asInt();
      song.strTitle = m_pDS2->fv("strTitle").get_asString();
      song.iDuration = m_pDS2->fv("iDuration").get_asInt();

      songs.push_back(song);
      m_pDS2->next();
    }

    m_pDS2->close(); // cleanup recordset data

    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%i) failed", __FUNCTION__, idAlbumInfo);
  }

  return false;
}

bool CMusicDatabase::GetTop100(const CStdString& strBaseDir, CFileItemList& items)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

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
      GetFileItemFromDataset(item.get(), strBaseDir);
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

    CStdString strSQL;
    strSQL.Format("select * from songview join albumview on (songview.idAlbum = albumview.idAlbum) where albumview.idAlbum in (select song.idAlbum from song where song.iTimesPlayed>0 group by idAlbum order by sum(song.iTimesPlayed) desc limit 100) order by albumview.idAlbum in (select song.idAlbum from song where song.iTimesPlayed>0 group by idAlbum order by sum(song.iTimesPlayed) desc limit 100)");
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
      GetFileItemFromDataset(item.get(), strBaseDir);
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

    CStdString strSQL;
    strSQL.Format("select distinct albumview.* from song join albumview on albumview.idAlbum=song.idAlbum where song.lastplayed IS NOT NULL order by song.lastplayed desc limit %i", RECENTLY_PLAYED_LIMIT);
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

    CStdString strSQL;
    strSQL.Format("select * from songview join albumview on (songview.idAlbum = albumview.idAlbum) where albumview.idAlbum in (select distinct albumview.idAlbum from albumview join song on albumview.idAlbum=song.idAlbum where song.lastplayed IS NOT NULL order by song.lastplayed desc limit %i)", g_advancedSettings.m_iMusicLibraryRecentlyAddedItems);
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
      GetFileItemFromDataset(item.get(), strBaseDir);
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

    CStdString strSQL;
    strSQL.Format("select * from albumview order by idAlbum desc limit %u", limit ? limit : g_advancedSettings.m_iMusicLibraryRecentlyAddedItems);

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
      GetFileItemFromDataset(item.get(), strBaseDir);
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

bool CMusicDatabase::GetSongsByPath(const CStdString& strPath1, CSongMap& songs, bool bAppendToMap)
{
  CStdString strPath(strPath1);
  try
  {
    if (!URIUtils::HasSlashAtEnd(strPath))
      URIUtils::AddSlashAtEnd(strPath);

    if (!bAppendToMap)
      songs.Clear();

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
      songs.Add(song.strFileName, song);
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

    CStdString strSQL;
    if (search.GetLength() >= MIN_FULL_SEARCH_LENGTH)
      strSQL=PrepareSQL("select * from songview where strTitle like '%s%%' or strTitle like '%% %s%%' limit 1000", search.c_str(), search.c_str());
    else
      strSQL=PrepareSQL("select * from songview where strTitle like '%s%%' limit 1000", search.c_str());

    if (!m_pDS->query(strSQL.c_str())) return false;
    if (m_pDS->num_rows() == 0) return false;

    CStdString songLabel = g_localizeStrings.Get(179); // Song
    while (!m_pDS->eof())
    {
      CFileItemPtr item(new CFileItem);
      GetFileItemFromDataset(item.get(), "musicdb://4/");
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
    if (search.GetLength() >= MIN_FULL_SEARCH_LENGTH)
      strSQL=PrepareSQL("select * from albumview where strAlbum like '%s%%' or strAlbum like '%% %s%%'", search.c_str(), search.c_str());
    else
      strSQL=PrepareSQL("select * from albumview where strAlbum like '%s%%'", search.c_str());

    if (!m_pDS->query(strSQL.c_str())) return false;

    CStdString albumLabel(g_localizeStrings.Get(558)); // Album
    while (!m_pDS->eof())
    {
      CAlbum album = GetAlbumFromDataset(m_pDS.get());
      CStdString path;
      path.Format("musicdb://3/%ld/", album.idAlbum);
      CFileItemPtr pItem(new CFileItem(path, album));
      CStdString label;
      label.Format("[%s] %s", albumLabel.c_str(), album.strAlbum);
      pItem->SetLabel(label);
      label.Format("B %s", album.strAlbum); // sort label is stored in the title tag
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

int CMusicDatabase::SetAlbumInfo(int idAlbum, const CAlbum& album, const VECSONGS& songs, bool bTransaction)
{
  CStdString strSQL;
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    if (bTransaction)
      BeginTransaction();

    // delete any album info we may have
    strSQL=PrepareSQL("delete from albuminfo where idAlbum=%i", idAlbum);
    m_pDS->exec(strSQL.c_str());

    // insert the albuminfo
    strSQL=PrepareSQL("insert into albuminfo (idAlbumInfo,idAlbum,strMoods,strStyles,strThemes,strReview,strImage,strLabel,strType,iRating,iYear) values(NULL,%i,'%s','%s','%s','%s','%s','%s','%s',%i,%i)",
                  idAlbum,
                  StringUtils::Join(album.moods, g_advancedSettings.m_musicItemSeparator).c_str(),
                  StringUtils::Join(album.styles, g_advancedSettings.m_musicItemSeparator).c_str(),
                  StringUtils::Join(album.themes, g_advancedSettings.m_musicItemSeparator).c_str(),
                  album.strReview.c_str(),
                  album.thumbURL.m_xml.c_str(),
                  album.strLabel.c_str(),
                  album.strType.c_str(),
                  album.iRating,
                  album.iYear);
    m_pDS->exec(strSQL.c_str());
    int idAlbumInfo = (int)m_pDS->lastinsertid();

    if (SetAlbumInfoSongs(idAlbumInfo, songs))
    {
      if (bTransaction)
        CommitTransaction();
    }
    else
    {
      if (bTransaction) // icky
        RollbackTransaction();
      idAlbumInfo = -1;
    }

    return idAlbumInfo;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed with query (%s)", __FUNCTION__, strSQL.c_str());
  }

  if (bTransaction)
    RollbackTransaction();

  return -1;
}

int CMusicDatabase::SetArtistInfo(int idArtist, const CArtist& artist)
{
  CStdString strSQL;
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    // delete any artist info we may have
    strSQL=PrepareSQL("delete from artistinfo where idArtist=%i", idArtist);
    m_pDS->exec(strSQL.c_str());
    strSQL=PrepareSQL("delete from discography where idArtist=%i", idArtist);
    m_pDS->exec(strSQL.c_str());

    // insert the artistinfo
    strSQL=PrepareSQL("insert into artistinfo (idArtistInfo,idArtist,strBorn,strFormed,strGenres,strMoods,strStyles,strInstruments,strBiography,strDied,strDisbanded,strYearsActive,strImage,strFanart) values(NULL,%i,'%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s')",
                  idArtist, artist.strBorn.c_str(),
                  artist.strFormed.c_str(),
                  StringUtils::Join(artist.genre, g_advancedSettings.m_musicItemSeparator).c_str(),
                  StringUtils::Join(artist.moods, g_advancedSettings.m_musicItemSeparator).c_str(),
                  StringUtils::Join(artist.styles, g_advancedSettings.m_musicItemSeparator).c_str(),
                  StringUtils::Join(artist.instruments, g_advancedSettings.m_musicItemSeparator).c_str(),
                  artist.strBiography.c_str(),
                  artist.strDied.c_str(),
                  artist.strDisbanded.c_str(),
                  StringUtils::Join(artist.yearsActive, g_advancedSettings.m_musicItemSeparator).c_str(),
                  artist.thumbURL.m_xml.c_str(),
                  artist.fanart.m_xml.c_str());
    m_pDS->exec(strSQL.c_str());
    int idArtistInfo = (int)m_pDS->lastinsertid();
    for (unsigned int i=0;i<artist.discography.size();++i)
    {
      strSQL=PrepareSQL("insert into discography (idArtist,strAlbum,strYear) values (%i,'%s','%s')",idArtist,artist.discography[i].first.c_str(),artist.discography[i].second.c_str());
      m_pDS->exec(strSQL.c_str());
    }

    return idArtistInfo;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s -  failed with query (%s)", __FUNCTION__, strSQL.c_str());
  }


  return -1;
}

bool CMusicDatabase::SetAlbumInfoSongs(int idAlbumInfo, const VECSONGS& songs)
{
  CStdString strSQL;
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    strSQL=PrepareSQL("delete from albuminfosong where idAlbumInfo=%i", idAlbumInfo);
    m_pDS->exec(strSQL.c_str());

    for (int i = 0; i < (int)songs.size(); i++)
    {
      CSong song = songs[i];
      strSQL=PrepareSQL("insert into albuminfosong (idAlbumInfoSong,idAlbumInfo,iTrack,strTitle,iDuration) values(NULL,%i,%i,'%s',%i)",
                    idAlbumInfo,
                    song.iTrack,
                    song.strTitle.c_str(),
                    song.iDuration);
      m_pDS->exec(strSQL.c_str());
    }
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed with query (%s)", __FUNCTION__, strSQL.c_str());
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
    CStdString strSongsToDelete = "";
    while (!m_pDS->eof())
    { // get the full song path
      CStdString strFileName;
      URIUtils::AddFileToFolder(m_pDS->fv("path.strPath").get_asString(), m_pDS->fv("song.strFileName").get_asString(), strFileName);

      //  Special case for streams inside an ogg file. (oggstream)
      //  The last dir in the path is the ogg file that
      //  contains the stream, so test if its there
      CStdString strExtension=URIUtils::GetExtension(strFileName);
      if (strExtension==".oggstream" || strExtension==".nsfstream")
      {
        CStdString strFileAndPath=strFileName;
        URIUtils::GetDirectory(strFileAndPath, strFileName);
        // we are dropping back to a file, so remove the slash at end
        URIUtils::RemoveSlashAtEnd(strFileName);
      }

      if (!CFile::Exists(strFileName))
      { // file no longer exists, so add to deletion list
        strSongsToDelete += m_pDS->fv("song.idSong").get_asString() + ",";
      }
      m_pDS->next();
    }
    m_pDS->close();

    if ( ! strSongsToDelete.IsEmpty() )
    {
      strSongsToDelete = "(" + strSongsToDelete.TrimRight(",") + ")";
      // ok, now delete these songs + all references to them from the linked tables
      strSQL = "delete from song where idSong in " + strSongsToDelete;
      m_pDS->exec(strSQL.c_str());
      strSQL = "delete from song_artist where idSong in " + strSongsToDelete;
      m_pDS->exec(strSQL.c_str());
      strSQL = "delete from song_genre where idSong in " + strSongsToDelete;
      m_pDS->exec(strSQL.c_str());
      strSQL = "delete from karaokedata where idSong in " + strSongsToDelete;
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
      CStdString strSongIds = "(";
      while (!m_pDS->eof())
      {
        strSongIds += m_pDS->fv("song.idSong").get_asString() + ",";
        m_pDS->next();
      }
      m_pDS->close();
      strSongIds.TrimRight(",");
      strSongIds += ")";
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
    CStdString strAlbumIds = "(";
    while (!m_pDS->eof())
    {
      strAlbumIds += m_pDS->fv("album.idAlbum").get_asString() + ",";
      m_pDS->next();
    }
    m_pDS->close();

    strAlbumIds.TrimRight(",");
    strAlbumIds += ")";
    // ok, now we can delete them and the references in the linked tables
    strSQL = "delete from album where idAlbum in " + strAlbumIds;
    m_pDS->exec(strSQL.c_str());
    strSQL = "delete from album_artist where idAlbum in " + strAlbumIds;
    m_pDS->exec(strSQL.c_str());
    strSQL = "delete from album_genre where idAlbum in " + strAlbumIds;
    m_pDS->exec(strSQL.c_str());
    strSQL = "delete from albuminfo where idAlbum in " + strAlbumIds;
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
    CStdString deleteSQL;
    while (!m_pDS->eof())
    {
      // anything that isn't a parent path of a song path is to be deleted
      CStdString path = m_pDS->fv("strPath").get_asString();
      CStdString sql = PrepareSQL("select count(idPath) from songpaths where SUBSTR(strPath,1,%i)='%s'", StringUtils::utf8_strlen(path.c_str()), path.c_str());
      if (m_pDS2->query(sql.c_str()) && m_pDS2->num_rows() == 1 && m_pDS2->fv(0).get_asInt() == 0)
        deleteSQL += PrepareSQL("%i,", m_pDS->fv("idPath").get_asInt()); // nothing found, so delete
      m_pDS2->close();
      m_pDS->next();
    }
    m_pDS->close();

    if ( ! deleteSQL.IsEmpty() )
    {
      deleteSQL = "DELETE FROM path WHERE idPath IN (" + deleteSQL.TrimRight(',') + ")";
      // do the deletion, and drop our temp table
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

bool CMusicDatabase::CleanupArtists()
{
  try
  {
    // (nested queries by Bobbin007)
    // must be executed AFTER the song, album and their artist link tables are cleaned.
    // don't delete the "Various Artists" string
    CStdString strVariousArtists = g_localizeStrings.Get(340);
    int idVariousArtists = AddArtist(strVariousArtists);
    CStdString strSQL = "delete from artist where idArtist not in (select idArtist from song_artist)";
    strSQL += " and idArtist not in (select idArtist from album_artist)";
    CStdString strSQL2;
    strSQL2.Format(" and idArtist<>%i", idVariousArtists);
    strSQL += strSQL2;
    m_pDS->exec(strSQL.c_str());
    m_pDS->exec("delete from artistinfo where idArtist not in (select idArtist from artist)");
    m_pDS->exec("delete from album_artist where idArtist not in (select idArtist from artist)");
    m_pDS->exec("delete from song_artist where idArtist not in (select idArtist from artist)");
    m_pDS->exec("delete from discography where idArtist not in (select idArtist from artist)");
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
    // Cleanup orphaned genres (ie those that don't belong to a song or an albuminfo entry)
    // (nested queries by Bobbin007)
    // Must be executed AFTER the song, song_genre, albuminfo and album_genre tables have been cleaned.
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

int CMusicDatabase::Cleanup(CGUIDialogProgress *pDlgProgress)
{
  if (NULL == m_pDB.get()) return ERROR_DATABASE;
  if (NULL == m_pDS.get()) return ERROR_DATABASE;

  int ret = ERROR_OK;
  unsigned int time = XbmcThreads::SystemClockMillis();
  CLog::Log(LOGNOTICE, "%s: Starting musicdatabase cleanup ..", __FUNCTION__);
  ANNOUNCEMENT::CAnnouncementManager::Announce(ANNOUNCEMENT::AudioLibrary, "xbmc", "OnCleanStarted");

  // first cleanup any songs with invalid paths
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
  if (!CleanupSongs())
  {
    ret = ERROR_REORG_SONGS;
    goto error;
  }
  // then the albums that are not linked to a song or to albuminfo, or whose path is removed
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
  }
  time = XbmcThreads::SystemClockMillis() - time;
  CLog::Log(LOGNOTICE, "%s: Cleaning musicdatabase done. Operation took %s", __FUNCTION__, StringUtils::SecondsToTimeString(time / 1000).c_str());
  ANNOUNCEMENT::CAnnouncementManager::Announce(ANNOUNCEMENT::AudioLibrary, "xbmc", "OnCleanFinished");

  if (!Compress(false))
  {
    return ERROR_COMPRESSING;
  }
  return ERROR_OK;

error:
  RollbackTransaction();
  ANNOUNCEMENT::CAnnouncementManager::Announce(ANNOUNCEMENT::AudioLibrary, "xbmc", "OnCleanFinished");
  return ret;
}

void CMusicDatabase::DeleteAlbumInfo()
{
  // open our database
  Open();
  if (NULL == m_pDB.get()) return ;
  if (NULL == m_pDS.get()) return ;

  // If we are scanning for music info in the background,
  // other writing access to the database is prohibited.
  if (g_application.IsMusicScanning())
  {
    CGUIDialogOK::ShowAndGetInput(189, 14057, 0, 0);
    return;
  }

  CStdString strSQL="select * from albuminfo,album,artist where and albuminfo.idAlbum=album.idAlbum and album.idArtist=artist.idArtist order by album.strAlbum";
  if (!m_pDS->query(strSQL.c_str())) return ;
  int iRowsFound = m_pDS->num_rows();
  if (iRowsFound == 0)
  {
    m_pDS->close();
    CGUIDialogOK::ShowAndGetInput(313, 425, 0, 0);
  }
  vector<CAlbum> vecAlbums;
  while (!m_pDS->eof())
  {
    CAlbum album;
    album.idAlbum = m_pDS->fv("album.idAlbum").get_asInt() ;
    album.strAlbum = m_pDS->fv("album.strAlbum").get_asString();
    album.artist = StringUtils::Split(m_pDS->fv("album.strArtists").get_asString(), g_advancedSettings.m_musicItemSeparator);
    vecAlbums.push_back(album);
    m_pDS->next();
  }
  m_pDS->close();

  // Show a selectdialog that the user can select the albuminfo to delete
  CGUIDialogSelect *pDlg = (CGUIDialogSelect*)g_windowManager.GetWindow(WINDOW_DIALOG_SELECT);
  if (pDlg)
  {
    pDlg->SetHeading(g_localizeStrings.Get(181).c_str());
    pDlg->Reset();
    for (int i = 0; i < (int)vecAlbums.size(); ++i)
    {
      CAlbum& album = vecAlbums[i];
      pDlg->Add(album.strAlbum + " - " + StringUtils::Join(album.artist, g_advancedSettings.m_musicItemSeparator));
    }
    pDlg->DoModal();

    // and wait till user selects one
    int iSelectedAlbum = pDlg->GetSelectedLabel();
    if (iSelectedAlbum < 0)
    {
      vecAlbums.erase(vecAlbums.begin(), vecAlbums.end());
      return ;
    }

    CAlbum& album = vecAlbums[iSelectedAlbum];
    strSQL=PrepareSQL("delete from albuminfo where albuminfo.idAlbum=%i", album.idAlbum);
    if (!m_pDS->exec(strSQL.c_str())) return ;

    vecAlbums.erase(vecAlbums.begin(), vecAlbums.end());
  }
}

bool CMusicDatabase::LookupCDDBInfo(bool bRequery/*=false*/)
{
#ifdef HAS_DVD_DRIVE
  if (!g_guiSettings.GetBool("audiocds.usecddb"))
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
    CStdString strFile;
    strFile.Format("%x.cddb", pCdInfo->GetCddbDiscId());
    CFile::Delete(URIUtils::AddFileToFolder(g_settings.GetCDDBFolder(), strFile));
  }

  // Prepare cddb
  Xcddb cddb;
  cddb.setCacheDir(g_settings.GetCDDBFolder());

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
          if (!strArtist.IsEmpty())
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
        CStdString strErrorText;
        strErrorText.Format("[%d] %s", cddb.getLastError(), cddb.getLastErrorText());
        CGUIDialogOK::ShowAndGetInput(255, 257, strErrorText, 0);
      }
    } // if ( !cddb.queryCDinfo( pCdInfo ) )
    else
      pDialogProgress->Close();
  } // if (pCdInfo->HasCDDBInfo() && g_settings.m_bUseCDDB)

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
  if (!CDirectory::GetDirectory(g_settings.GetCDDBFolder(), items, ".cddb", DIR_FLAG_NO_FILE_DIRS))
  {
    CGUIDialogOK::ShowAndGetInput(313, 426, 0, 0);
    return ;
  }
  // Show a selectdialog that the user can select the albuminfo to delete
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
      strFile.Delete(strFile.size() - 5, 5);
      ULONG lDiscId = strtoul(strFile.c_str(), NULL, 16);
      Xcddb cddb;
      cddb.setCacheDir(g_settings.GetCDDBFolder());

      if (!cddb.queryCache(lDiscId))
        continue;

      CStdString strDiskTitle, strDiskArtist;
      cddb.getDiskTitle(strDiskTitle);
      cddb.getDiskArtist(strDiskArtist);

      CStdString str;
      if (strDiskArtist.IsEmpty())
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
        CStdString strFile;
        strFile.Format("%x.cddb", it->first);
        CFile::Delete(URIUtils::AddFileToFolder(g_settings.GetCDDBFolder(), strFile));
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
    CGUIDialogProgress* dlgProgress = (CGUIDialogProgress*)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
    if (dlgProgress)
    {
      CMusicDatabase musicdatabase;
      if (musicdatabase.Open())
      {
        int iReturnString = musicdatabase.Cleanup(dlgProgress);
        musicdatabase.Close();

        if (iReturnString != ERROR_OK)
        {
          CGUIDialogOK::ShowAndGetInput(313, iReturnString, 0, 0);
        }
      }
      dlgProgress->Close();
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

    CStdString strSQLExtra;
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
      CStdString strDir; strDir.Format("%ld/", m_pDS->fv("genre.idGenre").get_asInt());
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

bool CMusicDatabase::GetYearsNav(const CStdString& strBaseDir, CFileItemList& items)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CMusicDbUrl musicUrl;
    if (!musicUrl.FromString(strBaseDir))
      return false;

    // get years from album list
    CStdString strSQL="select distinct iYear from album where iYear <> 0";

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
      CFileItemPtr pItem(new CFileItem(m_pDS->fv("iYear").get_asString()));
      SYSTEMTIME stTime;
      stTime.wYear = (WORD)m_pDS->fv("iYear").get_asInt();
      pItem->GetMusicInfoTag()->SetReleaseDate(stTime);

      CMusicDbUrl itemUrl = musicUrl;
      CStdString strDir; strDir.Format("%ld/", m_pDS->fv("iYear").get_asInt());
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
      CStdString strDir; strDir.Format("%s/", labelValue.c_str());
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

    CStdString strSQLExtra;
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
        CStdString path; path.Format("%ld/", artist.idArtist);
        itemUrl.AppendPath(path);
        pItem->SetPath(itemUrl.ToString());

        pItem->GetMusicInfoTag()->SetDatabaseId(artist.idArtist, "artist");
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

// This function won't be required if/when the fileitem tag has idSong information
bool CMusicDatabase::GetAlbumFromSong(const CSong &song, CAlbum &album)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    if (song.idSong != -1) return GetAlbumFromSong(song.idSong, album);

    CStdString path, file;
    URIUtils::Split(song.strFileName, path, file);

    CStdString strSQL = PrepareSQL("select albumview.* from song join albumview on song.idAlbum = albumview.idAlbum join path on song.idPath = path.idPath where song.strFileName='%s' and path.strPath='%s'", file.c_str(), path.c_str());
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

    CStdString strSQLExtra;
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
        CStdString path; path.Format("%ld/", record->at(album_idAlbum).get_asInt());
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

    CStdString strSQLExtra;
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
        GetFileItemFromDataset(record, item.get(), musicUrl.ToString());
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

bool CMusicDatabase::UpdateOldVersion(int version)
{
  if (version < 16)
  {
    // only if MySQL is used and default character set is not utf8
    // string data needs to be converted to proper utf8
    CStdString charset = m_pDS->getDatabase()->getDefaultCharset();
    if (!m_sqlite && !charset.empty() && charset != "utf8")
    {
      map<CStdString, CStdStringArray> tables;
      map<CStdString, CStdStringArray>::iterator itt;
      CStdStringArray::iterator itc;

      //columns that need to be converted
      CStdStringArray c1;
      c1.push_back("strAlbum");
      c1.push_back("strExtraArtists");
      c1.push_back("strExtraGenres");
      tables.insert(pair<CStdString, CStdStringArray> ("album", c1));

      CStdStringArray c2;
      c2.push_back("strExtraGenres");
      c2.push_back("strMoods");
      c2.push_back("strStyles");
      c2.push_back("strThemes");
      c2.push_back("strReview");
      c2.push_back("strLabel");
      tables.insert(pair<CStdString, CStdStringArray> ("albuminfo", c2));

      CStdStringArray c3;
      c3.push_back("strTitle");
      tables.insert(pair<CStdString, CStdStringArray> ("albuminfosong", c3));

      CStdStringArray c4;
      c4.push_back("strArtist");
      tables.insert(pair<CStdString, CStdStringArray> ("artist", c4));

      CStdStringArray c5;
      c5.push_back("strBorn");
      c5.push_back("strFormed");
      c5.push_back("strGenres");
      c5.push_back("strMoods");
      c5.push_back("strStyles");
      c5.push_back("strInstruments");
      c5.push_back("strBiography");
      c5.push_back("strDied");
      c5.push_back("strDisbanded");
      c5.push_back("strYearsActive");
      tables.insert(pair<CStdString, CStdStringArray> ("artistinfo", c5));

      CStdStringArray c6;
      c6.push_back("strAlbum");
      tables.insert(pair<CStdString, CStdStringArray> ("discography", c6));

      CStdStringArray c7;
      c7.push_back("strGenre");
      tables.insert(pair<CStdString, CStdStringArray> ("genre", c7));

      CStdStringArray c8;
      c8.push_back("strKaraLyrics");
      tables.insert(pair<CStdString, CStdStringArray> ("karaokedata", c8));

      CStdStringArray c9;
      c9.push_back("strTitle");
      c9.push_back("strFilename");
      c9.push_back("comment");
      tables.insert(pair<CStdString, CStdStringArray> ("song", c9));

      CStdStringArray c10;
      c10.push_back("strPath");
      tables.insert(pair<CStdString, CStdStringArray> ("path", c10));

      for (itt = tables.begin(); itt != tables.end(); ++itt)
      {
        CStdString q;
        q = PrepareSQL("UPDATE `%s` SET", itt->first.c_str());
        for (itc = itt->second.begin(); itc != itt->second.end(); ++itc)
        {
          q += PrepareSQL(" `%s` = CONVERT(CAST(CONVERT(`%s` USING %s) AS BINARY) USING utf8)",
                          itc->c_str(), itc->c_str(), charset.c_str());
          if (*itc != itt->second.back())
          {
            q += ", ";
          }
        }
        m_pDS->exec(q);
      }
    }
  }
  if (version < 17)
  {
    m_pDS->exec("CREATE INDEX idxAlbum2 ON album(idArtist)");
    m_pDS->exec("CREATE INDEX idxSong3 ON song(idAlbum)");
    m_pDS->exec("CREATE INDEX idxSong4 ON song(idArtist)");
    m_pDS->exec("CREATE INDEX idxSong5 ON song(idGenre)");
    m_pDS->exec("CREATE INDEX idxSong6 ON song(idPath)");
  }
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
    m_pDS->exec("CREATE UNIQUE INDEX idxAlbumArtist_1 ON album_artist ( idAlbum, idArtist )\n");
    m_pDS->exec("CREATE UNIQUE INDEX idxAlbumArtist_2 ON album_artist ( idArtist, idAlbum )\n");
    m_pDS->exec("CREATE INDEX idxAlbumArtist_3 ON album_artist ( boolFeatured )\n");
    m_pDS->exec("INSERT INTO album_artist (idArtist, idAlbum, boolFeatured, iOrder) SELECT idArtist, idAlbum, 1, iPosition FROM exartistalbum");
    m_pDS->exec("REPLACE INTO album_artist (idArtist, idAlbum, boolFeatured, iOrder) SELECT idArtist, idAlbum, 0, 0 FROM album");

    CStdString strSQL;
    strSQL=PrepareSQL("SELECT album.idAlbum AS idAlbum, strExtraArtists,"
                      "  album.idArtist AS idArtist, strArtist FROM album "
                      "  LEFT OUTER JOIN artist ON album.idArtist=artist.idArtist");
    if (!m_pDS->query(strSQL.c_str()))
    {
      CLog::Log(LOGDEBUG, "%s could not upgrade albums table", __FUNCTION__);
      return false;
    }

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
    m_pDS->exec("CREATE INDEX idxAlbum ON album(strAlbum)");
    m_pDS->exec("DROP TABLE IF EXISTS exartistalbum");
  }

  if (version < 22)
  {
    m_pDS->exec("CREATE TABLE song_artist ( idArtist integer, idSong integer, boolFeatured integer, iOrder integer )\n");
    m_pDS->exec("CREATE UNIQUE INDEX idxSongArtist_1 ON song_artist ( idSong, idArtist )\n");
    m_pDS->exec("CREATE UNIQUE INDEX idxSongArtist_2 ON song_artist ( idArtist, idSong )\n");
    m_pDS->exec("CREATE INDEX idxSongArtist_3 ON song_artist ( boolFeatured )\n");
    m_pDS->exec("INSERT INTO song_artist (idArtist, idSong, boolFeatured, iOrder) SELECT idArtist, idSong, 1, iPosition FROM exartistsong");
    m_pDS->exec("REPLACE INTO song_artist (idArtist, idSong, boolFeatured, iOrder) SELECT idArtist, idSong, 0, 0 FROM song");

    CStdString strSQL;
    strSQL=PrepareSQL("SELECT song.idSong AS idSong, strExtraArtists,"
                      "  song.idArtist AS idArtist, strArtist FROM song "
                      "  LEFT OUTER JOIN artist ON song.idArtist=artist.idArtist");
    if (!m_pDS->query(strSQL.c_str()))
    {
      CLog::Log(LOGDEBUG, "%s could not upgrade songs table", __FUNCTION__);
      return false;
    }

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
    m_pDS->exec("CREATE INDEX idxSong ON song(strTitle)");
    m_pDS->exec("CREATE INDEX idxSong1 ON song(iTimesPlayed)");
    m_pDS->exec("CREATE INDEX idxSong2 ON song(lastplayed)");
    m_pDS->exec("CREATE INDEX idxSong3 ON song(idAlbum)");
    m_pDS->exec("CREATE INDEX idxSong6 ON song(idPath)");
    m_pDS->exec("DROP TABLE IF EXISTS exartistsong");
  }

  if (version < 23)
  {
    m_pDS->exec("CREATE TABLE album_genre ( idGenre integer, idAlbum integer, iOrder integer )\n");
    m_pDS->exec("CREATE UNIQUE INDEX idxAlbumGenre_1 ON album_genre ( idAlbum, idGenre )\n");
    m_pDS->exec("CREATE UNIQUE INDEX idxAlbumGenre_2 ON album_genre ( idGenre, idAlbum )\n");
    m_pDS->exec("INSERT INTO album_genre ( idGenre, idAlbum, iOrder) SELECT idGenre, idAlbum, iPosition FROM exgenrealbum");
    m_pDS->exec("REPLACE INTO album_genre ( idGenre, idAlbum, iOrder) SELECT idGenre, idAlbum, 0 FROM album");

    CStdString strSQL;
    strSQL=PrepareSQL("SELECT album.idAlbum AS idAlbum, strExtraGenres,"
                      "  album.idGenre AS idGenre, strGenre FROM album "
                      "  JOIN genre ON album.idGenre=genre.idGenre");
    if (!m_pDS->query(strSQL.c_str()))
    {
      CLog::Log(LOGDEBUG, "%s could not upgrade albums table", __FUNCTION__);
      return false;
    }

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
    m_pDS->exec("CREATE INDEX idxAlbum ON album(strAlbum)");
    m_pDS->exec("DROP TABLE IF EXISTS exgenrealbum");
  }

  if (version < 24)
  {
    m_pDS->exec("CREATE TABLE song_genre ( idGenre integer, idSong integer, iOrder integer )\n");
    m_pDS->exec("CREATE UNIQUE INDEX idxSongGenre_1 ON song_genre ( idSong, idGenre )\n");
    m_pDS->exec("CREATE UNIQUE INDEX idxSongGenre_2 ON song_genre ( idGenre, idSong )\n");
    m_pDS->exec("INSERT INTO song_genre ( idGenre, idSong, iOrder) SELECT idGenre, idSong, iPosition FROM exgenresong");
    m_pDS->exec("REPLACE INTO song_genre ( idGenre, idSong, iOrder) SELECT idGenre, idSong, 0 FROM song");

    CStdString strSQL;
    strSQL=PrepareSQL("SELECT song.idSong AS idSong, strExtraGenres,"
                      "  song.idGenre AS idGenre, strGenre FROM song "
                      "  JOIN genre ON song.idGenre=genre.idGenre");
    if (!m_pDS->query(strSQL.c_str()))
    {
      CLog::Log(LOGDEBUG, "%s could not upgrade songs table", __FUNCTION__);
      return false;
    }

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
    m_pDS->exec("CREATE INDEX idxSong ON song(strTitle)");
    m_pDS->exec("CREATE INDEX idxSong1 ON song(iTimesPlayed)");
    m_pDS->exec("CREATE INDEX idxSong2 ON song(lastplayed)");
    m_pDS->exec("CREATE INDEX idxSong3 ON song(idAlbum)");
    m_pDS->exec("CREATE INDEX idxSong6 ON song(idPath)");
    m_pDS->exec("DROP TABLE IF EXISTS exgenresong");
  }

  if (version < 25)
  {
    m_pDS->exec("ALTER TABLE album ADD bCompilation integer not null default '0'");
    m_pDS->exec("CREATE INDEX idxAlbum_1 ON album(bCompilation)");
  }

  if (version < 26)
  { // add art table
    m_pDS->exec("CREATE TABLE art(art_id INTEGER PRIMARY KEY, media_id INTEGER, media_type TEXT, type TEXT, url TEXT)");
    m_pDS->exec("CREATE INDEX ix_art ON art(media_id, media_type(20), type(20))");
    m_pDS->exec("CREATE TRIGGER delete_song AFTER DELETE ON song FOR EACH ROW BEGIN DELETE FROM art WHERE media_id=old.idSong AND media_type='song'; END");
    m_pDS->exec("CREATE TRIGGER delete_album AFTER DELETE ON album FOR EACH ROW BEGIN DELETE FROM art WHERE media_id=old.idAlbum AND media_type='album'; END");
    m_pDS->exec("CREATE TRIGGER delete_artist AFTER DELETE ON artist FOR EACH ROW BEGIN DELETE FROM art WHERE media_id=old.idArtist AND media_type='artist'; END");
  }

  if (version < 27)
  {
    m_pDS->exec("DROP TABLE thumb");

    g_settings.m_musicNeedsUpdate = 27;
    g_settings.Save();
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
  // always recreate the views after any table change
  CreateViews();

  return true;
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

bool CMusicDatabase::GetAlbumPath(int idAlbum, CStdString& path)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS2.get()) return false;

    path.Empty();

    CStdString strSQL=PrepareSQL("select strPath from song join path on song.idPath = path.idPath where song.idAlbum=%ld", idAlbum);
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
  SetArtForItem(idAlbum, "album", "thumb", strThumb);
  // TODO: We should prompt the user to update the art for songs
  CStdString sql = PrepareSQL("UPDATE art"
                              " SET art_url='-'"
                              " WHERE media_type='song'"
                              " AND art_type='thumb'"
                              " AND media_id IN"
                              " (SELECT idSong FROM song WHERE idAlbum=%ld)", idAlbum);
  ExecuteQuery(sql);
  return true;
}

bool CMusicDatabase::GetArtistPath(int idArtist, CStdString &basePath)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS2.get()) return false;

    // find all albums from this artist, and all the paths to the songs from those albums
    CStdString strSQL=PrepareSQL("SELECT strPath"
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
    basePath.Empty();
    while (!m_pDS2->eof())
    {
      CStdString path = m_pDS2->fv("strPath").get_asString();
      if (basePath.IsEmpty())
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
    if (strArtist.IsEmpty())
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

    int iCount = GetSongsCount(filter);
    if (iCount <= 0)
      return false;
    int iRandom = rand() % iCount;

    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    // We don't use PrepareSQL here, as the WHERE clause is already formatted
    CStdString strSQL = PrepareSQL("select %s from songview ", !filter.fields.empty() ? filter.fields.c_str() : "*");
    Filter extFilter = filter;
    extFilter.AppendOrder("idSong");
    extFilter.limit = "1";

    if (!CDatabase::BuildSQL(strSQL, extFilter, strSQL))
      return false;

    strSQL += PrepareSQL(" OFFSET %i", iRandom);

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
    GetFileItemFromDataset(item, "");
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
  return GetAlbumsByWhere(strBaseDir, filter, items);
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
  return strtol(GetSingleValue("album", "count(idAlbum)", "bCompilation = 1"), NULL, 10);
}

void CMusicDatabase::SplitString(const CStdString &multiString, vector<string> &vecStrings, CStdString &extraStrings)
{
  vecStrings = StringUtils::Split(multiString, g_advancedSettings.m_musicItemSeparator);
  for (unsigned int i = 1; i < vecStrings.size(); i++)
    extraStrings += g_advancedSettings.m_musicItemSeparator + CStdString(vecStrings[i]);
}

bool CMusicDatabase::SetPathHash(const CStdString &path, const CStdString &hash)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    if (hash.IsEmpty())
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

bool CMusicDatabase::RemoveSongsFromPath(const CStdString &path1, CSongMap &songs, bool exact)
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
      std::vector<int> ids;
      CStdString songIds = "(";
      while (!m_pDS->eof())
      {
        CSong song = GetSongFromDataset();
        song.strThumb = GetArtForItem(song.idSong, "song", "thumb");
        songs.Add(song.strFileName, song);
        songIds += PrepareSQL("%i,", song.idSong);
        ids.push_back(song.idSong);
        m_pDS->next();
      }
      songIds.TrimRight(",");
      songIds += ")";

      m_pDS->close();

      //TODO: move this below the m_pDS->exec block, once UPnP doesn't rely on this anymore
      for (unsigned int i = 0; i < ids.size(); i++)
        AnnounceRemove("song", ids[i]);

      // and delete all songs, and anything linked to them
      sql = "delete from song where idSong in " + songIds;
      m_pDS->exec(sql.c_str());
      sql = "delete from song_artist where idSong in " + songIds;
      m_pDS->exec(sql.c_str());
      sql = "delete from song_genre where idSong in " + songIds;
      m_pDS->exec(sql.c_str());
      sql = "delete from karaokedata where idSong in " + songIds;
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

bool CMusicDatabase::GetPaths(set<CStdString> &paths)
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
    if (filePath.IsEmpty()) return false;
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
  if (url.GetProtocol()=="musicdb")
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
    CStdString strPath;
    URIUtils::GetDirectory(filePath, strPath);
    URIUtils::AddSlashAtEnd(strPath);

    DWORD crc = ComputeCRC(filePath);

    CStdString sql = PrepareSQL("select idSong from song join path on song.idPath = path.idPath where song.dwFileNameCRC='%ul'and path.strPath='%s'", crc, strPath.c_str());
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
        strSQL = PrepareSQL("select * from content where strPath='musicdb://1/%i/'",params.GetGenreId());
        m_pDS->query(strSQL.c_str());
      }
      if (m_pDS->eof() && params.GetAlbumId() != -1) // check album
      {
        strSQL = PrepareSQL("select * from content where strPath='musicdb://3/%i/'",params.GetGenreId());
        m_pDS->query(strSQL.c_str());
      }
      if (m_pDS->eof() && params.GetArtistId() != -1) // check artist
      {
        strSQL = PrepareSQL("select * from content where strPath='musicdb://2/%i/'",params.GetArtistId());
        m_pDS->query(strSQL.c_str());
      }
      if (m_pDS->eof()) // general albums setting
      {
        strSQL = PrepareSQL("select * from content where strPath='musicdb://3/'");
        m_pDS->query(strSQL.c_str());
      }
      if (m_pDS->eof()) // general artist setting
      {
        strSQL = PrepareSQL("select * from content where strPath='musicdb://2/'");
        m_pDS->query(strSQL.c_str());
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
          info = boost::dynamic_pointer_cast<ADDON::CScraper>(addon->Clone(addon));
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
          info = boost::dynamic_pointer_cast<ADDON::CScraper>(defaultScraper->Clone(defaultScraper));
        }
      }
    }
    m_pDS->close();

    if (!info)
    { // use default music scraper instead
      ADDON::AddonPtr addon;
      if(ADDON::CAddonMgr::Get().GetDefault(type, addon))
      {
        info = boost::dynamic_pointer_cast<ADDON::CScraper>(addon);
        return (info);
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

void CMusicDatabase::ExportToXML(const CStdString &xmlFile, bool singleFiles, bool images, bool overwrite)
{
  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;
    if (NULL == m_pDS2.get()) return;

    // find all albums
    CStdString sql = "select albumview.*,albuminfo.strImage,albuminfo.idAlbumInfo from albuminfo "
                     "join albumview on albuminfo.idAlbum=albumview.idAlbum ";

    m_pDS->query(sql.c_str());

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

    int total = m_pDS->num_rows();
    int current = 0;

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
    while (!m_pDS->eof())
    {
      CAlbum album = GetAlbumFromDataset(m_pDS.get());
      album.thumbURL.Clear();
      album.thumbURL.ParseString(m_pDS->fv("albuminfo.strImage").get_asString());
      int idAlbumInfo = m_pDS->fv("albuminfo.idAlbumInfo").get_asInt();
      GetAlbumInfoSongs(idAlbumInfo,album.songs);
      CStdString strPath;
      GetAlbumPath(album.idAlbum,strPath);
      album.Save(pMain, "album", strPath);
      if (singleFiles)
      {
        if (!CDirectory::Exists(strPath))
          CLog::Log(LOGDEBUG, "%s - Not exporting item %s as it does not exist", __FUNCTION__, strPath.c_str());
        else
        {
          CStdString nfoFile;
          URIUtils::AddFileToFolder(strPath, "album.nfo", nfoFile);
          if (overwrite || !CFile::Exists(nfoFile))
          {
            if (!xmlDoc.SaveFile(nfoFile))
              CLog::Log(LOGERROR, "%s: Album nfo export failed! ('%s')", __FUNCTION__, nfoFile.c_str());
          }

          if (images)
          {
            string thumb = GetArtForItem(album.idAlbum, "album", "thumb");
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
      m_pDS->next();
      current++;
    }
    m_pDS->close();

    // find all artists
    sql = "SELECT artist.idArtist AS idArtist, strArtist, "
          "  strBorn, strFormed, strGenres,"
          "  strMoods, strStyles, strInstruments, "
          "  strBiography, strDied, strDisbanded, "
          "  strYearsActive, strImage, strFanart "
          "  FROM artist "
          "  JOIN artistinfo "
          "    ON artist.idArtist=artistinfo.idArtist";

    // needed due to getartistpath
    auto_ptr<dbiplus::Dataset> pDS;
    pDS.reset(m_pDB->CreateDataset());
    pDS->query(sql.c_str());

    total = pDS->num_rows();
    current = 0;

    while (!pDS->eof())
    {
      CArtist artist = GetArtistFromDataset(pDS.get());
      CStdString strSQL=PrepareSQL("select * from discography where idArtist=%i",artist.idArtist);
      m_pDS->query(strSQL.c_str());
      while (!m_pDS->eof())
      {
        artist.discography.push_back(make_pair(m_pDS->fv("strAlbum").get_asString(),m_pDS->fv("strYear").get_asString()));
        m_pDS->next();
      }
      m_pDS->close();
      CStdString strPath;
      GetArtistPath(artist.idArtist,strPath);
      artist.Save(pMain, "artist", strPath);

      map<string, string> artwork;
      if (GetArtForItem(artist.idArtist, "artist", artwork) && !singleFiles)
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
          CStdString nfoFile;
          URIUtils::AddFileToFolder(strPath, "artist.nfo", nfoFile);
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
      pDS->next();
      current++;
    }
    pDS->close();

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
        CArtist artist;
        artist.Load(entry);
        strTitle = artist.strArtist;
        int idArtist = GetArtistByName(artist.strArtist);
        if (idArtist > -1)
          SetArtistInfo(idArtist,artist);

        current++;
      }
      else if (strnicmp(entry->Value(), "album", 5) == 0)
      {
        CAlbum album;
        album.Load(entry);
        strTitle = album.strAlbum;
        int idAlbum = GetAlbumByName(album.strAlbum,album.artist);
        if (idAlbum > -1)
          SetAlbumInfo(idAlbum,album,album.songs,false);

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
  }
  if (progress)
    progress->Close();
}

void CMusicDatabase::AddKaraokeData(int idSong, const CSong& song)
{
  try
  {
    CStdString strSQL;

    // If song.iKaraokeNumber is non-zero, we already have it in the database. Just replace the song ID.
    if ( song.iKaraokeNumber > 0 )
    {
      CStdString strSQL = PrepareSQL("UPDATE karaokedata SET idSong=%i WHERE iKaraNumber=%i", idSong, song.iKaraokeNumber);
      m_pDS->exec(strSQL.c_str());
      return;
    }

    // Add new karaoke data
    DWORD crc = ComputeCRC( song.strFileName );

    // Get the maximum number allocated
    strSQL=PrepareSQL( "SELECT MAX(iKaraNumber) FROM karaokedata" );
    if (!m_pDS->query(strSQL.c_str())) return;

    int iKaraokeNumber = g_advancedSettings.m_karaokeStartIndex;

    if ( m_pDS->num_rows() == 1 )
      iKaraokeNumber = m_pDS->fv("MAX(iKaraNumber)").get_asInt() + 1;

    // Add the data
    strSQL=PrepareSQL( "INSERT INTO karaokedata (iKaraNumber, idSong, iKaraDelay, strKaraEncoding, strKaralyrics, strKaraLyrFileCRC) "
        "VALUES( %i, %i, 0, NULL, NULL, '%ul' )", iKaraokeNumber, idSong, crc );

    m_pDS->exec(strSQL.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s -(%s) failed", __FUNCTION__, song.strFileName.c_str());
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

    return GetSongById( idSong, song );
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

      file.Write( outdoc, outdoc.size() );
    }

    while (!m_pDS->eof())
    {
      CSong song = GetSongFromDataset( false );
      CStdString songnum;
      songnum.Format( "%06d", song.iKaraokeNumber );

      if ( asHTML )
        outdoc = "<tr><td>" + songnum + "</td><td>" + StringUtils::Join(song.artist, g_advancedSettings.m_musicItemSeparator) + "</td><td>" + song.strTitle + "</td></tr>\r\n";
      else
        outdoc = songnum + "\t" + StringUtils::Join(song.artist, g_advancedSettings.m_musicItemSeparator) + "\t" + song.strTitle + "\t" + song.strFileName + "\r\n";

      file.Write( outdoc, outdoc.size() );

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
      file.Write( outdoc, outdoc.size() );
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

    if ( !file.Open( inputFile ) )
    {
      CLog::Log( LOGERROR, "Cannot open karaoke import file %s", inputFile.c_str() );
      return;
    }

    unsigned int size = (unsigned int) file.GetLength();

    if ( !size )
      return;

    // Read the file into memory array
    std::vector<char> data( size + 1 );

    file.Seek( 0, SEEK_SET );

    // Read the whole file
    if ( file.Read( &data[0], size) != size )
    {
      CLog::Log( LOGERROR, "Cannot read karaoke import file %s", inputFile.c_str() );
      return;
    }

    file.Close();
    data[ size ] = '\0';

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
    char * linestart = &data[0];
    unsigned int offset = 0, lastpercentage = 0;

    for ( char * p = &data[0]; *p; p++, offset++ )
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
                     "where strArtist like '%s' and strTitle like '%s'", artist, title );

        if ( !m_pDS->query(strSQL.c_str()) )
        {
            RollbackTransaction();
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

        if ( progress && (offset * 100 / size) != lastpercentage )
        {
          lastpercentage = offset * 100 / size;
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
    if (GetArtistInfo(idArtist,artist))
      SetPropertiesFromArtist(item,artist);
  }
  int idAlbum = GetAlbumByName(item.GetMusicInfoTag()->GetAlbum(),
                               item.GetMusicInfoTag()->GetArtist());
  if (idAlbum > -1)
  {
    CAlbum album;
    if (GetAlbumInfo(idAlbum,album,NULL))
      SetPropertiesFromAlbum(item,album);
  }
}

void CMusicDatabase::AnnounceRemove(std::string content, int id)
{
  CVariant data;
  data["type"] = content;
  data["id"] = id;
  ANNOUNCEMENT::CAnnouncementManager::Announce(ANNOUNCEMENT::AudioLibrary, "xbmc", "OnRemove", data);
}

void CMusicDatabase::AnnounceUpdate(std::string content, int id)
{
  CVariant data;
  data["type"] = content;
  data["id"] = id;
  ANNOUNCEMENT::CAnnouncementManager::Announce(ANNOUNCEMENT::AudioLibrary, "xbmc", "OnUpdate", data);
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

    CStdString sql = PrepareSQL("SELECT type,url FROM art WHERE media_id=(SELECT idArtist from %s_artist WHERE id%s=%i) AND media_type='artist'", mediaType.c_str(), mediaType.c_str(), mediaId);
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
  std::string query = PrepareSQL("SELECT url FROM art WHERE media_id=(SELECT idArtist from %s_artist WHERE id%s=%i) AND media_type='artist' AND type='%s'", mediaType.c_str(), mediaType.c_str(), mediaId, artType.c_str());
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
        strSQL += " WHERE album_artist.boolFeatured = 0";            // then exclude those that have no extra artists
      strSQL +=   ")";
    }

    // remove the null string
    strSQL += ") and artistview.strArtist != ''";

    // and the various artist entry if applicable
    if (!albumArtistsOnly)
    {
      CStdString strVariousArtists = g_localizeStrings.Get(340);
      int idVariousArtists = AddArtist(strVariousArtists);
      strSQL += PrepareSQL(" and artistview.idArtist <> %i", idVariousArtists);
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
      filter.AppendWhere(PrepareSQL("albumview.idAlbum IN (SELECT song.idAlbum FROM song JOIN song_artist ON song.idSong = song_artist.idSong WHERE song_artist.idArtist = %i)" // All albums linked to this artist via songs
                                    " OR albumview.idAlbum IN (SELECT album_artist.idAlbum FROM album_artist WHERE album_artist.idArtist = %i)", // All albums where album artists fit
                                    (int)option->second.asInteger(), (int)option->second.asInteger()));
    else
    {
      option = options.find("artist");
      if (option != options.end())
        filter.AppendWhere(PrepareSQL("albumview.idAlbum IN (SELECT song.idAlbum FROM song JOIN song_artist ON song.idSong = song_artist.idSong JOIN artist ON artist.idArtist = song_artist.idArtist WHERE artist.strArtist like '%s')" // All albums linked to this artist via songs
                                      " OR albumview.idAlbum IN (SELECT album_artist.idAlbum FROM album_artist JOIN artist ON artist.idArtist = album_artist.idArtist WHERE artist.strArtist like '%s')", // All albums where album artists fit
                                      option->second.asString().c_str(), option->second.asString().c_str()));
      // no artist given, so exclude any single albums (aka empty tagged albums)
      else
        filter.AppendWhere("albumview.strAlbum <> ''");
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
    if (xsp.GetType() != "artists" || xsp.GetType()  == type)
    {
      std::set<CStdString> playlists;
      filter.AppendWhere(xsp.GetWhereClause(*this, playlists));

      if (xsp.GetLimit() > 0)
        sorting.limitEnd = xsp.GetLimit();
      if (xsp.GetOrder() != SortByNone)
        sorting.sortBy = xsp.GetOrder();
      sorting.sortOrder = xsp.GetOrderAscending() ? SortOrderAscending : SortOrderDescending;
      if (g_guiSettings.GetBool("filelists.ignorethewhensorting"))
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
      std::set<CStdString> playlists;
      filter.AppendWhere(xspFilter.GetWhereClause(*this, playlists));
    }
    // remove the filter if it doesn't match the item type
    else
      musicUrl.AddOption("filter", "");
  }

  return true;
}
