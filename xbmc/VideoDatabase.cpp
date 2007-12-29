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
#include "videodatabase.h"
#include "GUIWindowVideoBase.h"
#include "utils/fstrcmp.h"
#include "utils/RegExp.h"
#include "util.h"
#include "XMLUtils.h"
#include "GUIPassword.h"
#include "filesystem/StackDirectory.h"
#include "filesystem/MultiPathDirectory.h"
#include "VideoInfoScanner.h"

using namespace XFILE;
using namespace DIRECTORY;
using namespace VIDEO;

#define VIDEO_DATABASE_VERSION 13
#define VIDEO_DATABASE_OLD_VERSION 3.f
#define VIDEO_DATABASE_NAME "MyVideos34.db"
#define RECENTLY_ADDED_LIMIT  25

CBookmark::CBookmark()
{
  timeInSeconds = 0.0f;
  type = STANDARD;
}

//********************************************************************************************************************************
CVideoDatabase::CVideoDatabase(void)
{
  m_preV2version=VIDEO_DATABASE_OLD_VERSION;
  m_version = VIDEO_DATABASE_VERSION;
  m_strDatabaseFile=VIDEO_DATABASE_NAME;
}

//********************************************************************************************************************************
CVideoDatabase::~CVideoDatabase(void)
{}

//********************************************************************************************************************************
bool CVideoDatabase::CreateTables()
{
  /* indexes should be added on any columns that are used in in  */ 
  /* a where or a join. primary key on a column is the same as a */ 
  /* unique index on that column, so there is no need to add any */
  /* index if no other columns are refered                       */

  /* order of indexes are important, for an index to be considered all  */
  /* columns up to the column in question have to have been specified   */
  /* select * from actorlinkmovie where idMovie = 1, can not take       */
  /* advantage of a index that has been created on ( idGenre, idMovie ) */
  /*, hower on on ( idMovie, idGenre ) will be considered for use       */

  try
  {
    CDatabase::CreateTables();

    CLog::Log(LOGINFO, "create bookmark table");
    m_pDS->exec("CREATE TABLE bookmark ( idBookmark integer primary key, idFile integer, timeInSeconds double, thumbNailImage text, player text, playerState text, type integer)\n");
    m_pDS->exec("CREATE INDEX ix_bookmark ON bookmark (idFile)");

    CLog::Log(LOGINFO, "create settings table");
    m_pDS->exec("CREATE TABLE settings ( idFile integer, Interleaved bool, NoCache bool, Deinterlace bool, FilmGrain integer,"
                "ViewMode integer,ZoomAmount float, PixelRatio float, AudioStream integer, SubtitleStream integer,"
                "SubtitleDelay float, SubtitlesOn bool, Brightness integer, Contrast integer, Gamma integer,"
                "VolumeAmplification float, AudioDelay float, OutputToAllSpeakers bool, ResumeTime integer, Crop bool, CropLeft integer,"
                "CropRight integer, CropTop integer, CropBottom integer)\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_settings ON settings ( idFile )\n");

    CLog::Log(LOGINFO, "create stacktimes table");
    m_pDS->exec("CREATE TABLE stacktimes (idFile integer, times text)\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_stacktimes ON stacktimes ( idFile )\n");

    CLog::Log(LOGINFO, "create genre table");
    m_pDS->exec("CREATE TABLE genre ( idGenre integer primary key, strGenre text)\n");

    CLog::Log(LOGINFO, "create genrelinkmovie table");
    m_pDS->exec("CREATE TABLE genrelinkmovie ( idGenre integer, idMovie integer)\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_genrelinkmovie_1 ON genrelinkmovie ( idGenre, idMovie)\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_genrelinkmovie_2 ON genrelinkmovie ( idMovie, idGenre)\n");

    CLog::Log(LOGINFO, "create movie table");
    CStdString columns = "CREATE TABLE movie ( idMovie integer primary key";
    for (int i = 0; i < VIDEODB_MAX_COLUMNS; i++)
    {
      CStdString column;
      column.Format(",c%02d text", i);
      columns += column;
    }
    columns += ",idFile integer)";
    m_pDS->exec(columns.c_str());
    m_pDS->exec("CREATE UNIQUE INDEX ix_movie_file_1 ON movie (idFile, idMovie)");
    m_pDS->exec("CREATE UNIQUE INDEX ix_movie_file_2 ON movie (idMovie, idFile)");

    CLog::Log(LOGINFO, "create actorlinkmovie table");
    m_pDS->exec("CREATE TABLE actorlinkmovie ( idActor integer, idMovie integer, strRole text)\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_actorlinkmovie_1 ON actorlinkmovie ( idActor, idMovie )\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_actorlinkmovie_2 ON actorlinkmovie ( idMovie, idActor )\n");

    CLog::Log(LOGINFO, "create directorlinkmovie table");
    m_pDS->exec("CREATE TABLE directorlinkmovie ( idDirector integer, idMovie integer)\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_directorlinkmovie_1 ON directorlinkmovie ( idDirector, idMovie )\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_directorlinkmovie_2 ON directorlinkmovie ( idMovie, idDirector )\n");

    CLog::Log(LOGINFO, "create actors table");
    m_pDS->exec("CREATE TABLE actors ( idActor integer primary key, strActor text, strThumb text )\n");

    CLog::Log(LOGINFO, "create path table");
    m_pDS->exec("CREATE TABLE path ( idPath integer primary key, strPath text, strContent text, strScraper text, strHash text, scanRecursive integer, useFolderNames bool)\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_path ON path ( strPath )\n");

    CLog::Log(LOGINFO, "create files table");
    m_pDS->exec("CREATE TABLE files ( idFile integer primary key, idPath integer, strFilename text)\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_files ON files ( idPath, strFilename )\n");

    CLog::Log(LOGINFO, "create tvshow table");
    columns = "CREATE TABLE tvshow ( idShow integer primary key";
    for (int i = 0; i < VIDEODB_MAX_COLUMNS; i++)
    {
      CStdString column;
      column.Format(",c%02d text", i);
      columns += column;
    }
    columns += ")";
    m_pDS->exec(columns.c_str());

    CLog::Log(LOGINFO, "create directorlinktvshow table");
    m_pDS->exec("CREATE TABLE directorlinktvshow ( idDirector integer, idShow integer)\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_directorlinktvshow_1 ON directorlinktvshow ( idDirector, idShow )\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_directorlinktvshow_2 ON directorlinktvshow ( idShow, idDirector )\n");

    CLog::Log(LOGINFO, "create actorlinktvshow table");
    m_pDS->exec("CREATE TABLE actorlinktvshow ( idActor integer, idShow integer, strRole text)\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_actorlinktvshow_1 ON actorlinktvshow ( idActor, idShow )\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_actorlinktvshow_2 ON actorlinktvshow ( idShow, idActor )\n");
  
    CLog::Log(LOGINFO, "create episode table");
    columns = "CREATE TABLE episode ( idEpisode integer primary key";
    for (int i = 0; i < VIDEODB_MAX_COLUMNS; i++)
    {
      CStdString column;
      column.Format(",c%02d text", i);
      columns += column;
    }
    columns += ",idFile integer)";
    m_pDS->exec(columns.c_str());
    m_pDS->exec("CREATE UNIQUE INDEX ix_episode_file_1 on episode (idEpisode, idFile)");
    m_pDS->exec("CREATE UNIQUE INDEX id_episode_file_2 on episode (idFile, idEpisode)");
    CStdString createColIndex;
    createColIndex.Format("CREATE INDEX ix_episode_season_episode on episode (c%02d, c%02d)", VIDEODB_ID_EPISODE_SEASON, VIDEODB_ID_EPISODE_EPISODE);
    m_pDS->exec(createColIndex.c_str());
    createColIndex.Format("CREATE INDEX ix_episode_bookmark on episode (c%02d)", VIDEODB_ID_EPISODE_BOOKMARK);
    m_pDS->exec(createColIndex.c_str());

    CLog::Log(LOGINFO, "create tvshowlinkepisode table");
    m_pDS->exec("CREATE TABLE tvshowlinkepisode ( idShow integer, idEpisode integer)\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_tvshowlinkepisode_1 ON tvshowlinkepisode ( idShow, idEpisode )\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_tvshowlinkepisode_2 ON tvshowlinkepisode ( idEpisode, idShow )\n");

    CLog::Log(LOGINFO, "create tvshowlinkpath table");
    m_pDS->exec("CREATE TABLE tvshowlinkpath (idShow integer, idPath integer)\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_tvshowlinkpath_1 ON tvshowlinkpath ( idShow, idPath )\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_tvshowlinkpath_2 ON tvshowlinkpath ( idPath, idShow )\n");

    CLog::Log(LOGINFO, "create actorlinkepisode table");
    m_pDS->exec("CREATE TABLE actorlinkepisode ( idActor integer, idEpisode integer, strRole text)\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_actorlinkepisode_1 ON actorlinkepisode ( idActor, idEpisode )\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_actorlinkepisode_2 ON actorlinkepisode ( idEpisode, idActor )\n");

    CLog::Log(LOGINFO, "create directorlinkepisode table");
    m_pDS->exec("CREATE TABLE directorlinkepisode ( idDirector integer, idEpisode integer, strRole text)\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_directorlinkepisode_1 ON directorlinkepisode ( idDirector, idEpisode )\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_directorlinkepisode_2 ON directorlinkepisode ( idEpisode, idDirector )\n");

    CLog::Log(LOGINFO, "create genrelinktvshow table");
    m_pDS->exec("CREATE TABLE genrelinktvshow ( idGenre integer, idShow integer)\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_genrelinktvshow_1 ON genrelinktvshow ( idGenre, idShow)\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_genrelinktvshow_2 ON genrelinktvshow ( idShow, idGenre)\n");

    CLog::Log(LOGINFO, "create genrelinkepisode table");
    m_pDS->exec("CREATE TABLE genrelinkepisode ( idGenre integer, idEpisode integer)\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_genrelinkepisode_1 ON genrelinkepisode ( idGenre, idEpisode)\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_genrelinkepisode_2 ON genrelinkepisode ( idEpisode, idGenre)\n");

    CLog::Log(LOGINFO, "create movielinktvshow table");
    m_pDS->exec("CREATE TABLE movielinktvshow ( idMovie integer, IdShow integer)\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_movielinktvshow_1 ON movielinktvshow ( idShow, idMovie)\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_movielinktvshow_2 ON movielinktvshow ( idMovie, idShow)\n");

    CLog::Log(LOGINFO, "create studio table");
    m_pDS->exec("CREATE TABLE studio ( idStudio integer primary key, strStudio text)\n");

    CLog::Log(LOGINFO, "create studiolinkmovie table");
    m_pDS->exec("CREATE TABLE studiolinkmovie ( idStudio integer, idMovie integer)\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_studiolinkmovie_1 ON studiolinkmovie ( idStudio, idMovie)\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_studiolinkmovie_2 ON studiolinkmovie ( idMovie, idStudio)\n");

    CLog::Log(LOGINFO, "create musicvideo table");
    columns = "CREATE TABLE musicvideo ( idMVideo integer primary key";
    for (int i = 0; i < VIDEODB_MAX_COLUMNS; i++)
    {
      CStdString column;
      column.Format(",c%02d text", i);
      columns += column;
    }
    columns += ",idFile integer)";
    m_pDS->exec(columns.c_str());
    m_pDS->exec("CREATE UNIQUE INDEX ix_musicvideo_file_1 on musicvideo (idMVideo, idFile)");
    m_pDS->exec("CREATE UNIQUE INDEX ix_musicvideo_file_2 on musicvideo (idFile, idMVideo)");
    
    CLog::Log(LOGINFO, "create artistlinkmusicvideo table");
    m_pDS->exec("CREATE TABLE artistlinkmusicvideo ( idArtist integer, idMVideo integer)\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_artistlinkmusicvideo_1 ON artistlinkmusicvideo ( idArtist, idMVideo)\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_artistlinkmusicvideo_2 ON artistlinkmusicvideo ( idMVideo, idArtist)\n");

    CLog::Log(LOGINFO, "create genrelinkmusicvideo table");
    m_pDS->exec("CREATE TABLE genrelinkmusicvideo ( idGenre integer, idMVideo integer)\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_genrelinkmusicvideo_1 ON genrelinkmusicvideo ( idGenre, idMVideo)\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_genrelinkmusicvideo_2 ON genrelinkmusicvideo ( idMVideo, idGenre)\n");

    CLog::Log(LOGINFO, "create studiolinkmusicvideo table");
    m_pDS->exec("CREATE TABLE studiolinkmusicvideo ( idStudio integer, idMVideo integer)\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_studiolinkmusicvideo_1 ON studiolinkmusicvideo ( idStudio, idMVideo)\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_studiolinkmusicvideo_2 ON studiolinkmusicvideo ( idMVideo, idStudio)\n");

    CLog::Log(LOGINFO, "create directorlinkmusicvideo table");
    m_pDS->exec("CREATE TABLE directorlinkmusicvideo ( idDirector integer, idMVideo integer)\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_directorlinkmusicvideo_1 ON directorlinkmusicvideo ( idDirector, idMVideo )\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_directorlinkmusicvideo_2 ON directorlinkmusicvideo ( idMVideo, idDirector )\n");
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
    
    CStdString strPath1(strPath);
    CUtil::AddSlashAtEnd(strPath1);

    strSQL=FormatSQL("select idPath from path where strPath like '%s'",strPath1.c_str());
    m_pDS->query(strSQL.c_str());
    if (!m_pDS->eof())
      lPathId = m_pDS->fv("path.idPath").get_asLong();

    return lPathId;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "videodatabase:unable to getpath (%s)", strSQL.c_str());
  }
  return -1;
}

bool CVideoDatabase::GetPaths(map<CStdString,VIDEO::SScanSettings> &paths)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    paths.clear();
     
    SScanSettings settings;
    SScraperInfo info;

    // grab all paths with movie content set
    if (!m_pDS->query("select strPath,scanRecursive,useFolderNames from path"
                      " where (strContent = 'movies' or strContent = 'musicvideos')"
                      " and strPath NOT like 'stack://%%'"
                      " and strPath NOT like 'multipath://%%'"
                      " order by strPath"))
      return false;

    while (!m_pDS->eof())
    {
      CStdString strPath = m_pDS->fv("strPath").get_asString();
      
      settings.parent_name = m_pDS->fv("useFolderNames").get_asBool();
      settings.recurse     = m_pDS->fv("scanRecursive").get_asInteger();
      settings.parent_name_root = settings.parent_name && !settings.recurse;

      paths.insert(pair<CStdString,SScanSettings>(strPath,settings));
      m_pDS->next();
    }
    m_pDS->close();

    // then grab all tvshow paths
    if (!m_pDS->query("select strPath,scanRecursive,useFolderNames,strContent from path"
                      " where ( strContent = 'tvshows'"
                      "       or idPath in (select idpath from tvshowlinkpath))"
                      " and strPath NOT like 'stack://%%'"
                      " and strPath NOT like 'multipath://%%'"
                      " order by strPath"))
      return false;

    while (!m_pDS->eof())
    {
      CStdString strPath = m_pDS->fv("strPath").get_asString();
      CStdString strContent = m_pDS->fv("strContent").get_asString();
      if(strContent.Equals("tvshows"))
      {      
        settings.parent_name = m_pDS->fv("useFolderNames").get_asBool();
        settings.recurse     = m_pDS->fv("scanRecursive").get_asInteger();
        settings.parent_name_root = settings.parent_name && !settings.recurse;
      }
      else
      {
        settings.parent_name = true;
        settings.recurse = 0;
        settings.parent_name_root = true;
      }

      paths.insert(pair<CStdString,SScanSettings>(strPath,settings));
      m_pDS->next();
    }
    m_pDS->close();

    // finally grab all other paths holding a movie which is not a stack or a rar archive 
    // - this isnt perfect but it should do fine in most situations.
    // reason we need it to hold a movie is stacks from different directories (cdx folders for instance)
    // not making mistakes must take priority
    if (!m_pDS->query("select strPath from path" 
                       " where idPath in (select idPath from files join movie on movie.idFile=files.idFile)"
                       " and idPath NOT in (select idpath from tvshowlinkpath)"
                       " and idPath NOT in (select idpath from files where strFileName like 'video_ts.ifo')" // dvdfolders get stacked to a single item in parent folder
                       " and strPath NOT like 'stack://%%'"
                       " and strPath NOT like 'multipath://%%'"
                       " and strPath NOT like 'rar://%%'"
                       " and strPath NOT like 'zip://%%'"
                       " and strContent NOT in ('movies', 'tvshows', 'None')" // these have been added above
                       " order by strPath"))

      return false;
    while (!m_pDS->eof())
    {
      CStdString strPath = m_pDS->fv("strPath").get_asString();
      
      settings.parent_name = false;
      settings.recurse = 0;
      settings.parent_name_root = settings.parent_name && !settings.recurse;
  
      paths.insert(pair<CStdString,SScanSettings>(strPath,settings));
      m_pDS->next();
    }
    m_pDS->close();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, __FUNCTION__" failed");
  }
  return false;
}

long CVideoDatabase::AddPath(const CStdString& strPath)
{
  CStdString strSQL;
  try
  {
    long lPathId;
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    CStdString strPath1(strPath);
    CUtil::AddSlashAtEnd(strPath1);

    strSQL=FormatSQL("insert into path (idPath, strPath, strContent, strScraper) values (NULL,'%s','','')", strPath1.c_str());
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

bool CVideoDatabase::GetPathHash(const CStdString &path, CStdString &hash)
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
    CLog::Log(LOGERROR, __FUNCTION__"(%s) failed", path.c_str());
  }

  return false;
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
    long lPathId=GetPath(strPath);
    if (lPathId < 0)
      lPathId = AddPath(strPath);

    if (lPathId < 0)
      return -1;

    CStdString strSQL=FormatSQL("select idFile from files where strFileName like '%s' and idPath=%u", strFileName.c_str(),lPathId);

    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() > 0)
    {
      lFileId = m_pDS->fv("idFile").get_asLong() ;
      m_pDS->close();
      return lFileId;
    }
    m_pDS->close();
    strSQL=FormatSQL("insert into files (idFile,idPath,strFileName) values(NULL, %u, '%s')", lPathId,strFileName.c_str());
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

bool CVideoDatabase::SetPathHash(const CStdString &path, const CStdString &hash)
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
    long pathId = GetPath(path);
    if (pathId < 0) return false;

    CStdString strSQL=FormatSQL("update path set strHash='%s' where idPath=%ld", hash.c_str(), pathId);
    m_pDS->exec(strSQL.c_str());

    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, __FUNCTION__"(%s, %s) failed", path.c_str(), hash.c_str());
  }

  return false;
}

bool CVideoDatabase::LinkMovieToTvshow(long idMovie, long idShow)
{
   try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    if (idShow == -1) // delete link
    {
      CStdString strSQL=FormatSQL("delete from movielinktvshow where idMovie=%u", idMovie);
      m_pDS->exec(strSQL.c_str());
      return true;
    }

    CStdString strSQL=FormatSQL("insert into movielinktvshow (idShow,idMovie) values (%u,%u)", idShow,idMovie);
    m_pDS->exec(strSQL.c_str());

    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, __FUNCTION__"(%u, %u) failed", idMovie, idShow);
  }

  return false;
}

bool CVideoDatabase::IsLinkedToTvshow(long idMovie)
{
   try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL=FormatSQL("select * from movielinktvshow where idMovie=%u", idMovie);
    m_pDS->query(strSQL.c_str());
    if (m_pDS->eof())
    {
      m_pDS->close();
      return false;
    }

    m_pDS->close();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, __FUNCTION__"(%u) failed", idMovie);
  }

  return false;
}


//********************************************************************************************************************************
long CVideoDatabase::GetFile(const CStdString& strFilenameAndPath)
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
    strSQL=FormatSQL("select idFile from files where strFileName like '%s' and idPath=%u", strFileName.c_str(),lPathId);
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() > 0)
    {
      long lFileId = m_pDS->fv("files.idFile").get_asLong();
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
long CVideoDatabase::GetMovieInfo(const CStdString& strFilenameAndPath)
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;
    long lMovieId = -1;

    // needed for query parameters
    long lFileId = GetFile(strFilenameAndPath);
    long lPathId=-1;
    CStdString strPath, strFile;
    if (lFileId < 0)
    {
      CUtil::Split(strFilenameAndPath, strPath, strFile);

      // have to join movieinfo table for correct results
      lPathId = GetPath(strPath);
      if (lPathId < 0 && strPath != strFilenameAndPath)
        return -1;
    }

    if (lFileId == -1 && strPath != strFilenameAndPath)
      return -1;
    
    CStdString strSQL;
    if (lFileId == -1 || strPath == strFilenameAndPath) // i.e. we where handed a path, we may have rarred items in it
    {
      if (lPathId == -1)
      {
        strSQL=FormatSQL("select movie.idMovie from movie join path on files.idPath = path.idPath join files on movie.idFile=files.idFile where path.strPath like '%%%s%%'",strPath.c_str());
        m_pDS->query(strSQL.c_str());
        if (m_pDS->eof())
        {
          CUtil::URLEncode(strPath);
          strSQL=FormatSQL("select movie.idMovie from movie join files on files.idPath = path.idPath where movie.idFile=files.idFile and path.strPath like '%%%s%%'",strPath.c_str());
        }
      }
      else
      {
        strSQL=FormatSQL("select movie.idMovie from movie join files on files.idFile=movie.idFile where files.idpath = %u",lPathId);
        m_pDS->query(strSQL.c_str());
        if (m_pDS->num_rows() > 0)
          lMovieId = m_pDS->fv("movie.idMovie").get_asLong();  
        if (m_pDS->eof() || lMovieId == -1)
        {
          strSQL=FormatSQL("select movie.idMovie from movie join files on files.idFile=movie.idfile join path on files.idPath = path.idPath where path.strPath like '%%%s%%'",strPath.c_str());
          m_pDS->query(strSQL.c_str());
          if (m_pDS->eof())
          {
            CUtil::URLEncode(strPath);
            strSQL=FormatSQL("select movie.idMovie from movie join files on files.idPath = path.idPath join path on files.idPath = path.idPath where movie.idFile=files.idFile and path.strPath like '%%%s%%'",strPath.c_str());
          }
        }
      }
    }
    else
      strSQL=FormatSQL("select idMovie from movie where idFile=%u", lFileId);
    
    CLog::Log(LOGDEBUG,"CVideoDatabase::GetMovieInfo(%s), query = %s", strFilenameAndPath.c_str(), strSQL.c_str());
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() > 0)
      lMovieId = m_pDS->fv("movie.idMovie").get_asLong();  
    m_pDS->close();

    return lMovieId;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetMovieInfo(%s) failed", strFilenameAndPath.c_str());
  }
  return -1;
}

long CVideoDatabase::GetTvShowInfo(const CStdString& strPath)
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;
    long lTvShowId = -1;

    // have to join movieinfo table for correct results
    long lPathId = GetPath(strPath);
    if (lPathId < 0)
      return -1;

    CStdString strSQL;
    CStdString strPath1=strPath;
    CStdString strParent;
    int iFound=0;

    strSQL=FormatSQL("select tvshow.idshow from tvshow,tvshowlinkpath where tvshowlinkpath.idPath=%u and tvshow.idshow=tvshowlinkpath.idshow",lPathId);
    m_pDS->query(strSQL);
    if (!m_pDS->eof())
      iFound = 1;

    while (iFound == 0 && CUtil::GetParentPath(strPath1, strParent))
    {
      strSQL=FormatSQL("select tvshowlinkpath.idShow from path,tvshowlinkpath where tvshowlinkpath.idpath = path.idpath and strPath like '%s'",strParent.c_str());
      m_pDS->query(strSQL.c_str());
      if (!m_pDS->eof())
      {
        long idShow = m_pDS->fv("tvshowlinkpath.idshow").get_asLong();
        if (idShow != -1)
        {
          strSQL=FormatSQL("select tvshow.idshow from tvshow where idShow=%i",idShow);
          iFound = 2;
        }
      }
      strPath1 = strParent;
    }

    if (m_pDS->num_rows() > 0)
      lTvShowId = m_pDS->fv("tvshow.idShow").get_asLong();  
    m_pDS->close();

    return lTvShowId;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetTvShowInfo(%s) failed", strPath.c_str());
  }
  return -1;
}

long CVideoDatabase::GetEpisodeInfo(const CStdString& strFilenameAndPath, long lEpisodeId) // input value is episode number hint - for twoparters
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    // need this due to the nested GetEpisodeInfo query
    auto_ptr<Dataset> pDS;
    pDS.reset(m_pDB->CreateDataset());
    if (NULL == pDS.get()) return -1;

    long lFileId = GetFile(strFilenameAndPath);
    if (lFileId < 0)
      return -1;

    CStdString strSQL=FormatSQL("select idEpisode from episode where idFile=%u", lFileId);
    
    CLog::Log(LOGDEBUG,"CVideoDatabase::GetEpisodeInfo(%s), query = %s", strFilenameAndPath.c_str(), strSQL.c_str());
    pDS->query(strSQL.c_str());
    if (pDS->num_rows() > 0)
    {
      if (lEpisodeId == -1)
        lEpisodeId = pDS->fv("episode.idEpisode").get_asLong();
      else // use the hint!
      {
        while (!pDS->eof())
        {
          CVideoInfoTag tag;
          GetEpisodeInfo(strFilenameAndPath,tag,pDS->fv("episode.idEpisode").get_asLong());
          if (tag.m_iEpisode == lEpisodeId)
          {
            lEpisodeId = pDS->fv("episode.idEpisode").get_asLong();
            break;
          }
          pDS->next();
        }
        if (pDS->eof())
          lEpisodeId = -1;
      }
    }
    else
      lEpisodeId = -1;

    pDS->close();

    return lEpisodeId;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetEpisodeInfo(%s) failed", strFilenameAndPath.c_str());
  }
  return -1;
}

long CVideoDatabase::GetMusicVideoInfo(const CStdString& strFilenameAndPath)
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    long lFileId = GetFile(strFilenameAndPath);
    if (lFileId < 0)
      return -1;

    CStdString strSQL=FormatSQL("select idMVideo from musicvideo where idFile=%u", lFileId);
    
    CLog::Log(LOGDEBUG,"CVideoDatabase::GetMusicVideoInfo(%s), query = %s", strFilenameAndPath.c_str(), strSQL.c_str());
    m_pDS->query(strSQL.c_str());
    long lMVideoId=-1;
    if (m_pDS->num_rows() > 0)
      lMVideoId = m_pDS->fv("musicvideo.idMVideo").get_asLong();
    m_pDS->close();

    return lMVideoId;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetMusicVideoInfo(%s) failed", strFilenameAndPath.c_str());
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

    CStdString strSQL=FormatSQL("select idMovie from movie order by idMovie desc limit %d", nSize);
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
    lFileId = GetFile(strFilenameAndPath);
    if (lFileId < 0)
      lFileId = AddFile(strFilenameAndPath);
    else
      lMovieId = GetMovieInfo(strFilenameAndPath);
    if (lMovieId < 0)
    {
      CStdString strSQL=FormatSQL("insert into movie (idMovie, idFile) values (NULL, %u)", lFileId);
      m_pDS->exec(strSQL.c_str());
      lMovieId = (long)sqlite3_last_insert_rowid(m_pDB->getHandle());
//      CommitTransaction();
    }
    
    return lMovieId;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::AddMovie(%s) failed", strFilenameAndPath.c_str());
  }
  return -1;
}

long CVideoDatabase::AddTvShow(const CStdString& strPath)
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;
    
    CStdString strSQL=FormatSQL("select tvshowlinkpath.idShow from path,tvshowlinkpath where path.strPath like '%s' and path.idPath = tvshowlinkpath.idPath",strPath.c_str());
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() != 0)
      return m_pDS->fv("tvshowlinkpath.idShow").get_asLong();

    strSQL=FormatSQL("insert into tvshow (idShow) values (NULL)");
    m_pDS->exec(strSQL.c_str());
    long lTvShow = (long)sqlite3_last_insert_rowid(m_pDB->getHandle());
    
    long lPathId = GetPath(strPath);
    if (lPathId < 0)
      lPathId = AddPath(strPath);
    strSQL=FormatSQL("insert into tvshowlinkpath values (%u,%u)",lTvShow,lPathId);
    m_pDS->exec(strSQL.c_str());

//    CommitTransaction();
    
    return lTvShow;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::AddTvShow(%s) failed", strPath.c_str());
  }
  return -1;
}

//********************************************************************************************************************************
long CVideoDatabase::AddEpisode(long idShow, const CStdString& strFilenameAndPath)
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;
    
    long lFileId, lEpisodeId=-1;
    lFileId = GetFile(strFilenameAndPath);
    if (lFileId < 0)
      lFileId = AddFile(strFilenameAndPath);

    CStdString strSQL=FormatSQL("insert into episode (idEpisode, idFile) values (NULL, %u)", lFileId);
    m_pDS->exec(strSQL.c_str());
    lEpisodeId = (long)sqlite3_last_insert_rowid(m_pDB->getHandle());

    strSQL=FormatSQL("insert into tvshowlinkepisode (idShow,idEpisode) values (%i,%i)",idShow,lEpisodeId);
    m_pDS->exec(strSQL.c_str());
    // and update the show
    strSQL=FormatSQL("update tvshow set c%02d=(select count(idEpisode) from tvshowlinkepisode where idshow=%u) where idshow=%u",VIDEODB_ID_TV_EPISODES,idShow,idShow);
    m_pDS->exec(strSQL.c_str());
