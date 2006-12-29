//********************************************************************************************************************************
//**
//** see docs/videodatabase.png for a diagram of the database
//**
//********************************************************************************************************************************

#include "stdafx.h"
#include "videodatabase.h"
#include "GUIWindowVideoBase.h"
#include "utils/fstrcmp.h"
#include "util.h"
#include "GUIPassword.h"
#include "filesystem/VirtualPathDirectory.h"
#include "filesystem/StackDirectory.h"

#define VIDEO_DATABASE_VERSION 3.f
#define VIDEO_DATABASE_NAME "MyVideos32.db"

CBookmark::CBookmark()
{
  timeInSeconds = 0.0f;
  type = STANDARD;
}

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
    m_pDS->exec("CREATE TABLE bookmark ( idBookmark integer primary key, idFile integer, timeInSeconds double, thumbNailImage text, player text, playerState text, type integer)\n");

    CLog::Log(LOGINFO, "create settings table");
    m_pDS->exec("CREATE TABLE settings ( idFile integer, Interleaved bool, NoCache bool, Deinterlace bool, FilmGrain integer,"
                "ViewMode integer,ZoomAmount float, PixelRatio float, AudioStream integer, SubtitleStream integer,"
                "SubtitleDelay float, SubtitlesOn bool, Brightness integer, Contrast integer, Gamma integer,"
                "VolumeAmplification float, AudioDelay float, OutputToAllSpeakers bool, ResumeTime integer, Crop bool, CropLeft integer,"
                "CropRight integer, CropTop integer, CropBottom integer)\n");

    CLog::Log(LOGINFO, "create stacktimes table");
    m_pDS->exec("CREATE TABLE stacktimes (idFile integer, times text)\n");

    CLog::Log(LOGINFO, "create genre table");
    m_pDS->exec("CREATE TABLE genre ( idGenre integer primary key, strGenre text)\n");

    CLog::Log(LOGINFO, "create genrelinkmovie table");
    m_pDS->exec("CREATE TABLE genrelinkmovie ( idGenre integer, idMovie integer)\n");

    CLog::Log(LOGINFO, "create movie table");
    m_pDS->exec("CREATE TABLE movie ( idMovie integer, idType integer, strValue text)\n");

    CLog::Log(LOGINFO, "create actorlinkmovie table");
    m_pDS->exec("CREATE TABLE actorlinkmovie ( idActor integer, idMovie integer )\n");

    CLog::Log(LOGINFO, "create actors table");
    m_pDS->exec("CREATE TABLE actors ( idActor integer primary key, strActor text )\n");

    CLog::Log(LOGINFO, "create movielinkfile table");
    m_pDS->exec("create table movielinkfile (idMovie integer primary key, idFile integer)\n");  

    CLog::Log(LOGINFO, "create path table");
    m_pDS->exec("CREATE TABLE path ( idPath integer primary key, strPath text, idContent integer, strScraper text)\n");

    CLog::Log(LOGINFO, "create files table");
    m_pDS->exec("CREATE TABLE files ( idFile integer primary key, idPath integer, strFilename text, idMovie integer, idEpisode integer)\n");

    CLog::Log(LOGINFO, "create tvshow table");
    m_pDS->exec("CREATE TABLE tvshows ( idShow integer primary key, strPlot text, strDirector text, iYear integer, strGenre text, strCast text, strPictureURL text, strId text)\n");
  
    CLog::Log(LOGINFO, "episode tvshow table");
    m_pDS->exec("CREATE TABLE episodes ( idEpisode integer primary key, iEpisode integer, iSeason integer, strTitle text, strPlot text, strDirector text, strWriter text, iYear integer, strCast text, strId text, bWatched bool)\n");
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "videodatabase::unable to create tables:%i", GetLastError());
    return false;
  }

  return true;
}

//********************************************************************************************************************************
long CVideoDatabase::GetPath(const CStdString& strPath)
{
  CStdString strSQL;
  try
  {
    long lPathId=-1;
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    strSQL=FormatSQL("select * from path where strPath like '%s'",strPath.c_str());
    m_pDS->query(strSQL.c_str());
    if (!m_pDS->eof())
      lPathId = m_pDS->fv("path.idPath").get_asLong();

    return lPathId;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "videodatabase:unable to addpath (%s)", strSQL.c_str());
  }
  return -1;
}

long CVideoDatabase::AddPath(const CStdString& strPath)
{
  CStdString strSQL;
  try
  {
    long lPathId;
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    strSQL=FormatSQL("insert into path (idPath, strPath, idContent, strScraper) values (NULL,'%s',-1,'')", strPath.c_str());
    m_pDS->exec(strSQL.c_str());
    lPathId = (long)sqlite3_last_insert_rowid( m_pDB->getHandle() );
    return lPathId;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "videodatabase:unable to addpath (%s)", strSQL.c_str());
  }
  return -1;
}


//********************************************************************************************************************************
long CVideoDatabase::AddFile(const CStdString& strFileNameAndPath)
{
  CStdString strSQL = "";
  try
  {
    long lFileId;
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    CStdString strFileName, strPath;
    CUtil::Split(strFileNameAndPath,strPath, strFileName);
    strSQL=FormatSQL("select * from path where strPath like '%s'",strPath.c_str());
    m_pDS->query(strSQL.c_str());
    long lPathId=-1;
    if (m_pDS->num_rows() > 0)
      lPathId = m_pDS->fv("path.idPath").get_asLong();
    else
      lPathId = AddPath(strPath);

    if (lPathId < 0)
      return -1;

    strSQL=FormatSQL("select * from files where strFileName like '%s' and idPath=%u", strFileName.c_str(),lPathId);

    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() > 0)
    {
      lFileId = m_pDS->fv("idFile").get_asLong() ;
      m_pDS->close();
      return lFileId;
    }
    m_pDS->close();
    strSQL=FormatSQL("insert into files (idFile,idPath,strFileName,idMovie,idEpisode) values(NULL, %u, '%s',-1,-1)", lPathId,strFileName.c_str());
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
long CVideoDatabase::GetFile(const CStdString& strFilenameAndPath, long& lMovieId, long& lEpisodeId, bool bExact)
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;
    CStdString strPath, strFileName ;
    CUtil::Split(strFilenameAndPath, strPath, strFileName);
    long lPathId = GetPath(strPath);
    if (lPathId < 0)
      return -1;

    CStdString strSQL;
    strSQL=FormatSQL("select * from files where strFileName like '%s' and idPath=%u", strFileName.c_str(),lPathId);
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() > 0)
    {
      long lFileId = m_pDS->fv("files.idFile").get_asLong();
      lMovieId = m_pDS->fv("files.idMovie").get_asLong();
      lEpisodeId = m_pDS->fv("files.idEpisode").get_asLong();
      m_pDS->close();
      return lFileId;
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetFile(%s) failed", strFilenameAndPath.c_str());
  }
  return -1;
}

//********************************************************************************************************************************

long CVideoDatabase::GetMovie(const CStdString& strFilenameAndPath)
{
  long lMovieId, lEpisodeId;
  if (GetFile(strFilenameAndPath, lMovieId, lEpisodeId) < 0)
  {
    return -1;
  }
  return lMovieId;
}

