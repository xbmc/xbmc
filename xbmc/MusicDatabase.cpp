/*
 *      Copyright (C) 2005-2007 Team XboxMediaCenter
 *      http://www.xboxmediacenter.com
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
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "MusicDatabase.h"
#include "FileSystem/cddb.h"
#include "FileSystem/DirectoryCache.h"
#include "FileSystem/MusicDatabaseDirectory/DirectoryNode.h"
#include "FileSystem/MusicDatabaseDirectory/QueryParams.h"
#include "GUIDialogMusicScan.h"
#include "DetectDVDType.h"

using namespace XFILE;
using namespace DIRECTORY;
using namespace MUSICDATABASEDIRECTORY;

#define MUSIC_DATABASE_OLD_VERSION 1.6f
#define MUSIC_DATABASE_VERSION        7
#define MUSIC_DATABASE_NAME "MyMusic7.db"
#define RECENTLY_ADDED_LIMIT  25
#define RECENTLY_PLAYED_LIMIT 25
#define MIN_FULL_SEARCH_LENGTH 3

using namespace CDDB;

CMusicDatabase::CMusicDatabase(void)
{
  m_preV2version=MUSIC_DATABASE_OLD_VERSION;
  m_version=MUSIC_DATABASE_VERSION;
  m_strDatabaseFile=MUSIC_DATABASE_NAME;
  m_iSongsBeforeCommit = 0;
}

CMusicDatabase::~CMusicDatabase(void)
{
  EmptyCache();
}

bool CMusicDatabase::CreateTables()
{
  try
  {
    CDatabase::CreateTables();

    CLog::Log(LOGINFO, "create artist table");
    m_pDS->exec("CREATE TABLE artist ( idArtist integer primary key, strArtist text)\n");
    CLog::Log(LOGINFO, "create album table");
    m_pDS->exec("CREATE TABLE album ( idAlbum integer primary key, strAlbum text, idArtist integer, strExtraArtists text, idGenre integer, strExtraGenres text, iYear integer, idThumb integer)\n");
    CLog::Log(LOGINFO, "create genre table");
    m_pDS->exec("CREATE TABLE genre ( idGenre integer primary key, strGenre text)\n");
    CLog::Log(LOGINFO, "create path table");
    m_pDS->exec("CREATE TABLE path ( idPath integer primary key, strPath text, strHash text)\n");
    CLog::Log(LOGINFO, "create song table");
    m_pDS->exec("CREATE TABLE song ( idSong integer primary key, idAlbum integer, idPath integer, idArtist integer, strExtraArtists text, idGenre integer, strExtraGenres text, strTitle text, iTrack integer, iDuration integer, iYear integer, dwFileNameCRC text, strFileName text, strMusicBrainzTrackID text, strMusicBrainzArtistID text, strMusicBrainzAlbumID text, strMusicBrainzAlbumArtistID text, strMusicBrainzTRMID text, iTimesPlayed integer, iStartOffset integer, iEndOffset integer, idThumb integer, lastplayed text default NULL, rating char default '0', comment text)\n");
    CLog::Log(LOGINFO, "create albuminfo table");
    m_pDS->exec("CREATE TABLE albuminfo ( idAlbumInfo integer primary key, idAlbum integer, iYear integer, idGenre integer, strExtraGenres text, strTones text, strStyles text, strReview text, strImage text, iRating integer)\n");
    CLog::Log(LOGINFO, "create albuminfosong table");
    m_pDS->exec("CREATE TABLE albuminfosong ( idAlbumInfoSong integer primary key, idAlbumInfo integer, iTrack integer, strTitle text, iDuration integer)\n");
    CLog::Log(LOGINFO, "create thumb table");
    m_pDS->exec("CREATE TABLE thumb (idThumb integer primary key, strThumb text)\n");

    CLog::Log(LOGINFO, "create exartistsong table");
    m_pDS->exec("CREATE TABLE exartistsong ( idSong integer, iPosition integer, idArtist integer)\n");
    CLog::Log(LOGINFO, "create extragenresong table");
    m_pDS->exec("CREATE TABLE exgenresong ( idSong integer, iPosition integer, idGenre integer)\n");
    CLog::Log(LOGINFO, "create exartistalbum table");
    m_pDS->exec("CREATE TABLE exartistalbum ( idAlbum integer, iPosition integer, idArtist integer)\n");
    CLog::Log(LOGINFO, "create exgenrealbum table");
    m_pDS->exec("CREATE TABLE exgenrealbum ( idAlbum integer, iPosition integer, idGenre integer)\n");

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
    CLog::Log(LOGINFO, "create thumb index");
    m_pDS->exec("CREATE INDEX idxThumb ON thumb(strThumb)");
    //m_pDS->exec("CREATE INDEX idxSong ON song(dwFileNameCRC)");

    // Trigger
    CLog::Log(LOGINFO, "create albuminfo trigger");
    m_pDS->exec("CREATE TRIGGER tgrAlbumInfo AFTER delete ON albuminfo FOR EACH ROW BEGIN delete from albuminfosong where albuminfosong.idAlbumInfo=old.idAlbumInfo; END");

    // views
    CLog::Log(LOGINFO, "create song view");
    m_pDS->exec("create view songview as select idSong, song.strExtraArtists as strExtraArtists, song.strExtraGenres as strExtraGenres, strTitle, iTrack, iDuration, song.iYear as iYear, dwFileNameCRC, strFileName, strMusicBrainzTrackID, strMusicBrainzArtistID, strMusicBrainzAlbumID, strMusicBrainzAlbumArtistID, strMusicBrainzTRMID, iTimesPlayed, iStartOffset, iEndOffset, lastplayed, rating, comment, song.idAlbum as idAlbum, strAlbum, strPath, song.idArtist as idArtist, strArtist, song.idGenre as idGenre, strGenre, strThumb from song join album on song.idAlbum=album.idAlbum join path on song.idPath=path.idPath join artist on song.idArtist=artist.idArtist join genre on song.idGenre=genre.idGenre join thumb on song.idThumb=thumb.idThumb");
    CLog::Log(LOGINFO, "create album view");
    m_pDS->exec("create view albumview as select idAlbum, strAlbum, strExtraArtists, album.idArtist as idArtist, strExtraGenres, album.idGenre as idGenre, strArtist, strGenre, iYear, strThumb from album left outer join artist on album.idArtist=artist.idArtist left outer join genre on album.idGenre=genre.idGenre left outer join thumb on album.idThumb=thumb.idThumb");
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "musicbase::unable to create tables:%lu", GetLastError());
    return false;
  }

  return true;
}

void CMusicDatabase::AddSong(const CSong& song, bool bCheck)
{
  CStdString strSQL;
  try
  {
    // We need at least the title
    if (song.strTitle.IsEmpty())
      return;

    CStdString strPath, strFileName;
    CUtil::Split(song.strFileName, strPath, strFileName);
    ASSERT(CUtil::HasSlashAtEnd(strPath));

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
    long lArtistId = AddArtist(vecArtists[0]);
    long lGenreId = AddGenre(vecGenres[0]);
    // and also the primary album artist (if applicable)
    long lAlbumArtistId = -1;
    if (!vecAlbumArtists[0].IsEmpty())
      lAlbumArtistId = AddArtist(vecAlbumArtists[0]);

    long lPathId = AddPath(strPath);
    long lThumbId = AddThumb(song.strThumb);
    long lAlbumId;
    if (lAlbumArtistId > -1)  // have an album artist
      lAlbumId = AddAlbum(song.strAlbum, lAlbumArtistId, extraAlbumArtists, song.strAlbumArtist, lThumbId, lGenreId, extraGenres, song.iYear);
    else
      lAlbumId = AddAlbum(song.strAlbum, lArtistId, extraArtists, song.strArtist, lThumbId, lGenreId, extraGenres, song.iYear);

    DWORD crc = ComputeCRC(song.strFileName);

    bool bInsert = true;
    int lSongId = -1;
    if (bCheck)
    {
      strSQL=FormatSQL("select * from song where idAlbum=%i and dwFileNameCRC='%ul' and strTitle='%s'",
                    lAlbumId, crc, song.strTitle.c_str());
      if (!m_pDS->query(strSQL.c_str())) return ;
      if (m_pDS->num_rows() != 0)
      {
        lSongId = m_pDS->fv("idSong").get_asLong();
        bInsert = false;
      }
      m_pDS->close();
    }
    if (bInsert)
    {
      CStdString strSQL1;

      strSQL=FormatSQL("insert into song (idSong,idAlbum,idPath,idArtist,strExtraArtists,idGenre,strExtraGenres,strTitle,iTrack,iDuration,iYear,dwFileNameCRC,strFileName,strMusicBrainzTrackID,strMusicBrainzArtistID,strMusicBrainzAlbumID,strMusicBrainzAlbumArtistID,strMusicBrainzTRMID,iTimesPlayed,iStartOffset,iEndOffset,idThumb,lastplayed,rating,comment) values (NULL,%i,%i,%i,'%s',%i,'%s','%s',%i,%i,%i,'%ul','%s','%s','%s','%s','%s','%s'",
                    lAlbumId, lPathId, lArtistId, extraArtists.c_str(), lGenreId, extraGenres.c_str(),
                    song.strTitle.c_str(),
                    song.iTrack, song.iDuration, song.iYear,
                    crc, strFileName.c_str(),
                    song.strMusicBrainzTrackID.c_str(),
                    song.strMusicBrainzArtistID.c_str(),
                    song.strMusicBrainzAlbumID.c_str(),
                    song.strMusicBrainzAlbumArtistID.c_str(),
                    song.strMusicBrainzTRMID.c_str());

      if (song.lastPlayed.GetLength())
        strSQL1=FormatSQL(",%i,%i,%i,%i,'%s','%c','%s')",
                      song.iTimesPlayed, song.iStartOffset, song.iEndOffset, lThumbId, song.lastPlayed.c_str(), song.rating, song.strComment.c_str());
      else
        strSQL1=FormatSQL(",%i,%i,%i,%i,NULL,'%c','%s')",
                      song.iTimesPlayed, song.iStartOffset, song.iEndOffset, lThumbId, song.rating, song.strComment.c_str());
      strSQL+=strSQL1;

      m_pDS->exec(strSQL.c_str());
      lSongId = (long)sqlite3_last_insert_rowid(m_pDB->getHandle());
    }

    // add extra artists and genres
    AddExtraSongArtists(vecArtists, lSongId, bCheck);
    if (lAlbumArtistId > -1)
      AddExtraAlbumArtists(vecAlbumArtists, lAlbumId);
    else
      AddExtraAlbumArtists(vecArtists, lAlbumId);
    AddExtraGenres(vecGenres, lSongId, lAlbumId, bCheck);

    // increment the number of songs we've added since the last commit, and check if we should commit
    if (m_iSongsBeforeCommit++ > NUM_SONGS_BEFORE_COMMIT)
    {
      CommitTransaction();
      m_iSongsBeforeCommit=0;
      BeginTransaction();
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "musicdatabase:unable to addsong (%s)", strSQL.c_str());
  }
}

long CMusicDatabase::AddAlbum(const CStdString& strAlbum1, long lArtistId, const CStdString &extraArtists, const CStdString &strArtist1, long idThumb, long idGenre, const CStdString &extraGenres, long year)
{
  CStdString strSQL;
  try
  {
    CStdString strAlbum=strAlbum1;
    strAlbum.TrimLeft(" ");
    strAlbum.TrimRight(" ");

    CStdString strArtist=strArtist1;

    if (strAlbum.IsEmpty())
    {
      // album tag is empty, fake an album
      // with no path and artist and add this
      // instead of an empty string
      strAlbum=g_localizeStrings.Get(13205); // Unknown
      lArtistId=-1;
      strArtist.Empty();
      idThumb=AddThumb("");
      idGenre=-1;
      year=0;
    }

    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    map <CStdString, CAlbumCache>::const_iterator it;

    it = m_albumCache.find(strAlbum + strArtist);
    if (it != m_albumCache.end())
      return it->second.idAlbum;

    strSQL=FormatSQL("select * from album where idArtist=%i and strAlbum like '%s'", lArtistId, strAlbum.c_str());
    m_pDS->query(strSQL.c_str());

    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      // doesnt exists, add it
      strSQL=FormatSQL("insert into album (idAlbum, strAlbum, idArtist, strExtraArtists, idGenre, strExtraGenres, iYear, idThumb) values( NULL, '%s', %i, '%s', %i, '%s', %i, %i)", strAlbum.c_str(), lArtistId, extraArtists.c_str(), idGenre, extraGenres.c_str(), year, idThumb);
      m_pDS->exec(strSQL.c_str());

      CAlbumCache album;
      album.idAlbum = (long)sqlite3_last_insert_rowid(m_pDB->getHandle());
      album.strAlbum = strAlbum;
      album.idArtist = lArtistId;
      album.strArtist = strArtist;
      m_albumCache.insert(pair<CStdString, CAlbumCache>(album.strAlbum + album.strArtist, album));
      return album.idAlbum;
    }
    else
    {
      // exists in our database and not scanned during this scan, so we should update it as the details
      // may have changed (there's a reason we're rescanning, afterall!)
      CAlbumCache album;
      album.idAlbum = m_pDS->fv("idAlbum").get_asLong();
      album.strAlbum = strAlbum;
      album.idArtist = lArtistId;
      album.strArtist = strArtist;
      m_albumCache.insert(pair<CStdString, CAlbumCache>(album.strAlbum + album.strArtist, album));
      m_pDS->close();
      strSQL=FormatSQL("update album set strExtraArtists='%s', idGenre=%i, strExtraGenres='%s', iYear=%i, idThumb=%i where idAlbum=%i", extraArtists.c_str(), idGenre, extraGenres.c_str(), year, idThumb, album.idAlbum);
      m_pDS->exec(strSQL.c_str());
      // and clear the exartistalbum and exgenrealbum tables - these are updated in AddSong()
      strSQL=FormatSQL("delete from exartistalbum where idAlbum=%i", album.idAlbum);
      m_pDS->exec(strSQL.c_str());
      strSQL=FormatSQL("delete from exgenrealbum where idAlbum=%i", album.idAlbum);
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

long CMusicDatabase::AddGenre(const CStdString& strGenre1)
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


    strSQL=FormatSQL("select * from genre where strGenre like '%s'", strGenre.c_str());
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      // doesnt exists, add it
      strSQL=FormatSQL("insert into genre (idGenre, strGenre) values( NULL, '%s' )", strGenre.c_str());
      m_pDS->exec(strSQL.c_str());

      int idGenre = (int)sqlite3_last_insert_rowid(m_pDB->getHandle());
      m_genreCache.insert(pair<CStdString, int>(strGenre1, idGenre));
      return idGenre;
    }
    else
    {
      int idGenre = m_pDS->fv("idGenre").get_asLong();
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

long CMusicDatabase::AddArtist(const CStdString& strArtist1)
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

    strSQL=FormatSQL("select * from artist where strArtist like '%s'", strArtist.c_str());
    m_pDS->query(strSQL.c_str());

    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      // doesnt exists, add it
      strSQL=FormatSQL("insert into artist (idArtist, strArtist) values( NULL, '%s' )", strArtist.c_str());
      m_pDS->exec(strSQL.c_str());
      int idArtist = (long)sqlite3_last_insert_rowid(m_pDB->getHandle());
      m_artistCache.insert(pair<CStdString, int>(strArtist1, idArtist));
      return idArtist;
    }
    else
    {
      int idArtist = (long)m_pDS->fv("idArtist").get_asLong();
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

void CMusicDatabase::AddExtraSongArtists(const CStdStringArray &vecArtists, long lSongId, bool bCheck)
{
  try
  {
    // add each of the artists in the vector of artists
    for (int i = 1; i < (int)vecArtists.size(); i++)
    {
      long lArtistId = AddArtist(vecArtists[i]);
      if (lArtistId >= 0)
      { // added successfully, we must now add entries to the exartistsong table
        CStdString strSQL;
        // first link the artist with the song
        bool bInsert = true;
        if (bCheck)
        {
          strSQL=FormatSQL("select * from exartistsong where idSong=%i and idArtist=%i",
                        lSongId, lArtistId);
          if (!m_pDS->query(strSQL.c_str())) return ;
          if (m_pDS->num_rows() != 0)
            bInsert = false; // already exists
          m_pDS->close();
        }
        if (bInsert)
        {
          strSQL=FormatSQL("insert into exartistsong (idSong,iPosition,idArtist) values(%i,%i,%i)",
                        lSongId, i, lArtistId);

          m_pDS->exec(strSQL.c_str());
        }
      }
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%lu) failed", __FUNCTION__, lSongId);
  }
}

void CMusicDatabase::AddExtraAlbumArtists(const CStdStringArray &vecArtists, long lAlbumId)
{
  try
  {
    // add each of the artists in the vector of artists
    for (int i = 1; i < (int)vecArtists.size(); i++)
    {
      long lArtistId = AddArtist(vecArtists[i]);
      if (lArtistId >= 0)
      { // added successfully, we must now add entries to the exartistalbum table
        CStdString strSQL;
        bool bInsert = true;
        // always check artists (as this routine is called whenever a song is added)
        strSQL=FormatSQL("select * from exartistalbum where idAlbum=%i and idArtist=%i",
                      lAlbumId, lArtistId);
        if (!m_pDS->query(strSQL.c_str())) return ;
        if (m_pDS->num_rows() != 0)
          bInsert = false; // already exists
        m_pDS->close();
        if (bInsert)
        {
          strSQL=FormatSQL("insert into exartistalbum (idAlbum,iPosition,idArtist) values(%i,%i,%i)",
                        lAlbumId, i, lArtistId);

          m_pDS->exec(strSQL.c_str());
        }
      }
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%lu) failed", __FUNCTION__, lAlbumId);
  }
}

void CMusicDatabase::AddExtraGenres(const CStdStringArray &vecGenres, long lSongId, long lAlbumId, bool bCheck)
{
  try
  {
    // add each of the genres in the vector
    for (int i = 1; i < (int)vecGenres.size(); i++)
    {
      long lGenreId = AddGenre(vecGenres[i]);
      if (lGenreId >= 0)
      { // added successfully!
        CStdString strSQL;
        // first link the genre with the song
        bool bInsert = true;
        if (lSongId)
        {
          if (bCheck)
          {
            strSQL=FormatSQL("select * from exgenresong where idSong=%i and idGenre=%i",
                          lSongId, lGenreId);
            if (!m_pDS->query(strSQL.c_str())) return ;
            if (m_pDS->num_rows() != 0)
              bInsert = false; // already exists
            m_pDS->close();
          }
          if (bInsert)
          {
            strSQL=FormatSQL("insert into exgenresong (idSong,iPosition,idGenre) values(%i,%i,%i)",
                          lSongId, i, lGenreId);

            m_pDS->exec(strSQL.c_str());
          }
        }
        // now link the genre with the album - we always check these as there's usually
        // more than one song per album with the same extra genres
        if (lAlbumId)
        {
          strSQL=FormatSQL("select * from exgenrealbum where idAlbum=%i and idGenre=%i",
                        lAlbumId, lGenreId);
          if (!m_pDS->query(strSQL.c_str())) return ;
          if (m_pDS->num_rows() == 0)
          { // insert
            m_pDS->close();
            strSQL=FormatSQL("insert into exgenrealbum (idAlbum,iPosition,idGenre) values(%i,%i,%i)",
                          lAlbumId, i, lGenreId);

            m_pDS->exec(strSQL.c_str());
          }
        }
      }
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%lu,%lu) failed", __FUNCTION__, lSongId, lAlbumId);
  }
}

long CMusicDatabase::AddPath(const CStdString& strPath)
{
  CStdString strSQL;
  try
  {
    ASSERT(CUtil::HasSlashAtEnd(strPath));

    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    map <CStdString, int>::const_iterator it;

    it = m_pathCache.find(strPath);
    if (it != m_pathCache.end())
      return it->second;

    strSQL=FormatSQL( "select * from path where strPath like '%s'", strPath.c_str());
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      // doesnt exists, add it
      strSQL=FormatSQL("insert into path (idPath, strPath) values( NULL, '%s' )", strPath.c_str());
      m_pDS->exec(strSQL.c_str());

      int idPath = (long)sqlite3_last_insert_rowid(m_pDB->getHandle());
      m_pathCache.insert(pair<CStdString, int>(strPath, idPath));
      return idPath;
    }
    else
    {
      int idPath = m_pDS->fv("idPath").get_asLong();
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
  song.idSong = m_pDS->fv(song_idSong).get_asLong();
  // get the full artist string
  song.strArtist = m_pDS->fv(song_strArtist).get_asString();
  song.strArtist += m_pDS->fv(song_strExtraArtists).get_asString();
  // and the full genre string
  song.strGenre = m_pDS->fv(song_strGenre).get_asString();
  song.strGenre += m_pDS->fv(song_strExtraGenres).get_asString();
  // and the rest...
  song.strAlbum = m_pDS->fv(song_strAlbum).get_asString();
  song.iTrack = m_pDS->fv(song_iTrack).get_asLong() ;
  song.iDuration = m_pDS->fv(song_iDuration).get_asLong() ;
  song.iYear = m_pDS->fv(song_iYear).get_asLong() ;
  song.strTitle = m_pDS->fv(song_strTitle).get_asString();
  song.iTimesPlayed = m_pDS->fv(song_iTimesPlayed).get_asLong();
  song.lastPlayed = m_pDS->fv(song_lastplayed).get_asString();
  song.iStartOffset = m_pDS->fv(song_iStartOffset).get_asLong();
  song.iEndOffset = m_pDS->fv(song_iEndOffset).get_asLong();
  song.strMusicBrainzTrackID = m_pDS->fv(song_strMusicBrainzTrackID).get_asString();
  song.strMusicBrainzArtistID = m_pDS->fv(song_strMusicBrainzArtistID).get_asString();
  song.strMusicBrainzAlbumID = m_pDS->fv(song_strMusicBrainzAlbumID).get_asString();
  song.strMusicBrainzAlbumArtistID = m_pDS->fv(song_strMusicBrainzAlbumArtistID).get_asString();
  song.strMusicBrainzTRMID = m_pDS->fv(song_strMusicBrainzTRMID).get_asString();
  song.rating = m_pDS->fv(song_rating).get_asChar();
  song.strComment = m_pDS->fv(song_comment).get_asString();
  song.strThumb = m_pDS->fv(song_strThumb).get_asString();
  if (song.strThumb == "NONE")
    song.strThumb.Empty();
  // Get filename with full path
  if (!bWithMusicDbPath)
    CUtil::AddFileToFolder(m_pDS->fv(song_strPath).get_asString(), m_pDS->fv(song_strFileName).get_asString(), song.strFileName);
  else
  {
    CStdString strFileName=m_pDS->fv(song_strFileName).get_asString();
    CStdString strExt=CUtil::GetExtension(strFileName);
    song.strFileName.Format("musicdb://3/%ld/%ld%s", m_pDS->fv(song_idAlbum).get_asLong(), m_pDS->fv(song_idSong).get_asLong(), strExt.c_str());
  }

  return song;
}

void CMusicDatabase::GetFileItemFromDataset(CFileItem* item, const CStdString& strMusicDBbasePath)
{
  // get the full artist string
  CStdString strArtist=m_pDS->fv(song_strArtist).get_asString();
  strArtist += m_pDS->fv(song_strExtraArtists).get_asString();
  item->GetMusicInfoTag()->SetArtist(strArtist);
  // and the full genre string
  CStdString strGenre = m_pDS->fv(song_strGenre).get_asString();
  strGenre += m_pDS->fv(song_strExtraGenres).get_asString();
  item->GetMusicInfoTag()->SetGenre(strGenre);
  // and the rest...
  item->GetMusicInfoTag()->SetAlbum(m_pDS->fv(song_strAlbum).get_asString());
  item->GetMusicInfoTag()->SetTrackAndDiskNumber(m_pDS->fv(song_iTrack).get_asLong());
  item->GetMusicInfoTag()->SetDuration(m_pDS->fv(song_iDuration).get_asLong());
  SYSTEMTIME stTime;
  stTime.wYear = (WORD)m_pDS->fv(song_iYear).get_asLong();
  item->GetMusicInfoTag()->SetReleaseDate(stTime);
  item->GetMusicInfoTag()->SetTitle(m_pDS->fv(song_strTitle).get_asString());
  item->SetLabel(m_pDS->fv(song_strTitle).get_asString());
  //song.iTimesPlayed = m_pDS->fv(song_iTimesPlayed).get_asLong();
  item->m_lStartOffset = m_pDS->fv(song_iStartOffset).get_asLong();
  item->m_lEndOffset = m_pDS->fv(song_iEndOffset).get_asLong();
  item->GetMusicInfoTag()->SetMusicBrainzTrackID(m_pDS->fv(song_strMusicBrainzTrackID).get_asString());
  item->GetMusicInfoTag()->SetMusicBrainzArtistID(m_pDS->fv(song_strMusicBrainzArtistID).get_asString());
  item->GetMusicInfoTag()->SetMusicBrainzAlbumID(m_pDS->fv(song_strMusicBrainzAlbumID).get_asString());
  item->GetMusicInfoTag()->SetMusicBrainzAlbumArtistID(m_pDS->fv(song_strMusicBrainzAlbumArtistID).get_asString());
  item->GetMusicInfoTag()->SetMusicBrainzTRMID(m_pDS->fv(song_strMusicBrainzTRMID).get_asString());
  item->GetMusicInfoTag()->SetRating(m_pDS->fv(song_rating).get_asChar());
  item->GetMusicInfoTag()->SetComment(m_pDS->fv(song_comment).get_asString());
  CStdString strRealPath;
  CUtil::AddFileToFolder(m_pDS->fv(song_strPath).get_asString(), m_pDS->fv(song_strFileName).get_asString(), strRealPath);
  item->GetMusicInfoTag()->SetURL(strRealPath);
  item->GetMusicInfoTag()->SetLoaded(true);
  CStdString strThumb=m_pDS->fv(song_strThumb).get_asString();
  if (strThumb != "NONE")
    item->SetThumbnailImage(strThumb);
  // Get filename with full path
  if (strMusicDBbasePath.IsEmpty())
  {
    item->m_strPath = strRealPath;
  }
  else
  {
    CStdString strFileName=m_pDS->fv(song_strFileName).get_asString();
    CStdString strExt=CUtil::GetExtension(strFileName);
    item->m_strPath.Format("%s%ld%s", strMusicDBbasePath.c_str(), m_pDS->fv(song_idSong).get_asLong(), strExt.c_str());
  }
}

CAlbum CMusicDatabase::GetAlbumFromDataset()
{
  CAlbum album;
  album.idAlbum = m_pDS->fv(album_idAlbum).get_asLong();
  album.strAlbum = m_pDS->fv(album_strAlbum).get_asString();
  album.strArtist = m_pDS->fv(album_strArtist).get_asString();
  album.strArtist += m_pDS->fv(album_strExtraArtists).get_asString();
  // workaround... the fake "Unknown" album usually has a NULL artist.
  // since it can contain songs from lots of different artists, lets set
  // it to "Various Artists" instead
  if (m_pDS->fv("artist.idArtist").get_asLong() == -1)
    album.strArtist = g_localizeStrings.Get(340);
  album.strGenre = m_pDS->fv(album_strGenre).get_asString();
  album.strGenre += m_pDS->fv(album_strExtraGenres).get_asString();
  album.iYear = m_pDS->fv(album_iYear).get_asLong();
  album.strThumb = m_pDS->fv(album_strThumb).get_asString();
  if (album.strThumb == "NONE")
    album.strThumb.Empty();
  return album;
}

bool CMusicDatabase::GetSongByFileName(const CStdString& strFileName, CSong& song)
{
  try
  {
    song.Clear();
    CURL url(strFileName);
    
    if (url.GetProtocol()=="musicdb")
    {
      CStdString strFile = CUtil::GetFileName(strFileName);
      CUtil::RemoveExtension(strFile);
      return GetSongById(atol(strFile.c_str()), song);
    }

    CStdString strPath;
    CUtil::GetDirectory(strFileName, strPath);
    CUtil::AddSlashAtEnd(strPath);

    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    DWORD crc = ComputeCRC(strFileName);

    CStdString strSQL=FormatSQL("select * from songview "
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

long CMusicDatabase::GetSongByArtistAndAlbumAndTitle(const CStdString& strArtist, const CStdString& strAlbum, const CStdString& strTitle)
{
  try
  {
    CStdString strSQL=FormatSQL("select idsong from songview "
                                "where strArtist like '%s' and strAlbum like '%s' and "
                                "strTitle like '%s'",strArtist.c_str(),strAlbum.c_str(),strTitle.c_str());

    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return -1;
    }
    long lResult = m_pDS->fv(0).get_asLong();
    m_pDS->close(); // cleanup recordset data
    return lResult;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s,%s,%s) failed", __FUNCTION__, strArtist.c_str(),strAlbum.c_str(),strTitle.c_str());
  }

  return -1;
}

bool CMusicDatabase::GetSongById(long idSong, CSong& song)
{
  try
  {
    song.Clear();

    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL=FormatSQL("select * from songview "
                                "where idSong=%ld"
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
    CLog::Log(LOGERROR, "%s(%ld) failed", __FUNCTION__, idSong);
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
    long lVariousArtistId = AddArtist(g_localizeStrings.Get(340));

    CStdString strSQL;
    if (search.GetLength() >= MIN_FULL_SEARCH_LENGTH)
      strSQL=FormatSQL("select * from artist "
                                "where (strArtist like '%s%%' or strArtist like '%% %s%%') and idArtist <> %i "
                                , search.c_str(), search.c_str(), lVariousArtistId );
    else
      strSQL=FormatSQL("select * from artist "
                                "where strArtist like '%s%%' and idArtist <> %i "
                                , search.c_str(), lVariousArtistId );

    if (!m_pDS->query(strSQL.c_str())) return false;
    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      return false;
    }
    
    CStdString artistLabel(g_localizeStrings.Get(484)); // Artist
    while (!m_pDS->eof())
    {
      CStdString path;
      path.Format("musicdb://2/%ld/", m_pDS->fv(0).get_asLong());
      CFileItem* pItem = new CFileItem(path, true);
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
	const CStdString& strOpenRecord, const CStdString& strCloseRecord, const CStdString& strOpenField, const CStdString& strCloseField, CStdString& strResult)
{
  try
  {
    strResult = "";
	if (NULL == m_pDB.get()) return false;
	if (NULL == m_pDS.get()) return false;
	CStdString strSQL=FormatSQL(strQuery);
	if (!m_pDS->query(strSQL.c_str()))
	{
	  strResult = m_pDB->getErrorMsg();
	  return false;
	}
	//int iRowsFound = m_pDS->num_rows();
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

bool CMusicDatabase::GetAlbumInfo(long idAlbum, CAlbum &info, VECSONGS &songs)
{
  try
  {
    if (idAlbum == -1)
      return false; // not in the database

    CStdString strSQL=FormatSQL("select * from albuminfo "
                                  "join albumview on albuminfo.idAlbum=albumview.idAlbum "
                                  "join genre on albuminfo.idGenre=genre.idGenre "
                                "where albumview.idAlbum = %ld"
                                , idAlbum);

    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound != 0)
    {
      info.idAlbum = idAlbum;
      info.iRating = m_pDS->fv("albuminfo.iRating").get_asLong() ;
      info.iYear = m_pDS->fv("albuminfo.iYear").get_asLong() ;
      info.strAlbum = m_pDS->fv("albumview.strAlbum").get_asString();
      info.strArtist = m_pDS->fv("albumview.strArtist").get_asString();
      info.strArtist += m_pDS->fv("albumview.strExtraArtists").get_asString();
      info.strGenre = m_pDS->fv("genre.strGenre").get_asString();
      info.strGenre += m_pDS->fv("albumview.strExtraGenres").get_asString();
      info.strImage = m_pDS->fv("albuminfo.strImage").get_asString();
      info.strReview = m_pDS->fv("albuminfo.strReview").get_asString();
      info.strStyles = m_pDS->fv("albuminfo.strStyles").get_asString();
      info.strTones = m_pDS->fv("albuminfo.strTones").get_asString();

      long idAlbumInfo = m_pDS->fv("albuminfo.idAlbumInfo").get_asLong();

      m_pDS->close(); // cleanup recordset data

      GetAlbumInfoSongs(idAlbumInfo, songs);
      return true;
    }
    m_pDS->close();
    return false;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%ld) failed", __FUNCTION__, idAlbum);
  }

  return false;
}

bool CMusicDatabase::GetAlbumInfoSongs(long idAlbumInfo, VECSONGS& songs)
{
  try
  {
    CStdString strSQL=FormatSQL("select * from albuminfosong "
                                "where idAlbumInfo=%i "
                                "order by iTrack", idAlbumInfo);

    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0) return false;
    while (!m_pDS->eof())
    {
      CSong song;
      song.iTrack = m_pDS->fv("iTrack").get_asLong();
      song.strTitle = m_pDS->fv("strTitle").get_asString();
      song.iDuration = m_pDS->fv("iDuration").get_asLong();

      songs.push_back(song);
      m_pDS->next();
    }

    m_pDS->close(); // cleanup recordset data

    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%lu) failed", __FUNCTION__, idAlbumInfo);
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
      CFileItem *item = new CFileItem;
      GetFileItemFromDataset(item, strBaseDir);
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
                    "where song.iTimesPlayed>0 "
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
    //int iCount = 1;
    while (!m_pDS->eof())
    {
      //int total = m_pDS->fv("total").get_asInteger();
      albums.push_back(GetAlbumFromDataset());
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
    strSQL.Format("select * from songview join albumview on (songview.idAlbum = albumview.idAlbum) where albumview.idalbum in (select song.idAlbum from song where song.iTimesPlayed>0 group by idalbum order by sum(song.iTimesPlayed) desc limit 100) order by albumview.idalbum in (select song.idAlbum from song where song.iTimesPlayed>0 group by idalbum order by sum(song.iTimesPlayed) desc limit 100)");
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
      CFileItem *item = new CFileItem;
      GetFileItemFromDataset(item, strBaseDir);
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
    strSQL.Format("select distinct albumview.* from song join albumview on albumview.idAlbum=song.idAlbum where song.lastplayed NOT NULL order by song.lastplayed desc limit %i", RECENTLY_PLAYED_LIMIT);
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
      albums.push_back(GetAlbumFromDataset());
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
    strSQL.Format("select * from songview join albumview on (songview.idAlbum = albumview.idAlbum) where albumview.idalbum in (select distinct albumview.idalbum from albumview join song on albumview.idAlbum=song.idAlbum where song.lastplayed NOT NULL order by song.lastplayed desc limit %i)", RECENTLY_ADDED_LIMIT);
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
      CFileItem *item = new CFileItem;
      GetFileItemFromDataset(item, strBaseDir);
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

bool CMusicDatabase::GetRecentlyAddedAlbums(VECALBUMS& albums)
{
  try
  {
    albums.erase(albums.begin(), albums.end());
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL;
    strSQL.Format("select * from albumview order by idAlbum desc limit %i", RECENTLY_ADDED_LIMIT);

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
      albums.push_back(GetAlbumFromDataset());
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

bool CMusicDatabase::GetRecentlyAddedAlbumSongs(const CStdString& strBaseDir, CFileItemList& items)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL;
    strSQL.Format("select songview.* from albumview join songview on (songview.idAlbum = albumview.idAlbum) where albumview.idalbum in ( select idAlbum from albumview order by idAlbum desc limit %i)", RECENTLY_ADDED_LIMIT);
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
      CFileItem *item = new CFileItem;
      GetFileItemFromDataset(item, strBaseDir);
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

    long songID = GetSongIDFromPath(strFileName);

    m_pDS->close();

    CStdString sql=FormatSQL("UPDATE song SET iTimesPlayed=iTimesPlayed+1, lastplayed=CURRENT_TIMESTAMP where idSong=%ld", songID);
    m_pDS->exec(sql.c_str());
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%s) failed", __FUNCTION__, strFileName.c_str());
  }

  return false;
}

bool CMusicDatabase::GetSongsByPath(const CStdString& strPath, CSongMap& songs, bool bAppendToMap)
{
  try
  {
    ASSERT(CUtil::HasSlashAtEnd(strPath));

    if (!bAppendToMap)
      songs.Clear();

    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL=FormatSQL("select * from songview where strPath like '%s'", strPath.c_str() );
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
  DWORD time = timeGetTime();
  // first grab all the artists that match
  SearchArtists(search, items);
  CLog::Log(LOGDEBUG, "%s Artist search in %lu ms", __FUNCTION__, timeGetTime() - time); time = timeGetTime();

  // then albums that match
  SearchAlbums(search, items);
  CLog::Log(LOGDEBUG, "%s Album search in %lu ms", __FUNCTION__, timeGetTime() - time); time = timeGetTime();

  // and finally songs
  SearchSongs(search, items);
  CLog::Log(LOGDEBUG, "%s Songs search in %lu ms", __FUNCTION__, timeGetTime() - time); time = timeGetTime();
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
      strSQL=FormatSQL("select * from songview where strTitle like '%s%%' or strTitle like '%% %s%%' limit 1000", search.c_str(), search.c_str());
    else
      strSQL=FormatSQL("select * from songview where strTitle like '%s%%' limit 1000", search.c_str());

    if (!m_pDS->query(strSQL.c_str())) return false;
    if (m_pDS->num_rows() == 0) return false;

    CStdString songLabel = g_localizeStrings.Get(179); // Song
    while (!m_pDS->eof())
    {
      CFileItem* item = new CFileItem;
      GetFileItemFromDataset(item, "musicdb://4/");
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
      strSQL=FormatSQL("select * from albumview where strAlbum like '%s%%' or strAlbum like '%% %s%%'", search.c_str(), search.c_str());
    else
      strSQL=FormatSQL("select * from albumview where strAlbum like '%s%%'", search.c_str());

    if (!m_pDS->query(strSQL.c_str())) return false;

    CStdString albumLabel(g_localizeStrings.Get(483)); // Album
    while (!m_pDS->eof())
    {
      CAlbum album = GetAlbumFromDataset();
      CStdString path;
      path.Format("musicdb://3/%ld/", album.idAlbum);
      CFileItem* pItem = new CFileItem(path, album);
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

long CMusicDatabase::SetAlbumInfo(long idAlbum, const CAlbum& album, const VECSONGS& songs)
{
  CStdString strSQL;
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    BeginTransaction();

    // and also the multiple genre string into single genres.
    CStdStringArray vecGenres; CStdString extraGenres;
    SplitString(album.strGenre, vecGenres, extraGenres);
    long lGenreId = AddGenre(vecGenres[0]);

    // delete any album info we may have
    strSQL=FormatSQL("delete from albuminfo where idAlbum=%i", idAlbum);
    m_pDS->exec(strSQL.c_str());

    // insert the albuminfo
    strSQL=FormatSQL("insert into albuminfo (idAlbumInfo,idAlbum,idGenre,strExtraGenres,strTones,strStyles,strReview,strImage,iRating,iYear) values(NULL,%i,%i,'%s','%s','%s','%s','%s',%i,%i)",
                  idAlbum, lGenreId, extraGenres.c_str(),
                  album.strTones.c_str(),
                  album.strStyles.c_str(),
                  album.strReview.c_str(),
                  album.strImage.c_str(),
                  album.iRating,
                  album.iYear);
    m_pDS->exec(strSQL.c_str());
    long idAlbumInfo = (long)sqlite3_last_insert_rowid(m_pDB->getHandle());

    if (SetAlbumInfoSongs(idAlbumInfo, songs))
      CommitTransaction();
    else
    {
      RollbackTransaction();
      idAlbumInfo = -1;
    }

    return idAlbumInfo;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed with query (%s)", __FUNCTION__, strSQL.c_str());
  }

  RollbackTransaction();

  return -1;
}

bool CMusicDatabase::SetAlbumInfoSongs(long idAlbumInfo, const VECSONGS& songs)
{
  CStdString strSQL;
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    strSQL=FormatSQL("delete from albuminfosong where idAlbumInfo=%i", idAlbumInfo);
    if (!m_pDS->exec(strSQL.c_str())) return false;

    for (int i = 0; i < (int)songs.size(); i++)
    {
      CSong song = songs[i];

      strSQL=FormatSQL("select * from albuminfosong where idAlbumInfo=%i and iTrack=%i", idAlbumInfo, song.iTrack);
      if (!m_pDS->query(strSQL.c_str())) continue;

      int iRowsFound = m_pDS->num_rows();
      if (iRowsFound != 0)
        continue;

      strSQL=FormatSQL("insert into albuminfosong (idAlbumInfoSong,idAlbumInfo,iTrack,strTitle,iDuration) values(NULL,%i,%i,'%s',%i)",
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
    CStdString strSQL=FormatSQL("select * from song join path on song.idPath = path.idPath where song.idSong in %s", strSongIds.c_str());
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return true;
    }
    CStdString strSongsToDelete = "(";
    while (!m_pDS->eof())
    { // get the full song path
      CStdString strFileName;
      CUtil::AddFileToFolder(m_pDS->fv("path.strPath").get_asString(), m_pDS->fv("song.strFileName").get_asString(), strFileName);

      //  Special case for streams inside an ogg file. (oggstream)
      //  The last dir in the path is the ogg file that
      //  contains the stream, so test if its there
      CStdString strExtension=CUtil::GetExtension(strFileName);
      if (strExtension==".oggstream" || strExtension==".nsfstream")
      {
        CStdString strFileAndPath=strFileName;
        CUtil::GetDirectory(strFileAndPath, strFileName);
        // we are dropping back to a file, so remove the slash at end
        CUtil::RemoveSlashAtEnd(strFileName);
      }

      if (!CFile::Exists(strFileName))
      { // file no longer exists, so add to deletion list
        strSongsToDelete += m_pDS->fv("song.idSong").get_asString() + ",";
      }
      m_pDS->next();
    }
    m_pDS->close();
    strSongsToDelete.TrimRight(",");
    strSongsToDelete += ")";
    // ok, now delete these songs + all references to them from the exartistsong and exgenresong tables
    strSQL = "delete from song where idSong in " + strSongsToDelete;
    m_pDS->exec(strSQL.c_str());
    strSQL = "delete from exartistsong where idSong in " + strSongsToDelete;
    m_pDS->exec(strSQL.c_str());
    strSQL = "delete from exgenresong where idSong in " + strSongsToDelete;
    m_pDS->exec(strSQL.c_str());
    m_pDS->close();
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
      CStdString strSQL=FormatSQL("select song.idsong from song order by song.idsong limit %i offset %i",iLIMIT,i);
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
    CStdString sql = "select strPath from path where idPath in (select idPath from song)";
    if (!m_pDS->query(sql.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      m_pDS->exec("delete from path");
      return true;
    }
    sql = "delete from path where ";
    while (!m_pDS->eof())
    {
      sql += FormatSQL("strPath not like substr('%s',0,length(strPath)) and ", m_pDS->fv("strPath").get_asString().c_str());
      m_pDS->next();
    }
    m_pDS->close();
    sql = sql.Left(sql.GetLength() - 4);
    m_pDS->exec(sql.c_str());
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
        ::DeleteFile(strThumb.c_str());
      }
      m_pDS->next();
    }
    // clear the thumb cache
    //CUtil::ThumbCacheClear();
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
    long lVariousArtistsId = AddArtist(strVariousArtists);
    CStdString strSQL = "delete from artist where idArtist not in (select idArtist from song)";
    strSQL += " and idArtist not in (select idArtist from exartistsong)";
    strSQL += " and idArtist not in (select idArtist from album)";
    strSQL += " and idArtist not in (select idArtist from exartistalbum)";
    CStdString strSQL2;
    strSQL2.Format(" and idArtist<>%i", lVariousArtistsId);
    strSQL += strSQL2;
    m_pDS->exec(strSQL.c_str());
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
  // first cleanup any songs with invalid paths
  pDlgProgress->SetHeading(700);
  pDlgProgress->SetLine(0, "");
  pDlgProgress->SetLine(1, 318);
  pDlgProgress->SetLine(2, 330);
  pDlgProgress->SetPercentage(0);
  pDlgProgress->StartModal();
  pDlgProgress->ShowProgressBar(true);

  if (!CleanupSongs())
  {
    RollbackTransaction();
    return ERROR_REORG_SONGS;
  }
  // then the albums that are not linked to a song or to albuminfo, or whose path is removed
  pDlgProgress->SetLine(1, 326);
  pDlgProgress->SetPercentage(20);
  pDlgProgress->Progress();
  if (!CleanupAlbums())
  {
    RollbackTransaction();
    return ERROR_REORG_ALBUM;
  }
  // now the paths
  pDlgProgress->SetLine(1, 324);
  pDlgProgress->SetPercentage(40);
  pDlgProgress->Progress();
  if (!CleanupPaths() || !CleanupThumbs())
  {
    RollbackTransaction();
    return ERROR_REORG_PATH;
  }
  // and finally artists + genres
  pDlgProgress->SetLine(1, 320);
  pDlgProgress->SetPercentage(60);
  pDlgProgress->Progress();
  if (!CleanupArtists())
  {
    RollbackTransaction();
    return ERROR_REORG_ARTIST;
  }
  pDlgProgress->SetLine(1, 322);
  pDlgProgress->SetPercentage(80);
  pDlgProgress->Progress();
  if (!CleanupGenres())
  {
    RollbackTransaction();
    return ERROR_REORG_GENRE;
  }
  // commit transaction
  pDlgProgress->SetLine(1, 328);
  pDlgProgress->SetPercentage(90);
  pDlgProgress->Progress();
  if (!CommitTransaction())
  {
    RollbackTransaction();
    return ERROR_WRITING_CHANGES;
  }
  // and compress the database
  pDlgProgress->SetLine(1, 331);
  pDlgProgress->SetPercentage(100);
  pDlgProgress->Progress();
  if (!Compress())
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
  CGUIDialogMusicScan* dlgMusicScan = (CGUIDialogMusicScan*)m_gWindowManager.GetWindow(WINDOW_DIALOG_MUSIC_SCAN);
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
    album.idAlbum = m_pDS->fv("album.idAlbum").get_asLong() ;
    album.strAlbum = m_pDS->fv("album.strAlbum").get_asString();
    album.strArtist = m_pDS->fv("artist.strArtist").get_asString();
    album.strArtist += m_pDS->fv("album.strExtraArtists").get_asString();
    vecAlbums.push_back(album);
    m_pDS->next();
  }
  m_pDS->close();

  // Show a selectdialog that the user can select the albuminfo to delete
  CGUIDialogSelect *pDlg = (CGUIDialogSelect*)m_gWindowManager.GetWindow(WINDOW_DIALOG_SELECT);
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
    strSQL=FormatSQL("delete from albuminfo where albuminfo.idAlbum=%i", album.idAlbum);
    if (!m_pDS->exec(strSQL.c_str())) return ;

    vecAlbums.erase(vecAlbums.begin(), vecAlbums.end());
  }
}

bool CMusicDatabase::LookupCDDBInfo(bool bRequery/*=false*/)
{
  if (!g_guiSettings.GetBool("musicfiles.usecddb") || !g_guiSettings.GetBool("network.enableinternet"))
    return false;

  // Get information for the inserted disc
  CCdInfo* pCdInfo = CDetectDVDMedia::GetCdInfo();
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
    strFile.Format("%s\\%x.cddb", g_settings.GetCDDBFolder().c_str(), pCdInfo->GetCddbDiscId());
    ::DeleteFile(strFile.c_str());
  }

  // Prepare cddb
  Xcddb cddb;
  cddb.setCacheDir(g_settings.GetCDDBFolder());

  // Do we have to look for cddb information
  if (pCdInfo->HasCDDBInfo() && !cddb.isCDCached(pCdInfo))
  {
    CGUIDialogProgress* pDialogProgress = (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
    CGUIDialogSelect *pDlgSelect = (CGUIDialogSelect*)m_gWindowManager.GetWindow(WINDOW_DIALOG_SELECT);

    if (!pDialogProgress) return false;
    if (!pDlgSelect) return false;

    // Show progress dialog if we have to connect to freedb.org
    pDialogProgress->SetHeading(255); //CDDB
    pDialogProgress->SetLine(0, ""); // Querying freedb for CDDB info
    pDialogProgress->SetLine(1, 256);
    pDialogProgress->SetLine(2, "");
    pDialogProgress->ShowProgressBar(false);
    pDialogProgress->StartModal();
    while (pDialogProgress->IsAnimating(ANIM_TYPE_WINDOW_OPEN))
      pDialogProgress->Progress();

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

        pDialogProgress->Close();
      }
      else if (lasterror == E_NO_MATCH_FOUND)
      {
        pCdInfo->SetNoCDDBInfo();
        pDialogProgress->Close();
      }
      else
      {
        pCdInfo->SetNoCDDBInfo();
        pDialogProgress->Close();
        // ..no, an error occured, display it to the user
        CGUIDialogOK *pDialogOK = (CGUIDialogOK *)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
        if (pDialogOK)
        {
          CStdString strErrorText;
          strErrorText.Format("[%d] %s", cddb.getLastError(), cddb.getLastErrorText());

          pDialogOK->SetHeading(255);
          pDialogOK->SetLine(0, 257); //ERROR
          pDialogOK->SetLine(1, strErrorText.c_str() );
          pDialogOK->SetLine(2, "");
          pDialogOK->DoModal();
        }
      }
    } // if ( !cddb.queryCDinfo( pCdInfo ) )
    pDialogProgress->Close();
  } // if (pCdInfo->HasCDDBInfo() && g_stSettings.m_bUseCDDB)

  // Filling the file items with cddb info happens in CMusicInfoTagLoaderCDDA

  return pCdInfo->HasCDDBInfo();
}