//    CommitTransaction();

    return lEpisodeId;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::AddEpisode(%s) failed", strFilenameAndPath.c_str());
  }
  return -1;
}

long CVideoDatabase::AddMusicVideo(const CStdString& strFilenameAndPath)
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;
    
    long lFileId, lMVideoId=-11;
    lFileId = GetFile(strFilenameAndPath);
    if (lFileId < 0)
      lFileId = AddFile(strFilenameAndPath);
    else
      lMVideoId = GetMusicVideoInfo(strFilenameAndPath);
    if (lMVideoId < 0)
    {
      CStdString strSQL=FormatSQL("insert into musicvideo (idMVideo, idFile) values (NULL, %u)", lFileId);
      m_pDS->exec(strSQL.c_str());
      lMVideoId = (long)sqlite3_last_insert_rowid(m_pDB->getHandle());
    }
    
    return lMVideoId;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::AddMusicVideo(%s) failed", strFilenameAndPath.c_str());
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

    CStdString strSQL=FormatSQL("select idGenre from genre where strGenre like '%s'", strGenre.c_str());
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
long CVideoDatabase::AddActor(const CStdString& strActor, const CStdString& strThumb)
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;
    CStdString strSQL=FormatSQL("select idActor from Actors where strActor like '%s'", strActor.c_str());
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      // doesnt exists, add it
      strSQL=FormatSQL("insert into Actors (idActor, strActor, strThumb) values( NULL, '%s','%s')", strActor.c_str(),strThumb.c_str());
      m_pDS->exec(strSQL.c_str());
      long lActorId = (long)sqlite3_last_insert_rowid(m_pDB->getHandle());
      return lActorId;
    }
    else
    {
      const field_value value = m_pDS->fv("idActor");
      long lActorId = value.get_asLong() ;
      // update the thumb url's
      strSQL=FormatSQL("update actors set strThumb='%s' where idActor=%u",strThumb.c_str(),lActorId);
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

long CVideoDatabase::AddStudio(const CStdString& strStudio)
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    CStdString strSQL=FormatSQL("select idStudio from studio where strStudio like '%s'", strStudio.c_str());
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      // doesnt exists, add it
      strSQL=FormatSQL("insert into studio (idStudio, strStudio) values( NULL, '%s')", strStudio.c_str());
      m_pDS->exec(strSQL.c_str());
      long lStudioId = (long)sqlite3_last_insert_rowid(m_pDB->getHandle());
      return lStudioId;
    }
    else
    {
      const field_value value = m_pDS->fv("idStudio");
      long lStudioId = value.get_asLong() ;
      m_pDS->close();
      return lStudioId;
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::AddStudio(%s) failed", strStudio.c_str() );
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

void CVideoDatabase::AddGenreToTvShow(long lTvShowId, long lGenreId)
{
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    CStdString strSQL=FormatSQL("select * from genrelinktvshow where idGenre=%i and idShow=%i", lGenreId, lTvShowId);
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() == 0)
    {
      // doesnt exists, add it
      strSQL=FormatSQL("insert into genrelinktvshow (idGenre, idShow) values( %i,%i)", lGenreId, lTvShowId);
      m_pDS->exec(strSQL.c_str());
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::AddGenreToTvShow() failed");
  }
}

void CVideoDatabase::AddGenreToEpisode(long lEpisodeId, long lGenreId)
{
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    CStdString strSQL=FormatSQL("select * from genrelinkepisode where idGenre=%i and idEpisode=%i", lGenreId, lEpisodeId);
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() == 0)
    {
      // doesnt exists, add it
      strSQL=FormatSQL("insert into genrelinkepisode (idGenre, idEpisode) values( %i,%i)", lGenreId, lEpisodeId);
      m_pDS->exec(strSQL.c_str());
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::AddGenreToEpisode() failed");
  }
}

void CVideoDatabase::AddGenreToMusicVideo(long lMVideoId, long lGenreId)
{
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    CStdString strSQL=FormatSQL("select * from genrelinkmusicvideo where idGenre=%i and idMVideo=%i", lGenreId, lMVideoId);
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() == 0)
    {
      // doesnt exists, add it
      strSQL=FormatSQL("insert into genrelinkmusicvideo (idGenre, idMVideo) values( %i,%i)", lGenreId, lMVideoId);
      m_pDS->exec(strSQL.c_str());
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::AddGenreToMusicVideo() failed");
  }
}

//********************************************************************************************************************************
void CVideoDatabase::AddActorToMovie(long lMovieId, long lActorId, const CStdString& strRole)
{
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    CStdString strSQL=FormatSQL("select * from actorlinkmovie where idActor=%u and idMovie=%u", lActorId, lMovieId);
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() == 0)
    {
      // doesnt exists, add it
      strSQL=FormatSQL("insert into actorlinkmovie (idActor, idMovie, strRole) values( %i,%i,'%s')", lActorId, lMovieId, strRole.c_str());
      m_pDS->exec(strSQL.c_str());
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::AddActorToMovie() failed");
  }
}

void CVideoDatabase::AddActorToTvShow(long lTvShowId, long lActorId, const CStdString& strRole)
{
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    CStdString strSQL=FormatSQL("select * from actorlinktvshow where idActor=%u and idShow=%u", lActorId, lTvShowId);
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() == 0)
    {
      // doesnt exists, add it
      strSQL=FormatSQL("insert into actorlinktvshow (idActor, idShow, strRole) values( %i,%i,'%s')", lActorId, lTvShowId, strRole.c_str());
      m_pDS->exec(strSQL.c_str());
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::AddActorToTvShow() failed");
  }
}

void CVideoDatabase::AddActorToEpisode(long lEpisodeId, long lActorId, const CStdString& strRole)
{
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    CStdString strSQL=FormatSQL("select * from actorlinkepisode where idActor=%u and idEpisode=%u", lActorId, lEpisodeId);
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() == 0)
    {
      // doesnt exists, add it
      strSQL=FormatSQL("insert into actorlinkepisode (idActor, idEpisode, strRole) values( %i,%i,'%s')", lActorId, lEpisodeId, strRole.c_str());
      m_pDS->exec(strSQL.c_str());
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::AddActorToEpisode() failed");
  }
}

void CVideoDatabase::AddArtistToMusicVideo(long lMVideoId, long lArtistId)
{
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    CStdString strSQL=FormatSQL("select * from artistlinkmusicvideo where idArtist=%u and idMVideo=%u", lArtistId, lMVideoId);
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() == 0)
    {
      // doesnt exists, add it
      strSQL=FormatSQL("insert into artistlinkmusicvideo (idArtist, idMVideo) values( %i,%i)", lArtistId, lMVideoId);
      m_pDS->exec(strSQL.c_str());
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::AddArtistToMusicVideo() failed");
  }
}

//********************************************************************************************************************************
void CVideoDatabase::AddDirectorToMovie(long lMovieId, long lDirectorId)
{
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    CStdString strSQL=FormatSQL("select * from directorlinkmovie where idDirector=%u and idMovie=%u", lDirectorId, lMovieId);
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() == 0)
    {
      // doesnt exists, add it
      strSQL=FormatSQL("insert into directorlinkmovie (idDirector, idMovie) values( %i,%i)", lDirectorId, lMovieId);
      m_pDS->exec(strSQL.c_str());
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::AddDirectorToMovie() failed");
  }
}

void CVideoDatabase::AddDirectorToTvShow(long lTvShowId, long lDirectorId)
{
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    CStdString strSQL=FormatSQL("select * from directorlinktvshow where idDirector=%u and idShow=%u", lDirectorId, lTvShowId);
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() == 0)
    {
      // doesnt exists, add it
      strSQL=FormatSQL("insert into directorlinktvshow (idDirector, idShow) values( %i,%i)", lDirectorId, lTvShowId);
      m_pDS->exec(strSQL.c_str());
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::AddDirectorToTvShow() failed");
  }
}

void CVideoDatabase::AddDirectorToEpisode(long lEpisodeId, long lDirectorId)
{
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    CStdString strSQL=FormatSQL("select * from directorlinkepisode where idDirector=%u and idEpisode=%u", lDirectorId, lEpisodeId);
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() == 0)
    {
      // doesnt exists, add it
      strSQL=FormatSQL("insert into directorlinkepisode (idDirector, idEpisode) values( %i,%i)", lDirectorId, lEpisodeId);
      m_pDS->exec(strSQL.c_str());
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::AddDirectorToEpisode() failed");
  }
}
void CVideoDatabase::AddDirectorToMusicVideo(long lMVideoId, long lDirectorId)
{
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    CStdString strSQL=FormatSQL("select * from directorlinkmusicvideo where idDirector=%u and idMVideo=%u", lDirectorId, lMVideoId);
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() == 0)
    {
      // doesnt exists, add it
      strSQL=FormatSQL("insert into directorlinkmusicvideo (idDirector, idMVideo) values( %i,%i)", lDirectorId, lMVideoId);
      m_pDS->exec(strSQL.c_str());
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::AddDirectorToMusicVideo() failed");
  }
}

void CVideoDatabase::AddStudioToMovie(long lMovieId, long lStudioId)
{
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    CStdString strSQL=FormatSQL("select * from studiolinkmovie where idStudio=%i and idMovie=%i", lStudioId, lMovieId);
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() == 0)
    {
      // doesnt exists, add it
      strSQL=FormatSQL("insert into studiolinkmovie (idStudio, idMovie) values( %i,%i)", lStudioId, lMovieId);
      m_pDS->exec(strSQL.c_str());
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::AddStudioToMovie() failed");
  }
}

void CVideoDatabase::AddStudioToMusicVideo(long lMVideoId, long lStudioId)
{
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    CStdString strSQL=FormatSQL("select * from studiolinkmusicvideo where idStudio=%i and idMVideo=%i", lStudioId, lMVideoId);
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() == 0)
    {
      // doesnt exists, add it
      strSQL=FormatSQL("insert into studiolinkmusicvideo (idStudio, idMVideo) values( %i,%i)", lStudioId, lMVideoId);
      m_pDS->exec(strSQL.c_str());
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::AddStudioToMusicVideo() failed");
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
    return lMovieId != -1;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::HasMovieInfo(%s) failed", strFilenameAndPath.c_str());
  }

  return false;
}

bool CVideoDatabase::HasTvShowInfo(const CStdString& strPath)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    long lTvShowId = GetTvShowInfo(strPath);
    if ( lTvShowId < 0) return false;
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::HasTvShowInfo(%s) failed", strPath.c_str());
  }

  return false;
}

bool CVideoDatabase::HasEpisodeInfo(const CStdString& strFilenameAndPath)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    long lEpisodeId = GetEpisodeInfo(strFilenameAndPath);
    if ( lEpisodeId < 0) return false;
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::HasEpisodeInfo(%s) failed", strFilenameAndPath.c_str());
  }

  return false;
}

bool CVideoDatabase::HasMusicVideoInfo(const CStdString& strFilenameAndPath)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    long lMVideoId = GetMusicVideoInfo(strFilenameAndPath);
    return lMVideoId != -1;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::HasMusicVideoInfo(%s) failed", strFilenameAndPath.c_str());
  }

  return false;
}

void CVideoDatabase::DeleteDetailsForTvShow(const CStdString& strPath)
{
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    long lTvShowId = GetTvShowInfo(strPath);
    if ( lTvShowId < 0) return ;

    CStdString strSQL;
    strSQL=FormatSQL("delete from genrelinktvshow where idshow=%i", lTvShowId);
    m_pDS->exec(strSQL.c_str());

    strSQL=FormatSQL("delete from actorlinktvshow where idshow=%i", lTvShowId);
    m_pDS->exec(strSQL.c_str());

    strSQL=FormatSQL("delete from directorlinktvshow where idshow=%i", lTvShowId);
    m_pDS->exec(strSQL.c_str());

    // remove all info other than the id
    // we do this due to the way we have the link between the file + movie tables.

    strSQL = "update tvshow set ";
    for (int iType = VIDEODB_ID_TV_MIN + 1; iType < VIDEODB_ID_TV_MAX; iType++)
    {
      CStdString column;
      column.Format("c%02d=NULL,", iType);
      strSQL += column;
    }
    strSQL = strSQL.Mid(0, strSQL.size() - 1) + FormatSQL(" where idShow=%i", lTvShowId);
    m_pDS->exec(strSQL.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::DeleteDetailsForTvShow(%s) failed", strPath.c_str());
  }
}