long CVideoDatabase::GetMovieInfo(const CStdString& strFilenameAndPath)
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;
    long lMovieId = -1;

    // needed for query parameters
    CStdString strPath, strFile;
    CUtil::Split(strFilenameAndPath, strPath, strFile);

    // have to join movieinfo table for correct results
    long lPathId = GetPath(strPath);
    if (lPathId < 0)
      return -1;

    CStdString strSQL=FormatSQL("select * from files where strFileName like '%s' and idPath=%i", strFile.c_str(),lPathId);
    CLog::Log(LOGDEBUG,"CVideoDatabase::GetMovieInfo(%s), query = %s", strFilenameAndPath.c_str(), strSQL.c_str());
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() > 0)
      lMovieId = m_pDS->fv("files.idMovie").get_asLong();  
    m_pDS->close();

    return lMovieId;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetMovieInfo(%s) failed", strFilenameAndPath.c_str());
  }
  return -1;
}

int CVideoDatabase::GetRecentMovies(long* pMovieIdArray, int nSize)
{
  int count = 0;

  try
  {
    if (NULL == m_pDB.get())
      return -1;

    if (NULL == m_pDS.get())
      return -1;

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
long CVideoDatabase::AddMovie(const CStdString& strFilenameAndPath)
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;
    
    long lFileId, lMovieId=-1,lEpisodeId=-1;
    lFileId = GetFile(strFilenameAndPath,lMovieId,lEpisodeId);
    if (lFileId < 0)
      lFileId = AddFile(strFilenameAndPath);
    if (lMovieId < 0)
    {
      CStdString strSQL=FormatSQL("insert into movie (idMovie,idType,strValue) values (0,%u,'')",VIDEODB_ID_TITLE);
      m_pDS->exec(strSQL.c_str());
      lMovieId = (long)sqlite3_last_insert_rowid(m_pDB->getHandle());
      strSQL=FormatSQL("update movie set idMovie=%i where idMovie=0 and idType=%i",lMovieId,VIDEODB_ID_TITLE);
      m_pDS->exec(strSQL.c_str());
      
      BeginTransaction();
      strSQL=FormatSQL("insert into movie (idMovie,idType,strValue) values (%u,%u,'')",lMovieId,VIDEODB_ID_DIRECTOR);
      m_pDS->exec(strSQL.c_str());
      strSQL=FormatSQL("insert into movie (idMovie,idType,strValue) values (%u,%u,'')",lMovieId,VIDEODB_ID_PLOTOUTLINE);
      m_pDS->exec(strSQL.c_str());
      strSQL=FormatSQL("insert into movie (idMovie,idType,strValue) values (%u,%u,'')",lMovieId,VIDEODB_ID_PLOT);
      m_pDS->exec(strSQL.c_str());
      strSQL=FormatSQL("insert into movie (idMovie,idType,strValue) values (%u,%u,'')",lMovieId,VIDEODB_ID_TAGLINE);
      m_pDS->exec(strSQL.c_str());
      strSQL=FormatSQL("insert into movie (idMovie,idType,strValue) values (%u,%u,'')",lMovieId,VIDEODB_ID_VOTES);
      m_pDS->exec(strSQL.c_str());
      strSQL=FormatSQL("insert into movie (idMovie,idType,strValue) values (%u,%u,'')",lMovieId,VIDEODB_ID_RATING);
      m_pDS->exec(strSQL.c_str());
      strSQL=FormatSQL("insert into movie (idMovie,idType,strValue) values (%u,%u,'')",lMovieId,VIDEODB_ID_CREDITS);
      m_pDS->exec(strSQL.c_str());
      strSQL=FormatSQL("insert into movie (idMovie,idType,strValue) values (%u,%u,'')",lMovieId,VIDEODB_ID_YEAR);
      m_pDS->exec(strSQL.c_str());
      strSQL=FormatSQL("insert into movie (idMovie,idType,strValue) values (%u,%u,'')",lMovieId,VIDEODB_ID_GENRE);
      m_pDS->exec(strSQL.c_str());
      strSQL=FormatSQL("insert into movie (idMovie,idType,strValue) values (%u,%u,'')",lMovieId,VIDEODB_ID_THUMBURL);
      m_pDS->exec(strSQL.c_str());
      strSQL=FormatSQL("insert into movie (idMovie,idType,strValue) values (%u,%u,'')",lMovieId,VIDEODB_ID_IDENT);
      m_pDS->exec(strSQL.c_str());
      strSQL=FormatSQL("insert into movie (idMovie,idType,strValue) values (%u,%u,'')",lMovieId,VIDEODB_ID_WATCHED);
      m_pDS->exec(strSQL.c_str());
      strSQL=FormatSQL("insert into movie (idMovie,idType,strValue) values (%u,%u,'')",lMovieId,VIDEODB_ID_RUNTIME);
      m_pDS->exec(strSQL.c_str());
      strSQL=FormatSQL("insert into movie (idMovie,idType,strValue) values (%u,%u,'')",lMovieId,VIDEODB_ID_MPAA);
      m_pDS->exec(strSQL.c_str());
      strSQL=FormatSQL("insert into movie (idMovie,idType,strValue) values (%u,%u,'')",lMovieId,VIDEODB_ID_TOP250);
      m_pDS->exec(strSQL.c_str());

      // update file info to reflect it points to this movie
      CStdString strPath, strFileName;
      CUtil::Split(strFilenameAndPath,strPath,strFileName);
      long lPathId = GetPath(strPath);
      if (lPathId < 0)
        lPathId = AddPath(strPath);
      strSQL=FormatSQL("update files set idMovie=%i,idEpisode=-1 where strFilename like '%s' and idPath=%u",lMovieId,strFileName.c_str(),lPathId);
      m_pDS->exec(strSQL.c_str());
      CommitTransaction();
    }
    
    return lMovieId;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::AddMovie(%s) failed", strFilenameAndPath.c_str());
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
bool CVideoDatabase::HasMovieInfo(const CStdString& strFilenameAndPath)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    long lMovieId = GetMovieInfo(strFilenameAndPath);
    if ( lMovieId < 0) return false;

    CStdString strSQL=FormatSQL("select * from movie where movie.idmovie=%i", lMovieId);
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

    strSQL=FormatSQL("delete from movie where idmovie=%i", lMovieId);
    m_pDS->exec(strSQL.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::DeleteMovieInfo(%s) failed", strFileNameAndPath.c_str());
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

    CStdString strSQL=FormatSQL("select * from actorlinkmovie,actors,movie where actors.idActor=actorlinkmovie.idActor and actorlinkmovie.idmovie=movie.idmovie and actors.stractor='%s' ", strActor.c_str());
    m_pDS->query( strSQL.c_str() );

    long lLastPathId = -1;
    while (!m_pDS->eof())
    {
      // if the current path is the same as the last path tested and unlocked,
      // just add the movie without retesting
      //long lPathId = m_pDS->fv("path.idPath").get_asLong(); // TODO
      long lMovieId = m_pDS->fv("movie.idMovie").get_asLong();
      movies.push_back(GetDetailsForMovie(lMovieId));
      /*      if (lPathId == lLastPathId)
        movies.push_back(GetDetailsForMovie());
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
      }*/
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
      lMovieId = GetMovieInfo(strFilenameAndPath);
    if (lMovieId < 0) return ;

    details = GetDetailsForMovie(lMovieId);
    if (details.m_strWritingCredits.IsEmpty())
    { // try loading off disk
      CIMDB imdb;
      imdb.LoadDetails(details.m_strIMDBNumber, details);
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
    if (lMovieId < 0)
      lMovieId = AddMovie(strFilenameAndPath);

    BeginTransaction();
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
    
    CStdString strValue; strValue.Format("%i",lDirector);
    CStdString strSQL=FormatSQL("update movie set strValue='%s' where idMovie=%i and idType=%i",strValue.c_str(),lMovieId,VIDEODB_ID_DIRECTOR);
    m_pDS->exec(strSQL.c_str());
    strSQL=FormatSQL("update movie set strValue='%s' where idMovie=%i and idType=%i",details.m_strRuntime.c_str(),lMovieId, VIDEODB_ID_RUNTIME);
    m_pDS->exec(strSQL.c_str());
    strValue.Format("%3.3f",details.m_fRating);
    strSQL=FormatSQL("update movie set strValue='%s' where idMovie=%i and idType=%i",strValue.c_str(),lMovieId,VIDEODB_ID_RATING);
    m_pDS->exec(strSQL.c_str());
    strValue.Format("%i",details.m_iYear);
    strSQL=FormatSQL("update movie set strValue='%s' where idMovie=%i and idType=%i",strValue.c_str(),lMovieId,VIDEODB_ID_YEAR);
    m_pDS->exec(strSQL.c_str());
    strSQL=FormatSQL("update movie set strValue='%s' where idMovie=%i and idType=%i",details.m_strTitle.c_str(),lMovieId,VIDEODB_ID_TITLE);
    m_pDS->exec(strSQL.c_str());
    strSQL=FormatSQL("update movie set strValue='%s' where idMovie=%i and idType=%i",details.m_strGenre.c_str(),lMovieId,VIDEODB_ID_GENRE);
    m_pDS->exec(strSQL.c_str());
    strSQL=FormatSQL("update movie set strValue='%s' where idMovie=%i and idType=%i",details.m_strIMDBNumber.c_str(),lMovieId,VIDEODB_ID_IDENT);
    m_pDS->exec(strSQL.c_str());
    strSQL=FormatSQL("update movie set strValue='%s' where idMovie=%i and idType=%i","false",lMovieId,VIDEODB_ID_WATCHED);
    m_pDS->exec(strSQL.c_str());
    strSQL=FormatSQL("update movie set strValue='%s' where idMovie=%i and idType=%i",details.m_strWritingCredits.c_str(),lMovieId,VIDEODB_ID_CREDITS);
    m_pDS->exec(strSQL.c_str());
    strSQL=FormatSQL("update movie set strValue='%s' where idMovie=%i and idType=%i",details.m_strPictureURL.c_str(),lMovieId,VIDEODB_ID_THUMBURL);
    m_pDS->exec(strSQL.c_str());
    strSQL=FormatSQL("update movie set strValue='%s' where idMovie=%i and idType=%i",details.m_strPlot.c_str(),lMovieId,VIDEODB_ID_PLOT);
    m_pDS->exec(strSQL.c_str());
    strSQL=FormatSQL("update movie set strValue='%s' where idMovie=%i and idType=%i",details.m_strPlotOutline.c_str(),lMovieId,VIDEODB_ID_PLOTOUTLINE);
    m_pDS->exec(strSQL.c_str());
    strSQL=FormatSQL("update movie set strValue='%s' where idMovie=%i and idType=%i",details.m_strTagLine.c_str(),lMovieId,VIDEODB_ID_TAGLINE);
    m_pDS->exec(strSQL.c_str());
    strSQL=FormatSQL("update movie set strValue='%s' where idMovie=%i and idType=%i",details.m_strVotes.c_str(),lMovieId,VIDEODB_ID_VOTES);
    m_pDS->exec(strSQL.c_str());
    strSQL=FormatSQL("update movie set strValue='%s' where idMovie=%i and idType=%i",details.m_strMPAARating.c_str(),lMovieId,VIDEODB_ID_MPAA);
    m_pDS->exec(strSQL.c_str());
    strValue.Format("%i",details.m_iTop250);
    strSQL=FormatSQL("update movie set strValue='%s' where idMovie=%i and idType=%i",strValue.c_str(),lMovieId,VIDEODB_ID_TOP250);
    m_pDS->exec(strSQL.c_str());

    CommitTransaction();
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
    if (strPath1.size() == 0) return;
    movies.erase(movies.begin(), movies.end());

    vector<CStdString> vecPaths;
    CStdString strPath = strPath1;
    if (strPath.Left(14).Equals("virtualpath://"))
    {
      CVirtualPathDirectory vpath;
      if (!vpath.GetPathes(strPath, vecPaths))
        return;
    }
    else
      vecPaths.push_back(strPath);

    // query for each patg in the vector!
    for (int i = 0; i < (int)vecPaths.size(); ++i)
    {
      strPath = vecPaths[i];
      if (CUtil::HasSlashAtEnd(strPath)) strPath = strPath.Left(strPath.size() - 1);
      CStdString strStackPath = "stack://" + strPath;

      if (NULL == m_pDB.get()) return ;
      if (NULL == m_pDS.get()) return ;
      CStdString strSQL=FormatSQL("select * from path join files on path.idPath = files.idPath join movieinfo on files.idMovie = movieinfo.idMovie where path.strPath like '%%%s' or path.strPath like '%%%s'", strPath.c_str(), strStackPath.c_str());
      CLog::Log(LOGDEBUG,"CVideoDatabase::GetMoviesByPath query = %s", strSQL.c_str());

      m_pDS->query( strSQL.c_str() );
      while (!m_pDS->eof())
      {
        CIMDBMovie details;
        long lMovieId = m_pDS->fv("files.idMovie").get_asLong();
        details.m_strSearchString.Format("%i", lMovieId);
        details.m_strIMDBNumber = m_pDS->fv("movieinfo.IMDBID").get_asString();
        details.m_strFile = m_pDS->fv("files.strFilename").get_asString();
        details.m_strPath = m_pDS->fv("path.strPath").get_asString();
        //CLog::Log(LOGDEBUG,"  movie [%s%s]", details.m_strPath.c_str(), details.m_strFile.c_str());
        movies.push_back(details);
        m_pDS->next();
      }
      m_pDS->close();
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetMoviesByPath(%s) failed", strPath1.c_str());
  }
}

//********************************************************************************************************************************
void CVideoDatabase::GetFilePath(long lMovieId, CStdString &filePath)
{
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    if (lMovieId < 0) return ;

    CStdString strSQL=FormatSQL("select * from path, files where path.idPath=files.idPath and files.idmovie=%i order by strFilename", lMovieId );
    m_pDS->query( strSQL.c_str() );
    if (!m_pDS->eof())
    {
      filePath = m_pDS->fv("path.strPath").get_asString();
      filePath += m_pDS->fv("files.strFilename").get_asString();
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetFilePath() failed");
  }
}

//********************************************************************************************************************************
void CVideoDatabase::GetBookMarksForFile(const CStdString& strFilenameAndPath, VECBOOKMARKS& bookmarks, CBookmark::EType type /*= CBookmark::STANDARD*/)
{
  try
  {
    long lMovieId, lEpisodeId;
    long lFileId = GetFile(strFilenameAndPath, lMovieId, lEpisodeId, true);
    if (lFileId < 0) return ;
    bookmarks.erase(bookmarks.begin(), bookmarks.end());
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    CStdString strSQL=FormatSQL("select * from bookmark where idFile=%i and type=%i order by timeInSeconds", lFileId, (int)type);
    m_pDS->query( strSQL.c_str() );
    while (!m_pDS->eof())
    {
      CBookmark bookmark;
      bookmark.timeInSeconds = m_pDS->fv("timeInSeconds").get_asDouble();
      bookmark.thumbNailImage = m_pDS->fv("thumbNailImage").get_asString();
      bookmark.playerState = m_pDS->fv("playerState").get_asString();
      bookmark.player = m_pDS->fv("player").get_asString();
      bookmark.type = type;
      bookmarks.push_back(bookmark);
      m_pDS->next();
    }
    //sort(bookmarks.begin(), bookmarks.end(), SortBookmarks);
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetBookMarksForMovie(%s) failed", strFilenameAndPath.c_str());
  }
}

bool CVideoDatabase::GetResumeBookMark(const CStdString& strFilenameAndPath, CBookmark &bookmark)
{
  VECBOOKMARKS bookmarks;
  GetBookMarksForFile(strFilenameAndPath, bookmarks, CBookmark::RESUME);
  if(bookmarks.size()>0)
  {
    bookmark = bookmarks[0];
    return true;
  }
  return false;
}

//********************************************************************************************************************************
void CVideoDatabase::AddBookMarkToFile(const CStdString& strFilenameAndPath, const CBookmark &bookmark, CBookmark::EType type /*= CBookmark::STANDARD*/)
{
  try
  {
    long lMovieId, lEpisodeId;
    long lFileId = GetFile(strFilenameAndPath, lMovieId, lEpisodeId, true);
    if (lFileId < 0)
    {
      // Doesn't exist in the database yet - add it.
      // TODO: It doesn't appear to me that the CDLabel parameter or the subtitles
      // parameter is anywhere in use in XBMC.
      lFileId = AddFile(strFilenameAndPath);
      if (lFileId < 0)
        return ;
    }
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    CStdString strSQL;
    int idBookmark=-1;
    if( type == CBookmark::RESUME ) // get the same resume mark bookmark each time type
    {
      strSQL=FormatSQL("select idBookmark from bookmark where idFile=%i and type=1", lFileId);   
    }
    else // get the same bookmark again, and update. not sure here as a dvd can have same time in multiple places, state will differ thou
    {
      /* get a bookmark within the same time as previous */
      double mintime = bookmark.timeInSeconds - 0.5f;
      double maxtime = bookmark.timeInSeconds + 0.5f;
      strSQL=FormatSQL("select idBookmark from bookmark where idFile=%i and type=%i and (timeInSeconds between %f and %f) and playerState='%s'", lFileId, (int)type, mintime, maxtime, bookmark.playerState.c_str());
    }

    // get current id
    m_pDS->query( strSQL.c_str() );
    if (m_pDS->num_rows() != 0)
      idBookmark = m_pDS->get_field_value("idBookmark").get_asInteger();            
    m_pDS->close();

    // update or insert depending if it existed before
    if( idBookmark >= 0 )
      strSQL=FormatSQL("update bookmark set timeInSeconds = %f, thumbNailImage = '%s', player = '%s', playerState = '%s' where idBookmark = %i", bookmark.timeInSeconds, bookmark.thumbNailImage.c_str(), bookmark.player.c_str(), bookmark.playerState.c_str(), idBookmark);
    else
      strSQL=FormatSQL("insert into bookmark (idBookmark, idFile, timeInSeconds, thumbNailImage, player, playerState, type) values(NULL,%i,%f,'%s','%s','%s', %i)", lFileId, bookmark.timeInSeconds, bookmark.thumbNailImage.c_str(), bookmark.player.c_str(), bookmark.playerState.c_str(), (int)type);

    m_pDS->exec(strSQL.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::AddBookMarkToMovie(%s) failed", strFilenameAndPath.c_str());
  }
}

void CVideoDatabase::ClearBookMarkOfFile(const CStdString& strFilenameAndPath, CBookmark& bookmark, CBookmark::EType type /*= CBookmark::STANDARD*/)
{
  try
  {
    long lMovieId, lEpisodeId;
    long lFileId = GetFile(strFilenameAndPath, lMovieId, lEpisodeId, true);
    if (lFileId < 0) return ;
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    /* a litle bit uggly, we clear first bookmark that is within one second of given */
    /* should be no problem since we never add bookmarks that are closer than that   */
    double mintime = bookmark.timeInSeconds - 0.5f;
    double maxtime = bookmark.timeInSeconds + 0.5f;
    CStdString strSQL=FormatSQL("select idBookmark from bookmark where idFile=%i and type=%i and (timeInSeconds between %f and %f)", lFileId, type, mintime, maxtime);
    m_pDS->query( strSQL.c_str() );
    if (m_pDS->num_rows() != 0)
    {
      int idBookmark = m_pDS->get_field_value("idBookmark").get_asInteger();
      strSQL=FormatSQL("delete from bookmark where idBookmark=%i",idBookmark);
      m_pDS->exec(strSQL.c_str());
    }

    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::ClearBookMarkOfFile(%s) failed", strFilenameAndPath.c_str());
  }
}

//********************************************************************************************************************************
void CVideoDatabase::ClearBookMarksOfFile(const CStdString& strFilenameAndPath, CBookmark::EType type /*= CBookmark::STANDARD*/)
{
  try
  {
    long lMovieId, lEpisodeId;
    long lFileId = GetFile(strFilenameAndPath, lMovieId, lEpisodeId, true);
    if (lFileId < 0) return ;
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    CStdString strSQL=FormatSQL("delete from bookmark where idFile=%i and type=%i", lFileId, (int)type);
    m_pDS->exec(strSQL.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::ClearBookMarksOfMovie(%s) failed", strFilenameAndPath.c_str());
  }
}

//********************************************************************************************************************************
void CVideoDatabase::DeleteMovie(const CStdString& strFilenameAndPath)
{
  try
  {
    long lMovieId, lEpisodeId;
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;
    if (GetFile(strFilenameAndPath, lMovieId, lEpisodeId) < 0)
    {
      return ;
    }

    ClearBookMarksOfFile(strFilenameAndPath);

    CStdString strSQL;
    strSQL=FormatSQL("delete from files where idmovie=%i", lMovieId);
    m_pDS->exec(strSQL.c_str());

    strSQL=FormatSQL("delete from genrelinkmovie where idmovie=%i", lMovieId);
    m_pDS->exec(strSQL.c_str());

    strSQL=FormatSQL("delete from actorlinkmovie where idmovie=%i", lMovieId);
    m_pDS->exec(strSQL.c_str());

    strSQL=FormatSQL("delete from movie where idmovie=%i", lMovieId);
    m_pDS->exec(strSQL.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::DeleteMovie() failed");
  }
}

CIMDBMovie CVideoDatabase::GetDetailsForMovie(long lMovieId)
{
  CIMDBMovie details;
  auto_ptr<Dataset> pDS;
  pDS.reset(m_pDB->CreateDataset());

  CStdString strSQL=FormatSQL("select * from movie where idMovie=%u",lMovieId);
  m_pDS2->query(strSQL.c_str());
  if (m_pDS2->eof()) 
    return details;

  while (!m_pDS2->eof())
  {
    int iType = m_pDS2->fv("movie.idType").get_asInteger();
    // special case the director one
    if (iType == VIDEODB_ID_DIRECTOR)
    {
      long lDirectorId = m_pDS2->fv("movie.strValue").get_asLong();
      strSQL = FormatSQL("select * from actors where idActor=%u",lDirectorId);
      pDS->query(strSQL.c_str());
      if (!pDS->eof())
        details.m_strDirector = pDS->fv("actor.strActor").get_asString();
    }
    else
    {
      switch (DbMovieOffsets[iType].type)
      {
        case VIDEODB_TYPE_STRING:
          *(CStdString*)(((char*)&details)+DbMovieOffsets[iType].offset) = m_pDS2->fv("movie.strValue").get_asString();
          break;
        case VIDEODB_TYPE_INT:
          *(int*)(((char*)&details)+DbMovieOffsets[iType].offset) = m_pDS2->fv("movie.strValue").get_asInteger();
          break;
        case VIDEODB_TYPE_BOOL:
          *(bool*)(((char*)&details)+DbMovieOffsets[iType].offset) = m_pDS2->fv("movie.strValue").get_asBool();
          break;
        case VIDEODB_TYPE_FLOAT:
          *(float*)(((char*)&details)+DbMovieOffsets[iType].offset) = m_pDS2->fv("movie.strValue").get_asFloat();
          break;
      }
    }
    m_pDS2->next();
  }

  strSQL=FormatSQL("select * from files where idMovie=%u",lMovieId);
  pDS->query(strSQL.c_str());
  if (!pDS->eof())
  {
    CStdString strFileName = pDS->fv("files.strFileName").get_asString();
    strSQL=FormatSQL("select * from path where idPath=%u",pDS->fv("files.idPath").get_asLong());
    pDS->query(strSQL.c_str());
    if (!pDS->eof())
    {
      CUtil::AddFileToFolder(pDS->fv("path.strPath").get_asString(),strFileName,details.m_strFileNameAndPath);
      details.m_strPath = pDS->fv("path.strPath").get_asString();
    }
  }

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
#ifdef NEW_VIDEODB_METHODS
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    CStdString strPath, strFileName;
    CUtil::Split(strFilenameAndPath, strPath, strFileName);
    CStdString strSQL=FormatSQL("select * from settings, files, path where settings.idfile=files.idfile and path.idpath=files.idpath and path.strPath like '%s' and files.strFileName like '%s'", strPath.c_str() , strFileName.c_str());
#else
    long lPathId, lMovieId;
    long lFileId = GetFile(strFilenameAndPath, lPathId, lMovieId, true);
    if (lFileId < 0) return false;
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    // ok, now obtain the settings for this file
    CStdString strSQL=FormatSQL("select settings.* settings where settings.idFile = '%i'", lFileId);
#endif
    m_pDS->query( strSQL.c_str() );
    if (m_pDS->num_rows() > 0)
    { // get the video settings info
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
      settings.m_VolumeAmplification = m_pDS->fv("VolumeAmplification").get_asFloat();
      settings.m_OutputToAllSpeakers = m_pDS->fv("OutputToAllSpeakers").get_asBool();
      settings.m_SubtitleCached = false;
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
    long lMovieId, lEpisodeId;
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;
    long lFileId = GetFile(strFilenameAndPath, lMovieId, lEpisodeId, true);
    if (lFileId < 0)
    { // no files found - we have to add one
      lMovieId = AddMovie(strFilenameAndPath);
      lFileId = GetFile(strFilenameAndPath, lMovieId, lEpisodeId, true);
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
                       "VolumeAmplification=%f,AudioDelay=%f,OutputToAllSpeakers=%i,",
                       setting.m_NonInterleaved, setting.m_NoCache, setting.m_InterlaceMethod, setting.m_FilmGrain, setting.m_ViewMode, setting.m_CustomZoomAmount, setting.m_CustomPixelRatio,
                       setting.m_AudioStream, setting.m_SubtitleStream, setting.m_SubtitleDelay, setting.m_SubtitleOn,
                       setting.m_Brightness, setting.m_Contrast, setting.m_Gamma, setting.m_VolumeAmplification, setting.m_AudioDelay,
                       setting.m_OutputToAllSpeakers);
      CStdString strSQL2;
      strSQL2=FormatSQL("ResumeTime=%i,Crop=%i,CropLeft=%i,CropRight=%i,CropTop=%i,CropBottom=%i where idFile=%i\n", setting.m_ResumeTime, setting.m_Crop, setting.m_CropLeft, setting.m_CropRight, setting.m_CropTop, setting.m_CropBottom, lFileId);
      strSQL += strSQL2;
      m_pDS->exec(strSQL.c_str());
      return ;
    }
    else
    { // add the items
      m_pDS->close();
      strSQL=FormatSQL("insert into settings ( idFile,Interleaved,NoCache,Deinterlace,FilmGrain,ViewMode,ZoomAmount,PixelRatio,"
                       "AudioStream,SubtitleStream,SubtitleDelay,SubtitlesOn,Brightness,Contrast,Gamma,"
                       "VolumeAmplification,AudioDelay,OutputToAllSpeakers,ResumeTime,Crop,CropLeft,CropRight,CropTop,CropBottom)"
                       " values (%i,%i,%i,%i,%i,%i,%f,%f,%i,%i,%f,%i,%i,%i,%i,%f,%f,",
                       lFileId, setting.m_NonInterleaved, setting.m_NoCache, setting.m_InterlaceMethod, setting.m_FilmGrain, setting.m_ViewMode, setting.m_CustomZoomAmount, setting.m_CustomPixelRatio,
                       setting.m_AudioStream, setting.m_SubtitleStream, setting.m_SubtitleDelay, setting.m_SubtitleOn,
                       setting.m_Brightness, setting.m_Contrast, setting.m_Gamma, setting.m_VolumeAmplification, setting.m_AudioDelay);
      CStdString strSQL2;
      strSQL2=FormatSQL("%i,%i,%i,%i,%i,%i,%i)\n", setting.m_OutputToAllSpeakers, setting.m_ResumeTime, setting.m_Crop, setting.m_CropLeft, setting.m_CropRight,
                    setting.m_CropTop, setting.m_CropBottom);
      strSQL += strSQL2;
      m_pDS->exec(strSQL.c_str());
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::SetVideoSettings(%s) failed", strFilenameAndPath.c_str());
  }
}

/// \brief GetStackTimes() obtains any saved video times for the stacked file
/// \retval Returns true if the stack times exist, false otherwise.
bool CVideoDatabase::GetStackTimes(const CStdString &filePath, vector<long> &times)
{
  try
  {
    // obtain the FileID (if it exists)
    long lMovieId, lEpisodeId;
    long lFileId = GetFile(filePath, lMovieId, lEpisodeId, true);
    if (lFileId < 0) return false;
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    // ok, now obtain the settings for this file
    CStdString strSQL=FormatSQL("select times from stacktimes where idFile='%i'\n", lFileId);
    m_pDS->query( strSQL.c_str() );
    if (m_pDS->num_rows() > 0)
    { // get the video settings info
      CStdStringArray timeString;
      long timeTotal = 0;
      StringUtils::SplitString(m_pDS->fv("times").get_asString(), ",", timeString);
      times.clear();
      for (unsigned int i = 0; i < timeString.size(); i++)
      {
        times.push_back(atoi(timeString[i].c_str()));
        timeTotal += atoi(timeString[i].c_str());
      }
      m_pDS->close();
      return (timeTotal > 0);
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetStackTimes() failed");
  }
  return false;
}

/// \brief Sets the stack times for a particular video file
void CVideoDatabase::SetStackTimes(const CStdString& filePath, vector<long> &times)
{
  try
  {
    long lMovieId, lEpisodeId;
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;
    long lFileId = GetFile(filePath, lMovieId, lEpisodeId, true);
    if (lFileId < 0)
    { // no files found - we have to add one
      lMovieId = AddMovie(filePath);
      lFileId = GetFile(filePath, lMovieId, lEpisodeId, true);
      if (lFileId < 0) return ;
    }

    // delete any existing items
    CStdString strSQL;
    strSQL.Format("delete from stacktimes where idFile=%i", lFileId);
    m_pDS->exec( strSQL.c_str() );

    // add the items
    CStdString timeString;
    timeString.Format("%i", times[0]);
    for (unsigned int i = 1; i < times.size(); i++)
    {
      CStdString time;
      time.Format(",%i", times[i]);
      timeString += time;
    }
    strSQL.Format("insert into stacktimes (idFile,usingConversions,times) values (%i,%i,'%s')\n", lFileId, false, timeString.c_str());
    m_pDS->exec(strSQL.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::SetStackTimes(%s) failed", filePath.c_str());
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
//*************
// 2005-10-08
// MercuryInc
  if (fVersion < 1.5f)
  {
    // v 1.4 -> 1.5 (new layout for Watched tag)
    // Add watched column to movieinfo
    try
    {
      CLog::Log(LOGINFO, "Adding column to movieinfo");
      m_pDS->exec("CREATE TABLE TMPmovieinfo ( idMovie integer, idDirector integer, strPlotOutline text, strPlot text, strTagLine text, strVotes text, strRuntime text, fRating text, strCast text,strCredits text, iYear integer, strGenre text, strPictureURL text, strTitle text, IMDBID text, bWatched bool)\n");
      m_pDS->exec("INSERT INTO TMPmovieinfo SELECT *, 'false' FROM movieinfo\n");
      m_pDS->exec("DROP TABLE movieinfo\n");
      m_pDS->exec("CREATE TABLE movieinfo ( idMovie integer, idDirector integer, strPlotOutline text, strPlot text, strTagLine text, strVotes text, strRuntime text, fRating text, strCast text,strCredits text, iYear integer, strGenre text, strPictureURL text, strTitle text, IMDBID text, bWatched bool)\n");
      m_pDS->exec("INSERT INTO movieinfo SELECT * FROM TMPmovieinfo\n");
      m_pDS->exec("DROP TABLE TMPmovieinfo\n");      
    }
    catch (...)
    {
      CLog::Log(LOGERROR, "Failed to add bWatched to movieinfo");
      return false;
    }
    
    //vacuum movieinfo so the db is readable again
    try
    {
      CLog::Log(LOGINFO, "Vacuuming movieinfo");
      m_pDS->exec("VACUUM movieinfo\n");      
    }
    catch (...)
    {
      CLog::Log(LOGERROR, "Failed to vacuum movieinfo");
      return false;
    }
  }
//*************
  if (fVersion < 1.6f)
  {
    // dropping of AdjustFrameRate setting, and addition of VolumeAmplification setting
    CLog::Log(LOGINFO, "Deleting old settings table");
    m_pDS->exec("DROP TABLE settings");
    CLog::Log(LOGINFO, "Creating new settings table");
    m_pDS->exec("CREATE TABLE settings ( idFile integer, Interleaved bool, NoCache bool, Deinterlace bool, FilmGrain integer,"
                "ViewMode integer,ZoomAmount float, PixelRatio float, AudioStream integer, SubtitleStream integer,"
                "SubtitleDelay float, SubtitlesOn bool, Brightness integer, Contrast integer, Gamma integer,"
                "VolumeAmplification float, AudioDelay float, ResumeTime integer, Crop bool, CropLeft integer,"
                "CropRight integer, CropTop integer, CropBottom integer)\n");
  }

//*************
// 2006-02-20
// dvdplayer state
  if (fVersion < 1.7f)
  {
    try
    {
      CLog::Log(LOGINFO, "Adding playerState and type to bookmark");
      m_pDS->exec("CREATE TABLE TMPbookmark ( idBookmark integer primary key, idFile integer, timeInSeconds integer, thumbNailImage text, playerState text, type integer)\n");
      m_pDS->exec("INSERT INTO TMPbookmark SELECT idBookmark, idFile, timeInSeconds, thumbNailImage, '', 0 FROM bookmark");
      m_pDS->exec("DROP TABLE bookmark");
      m_pDS->exec("CREATE TABLE bookmark ( idBookmark integer primary key, idFile integer, timeInSeconds integer, thumbNailImage text, playerState text, type integer)\n");
      m_pDS->exec("INSERT INTO bookmark SELECT * FROM TMPbookmark");
      m_pDS->exec("DROP TABLE TMPbookmark");
    }
    catch(...)
    {
      CLog::Log(LOGERROR, "Failed to add playerState and type to bookmark");
      return false;
    }
  }
  if (fVersion < 1.8f)
  {
    // Adding of OutputToAllSpeakers setting
    CLog::Log(LOGINFO, "Deleting old settings table");
    m_pDS->exec("DROP TABLE settings");
    CLog::Log(LOGINFO, "Creating new settings table");
    m_pDS->exec("CREATE TABLE settings ( idFile integer, Interleaved bool, NoCache bool, Deinterlace bool, FilmGrain integer,"
                "ViewMode integer,ZoomAmount float, PixelRatio float, AudioStream integer, SubtitleStream integer,"
                "SubtitleDelay float, SubtitlesOn bool, Brightness integer, Contrast integer, Gamma integer,"
                "VolumeAmplification float, AudioDelay float, OutputToAllSpeakers bool, ResumeTime integer, Crop bool, CropLeft integer,"
                "CropRight integer, CropTop integer, CropBottom integer)\n");
  }
  if (fVersion < 1.9f)
  { // Add the stacktimes table
    try
    {
    CLog::Log(LOGINFO, "Adding stacktimes table");
    m_pDS->exec("CREATE TABLE stacktimes (idFile integer, usingConversions bool, times text)\n");
    }
    catch (...)
    {
      CLog::Log(LOGERROR, "Error adding stacktimes table to the database");
    }
  }


//*************
// 2006-02-20
// dvdplayer state
  if (fVersion < 2.0f)
  {
    try
    {
      CLog::Log(LOGINFO, "Adding player and changing timeInSeconds to float in bookmark table");
      m_pDS->exec("CREATE TABLE TMPbookmark ( idBookmark integer primary key, idFile integer, timeInSeconds integer, thumbNailImage text, playerState text, type integer)\n");
      m_pDS->exec("INSERT INTO TMPbookmark SELECT idBookmark, idFile, timeInSeconds, thumbNailImage, playerState, type FROM bookmark");
      m_pDS->exec("DROP TABLE bookmark");
      m_pDS->exec("CREATE TABLE bookmark ( idBookmark integer primary key, idFile integer, timeInSeconds double, thumbNailImage text, player text, playerState text, type integer)\n");
      m_pDS->exec("INSERT INTO bookmark SELECT idBookmark, idFile, timeInSeconds, thumbNailImage, '', playerState, type FROM TMPbookmark");
      m_pDS->exec("DROP TABLE TMPbookmark");
    }
    catch(...)
    {
      CLog::Log(LOGERROR, "Failed to upgrade bookmarks");
      return false;
    }
    
  }


  return true;
}

void CVideoDatabase::MarkAsWatched(const CFileItem &item)
{
  // find the movie in the db
  long movieID = -1;
  if (item.m_musicInfoTag.GetURL().IsEmpty())
    movieID = GetMovieInfo(item.m_strPath);
  else
    movieID = atol(item.m_musicInfoTag.GetURL().c_str());
  if (movieID < 0)
    return;
  // and mark as watched
  MarkAsWatched(movieID);
}

void CVideoDatabase::MarkAsWatched(long lMovieId)
{
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;
    CLog::Log(LOGINFO, "Updating Movie:%i as Watched", lMovieId);
    CStdString strSQL;
    strSQL.Format("UPDATE movie set strValue='true' WHERE idMovie=%u and idType=%i", lMovieId, VIDEODB_ID_WATCHED);
    m_pDS->exec(strSQL.c_str());
  }
  catch (...)
  {
	  CLog::Log(LOGERROR, "CVideoDatabase::MarkAsWatched(long lMovieId) failed on MovieID:%i", lMovieId);
  }
}

void CVideoDatabase::MarkAsUnWatched(long lMovieId)
{
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;
    CLog::Log(LOGINFO, "Updating Movie:%i as UnWatched", lMovieId);
    CStdString strSQL;
    strSQL.Format("UPDATE movie set strValue='false' WHERE idMovie=%i and idType=%i", lMovieId, VIDEODB_ID_WATCHED );
    m_pDS->exec(strSQL.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::MarkAsUnWatched(long lMovieId) failed on MovieID:%i", lMovieId);
  }
}

void CVideoDatabase::UpdateMovieTitle(long lMovieId, const CStdString& strNewMovieTitle)
{
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;
    CLog::Log(LOGINFO, "Changing Movie:id:%i New Title:%s", lMovieId, strNewMovieTitle.c_str());
    CStdString strSQL;
    strSQL.Format("UPDATE movie SET strValue='%s' WHERE idMovie=%i and idType=%i", strNewMovieTitle.c_str(), lMovieId, VIDEODB_ID_TITLE );
    m_pDS->exec(strSQL.c_str());
  }
  catch (...)
  {
	  CLog::Log(LOGERROR, "CVideoDatabase::UpdateMovieTitle(long lMovieId, const CStdString& strNewMovieTitle) failed on MovieID:%i and Title:%s", lMovieId, strNewMovieTitle);
  }
}

/// \brief EraseVideoSettings() Erases the videoSettings table and reconstructs it
void CVideoDatabase::EraseVideoSettings()
{
  try
  {
    CLog::Log(LOGINFO, "Deleting settings information for all movies");
    m_pDS->exec("delete from settings");
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::EraseVideoSettings() failed");
  }
}

bool CVideoDatabase::GetGenresNav(const CStdString& strBaseDir, CFileItemList& items)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    // get primary genres for movies
    CStdString strSQL=FormatSQL("select * from genre,genrelinkmovie,movie where genre.idGenre=genrelinkMovie.idGenre and genrelinkMovie.idMovie = movie.idMovie and movie.idType=%i",VIDEODB_ID_WATCHED);

    if (g_stSettings.m_iMyVideoWatchMode == 1)
      strSQL += FormatSQL(" and NOT (movie.strValue='true')");

    if (g_stSettings.m_iMyVideoWatchMode == 2)
      strSQL += FormatSQL(" and movie.strValue='true'");

    // run query
    CLog::Log(LOGDEBUG, "CVideoDatabase::GetGenresNav() query: %s", strSQL.c_str());
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return true;
    }

    map<long, CStdString> mapGenres;
    map<long, CStdString>::iterator it;
    while (!m_pDS->eof())
    {
      long lGenreId = m_pDS->fv("genre.idgenre").get_asLong();
      CStdString strGenre = m_pDS->fv("genre.strgenre").get_asString();
      it = mapGenres.find(lGenreId);
      // was this genre already found?
      if (it == mapGenres.end())
        mapGenres.insert(pair<long, CStdString>(lGenreId, strGenre));

      m_pDS->next();
    }
    m_pDS->close();

    for (it=mapGenres.begin();it != mapGenres.end();++it)
    {
      CFileItem* pItem=new CFileItem(it->second);
      CStdString strDir;
      strDir.Format("%ld/", it->first);
      pItem->m_strPath=strBaseDir + strDir;
      pItem->m_bIsFolder=true;
      items.Add(pItem);
    }
    
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase:GetGenresNav() failed");
  }
  return false;
}

bool CVideoDatabase::GetActorsNav(const CStdString& strBaseDir, CFileItemList& items)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

		CStdString strSQL=FormatSQL("select * from actorlinkmovie,actors,movie where actors.idActor=actorlinkmovie.idActor and actorlinkmovie.idmovie=movie.idmovie and movie.idType=%i",VIDEODB_ID_WATCHED);
    if (g_stSettings.m_iMyVideoWatchMode == 1)
      strSQL += FormatSQL(" and NOT (movie.strValue='true')");

    if (g_stSettings.m_iMyVideoWatchMode == 2)
      strSQL += FormatSQL(" and movie.strValue='true'");

    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return true;
    }

    map<long, CStdString> mapActors;
    map<long, CStdString>::iterator it;

    long lLastPathId = -1;
    while (!m_pDS->eof())
    {
      long lActorId = m_pDS->fv("actors.idactor").get_asLong();
      CStdString strActor = m_pDS->fv("actors.strActor").get_asString();
      it = mapActors.find(lActorId);
      // is this actor already known?
      if (it == mapActors.end())
        mapActors.insert(pair<long, CStdString>(lActorId, strActor));
      
      m_pDS->next();
    }
    m_pDS->close();

     for (it=mapActors.begin();it != mapActors.end();++it)
    {
      CFileItem* pItem=new CFileItem(it->second);
      CStdString strDir;
      strDir.Format("%ld/", it->first);
      pItem->m_strPath=strBaseDir + strDir;
      pItem->m_bIsFolder=true;
      items.Add(pItem);
    }


    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase:GetActorsNav() failed");
  }
  return false;
}

bool CVideoDatabase::GetYearsNav(const CStdString& strBaseDir, CFileItemList& items)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    // get primary genres for movies
    CStdString strSQL=FormatSQL("select * from movie where idType=%i",VIDEODB_ID_WATCHED);
    CStdString addParam;

    if (g_stSettings.m_iMyVideoWatchMode == 1)
      strSQL += FormatSQL(" and NOT (strValue='true')");

    if (g_stSettings.m_iMyVideoWatchMode == 2)
      strSQL += FormatSQL(" and strValue='true'");

    // run query
    CLog::Log(LOGDEBUG, "CVideoDatabase::GetGenresNav() query: %s", strSQL.c_str());
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return true;
    }

    map<long, CStdString> mapYears;
    map<long, CStdString>::iterator it;
    long lLastPathId = -1;
    while (!m_pDS->eof())
    {
      strSQL=FormatSQL("select * from movie where idMovie=%u and idType=%i",m_pDS->fv("movie.idMovie").get_asLong(),VIDEODB_ID_YEAR);
      m_pDS2->query(strSQL.c_str());
      if (m_pDS2->eof())
        continue;

      CStdString strYear = m_pDS2->fv("movie.strValue").get_asString();
      long lYear = atol(strYear.c_str());
      it = mapYears.find(lYear);
      if (it == mapYears.end())
      {
        mapYears.insert(pair<long, CStdString>(lYear, strYear));
      }
      m_pDS->next();
    }
    m_pDS->close();
    
    for (it=mapYears.begin();it != mapYears.end();++it)
    {
      CFileItem* pItem=new CFileItem(it->second);
      CStdString strDir;
      strDir.Format("%ld/", it->first);
      pItem->m_strPath=strBaseDir + strDir;
      pItem->m_bIsFolder=true;
      items.Add(pItem);
    }

    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase:GetYearsNav() failed");
  }
  return false;
}

