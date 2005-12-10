#include "stdafx.h"
#include "musicdatabase.h"
#include "filesystem/cddb.h"
#include "filesystem/directorycache.h"
#include "GUIDialogMusicScan.h"

#define MUSIC_DATABASE_VERSION 1.3f
#define MUSIC_DATABASE_NAME "MyMusic6.db"

using namespace CDDB;

CMusicDatabase g_musicDatabase;

CMusicDatabase::CMusicDatabase(void)
{
  m_fVersion=MUSIC_DATABASE_VERSION;
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
    m_pDS->exec("CREATE TABLE album ( idAlbum integer primary key, strAlbum text, idArtist integer, iNumArtists integer, idPath integer)\n");
    CLog::Log(LOGINFO, "create genre table");
    m_pDS->exec("CREATE TABLE genre ( idGenre integer primary key, strGenre text)\n");
    CLog::Log(LOGINFO, "create path table");
    m_pDS->exec("CREATE TABLE path ( idPath integer primary key,  strPath text)\n");
    CLog::Log(LOGINFO, "create song table");
    m_pDS->exec("CREATE TABLE song ( idSong integer primary key, idAlbum integer, idPath integer, idArtist integer, iNumArtists integer, idGenre integer, iNumGenres integer, strTitle text, iTrack integer, iDuration integer, iYear integer, dwFileNameCRC text, strFileName text, strMusicBrainzTrackID text, strMusicBrainzArtistID text, strMusicBrainzAlbumID text, strMusicBrainzAlbumArtistID text, strMusicBrainzTRMID text, iTimesPlayed integer, iStartOffset integer, iEndOffset integer, idThumb integer, lastplayed text default NULL)\n");
    CLog::Log(LOGINFO, "create albuminfo table");
    m_pDS->exec("CREATE TABLE albuminfo ( idAlbumInfo integer primary key, idAlbum integer, iYear integer, idGenre integer, iNumGenres integer, strTones text, strStyles text, strReview text, strImage text, iRating integer)\n");
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
    m_pDS->exec("create view songview as select idSong, song.iNumArtists as iNumArtists, song.iNumGenres as iNumGenres, strTitle, iTrack, iDuration, iYear, dwFileNameCRC, strFileName, strMusicBrainzTrackID, strMusicBrainzArtistID, strMusicBrainzAlbumID, strMusicBrainzAlbumArtistID, strMusicBrainzTRMID, iTimesPlayed, iStartOffset, iEndOffset, lastplayed, strAlbum, strPath, strArtist, strGenre, strThumb from song join album on song.idAlbum=album.idAlbum join path on song.idPath=path.idPath join artist on song.idArtist=artist.idArtist join genre on song.idGenre=genre.idGenre join thumb on song.idThumb=thumb.idThumb");
    CLog::Log(LOGINFO, "create album view");
    m_pDS->exec("create view albumview as select idAlbum, strAlbum, iNumArtists, strArtist, strPath from album join path on album.idPath=path.idPath join artist on album.idArtist=artist.idArtist");
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "musicbase::unable to create tables:%i", GetLastError());
    return false;
  }

  return true;
}

void CMusicDatabase::AddSong(const CSong& song, bool bCheck)
{
  CStdString strSQL;
  try
  {
    CStdString strPath, strFileName;
    CUtil::Split(song.strFileName, strPath, strFileName);
    if (CUtil::HasSlashAtEnd(strPath))
      strPath.Delete(strPath.size() - 1);

    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    // split our (possibly) multiple artist string into individual artists
    CStdStringArray vecArtists;
    int iNumArtists = StringUtils::SplitString(song.strArtist, " / ", vecArtists);
    // and the same for our genres
    CStdStringArray vecGenres;
    int iNumGenres = StringUtils::SplitString(song.strGenre, " / ", vecGenres);
    // add the primary artist/genre (and pop off of the vectors)
    long lArtistId = AddArtist(vecArtists[0]);
    long lGenreId = AddGenre(vecGenres[0]);

    long lPathId = AddPath(strPath);
    long lThumbId = AddThumb(song.strThumb);
    long lAlbumId = AddAlbum(song.strAlbum, lArtistId, iNumArtists, song.strArtist, lPathId, strPath);

    Crc32 crc;
    crc.Compute(song.strFileName);

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

      strSQL=FormatSQL("insert into song (idSong,idAlbum,idPath,idArtist,iNumArtists,idGenre,iNumGenres,strTitle,iTrack,iDuration,iYear,dwFileNameCRC,strFileName,strMusicBrainzTrackID,strMusicBrainzArtistID,strMusicBrainzAlbumID,strMusicBrainzAlbumArtistID,strMusicBrainzTRMID,iTimesPlayed,iStartOffset,iEndOffset,idThumb) values (NULL,%i,%i,%i,%i,%i,%i,'%s',%i,%i,%i,'%ul','%s','%s','%s','%s','%s','%s'",
                    lAlbumId, lPathId, lArtistId, iNumArtists, lGenreId, iNumGenres,
                    song.strTitle.c_str(),
                    song.iTrack, song.iDuration, song.iYear,
                    (DWORD)crc, strFileName.c_str(),
                    song.strMusicBrainzTrackID.c_str(),
                    song.strMusicBrainzArtistID.c_str(),
                    song.strMusicBrainzAlbumID.c_str(),
                    song.strMusicBrainzAlbumArtistID.c_str(),
                    song.strMusicBrainzTRMID.c_str());

      strSQL1=FormatSQL(",%i,%i,%i,%i)",
                     0, song.iStartOffset, song.iEndOffset, lThumbId);
      strSQL+=strSQL1;

      m_pDS->exec(strSQL.c_str());
      lSongId = (long)sqlite3_last_insert_rowid(m_pDB->getHandle());
    }

    // add artists and genres (don't need to add to exgenrealbum table - it's only used for albuminfo)
    AddExtraArtists(vecArtists, lSongId, lAlbumId, bCheck);
    AddExtraGenres(vecGenres, lSongId, 0, bCheck);

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

long CMusicDatabase::AddAlbum(const CStdString& strAlbum, const long lArtistId, const int iNumArtists, const CStdString &strArtist, long lPathId, const CStdString& strPath)
{
  CStdString strSQL;
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    map <CStdString, CAlbumCache>::const_iterator it;

    it = m_albumCache.find(strAlbum + strPath);
    if (it != m_albumCache.end())
      return it->second.idAlbum;

    strSQL=FormatSQL("select * from album where idPath=%i and strAlbum like '%s'", lPathId, strAlbum.c_str());
    m_pDS->query(strSQL.c_str());

    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      // doesnt exists, add it
      strSQL=FormatSQL("insert into album (idAlbum, strAlbum, idArtist, iNumArtists, idPath) values( NULL, '%s', %i, %i, %i)", strAlbum.c_str(), lArtistId, iNumArtists, lPathId);
      m_pDS->exec(strSQL.c_str());

      CAlbumCache album;
      album.idAlbum = (long)sqlite3_last_insert_rowid(m_pDB->getHandle());
      album.strPath = strPath;
      album.idPath = lPathId;
      album.strAlbum = strAlbum;
      album.strArtist = strArtist;
      m_albumCache.insert(pair<CStdString, CAlbumCache>(album.strAlbum + album.strPath, album));
      return album.idAlbum;
    }
    else
    {
      CAlbumCache album;
      album.idAlbum = m_pDS->fv("idAlbum").get_asLong();
      album.strPath = strPath;
      album.idPath = lPathId;
      album.strAlbum = strAlbum;
      album.strArtist = strArtist;
      m_albumCache.insert(pair<CStdString, CAlbumCache>(album.strAlbum + album.strPath, album));
      m_pDS->close();
      return album.idAlbum;
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "musicdatabase:unable to addalbum (%s)", strSQL.c_str());
  }

  return -1;
}