//********************************************************************************************************************************
void CVideoDatabase::GetMoviesByActor(const CStdString& strActor, VECMOVIES& movies)
{
  try
  {
    movies.erase(movies.begin(), movies.end());
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    CStdString strSQL=FormatSQL("select movie.*,files.strFileName,path.strPath from movie join files on files.idFile=movie.idFile join path on files.idPath=path.idPath join actorlinkmovie on actorlinkmovie.idmovie=movie.idmovie join actors on actors.idActor=actorlinkmovie.idActor where actors.stractor='%s'", strActor.c_str());
    m_pDS->query( strSQL.c_str() );

    long lLastPathId = -1;
    while (!m_pDS->eof())
    {
      movies.push_back(GetDetailsForMovie(m_pDS));
      m_pDS->next();
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetMoviesByActor(%s) failed", strActor.c_str());
  }
}

void CVideoDatabase::GetTvShowsByActor(const CStdString& strActor, VECMOVIES& movies)
{
  try
  {
    movies.erase(movies.begin(), movies.end());
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    CStdString strSQL=FormatSQL("select tvshow.*,path.strPath,path.strPath from tvshow join path on tvshowlinkpath.idpath = path.idpath join tvshowlinkpath on tvshowlinkpath.idshow=tvshow.idshow join actorlinktvshow on actorlinktvshow.idshow=tvshow.idshow join actors on actors.idActor=actorlinktvshow.idActor where actors.stractor='%s'", strActor.c_str());
    m_pDS->query( strSQL.c_str() );

    long lLastPathId = -1;
    while (!m_pDS->eof())
    {
      movies.push_back(GetDetailsForTvShow(m_pDS));
      m_pDS->next();
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetTvShowsByActor(%s) failed", strActor.c_str());
  }
}

void CVideoDatabase::GetEpisodesByActor(const CStdString& strActor, VECMOVIES& movies)
{
  try
  {
    movies.erase(movies.begin(), movies.end());
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    CStdString strSQL=FormatSQL("select episode.*,files.strFileName,path.strPath,tvshow.c%02d from episode join files on files.idFile=episode.idfile join tvshowlinkepisode on tvshowlinkepisode.idepisode=episode.idepisode join tvshow on tvshowlinkepisode.idshow=tvshow.idshow join path on files.idPath=path.idPath join actorlinkepisode on actorlinkepisode.idepisode=episode.idepisode join actors on actors.idActor=actorlinkepisode.idActor where actors.stractor='%s'", VIDEODB_ID_TV_TITLE,strActor.c_str());
    m_pDS->query( strSQL.c_str() );

    long lLastPathId = -1;
    while (!m_pDS->eof())
    {
      CVideoInfoTag movie=GetDetailsForEpisode(m_pDS);
      movie.m_strTitle += " ("+m_pDS->fv(VIDEODB_DETAILS_PATH+1).get_asString()+")";
      movies.push_back(movie);
      m_pDS->next();
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetEpisodesByActor(%s) failed", strActor.c_str());
  }
}

void CVideoDatabase::GetMusicVideosByArtist(const CStdString& strArtist, CFileItemList& items)
{
  try
  {
    items.Clear();
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;
    
    CStdString strSQL;
    if (strArtist.IsEmpty())
      strSQL=FormatSQL("select distinct musicvideo.*,files.strFileName,path.strPath from musicvideo join files on files.idFile=musicvideo.idFile join path on files.idPath=path.idPath join artistlinkmusicvideo on artistlinkmusicvideo.idmvideo=musicvideo.idmvideo join actors on actors.idActor=artistlinkmusicvideo.idArtist");
    else
      strSQL=FormatSQL("select musicvideo.*,files.strFileName,path.strPath from musicvideo join files on files.idFile=musicvideo.idFile join path on files.idPath=path.idPath join artistlinkmusicvideo on artistlinkmusicvideo.idmvideo=musicvideo.idmvideo join actors on actors.idActor=artistlinkmusicvideo.idArtist where actors.stractor='%s'", strArtist.c_str());
    m_pDS->query( strSQL.c_str() );

    long lLastPathId = -1;
    while (!m_pDS->eof())
    {
      CVideoInfoTag tag = GetDetailsForMusicVideo(m_pDS);
      CFileItem* pItem = new CFileItem(tag);
      pItem->SetLabel(tag.GetArtist());
      items.Add(pItem);
      m_pDS->next();
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetMusicVideosByArtist(%s) failed", strArtist.c_str());
  }
}

//********************************************************************************************************************************
void CVideoDatabase::GetMovieInfo(const CStdString& strFilenameAndPath, CVideoInfoTag& details, long lMovieId /* = -1 */)
{
  try
  {
    // TODO: Optimize this - no need for all the queries!
    if (lMovieId < 0)
      lMovieId = GetMovieInfo(strFilenameAndPath);
    if (lMovieId < 0) return ;

    CStdString sql = FormatSQL("select movie.*,files.strFileName,path.strPath from movie join files on files.idFile = movie.idfile join path on files.idPath=path.idPath where movie.idMovie=%i", lMovieId);
    if (!m_pDS->query(sql.c_str()))
      return;
    details = GetDetailsForMovie(m_pDS, true);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetMovieInfo(%s) failed", strFilenameAndPath.c_str());
  }
}

//********************************************************************************************************************************
void CVideoDatabase::GetTvShowInfo(const CStdString& strPath, CVideoInfoTag& details, long lTvShowId /* = -1 */)
{
  try
  {
    if (lTvShowId < 0)
      lTvShowId = GetTvShowInfo(strPath);
    if (lTvShowId < 0) return ;

    CStdString sql = FormatSQL("select tvshow.*,path.strPath,path.strPath from tvshow,tvshowlinkpath,path where tvshow.idshow=%i and tvshowlinkpath.idshow=tvshow.idshow and tvshowlinkpath.idpath=path.idpath", lTvShowId);
    if (!m_pDS->query(sql.c_str()))
      return;
    details = GetDetailsForTvShow(m_pDS, true);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetTvShowInfo(%s) failed", strPath.c_str());
  }
}

bool CVideoDatabase::GetEpisodeInfo(const CStdString& strFilenameAndPath, CVideoInfoTag& details, long lEpisodeId /* = -1 */)
{
  try
  {
    // TODO: Optimize this - no need for all the queries!
    if (lEpisodeId < 0)
      lEpisodeId = GetEpisodeInfo(strFilenameAndPath);
    if (lEpisodeId < 0) return false;

    CStdString sql = FormatSQL("select episode.*,files.strFileName,path.strPath,tvshow.c%02d from episode join files on files.idFile=episode.idFile join tvshowlinkepisode on episode.idepisode=tvshowlinkepisode.idepisode join tvshow on tvshow.idshow=tvshowlinkepisode.idshow join path on files.idPath=path.idPath where episode.idepisode=%u",VIDEODB_ID_TV_TITLE,lEpisodeId);
    if (!m_pDS->query(sql.c_str()))
      return false;
    details = GetDetailsForEpisode(m_pDS, true);
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetEpisodeInfo(%s) failed", strFilenameAndPath.c_str());
  }
  return false;
}

void CVideoDatabase::GetMusicVideoInfo(const CStdString& strFilenameAndPath, CVideoInfoTag& details, long lMVideoId /* = -1 */)
{
  try
  {
    // TODO: Optimize this - no need for all the queries!
    if (lMVideoId < 0)
      lMVideoId = GetMusicVideoInfo(strFilenameAndPath);
    if (lMVideoId < 0) return ;

    CStdString sql = FormatSQL("select musicvideo.*,files.strFileName,path.strPath from musicvideo join files on files.idFile = musicvideo.idfile join path on files.idPath=path.idPath where musicvideo.idmvideo=%i", lMVideoId);
    if (!m_pDS->query(sql.c_str()))
      return;
    details = GetDetailsForMusicVideo(m_pDS);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetMusicVideoInfo(%s) failed", strFilenameAndPath.c_str());
  }
}

void CVideoDatabase::AddGenreAndDirectorsAndStudios(const CVideoInfoTag& details, vector<long>& vecDirectors, vector<long>& vecGenres, vector<long>& vecStudios)
{
  // add all directors
  char szDirector[1024];
  strcpy(szDirector, details.m_strDirector.c_str());
  if (strstr(szDirector, "/"))
  {
    char *pToken = strtok(szDirector, "/");
    while ( pToken != NULL )
    {
      CStdString strDirector = pToken;
      strDirector.Trim();
      long lDirectorId = AddActor(strDirector,"");
      vecDirectors.push_back(lDirectorId);
      pToken = strtok( NULL, "/" );
    }
  }
  else if (!details.m_strDirector.IsEmpty())
  {
    CStdString strDirector = details.m_strDirector;
    strDirector.Trim();
    long lDirectorId = AddActor(strDirector,"");
    vecDirectors.push_back(lDirectorId);
  }

  // add all genres
  char szGenres[1024];
  strcpy(szGenres, details.m_strGenre.c_str());
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
  else if (!details.m_strGenre.IsEmpty())
  {
    CStdString strGenre = details.m_strGenre;
    strGenre.Trim();
    long lGenreId = AddGenre(strGenre);
    vecGenres.push_back(lGenreId);
  }
    // add all studios
  char szStudios[1024];
  strcpy(szStudios, details.m_strStudio.c_str());
  if (strstr(szStudios, "/"))
  {
    char *pToken = strtok(szStudios, "/");
    while ( pToken != NULL )
    {
      CStdString strStudio = pToken;
      strStudio.Trim();
      long lStudioId = AddStudio(strStudio);
      vecStudios.push_back(lStudioId);
      pToken = strtok( NULL, "/" );
    }
  }
  else if (!details.m_strStudio.IsEmpty())
  {
    CStdString strStudio = details.m_strStudio;
    strStudio.Trim();
    long lStudioId = AddStudio(strStudio);
    vecStudios.push_back(lStudioId);
  }
}

//********************************************************************************************************************************
void CVideoDatabase::SetDetailsForMovie(const CStdString& strFilenameAndPath, const CVideoInfoTag& details)
{
  try
  {
    long lFileId = GetFile(strFilenameAndPath);
    if (lFileId < 0)
      lFileId = AddFile(strFilenameAndPath);
    long lMovieId = GetMovieInfo(strFilenameAndPath);
    if (lMovieId > -1)
    {
      DeleteMovie(strFilenameAndPath);
    }
    lMovieId = AddMovie(strFilenameAndPath);
    if (lMovieId < 0)
      return;

//    BeginTransaction();

    vector<long> vecDirectors;
    vector<long> vecGenres;
    vector<long> vecStudios;
    AddGenreAndDirectorsAndStudios(details,vecDirectors,vecGenres,vecStudios);
    
    // add cast...
    for (CVideoInfoTag::iCast it = details.m_cast.begin(); it != details.m_cast.end(); ++it)
    {
      long lActor = AddActor(it->strName,it->thumbUrl.m_xml);
      AddActorToMovie(lMovieId, lActor, it->strRole);
    }
    
    for (unsigned int i = 0; i < vecGenres.size(); ++i)
    {
      AddGenreToMovie(lMovieId, vecGenres[i]);
    }

    for (unsigned int i = 0; i < vecDirectors.size(); ++i)
    {
      AddDirectorToMovie(lMovieId, vecDirectors[i]);
    }
    
    for (unsigned int i = 0; i < vecStudios.size(); ++i)
    {
      AddStudioToMovie(lMovieId, vecStudios[i]);
    }

    // update our movie table (we know it was added already above)
    // and insert the new row
    CStdString sql = "update movie set ";
    for (int iType=VIDEODB_ID_MIN+1;iType<VIDEODB_ID_MAX;++iType)
    {
      CStdString strValue;
      switch (DbMovieOffsets[iType].type)
      {
      case VIDEODB_TYPE_STRING:
        strValue = *(CStdString*)(((char*)&details)+DbMovieOffsets[iType].offset);
        break;
      case VIDEODB_TYPE_INT:
        strValue.Format("%i",*(int*)(((char*)&details)+DbMovieOffsets[iType].offset));
        break;
      case VIDEODB_TYPE_BOOL:
        strValue = *(bool*)(((char*)&details)+DbMovieOffsets[iType].offset)?"true":"false";
        break;
      case VIDEODB_TYPE_FLOAT:
        strValue.Format("%f",*(float*)(((char*)&details)+DbMovieOffsets[iType].offset));
        break;
      }
      sql += FormatSQL("c%02d='%s',", iType, strValue.c_str());
    }
    sql.TrimRight(',');
    sql += FormatSQL(" where idMovie=%u", lMovieId);
    m_pDS->exec(sql.c_str());
//    CommitTransaction();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::SetDetailsForMovie(%s) failed", strFilenameAndPath.c_str());
  }
}

long CVideoDatabase::SetDetailsForTvShow(const CStdString& strPath, const CVideoInfoTag& details)
{
  try
  {
    long lTvShowId = GetTvShowInfo(strPath);
    if (lTvShowId < 0)
      lTvShowId = AddTvShow(strPath);

//    BeginTransaction();

    vector<long> vecDirectors;
    vector<long> vecGenres;
    vector<long> vecStudios;
    AddGenreAndDirectorsAndStudios(details,vecDirectors,vecGenres,vecStudios);
  
    // add cast...
    for (CVideoInfoTag::iCast it = details.m_cast.begin(); it != details.m_cast.end(); ++it)
    {
      long lActor = AddActor(it->strName,it->thumbUrl.m_xml);
      AddActorToTvShow(lTvShowId, lActor, it->strRole);
    }
    
    for (unsigned int i = 0; i < vecGenres.size(); ++i)
    {
      AddGenreToTvShow(lTvShowId, vecGenres[i]);
    }

    for (unsigned int i = 0; i < vecDirectors.size(); ++i)
    {
      AddDirectorToTvShow(lTvShowId, vecDirectors[i]);
    }

    // and insert the new row
    CStdString sql = "update tvshow set ";
    for (int iType=VIDEODB_ID_MIN+1;iType<VIDEODB_ID_MAX;++iType)
    {
      CStdString strValue;
      switch (DbTvShowOffsets[iType].type)
      {
      case VIDEODB_TYPE_STRING:
        strValue = *(CStdString*)(((char*)&details)+DbTvShowOffsets[iType].offset);
        break;
      case VIDEODB_TYPE_INT:
        strValue.Format("%i",*(int*)(((char*)&details)+DbTvShowOffsets[iType].offset));
        break;
      case VIDEODB_TYPE_BOOL:
        strValue = *(bool*)(((char*)&details)+DbTvShowOffsets[iType].offset)?"true":"false";
        break;
      case VIDEODB_TYPE_FLOAT:
        strValue.Format("%f",*(float*)(((char*)&details)+DbTvShowOffsets[iType].offset));
        break;
      }
      sql += FormatSQL("c%02d='%s',", iType, strValue.c_str());
    }
    sql.TrimRight(',');
    sql += FormatSQL("where idShow=%u", lTvShowId);
    m_pDS->exec(sql.c_str());
    sql = FormatSQL("update tvshow set c%02d=(select count(idEpisode) from tvshowlinkepisode where idshow=%u) where idshow=%u",VIDEODB_ID_TV_EPISODES,lTvShowId,lTvShowId);
    m_pDS->exec(sql.c_str());
    // update tvshowlinkpath info to reflect it points to this tvshow
    long lPathId = GetPath(strPath);
    if (lPathId < 0)
      lPathId = AddPath(strPath);
//    CommitTransaction();
    return lTvShowId;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::SetDetailsForTvShow(%s) failed", strPath.c_str());
  }

  return -1;
}

long CVideoDatabase::SetDetailsForEpisode(const CStdString& strFilenameAndPath, const CVideoInfoTag& details, long idShow, long lEpisodeId)
{
  try
  {
    long lFileId = GetFile(strFilenameAndPath);
    if (lEpisodeId == -1)
    {
      lEpisodeId = GetEpisodeInfo(strFilenameAndPath);
      if (lEpisodeId > 0)
        DeleteEpisode(strFilenameAndPath,lEpisodeId);

      lEpisodeId = AddEpisode(idShow,strFilenameAndPath);
      if (lEpisodeId < 0)
        return -1;
    }

    vector<long> vecDirectors;
    vector<long> vecGenres;
    vector<long> vecStudios;
    AddGenreAndDirectorsAndStudios(details,vecDirectors,vecGenres,vecStudios);
    
    // add cast...
    for (CVideoInfoTag::iCast it = details.m_cast.begin(); it != details.m_cast.end(); ++it)
    {
      long lActor = AddActor(it->strName,it->thumbUrl.m_xml);
      AddActorToEpisode(lEpisodeId, lActor, it->strRole);
    }
    
    for (unsigned int i = 0; i < vecGenres.size(); ++i)
    {
      AddGenreToEpisode(lEpisodeId, vecGenres[i]);
    }

    for (unsigned int i = 0; i < vecDirectors.size(); ++i)
    {
      AddDirectorToEpisode(lEpisodeId, vecDirectors[i]);
    }

    // and insert the new row
    CStdString sql = "update episode set ";
    for (int iType=VIDEODB_ID_EPISODE_MIN+1;iType<VIDEODB_ID_EPISODE_MAX;++iType)
    {
      CStdString strValue;
      switch (DbEpisodeOffsets[iType].type)
      {
      case VIDEODB_TYPE_STRING:
        strValue = *(CStdString*)(((char*)&details)+DbEpisodeOffsets[iType].offset);
        break;
      case VIDEODB_TYPE_INT:
        strValue.Format("%i",*(int*)(((char*)&details)+DbEpisodeOffsets[iType].offset));
        break;
      case VIDEODB_TYPE_BOOL:
        strValue = *(bool*)(((char*)&details)+DbEpisodeOffsets[iType].offset)?"true":"false";
        break;
      case VIDEODB_TYPE_FLOAT:
        strValue.Format("%f",*(float*)(((char*)&details)+DbEpisodeOffsets[iType].offset));
        break;
      }
      sql += FormatSQL("c%02d='%s',", iType, strValue.c_str());
    }
    sql.TrimRight(',');
    sql += FormatSQL("where idEpisode=%u", lEpisodeId);
    m_pDS->exec(sql.c_str());
    return lEpisodeId;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::SetDetailsForEpisode(%s) failed", strFilenameAndPath.c_str());
  }
  return -1;
}

void CVideoDatabase::SetDetailsForMusicVideo(const CStdString& strFilenameAndPath, const CVideoInfoTag& details)
{
  try
  {
    long lFileId = GetFile(strFilenameAndPath);
    if (lFileId < 0)
      lFileId = AddFile(strFilenameAndPath);
    long lMVideoId = GetMusicVideoInfo(strFilenameAndPath);
    if (lMVideoId > -1)
    {
      DeleteMusicVideo(strFilenameAndPath);
    }
    lMVideoId = AddMusicVideo(strFilenameAndPath);
    if (lMVideoId < 0)
      return;

//    BeginTransaction();

    vector<long> vecDirectors;
    vector<long> vecGenres;
    vector<long> vecStudios;
    AddGenreAndDirectorsAndStudios(details,vecDirectors,vecGenres,vecStudios);
    
    // add artists...
    for (vector<CStdString>::const_iterator it = details.m_artist.begin(); it != details.m_artist.end(); ++it)
    {
      long lArtist = AddActor(*it,"");
      AddArtistToMusicVideo(lMVideoId, lArtist);
    }
    
    for (unsigned int i = 0; i < vecGenres.size(); ++i)
    {
      AddGenreToMusicVideo(lMVideoId, vecGenres[i]);
    }

    for (unsigned int i = 0; i < vecDirectors.size(); ++i)
    {
      AddDirectorToMusicVideo(lMVideoId, vecDirectors[i]);
    }
    
    for (unsigned int i = 0; i < vecStudios.size(); ++i)
    {
      AddStudioToMusicVideo(lMVideoId, vecStudios[i]);
    }

    // update our movie table (we know it was added already above)
    // and insert the new row
    CStdString sql = "update musicvideo set ";
    for (int iType=VIDEODB_ID_MUSICVIDEO_MIN+1;iType<VIDEODB_ID_MUSICVIDEO_MAX;++iType)
    {
      CStdString strValue;
      switch (DbMusicVideoOffsets[iType].type)
      {
      case VIDEODB_TYPE_STRING:
        strValue = *(CStdString*)(((char*)&details)+DbMusicVideoOffsets[iType].offset);
        break;
      case VIDEODB_TYPE_INT:
        strValue.Format("%i",*(int*)(((char*)&details)+DbMusicVideoOffsets[iType].offset));
        break;
      case VIDEODB_TYPE_BOOL:
        strValue = *(bool*)(((char*)&details)+DbMusicVideoOffsets[iType].offset)?"true":"false";
        break;
      case VIDEODB_TYPE_FLOAT:
        strValue.Format("%f",*(float*)(((char*)&details)+DbMusicVideoOffsets[iType].offset));
        break;
      }
      sql += FormatSQL("c%02d='%s',", iType, strValue.c_str());
    }
    sql.TrimRight(',');
    sql += FormatSQL(" where idMVideo=%u", lMVideoId);
    m_pDS->exec(sql.c_str());
//    CommitTransaction();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::SetDetailsForMusicVideo(%s) failed", strFilenameAndPath.c_str());
  }
}

//********************************************************************************************************************************
void CVideoDatabase::GetFilePath(long lMovieId, CStdString &filePath, int iType)
{
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    if (lMovieId < 0) return ;
    
    CStdString strSQL;
    if (iType == 0)
      strSQL=FormatSQL("select path.strPath,files.strFileName from path, files, movie where path.idPath=files.idPath and files.idFile=movie.idFile and movie.idMovie=%u order by strFilename", lMovieId );
    if (iType == 1)
      strSQL=FormatSQL("select path.strPath,files.strFileName from path, files, episode where path.idPath=files.idPath and files.idFile=episode.idFile and episode.idEpisode=%u order by strFilename", lMovieId );
    if (iType == 2)
      strSQL=FormatSQL("select path.strPath from path,tvshowlinkpath where path.idpath=tvshowlinkpath.idpath and tvshowlinkpath.idshow=%i", lMovieId );
    if (iType ==3)
      strSQL=FormatSQL("select path.strPath,files.strFileName from path, files, musicvideo where path.idPath=files.idPath and files.idFile=musicvideo.idFile and musicvideo.idmvideo=%u order by strFilename", lMovieId );
    
    m_pDS->query( strSQL.c_str() );
    if (!m_pDS->eof())
    {
      if (iType != 2)
        CUtil::AddFileToFolder(m_pDS->fv("path.strPath").get_asString(),m_pDS->fv("files.strFilename").get_asString(),filePath);
      else
        filePath = m_pDS->fv("path.strPath").get_asString();
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetFilePath() failed");
  }
}

//********************************************************************************************************************************
void CVideoDatabase::GetBookMarksForFile(const CStdString& strFilenameAndPath, VECBOOKMARKS& bookmarks, CBookmark::EType type /*= CBookmark::STANDARD*/, bool bAppend)
{
  try
  {
    long lFileId = GetFile(strFilenameAndPath);
    if (lFileId < 0) return ;
    if (!bAppend)
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
      if (type == CBookmark::EPISODE)
      {
        CStdString strSQL2=FormatSQL("select c%02d, c%02d from episode where c%02d=%i order by c%02d, c%02d", VIDEODB_ID_EPISODE_EPISODE, VIDEODB_ID_EPISODE_SEASON, VIDEODB_ID_EPISODE_BOOKMARK, m_pDS->fv("idBookmark").get_asInteger(), VIDEODB_ID_EPISODE_SORTSEASON, VIDEODB_ID_EPISODE_SORTEPISODE);
        m_pDS2->query(strSQL2.c_str());
        bookmark.episodeNumber = m_pDS2->fv(0).get_asLong();
        bookmark.seasonNumber = m_pDS2->fv(1).get_asLong();
        m_pDS2->close();
      }
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
  if (bookmarks.size() > 0)
  {
    bookmark = bookmarks[0];
    return true;
  }
  return false;
}

void CVideoDatabase::GetEpisodesByFile(const CStdString& strFilenameAndPath, vector<CVideoInfoTag>& episodes)
{
  try
  {
    CStdString strSQL = FormatSQL("select episode.*,files.strFileName,path.strPath,tvshow.c%02d from episode join files on files.idFile=episode.idFile join tvshowlinkepisode on episode.idepisode=tvshowlinkepisode.idepisode join tvshow on tvshow.idshow=tvshowlinkepisode.idshow join path on files.idPath=path.idPath where files.idFile=%i order by episode.c%02d, episode.c%02d asc", VIDEODB_ID_TV_TITLE, GetFile(strFilenameAndPath), VIDEODB_ID_EPISODE_SORTSEASON, VIDEODB_ID_EPISODE_SORTEPISODE);
    m_pDS->query(strSQL.c_str());
    while (!m_pDS->eof())
    {
      episodes.push_back(GetDetailsForEpisode(m_pDS));
      m_pDS->next();
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetEpisodesByFile(%s) failed", strFilenameAndPath.c_str());
  }
}

//********************************************************************************************************************************
void CVideoDatabase::AddBookMarkToFile(const CStdString& strFilenameAndPath, const CBookmark &bookmark, CBookmark::EType type /*= CBookmark::STANDARD*/)
{
  try
  {
    long lFileId = GetFile(strFilenameAndPath);
    if (lFileId < 0)
    {
      // Doesn't exist in the database yet - add it.
      // TODO: It doesn't appear to me that the CDLabel parameter or the subtitles
      // parameter is anywhere in use in XBMC.
      lFileId = AddFile(strFilenameAndPath);
      if (lFileId < 0)
        return;
    }
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    CStdString strSQL;
    int idBookmark=-1;
    if (type == CBookmark::RESUME) // get the same resume mark bookmark each time type
    {
      strSQL=FormatSQL("select idBookmark from bookmark where idFile=%i and type=1", lFileId);   
    }
    else if (type == CBookmark::STANDARD) // get the same bookmark again, and update. not sure here as a dvd can have same time in multiple places, state will differ thou
    {
      /* get a bookmark within the same time as previous */
      double mintime = bookmark.timeInSeconds - 0.5f;
      double maxtime = bookmark.timeInSeconds + 0.5f;
      strSQL=FormatSQL("select idBookmark from bookmark where idFile=%i and type=%i and (timeInSeconds between %f and %f) and playerState='%s'", lFileId, (int)type, mintime, maxtime, bookmark.playerState.c_str());
    }

    if (type != CBookmark::EPISODE)
    {
      // get current id
      m_pDS->query( strSQL.c_str() );
      if (m_pDS->num_rows() != 0)
        idBookmark = m_pDS->get_field_value("idBookmark").get_asInteger();            
      m_pDS->close();
    }
    // update or insert depending if it existed before
    if (idBookmark >= 0 )
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
    long lFileId = GetFile(strFilenameAndPath);
    if (lFileId < 0) return ;
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    /* a litle bit uggly, we clear first bookmark that is within one second of given */
    /* should be no problem since we never add bookmarks that are closer than that   */
    double mintime = bookmark.timeInSeconds - 0.5f;
    double maxtime = bookmark.timeInSeconds + 0.5f;
    CStdString strSQL = FormatSQL("select idBookmark from bookmark where idFile=%i and type=%i and playerState like '%s' and player like '%s' and (timeInSeconds between %f and %f)", lFileId, type, bookmark.playerState.c_str(), bookmark.player.c_str(), mintime, maxtime);

    m_pDS->query( strSQL.c_str() );
    if (m_pDS->num_rows() != 0)
    {
      int idBookmark = m_pDS->get_field_value("idBookmark").get_asInteger();
      strSQL=FormatSQL("delete from bookmark where idBookmark=%i",idBookmark);
      m_pDS->exec(strSQL.c_str());
      if (type == CBookmark::EPISODE)
      {
        strSQL=FormatSQL("update episode set c%02d=-1 where idFile=%i and c%02d=%i", VIDEODB_ID_EPISODE_BOOKMARK, lFileId, VIDEODB_ID_EPISODE_BOOKMARK, idBookmark);
        m_pDS->exec(strSQL.c_str());
      }
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
    long lFileId = GetFile(strFilenameAndPath);
    if (lFileId < 0) return ;
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    CStdString strSQL=FormatSQL("delete from bookmark where idFile=%i and type=%i", lFileId, (int)type);
    m_pDS->exec(strSQL.c_str());
    if (type == CBookmark::EPISODE)
    {
      strSQL=FormatSQL("update episode set c%02d=-1 where idFile=%i", VIDEODB_ID_EPISODE_BOOKMARK, lFileId);
      m_pDS->exec(strSQL.c_str());
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::ClearBookMarksOfMovie(%s) failed", strFilenameAndPath.c_str());
  }
}


bool CVideoDatabase::GetBookMarkForEpisode(const CVideoInfoTag& tag, CBookmark& bookmark)
{
  try
  {
    CStdString strSQL = FormatSQL("select bookmark.* from bookmark join episode on episode.c%02d=bookmark.idBookmark where episode.idEpisode=%i", VIDEODB_ID_EPISODE_BOOKMARK, tag.m_iDbId);
    m_pDS->query( strSQL.c_str() );
    if (!m_pDS->eof())
    {
      bookmark.timeInSeconds = m_pDS->fv("timeInSeconds").get_asDouble();
      bookmark.thumbNailImage = m_pDS->fv("thumbNailImage").get_asString();
      bookmark.playerState = m_pDS->fv("playerState").get_asString();
      bookmark.player = m_pDS->fv("player").get_asString();
      bookmark.type = (CBookmark::EType)m_pDS->fv("type").get_asInteger();
    }
    else 
    {
      m_pDS->close();
      return false;
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetBookMarkForEpisode failed!");
    return false;
  }
  return true;
}

void CVideoDatabase::AddBookMarkForEpisode(const CVideoInfoTag& tag, const CBookmark& bookmark)
{
  try
  {
    long lFileId = GetFile(tag.m_strFileNameAndPath);
    // delete the current episode for the selected episode number
    CStdString strSQL = FormatSQL("delete from bookmark where idBookmark in (select c%02d from episode where c%02d=%i and c%02d=%i and idFile=%i)", VIDEODB_ID_EPISODE_BOOKMARK, VIDEODB_ID_EPISODE_SEASON, tag.m_iSeason, VIDEODB_ID_EPISODE_EPISODE, tag.m_iEpisode, lFileId);
    m_pDS->exec(strSQL.c_str());

    AddBookMarkToFile(tag.m_strFileNameAndPath, bookmark, CBookmark::EPISODE);
    long lBookmarkId = (long)sqlite3_last_insert_rowid(m_pDB->getHandle());
    strSQL = FormatSQL("update episode set c%02d=%i where c%02d=%i and c%02d=%i and idFile=%i", VIDEODB_ID_EPISODE_BOOKMARK, lBookmarkId, VIDEODB_ID_EPISODE_SEASON, tag.m_iSeason, VIDEODB_ID_EPISODE_EPISODE, tag.m_iEpisode, lFileId);
    m_pDS->exec(strSQL.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::AddBookMarkForEpisode(%i) failed", tag.m_iDbId);
  }
}

void CVideoDatabase::DeleteBookMarkForEpisode(const CVideoInfoTag& tag)
{
  try
  {
    CStdString strSQL = FormatSQL("delete from bookmark where idBookmark in (select c%02d from episode where idEpisode=%i)", VIDEODB_ID_EPISODE_BOOKMARK, tag.m_iDbId);
    m_pDS->exec(strSQL.c_str());
    strSQL = FormatSQL("update episode set c%02d=-1 where idEpisode=%i", VIDEODB_ID_EPISODE_BOOKMARK, tag.m_iDbId);
    m_pDS->exec(strSQL.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::DeleteBookMarkForEpisode(%i) failed", tag.m_iDbId);
  }
}

//********************************************************************************************************************************
void CVideoDatabase::DeleteMovie(const CStdString& strFilenameAndPath)
{
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;
    long lMovieId = GetMovieInfo(strFilenameAndPath);
    if (lMovieId < 0)
    {
      return ;
    }

    ClearBookMarksOfFile(strFilenameAndPath);

    CStdString strSQL;
    strSQL=FormatSQL("delete from genrelinkmovie where idmovie=%i", lMovieId);
    m_pDS->exec(strSQL.c_str());

    strSQL=FormatSQL("delete from actorlinkmovie where idmovie=%i", lMovieId);
    m_pDS->exec(strSQL.c_str());

    strSQL=FormatSQL("delete from directorlinkmovie where idmovie=%i", lMovieId);
    m_pDS->exec(strSQL.c_str());

    strSQL=FormatSQL("delete from studiolinkmovie where idmovie=%i", lMovieId);
    m_pDS->exec(strSQL.c_str());

    strSQL=FormatSQL("delete from movie where idmovie=%i", lMovieId);
    m_pDS->exec(strSQL.c_str());

    strSQL=FormatSQL("delete from movielinktvshow where idmovie=%i", lMovieId);
    m_pDS->exec(strSQL.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::DeleteMovie() failed");
  }
}

void CVideoDatabase::DeleteTvShow(const CStdString& strPath)
{
  try
  {
    long lTvShowId=-1;
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;
    lTvShowId = GetTvShowInfo(strPath);
    if (lTvShowId < 0)
    {
      return ;
    }

    BeginTransaction();
    CStdString strSQL=FormatSQL("select tvshowlinkepisode.idepisode,path.strPath,files.strFileName from tvshowlinkepisode,path,files,episode where tvshowlinkepisode.idshow=%u and tvshowlinkepisode.idepisode=episode.idEpisode and episode.idFile=files.idFile and files.idPath=path.idPath",lTvShowId);
    m_pDS2->query(strSQL.c_str());
    while (!m_pDS2->eof())
    {
      CStdString strPath;
      CUtil::AddFileToFolder(m_pDS2->fv("path.strPath").get_asString(),m_pDS2->fv("files.strFilename").get_asString(),strPath);
      DeleteEpisode(strPath,m_pDS2->fv(0).get_asLong());
      m_pDS2->next();
    }

    strSQL=FormatSQL("delete from genrelinktvshow where idshow=%i", lTvShowId);
    m_pDS->exec(strSQL.c_str());

    strSQL=FormatSQL("delete from actorlinktvshow where idshow=%i", lTvShowId);
    m_pDS->exec(strSQL.c_str());

    strSQL=FormatSQL("delete from directorlinktvshow where idshow=%i", lTvShowId);
    m_pDS->exec(strSQL.c_str());

    strSQL=FormatSQL("delete from tvshowlinkpath where idshow=%u", lTvShowId);
    m_pDS->exec(strSQL.c_str());

    strSQL=FormatSQL("delete from tvshow where idshow=%i", lTvShowId);
    m_pDS->exec(strSQL.c_str());

    strSQL=FormatSQL("delete from movielinktvshow where idshow=%i", lTvShowId);
    m_pDS->exec(strSQL.c_str());

    CommitTransaction();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::DeleteTvShow() failed");
  }
}

void CVideoDatabase::DeleteEpisode(const CStdString& strFilenameAndPath, long lEpisodeId)
{
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;
    if (lEpisodeId < 0)
    {
      lEpisodeId = GetEpisodeInfo(strFilenameAndPath);
      if (lEpisodeId < 0)
      {
        return ;
      }
    }

    ClearBookMarksOfFile(strFilenameAndPath);

    CStdString strSQL;
    strSQL=FormatSQL("delete from genrelinkepisode where idepisode=%i", lEpisodeId);
    m_pDS->exec(strSQL.c_str());

    strSQL=FormatSQL("delete from actorlinkepisode where idepisode=%i", lEpisodeId);
    m_pDS->exec(strSQL.c_str());

    strSQL=FormatSQL("delete from directorlinkepisode where idepisode=%i", lEpisodeId);
    m_pDS->exec(strSQL.c_str());

    strSQL=FormatSQL("select tvshowlinkepisode.idshow from tvshowlinkepisode where idepisode=%u",lEpisodeId);
    m_pDS->query(strSQL.c_str());

    long idShow = m_pDS->fv(0).get_asLong();
    strSQL=FormatSQL("delete from tvshowlinkepisode where idepisode=%i", lEpisodeId);
    m_pDS->exec(strSQL.c_str());

    strSQL=FormatSQL("update tvshow set c%02d=(select count(idEpisode) from tvshowlinkepisode where idShow=%u) where idshow=%u",VIDEODB_ID_TV_EPISODES,idShow, idShow);
    m_pDS->exec(strSQL);

    strSQL=FormatSQL("delete from episode where idepisode=%i", lEpisodeId);
    m_pDS->exec(strSQL.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::DeleteEpisode() failed");
  }
}

void CVideoDatabase::DeleteMusicVideo(const CStdString& strFilenameAndPath)
{
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;
    long lMVideoId = GetMusicVideoInfo(strFilenameAndPath);
    if (lMVideoId < 0)
    {
      return ;
    }

    ClearBookMarksOfFile(strFilenameAndPath);

    CStdString strSQL;
    strSQL=FormatSQL("delete from genrelinkmusicvideo where idmvideo=%i", lMVideoId);
    m_pDS->exec(strSQL.c_str());

    strSQL=FormatSQL("delete from artistlinkmusicvideo where idmvideo=%i", lMVideoId);
    m_pDS->exec(strSQL.c_str());

    strSQL=FormatSQL("delete from directorlinkmusicvideo where idmvideo=%i", lMVideoId);
    m_pDS->exec(strSQL.c_str());

    strSQL=FormatSQL("delete from studiolinkmusicvideo where idmvideo=%i", lMVideoId);
    m_pDS->exec(strSQL.c_str());

    strSQL=FormatSQL("delete from musicvideo where idmvideo=%i", lMVideoId);
    m_pDS->exec(strSQL.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::DeleteMovie() failed");
  }
}

DWORD movieTime = 0;
DWORD castTime = 0;

CVideoInfoTag CVideoDatabase::GetDetailsForMovie(auto_ptr<Dataset> &pDS, bool needsCast /* = false */)
{
  CVideoInfoTag details;
  details.Reset();

  DWORD time = timeGetTime();
  long lMovieId = pDS->fv(0).get_asLong();

  for (int iType = VIDEODB_ID_MIN + 1; iType < VIDEODB_ID_MAX; iType++)
  {
    switch (DbMovieOffsets[iType].type)
    {
    case VIDEODB_TYPE_STRING:
      *(CStdString*)(((char*)&details)+DbMovieOffsets[iType].offset) = pDS->fv(iType+1).get_asString();
      break;
    case VIDEODB_TYPE_INT:
      *(int*)(((char*)&details)+DbMovieOffsets[iType].offset) = pDS->fv(iType+1).get_asInteger();
      break;
    case VIDEODB_TYPE_BOOL:
      *(bool*)(((char*)&details)+DbMovieOffsets[iType].offset) = pDS->fv(iType+1).get_asBool();
      break;
    case VIDEODB_TYPE_FLOAT:
      *(float*)(((char*)&details)+DbMovieOffsets[iType].offset) = pDS->fv(iType+1).get_asFloat();
      break;
    }
  }
  details.m_iDbId = lMovieId;

  details.m_strPath = pDS->fv(VIDEODB_DETAILS_PATH).get_asString();
  CUtil::AddFileToFolder(details.m_strPath, pDS->fv(VIDEODB_DETAILS_FILE).get_asString(),details.m_strFileNameAndPath);
  movieTime += timeGetTime() - time; time = timeGetTime();

  if (needsCast)
  {
    // create cast string
    CStdString strSQL = FormatSQL("select  actors.strActor,actorlinkmovie.strRole,actors.strThumb from  actorlinkmovie,actors where actorlinkmovie.idMovie=%u and actorlinkmovie.idActor  = actors.idActor order by actorlinkmovie.ROWID",lMovieId);
    m_pDS2->query(strSQL.c_str());
    while (!m_pDS2->eof())
    {
      SActorInfo info;
      info.strName = m_pDS2->fv("actors.strActor").get_asString();
      info.strRole = m_pDS2->fv("actorlinkmovie.strRole").get_asString();
      info.thumbUrl.ParseString(m_pDS2->fv("actors.strThumb").get_asString());
      details.m_cast.push_back(info);
      m_pDS2->next();
    }
    castTime += timeGetTime() - time; time = timeGetTime();
  }
  details.m_strPictureURL.Parse();
  return details;
}

CVideoInfoTag CVideoDatabase::GetDetailsForTvShow(auto_ptr<Dataset> &pDS, bool needsCast /* = false */)
{
  CVideoInfoTag details;
  details.Reset();

  DWORD time = timeGetTime();
  long lTvShowId = pDS->fv(0).get_asLong();

  for (int iType = VIDEODB_ID_TV_MIN + 1; iType < VIDEODB_ID_TV_MAX; iType++)
  {
    switch (DbTvShowOffsets[iType].type)
    {
    case VIDEODB_TYPE_STRING:
      *(CStdString*)(((char*)&details)+DbTvShowOffsets[iType].offset) = pDS->fv(iType+1).get_asString();
      break;
    case VIDEODB_TYPE_INT:
      *(int*)(((char*)&details)+DbTvShowOffsets[iType].offset) = pDS->fv(iType+1).get_asInteger();
      break;
    case VIDEODB_TYPE_BOOL:
      *(bool*)(((char*)&details)+DbTvShowOffsets[iType].offset) = pDS->fv(iType+1).get_asBool();
      break;
    case VIDEODB_TYPE_FLOAT:
      *(float*)(((char*)&details)+DbTvShowOffsets[iType].offset) = pDS->fv(iType+1).get_asFloat();
      break;
    }
  }
  details.m_iDbId = lTvShowId;
  // note use of -2 here as opposed to path, as there is no file.strFileName in the query nor any fileid, so two less entries
  details.m_strPath = pDS->fv(VIDEODB_DETAILS_PATH - 2).get_asString();
  details.m_strShowTitle = details.m_strTitle;

  movieTime += timeGetTime() - time; time = timeGetTime();

  if (needsCast)
  {
    // create cast string
    CStdString strSQL = FormatSQL("select actors.strActor,actorlinktvshow.strRole,actors.strThumb from actorlinktvshow,actors where actorlinktvshow.idShow=%u and actorlinktvshow.idActor = actors.idActor",lTvShowId);
    m_pDS2->query(strSQL.c_str());
    while (!m_pDS2->eof())
    {
      SActorInfo info;
      info.strName = m_pDS2->fv("actors.strActor").get_asString();
      info.strRole = m_pDS2->fv("actorlinktvshow.strRole").get_asString();
      info.thumbUrl.ParseString(m_pDS2->fv("actors.strThumb").get_asString());
      details.m_cast.push_back(info);
      m_pDS2->next();
    }
    castTime += timeGetTime() - time; time = timeGetTime();
  }
  details.m_strPictureURL.Parse();
  return details;
}

CVideoInfoTag CVideoDatabase::GetDetailsForEpisode(auto_ptr<Dataset> &pDS, bool needsCast /* = false */)
{
  CVideoInfoTag details;
  details.Reset();

  DWORD time = timeGetTime();
  long lEpisodeId = pDS->fv(0).get_asLong();

  for (int iType = VIDEODB_ID_EPISODE_MIN + 1; iType < VIDEODB_ID_EPISODE_MAX; iType++)
  {
    switch (DbEpisodeOffsets[iType].type)
    {
    case VIDEODB_TYPE_STRING:
      *(CStdString*)(((char*)&details)+DbEpisodeOffsets[iType].offset) = pDS->fv(iType+1).get_asString();
      break;
    case VIDEODB_TYPE_INT:
      *(int*)(((char*)&details)+DbEpisodeOffsets[iType].offset) = pDS->fv(iType+1).get_asInteger();
      break;
    case VIDEODB_TYPE_BOOL:
      *(bool*)(((char*)&details)+DbEpisodeOffsets[iType].offset) = pDS->fv(iType+1).get_asBool();
      break;
    case VIDEODB_TYPE_FLOAT:
      *(float*)(((char*)&details)+DbEpisodeOffsets[iType].offset) = pDS->fv(iType+1).get_asFloat();
      break;
    }
  }
  details.m_iDbId = lEpisodeId;

  details.m_strPath = pDS->fv(VIDEODB_DETAILS_PATH).get_asString();
  CUtil::AddFileToFolder(details.m_strPath, pDS->fv(VIDEODB_DETAILS_FILE).get_asString(),details.m_strFileNameAndPath);
  movieTime += timeGetTime() - time; time = timeGetTime();

  details.m_strShowTitle = pDS->fv(VIDEODB_DETAILS_PATH+1).get_asString();

  if (needsCast)
  {
    // create cast string
    CStdString strSQL = FormatSQL("select actors.strActor,actorlinkepisode.strRole,actors.strThumb from actorlinkepisode,actors where actorlinkepisode.idEpisode=%u and actorlinkepisode.idActor = actors.idActor",lEpisodeId);
    m_pDS2->query(strSQL.c_str());
    while (!m_pDS2->eof())
    {
      SActorInfo info;
      info.strName = m_pDS2->fv("actors.strActor").get_asString();
      info.strRole = m_pDS2->fv("actorlinkepisode.strRole").get_asString();
      info.thumbUrl.ParseString(m_pDS2->fv("actors.strThumb").get_asString());
      details.m_cast.push_back(info);      
      m_pDS2->next();
    }
    castTime += timeGetTime() - time; time = timeGetTime();
    m_pDS2->close();
  }
  details.m_strPictureURL.Parse();
  return details;
}

CVideoInfoTag CVideoDatabase::GetDetailsForMusicVideo(auto_ptr<Dataset> &pDS)
{
  CVideoInfoTag details;
  details.Reset();

  DWORD time = timeGetTime();
  long lMovieId = pDS->fv(0).get_asLong();

  for (int iType = VIDEODB_ID_MUSICVIDEO_MIN + 1; iType < VIDEODB_ID_MUSICVIDEO_MAX; iType++)
  {
    switch (DbMusicVideoOffsets[iType].type)
    {
    case VIDEODB_TYPE_STRING:
      *(CStdString*)(((char*)&details)+DbMusicVideoOffsets[iType].offset) = pDS->fv(iType+1).get_asString();
      break;
    case VIDEODB_TYPE_INT:
      *(int*)(((char*)&details)+DbMusicVideoOffsets[iType].offset) = pDS->fv(iType+1).get_asInteger();
      break;
    case VIDEODB_TYPE_BOOL:
      *(bool*)(((char*)&details)+DbMusicVideoOffsets[iType].offset) = pDS->fv(iType+1).get_asBool();
      break;
    case VIDEODB_TYPE_FLOAT:
      *(float*)(((char*)&details)+DbMusicVideoOffsets[iType].offset) = pDS->fv(iType+1).get_asFloat();
      break;
    }
  }
  details.m_iDbId = lMovieId;

  details.m_strPath = pDS->fv(VIDEODB_DETAILS_PATH).get_asString();
  CUtil::AddFileToFolder(details.m_strPath, pDS->fv(VIDEODB_DETAILS_FILE).get_asString(),details.m_strFileNameAndPath);
  movieTime += timeGetTime() - time; time = timeGetTime();

  // create artist string
  CStdString strSQL = FormatSQL("select actors.strActor from artistlinkmusicvideo,actors where artistlinkmusicvideo.idmvideo=%u and artistlinkmusicvideo.idartist = actors.idActor",lMovieId);
  m_pDS2->query(strSQL.c_str());
  while (!m_pDS2->eof())
  {
    details.m_artist.push_back(m_pDS2->fv("actors.strActor").get_asString());
    m_pDS2->next();
  }
  castTime += timeGetTime() - time; time = timeGetTime();

  // create genre string
  strSQL = FormatSQL("select genre.strGenre from genrelinkmusicvideo,genre where genrelinkmusicvideo.idmvideo=%u and genrelinkmusicvideo.idgenre = genre.idGenre",lMovieId);
  m_pDS2->query(strSQL.c_str());
  while (!m_pDS2->eof())
  {
    details.m_strGenre += m_pDS2->fv("genre.strGenre").get_asString()+g_advancedSettings.m_videoItemSeparator;
    m_pDS2->next();
  }
  details.m_strGenre.TrimRight(g_advancedSettings.m_videoItemSeparator);

  details.m_strPictureURL.Parse();
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
    long lFileId = GetFile(strFilenameAndPath);
    if (lFileId < 0) return false;
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    // ok, now obtain the settings for this file
    CStdString strSQL=FormatSQL("select * from settings where settings.idFile = '%i'", lFileId);
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
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;
    long lFileId = GetFile(strFilenameAndPath);
    if (lFileId < 0)
    { // no files found - we have to add one
      lFileId = AddFile(strFilenameAndPath);
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
    long lFileId = GetFile(filePath);
    if (lFileId < 0) return false;
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    // ok, now obtain the settings for this file
    CStdString strSQL=FormatSQL("select times from stacktimes where idFile=%i\n", lFileId);
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
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;
    long lFileId = GetFile(filePath);
    if (lFileId < 0)
    { // no files found - we have to add one
      lFileId = AddFile(filePath);
      if (lFileId < 0) return ;
    }

    // delete any existing items
    m_pDS->exec( FormatSQL("delete from stacktimes where idFile=%i", lFileId) );

    // add the items
    CStdString timeString;
    timeString.Format("%i", times[0]);
    for (unsigned int i = 1; i < times.size(); i++)
    {
      CStdString time;
      time.Format(",%i", times[i]);
      timeString += time;
    }
    m_pDS->exec( FormatSQL("insert into stacktimes (idFile,times) values (%i,'%s')\n", lFileId, timeString.c_str()) );
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::SetStackTimes(%s) failed", filePath.c_str());
  }
}

void CVideoDatabase::RemoveContentForPath(const CStdString& strPath, CGUIDialogProgress *progress /* = NULL */)
{
  if(CUtil::IsMultiPath(strPath))
  {
    std::vector<CStdString> paths;
    CMultiPathDirectory::GetPaths(strPath, paths);

    for(unsigned i=0;i<paths.size();i++)
      RemoveContentForPath(paths[i], progress);
  }

  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    std::auto_ptr<Dataset> pDS(m_pDB->CreateDataset());
    CStdString strPath1(strPath);
    CStdString strSQL = FormatSQL("select idPath,strContent,strPath from path where strPath like '%%%s%%'",strPath1.c_str());
    CGUIDialogProgress *progress = (CGUIDialogProgress *)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
    pDS->query(strSQL.c_str());
    bool bEncodedChecked=false;
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
    int iCurr=0;
    int iMax = pDS->num_rows();
    while (!pDS->eof())
    {
      bool bMvidsChecked=false;
      if (progress)
      {
        progress->SetPercentage((int)((float)(iCurr++)/iMax*100.f));
        progress->Progress();
      }
      long lPathId = pDS->fv("path.idPath").get_asLong();
      CStdString strCurrPath = pDS->fv("path.strPath").get_asString();
      if (HasTvShowInfo(strCurrPath))
        DeleteTvShow(strCurrPath);
      else
      {
        strSQL=FormatSQL("select files.strFilename from files join movie on movie.idFile=files.idFile where files.idPath=%u",lPathId);
        m_pDS2->query(strSQL.c_str());
        if (m_pDS2->eof())
        {
          strSQL=FormatSQL("select files.strFilename from files join musicvideo on musicvideo.idFile=files.idFile where files.idPath=%u",lPathId);
          m_pDS2->query(strSQL.c_str());
          bMvidsChecked = true;
        }
        while (!m_pDS2->eof())
        {
          CStdString strMoviePath;
          CUtil::AddFileToFolder(strCurrPath,m_pDS2->fv("files.strFilename").get_asString(),strMoviePath);
          if (HasMovieInfo(strMoviePath))
            DeleteMovie(strMoviePath);
          if (HasMusicVideoInfo(strMoviePath))
            DeleteMusicVideo(strMoviePath);
          m_pDS2->next();
          if (m_pDS2->eof() && !bMvidsChecked)
          {
            strSQL=FormatSQL("select files.strFilename from files join musicvideo on musicvideo.idFile=files.idFile where files.idPath=%u",lPathId);
            m_pDS2->query(strSQL.c_str());
            bMvidsChecked = true;
          }
        }
        m_pDS2->close();
      }
      pDS->next();
      if (pDS->eof() && !bEncodedChecked) // rarred titles needs this
      {
        CStdString strEncoded(strPath);
        CUtil::URLEncode(strEncoded);
        CStdString strSQL = FormatSQL("select idPath,strContent,strPath from path where strPath like '%%%s%%'",strEncoded.c_str());
        pDS->query(strSQL.c_str());
        bEncodedChecked = true;
      }
    }
    strSQL = FormatSQL("update path set strContent = '', strScraper='', strHash='',useFolderNames=0,scanRecursive=0 where strPath like '%%%s%%'",strPath1.c_str());
    pDS->exec(strSQL);

    CStdString strEncoded(strPath);
    CUtil::URLEncode(strEncoded);
    strSQL = FormatSQL("update path set strContent = '', strScraper='',strHash='',useFolderNames=0,scanRecursive=0 where strPath like '%%%s%%'",strEncoded.c_str());
    pDS->exec(strSQL);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::RemoveContentFromPath(%s) failed", strPath.c_str());
  }
  if (progress)
    progress->Close();
}

void CVideoDatabase::SetScraperForPath(const CStdString& filePath, const SScraperInfo& info, const VIDEO::SScanSettings& settings)
{
  // if we have a multipath, set scraper for all contained paths too
  if(CUtil::IsMultiPath(filePath))
  {
    std::vector<CStdString> paths;
    CMultiPathDirectory::GetPaths(filePath, paths);

    for(unsigned i=0;i<paths.size();i++)
      SetScraperForPath(paths[i],info,settings);
  }

  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;
    long lPathId = GetPath(filePath);
    if (lPathId < 0)
    { // no path found - we have to add one
      lPathId = AddPath(filePath);
      if (lPathId < 0) return ;
    }

    // Update
    CStdString strSQL=FormatSQL("update path set strContent='%s',strScraper='%s', scanRecursive=%i, useFolderNames=%i where idPath=%u", info.strContent.c_str(), info.strPath.c_str(),settings.recurse,settings.parent_name,lPathId);
    m_pDS->exec(strSQL.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::SetScraperForPath(%s) failed", filePath.c_str());
  }
}

bool CVideoDatabase::UpdateOldVersion(int iVersion)
{
  BeginTransaction();
  try 
  {
    if (iVersion < 4)
    {
      m_pDS->exec("CREATE UNIQUE INDEX ix_tvshowlinkepisode_1 ON tvshowlinkepisode ( idShow, idEpisode )\n");
      m_pDS->exec("CREATE UNIQUE INDEX ix_tvshowlinkepisode_2 ON tvshowlinkepisode ( idEpisode, idShow )\n");
    }
    if (iVersion < 5)
    {
      CLog::Log(LOGINFO,"Creating temporary movie table");
      CStdString columns;
      for (int i = 0; i < VIDEODB_MAX_COLUMNS; i++)
      {
        CStdString column, select;
        column.Format(",c%02d text", i);
        select.Format(",c%02d",i);
        columns += column;
      }
      CStdString strSQL=FormatSQL("CREATE TEMPORARY TABLE tempmovie ( idMovie integer primary key%s,idFile integer)\n",columns.c_str());
      m_pDS->exec(strSQL.c_str());
      CLog::Log(LOGINFO, "Copying movies into temporary movie table");
      strSQL=FormatSQL("INSERT INTO tempmovie select *,0 from movie");
      m_pDS->exec(strSQL.c_str());
      CLog::Log(LOGINFO, "Dropping old movie table");
      m_pDS->exec("DROP TABLE movie");
      CLog::Log(LOGINFO, "Creating new movie table");
      strSQL = "CREATE TABLE movie ( idMovie integer primary key"+columns+",idFile integer)\n";
      m_pDS->exec(strSQL.c_str());
      CLog::Log(LOGINFO, "Copying movies into new movie table");
      m_pDS->exec("INSERT INTO movie select * from tempmovie");
      CLog::Log(LOGINFO, "Dropping temporary movie table");
      m_pDS->exec("DROP TABLE tempmovie");
      m_pDS->exec("CREATE UNIQUE INDEX ix_movie_file_1 ON movie (idFile, idMovie)");
      m_pDS->exec("CREATE UNIQUE INDEX ix_movie_file_2 ON movie (idMovie, idFile)");

      CLog::Log(LOGINFO,"Creating temporary episode table");
      strSQL=FormatSQL("CREATE TEMPORARY TABLE tempepisode ( idEpisode integer primary key%s,idFile integer)\n",columns.c_str());
      m_pDS->exec(strSQL.c_str());
      CLog::Log(LOGINFO, "Copying episodes into temporary episode table");
      strSQL=FormatSQL("INSERT INTO tempepisode select idEpisode%s,0 from episode",columns.c_str());
      m_pDS->exec(strSQL.c_str());
      CLog::Log(LOGINFO, "Dropping old episode table");
      m_pDS->exec("DROP TABLE episode");
      CLog::Log(LOGINFO, "Creating new episode table");
      strSQL = "CREATE TABLE episode ( idEpisode integer primary key"+columns+",idFile integer)\n";
      m_pDS->exec(strSQL.c_str());
      CLog::Log(LOGINFO, "Copying episodes into new episode table");
      m_pDS->exec("INSERT INTO episode select * from tempepisode");
      CLog::Log(LOGINFO, "Dropping temporary episode table");
      m_pDS->exec("DROP TABLE tempepisode");
      m_pDS->exec("CREATE UNIQUE INDEX ix_episode_file_1 on episode (idEpisode, idFile)");
      m_pDS->exec("CREATE UNIQUE INDEX id_episode_file_2 on episode (idFile, idEpisode)");

      // run over all files, creating the approriate links
      strSQL=FormatSQL("select * from files");
      m_pDS->query(strSQL.c_str());
      while (!m_pDS->eof())
      {
        strSQL.Empty();
        long lEpisodeId = m_pDS->fv("files.idEpisode").get_asLong();
        long lMovieId = m_pDS->fv("files.idMovie").get_asLong();
        if (lEpisodeId > -1)
        {
          strSQL=FormatSQL("update episode set idFile=%u where idEpisode=%u",m_pDS->fv("files.idFile").get_asLong(),lEpisodeId);
        }
        if (lMovieId > -1)
          strSQL=FormatSQL("update movie set idFile=%u where idMovie=%u",m_pDS->fv("files.idFile").get_asLong(),lMovieId);

        if (!strSQL.IsEmpty())
          m_pDS2->exec(strSQL.c_str());

        m_pDS->next();
      }
      // now fix them paths
      strSQL = "select * from path";
      m_pDS->query(strSQL.c_str());
      while (!m_pDS->eof())
      {
        CStdString strPath = m_pDS->fv("path.strPath").get_asString();
        CUtil::AddSlashAtEnd(strPath);
        strSQL = FormatSQL("update path set strPath='%s' where idPath=%u",strPath.c_str(),m_pDS->fv("path.idPath").get_asLong());
        m_pDS2->exec(strSQL.c_str());
        m_pDS->next();
      }
      m_pDS->exec("DROP TABLE movielinkfile");
      m_pDS->close();
    }
    if (iVersion < 6)
    {
      // add the strHash column to path table
      m_pDS->exec("alter table path add strHash text");
    }
    if (iVersion < 7)
    {
      // add the scan settings to path table
      m_pDS->exec("alter table path add scanRecursive bool");
      m_pDS->exec("alter table path add useFolderNames bool");
    }
    if (iVersion < 8)
    {
      // modify scanRecursive to be an integer
      m_pDS->exec("CREATE TEMPORARY TABLE temppath ( idPath integer primary key, strPath text, strContent text, strScraper text, strHash text, scanRecursive bool, useFolderNames bool)\n");
      m_pDS->exec("INSERT INTO temppath SELECT * FROM path\n");
      m_pDS->exec("DROP TABLE path\n");
      m_pDS->exec("CREATE TABLE path ( idPath integer primary key, strPath text, strContent text, strScraper text, strHash text, scanRecursive integer, useFolderNames bool)\n");
      m_pDS->exec("CREATE UNIQUE INDEX ix_path ON path ( strPath )\n");
      m_pDS->exec(FormatSQL("INSERT INTO path SELECT idPath,strPath,strContent,strScraper,strHash,CASE scanRecursive WHEN 0 THEN 0 ELSE %i END AS scanRecursive,useFolderNames FROM temppath\n", INT_MAX));
      m_pDS->exec("DROP TABLE temppath\n");
    }
    if (iVersion < 9)
    {
      CLog::Log(LOGINFO, "create movielinktvshow table");
      m_pDS->exec("CREATE TABLE movielinktvshow ( idMovie integer, IdShow integer)\n");
      m_pDS->exec("CREATE UNIQUE INDEX ix_movielinktvshow_1 ON movielinktvshow ( idShow, idMovie)\n");
      m_pDS->exec("CREATE UNIQUE INDEX ix_movielinktvshow_2 ON movielinktvshow ( idMovie, idShow)\n");
    }
    if (iVersion < 10)
    {
      CLog::Log(LOGINFO, "create studio table");
      m_pDS->exec("CREATE TABLE studio ( idStudio integer primary key, strStudio text)\n");

      CLog::Log(LOGINFO, "create studiolinkmovie table");
      m_pDS->exec("CREATE TABLE studiolinkmovie ( idStudio integer, idMovie integer)\n");
      m_pDS->exec("CREATE UNIQUE INDEX ix_studiolinkmovie_1 ON studiolinkmovie ( idStudio, idMovie)\n");
      m_pDS->exec("CREATE UNIQUE INDEX ix_studiolinkmovie_2 ON studiolinkmovie ( idMovie, idStudio)\n");
    }
    if (iVersion < 11)
    {
      CLog::Log(LOGINFO, "create musicvideo table");
      CStdString columns = "CREATE TABLE musicvideo ( idMVideo integer primary key";
      for (int i = 0; i < VIDEODB_MAX_COLUMNS; i++)
      {
        CStdString column;
        column.Format(",c%02d text", i);
        columns += column;
      }
      columns += ",idFile integer)";
      m_pDS->exec(columns.c_str());
      m_pDS->exec("CREATE UNIQUE INDEX ix_musicvideo_file_1 on musicvideo (idMVideo, idFile)");
      m_pDS->exec("CREATE UNIQUE INDEX ix_musicvideo_file_2 on musicvideo (idFile, idMVideo)");

      CLog::Log(LOGINFO, "create artistlinkmusicvideo table");
      m_pDS->exec("CREATE TABLE artistlinkmusicvideo ( idArtist integer, idMVideo integer)\n");
      m_pDS->exec("CREATE UNIQUE INDEX ix_artistlinkmusicvideo_1 ON artistlinkmusicvideo ( idArtist, idMVideo)\n");
      m_pDS->exec("CREATE UNIQUE INDEX ix_artistlinkmusicvideo_2 ON artistlinkmusicvideo ( idMVideo, idArtist)\n");

      CLog::Log(LOGINFO, "create genrelinkmusicvideo table");
      m_pDS->exec("CREATE TABLE genrelinkmusicvideo ( idGenre integer, idMVideo integer)\n");
      m_pDS->exec("CREATE UNIQUE INDEX ix_genrelinkmusicvideo_1 ON genrelinkmusicvideo ( idGenre, idMVideo)\n");
      m_pDS->exec("CREATE UNIQUE INDEX ix_genrelinkmusicvideo_2 ON genrelinkmusicvideo ( idMVideo, idGenre)\n");

      CLog::Log(LOGINFO, "create studiolinkmusicvideo table");
      m_pDS->exec("CREATE TABLE studiolinkmusicvideo ( idStudio integer, idMVideo integer)\n");
      m_pDS->exec("CREATE UNIQUE INDEX ix_studiolinkmusicvideo_1 ON studiolinkmusicvideo ( idStudio, idMVideo)\n");
      m_pDS->exec("CREATE UNIQUE INDEX ix_studiolinkmusicvideo_2 ON studiolinkmusicvideo ( idMVideo, idStudio)\n");

      CLog::Log(LOGINFO, "create directorlinkmusicvideo table");
      m_pDS->exec("CREATE TABLE directorlinkmusicvideo ( idDirector integer, idMVideo integer)\n");
      m_pDS->exec("CREATE UNIQUE INDEX ix_directorlinkmusicvideo_1 ON directorlinkmusicvideo ( idDirector, idMVideo )\n");
      m_pDS->exec("CREATE UNIQUE INDEX ix_directorlinkmusicvideo_2 ON directorlinkmusicvideo ( idMVideo, idDirector )\n");
    }
    if (iVersion < 12)
    {
      // add the thumb column to the actors table
      m_pDS->exec("alter table actors add strThumb text");
    }
    if (iVersion < 13)
    {
      // add some indices
      m_pDS->exec("CREATE INDEX ix_bookmark ON bookmark (idFile)");
      CStdString createColIndex;
      createColIndex.Format("CREATE INDEX ix_episode_season_episode on episode (c%02d, c%02d)", VIDEODB_ID_EPISODE_SEASON, VIDEODB_ID_EPISODE_EPISODE);
      m_pDS->exec(createColIndex.c_str());
      createColIndex.Format("CREATE INDEX ix_episode_bookmark on episode (c%02d)", VIDEODB_ID_EPISODE_BOOKMARK);
      m_pDS->exec(createColIndex.c_str());
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "Error attempting to update the database version!");
    RollbackTransaction();
    return false;
  }
  CommitTransaction();
  return true;
}

void CVideoDatabase::MarkAsWatched(const CFileItem &item)
{
  // find the movie in the db
  long movieID = -1;
  if (item.HasVideoInfoTag() && item.GetVideoInfoTag()->m_iEpisode == -1) // movie
    movieID = GetMovieInfo(item.m_strPath);

  int iType=0;
  if (movieID < 0)
  {
    iType = 1;
    movieID = GetEpisodeInfo(item.m_strPath);
  }
  if (movieID < 0)
  {
    movieID = GetMusicVideoInfo(item.m_strPath);
    if (movieID < 0)
      return;    
    iType = 3;
  }

  // and mark as watched
  MarkAsWatched(movieID,iType);
}

void CVideoDatabase::MarkAsWatched(long lMovieId, int iType /* = 0 */)
{
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;
    CStdString strSQL;
    if (iType == 0)
    {
      CLog::Log(LOGINFO, "Updating Movie:%i as Watched", lMovieId);
      strSQL.Format("UPDATE movie set c%02d='true' WHERE idMovie=%u", VIDEODB_ID_WATCHED, lMovieId);
    }
    else if (iType == 1)
    {
      CLog::Log(LOGINFO, "Updating Episode:%i as Watched", lMovieId);
      strSQL.Format("UPDATE episode set c%02d='true' WHERE idEpisode=%u", VIDEODB_ID_EPISODE_WATCHED, lMovieId);
    }
    else if (iType == 3)
    {
      CLog::Log(LOGINFO, "Updating MusicVideo:%i as Watched", lMovieId);
      strSQL.Format("UPDATE musicvideo set c%02d='true' WHERE idMVideo=%u", VIDEODB_ID_MUSICVIDEO_WATCHED, lMovieId);
    }

    m_pDS->exec(strSQL.c_str());
  }
  catch (...)
  {
	  CLog::Log(LOGERROR, "CVideoDatabase::MarkAsWatched(long lMovieId) failed on MovieID:%i", lMovieId);
  }
}

void CVideoDatabase::MarkAsUnWatched(long lMovieId, int iType /* = 0 */)
{
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;
    CStdString strSQL;
    if (iType == 0)
    {
      CLog::Log(LOGINFO, "Updating Movie:%i as UnWatched", lMovieId);
      strSQL.Format("UPDATE movie set c%02d='false' WHERE idMovie=%u", VIDEODB_ID_WATCHED, lMovieId);
    }

    else if (iType == 1)
    {
      CLog::Log(LOGINFO, "Updating Episode:%i as UnWatched", lMovieId);
      strSQL.Format("UPDATE episode set c%02d='false' WHERE idEpisode=%u", VIDEODB_ID_EPISODE_WATCHED, lMovieId);
    }
    else if (iType == 3)
    {
      CLog::Log(LOGINFO, "Updating MusicVideo:%i as UnWatched", lMovieId);
      strSQL.Format("UPDATE musicvideo set c%02d='false' WHERE idMVideo=%u", VIDEODB_ID_MUSICVIDEO_WATCHED, lMovieId);
    }
    m_pDS->exec(strSQL.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::MarkAsUnWatched(long lMovieId) failed on MovieID:%i", lMovieId);
  }
}

void CVideoDatabase::UpdateMovieTitle(long lMovieId, const CStdString& strNewMovieTitle, int iType)
{
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;
    CStdString strSQL;
    if (iType == 0)    
    {
      CLog::Log(LOGINFO, "Changing Movie:id:%i New Title:%s", lMovieId, strNewMovieTitle.c_str());
      strSQL = FormatSQL("UPDATE movie SET c%02d='%s' WHERE idMovie=%i", VIDEODB_ID_TITLE, strNewMovieTitle.c_str(), lMovieId );
    }
    else if (iType == 1)
    {
      CLog::Log(LOGINFO, "Changing Episode:id:%i New Title:%s", lMovieId, strNewMovieTitle.c_str());
      strSQL = FormatSQL("UPDATE episode SET c%02d='%s' WHERE idEpisode=%i", VIDEODB_ID_EPISODE_TITLE, strNewMovieTitle.c_str(), lMovieId );
    }
    else if (iType == 2)
    {
      CLog::Log(LOGINFO, "Changing TvShow:id:%i New Title:%s", lMovieId, strNewMovieTitle.c_str());
      strSQL = FormatSQL("UPDATE tvshow SET c%02d='%s' WHERE idShow=%i", VIDEODB_ID_TV_TITLE, strNewMovieTitle.c_str(), lMovieId );
    }
    else if (iType == 3)
    {
      CLog::Log(LOGINFO, "Changing MusicVideo:id:%i New Title:%s", lMovieId, strNewMovieTitle.c_str());
      strSQL = FormatSQL("UPDATE musicvideo SET c%02d='%s' WHERE idMVideo=%i", VIDEODB_ID_MUSICVIDEO_TITLE, strNewMovieTitle.c_str(), lMovieId );
    }
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

bool CVideoDatabase::GetGenresNav(const CStdString& strBaseDir, CFileItemList& items, long idContent)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

//    DWORD time = timeGetTime();
    // get primary genres for movies
    CStdString strSQL;
    if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
    {
      if (idContent == VIDEODB_CONTENT_MOVIES)
      {
        strSQL=FormatSQL("select genre.idgenre,genre.strgenre,path.strPath,movie.c%02d from genre,genrelinkmovie,movie,path,files where genre.idGenre=genrelinkMovie.idGenre and genrelinkMovie.idMovie = movie.idMovie and files.idFile=movie.idFile and path.idPath = files.idPath",VIDEODB_ID_WATCHED);
      }
      else if (idContent == VIDEODB_CONTENT_TVSHOWS)
      {
        strSQL=FormatSQL("select genre.idgenre,genre.strgenre,path.strPath from genre,genrelinktvshow,tvshow,tvshowlinkpath,files,path where genre.idGenre=genrelinkTvShow.idGenre and genrelinkTvShow.idShow = tvshow.idshow and files.idPath=tvshowlinkpath.idPath and tvshowlinkpath.idShow=tvshow.idShow and path.idPath = files.idPath");
      }
      else if (idContent == VIDEODB_CONTENT_MUSICVIDEOS)
      {
        strSQL=FormatSQL("select genre.idgenre,genre.strgenre,path.strPath,musicvideo.c%02d from genre,genrelinkmusicvideo,musicvideo,files,path where genre.idGenre=genrelinkmusicvideo.idGenre and genrelinkmusicvideo.idMVideo = musicvideo.idmvideo and musicvideo.idfile=files.idfile and path.idpath = files.idpath",VIDEODB_ID_MUSICVIDEO_WATCHED);
      }
    }
    else
    {
      if (idContent == VIDEODB_CONTENT_MOVIES)
      {
        strSQL=FormatSQL("select distinct genre.idgenre,genre.strgenre,movie.c%02d from genre,genrelinkmovie,movie where genre.idGenre=genrelinkMovie.idGenre and genrelinkMovie.idMovie = movie.idMovie",VIDEODB_ID_WATCHED);
      }
      else if (idContent == VIDEODB_CONTENT_TVSHOWS)
      {
        strSQL=FormatSQL("select distinct genre.idgenre,genre.strgenre from genre,genrelinktvshow,tvshow where genre.idGenre=genrelinkTvShow.idGenre and genrelinkTvShow.idShow = tvshow.idshow");
      }
      else if (idContent == VIDEODB_CONTENT_MUSICVIDEOS)
      {
        strSQL=FormatSQL("select distinct genre.idgenre,genre.strgenre,musicvideo.c%02d from genre,genrelinkmusicvideo,musicvideo where genre.idGenre=genrelinkmusicvideo.idGenre and genrelinkmusicvideo.idmvideo = musicvideo.idmvideo",VIDEODB_ID_MUSICVIDEO_WATCHED);
      }
    }

    // run query
    CLog::Log(LOGDEBUG, "CVideoDatabase::GetGenresNav() query: %s", strSQL.c_str());
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return true;
    }

    if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
    {
      map<long, pair<CStdString,bool> > mapGenres;
      map<long, pair<CStdString,bool> >::iterator it;
      while (!m_pDS->eof())
      {
        long lGenreId = m_pDS->fv("genre.idgenre").get_asLong();
        CStdString strGenre = m_pDS->fv("genre.strgenre").get_asString();
        it = mapGenres.find(lGenreId);
        // was this genre already found?
        if (it == mapGenres.end())
        {
          // check path
          CStdString strPath;
          if (g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv("path.strPath").get_asString()),g_settings.m_videoSources))
            if (idContent == VIDEODB_CONTENT_MOVIES || idContent == VIDEODB_CONTENT_MUSICVIDEOS)
              mapGenres.insert(pair<long, pair<CStdString,bool> >(lGenreId, pair<CStdString,bool>(strGenre,m_pDS->fv(3).get_asBool())));
            else
              mapGenres.insert(pair<long, pair<CStdString,bool> >(lGenreId, pair<CStdString,bool>(strGenre,false)));
        }
        m_pDS->next();
      }
      m_pDS->close();

      for (it=mapGenres.begin();it != mapGenres.end();++it)
      {
        CFileItem* pItem=new CFileItem(it->second.first);
        CStdString strDir;
        strDir.Format("%ld/", it->first);
        pItem->m_strPath=strBaseDir + strDir;
        pItem->m_bIsFolder=true;
        if (idContent == VIDEODB_CONTENT_MOVIES || idContent == VIDEODB_CONTENT_MUSICVIDEOS)
          pItem->GetVideoInfoTag()->m_bWatched = it->second.second;
        if (!items.Contains(pItem->m_strPath))
        {
          pItem->SetLabelPreformated(true);
          items.Add(pItem);
        }
        else
          delete pItem;
      }
    }
    else
    {
      while (!m_pDS->eof())
      {
        CFileItem* pItem=new CFileItem(m_pDS->fv("genre.strgenre").get_asString());
        CStdString strDir;
        strDir.Format("%ld/", m_pDS->fv("genre.idgenre").get_asLong());
        pItem->m_strPath=strBaseDir + strDir;
        pItem->m_bIsFolder=true;
        pItem->SetLabelPreformated(true);
        if (idContent == VIDEODB_CONTENT_MOVIES || idContent==VIDEODB_CONTENT_MUSICVIDEOS)
          pItem->GetVideoInfoTag()->m_bWatched = m_pDS->fv(2).get_asBool();
        items.Add(pItem);
        m_pDS->next();
      }
      m_pDS->close();
    }

//    CLog::Log(LOGDEBUG, __FUNCTION__" Time: %d ms", timeGetTime() - time);
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase:GetGenresNav() failed");
  }
  return false;
}

bool CVideoDatabase::GetStudiosNav(const CStdString& strBaseDir, CFileItemList& items, long idContent)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

//    DWORD time = timeGetTime();
    // get primary genres for movies
    CStdString strSQL;
    if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
    {
      if (idContent == VIDEODB_CONTENT_MOVIES)
      {
        strSQL=FormatSQL("select studio.idstudio,studio.strstudio,path.strPath,movie.c%02d from studio,studiolinkmovie,movie,path,files where studio.idStudio=studiolinkmovie.idstudio and studiolinkMovie.idMovie = movie.idMovie and files.idFile=movie.idFile and path.idPath = files.idPath",VIDEODB_ID_WATCHED);
      }
      else if (idContent == VIDEODB_CONTENT_MUSICVIDEOS)
      {
        strSQL=FormatSQL("select studio.idstudio,studio.strstudio,path.strPath,musicvideo.c%02d from studio,studiolinkmusicvideo,musicvideo,path,files where studio.idStudio=studiolinkmusicvideo.idstudio and studiolinkmusicvideo.idMVideo = musicvideo.idMVideo and files.idFile=musicvideo.idFile and path.idPath = files.idPath",VIDEODB_ID_MUSICVIDEO_WATCHED);
      }
/*      else if (idContent == VIDEODB_CONTENT_TVSHOWS) // TODO?
      {
        strSQL=FormatSQL("select genre.idgenre,genre.strgenre,path.strPath from genre,genrelinktvshow,tvshow,tvshowlinkpath,files,path where genre.idGenre=genrelinkTvShow.idGenre and genrelinkTvShow.idShow = tvshow.idshow and files.idPath=tvshowlinkpath.idPath and tvshowlinkpath.idShow=tvshow.idShow and path.idPath = files.idPath");
      }*/
    }
    else
    {
      if (idContent == VIDEODB_CONTENT_MOVIES)
      {
        strSQL=FormatSQL("select distinct studio.idstudio,studio.strstudio,movie.c%02d from studio,studiolinkmovie,movie where studio.idstudio=studiolinkMovie.idstudio and studiolinkMovie.idMovie = movie.idMovie",VIDEODB_ID_WATCHED);
      }
      else if (idContent == VIDEODB_CONTENT_MUSICVIDEOS)
      {
        strSQL=FormatSQL("select distinct studio.idstudio,studio.strstudio,musicvideo.c%02d from studio,studiolinkmusicvideo,musicvideo where studio.idstudio=studiolinkmusicvideo.idstudio and studiolinkmusicvideo.idMVideo = musicvideo.idMVideo",VIDEODB_ID_MUSICVIDEO_WATCHED);
      }
/*      else if (idContent == VIDEODB_CONTENT_TVSHOWS) // TODO?
      {
        strSQL=FormatSQL("select distinct genre.idgenre,genre.strgenre from genre,genrelinktvshow,tvshow where genre.idGenre=genrelinkTvShow.idGenre and genrelinkTvShow.idShow = tvshow.idshow");
      }*/
    }

    // run query
    CLog::Log(LOGDEBUG, "CVideoDatabase::GetStudiosNav() query: %s", strSQL.c_str());
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return true;
    }

    if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
    {
      map<long, pair<CStdString,bool> > mapStudios;
      map<long, pair<CStdString,bool> >::iterator it;
      while (!m_pDS->eof())
      {
        long lStudioId = m_pDS->fv("studio.idstudio").get_asLong();
        CStdString strStudio = m_pDS->fv("studio.strstudio").get_asString();
        it = mapStudios.find(lStudioId);
        // was this genre already found?
        if (it == mapStudios.end())
        {
          // check path
          CStdString strPath;
          if (g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv("path.strPath").get_asString()),g_settings.m_videoSources))
            if (idContent == VIDEODB_CONTENT_MOVIES || idContent == VIDEODB_CONTENT_MUSICVIDEOS)
              mapStudios.insert(pair<long, pair<CStdString,bool> >(lStudioId, pair<CStdString,bool>(strStudio,m_pDS->fv(3).get_asBool())));
        }
        m_pDS->next();
      }
      m_pDS->close();

      for (it=mapStudios.begin();it != mapStudios.end();++it)
      {
        CFileItem* pItem=new CFileItem(it->second.first);
        CStdString strDir;
        strDir.Format("%ld/", it->first);
        pItem->m_strPath=strBaseDir + strDir;
        pItem->m_bIsFolder=true;
        if (idContent == VIDEODB_CONTENT_MOVIES || idContent == VIDEODB_CONTENT_MUSICVIDEOS)
          pItem->GetVideoInfoTag()->m_bWatched = it->second.second;
        if (!items.Contains(pItem->m_strPath))
        {
          pItem->SetLabelPreformated(true);
          items.Add(pItem);
        }
        else
          delete pItem;
      }
    }
    else
    {
      while (!m_pDS->eof())
      {
        CFileItem* pItem=new CFileItem(m_pDS->fv("studio.strstudio").get_asString());
        CStdString strDir;
        strDir.Format("%ld/", m_pDS->fv("studio.idstudio").get_asLong());
        pItem->m_strPath=strBaseDir + strDir;
        pItem->m_bIsFolder=true;
        pItem->SetLabelPreformated(true);
        if (idContent == VIDEODB_CONTENT_MOVIES || idContent == VIDEODB_CONTENT_MUSICVIDEOS)
          pItem->GetVideoInfoTag()->m_bWatched = m_pDS->fv(2).get_asBool();
        items.Add(pItem);
        m_pDS->next();
      }
      m_pDS->close();
    }

//    CLog::Log(LOGDEBUG, __FUNCTION__" Time: %d ms", timeGetTime() - time);
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase:GetStudiosNav() failed");
  }
  return false;
}

