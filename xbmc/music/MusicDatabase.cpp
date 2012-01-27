/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
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
#include "music/dialogs/GUIDialogMusicScan.h"
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
    m_pDS->exec("CREATE TABLE album ( idAlbum integer primary key, strAlbum varchar(256), idArtist integer, strExtraArtists text, idGenre integer, strExtraGenres text, iYear integer, idThumb integer)\n");
    CLog::Log(LOGINFO, "create genre table");
    m_pDS->exec("CREATE TABLE genre ( idGenre integer primary key, strGenre varchar(256))\n");
    CLog::Log(LOGINFO, "create path table");
    m_pDS->exec("CREATE TABLE path ( idPath integer primary key, strPath varchar(512), strHash text)\n");
    CLog::Log(LOGINFO, "create song table");
    m_pDS->exec("CREATE TABLE song ( idSong integer primary key, idAlbum integer, idPath integer, idArtist integer, strExtraArtists text, idGenre integer, strExtraGenres text, strTitle varchar(512), iTrack integer, iDuration integer, iYear integer, dwFileNameCRC text, strFileName text, strMusicBrainzTrackID text, strMusicBrainzArtistID text, strMusicBrainzAlbumID text, strMusicBrainzAlbumArtistID text, strMusicBrainzTRMID text, iTimesPlayed integer, iStartOffset integer, iEndOffset integer, idThumb integer, lastplayed varchar(20) default NULL, rating char default '0', comment text)\n");
    CLog::Log(LOGINFO, "create albuminfo table");
    m_pDS->exec("CREATE TABLE albuminfo ( idAlbumInfo integer primary key, idAlbum integer, iYear integer, idGenre integer, strExtraGenres text, strMoods text, strStyles text, strThemes text, strReview text, strImage text, strLabel text, strType text, iRating integer)\n");
    CLog::Log(LOGINFO, "create albuminfosong table");
    m_pDS->exec("CREATE TABLE albuminfosong ( idAlbumInfoSong integer primary key, idAlbumInfo integer, iTrack integer, strTitle text, iDuration integer)\n");
    CLog::Log(LOGINFO, "create thumb table");
    m_pDS->exec("CREATE TABLE thumb (idThumb integer primary key, strThumb varchar(256))\n");
    CLog::Log(LOGINFO, "create artistnfo table");
    m_pDS->exec("CREATE TABLE artistinfo ( idArtistInfo integer primary key, idArtist integer, strBorn text, strFormed text, strGenres text, strMoods text, strStyles text, strInstruments text, strBiography text, strDied text, strDisbanded text, strYearsActive text, strImage text, strFanart text)\n");
    CLog::Log(LOGINFO, "create content table");
    m_pDS->exec("CREATE TABLE content (strPath text, strScraperPath text, strContent text, strSettings text)\n");
    CLog::Log(LOGINFO, "create discography table");
    m_pDS->exec("CREATE TABLE discography (idArtist integer, strAlbum text, strYear text)\n");

    CLog::Log(LOGINFO, "create exartistsong table");
    m_pDS->exec("CREATE TABLE exartistsong ( idSong integer, iPosition integer, idArtist integer)\n");
    CLog::Log(LOGINFO, "create extragenresong table");
    m_pDS->exec("CREATE TABLE exgenresong ( idSong integer, iPosition integer, idGenre integer)\n");
    CLog::Log(LOGINFO, "create exartistalbum table");
    m_pDS->exec("CREATE TABLE exartistalbum ( idAlbum integer, iPosition integer, idArtist integer)\n");
    CLog::Log(LOGINFO, "create exgenrealbum table");
    m_pDS->exec("CREATE TABLE exgenrealbum ( idAlbum integer, iPosition integer, idGenre integer)\n");

    CLog::Log(LOGINFO, "create karaokedata table");
    m_pDS->exec("CREATE TABLE karaokedata ( iKaraNumber integer, idSong integer, iKaraDelay integer, strKaraEncoding text, "
                "strKaralyrics text, strKaraLyrFileCRC text )\n");

    // Indexes
    CLog::Log(LOGINFO, "create exartistsong index");
    m_pDS->exec("CREATE INDEX idxExtraArtistSong ON exartistsong(idSong)");
    m_pDS->exec("CREATE INDEX idxExtraArtistSong2 ON exartistsong(idArtist)");
    CLog::Log(LOGINFO, "create exgenresong index");
    m_pDS->exec("CREATE INDEX idxExtraGenreSong ON exgenresong(idSong)");
    m_pDS->exec("CREATE INDEX idxExtraGenreSong2 ON exgenresong(idGenre)");
    CLog::Log(LOGINFO, "create exartistalbum index");
    m_pDS->exec("CREATE INDEX idxExtraArtistAlbum ON exartistalbum(idAlbum)");
    m_pDS->exec("CREATE INDEX idxExtraArtistAlbum2 ON exartistalbum(idArtist)");
    CLog::Log(LOGINFO, "create exgenrealbum index");
    m_pDS->exec("CREATE INDEX idxExtraGenreAlbum ON exgenrealbum(idAlbum)");
    m_pDS->exec("CREATE INDEX idxExtraGenreAlbum2 ON exgenrealbum(idGenre)");

    CLog::Log(LOGINFO, "create album index");
    m_pDS->exec("CREATE INDEX idxAlbum ON album(strAlbum)");
    CLog::Log(LOGINFO, "create album index2");
    m_pDS->exec("CREATE INDEX idxAlbum2 ON album(idArtist)");

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
    CLog::Log(LOGINFO, "create song index4");
    m_pDS->exec("CREATE INDEX idxSong4 ON song(idArtist)");
    CLog::Log(LOGINFO, "create song index5");
    m_pDS->exec("CREATE INDEX idxSong5 ON song(idGenre)");
    CLog::Log(LOGINFO, "create song index6");
    m_pDS->exec("CREATE INDEX idxSong6 ON song(idPath)");

    CLog::Log(LOGINFO, "create thumb index");
    m_pDS->exec("CREATE INDEX idxThumb ON thumb(strThumb)");
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
  m_pDS->exec("create view songview as select song.idSong as idSong, song.strExtraArtists as strExtraArtists, song.strExtraGenres as strExtraGenres, strTitle, iTrack, iDuration, song.iYear as iYear, dwFileNameCRC, strFileName, strMusicBrainzTrackID, strMusicBrainzArtistID, strMusicBrainzAlbumID, strMusicBrainzAlbumArtistID, strMusicBrainzTRMID, iTimesPlayed, iStartOffset, iEndOffset, lastplayed, rating, comment, song.idAlbum as idAlbum, strAlbum, strPath, song.idArtist as idArtist, strArtist, song.idGenre as idGenre, strGenre, strThumb, iKaraNumber, iKaraDelay, strKaraEncoding from song join album on song.idAlbum=album.idAlbum join path on song.idPath=path.idPath join  artist on song.idArtist=artist.idArtist join genre on song.idGenre=genre.idGenre join thumb on song.idThumb=thumb.idThumb left outer join karaokedata on song.idSong=karaokedata.idSong");

  CLog::Log(LOGINFO, "create album view");
  m_pDS->exec("DROP VIEW IF EXISTS albumview");
  m_pDS->exec("create view albumview as select album.idAlbum as idAlbum, strAlbum, strExtraArtists, "
              "album.idArtist as idArtist, album.strExtraGenres as strExtraGenres, album.idGenre as idGenre, "
              "strArtist, strGenre, album.iYear as iYear, strThumb, idAlbumInfo, strMoods, strStyles, strThemes, "
              "strReview, strLabel, strType, strImage, iRating from album "
              "left outer join artist on album.idArtist=artist.idArtist "
              "left outer join genre on album.idGenre=genre.idGenre "
              "left outer join thumb on album.idThumb=thumb.idThumb "
              "left outer join albuminfo on album.idAlbum=albuminfo.idAlbum");
}

