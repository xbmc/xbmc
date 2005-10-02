//********************************************************************************************************************************
//**
//** see docs/videodatabase.png for a diagram of the database
//**
//********************************************************************************************************************************

#include "stdafx.h"
#include "videodatabase.h"
#include "utils/fstrcmp.h"
#include "util.h"
#include "GUIPassword.h"

#define VIDEO_DATABASE_VERSION 1.4f
#define VIDEO_DATABASE_NAME "MyVideos31.db"

//********************************************************************************************************************************
CVideoDatabase::CVideoDatabase(void)
{
  m_fVersion=VIDEO_DATABASE_VERSION;
  m_strDatabaseFile=VIDEO_DATABASE_NAME;
}

//********************************************************************************************************************************
CVideoDatabase::~CVideoDatabase(void)
{}

//********************************************************************************************************************************
bool CVideoDatabase::CreateTables()
{

  try
  {
    CDatabase::CreateTables();

    CLog::Log(LOGINFO, "create bookmark table");
    m_pDS->exec("CREATE TABLE bookmark ( idBookmark integer primary key, idFile integer, timeInSeconds integer, thumbNailImage text)\n");

    CLog::Log(LOGINFO, "create settings table");
    m_pDS->exec("CREATE TABLE settings ( idFile integer, Interleaved bool, NoCache bool, Deinterlace bool, FilmGrain integer,"
                "ViewMode integer,ZoomAmount float, PixelRatio float, AudioStream integer, SubtitleStream integer,"
                "SubtitleDelay float, SubtitlesOn bool, Brightness integer, Contrast integer, Gamma integer,"
                "AdjustFrameRate integer, AudioDelay float, ResumeTime integer, Crop bool, CropLeft integer,"
                "CropRight integer, CropTop integer, CropBottom integer)\n");

    CLog::Log(LOGINFO, "create genre table");
    m_pDS->exec("CREATE TABLE genre ( idGenre integer primary key, strGenre text)\n");

    CLog::Log(LOGINFO, "create genrelinkmovie table");
    m_pDS->exec("CREATE TABLE genrelinkmovie ( idGenre integer, idMovie integer)\n");

    CLog::Log(LOGINFO, "create movie table");
    m_pDS->exec("CREATE TABLE movie ( idMovie integer primary key, idPath integer, hasSubtitles integer, cdlabel text)\n");

    CLog::Log(LOGINFO, "create movieinfo table");
    m_pDS->exec("CREATE TABLE movieinfo ( idMovie integer, idDirector integer, strPlotOutline text, strPlot text, strTagLine text, strVotes text, strRuntime text, fRating text, strCast text,strCredits text, iYear integer, strGenre text, strPictureURL text, strTitle text, IMDBID text)\n");

    CLog::Log(LOGINFO, "create actorlinkmovie table");
    m_pDS->exec("CREATE TABLE actorlinkmovie ( idActor integer, idMovie integer )\n");

    CLog::Log(LOGINFO, "create actors table");
    m_pDS->exec("CREATE TABLE actors ( idActor integer primary key, strActor text )\n");

    CLog::Log(LOGINFO, "create path table");
    m_pDS->exec("CREATE TABLE path ( idPath integer primary key, strPath text)\n");

    CLog::Log(LOGINFO, "create files table");
    m_pDS->exec("CREATE TABLE files ( idFile integer primary key, idPath integer, idMovie integer,strFilename text)\n");
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "videodatabase::unable to create tables:%i", GetLastError());
    return false;
  }

  return true;
}

//********************************************************************************************************************************
long CVideoDatabase::AddFile(long lMovieId, long lPathId, const CStdString& strFileName)
{
  CStdString strSQL = "";
  try
  {
    long lFileId;
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    strSQL=FormatSQL("select * from files where idmovie=%i and idpath=%i and strFileName like '%s'", lMovieId, lPathId, strFileName.c_str());
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() > 0)
    {
      lFileId = m_pDS->fv("idFile").get_asLong() ;
      m_pDS->close();
      return lFileId;
    }
    m_pDS->close();
    strSQL=FormatSQL("insert into files (idFile, idMovie,idPath, strFileName) values(NULL, %i,%i,'%s')", lMovieId, lPathId, strFileName.c_str() );
    m_pDS->exec(strSQL.c_str());
    lFileId = (long)sqlite3_last_insert_rowid( m_pDB->getHandle() );
    return lFileId;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "videodatabase:unable to addfile (%s)", strSQL.c_str());
  }
  return -1;
}


//********************************************************************************************************************************
long CVideoDatabase::GetFile(const CStdString& strFilenameAndPath, long &lPathId, long& lMovieId, bool bExact)
{
  try
  {
    lPathId = -1;
    lMovieId = -1;
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;
    CStdString strPath, strFileName ;
    Split(strFilenameAndPath, strPath, strFileName);

    // check for a stackable file
    CStdString fileTitle;
    CStdString volumeNumber;
    if (!bExact && !CUtil::GetVolumeFromFileName(strFileName, fileTitle, volumeNumber))
      bExact = true;  // can't stack this file

    lPathId = GetPath(strPath);
    if (lPathId < 0) return -1;

    CStdString strSQL;
    strSQL=FormatSQL("select * from files where idpath=%i", lPathId);
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() > 0)
    {
      while (!m_pDS->eof())
      {
        CStdString strFname = m_pDS->fv("strFilename").get_asString() ;
        if (bExact)
        {
          if (strFname == strFileName)
          {
            // was just returning 'true' here, but this caused problems with
            // the bookmarks as these are stored by fileid. forza.
            long lFileId = m_pDS->fv("idFile").get_asLong() ;
            lMovieId = m_pDS->fv("idMovie").get_asLong() ;
            m_pDS->close();
            return lFileId;
            //return true;
          }
        }
        else
        {
          CStdString fileTitle2;
          CStdString volumeNumber2;
          if (CUtil::GetVolumeFromFileName(strFname, fileTitle2, volumeNumber2))
          {
            if (fileTitle.Equals(fileTitle2))
            { // found a file already
              long lFileId = m_pDS->fv("idFile").get_asLong() ;
              lMovieId = m_pDS->fv("idMovie").get_asLong() ;
              m_pDS->close();
              return lFileId;
            }
          }
          /*
          double fPercentage = fstrcmp(strFname.c_str(), strFileName.c_str(), COMPARE_PERCENTAGE_MIN );
          if ( fPercentage >= COMPARE_PERCENTAGE)
          {
            long lFileId = m_pDS->fv("idFile").get_asLong() ;
            lMovieId = m_pDS->fv("idMovie").get_asLong() ;
            m_pDS->close();
            return lFileId;
          }
          */
        }
        m_pDS->next();
      }
      m_pDS->close();
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetFile(%s) failed", strFilenameAndPath.c_str());
  }
  return -1;
}