void CMusicDatabase::DeleteCDDBInfo()
{
  WIN32_FIND_DATA wfd;
  memset(&wfd, 0, sizeof(wfd));

  CStdString strCDDBFileMask;
  strCDDBFileMask.Format("%s\\*.cddb", g_settings.GetCDDBFolder().c_str());

  map<ULONG, CStdString> mapCDDBIds;

  CAutoPtrFind hFind( FindFirstFile(strCDDBFileMask.c_str(), &wfd));
  if (!hFind.isValid())
  {
    CGUIDialogOK::ShowAndGetInput(313, 426, 0, 0);
    return ;
  }

  // Show a selectdialog that the user can select the albuminfo to delete
  CGUIDialogSelect *pDlg = (CGUIDialogSelect*)m_gWindowManager.GetWindow(WINDOW_DIALOG_SELECT);
  if (pDlg)
  {
    pDlg->SetHeading(g_localizeStrings.Get(181).c_str());
    pDlg->Reset();
    CStdString strDir = g_settings.GetCDDBFolder();
    do
    {
      if ( !(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
      {
        CStdString strFile = wfd.cFileName;
        strFile.Delete(strFile.size() - 5, 5);
        ULONG lDiscId = strtoul(strFile.c_str(), NULL, 16);
        Xcddb cddb;
        cddb.setCacheDir(strDir);

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
    }
    while (FindNextFile(hFind, &wfd));

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
        strFile.Format("%s\\%x.cddb", g_settings.GetCDDBFolder().c_str(), it->first );
        ::DeleteFile(strFile.c_str());
        break;
      }
    }
    mapCDDBIds.erase(mapCDDBIds.begin(), mapCDDBIds.end());
  }
}

void CMusicDatabase::Clean()
{
  // If we are scanning for music info in the background,
  // other writing access to the database is prohibited.
  CGUIDialogMusicScan* dlgMusicScan = (CGUIDialogMusicScan*)m_gWindowManager.GetWindow(WINDOW_DIALOG_MUSIC_SCAN);
  if (dlgMusicScan->IsDialogRunning())
  {
    CGUIDialogOK::ShowAndGetInput(189, 14057, 0, 0);
    return;
  }

  if (CGUIDialogYesNo::ShowAndGetInput(313, 333, 0, 0))
  {
    CGUIDialogProgress* dlgProgress = (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
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
      CFileItem* pItem=new CFileItem(m_pDS->fv("strGenre").get_asString());
      pItem->GetMusicInfoTag()->SetGenre(m_pDS->fv("strGenre").get_asString());
      CStdString strDir;
      strDir.Format("%ld/", m_pDS->fv("idGenre").get_asLong());
      pItem->m_strPath=strBaseDir + strDir;
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
      CFileItem* pItem=new CFileItem(m_pDS->fv("iYear").get_asString());
      SYSTEMTIME stTime;
      stTime.wYear = (WORD)m_pDS->fv("iYear").get_asLong();
      pItem->GetMusicInfoTag()->SetReleaseDate(stTime);
      CStdString strDir;
      strDir.Format("%ld/", m_pDS->fv("iYear").get_asLong());
      pItem->m_strPath=strBaseDir + strDir;
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

bool CMusicDatabase::GetAlbumsByYear(const CStdString& strBaseDir, CFileItemList& items, long year)
{
  CStdString where = FormatSQL("where iYear=%ld", year);

  return GetAlbumsByWhere(strBaseDir, where, items);
}

bool CMusicDatabase::GetArtistsNav(const CStdString& strBaseDir, CFileItemList& items, long idGenre, bool albumArtistsOnly)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

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
                          "where album.strExtraArtists != ''";                      // then exclude those where that have no extra artists
      strSQL +=           ")"
                        ") ";
    }
    else
    { // same statements as above, but limit to the specified genre
      // in this case we show the whole lot always - there is no limitation to just album artists
      strSQL+=FormatSQL("("
                        "select song.idArtist from song " // All primary artists linked to primary genres
                        "where song.idGenre=%ld"
                        ") "
                      "or idArtist IN "
                        "("
                        "select song.idArtist from song " // All primary artists linked to extra genres
                          "join exgenresong on song.idSong=exgenresong.idSong "
                        "where exgenresong.idGenre=%ld"
                        ")"
                      "or idArtist IN "
                        "("
                        "select exartistsong.idArtist from exartistsong " // All extra artists linked to extra genres
                          "join song on exartistsong.idSong=song.idSong "
                          "join exgenresong on song.idSong=exgenresong.idSong "
                        "where exgenresong.idGenre=%ld"
                        ") "
                      "or idArtist IN "
                        "("
                        "select exartistsong.idArtist from exartistsong " // All extra artists linked to primary genres
                          "join song on exartistsong.idSong=song.idSong "
                        "where song.idGenre=%ld"
                        ") "
                      "or idArtist IN "
                      , idGenre, idGenre, idGenre, idGenre);
      // and add any artists linked to an album (may be different from above due to album artist tag)
      strSQL += FormatSQL("("
                          "select album.idArtist from album " // All primary album artists linked to primary genres
                          "where album.idGenre=%ld"
                          ") "
                        "or idArtist IN "
                          "("
                          "select album.idArtist from album " // All primary album artists linked to extra genres
                            "join exgenrealbum on album.idAlbum=exgenrealbum.idAlbum "
                          "where exgenrealbum.idGenre=%ld"
                          ")"
                        "or idArtist IN "
                          "("
                          "select exartistalbum.idArtist from exartistalbum " // All extra album artists linked to extra genres
                            "join album on exartistalbum.idAlbum=album.idAlbum "
                            "join exgenrealbum on album.idAlbum=exgenrealbum.idAlbum "
                          "where exgenrealbum.idGenre=%ld"
                          ") "
                        "or idArtist IN "
                          "("
                          "select exartistalbum.idArtist from exartistalbum " // All extra album artists linked to primary genres
                            "join album on exartistalbum.idAlbum=album.idAlbum "
                          "where album.idGenre=%ld"
                          ") "
                        ")", idGenre, idGenre, idGenre, idGenre);
    }

    // remove the null string
    strSQL += " and artist.strArtist != \"\"";
    // and the various artist entry if applicable
    if (!albumArtistsOnly || idGenre > -1)  
    {
      CStdString strVariousArtists = g_localizeStrings.Get(340);
      long lVariousArtistsId = AddArtist(strVariousArtists);
      strSQL+=FormatSQL(" and artist.idArtist<>%i", lVariousArtistsId);
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
      CFileItem* pItem=new CFileItem(m_pDS->fv("strArtist").get_asString());
      pItem->GetMusicInfoTag()->SetArtist(m_pDS->fv("strArtist").get_asString());
      CStdString strDir;
      strDir.Format("%ld/", m_pDS->fv("idArtist").get_asLong());
      pItem->m_strPath=strBaseDir + strDir;
      pItem->m_bIsFolder=true;
      if (CFile::Exists(pItem->GetCachedArtistThumb()))
        pItem->SetThumbnailImage(pItem->GetCachedArtistThumb());
      else
        pItem->SetThumbnailImage("DefaultArtistBig.png");
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

bool CMusicDatabase::GetAlbumFromSong(long idSong, CAlbum &album)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL = FormatSQL("select albumview.* from song join albumview on song.idAlbum = albumview.idAlbum where song.idSong='%ld'", idSong);
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound != 1)
    {
      m_pDS->close();
      return false;
    }

    album = GetAlbumFromDataset();

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
    CUtil::Split(song.strFileName, path, file);

    CStdString strSQL = FormatSQL("select albumview.* from song join albumview on song.idAlbum = albumview.idAlbum join path on song.idPath = path.idPath where song.strFileName like '%s' and path.strPath like '%s'", file.c_str(), path.c_str());
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound != 1)
    {
      m_pDS->close();
      return false;
    }

    album = GetAlbumFromDataset();

    m_pDS->close();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}

bool CMusicDatabase::GetAlbumsNav(const CStdString& strBaseDir, CFileItemList& items, long idGenre, long idArtist)
{
  // where clause
  CStdString strWhere;
  if (idGenre!=-1)
  {
    strWhere+=FormatSQL("where (idAlbum IN "
                          "("
                          "select song.idAlbum from song " // All albums where the primary genre fits
                          "where song.idGenre=%ld"
                          ") "
                        "or idAlbum IN "
                          "("
                          "select song.idAlbum from song " // All albums where extra genres fits
                            "join exgenresong on song.idSong=exgenresong.idSong "
                          "where exgenresong.idGenre=%ld"
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

    strWhere +=FormatSQL("(idAlbum IN "
                            "("
                              "select song.idAlbum from song "  // All albums where the primary artist fits
                              "where song.idArtist=%ld"
                            ")"
                          " or idAlbum IN "
                            "("
                              "select song.idAlbum from song "  // All albums where extra artists fit
                                "join exartistsong on song.idSong=exartistsong.idSong "
                              "where exartistsong.idArtist=%ld"
                            ")"
                          " or idAlbum IN "
                            "("
                              "select album.idAlbum from album " // All albums where primary album artist fits
                              "where album.idArtist=%ld"
                            ")"
                          " or idAlbum IN "
                            "("
                              "select exartistalbum.idAlbum from exartistalbum " // All albums where extra album artists fit
                              "where exartistalbum.idArtist=%ld"
                            ")"
                          ") "
                          , idArtist, idArtist, idArtist, idArtist);
  }

  return GetAlbumsByWhere(strBaseDir, strWhere, items);
}

bool CMusicDatabase::GetAlbumsByWhere(const CStdString &baseDir, const CStdString &where, CFileItemList &items)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString sql = "select * from albumview " + where;

    // block null album names
    if (where.IsEmpty())
      sql += "where ";
    else
      sql += " and ";
    sql += "albumview.strAlbum != \"\"";

    // run query
    CLog::Log(LOGDEBUG, "%s query: %s", __FUNCTION__, sql.c_str());
    if (!m_pDS->query(sql.c_str())) return false;
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
      strDir.Format("%s%ld/", baseDir.c_str(), m_pDS->fv("idAlbum").get_asLong());
      CFileItem* pItem=new CFileItem(strDir, GetAlbumFromDataset());
      items.Add(pItem);

      m_pDS->next();
    }

    // cleanup
    m_pDS->close();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, where.c_str());
  }
  return false;
}