void CMusicDatabase::AddSong(CSong& song, bool bCheck)
{
  CStdString strSQL;
  try
  {
    // We need at least the title
    if (song.strTitle.IsEmpty())
      return;

    CStdString strPath, strFileName;
    URIUtils::Split(song.strFileName, strPath, strFileName);

    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    // split our (possibly) multiple artist string into individual artists
    CStdStringArray vecArtists; CStdString extraArtists;
    SplitString(song.strArtist, vecArtists, extraArtists);

    // do the same with our albumartist
    CStdStringArray vecAlbumArtists; CStdString extraAlbumArtists;
    SplitString(song.strAlbumArtist, vecAlbumArtists, extraAlbumArtists);

    // and the same for our genres
    CStdStringArray vecGenres; CStdString extraGenres;
    SplitString(song.strGenre, vecGenres, extraGenres);

    // add the primary artist/genre
    // SplitString returns >= 1 so no worries referencing the first item here
    int idArtist = AddArtist(vecArtists[0]);
    int idGenre = AddGenre(vecGenres[0]);
    // and also the primary album artist (if applicable)
    int idAlbumArtist = -1;
    if (!vecAlbumArtists[0].IsEmpty())
      idAlbumArtist = AddArtist(vecAlbumArtists[0]);

    int idPath = AddPath(strPath);
    int idThumb = AddThumb(song.strThumb);
    int idAlbum;
    if (idAlbumArtist > -1)  // have an album artist
      idAlbum = AddAlbum(song.strAlbum, idAlbumArtist, extraAlbumArtists, song.strAlbumArtist, idThumb, idGenre, extraGenres, song.iYear);
    else
      idAlbum = AddAlbum(song.strAlbum, idArtist, extraArtists, song.strArtist, idThumb, idGenre, extraGenres, song.iYear);

    DWORD crc = ComputeCRC(song.strFileName);

    bool bInsert = true;
    int idSong = -1;
    bool bHasKaraoke = false;
#ifdef HAS_KARAOKE
    bHasKaraoke = CKaraokeLyricsFactory::HasLyrics( song.strFileName );
#endif

    // If this is karaoke song, change the genre to 'Karaoke' (and add it if it's not there)
    if ( bHasKaraoke && g_advancedSettings.m_karaokeChangeGenreForKaraokeSongs )
      idGenre = AddGenre( "Karaoke" );

    if (bCheck)
    {
      strSQL=PrepareSQL("select * from song where idAlbum=%i and dwFileNameCRC='%ul' and strTitle='%s'",
                    idAlbum, crc, song.strTitle.c_str());

      if (!m_pDS->query(strSQL.c_str()))
        return;

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

      strSQL=PrepareSQL("insert into song (idSong,idAlbum,idPath,idArtist,strExtraArtists,idGenre,strExtraGenres,strTitle,iTrack,iDuration,iYear,dwFileNameCRC,strFileName,strMusicBrainzTrackID,strMusicBrainzArtistID,strMusicBrainzAlbumID,strMusicBrainzAlbumArtistID,strMusicBrainzTRMID,iTimesPlayed,iStartOffset,iEndOffset,idThumb,lastplayed,rating,comment) values (NULL,%i,%i,%i,'%s',%i,'%s','%s',%i,%i,%i,'%ul','%s','%s','%s','%s','%s','%s'",
                    idAlbum, idPath, idArtist, extraArtists.c_str(), idGenre, extraGenres.c_str(),
                    song.strTitle.c_str(),
                    song.iTrack, song.iDuration, song.iYear,
                    crc, strFileName.c_str(),
                    song.strMusicBrainzTrackID.c_str(),
                    song.strMusicBrainzArtistID.c_str(),
                    song.strMusicBrainzAlbumID.c_str(),
                    song.strMusicBrainzAlbumArtistID.c_str(),
                    song.strMusicBrainzTRMID.c_str());

      if (song.lastPlayed.GetLength())
        strSQL1=PrepareSQL(",%i,%i,%i,%i,'%s','%c','%s')",
                      song.iTimesPlayed, song.iStartOffset, song.iEndOffset, idThumb, song.lastPlayed.c_str(), song.rating, song.strComment.c_str());
      else
        strSQL1=PrepareSQL(",%i,%i,%i,%i,NULL,'%c','%s')",
                      song.iTimesPlayed, song.iStartOffset, song.iEndOffset, idThumb, song.rating, song.strComment.c_str());
      strSQL+=strSQL1;

      m_pDS->exec(strSQL.c_str());
      idSong = (int)m_pDS->lastinsertid();
    }

    // add extra artists and genres
    AddExtraSongArtists(vecArtists, idSong, bCheck);
    if (idAlbumArtist > -1)
      AddExtraAlbumArtists(vecAlbumArtists, idAlbum);
    else
      AddExtraAlbumArtists(vecArtists, idAlbum);
    AddExtraGenres(vecGenres, idSong, idAlbum, bCheck);

    song.idSong = idSong;

    // Add karaoke information (if any)
    if ( bHasKaraoke )
      AddKaraokeData( song );

    AnnounceUpdate("song", idSong);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "musicdatabase:unable to addsong (%s)", strSQL.c_str());
  }
}