bool CVideoDatabase::GetTitlesNav(const CStdString& strBaseDir, CFileItemList& items, long idGenre, long idYear, long idActor)
{
  try
  {
    DWORD time = timeGetTime();

    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    //CStdString strSQL="select * from movie,movieinfo,actors,path,files where movieinfo.idmovie=movie.idmovie and movieinfo.iddirector=actors.idActor and movie.idpath=path.idpath and files.idmovie = movie.idmovie";

    CStdString strSQL = FormatSQL("select * from movie where idType=%i",VIDEODB_ID_TITLE);

    if (idGenre != -1)
    {
      strSQL=FormatSQL("select * from movie, genrelinkmovie where genrelinkmovie.idGenre=%u and genrelinkmovie.idmovie=movie.idmovie and movie.idType=%i", idGenre, VIDEODB_ID_TITLE);
    }

    if (idYear !=-1)
    {
      strSQL=FormatSQL("select * from movie where idType=%i and strValue='%i'",VIDEODB_ID_YEAR,idYear);
    }

    if (idActor != -1)
    {
      strSQL=FormatSQL("select * from movie, actorlinkmovie, actors where actorlinkmovie.idActor=actors.idActor and movie.idType=%i and actorlinkmovie.idMovie=movie.idMovie and actors.idActor=%u",VIDEODB_ID_TITLE,idActor);
    }

    // get all songs out of the database in fixed size chunks
    // dont reserve the items ahead of time just in case it fails part way though
    VECMOVIES movies;
    if (idGenre == -1 && idYear == -1 && idActor == -1)
    {
      int iLIMIT = 5000;    // chunk size
      int iSONGS = 0;       // number of movies added to items
      int iITERATIONS = 0;  // number of iterations
      
      for (int i=0;;i+=iLIMIT)
      {
        CStdString strSQL2=strSQL+FormatSQL(" limit %i offset %i", iLIMIT, i);
        CLog::Log(LOGDEBUG, "CVideoDatabase::GetTitlesNav() query: %s", strSQL2.c_str());
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
              return true; // failed on first iteration, so there's probably no songs in the db
            else
            {
              CGUIWindowVideoBase::SetDatabaseDirectory(movies,items);
              return true; // there no more songs left to process (aborts the unbounded for loop)
            }
          }

          // get movies from returned subtable
          while (!m_pDS->eof())
          {
            long lMovieId = m_pDS->fv("movie.idMovie").get_asLong();
            CIMDBMovie movie = GetDetailsForMovie(lMovieId);
            movies.push_back(movie);
            iSONGS++;
            m_pDS->next();
          }
        }
        catch (...)
        {
          CLog::Log(LOGERROR, "CVideoDatabase::GetTitlesNav() failed at iteration %i, num songs %i", iITERATIONS, iSONGS);

          if (iSONGS > 0)
          {
            CGUIWindowVideoBase::SetDatabaseDirectory(movies,items);
            return true; // keep whatever songs we may have gotten before the failure
          }
          else
            return true; // no songs, return false
        }
        // next iteration
        iITERATIONS++;
        m_pDS->close();
      }
      CGUIWindowVideoBase::SetDatabaseDirectory(movies,items);
      return true;
    }

    // run query
    CLog::Log(LOGDEBUG, "CVideoDatabase::GetTitlesNav() query: %s", strSQL.c_str());
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return true;
    }

    CLog::DebugLog("Time for actual SQL query = %d", timeGetTime() - time); time = timeGetTime();

    // get data from returned rows
    items.Reserve(iRowsFound);

    while (!m_pDS->eof())
    {
      long lMovieId = m_pDS->fv("movie.idMovie").get_asLong();
      CIMDBMovie movie = GetDetailsForMovie(lMovieId);
      movies.push_back(movie);

      m_pDS->next();
    }

    CLog::DebugLog("Time to retrieve movies from dataset = %d", timeGetTime() - time);

    // cleanup
    m_pDS->close();
    CGUIWindowVideoBase::SetDatabaseDirectory(movies,items);
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetTitlesNav() failed");
  }
  return false;
}