bool CVideoDatabase::GetDirectorsNav(const CStdString& strBaseDir, CFileItemList& items, long idContent)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    // get primary genres for movies
    CStdString strSQL;
    if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
    {
      if (idContent == VIDEODB_CONTENT_MOVIES)
      {
        strSQL=FormatSQL("select actors.idActor,actors.strActor,path.strPath,movie.c%02d from actors,directorlinkmovie,movie,path,files where actors.idActor=directorlinkMovie.idDirector and directorlinkMovie.idMovie = movie.idMovie and files.idFile=movie.idFile and path.idPath = files.idPath",VIDEODB_ID_WATCHED);
      }
      else if (idContent == VIDEODB_CONTENT_TVSHOWS)
      {
        strSQL=FormatSQL("select actors.idActor,actors.strActor,path.strPath from actors,directorlinktvshow,tvshow,path,files,episode,tvshowlinkepisode where actors.idActor=directorlinktvshow.idDirector and directorlinktvshow.idShow = tvshow.idShow and files.idFile=episode.idFile and tvshowlinkepisode.idShow=tvshow.idShow and episode.idEpisode=tvshowlinkepisode.idEpisode and path.idPath = files.idPath");
      }
      else if (idContent == VIDEODB_CONTENT_MUSICVIDEOS)
      {
        strSQL=FormatSQL("select actors.idActor,actors.strActor,path.strPath,musicvideo.c%02d from actors,directorlinkmusicvideo,musicvideo,path,files where actors.idActor=directorlinkmusicvideo.idDirector and directorlinkmusicvideo.idMVideo = musicvideo.idMVideo and files.idFile=musicvideo.idFile and path.idPath = files.idPath",VIDEODB_ID_MUSICVIDEO_WATCHED);
      }
    }
    else
    {
      if (idContent == VIDEODB_CONTENT_MOVIES)
      {
        strSQL=FormatSQL("select distinct actors.idactor,actors.strActor,movie.c%02d from directorlinkmovie,actors,movie where actors.idActor=directorlinkmovie.idDirector and directorlinkmovie.idmovie=movie.idmovie",VIDEODB_ID_WATCHED);
      }
      else if (idContent == VIDEODB_CONTENT_TVSHOWS)
      {
        strSQL=FormatSQL("select distinct actors.idActor,actors.strActor from actors,directorlinktvshow,tvshow where actors.idActor=directorlinktvshow.idDirector and directorlinktvshow.idShow = tvshow.idShow");
      }
      else if (idContent == VIDEODB_CONTENT_MUSICVIDEOS)
      {
        strSQL=FormatSQL("select distinct actors.idactor,actors.strActor,musicvideo.c%02d from directorlinkmusicvideo,actors,musicvideo where actors.idActor=directorlinkmusicvideo.idDirector and directorlinkmusicvideo.idmvideo=musicvideo.idmvideo",VIDEODB_ID_MUSICVIDEO_WATCHED);
      }
    }

    // run query
    CLog::Log(LOGDEBUG, "CVideoDatabase::GetDirectorsNav() query: %s", strSQL.c_str());
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return true;
    }

    if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
    {
      map<long, std::pair<CStdString,bool> > mapDirector;
      map<long, std::pair<CStdString,bool> >::iterator it;
      while (!m_pDS->eof())
      {
        long lDirectorId = m_pDS->fv("actors.idactor").get_asLong();
        CStdString strDirector = m_pDS->fv("actors.strActor").get_asString();
        it = mapDirector.find(lDirectorId);
        // was this genre already found?
        if (it == mapDirector.end())
        {
          // check path
          CStdString strPath;
          if (g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv("path.strPath").get_asString()),g_settings.m_videoSources))
            if (idContent == VIDEODB_CONTENT_MOVIES || idContent == VIDEODB_CONTENT_MUSICVIDEOS)
              mapDirector.insert(pair<long, pair<CStdString,bool> >(lDirectorId, pair<CStdString,bool>(strDirector,m_pDS->fv(3).get_asBool())));
            else
              mapDirector.insert(pair<long, pair<CStdString,bool> >(lDirectorId, pair<CStdString,bool>(strDirector,false)));
        }
        m_pDS->next();
      }
      m_pDS->close();

      for (it=mapDirector.begin();it != mapDirector.end();++it)
      {
        CFileItem* pItem=new CFileItem(it->second.first);
        CStdString strDir;
        strDir.Format("%ld/", it->first);
        pItem->m_strPath=strBaseDir + strDir;
        pItem->m_bIsFolder=true;
        if (idContent == VIDEODB_CONTENT_MOVIES || idContent == VIDEODB_CONTENT_MUSICVIDEOS)
          pItem->GetVideoInfoTag()->m_bWatched = it->second.second;
        pItem->SetLabelPreformated(true);
        items.Add(pItem);
      }
    }
    else
    {
      while (!m_pDS->eof())
      {
        CFileItem* pItem=new CFileItem(m_pDS->fv("actors.strActor").get_asString());
        CStdString strDir;
        strDir.Format("%ld/", m_pDS->fv("actors.idactor").get_asLong());
        pItem->m_strPath=strBaseDir + strDir;
        pItem->m_bIsFolder=true;
        pItem->SetLabelPreformated(true);
        if (idContent == VIDEODB_CONTENT_MOVIES || idContent == VIDEODB_CONTENT_MUSICVIDEOS)
          pItem->GetVideoInfoTag()->m_bWatched = m_pDS->fv(2).get_asBool();
        items.Add(pItem);
        m_pDS->next();
      }
      m_pDS->close();
    }
      
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase:GetDirectorsNav() failed");
  }
  return false;
}