int CMusicDatabase::AddAlbum(const CStdString& strAlbum1, int idArtist, const CStdString &extraArtists, const CStdString &strArtist, int idThumb, int idGenre, const CStdString &extraGenres, int year)
{
  CStdString strSQL;
  try
  {
    CStdString strAlbum=strAlbum1;
    strAlbum.TrimLeft(" ");
    strAlbum.TrimRight(" ");

    if (strAlbum.IsEmpty())
    {
      // album tag is empty, so we treat this as a single, or a collection of singles,
      // so we don't specify a thumb
      idThumb = AddThumb("");
    }

    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    map <CStdString, CAlbumCache>::const_iterator it;

    it = m_albumCache.find(strAlbum + strArtist);
    if (it != m_albumCache.end())
      return it->second.idAlbum;

    strSQL=PrepareSQL("select * from album where idArtist=%i and strAlbum like '%s'", idArtist, strAlbum.c_str());
    m_pDS->query(strSQL.c_str());

    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      // doesnt exists, add it
      strSQL=PrepareSQL("insert into album (idAlbum, strAlbum, idArtist, strExtraArtists, idGenre, strExtraGenres, iYear, idThumb) values( NULL, '%s', %i, '%s', %i, '%s', %i, %i)", strAlbum.c_str(), idArtist, extraArtists.c_str(), idGenre, extraGenres.c_str(), year, idThumb);
      m_pDS->exec(strSQL.c_str());

      CAlbumCache album;
      album.idAlbum = (int)m_pDS->lastinsertid();
      album.strAlbum = strAlbum;
      album.idArtist = idArtist;
      album.strArtist = strArtist;
      m_albumCache.insert(pair<CStdString, CAlbumCache>(album.strAlbum + album.strArtist, album));
      return album.idAlbum;
    }
    else
    {
      // exists in our database and not scanned during this scan, so we should update it as the details
      // may have changed (there's a reason we're rescanning, afterall!)
      CAlbumCache album;
      album.idAlbum = m_pDS->fv("idAlbum").get_asInt();
      album.strAlbum = strAlbum;
      album.idArtist = idArtist;
      album.strArtist = strArtist;
      m_albumCache.insert(pair<CStdString, CAlbumCache>(album.strAlbum + album.strArtist, album));
      m_pDS->close();
      strSQL=PrepareSQL("update album set strExtraArtists='%s', idGenre=%i, strExtraGenres='%s', iYear=%i, idThumb=%i where idAlbum=%i", extraArtists.c_str(), idGenre, extraGenres.c_str(), year, idThumb, album.idAlbum);
      m_pDS->exec(strSQL.c_str());
      // and clear the exartistalbum and exgenrealbum tables - these are updated in AddSong()
      strSQL=PrepareSQL("delete from exartistalbum where idAlbum=%i", album.idAlbum);
      m_pDS->exec(strSQL.c_str());
      strSQL=PrepareSQL("delete from exgenrealbum where idAlbum=%i", album.idAlbum);
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
    strGenre.TrimLeft(" ");
    strGenre.TrimRight(" ");

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
    strArtist.TrimLeft(" ");
    strArtist.TrimRight(" ");

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

void CMusicDatabase::AddExtraSongArtists(const CStdStringArray &vecArtists, int idSong, bool bCheck)
{
  try
  {
    // add each of the artists in the vector of artists
    for (int i = 1; i < (int)vecArtists.size(); i++)
    {
      int idArtist = AddArtist(vecArtists[i]);
      if (idArtist >= 0)
      { // added successfully, we must now add entries to the exartistsong table
        CStdString strSQL;
        // first link the artist with the song
        bool bInsert = true;
        if (bCheck)
        {
          strSQL=PrepareSQL("select * from exartistsong where idSong=%i and idArtist=%i",
                        idSong, idArtist);
          if (!m_pDS->query(strSQL.c_str())) return ;
          if (m_pDS->num_rows() != 0)
            bInsert = false; // already exists
          m_pDS->close();
        }
        if (bInsert)
        {
          strSQL=PrepareSQL("insert into exartistsong (idSong,iPosition,idArtist) values(%i,%i,%i)",
                        idSong, i, idArtist);

          m_pDS->exec(strSQL.c_str());
        }
      }
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%i) failed", __FUNCTION__, idSong);
  }
}

void CMusicDatabase::AddExtraAlbumArtists(const CStdStringArray &vecArtists, int idAlbum)
{
  try
  {
    // add each of the artists in the vector of artists
    for (int i = 1; i < (int)vecArtists.size(); i++)
    {
      int idArtist = AddArtist(vecArtists[i]);
      if (idArtist >= 0)
      { // added successfully, we must now add entries to the exartistalbum table
        CStdString strSQL;
        bool bInsert = true;
        // always check artists (as this routine is called whenever a song is added)
        strSQL=PrepareSQL("select * from exartistalbum where idAlbum=%i and idArtist=%i",
                      idAlbum, idArtist);
        if (!m_pDS->query(strSQL.c_str())) return ;
        if (m_pDS->num_rows() != 0)
          bInsert = false; // already exists
        m_pDS->close();
        if (bInsert)
        {
          strSQL=PrepareSQL("insert into exartistalbum (idAlbum,iPosition,idArtist) values(%i,%i,%i)",
                        idAlbum, i, idArtist);

          m_pDS->exec(strSQL.c_str());
        }
      }
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%i) failed", __FUNCTION__, idAlbum);
  }
}

void CMusicDatabase::AddExtraGenres(const CStdStringArray &vecGenres, int idSong, int idAlbum, bool bCheck)
{
  try
  {
    // add each of the genres in the vector
    for (int i = 1; i < (int)vecGenres.size(); i++)
    {
      int idGenre = AddGenre(vecGenres[i]);
      if (idGenre >= 0)
      { // added successfully!
        CStdString strSQL;
        // first link the genre with the song
        bool bInsert = true;
        if (idSong)
        {
          if (bCheck)
          {
            strSQL=PrepareSQL("select * from exgenresong where idSong=%i and idGenre=%i",
                          idSong, idGenre);
            if (!m_pDS->query(strSQL.c_str())) return ;
            if (m_pDS->num_rows() != 0)
              bInsert = false; // already exists
            m_pDS->close();
          }
          if (bInsert)
          {
            strSQL=PrepareSQL("insert into exgenresong (idSong,iPosition,idGenre) values(%i,%i,%i)",
                          idSong, i, idGenre);

            m_pDS->exec(strSQL.c_str());
          }
        }
        // now link the genre with the album - we always check these as there's usually
        // more than one song per album with the same extra genres
        if (idAlbum)
        {
          strSQL=PrepareSQL("select * from exgenrealbum where idAlbum=%i and idGenre=%i",
                        idAlbum, idGenre);
          if (!m_pDS->query(strSQL.c_str())) return ;
          if (m_pDS->num_rows() == 0)
          { // insert
            m_pDS->close();
            strSQL=PrepareSQL("insert into exgenrealbum (idAlbum,iPosition,idGenre) values(%i,%i,%i)",
                          idAlbum, i, idGenre);

            m_pDS->exec(strSQL.c_str());
          }
        }
      }
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%i,%i) failed", __FUNCTION__, idSong, idAlbum);
  }
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

    strSQL=PrepareSQL( "select * from path where strPath like '%s'", strPath.c_str());
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
  song.strArtist = m_pDS->fv(song_strArtist).get_asString();
  song.strArtist += m_pDS->fv(song_strExtraArtists).get_asString();
  song.iArtistId = m_pDS->fv(song_idArtist).get_asInt();
  // and the full genre string
  song.strGenre = m_pDS->fv(song_strGenre).get_asString();
  song.strGenre += m_pDS->fv(song_strExtraGenres).get_asString();
  // and the rest...
  song.strAlbum = m_pDS->fv(song_strAlbum).get_asString();
  song.iAlbumId = m_pDS->fv(song_idAlbum).get_asInt();
  song.iTrack = m_pDS->fv(song_iTrack).get_asInt() ;
  song.iDuration = m_pDS->fv(song_iDuration).get_asInt() ;
  song.iYear = m_pDS->fv(song_iYear).get_asInt() ;
  song.strTitle = m_pDS->fv(song_strTitle).get_asString();
  song.iTimesPlayed = m_pDS->fv(song_iTimesPlayed).get_asInt();
  song.lastPlayed = m_pDS->fv(song_lastplayed).get_asString();
  song.iStartOffset = m_pDS->fv(song_iStartOffset).get_asInt();
  song.iEndOffset = m_pDS->fv(song_iEndOffset).get_asInt();
  song.strMusicBrainzTrackID = m_pDS->fv(song_strMusicBrainzTrackID).get_asString();
  song.strMusicBrainzArtistID = m_pDS->fv(song_strMusicBrainzArtistID).get_asString();
  song.strMusicBrainzAlbumID = m_pDS->fv(song_strMusicBrainzAlbumID).get_asString();
  song.strMusicBrainzAlbumArtistID = m_pDS->fv(song_strMusicBrainzAlbumArtistID).get_asString();
  song.strMusicBrainzTRMID = m_pDS->fv(song_strMusicBrainzTRMID).get_asString();
  song.rating = m_pDS->fv(song_rating).get_asChar();
  song.strComment = m_pDS->fv(song_comment).get_asString();
  song.strThumb = m_pDS->fv(song_strThumb).get_asString();
  song.iKaraokeNumber = m_pDS->fv(song_iKarNumber).get_asInt();
  song.strKaraokeLyrEncoding = m_pDS->fv(song_strKarEncoding).get_asString();
  song.iKaraokeDelay = m_pDS->fv(song_iKarDelay).get_asInt();

  if (song.strThumb == "NONE")
    song.strThumb.Empty();
  // Get filename with full path
  if (!bWithMusicDbPath)
    URIUtils::AddFileToFolder(m_pDS->fv(song_strPath).get_asString(), m_pDS->fv(song_strFileName).get_asString(), song.strFileName);
  else
  {
    CStdString strFileName=m_pDS->fv(song_strFileName).get_asString();
    CStdString strExt=URIUtils::GetExtension(strFileName);
    song.strFileName.Format("musicdb://3/%ld/%ld%s", m_pDS->fv(song_idAlbum).get_asInt(), m_pDS->fv(song_idSong).get_asInt(), strExt.c_str());
  }

  return song;
}

void CMusicDatabase::GetFileItemFromDataset(CFileItem* item, const CStdString& strMusicDBbasePath)
{
  // get the full artist string
  CStdString strArtist=m_pDS->fv(song_strArtist).get_asString();
  strArtist += m_pDS->fv(song_strExtraArtists).get_asString();
  item->GetMusicInfoTag()->SetArtist(strArtist);
  item->GetMusicInfoTag()->SetArtistId(m_pDS->fv(song_idArtist).get_asInt());
  // and the full genre string
  CStdString strGenre = m_pDS->fv(song_strGenre).get_asString();
  strGenre += m_pDS->fv(song_strExtraGenres).get_asString();
  item->GetMusicInfoTag()->SetGenre(strGenre);
  // and the rest...
  item->GetMusicInfoTag()->SetAlbum(m_pDS->fv(song_strAlbum).get_asString());
  item->GetMusicInfoTag()->SetAlbumId(m_pDS->fv(song_idAlbum).get_asInt());
  item->GetMusicInfoTag()->SetTrackAndDiskNumber(m_pDS->fv(song_iTrack).get_asInt());
  item->GetMusicInfoTag()->SetDuration(m_pDS->fv(song_iDuration).get_asInt());
  item->GetMusicInfoTag()->SetDatabaseId(m_pDS->fv(song_idSong).get_asInt());
  SYSTEMTIME stTime;
  stTime.wYear = (WORD)m_pDS->fv(song_iYear).get_asInt();
  item->GetMusicInfoTag()->SetReleaseDate(stTime);
  item->GetMusicInfoTag()->SetTitle(m_pDS->fv(song_strTitle).get_asString());
  item->SetLabel(m_pDS->fv(song_strTitle).get_asString());
  item->m_lStartOffset = m_pDS->fv(song_iStartOffset).get_asInt();
  item->m_lEndOffset = m_pDS->fv(song_iEndOffset).get_asInt();
  item->GetMusicInfoTag()->SetMusicBrainzTrackID(m_pDS->fv(song_strMusicBrainzTrackID).get_asString());
  item->GetMusicInfoTag()->SetMusicBrainzArtistID(m_pDS->fv(song_strMusicBrainzArtistID).get_asString());
  item->GetMusicInfoTag()->SetMusicBrainzAlbumID(m_pDS->fv(song_strMusicBrainzAlbumID).get_asString());
  item->GetMusicInfoTag()->SetMusicBrainzAlbumArtistID(m_pDS->fv(song_strMusicBrainzAlbumArtistID).get_asString());
  item->GetMusicInfoTag()->SetMusicBrainzTRMID(m_pDS->fv(song_strMusicBrainzTRMID).get_asString());
  item->GetMusicInfoTag()->SetRating(m_pDS->fv(song_rating).get_asChar());
  item->GetMusicInfoTag()->SetComment(m_pDS->fv(song_comment).get_asString());
  item->GetMusicInfoTag()->SetPlayCount(m_pDS->fv(song_iTimesPlayed).get_asInt());
  item->GetMusicInfoTag()->SetLastPlayed(m_pDS->fv(song_lastplayed).get_asString());
  CStdString strRealPath;
  URIUtils::AddFileToFolder(m_pDS->fv(song_strPath).get_asString(), m_pDS->fv(song_strFileName).get_asString(), strRealPath);
  item->GetMusicInfoTag()->SetURL(strRealPath);
  item->GetMusicInfoTag()->SetLoaded(true);
  CStdString strThumb=m_pDS->fv(song_strThumb).get_asString();
  if (strThumb != "NONE")
    item->SetThumbnailImage(strThumb);
  // Get filename with full path
  if (strMusicDBbasePath.IsEmpty())
  {
    item->SetPath(strRealPath);
  }
  else
  {
    CStdString strFileName=m_pDS->fv(song_strFileName).get_asString();
    CStdString strExt=URIUtils::GetExtension(strFileName);
    CStdString path; path.Format("%s%ld%s", strMusicDBbasePath.c_str(), m_pDS->fv(song_idSong).get_asInt(), strExt.c_str());
    item->SetPath(path);
  }
}

CAlbum CMusicDatabase::GetAlbumFromDataset(dbiplus::Dataset* pDS, bool imageURL /* = false*/)
{
  CAlbum album;
  album.idAlbum = pDS->fv(album_idAlbum).get_asInt();
  album.strAlbum = pDS->fv(album_strAlbum).get_asString();
  if (album.strAlbum.IsEmpty())
    album.strAlbum = g_localizeStrings.Get(1050);
  album.strArtist = pDS->fv(album_strArtist).get_asString();
  album.strArtist += pDS->fv(album_strExtraArtists).get_asString();
  album.idArtist = pDS->fv(album_idArtist).get_asInt();
  album.strGenre = pDS->fv(album_strGenre).get_asString();
  album.strGenre += pDS->fv(album_strExtraGenres).get_asString();
  album.iYear = pDS->fv(album_iYear).get_asInt();
  if (imageURL)
    album.thumbURL.ParseString(pDS->fv(album_strThumbURL).get_asString());
  else
  {
    CStdString strThumb = pDS->fv(album_strThumb).get_asString();
    if (strThumb != "NONE")
      album.thumbURL.ParseString(strThumb);
  }
  album.iRating = pDS->fv(album_iRating).get_asInt();
  album.iYear = pDS->fv(album_iYear).get_asInt();
  album.strReview = pDS->fv(album_strReview).get_asString();
  album.strStyles = pDS->fv(album_strStyles).get_asString();
  album.strMoods = pDS->fv(album_strMoods).get_asString();
  album.strThemes = pDS->fv(album_strThemes).get_asString();
  album.strLabel = pDS->fv(album_strLabel).get_asString();
  album.strType = pDS->fv(album_strType).get_asString();
  return album;
}

CArtist CMusicDatabase::GetArtistFromDataset(dbiplus::Dataset* pDS, bool needThumb)
{
  CArtist artist;
  artist.idArtist = pDS->fv(artist_idArtist).get_asInt();
  artist.strArtist = pDS->fv("artist.strArtist").get_asString();
  artist.strGenre = pDS->fv(artist_strGenres).get_asString();
  artist.strBiography = pDS->fv(artist_strBiography).get_asString();
  artist.strStyles = pDS->fv(artist_strStyles).get_asString();
  artist.strMoods = pDS->fv(artist_strMoods).get_asString();
  artist.strBorn = pDS->fv(artist_strBorn).get_asString();
  artist.strFormed = pDS->fv(artist_strFormed).get_asString();
  artist.strDied = pDS->fv(artist_strDied).get_asString();
  artist.strDisbanded = pDS->fv(artist_strDisbanded).get_asString();
  artist.strYearsActive = pDS->fv(artist_strYearsActive).get_asString();
  artist.strInstruments = pDS->fv(artist_strInstruments).get_asString();

  if (needThumb)
  {
    artist.fanart.m_xml = pDS->fv(artist_strFanart).get_asString();
    artist.fanart.Unpack();
    artist.thumbURL.ParseString(pDS->fv(artist_strImage).get_asString());
  }

  return artist;
}

bool CMusicDatabase::GetSongByFileName(const CStdString& strFileName, CSong& song)
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
                                "where dwFileNameCRC='%ul' and strPath like'%s'"
                                , crc,
                                strPath.c_str());

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
    CStdString strSQL=PrepareSQL("select distinct idAlbum from song join path on song.idPath = path.idPath where path.strPath like '%s'", strPath.c_str());
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
      pItem->SetCachedArtistThumb();
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

bool CMusicDatabase::GetArbitraryQuery(const CStdString& strQuery, const CStdString& strOpenRecordSet, const CStdString& strCloseRecordSet,
                                       const CStdString& strOpenRecord, const CStdString& strCloseRecord, const CStdString& strOpenField,
                                       const CStdString& strCloseField, CStdString& strResult)
{
  try
  {
    strResult = "";
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    CStdString strSQL=strQuery;
    if (!m_pDS->query(strSQL.c_str()))
    {
      strResult = m_pDB->getErrorMsg();
      return false;
    }
    strResult=strOpenRecordSet;
    while (!m_pDS->eof())
    {
      strResult += strOpenRecord;
      for (int i=0; i<m_pDS->fieldCount(); i++)
      {
        strResult += strOpenField;
        strResult += m_pDS->fv(i).get_asString();
        strResult += strCloseField;
      }
      strResult += strCloseRecord;
      m_pDS->next();
    }
    strResult += strCloseRecordSet;
    m_pDS->close();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%s) failed", __FUNCTION__, strQuery.c_str());
  }
  try
  {
    if (NULL == m_pDB.get()) return false;
      strResult = m_pDB->getErrorMsg();
  }
  catch (...)
  {

  }

  return false;
}