void CMusicDatabase::CheckVariousArtistsAndCoverArt()
{
  if (m_albumCache.size() <= 0)
    return ;

  try
  {
    map <CStdString, CAlbumCache>::const_iterator it;

    VECSONGS songs;
    for (it = m_albumCache.begin(); it != m_albumCache.end(); ++it)
    {
      CAlbumCache album = it->second;
      long lAlbumId = album.idAlbum;
      CStdString strSQL;
      // Albums by various artists will have different songs with different artists
      GetSongsByAlbum(album.strAlbum, album.strPath, songs);
      long lVariousArtistsId = -1;
      long lArtistsId = -1;
      bool bVarious = false;
      bool bSingleArtistCompilation = false;
      CStdString strArtist;
      if (songs.size() > 1)
      {
        // Are the artists of this album all the same
        for (int i = 0; i < (int)songs.size() - 1; i++)
        {
          CSong song = songs[i];
          CSong song1 = songs[i + 1];

          CStdStringArray vecArtists, vecArtists1;
          CStdString strArtist, strArtist1;
          int iNumArtists = StringUtils::SplitString(song.strArtist, " / ", vecArtists);
          int iNumArtists1 = StringUtils::SplitString(song1.strArtist, " / ", vecArtists1);
          strArtist = vecArtists[0];
          strArtist1 = vecArtists1[0];
          strArtist.TrimRight();
          strArtist1.TrimRight();
          // Only check the first artist if its different
          if (strArtist != strArtist1)
          {
            CStdString strVariousArtists = g_localizeStrings.Get(340);
            lVariousArtistsId = AddArtist(strVariousArtists);
            bSingleArtistCompilation = false;
            bVarious = true;
            break;
          }
          else if (iNumArtists > 1 && iNumArtists1 > 1 && song.strArtist != song1.strArtist)
          {
            // Artists don't all agree.  Instead of some complex function to compare all
            // the artists of each particular track, let's just take the first artist
            // and just use that.
            bSingleArtistCompilation = true;
            lArtistsId = AddArtist(vecArtists[0]);
          }
        }
      }

      if (bVarious)
      {
        CStdString strSQL=FormatSQL("UPDATE album SET iNumArtists=1, idArtist=%i where idAlbum=%i", lVariousArtistsId, album.idAlbum);
        m_pDS->exec(strSQL.c_str());
        // we must also update our exartistalbum mapping.
        // In the case of multiple artists, we need to add entries to extra artist album table so that we
        // can find the albums by a particular artist (even if they only contribute a single song to the
        // album in question)
        strSQL=FormatSQL("DELETE from exartistalbum where idAlbum=%i", album.idAlbum);
        m_pDS->exec(strSQL.c_str());
        // run through the songs and add the artists we need to link to...
        CStdStringArray vecArtists;
        for (int i = 0; i < (int)songs.size(); i++)
        {
          CStdStringArray vecTemp;
          StringUtils::SplitString(songs[i].strArtist, " / ", vecTemp);
          for (int j = 0; j < (int)vecTemp.size(); j++)
            vecArtists.push_back(vecTemp[j]);
        }
        AddExtraArtists(vecArtists, 0, album.idAlbum, true);
      }

      if (bSingleArtistCompilation)
      {
        CStdString strSQL=FormatSQL("UPDATE album SET iNumArtists=1, idArtist=%i where idAlbum=%i", lArtistsId, album.idAlbum);
        m_pDS->exec(strSQL.c_str());
        // we must also update our exartistalbum mapping.
        // In the case of multiple artists, we need to add entries to extra artist album table so that we
        // can find the albums by a particular artist (even if they only contribute a single song to the
        // album in question)
        strSQL=FormatSQL("DELETE from exartistalbum where idAlbum=%i", album.idAlbum);
        m_pDS->exec(strSQL.c_str());
        // run through the songs and add the artists we need to link to...
        CStdStringArray vecArtists;
        for (int i = 0; i < (int)songs.size(); i++)
        {
          CStdStringArray vecTemp;
          StringUtils::SplitString(songs[i].strArtist, " / ", vecTemp);
          for (int j = 1; j < (int)vecTemp.size(); j++)
            vecArtists.push_back(vecTemp[j]);
        }
        AddExtraArtists(vecArtists, 0, album.idAlbum, true);
      }

      CStdString strTempCoverArt;
      CStdString strCoverArt;
      CUtil::GetAlbumThumb(album.strAlbum, album.strPath, strTempCoverArt, true);
      // Was the album art of this album read during scan?
      if (CUtil::ThumbCached(strTempCoverArt))
      {
        // Yes.
        VECALBUMS albums;
        GetAlbumsByPath(album.strPath, albums);
        CUtil::GetAlbumFolderThumb(album.strPath, strCoverArt);
        // Do we have more then one album in this directory...
        if (albums.size() == 1)
        {
          // ...no, copy as permanent directory thumb
          if (::CopyFile(strTempCoverArt, strCoverArt, false))
            CUtil::ThumbCacheAdd(strCoverArt, true);
        }
        else if (CUtil::ThumbCached(strCoverArt))
        {
          // ...yes, we have more then one album in this directory
          // and have saved a thumb for this directory from another album
          // so delete the directory thumb.
          if (CUtil::ThumbExists(strCoverArt))
          {
            if (::DeleteFile(strCoverArt))
              CUtil::ThumbCacheAdd(strCoverArt, false);
          }
        }

        // And move as permanent thumb for files and directory, where
        // album and path is known
        CUtil::GetAlbumThumb(album.strAlbum, album.strPath, strCoverArt);
        ::MoveFileEx(strTempCoverArt, strCoverArt, MOVEFILE_REPLACE_EXISTING);

        // Update database entry in thumb table
        strSQL=FormatSQL("UPDATE thumb SET strThumb='%s' where strThumb='%s'", strCoverArt.c_str(), strTempCoverArt.c_str());
        m_pDS->exec(strSQL.c_str());

      }
    }
    m_albumCache.erase(m_albumCache.begin(), m_albumCache.end());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "musicdatabase:unable to checkvariousartistsandcoverart");
  }

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
    map <CStdString, CGenreCache>::const_iterator it;

    it = m_genreCache.find(strGenre);
    if (it != m_genreCache.end())
      return it->second.idGenre;


    strSQL=FormatSQL("select * from genre where strGenre like '%s'", strGenre.c_str());
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      // doesnt exists, add it
      strSQL=FormatSQL("insert into genre (idGenre, strGenre) values( NULL, '%s' )", strGenre.c_str());
      m_pDS->exec(strSQL.c_str());

      CGenreCache genre;
      genre.idGenre = (long)sqlite3_last_insert_rowid(m_pDB->getHandle());
      genre.strGenre = strGenre1;
      m_genreCache.insert(pair<CStdString, CGenreCache>(genre.strGenre, genre));
      return genre.idGenre;
    }
    else
    {
      CGenreCache genre;
      genre.idGenre = m_pDS->fv("idGenre").get_asLong();
      genre.strGenre = strGenre1;
      m_genreCache.insert(pair<CStdString, CGenreCache>(genre.strGenre, genre));
      m_pDS->close();
      return genre.idGenre;
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

    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    map <CStdString, CArtistCache>::const_iterator it;

    it = m_artistCache.find(strArtist);
    if (it != m_artistCache.end())
      return it->second.idArtist;

    strSQL=FormatSQL("select * from artist where strArtist like '%s'", strArtist.c_str());
    m_pDS->query(strSQL.c_str());

    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      // doesnt exists, add it
      strSQL=FormatSQL("insert into artist (idArtist, strArtist) values( NULL, '%s' )", strArtist.c_str());
      m_pDS->exec(strSQL.c_str());
      CArtistCache artist;
      artist.idArtist = (long)sqlite3_last_insert_rowid(m_pDB->getHandle());
      artist.strArtist = strArtist1;
      m_artistCache.insert(pair<CStdString, CArtistCache>(artist.strArtist, artist));
      return artist.idArtist;
    }
    else
    {
      CArtistCache artist;
      artist.idArtist = m_pDS->fv("idArtist").get_asLong();
      artist.strArtist = strArtist1;
      m_artistCache.insert(pair<CStdString, CArtistCache>(artist.strArtist, artist));
      m_pDS->close();
      return artist.idArtist;
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "musicdatabase:unable to addartist (%s)", strSQL.c_str());
  }

  return -1;
}