bool CVideoDatabase::GetActorsNav(const CStdString& strBaseDir, CFileItemList& items, long idContent)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

//    DWORD time = timeGetTime();
    CStdString strSQL;
    
    if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
    {
      if (idContent == VIDEODB_CONTENT_MOVIES)
      {
        strSQL=FormatSQL("select actors.idactor,actors.strActor,path.strPath,movie.c%02d,actors.strThumb from actorlinkmovie,actors,movie,files,path where actors.idActor=actorlinkmovie.idActor and actorlinkmovie.idmovie=movie.idmovie and files.idFile = movie.idFile and files.idPath = path.idPath",VIDEODB_ID_WATCHED);
      }
      else if (idContent == VIDEODB_CONTENT_TVSHOWS)
      {
        strSQL=FormatSQL("select actors.idactor,actors.strActor,path.strPath,actors.strThumb from actorlinktvshow,actors,tvshow,tvshowlinkpath,path where actors.idActor=actorlinktvshow.idActor and actorlinktvshow.idshow=tvshow.idshow and tvshowlinkpath.idshow=tvshow.idshow and tvshowlinkpath.idpath=path.idpath");
      }
      else if (idContent == VIDEODB_CONTENT_MUSICVIDEOS)
      {
        strSQL=FormatSQL("select actors.idactor,actors.strActor,path.strPath,musicvideo.c%02d,actors.strThumb from artistlinkmusicvideo,actors,musicvideo,files,path where actors.idActor=artistlinkmusicvideo.idArtist and artistlinkmusicvideo.idmvideo=musicvideo.idmvideo and files.idFile = musicvideo.idFile and files.idPath = path.idPath",VIDEODB_ID_MUSICVIDEO_WATCHED);
      }
    }
    else
    {
      if (idContent == VIDEODB_CONTENT_MOVIES)
      {
        strSQL=FormatSQL("select distinct actors.idactor,actors.strActor,movie.c%02d,actors.strThumb from actorlinkmovie,actors,movie where actors.idActor=actorlinkmovie.idActor and actorlinkmovie.idmovie=movie.idmovie",VIDEODB_ID_WATCHED);
      }
      else if (idContent == VIDEODB_CONTENT_TVSHOWS)
      {
        strSQL=FormatSQL("select distinct actors.idactor,actors.strActor,actors.strThumb from actorlinktvshow,actors,tvshow where actors.idActor=actorlinktvshow.idActor and actorlinktvshow.idshow=tvshow.idshow");
      }
      else if (idContent == VIDEODB_CONTENT_MUSICVIDEOS)
      {
        strSQL=FormatSQL("select distinct actors.idactor,actors.strActor,musicvideo.c%02d,actors.strThumb from artistlinkmusicvideo,actors,musicvideo where actors.idActor=artistlinkmusicvideo.idartist and artistlinkmusicvideo.idmvideo=musicvideo.idmvideo",VIDEODB_ID_MUSICVIDEO_WATCHED);
      }
    }

    unsigned int time = timeGetTime();
    if (!m_pDS->query(strSQL.c_str())) return false;
    CLog::Log(LOGDEBUG, "%s -  query took %lu ms", __FUNCTION__, timeGetTime() - time); time = timeGetTime();
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return true;
    }

    if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
    {
      map<long, CActor> mapActors;
      map<long, CActor>::iterator it;
      long lLastPathId = -1;

      while (!m_pDS->eof())
      {
        long lActorId = m_pDS->fv("actors.idactor").get_asLong();
        CActor actor;
        actor.name = m_pDS->fv("actors.strActor").get_asString();
        actor.thumb = m_pDS->fv("actors.strThumb").get_asString();
        actor.watched = m_pDS->fv(3).get_asBool();
        it = mapActors.find(lActorId);
        // is this actor already known?
        if (it == mapActors.end())
        {
          // check path
          if (g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv("path.strPath").get_asString()),g_settings.m_videoSources))
          {
            if (idContent == VIDEODB_CONTENT_MOVIES || idContent == VIDEODB_CONTENT_MUSICVIDEOS)
              mapActors.insert(pair<long, CActor>(lActorId, actor));
            else
            {
              actor.watched = false;
              mapActors.insert(pair<long, CActor>(lActorId, actor));
            }
          }
        }
        m_pDS->next();
      }
      m_pDS->close();

      for (it=mapActors.begin();it != mapActors.end();++it)
      {
        CFileItem* pItem=new CFileItem(it->second.name);
        CStdString strDir;
        strDir.Format("%ld/", it->first);
        pItem->m_strPath=strBaseDir + strDir;
        pItem->m_bIsFolder=true;
        if (idContent == VIDEODB_CONTENT_MOVIES || idContent == VIDEODB_CONTENT_MUSICVIDEOS)
          pItem->GetVideoInfoTag()->m_bWatched = it->second.watched;
        pItem->GetVideoInfoTag()->m_strPictureURL.ParseString(it->second.thumb);
        pItem->SetThumbnailImage("DefaultActorBig.png");
        if (idContent != VIDEODB_CONTENT_MUSICVIDEOS && CFile::Exists(pItem->GetCachedActorThumb()))
          pItem->SetThumbnailImage(pItem->GetCachedActorThumb());
        if (idContent == VIDEODB_CONTENT_MUSICVIDEOS)
        {
          if (CFile::Exists(pItem->GetCachedArtistThumb()))
            pItem->SetThumbnailImage(pItem->GetCachedArtistThumb());
          else
            pItem->SetThumbnailImage("DefaultArtistBig.png");
        }
        items.Add(pItem);
      }
    }
    else
    {
      while (!m_pDS->eof())
      {
        CFileItem* pItem=new CFileItem(m_pDS->fv("actors.strActor").get_asString());
        CStdString strDir;
        strDir.Format("%ld/", m_pDS->fv("actors.idactor").get_asLong());
        pItem->m_strPath=strBaseDir + strDir;
        pItem->m_bIsFolder=true;
        pItem->SetThumbnailImage("DefaultActorBig.png");
        if (idContent == VIDEODB_CONTENT_MOVIES || idContent == VIDEODB_CONTENT_MUSICVIDEOS)
          pItem->GetVideoInfoTag()->m_bWatched = m_pDS->fv(2).get_asBool();
        if (idContent != VIDEODB_CONTENT_MUSICVIDEOS && CFile::Exists(pItem->GetCachedActorThumb()))
          pItem->SetThumbnailImage(pItem->GetCachedActorThumb());
        if (idContent == VIDEODB_CONTENT_MUSICVIDEOS)
        {
          if (CFile::Exists(pItem->GetCachedArtistThumb()))
            pItem->SetThumbnailImage(pItem->GetCachedArtistThumb());
          else
            pItem->SetThumbnailImage("DefaultArtistBig.png");
        }
        pItem->GetVideoInfoTag()->m_strPictureURL.ParseString(m_pDS->fv("actors.strThumb").get_asString());
        items.Add(pItem);
        m_pDS->next();
      }
      m_pDS->close();
    }
    CLog::Log(LOGDEBUG, "%s item retrieval took %lu ms", __FUNCTION__, timeGetTime() - time); time = timeGetTime();

//    CLog::Log(LOGDEBUG, __FUNCTION__" Time: %d ms", timeGetTime() - time);
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase:GetActorsNav() failed");
  }
  return false;
}

bool CVideoDatabase::GetYearsNav(const CStdString& strBaseDir, CFileItemList& items, long idContent)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL;
    if (idContent == VIDEODB_CONTENT_MOVIES)
    {
      if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      {
        strSQL = FormatSQL("select movie.c%02d,path.strPath,movie.c%02d from movie join files on files.idFile=movie.idFile join path on files.idPath = path.idPath", VIDEODB_ID_YEAR,VIDEODB_ID_WATCHED);
      }
      else
      {
        strSQL = FormatSQL("select distinct movie.c%02d,movie.c%02d from movie", VIDEODB_ID_YEAR,VIDEODB_ID_WATCHED);
      }
    }
    else if (idContent == VIDEODB_CONTENT_TVSHOWS)
    {
      if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        strSQL = FormatSQL("select tvshow.c%02d,path.strPath from tvshow join files on files.idFile=episode.idFile join episode on episode.idEpisode=tvshowlinkepisode.idEpisode join tvshowlinkepisode on tvshow.idShow = tvshowlinkepisode.idShow join path on files.idPath = path.idPath", VIDEODB_ID_TV_PREMIERED);
      else
        strSQL = FormatSQL("select distinct tvshow.c%02d from tvshow", VIDEODB_ID_TV_PREMIERED);
    }
    else if (idContent == VIDEODB_CONTENT_MUSICVIDEOS)
    {
      if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      {
        strSQL = FormatSQL("select musicvideo.c%02d,path.strPath,musicvideo.c%02d from musicvideo join files on files.idFile=musicvideo.idFile join path on files.idPath = path.idPath", VIDEODB_ID_MUSICVIDEO_YEAR,VIDEODB_ID_MUSICVIDEO_WATCHED);
      }
      else
      {
        strSQL = FormatSQL("select distinct musicvideo.c%02d,musicvideo.c%02d from musicvideo", VIDEODB_ID_MUSICVIDEO_YEAR,VIDEODB_ID_MUSICVIDEO_WATCHED);
      }
    }

    // run query
    CLog::Log(LOGDEBUG, "CVideoDatabase::GetYearsNav() query: %s", strSQL.c_str());
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return true;
    }

    if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
    {
      map<long, pair<CStdString,bool> > mapYears;
      map<long, pair<CStdString,bool> >::iterator it;
      long lLastPathId = -1;
      while (!m_pDS->eof())
      {
        long lYear = 0;
        if (idContent == VIDEODB_CONTENT_TVSHOWS)
        {
          CDateTime time;
          time.SetFromDateString(m_pDS->fv(0).get_asString());
          lYear = time.GetYear();
        }
        else if (idContent == VIDEODB_CONTENT_MOVIES || idContent == VIDEODB_CONTENT_MUSICVIDEOS)
          lYear = m_pDS->fv(0).get_asLong();
        it = mapYears.find(lYear);
        if (it == mapYears.end())
        {
          // check path
          if (g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv("path.strPath").get_asString()),g_settings.m_videoSources))
          {
            CStdString year;
            year.Format("%d", lYear);
            if (idContent == VIDEODB_CONTENT_MOVIES || idContent == VIDEODB_CONTENT_MUSICVIDEOS)
              mapYears.insert(pair<long, pair<CStdString,bool> >(lYear, pair<CStdString,bool>(year,m_pDS->fv(2).get_asBool())));
            else
              mapYears.insert(pair<long, pair<CStdString,bool> >(lYear, pair<CStdString,bool>(year,false)));
          }
        }
        m_pDS->next();
      }
      m_pDS->close();
    
      for (it=mapYears.begin();it != mapYears.end();++it)
      {
        if (it->first == 0)
          continue;
        CFileItem* pItem=new CFileItem(it->second.first);
        CStdString strDir;
        strDir.Format("%ld/", it->first);
        pItem->m_strPath=strBaseDir + strDir;
        pItem->m_bIsFolder=true;
        if (idContent == VIDEODB_CONTENT_MOVIES || idContent == VIDEODB_CONTENT_MUSICVIDEOS)
          pItem->GetVideoInfoTag()->m_bWatched = it->second.second;
        items.Add(pItem);
      }
    }
    else
    {
      while (!m_pDS->eof())
      {
        long lYear;
        CStdString strLabel;
        if (idContent == VIDEODB_CONTENT_TVSHOWS)
        {
          CDateTime time;
          time.SetFromDateString(m_pDS->fv(0).get_asString());
          lYear = time.GetYear();
          strLabel.Format("%u",lYear);
        }
        else if (idContent == VIDEODB_CONTENT_MOVIES || idContent == VIDEODB_CONTENT_MUSICVIDEOS)
        {
          lYear = m_pDS->fv(0).get_asLong();
          strLabel = m_pDS->fv(0).get_asString();
        }
        if (lYear == 0)
        {
          m_pDS->next();
          continue;
        }
        CFileItem* pItem=new CFileItem(strLabel);
        CStdString strDir;
        strDir.Format("%ld/", lYear);
        pItem->m_strPath=strBaseDir + strDir;
        pItem->m_bIsFolder=true;        
        if (!items.Contains(pItem->m_strPath))
        {
          if (idContent == VIDEODB_CONTENT_MOVIES || idContent == VIDEODB_CONTENT_MUSICVIDEOS)
            pItem->GetVideoInfoTag()->m_bWatched = m_pDS->fv(1).get_asBool();
          items.Add(pItem);
        }
        else
          delete pItem;
        m_pDS->next();
      }
      m_pDS->close();
    }

    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase:GetYearsNav() failed");
  }
  return false;
}

bool CVideoDatabase::GetSeasonsNav(const CStdString& strBaseDir, CFileItemList& items, long idActor, long idDirector, long idGenre, long idYear, long idShow)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL;

    strSQL = FormatSQL("select distinct episode.c%02d,path.strPath from episode join tvshow on tvshow.idshow=tvshowlinkepisode.idshow join tvshowlinkepisode on tvshowlinkepisode.idepisode=episode.idepisode join tvshowlinkpath on tvshowlinkpath.idshow=tvshow.idshow join path on path.idpath=tvshowlinkpath.idpath where tvshow.idshow=%u", VIDEODB_ID_EPISODE_SEASON,idShow);

    if (idActor != -1)
      strSQL = FormatSQL("select distinct episode.c%02d,path.strPath from episode join tvshow on tvshow.idshow=tvshowlinkepisode.idshow join tvshowlinkepisode on tvshowlinkepisode.idepisode = episode.idepisode join actorlinktvshow on actorlinktvshow.idshow=tvshow.idshow join tvshowlinkpath on tvshowlinkpath.idshow=tvshow.idshow join path on path.idpath=tvshowlinkpath.idpath where tvshow.idshow=%u and actorlinktvshow.idActor=%u",VIDEODB_ID_EPISODE_SEASON,idShow,idActor);

    if (idDirector != -1)
      strSQL = FormatSQL("select distinct episode.c%02d, path.strPath from episode join tvshow on tvshow.idshow=tvshowlinkepisode.idshow join directorlinktvshow on directorlinktvshow.idshow=tvshow.idshow join tvshowlinkpath on tvshowlinpath.idshow=tvshow.idshow join path on path.idpath=tvshowlinkpath.idpath where tvshow.idshow=%u and directorlinktvshow.idDirector=%u",VIDEODB_ID_EPISODE_SEASON,idShow,idDirector);

    if (idGenre != -1)
      strSQL = FormatSQL("select distinct episode.c%02d, path.strPath from episode join tvshow on tvshow.idshow=tvshowlinkepisode.idshow join tvshowlinkepisode on tvshowlinkepisode.idepisode = episode.idepisode join genrelinktvshow on genrelinktvshow.idshow=tvshow.idshow join tvshowlinkpath on tvshowlinkpath.idshow=tvshow.idshow join path on path.idpath=tvshowlinkpath.idpath where tvshow.idshow=%u and genrelinktvshow.idGenre=%u",VIDEODB_ID_EPISODE_SEASON,idShow,idGenre);

    CStdString strSQL2 = FormatSQL("select idMovie from movielinktvshow where idShow=%u",idShow);

    // run query
    CLog::Log(LOGDEBUG, __FUNCTION__" query: %s", strSQL.c_str());
    if (!m_pDS->query(strSQL.c_str())) return false;
    if (!m_pDS2->query(strSQL2.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return true;
    }

    if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
    {
      map<long, CStdString> mapYears;
      map<long, CStdString>::iterator it;
      long lLastPathId = -1;
      while (!m_pDS->eof())
      {
        long lYear = m_pDS->fv(0).get_asLong();
        it = mapYears.find(lYear);
        // check path
        if (!g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv("path.strPath").get_asString()),g_settings.m_videoSources))
        {
          m_pDS->next();
          continue;
        }
        if (it == mapYears.end())
          mapYears.insert(pair<long, CStdString>(lYear, m_pDS->fv(1).get_asString()));
        m_pDS->next();
      }
      m_pDS->close();
    
      for (it=mapYears.begin();it != mapYears.end();++it)
      {
        long lSeason = it->first;
        CStdString strLabel;
        if (lSeason == 0)
          strLabel = g_localizeStrings.Get(20381);
        else
          strLabel.Format(g_localizeStrings.Get(20358),lSeason);
        CFileItem* pItem=new CFileItem(strLabel);
        CStdString strDir;
        strDir.Format("%ld/", it->first);
        pItem->m_strPath=strBaseDir + strDir;
        pItem->m_bIsFolder=true;
        pItem->GetVideoInfoTag()->m_strTitle = strLabel;
        pItem->GetVideoInfoTag()->m_iSeason = lSeason;
        pItem->GetVideoInfoTag()->m_iDbId = idShow;
        pItem->GetVideoInfoTag()->m_strPath = it->second;
        pItem->SetCachedSeasonThumb();
        items.Add(pItem);
      }      
    }
    else
    {
      while (!m_pDS->eof())
      {
        long lSeason = m_pDS->fv(0).get_asLong();
        CStdString strLabel;
        if (lSeason == 0)
          strLabel = g_localizeStrings.Get(20381);
        else
          strLabel.Format(g_localizeStrings.Get(20358),lSeason);
        CFileItem* pItem=new CFileItem(strLabel);
        CStdString strDir;
        strDir.Format("%ld/", lSeason);
        pItem->m_strPath=strBaseDir + strDir;
        pItem->m_bIsFolder=true;
        pItem->GetVideoInfoTag()->m_strTitle = strLabel;
        pItem->GetVideoInfoTag()->m_iSeason = lSeason;
        pItem->GetVideoInfoTag()->m_iDbId = idShow;
        pItem->GetVideoInfoTag()->m_strPath = m_pDS->fv(1).get_asString();
        pItem->SetCachedSeasonThumb();
        items.Add(pItem);
        m_pDS->next();
      }
      m_pDS->close();
      // now add the linked movies if appropriate
      while (!m_pDS2->eof())
      {
        long lMovieId = m_pDS2->fv("idMovie").get_asLong();
        strSQL = FormatSQL("select movie.*,files.strFileName,path.strPath from movie join files on files.idFile=movie.idFile join path on files.idPath=path.idPath where idMovie=%u",lMovieId);
        m_pDS->query(strSQL);
        if (m_pDS->eof())
        {
          m_pDS2->next();
          continue;
        }
        CVideoInfoTag movie = GetDetailsForMovie(m_pDS);
        m_pDS->close();
        CFileItem* pItem=new CFileItem(movie);
        CStdString strDir;
        pItem->m_strPath.Format("videodb://1/2/%u", lMovieId);
        pItem->SetOverlayImage(CGUIListItem::ICON_OVERLAY_UNWATCHED,movie.m_bWatched);
        items.Add(pItem);
        m_pDS2->next();
      }
      m_pDS2->close();
    }

    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase:GetSeasonsNav() failed");
  }
  return false;
}