bool CMusicDatabase::ArbitraryExec(const CStdString& strExec)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    CStdString strSQL = strExec;
    m_pDS->exec(strSQL.c_str());
    m_pDS->close();
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

    CStdString strSQL=PrepareSQL("select * from artistinfo "
                                "join artist on artist.idArtist=artistinfo.idArtist "
                                "where artistinfo.idArtist = %i"
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
      return false;
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
    CStdString strSQL = "select albumview.*, sum(song.iTimesPlayed) as total, song.idAlbum from song "
                    "join albumview on albumview.idAlbum=song.idAlbum "
                    "where song.iTimesPlayed>0 and albumview.strAlbum != '' "
                    "group by song.idAlbum "
                    "order by total desc "
                    "limit 100 ";

    CLog::Log(LOGDEBUG, "%s query: %s", __FUNCTION__, strSQL.c_str());
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return false;
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
      return false;
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
      return false;
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
      return false;
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
      return false;
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
      return false;
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

bool CMusicDatabase::IncrTop100CounterByFileName(const CStdString& strFileName)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    int idSong = GetSongIDFromPath(strFileName);

    CStdString sql=PrepareSQL("UPDATE song SET iTimesPlayed=iTimesPlayed+1, lastplayed=CURRENT_TIMESTAMP where idSong=%i", idSong);
    m_pDS->exec(sql.c_str());
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%s) failed", __FUNCTION__, strFileName.c_str());
  }

  return false;
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

    CStdString strSQL=PrepareSQL("select * from songview where strPath like '%s'", strPath.c_str() );
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

    // and also the multiple genre string into single genres.
    CStdStringArray vecGenres; CStdString extraGenres;
    SplitString(album.strGenre, vecGenres, extraGenres);
    int idGenre = AddGenre(vecGenres[0]);

    // delete any album info we may have
    strSQL=PrepareSQL("delete from albuminfo where idAlbum=%i", idAlbum);
    m_pDS->exec(strSQL.c_str());

    // insert the albuminfo
    strSQL=PrepareSQL("insert into albuminfo (idAlbumInfo,idAlbum,idGenre,strExtraGenres,strMoods,strStyles,strThemes,strReview,strImage,strLabel,strType,iRating,iYear) values(NULL,%i,%i,'%s','%s','%s','%s','%s','%s','%s','%s',%i,%i)",
                  idAlbum, idGenre, extraGenres.c_str(),
                  album.strMoods.c_str(),
                  album.strStyles.c_str(),
                  album.strThemes.c_str(),
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
                  artist.strGenre.c_str(),
                  artist.strMoods.c_str(),
                  artist.strStyles.c_str(),
                  artist.strInstruments.c_str(),
                  artist.strBiography.c_str(),
                  artist.strDied.c_str(),
                  artist.strDisbanded.c_str(),
                  artist.strYearsActive.c_str(),
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
      // ok, now delete these songs + all references to them from the exartistsong and exgenresong tables
      strSQL = "delete from song where idSong in " + strSongsToDelete;
      m_pDS->exec(strSQL.c_str());
      strSQL = "delete from exartistsong where idSong in " + strSongsToDelete;
      m_pDS->exec(strSQL.c_str());
      strSQL = "delete from exgenresong where idSong in " + strSongsToDelete;
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
    // ok, now we can delete them and the references in the exartistalbum, exgenrealbum and albuminfo tables
    strSQL = "delete from album where idAlbum in " + strAlbumIds;
    m_pDS->exec(strSQL.c_str());
    strSQL = "delete from albuminfo where idAlbum in " + strAlbumIds;
    m_pDS->exec(strSQL.c_str());
    strSQL = "delete from exartistalbum where idAlbum in " + strAlbumIds;
    m_pDS->exec(strSQL.c_str());
    strSQL = "delete from exgenrealbum where idAlbum in " + strAlbumIds;
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
      CStdString sql = PrepareSQL("select count(idPath) from songpaths where strPath like '%s%%'", m_pDS->fv("strPath").get_asString().c_str());
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

bool CMusicDatabase::CleanupThumbs()
{
  try
  {
    // needs to be done AFTER the songs have been cleaned up.
    // we can happily delete any thumb that has no reference to a song
    CStdString strSQL = "select * from thumb where idThumb not in (select idThumb from song) and idThumb not in (select idThumb from album)";
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return true;
    }
    // get albums dir
    CStdString strThumbsDir = g_settings.GetMusicThumbFolder();
    while (!m_pDS->eof())
    {
      CStdString strThumb = m_pDS->fv("strThumb").get_asString();
      if (strThumb.Left(strThumbsDir.size()) == strThumbsDir)
      { // only delete cached thumbs
        CTextureCache::Get().ClearCachedImage(strThumb, true);
      }
      m_pDS->next();
    }
    // clear the thumb cache
    //URIUtils::ThumbCacheClear();
    //g_directoryCache.ClearMusicThumbCache();
    // now we can delete
    m_pDS->close();
    strSQL = "delete from thumb where idThumb not in (select idThumb from song) and idThumb not in (select idThumb from album)";
    m_pDS->exec(strSQL.c_str());
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "Exception in CMusicDatabase::CleanupThumbs() or was aborted");
  }
  return false;
}