bool CVideoDatabase::GetGenreById(long lIdGenre, CStdString& strGenre)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL=FormatSQL("select * from genre where genre.idGenre=%u", lIdGenre);
    m_pDS->query( strSQL.c_str() );

    bool bResult = false;
    if (!m_pDS->eof())
    {
      strGenre  = m_pDS->fv("genre.strGenre").get_asString();
      bResult = true;
    }
    m_pDS->close();
    return bResult;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetMoviesByGenre(%s) failed", strGenre.c_str());
  }
  return false;
}

int CVideoDatabase::GetMovieCount()
{
  try
  {
    if (NULL == m_pDB.get()) return 0;
    if (NULL == m_pDS.get()) return 0;

    CStdString strSQL=FormatSQL("select count (idMovie) as nummovies from movie");
    m_pDS->query( strSQL.c_str() );

    int iResult = 0;
    if (!m_pDS->eof())
    {
      iResult = m_pDS->fv("nummovies").get_asInteger();
    }
    m_pDS->close();
    return iResult;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetMovieCount() failed");
  }
  return 0;
}

void CVideoDatabase::CleanDatabase()
{
  try
  {
    BeginTransaction();
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    // find all the files
    CStdString sql = "select * from files";;
    m_pDS->query(sql.c_str());
    if (m_pDS->num_rows() == 0) return;

    CGUIDialogProgress *progress = (CGUIDialogProgress *)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
    if (progress)
    {
      progress->SetHeading(700);
      progress->SetLine(0, "");
      progress->SetLine(1, 313);
      progress->SetLine(2, 330);
      progress->SetPercentage(0);
      progress->StartModal();
      progress->ShowProgressBar(true);
    }

    CStdString filesToDelete = "(";
    CStdString moviesToDelete = "(";
    int total = m_pDS->num_rows();
    int current = 0;
    while (!m_pDS->eof())
    {
      CStdString fullPath = m_pDS->fv("files.strFileName").get_asString();

      if (CUtil::IsStack(fullPath))
      { // do something?
        CStackDirectory dir;
        CFileItemList items;
        if (dir.GetDirectory(fullPath, items) && items.Size())
          fullPath = items[0]->m_strPath; // just test the first path
      }

      // delete all removable media + ftp/http streams
      CURL url(fullPath);
      if (CUtil::IsOnDVD(fullPath) ||
          CUtil::IsMemCard(fullPath) ||
          url.GetProtocol() == "http" ||
          url.GetProtocol() == "ftp" ||
          !CFile::Exists(fullPath))
      { // mark for deletion
        filesToDelete += m_pDS->fv("files.idFile").get_asString() + ",";
        moviesToDelete += m_pDS->fv("files.idMovie").get_asString() + ",";
      }
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
    filesToDelete.TrimRight(",");
    filesToDelete += ")";
    moviesToDelete.TrimRight(",");
    moviesToDelete += ")";

    if (progress)
    {
      progress->SetPercentage(100);
      progress->Progress();
    }

    CLog::Log(LOGDEBUG, __FUNCTION__" Cleaning files table");
    sql = "delete from files where idFile in " + filesToDelete;
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, __FUNCTION__" Cleaning bookmark table");
    sql = "delete from bookmark where idFile in " + filesToDelete;
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, __FUNCTION__" Cleaning settings table");
    sql = "delete from settings where idFile in " + filesToDelete;
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, __FUNCTION__" Cleaning stacktimes table");
    sql = "delete from stacktimes where idFile in " + filesToDelete;
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, __FUNCTION__" Cleaning path table");
    sql = "delete from path where idPath not in (select distinct idPath from files)";
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, __FUNCTION__" Cleaning movie table");
    sql = "delete from movie where idMovie in " + moviesToDelete;
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, __FUNCTION__" Cleaning movieinfo table");
    sql = "delete from movieinfo where idMovie in " + moviesToDelete;
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, __FUNCTION__" Cleaning actorlinkmovie table");
    sql = "delete from actorlinkmovie where idMovie in " + moviesToDelete;
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, __FUNCTION__" Cleaning actor table of actors and directors");
    sql = "delete from actors where idActor not in (select distinct idActor from actorlinkmovie) and idActor not in (select distinct idDirector from movieinfo)";
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, __FUNCTION__" Cleaning genrelinkmovie table");
    sql = "delete from genrelinkmovie where idMovie in " + moviesToDelete;
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, __FUNCTION__" Cleaning genre table");
    sql = "delete from genre where idGenre not in (select distinct idGenre from genrelinkmovie)";
    m_pDS->exec(sql.c_str());
 
    CommitTransaction();

    Compress();

    if (progress)
      progress->Close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::CleanDatabase() failed");
  }
}