void CMusicDatabase::AddExtraArtists(const CStdStringArray &vecArtists, long lSongId, long lAlbumId, bool bCheck)
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
        if (lSongId)
        {
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
        // now link the artist with the album
        bInsert = true;
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
    CLog::Log(LOGERROR, "CMusicDatabase:AddExtraArtists(%i,%i) failed", lSongId, lAlbumId);
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
        // now link the genre with the album
        bInsert = true;
        if (lAlbumId)
        {
          if (bCheck)
          {
            strSQL=FormatSQL("select * from exgenrealbum where idAlbum=%i and idGenre=%i",
                          lAlbumId, lGenreId);
            if (!m_pDS->query(strSQL.c_str())) return ;
            if (m_pDS->num_rows() != 0)
              bInsert = false; // already exists
            m_pDS->close();
          }
          if (bInsert)
          {
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
    CLog::Log(LOGERROR, "CMusicDatabase:AddExtraGenres(%i,%i) failed", lSongId, lAlbumId);
  }
}

long CMusicDatabase::AddPath(const CStdString& strPath1)
{
  CStdString strSQL;
  try
  {
    CStdString strPath = strPath1;
    // musicdatabase always stores directories
    // without a slash at the end
    if (CUtil::HasSlashAtEnd(strPath))
      strPath.Delete(strPath.size() - 1);

    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    map <CStdString, CPathCache>::const_iterator it;

    it = m_pathCache.find(strPath1);
    if (it != m_pathCache.end())
      return it->second.idPath;

    strSQL=FormatSQL( "select * from path where strPath='%s'", strPath.c_str());
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      // doesnt exists, add it
      strSQL=FormatSQL("insert into path (idPath, strPath) values( NULL, '%s' )", strPath.c_str());
      m_pDS->exec(strSQL.c_str());

      CPathCache path;
      path.idPath = (long)sqlite3_last_insert_rowid(m_pDB->getHandle());
      path.strPath = strPath1;
      m_pathCache.insert(pair<CStdString, CPathCache>(path.strPath, path));
      return path.idPath;
    }
    else
    {
      CPathCache path;
      path.idPath = m_pDS->fv("idPath").get_asLong();
      path.strPath = strPath1;
      m_pathCache.insert(pair<CStdString, CPathCache>(path.strPath, path));
      m_pDS->close();
      return path.idPath;
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
  CSong song;
  // get the full artist string
  song.strArtist = m_pDS->fv("strArtist").get_asString();
  if (m_pDS->fv("iNumArtists").get_asLong() > 1)
    GetExtraArtistsForSong(m_pDS->fv("idSong").get_asLong(), song.strArtist);
  // and the full genre string
  song.strGenre = m_pDS->fv("strGenre").get_asString();
  if (m_pDS->fv("iNumGenres").get_asLong() > 1)
    GetExtraGenresForSong(m_pDS->fv("idSong").get_asLong(), song.strGenre);
  // and the rest...
  song.strAlbum = m_pDS->fv("strAlbum").get_asString();
  song.iTrack = m_pDS->fv("iTrack").get_asLong() ;
  song.iDuration = m_pDS->fv("iDuration").get_asLong() ;
  song.iYear = m_pDS->fv("iYear").get_asLong() ;
  song.strTitle = m_pDS->fv("strTitle").get_asString();
  song.iTimedPlayed = m_pDS->fv("iTimesPlayed").get_asLong();
  song.iStartOffset = m_pDS->fv("iStartOffset").get_asLong();
  song.iEndOffset = m_pDS->fv("iEndOffset").get_asLong();
  song.strMusicBrainzTrackID = m_pDS->fv("strMusicBrainzTrackID").get_asString();
  song.strMusicBrainzArtistID = m_pDS->fv("strMusicBrainzArtistID").get_asString();
  song.strMusicBrainzAlbumID = m_pDS->fv("strMusicBrainzAlbumID").get_asString();
  song.strMusicBrainzAlbumArtistID = m_pDS->fv("strMusicBrainzAlbumArtistID").get_asString();
  song.strMusicBrainzTRMID = m_pDS->fv("strMusicBrainzTRMID").get_asString();
  song.strThumb = m_pDS->fv("strThumb").get_asString();
  if (song.strThumb == "NONE")
    song.strThumb.Empty();
  // Get filename with full path
  song.strFileName = m_pDS->fv("strPath").get_asString();
  CUtil::AddDirectorySeperator(song.strFileName);
  song.strFileName += m_pDS->fv("strFileName").get_asString();

  return song;
}

CAlbum CMusicDatabase::GetAlbumFromDataset()
{
  CAlbum album;
  album.strAlbum = m_pDS->fv("strAlbum").get_asString();
  album.strArtist = m_pDS->fv("strArtist").get_asString();
  if (m_pDS->fv("iNumArtists").get_asLong() > 1)
    GetExtraArtistsForAlbum(m_pDS->fv("idAlbum").get_asLong(), album.strArtist);
  album.strPath = m_pDS->fv("strPath").get_asString();
  return album;
}

bool CMusicDatabase::GetSongByFileName(const CStdString& strFileName, CSong& song)
{
  try
  {
    song.Clear();

    CStdString strPath;
    CUtil::GetDirectory(strFileName, strPath);
    if (CUtil::HasSlashAtEnd(strPath))
      strPath.Delete(strPath.size() - 1);

    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    Crc32 crc;
    crc.Compute(strFileName);

    CStdString strSQL=FormatSQL("select * from songview "
                                "where dwFileNameCRC='%ul' and strPath='%s'"
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
    CLog::Log(LOGERROR, "CMusicDatabase:GetSongByFileName(%s) failed", strFileName.c_str());
  }

  return false;
}

bool CMusicDatabase::GetExtraArtistsForSong(long lSongId, CStdString &strArtist)
{
  // gets called when we have multiple artists for this song.
  CStdString strSQL=FormatSQL("select artist.strArtist from exartistsong, artist "
                              "where exartistsong.idSong=%i and exartistsong.idArtist=artist.idArtist "
                              "order by exartistsong.iPosition"
                              , lSongId);

  if (NULL == m_pDS2.get() || !m_pDS2->query(strSQL.c_str())) return false;
  while (!m_pDS2->eof())
  {
    strArtist += " / " + m_pDS2->fv("artist.strArtist").get_asString();
    m_pDS2->next();
  }
  m_pDS2->close();
  return true;
}

bool CMusicDatabase::GetExtraGenresForSong(long lSongId, CStdString &strGenre)
{
  // gets called when we have multiple genres for this song.
  CStdString strSQL=FormatSQL("select genre.strGenre from exgenresong,genre "
                              "where exgenresong.idSong=%i and exgenresong.idGenre=genre.idGenre "
                              "order by exgenresong.iPosition"
                              , lSongId);
  if (NULL == m_pDS2.get() || !m_pDS2->query(strSQL.c_str())) return false;
  while (!m_pDS2->eof())
  {
    strGenre += " / " + m_pDS2->fv("genre.strGenre").get_asString();
    m_pDS2->next();
  }
  m_pDS2->close();
  return true;
}

bool CMusicDatabase::GetExtraArtistsForAlbum(long lAlbumId, CStdString &strArtist)
{
  // gets called when we have multiple artists for this album (but not for various artists).
  CStdString strSQL=FormatSQL("select artist.strArtist from exartistalbum, artist "
                              "where exartistalbum.idAlbum=%i and exartistalbum.idArtist=artist.idArtist "
                              "order by exartistalbum.iPosition"
                              , lAlbumId);

  if (NULL == m_pDS2.get() || !m_pDS2->query(strSQL.c_str())) return false;
  while (!m_pDS2->eof())
  {
    strArtist += " / " + m_pDS2->fv("artist.strArtist").get_asString();
    m_pDS2->next();
  }
  return true;
}

bool CMusicDatabase::GetExtraGenresForAlbum(long lAlbumId, CStdString &strGenre)
{
  // gets called when we have multiple genres for this album (only for albuminfo).
  CStdString strSQL=FormatSQL("select genre.strGenre from exgenrealbum,genre "
                              "where exgenrealbum.idAlbum=%i and exgenrealbum.idGenre=genre.idGenre "
                              "order by exgenrealbum.iPosition"
                              , lAlbumId);

  if (NULL == m_pDS2.get() || !m_pDS2->query(strSQL.c_str())) return false;
  while (!m_pDS2->eof())
  {
    strGenre += " / " + m_pDS2->fv("genre.strGenre").get_asString();
    m_pDS2->next();
  }
  m_pDS2->close();
  return true;
}

bool CMusicDatabase::GetSong(const CStdString& strTitle, CSong& song)
{
  try
  {
    song.Clear();

    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL=FormatSQL("select * from songview "
                                "where strTitle like '%s'"
                                , strTitle.c_str() );

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
    CLog::Log(LOGERROR, "CMusicDatabase:GetSong(%s) failed", strTitle.c_str());
  }

  return false;
}

bool CMusicDatabase::GetSongsByAlbum(const CStdString& strAlbum, const CStdString& strPath1, VECSONGS& songs)
{
  try
  {
    CStdString strPath = strPath1;
    // musicdatabase always stores directories
    // without a slash at the end
    if (CUtil::HasSlashAtEnd(strPath))
      strPath.Delete(strPath.size() - 1);

    songs.erase(songs.begin(), songs.end());
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL=FormatSQL("select * from songview "
                                "where strAlbum like '%s' and strPath like '%s' "
                                "order by iTrack"
                                , strAlbum.c_str(), strPath.c_str());
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return false;
    }
    while (!m_pDS->eof())
    {
      songs.push_back(GetSongFromDataset());
      m_pDS->next();
    }

    m_pDS->close(); // cleanup recordset data
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CMusicDatabase:GetSongsByAlbum(%s, %s) failed", strAlbum.c_str(), strPath1.c_str());
  }

  return false;
}

bool CMusicDatabase::GetArtistsByName(const CStdString& strArtist, VECARTISTS& artists)
{
  try
  {
    artists.erase(artists.begin(), artists.end());

    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    // Exclude "Various Artists"
    CStdString strVariousArtists = g_localizeStrings.Get(340);
    long lVariousArtistId = AddArtist(strVariousArtists);

    CStdString strSQL=FormatSQL("select * from artist "
                                "where strArtist LIKE '%%%s%%' and idArtist <> %i "
                                , strArtist.c_str(), lVariousArtistId );
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return false;
    }
    while (!m_pDS->eof())
    {
      CStdString strArtist = m_pDS->fv("strArtist").get_asString();
      artists.push_back(strArtist);
      m_pDS->next();
    }

    m_pDS->close(); // cleanup recordset data
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CMusicDatabase:GetArtistsByName() failed");
  }

  return false;
}

bool CMusicDatabase::GetGenresByName(const CStdString& strGenre, VECGENRES& genres)
{
  try
  {
    genres.erase(genres.begin(), genres.end());

    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL=FormatSQL("select * from genre "
                                "where strGenre LIKE '%%%s%%' ", strGenre.c_str());
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return false;
    }
    while (!m_pDS->eof())
    {
      CStdString strGenre = m_pDS->fv("strGenre").get_asString();
      genres.push_back(strGenre);
      m_pDS->next();
    }

    m_pDS->close(); // cleanup recordset data
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CMusicDatabase:GetGenresByName() failed");
  }

  return false;
}

long CMusicDatabase::AddAlbumInfo(const CAlbum& album, const VECSONGS& songs)
{
  CStdString strSQL;
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    BeginTransaction();

    // split our (possibly) multiple artist string into individual artists
    CStdStringArray vecArtists;
    int iNumArtists = StringUtils::SplitString(album.strArtist, " / ", vecArtists);
    // split our (possibly) multiple genre string into individual genres
    CStdStringArray vecGenres;
    int iNumGenres = StringUtils::SplitString(album.strGenre, " / ", vecGenres);
    // add the primary artist/genre
    long lArtistId = AddArtist(vecArtists[0]);
    long lGenreId = AddGenre(vecGenres[0]);

    long lPathId = AddPath(album.strPath);
    long lAlbumId = AddAlbum(album.strAlbum, lArtistId, iNumArtists, album.strArtist, lPathId, album.strPath);

    AddExtraArtists(vecArtists, 0, lAlbumId);
    AddExtraGenres(vecGenres, 0, lAlbumId);

    strSQL=FormatSQL("select * from albuminfo where idAlbum=%i", lAlbumId);
    if (!m_pDS->query(strSQL.c_str())) return -1;

    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound != 0)
    {
      long iID = m_pDS->fv("idAlbumInfo").get_asLong();
      m_pDS->close();
      return iID;
    }
    m_pDS->close();

    strSQL=FormatSQL("insert into albuminfo (idAlbumInfo,idAlbum,idGenre,iNumGenres,strTones,strStyles,strReview,strImage,iRating,iYear) values(NULL,%i,%i,%i,'%s','%s','%s','%s',%i,%i)",
                  lAlbumId, lGenreId, iNumGenres,
                  album.strTones.c_str(),
                  album.strStyles.c_str(),
                  album.strReview.c_str(),
                  album.strImage.c_str(),
                  album.iRating,
                  album.iYear);
    m_pDS->exec(strSQL.c_str());

    long lAlbumInfoId = (long)sqlite3_last_insert_rowid(m_pDB->getHandle());

    if (AddAlbumInfoSongs(lAlbumInfoId, songs))
      CommitTransaction();
    else
    {
      RollbackTransaction();
      lAlbumInfoId = -1;
    }

    return lAlbumInfoId;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "musicdatabase:unable to addalbuminfo (%s)", strSQL.c_str());
  }

  RollbackTransaction();

  return -1;
}

bool CMusicDatabase::AddAlbumInfoSongs(long idAlbumInfo, const VECSONGS& songs)
{
  CStdString strSQL;
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

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
    CLog::Log(LOGERROR, "musicdatabase:unable to addalbuminfosong (%s)", strSQL.c_str());
  }

  return false;
}