bool CMusicDatabase::CleanupArtists()
{
  try
  {
    // (nested queries by Bobbin007)
    // must be executed AFTER the song, exartistsong, album and exartistalbum tables are cleaned.
    // don't delete the "Various Artists" string
    CStdString strVariousArtists = g_localizeStrings.Get(340);
    int idVariousArtists = AddArtist(strVariousArtists);
    CStdString strSQL = "delete from artist where idArtist not in (select idArtist from song)";
    strSQL += " and idArtist not in (select idArtist from exartistsong)";
    strSQL += " and idArtist not in (select idArtist from album)";
    strSQL += " and idArtist not in (select idArtist from exartistalbum)";
    CStdString strSQL2;
    strSQL2.Format(" and idArtist<>%i", idVariousArtists);
    strSQL += strSQL2;
    m_pDS->exec(strSQL.c_str());
    m_pDS->exec("delete from artistinfo where idArtist not in (select idArtist from artist)");
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
    // Must be executed AFTER the song, exgenresong, albuminfo and exgenrealbum tables have been cleaned.
    CStdString strSQL = "delete from genre where idGenre not in (select idGenre from song) and";
    strSQL += " idGenre not in (select idGenre from exgenresong) and";
    strSQL += " idGenre not in (select idGenre from albuminfo) and";
    strSQL += " idGenre not in (select idGenre from exgenrealbum)";
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
  if (!CleanupThumbs()) return false;
  return true;
}

int CMusicDatabase::Cleanup(CGUIDialogProgress *pDlgProgress)
{
  if (NULL == m_pDB.get()) return ERROR_DATABASE;
  if (NULL == m_pDS.get()) return ERROR_DATABASE;

  unsigned int time = XbmcThreads::SystemClockMillis();
  CLog::Log(LOGNOTICE, "%s: Starting musicdatabase cleanup ..", __FUNCTION__);

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
    RollbackTransaction();
    return ERROR_REORG_SONGS;
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
    RollbackTransaction();
    return ERROR_REORG_ALBUM;
  }
  // now the paths
  if (pDlgProgress)
  {
    pDlgProgress->SetLine(1, 324);
    pDlgProgress->SetPercentage(40);
    pDlgProgress->Progress();
  }
  if (!CleanupPaths() || !CleanupThumbs())
  {
    RollbackTransaction();
    return ERROR_REORG_PATH;
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
    RollbackTransaction();
    return ERROR_REORG_ARTIST;
  }
  if (pDlgProgress)
  {
    pDlgProgress->SetLine(1, 322);
    pDlgProgress->SetPercentage(80);
    pDlgProgress->Progress();
  }
  if (!CleanupGenres())
  {
    RollbackTransaction();
    return ERROR_REORG_GENRE;
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
    RollbackTransaction();
    return ERROR_WRITING_CHANGES;
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

  if (!Compress(false))
  {
    return ERROR_COMPRESSING;
  }
  return ERROR_OK;
}

void CMusicDatabase::DeleteAlbumInfo()
{
  // open our database
  Open();
  if (NULL == m_pDB.get()) return ;
  if (NULL == m_pDS.get()) return ;

  // If we are scanning for music info in the background,
  // other writing access to the database is prohibited.
  CGUIDialogMusicScan* dlgMusicScan = (CGUIDialogMusicScan*)g_windowManager.GetWindow(WINDOW_DIALOG_MUSIC_SCAN);
  if (dlgMusicScan->IsDialogRunning())
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
  vector<CAlbumCache> vecAlbums;
  while (!m_pDS->eof())
  {
    CAlbumCache album;
    album.idAlbum = m_pDS->fv("album.idAlbum").get_asInt() ;
    album.strAlbum = m_pDS->fv("album.strAlbum").get_asString();
    album.strArtist = m_pDS->fv("artist.strArtist").get_asString();
    album.strArtist += m_pDS->fv("album.strExtraArtists").get_asString();
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
      CMusicDatabase::CAlbumCache& album = vecAlbums[i];
      pDlg->Add(album.strAlbum + " - " + album.strArtist);
    }
    pDlg->DoModal();

    // and wait till user selects one
    int iSelectedAlbum = pDlg->GetSelectedLabel();
    if (iSelectedAlbum < 0)
    {
      vecAlbums.erase(vecAlbums.begin(), vecAlbums.end());
      return ;
    }

    CAlbumCache& album = vecAlbums[iSelectedAlbum];
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
  if (!CDirectory::GetDirectory(g_settings.GetCDDBFolder(), items, ".cddb", false))
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
  CGUIDialogMusicScan* dlgMusicScan = (CGUIDialogMusicScan*)g_windowManager.GetWindow(WINDOW_DIALOG_MUSIC_SCAN);
  if (dlgMusicScan->IsDialogRunning())
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

bool CMusicDatabase::GetGenresNav(const CStdString& strBaseDir, CFileItemList& items)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    // get primary genres for songs
    CStdString strSQL="select * from genre "
                      "where (idGenre IN ("
                        "select song.idGenre from song) "
                      "or idGenre IN ("
                        "select exgenresong.idGenre from exgenresong)) ";

    // block null strings
    strSQL += " and genre.strGenre != \"\"";

    // run query
    CLog::Log(LOGDEBUG, "%s query: %s", __FUNCTION__, strSQL.c_str());
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return false;
    }

    // get data from returned rows
    while (!m_pDS->eof())
    {
      CFileItemPtr pItem(new CFileItem(m_pDS->fv("strGenre").get_asString()));
      pItem->GetMusicInfoTag()->SetGenre(m_pDS->fv("strGenre").get_asString());
      pItem->GetMusicInfoTag()->SetDatabaseId(m_pDS->fv("idGenre").get_asInt());
      CStdString strDir;
      strDir.Format("%ld/", m_pDS->fv("idGenre").get_asInt());
      pItem->SetPath(strBaseDir + strDir);
      pItem->m_bIsFolder=true;
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

    // get years from album list
    CStdString strSQL="select distinct iYear from album where iYear <> 0";

    // run query
    CLog::Log(LOGDEBUG, "%s query: %s", __FUNCTION__, strSQL.c_str());
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return false;
    }

    // get data from returned rows
    while (!m_pDS->eof())
    {
      CFileItemPtr pItem(new CFileItem(m_pDS->fv("iYear").get_asString()));
      SYSTEMTIME stTime;
      stTime.wYear = (WORD)m_pDS->fv("iYear").get_asInt();
      pItem->GetMusicInfoTag()->SetReleaseDate(stTime);
      CStdString strDir;
      strDir.Format("%ld/", m_pDS->fv("iYear").get_asInt());
      pItem->SetPath(strBaseDir + strDir);
      pItem->m_bIsFolder=true;
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
  CStdString where = PrepareSQL("where iYear=%ld", year);

  return GetAlbumsByWhere(strBaseDir, where, "", items);
}

bool CMusicDatabase::GetArtistsNav(const CStdString& strBaseDir, CFileItemList& items, int idGenre, bool albumArtistsOnly)
{
  if (NULL == m_pDB.get()) return false;
  if (NULL == m_pDS.get()) return false;
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    unsigned int time = XbmcThreads::SystemClockMillis();

    CStdString strSQL = "select * from artist where (idArtist IN ";

    if (idGenre==-1)
    {
      if (!albumArtistsOnly)  // show all artists in this case (ie those linked to a song)
        strSQL +=         "("
                          "select song.idArtist from song" // All primary artists linked to a song
                          ") "
                        "or idArtist IN "
                          "("
                          "select exartistsong.idArtist from exartistsong" // All extra artists linked to a song
                          ") "
                        "or idArtist IN ";

      // and always show any artists linked to an album (may be different from above due to album artist tag)
      strSQL +=          "("
                          "select album.idArtist from album" // All primary artists linked to an album
                          ") "
                        "or idArtist IN "
                          "("
                          "select exartistalbum.idArtist from exartistalbum "; // All extra artists linked to an album
      if (albumArtistsOnly)
        strSQL +=         "join album on album.idAlbum = exartistalbum.idAlbum " // if we're hiding compilation artists,
                          "where album.strExtraArtists != ''";                   // then exclude those that have no extra artists
      strSQL +=           ")"
                        ") ";
    }
    else
    { // same statements as above, but limit to the specified genre
      // in this case we show the whole lot always - there is no limitation to just album artists
      if (!albumArtistsOnly)  // show all artists in this case (ie those linked to a song)
        strSQL+=PrepareSQL("("
                          "select song.idArtist from song " // All primary artists linked to primary genres
                          "where song.idGenre=%i"
                          ") "
                        "or idArtist IN "
                          "("
                          "select song.idArtist from song " // All primary artists linked to extra genres
                            "join exgenresong on song.idSong=exgenresong.idSong "
                          "where exgenresong.idGenre=%i"
                          ")"
                        "or idArtist IN "
                          "("
                          "select exartistsong.idArtist from exartistsong " // All extra artists linked to extra genres
                            "join song on exartistsong.idSong=song.idSong "
                            "join exgenresong on song.idSong=exgenresong.idSong "
                          "where exgenresong.idGenre=%i"
                          ") "
                        "or idArtist IN "
                          "("
                          "select exartistsong.idArtist from exartistsong " // All extra artists linked to primary genres
                            "join song on exartistsong.idSong=song.idSong "
                          "where song.idGenre=%i"
                          ") "
                        "or idArtist IN "
                        , idGenre, idGenre, idGenre, idGenre);
      // and add any artists linked to an album (may be different from above due to album artist tag)
      strSQL += PrepareSQL("("
                          "select album.idArtist from album " // All primary album artists linked to primary genres
                          "where album.idGenre=%i"
                          ") "
                        "or idArtist IN "
                          "("
                          "select album.idArtist from album " // All primary album artists linked to extra genres
                            "join exgenrealbum on album.idAlbum=exgenrealbum.idAlbum "
                          "where exgenrealbum.idGenre=%i"
                          ")"
                        "or idArtist IN "
                          "("
                          "select exartistalbum.idArtist from exartistalbum " // All extra album artists linked to extra genres
                            "join album on exartistalbum.idAlbum=album.idAlbum "
                            "join exgenrealbum on album.idAlbum=exgenrealbum.idAlbum "
                          "where exgenrealbum.idGenre=%i"
                          ") "
                        "or idArtist IN "
                          "("
                          "select exartistalbum.idArtist from exartistalbum " // All extra album artists linked to primary genres
                            "join album on exartistalbum.idAlbum=album.idAlbum "
                          "where album.idGenre=%i"
                          ") "
                        ")", idGenre, idGenre, idGenre, idGenre);
    }

    // remove the null string
    strSQL += " and artist.strArtist != \"\"";
    // and the various artist entry if applicable
    if (!albumArtistsOnly)
    {
      CStdString strVariousArtists = g_localizeStrings.Get(340);
      int idVariousArtists = AddArtist(strVariousArtists);
      strSQL+=PrepareSQL(" and artist.idArtist<>%i", idVariousArtists);
    }

    // run query
    CLog::Log(LOGDEBUG, "%s query: %s", __FUNCTION__, strSQL.c_str());
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return false;
    }

    items.Reserve(iRowsFound);

    // get data from returned rows
    while (!m_pDS->eof())
    {
      CStdString strArtist = m_pDS->fv("strArtist").get_asString();
      CFileItemPtr pItem(new CFileItem(strArtist));
      pItem->GetMusicInfoTag()->SetArtist(strArtist);
      CStdString strDir;
      int idArtist = m_pDS->fv("idArtist").get_asInt();
      strDir.Format("%ld/", idArtist);
      pItem->SetPath(strBaseDir + strDir);
      pItem->m_bIsFolder=true;
      pItem->GetMusicInfoTag()->SetDatabaseId(idArtist);
      if (CFile::Exists(pItem->GetCachedArtistThumb()))
        pItem->SetThumbnailImage(pItem->GetCachedArtistThumb());
      pItem->SetIconImage("DefaultArtist.png");
      CArtist artist;
      GetArtistInfo(idArtist,artist,false);

      SetPropertiesFromArtist(*pItem,artist);
      items.Add(pItem);

      m_pDS->next();
    }
    CLog::Log(LOGDEBUG,"Time to retrieve artists from dataset = %i", XbmcThreads::SystemClockMillis() - time);

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

    CStdString strSQL = PrepareSQL("select albumview.* from song join albumview on song.idAlbum = albumview.idAlbum join path on song.idPath = path.idPath where song.strFileName like '%s' and path.strPath like '%s'", file.c_str(), path.c_str());
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

bool CMusicDatabase::GetAlbumsNav(const CStdString& strBaseDir, CFileItemList& items, int idGenre, int idArtist, int start, int end)
{
  //Create limit
  CStdString limit;
  if (start >= 0 && end >= 0)
  {
    limit.Format(" limit %i,%i", start, end);
  }

  // where clause
  CStdString strWhere;
  if (idGenre!=-1)
  {
    strWhere+=PrepareSQL("where (idAlbum IN "
                          "("
                          "select song.idAlbum from song " // All albums where the primary genre fits
                          "where song.idGenre=%i"
                          ") "
                        "or idAlbum IN "
                          "("
                          "select song.idAlbum from song " // All albums where extra genres fits
                            "join exgenresong on song.idSong=exgenresong.idSong "
                          "where exgenresong.idGenre=%i"
                          ")"
                        ") " + limit
                        , idGenre, idGenre);
  }

  if (idArtist!=-1)
  {
    if (strWhere.IsEmpty())
      strWhere += "where ";
    else
      strWhere += "and ";

    strWhere +=PrepareSQL("(idAlbum IN "
                            "("
                              "select song.idAlbum from song "  // All albums where the primary artist fits
                              "where song.idArtist=%i"
                            ")"
                          " or idAlbum IN "
                            "("
                              "select song.idAlbum from song "  // All albums where extra artists fit
                                "join exartistsong on song.idSong=exartistsong.idSong "
                              "where exartistsong.idArtist=%i"
                            ")"
                          " or idAlbum IN "
                            "("
                              "select album.idAlbum from album " // All albums where primary album artist fits
                              "where album.idArtist=%i"
                            ")"
                          " or idAlbum IN "
                            "("
                              "select exartistalbum.idAlbum from exartistalbum " // All albums where extra album artists fit
                              "where exartistalbum.idArtist=%i"
                            ")"
                          ") " + limit
                          , idArtist, idArtist, idArtist, idArtist);
  }
  else
  { // no artist given, so exclude any single albums (aka empty tagged albums)
    if (strWhere.IsEmpty())
      strWhere += "where albumview.strAlbum <> ''" + limit;
    else
      strWhere += "and albumview.strAlbum <> ''" + limit;
  }

  bool bResult = GetAlbumsByWhere(strBaseDir, strWhere, "", items);
  if (bResult && idArtist != -1)
  {
    CStdString strArtist = GetArtistById(idArtist);
    CStdString strFanart = items.GetCachedThumb(strArtist,g_settings.GetMusicFanartFolder());
    if (CFile::Exists(strFanart))
      items.SetProperty("fanart_image",strFanart);
  }

  return bResult;
}

bool CMusicDatabase::GetAlbumsByWhere(const CStdString &baseDir, const CStdString &where, const CStdString &order, CFileItemList &items)
{
  if (m_pDB.get() == NULL || m_pDS.get() == NULL)
    return false;

  try
  {
    CStdString sql = "select * from albumview " + where + order;

    CLog::Log(LOGDEBUG, "%s query: %s", __FUNCTION__, sql.c_str());
    // run query
    unsigned int time = XbmcThreads::SystemClockMillis();
    if (!m_pDS->query(sql.c_str())) return false;
    CLog::Log(LOGDEBUG, "%s - query took %i ms",
              __FUNCTION__, XbmcThreads::SystemClockMillis() - time); time = XbmcThreads::SystemClockMillis();

    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return false;
    }

    items.Reserve(iRowsFound);

    // get data from returned rows
    while (!m_pDS->eof())
    {
      try
      {
        CStdString strDir;
        int idAlbum = m_pDS->fv("idAlbum").get_asInt();
        strDir.Format("%s%ld/", baseDir.c_str(), idAlbum);
        CFileItemPtr pItem(new CFileItem(strDir, GetAlbumFromDataset(m_pDS.get())));
        pItem->SetIconImage("DefaultAlbumCover.png");
        items.Add(pItem);
        m_pDS->next();
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
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, where.c_str());
  }
  return false;
}

bool CMusicDatabase::GetSongsByWhere(const CStdString &baseDir, const CStdString &whereClause, CFileItemList &items)
{
  if (NULL == m_pDB.get()) return false;
  if (NULL == m_pDS.get()) return false;

  try
  {
    unsigned int time = XbmcThreads::SystemClockMillis();
    // We don't use PrepareSQL here, as the WHERE clause is already formatted.
    CStdString strSQL = "select * from songview " + whereClause;
    CLog::Log(LOGDEBUG, "%s query = %s", __FUNCTION__, strSQL.c_str());
    // run query
    if (!m_pDS->query(strSQL.c_str()))
      return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return false;
    }

    // get data from returned rows
    items.Reserve(items.Size() + iRowsFound);
    // get songs from returned subtable
    int count = 0;
    while (!m_pDS->eof())
    {
      try
      {
        CFileItemPtr item(new CFileItem);
        GetFileItemFromDataset(item.get(), baseDir);
        // HACK for sorting by database returned order
        item->m_iprogramCount = ++count;
        items.Add(item);
        m_pDS->next();
      }
      catch (...)
      {
        m_pDS->close();
        CLog::Log(LOGERROR, "%s: out of memory loading query: %s", __FUNCTION__, whereClause.c_str());
        return (items.Size() > 0);
      }
    }
    // cleanup
    m_pDS->close();
    CLog::Log(LOGDEBUG, "%s(%s) - took %d ms", __FUNCTION__, whereClause.c_str(), XbmcThreads::SystemClockMillis() - time);
    return true;
  }
  catch (...)
  {
    // cleanup
    m_pDS->close();
    CLog::Log(LOGERROR, "%s(%s) failed", __FUNCTION__, whereClause.c_str());
  }
  return false;
}