bool CVideoDatabase::GetTitlesNav(const CStdString& strBaseDir, CFileItemList& items, long idGenre, long idYear, long idActor, long idDirector, long idStudio)
{
  try
  {
    DWORD time = timeGetTime();
	  movieTime = 0;
	  castTime = 0;
	
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL = "select movie.*,files.strFileName,path.strPath from movie join files on files.idFile=movie.idFile join path on files.idPath=path.idPath";

    if (idGenre != -1)
    {
      strSQL=FormatSQL("select movie.*,files.strFileName,path.strPath from movie join files on files.idFile=movie.idFile join path on files.idPath=path.idPath join genrelinkmovie on genrelinkmovie.idmovie=movie.idmovie where genrelinkmovie.idGenre=%u", idGenre);
    }

    if (idStudio != -1)
    {
      strSQL=FormatSQL("select movie.*,files.strFileName,path.strPath from movie join files on files.idFile=movie.idFile join path on files.idPath=path.idPath join studiolinkmovie on studiolinkmovie.idmovie=movie.idmovie where studiolinkmovie.idstudio=%u", idStudio);
    }

    if (idDirector != -1)
    {
      strSQL=FormatSQL("select movie.*,files.strFileName,path.strPath from movie join files on files.idFile=movie.idFile join path on files.idPath=path.idPath join directorlinkmovie on directorlinkmovie.idmovie=movie.idmovie where directorlinkmovie.idDirector=%u", idDirector);
    }

    if (idYear !=-1)
    {
      strSQL=FormatSQL("select movie.*,files.strFileName,path.strPath from movie join files on files.idFile=movie.idFile join path on files.idPath=path.idPath where movie.c%02d='%i'",VIDEODB_ID_YEAR,idYear);
    }

    if (idActor != -1)
    {
      strSQL=FormatSQL("select movie.*,files.strFileName,path.strPath from movie join files on files.idFile = movie.idFile join path on files.idPath=path.idPath join actorlinkmovie on actorlinkmovie.idmovie=movie.idmovie join actors on actors.idActor=actorlinkmovie.idActor where actors.idActor=%u",idActor);
    }

    // get all songs out of the database in fixed size chunks
    // dont reserve the items ahead of time just in case it fails part way though
    VECMOVIES movies;
    if (idGenre == -1 && idYear == -1 && idActor == -1 && idDirector == -1)
    {
      int iLIMIT = 5000;    // chunk size
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
            return true;
          }

          CLog::DebugLog("Time for actual SQL query = %d", timeGetTime() - time); time = timeGetTime();
          // get movies from returned subtable
          while (!m_pDS->eof())
          {
            long lMovieId = m_pDS->fv("movie.idMovie").get_asLong();            
            CVideoInfoTag movie = GetDetailsForMovie(m_pDS);
            if (g_settings.m_vecProfiles[0].getLockMode() == LOCK_MODE_EVERYONE || g_passwordManager.bMasterUser ||
                g_passwordManager.IsDatabasePathUnlocked(movie.m_strPath,g_settings.m_videoSources))
            {
              CFileItem* pItem=new CFileItem(movie);
              CStdString strDir;
              strDir.Format("%ld/", lMovieId);
              pItem->m_strPath=strBaseDir + strDir;
              pItem->SetOverlayImage(CGUIListItem::ICON_OVERLAY_UNWATCHED,movie.m_bWatched);
              
              items.Add(pItem);
            }
            m_pDS->next();
          }
          CLog::DebugLog("Time to retrieve movies from dataset = %d", timeGetTime() - time);
    		  CLog::Log(LOGDEBUG, __FUNCTION__" times: Info %d, Cast %d", movieTime, castTime);
        }
        catch (...)
        {
          CLog::Log(LOGERROR, "CVideoDatabase::GetTitlesNav() failed at iteration %i", iITERATIONS);
        }
        // next iteration
        iITERATIONS++;
        m_pDS->close();
      }
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
      CVideoInfoTag movie = GetDetailsForMovie(m_pDS);
      if (g_settings.m_vecProfiles[0].getLockMode() == LOCK_MODE_EVERYONE || g_passwordManager.bMasterUser ||
          g_passwordManager.IsDatabasePathUnlocked(movie.m_strPath,g_settings.m_videoSources))
      {
        CFileItem* pItem=new CFileItem(movie);
        CStdString strDir;
        strDir.Format("%ld", lMovieId);
        pItem->m_strPath=strBaseDir + strDir;
        pItem->SetOverlayImage(CGUIListItem::ICON_OVERLAY_UNWATCHED,movie.m_bWatched);

        items.Add(pItem);
      }

      m_pDS->next();
    }

    CLog::DebugLog("Time to retrieve movies from dataset = %d", timeGetTime() - time);

    // cleanup
    m_pDS->close();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetTitlesNav() failed");
  }
  return false;
}

bool CVideoDatabase::GetTvShowsNav(const CStdString& strBaseDir, CFileItemList& items, long idGenre, long idYear, long idActor, long idDirector)
{
  try
  {
    DWORD time = timeGetTime();
	  movieTime = 0;
	  castTime = 0;
	
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL = FormatSQL("select tvshow.*,path.strPath,path.strPath from tvshow join tvshowlinkpath on tvshow.idShow=tvshowlinkpath.idShow join path on path.idpath=tvshowlinkpath.idPath");

    if (idGenre != -1)
    {
      strSQL=FormatSQL("select tvshow.*,path.strPath,path.strPath from tvshow join genrelinktvshow on genrelinktvshow.idshow=tvshow.idshow join tvshowlinkpath on tvshowlinkpath.idshow = tvshow.idshow join path on path.idpath=tvshowlinkpath.idpath where genrelinktvshow.idGenre=%u ", idGenre);
    }

    if (idDirector != -1)
    {
       strSQL=FormatSQL("select tvshow.*,path.strPath,path.strPath from tvshow join directorlinktvshow on directorlinktvshow.idshow=tvshow.idshow join tvshowlinkpath on tvshowlinkpath.idshow = tvshow.idshow join path on path.idpath=tvshowlinkpath.idpath where directorlinktvshow.idDirector=%u", idDirector);
    }

    if (idYear != -1)
    {
      strSQL=FormatSQL("select tvshow.*,path.strPath,path.strPath from tvshow,path,tvshowlinkpath where tvshow.c%02d like '%%%u%%' and path.idPath=tvshowlinkpath.idpath and tvshowlinkpath.idshow=tvshow.idshow",VIDEODB_ID_TV_PREMIERED,idYear);
    }

    if (idActor != -1)
    {
      strSQL=FormatSQL("select tvshow.*,path.strPath,path.strPath from tvshow join actorlinktvshow on actorlinktvshow.idshow=tvshow.idshow join actors on actors.idActor=actorlinktvshow.idActor join tvshowlinkpath on tvshowlinkpath.idshow = tvshow.idshow join path on tvshowlinkpath.idpath = path.idpath where actors.idActor=%u",idActor);
    }

    // get all songs out of the database in fixed size chunks
    // dont reserve the items ahead of time just in case it fails part way though
    VECMOVIES movies;
    if (idGenre == -1 && idYear == -1 && idActor == -1 && idDirector == -1)
    {
      int iLIMIT = 5000;    // chunk size
      int iSONGS = 0;       // number of movies added to items
      int iITERATIONS = 0;  // number of iterations
      
      for (int i=0;;i+=iLIMIT)
      {
        CStdString strSQL2=strSQL+FormatSQL(" limit %i offset %i", iLIMIT, i);
        CLog::Log(LOGDEBUG, "CVideoDatabase::GetTvShowsNav() query: %s", strSQL2.c_str());
        try
        {
          if (!m_pDS->query(strSQL2.c_str()))
            return false;

          // keep going until no rows are left!
          int iRowsFound = m_pDS->num_rows();
          if (iRowsFound == 0)
          {
//            m_pDS->close();
            if (iITERATIONS == 0)
              return true; // failed on first iteration, so there's probably no songs in the db
            else
            {
              return true; // there no more songs left to process (aborts the unbounded for loop)
            }
          }

          CLog::DebugLog("Time for actual SQL query = %d", timeGetTime() - time); time = timeGetTime();
          // get movies from returned subtable
          while (!m_pDS->eof())
          {
            long lShowId = m_pDS->fv("tvshow.idShow").get_asLong();
            CVideoInfoTag movie = GetDetailsForTvShow(m_pDS, false);
            CFileItem* pItem=new CFileItem(movie);
            CStdString strDir;
            strDir.Format("%ld/", lShowId);
            pItem->m_strPath=strBaseDir + strDir;
            pItem->m_dateTime.SetFromDateString(movie.m_strPremiered);
            pItem->GetVideoInfoTag()->m_iYear = pItem->m_dateTime.GetYear();

            items.Add(pItem);
            iSONGS++;
            m_pDS->next();
          }
          CLog::DebugLog("Time to retrieve movies from dataset = %d", timeGetTime() - time);
    		  CLog::Log(LOGDEBUG, __FUNCTION__" times: Info %d, Cast %d", movieTime, castTime);
        }
        catch (...)
        {
          CLog::Log(LOGERROR, "CVideoDatabase::GetTvShowNav() failed at iteration %i, num songs %i", iITERATIONS, iSONGS);

          if (iSONGS > 0)
          {
            return true; // keep whatever songs we may have gotten before the failure
          }
          else
            return true; // no songs, return false
        }
        // next iteration
        iITERATIONS++;
        m_pDS->close();
      }
      return true;
    }

    // run query
    CLog::Log(LOGDEBUG, "CVideoDatabase::GetTvShowsNav() query: %s", strSQL.c_str());
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
      long lShowId = m_pDS->fv("tvshow.idShow").get_asLong();
  
      CVideoInfoTag movie = GetDetailsForTvShow(m_pDS, false);
      CFileItem* pItem=new CFileItem(movie);
      CStdString strDir;
      strDir.Format("%ld/", lShowId);
      pItem->m_strPath=strBaseDir + strDir;
      pItem->m_dateTime.SetFromDateString(movie.m_strPremiered);
      pItem->GetVideoInfoTag()->m_iYear = pItem->m_dateTime.GetYear();
      items.Add(pItem);

      m_pDS->next();
    }

    CLog::DebugLog("Time to retrieve movies from dataset = %d", timeGetTime() - time);

    // cleanup
    m_pDS->close();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetTvShowsNav() failed");
  }
  return false;
}

bool CVideoDatabase::GetEpisodesNav(const CStdString& strBaseDir, CFileItemList& items, long idGenre, long idYear, long idActor, long idDirector, long idShow, long idSeason)
{
  try
  {
    DWORD time = timeGetTime();
	  movieTime = 0;
	  castTime = 0;
	
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL = FormatSQL("select episode.*,files.strFileName,path.strPath,tvshow.c%02d from episode join files on files.idFile=episode.idFile join tvshowlinkepisode on episode.idepisode=tvshowlinkepisode.idepisode join tvshow on tvshow.idshow=tvshowlinkepisode.idshow join path on files.idPath=path.idPath",VIDEODB_ID_TV_TITLE);

    if (idShow != -1)
    {
      strSQL = FormatSQL("select episode.*,files.strFileName,path.strPath,tvshow.c%02d from episode join files on files.idFile=episode.idFile join tvshowlinkepisode on episode.idepisode=tvshowlinkepisode.idepisode join tvshow on tvshow.idshow=tvshowlinkepisode.idshow join path on files.idPath=path.idPath where tvshowlinkepisode.idShow=%u",VIDEODB_ID_TV_TITLE,idShow);
    }

    if (idGenre != -1)
    {
      strSQL = FormatSQL("select episode.*,files.strFileName,path.strPath,tvshow.c%02d from episode join files on files.idFile=episode.idFile join tvshowlinkepisode on tvshowlinkepisode.idepisode=episode.idepisode join tvshow on tvshow.idshow=tvshowlinkepisode.idshow join path on files.idPath=path.idPath join genrelinktvshow on genrelinktvshow.idshow = tvshow.idshow where tvshowlinkepisode.idShow=%u and genrelinktvshow.idgenre=%u",VIDEODB_ID_TV_TITLE,idShow,idGenre);
    }

    if (idDirector != -1)
    {
      strSQL = FormatSQL("select episode.*,files.strFileName,path.strPath,tvshow.c%02d from episode join files on files.idFile=episode.idFile join tvshowlinkepisode on tvshow.idshow=tvshowlinkepisode.idshow join path on files.idPath=path.idPath join directorlinktvshow on directorlinktvshow.idshow = tvshow.idshow where tvshow.idShow=%u and directorlinktvshow.iddirector=%u",VIDEODB_ID_TV_TITLE,idShow,idDirector);
    }

    if (idYear !=-1)
    {
      strSQL=FormatSQL("select episode.*,files.strFileName,path.strPath,tvshow.c%02d from episode join files on files.idFile=episode.idFile join tvshowlinkepisode on episode.idepisode=tvshowlinkepisode.idepisode join path on files.idPath=path.idPath join tvshow on tvshowlinkepisode.idshow=tvshow.idshow where tvshowlinkepisode.idShow=%u and tvshow.c%02d like '%%%u%%'",VIDEODB_ID_TV_TITLE,idShow,VIDEODB_ID_TV_PREMIERED,idYear);
    }

    if (idActor != -1)
    {
      strSQL = FormatSQL("select episode.*,files.strFileName,path.strPath,tvshow.c%02d from episode join files on files.idFile=episode.idFile join tvshowlinkepisode on episode.idepisode=tvshowlinkepisode.idepisode join tvshow on tvshowlinkepisode.idshow=tvshow.idshow join path on files.idPath=path.idPath join actorlinktvshow on actorlinktvshow.idshow = tvshow.idshow where tvshow.idShow=%u and actorlinktvshow.idactor=%u",VIDEODB_ID_TV_TITLE,idShow,idActor);
    }

    if (idSeason != -1)
    {
      if (idSeason != 0)
        strSQL += FormatSQL(" and (episode.c%02d=%u or episode.c%02d=0)",VIDEODB_ID_EPISODE_SEASON,idSeason,VIDEODB_ID_EPISODE_SEASON);
      else
        strSQL += FormatSQL(" and episode.c%02d=%u",VIDEODB_ID_EPISODE_SEASON,idSeason);
    }

    // get all songs out of the database in fixed size chunks
    // dont reserve the items ahead of time just in case it fails part way though
    VECMOVIES movies;
    if (idGenre == -1 && idYear == -1 && idActor == -1 && idDirector == -1)
    {
      int iLIMIT = 5000;    // chunk size
      int iSONGS = 0;       // number of movies added to items
      int iITERATIONS = 0;  // number of iterations
      
      for (int i=0;;i+=iLIMIT)
      {
        CStdString strSQL2=strSQL+FormatSQL(" limit %i offset %i", iLIMIT, i);
        CLog::Log(LOGDEBUG, "CVideoDatabase::GetEpisodesNav() query: %s", strSQL2.c_str());
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
              return true; // there no more songs left to process (aborts the unbounded for loop)
            }
          }

          CLog::DebugLog("Time for actual SQL query = %d", timeGetTime() - time); time = timeGetTime();
          // get movies from returned subtable
          while (!m_pDS->eof())
          {
            long lEpisodeId = m_pDS->fv("episode.idepisode").get_asLong();
            CVideoInfoTag movie = GetDetailsForEpisode(m_pDS);
            if (idSeason > 0 && movie.m_iSpecialSortSeason > 0 && movie.m_iSpecialSortSeason != idSeason)
            {
              m_pDS->next();
              continue;
            }

            CFileItem* pItem=new CFileItem(movie);
            CStdString strDir;
            strDir.Format("%ld", lEpisodeId);
            pItem->m_strPath=strBaseDir + strDir;
            pItem->SetOverlayImage(CGUIListItem::ICON_OVERLAY_UNWATCHED,movie.m_bWatched);
            pItem->m_dateTime.SetFromDateString(movie.m_strFirstAired);
            pItem->GetVideoInfoTag()->m_iYear = pItem->m_dateTime.GetYear();

            items.Add(pItem);

            iSONGS++;
            m_pDS->next();
          }
          CLog::DebugLog("Time to retrieve movies from dataset = %d", timeGetTime() - time);
    		  CLog::Log(LOGDEBUG, __FUNCTION__" times: Info %d, Cast %d", movieTime, castTime);
        }
        catch (...)
        {
          CLog::Log(LOGERROR, "CVideoDatabase::GetEpisodesNav() failed at iteration %i, num songs %i", iITERATIONS, iSONGS);

          if (iSONGS > 0)
          {
            return true; // keep whatever songs we may have gotten before the failure
          }
          else
            return true; // no songs, return false
        }
        // next iteration
        iITERATIONS++;
        m_pDS->close();
      }
      return true;
    }

    // run query
    CLog::Log(LOGDEBUG, "CVideoDatabase::GetEpisodesNav() query: %s", strSQL.c_str());
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
      long lEpisodeId = m_pDS->fv("episode.idEpisode").get_asLong();

      CVideoInfoTag movie = GetDetailsForEpisode(m_pDS);
      if (idSeason > 0 && movie.m_iSpecialSortSeason > 0 && movie.m_iSpecialSortSeason != idSeason)
      {
        m_pDS->next();
        continue;
      }

      CFileItem* pItem=new CFileItem(movie);
      CStdString strDir;
      strDir.Format("%ld", lEpisodeId);
      pItem->m_strPath=strBaseDir + strDir;
      pItem->SetOverlayImage(CGUIListItem::ICON_OVERLAY_UNWATCHED,movie.m_bWatched);
      pItem->m_dateTime.SetFromDateString(movie.m_strFirstAired);
      pItem->GetVideoInfoTag()->m_iYear = pItem->m_dateTime.GetYear();
      items.Add(pItem);

      m_pDS->next();
    }

    CLog::DebugLog("Time to retrieve movies from dataset = %d", timeGetTime() - time);

    // cleanup
    m_pDS->close();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetEpisodesNav() failed");
  }
  return false;
}

bool CVideoDatabase::GetMusicVideosNav(const CStdString& strBaseDir, CFileItemList& items, long idGenre, long idYear, long idArtist, long idDirector, long idStudio)
{
  try
  {
    DWORD time = timeGetTime();
    movieTime = 0;
    castTime = 0;

    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL = "select musicvideo.*,files.strFileName,path.strPath from musicvideo join files on files.idFile=musicvideo.idFile join path on files.idPath=path.idPath";

    if (idGenre != -1)
    {
      strSQL=FormatSQL("select musicvideo.*,files.strFileName,path.strPath from musicvideo join files on files.idFile=musicvideo.idFile join path on files.idPath=path.idPath join genrelinkmusicvideo on genrelinkmusicvideo.idmvideo=musicvideo.idmvideo where genrelinkmusicvideo.idGenre=%u", idGenre);
    }

    if (idStudio != -1)
    {
      strSQL=FormatSQL("select musicvideo.*,files.strFileName,path.strPath from musicvideo join files on files.idFile=musicvideo.idFile join path on files.idPath=path.idPath join studiolinkmusicvideo on studiolinkmusicvideo.idmvideo=musicvideo.idmvideo where studiolinkmusicvideo.idstudio=%u", idStudio);
    }

    if (idDirector != -1)
    {
      strSQL=FormatSQL("select musicvideo.*,files.strFileName,path.strPath from musicvideo join files on files.idFile=musicvideo.idFile join path on files.idPath=path.idPath join directorlinkmusicvideo on directorlinkmusicvideo.idmvideo=musicvideo.idmvideo where directorlinkmusicvideo.idDirector=%u", idDirector);
    }

    if (idYear !=-1)
    {
      strSQL=FormatSQL("select musicvideo.*,files.strFileName,path.strPath from musicvideo join files on files.idFile=musicvideo.idFile join path on files.idPath=path.idPath where musicvideo.c%02d='%i'",VIDEODB_ID_MUSICVIDEO_YEAR,idYear);
    }

    if (idArtist != -1)
    {
      strSQL=FormatSQL("select musicvideo.*,files.strFileName,path.strPath from musicvideo join files on files.idFile = musicvideo.idFile join path on files.idPath=path.idPath join artistlinkmusicvideo on artistlinkmusicvideo.idmvideo=musicvideo.idmvideo join actors on actors.idActor=artistlinkmusicvideo.idartist where actors.idActor=%u",idArtist);
    }

    // get all songs out of the database in fixed size chunks
    // dont reserve the items ahead of time just in case it fails part way though
    VECMOVIES movies;
    if (idGenre == -1 && idYear == -1 && idArtist == -1 && idDirector == -1 && idStudio == -1)
    {
      int iLIMIT = 5000;    // chunk size
      int iITERATIONS = 0;  // number of iterations
      
      for (int i=0;;i+=iLIMIT)
      {
        CStdString strSQL2=strSQL+FormatSQL(" limit %i offset %i", iLIMIT, i);
        CLog::Log(LOGDEBUG, "CVideoDatabase::GetMusicVideoNav() query: %s", strSQL2.c_str());
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
              return true; // there no more songs left to process (aborts the unbounded for loop)
            }
          }

          CLog::DebugLog("Time for actual SQL query = %d", timeGetTime() - time); time = timeGetTime();
          // get movies from returned subtable
          while (!m_pDS->eof())
          {
            long lMVideoId = m_pDS->fv("musicvideo.idmvideo").get_asLong();            
            CVideoInfoTag movie = GetDetailsForMusicVideo(m_pDS);
            if (g_settings.m_vecProfiles[0].getLockMode() == LOCK_MODE_EVERYONE || g_passwordManager.bMasterUser ||
                g_passwordManager.IsDatabasePathUnlocked(movie.m_strPath,g_settings.m_videoSources))
            {
              CFileItem* pItem=new CFileItem(movie);
              CStdString strDir;
              strDir.Format("%ld", lMVideoId);
              pItem->m_strPath=strBaseDir + strDir;
              pItem->SetOverlayImage(CGUIListItem::ICON_OVERLAY_UNWATCHED,movie.m_bWatched);
              
              items.Add(pItem);
            }
            m_pDS->next();
          }
          CLog::DebugLog("Time to retrieve musicvideos from dataset = %d", timeGetTime() - time);
    		  CLog::Log(LOGDEBUG, __FUNCTION__" times: Info %d, Cast %d", movieTime, castTime);
        }
        catch (...)
        {
          CLog::Log(LOGERROR, "CVideoDatabase::GetMusicVideosNav() failed at iteration %i", iITERATIONS);
        }
        // next iteration
        iITERATIONS++;
        m_pDS->close();
      }
      return true;
    }

    // run query
    CLog::Log(LOGDEBUG, "CVideoDatabase::GetMusicVideosNav() query: %s", strSQL.c_str());
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
      long lMVideoId = m_pDS->fv("musicvideo.idmvideo").get_asLong();
      CVideoInfoTag movie = GetDetailsForMusicVideo(m_pDS);
      if (g_settings.m_vecProfiles[0].getLockMode() == LOCK_MODE_EVERYONE || g_passwordManager.bMasterUser ||
          g_passwordManager.IsDatabasePathUnlocked(movie.m_strPath,g_settings.m_videoSources))
      {
        CFileItem* pItem=new CFileItem(movie);
        CStdString strDir;
        strDir.Format("%ld", lMVideoId);
        pItem->m_strPath=strBaseDir + strDir;
        pItem->SetOverlayImage(CGUIListItem::ICON_OVERLAY_UNWATCHED,movie.m_bWatched);

        items.Add(pItem);
      }

      m_pDS->next();
    }

    CLog::DebugLog("Time to retrieve musicvideos from dataset = %d", timeGetTime() - time);

    // cleanup
    m_pDS->close();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetMusicVideosNav() failed");
  }
  return false;
}

bool CVideoDatabase::GetRecentlyAddedMoviesNav(const CStdString& strBaseDir, CFileItemList& items)
{
  try
  {
    DWORD time = timeGetTime();
    movieTime = 0;
    castTime = 0;

    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL=FormatSQL("select idmovie from movie order by idMovie desc limit %u",RECENTLY_ADDED_LIMIT);

    // run query
    CLog::Log(LOGDEBUG, "CVideoDatabase::GetRecentlyAddedMoviesNav() query: %s", strSQL.c_str());
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return true;
    }
    CStdString strMovies="(";
    while (!m_pDS->eof())
    {
      strMovies += m_pDS->fv(0).get_asString()+",";
      m_pDS->next();
    }
    strMovies[strMovies.size()-1] = ')';

    strSQL = FormatSQL("select movie.*,files.strFileName,path.strPath from movie join files on files.idFile=movie.idFile join path on files.idPath=path.idPath where movie.idMovie in %s order by movie.idMovie desc",strMovies.c_str());
    m_pDS->query(strSQL.c_str());
    CLog::DebugLog("Time for actual SQL query = %d", timeGetTime() - time); time = timeGetTime();

    // get data from returned rows
    items.Reserve(iRowsFound);

    while (!m_pDS->eof())
    {
      long lMovieId = m_pDS->fv("movie.idMovie").get_asLong();
      CVideoInfoTag movie = GetDetailsForMovie(m_pDS);
      if (g_settings.m_vecProfiles[0].getLockMode() == LOCK_MODE_EVERYONE || g_passwordManager.bMasterUser ||
          g_passwordManager.IsDatabasePathUnlocked(movie.m_strPath,g_settings.m_videoSources))
      {
        CFileItem* pItem=new CFileItem(movie);
        CStdString strDir;
        strDir.Format("%ld", lMovieId);
        pItem->m_strPath=strBaseDir + strDir;
        pItem->SetOverlayImage(CGUIListItem::ICON_OVERLAY_UNWATCHED,movie.m_bWatched);

        items.Add(pItem);
      }

      m_pDS->next();
    }

    CLog::DebugLog("Time to retrieve movies from dataset = %d", timeGetTime() - time);

    // cleanup
    m_pDS->close();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetRecentlyAddedMoviesNav() failed");
  }
  return false;
}

bool CVideoDatabase::GetRecentlyAddedEpisodesNav(const CStdString& strBaseDir, CFileItemList& items)
{
  try
  {
    DWORD time = timeGetTime();
    movieTime = 0;
    castTime = 0;

    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    // run query
    CStdString strSQL=FormatSQL("select idepisode from episode order by idEpisode desc limit %u",RECENTLY_ADDED_LIMIT);
    CLog::Log(LOGDEBUG, "CVideoDatabase::GetEpisodesNav() query: %s", strSQL.c_str());
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return true;
    }
    CStdString strEpisodes="(";
    while (!m_pDS->eof())
    {
      strEpisodes += m_pDS->fv(0).get_asString()+",";
      m_pDS->next();
    }
    strEpisodes[strEpisodes.size()-1] = ')';

    strSQL = FormatSQL("select episode.*,files.strFileName,path.strPath,tvshow.c%02d from episode join files on files.idFile=episode.idFile join tvshowlinkepisode on episode.idepisode=tvshowlinkepisode.idepisode join tvshow on tvshow.idshow=tvshowlinkepisode.idshow join path on files.idPath=path.idPath where episode.idepisode in %s order by episode.idEpisode desc",VIDEODB_ID_TV_TITLE,strEpisodes.c_str());
    m_pDS->query(strSQL.c_str());
    CLog::DebugLog("Time for actual SQL query = %d", timeGetTime() - time); time = timeGetTime();

    // get data from returned rows
    items.Reserve(iRowsFound);

    while (!m_pDS->eof())
    {
      long lEpisodeId = m_pDS->fv("episode.idEpisode").get_asLong();

      CVideoInfoTag movie = GetDetailsForEpisode(m_pDS);

      CFileItem* pItem=new CFileItem(movie);
      CStdString strDir;
      strDir.Format("%ld", lEpisodeId);
      pItem->m_strPath=strBaseDir + strDir;
      pItem->SetOverlayImage(CGUIListItem::ICON_OVERLAY_UNWATCHED,movie.m_bWatched);
      pItem->m_dateTime.SetFromDateString(movie.m_strFirstAired);
      pItem->GetVideoInfoTag()->m_iYear = pItem->m_dateTime.GetYear();
      items.Add(pItem);

      m_pDS->next();
    }

    CLog::DebugLog("Time to retrieve movies from dataset = %d", timeGetTime() - time);

    // cleanup
    m_pDS->close();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetEpisodesNav() failed");
  }
  return false;
}