bool CMusicDatabase::GetAlbumInfo(const CStdString& strAlbum, const CStdString& strPath1, CAlbum& album, VECSONGS& songs)
{
  try
  {
    CStdString strPath = strPath1;
    // musicdatabase always stores directories
    // without a slash at the end
    if (CUtil::HasSlashAtEnd(strPath))
      strPath.Delete(strPath.size() - 1);

    CStdString strSQL=FormatSQL("select * from albuminfo "
                                  "join albumview on albuminfo.idAlbum=albumview.idAlbum "
                                  "join genre on albuminfo.idGenre=genre.idGenre "
                                "where albumview.strAlbum like '%s' and albumview.strPath='%s'"
                                , strAlbum.c_str(), strPath.c_str());

    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound != 0)
    {
      album.iRating = m_pDS->fv("albuminfo.iRating").get_asLong() ;
      album.iYear = m_pDS->fv("albuminfo.iYear").get_asLong() ;
      album.strAlbum = m_pDS->fv("albumview.strAlbum").get_asString();
      album.strArtist = m_pDS->fv("albumview.strArtist").get_asString();
      if (m_pDS->fv("albumview.iNumArtists").get_asLong() > 1)
        GetExtraArtistsForAlbum(m_pDS->fv("albumview.idAlbum").get_asLong(), album.strArtist);
      album.strGenre = m_pDS->fv("genre.strGenre").get_asString();
      if (m_pDS->fv("albuminfo.iNumGenres").get_asLong() > 1)
        GetExtraGenresForAlbum(m_pDS->fv("albuminfo.idAlbum").get_asLong(), album.strGenre);
      album.strImage = m_pDS->fv("albuminfo.strImage").get_asString();
      album.strReview = m_pDS->fv("albuminfo.strReview").get_asString();
      album.strStyles = m_pDS->fv("albuminfo.strStyles").get_asString();
      album.strTones = m_pDS->fv("albuminfo.strTones").get_asString();
      album.strPath = m_pDS->fv("albumview.strPath").get_asString();

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
    CLog::Log(LOGERROR, "CMusicDatabase:GetAlbumInfo(%s, %s) failed", strAlbum.c_str(), strPath1.c_str());
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
    CLog::Log(LOGERROR, "CMusicDatabase:GetAlbumInfoSongs(%i) failed", idAlbumInfo);
  }

  return false;
}

bool CMusicDatabase::GetTop100Songs(CFileItemList& items, bool bClearItems /* = true */)
{
  try
  {
    if (bClearItems)
      items.Clear();

    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL="select * from songview "
                      "where iTimesPlayed>0 "
                      "order by iTimesPlayed desc "
                      "limit 100";

    CLog::Log(LOGDEBUG, "CMusicDatabase::GetTop100Songs() query: %s", strSQL.c_str());
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
      CFileItem *item = new CFileItem(GetSongFromDataset());
      items.Add(item);
      m_pDS->next();
    }

    m_pDS->close(); // cleanup recordset data
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CMusicDatabase:GetTop100Songs() failed");
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

CStdString strSQL = "select albumview.*, sum(song.iTimesPlayed) as total from albumview "
                      "join song on albumview.idAlbum=song.idAlbum "
                    "where song.iTimesPlayed>0 "
                    "group by albumview.idalbum "
                    "order by total desc "
                    "limit 100 ";

    CLog::Log(LOGDEBUG, "CMusicDatabase::GetTop100Albums() query: %s", strSQL.c_str());
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return false;
    }
    int iCount = 1;
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
    CLog::Log(LOGERROR, "CMusicDatabase:GetTop100Albums() failed");
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

    CStdString strSQL = "select distinct albumview.* from albumview "
                          "join song on albumview.idAlbum=song.idAlbum "
                        "where song.lastplayed NOT NULL "
                        "order by song.lastplayed desc "
                        "limit 25 ";

    CLog::Log(LOGDEBUG, "CMusicDatabase::GetRecentlyPlayedAlbums() query: %s", strSQL.c_str());
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return false;
    }
    int iCount = 1;
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
    CLog::Log(LOGERROR, "CMusicDatabase:GetRecentlyPlayedAlbums() failed");
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

    CStdString strSQL = "select * from albumview "
                        "order by idalbum desc "
                        "limit 25 ";

    CLog::Log(LOGDEBUG, "CMusicDatabase::GetRecentlyAddedAlbums() query: %s", strSQL.c_str());
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
    CLog::Log(LOGERROR, "CMusicDatabase:GetRecentlyAddedAlbums() failed");
  }

  return false;
}

bool CMusicDatabase::IncrTop100CounterByFileName(const CStdString& strFileName)
{
  try
  {
    CSong song;

    CStdString strPath;
    CUtil::GetDirectory(strFileName, strPath);
    if (CUtil::HasSlashAtEnd(strPath))
      strPath.Delete(strPath.size() - 1);

    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    Crc32 crc;
    crc.Compute(strFileName);

    CStdString strSQL=FormatSQL("select idSong, iTimesPlayed from song,path where song.dwFileNameCRC='%ul' and song.idPath=path.idPath and path.strPath='%s'",
                  crc,
                  strPath.c_str());
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return false;
    }

    int idSong = m_pDS->fv("song.idSong").get_asLong();
    int iTimesPlayed = m_pDS->fv("song.iTimesPlayed").get_asLong();
    m_pDS->close();

    strSQL=FormatSQL("UPDATE song SET iTimesPlayed=%i, lastplayed=CURRENT_TIMESTAMP where idSong=%i",
                  ++iTimesPlayed, idSong);
    m_pDS->exec(strSQL.c_str());
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CMusicDatabase:IncrTop100Counter(%s) failed", strFileName.c_str());
  }

  return false;
}

bool CMusicDatabase::GetSongsByPath(const CStdString& strPath1, VECSONGS& songs)
{
  try
  {
    songs.erase(songs.begin(), songs.end());
    CStdString strPath = strPath1;
    // musicdatabase always stores directories
    // without a slash at the end
    if (CUtil::HasSlashAtEnd(strPath))
      strPath.Delete(strPath.size() - 1);
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL=FormatSQL("select * from songview where strPath='%s'", strPath.c_str() );
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return false;
    }
    while (!m_pDS->eof())
    {
      songs.push_back(GetSongFromDataset());
      m_pDS->next();
    }

    m_pDS->close(); // cleanup recordset data
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CMusicDatabase:GetVecSongsByPath(%s) failed", strPath1.c_str());
  }

  return false;
}

bool CMusicDatabase::GetSongsByPath(const CStdString& strPath1, MAPSONGS& songs, bool bAppendToMap)
{
  try
  {
    if (!bAppendToMap)
      songs.erase(songs.begin(), songs.end());
    CStdString strPath = strPath1;
    // musicdatabase always stores directories
    // without a slash at the end
    if (CUtil::HasSlashAtEnd(strPath))
      strPath.Delete(strPath.size() - 1);

    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL=FormatSQL("select * from songview where strPath='%s'", strPath.c_str() );
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
      songs.insert(pair<CStdString, CSong>(song.strFileName, song));
      m_pDS->next();
    }

    m_pDS->close(); // cleanup recordset data
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CMusicDatabase:GetMapSongsByPath(%s) failed", strPath1.c_str());
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

bool CMusicDatabase::GetAlbumsByPath(const CStdString& strPath1, VECALBUMS& albums)
{
  try
  {
    CStdString strPath = strPath1;
    albums.erase(albums.begin(), albums.end());
    // musicdatabase always stores directories
    // without a slash at the end
    if (CUtil::HasSlashAtEnd(strPath))
      strPath.Delete(strPath.size() - 1);

    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL=FormatSQL("select * from albumview where strPath='%s'", strPath.c_str() );
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
    CLog::Log(LOGERROR, "CMusicDatabase:GetAlbumsByPath() for %s failed", strPath1.c_str());
  }

  return false;
}

bool CMusicDatabase::FindSongsByName(const CStdString& strSearch, VECSONGS& songs)
{
  try
  {
    songs.erase(songs.begin(), songs.end());

    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL=FormatSQL("select * from songview where strTitle LIKE '%%%s%%'", strSearch.c_str());
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0) return false;
    while (!m_pDS->eof())
    {
      songs.push_back(GetSongFromDataset());
      m_pDS->next();
    }

    m_pDS->close();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CMusicDatabase:GetSongsByName() failed");
  }

  return false;
}

bool CMusicDatabase::FindSongsByNameAndArtist(const CStdString& strSearch, VECSONGS& songs)
{
  try
  {
    songs.erase(songs.begin(), songs.end());
    // use a set as we don't need more than 1 of each thing
    set<CSong> setSongs;

    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    // find songs that match in name
    CStdString strSQL=FormatSQL("select * from songview where strTitle LIKE '%%%s%%'", strSearch.c_str());
    if (!m_pDS->query(strSQL.c_str())) return false;
    while (!m_pDS->eof())
    {
      //songs.push_back(GetSongFromDataset());
      setSongs.insert(GetSongFromDataset());
      m_pDS->next();
    }
    m_pDS->close();
    // and then songs that match in primary artist
    strSQL=FormatSQL("select * from songview where strArtist LIKE '%%%s%%'", strSearch.c_str());
    if (!m_pDS->query(strSQL.c_str())) return false;
    while (!m_pDS->eof())
    {
      setSongs.insert(GetSongFromDataset());
      m_pDS->next();
    }
    m_pDS->close();
    // and then songs that match in the secondary artists

    strSQL=FormatSQL("select * from exartistsong join artist on exartistsong.idartist=artist.idartist join song on exartistsong.idsong=song.idsong join album on song.idalbum=album.idalbum join genre on song.idgenre=genre.idgenre join path on song.idpath=path.idpath join thumb on song.idThumb=thumb.idThumb where artist.strArtist like '%%%s%%'", strSearch.c_str());
    if (!m_pDS->query(strSQL.c_str())) return false;
    while (!m_pDS->eof())
    {
      setSongs.insert(GetSongFromDataset());
      m_pDS->next();
    }
    m_pDS->close();
    // now dump our set back into our vector
    for (set<CSong>::iterator it = setSongs.begin(); it != setSongs.end(); it++) songs.push_back(*it);
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CMusicDatabase:GetSongsByName() failed");
  }

  return false;
}

bool CMusicDatabase::GetAlbumsByName(const CStdString& strSearch, VECALBUMS& albums)
{
  try
  {
    albums.erase(albums.begin(), albums.end());

    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL=FormatSQL("select * from albumview where strAlbum like '%%%s%%'", strSearch.c_str());
    if (!m_pDS->query(strSQL.c_str())) return false;

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
    CLog::Log(LOGERROR, "CMusicDatabase:GetAlbumsByName() failed");
  }

  return false;
}