bool CMusicDatabase::GetSongsByYear(const CStdString& baseDir, CFileItemList& items, int year)
{
  CStdString where=PrepareSQL("where (iYear=%ld)", year);
  return GetSongsByWhere(baseDir, where, items);
}

bool CMusicDatabase::GetSongsNav(const CStdString& strBaseDir, CFileItemList& items, int idGenre, int idArtist,int idAlbum)
{
  CStdString strWhere;

  if (idAlbum!=-1)
    strWhere=PrepareSQL("where (idAlbum=%ld) ", idAlbum);

  if (idGenre!=-1)
  {
    if (strWhere.IsEmpty())
      strWhere += "where ";
    else
      strWhere += "and ";

    strWhere += PrepareSQL("(idGenre=%i " // All songs where primary genre fits
                          "or idSong IN "
                            "("
                            "select exgenresong.idSong from exgenresong " // All songs by where extra genres fit
                            "where exgenresong.idGenre=%i"
                            ")"
                          ") "
                          , idGenre, idGenre);
  }

  if (idArtist!=-1)
  {
    if (strWhere.IsEmpty())
      strWhere += "where ";
    else
      strWhere += "and ";

    strWhere += PrepareSQL("(idArtist=%i " // All songs where primary artist fits
                          "or idSong IN "
                            "("
                            "select exartistsong.idSong from exartistsong " // All songs where extra artists fit
                            "where exartistsong.idArtist=%i"
                            ")"
                          "or idSong IN "
                            "("
                            "select song.idSong from song " // All songs where the primary album artist fits
                            "join album on song.idAlbum=album.idAlbum "
                            "where album.idArtist=%i"
                            ")"
                          "or idSong IN "
                            "("
                            "select song.idSong from song " // All songs where the extra album artist fit, excluding
                            "join exartistalbum on song.idAlbum=exartistalbum.idAlbum " // various artist albums
                            "join album on song.idAlbum=album.idAlbum "
                            "where exartistalbum.idArtist=%i and album.strExtraArtists != ''"
                            ")"
                          ") "
                          , idArtist, idArtist, idArtist, idArtist);
  }

  // run query
  bool bResult = GetSongsByWhere(strBaseDir, strWhere, items);
  if (bResult && idArtist != -1)
  {
    CStdString strArtist = GetArtistById(idArtist);
    CStdString strFanart = items.GetCachedThumb(strArtist,g_settings.GetMusicFanartFolder());
    if (CFile::Exists(strFanart))
      items.SetProperty("fanart_image",strFanart);
  }

  return bResult;
}