bool CVideoDatabase::GetRecentlyAddedMusicVideosNav(const CStdString& strBaseDir, CFileItemList& items)
{
  try
  {
    DWORD time = timeGetTime();
    movieTime = 0;
    castTime = 0;

    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL=FormatSQL("select idmvideo from musicvideo order by idmvideo desc limit %u",RECENTLY_ADDED_LIMIT);

    // run query
    CLog::Log(LOGDEBUG, "CVideoDatabase::GetRecentlyAddedMusicVideosNav() query: %s", strSQL.c_str());
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return true;
    }
    CStdString strMVideos="(";
    while (!m_pDS->eof())
    {
      strMVideos += m_pDS->fv(0).get_asString()+",";
      m_pDS->next();
    }
    strMVideos[strMVideos.size()-1] = ')';

    strSQL = FormatSQL("select musicvideo.*,files.strFileName,path.strPath from musicvideo join files on files.idFile=musicvideo.idFile join path on files.idPath=path.idPath where musicvideo.idmvideo in %s order by musicvideo.idmvideo desc",strMVideos.c_str());
    m_pDS->query(strSQL.c_str());
    CLog::DebugLog("Time for actual SQL query = %d", timeGetTime() - time); time = timeGetTime();

    // get data from returned rows
    items.Reserve(iRowsFound);

    while (!m_pDS->eof())
    {
      long lMVideoId = m_pDS->fv("musicvideo.idmvideo").get_asLong();
      CVideoInfoTag movie = GetDetailsForMusicVideo(m_pDS);
      if (g_settings.m_vecProfiles[0].getLockMode() == LOCK_MODE_EVERYONE || g_passwordManager.bMasterUser ||
          g_passwordManager.IsDatabasePathUnlocked(movie.m_strPath,g_settings.m_videoSources))
      {
        CFileItem* pItem=new CFileItem(movie);
        CStdString strDir;
        strDir.Format("%ld", lMVideoId);
        pItem->m_strPath=strBaseDir + strDir;
        pItem->SetOverlayImage(CGUIListItem::ICON_OVERLAY_UNWATCHED,movie.m_bWatched);

        items.Add(pItem);
      }

      m_pDS->next();
    }

    CLog::DebugLog("Time to retrieve musicvideos from dataset = %d", timeGetTime() - time);

    // cleanup
    m_pDS->close();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetRecentlyAddedMusicVideosNav() failed");
  }
  return false;
}

bool CVideoDatabase::GetGenreById(long lIdGenre, CStdString& strGenre)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL=FormatSQL("select genre.strGenre from genre where genre.idGenre=%u", lIdGenre);
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
      iResult = m_pDS->fv("nummovies").get_asInteger();

    m_pDS->close();
    return iResult;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetMovieCount() failed");
  }
  return 0;
}

int CVideoDatabase::GetTvShowCount()
{
  try
  {
    if (NULL == m_pDB.get()) return 0;
    if (NULL == m_pDS.get()) return 0;

    CStdString strSQL=FormatSQL("select count (idShow) as numshows from tvshow");
    m_pDS->query( strSQL.c_str() );

    int iResult = 0;
    if (!m_pDS->eof())
      iResult = m_pDS->fv("numshows").get_asInteger();

    m_pDS->close();
    return iResult;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetTvShowCount() failed");
  }
  return 0;
}

int CVideoDatabase::GetMusicVideoCount()
{
  return GetMusicVideoCount("");
}

int CVideoDatabase::GetMusicVideoCount(const CStdString& strWhere)
{
  try
  {
    if (NULL == m_pDB.get()) return 0;
    if (NULL == m_pDS.get()) return 0;

    CStdString strSQL; 
    strSQL.Format("select count (musicvideo.idMVideo) as nummovies from musicvideo join artistlinkmusicvideo on artistlinkmusicvideo.idmvideo = musicvideo.idmvideo join actors on actors.idactor = artistlinkmusicvideo.idartist join files on files.idFile=musicvideo.idFile left outer join genrelinkmusicvideo on genrelinkmusicvideo.idmvideo=musicvideo.idmvideo left outer join genre on genre.idgenre=genrelinkmusicvideo.idgenre %s",strWhere.c_str());
    m_pDS->query( strSQL.c_str() );

    int iResult = 0;
    if (!m_pDS->eof())
      iResult = m_pDS->fv("nummovies").get_asInteger();

    m_pDS->close();
    return iResult;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetMusicVideoCount() failed");
  }
  return 0;
}

bool CVideoDatabase::GetScraperForPath(const CStdString& strPath, SScraperInfo& info)
{
  int iDummy;
  return GetScraperForPath(strPath, info, iDummy);
}

bool CVideoDatabase::GetScraperForPath(const CStdString& strPath, SScraperInfo& info, int& iFound)
{  
  SScanSettings settings;
  return GetScraperForPath(strPath, info, settings, iFound);
}

bool CVideoDatabase::GetScraperForPath(const CStdString& strPath, SScraperInfo& info, SScanSettings& settings)
{
  int iDummy;
  return GetScraperForPath(strPath, info, settings, iDummy);
}

bool CVideoDatabase::GetScraperForPath(const CStdString& strPath, SScraperInfo& info, SScanSettings& settings, int& iFound)
{
  try
  {
    if (strPath.IsEmpty()) return false;
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strPath1(strPath);
    CUtil::AddSlashAtEnd(strPath1);
    long lPathId = GetPath(strPath1);
    if (lPathId > -1)
    {
      CStdString strSQL=FormatSQL("select path.strContent,path.strScraper,path.scanRecursive,path.useFolderNames from path where path.idPath=%u",lPathId);
      m_pDS->query( strSQL.c_str() );
    }

    info.strContent = "";

    iFound = 1;
    if (!m_pDS->eof())
    {
      info.strContent = m_pDS->fv("path.strContent").get_asString();
      info.strPath = m_pDS->fv("path.strScraper").get_asString();
      settings.parent_name = m_pDS->fv("path.useFolderNames").get_asBool();
      settings.recurse = m_pDS->fv("path.scanRecursive").get_asInteger();
    }
    if (info.strContent.IsEmpty())
    {
      CStdString strParent;

      while (CUtil::GetParentPath(strPath1, strParent))
      {
        iFound++;

        CStdString strSQL=FormatSQL("select path.strContent,path.strScraper,path.scanRecursive,path.useFolderNames from path where strPath like '%s'",strParent.c_str());
        m_pDS->query(strSQL.c_str());
        if (!m_pDS->eof())
        {
          info.strContent = m_pDS->fv("path.strContent").get_asString();
          info.strPath = m_pDS->fv("path.strScraper").get_asString();
          settings.parent_name = m_pDS->fv("path.useFolderNames").get_asBool();
          settings.recurse = m_pDS->fv("path.scanRecursive").get_asInteger();

          if (!info.strContent.IsEmpty())
            break;
        }

        strPath1 = strParent;
      }
    }
    m_pDS->close();

    if (info.strContent.Equals("tvshows"))
    {
      settings.recurse = 0;
      if(settings.parent_name) // single show
      {
        settings.parent_name_root = settings.parent_name = (iFound == 1);
        return iFound <= 2; 
      }
      else // show root
      {
        settings.parent_name_root = settings.parent_name = (iFound == 2);
        return iFound <= 3;
      }
    }
    else if (info.strContent.Equals("movies"))
    {
      settings.recurse = settings.recurse - (iFound-1);
      settings.parent_name_root = settings.parent_name && (!settings.recurse || iFound > 1);
      return settings.recurse >= 0;
    }
    else if (info.strContent.Equals("musicvideos"))
    {
      settings.recurse = settings.recurse - (iFound-1);
      settings.parent_name_root = settings.parent_name && (!settings.recurse || iFound > 1);
      return settings.recurse >= 0;
    }
    else
    {
      iFound = 0;
      // this is setup so set content dialog will show correct defaults
      settings.recurse = -1;
      settings.parent_name = false;
      settings.parent_name_root = false;
      return false;
    }

  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetScraperForPath() failed");
  }
  return false;
}

void CVideoDatabase::GetMovieGenresByName(const CStdString& strSearch, CFileItemList& items)
{
  CStdString strSQL;

  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL=FormatSQL("select genre.idgenre,genre.strgenre,path.strPath from genre,genrelinkmovie,movie,path,files where genre.idGenre=genrelinkMovie.idGenre and genrelinkMovie.idMovie = movie.idMovie and files.idFile=movie.idFile and path.idPath = files.idPath and genre.strGenre like '%%%s%%'",strSearch.c_str());
    else
      strSQL=FormatSQL("select distinct genre.idgenre,genre.strgenre from genre,genrelinkmovie where genrelinkmovie.idgenre=genre.idgenre and strGenre like '%%%s%%'", strSearch.c_str());
    m_pDS->query( strSQL.c_str() );

    while (!m_pDS->eof())
    {
      if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv("path.strPath").get_asString()),g_settings.m_videoSources))
        {
          m_pDS->next();
          continue;
        }

      CFileItem* pItem=new CFileItem(m_pDS->fv("genre.strGenre").get_asString());
      CStdString strDir;
      strDir.Format("%ld/", m_pDS->fv("genre.idGenre").get_asLong());
      pItem->m_strPath="videodb://1/1/"+ strDir;
      pItem->m_bIsFolder=true;
      items.Add(pItem);
      m_pDS->next();
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetMovieGenresByName(%s) failed",strSQL.c_str());
  }
}

void CVideoDatabase::GetTvShowGenresByName(const CStdString& strSearch, CFileItemList& items)
{
  CStdString strSQL;

  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL=FormatSQL("select genre.idgenre,genre.strgenre,path.strPath from genre,genrelinktvshow,tvshow,path,tvshowlinkpath where genre.idGenre=genrelinktvshow.idGenre and genrelinktvshow.idshow = tvshow.idshow and path.idPath = tvshowlinkpath.idPath and tvshowlinkpath.idshow = tvshow.idshow and genre.strGenre like '%%%s%%'",strSearch.c_str());
    else
      strSQL=FormatSQL("select distinct genre.idgenre,genre.strgenre from genre,genrelinktvshow where genrelinktvshow.idgenre=genre.idgenre and strGenre like '%%%s%%'", strSearch.c_str());
    m_pDS->query( strSQL.c_str() );

    while (!m_pDS->eof())
    {
      if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv("path.strPath").get_asString()),g_settings.m_videoSources))
        {
          m_pDS->next();
          continue;
        }

      CFileItem* pItem=new CFileItem(m_pDS->fv("genre.strGenre").get_asString());
      CStdString strDir;
      strDir.Format("%ld/", m_pDS->fv("genre.idGenre").get_asLong());
      pItem->m_strPath="videodb://1/1/"+ strDir;
      pItem->m_bIsFolder=true;
      items.Add(pItem);
      m_pDS->next();
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetMovieGenresByName(%s) failed",strSQL.c_str());
  }
}

void CVideoDatabase::GetMovieActorsByName(const CStdString& strSearch, CFileItemList& items)
{
  CStdString strSQL;

  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL=FormatSQL("select actors.idactor,actors.strActor,path.strPath from actorlinkmovie,actors,movie,files,path where actors.idActor=actorlinkmovie.idActor and actorlinkmovie.idmovie=movie.idmovie and files.idFile = movie.idFile and files.idPath = path.idPath and actors.strActor like '%%%s%%'",strSearch.c_str());
    else
      strSQL=FormatSQL("select actors.idactor,actors.strActor from actorlinkmovie,actors,movie where actors.idActor=actorlinkmovie.idActor and actorlinkmovie.idmovie=movie.idmovie and actors.strActor like '%%%s%%'",strSearch.c_str());
    m_pDS->query( strSQL.c_str() );

    while (!m_pDS->eof())
    {
      if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv("path.strPath").get_asString()),g_settings.m_videoSources))
        {
          m_pDS->next();
          continue;
        }

      CFileItem* pItem=new CFileItem(m_pDS->fv("actors.strActor").get_asString());
      CStdString strDir;
      strDir.Format("%ld/", m_pDS->fv("actors.idActor").get_asLong());
      pItem->m_strPath="videodb://1/4/"+ strDir;
      pItem->m_bIsFolder=true;
      items.Add(pItem);
      m_pDS->next();
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, __FUNCTION__"(%s) failed",strSQL.c_str());
  }
}

void CVideoDatabase::GetTvShowsActorsByName(const CStdString& strSearch, CFileItemList& items)
{
  CStdString strSQL;

  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL=FormatSQL("select actors.idactor,actors.strActor,path.strPath from actorlinktvshow,actors,tvshow,path,tvshowlinkpath where actors.idActor=actorlinktvshow.idActor and actorlinktvshow.idshow=tvshow.idshow and tvshowlinkpath.idpath=tvshow.idshow and tvshowlinkpath.idpath=path.idpath and actors.strActor like '%%%s%%'",strSearch.c_str());
    else
      strSQL=FormatSQL("select actors.idactor,actors.strActor from actorlinktvshow,actors,tvshow where actors.idActor=actorlinktvshow.idActor and actorlinktvshow.idshow=tvshow.idshow and actors.strActor like '%%%s%%'",strSearch.c_str());
    m_pDS->query( strSQL.c_str() );

    while (!m_pDS->eof())
    {
      if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv("path.strPath").get_asString()),g_settings.m_videoSources))
        {
          m_pDS->next();
          continue;
        }

      CFileItem* pItem=new CFileItem(m_pDS->fv("actors.strActor").get_asString());
      CStdString strDir;
      strDir.Format("%ld/", m_pDS->fv("actors.idActor").get_asLong());
      pItem->m_strPath="videodb://2/4/"+ strDir;
      pItem->m_bIsFolder=true;
      items.Add(pItem);
      m_pDS->next();
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetTvShowsGenresByName(%s) failed",strSQL.c_str());
  }
}

void CVideoDatabase::GetMusicVideoArtistsByName(const CStdString& strSearch, CFileItemList& items)
{
  CStdString strSQL;

  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    CStdString strLike; 
    if (!strSearch.IsEmpty())
      strLike.Format("and actors.strActor like '%%%s%%'",strSearch.c_str());
    if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL=FormatSQL("select distinct actors.idactor,actors.strActor,path.strPath from artistlinkmusicvideo,actors,musicvideo,files,path where actors.idActor=artistlinkmusicvideo.idartist and artistlinkmusicvideo.idmvideo=musicvideo.idmvideo and files.idFile = musicvideo.idFile and files.idPath = path.idPath %s",strLike.c_str());
    else
      strSQL=FormatSQL("select distinct actors.idactor,actors.strActor from artistlinkmusicvideo,actors,musicvideo where actors.idActor=artistlinkmusicvideo.idartist and artistlinkmusicvideo.idmvideo=musicvideo.idmvideo %s",strLike.c_str());
    m_pDS->query( strSQL.c_str() );

    while (!m_pDS->eof())
    {
      if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv("path.strPath").get_asString()),g_settings.m_videoSources))
        {
          m_pDS->next();
          continue;
        }

      CFileItem* pItem=new CFileItem(m_pDS->fv("actors.strActor").get_asString());
      CStdString strDir;
      strDir.Format("%ld/", m_pDS->fv("actors.idActor").get_asLong());
      pItem->m_strPath="videodb://3/4/"+ strDir;
      pItem->m_bIsFolder=true;
      items.Add(pItem);
      m_pDS->next();
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, __FUNCTION__"(%s) failed",strSQL.c_str());
  }
}

void CVideoDatabase::GetMusicVideoGenresByName(const CStdString& strSearch, CFileItemList& items)
{
  CStdString strSQL;

  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL=FormatSQL("select genre.idgenre,genre.strgenre,path.strPath from genre,genrelinkmusicvideo,musicvideo,path,files where genre.idGenre=genrelinkmusicvideo.idGenre and genrelinkmusicvideo.idmvideo = musicvideo.idmvideo and files.idFile=musicvideo.idFile and path.idPath = files.idPath and genre.strGenre like '%%%s%%'",strSearch.c_str());
    else
      strSQL=FormatSQL("select distinct genre.idgenre,genre.strgenre from genre,genrelinkmusicvideo where genrelinkmusicvideo.idgenre=genre.idgenre and strGenre like '%%%s%%'", strSearch.c_str());
    m_pDS->query( strSQL.c_str() );

    while (!m_pDS->eof())
    {
      if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv("path.strPath").get_asString()),g_settings.m_videoSources))
        {
          m_pDS->next();
          continue;
        }

      CFileItem* pItem=new CFileItem(m_pDS->fv("genre.strGenre").get_asString());
      CStdString strDir;
      strDir.Format("%ld/", m_pDS->fv("genre.idGenre").get_asLong());
      pItem->m_strPath="videodb://3/1/"+ strDir;
      pItem->m_bIsFolder=true;
      items.Add(pItem);
      m_pDS->next();
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetMusicVideoGenresByName(%s) failed",strSQL.c_str());
  }
}

void CVideoDatabase::GetMusicVideoAlbumsByName(const CStdString& strSearch, CFileItemList& items)
{
  CStdString strSQL;

  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    CStdString strLike; 
    if (!strSearch.IsEmpty())
      strLike.Format("and musicvideo.c%02d like '%%%s%%'",VIDEODB_ID_MUSICVIDEO_ALBUM,strSearch.c_str());
    if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL=FormatSQL("select distinct musicvideo.c%02d,musicvideo.idmvideo,path.strPath from musicvideo,files,path where files.idFile = musicvideo.idFile and files.idPath = path.idPath %s",VIDEODB_ID_MUSICVIDEO_ALBUM,strLike.c_str());
    else
    {
      if (!strLike.IsEmpty())
        strLike = "where "+strLike.Mid(4);
      strSQL=FormatSQL("select distinct musicvideo.c%02d,musicvideo.idmvideo from musicvideo %s",VIDEODB_ID_MUSICVIDEO_ALBUM,strLike.c_str());
    }
    m_pDS->query( strSQL.c_str() );

    while (!m_pDS->eof())
    {
      if (m_pDS->fv(0).get_asString().empty())
      {
        m_pDS->next();
        continue;
      }

      if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv("path.strPath").get_asString()),g_settings.m_videoSources))
        {
          m_pDS->next();
          continue;
        }

      CFileItem* pItem=new CFileItem(m_pDS->fv(0).get_asString());
      CStdString strDir;
      strDir.Format("%ld", m_pDS->fv(1).get_asLong());
      pItem->m_strPath="videodb://3/2/"+ strDir;
      pItem->m_bIsFolder=false;
      items.Add(pItem);
      m_pDS->next();
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, __FUNCTION__"(%s) failed",strSQL.c_str());
  }
}

void CVideoDatabase::GetMusicVideosByAlbum(const CStdString& strSearch, CFileItemList& items)
{
  CStdString strSQL;

  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL = FormatSQL("select musicvideo.idmvideo,musicvideo.c%02d,musicvideo.c%02d,path.strPath from musicvideo,file,path where file.idfile=musicvideo.idfile and file.idPath=path.idPath and musicvideo.%c02d like '%%%s%%'",VIDEODB_ID_MUSICVIDEO_ALBUM,VIDEODB_ID_MUSICVIDEO_TITLE,VIDEODB_ID_MUSICVIDEO_ALBUM,strSearch.c_str());
    else
      strSQL = FormatSQL("select musicvideo.idmvideo,musicvideo.c%02d,musicvideo.c%02d from musicvideo where musicvideo.c%02d like '%%%s%%'",VIDEODB_ID_MUSICVIDEO_ALBUM,VIDEODB_ID_MUSICVIDEO_TITLE,VIDEODB_ID_MUSICVIDEO_ALBUM,strSearch.c_str());
    m_pDS->query( strSQL.c_str() );

    while (!m_pDS->eof())
    {
      if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv("path.strPath").get_asString()),g_settings.m_videoSources))
        {
          m_pDS->next();
          continue;
        }

      CFileItem* pItem=new CFileItem(m_pDS->fv(1).get_asString()+" - "+m_pDS->fv(2).get_asString());
      CStdString strDir;
      strDir.Format("3/2/%ld",m_pDS->fv("musicvideo.idmvideo").get_asLong());
      
      pItem->m_strPath="videodb://"+ strDir;
      pItem->m_bIsFolder=false;
      items.Add(pItem);
      m_pDS->next();
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetMusicVideosByAlbums(%s) failed",strSQL.c_str());
  }
}

bool CVideoDatabase::GetMusicVideosByWhere(const CStdString &baseDir, const CStdString &whereClause, CFileItemList &items)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    // We don't use FormatSQL here, as the WHERE clause is already formatted.
    CStdString strSQL = "select distinct musicvideo.*,files.strFileName,path.strPath from musicvideo join files on files.idFile=musicvideo.idFile join path on files.idPath=path.idPath left outer join genrelinkmusicvideo on genrelinkmusicvideo.idmvideo=musicvideo.idmvideo left outer join genre on genre.idgenre = genrelinkmusicvideo.idgenre join artistlinkmusicvideo on artistlinkmusicvideo.idmvideo=musicvideo.idmvideo join actors on actors.idactor = artistlinkmusicvideo.idartist " + whereClause;
    CLog::Log(LOGDEBUG, __FUNCTION__" query = %s", strSQL.c_str());
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
      CFileItem *item = new CFileItem(GetDetailsForMusicVideo(m_pDS));
      item->m_strPath.Format("%s%ld",baseDir,m_pDS->fv("musicvideo.idmvideo").get_asLong());
      items.Add(item);
      m_pDS->next();
    }

    // cleanup
    m_pDS->close();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, __FUNCTION__"(%s) failed", whereClause.c_str());
  }
  return false;
}

unsigned int CVideoDatabase::GetMusicVideoIDs(const CStdString& strWhere, vector<pair<int,long> > &songIDs)
{
  try
  {
    if (NULL == m_pDB.get()) return 0;
    if (NULL == m_pDS.get()) return 0;

    CStdString strSQL = "select distinct musicvideo.idmvideo from musicvideo join genrelinkmusicvideo on genrelinkmusicvideo.idmvideo=musicvideo.idmvideo join genre on genre.idgenre = genrelinkmusicvideo.idgenre join artistlinkmusicvideo on artistlinkmusicvideo.idmvideo=musicvideo.idmvideo join actors on actors.idactor = artistlinkmusicvideo.idartist " + strWhere;
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
      songIDs.push_back(make_pair<int,long>(2,m_pDS->fv(0).get_asLong()));
      m_pDS->next();
    }    // cleanup
    m_pDS->close();
    return songIDs.size();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, __FUNCTION__"(%s) failed", strWhere.c_str());
  }
  return 0;
}

bool CVideoDatabase::GetRandomMusicVideo(CFileItem* item, long& lSongId, const CStdString& strWhere)
{
  try
  {
    lSongId = -1;

    // seed random function
    srand(timeGetTime());

    int iCount = GetMusicVideoCount(strWhere);
    if (iCount <= 0)
      return false;
    int iRandom = rand() % iCount;

    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    // We don't use FormatSQL here, as the WHERE clause is already formatted.
    CStdString strSQL;
    strSQL.Format("select musicvideo.*,files.strFileName,path.strPath from musicvideo join files on files.idFile=musicvideo.idFile join path on files.idPath=path.idPath join artistlinkmusicvideo on artistlinkmusicvideo.idmvideo = musicvideo.idmvideo join actors on actors.idactor = artistlinkmusicvideo.idartist join genrelinkmusicvideo on genrelinkmusicvideo.idmvideo=musicvideo.idmvideo %s order by musicvideo.idMVideo limit 1 offset %i",strWhere.c_str(),iRandom);
    CLog::Log(LOGDEBUG, __FUNCTION__" query = %s", strSQL.c_str());
    // run query
    if (!m_pDS->query(strSQL.c_str()))
      return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound != 1)
    {
      m_pDS->close();
      return false;
    }
    *item->GetVideoInfoTag() = GetDetailsForMusicVideo(m_pDS);
    item->m_strPath.Format("videodb://3/2/%ld",item->GetVideoInfoTag()->m_iDbId);
    lSongId = m_pDS->fv("musicvideo.idmvideo").get_asLong();
    item->SetLabel(item->GetVideoInfoTag()->m_strTitle);
    m_pDS->close();
    return true;
  }
  catch(...)
  {
    CLog::Log(LOGERROR, __FUNCTION__"(%s) failed", strWhere.c_str());
  }
  return false;
}

long CVideoDatabase::GetMusicVideoArtistByName(const CStdString& strArtist)
{
  CStdString strSQL;

  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL=FormatSQL("select distinct actors.idactor,path.strPath from artistlinkmusicvideo,actors,musicvideo,files,path where actors.idActor=artistlinkmusicvideo.idartist and artistlinkmusicvideo.idmvideo=musicvideo.idmvideo and files.idFile = musicvideo.idFile and files.idPath = path.idPath and actors.strActor like '%s'",strArtist.c_str());
    else
      strSQL=FormatSQL("select distinct actors.idactor from artistlinkmusicvideo,actors,musicvideo where actors.idActor=artistlinkmusicvideo.idartist and artistlinkmusicvideo.idmvideo=musicvideo.idmvideo and actors.strActor like '%s'",strArtist.c_str());
    m_pDS->query( strSQL.c_str() );
    if (m_pDS->eof())
      return -1;

    if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      if (!g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv("path.strPath").get_asString()),g_settings.m_videoSources))
      {
        m_pDS->close();
        return -1;
      }

    long lResult = m_pDS->fv(0).get_asLong();
    m_pDS->close();
    return lResult;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, __FUNCTION__"(%s) failed",strSQL.c_str());
  }
  return -1;
}

void CVideoDatabase::GetMoviesByName(const CStdString& strSearch, CFileItemList& items)
{
  CStdString strSQL;

  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL = FormatSQL("select movie.idmovie,movie.c%02d,path.strPath from movie,file,path where file.idfile=movie.idfile and file.idPath=path.idPath and movie.c%02d like '%%%s%%'",VIDEODB_ID_TITLE,VIDEODB_ID_TITLE,strSearch.c_str());
    else
      strSQL = FormatSQL("select movie.idmovie,movie.c%02d from movie where movie.c%02d like '%%%s%%'",VIDEODB_ID_TITLE,VIDEODB_ID_TITLE,strSearch.c_str());
    m_pDS->query( strSQL.c_str() );

    while (!m_pDS->eof())
    {
      if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv("path.strPath").get_asString()),g_settings.m_videoSources))
        {
          m_pDS->next();
          continue;
        }

      CFileItem* pItem=new CFileItem(m_pDS->fv(1).get_asString());
      CStdString strDir;
      strDir.Format("1/2/%ld",m_pDS->fv("movie.idMovie").get_asLong());
      
      pItem->m_strPath="videodb://"+ strDir;
      pItem->m_bIsFolder=false;
      items.Add(pItem);
      m_pDS->next();
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetTitlesByName(%s) failed",strSQL.c_str());
  }
}

void CVideoDatabase::GetTvShowsByName(const CStdString& strSearch, CFileItemList& items)
{
  CStdString strSQL;

  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL = FormatSQL("select tvshow.idshow,tvshow.c%02d,path.strPath from tvshow,path,tvshowlinkpath where tvshowlinkpath.idpath=path.idpath and tvshow.c%02d like '%%%s%%'",VIDEODB_ID_TV_TITLE,VIDEODB_ID_TV_TITLE,strSearch.c_str());
    else
      strSQL = FormatSQL("select tvshow.idshow,tvshow.c%02d from tvshow where tvshow.c%02d like '%%%s%%'",VIDEODB_ID_TV_TITLE,VIDEODB_ID_TV_TITLE,strSearch.c_str());
    m_pDS->query( strSQL.c_str() );

    while (!m_pDS->eof())
    {
      if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv("path.strPath").get_asString()),g_settings.m_videoSources))
        {
          m_pDS->next();
          continue;
        }

      CFileItem* pItem=new CFileItem(m_pDS->fv(1).get_asString());
      CStdString strDir;
      strDir.Format("2/2/%ld/", m_pDS->fv("tvshow.idshow").get_asLong());
      
      pItem->m_strPath="videodb://"+ strDir;
      pItem->m_bIsFolder=true;
      items.Add(pItem);
      m_pDS->next();
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetTitlesByName(%s) failed",strSQL.c_str());
  }
}

void CVideoDatabase::GetEpisodesByName(const CStdString& strSearch, CFileItemList& items)
{
  CStdString strSQL;

  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL = FormatSQL("select episode.idepisode,episode.c%02d,episode.c%02d,tvshowlinkepisode.idshow,tvshow.c%02d,path.strPath from episode,file,path,tvshowlinkepisode,tvshow where file.idfile=episode.idfile and tvshowlinkepisode.idepisode=episode.idepisode and tvshowlinkepisode.idshow=tvshow.idshow and file.idPath=path.idPath and episode.c%02d like '%%%s%%'",VIDEODB_ID_EPISODE_TITLE,VIDEODB_ID_EPISODE_SEASON,VIDEODB_ID_TV_TITLE,VIDEODB_ID_EPISODE_TITLE,strSearch.c_str());
    else
      strSQL = FormatSQL("select episode.idepisode,episode.c%02d,episode.c%02d,tvshowlinkepisode.idshow,tvshow.c%02d from episode,tvshowlinkepisode,tvshow where tvshowlinkepisode.idepisode=episode.idepisode and tvshow.idshow=tvshowlinkepisode.idshow and episode.c%02d like '%%%s%%'",VIDEODB_ID_EPISODE_TITLE,VIDEODB_ID_EPISODE_SEASON,VIDEODB_ID_TV_TITLE,VIDEODB_ID_EPISODE_TITLE,strSearch.c_str());
    m_pDS->query( strSQL.c_str() );

    while (!m_pDS->eof())
    {
      if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv("path.strPath").get_asString()),g_settings.m_videoSources))
        {
          m_pDS->next();
          continue;
        }

      CFileItem* pItem=new CFileItem(m_pDS->fv(1).get_asString()+" ("+m_pDS->fv(4).get_asString()+")");
      pItem->m_strPath.Format("videodb://2/2/%ld/%ld/%ld",m_pDS->fv("tvshowlinkepisode.idshow").get_asLong(),m_pDS->fv(2).get_asLong(),m_pDS->fv(0).get_asLong());
      pItem->m_bIsFolder=false;
      items.Add(pItem);
      m_pDS->next();
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetEpisodesByName(%s) failed",strSQL.c_str());
  }
}