bool CMusicDatabase::FindAlbumsByName(const CStdString& strSearch, VECALBUMS& albums)
{
  try
  {
    albums.erase(albums.begin(), albums.end());
    // use a set for fast lookups
    set<CAlbum> setAlbums;
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    // first get matching albums by name.
    CStdString strSQL=FormatSQL("select * from albumview where strAlbum like '%%%s%%'", strSearch.c_str());
    if (!m_pDS->query(strSQL.c_str())) return false;
    while (!m_pDS->eof())
    {
      setAlbums.insert(GetAlbumFromDataset());
      m_pDS->next();
    }
    m_pDS->close();
    // now try and match the primary artist.
    strSQL=FormatSQL("select * from albumview where strArtist like '%%%s%%'", strSearch.c_str());
    if (!m_pDS->query(strSQL.c_str())) return false;
    while (!m_pDS->eof())
    {
      setAlbums.insert(GetAlbumFromDataset());
      m_pDS->next();
    }
    m_pDS->close();
    // and finally try the secondary artists.
    strSQL=FormatSQL("select * from album,path,artist,exartistalbum where artist.strArtist like '%%%s%%' and artist.idArtist=exartistalbum.idArtist and album.idAlbum=exartistalbum.idAlbum and album.idPath=path.idPath", strSearch.c_str());
    if (!m_pDS->query(strSQL.c_str())) return false;
    while (!m_pDS->eof())
    {
      setAlbums.insert(GetAlbumFromDataset());
      m_pDS->next();
    }
    m_pDS->close();
    // and copy our set into the vector
    for (set<CAlbum>::iterator it = setAlbums.begin(); it != setAlbums.end(); it++) albums.push_back(*it);
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CMusicDatabase:GetAlbumsByName() failed");
  }

  return false;
}

long CMusicDatabase::UpdateAlbumInfo(const CAlbum& album, const VECSONGS& songs)
{
  CStdString strSQL;
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    BeginTransaction();

    // split our (possibly) multiple artist string into single artists.
    CStdStringArray vecArtists;
    long iNumArtists = StringUtils::SplitString(album.strArtist, " / ", vecArtists);
    // and also the multiple genre string into single genres.
    CStdStringArray vecGenres;
    long iNumGenres = StringUtils::SplitString(album.strGenre, " / ", vecGenres);

    long lArtistId = AddArtist(vecArtists[0]);
    long lGenreId = AddGenre(vecGenres[0]);
    long lPathId = AddPath(album.strPath);
    long lAlbumId = AddAlbum(album.strAlbum, lArtistId, iNumArtists, album.strArtist, lPathId, album.strPath);
    // add any extra artists/genres
    AddExtraArtists(vecArtists, 0, lAlbumId);
    AddExtraGenres(vecGenres, 0, lAlbumId);

    strSQL=FormatSQL("select * from albuminfo where idAlbum=%i", lAlbumId);
    if (!m_pDS->query(strSQL.c_str())) return -1;

    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      long idAlbumInfo = AddAlbumInfo(album, songs);
      if (idAlbumInfo > -1)
        CommitTransaction();
      else
        RollbackTransaction();

      return idAlbumInfo;
    }

    long idAlbumInfo = m_pDS->fv("idAlbumInfo").get_asLong();
    m_pDS->close();


    strSQL=FormatSQL("update albuminfo set idAlbum=%i,idGenre=%i,iNumGenres=%i,strTones='%s',strStyles='%s',strReview='%s',strImage='%s',iRating=%i,iYear=%i where idAlbumInfo=%i",
                  lAlbumId, lGenreId, iNumGenres,
                  album.strTones.c_str(),
                  album.strStyles.c_str(),
                  album.strReview.c_str(),
                  album.strImage.c_str(),
                  album.iRating,
                  album.iYear,
                  idAlbumInfo);
    m_pDS->exec(strSQL.c_str());

    if (UpdateAlbumInfoSongs(idAlbumInfo, songs))
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
    CLog::Log(LOGERROR, "musicdatabase:unable to updatealbuminfo (%s)", strSQL.c_str());
  }

  RollbackTransaction();

  return -1;
}

bool CMusicDatabase::UpdateAlbumInfoSongs(long idAlbumInfo, const VECSONGS& songs)
{
  CStdString strSQL;
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    strSQL=FormatSQL("delete from albuminfosong where idAlbumInfo=%i", idAlbumInfo);
    if (!m_pDS->exec(strSQL.c_str())) return false;

    return AddAlbumInfoSongs(idAlbumInfo, songs);

  }
  catch (...)
  {
    CLog::Log(LOGERROR, "musicdatabase:unable to updatealbuminfosongs (%s)", strSQL.c_str());
  }

  return false;
}

bool CMusicDatabase::GetSubpathsFromPath(const CStdString &strPath1, CStdString& strPathIds)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    // musicdatabase always stores directories
    // without a slash at the end
    CStdString strPath = strPath1;
    if (CUtil::HasSlashAtEnd(strPath))
      strPath.Delete(strPath.size() - 1);

    // get all the path id's that are sub dirs of this directory
    CStdString strSQL=FormatSQL("select idPath from path where strPath like '%s%%'", strPath.c_str());
    if (!m_pDS->query(strSQL.c_str())) return false;
    // create the idPath search string
    strPathIds = "(";
    while (!m_pDS->eof())
    {
      strPathIds += m_pDS->fv("idPath").get_asString() + ", ";
      m_pDS->next();
    }
    strPathIds.TrimRight(", ");
    strPathIds += ")";
    m_pDS->close();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "GetSubpathsFromPath() failed or was aborted!");
  }
  return false;
}

bool CMusicDatabase::RemoveSongsFromPaths(const CStdString &strPathIds)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    // ok, now find all idSong's
    CStdString strSQL=FormatSQL("select idSong from song where song.idPath in %s", strPathIds.c_str());
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return true;
    }
    CStdString strSongIds = "(";
    while (!m_pDS->eof())
    {
      strSongIds += m_pDS->fv("idSong").get_asString() + ",";
      m_pDS->next();
    }
    m_pDS->close();
    strSongIds.TrimRight(",");
    strSongIds += ")";

    // and all songs + exartistsongs and exgenresongs
    strSQL = "delete from song where idSong in " + strSongIds;
    m_pDS->exec(strSQL.c_str());
    strSQL = "delete from exartistsong where idSong in " + strSongIds;
    m_pDS->exec(strSQL.c_str());
    strSQL = "delete from exgenresong where idSong in " + strSongIds;
    m_pDS->exec(strSQL.c_str());
    m_pDS->close();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "RemoveSongsFromPath() failed or was aborted!");
  }
  return false;
}

bool CMusicDatabase::CleanupAlbumsFromPaths(const CStdString &strPathIds)
{
  try
  {
    CStdString strSQL = "select * from album,path where album.idPath in " + strPathIds + " and album.idAlbum not in (select distinct idAlbum from song) and album.idPath=path.idPath";
    //CStdString strSQL = "select * from album where album.idPath in " + strPathIds + " and album.idAlbum not in (select distinct idAlbum from song)";
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return true;
    }
    // now we have all albums that need to be removed
    CStdString strAlbumIds = "(";
    while (!m_pDS->eof())
    {
      strAlbumIds += m_pDS->fv("album.idAlbum").get_asString() + ",";
      // delete the thumb
      
      CStdString strThumb;
      CUtil::GetAlbumThumb(m_pDS->fv("album.strAlbum").get_asString(), m_pDS->fv("path.strPath").get_asString(), strThumb);
      ::DeleteFile(strThumb);

      // delete directory thumb
      CUtil::GetAlbumFolderThumb(m_pDS->fv("path.strPath").get_asString(), strThumb);
      ::DeleteFile(strThumb);
      m_pDS->next();
    }
    m_pDS->close();
    strAlbumIds.TrimRight(",");
    strAlbumIds += ")";
    // ok, now we can delete them and the references in the exartistalbum and exgenrealbum and albuminfo tables
    strSQL = "delete from album where idAlbum in " + strAlbumIds;
    m_pDS->exec(strSQL.c_str());
    strSQL = "delete from albuminfo where idAlbum in " + strAlbumIds;
    m_pDS->exec(strSQL.c_str());
    strSQL = "delete from exartistalbum where idAlbum in " + strAlbumIds;
    m_pDS->exec(strSQL.c_str());
    strSQL = "delete from exgenrealbum where idAlbum in " + strAlbumIds;
    m_pDS->exec(strSQL.c_str());
    // we must also cleanup any thumbs
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "Exception in CMusicDatabase::CleanupAlbumsFromPaths() or was aborted");
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
      CStdString strFileName = m_pDS->fv("path.strPath").get_asString();
      CUtil::AddDirectorySeperator(strFileName);
      strFileName += m_pDS->fv("song.strFileName").get_asString();

      //  Special case for streams inside an ogg file. (oggstream)
      //  The last dir in the path is the ogg file that 
      //  contains the stream, so test if its there
      CStdString strExtension=CUtil::GetExtension(strFileName);
      if (strExtension==".oggstream" || strExtension==".nsfstream")
      {
        CStdString strFileAndPath=strFileName;
        CUtil::GetDirectory(strFileAndPath, strFileName);
        if (CUtil::HasSlashAtEnd(strFileName))
          strFileName.Delete(strFileName.size()-1);
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
    // TODO: shouldn't really delete them if they contain albuminfo
    CStdString strSQL = "select * from album,path where album.idAlbum not in (select distinct idAlbum from song) and album.idPath=path.idPath";
    //  strSQL += " or idAlbum not in (select distinct idAlbum from albuminfo)";
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
      // delete the thumb for this file, if it has been cached
      CStdString strThumb;
      CUtil::GetAlbumThumb(m_pDS->fv("album.strAlbum").get_asString(), m_pDS->fv("path.strPath").get_asString(), strThumb);
      ::DeleteFile(strThumb);
      m_pDS->next();
    }
    m_pDS->close();
    // we should also delete albums with invalid paths
    /*  strSQL = "select * from album,path where album.idPath=path.idPath";
      if (!m_pDS->query(strSQL.c_str())) return false;
      while (!m_pDS->eof())
      {
       if (!CFile::Exists(m_pDS->fv("path.strPath").get_asString()))
       {
        strAlbumIds += m_pDS->fv("album.idAlbum").get_asString() + ",";
        // delete the thumb for this file, if it has been cached
        CStdString strThumb;
        CUtil::GetAlbumThumb(m_pDS->fv("album.strAlbum").get_asString(), m_pDS->fv("path.strPath").get_asString(), strThumb);
        ::DeleteFile(strThumb);
       }
       m_pDS->next();
      }*/
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
    // we can happily delete any path that has no reference to a song or album (we need album, as the
    // user can have albuminfo without having song info for that particular album)
    CStdString strSQL = "delete from path where idPath not in (select distinct idPath from song)";
    strSQL += " and idPath not in (select distinct idPath from album)";
    m_pDS->exec(strSQL.c_str());
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
    CStdString strSQL = "select * from thumb where idThumb not in (select distinct idThumb from song)";
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return true;
    }
    // get albums dir
    CStdString strThumbsDir;
    strThumbsDir.Format("%s\\thumbs\\", g_stSettings.m_szAlbumDirectory);
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
    strSQL = "delete from thumb where idThumb not in (select distinct idThumb from song)";
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
    CStdString strSQL = "delete from artist where idArtist not in (select distinct idArtist from song)";
    strSQL += " and idArtist not in (select distinct idArtist from exartistsong)";
    strSQL += " and idArtist not in (select distinct idArtist from album)";
    strSQL += " and idArtist not in (select distinct idArtist from exartistalbum)";
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
    CStdString strSQL = "delete from genre where idGenre not in (select distinct idGenre from song) and";
    strSQL += " idGenre not in (select distinct idGenre from exgenresong) and";
    strSQL += " idGenre not in (select distinct idGenre from albuminfo) and";
    strSQL += " idGenre not in (select distinct idGenre from exgenrealbum)";
    m_pDS->exec(strSQL.c_str());
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "Exception in CMusicDatabase::CleanupGenres() or was aborted");
  }
  return false;
}