bool CMusicDatabase::UpdateOldVersion(int version)
{
  if (NULL == m_pDB.get()) return false;
  if (NULL == m_pDS.get()) return false;
  if (NULL == m_pDS2.get()) return false;

  BeginTransaction();

  try
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

    // always recreate the views after any table change
    CreateViews();

    CommitTransaction();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "Error attempting to update the database version!");
    return false;
  }
  return true;
}

int CMusicDatabase::AddThumb(const CStdString& strThumb1)
{
  CStdString strSQL;
  try
  {
    CStdString strThumb = strThumb1;
    if (strThumb.IsEmpty())
      strThumb = "NONE";

    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    map <CStdString, int>::const_iterator it;

    it = m_thumbCache.find(strThumb1);
    if (it != m_thumbCache.end())
      return it->second;

    strSQL=PrepareSQL( "select * from thumb where strThumb='%s'", strThumb.c_str());
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      // doesnt exists, add it
      strSQL=PrepareSQL("insert into thumb (idThumb, strThumb) values( NULL, '%s' )", strThumb.c_str());
      m_pDS->exec(strSQL.c_str());

      int idPath = (int)m_pDS->lastinsertid();
      m_thumbCache.insert(pair<CStdString, int>(strThumb1, idPath));
      return idPath;
    }
    else
    {
      int idPath = m_pDS->fv("idThumb").get_asInt();
      m_thumbCache.insert(pair<CStdString, int>(strThumb1, idPath));
      m_pDS->close();
      return idPath;
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "musicdatabase:unable to addthumb (%s)", strSQL.c_str());
  }

  return -1;
}

unsigned int CMusicDatabase::GetSongIDs(const CStdString& strWhere, vector<pair<int,int> > &songIDs)
{
  try
  {
    if (NULL == m_pDB.get()) return 0;
    if (NULL == m_pDS.get()) return 0;

    CStdString strSQL = "select idSong from songview " + strWhere;
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
    CLog::Log(LOGERROR, "%s(%s) failed", __FUNCTION__, strWhere.c_str());
  }
  return 0;
}

int CMusicDatabase::GetSongsCount(const CStdString& strWhere)
{
  try
  {
    if (NULL == m_pDB.get()) return 0;
    if (NULL == m_pDS.get()) return 0;

    CStdString strSQL = "select count(idSong) as NumSongs from songview " + strWhere;
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
    CLog::Log(LOGERROR, "%s(%s) failed", __FUNCTION__, strWhere.c_str());
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
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    int idThumb=AddThumb(strThumb);

    if (idThumb>-1)
    {
      CStdString strSQL=PrepareSQL("UPDATE album SET idThumb=%ld where idAlbum=%ld", idThumb, idAlbum);
      CLog::Log(LOGDEBUG, "%s exec: %s", __FUNCTION__, strSQL.c_str());
      m_pDS->exec(strSQL.c_str());
      strSQL=PrepareSQL("UPDATE song SET idThumb=%ld where idAlbum=%ld", idThumb, idAlbum);
      CLog::Log(LOGDEBUG, "%s exec: %s", __FUNCTION__, strSQL.c_str());
      m_pDS->exec(strSQL.c_str());
      return true;
    }
    return false;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%i) failed", __FUNCTION__, idAlbum);
  }

  return false;
}

bool CMusicDatabase::GetAlbumThumb(int idAlbum, CStdString& strThumb)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL=PrepareSQL("select strThumb from thumb join album on album.idThumb = thumb.idThumb where album.idAlbum=%i", idAlbum);
    m_pDS2->query(strSQL.c_str());
    if (m_pDS2->eof())
      return false;

    strThumb = m_pDS2->fv("strThumb").get_asString();
    m_pDS2->close();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s - (%i) failed", __FUNCTION__, idAlbum);
  }

  return false;
}

bool CMusicDatabase::GetArtistPath(int idArtist, CStdString &basePath)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS2.get()) return false;

    // find all albums from this artist, and all the paths to the songs from those albums
    CStdString strSQL=PrepareSQL("select strPath from album join song on album.idAlbum = song.idAlbum join path on song.idPath = path.idPath "
                                "where album.idAlbum in (select idAlbum from album where album.idArtist=%i) "
                                "or album.idAlbum in (select idAlbum from exartistalbum where exartistalbum.idArtist = %i) "
                                "group by song.idPath", idArtist, idArtist);

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
      strSQL=PrepareSQL("select idAlbum from album where album.strAlbum like '%s'", strAlbum.c_str());
    else
      strSQL=PrepareSQL("select album.idAlbum from album join artist on artist.idArtist = album.idArtist where album.strAlbum like '%s' and artist.strArtist like '%s'", strAlbum.c_str(),strArtist.c_str());
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

bool CMusicDatabase::GetRandomSong(CFileItem* item, int& idSong, const CStdString& strWhere)
{
  try
  {
    idSong = -1;

    int iCount = GetSongsCount(strWhere);
    if (iCount <= 0)
      return false;
    int iRandom = rand() % iCount;

    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    // We don't use PrepareSQL here, as the WHERE clause is already formatted
    CStdString strSQL;
    strSQL.Format("select * from songview %s order by idSong limit 1 offset %i", strWhere.c_str(), iRandom);

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
    CLog::Log(LOGERROR, "%s(%s) failed", __FUNCTION__, strWhere.c_str());
  }
  return false;
}

bool CMusicDatabase::GetVariousArtistsAlbums(const CStdString& strBaseDir, CFileItemList& items)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strVariousArtists = g_localizeStrings.Get(340);
    int idVariousArtists=AddArtist(strVariousArtists);
    if (idVariousArtists<0)
      return false;

    CStdString strSQL = PrepareSQL("select * from albumview where idArtist=%i", idVariousArtists);

    // run query
    CLog::Log(LOGDEBUG, "%s query: %s", __FUNCTION__, strSQL.c_str());
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return false;
    }

    items.Reserve(iRowsFound);

    // get data from returned rows
    while (!m_pDS->eof())
    {
      CStdString strDir;
      strDir.Format("%s%ld/", strBaseDir.c_str(), m_pDS->fv("idAlbum").get_asInt());
      CFileItemPtr pItem(new CFileItem(strDir, GetAlbumFromDataset(m_pDS.get())));
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

bool CMusicDatabase::GetVariousArtistsAlbumsSongs(const CStdString& strBaseDir, CFileItemList& items)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strVariousArtists = g_localizeStrings.Get(340);
    int idVariousArtists=AddArtist(strVariousArtists);
    if (idVariousArtists<0)
      return false;

    CStdString strSQL = PrepareSQL("select * from songview where idAlbum IN (select idAlbum from album where idArtist=%i)", idVariousArtists);

    // run query
    CLog::Log(LOGDEBUG, "%s query: %s", __FUNCTION__, strSQL.c_str());
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return false;
    }

    items.Reserve(iRowsFound);

    // get data from returned rows
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