void CVideoDatabase::GetMusicVideosByName(const CStdString& strSearch, CFileItemList& items)
{
  CStdString strSQL;

  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL = FormatSQL("select musicvideo.idmvideo,musicvideo.c%02d,path.strPath from musicvideo,file,path where file.idfile=musicvideo.idfile and file.idPath=path.idPath and musicvideo.%c02d like '%%%s%%'",VIDEODB_ID_MUSICVIDEO_TITLE,VIDEODB_ID_MUSICVIDEO_TITLE,strSearch.c_str());
    else
      strSQL = FormatSQL("select musicvideo.idmvideo,musicvideo.c%02d from musicvideo where musicvideo.c%02d like '%%%s%%'",VIDEODB_ID_MUSICVIDEO_TITLE,VIDEODB_ID_MUSICVIDEO_TITLE,strSearch.c_str());
    m_pDS->query( strSQL.c_str() );

    while (!m_pDS->eof())
    {
      if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv("path.strPath").get_asString()),g_settings.m_videoSources))
        {
          m_pDS->next();
          continue;
        }

      CFileItem* pItem=new CFileItem(m_pDS->fv(1).get_asString());
      CStdString strDir;
      strDir.Format("3/1/%ld",m_pDS->fv("musicvideo.idmvideo").get_asLong());
      
      pItem->m_strPath="videodb://"+ strDir;
      pItem->m_bIsFolder=false;
      items.Add(pItem);
      m_pDS->next();
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetMusicVideosByName(%s) failed",strSQL.c_str());
  }
}

long CVideoDatabase::GetMusicVideoByArtistAndAlbumAndTitle(const CStdString& strArtist, const CStdString& strAlbum, const CStdString& strTitle)
{
  CStdString strSQL;

  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL = FormatSQL("select musicvideo.idmvideo from musicvideo,file,path,artistlinkmusicvideo,actors where file.idfile=musicvideo.idfile and file.idPath=path.idPath and musicvideo.%c02d like '%s' and musicvideo.%c02d like '%s' and artistlinkmusicvideo.idmvideo=musicvideo.idmvideo and artistlinkmusicvideo.idartist = actors.idactors and actors.strActor like '%s'",VIDEODB_ID_MUSICVIDEO_ALBUM,strAlbum.c_str(),VIDEODB_ID_MUSICVIDEO_TITLE,strTitle.c_str(),strArtist.c_str());
    else
      strSQL = FormatSQL("select musicvideo.idmvideo from musicvideo join artistlinkmusicvideo on artistlinkmusicvideo.idmvideo=musicvideo.idmvideo join actors on actors.idactor=artistlinkmusicvideo.idartist where musicvideo.c%02d like '%s' and musicvideo.c%02d like '%s' and actors.strActor like '%s'",VIDEODB_ID_MUSICVIDEO_ALBUM,strAlbum.c_str(),VIDEODB_ID_MUSICVIDEO_TITLE,strTitle.c_str(),strArtist.c_str());
    m_pDS->query( strSQL.c_str() );

    if (m_pDS->eof())
      return -1;

    if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      if (!g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv("path.strPath").get_asString()),g_settings.m_videoSources))
      {
        m_pDS->close();
        return -1;
      }
    
    long lResult = m_pDS->fv(0).get_asLong();
    m_pDS->close();
    return lResult;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetMusicVideosByName(%s) failed",strSQL.c_str());
  }
  return -1;
}

void CVideoDatabase::GetEpisodesByPlot(const CStdString& strSearch, CFileItemList& items)
{
  CStdString strSQL;

  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL = FormatSQL("select episode.idepisode,episode.c%02d,episode,c%02d,tvshowlinkepisode.idshow,tvshow.c%02d,path.strPath from episode,file,path,tvshowlinkepisode,tvshow where file.idepisode=episode.idepisode and tvshowlinkepisode.idepisode=episode.idepisode and file.idPath=path.idPath and tvshow.idshow=tvshowlinkepisode.idshow and episode.c%02d like '%%%s%%'",VIDEODB_ID_EPISODE_TITLE,VIDEODB_ID_EPISODE_SEASON,VIDEODB_ID_TV_TITLE,VIDEODB_ID_EPISODE_PLOT,strSearch.c_str());
    else
      strSQL = FormatSQL("select episode.idepisode,episode.c%02d,episode.c%02d,tvshowlinkepisode.idshow,tvshow.c%02d from episode,tvshowlinkepisode,tvshow where tvshowlinkepisode.idepisode=episode.idepisode and tvshow.idshow=tvshowlinkepisode.idshow and episode.c%02d like '%%%s%%'",VIDEODB_ID_EPISODE_TITLE,VIDEODB_ID_EPISODE_SEASON,VIDEODB_ID_TV_TITLE,VIDEODB_ID_EPISODE_PLOT,strSearch.c_str());
    m_pDS->query( strSQL.c_str() );

    while (!m_pDS->eof())
    {
      if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv("path.strPath").get_asString()),g_settings.m_videoSources))
        {
          m_pDS->next();
          continue;
        }

      CFileItem* pItem=new CFileItem(m_pDS->fv(1).get_asString()+" ("+m_pDS->fv(4).get_asString()+")");
      pItem->m_strPath.Format("videodb://2/2/%ld/%ld/%ld",m_pDS->fv("tvshowlinkepisode.idshow").get_asLong(),m_pDS->fv(2).get_asLong(),m_pDS->fv(0).get_asLong());
      pItem->m_bIsFolder=false;
      items.Add(pItem);
      m_pDS->next();
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetEpisodesByPlot(%s) failed",strSQL.c_str());
  }
}

void CVideoDatabase::GetMovieDirectorsByName(const CStdString& strSearch, CFileItemList& items)
{
  CStdString strSQL;

  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL = FormatSQL("select directorlinkmovie.idDirector,actors.strActor,path.strPath from movie,file,path,actors,directorlinkmovie where file.idfile=movie.idfile and file.idPath=path.idPath and directorlinkmovie.idmovie = movie.idMovie and directorlinkmovie.idDirector = actors.idActor and actors.strActor like '%%%s%%'",strSearch.c_str());
    else
      strSQL = FormatSQL("select directorlinkmovie.idDirector,actors.strActor from movie,actors,directorlinkmovie where directorlinkmovie.idmovie = movie.idMovie and directorlinkmovie.idDirector = actors.idActor and actors.strActor like '%%%s%%'",strSearch.c_str());

    m_pDS->query( strSQL.c_str() );

    while (!m_pDS->eof())
    {
      if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv("path.strPath").get_asString()),g_settings.m_videoSources))
        {
          m_pDS->next();
          continue;
        }

      CStdString strDir;
      strDir.Format("%ld/", m_pDS->fv("directorlinkmovie.idDirector").get_asLong());
      CFileItem* pItem=new CFileItem(m_pDS->fv("actors.strActor").get_asString());
      
      pItem->m_strPath="videodb://1/5/"+ strDir;
      pItem->m_bIsFolder=true;
      items.Add(pItem);
      m_pDS->next();
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, __FUNCTION__"(%s) failed",strSQL.c_str());
  }
}

void CVideoDatabase::GetTvShowsDirectorsByName(const CStdString& strSearch, CFileItemList& items)
{
  CStdString strSQL;

  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL = FormatSQL("select directorlinktvshow.idDirector,actors.strActor,path.strPath from tvshow,path,actors,directorlinktvshow,tvshowlinkpath where tvshowlinkpath.idPath=path.idPath and tvshowlinkpath.idshow=tvshow.idshow and directorlinktvshow.idshow = tvshow.idshow and directorlinktvshow.idDirector = actors.idActor and actors.strActor like '%%%s%%'",strSearch.c_str());
    else
      strSQL = FormatSQL("select directorlinktvshow.idDirector,actors.strActor from tvshow,actors,directorlinktvshow where directorlinktvshow.idshow = tvshow.idshow and directorlinktvshow.idDirector = actors.idActor and actors.strActor like '%%%s%%'",strSearch.c_str());

    m_pDS->query( strSQL.c_str() );

    while (!m_pDS->eof())
    {
      if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv("path.strPath").get_asString()),g_settings.m_videoSources))
        {
          m_pDS->next();
          continue;
        }

      CStdString strDir;
      strDir.Format("%ld/", m_pDS->fv("directorlinktvshow.idDirector").get_asLong());
      CFileItem* pItem=new CFileItem(m_pDS->fv("actors.strActor").get_asString());
      
      pItem->m_strPath="videodb://2/5/"+ strDir;
      pItem->m_bIsFolder=true;
      items.Add(pItem);
      m_pDS->next();
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, __FUNCTION__"(%s) failed",strSQL.c_str());
  }
}

void CVideoDatabase::GetMusicVideoDirectorsByName(const CStdString& strSearch, CFileItemList& items)
{
  CStdString strSQL;

  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL = FormatSQL("select directorlinkmusicvideo.idDirector,actors.strActor,path.strPath from musicvideo,file,path,actors,directorlinkmusicvideo where file.idfile=musicvideo.idfile and file.idPath=path.idPath and directorlinkmusicvideo.idmvideo = musicvideo.idmvideo and directorlinkmusicvideo.idDirector = actors.idActor and actors.strActor like '%%%s%%'",strSearch.c_str());
    else
      strSQL = FormatSQL("select directorlinkmusicvideo.idDirector,actors.strActor from musicvideo,actors,directorlinkmusicvideo where directorlinkmusicvideo.idmvideo = musicvideo.idmvideo and directorlinkmusicvideo.idDirector = actors.idActor and actors.strActor like '%%%s%%'",strSearch.c_str());

    m_pDS->query( strSQL.c_str() );

    while (!m_pDS->eof())
    {
      if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv("path.strPath").get_asString()),g_settings.m_videoSources))
        {
          m_pDS->next();
          continue;
        }

      CStdString strDir;
      strDir.Format("%ld/", m_pDS->fv("directorlinkmusicvideo.idDirector").get_asLong());
      CFileItem* pItem=new CFileItem(m_pDS->fv("actors.strActor").get_asString());
      
      pItem->m_strPath="videodb://3/5/"+ strDir;
      pItem->m_bIsFolder=true;
      items.Add(pItem);
      m_pDS->next();
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, __FUNCTION__"(%s) failed",strSQL.c_str());
  }
}

void CVideoDatabase::CleanDatabase(IVideoInfoScannerObserver* pObserver)
{
  try
  {
    BeginTransaction();
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    // find all the files
    CStdString sql = "select * from files, path where files.idPath = path.idPath";
 
    m_pDS->query(sql.c_str());
    if (m_pDS->num_rows() == 0) return;

    CGUIDialogProgress *progress=NULL;
    if (!pObserver)
    {
      progress = (CGUIDialogProgress *)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
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
    }
    else
    {
      pObserver->OnDirectoryChanged("");
      pObserver->OnSetCurrentProgress(0,1);
      pObserver->OnStateChanged(CLEANING_UP_DATABASE);
    }

    CStdString filesToDelete = "(";
    CStdString moviesToDelete = "(";
    CStdString episodesToDelete = "(";
    CStdString musicVideosToDelete = "(";
    int total = m_pDS->num_rows();
    int current = 0;
    while (!m_pDS->eof())
    {
      CStdString path = m_pDS->fv("path.strPath").get_asString();
      CStdString fileName = m_pDS->fv("files.strFileName").get_asString();
      CStdString fullPath;
      CUtil::AddFileToFolder(path, fileName, fullPath);

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
          url.GetProtocol() == "https" ||
          url.GetProtocol() == "ftp" ||
          url.GetProtocol() == "ftps" ||
          !CFile::Exists(fullPath))
      { // mark for deletion
        filesToDelete += m_pDS->fv("files.idFile").get_asString() + ",";
      }
      if (!pObserver)
      {
        if (progress)
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
      }
      else
        pObserver->OnSetProgress(current,total);
      
      m_pDS->next();
      current++;
    }
    m_pDS->close();
    filesToDelete.TrimRight(",");
    filesToDelete += ")";
    // now grab them movies
    sql = FormatSQL("select idMovie from movie where idFile in %s",filesToDelete.c_str());
    m_pDS->query(sql.c_str());
    while (!m_pDS->eof())
    {
      moviesToDelete += m_pDS->fv(0).get_asString() + ",";
      m_pDS->next();
    }
    m_pDS->close();
    // now grab them episodes
    sql = FormatSQL("select idEpisode from episode where idFile in %s",filesToDelete.c_str());
    m_pDS->query(sql.c_str());
    while (!m_pDS->eof())
    {
      episodesToDelete += m_pDS->fv(0).get_asString() + ",";
      m_pDS->next();
    }
    m_pDS->close();

    // now grab them musicvideos
    sql = FormatSQL("select idMVideo from musicvideo where idFile in %s",filesToDelete.c_str());
    m_pDS->query(sql.c_str());
    while (!m_pDS->eof())
    {
      musicVideosToDelete += m_pDS->fv(0).get_asString() + ",";
      m_pDS->next();
    }
    m_pDS->close();

    moviesToDelete.TrimRight(",");
    moviesToDelete += ")";
    episodesToDelete.TrimRight(",");
    episodesToDelete += ")";
    musicVideosToDelete.TrimRight(",");
    musicVideosToDelete += ")";

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

    CLog::Log(LOGDEBUG, __FUNCTION__" Cleaning movie table");
    sql = "delete from movie where idMovie in " + moviesToDelete;
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, __FUNCTION__" Cleaning actorlinkmovie table");
    sql = "delete from actorlinkmovie where idMovie in " + moviesToDelete;
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, __FUNCTION__" Cleaning directorlinkmovie table");
    sql = "delete from directorlinkmovie where idMovie in " + moviesToDelete;
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, __FUNCTION__" Cleaning genrelinkmovie table");
    sql = "delete from genrelinkmovie where idMovie in " + moviesToDelete;
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, __FUNCTION__" Cleaning studiolinkmovie table");
    sql = "delete from studiolinkmovie where idMovie in " + moviesToDelete;
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, __FUNCTION__" Cleaning episode table");
    sql = "delete from episode where idEpisode in " + episodesToDelete;
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, __FUNCTION__" Cleaning actorlinkepisode table");
    sql = "delete from actorlinkepisode where idEpisode in " + episodesToDelete;
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, __FUNCTION__" Cleaning directorlinkepisode table");
    sql = "delete from directorlinkepisode where idEpisode in " + episodesToDelete;
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, __FUNCTION__" Cleaning tvshowlinkepisode table");
    sql = "delete from tvshowlinkepisode where idEpisode in " + episodesToDelete;
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, __FUNCTION__" Cleaning genrelinkepisode table");
    sql = "delete from genrelinkepisode where idEpisode in " + episodesToDelete;
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, __FUNCTION__" Cleaning tvshow table");
    sql = "delete from tvshow where idshow not in (select distinct idShow from tvshowlinkepisode)";
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, __FUNCTION__" Cleaning actorlinktvshow table");
    sql = "delete from actorlinktvshow where idShow not in (select distinct idShow from tvshowlinkepisode)";
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, __FUNCTION__" Cleaning directorlinktvshow table");
    sql = "delete from directorlinktvshow where idShow not in (select distinct idShow from tvshowlinkepisode)";
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, __FUNCTION__" Cleaning tvshowlinkpath table");
    sql = "delete from tvshowlinkpath where idshow not in (select distinct idShow from tvshowlinkepisode)";
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, __FUNCTION__" Cleaning genrelinktvshow table");
    sql = "delete from genrelinktvshow where idShow not in (select distinct idShow from tvshowlinkepisode)";
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, __FUNCTION__" Cleaning movielinktvshow table");
    sql = "delete from movielinktvshow where idShow not in (select distinct idShow from tvshowlinkepisode)";
    m_pDS->exec(sql.c_str());
    sql = "delete from movielinktvshow where idMovie not in (select distinct idMovie from movie)";
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, __FUNCTION__" Updating episode counts");
    m_pDS->query("select idshow from tvshow");
    while (!m_pDS->eof())
    {
      CStdString strSQL=FormatSQL("update tvshow set c%02d=(select count(idEpisode) from tvshowlinkepisode where idshow=%u) where idshow=%u",VIDEODB_ID_TV_EPISODES,m_pDS->fv("tvshow.idshow").get_asInteger(),m_pDS->fv("tvshow.idshow").get_asInteger());
      m_pDS2->exec(strSQL.c_str());
      m_pDS->next();
    }

    CLog::Log(LOGDEBUG, __FUNCTION__" Cleaning musicvideo table");
    sql = "delete from musicvideo where idmvideo in " + musicVideosToDelete;
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, __FUNCTION__" Cleaning artistlinkmusicvideo table");
    sql = "delete from artistlinkmusicvideo where idMVideo in " + musicVideosToDelete;
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, __FUNCTION__" Cleaning directorlinkmusicvideo table");
    sql = "delete from directorlinkmusicvideo where idMVideo in " + musicVideosToDelete;
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, __FUNCTION__" Cleaning genrelinkmusicvideo table");
    sql = "delete from genrelinkmusicvideo where idMVideo in " + musicVideosToDelete;
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, __FUNCTION__" Cleaning studiolinkmusicvideo table");
    sql = "delete from studiolinkmusicvideo where idMVideo in " + musicVideosToDelete;
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, __FUNCTION__" Cleaning path table");
    sql = "delete from path where idPath not in (select distinct idPath from files) and idPath not in (select distinct idPath from tvshowlinkpath) and strContent=''";
    m_pDS->exec(sql.c_str());
    sql = "select * from path where strContent not like ''";
    m_pDS->query(sql.c_str());
    CStdString strIds;
    while (!m_pDS->eof())
    {
      if (!CDirectory::Exists(m_pDS->fv("path.strPath").get_asString()))
        strIds.Format("%s %u,",strIds.Mid(0),m_pDS->fv("path.idPath").get_asLong()); // mid since we cannot format the same string
      m_pDS->next();
    }
    if (!strIds.IsEmpty())
    {
      strIds.TrimLeft(" ");
      strIds.TrimRight(",");
      sql = FormatSQL("delete from path where idpath in (%s)",strIds.c_str());
      m_pDS->exec(sql.c_str());
    }

    CLog::Log(LOGDEBUG, __FUNCTION__" Cleaning genre table");
    sql = "delete from genre where idGenre not in (select distinct idGenre from genrelinkmovie) and idGenre not in (select distinct idGenre from genrelinktvshow) and idGenre not in (select distinct idGenre from genrelinkepisode) and idGenre not in (select distinct idGenre from genrelinkmusicvideo)";
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, __FUNCTION__" Cleaning actor table of actors and directors");
    sql = "delete from actors where idActor not in (select distinct idActor from actorlinkmovie) and idActor not in (select distinct idDirector from directorlinkmovie) and idActor not in (select distinct idActor from actorlinktvshow) and idActor not in (select distinct idActor from actorlinkepisode) and idActor not in (select distinct idDirector from directorlinktvshow) and idActor not in (select distinct idDirector from directorlinkepisode) and idActor not in (select distinct idArtist from artistlinkmusicvideo) and idActor not in (select distinct idDirector from directorlinkmusicvideo)";
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, __FUNCTION__" Cleaning studio table");
    sql = "delete from studio where idStudio not in (select distinct idStudio from studiolinkmovie) and idStudio not in (select distinct idStudio from studiolinkmusicvideo)";
    m_pDS->exec(sql.c_str());

    CommitTransaction();

    Compress();

    CUtil::DeleteVideoDatabaseDirectoryCache();

    if (progress)
      progress->Close(); 
 }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::CleanDatabase() failed");
  }
}

void CVideoDatabase::ExportToXML(const CStdString &xmlFile, bool singleFiles /* = false */)
{
  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;
    if (NULL == m_pDS2.get()) return;

    // create a 3rd dataset as well as GetEpisodeDetails() etc. uses m_pDS2, and we need to do 3 nested queries on tv shows
    auto_ptr<Dataset> pDS;
    pDS.reset(m_pDB->CreateDataset());
    if (NULL == pDS.get()) return;

    // find all movies
    CStdString sql = "select movie.*,files.strFileName,path.strPath from movie join files on files.idFile=movie.idFile join path on files.idPath=path.idPath";
 
    m_pDS->query(sql.c_str());

    CGUIDialogProgress *progress = (CGUIDialogProgress *)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
    if (progress)
    {
      progress->SetHeading(647);
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
      TiXmlElement xmlMainElement("videodb");
      pMain = xmlDoc.InsertEndChild(xmlMainElement);
    }
    while (!m_pDS->eof())
    {
      CVideoInfoTag movie = GetDetailsForMovie(m_pDS, true);
      movie.Save(pMain, "movie", !singleFiles);
      if (singleFiles)
      {
        CStdString nfoFile;
        CUtil::ReplaceExtension(movie.m_strFileNameAndPath, ".nfo", nfoFile);
        xmlDoc.SaveFile(nfoFile.c_str());
        xmlDoc.Clear();
        TiXmlDeclaration decl("1.0", "UTF-8", "yes");
        xmlDoc.InsertEndChild(decl);
      }
      if ((current % 50) == 0 && progress)
      {
        progress->SetLine(1, movie.m_strTitle);
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

    // find all musicvideos
    sql = "select musicvideo.*,files.strFileName,path.strPath from musicvideo join files on files.idFile=musicvideo.idFile join path on files.idPath=path.idPath";
 
    m_pDS->query(sql.c_str());

    total = m_pDS->num_rows();
    current = 0;

    while (!m_pDS->eof())
    {
      CVideoInfoTag movie = GetDetailsForMusicVideo(m_pDS);
      movie.Save(pMain, "musicvideo", !singleFiles);
      if (singleFiles)
      {
        CStdString nfoFile;
        CUtil::ReplaceExtension(movie.m_strFileNameAndPath, ".nfo", nfoFile);
        xmlDoc.SaveFile(nfoFile.c_str());
        xmlDoc.Clear();
        TiXmlDeclaration decl("1.0", "UTF-8", "yes");
        xmlDoc.InsertEndChild(decl);
      }
      if ((current % 50) == 0 && progress)
      {
        progress->SetLine(1, movie.m_strTitle);
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

    if (singleFiles)
    { // don't support tv shows at present
      if (progress)
        progress->Close();
      return;
    }

    // repeat for all tvshows
    sql = "select tvshow.*,path.strPath from tvshow join tvshowlinkpath on tvshow.idShow=tvshowlinkpath.idShow join path on tvshowlinkpath.idPath=path.idPath";

    m_pDS->query(sql.c_str());

    if (progress)
    {
      progress->SetHeading(647);
      progress->SetLine(0, 650);
      progress->SetLine(1, "");
      progress->SetLine(2, "");
      progress->SetPercentage(0);
      progress->StartModal();
      progress->ShowProgressBar(true);
    }

    total = m_pDS->num_rows();
    current = 0;

    while (!m_pDS->eof())
    {
      CVideoInfoTag tvshow = GetDetailsForTvShow(m_pDS, true);
      tvshow.Save(pMain, "tvshow");
      // now save the episodes from this show
      sql = FormatSQL("select episode.*,files.strFileName,path.strPath,tvshow.c%02d from episode join files on files.idFile=episode.idfile join tvshowlinkepisode on episode.idepisode=tvshowlinkepisode.idepisode join tvshow on tvshow.idshow=tvshowlinkepisode.idshow join path on files.idPath=path.idPath where tvshowlinkepisode.idShow=%i",VIDEODB_ID_TV_TITLE,tvshow.m_iDbId);
      pDS->query(sql.c_str());

      while (!pDS->eof())
      {
        CVideoInfoTag episode = GetDetailsForEpisode(pDS, true);
        episode.Save(pMain->LastChild(), "episodedetails");
        pDS->next();
      }
      pDS->close();

      if ((current % 50) == 0 && progress)
      {
        progress->SetLine(1, tvshow.m_strTitle);
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

    if (progress)
    {
      progress->SetHeading(647);
      progress->SetLine(0, 650);
      progress->SetLine(1, "");
      progress->SetLine(2, "");
      progress->SetPercentage(0);
      progress->StartModal();
      progress->ShowProgressBar(true);
    }

    // now dump path info
    map<CStdString,SScanSettings> paths;
    GetPaths(paths);
    TiXmlElement xmlPathElement("paths");
    TiXmlNode *pPaths = pMain->InsertEndChild(xmlPathElement);
    for( map<CStdString,SScanSettings>::iterator iter=paths.begin();iter != paths.end();++iter)
    {
      SScraperInfo info;
      int iFound=0;
      if (GetScraperForPath(iter->first,info,iFound) && iFound == 1)
      {
        TiXmlElement xmlPathElement2("path");
        TiXmlNode *pPath = pPaths->InsertEndChild(xmlPathElement2);
        XMLUtils::SetString(pPath,"url",iter->first);
        XMLUtils::SetInt(pPath,"scanrecursive",iter->second.recurse);
        XMLUtils::SetBoolean(pPath,"usefoldernames",iter->second.parent_name);
        XMLUtils::SetString(pPath,"content",info.strContent);
        XMLUtils::SetString(pPath,"scraperpath",info.strPath);
      }
    }

    if (progress)
      progress->Close();

    xmlDoc.SaveFile(xmlFile.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, __FUNCTION__" failed");
  }
}

void CVideoDatabase::ImportFromXML(const CStdString &xmlFile)
{
  CGUIDialogProgress *progress = (CGUIDialogProgress *)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
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
      progress->SetHeading(648);
      progress->SetLine(0, 649);
      progress->SetLine(1, 330);
      progress->SetLine(2, "");
      progress->SetPercentage(0);
      progress->StartModal();
      progress->ShowProgressBar(true);
    }

    TiXmlElement *movie = root->FirstChildElement();
    int current = 0;
    int total = 0;
    // first count the number of items...
    while (movie)
    {
      if (strnicmp(movie->Value(), "movie", 5)==0 ||
          strnicmp(movie->Value(), "tvshow", 6)==0 ||
          strnicmp(movie->Value(), "musicvideo",10)==0 )
        total++;
      movie = movie->NextSiblingElement();
    }

    BeginTransaction();
    movie = root->FirstChildElement();
    while (movie)
    {
      CVideoInfoTag info;
      if (strnicmp(movie->Value(), "movie", 5) == 0)
      { 
        info.Load(movie);
        SetDetailsForMovie(info.m_strFileNameAndPath, info);
        current++;
      }
      else if (strnicmp(movie->Value(), "musicvideo", 10) == 0)
      { 
        info.Load(movie);
        SetDetailsForMusicVideo(info.m_strFileNameAndPath, info);
        current++;
      }
      else if (strnicmp(movie->Value(), "tvshow", 6) == 0)
      {
        // load the TV show in.  NOTE: This deletes all episodes under the TV Show, which may not be
        // what we desire.  It may make better sense to only delete (or even better, update) the show information
        info.Load(movie);
        CUtil::AddSlashAtEnd(info.m_strPath);
        DeleteTvShow(info.m_strPath);
        long showID = SetDetailsForTvShow(info.m_strPath, info);
        current++;
        // now load the episodes
        TiXmlElement *episode = movie->FirstChildElement("episodedetails");
        while (episode)
        {
          // no need to delete the episode info, due to the above deletion
          CVideoInfoTag info;
          info.Load(episode);
          long lEpisodeId = AddEpisode(showID,info.m_strFileNameAndPath); // do this here due to multi episode files
          SetDetailsForEpisode(info.m_strFileNameAndPath, info, showID, lEpisodeId);
          episode = episode->NextSiblingElement("episodedetails");
        }
      }
      else if (strnicmp(movie->Value(), "paths", 5) == 0)
      {
        const TiXmlElement* path = movie->FirstChildElement("path");
        while (path)
        {
          CStdString strPath;
          XMLUtils::GetString(path,"url",strPath);
          SScraperInfo info;
          SScanSettings settings;
          XMLUtils::GetString(path,"content",info.strContent);
          XMLUtils::GetString(path,"scraperpath",info.strPath);
          XMLUtils::GetInt(path,"scanrecursive",settings.recurse);
          XMLUtils::GetBoolean(path,"usefoldernames",settings.parent_name);
          SetScraperForPath(strPath,info,settings);
          path = path->NextSiblingElement();
        }
      }
      movie = movie->NextSiblingElement();
      if (progress && total)
      {
        progress->SetPercentage(current * 100 / total);
        progress->SetLine(2, info.m_strTitle);
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
  }
  catch (...)
  {
    CLog::Log(LOGERROR, __FUNCTION__" failed");
  }
  if (progress)
    progress->Close();
}

bool CVideoDatabase::GetArbitraryQuery(const CStdString& strQuery, const CStdString& strOpenRecordSet, const CStdString& strCloseRecordSet,
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
    int iRowsFound = m_pDS->num_rows();
    strResult=strOpenRecordSet;
    while (!m_pDS->eof())
    {
      strResult += strOpenRecord;
      for (int i=0; i<m_pDS->fieldCount(); i++)
      {
        strResult += strOpenField + m_pDS->fv(i).get_asString() + strCloseField;
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
    CLog::Log(LOGERROR, "CVideoDatabase:GetArbitraryQuery() failed");
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