bool CMusicDatabase::CleanupAlbumsArtistsGenres(const CStdString &strPathIds)
{
  if (NULL == m_pDB.get()) return false;
  if (NULL == m_pDS.get()) return false;
  // first cleanup any albums that are about
  if (!CleanupAlbumsFromPaths(strPathIds)) return false;
  if (!CleanupArtists()) return false;
  if (!CleanupGenres()) return false;
  if (!CleanupPaths()) return false;
  if (!CleanupThumbs()) return false;
  return true;
}

int CMusicDatabase::Cleanup(CGUIDialogProgress *pDlgProgress)
{
  Open();
  if (NULL == m_pDB.get()) return ERROR_DATABASE;
  if (NULL == m_pDS.get()) return ERROR_DATABASE;
  // first cleanup any songs with invalid paths
  pDlgProgress->SetLine(0, "");
  pDlgProgress->SetLine(1, 318);
  pDlgProgress->SetLine(2, 330);
  pDlgProgress->SetPercentage(0);
  pDlgProgress->Progress();
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
  if (dlgMusicScan->IsRunning())
  {
    CGUIDialogOK::ShowAndGetInput(189, 14057, 0, 0);
    return;
  }

  CStdString strSQL="select * from albuminfo,album,path,artist where album.idPath=path.idPath and albuminfo.idAlbum=album.idAlbum and album.idArtist=artist.idArtist order by album.strAlbum";
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
    if (m_pDS->fv("album.iNumArtists").get_asLong() > 1)
    {
      auto_ptr<Dataset> pDS;
      pDS.reset(m_pDB->CreateDataset());
      CStdString strSQL=FormatSQL("select artist.strArtist from exartistalbum, artist where exartistalbum.idAlbum=%i and exartistalbum.idArtist=artist.idArtist order by exartistalbum.iPosition", album.idAlbum);
      if (NULL == pDS.get() || !pDS->query(strSQL.c_str())) return ;
      while (!pDS->eof())
      {
        album.strArtist += " / " + pDS->fv("artist.strArtist").get_asString();
        pDS->next();
      }
      pDS->close();
    }
    album.strPath = m_pDS->fv("path.strPath").get_asString();
    vecAlbums.push_back(album);
    m_pDS->next();
  }
  m_pDS->close();

  // Show a selectdialog that the user can select the albuminfo to delete
  const WCHAR* szText = g_localizeStrings.Get(181).c_str();
  CGUIDialogSelect *pDlg = (CGUIDialogSelect*)m_gWindowManager.GetWindow(WINDOW_DIALOG_SELECT);
  if (pDlg)
  {
    pDlg->SetHeading(szText);
    pDlg->Reset();
    for (int i = 0; i < (int)vecAlbums.size(); ++i)
    {
      CMusicDatabase::CAlbumCache& album = vecAlbums[i];
      pDlg->Add(album.strAlbum + " - " + album.strArtist);
    }
    pDlg->DoModal(m_gWindowManager.GetActiveWindow());

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
  if (!g_guiSettings.GetBool("MusicFiles.UseCDDB"))
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
    strFile.Format("%s\\cddb\\%x.cddb", g_stSettings.m_szAlbumDirectory, pCdInfo->GetCddbDiscId() );
    ::DeleteFile(strFile.c_str());
  }

  // Prepare cddb
  Xcddb cddb;
  CStdString strDir;
  strDir.Format("%s\\cddb", g_stSettings.m_szAlbumDirectory);
  cddb.setCacheDir(strDir);

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
    pDialogProgress->StartModal(m_gWindowManager.GetActiveWindow());
    while (pDialogProgress->GetEffectState() == EFFECT_IN)
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
        pDlgSelect->DoModal(m_gWindowManager.GetActiveWindow());

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
          pDialogOK->DoModal(m_gWindowManager.GetActiveWindow() );
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
  strCDDBFileMask.Format("%s\\cddb\\*.cddb", g_stSettings.m_szAlbumDirectory);

  map<ULONG, CStdString> mapCDDBIds;

  CAutoPtrFind hFind( FindFirstFile(strCDDBFileMask.c_str(), &wfd));
  if (!hFind.isValid())
  {
    CGUIDialogOK::ShowAndGetInput(313, 426, 0, 0);
    return ;
  }

  // Show a selectdialog that the user can select the albuminfo to delete
  const WCHAR* szText = g_localizeStrings.Get(181).c_str();
  CGUIDialogSelect *pDlg = (CGUIDialogSelect*)m_gWindowManager.GetWindow(WINDOW_DIALOG_SELECT);
  if (pDlg)
  {
    pDlg->SetHeading(szText);
    pDlg->Reset();
    CStdString strDir;
    strDir.Format("%s\\cddb", g_stSettings.m_szAlbumDirectory);
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
    pDlg->DoModal(m_gWindowManager.GetActiveWindow());

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
        strFile.Format("%s\\cddb\\%x.cddb", g_stSettings.m_szAlbumDirectory, it->first );
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
  if (dlgMusicScan->IsRunning())
  {
    CGUIDialogOK::ShowAndGetInput(189, 14057, 0, 0);
    return;
  }

  if (CGUIDialogYesNo::ShowAndGetInput(313, 333, 0, 0))
  {
    CGUIDialogProgress* dlgProgress = (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
    if (dlgProgress)
    {
      dlgProgress->StartModal(m_gWindowManager.GetActiveWindow());

      int iReturnString = g_musicDatabase.Cleanup(dlgProgress);
      g_musicDatabase.Close();

      if (iReturnString != ERROR_OK)
      {
        CGUIDialogOK::ShowAndGetInput(313, iReturnString, 0, 0);
      }
      dlgProgress->Close();
    }
  }
}

bool CMusicDatabase::GetGenresNav(VECGENRES& genres)
{
  try
  {
    genres.erase(genres.begin(), genres.end());

    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    // get primary genres for songs
    CStdString strSQL="select strGenre from genre "
                      "where (idGenre IN ("
                        "select distinct song.idGenre from song) "
                      "or idGenre IN ("
                        "select distinct exgenresong.idGenre from exgenresong)) ";

    // run query
    CLog::Log(LOGDEBUG, "CMusicDatabase::GetGenresNav() query: %s", strSQL.c_str());
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
      CStdString strGenre = m_pDS->fv("genre.strGenre").get_asString();
      genres.push_back(strGenre);
      m_pDS->next();
    }

    // cleanup
    m_pDS->close();

    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CMusicDatabase:GetGenresNav() failed");
  }
  return false;
}

bool CMusicDatabase::GetArtistsNav(VECARTISTS& artists, const CStdString &strGenre)
{
  try
  {
    artists.erase(artists.begin(), artists.end());

    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL = "select strArtist from artist ";

    if (strGenre.IsEmpty())
      strSQL +=         "where (idArtist IN "
                          "("
                          "select distinct song.idArtist from song" // All primary artists linked to a song
                          ") "
                        "or idArtist IN "
                          "("
                          "select distinct exartistsong.idArtist from exartistsong" // All extra artists linked to a song
                          ")"
                        ") ";
    else
      strSQL+=FormatSQL("where (idArtist IN "
                          "("
                          "select distinct song.idArtist from song " // All primary artists linked to primary genres
                            "join genre on song.idGenre=genre.idGenre "
                          "where genre.strGenre='%s'"
                          ") "
                        "or idArtist IN "
                          "("
                          "select distinct song.idArtist from song " // All primary artists linked to extra genres
                            "join exgenresong on song.idSong=exgenresong.idSong "
                            "join genre on exgenresong.idGenre=genre.idGenre "
                          "where genre.strGenre='%s'"
                          ")"
                        ") "
                        "or idArtist IN "
                          "("
                          "select distinct exartistsong.idArtist from exartistsong " // All extra artists linked to extra genres
                            "join song on exartistsong.idSong=song.idSong "
                            "join exgenresong on song.idSong=exgenresong.idSong "
                            "join genre on exgenresong.idGenre=genre.idGenre "
                          "where genre.strGenre='%s'"
                          ") "
                        "or idArtist IN "
                          "("
                          "select distinct exartistsong.idArtist from exartistsong " // All extra artists linked to primary genres
                            "join song on exartistsong.idSong=song.idSong "
                            "join genre on song.idGenre=genre.idGenre "
                          "where genre.strGenre='%s'"
                          ") "
                        , strGenre.c_str(), strGenre.c_str(), strGenre.c_str(), strGenre.c_str());

    CStdString strVariousArtists = g_localizeStrings.Get(340);
    long lVariousArtistsId = AddArtist(strVariousArtists);
    CStdString strTemp;
    strTemp.Format("and artist.idArtist<>%i ", lVariousArtistsId);
    strSQL+=strTemp;

    // run query
    CLog::Log(LOGDEBUG, "CMusicDatabase::GetArtistsNav() query: %s", strSQL.c_str());
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
      CStdString strArtist = m_pDS->fv("strArtist").get_asString();
      artists.push_back(strArtist);
      m_pDS->next();
    }
    // cleanup
    m_pDS->close();

    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CMusicDatabase::GetArtistsNav() failed");
  }
  return false;
}