//********************************************************************************************************************************
long CVideoDatabase::AddPath(const CStdString& strPath)
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    CStdString strSQL=FormatSQL("select * from path where strPath like '%s'", strPath.c_str());
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      // doesnt exists, add it
      strSQL=FormatSQL("insert into Path (idPath, strPath) values( NULL, '%s')",
                    strPath.c_str());
      m_pDS->exec(strSQL.c_str());
      long lPathId = (long)sqlite3_last_insert_rowid(m_pDB->getHandle());
      return lPathId;
    }
    else
    {
      const field_value value = m_pDS->fv("idPath");
      long lPathId = value.get_asLong() ;
      m_pDS->close();
      return lPathId;
    }

  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::AddPath(%s) failed", strPath.c_str());
  }
  return -1;
}

//********************************************************************************************************************************
long CVideoDatabase::GetPath(const CStdString& strPath)
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    CStdString strSQL=FormatSQL("select * from path where strPath like '%s' ", strPath.c_str());
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() > 0)
    {
      long lPathId = m_pDS->fv("idPath").get_asLong();
      m_pDS->close();
      return lPathId;
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetPath(%s) failed", strPath.c_str());
  }
  return -1;
}

//********************************************************************************************************************************
long CVideoDatabase::GetMovie(const CStdString& strFilenameAndPath)
{
  long lPathId;
  long lMovieId;
  if (GetFile(strFilenameAndPath, lPathId, lMovieId) < 0)
  {
    return -1;
  }
  return lMovieId;
}

int CVideoDatabase::GetRecentMovies(long* pMovieIdArray, int nSize)
{
  int count = 0;

  try
  {
    if (NULL == m_pDB.get())
    {
      return -1;
    }

    if (NULL == m_pDS.get())
    {
      return -1;
    }

    CStdString strSQL=FormatSQL("select * from movie order by idMovie desc limit %d", nSize);
    if (m_pDS->query(strSQL.c_str()))
    {
      while ((!m_pDS->eof()) && (count < nSize))
      {
        pMovieIdArray[count++] = m_pDS->fv("idMovie").get_asLong() ;
        m_pDS->next();
      }
      m_pDS->close();
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetRecentMovies failed.");
  }

  return count;
}


//********************************************************************************************************************************
long CVideoDatabase::AddMovie(const CStdString& strFilenameAndPath, const CStdString& strcdLabel, bool bHassubtitles)
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;
    CStdString strPath, strFileName;
    Split(strFilenameAndPath, strPath, strFileName);

    long lMovieId = GetMovie(strFilenameAndPath);
    if (lMovieId < 0)
    {
      long lPathId = AddPath(strPath);
      if (lPathId < 0) return -1;
      CStdString strSQL=FormatSQL("insert into movie (idMovie, idPath, hasSubtitles, cdlabel) values( NULL, %i, %i, '%s')",
                    lPathId, bHassubtitles, strcdLabel.c_str());
      m_pDS->exec(strSQL.c_str());
      lMovieId = (long)sqlite3_last_insert_rowid(m_pDB->getHandle());
      AddFile(lMovieId, lPathId, strFileName);
    }
    else
    {
      long lPathId = GetPath(strPath);
      if (lPathId < 0) return -1;
      AddFile(lMovieId, lPathId, strFileName);
    }
    return lMovieId;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::AddMovie(%s,%s) failed", strFilenameAndPath.c_str() , strcdLabel.c_str() );
  }
  return -1;
}


//********************************************************************************************************************************
long CVideoDatabase::AddGenre(const CStdString& strGenre)
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    CStdString strSQL=FormatSQL("select * from genre where strGenre like '%s'", strGenre.c_str());
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      // doesnt exists, add it
      strSQL=FormatSQL("insert into genre (idGenre, strGenre) values( NULL, '%s')", strGenre.c_str());
      m_pDS->exec(strSQL.c_str());
      long lGenreId = (long)sqlite3_last_insert_rowid(m_pDB->getHandle());
      return lGenreId;
    }
    else
    {
      const field_value value = m_pDS->fv("idGenre");
      long lGenreId = value.get_asLong() ;
      m_pDS->close();
      return lGenreId;
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::AddGenre(%s) failed", strGenre.c_str() );
  }

  return -1;
}