bool CMusicDatabase::GetSongsByWhere(const CStdString &baseDir, const CStdString &whereClause, CFileItemList &items)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    // We don't use FormatSQL here, as the WHERE clause is already formatted.
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
    items.Reserve(iRowsFound);
    // get songs from returned subtable
    int count = 0;
    while (!m_pDS->eof())
    {
      CFileItem *item = new CFileItem;
      GetFileItemFromDataset(item, baseDir);
      // HACK for sorting by database returned order
      item->m_iprogramCount = ++count;
      items.Add(item);
      m_pDS->next();
    }

    // cleanup
    m_pDS->close();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%s) failed", __FUNCTION__, whereClause.c_str());
  }
  return false;
}

bool CMusicDatabase::GetSongsByYear(const CStdString& baseDir, CFileItemList& items, long year)
{
  CStdString where=FormatSQL("where (iYear=%ld)", year);
  return GetSongsByWhere(baseDir, where, items);
}

bool CMusicDatabase::GetSongsNav(const CStdString& strBaseDir, CFileItemList& items, long idGenre, long idArtist,long idAlbum)
{
  try
  {
    DWORD time = timeGetTime();

    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strWhere;
    CStdString strSQL = "select * from songview ";

    if (idAlbum!=-1)
      strWhere=FormatSQL("where (idAlbum=%ld) ", idAlbum);

    if (idGenre!=-1)
    {
      if (strWhere.IsEmpty())
        strWhere += "where ";
      else
        strWhere += "and ";

      strWhere += FormatSQL("(idGenre=%ld " // All songs where primary genre fits
                            "or idSong IN "
                              "("
                              "select exgenresong.idSong from exgenresong " // All songs by where extra genres fit
                              "where exgenresong.idGenre=%ld"
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

      strWhere += FormatSQL("(idArtist=%ld " // All songs where primary artist fits
                            "or idSong IN "
                              "("
                              "select exartistsong.idSong from exartistsong " // All songs where extra artists fit
                              "where exartistsong.idArtist=%ld"
                              ")"
                            "or idSong IN "
                              "("
                              "select song.idSong from song " // All songs where the primary album artist fits
                              "join album on song.idAlbum=album.idAlbum "
                              "where album.idArtist=%ld"
                              ")"
                            "or idSong IN "
                              "("
                              "select song.idSong from song " // All songs where the extra album artist fit, excluding
                              "join exartistalbum on song.idAlbum=exartistalbum.idAlbum " // various artist albums
                              "join album on song.idAlbum=album.idAlbum "
                              "where exartistalbum.idArtist=%ld and album.strExtraArtists != ''"
                              ")"
                            ") "
                            , idArtist, idArtist, idArtist, idArtist);
    }

    strSQL += strWhere;

    // get all songs out of the database in fixed size chunks
    // dont reserve the items ahead of time just in case it fails part way though
    if (idAlbum == -1 && idArtist == -1 && idGenre == -1)
    {
      int iLIMIT = 5000;    // chunk size
      int iSONGS = 0;       // number of songs added to items
      int iITERATIONS = 0;  // number of iterations
      for (int i=0;;i+=iLIMIT)
      {
        CStdString strSQL2=FormatSQL("%s limit %i offset %i", strSQL.c_str(), iLIMIT, i);
        CLog::Log(LOGDEBUG, "%s query: %s", __FUNCTION__, strSQL2.c_str());
        try
        {
          if (!m_pDS->query(strSQL2.c_str()))
            return false;

          // keep going until no rows are left!
          int iRowsFound = m_pDS->num_rows();
          if (iRowsFound == 0)
          {
            m_pDS->close();
            if (iITERATIONS == 0)
              return false; // failed on first iteration, so there's probably no songs in the db
            else
              return true; // there no more songs left to process (aborts the unbounded for loop)
          }

          // get songs from returned subtable
          while (!m_pDS->eof())
          {
            CFileItem *item = new CFileItem;
            GetFileItemFromDataset(item, strBaseDir);
            items.Add(item);
            iSONGS++;
            m_pDS->next();
          }
        }
        catch (...)
        {
          CLog::Log(LOGERROR, "%s failed at iteration %i, num songs %i", __FUNCTION__, iITERATIONS, iSONGS);

          if (iSONGS > 0)
            return true; // keep whatever songs we may have gotten before the failure
          else
            return false; // no songs, return false
        }
        // next iteration
        iITERATIONS++;
        m_pDS->close();
      }
      return true;
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

    CLog::Log(LOGDEBUG,"Time for actual SQL query = %lu", timeGetTime() - time); time = timeGetTime();

    // get data from returned rows
    items.Reserve(iRowsFound);

    while (!m_pDS->eof())
    {
      CFileItem *item = new CFileItem;
      GetFileItemFromDataset(item, strBaseDir);
      items.Add(item);

      m_pDS->next();
    }

    CLog::Log(LOGDEBUG,"Time to retrieve songs from dataset = %lu", timeGetTime() - time);

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

bool CMusicDatabase::UpdateOldVersion(int version)
{
  if (NULL == m_pDB.get()) return false;
  if (NULL == m_pDS.get()) return false;
  if (NULL == m_pDS2.get()) return false;

  try
  {
    if (version < 6)
    {
      // add the strHash column to path table
      m_pDS->exec("alter table path add strHash text");
    }
    if (version < 7)
    {
      // add the rating and comment columns to the song table (and song view)
      m_pDS->exec("alter table song add rating char default '0'");
      m_pDS->exec("alter table song add comment text");

      // drop the song view
      m_pDS->exec("drop view songview");

      // and re-add it with the updated columns.
      m_pDS->exec("create view songview as select idSong, song.strExtraArtists as strExtraArtists, song.strExtraGenres as strExtraGenres, strTitle, iTrack, iDuration, song.iYear as iYear, dwFileNameCRC, strFileName, strMusicBrainzTrackID, strMusicBrainzArtistID, strMusicBrainzAlbumID, strMusicBrainzAlbumArtistID, strMusicBrainzTRMID, iTimesPlayed, iStartOffset, iEndOffset, lastplayed, rating, comment, song.idAlbum as idAlbum, strAlbum, strPath, song.idArtist as idArtist, strArtist, song.idGenre as idGenre, strGenre, strThumb from song join album on song.idAlbum=album.idAlbum join path on song.idPath=path.idPath join artist on song.idArtist=artist.idArtist join genre on song.idGenre=genre.idGenre join thumb on song.idThumb=thumb.idThumb");
    }

    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "Error attempting to update the database version!");
    return false;
  }
  return true;
}

long CMusicDatabase::AddThumb(const CStdString& strThumb1)
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

    strSQL=FormatSQL( "select * from thumb where strThumb='%s'", strThumb.c_str());
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      // doesnt exists, add it
      strSQL=FormatSQL("insert into thumb (idThumb, strThumb) values( NULL, '%s' )", strThumb.c_str());
      m_pDS->exec(strSQL.c_str());

      int idPath = (long)sqlite3_last_insert_rowid(m_pDB->getHandle());
      m_thumbCache.insert(pair<CStdString, int>(strThumb1, idPath));
      return idPath;
    }
    else
    {
      int idPath = m_pDS->fv("idThumb").get_asLong();
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

unsigned int CMusicDatabase::GetSongIDs(const CStdString& strWhere, vector<pair<int,long> > &songIDs)
{
  try
  {
    if (NULL == m_pDB.get()) return 0;
    if (NULL == m_pDS.get()) return 0;

    CStdString strSQL = "select idsong from songview " + strWhere;
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
      songIDs.push_back(make_pair<int,long>(1,m_pDS->fv(song_idSong).get_asLong()));
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

int CMusicDatabase::GetSongsCount()
{
  return GetSongsCount((CStdString)"");
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

    int iNumSongs = m_pDS->fv("NumSongs").get_asLong();
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

bool CMusicDatabase::GetAlbumPath(long idAlbum, CStdString& path)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    path.Empty();

    CStdString strSQL=FormatSQL("select distinct strPath from song join path on song.idPath = path.idPath where song.idAlbum=%ld", idAlbum);
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return false;
    }

    // if this returns more than one path, we just grab the first one.  It's just for determining where to obtain + place
    // a local thumbnail
    path = m_pDS->fv("strPath").get_asString();

    m_pDS->close(); // cleanup recordset data
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%ld) failed", __FUNCTION__, idAlbum);
  }

  return false;
}


bool CMusicDatabase::SaveAlbumThumb(long idAlbum, const CStdString& strThumb)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    long idThumb=AddThumb(strThumb);

    if (idThumb>-1)
    {
      CStdString strSQL=FormatSQL("UPDATE album SET idThumb=%ld where idAlbum=%ld", idThumb, idAlbum);
      CLog::Log(LOGDEBUG, "%s exec: %s", __FUNCTION__, strSQL.c_str());
      m_pDS->exec(strSQL.c_str());
      strSQL=FormatSQL("UPDATE song SET idThumb=%ld where idAlbum=%ld", idThumb, idAlbum);
      CLog::Log(LOGDEBUG, "%s exec: %s", __FUNCTION__, strSQL.c_str());
      m_pDS->exec(strSQL.c_str());
      return true;
    }
    return false;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%ld) failed", __FUNCTION__, idAlbum);
  }

  return false;
}