bool CMusicDatabase::GetAlbumsNav(VECALBUMS& albums, const CStdString &strGenre, const CStdString &strArtist)
{
  try
  {
    albums.erase(albums.begin(), albums.end());

    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL = "select * from albumview ";

    // where clause
    CStdString strWhere;
    if (!strGenre.IsEmpty())
    {
      strWhere+=FormatSQL("where (idAlbum IN "
                            "("
                            "select distinct song.idAlbum from song " // All albums where the primary genre fits
                              "join genre on song.idGenre=genre.idGenre "
                            "where genre.strGenre='%s'"
                            ") "
                          "or idAlbum IN "
                            "("
                            "select distinct song.idAlbum from song " // All albums where extra genres fits
                              "join exgenresong on song.idSong=exgenresong.idSong "
                              "join genre on exgenresong.idGenre=genre.idGenre "
                            "where genre.strGenre='%s'"
                            ")"
                          ") "
                          , strGenre.c_str(), strGenre.c_str());
    }

    if (!strArtist.IsEmpty())
    {
      if (strWhere.IsEmpty())
        strWhere += "where ";
      else
        strWhere += "and ";

      strWhere +=FormatSQL("(idAlbum IN "
                             "("
                                "select distinct song.idAlbum from song "  // All albums where the primary artist fits
                                  "join artist on song.idArtist=artist.idArtist "
                                "where artist.strArtist='%s'"
                             ")"
                           " or idAlbum IN "
                             "("
                                "select distinct song.idAlbum from song "  // All albums where extra artists fit
                                  "join exartistsong on song.idSong=exartistsong.idSong "
                                  "join artist on exartistsong.idArtist=artist.idArtist "
                                "where artist.strArtist='%s'"
                             ")"
                           ") "
                           , strArtist.c_str(), strArtist.c_str());
    }

    strSQL += strWhere;

    // run query
    CLog::Log(LOGDEBUG, "CMusicDatabase::GetAlbumsNav() query: %s", strSQL.c_str());
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
      albums.push_back(GetAlbumFromDataset());
      m_pDS->next();
    }

    // cleanup
    m_pDS->close();
    return true;

  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CMusicDatabase::GetAlbumsNav() failed");
  }
  return false;
}

bool CMusicDatabase::GetSongsNav(CFileItemList& items, const CStdString &strGenre, const CStdString &strArtist, const CStdString &strAlbum, const CStdString &strAlbumPath, bool bClearItems /* = true */)
{
  try
  {
    DWORD time = timeGetTime();

    if (bClearItems)
      items.Clear();

    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    int iQueryType = 0;
    if (!strGenre.IsEmpty())
      iQueryType += 4;
    if (!strArtist.IsEmpty())
      iQueryType += 2;
    if (!strAlbum.IsEmpty())
      iQueryType += 1;

    CStdString strWhere;
    CStdString strSQL = "select * from songview ";


    if (!strAlbum.IsEmpty() && !strAlbumPath.IsEmpty())
      strWhere=FormatSQL("where (strAlbum='%s' and strPath='%s') ", strAlbum.c_str(), strAlbumPath.c_str());

    if (!strGenre.IsEmpty())
    {
      if (strWhere.IsEmpty())
        strWhere += "where ";
      else
        strWhere += "and ";

      strWhere += FormatSQL("(strGenre='%s' " // All songs where primary genre fits
                            "or idSong IN "
                              "("
                              "select distinct exgenresong.idSong from exgenresong " // All songs by where extra genres fit
                                "join genre on exgenresong.idGenre=genre.idGenre "
                              "where genre.strGenre='%s'"
                              ")"
                            ") "
                            , strGenre.c_str(), strGenre.c_str());
    }

    if (!strArtist.IsEmpty())
    {
      if (strWhere.IsEmpty())
        strWhere += "where ";
      else
        strWhere += "and ";

      strWhere += FormatSQL("(strArtist='%s' " // All songs where primary artist fits
                            "or idSong IN "
                              "("
                              "select distinct exartistsong.idSong from exartistsong " // All songs where extra artists fit
                                "join artist on exartistsong.idArtist=artist.idArtist "
                              "where artist.strArtist='%s'"
                              ")"
                            ") "
                            , strArtist.c_str(), strArtist.c_str());
    }

    strSQL += strWhere;

    // run query
    CLog::Log(LOGDEBUG, "CMusicDatabase::GetSongsNav() query: %s", strSQL.c_str());
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return false;
    }

    CLog::DebugLog("Time for actual SQL query = %d", timeGetTime() - time); time = timeGetTime();

    // get data from returned rows
    items.Reserve(iRowsFound);

    while (!m_pDS->eof())
    {
      CFileItem *item = new CFileItem(GetSongFromDataset());
      items.Add(item);
      m_pDS->next();
    }

    CLog::DebugLog("Time to retrieve songs from dataset = %d", timeGetTime() - time);

    // cleanup
    m_pDS->close();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CMusicDatabase::GetSongsNav() failed");
  }
  return false;
}