void CMusicDatabase::SplitString(const CStdString &multiString, vector<CStdString> &vecStrings, CStdString &extraStrings)
{
  int numStrings = StringUtils::SplitString(multiString, g_advancedSettings.m_musicItemSeparator, vecStrings);
  for (int i = 1; i < numStrings; i++)
    extraStrings += g_advancedSettings.m_musicItemSeparator + vecStrings[i];
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

    CStdString strSQL=PrepareSQL("select strHash from path where strPath like '%s'", path.c_str());
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
  // from the exartistsong and exgenresong tables (as otherwise if a song is added back
  // to the table with the same idSong, these tables can't be cleaned up properly later)

  // TODO: SQLite probably doesn't allow this, but can we rely on that??

  // We don't need to remove orphaned albums at this point as in AddAlbum() we check
  // first whether the album has already been read during this scan, and if it hasn't
  // we check whether it's in the table and update accordingly at that point, removing the entries from
  // the exartistalbum and exgenrealbum tables.  The only failure point for this is albums
  // that span multiple folders, where just the files in one folder have been changed.  In this case
  // any exalbumartist(s) that are only in the files that haven't changed will be removed.  Clearly
  // the primary albumartist still matches (as that's what we looked up based on) so is this really
  // an issue?  I don't think it is, as those artists will still have links to the album via the songs
  // which is generally what we rely on, so the only failure point is albumartist lookup.  In this
  // case, it will return only things in the exartistalbum table from the newly updated songs (and
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

    CStdString sql=PrepareSQL("select * from songview where strPath like '%s%s'", path.c_str(), (exact?"":"%"));
    if (!m_pDS->query(sql.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound > 0)
    {
      std::vector<int> ids;
      CStdString songIds = "(";
      while (!m_pDS->eof())
      {
        CSong song = GetSongFromDataset();
        songs.Add(song.strFileName, song);
        songIds += PrepareSQL("%i,", song.idSong);
        ids.push_back(song.idSong);
        m_pDS->next();
      }
      songIds.TrimRight(",");
      songIds += ")";

      m_pDS->close();

      // and delete all songs, exartistsongs and exgenresongs and karaoke
      sql = "delete from song where idSong in " + songIds;
      m_pDS->exec(sql.c_str());
      sql = "delete from exartistsong where idSong in " + songIds;
      m_pDS->exec(sql.c_str());
      sql = "delete from exgenresong where idSong in " + songIds;
      m_pDS->exec(sql.c_str());
      sql = "delete from karaokedata where idSong in " + songIds;
      m_pDS->exec(sql.c_str());

      for (unsigned int i = 0; i < ids.size(); i++)
        AnnounceRemove("song", ids[i]);
    }
    // and remove the path as well (it'll be re-added later on with the new hash if it's non-empty)
    sql = PrepareSQL("delete from path where strPath like '%s%s'", path.c_str(), (exact?"":"%"));
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
    g_infoManager.SetLibraryBool(LIBRARY_HAS_MUSIC, GetSongsCount("") > 0);
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
    CStdString strSQL = PrepareSQL("delete from content where strPath like '%s'",strPath.c_str());
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

    CStdString strSQL = PrepareSQL("select * from content where strPath like '%s'",strPath.c_str());
    m_pDS->query(strSQL.c_str());
    if (m_pDS->eof()) // no info set for path - fallback logic commencing
    {
      CQueryParams params;
      CDirectoryNode::GetDatabaseInfo(strPath, params);
      if (params.GetGenreId() != -1) // check genre
      {
        strSQL = PrepareSQL("select * from content where strPath like 'musicdb://1/%i/'",params.GetGenreId());
        m_pDS->query(strSQL.c_str());
      }
      if (m_pDS->eof() && params.GetAlbumId() != -1) // check album
      {
        strSQL = PrepareSQL("select * from content where strPath like 'musicdb://3/%i/'",params.GetGenreId());
        m_pDS->query(strSQL.c_str());
      }
      if (m_pDS->eof() && params.GetArtistId() != -1) // check artist
      {
        strSQL = PrepareSQL("select * from content where strPath like 'musicdb://2/%i/'",params.GetArtistId());
        m_pDS->query(strSQL.c_str());
      }
      if (m_pDS->eof()) // general albums setting
      {
        strSQL = PrepareSQL("select * from content where strPath like 'musicdb://3/'");
        m_pDS->query(strSQL.c_str());
      }
      if (m_pDS->eof()) // general artist setting
      {
        strSQL = PrepareSQL("select * from content where strPath like 'musicdb://2/'");
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
                     "join albumview on albuminfo.idAlbum=albumview.idAlbum "
                     "join genre on albuminfo.idGenre=genre.idGenre";

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
    TiXmlDocument xmlDoc;
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
            CStdString strThumb;
            if (GetAlbumThumb(album.idAlbum,strThumb) && (overwrite || !CFile::Exists(URIUtils::AddFileToFolder(strPath,"folder.jpg"))))
              CFile::Cache(strThumb,URIUtils::AddFileToFolder(strPath,"folder.jpg"));
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
    sql = "select * from artistinfo "
          "join artist on artist.idArtist=artistinfo.idArtist";

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

          if (images)
          {
            CFileItem item(artist);
            if (CFile::Exists(item.GetCachedArtistThumb()) && (overwrite || !CFile::Exists(URIUtils::AddFileToFolder(strPath,"folder.jpg"))))
              CFile::Cache(item.GetCachedArtistThumb(),URIUtils::AddFileToFolder(strPath,"folder.jpg"));
            if (CFile::Exists(item.GetCachedFanart()) && (overwrite || !CFile::Exists(URIUtils::AddFileToFolder(strPath,"fanart.jpg"))))
              CFile::Cache(item.GetCachedFanart(),URIUtils::AddFileToFolder(strPath,"fanart.jpg"));
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

    TiXmlDocument xmlDoc;
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
        int idAlbum = GetAlbumByName(album.strAlbum,album.strArtist);
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

void CMusicDatabase::AddKaraokeData(const CSong& song)
{
  try
  {
    CStdString strSQL;

    // If song.iKaraokeNumber is non-zero, we already have it in the database. Just replace the song ID.
    if ( song.iKaraokeNumber > 0 )
    {
      CStdString strSQL = PrepareSQL("UPDATE karaokedata SET idSong=%i WHERE iKaraNumber=%i", song.idSong, song.iKaraokeNumber);
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
        "VALUES( %i, %i, 0, NULL, NULL, '%ul' )", iKaraokeNumber, song.idSong, crc );

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
        outdoc = "<tr><td>" + songnum + "</td><td>" + song.strArtist + "</td><td>" + song.strTitle + "</td></tr>\r\n";
      else
        outdoc = songnum + "\t" + song.strArtist + "\t" + song.strTitle + "\t" + song.strFileName + "\r\n";

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
        char * songpath;
        for ( songpath = linestart; *songpath; songpath++ )
        {
          if ( *songpath == '\t' )
          {
            tabs++;

            if ( tabs == 1 )
              *songpath = '\0'; // terminate number

            if ( tabs == 3 )
            {
              songpath++;
              break; // songpath points to file name
            }
          }
        }

        int num = atoi( linestart );
        if ( num <= 0 || *songpath == '\0' )
        {
          CLog::Log( LOGERROR, "Karaoke import: error in line %s", linestart );
          m_pDS->close();
          return;
        }

        // Update the database
        CSong song;
        if ( GetSongByFileName( songpath, song) )
        {
          CStdString strSQL = PrepareSQL("UPDATE karaokedata SET iKaraNumber=%i WHERE idSong=%i", num, song.idSong);
          m_pDS->exec(strSQL.c_str());
        }
        else
        {
          CLog::Log( LOGDEBUG, "Karaoke import: file '%s' was not found in database, skipped", songpath );
        }

        linestart = p + 1;

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
  item.SetProperty("artist_instrument",artist.strInstruments);
  item.SetProperty("artist_style",artist.strStyles);
  item.SetProperty("artist_mood",artist.strMoods);
  item.SetProperty("artist_born",artist.strBorn);
  item.SetProperty("artist_formed",artist.strFormed);
  item.SetProperty("artist_description",artist.strBiography);
  item.SetProperty("artist_genre",artist.strGenre);
  item.SetProperty("artist_died",artist.strDied);
  item.SetProperty("artist_disbanded",artist.strDisbanded);
  item.SetProperty("artist_yearsactive",artist.strYearsActive);
}

void CMusicDatabase::SetPropertiesFromAlbum(CFileItem& item, const CAlbum& album)
{
  item.SetProperty("album_description", album.strReview);
  item.SetProperty("album_theme", album.strThemes);
  item.SetProperty("album_mood", album.strMoods);
  item.SetProperty("album_style", album.strStyles);
  item.SetProperty("album_type", album.strType);
  item.SetProperty("album_label", album.strLabel);
  item.SetProperty("album_artist", album.strArtist);
  item.SetProperty("album_genre", album.strGenre);
  item.SetProperty("album_title", album.strAlbum);
  if (album.iRating > 0)
    item.SetProperty("album_rating", album.iRating);
}

void CMusicDatabase::SetPropertiesForFileItem(CFileItem& item)
{
  if (!item.HasMusicInfoTag())
    return;
  int idArtist = GetArtistByName(item.GetMusicInfoTag()->GetArtist());
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

  CStdString strFanart = item.GetCachedFanart();
  if (XFILE::CFile::Exists(strFanart))
    item.SetProperty("fanart_image",strFanart);
}

int CMusicDatabase::GetVariousArtistsAlbumsCount()
{
  CStdString strVariousArtists = g_localizeStrings.Get(340);
  int idVariousArtists=AddArtist(strVariousArtists);
  CStdString strSQL = PrepareSQL("select count(idAlbum) from album where idArtist=%i", idVariousArtists);
  int result=0;
  try
  {
    if (NULL == m_pDB.get()) return 0;
    if (NULL == m_pDS.get()) return 0;
    m_pDS->query(strSQL.c_str());
    if (!m_pDS->eof())
      result = m_pDS->fv(0).get_asInt();
    m_pDS->close();
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }

  return result;
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