//********************************************************************************************************************************
long CVideoDatabase::AddActor(const CStdString& strActor)
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;
    CStdString strSQL=FormatSQL("select * from Actors where strActor like '%s'", strActor.c_str());
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      // doesnt exists, add it
      strSQL=FormatSQL("insert into Actors (idActor, strActor) values( NULL, '%s')", strActor.c_str());
      m_pDS->exec(strSQL.c_str());
      long lActorId = (long)sqlite3_last_insert_rowid(m_pDB->getHandle());
      return lActorId;
    }
    else
    {
      const field_value value = m_pDS->fv("idActor");
      long lActorId = value.get_asLong() ;
      m_pDS->close();
      return lActorId;
    }

  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::AddActor(%s) failed", strActor.c_str() );
  }
  return -1;
}
//********************************************************************************************************************************
void CVideoDatabase::AddGenreToMovie(long lMovieId, long lGenreId)
{
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    CStdString strSQL=FormatSQL("select * from genrelinkmovie where idGenre=%i and idMovie=%i", lGenreId, lMovieId);
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() == 0)
    {
      // doesnt exists, add it
      strSQL=FormatSQL("insert into genrelinkmovie (idGenre, idMovie) values( %i,%i)", lGenreId, lMovieId);
      m_pDS->exec(strSQL.c_str());
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::AddGenreToMovie() failed");
  }
}

//********************************************************************************************************************************
void CVideoDatabase::AddActorToMovie(long lMovieId, long lActorId)
{
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    CStdString strSQL=FormatSQL("select * from actorlinkmovie where idActor=%i and idMovie=%i", lActorId, lMovieId);
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() == 0)
    {
      // doesnt exists, add it
      strSQL=FormatSQL("insert into actorlinkmovie (idActor, idMovie) values( %i,%i)", lActorId, lMovieId);
      m_pDS->exec(strSQL.c_str());
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::AddActorToMovie() failed");
  }
}

//********************************************************************************************************************************
void CVideoDatabase::GetGenres(VECMOVIEGENRES& genres)
{
  try
  {
    genres.erase(genres.begin(), genres.end());
    map<long, CStdString> mapGenres;
    map<long, CStdString>::iterator it;

    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;
    //m_pDS->query("select distinct genre.strGenre from genre join genrelinkmovie on genre.idGenre = genrelinkmovie.idGenre join movie on genrelinkmovie.idMovie = movie.idMovie");
    m_pDS->query("select * from genrelinkmovie,genre,movie,movieinfo,actors,path where path.idpath=movie.idpath and genrelinkmovie.idGenre=genre.idGenre and genrelinkmovie.idmovie=movie.idmovie and movieinfo.idmovie=movie.idmovie and movieinfo.iddirector=actors.idActor order by path.idpath");
    long lLastPathId = -1;
    while (!m_pDS->eof())
    {
      long lGenreId = m_pDS->fv("genre.idgenre").get_asLong();
      CStdString strGenre = m_pDS->fv("genre.strgenre").get_asString();
      it = mapGenres.find(lGenreId);
      // was this genre already found?
      if (it == mapGenres.end())
      {
        // is the current path the same as the previous path?
        long lPathId = m_pDS->fv("path.idpath").get_asLong();
        if (lPathId == lLastPathId)
          mapGenres.insert(pair<long, CStdString>(lGenreId, strGenre));
        // test path
        else
        {
          CStdString strPath = m_pDS->fv("path.strPath").get_asString();
          if (g_passwordManager.IsDatabasePathUnlocked(strPath, g_settings.m_vecMyVideoShares))
          {
            // the path is unlocked so set last path to current path
            lLastPathId = lPathId;
            mapGenres.insert(pair<long, CStdString>(lGenreId, strGenre));
          }
        }
      }
      m_pDS->next();
    }
    m_pDS->close();

    // convert map back to vector
    for (it = mapGenres.begin(); it != mapGenres.end(); it++)
      genres.push_back(it->second);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetGenres() failed");
  }
}

//********************************************************************************************************************************
void CVideoDatabase::GetActors(VECMOVIEACTORS& actors)
{
  try
  {
    actors.erase(actors.begin(), actors.end());
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;
    //m_pDS->query("select * from actors join actorlinkmovie on actors.idActor = actorlinkmovie.idActor");
    m_pDS->query("select distinct actors.strActor from actors join actorlinkmovie on actors.idActor = actorlinkmovie.idActor join movie on actorlinkmovie.idMovie = movie.idMovie");
    while (!m_pDS->eof())
    {
      actors.push_back( m_pDS->fv("actors.strActor").get_asString() );
      m_pDS->next();
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetActors() failed");
  }
}

//********************************************************************************************************************************
bool CVideoDatabase::HasMovieInfo(const CStdString& strFilenameAndPath)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    long lMovieId = GetMovie(strFilenameAndPath);
    if ( lMovieId < 0) return false;

    CStdString strSQL=FormatSQL("select * from movieinfo where movieinfo.idmovie=%i", lMovieId);
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      return false;
    }
    m_pDS->close();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::HasMovieInfo(%s) failed", strFilenameAndPath.c_str());
  }
  return false;

}

//********************************************************************************************************************************
bool CVideoDatabase::HasSubtitle(const CStdString& strFilenameAndPath)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    long lMovieId = GetMovie(strFilenameAndPath);
    if ( lMovieId < 0) return false;

    CStdString strSQL=FormatSQL("select * from movie where movie.idMovie=%i", lMovieId);
    m_pDS->query(strSQL.c_str());
    long lHasSubs = 0;
    if (m_pDS->num_rows() > 0)
    {
      lHasSubs = m_pDS->fv("hasSubtitles").get_asLong() ;
    }
    m_pDS->close();
    return (lHasSubs != 0);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::HasSubtitle(%s) failed", strFilenameAndPath.c_str());
  }
  return false;
}