bool CMusicDatabase::UpdateOldVersion(float fVersion)
{
  if (NULL == m_pDB.get()) return false;
  if (NULL == m_pDS.get()) return false;
  if (NULL == m_pDS2.get()) return false;

  try
  {
    if (fVersion < 0.5f)
    { // Version 0 to 0.5 upgrade - we need to add the version table
      CLog::Log(LOGINFO, "creating versions table");
      m_pDS->exec("CREATE TABLE version (idVersion float)\n");
    }
    if (fVersion < 1.0f)
    {
      // version 0.5 to 1.0 upgrade - we need to add the thumbs table + run SetMusicThumbs()
      // on all elements and then produce a new songs table
      // check if we already have a thumbs table (could happen if the code below asserts for whatever reason)
      m_pDS->query("SELECT * FROM sqlite_master WHERE type = 'table' AND name = 'thumb'\n");
      if (m_pDS->num_rows() > 0)
      {
        m_pDS->close();
      }
      else
      { // create it
        CLog::Log(LOGINFO, "create thumbs table");
        m_pDS->exec("CREATE TABLE thumb ( idThumb integer primary key, strThumb text )\n");
      }
      m_pDS->exec("PRAGMA cache_size=8192\n");
      m_pDS->exec("PRAGMA synchronous='NORMAL'\n");
      m_pDS->exec("PRAGMA count_changes='OFF'\n");
      m_pDS2->exec("PRAGMA cache_size=8192\n");
      m_pDS2->exec("PRAGMA synchronous='NORMAL'\n");
      m_pDS2->exec("PRAGMA count_changes='OFF'\n");
      m_bOpen = true;
      // ok, now we need to run through our table + fill in all the thumbs we need
      CGUIDialogProgress *dialog = (CGUIDialogProgress *)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
      if (dialog)
      {
        dialog->SetHeading("Updating old database version");
        dialog->SetLine(0, "");
        dialog->SetLine(1, "");
        dialog->SetLine(2, "");
        dialog->StartModal(m_gWindowManager.GetActiveWindow());
        dialog->SetLine(1, "Creating newly formatted tables");
        dialog->Progress();
      }
      BeginTransaction();
      CLog::Log(LOGINFO, "Creating temporary songs table");
      m_pDS->exec("CREATE TEMPORARY TABLE tempsong ( idSong integer primary key, idAlbum integer, idPath integer, idArtist integer, iNumArtists integer, idGenre integer, iNumGenres integer, strTitle text, iTrack integer, iDuration integer, iYear integer, dwFileNameCRC text, strFileName text, iTimesPlayed integer, iStartOffset integer, iEndOffset integer)\n");
      CLog::Log(LOGINFO, "Copying songs into temporary song table");
      m_pDS->exec("INSERT INTO tempsong SELECT idSong,idAlbum,idPath,idArtist,iNumArtists,idGenre,iNumGenres,strTitle,iTrack,iDuration,iYear,dwFileNameCRC,strFileName,iTimesPlayed,iStartOffset,iEndOffset FROM song");
      CLog::Log(LOGINFO, "Destroying old songs table");
      m_pDS->exec("DROP TABLE song");
      CLog::Log(LOGINFO, "Creating new songs table");
      m_pDS->exec("CREATE TABLE song ( idSong integer primary key, idAlbum integer, idPath integer, idArtist integer, iNumArtists integer, idGenre integer, iNumGenres integer, strTitle text, iTrack integer, iDuration integer, iYear integer, dwFileNameCRC text, strFileName text, iTimesPlayed integer, iStartOffset integer, iEndOffset integer, idThumb integer)\n");
      CLog::Log(LOGINFO, "Copying songs into new songs table");
      m_pDS->exec("INSERT INTO song(idSong,idAlbum,idPath,idArtist,iNumArtists,idGenre,iNumGenres,strTitle,iTrack,iDuration,iYear,dwFileNameCRC,strFileName,iTimesPlayed,iStartOffset,iEndOffset) SELECT * FROM tempsong");
      CLog::Log(LOGINFO, "Deleting temporary songs table");
      m_pDS->exec("DROP TABLE tempsong");
      CommitTransaction();
      BeginTransaction();
      if (dialog)
      {
        dialog->SetLine(0, "Retrieving updated information on songs...");
        dialog->SetLine(1, "");
      }
      CLog::Log(LOGINFO, "Finding thumbs");
      if (!m_pDS2->query("SELECT * from song join album on song.idAlbum = album.idAlbum join path on path.idPath = song.idPath\n"))
        return false;
      if (m_pDS2->num_rows() > 0)
      {
        CStdString strProgress;
        strProgress.Format("Processing %i of %i", 1, m_pDS2->num_rows());
        if (dialog)
        {
          dialog->SetLine(2, strProgress);
          dialog->Progress();
        }
        // turn on thumb caching - mayaswell make it as fast as we can
        g_directoryCache.InitMusicThumbCache();
        // get data from returned rows
        int count = 1;
        while (!m_pDS2->eof())
        {
          if (!(count % 10))
          {
            strProgress.Format("Processing %i of %i", count, m_pDS2->num_rows());
            if (dialog)
            {
              dialog->SetLine(2, strProgress);
              dialog->Progress();
            }
          }
          CSong song;
          // construct a song to be transferred to a fileitem
          song.strAlbum = m_pDS2->fv("album.strAlbum").get_asString();
          song.iTrack = m_pDS2->fv("song.iTrack").get_asLong() ;
          song.iDuration = m_pDS2->fv("song.iDuration").get_asLong() ;
          song.iYear = m_pDS2->fv("song.iYear").get_asLong() ;
          song.strTitle = m_pDS2->fv("song.strTitle").get_asString();
          song.iTimedPlayed = m_pDS2->fv("song.iTimesPlayed").get_asLong();
          song.iStartOffset = m_pDS2->fv("song.iStartOffset").get_asLong();
          song.iEndOffset = m_pDS2->fv("song.iEndOffset").get_asLong();
          // Get filename with full path
          song.strFileName = m_pDS2->fv("path.strPath").get_asString();
          CUtil::AddDirectorySeperator(song.strFileName);
          song.strFileName += m_pDS2->fv("song.strFileName").get_asString();
          // ok, now transfer to a file item and obtain the thumb
          CFileItem item(song);
          item.SetMusicThumb();
          long idSong = m_pDS2->fv("song.idSong").get_asLong();
          // add any found thumb
          CStdString strThumb = item.GetThumbnailImage();
          long lThumb = AddThumb(strThumb);
          CStdString strSQL=FormatSQL("UPDATE song SET idThumb=%i where idSong=%i", lThumb, idSong);
          m_pDS->exec(strSQL.c_str());
          m_pDS2->next();
          count++;
        }
      }
      // cleanup
      m_pDS2->close();
      g_directoryCache.ClearMusicThumbCache();
      CommitTransaction();
      if (dialog) dialog->Close();

    }
    if (fVersion < 1.1f)
    {
      // version 1.0 to 1.1 upgrade - we need to create an index on the thumb table
      CLog::Log(LOGINFO, "create thumb index");
      m_pDS->exec("CREATE INDEX idxThumb ON thumb(strThumb)");
      CLog::Log(LOGINFO, "create thumb index successfull");
      fVersion = MUSIC_DATABASE_VERSION;
    }
    if (fVersion < 1.2f)
    {
      // version 1.1 to 1.2 upgrade - we need to add the musicbrainz columns to the song table
      CLog::Log(LOGINFO, "Updating song table with musicbrainz columns");
      BeginTransaction();
      CLog::Log(LOGINFO, "Creating temporary songs table");
      m_pDS->exec("CREATE TEMPORARY TABLE tempsong ( idSong integer primary key, idAlbum integer, idPath integer, idArtist integer, iNumArtists integer, idGenre integer, iNumGenres integer, strTitle text, iTrack integer, iDuration integer, iYear integer, dwFileNameCRC text, strFileName text, iTimesPlayed integer, iStartOffset integer, iEndOffset integer, idThumb integer)\n");
      CLog::Log(LOGINFO, "Copying songs into temporary song table");
      m_pDS->exec("INSERT INTO tempsong SELECT idSong,idAlbum,idPath,idArtist,iNumArtists,idGenre,iNumGenres,strTitle,iTrack,iDuration,iYear,dwFileNameCRC,strFileName,iTimesPlayed,iStartOffset,iEndOffset,idThumb FROM song");
      CLog::Log(LOGINFO, "Destroying old songs table");
      m_pDS->exec("DROP TABLE song");
      CLog::Log(LOGINFO, "Creating new songs table");
      m_pDS->exec("CREATE TABLE song ( idSong integer primary key, idAlbum integer, idPath integer, idArtist integer, iNumArtists integer, idGenre integer, iNumGenres integer, strTitle text, iTrack integer, iDuration integer, iYear integer, dwFileNameCRC text, strFileName text, strMusicBrainzTrackID text, strMusicBrainzArtistID text, strMusicBrainzAlbumID text, strMusicBrainzAlbumArtistID text, strMusicBrainzTRMID text, iTimesPlayed integer, iStartOffset integer, iEndOffset integer, idThumb integer)\n");
      CLog::Log(LOGINFO, "Copying songs into new songs table");
      m_pDS->exec("INSERT INTO song(idSong,idAlbum,idPath,idArtist,iNumArtists,idGenre,iNumGenres,strTitle,iTrack,iDuration,iYear,dwFileNameCRC,strFileName,iTimesPlayed,iStartOffset,iEndOffset,idThumb) SELECT * FROM tempsong");
      CLog::Log(LOGINFO, "Deleting temporary songs table");
      m_pDS->exec("DROP TABLE tempsong");
      CommitTransaction();
    }
    if (fVersion < 1.3f)
    {
      // version 1.2 to 1.3 upgrade - we need to create a date column when a song was played the last time
      CLog::Log(LOGINFO, "Updating song table with lastplayed column");
      BeginTransaction();
      CLog::Log(LOGINFO, "Creating temporary songs table");
      m_pDS->exec("CREATE TEMPORARY TABLE tempsong ( idSong integer primary key, idAlbum integer, idPath integer, idArtist integer, iNumArtists integer, idGenre integer, iNumGenres integer, strTitle text, iTrack integer, iDuration integer, iYear integer, dwFileNameCRC text, strFileName text, strMusicBrainzTrackID text, strMusicBrainzArtistID text, strMusicBrainzAlbumID text, strMusicBrainzAlbumArtistID text, strMusicBrainzTRMID text, iTimesPlayed integer, iStartOffset integer, iEndOffset integer, idThumb integer)\n");
      CLog::Log(LOGINFO, "Copying songs into temporary song table");
      m_pDS->exec("INSERT INTO tempsong SELECT idSong, idAlbum, idPath, idArtist, iNumArtists, idGenre, iNumGenres, strTitle, iTrack, iDuration, iYear, dwFileNameCRC, strFileName, strMusicBrainzTrackID, strMusicBrainzArtistID, strMusicBrainzAlbumID, strMusicBrainzAlbumArtistID, strMusicBrainzTRMID, iTimesPlayed, iStartOffset, iEndOffset, idThumb FROM song");
      CLog::Log(LOGINFO, "Destroying old songs table");
      m_pDS->exec("DROP TABLE song");
      CLog::Log(LOGINFO, "Creating new songs table");
      m_pDS->exec("CREATE TABLE song ( idSong integer primary key, idAlbum integer, idPath integer, idArtist integer, iNumArtists integer, idGenre integer, iNumGenres integer, strTitle text, iTrack integer, iDuration integer, iYear integer, dwFileNameCRC text, strFileName text, strMusicBrainzTrackID text, strMusicBrainzArtistID text, strMusicBrainzAlbumID text, strMusicBrainzAlbumArtistID text, strMusicBrainzTRMID text, iTimesPlayed integer, iStartOffset integer, iEndOffset integer, idThumb integer, lastplayed date)\n");
      CLog::Log(LOGINFO, "Copying songs into new songs table");
      m_pDS->exec("INSERT INTO song(idSong, idAlbum, idPath, idArtist, iNumArtists, idGenre, iNumGenres, strTitle, iTrack, iDuration, iYear, dwFileNameCRC, strFileName, strMusicBrainzTrackID, strMusicBrainzArtistID, strMusicBrainzAlbumID, strMusicBrainzAlbumArtistID, strMusicBrainzTRMID, iTimesPlayed, iStartOffset, iEndOffset, idThumb) SELECT * FROM tempsong");
      CLog::Log(LOGINFO, "Deleting temporary songs table");
      m_pDS->exec("DROP TABLE tempsong");
      CLog::Log(LOGINFO, "create idxSong1 index");
      m_pDS->exec("CREATE INDEX idxSong1 ON song(iTimesPlayed)");
      CLog::Log(LOGINFO, "create idxSong2 index");
      m_pDS->exec("CREATE INDEX idxSong2 ON song(lastplayed)");
      CLog::Log(LOGINFO, "create song view");
      m_pDS->exec("create view songview as select idSong, song.iNumArtists as iNumArtists, song.iNumGenres as iNumGenres, strTitle, iTrack, iDuration, iYear, dwFileNameCRC, strFileName, strMusicBrainzTrackID, strMusicBrainzArtistID, strMusicBrainzAlbumID, strMusicBrainzAlbumArtistID, strMusicBrainzTRMID, iTimesPlayed, iStartOffset, iEndOffset, lastplayed, strAlbum, strPath, strArtist, strGenre, strThumb from song join album on song.idAlbum=album.idAlbum join path on song.idPath=path.idPath join artist on song.idArtist=artist.idArtist join genre on song.idGenre=genre.idGenre join thumb on song.idThumb=thumb.idThumb");
      CLog::Log(LOGINFO, "create album view");
      m_pDS->exec("create view albumview as select idAlbum, strAlbum, iNumArtists, strArtist, strPath from album join path on album.idPath=path.idPath join artist on album.idArtist=artist.idArtist");
      CommitTransaction();
    }
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

    map <CStdString, CPathCache>::const_iterator it;

    it = m_thumbCache.find(strThumb1);
    if (it != m_thumbCache.end())
      return it->second.idPath;

    strSQL=FormatSQL( "select * from thumb where strThumb='%s'", strThumb.c_str());
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      // doesnt exists, add it
      strSQL=FormatSQL("insert into thumb (idThumb, strThumb) values( NULL, '%s' )", strThumb.c_str());
      m_pDS->exec(strSQL.c_str());

      CPathCache thumb;
      thumb.idPath = (long)sqlite3_last_insert_rowid(m_pDB->getHandle());
      thumb.strPath = strThumb1;
      m_thumbCache.insert(pair<CStdString, CPathCache>(thumb.strPath, thumb));
      return thumb.idPath;
    }
    else
    {
      CPathCache thumb;
      thumb.idPath = m_pDS->fv("idThumb").get_asLong();
      thumb.strPath = strThumb1;
      m_thumbCache.insert(pair<CStdString, CPathCache>(thumb.strPath, thumb));
      m_pDS->close();
      return thumb.idPath;
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "musicdatabase:unable to addthumb (%s)", strSQL.c_str());
  }

  return -1;
}

int CMusicDatabase::GetSongsCount()
{
  try
  {
    if (NULL == m_pDB.get()) return 0;
    if (NULL == m_pDS.get()) return 0;

    CStdString strSQL = "select count(idSong) as NumSongs from song";
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
    CLog::Log(LOGERROR, "CMusicDatabase::GetSongsCount() failed");
  }
  return 0;
}