bool CMusicDatabase::GetArtistPath(long idArtist, CStdString &basePath)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    // find all albums from this artist, and all the paths to the songs from those albums
    CStdString strSQL=FormatSQL("select distinct strPath from album join song on album.idAlbum = song.idAlbum join path on song.idPath = path.idPath "
                                "where album.idAlbum in (select idAlbum from album where album.idArtist=%ld) "
                                "or album.idAlbum in (select idAlbum from exartistalbum where exartistalbum.idArtist = %ld)", idArtist, idArtist);

    // run query
    CLog::Log(LOGDEBUG, "%s query: %s", __FUNCTION__, strSQL.c_str());
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return false;
    }
    
    // special case for single path - assume that we're in an artist/album/songs filesystem
    if (iRowsFound == 1)
    {
      CUtil::GetParentPath(m_pDS->fv("strPath").get_asString(), basePath);
      m_pDS->close();
      return true;
    }

    // find the common path (if any) to these albums
    basePath.Empty();
    while (!m_pDS->eof())
    {
      CStdString path = m_pDS->fv("strPath").get_asString();
      if (basePath.IsEmpty())
        basePath = path;
      else
      {
        // find the common path of basePath and path
        unsigned int i = 1;
        while (i <= min(basePath.size(), path.size()) && strnicmp(basePath.c_str(), path.c_str(), i) == 0)
          i++;
        basePath = basePath.Left(i - 1);
        // they should at least share a / at the end, though for things such as path/cd1 and path/cd2 there won't be
        if (!CUtil::HasSlashAtEnd(basePath))
        {
          // currently GetDirectory() removes trailing slashes
          CUtil::GetDirectory(basePath, basePath);
          CUtil::AddSlashAtEnd(basePath);
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

long CMusicDatabase::GetArtistByName(const CStdString& strArtist)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL=FormatSQL("select idArtist from artist where artist.strArtist like '%s'", strArtist.c_str());

    // run query
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound != 1)
    {
      m_pDS->close();
      return -1;
    }
    long lResult = m_pDS->fv("artist.idArtist").get_asLong();
    m_pDS->close();
    return lResult;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return -1;
}

long CMusicDatabase::GetAlbumByName(const CStdString& strAlbum)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL=FormatSQL("select idAlbum from album where album.strAlbum like '%s'", strAlbum.c_str());
    // run query
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound != 1)
    {
      m_pDS->close();
      return -1;
    }
    return m_pDS->fv("album.idAlbum").get_asLong();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return -1;
}