//********************************************************************************************************************************
void CVideoDatabase::DeleteMovieInfo(const CStdString& strFileNameAndPath)
{
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    long lMovieId = GetMovie(strFileNameAndPath);
    if ( lMovieId < 0) return ;

    CStdString strSQL;
    strSQL=FormatSQL("delete from genrelinkmovie where idmovie=%i", lMovieId);
    m_pDS->exec(strSQL.c_str());

    strSQL=FormatSQL("delete from actorlinkmovie where idmovie=%i", lMovieId);
    m_pDS->exec(strSQL.c_str());

    strSQL=FormatSQL("delete from movieinfo where idmovie=%i", lMovieId);
    m_pDS->exec(strSQL.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::DeleteMovieInfo(%s) failed", strFileNameAndPath.c_str());
  }
}

//********************************************************************************************************************************
void CVideoDatabase::GetMoviesByGenre(CStdString& strGenre, VECMOVIES& movies)
{
  try
  {
    movies.erase(movies.begin(), movies.end());
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    CStdString strSQL=FormatSQL("select * from genrelinkmovie,genre,movie,movieinfo,actors,path where path.idpath=movie.idpath and genrelinkmovie.idGenre=genre.idGenre and genrelinkmovie.idmovie=movie.idmovie and movieinfo.idmovie=movie.idmovie and genre.strGenre='%s' and movieinfo.iddirector=actors.idActor", strGenre.c_str());
    m_pDS->query( strSQL.c_str() );

    int lLastPathId = -1;
    while (!m_pDS->eof())
    {
      // if the current path is the same as the last path tested and unlocked,
      // just add the movie without retesting
      long lPathId = m_pDS->fv("path.idPath").get_asLong();
      if (lPathId == lLastPathId)
        movies.push_back(GetDetailsFromDataset(m_pDS));
      // we have a new path so test it.
      else
      {
        CStdString strPath = m_pDS->fv("path.strPath").get_asString();
        if (g_passwordManager.IsDatabasePathUnlocked(strPath, g_settings.m_vecMyVideoShares))
        {
          // the path is unlocked so set last path to current path
          lLastPathId = lPathId;
          movies.push_back(GetDetailsFromDataset(m_pDS));
        }
      }
      m_pDS->next();
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetMoviesByGenre(%s) failed", strGenre.c_str());
  }
}

//********************************************************************************************************************************
void CVideoDatabase::GetMoviesByActor(CStdString& strActor, VECMOVIES& movies)
{
  try
  {
    movies.erase(movies.begin(), movies.end());
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    CStdString strSQL=FormatSQL("select * from actorlinkmovie,actors,movie,movieinfo,path where path.idpath=movie.idpath and actors.idActor=actorlinkmovie.idActor and actorlinkmovie.idmovie=movie.idmovie and movieinfo.idmovie=movie.idmovie and actors.stractor='%s'", strActor.c_str());
    m_pDS->query( strSQL.c_str() );
    while (!m_pDS->eof())
    {
      movies.push_back(GetDetailsFromDataset(m_pDS));
      m_pDS->next();
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetMoviesByActor(%s) failed", strActor.c_str());
  }
}


//********************************************************************************************************************************
void CVideoDatabase::GetMovieInfo(const CStdString& strFilenameAndPath, CIMDBMovie& details, long lMovieId /* = -1 */)
{
  try
  {
    if (lMovieId < 0)
      lMovieId = GetMovie(strFilenameAndPath);
    if (lMovieId < 0) return ;

    CStdString strSQL=FormatSQL("select * from movieinfo,actors,movie,path where path.idpath=movie.idpath and movie.idMovie=movieinfo.idMovie and movieinfo.idmovie=%i and idDirector=idActor", lMovieId);

    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() > 0)
    {
      details = GetDetailsFromDataset(m_pDS);
      if (details.m_strWritingCredits.IsEmpty())
      { // try loading off disk
        CIMDB imdb;
        imdb.LoadDetails(details.m_strIMDBNumber, details);
      }
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetMovieInfo(%s) failed", strFilenameAndPath.c_str());
  }
}

//********************************************************************************************************************************
void CVideoDatabase::SetMovieInfo(const CStdString& strFilenameAndPath, CIMDBMovie& details)
{
  try
  {
    long lMovieId = GetMovie(strFilenameAndPath);
    if (lMovieId < 0) return ;
    details.m_strSearchString.Format("%i", lMovieId);

    CStdString strPath, strFileName ;
    Split(strFilenameAndPath, strPath, strFileName);
    details.m_strPath = strPath;
    details.m_strFile = strFileName;

    // add director
    long lDirector = AddActor(details.m_strDirector);

    // add all genres
    char szGenres[1024];
    strcpy(szGenres, details.m_strGenre.c_str());
    vector<long> vecGenres;
    if (strstr(szGenres, "/"))
    {
      char *pToken = strtok(szGenres, "/");
      while ( pToken != NULL )
      {
        CStdString strGenre = pToken;
        strGenre.Trim();
        long lGenreId = AddGenre(strGenre);
        vecGenres.push_back(lGenreId);
        pToken = strtok( NULL, "/" );
      }
    }
    else
    {
      CStdString strGenre = details.m_strGenre;
      strGenre.Trim();
      long lGenreId = AddGenre(strGenre);
      vecGenres.push_back(lGenreId);
    }

    // add cast...
    vector<long> vecActors;
    CStdString strCast;
    int ipos = 0;
    strCast = details.m_strCast.c_str();
    ipos = strCast.Find(" as ");
    while (ipos > 0)
    {
      CStdString strActor;
      int x = ipos;
      while (x > 0)
      {
        if (strCast[x] != '\r' && strCast[x] != '\n') x--;
        else
        {
          x++;
          break;
        }
      }
      strActor = strCast.Mid(x, ipos - x);
      long lActorId = AddActor(strActor);
      vecActors.push_back(lActorId);
      ipos = strCast.Find(" as ", ipos + 3);
    }

    for (int i = 0; i < (int)vecGenres.size(); ++i)
    {
      AddGenreToMovie(lMovieId, vecGenres[i]);
    }

    for (i = 0; i < (int)vecActors.size(); ++i)
    {
      AddActorToMovie(lMovieId, vecActors[i]);
    }

    CStdString strSQL;
    CStdString strRating;
    strRating.Format("%3.3f", details.m_fRating);
    if (strRating == "") strRating = "0.0";
    strSQL=FormatSQL("select * from movieinfo where idmovie=%i", lMovieId);
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      CStdString strTmp = "";
      strSQL=FormatSQL("insert into movieinfo ( idMovie,idDirector,strRuntime,fRating,iYear,strTitle,IMDBID) values(%i,%i,'%s','%s',%i,'%s','%s')",
                    lMovieId, lDirector, details.m_strRuntime.c_str(), strRating.c_str(),
                    details.m_iYear, details.m_strTitle.c_str(),
                    details.m_strIMDBNumber.c_str() );

      m_pDS->exec(strSQL.c_str());

    }
    else
    {
      m_pDS->close();
      strSQL=FormatSQL("update movieinfo set idDirector=%i, strRuntime='%s', fRating='%s', iYear=%i, strTitle='%s', IMDBID='%s' where idMovie=%i",
                    lDirector, details.m_strRuntime.c_str(), strRating.c_str(),
                    details.m_iYear, details.m_strTitle.c_str(),
                    details.m_strIMDBNumber.c_str(),
                    lMovieId);
      m_pDS->exec(strSQL.c_str());
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::SetMovieInfo(%s) failed", strFilenameAndPath.c_str());
  }
}


//********************************************************************************************************************************
void CVideoDatabase::GetMoviesByPath(CStdString& strPath1, VECMOVIES& movies)
{
  try
  {
    CStdString strPath = strPath1;
    if (CUtil::HasSlashAtEnd(strPath)) strPath = strPath.Left(strPath.size() - 1);

    movies.erase(movies.begin(), movies.end());
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;
    long lPathId = GetPath(strPath);
    if (lPathId < 0) return ;

    CStdString strSQL=FormatSQL("select * from files,movieinfo where files.idpath=%i and files.idMovie=movieinfo.idMovie", lPathId);

    m_pDS->query( strSQL.c_str() );
    while (!m_pDS->eof())
    {
      CIMDBMovie details;
      long lMovieId = m_pDS->fv("files.idMovie").get_asLong();
      details.m_strSearchString.Format("%i", lMovieId);
      details.m_strIMDBNumber = m_pDS->fv("movieinfo.IMDBID").get_asString();
      details.m_strFile = m_pDS->fv("files.strFilename").get_asString();
      movies.push_back(details);
      m_pDS->next();
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetMoviesByPath(%s) failed", strPath1.c_str());
  }
}

//********************************************************************************************************************************
void CVideoDatabase::GetFiles(long lMovieId, VECMOVIESFILES& movies)
{
  try
  {
    movies.erase(movies.begin(), movies.end());
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    //long lMovieId=GetMovie(strFile);
    if (lMovieId < 0) return ;

    CStdString strSQL=FormatSQL("select * from path,files where path.idPath=files.idPath and files.idmovie=%i order by strFilename", lMovieId );
    m_pDS->query( strSQL.c_str() );
    while (!m_pDS->eof())
    {
      CStdString strPath, strFile;
      strFile = m_pDS->fv("files.strFilename").get_asString();
      strPath = m_pDS->fv("path.strPath").get_asString();
      strFile = strPath + strFile;
      movies.push_back(strFile);
      m_pDS->next();
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetFiles() failed");
  }
}

//********************************************************************************************************************************
void CVideoDatabase::GetYears(VECMOVIEYEARS& years)
{
  try
  {
    years.erase(years.begin(), years.end());
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;
    //m_pDS->query("select * from movieinfo");
    m_pDS->query("select distinct movieinfo.iYear from movieinfo join movie on movieinfo.idMovie = movie.idMovie");
    while (!m_pDS->eof())
    {
      /*
      CStdString strYear = m_pDS->fv("iYear").get_asString();
      bool bAdd = true;
      for (int i = 0; i < (int)years.size();++i)
      {
        if (strYear == years[i]) bAdd = false;
      }
      if (bAdd) years.push_back( strYear);
      */
      years.push_back(m_pDS->fv("movieinfo.iYear").get_asString());
      m_pDS->next();
    }
    m_pDS->close();

  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetYears() failed");
  }
}

//********************************************************************************************************************************
void CVideoDatabase::GetMoviesByYear(CStdString& strYear, VECMOVIES& movies)
{
  try
  {
    int iYear = atoi(strYear.c_str());
    movies.erase(movies.begin(), movies.end());
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    CStdString strSQL=FormatSQL("select * from movie,movieinfo,actors,path where path.idpath=movie.idpath and movieinfo.idmovie=movie.idmovie and movieinfo.iddirector=actors.idActor and movieinfo.iYear=%i", iYear);

    m_pDS->query( strSQL.c_str() );
    while (!m_pDS->eof())
    {
      movies.push_back(GetDetailsFromDataset(m_pDS));
      m_pDS->next();
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetMoviesByYear(%s) failed", strYear.c_str());
  }
}

bool SortBookmarks(const CBookmark &left, const CBookmark &right)
{
  return left.timeInSeconds < right.timeInSeconds;
}

//********************************************************************************************************************************
void CVideoDatabase::GetBookMarksForMovie(const CStdString& strFilenameAndPath, VECBOOKMARKS& bookmarks)
{
  try
  {
    long lPathId, lMovieId;
    long lFileId = GetFile(strFilenameAndPath, lPathId, lMovieId, true);
    if (lFileId < 0) return ;
    bookmarks.erase(bookmarks.begin(), bookmarks.end());
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    CStdString strSQL=FormatSQL("select * from bookmark where idFile=%i order by timeInSeconds", lFileId);
    m_pDS->query( strSQL.c_str() );
    while (!m_pDS->eof())
    {
      CBookmark bookmark;
      bookmark.timeInSeconds = m_pDS->fv("timeInSeconds").get_asLong();
      bookmark.thumbNailImage = m_pDS->fv("thumbNailImage").get_asString();
      bookmarks.push_back(bookmark);
      m_pDS->next();
    }
    sort(bookmarks.begin(), bookmarks.end(), SortBookmarks);
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetBookMarksForMovie(%s) failed", strFilenameAndPath.c_str());
  }
}

//********************************************************************************************************************************
void CVideoDatabase::AddBookMarkToMovie(const CStdString& strFilenameAndPath, const CBookmark &bookmark)
{
  try
  {
    long lPathId, lMovieId;
    long lFileId = GetFile(strFilenameAndPath, lPathId, lMovieId, true);
    if (lFileId < 0) return ;
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    CStdString strSQL=FormatSQL("select * from bookmark where idFile=%i and timeInSeconds=%i", lFileId, bookmark.timeInSeconds);
    m_pDS->query( strSQL.c_str() );
    if (m_pDS->num_rows() != 0)
    {
      m_pDS->close();
      return ;
    }
    m_pDS->close();
    strSQL=FormatSQL("insert into bookmark (idBookmark, idFile, timeInSeconds, thumbNailImage) values(NULL,%i,%i,'%s')", lFileId, bookmark.timeInSeconds, bookmark.thumbNailImage.c_str());
    m_pDS->exec(strSQL.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::AddBookMarkToMovie(%s) failed", strFilenameAndPath.c_str());
  }
}

//********************************************************************************************************************************
void CVideoDatabase::ClearBookMarksOfMovie(const CStdString& strFilenameAndPath)
{
  try
  {
    long lPathId, lMovieId;
    long lFileId = GetFile(strFilenameAndPath, lPathId, lMovieId, true);
    if (lFileId < 0) return ;
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    CStdString strSQL=FormatSQL("delete from bookmark where idFile=%i", lFileId);
    m_pDS->exec(strSQL.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::ClearBookMarksOfMovie(%s) failed", strFilenameAndPath.c_str());
  }
}

//********************************************************************************************************************************
void CVideoDatabase::GetMovies(VECMOVIES& movies)
{
  try
  {
    movies.erase(movies.begin(), movies.end());
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    CStdString strSQL="select * from movie,movieinfo,actors,path where movieinfo.idmovie=movie.idmovie and movieinfo.iddirector=actors.idActor and movie.idpath=path.idpath order by path.idpath";
    m_pDS->query( strSQL.c_str() );

    int lLastPathId = -1;
    while (!m_pDS->eof())
    {
      // if the current path is the same as the last path tested and unlocked,
      // just add the movie without retesting
      long lPathId = m_pDS->fv("path.idPath").get_asLong();
      if (lPathId == lLastPathId)
        movies.push_back(GetDetailsFromDataset(m_pDS));
      // we have a new path so test it.
      else
      {
        CStdString strPath = m_pDS->fv("path.strPath").get_asString();
        if (g_passwordManager.IsDatabasePathUnlocked(strPath, g_settings.m_vecMyVideoShares))
        {
          // the path is unlocked so set last path to current path
          lLastPathId = lPathId;
          movies.push_back(GetDetailsFromDataset(m_pDS));
        }
      }
      m_pDS->next();
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetMovies() failed");
  }
}

//********************************************************************************************************************************
void CVideoDatabase::DeleteMovie(const CStdString& strFilenameAndPath)
{
  try
  {
    long lPathId;
    long lMovieId;
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;
    if (GetFile(strFilenameAndPath, lPathId, lMovieId) < 0)
    {
      return ;
    }

    ClearBookMarksOfMovie(strFilenameAndPath);

    CStdString strSQL;
    strSQL=FormatSQL("delete from files where idmovie=%i", lMovieId);
    m_pDS->exec(strSQL.c_str());

    strSQL=FormatSQL("delete from genrelinkmovie where idmovie=%i", lMovieId);
    m_pDS->exec(strSQL.c_str());

    strSQL=FormatSQL("delete from actorlinkmovie where idmovie=%i", lMovieId);
    m_pDS->exec(strSQL.c_str());

    strSQL=FormatSQL("delete from movieinfo where idmovie=%i", lMovieId);
    m_pDS->exec(strSQL.c_str());

    strSQL=FormatSQL("delete from movie where idmovie=%i", lMovieId);
    m_pDS->exec(strSQL.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::DeleteMovie() failed");
  }
}

//********************************************************************************************************************************
void CVideoDatabase::SetDVDLabel(long lMovieId, const CStdString& strDVDLabel)
{
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;
    CLog::Log(LOGERROR, "setdvdlabel:id:%i label:%s", lMovieId, strDVDLabel.c_str());

    CStdString strSQL;

    strSQL.Format("update movie set cdlabel='%s' where idMovie=%i", strDVDLabel.c_str(), lMovieId );
    m_pDS->exec(strSQL.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::SetDVDLabel() failed");
  }
}

//********************************************************************************************************************************
void CVideoDatabase::GetDVDLabel(long lMovieId, CStdString& strDVDLabel)
{
  strDVDLabel = "";
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    CStdString strSQL=FormatSQL("select cdlabel from movie where idMovie=%i", lMovieId);
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() == 1)
    {
      strDVDLabel = m_pDS->fv("cdlabel").get_asString();
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetDVDLabel() failed");
  }
}

CIMDBMovie CVideoDatabase::GetDetailsFromDataset(auto_ptr<Dataset> &pDS)
{
  CIMDBMovie details;
  details.m_fRating = (float)atof(pDS->fv("movieinfo.fRating").get_asString().c_str()) ;
  details.m_strDirector = pDS->fv("actors.strActor").get_asString();
  details.m_strWritingCredits = pDS->fv("movieinfo.strCredits").get_asString();
  details.m_strTagLine = pDS->fv("movieinfo.strTagLine").get_asString();
  details.m_strPlotOutline = pDS->fv("movieinfo.strPlotOutline").get_asString();
  details.m_strPlot = pDS->fv("movieinfo.strPlot").get_asString();
  details.m_strVotes = pDS->fv("movieinfo.strVotes").get_asString();
  details.m_strRuntime = pDS->fv("movieinfo.strRuntime").get_asString();
  details.m_strCast = pDS->fv("movieinfo.strCast").get_asString();
  details.m_iYear = pDS->fv("movieinfo.iYear").get_asLong();
  details.m_strGenre = pDS->fv("movieinfo.strGenre").get_asString();
  details.m_strPictureURL = pDS->fv("movieinfo.strPictureURL").get_asString();
  details.m_strTitle = pDS->fv("movieinfo.strTitle").get_asString();
  details.m_strPath = pDS->fv("path.strPath").get_asString();
  details.m_strDVDLabel = pDS->fv("movie.cdlabel").get_asString();
  details.m_strIMDBNumber = pDS->fv("movieinfo.IMDBID").get_asString();

  long lMovieId = pDS->fv("movieinfo.idMovie").get_asLong();
  details.m_strSearchString.Format("%i", lMovieId);
  return details;
}

/// \brief GetVideoSettings() obtains any saved video settings for the current file.
/// \retval Returns true if the settings exist, false otherwise.
bool CVideoDatabase::GetVideoSettings(const CStdString &strFilenameAndPath, CVideoSettings &settings)
{
  try
  {
    // obtain the FileID (if it exists)
    long lPathId, lMovieId;
    long lFileId = GetFile(strFilenameAndPath, lPathId, lMovieId, true);
    if (lFileId < 0) return false;
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    // ok, now obtain the settings for this file
    CStdString strSQL=FormatSQL("select * from settings where idFile='%i'\n", lFileId);
    m_pDS->query( strSQL.c_str() );
    if (m_pDS->num_rows() > 0)
    { // get the video settings info
      settings.m_AdjustFrameRate = m_pDS->fv("AdjustFrameRate").get_asBool();
      settings.m_AudioDelay = m_pDS->fv("AudioDelay").get_asFloat();
      settings.m_AudioStream = m_pDS->fv("AudioStream").get_asInteger();
      settings.m_Brightness = m_pDS->fv("Brightness").get_asInteger();
      settings.m_Contrast = m_pDS->fv("Contrast").get_asInteger();
      settings.m_CustomPixelRatio = m_pDS->fv("PixelRatio").get_asFloat();
      settings.m_CustomZoomAmount = m_pDS->fv("ZoomAmount").get_asFloat();
      settings.m_Gamma = m_pDS->fv("Gamma").get_asInteger();
      settings.m_NonInterleaved = m_pDS->fv("Interleaved").get_asBool();
      settings.m_NoCache = m_pDS->fv("NoCache").get_asBool();
      settings.m_SubtitleDelay = m_pDS->fv("SubtitleDelay").get_asFloat();
      settings.m_SubtitleOn = m_pDS->fv("SubtitlesOn").get_asBool();
      settings.m_SubtitleStream = m_pDS->fv("SubtitleStream").get_asInteger();
      settings.m_ViewMode = m_pDS->fv("ViewMode").get_asInteger();
      settings.m_ResumeTime = m_pDS->fv("ResumeTime").get_asInteger();
      settings.m_Crop = m_pDS->fv("Crop").get_asBool();
      settings.m_CropLeft = m_pDS->fv("CropLeft").get_asInteger();
      settings.m_CropRight = m_pDS->fv("CropRight").get_asInteger();
      settings.m_CropTop = m_pDS->fv("CropTop").get_asInteger();
      settings.m_CropBottom = m_pDS->fv("CropBottom").get_asInteger();
      settings.m_InterlaceMethod = (EINTERLACEMETHOD)m_pDS->fv("Deinterlace").get_asInteger();
      m_pDS->close();
      return true;
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetVideoSettings() failed");
  }
  return false;
}

/// \brief Sets the settings for a particular video file
void CVideoDatabase::SetVideoSettings(const CStdString& strFilenameAndPath, const CVideoSettings &setting)
{
  try
  {
    long lPathId, lMovieId;
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;
    long lFileId = GetFile(strFilenameAndPath, lPathId, lMovieId, true);
    if (lFileId < 0)
    { // no files found - we have to add one
      lMovieId = AddMovie(strFilenameAndPath, "", false);
      lFileId = GetFile(strFilenameAndPath, lPathId, lMovieId, true);
      if (lFileId < 0) return ;
    }
    CStdString strSQL;
    strSQL.Format("select * from settings where idFile=%i", lFileId);
    m_pDS->query( strSQL.c_str() );
    if (m_pDS->num_rows() > 0)
    {
      m_pDS->close();
      // update the item
      strSQL=FormatSQL("update settings set Interleaved=%i,NoCache=%i,Deinterlace=%i,FilmGrain=%i,ViewMode=%i,ZoomAmount=%f,PixelRatio=%f,"
                       "AudioStream=%i,SubtitleStream=%i,SubtitleDelay=%f,SubtitlesOn=%i,Brightness=%i,Contrast=%i,Gamma=%i,"
                       "AdjustFrameRate=%i,AudioDelay=%f,ResumeTime=%i,",
                       setting.m_NonInterleaved, setting.m_NoCache, setting.m_InterlaceMethod, setting.m_FilmGrain, setting.m_ViewMode, setting.m_CustomZoomAmount, setting.m_CustomPixelRatio,
                       setting.m_AudioStream, setting.m_SubtitleStream, setting.m_SubtitleDelay, setting.m_SubtitleOn,
                       setting.m_Brightness, setting.m_Contrast, setting.m_Gamma, setting.m_AdjustFrameRate, setting.m_AudioDelay,
                       setting.m_ResumeTime);
      CStdString strSQL2;
      strSQL2=FormatSQL("Crop=%i,CropLeft=%i,CropRight=%i,CropTop=%i,CropBottom=%i where idFile=%i\n", setting.m_Crop, setting.m_CropLeft, setting.m_CropRight, setting.m_CropTop, setting.m_CropBottom, lFileId);
      strSQL += strSQL2;
      m_pDS->exec(strSQL.c_str());
      return ;
    }
    else
    { // add the items
      m_pDS->close();
      strSQL=FormatSQL("insert into settings ( idFile,Interleaved,NoCache,Deinterlace,FilmGrain,ViewMode,ZoomAmount,PixelRatio,"
                       "AudioStream,SubtitleStream,SubtitleDelay,SubtitlesOn,Brightness,Contrast,Gamma,"
                       "AdjustFrameRate,AudioDelay,ResumeTime,Crop,CropLeft,CropRight,CropTop,CropBottom)"
                       " values (%i,%i,%i,%i,%i,%i,%f,%f,%i,%i,%f,%i,%i,%i,%i,%i,%f,",
                       lFileId, setting.m_NonInterleaved, setting.m_NoCache, setting.m_InterlaceMethod, setting.m_FilmGrain, setting.m_ViewMode, setting.m_CustomZoomAmount, setting.m_CustomPixelRatio,
                       setting.m_AudioStream, setting.m_SubtitleStream, setting.m_SubtitleDelay, setting.m_SubtitleOn,
                       setting.m_Brightness, setting.m_Contrast, setting.m_Gamma, setting.m_AdjustFrameRate, setting.m_AudioDelay);
      CStdString strSQL2;
      strSQL2=FormatSQL("%i,%i,%i,%i,%i,%i)\n", setting.m_ResumeTime, setting.m_Crop, setting.m_CropLeft, setting.m_CropRight,
                    setting.m_CropTop, setting.m_CropBottom);
      strSQL += strSQL2;
      m_pDS->exec(strSQL.c_str());
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::AddBookMarkToMovie(%s) failed", strFilenameAndPath.c_str());
  }
}

bool CVideoDatabase::UpdateOldVersion(float fVersion)
{
  if (fVersion < 0.5f)
  { // Version 0 to 0.5 upgrade - we need to add the version table and the settings table
    CLog::Log(LOGINFO, "creating versions table");
    m_pDS->exec("CREATE TABLE version (idVersion float)\n");

    CLog::Log(LOGINFO, "create settings table");
    m_pDS->exec("CREATE TABLE settings ( idFile integer, Interleaved bool, NoCache bool, Deinterlace bool, FilmGrain integer,"
                "ViewMode integer,ZoomAmount float, PixelRatio float, AudioStream integer, SubtitleStream integer,"
                "SubtitleDelay float, SubtitlesOn bool, Brightness integer, Contrast integer, Gamma integer,"
                "AdjustFrameRate integer, AudioDelay float)\n");
  }
  // REMOVED: v 1.0 -> 1.2 updates - they just delete the settings table in order
  // to add new settings - mayaswell just delete all the time.
  if (fVersion < 1.3f)
  { // v 1.0 -> 1.3 (new crop settings to video settings table)
    // Just delete and recreate the settings table is the easiest thing to do
    // all it means is per-video settings need recreating on playback - not a big deal
    // and it means the code can be kept reasonably simple.
    CLog::Log(LOGINFO, "Deleting old settings table");
    m_pDS->exec("DROP TABLE settings");
    CLog::Log(LOGINFO, "Creating new settings table");
    m_pDS->exec("CREATE TABLE settings ( idFile integer, Interleaved bool, NoCache bool, Deinterlace bool, FilmGrain integer,"
                "ViewMode integer,ZoomAmount float, PixelRatio float, AudioStream integer, SubtitleStream integer,"
                "SubtitleDelay float, SubtitlesOn bool, Brightness integer, Contrast integer, Gamma integer,"
                "AdjustFrameRate integer, AudioDelay float, ResumeTime integer, Crop bool, CropLeft integer,"
                "CropRight integer, CropTop integer, CropBottom integer)\n");
  }
  if (fVersion < 1.4f)
  { // v 1.3 -> 1.4 (new layout for bookmarks table)
    // Just delete the old bookmarks table and create it fresh
    CLog::Log(LOGINFO, "Deleting old bookmarks table");
    m_pDS->exec("DROP TABLE bookmark");
    CLog::Log(LOGINFO, "Creating new bookmarks table");
    m_pDS->exec("CREATE TABLE bookmark ( idBookmark integer primary key, idFile integer, timeInSeconds integer, thumbNailImage text)\n");
  }
  return true;
}