bool CMusicDatabase::GetGenreById(long idGenre, CStdString& strGenre)
{
  strGenre = "";
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL=FormatSQL("select strGenre from genre where genre.idGenre = %ld", idGenre);

    // run query
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound != 1)
    {
      m_pDS->close();
      return false;
    }
    strGenre = m_pDS->fv("genre.strGenre").get_asString();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}

bool CMusicDatabase::GetArtistById(long idArtist, CStdString& strArtist)
{
  strArtist = "";
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL=FormatSQL("select strArtist from artist where artist.idArtist = %ld", idArtist);

    // run query
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound != 1)
    {
      m_pDS->close();
      return false;
    }
    strArtist = m_pDS->fv("artist.strArtist").get_asString();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}

bool CMusicDatabase::GetAlbumById(long idAlbum, CStdString& strAlbum)
{
  strAlbum = "";
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL=FormatSQL("select strAlbum from album where album.idAlbum = %ld", idAlbum);

    // run query
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound != 1)
    {
      m_pDS->close();
      return false;
    }
    strAlbum = m_pDS->fv("album.strAlbum").get_asString();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}

bool CMusicDatabase::GetRandomSong(CFileItem* item, long& lSongId, const CStdString& strWhere)
{
  try
  {
    lSongId = -1;

    // seed random function
    srand(timeGetTime());

    int iCount = GetSongsCount(strWhere);
    if (iCount <= 0)
      return false;
    int iRandom = rand() % iCount;

    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    // We don't use FormatSQL here, as the WHERE clause is already formatted.
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
    lSongId = m_pDS->fv("songview.idSong").get_asLong();
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
    long idVariousArtists=AddArtist(strVariousArtists);
    if (idVariousArtists<0)
      return false;

    CStdString strSQL = FormatSQL("select * from albumview where idArtist=%ld", idVariousArtists);

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
      strDir.Format("%s%ld/", strBaseDir.c_str(), m_pDS->fv("idAlbum").get_asLong());
      CFileItem* pItem=new CFileItem(strDir, GetAlbumFromDataset());
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
    long idVariousArtists=AddArtist(strVariousArtists);
    if (idVariousArtists<0)
      return false;

    CStdString strSQL = FormatSQL("select * from songview where idAlbum IN (select idAlbum from album where idArtist=%ld)", idVariousArtists);

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
      CFileItem *item = new CFileItem;
      GetFileItemFromDataset(item, strBaseDir);
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
    long pathId = AddPath(path);
    if (pathId < 0) return false;

    CStdString strSQL=FormatSQL("update path set strHash='%s' where idPath=%ld", hash.c_str(), pathId);
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
  CStdString strSQL;
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL=FormatSQL("select strHash from path where strPath like '%s'", path.c_str());
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

bool CMusicDatabase::RemoveSongsFromPath(const CStdString &path, CSongMap &songs)
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
  try
  {
    ASSERT(CUtil::HasSlashAtEnd(path));

    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString sql=FormatSQL("select * from songview where strPath like '%s'", path.c_str() );
    if (!m_pDS->query(sql.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound > 0)
    {
      CStdString songIds = "(";
      while (!m_pDS->eof())
      {
        CSong song = GetSongFromDataset();
        songs.Add(song.strFileName, song);
        songIds += FormatSQL("%i,", song.idSong);
        m_pDS->next();
      }
      songIds.TrimRight(",");
      songIds += ")";

      m_pDS->close();

      // and delete all songs, exartistsongs and exgenresongs
      sql = "delete from song where idSong in " + songIds;
      m_pDS->exec(sql.c_str());
      sql = "delete from exartistsong where idSong in " + songIds;
      m_pDS->exec(sql.c_str());
      sql = "delete from exgenresong where idSong in " + songIds;
      m_pDS->exec(sql.c_str());
    }
    // and remove the path as well (it'll be re-added later on with the new hash if it's non-empty)
    sql = FormatSQL("delete from path where strPath like '%s'", path.c_str());
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

    long songID = GetSongIDFromPath(filePath);
    if (-1 == songID) return false;

    CStdString sql = FormatSQL("update song set rating='%c' where idSong = %i", rating, songID);
    m_pDS->exec(sql.c_str());
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s,%c) failed", __FUNCTION__, filePath.c_str(), rating);
  }
  return false;
}

long CMusicDatabase::GetSongIDFromPath(const CStdString &filePath)
{
  // grab the where string to identify the song id
  CURL url(filePath);
  if (url.GetProtocol()=="musicdb")
  {
    CStdString strFile=CUtil::GetFileName(filePath);
    CUtil::RemoveExtension(strFile);
    return atol(strFile.c_str());
  }
  // hit the db
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;
    CStdString strPath;
    CUtil::GetDirectory(filePath, strPath);
    CUtil::AddSlashAtEnd(strPath);

    DWORD crc = ComputeCRC(filePath);

    CStdString sql = FormatSQL("select idSong from song join path on song.idPath = path.idPath where song.dwFileNameCRC='%ul'and path.strPath='%s'", crc, strPath.c_str());
    if (!m_pDS->query(sql.c_str())) return -1;

    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      return -1;
    }

    long songID = m_pDS->fv("idSong").get_asLong();
    m_pDS->close();
    return songID;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, filePath.c_str());
  }
  return -1;
}
