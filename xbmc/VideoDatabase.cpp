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

#include "stdafx.h"
#include "VideoDatabase.h"
#include "GUIWindowVideoBase.h"
#include "utils/fstrcmp.h"
#include "utils/RegExp.h"
#include "utils/GUIInfoManager.h"
#include "Util.h"
#include "XMLUtils.h"
#include "GUIPassword.h"
#include "FileSystem/StackDirectory.h"
#include "FileSystem/MultiPathDirectory.h"
#include "VideoInfoScanner.h"
#include "GUIWindowManager.h"
#include "FileSystem/Directory.h"
#include "FileSystem/File.h"
#include "GUIDialogProgress.h"
#include "FileItem.h"

using namespace std;
using namespace dbiplus;
using namespace XFILE;
using namespace DIRECTORY;
using namespace VIDEO;

#define VIDEO_DATABASE_VERSION 23
#define VIDEO_DATABASE_OLD_VERSION 3.f
#define VIDEO_DATABASE_NAME "MyVideos34.db"
#define RECENTLY_ADDED_LIMIT  25

CBookmark::CBookmark()
{
  episodeNumber = 0;
  seasonNumber = 0;
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

    CLog::Log(LOGINFO, "create writerlinkmovie table");
    m_pDS->exec("CREATE TABLE writerlinkmovie ( idWriter integer, idMovie integer)\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_writerlinkmovie_1 ON writerlinkmovie ( idWriter, idMovie )\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_writerlinkmovie_2 ON writerlinkmovie ( idMovie, idWriter )\n");

    CLog::Log(LOGINFO, "create actors table");
    m_pDS->exec("CREATE TABLE actors ( idActor integer primary key, strActor text, strThumb text )\n");

    CLog::Log(LOGINFO, "create path table");
    m_pDS->exec("CREATE TABLE path ( idPath integer primary key, strPath text, strContent text, strScraper text, strHash text, scanRecursive integer, useFolderNames bool, strSettings text, noUpdate bool)\n");
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
    m_pDS->exec("CREATE TABLE directorlinkepisode ( idDirector integer, idEpisode integer)\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_directorlinkepisode_1 ON directorlinkepisode ( idDirector, idEpisode )\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_directorlinkepisode_2 ON directorlinkepisode ( idEpisode, idDirector )\n");

    CLog::Log(LOGINFO, "create writerlinkepisode table");
    m_pDS->exec("CREATE TABLE writerlinkepisode ( idWriter integer, idEpisode integer)\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_writerlinkepisode_1 ON writerlinkepisode ( idWriter, idEpisode )\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_writerlinkepisode_2 ON writerlinkepisode ( idEpisode, idWriter )\n");

    CLog::Log(LOGINFO, "create genrelinktvshow table");
    m_pDS->exec("CREATE TABLE genrelinktvshow ( idGenre integer, idShow integer)\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_genrelinktvshow_1 ON genrelinktvshow ( idGenre, idShow)\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_genrelinktvshow_2 ON genrelinktvshow ( idShow, idGenre)\n");

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

    CLog::Log(LOGINFO, "create tvshowview");
    CStdString showview=FormatSQL("create view tvshowview as select tvshow.*,path.strPath as strPath,"
                                    "counts.totalcount as totalCount,counts.watchedcount as watchedCount,"
                                    "counts.totalcount=counts.watchedcount as watched from tvshow "
                                    "join tvshowlinkpath on tvshow.idShow=tvshowlinkpath.idShow "
                                    "join path on path.idpath=tvshowlinkpath.idPath "
                                    "left outer join ("
                                    "    select tvshow.idShow as idShow,count(1) as totalcount,count(episode.c%02d) as watchedcount from tvshow "
                                    "    join tvshowlinkepisode on tvshow.idShow = tvshowlinkepisode.idShow "
                                    "    join episode on episode.idEpisode = tvshowlinkepisode.idEpisode "
                                    "    group by tvshow.idShow"
                                    ") counts on tvshow.idShow = counts.idShow",VIDEODB_ID_EPISODE_PLAYCOUNT);
    m_pDS->exec(showview.c_str());

    CLog::Log(LOGINFO, "create episodeview");
    CStdString episodeview = FormatSQL("create view episodeview as select episode.*,files.strFileName as strFileName,"
                                        "path.strPath as strPath,tvshow.c%02d as strTitle,tvshow.idShow as idShow,"
                                        "tvshow.c%02d as premiered from episode "
                                        "join files on files.idFile=episode.idFile "
                                        "join tvshowlinkepisode on episode.idepisode=tvshowlinkepisode.idepisode "
                                        "join tvshow on tvshow.idshow=tvshowlinkepisode.idshow "
                                        "join path on files.idPath=path.idPath",VIDEODB_ID_TV_TITLE, VIDEODB_ID_TV_PREMIERED);
    m_pDS->exec(episodeview.c_str());

    CLog::Log(LOGINFO, "create musicvideoview");
    m_pDS->exec("create view musicvideoview as select musicvideo.*,files.strFileName as strFileName,path.strPath as strPath "
                "from musicvideo join files on files.idFile=musicvideo.idFile join path on path.idPath=files.idPath");

    CLog::Log(LOGINFO, "create movieview");
    m_pDS->exec("create view movieview as select movie.*,files.strFileName as strFileName,path.strPath as strPath "
                "from movie join files on files.idFile=movie.idFile join path on path.idPath=files.idPath");
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s unable to create tables:%i", __FUNCTION__, (int)GetLastError());
    return false;
  }

  return true;
}

//********************************************************************************************************************************
long CVideoDatabase::GetPathId(const CStdString& strPath)
{
  CStdString strSQL;
  try
  {
    long lPathId=-1;
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    CStdString strPath1(strPath);
    if (CUtil::IsStack(strPath) || strPath.Mid(0,6).Equals("rar://") || strPath.Mid(0,6).Equals("zip://"))
      CUtil::GetParentPath(strPath,strPath1);

    CUtil::AddSlashAtEnd(strPath1);

    strSQL=FormatSQL("select idPath from path where strPath like '%s'",strPath1.c_str());
    m_pDS->query(strSQL.c_str());
    if (!m_pDS->eof())
      lPathId = m_pDS->fv("path.idPath").get_asLong();

    m_pDS->close();
    return lPathId;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s unable to getpath (%s)", __FUNCTION__, strSQL.c_str());
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

    memset(&settings, 0, sizeof(settings));

    // grab all paths with movie content set
    if (!m_pDS->query("select strPath,scanRecursive,useFolderNames,noUpdate from path"
                      " where (strContent = 'movies' or strContent = 'musicvideos')"
                      " and strPath NOT like 'multipath://%%'"
                      " order by strPath"))
      return false;

    while (!m_pDS->eof())
    {
      if (!m_pDS->fv("noUpdate").get_asBool())
      {
        CStdString strPath = m_pDS->fv("strPath").get_asString();

        settings.parent_name = m_pDS->fv("useFolderNames").get_asBool();
        settings.recurse     = m_pDS->fv("scanRecursive").get_asInteger();
        settings.parent_name_root = settings.parent_name && !settings.recurse;

        paths.insert(pair<CStdString,SScanSettings>(strPath,settings));
      }
      m_pDS->next();
    }
    m_pDS->close();

    // then grab all tvshow paths
    if (!m_pDS->query("select strPath,scanRecursive,useFolderNames,strContent,noUpdate from path"
                      " where ( strContent = 'tvshows'"
                      "       or idPath in (select idpath from tvshowlinkpath))"
                      " and strPath NOT like 'multipath://%%'"
                      " order by strPath"))
      return false;

    while (!m_pDS->eof())
    {
      if (!m_pDS->fv("noUpdate").get_asBool())
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
      }
      m_pDS->next();
    }
    m_pDS->close();

    // finally grab all other paths holding a movie which is not a stack or a rar archive
    // - this isnt perfect but it should do fine in most situations.
    // reason we need it to hold a movie is stacks from different directories (cdx folders for instance)
    // not making mistakes must take priority
    if (!m_pDS->query("select strPath,noUpdate from path"
                       " where idPath in (select idPath from files join movie on movie.idFile=files.idFile)"
                       " and idPath NOT in (select idpath from tvshowlinkpath)"
                       " and idPath NOT in (select idpath from files where strFileName like 'video_ts.ifo')" // dvdfolders get stacked to a single item in parent folder
                       " and strPath NOT like 'multipath://%%'"
                       " and strContent NOT in ('movies', 'tvshows', 'None')" // these have been added above
                       " order by strPath"))

      return false;
    while (!m_pDS->eof())
    {
      if (!m_pDS->fv("noUpdate").get_asBool())
      {
        CStdString strPath = m_pDS->fv("strPath").get_asString();

        settings.parent_name = false;
        settings.recurse = 0;
        settings.parent_name_root = settings.parent_name && !settings.recurse;

        paths.insert(pair<CStdString,SScanSettings>(strPath,settings));
      }
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

long CVideoDatabase::AddPath(const CStdString& strPath)
{
  CStdString strSQL;
  try
  {
    long lPathId;
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    CStdString strPath1(strPath);
    if (CUtil::IsStack(strPath) || strPath.Mid(0,6).Equals("rar://") || strPath.Mid(0,6).Equals("zip://"))
      CUtil::GetParentPath(strPath,strPath1);

    CUtil::AddSlashAtEnd(strPath1);

    strSQL=FormatSQL("insert into path (idPath, strPath, strContent, strScraper) values (NULL,'%s','','')", strPath1.c_str());
    m_pDS->exec(strSQL.c_str());
    lPathId = (long)sqlite3_last_insert_rowid( m_pDB->getHandle() );
    return lPathId;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s unable to addpath (%s)", __FUNCTION__, strSQL.c_str());
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
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, path.c_str());
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
    SplitPath(strFileNameAndPath,strPath,strFileName);

    long lPathId=GetPathId(strPath);
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
    CLog::Log(LOGERROR, "%s unable to addfile (%s)", __FUNCTION__, strSQL.c_str());
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
    long pathId = GetPathId(path);
    if (pathId < 0)
      pathId = AddPath(path);
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

bool CVideoDatabase::LinkMovieToTvshow(long idMovie, long idShow, bool bRemove)
{
   try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    if (bRemove) // delete link
    {
      CStdString strSQL=FormatSQL("delete from movielinktvshow where idMovie=%u and idShow=%u", idMovie, idShow);
      m_pDS->exec(strSQL.c_str());
      return true;
    }

    CStdString strSQL=FormatSQL("insert into movielinktvshow (idShow,idMovie) values (%u,%u)", idShow,idMovie);
    m_pDS->exec(strSQL.c_str());

    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%ld, %ld) failed", __FUNCTION__, idMovie, idShow);
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
    CLog::Log(LOGERROR, "%s (%ld) failed", __FUNCTION__, idMovie);
  }

  return false;
}

bool CVideoDatabase::GetLinksToTvShow(long idMovie, vector<long>& ids)
{
   try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL=FormatSQL("select * from movielinktvshow where idMovie=%u", idMovie);
    m_pDS->query(strSQL.c_str());
    while (!m_pDS->eof())
    {
      ids.push_back(m_pDS->fv(1).get_asLong());
      m_pDS->next();
    }

    m_pDS->close();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%ld) failed", __FUNCTION__, idMovie);
  }

  return false;
}


//********************************************************************************************************************************
long CVideoDatabase::GetFileId(const CStdString& strFilenameAndPath)
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;
    CStdString strPath, strFileName;
    SplitPath(strFilenameAndPath,strPath,strFileName);

    long lPathId = GetPathId(strPath);
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
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strFilenameAndPath.c_str());
  }
  return -1;
}

//********************************************************************************************************************************
long CVideoDatabase::GetMovieId(const CStdString& strFilenameAndPath)
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;
    long lMovieId = -1;

    // needed for query parameters
    long lFileId = GetFileId(strFilenameAndPath);
    long lPathId=-1;
    CStdString strPath, strFile;
    if (lFileId < 0)
    {
      SplitPath(strFilenameAndPath,strPath,strFile);

      // have to join movieinfo table for correct results
      lPathId = GetPathId(strPath);
      if (lPathId < 0 && strPath != strFilenameAndPath)
        return -1;
    }

    if (lFileId == -1 && strPath != strFilenameAndPath)
      return -1;

    CStdString strSQL;
    if (lFileId == -1)
      strSQL=FormatSQL("select idMovie from movie join files on files.idFile=movie.idFile where files.idpath = %u",lPathId);
    else
      strSQL=FormatSQL("select idMovie from movie where idFile=%u", lFileId);

    CLog::Log(LOGDEBUG, "%s (%s), query = %s", __FUNCTION__, strFilenameAndPath.c_str(), strSQL.c_str());
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() > 0)
      lMovieId = m_pDS->fv("idMovie").get_asLong();
    m_pDS->close();

    return lMovieId;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strFilenameAndPath.c_str());
  }
  return -1;
}

long CVideoDatabase::GetTvShowId(const CStdString& strPath)
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;
    long lTvShowId = -1;

    // have to join movieinfo table for correct results
    long lPathId = GetPathId(strPath);
    if (lPathId < 0)
      return -1;

    CStdString strSQL;
    CStdString strPath1=strPath;
    CStdString strParent;
    int iFound=0;

    strSQL=FormatSQL("select idShow from tvshowlinkpath where tvshowlinkpath.idPath=%u",lPathId);
    m_pDS->query(strSQL);
    if (!m_pDS->eof())
      iFound = 1;

    while (iFound == 0 && CUtil::GetParentPath(strPath1, strParent))
    {
      strSQL=FormatSQL("select idShow from path,tvshowlinkpath where tvshowlinkpath.idpath = path.idpath and strPath like '%s'",strParent.c_str());
      m_pDS->query(strSQL.c_str());
      if (!m_pDS->eof())
      {
        long idShow = m_pDS->fv("idShow").get_asLong();
        if (idShow != -1)
          iFound = 2;
      }
      strPath1 = strParent;
    }

    if (m_pDS->num_rows() > 0)
      lTvShowId = m_pDS->fv("idShow").get_asLong();
    m_pDS->close();

    return lTvShowId;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strPath.c_str());
  }
  return -1;
}

long CVideoDatabase::GetEpisodeId(const CStdString& strFilenameAndPath, long lEpisodeId, long lSeasonId) // input value is episode/season number hint - for multiparters
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    // need this due to the nested GetEpisodeInfo query
    auto_ptr<Dataset> pDS;
    pDS.reset(m_pDB->CreateDataset());
    if (NULL == pDS.get()) return -1;

    long lFileId = GetFileId(strFilenameAndPath);
    if (lFileId < 0)
      return -1;

    CStdString strSQL=FormatSQL("select idEpisode from episode where idFile=%u", lFileId);

    CLog::Log(LOGDEBUG, "%s (%s), query = %s", __FUNCTION__, strFilenameAndPath.c_str(), strSQL.c_str());
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
          long lTmpEpisodeId = pDS->fv("episode.idEpisode").get_asLong();
          GetEpisodeInfo(strFilenameAndPath,tag,lTmpEpisodeId);
          if (tag.m_iEpisode == lEpisodeId && (lSeasonId == -1 || tag.m_iSeason == lSeasonId)) {
            // match on the episode hint, and there's no season hint or a season hint match
            lEpisodeId = lTmpEpisodeId;
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
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strFilenameAndPath.c_str());
  }
  return -1;
}

long CVideoDatabase::GetMusicVideoId(const CStdString& strFilenameAndPath)
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    long lFileId = GetFileId(strFilenameAndPath);
    if (lFileId < 0)
      return -1;

    CStdString strSQL=FormatSQL("select idMVideo from musicvideo where idFile=%u", lFileId);

    CLog::Log(LOGDEBUG, "%s (%s), query = %s", __FUNCTION__, strFilenameAndPath.c_str(), strSQL.c_str());
    m_pDS->query(strSQL.c_str());
    long lMVideoId=-1;
    if (m_pDS->num_rows() > 0)
      lMVideoId = m_pDS->fv("idMVideo").get_asLong();
    m_pDS->close();

    return lMVideoId;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strFilenameAndPath.c_str());
  }
  return -1;
}

//********************************************************************************************************************************
long CVideoDatabase::AddMovie(const CStdString& strFilenameAndPath)
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    long lFileId, lMovieId=-1;
    lFileId = GetFileId(strFilenameAndPath);
    if (lFileId < 0)
      lFileId = AddFile(strFilenameAndPath);
    else
      lMovieId = GetMovieId(strFilenameAndPath);
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
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strFilenameAndPath.c_str());
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

    long lPathId = GetPathId(strPath);
    if (lPathId < 0)
      lPathId = AddPath(strPath);
    strSQL=FormatSQL("insert into tvshowlinkpath values (%u,%u)",lTvShow,lPathId);
    m_pDS->exec(strSQL.c_str());

//    CommitTransaction();

    return lTvShow;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strPath.c_str());
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
    lFileId = GetFileId(strFilenameAndPath);
    if (lFileId < 0)
      lFileId = AddFile(strFilenameAndPath);

    CStdString strSQL=FormatSQL("insert into episode (idEpisode, idFile) values (NULL, %u)", lFileId);
    m_pDS->exec(strSQL.c_str());
    lEpisodeId = (long)sqlite3_last_insert_rowid(m_pDB->getHandle());

    strSQL=FormatSQL("insert into tvshowlinkepisode (idShow,idEpisode) values (%i,%i)",idShow,lEpisodeId);
    m_pDS->exec(strSQL.c_str());
//    CommitTransaction();

    return lEpisodeId;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strFilenameAndPath.c_str());
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
    lFileId = GetFileId(strFilenameAndPath);
    if (lFileId < 0)
      lFileId = AddFile(strFilenameAndPath);
    else
      lMVideoId = GetMusicVideoId(strFilenameAndPath);
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
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strFilenameAndPath.c_str());
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
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strGenre.c_str() );
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
      if (!strThumb.IsEmpty())
        strSQL=FormatSQL("update actors set strThumb='%s' where idActor=%u",strThumb.c_str(),lActorId);
      m_pDS->close();
      return lActorId;
    }

  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strActor.c_str() );
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
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strStudio.c_str() );
  }

  return -1;
}

void CVideoDatabase::AddLinkToActor(const char *table, long actorID, const char *secondField, long secondID, const CStdString &role)
{
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    CStdString strSQL=FormatSQL("select * from %s where idActor=%u and %s=%u", table, actorID, secondField, secondID);
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() == 0)
    {
      // doesnt exists, add it
      strSQL=FormatSQL("insert into %s (idActor, %s, strRole) values(%i,%i,'%s')", table, secondField, actorID, secondID, role.c_str());
      m_pDS->exec(strSQL.c_str());
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
}

void CVideoDatabase::AddToLinkTable(const char *table, const char *firstField, long firstID, const char *secondField, long secondID)
{
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    CStdString strSQL=FormatSQL("select * from %s where %s=%u and %s=%u", table, firstField, firstID, secondField, secondID);
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() == 0)
    {
      // doesnt exists, add it
      strSQL=FormatSQL("insert into %s (%s,%s) values(%i,%i)", table, firstField, secondField, firstID, secondID);
      m_pDS->exec(strSQL.c_str());
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
}

//****Actors****
void CVideoDatabase::AddActorToMovie(long lMovieId, long lActorId, const CStdString& strRole)
{
  AddLinkToActor("actorlinkmovie", lActorId, "idMovie", lMovieId, strRole);
}

void CVideoDatabase::AddActorToTvShow(long lTvShowId, long lActorId, const CStdString& strRole)
{
  AddLinkToActor("actorlinktvshow", lActorId, "idShow", lTvShowId, strRole);
}

void CVideoDatabase::AddActorToEpisode(long lEpisodeId, long lActorId, const CStdString& strRole)
{
  AddLinkToActor("actorlinkepisode", lActorId, "idEpisode", lEpisodeId, strRole);
}

void CVideoDatabase::AddArtistToMusicVideo(long lMVideoId, long lArtistId)
{
  AddToLinkTable("artistlinkmusicvideo", "idArtist", lArtistId, "idMVideo", lMVideoId);
}

//****Directors + Writers****
void CVideoDatabase::AddDirectorToMovie(long lMovieId, long lDirectorId)
{
  AddToLinkTable("directorlinkmovie", "idDirector", lDirectorId, "idMovie", lMovieId);
}

void CVideoDatabase::AddDirectorToTvShow(long lTvShowId, long lDirectorId)
{
  AddToLinkTable("directorlinktvshow", "idDirector", lDirectorId, "idShow", lTvShowId);
}

void CVideoDatabase::AddWriterToEpisode(long lEpisodeId, long lWriterId)
{
  AddToLinkTable("writerlinkepisode", "idWriter", lWriterId, "idEpisode", lEpisodeId);
}

void CVideoDatabase::AddWriterToMovie(long lMovieId, long lWriterId)
{
  AddToLinkTable("writerlinkmovie", "idWriter", lWriterId, "idMovie", lMovieId);
}

void CVideoDatabase::AddDirectorToEpisode(long lEpisodeId, long lDirectorId)
{
  AddToLinkTable("directorlinkepisode", "idDirector", lDirectorId, "idEpisode", lEpisodeId);
}

void CVideoDatabase::AddDirectorToMusicVideo(long lMVideoId, long lDirectorId)
{
  AddToLinkTable("directorlinkmusicvideo", "idDirector", lDirectorId, "idMVideo", lMVideoId);
}

//****Studios****
void CVideoDatabase::AddStudioToMovie(long lMovieId, long lStudioId)
{
  AddToLinkTable("studiolinkmovie", "idStudio", lStudioId, "idMovie", lMovieId);
}

void CVideoDatabase::AddStudioToMusicVideo(long lMVideoId, long lStudioId)
{
  AddToLinkTable("studiolinkmusicvideo", "idStudio", lStudioId, "idMVideo", lMVideoId);
}

//****Genres****
void CVideoDatabase::AddGenreToMovie(long lMovieId, long lGenreId)
{
  AddToLinkTable("genrelinkmovie", "idGenre", lGenreId, "idMovie", lMovieId);
}

void CVideoDatabase::AddGenreToTvShow(long lTvShowId, long lGenreId)
{
  AddToLinkTable("genrelinktvshow", "idGenre", lGenreId, "idShow", lTvShowId);
}

void CVideoDatabase::AddGenreToMusicVideo(long lMVideoId, long lGenreId)
{
  AddToLinkTable("genrelinkmusicvideo", "idGenre", lGenreId, "idMVideo", lMVideoId);
}

//********************************************************************************************************************************
bool CVideoDatabase::HasMovieInfo(const CStdString& strFilenameAndPath)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    long lMovieId = GetMovieId(strFilenameAndPath);
    return (lMovieId > 0); // index of zero is also invalid

    // work in progress
    if (lMovieId > 0)
    {
      // get title.  if no title, the id was "deleted" for in-place update
      CVideoInfoTag details;
      GetMovieInfo(strFilenameAndPath, details, lMovieId);
      if (!details.m_strTitle.IsEmpty()) return true;
    }
    return false;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strFilenameAndPath.c_str());
  }
  return false;
}

bool CVideoDatabase::HasTvShowInfo(const CStdString& strPath)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    long lTvShowId = GetTvShowId(strPath);
    return (lTvShowId > 0); // index of zero is also invalid

    // work in progress
    if (lTvShowId > 0)
    {
      // get title. if no title, the id was "deleted" for in-place update
      CVideoInfoTag details;
      GetTvShowInfo(strPath, details, lTvShowId);
      if (!details.m_strTitle.IsEmpty()) return true;
    }
    return false;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strPath.c_str());
  }
  return false;
}

bool CVideoDatabase::HasEpisodeInfo(const CStdString& strFilenameAndPath)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    long lEpisodeId = GetEpisodeId(strFilenameAndPath);
    return (lEpisodeId > 0); // index of zero is also invalid

    // work in progress
    if (lEpisodeId > 0)
    {
      // get title.  if no title, the id was "deleted" for in-place update
      CVideoInfoTag details;
      GetEpisodeInfo(strFilenameAndPath, details, lEpisodeId);
      if (!details.m_strTitle.IsEmpty()) return true;
    }
    return false;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strFilenameAndPath.c_str());
  }
  return false;
}

bool CVideoDatabase::HasMusicVideoInfo(const CStdString& strFilenameAndPath)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    long lMVideoId = GetMusicVideoId(strFilenameAndPath);
    return (lMVideoId > 0); // index of zero is also invalid

    // work in progress
    if (lMVideoId > 0)
    {
      // get title.  if no title, the id was "deleted" for in-place update
      CVideoInfoTag details;
      GetMusicVideoInfo(strFilenameAndPath, details, lMVideoId);
      if (!details.m_strTitle.IsEmpty()) return true;
    }
    return false;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strFilenameAndPath.c_str());
  }
  return false;
}

void CVideoDatabase::DeleteDetailsForTvShow(const CStdString& strPath)
{// TODO: merge into DeleteTvShow
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    long lTvShowId = GetTvShowId(strPath);
    if ( lTvShowId < 0) return ;

    CFileItemList items;
    CStdString strPath2;
    strPath2.Format("videodb://2/2/%u/",lTvShowId);
    GetSeasonsNav(strPath2,items,-1,-1,-1,-1,lTvShowId);
    for( int i=0;i<items.Size();++i )
      XFILE::CFile::Delete(items[i]->GetCachedSeasonThumb());

    DeleteThumbForItem(strPath,true);

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
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strPath.c_str());
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

    CStdString strSQL=FormatSQL("select * from movieview join actorlinkmovie on actorlinkmovie.idmovie=movieview.idmovie join actors on actors.idActor=actorlinkmovie.idActor where actors.stractor='%s'", strActor.c_str());
    m_pDS->query( strSQL.c_str() );

    while (!m_pDS->eof())
    {
      movies.push_back(GetDetailsForMovie(m_pDS));
      m_pDS->next();
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strActor.c_str());
  }
}

void CVideoDatabase::GetTvShowsByActor(const CStdString& strActor, VECMOVIES& movies)
{
  try
  {
    movies.erase(movies.begin(), movies.end());
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    CStdString strSQL = FormatSQL("select * from tvshowview join actorlinktvshow on actorlinktvshow.idshow=tvshowview.idshow "
                                  "join actors on actors.idActor=actorlinktvshow.idActor "
                                  "where actors.stractor='%s'", strActor.c_str());

    m_pDS->query( strSQL.c_str() );

    while (!m_pDS->eof())
    {
      movies.push_back(GetDetailsForTvShow(m_pDS));
      m_pDS->next();
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strActor.c_str());
  }
}

void CVideoDatabase::GetEpisodesByActor(const CStdString& strActor, VECMOVIES& movies)
{
  try
  {
    movies.erase(movies.begin(), movies.end());
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    CStdString strSQL=FormatSQL("select * from episodeview join actorlinkepisode on actorlinkepisode.idepisode=episodeview.idepisode join actors on actors.idActor=actorlinkepisode.idActor where actors.stractor='%s'", strActor.c_str());
    m_pDS->query( strSQL.c_str() );

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
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strActor.c_str());
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
    if (strArtist.IsEmpty())  // TODO: SMARTPLAYLISTS what is this here for???
      strSQL=FormatSQL("select distinct * from musicvideoview join artistlinkmusicvideo on artistlinkmusicvideo.idmvideo=musicvideoview.idmvideo join actors on actors.idActor=artistlinkmusicvideo.idArtist");
    else
      strSQL=FormatSQL("select * from musicvideoview join artistlinkmusicvideo on artistlinkmusicvideo.idmvideo=musicvideoview.idmvideo join actors on actors.idActor=artistlinkmusicvideo.idArtist where actors.stractor='%s'", strArtist.c_str());
    m_pDS->query( strSQL.c_str() );

    while (!m_pDS->eof())
    {
      CVideoInfoTag tag = GetDetailsForMusicVideo(m_pDS);
      CFileItemPtr pItem(new CFileItem(tag));
      pItem->SetLabel(tag.m_strArtist);
      items.Add(pItem);
      m_pDS->next();
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strArtist.c_str());
  }
}

//********************************************************************************************************************************
void CVideoDatabase::GetMovieInfo(const CStdString& strFilenameAndPath, CVideoInfoTag& details, long lMovieId /* = -1 */)
{
  try
  {
    // TODO: Optimize this - no need for all the queries!
    if (lMovieId < 0)
      lMovieId = GetMovieId(strFilenameAndPath);
    if (lMovieId < 0) return ;

    CStdString sql = FormatSQL("select * from movieview where idMovie=%i", lMovieId);
    if (!m_pDS->query(sql.c_str()))
      return;
    details = GetDetailsForMovie(m_pDS, true);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strFilenameAndPath.c_str());
  }
}

//********************************************************************************************************************************
void CVideoDatabase::GetTvShowInfo(const CStdString& strPath, CVideoInfoTag& details, long lTvShowId /* = -1 */)
{
  try
  {
    if (lTvShowId < 0)
      lTvShowId = GetTvShowId(strPath);
    if (lTvShowId < 0) return ;

    CStdString sql = FormatSQL("select * from tvshowview where idshow=%i", lTvShowId);
    if (!m_pDS->query(sql.c_str()))
      return;
    details = GetDetailsForTvShow(m_pDS, true);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strPath.c_str());
  }
}

bool CVideoDatabase::GetEpisodeInfo(const CStdString& strFilenameAndPath, CVideoInfoTag& details, long lEpisodeId /* = -1 */)
{
  try
  {
    // TODO: Optimize this - no need for all the queries!
    if (lEpisodeId < 0)
      lEpisodeId = GetEpisodeId(strFilenameAndPath);
    if (lEpisodeId < 0) return false;

    CStdString sql = FormatSQL("select * from episodeview where idepisode=%u",lEpisodeId);
    if (!m_pDS->query(sql.c_str()))
      return false;
    details = GetDetailsForEpisode(m_pDS, true);
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strFilenameAndPath.c_str());
  }
  return false;
}

void CVideoDatabase::GetMusicVideoInfo(const CStdString& strFilenameAndPath, CVideoInfoTag& details, long lMVideoId /* = -1 */)
{
  try
  {
    // TODO: Optimize this - no need for all the queries!
    if (lMVideoId < 0)
      lMVideoId = GetMusicVideoId(strFilenameAndPath);
    if (lMVideoId < 0) return ;

    CStdString sql = FormatSQL("select * from musicvideoview where idmvideo=%i", lMVideoId);
    if (!m_pDS->query(sql.c_str()))
      return;
    details = GetDetailsForMusicVideo(m_pDS);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strFilenameAndPath.c_str());
  }
}

void CVideoDatabase::AddGenreAndDirectorsAndStudios(const CVideoInfoTag& details, vector<long>& vecDirectors, vector<long>& vecGenres, vector<long>& vecStudios)
{
  // add all directors
  if (!details.m_strDirector.IsEmpty())
  {
    CStdStringArray directors;
    StringUtils::SplitString(details.m_strDirector, "/", directors);
    for (unsigned int i = 0; i < directors.size(); i++)
    {
      CStdString strDirector(directors[i]);
      strDirector.Trim();
      long lDirectorId = AddActor(strDirector,"");
      vecDirectors.push_back(lDirectorId);
    }
  }

  // add all genres
  if (!details.m_strGenre.IsEmpty())
  {
    CStdStringArray genres;
    StringUtils::SplitString(details.m_strGenre, "/", genres);
    for (unsigned int i = 0; i < genres.size(); i++)
    {
      CStdString strGenre(genres[i]);
      strGenre.Trim();
      long lGenreId = AddGenre(strGenre);
      vecGenres.push_back(lGenreId);
    }
  }
  // add all studios
  if (!details.m_strStudio.IsEmpty())
  {
    CStdStringArray studios;
    StringUtils::SplitString(details.m_strStudio, "/", studios);
    for (unsigned int i = 0; i < studios.size(); i++)
    {
      CStdString strStudio(studios[i]);
      strStudio.Trim();
      long lStudioId = AddStudio(strStudio);
      vecStudios.push_back(lStudioId);
    }
  }
}

CStdString CVideoDatabase::GetValueString(const CVideoInfoTag &details, int min, int max, const SDbTableOffsets *offsets) const
{
  CStdString sql;
  for (int i = min + 1; i < max; ++i)
  {
    switch (offsets[i].type)
    {
    case VIDEODB_TYPE_STRING:
      sql += FormatSQL("c%02d='%s',", i, ((CStdString*)(((char*)&details)+offsets[i].offset))->c_str());
      break;
    case VIDEODB_TYPE_INT:
      sql += FormatSQL("c%02d='%i',", i, *(int*)(((char*)&details)+offsets[i].offset));
      break;
    case VIDEODB_TYPE_COUNT:
      {
        int value = *(int*)(((char*)&details)+offsets[i].offset);
        if (value)
          sql += FormatSQL("c%02d=%i,", i, value);
        else
          sql += FormatSQL("c%02d=NULL,", i);
      }
      break;
    case VIDEODB_TYPE_BOOL:
      sql += FormatSQL("c%02d='%s',", i, *(bool*)(((char*)&details)+offsets[i].offset)?"true":"false");
      break;
    case VIDEODB_TYPE_FLOAT:
      sql += FormatSQL("c%02d='%f',", i, *(float*)(((char*)&details)+offsets[i].offset));
      break;
    }
  }
  sql.TrimRight(',');
  return sql;
}

//********************************************************************************************************************************
void CVideoDatabase::SetDetailsForMovie(const CStdString& strFilenameAndPath, const CVideoInfoTag& details)
{
  try
  {
    CVideoInfoTag info = details;

    long lFileId = GetFileId(strFilenameAndPath);
    if (lFileId < 0)
      lFileId = AddFile(strFilenameAndPath);
    long lMovieId = GetMovieId(strFilenameAndPath);
    if (lMovieId > -1)
    {
      int count = GetPlayCount(VIDEODB_CONTENT_MOVIES, lMovieId);
      if (count > 0)
        info.m_playCount = count;
      CLog::Log(LOGDEBUG,"Keeping playcount, %s = %i", info.m_strTitle.c_str(), info.m_playCount);
      DeleteMovie(strFilenameAndPath, true); // true to keep the table entry
    }
    BeginTransaction();
    lMovieId = AddMovie(strFilenameAndPath);
    if (lMovieId < 0)
    {
      CommitTransaction();
      return;
    }

    vector<long> vecDirectors;
    vector<long> vecGenres;
    vector<long> vecStudios;
    AddGenreAndDirectorsAndStudios(info,vecDirectors,vecGenres,vecStudios);

    for (unsigned int i = 0; i < vecGenres.size(); ++i)
      AddGenreToMovie(lMovieId, vecGenres[i]);

    for (unsigned int i = 0; i < vecDirectors.size(); ++i)
      AddDirectorToMovie(lMovieId, vecDirectors[i]);

    for (unsigned int i = 0; i < vecStudios.size(); ++i)
      AddStudioToMovie(lMovieId, vecStudios[i]);

    // add writers...
    if (!info.m_strWritingCredits.IsEmpty())
    {
      CStdStringArray writers;
      StringUtils::SplitString(info.m_strWritingCredits, "/", writers);
      for (unsigned int i = 0; i < writers.size(); i++)
      {
        CStdString writer(writers[i]);
        writer.Trim();
        long lWriterId = AddActor(writer,"");
        AddWriterToMovie(lMovieId, lWriterId );
      }
    }

    // add cast...
    for (CVideoInfoTag::iCast it = info.m_cast.begin(); it != info.m_cast.end(); ++it)
    {
      long lActor = AddActor(it->strName,it->thumbUrl.m_xml);
      AddActorToMovie(lMovieId, lActor, it->strRole);
    }

    // update our movie table (we know it was added already above)
    // and insert the new row
    CStdString sql = "update movie set " + GetValueString(info, VIDEODB_ID_MIN, VIDEODB_ID_MAX, DbMovieOffsets);
    sql += FormatSQL(" where idMovie=%u", lMovieId);
    m_pDS->exec(sql.c_str());
    CommitTransaction();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strFilenameAndPath.c_str());
  }
}

long CVideoDatabase::SetDetailsForTvShow(const CStdString& strPath, const CVideoInfoTag& details)
{
  try
  {
    if (!m_pDB.get() || !m_pDS.get())
    {
      CLog::Log(LOGERROR, "%s: called without database open", __FUNCTION__);
      return -1;
    }
    BeginTransaction();
    long lTvShowId = GetTvShowId(strPath);
    if (lTvShowId < 0)
      lTvShowId = AddTvShow(strPath);

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

    unsigned int i;
    for (i = 0; i < vecGenres.size(); ++i)
    {
      AddGenreToTvShow(lTvShowId, vecGenres[i]);
    }

    for (i = 0; i < vecDirectors.size(); ++i)
    {
      AddDirectorToTvShow(lTvShowId, vecDirectors[i]);
    }

    // and insert the new row
    CStdString sql = "update tvshow set " + GetValueString(details, VIDEODB_ID_TV_MIN, VIDEODB_ID_TV_MAX, DbTvShowOffsets);
    sql += FormatSQL("where idShow=%u", lTvShowId);
    m_pDS->exec(sql.c_str());
    CommitTransaction();
    return lTvShowId;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strPath.c_str());
  }

  return -1;
}

long CVideoDatabase::SetDetailsForEpisode(const CStdString& strFilenameAndPath, const CVideoInfoTag& details, long idShow, long lEpisodeId)
{
  try
  {
    BeginTransaction();
    if (lEpisodeId == -1)
    {
      lEpisodeId = GetEpisodeId(strFilenameAndPath);
      if (lEpisodeId > 0)
        DeleteEpisode(strFilenameAndPath,lEpisodeId);

      lEpisodeId = AddEpisode(idShow,strFilenameAndPath);
      if (lEpisodeId < 0)
      {
        CommitTransaction();
        return -1;
      }
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

    // add writers...
    if (!details.m_strWritingCredits.IsEmpty())
    {
      CStdStringArray writers;
      StringUtils::SplitString(details.m_strWritingCredits, "/", writers);
      for (unsigned int i = 0; i < writers.size(); i++)
      {
        CStdString writer(writers[i]);
        writer.Trim();
        long lWriterId = AddActor(writer,"");
        AddWriterToEpisode(lEpisodeId, lWriterId );
      }
    }

    for (unsigned int i = 0; i < vecDirectors.size(); ++i)
    {
      AddDirectorToEpisode(lEpisodeId, vecDirectors[i]);
    }

    // and insert the new row
    CStdString sql = "update episode set " + GetValueString(details, VIDEODB_ID_EPISODE_MIN, VIDEODB_ID_EPISODE_MAX, DbEpisodeOffsets);
    sql += FormatSQL("where idEpisode=%u", lEpisodeId);
    m_pDS->exec(sql.c_str());
    CommitTransaction();
    return lEpisodeId;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strFilenameAndPath.c_str());
  }
  return -1;
}

void CVideoDatabase::SetDetailsForMusicVideo(const CStdString& strFilenameAndPath, const CVideoInfoTag& details)
{
  try
  {
    BeginTransaction();
    long lFileId = GetFileId(strFilenameAndPath);
    if (lFileId < 0)
      lFileId = AddFile(strFilenameAndPath);
    long lMVideoId = GetMusicVideoId(strFilenameAndPath);
    if (lMVideoId > -1)
    {
      DeleteMusicVideo(strFilenameAndPath);
    }
    lMVideoId = AddMusicVideo(strFilenameAndPath);
    if (lMVideoId < 0)
    {
      CommitTransaction();
      return;
    }

    vector<long> vecDirectors;
    vector<long> vecGenres;
    vector<long> vecStudios;
    AddGenreAndDirectorsAndStudios(details,vecDirectors,vecGenres,vecStudios);

    // add artists...
    if (!details.m_strArtist.IsEmpty())
    {
      CStdStringArray vecArtists;
      StringUtils::SplitString(details.m_strArtist, g_advancedSettings.m_videoItemSeparator, vecArtists);
      for (unsigned int i = 0; i < vecArtists.size(); i++)
      {
        CStdString artist = vecArtists[i];
        artist.Trim();
        long lArtist = AddActor(artist,"");
        AddArtistToMusicVideo(lMVideoId, lArtist);
      }
    }

    unsigned int i;
    for (i = 0; i < vecGenres.size(); ++i)
    {
      AddGenreToMusicVideo(lMVideoId, vecGenres[i]);
    }

    for (i = 0; i < vecDirectors.size(); ++i)
    {
      AddDirectorToMusicVideo(lMVideoId, vecDirectors[i]);
    }

    for (i = 0; i < vecStudios.size(); ++i)
    {
      AddStudioToMusicVideo(lMVideoId, vecStudios[i]);
    }

    // update our movie table (we know it was added already above)
    // and insert the new row
    CStdString sql = "update musicvideo set " + GetValueString(details, VIDEODB_ID_MUSICVIDEO_MIN, VIDEODB_ID_MUSICVIDEO_MAX, DbMusicVideoOffsets);
    sql += FormatSQL(" where idMVideo=%u", lMVideoId);
    m_pDS->exec(sql.c_str());
    CommitTransaction();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strFilenameAndPath.c_str());
  }
}

//********************************************************************************************************************************
void CVideoDatabase::GetFilePathById(long lMovieId, CStdString &filePath, VIDEODB_CONTENT_TYPE iType)
{
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    if (lMovieId < 0) return ;

    CStdString strSQL;
    if (iType == VIDEODB_CONTENT_MOVIES)
      strSQL=FormatSQL("select path.strPath,files.strFileName from path, files, movie where path.idPath=files.idPath and files.idFile=movie.idFile and movie.idMovie=%u order by strFilename", lMovieId );
    if (iType == VIDEODB_CONTENT_EPISODES)
      strSQL=FormatSQL("select path.strPath,files.strFileName from path, files, episode where path.idPath=files.idPath and files.idFile=episode.idFile and episode.idEpisode=%u order by strFilename", lMovieId );
    if (iType == VIDEODB_CONTENT_TVSHOWS)
      strSQL=FormatSQL("select path.strPath from path,tvshowlinkpath where path.idpath=tvshowlinkpath.idpath and tvshowlinkpath.idshow=%i", lMovieId );
    if (iType ==VIDEODB_CONTENT_MUSICVIDEOS)
      strSQL=FormatSQL("select path.strPath,files.strFileName from path, files, musicvideo where path.idPath=files.idPath and files.idFile=musicvideo.idFile and musicvideo.idmvideo=%u order by strFilename", lMovieId );

    m_pDS->query( strSQL.c_str() );
    if (!m_pDS->eof())
    {
      if (iType != VIDEODB_CONTENT_TVSHOWS)
      {
        CStdString fileName = m_pDS->fv("files.strFilename").get_asString();
        ConstructPath(filePath,m_pDS->fv("path.strPath").get_asString(),fileName);
      }
      else
        filePath = m_pDS->fv("path.strPath").get_asString();
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
}

//********************************************************************************************************************************
void CVideoDatabase::GetBookMarksForFile(const CStdString& strFilenameAndPath, VECBOOKMARKS& bookmarks, CBookmark::EType type /*= CBookmark::STANDARD*/, bool bAppend)
{
  try
  {
    long lFileId = GetFileId(strFilenameAndPath);
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
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strFilenameAndPath.c_str());
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

void CVideoDatabase::DeleteResumeBookMark(const CStdString &strFilenameAndPath)
{
  if (!m_pDB.get() || !m_pDS.get())
    return;

  int fileID = GetFileId(strFilenameAndPath);
  if (fileID < -1)
    return;

  try
  {
    CStdString sql = FormatSQL("delete from bookmark where idFile=%i and type=%i", fileID, CBookmark::RESUME);
    m_pDS->exec(sql.c_str());
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strFilenameAndPath.c_str());
  }
}

void CVideoDatabase::GetEpisodesByFile(const CStdString& strFilenameAndPath, vector<CVideoInfoTag>& episodes)
{
  try
  {
    CStdString strSQL = FormatSQL("select * from episodeview where idFile=%i order by c%02d, c%02d asc", GetFileId(strFilenameAndPath), VIDEODB_ID_EPISODE_SORTSEASON, VIDEODB_ID_EPISODE_SORTEPISODE);
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
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strFilenameAndPath.c_str());
  }
}

//********************************************************************************************************************************
void CVideoDatabase::AddBookMarkToFile(const CStdString& strFilenameAndPath, const CBookmark &bookmark, CBookmark::EType type /*= CBookmark::STANDARD*/)
{
  try
  {
    long lFileId = GetFileId(strFilenameAndPath);
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
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strFilenameAndPath.c_str());
  }
}

void CVideoDatabase::ClearBookMarkOfFile(const CStdString& strFilenameAndPath, CBookmark& bookmark, CBookmark::EType type /*= CBookmark::STANDARD*/)
{
  try
  {
    long lFileId = GetFileId(strFilenameAndPath);
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
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strFilenameAndPath.c_str());
  }
}

//********************************************************************************************************************************
void CVideoDatabase::ClearBookMarksOfFile(const CStdString& strFilenameAndPath, CBookmark::EType type /*= CBookmark::STANDARD*/)
{
  try
  {
    long lFileId = GetFileId(strFilenameAndPath);
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
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strFilenameAndPath.c_str());
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
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
    return false;
  }
  return true;
}

void CVideoDatabase::AddBookMarkForEpisode(const CVideoInfoTag& tag, const CBookmark& bookmark)
{
  try
  {
    long lFileId = GetFileId(tag.m_strFileNameAndPath);
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
    CLog::Log(LOGERROR, "%s (%i) failed", __FUNCTION__, tag.m_iDbId);
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
    CLog::Log(LOGERROR, "%s (%i) failed", __FUNCTION__, tag.m_iDbId);
  }
}

//********************************************************************************************************************************
void CVideoDatabase::DeleteMovie(const CStdString& strFilenameAndPath, bool bKeepId /* = false */, bool bKeepThumb /* = false */)
{
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;
    long lMovieId = GetMovieId(strFilenameAndPath);
    if (lMovieId < 0)
    {
      return ;
    }

    BeginTransaction();

    CStdString strSQL;
    strSQL=FormatSQL("delete from genrelinkmovie where idmovie=%i", lMovieId);
    m_pDS->exec(strSQL.c_str());

    strSQL=FormatSQL("delete from actorlinkmovie where idmovie=%i", lMovieId);
    m_pDS->exec(strSQL.c_str());

    strSQL=FormatSQL("delete from directorlinkmovie where idmovie=%i", lMovieId);
    m_pDS->exec(strSQL.c_str());

    strSQL=FormatSQL("delete from studiolinkmovie where idmovie=%i", lMovieId);
    m_pDS->exec(strSQL.c_str());

    if (!bKeepThumb)
      DeleteThumbForItem(strFilenameAndPath,false);

    // keep the movie table entry, linking to tv shows, and bookmarks
    // so we can update the data in place
    // the ancilliary tables are still purged
    if (!bKeepId)
    {
      ClearBookMarksOfFile(strFilenameAndPath);

      strSQL=FormatSQL("delete from movie where idmovie=%i", lMovieId);
      m_pDS->exec(strSQL.c_str());

      strSQL=FormatSQL("delete from movielinktvshow where idmovie=%i", lMovieId);
      m_pDS->exec(strSQL.c_str());
    }
    /*
    // work in progress
    else
    {
      // clear the title
      strSQL=FormatSQL("update movie set c%02d=NULL where idmovie=%i", VIDEODB_ID_TITLE, lMovieId);
      m_pDS->exec(strSQL.c_str());
    }
    */

    CStdString strPath, strFileName;
    SplitPath(strFilenameAndPath,strPath,strFileName);
    InvalidatePathHash(strPath);
    CommitTransaction();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
}

void CVideoDatabase::DeleteTvShow(const CStdString& strPath, bool bKeepId /* = false */, bool bKeepThumb /* = false */)
{
  try
  {
    long lTvShowId=-1;
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;
    lTvShowId = GetTvShowId(strPath);
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
      DeleteEpisode(strPath,m_pDS2->fv(0).get_asLong(), bKeepId);
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

    if (!bKeepThumb)
      DeleteThumbForItem(strPath,true);

    // keep tvshow table and movielink table so we can update data in place
    if (!bKeepId)
    {
      strSQL=FormatSQL("delete from tvshow where idshow=%i", lTvShowId);
      m_pDS->exec(strSQL.c_str());

      strSQL=FormatSQL("delete from movielinktvshow where idshow=%i", lTvShowId);
      m_pDS->exec(strSQL.c_str());
    }
    /*
    // work in progress
    else
    {
      // clear the title
      strSQL=FormatSQL("update tvshow set c%02d=NULL where idshow=%i", VIDEODB_ID_TV_TITLE, lTvShowId);
      m_pDS->exec(strSQL.c_str());
    }
    */

    InvalidatePathHash(strPath);

    CommitTransaction();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
}

void CVideoDatabase::DeleteEpisode(const CStdString& strFilenameAndPath, long lEpisodeId, bool bKeepId /* = false */, bool bKeepThumb /* = false */)
{
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;
    if (lEpisodeId < 0)
    {
      lEpisodeId = GetEpisodeId(strFilenameAndPath);
      if (lEpisodeId < 0)
      {
        return ;
      }
    }

    long lFileId = GetFileId(strFilenameAndPath);
    if (lFileId < 0)
      return ;

    CStdString strSQL;
    strSQL=FormatSQL("delete from actorlinkepisode where idepisode=%i", lEpisodeId);
    m_pDS->exec(strSQL.c_str());

    strSQL=FormatSQL("delete from directorlinkepisode where idepisode=%i", lEpisodeId);
    m_pDS->exec(strSQL.c_str());

    strSQL=FormatSQL("select tvshowlinkepisode.idshow from tvshowlinkepisode where idepisode=%u",lEpisodeId);
    m_pDS->query(strSQL.c_str());

    strSQL=FormatSQL("delete from tvshowlinkepisode where idepisode=%i", lEpisodeId);
    m_pDS->exec(strSQL.c_str());

    if (!bKeepThumb)
      DeleteThumbForItem(strFilenameAndPath,false);

    // keep episode table entry and bookmarks so we can update the data in place
    // the ancilliary tables are still purged
    if (!bKeepId)
    {
      ClearBookMarksOfFile(strFilenameAndPath);

      strSQL=FormatSQL("delete from episode where idfile=%i", lFileId);
      m_pDS->exec(strSQL.c_str());
    }
    /*
    // work in progress
    else
    {
      // clear the title
      strSQL=FormatSQL("update episode set c%02d=NULL where idepisode=%i", VIDEODB_ID_EPISODE_TITLE, lEpisodeId);
      m_pDS->exec(strSQL.c_str());
    }
    */

  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
}

void CVideoDatabase::DeleteMusicVideo(const CStdString& strFilenameAndPath, bool bKeepId /* = false */, bool bKeepThumb /* = false */)
{
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;
    long lMVideoId = GetMusicVideoId(strFilenameAndPath);
    if (lMVideoId < 0)
    {
      return ;
    }

    BeginTransaction();

    CStdString strSQL;
    strSQL=FormatSQL("delete from genrelinkmusicvideo where idmvideo=%i", lMVideoId);
    m_pDS->exec(strSQL.c_str());

    strSQL=FormatSQL("delete from artistlinkmusicvideo where idmvideo=%i", lMVideoId);
    m_pDS->exec(strSQL.c_str());

    strSQL=FormatSQL("delete from directorlinkmusicvideo where idmvideo=%i", lMVideoId);
    m_pDS->exec(strSQL.c_str());

    strSQL=FormatSQL("delete from studiolinkmusicvideo where idmvideo=%i", lMVideoId);
    m_pDS->exec(strSQL.c_str());

    if (!bKeepThumb)
      DeleteThumbForItem(strFilenameAndPath,false);

    // keep the music video table entry and bookmarks so we can update data in place
    // the ancilliary tables are still purged
    if (!bKeepId)
    {
      ClearBookMarksOfFile(strFilenameAndPath);

      strSQL=FormatSQL("delete from musicvideo where idmvideo=%i", lMVideoId);
      m_pDS->exec(strSQL.c_str());
    }
    /*
    // work in progress
    else
    {
      // clear the title
      strSQL=FormatSQL("update musicvideo set c%02d=NULL where idmvideo=%i", VIDEODB_ID_MUSICVIDEO_TITLE, lMVideoId);
      m_pDS->exec(strSQL.c_str());
    }
    */

    CStdString strPath, strFileName;
    SplitPath(strFilenameAndPath,strPath,strFileName);
    InvalidatePathHash(strPath);
    CommitTransaction();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
}

void CVideoDatabase::GetDetailsFromDB(auto_ptr<Dataset> &pDS, int min, int max, const SDbTableOffsets *offsets, CVideoInfoTag &details)
{
  for (int i = min + 1; i < max; i++)
  {
    switch (offsets[i].type)
    {
    case VIDEODB_TYPE_STRING:
      *(CStdString*)(((char*)&details)+offsets[i].offset) = pDS->fv(i+1).get_asString();
      break;
    case VIDEODB_TYPE_INT:
    case VIDEODB_TYPE_COUNT:
      *(int*)(((char*)&details)+offsets[i].offset) = pDS->fv(i+1).get_asInteger();
      break;
    case VIDEODB_TYPE_BOOL:
      *(bool*)(((char*)&details)+offsets[i].offset) = pDS->fv(i+1).get_asBool();
      break;
    case VIDEODB_TYPE_FLOAT:
      *(float*)(((char*)&details)+offsets[i].offset) = pDS->fv(i+1).get_asFloat();
      break;
    }
  }
}

DWORD movieTime = 0;
DWORD castTime = 0;

CVideoInfoTag CVideoDatabase::GetDetailsByTypeAndId(VIDEODB_CONTENT_TYPE type, long id)
{
  CVideoInfoTag details;
  details.Reset();

  switch (type)
  {
    case VIDEODB_CONTENT_MOVIES:
      GetMovieInfo("", details, id);
      break;
    case VIDEODB_CONTENT_TVSHOWS:
      GetTvShowInfo("", details, id);
      break;
    case VIDEODB_CONTENT_EPISODES:
      GetEpisodeInfo("", details, id);
      break;
    case VIDEODB_CONTENT_MUSICVIDEOS:
      GetMusicVideoInfo("", details, id);
  }

  return details;
}

CVideoInfoTag CVideoDatabase::GetDetailsForMovie(auto_ptr<Dataset> &pDS, bool needsCast /* = false */)
{
  CVideoInfoTag details;
  details.Reset();

  DWORD time = timeGetTime();
  long lMovieId = pDS->fv(0).get_asLong();

  GetDetailsFromDB(pDS, VIDEODB_ID_MIN, VIDEODB_ID_MAX, DbMovieOffsets, details);

  details.m_iDbId = lMovieId;

  details.m_strPath = pDS->fv(VIDEODB_DETAILS_PATH).get_asString();
  CStdString strFileName = pDS->fv(VIDEODB_DETAILS_FILE).get_asString();
  ConstructPath(details.m_strFileNameAndPath,details.m_strPath,strFileName);
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
    details.m_strPictureURL.Parse();
  }
  return details;
}

CVideoInfoTag CVideoDatabase::GetDetailsForTvShow(auto_ptr<Dataset> &pDS, bool needsCast /* = false */)
{
  CVideoInfoTag details;
  details.Reset();

  DWORD time = timeGetTime();
  long lTvShowId = pDS->fv(0).get_asLong();

  GetDetailsFromDB(pDS, VIDEODB_ID_TV_MIN, VIDEODB_ID_TV_MAX, DbTvShowOffsets, details);
  details.m_iDbId = lTvShowId;
  details.m_strPath = pDS->fv(VIDEODB_MAX_COLUMNS + 1).get_asString();
  details.m_iEpisode = m_pDS->fv(VIDEODB_MAX_COLUMNS + 2).get_asInteger();
  details.m_playCount = m_pDS->fv(VIDEODB_MAX_COLUMNS + 3).get_asInteger(); // number watched
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
    details.m_strPictureURL.Parse();
  }
  details.m_fanart.Unpack();
  return details;
}

CVideoInfoTag CVideoDatabase::GetDetailsForEpisode(auto_ptr<Dataset> &pDS, bool needsCast /* = false */)
{
  CVideoInfoTag details;
  details.Reset();

  DWORD time = timeGetTime();
  long lEpisodeId = pDS->fv(0).get_asLong();

  GetDetailsFromDB(pDS, VIDEODB_ID_EPISODE_MIN, VIDEODB_ID_EPISODE_MAX, DbEpisodeOffsets, details);
  details.m_iDbId = lEpisodeId;

  details.m_strPath = pDS->fv(VIDEODB_DETAILS_PATH).get_asString();
  CStdString strFileName = pDS->fv(VIDEODB_DETAILS_FILE).get_asString();
  ConstructPath(details.m_strFileNameAndPath,details.m_strPath,strFileName);
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
    details.m_strPictureURL.Parse();
  }
  return details;
}

CVideoInfoTag CVideoDatabase::GetDetailsForMusicVideo(auto_ptr<Dataset> &pDS)
{
  CVideoInfoTag details;
  details.Reset();

  DWORD time = timeGetTime();
  long lMovieId = pDS->fv(0).get_asLong();

  GetDetailsFromDB(pDS, VIDEODB_ID_MUSICVIDEO_MIN, VIDEODB_ID_MUSICVIDEO_MAX, DbMusicVideoOffsets, details);
  details.m_iDbId = lMovieId;

  details.m_strPath = pDS->fv(VIDEODB_DETAILS_PATH).get_asString();
  CStdString strFileName = pDS->fv(VIDEODB_DETAILS_FILE).get_asString();
  ConstructPath(details.m_strFileNameAndPath,details.m_strPath,strFileName);

  movieTime += timeGetTime() - time; time = timeGetTime();

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
    long lFileId = GetFileId(strFilenameAndPath);
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
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
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
    long lFileId = GetFileId(strFilenameAndPath);
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
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strFilenameAndPath.c_str());
  }
}

/// \brief GetStackTimes() obtains any saved video times for the stacked file
/// \retval Returns true if the stack times exist, false otherwise.
bool CVideoDatabase::GetStackTimes(const CStdString &filePath, vector<long> &times)
{
  try
  {
    // obtain the FileID (if it exists)
    long lFileId = GetFileId(filePath);
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
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
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
    long lFileId = GetFileId(filePath);
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
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, filePath.c_str());
  }
}

void CVideoDatabase::RemoveContentForPath(const CStdString& strPath, CGUIDialogProgress *progress /* = NULL */)
{
  if(CUtil::IsMultiPath(strPath))
  {
    vector<CStdString> paths;
    CMultiPathDirectory::GetPaths(strPath, paths);

    for(unsigned i=0;i<paths.size();i++)
      RemoveContentForPath(paths[i], progress);
  }

  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    auto_ptr<Dataset> pDS(m_pDB->CreateDataset());
    CStdString strPath1(strPath);
    CStdString strSQL = FormatSQL("select idPath,strContent,strPath from path where strPath like '%%%s%%'",strPath1.c_str());
    progress = (CGUIDialogProgress *)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
    pDS->query(strSQL.c_str());
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
          CStdString strFileName = m_pDS2->fv("files.strFilename").get_asString();
          ConstructPath(strMoviePath, strCurrPath, strFileName);
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
    }
    strSQL = FormatSQL("update path set strContent = '', strScraper='', strHash='',strSettings='',useFolderNames=0,scanRecursive=0 where strPath like '%%%s%%'",strPath1.c_str());
    pDS->exec(strSQL.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strPath.c_str());
  }
  if (progress)
    progress->Close();
}

void CVideoDatabase::SetScraperForPath(const CStdString& filePath, const SScraperInfo& info, const VIDEO::SScanSettings& settings)
{
  // if we have a multipath, set scraper for all contained paths too
  if(CUtil::IsMultiPath(filePath))
  {
    vector<CStdString> paths;
    CMultiPathDirectory::GetPaths(filePath, paths);

    for(unsigned i=0;i<paths.size();i++)
      SetScraperForPath(paths[i],info,settings);
  }

  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;
    long lPathId = GetPathId(filePath);
    if (lPathId < 0)
    { // no path found - we have to add one
      lPathId = AddPath(filePath);
      if (lPathId < 0) return ;
    }

    // Update
    CStdString strSQL=FormatSQL("update path set strContent='%s',strScraper='%s', scanRecursive=%i, useFolderNames=%i, strSettings='%s', noUpdate=%i where idPath=%u", info.strContent.c_str(), info.strPath.c_str(),settings.recurse,settings.parent_name,info.settings.GetSettings().c_str(),settings.noupdate, lPathId);
    m_pDS->exec(strSQL.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, filePath.c_str());
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
    if (iVersion < 14)
    {
      // add the scraper settings column
      m_pDS->exec("alter table path add strSettings text");
    }
    if (iVersion < 15)
    {
      // clear all tvshow columns above 11 to fix results of an old bug
      m_pDS->exec("UPDATE tvshow SET c12=NULL, c13=NULL, c14=NULL, c15=NULL, c16=NULL, c17=NULL, c18=NULL, c19=NULL, c20=NULL");
    }
    if (iVersion < 16)
    {
      // remove episodes column from tvshows
      m_pDS->exec("UPDATE tvshow SET c11=c12,c12=NULL");
    }
    if (iVersion < 17)
    {
      // change watched -> playcount (assume a single play)
      m_pDS->exec("UPDATE movie SET c10=NULL where c10='false'");
      m_pDS->exec("UPDATE movie SET c10=1 where c10='true'");
      m_pDS->exec("UPDATE episode SET c08=NULL where c08='false'");
      m_pDS->exec("UPDATE episode SET c08=1 where c08='true'");
      m_pDS->exec("UPDATE musicvideo SET c03=NULL where c03='false'");
      m_pDS->exec("UPDATE musicvideo SET c03=1 where c03='true'");
    }
    if (iVersion < 18)
    {
      // add tvshowview to simplify code
      CStdString showview=FormatSQL("create view tvshowview as select tvshow.*,path.strPath as strPath,"
                                    "counts.totalcount as totalCount,counts.watchedcount as watchedCount,"
                                    "counts.totalcount=counts.watchedcount as watched from tvshow "
                                    "join tvshowlinkpath on tvshow.idShow=tvshowlinkpath.idShow "
                                    "join path on path.idpath=tvshowlinkpath.idPath "
                                    "left outer join ("
                                    "    select tvshow.idShow as idShow,count(1) as totalcount,count(episode.c%02d) as watchedcount from tvshow "
                                    "    join tvshowlinkepisode on tvshow.idShow = tvshowlinkepisode.idShow "
                                    "    join episode on episode.idEpisode = tvshowlinkepisode.idEpisode "
                                    "    group by tvshow.idShow"
                                    ") counts on tvshow.idShow = counts.idShow", VIDEODB_ID_EPISODE_PLAYCOUNT);
      m_pDS->exec(showview.c_str());
      CStdString episodeview = FormatSQL("create view episodeview as select episode.*,files.strFileName as strFileName,"
                                         "path.strPath as strPath,tvshow.c%02d as strTitle,tvshow.idShow as idShow,"
                                         "tvshow.c%02d as premiered from episode "
                                         "join files on files.idFile=episode.idFile "
                                         "join tvshowlinkepisode on episode.idepisode=tvshowlinkepisode.idepisode "
                                         "join tvshow on tvshow.idshow=tvshowlinkepisode.idshow "
                                         "join path on files.idPath=path.idPath",VIDEODB_ID_TV_TITLE, VIDEODB_ID_TV_PREMIERED);
      m_pDS->exec(episodeview.c_str());
    }
    if (iVersion < 19)
    { // drop the unused genrelinkepisode table
      m_pDS->exec("drop table genrelinkepisode");
      // add the writerlinkepisode table, and fill it
      CLog::Log(LOGINFO, "create writerlinkepisode table");
      m_pDS->exec("CREATE TABLE writerlinkepisode ( idWriter integer, idEpisode integer)\n");
      m_pDS->exec("CREATE UNIQUE INDEX ix_writerlinkepisode_1 ON writerlinkepisode ( idWriter, idEpisode )\n");
      m_pDS->exec("CREATE UNIQUE INDEX ix_writerlinkepisode_2 ON writerlinkepisode ( idEpisode, idWriter )\n");
      CStdString sql = FormatSQL("select idEpisode,c%02d from episode", VIDEODB_ID_EPISODE_CREDITS);
      m_pDS->query(sql.c_str());
      vector< pair<long, CStdString> > writingCredits;
      while (!m_pDS->eof())
      {
        writingCredits.push_back(pair<long, CStdString>(m_pDS->fv(0).get_asLong(), m_pDS->fv(1).get_asString()));
        m_pDS->next();
      }
      m_pDS->close();
      for (unsigned int i = 0; i < writingCredits.size(); i++)
      {
        long idEpisode = writingCredits[i].first;
        if (writingCredits[i].second.size())
        {
          CStdStringArray writers;
          StringUtils::SplitString(writingCredits[i].second, "/", writers);
          for (unsigned int i = 0; i < writers.size(); i++)
          {
            CStdString writer(writers[i]);
            writer.Trim();
            long lWriterId = AddActor(writer,"");
            AddWriterToEpisode(idEpisode, lWriterId);
          }
        }
      }
    }
    if (iVersion < 20)
    {
      // artist and genre entries created in musicvideo table - no point doing this IMO - better to just require rescan.
      // also add musicvideoview and movieview
      m_pDS->exec("create view musicvideoview as select musicvideo.*,files.strFileName as strFileName,path.strPath as strPath "
                  "from musicvideo join files on files.idFile=musicvideo.idFile join path on path.idPath=files.idPath");
      m_pDS->exec("create view movieview as select movie.*,files.strFileName as strFileName,path.strPath as strPath "
                  "from movie join files on files.idFile=movie.idFile join path on path.idPath=files.idPath");
    }
    if (iVersion < 21)
    {
      // add the writerlinkmovie table, and fill it
      CLog::Log(LOGINFO, "create writerlinkmovie table");
      m_pDS->exec("CREATE TABLE writerlinkmovie ( idWriter integer, idMovie integer)\n");
      m_pDS->exec("CREATE UNIQUE INDEX ix_writerlinkmovie_1 ON writerlinkmovie ( idWriter, idMovie )\n");
      m_pDS->exec("CREATE UNIQUE INDEX ix_writerlinkmovie_2 ON writerlinkmovie ( idMovie, idWriter )\n");
      CStdString sql = FormatSQL("select idMovie,c%02d from movie", VIDEODB_ID_CREDITS);
      m_pDS->query(sql.c_str());
      vector< pair<long, CStdString> > writingCredits;
      while (!m_pDS->eof())
      {
        writingCredits.push_back(pair<long, CStdString>(m_pDS->fv(0).get_asLong(), m_pDS->fv(1).get_asString()));
        m_pDS->next();
      }
      m_pDS->close();
      for (unsigned int i = 0; i < writingCredits.size(); i++)
      {
        long idMovie = writingCredits[i].first;
        if (writingCredits[i].second.size())
        {
          CStdStringArray writers;
          StringUtils::SplitString(writingCredits[i].second, "/", writers);
          for (unsigned int i = 0; i < writers.size(); i++)
          {
            CStdString writer(writers[i]);
            writer.Trim();
            long lWriterId = AddActor(writer,"");
            AddWriterToMovie(idMovie, lWriterId);
          }
        }
      }
    }
    if (iVersion < 22) // reverse audio/subtitle offsets
      m_pDS->exec("update settings set SubtitleDelay=-SubtitleDelay and AudioDelay=-AudioDelay");
    if (iVersion < 23)
    {
      // add the noupdate column to the path table
      m_pDS->exec("alter table path add noUpdate bool");
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

int CVideoDatabase::GetPlayCount(VIDEODB_CONTENT_TYPE type, long id)
{
  try
  {
    // error!
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    CStdString strSQL;
    if (type == VIDEODB_CONTENT_MOVIES)
      strSQL.Format("select c%02d from movie WHERE idMovie=%u", VIDEODB_ID_PLAYCOUNT, id);
    else if (type == VIDEODB_CONTENT_EPISODES)
      strSQL.Format("select c%02d from episode WHERE idEpisode=%u", VIDEODB_ID_EPISODE_PLAYCOUNT, id);
    else if (type == VIDEODB_CONTENT_MUSICVIDEOS)
      strSQL.Format("select c%02d from musicvideo WHERE idMVideo=%u", VIDEODB_ID_MUSICVIDEO_PLAYCOUNT, id);
    else
      return -1;

    int count = -1;
    if (m_pDS->query(strSQL.c_str()))
    {
      // there should only ever be one row returned
      if (m_pDS->num_rows() == 1)
        count = m_pDS->fv(0).get_asInteger();
      m_pDS->close();
    }
    return count;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return -1;
}

void CVideoDatabase::UpdateFanart(const CFileItem &item, VIDEODB_CONTENT_TYPE type)
{
  if (NULL == m_pDB.get()) return;
  if (NULL == m_pDS.get()) return;
  if (!item.HasVideoInfoTag() || item.GetVideoInfoTag()->m_iDbId < 0) return;

  CStdString exec;
  if (type == VIDEODB_CONTENT_TVSHOWS)
    exec = FormatSQL("UPDATE tvshow set c%02d='%s' WHERE idshow=%i", VIDEODB_ID_TV_FANART, item.GetVideoInfoTag()->m_fanart.m_xml.c_str(), item.GetVideoInfoTag()->m_iDbId);
  else if (type == VIDEODB_CONTENT_MOVIES)
    exec = FormatSQL("UPDATE movie set c%02d='%s' WHERE idmovie=%i", VIDEODB_ID_FANART, item.GetVideoInfoTag()->m_fanart.m_xml.c_str(), item.GetVideoInfoTag()->m_iDbId);

  try
  {
    m_pDS->exec(exec.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s - error updating fanart for %s", __FUNCTION__, item.m_strPath.c_str());
  }
}

void CVideoDatabase::MarkAsWatched(const CFileItem &item)
{
  // first grab the type of video and it's id
  VIDEODB_CONTENT_TYPE type = VIDEODB_CONTENT_MOVIES;
  long id = -1;
  if (item.HasVideoInfoTag() && item.GetVideoInfoTag()->m_iDbId > -1)
  { // we have it's id already
    if (item.GetVideoInfoTag()->m_iSeason > -1 && !item.m_bIsFolder) // episode
      type = VIDEODB_CONTENT_EPISODES;
    else if (!item.GetVideoInfoTag()->m_strArtist.IsEmpty())
      type = VIDEODB_CONTENT_MUSICVIDEOS;
    id = item.GetVideoInfoTag()->m_iDbId;
  }
  if (id < 0)
    id = GetMovieId(item.m_strPath);
  if (id < 0)
  {
    type = VIDEODB_CONTENT_EPISODES;
    id = GetEpisodeId(item.m_strPath);
  }
  if (id < 0)
  {
    id = GetMusicVideoId(item.m_strPath);
    type = VIDEODB_CONTENT_MUSICVIDEOS;
  }
  if (id < 0)
    return;  // not in db

  // and mark as watched
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    int count = GetPlayCount(type, id);
    // hmm... what should be done upon an error getting the playcount?
    if (count > -1)
    {
      count++;
      CStdString strSQL;
      if (type == VIDEODB_CONTENT_MOVIES)
        strSQL.Format("UPDATE movie set c%02d=%i WHERE idMovie=%u", VIDEODB_ID_PLAYCOUNT, count, id);
      else if (type == VIDEODB_CONTENT_EPISODES)
        strSQL.Format("UPDATE episode set c%02d=%i WHERE idEpisode=%u", VIDEODB_ID_EPISODE_PLAYCOUNT, count, id);
      else if (type == VIDEODB_CONTENT_MUSICVIDEOS)
        strSQL.Format("UPDATE musicvideo set c%02d=%i WHERE idMVideo=%u", VIDEODB_ID_MUSICVIDEO_PLAYCOUNT, count, id);
      m_pDS->exec(strSQL.c_str());
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
}

void CVideoDatabase::MarkAsUnWatched(const CFileItem &item)
{
  // unlike MarkAsWatched, we assume the file is in the videodb and has it's tag info available
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    if (!item.HasVideoInfoTag() || item.GetVideoInfoTag()->m_iDbId < 0) return; // not in the db, or at least we don't have the info for it

    // NOTE: We clear to NULL here as then the episode counting works much more nicely
    CStdString strSQL;
    if (item.GetVideoInfoTag()->m_iSeason > -1 && !item.m_bIsFolder) // episode
      strSQL = FormatSQL("UPDATE episode set c%02d=NULL WHERE idEpisode=%u", VIDEODB_ID_EPISODE_PLAYCOUNT, item.GetVideoInfoTag()->m_iDbId);
    else if (!item.GetVideoInfoTag()->m_strArtist.IsEmpty())
      strSQL = FormatSQL("UPDATE musicvideo set c%02d=NULL WHERE idMVideo=%u", VIDEODB_ID_MUSICVIDEO_PLAYCOUNT, item.GetVideoInfoTag()->m_iDbId);
    else
      strSQL = FormatSQL("UPDATE movie set c%02d=NULL WHERE idMovie=%u", VIDEODB_ID_PLAYCOUNT, item.GetVideoInfoTag()->m_iDbId);

    m_pDS->exec(strSQL.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
}

void CVideoDatabase::UpdateMovieTitle(long lMovieId, const CStdString& strNewMovieTitle, VIDEODB_CONTENT_TYPE iType)
{
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;
    CStdString strSQL;
    if (iType == VIDEODB_CONTENT_MOVIES)
    {
      CLog::Log(LOGINFO, "Changing Movie:id:%ld New Title:%s", lMovieId, strNewMovieTitle.c_str());
      strSQL = FormatSQL("UPDATE movie SET c%02d='%s' WHERE idMovie=%i", VIDEODB_ID_TITLE, strNewMovieTitle.c_str(), lMovieId );
    }
    else if (iType == VIDEODB_CONTENT_EPISODES)
    {
      CLog::Log(LOGINFO, "Changing Episode:id:%ld New Title:%s", lMovieId, strNewMovieTitle.c_str());
      strSQL = FormatSQL("UPDATE episode SET c%02d='%s' WHERE idEpisode=%i", VIDEODB_ID_EPISODE_TITLE, strNewMovieTitle.c_str(), lMovieId );
    }
    else if (iType == VIDEODB_CONTENT_TVSHOWS)
    {
      CLog::Log(LOGINFO, "Changing TvShow:id:%ld New Title:%s", lMovieId, strNewMovieTitle.c_str());
      strSQL = FormatSQL("UPDATE tvshow SET c%02d='%s' WHERE idShow=%i", VIDEODB_ID_TV_TITLE, strNewMovieTitle.c_str(), lMovieId );
    }
    else if (iType == VIDEODB_CONTENT_MUSICVIDEOS)
    {
      CLog::Log(LOGINFO, "Changing MusicVideo:id:%ld New Title:%s", lMovieId, strNewMovieTitle.c_str());
      strSQL = FormatSQL("UPDATE musicvideo SET c%02d='%s' WHERE idMVideo=%i", VIDEODB_ID_MUSICVIDEO_TITLE, strNewMovieTitle.c_str(), lMovieId );
    }
    m_pDS->exec(strSQL.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (long lMovieId, const CStdString& strNewMovieTitle) failed on MovieID:%ld and Title:%s", __FUNCTION__, lMovieId, strNewMovieTitle.c_str());
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
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
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
        strSQL=FormatSQL("select genre.idgenre,genre.strgenre,path.strPath,movie.c%02d from genre,genrelinkmovie,movie,path,files where genre.idGenre=genrelinkMovie.idGenre and genrelinkMovie.idMovie = movie.idMovie and files.idFile=movie.idFile and path.idPath = files.idPath",VIDEODB_ID_PLAYCOUNT);
      }
      else if (idContent == VIDEODB_CONTENT_TVSHOWS)
      {
        strSQL=FormatSQL("select genre.idgenre,genre.strgenre,path.strPath from genre,genrelinktvshow,tvshow,tvshowlinkpath,files,path where genre.idGenre=genrelinkTvShow.idGenre and genrelinkTvShow.idShow = tvshow.idshow and files.idPath=tvshowlinkpath.idPath and tvshowlinkpath.idShow=tvshow.idShow and path.idPath = files.idPath");
      }
      else if (idContent == VIDEODB_CONTENT_MUSICVIDEOS)
      {
        strSQL=FormatSQL("select genre.idgenre,genre.strgenre,path.strPath,musicvideo.c%02d from genre,genrelinkmusicvideo,musicvideo,files,path where genre.idGenre=genrelinkmusicvideo.idGenre and genrelinkmusicvideo.idMVideo = musicvideo.idmvideo and musicvideo.idfile=files.idfile and path.idpath = files.idpath",VIDEODB_ID_MUSICVIDEO_PLAYCOUNT);
      }
    }
    else
    {
      if (idContent == VIDEODB_CONTENT_MOVIES)
      {
        strSQL=FormatSQL("select genre.idgenre,genre.strgenre,count(1),count(movie.c%02d) from genre,genrelinkmovie,movie where genre.idgenre=genrelinkmovie.idgenre and genrelinkmovie.idmovie=movie.idmovie group by genre.idgenre", VIDEODB_ID_PLAYCOUNT);
      }
      else if (idContent == VIDEODB_CONTENT_TVSHOWS)
      {
        strSQL=FormatSQL("select distinct genre.idgenre,genre.strgenre from genre,genrelinktvshow,tvshow where genre.idGenre=genrelinkTvShow.idGenre and genrelinkTvShow.idShow = tvshow.idshow");
      }
      else if (idContent == VIDEODB_CONTENT_MUSICVIDEOS)
      {
        strSQL=FormatSQL("select genre.idgenre,genre.strgenre,count(1),count(musicvideo.c%02d) from genre,genrelinkmusicvideo,musicvideo where genre.idgenre=genrelinkmusicvideo.idgenre and genrelinkmusicvideo.idmvideo=musicvideo.idmvideo group by genre.idgenre", VIDEODB_ID_MUSICVIDEO_PLAYCOUNT);
      }
    }

    // run query
    CLog::Log(LOGDEBUG, "%s query: %s", __FUNCTION__, strSQL.c_str());
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return true;
    }

    if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
    {
      map<long, pair<CStdString,int> > mapGenres;
      map<long, pair<CStdString,int> >::iterator it;
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
          {
            if (idContent == VIDEODB_CONTENT_MOVIES || idContent == VIDEODB_CONTENT_MUSICVIDEOS)
              mapGenres.insert(pair<long, pair<CStdString,int> >(lGenreId, pair<CStdString,int>(strGenre,m_pDS->fv(3).get_asInteger())));
            else
              mapGenres.insert(pair<long, pair<CStdString,int> >(lGenreId, pair<CStdString,int>(strGenre,0)));
          }
        }
        m_pDS->next();
      }
      m_pDS->close();

      for (it=mapGenres.begin();it != mapGenres.end();++it)
      {
        CFileItemPtr pItem(new CFileItem(it->second.first));
        CStdString strDir;
        strDir.Format("%ld/", it->first);
        pItem->m_strPath=strBaseDir + strDir;
        pItem->m_bIsFolder=true;
        if (idContent == VIDEODB_CONTENT_MOVIES || idContent == VIDEODB_CONTENT_MUSICVIDEOS)
          pItem->GetVideoInfoTag()->m_playCount = it->second.second;
        if (!items.Contains(pItem->m_strPath))
        {
          pItem->SetLabelPreformated(true);
          items.Add(pItem);
        }
      }
    }
    else
    {
      while (!m_pDS->eof())
      {
        CFileItemPtr pItem(new CFileItem(m_pDS->fv("genre.strgenre").get_asString()));
        CStdString strDir;
        strDir.Format("%ld/", m_pDS->fv("genre.idgenre").get_asLong());
        pItem->m_strPath=strBaseDir + strDir;
        pItem->m_bIsFolder=true;
        pItem->SetLabelPreformated(true);
        if (idContent == VIDEODB_CONTENT_MOVIES || idContent==VIDEODB_CONTENT_MUSICVIDEOS)
        {
          // fv(3) is the number of videos watched, fv(2) is the total number.  We set the playcount
          // only if the number of videos watched is equal to the total number (i.e. every video watched)
          pItem->GetVideoInfoTag()->m_playCount = (m_pDS->fv(3).get_asInteger() == m_pDS->fv(2).get_asInteger()) ? 1 : 0;
        }
        items.Add(pItem);
        m_pDS->next();
      }
      m_pDS->close();
    }

//    CLog::Log(LOGDEBUG, "%s Time: %d ms", timeGetTime() - time);
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}

bool CVideoDatabase::GetStudiosNav(const CStdString& strBaseDir, CFileItemList& items, long idContent)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL;
    if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
    {
      if (idContent == VIDEODB_CONTENT_MOVIES)
        strSQL=FormatSQL("select studio.idstudio,studio.strstudio,path.strPath,movie.c%02d from studio,studiolinkmovie,movie,path,files where studio.idStudio=studiolinkmovie.idstudio and studiolinkMovie.idMovie = movie.idMovie and files.idFile=movie.idFile and path.idPath = files.idPath",VIDEODB_ID_PLAYCOUNT);
      else if (idContent == VIDEODB_CONTENT_MUSICVIDEOS)
        strSQL=FormatSQL("select studio.idstudio,studio.strstudio,path.strPath,musicvideo.c%02d from studio,studiolinkmusicvideo,musicvideo,path,files where studio.idStudio=studiolinkmusicvideo.idstudio and studiolinkmusicvideo.idMVideo = musicvideo.idMVideo and files.idFile=musicvideo.idFile and path.idPath = files.idPath",VIDEODB_ID_MUSICVIDEO_PLAYCOUNT);
    }
    else
    {
      if (idContent == VIDEODB_CONTENT_MOVIES)
        strSQL=FormatSQL("select studio.idstudio,studio.strstudio,count(1),count(movie.c%02d) from studio,studiolinkmovie,movie where studio.idstudio=studiolinkMovie.idstudio and studiolinkMovie.idMovie = movie.idMovie group by studio.idstudio", VIDEODB_ID_PLAYCOUNT);
      else if (idContent == VIDEODB_CONTENT_MUSICVIDEOS)
        strSQL=FormatSQL("select studio.idstudio,studio.strstudio,count(1),count(musicvideo.c%02d) from studio,studiolinkmusicvideo,musicvideo where studio.idstudio=studiolinkmusicvideo.idstudio and studiolinkmusicvideo.idMVideo = musicvideo.idMVideo group by studio.idstudio", VIDEODB_ID_MUSICVIDEO_PLAYCOUNT);
    }

    // run query
    CLog::Log(LOGDEBUG, "%s query: %s", __FUNCTION__, strSQL.c_str());
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return true;
    }

    if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
    {
      map<long, pair<CStdString,int> > mapStudios;
      map<long, pair<CStdString,int> >::iterator it;
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
            mapStudios.insert(pair<long, pair<CStdString,int> >(lStudioId, pair<CStdString,int>(strStudio,m_pDS->fv(3).get_asInteger())));
        }
        m_pDS->next();
      }
      m_pDS->close();

      for (it=mapStudios.begin();it != mapStudios.end();++it)
      {
        CFileItemPtr pItem(new CFileItem(it->second.first));
        CStdString strDir;
        strDir.Format("%ld/", it->first);
        pItem->m_strPath=strBaseDir + strDir;
        pItem->m_bIsFolder=true;
        pItem->GetVideoInfoTag()->m_playCount = it->second.second;
        if (!items.Contains(pItem->m_strPath))
        {
          pItem->SetLabelPreformated(true);
          items.Add(pItem);
        }
      }
    }
    else
    {
      while (!m_pDS->eof())
      {
        CFileItemPtr pItem(new CFileItem(m_pDS->fv("studio.strstudio").get_asString()));
        CStdString strDir;
        strDir.Format("%ld/", m_pDS->fv("studio.idstudio").get_asLong());
        pItem->m_strPath=strBaseDir + strDir;
        pItem->m_bIsFolder=true;
        pItem->SetLabelPreformated(true);
        if (idContent == VIDEODB_CONTENT_MOVIES || idContent == VIDEODB_CONTENT_MUSICVIDEOS)
        {
          // fv(3) is the number of videos watched, fv(2) is the total number.  We set the playcount
          // only if the number of videos watched is equal to the total number (i.e. every video watched)
          pItem->GetVideoInfoTag()->m_playCount = (m_pDS->fv(3).get_asInteger() == m_pDS->fv(2).get_asInteger()) ? 1 : 0;
        }
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
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}

bool CVideoDatabase::GetMusicVideoAlbumsNav(const CStdString& strBaseDir, CFileItemList& items, long idArtist)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL;
    if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
    {
      strSQL=FormatSQL("select musicvideo.c%02d,musicvideo.idMVideo,actors.strActor,path.strPath from musicvideo,path,files join artistlinkmusicvideo on artistlinkmusicvideo.idmvideo=musicvideo.idmvideo join actors on actors.idActor=artistlinkmusicvideo.idartist join files on files.idFile = musicvideo.idfile join path on path.idpath = files.idpath",VIDEODB_ID_MUSICVIDEO_ALBUM);
    }
    else
    {
      strSQL=FormatSQL("select musicvideo.c%02d,musicvideo.idMVideo,actors.strActor from musicvideo join artistlinkmusicvideo on artistlinkmusicvideo.idmvideo=musicvideo.idmvideo join actors on actors.idActor=artistlinkmusicvideo.idartist",VIDEODB_ID_MUSICVIDEO_ALBUM);
    }
    if (idArtist > -1)
      strSQL += FormatSQL(" where artistlinkmusicvideo.idartist=%u",idArtist);

    strSQL += FormatSQL(" group by musicvideo.c%02d",VIDEODB_ID_MUSICVIDEO_ALBUM);

    // run query
    CLog::Log(LOGDEBUG, "%s query: %s", __FUNCTION__, strSQL.c_str());
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return true;
    }

    if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
    {
      map<long, pair<CStdString,CStdString> > mapAlbums;
      map<long, pair<CStdString,CStdString> >::iterator it;
      while (!m_pDS->eof())
      {
        long lMVidId = m_pDS->fv(1).get_asLong();
        CStdString strAlbum = m_pDS->fv(0).get_asString();
        it = mapAlbums.find(lMVidId);
        // was this genre already found?
        if (it == mapAlbums.end())
        {
          // check path
          CStdString strPath;
          if (g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv("path.strPath").get_asString()),g_settings.m_videoSources))
            mapAlbums.insert(make_pair(lMVidId, make_pair(strAlbum,m_pDS->fv(2).get_asString())));
        }
        m_pDS->next();
      }
      m_pDS->close();

      for (it=mapAlbums.begin();it != mapAlbums.end();++it)
      {
        CFileItemPtr pItem(new CFileItem(it->second.first));
        CStdString strDir;
        strDir.Format("%ld/", it->first);
        pItem->m_strPath=strBaseDir + strDir;
        pItem->m_bIsFolder=true;
        pItem->SetLabelPreformated(true);
        if (!items.Contains(pItem->m_strPath))
        {
          CStdString strThumb = CUtil::GetCachedAlbumThumb(pItem->GetLabel(),it->second.second);
          if (CFile::Exists(strThumb))
            pItem->SetThumbnailImage(strThumb);
          items.Add(pItem);
        }
      }
    }
    else
    {
      while (!m_pDS->eof())
      {
        CFileItemPtr pItem(new CFileItem(m_pDS->fv(0).get_asString()));
        CStdString strDir;
        strDir.Format("%ld/", m_pDS->fv(1).get_asLong());
        pItem->m_strPath=strBaseDir + strDir;
        pItem->m_bIsFolder=true;
        pItem->SetLabelPreformated(true);
        if (!items.Contains(pItem->m_strPath))
        {
          pItem->GetVideoInfoTag()->m_strArtist = m_pDS->fv(2).get_asString();
          CStdString strThumb = CUtil::GetCachedAlbumThumb(pItem->GetLabel(),pItem->GetVideoInfoTag()->m_strArtist);
          if (CFile::Exists(strThumb))
            pItem->SetThumbnailImage(strThumb);
          items.Add(pItem);
        }
        m_pDS->next();
      }
      m_pDS->close();
    }
    if (idArtist > -1 && items.Size())
    {
      if (CFile::Exists(items[0]->GetCachedFanart()))
        items.SetProperty("fanart_image",items[0]->GetCachedFanart());
    }

//    CLog::Log(LOGDEBUG, __FUNCTION__" Time: %d ms", timeGetTime() - time);
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}

bool CVideoDatabase::GetWritersNav(const CStdString& strBaseDir, CFileItemList& items, long idContent)
{
  return GetPeopleNav(strBaseDir, items, "studio", idContent);
}

bool CVideoDatabase::GetDirectorsNav(const CStdString& strBaseDir, CFileItemList& items, long idContent)
{
  return GetPeopleNav(strBaseDir, items, "director", idContent);
}

bool CVideoDatabase::GetActorsNav(const CStdString& strBaseDir, CFileItemList& items, long idContent)
{
  if (GetPeopleNav(strBaseDir, items, (idContent == VIDEODB_CONTENT_MUSICVIDEOS) ? "artist" : "actor", idContent))
  { // set thumbs - ideally this should be in the normal thumb setting routines
    for (int i = 0; i < items.Size(); i++)
    {
      CFileItemPtr pItem = items[i];
      if (idContent == VIDEODB_CONTENT_MUSICVIDEOS)
      {
        if (CFile::Exists(pItem->GetCachedArtistThumb()))
          pItem->SetThumbnailImage(pItem->GetCachedArtistThumb());
        else
          pItem->SetThumbnailImage("DefaultArtistBig.png");
        if (CFile::Exists(pItem->GetCachedFanart()))
          pItem->SetProperty("fanart_image",pItem->GetCachedFanart());
      }
      else
      {
        if (CFile::Exists(pItem->GetCachedActorThumb()))
          pItem->SetThumbnailImage(pItem->GetCachedActorThumb());
        else
          pItem->SetThumbnailImage("DefaultActorBig.png");
      }
    }
    return true;
  }
  return false;
}

bool CVideoDatabase::GetPeopleNav(const CStdString& strBaseDir, CFileItemList& items, const CStdString &type, long idContent)
{
  if (NULL == m_pDB.get()) return false;
  if (NULL == m_pDS.get()) return false;

  try
  {
    // TODO: This routine (and probably others at this same level) use playcount as a reference to filter on at a later
    //       point.  This means that we *MUST* filter these levels as you'll get double ups.  Ideally we'd allow playcount
    //       to filter through as we normally do for tvshows to save this happening.
    //       Also, we apply this same filtering logic to the locked or unlocked paths to prevent these from showing.
    //       Whether or not this should happen is a tricky one - it complicates all the high level categories (everything
    //       above titles).

    // General routine that the other actor/director/writer routines call

    // get primary genres for movies
    CStdString strSQL;
    if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
    {
      if (idContent == VIDEODB_CONTENT_MOVIES)
        strSQL=FormatSQL("select actors.idActor,actors.strActor,actors.strThumb,path.strPath,movie.c%02d from actors,%slinkmovie,movie,path,files where actors.idActor=%slinkMovie.id%s and %slinkMovie.idMovie = movie.idMovie and files.idFile=movie.idFile and path.idPath = files.idPath", VIDEODB_ID_PLAYCOUNT, type.c_str(), type.c_str(), type.c_str(), type.c_str());
      else if (idContent == VIDEODB_CONTENT_TVSHOWS)
        strSQL=FormatSQL("select actors.idActor,actors.strActor,actors.strThumb,path.strPath from actors,%slinktvshow,tvshow,path,files,episode,tvshowlinkepisode where actors.idActor=%slinktvshow.id%s and %slinktvshow.idShow = tvshow.idShow and files.idFile=episode.idFile and tvshowlinkepisode.idShow=tvshow.idShow and episode.idEpisode=tvshowlinkepisode.idEpisode and path.idPath = files.idPath", type.c_str(), type.c_str(), type.c_str(), type.c_str());
      else if (idContent == VIDEODB_CONTENT_EPISODES)
        strSQL=FormatSQL("select actors.idActor,actors.strActor,actors.strThumb,path.strPath,episode.c%02d from actors,%slinkepisode,episode,path,files where actors.idActor=%slinkepisode.id%s and %slinkepisode.idEpisode = episode.idEpisode and files.idFile=episode.idFile and path.idPath = files.idPath", VIDEODB_ID_EPISODE_PLAYCOUNT, type.c_str(), type.c_str(), type.c_str(), type.c_str());
      else if (idContent == VIDEODB_CONTENT_MUSICVIDEOS)
        strSQL=FormatSQL("select actors.idActor,actors.strActor,actors.strThumb,path.strPath,musicvideo.c%02d from actors,%slinkmusicvideo,musicvideo,path,files where actors.idActor=%slinkmusicvideo.id%s and %slinkmusicvideo.idMVideo = musicvideo.idMVideo and files.idFile=musicvideo.idFile and path.idPath = files.idPath", VIDEODB_ID_MUSICVIDEO_PLAYCOUNT, type.c_str(), type.c_str(), type.c_str(), type.c_str());
    }
    else
    {
      if (idContent == VIDEODB_CONTENT_MOVIES)
        strSQL=FormatSQL("select actors.idactor,actors.strActor,actors.strThumb,count(1),count(movie.c%02d) from %slinkmovie,actors,movie where actors.idactor=%slinkmovie.id%s and %slinkmovie.idmovie=movie.idmovie group by actors.idactor", VIDEODB_ID_PLAYCOUNT, type.c_str(), type.c_str(), type.c_str(), type.c_str());
      else if (idContent == VIDEODB_CONTENT_TVSHOWS)
        strSQL=FormatSQL("select distinct actors.idActor,actors.strActor,actors.strThumb from actors,%slinktvshow,tvshow where actors.idActor=%slinktvshow.id%s and %slinktvshow.idShow = tvshow.idShow", type.c_str(), type.c_str(), type.c_str(), type.c_str());
      else if (idContent == VIDEODB_CONTENT_EPISODES)
        strSQL=FormatSQL("select actors.idactor,actors.strActor,actors.strThumb,count(1),count(episode.c%02d) from %slinkepisode,actors,episode where actors.idactor=%slinkepisode.id%s and %slinkepisode.idEpisode=episode.idEpisode group by actors.idactor", VIDEODB_ID_EPISODE_PLAYCOUNT, type.c_str(), type.c_str(), type.c_str(), type.c_str());
      else if (idContent == VIDEODB_CONTENT_MUSICVIDEOS)
        strSQL=FormatSQL("select actors.idactor,actors.strActor,actors.strThumb,count(1),count(musicvideo.c%02d) from %slinkmusicvideo,actors,musicvideo where actors.idactor=%slinkmusicvideo.id%s and %slinkmusicvideo.idmvideo=musicvideo.idmvideo group by actors.idactor", VIDEODB_ID_MUSICVIDEO_PLAYCOUNT, type.c_str(), type.c_str(), type.c_str(), type.c_str());
    }

    // run query
    unsigned int time = timeGetTime();
    if (!m_pDS->query(strSQL.c_str())) return false;
    CLog::Log(LOGDEBUG, "%s -  query took %u ms",
              __FUNCTION__, timeGetTime() - time); time = timeGetTime();
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

      while (!m_pDS->eof())
      {
        long lActorId = m_pDS->fv(0).get_asLong();
        CActor actor;
        actor.name = m_pDS->fv(1).get_asString();
        actor.thumb = m_pDS->fv(2).get_asString();
        if (idContent != VIDEODB_CONTENT_TVSHOWS)
          actor.playcount = m_pDS->fv(3).get_asInteger();
        it = mapActors.find(lActorId);
        // is this actor already known?
        if (it == mapActors.end())
        {
          // check path
          if (g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv("path.strPath").get_asString()),g_settings.m_videoSources))
            mapActors.insert(pair<long, CActor>(lActorId, actor));
        }
        m_pDS->next();
      }
      m_pDS->close();

      for (it=mapActors.begin();it != mapActors.end();++it)
      {
        CFileItemPtr pItem(new CFileItem(it->second.name));
        CStdString strDir;
        strDir.Format("%ld/", it->first);
        pItem->m_strPath=strBaseDir + strDir;
        pItem->m_bIsFolder=true;
        pItem->GetVideoInfoTag()->m_playCount = it->second.playcount;
        pItem->GetVideoInfoTag()->m_strPictureURL.ParseString(it->second.thumb);
        items.Add(pItem);
      }
    }
    else
    {
      while (!m_pDS->eof())
      {
        try
        {
          CFileItemPtr pItem(new CFileItem(m_pDS->fv(1).get_asString()));
          CStdString strDir;
          strDir.Format("%ld/", m_pDS->fv(0).get_asLong());
          pItem->m_strPath=strBaseDir + strDir;
          pItem->m_bIsFolder=true;
          pItem->GetVideoInfoTag()->m_strPictureURL.ParseString(m_pDS->fv(2).get_asString());
          if (idContent != VIDEODB_CONTENT_TVSHOWS)
          {
            // fv(4) is the number of videos watched, fv(3) is the total number.  We set the playcount
            // only if the number of videos watched is equal to the total number (i.e. every video watched)
            pItem->GetVideoInfoTag()->m_playCount = (m_pDS->fv(4).get_asInteger() == m_pDS->fv(3).get_asInteger()) ? 1 : 0;
          }
          items.Add(pItem);
          m_pDS->next();
        }
        catch (...)
        {
          m_pDS->close();
          CLog::Log(LOGERROR, "%s: out of memory - retrieved %i items", __FUNCTION__, items.Size());
          return items.Size() > 0;
        }
      }
      m_pDS->close();
    }
    CLog::Log(LOGDEBUG, "%s item retrieval took %u ms",
              __FUNCTION__, timeGetTime() - time); time = timeGetTime();

    return true;
  }
  catch (...)
  {
    m_pDS->close();
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
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
        strSQL = FormatSQL("select movie.c%02d,path.strPath,movie.c%02d from movie join files on files.idFile=movie.idFile join path on files.idPath = path.idPath", VIDEODB_ID_YEAR,VIDEODB_ID_PLAYCOUNT);
      }
      else
      {
        strSQL = FormatSQL("select movie.c%02d,count(1),count(movie.c%02d) from movie group by movie.c%02d", VIDEODB_ID_YEAR, VIDEODB_ID_PLAYCOUNT, VIDEODB_ID_YEAR);
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
        strSQL = FormatSQL("select musicvideo.c%02d,path.strPath,musicvideo.c%02d from musicvideo join files on files.idFile=musicvideo.idFile join path on files.idPath = path.idPath", VIDEODB_ID_MUSICVIDEO_YEAR,VIDEODB_ID_MUSICVIDEO_PLAYCOUNT);
      }
      else
      {
        strSQL = FormatSQL("select musicvideo.c%02d,count(1),count(musicvideo.c%02d) from musicvideo group by musicvideo.c%02d", VIDEODB_ID_MUSICVIDEO_YEAR, VIDEODB_ID_MUSICVIDEO_PLAYCOUNT, VIDEODB_ID_MUSICVIDEO_YEAR);
      }
    }

    // run query
    CLog::Log(LOGDEBUG, "%s query: %s", __FUNCTION__, strSQL.c_str());
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return true;
    }

    if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
    {
      map<long, pair<CStdString,int> > mapYears;
      map<long, pair<CStdString,int> >::iterator it;
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
              mapYears.insert(pair<long, pair<CStdString,int> >(lYear, pair<CStdString,int>(year,m_pDS->fv(2).get_asInteger())));
            else
              mapYears.insert(pair<long, pair<CStdString,int> >(lYear, pair<CStdString,int>(year,0)));
          }
        }
        m_pDS->next();
      }
      m_pDS->close();

      for (it=mapYears.begin();it != mapYears.end();++it)
      {
        if (it->first == 0)
          continue;
        CFileItemPtr pItem(new CFileItem(it->second.first));
        CStdString strDir;
        strDir.Format("%ld/", it->first);
        pItem->m_strPath=strBaseDir + strDir;
        pItem->m_bIsFolder=true;
        if (idContent == VIDEODB_CONTENT_MOVIES || idContent == VIDEODB_CONTENT_MUSICVIDEOS)
          pItem->GetVideoInfoTag()->m_playCount = it->second.second;
        items.Add(pItem);
      }
    }
    else
    {
      while (!m_pDS->eof())
      {
        long lYear = 0;
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
        CFileItemPtr pItem(new CFileItem(strLabel));
        CStdString strDir;
        strDir.Format("%ld/", lYear);
        pItem->m_strPath=strBaseDir + strDir;
        pItem->m_bIsFolder=true;
        if (idContent == VIDEODB_CONTENT_MOVIES || idContent == VIDEODB_CONTENT_MUSICVIDEOS)
        {
          // fv(2) is the number of videos watched, fv(1) is the total number.  We set the playcount
          // only if the number of videos watched is equal to the total number (i.e. every video watched)
          pItem->GetVideoInfoTag()->m_playCount = (m_pDS->fv(2).get_asInteger() == m_pDS->fv(1).get_asInteger()) ? 1 : 0;
        }
        items.Add(pItem);
        m_pDS->next();
      }
      m_pDS->close();
    }

    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}

bool CVideoDatabase::GetStackedTvShowList(long idShow, CStdString& strIn)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    // look for duplicate show titles and stack them into a list
    if (idShow == -1)
      return false;
    CStdString strSQL = FormatSQL("select idShow from tvshow where c00 like (select c00 from tvshow where idShow=%u) order by idShow", idShow);
    CLog::Log(LOGDEBUG, "%s query: %s", __FUNCTION__, strSQL.c_str());
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRows = m_pDS->num_rows();
    if (iRows == 0) return false; // this should never happen!
    if (iRows > 1)
    { // more than one show, so stack them up
      strIn = "IN (";
      while (!m_pDS->eof())
      {
        strIn += FormatSQL("%u,", m_pDS->fv(0).get_asLong());
        m_pDS->next();
      }
      strIn[strIn.GetLength() - 1] = ')'; // replace last , with )
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

bool CVideoDatabase::GetSeasonsNav(const CStdString& strBaseDir, CFileItemList& items, long idActor, long idDirector, long idGenre, long idYear, long idShow)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strIn = FormatSQL("= %u", idShow);
    if (g_guiSettings.GetBool("videolibrary.removeduplicates"))
      GetStackedTvShowList(idShow, strIn);

    CStdString strSQL = FormatSQL("select episode.c%02d,path.strPath,tvshow.c%02d,count(1),count(episode.c%02d) from episode join tvshow on tvshow.idshow=tvshowlinkepisode.idshow join tvshowlinkepisode on tvshowlinkepisode.idEpisode = episode.idEpisode ", VIDEODB_ID_EPISODE_SEASON, VIDEODB_ID_TV_TITLE, VIDEODB_ID_EPISODE_PLAYCOUNT);
    CStdString joins = FormatSQL(" join tvshowlinkpath on tvshowlinkpath.idShow = tvshow.idShow join path on path.idPath = tvshowlinkpath.idPath where tvshow.idShow %s ", strIn.c_str());
    CStdString extraJoins, extraWhere;
    if (idActor != -1)
    {
      extraJoins = "join actorlinktvshow on actorlinktvshow.idshow=tvshow.idshow";
      extraWhere = FormatSQL("and actorlinktvshow.idActor=%u", idActor);
    }
    else if (idDirector != -1)
    {
      extraJoins = "join directorlinktvshow on directorlinktvshow.idshow=tvshow.idshow";
      extraWhere = FormatSQL("and directorlinktvshow.idDirector=%u",idDirector);
    }
    else if (idGenre != -1)
    {
      extraJoins = "join genrelinktvshow on genrelinktvshow.idshow=tvshow.idshow";
      extraWhere = FormatSQL("and genrelinktvshow.idGenre=%u", idGenre);
    }
    else if (idYear != -1)
    {
      extraWhere = FormatSQL("and tvshow.c%02d like '%%%u%%'", VIDEODB_ID_TV_PREMIERED, idYear);
    }
    strSQL += extraJoins + joins + extraWhere + FormatSQL(" group by episode.c%02d", VIDEODB_ID_EPISODE_SEASON);

    // run query
    CLog::Log(LOGDEBUG, "%s query: %s", __FUNCTION__, strSQL.c_str());
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return true;
    }

    // all show titles will be the same
    CStdString showTitle = m_pDS->fv(2).get_asString();

    if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
    {
      map<long, CSeason> mapSeasons;
      map<long, CSeason>::iterator it;
      while (!m_pDS->eof())
      {
        long lSeason = m_pDS->fv(0).get_asLong();
        it = mapSeasons.find(lSeason);
        // check path
        if (!g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv("path.strPath").get_asString()),g_settings.m_videoSources))
        {
          m_pDS->next();
          continue;
        }
        if (it == mapSeasons.end())
        {
          CSeason season;
          season.path = m_pDS->fv(1).get_asString();
          season.numEpisodes = m_pDS->fv(3).get_asInteger();
          season.numWatched = m_pDS->fv(4).get_asInteger();
          mapSeasons.insert(make_pair(lSeason, season));
        }
        m_pDS->next();
      }
      m_pDS->close();

      for (it=mapSeasons.begin();it != mapSeasons.end();++it)
      {
        long lSeason = it->first;
        CStdString strLabel;
        if (lSeason == 0)
          strLabel = g_localizeStrings.Get(20381);
        else
          strLabel.Format(g_localizeStrings.Get(20358),lSeason);
        CFileItemPtr pItem(new CFileItem(strLabel));
        CStdString strDir;
        strDir.Format("%ld/", it->first);
        pItem->m_strPath=strBaseDir + strDir;
        pItem->m_bIsFolder=true;
        pItem->GetVideoInfoTag()->m_strTitle = strLabel;
        pItem->GetVideoInfoTag()->m_iSeason = lSeason;
        pItem->GetVideoInfoTag()->m_iDbId = idShow;
        pItem->GetVideoInfoTag()->m_strPath = it->second.path;
        pItem->GetVideoInfoTag()->m_strShowTitle = showTitle;
        pItem->GetVideoInfoTag()->m_iEpisode = it->second.numEpisodes;
        pItem->SetProperty("watchedepisodes", it->second.numWatched);
        pItem->SetProperty("unwatchedepisodes", it->second.numEpisodes - it->second.numWatched);
        pItem->GetVideoInfoTag()->m_playCount = (it->second.numEpisodes == it->second.numWatched) ? 1 : 0;
        pItem->SetCachedSeasonThumb();
        pItem->SetOverlayImage(CGUIListItem::ICON_OVERLAY_UNWATCHED, (pItem->GetVideoInfoTag()->m_playCount > 0) && (pItem->GetVideoInfoTag()->m_iEpisode > 0));
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
        CFileItemPtr pItem(new CFileItem(strLabel));
        CStdString strDir;
        strDir.Format("%ld/", lSeason);
        pItem->m_strPath=strBaseDir + strDir;
        pItem->m_bIsFolder=true;
        pItem->GetVideoInfoTag()->m_strTitle = strLabel;
        pItem->GetVideoInfoTag()->m_iSeason = lSeason;
        pItem->GetVideoInfoTag()->m_iDbId = idShow;
        pItem->GetVideoInfoTag()->m_strPath = m_pDS->fv(1).get_asString();
        pItem->GetVideoInfoTag()->m_strShowTitle = showTitle;
        int totalEpisodes = m_pDS->fv(3).get_asInteger();
        int watchedEpisodes = m_pDS->fv(4).get_asInteger();
        pItem->GetVideoInfoTag()->m_iEpisode = totalEpisodes;
        pItem->SetProperty("watchedepisodes", watchedEpisodes);
        pItem->SetProperty("unwatchedepisodes", totalEpisodes - watchedEpisodes);
        pItem->GetVideoInfoTag()->m_playCount = (totalEpisodes == watchedEpisodes) ? 1 : 0;
        pItem->SetCachedSeasonThumb();
        pItem->SetOverlayImage(CGUIListItem::ICON_OVERLAY_UNWATCHED, (pItem->GetVideoInfoTag()->m_playCount > 0) && (pItem->GetVideoInfoTag()->m_iEpisode > 0));
        items.Add(pItem);
        m_pDS->next();
      }
      m_pDS->close();
    }
    // now add any linked movies
    CStdString where = FormatSQL("join movielinktvshow on movielinktvshow.idMovie=movieview.idMovie where movielinktvshow.idShow %s", strIn.c_str());
    GetMoviesByWhere("videodb://1/2/", where, items);
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}

bool CVideoDatabase::GetMoviesNav(const CStdString& strBaseDir, CFileItemList& items, long idGenre, long idYear, long idActor, long idDirector, long idStudio)
{
  CStdString where;
  if (idGenre != -1)
    where = FormatSQL("join genrelinkmovie on genrelinkmovie.idmovie=movieview.idmovie where genrelinkmovie.idGenre=%u", idGenre);
  else if (idStudio != -1)
    where = FormatSQL("join studiolinkmovie on studiolinkmovie.idmovie=movieview.idmovie where studiolinkmovie.idstudio=%u", idStudio);
  else if (idDirector != -1)
    where = FormatSQL("join directorlinkmovie on directorlinkmovie.idmovie=movieview.idmovie where directorlinkmovie.idDirector=%u", idDirector);
  else if (idYear !=-1)
    where = FormatSQL("where c%02d='%i'",VIDEODB_ID_YEAR,idYear);
  else if (idActor != -1)
    where = FormatSQL("join actorlinkmovie on actorlinkmovie.idmovie=movieview.idmovie join actors on actors.idActor=actorlinkmovie.idActor where actors.idActor=%u",idActor);
  return GetMoviesByWhere(strBaseDir, where, items);
}

bool CVideoDatabase::GetMoviesByWhere(const CStdString& strBaseDir, const CStdString &where, CFileItemList& items)
{
  try
  {
    DWORD time = timeGetTime();
    movieTime = 0;
    castTime = 0;

    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL = "select * from movieview " + where;

    // run query
    CLog::Log(LOGDEBUG, "%s query: %s", __FUNCTION__, strSQL.c_str());
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return true;
    }

    CLog::Log(LOGDEBUG,"Time for actual SQL query = %d",
              timeGetTime() - time); time = timeGetTime();

    // get data from returned rows
    items.Reserve(iRowsFound);
    while (!m_pDS->eof())
    {
      long lMovieId = m_pDS->fv("idMovie").get_asLong();
      CVideoInfoTag movie = GetDetailsForMovie(m_pDS);
      if (g_settings.m_vecProfiles[0].getLockMode() == LOCK_MODE_EVERYONE ||
          g_passwordManager.bMasterUser                                   ||
          g_passwordManager.IsDatabasePathUnlocked(movie.m_strPath, g_settings.m_videoSources))
      {
        CFileItemPtr pItem(new CFileItem(movie));
        pItem->m_strPath.Format("%s%ld", strBaseDir.c_str(), lMovieId);
        pItem->SetOverlayImage(CGUIListItem::ICON_OVERLAY_UNWATCHED,movie.m_playCount > 0);
        items.Add(pItem);
      }
      m_pDS->next();
    }

    CLog::Log(LOGDEBUG,"Time to retrieve movies from dataset = %d",
              timeGetTime() - time);

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

bool CVideoDatabase::GetTvShowsNav(const CStdString& strBaseDir, CFileItemList& items, long idGenre, long idYear, long idActor, long idDirector)
{
  CStdString where;
  if (idGenre != -1)
    where = FormatSQL("join genrelinktvshow on genrelinktvshow.idshow=tvshowview.idshow where genrelinktvshow.idGenre=%u ", idGenre);
  else if (idDirector != -1)
    where = FormatSQL("join directorlinktvshow on directorlinktvshow.idshow=tvshowview.idshow where directorlinktvshow.idDirector=%u", idDirector);
  else if (idYear != -1)
    where = FormatSQL("where c%02d like '%%%u%%'", VIDEODB_ID_TV_PREMIERED,idYear);
  else if (idActor != -1)
    where = FormatSQL("join actorlinktvshow on actorlinktvshow.idshow=tvshowview.idshow join actors on actors.idActor=actorlinktvshow.idActor where actors.idActor=%u",idActor);

  return GetTvShowsByWhere(strBaseDir, where, items);
}

bool CVideoDatabase::GetTvShowsByWhere(const CStdString& strBaseDir, const CStdString &where, CFileItemList& items)
{
  try
  {
    DWORD time = timeGetTime();
    movieTime = 0;

    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL = "select * from tvshowview " + where;
    // run query
    CLog::Log(LOGDEBUG, "%s query: %s", __FUNCTION__, strSQL.c_str());
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return true;
    }

    CLog::Log(LOGDEBUG,"Time for actual SQL query = %d",
              timeGetTime() - time); time = timeGetTime();

    // get data from returned rows
    items.Reserve(iRowsFound);

    while (!m_pDS->eof())
    {
      long lShowId = m_pDS->fv("tvshow.idShow").get_asLong();

      CVideoInfoTag movie = GetDetailsForTvShow(m_pDS, false);
      if (!g_advancedSettings.m_bVideoLibraryHideEmptySeries || movie.m_iEpisode > 0)
      {
        CFileItemPtr pItem(new CFileItem(movie));
        pItem->m_strPath.Format("%s%ld/", strBaseDir.c_str(), lShowId);
        pItem->m_dateTime.SetFromDateString(movie.m_strPremiered);
        pItem->GetVideoInfoTag()->m_iYear = pItem->m_dateTime.GetYear();
        pItem->SetProperty("watchedepisodes", movie.m_playCount);
        pItem->SetProperty("unwatchedepisodes", movie.m_iEpisode - movie.m_playCount);
        pItem->GetVideoInfoTag()->m_playCount = (movie.m_iEpisode == movie.m_playCount) ? 1 : 0;
        pItem->SetOverlayImage(CGUIListItem::ICON_OVERLAY_UNWATCHED, (pItem->GetVideoInfoTag()->m_playCount > 0) && (pItem->GetVideoInfoTag()->m_iEpisode > 0));
        pItem->CacheFanart();
        if (CFile::Exists(pItem->GetCachedFanart()))
          pItem->SetProperty("fanart_image",pItem->GetCachedFanart());
        items.Add(pItem);
      }
      m_pDS->next();
    }

    CLog::Log(LOGDEBUG,"Time to retrieve movies from dataset = %d",
              timeGetTime() - time);
    if (g_guiSettings.GetBool("videolibrary.removeduplicates"))
    {
      CStdString order(where);
      bool maintainOrder = (size_t)order.ToLower().Find("order by") != CStdString::npos;
      Stack(items, VIDEODB_CONTENT_TVSHOWS, maintainOrder);
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

void CVideoDatabase::Stack(CFileItemList& items, VIDEODB_CONTENT_TYPE type, bool maintainSortOrder /* = false */)
{
  if (maintainSortOrder)
  {
    // save current sort order
    for (int i = 0; i < items.Size(); i++)
      items[i]->m_iprogramCount = i;
  }

  switch (type)
  {
    case VIDEODB_CONTENT_TVSHOWS:
    {
      // sort by Title
      items.Sort(SORT_METHOD_VIDEO_TITLE, SORT_ORDER_ASC);

      int i = 0;
      while (i < items.Size())
      {
        CFileItemPtr pItem = items.Get(i);
        CStdString strTitle = pItem->GetVideoInfoTag()->m_strTitle;
        CStdString strFanArt = pItem->GetProperty("fanart_image");

        int j = i + 1;
        bool bStacked = false;
        while (j < items.Size())
        {
          CFileItemPtr jItem = items.Get(j);

          // matching title? append information
          if (jItem->GetVideoInfoTag()->m_strTitle.Equals(strTitle))
          {
            bStacked = true;

            // increment episode counts
            pItem->GetVideoInfoTag()->m_iEpisode += jItem->GetVideoInfoTag()->m_iEpisode;
            pItem->IncrementProperty("watchedepisodes", jItem->GetPropertyInt("watchedepisodes"));
            pItem->IncrementProperty("unwatchedepisodes", jItem->GetPropertyInt("unwatchedepisodes"));

            // check for fanart if not already set
            if (strFanArt.IsEmpty())
              strFanArt = jItem->GetProperty("fanart_image");

            // remove duplicate entry
            items.Remove(j);
          }
          // no match? exit loop
          else
            break;
        }
        // update playcount and fanart
        if (bStacked)
        {
          pItem->GetVideoInfoTag()->m_playCount = (pItem->GetVideoInfoTag()->m_iEpisode == pItem->GetPropertyInt("watchedepisodes")) ? 1 : 0;
          pItem->SetOverlayImage(CGUIListItem::ICON_OVERLAY_UNWATCHED, (pItem->GetVideoInfoTag()->m_playCount > 0) && (pItem->GetVideoInfoTag()->m_iEpisode == 0));
          if (!strFanArt.IsEmpty())
            pItem->SetProperty("fanart_image", strFanArt);
        }
        // increment i to j which is the next item
        i = j;
      }
    }
    break;
    case VIDEODB_CONTENT_EPISODES:
    {
      // sort by ShowTitle, Episode, Filename
      items.Sort(SORT_METHOD_EPISODE, SORT_ORDER_ASC);

      int i = 0;
      while (i < items.Size())
      {
        CFileItemPtr pItem = items.Get(i);
        CStdString strPath = pItem->GetVideoInfoTag()->m_strPath;
        int iSeason = pItem->GetVideoInfoTag()->m_iSeason;
        int iEpisode = pItem->GetVideoInfoTag()->m_iEpisode;
        //CStdString strFanArt = pItem->GetProperty("fanart_image");

        // do we have a dvd folder, ie foo/VIDEO_TS.IFO or foo/VIDEO_TS/VIDEO_TS.IFO
        CStdString strFileNameAndPath = pItem->GetVideoInfoTag()->m_strFileNameAndPath;
        bool bDvdFolder = strFileNameAndPath.Right(12).Equals("VIDEO_TS.IFO");

        vector<CStdString> paths;
        paths.push_back(strFileNameAndPath);
        CLog::Log(LOGDEBUG, "Stack episode (%i,%i):[%s]", iSeason, iEpisode, paths[0].c_str());

        int j = i + 1;
        int iPlayCount = pItem->GetVideoInfoTag()->m_playCount;
        while (j < items.Size())
        {
          CFileItemPtr jItem = items.Get(j);
          const CVideoInfoTag *jTag = jItem->GetVideoInfoTag();
          CStdString jFileNameAndPath = jTag->m_strFileNameAndPath;

          CLog::Log(LOGDEBUG, " *testing (%i,%i):[%s]", jTag->m_iSeason, jTag->m_iEpisode, jFileNameAndPath.c_str());
          // compare path, season, episode
          if (
            jTag &&
            jTag->m_strPath.Equals(strPath) &&
            jTag->m_iSeason == iSeason &&
            jTag->m_iEpisode == iEpisode
            )
          {
            // keep checking to see if this is dvd folder
            if (!bDvdFolder)
            {
              bDvdFolder = jFileNameAndPath.Right(12).Equals("VIDEO_TS.IFO");
              // if we have a dvd folder, we stack differently
              if (bDvdFolder)
              {
                // remove all the other items and ONLY show the VIDEO_TS.IFO file
                paths.empty();
                paths.push_back(jFileNameAndPath);
              }
              else
              {
                // increment playcount
                iPlayCount += jTag->m_playCount;

                /* episodes dont have fanart yet
                // check for fanart if not already set
                if (strFanArt.IsEmpty())
                  strFanArt = jItem->GetProperty("fanart_image");
                */
                paths.push_back(jFileNameAndPath);
              }
            }

            // remove duplicate entry
            jTag = NULL;
            items.Remove(j);
          }
          // no match? exit loop
          else
            break;
        }
        // update playcount and fanart if we have a stacked entry
        if (paths.size() > 1)
        {
          CStackDirectory dir;
          CStdString strStack;
          dir.ConstructStackPath(paths, strStack);
          pItem->GetVideoInfoTag()->m_strFileNameAndPath = strStack;
          pItem->GetVideoInfoTag()->m_playCount = iPlayCount;
          pItem->SetOverlayImage(CGUIListItem::ICON_OVERLAY_UNWATCHED, (pItem->GetVideoInfoTag()->m_playCount > 0) && (pItem->GetVideoInfoTag()->m_iEpisode > 0));

          /* episodes dont have fanart yet
          if (!strFanArt.IsEmpty())
            pItem->SetProperty("fanart_image", strFanArt);
          */
        }
        // increment i to j which is the next item
        i = j;
      }
    }
    break;

    // stack other types later
    default:
      break;
  }
  if (maintainSortOrder)
  {
    // restore original sort order - essential for smartplaylists
    items.Sort(SORT_METHOD_PROGRAM_COUNT, SORT_ORDER_ASC);
  }
}

bool CVideoDatabase::GetEpisodesNav(const CStdString& strBaseDir, CFileItemList& items, long idGenre, long idYear, long idActor, long idDirector, long idShow, long idSeason)
{
  CStdString strIn = FormatSQL("= %u", idShow);
  if (g_guiSettings.GetBool("videolibrary.removeduplicates"))
    GetStackedTvShowList(idShow, strIn);
  CStdString where = FormatSQL("where idShow %s",strIn.c_str());
  if (idGenre != -1)
    where = FormatSQL("join genrelinktvshow on genrelinktvshow.idShow=episodeview.idShow where episodeview.idShow=%u and genrelinktvshow.idgenre=%u",idShow,idGenre);
  else if (idDirector != -1)
    where = FormatSQL("join directorlinktvshow on directorlinktvshow.idshow=episodeview.idshow where episodeview.idShow=%u and directorlinktvshow.iddirector=%u",idShow,idDirector);
  else if (idYear !=-1)
    where=FormatSQL("where idShow=%u and premiered like '%%%u%%'",idShow,idYear);
  else if (idActor != -1)
    where = FormatSQL("actorlinktvshow on actorlinktvshow.idshow = episodeview.idshow where episodeview.idShow=%u and actorlinktvshow.idactor=%u",idShow,idActor);

  if (idSeason != -1)
  {
    if (idSeason != 0) // season = 0 indicates a special - we grab all specials here (see below)
      where += FormatSQL(" and (c%02d=%u or (c%02d=0 and (c%02d=0 or c%02d=%u)))",VIDEODB_ID_EPISODE_SEASON,idSeason,VIDEODB_ID_EPISODE_SEASON, VIDEODB_ID_EPISODE_SORTSEASON, VIDEODB_ID_EPISODE_SORTSEASON,idSeason);
    else
      where += FormatSQL(" and c%02d=%u",VIDEODB_ID_EPISODE_SEASON,idSeason);
  }

  // we always append show, season + episode in GetEpisodesByWhere
  CStdString parent, grandParent;
  CUtil::GetParentPath(strBaseDir,parent);
  CUtil::GetParentPath(parent,grandParent);

  bool ret = GetEpisodesByWhere(grandParent, where, items);

  if (idSeason == -1)
  { // add any linked movies
    CStdString where = FormatSQL("join movielinktvshow on movielinktvshow.idMovie=movieview.idMovie where movielinktvshow.idShow %s", strIn.c_str());
    GetMoviesByWhere("videodb://1/2/", where, items);
  }
  return ret;
}

bool CVideoDatabase::GetEpisodesByWhere(const CStdString& strBaseDir, const CStdString &where, CFileItemList& items, bool appendFullShowPath /* = true */)
{
  try
  {
    DWORD time = timeGetTime();
    movieTime = 0;
    castTime = 0;

    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL = "select * from episodeview " + where;

    // run query
    CLog::Log(LOGDEBUG, "%s query: %s", __FUNCTION__, strSQL.c_str());
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return true;
    }

    CLog::Log(LOGDEBUG,"Time for actual SQL query = %d",
              timeGetTime() - time); time = timeGetTime();

    // get data from returned rows
    items.Reserve(iRowsFound);
    while (!m_pDS->eof())
    {
      long lEpisodeId = m_pDS->fv("idEpisode").get_asLong();
      long lShowId = m_pDS->fv("idShow").get_asLong();

      CVideoInfoTag movie = GetDetailsForEpisode(m_pDS);
      CFileItemPtr pItem(new CFileItem(movie));
      if (appendFullShowPath)
        pItem->m_strPath.Format("%s%ld/%ld/%ld",strBaseDir.c_str(), lShowId, movie.m_iSeason,lEpisodeId);
      else
        pItem->m_strPath.Format("%s%ld",strBaseDir.c_str(), lEpisodeId);

      pItem->SetOverlayImage(CGUIListItem::ICON_OVERLAY_UNWATCHED,movie.m_playCount > 0);
      pItem->m_dateTime.SetFromDateString(movie.m_strFirstAired);
      pItem->GetVideoInfoTag()->m_iYear = pItem->m_dateTime.GetYear();
      if (CFile::Exists(pItem->GetCachedEpisodeThumb()))
        pItem->SetThumbnailImage(pItem->GetCachedEpisodeThumb());
      items.Add(pItem);

      m_pDS->next();
    }

    CLog::Log(LOGDEBUG,"Time to retrieve movies from dataset = %d",
              timeGetTime() - time);
    if (g_guiSettings.GetBool("videolibrary.removeduplicates"))
    {
      CStdString order(where);
      bool maintainOrder = (size_t)order.ToLower().Find("order by") != CStdString::npos;
      Stack(items, VIDEODB_CONTENT_EPISODES, maintainOrder);
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


bool CVideoDatabase::GetMusicVideosNav(const CStdString& strBaseDir, CFileItemList& items, long idGenre, long idYear, long idArtist, long idDirector, long idStudio, long idAlbum)
{
  CStdString where;
  if (idGenre != -1)
    where = FormatSQL("join genrelinkmusicvideo on genrelinkmusicvideo.idmvideo=musicvideoview.idmvideo where genrelinkmusicvideo.idGenre=%u", idGenre);
  else if (idStudio != -1)
    where = FormatSQL("join studiolinkmusicvideo on studiolinkmusicvideo.idmvideo=musicvideoview.idmvideo where studiolinkmusicvideo.idstudio=%u", idStudio);
  else if (idDirector != -1)
    where = FormatSQL("join directorlinkmusicvideo on directorlinkmusicvideo.idmvideo=musicvideoview.idmvideo where directorlinkmusicvideo.idDirector=%u", idDirector);
  else if (idYear !=-1)
    where = FormatSQL("where c%02d='%i'",VIDEODB_ID_MUSICVIDEO_YEAR,idYear);
  else if (idArtist != -1)
    where = FormatSQL("join artistlinkmusicvideo on artistlinkmusicvideo.idmvideo=musicvideoview.idmvideo join actors on actors.idActor=artistlinkmusicvideo.idartist where actors.idActor=%u",idArtist);
  else if (idAlbum != -1)
    where = FormatSQL("where c%02d=(select c%02d from musicvideo where idMVideo=%u)",VIDEODB_ID_MUSICVIDEO_ALBUM,VIDEODB_ID_MUSICVIDEO_ALBUM,idAlbum);

  bool bResult = GetMusicVideosByWhere(strBaseDir, where, items);
  if (bResult && idArtist > -1 && items.Size())
  {
   if (CFile::Exists(items[0]->GetCachedFanart()))
     items.SetProperty("fanart_image",items[0]->GetCachedFanart());
  }

  return bResult;
}

bool CVideoDatabase::GetRecentlyAddedMoviesNav(const CStdString& strBaseDir, CFileItemList& items)
{
  CStdString where = FormatSQL("order by idMovie desc limit %u",RECENTLY_ADDED_LIMIT);
  return GetMoviesByWhere(strBaseDir, where, items);
}

bool CVideoDatabase::GetRecentlyAddedEpisodesNav(const CStdString& strBaseDir, CFileItemList& items)
{
  CStdString where = FormatSQL("order by idepisode desc limit %u",RECENTLY_ADDED_LIMIT);
  return GetEpisodesByWhere(strBaseDir, where, items, false);
}

bool CVideoDatabase::GetRecentlyAddedMusicVideosNav(const CStdString& strBaseDir, CFileItemList& items)
{
  CStdString where = FormatSQL("order by idmvideo desc limit %u",RECENTLY_ADDED_LIMIT);
  return GetMusicVideosByWhere(strBaseDir, where, items);
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
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strGenre.c_str());
  }
  return false;
}

bool CVideoDatabase::HasContent()
{
  return (HasContent(VIDEODB_CONTENT_MOVIES) ||
          HasContent(VIDEODB_CONTENT_TVSHOWS) ||
          HasContent(VIDEODB_CONTENT_MUSICVIDEOS));
}

bool CVideoDatabase::HasContent(VIDEODB_CONTENT_TYPE type)
{
  bool result = false;
  try
  {
    if (NULL == m_pDB.get()) return 0;
    if (NULL == m_pDS.get()) return 0;

    CStdString sql;
    if (type == VIDEODB_CONTENT_MOVIES)
      sql = "select count(1) from movie";
    else if (type == VIDEODB_CONTENT_TVSHOWS)
      sql = "select count(1) from tvshow";
    else if (type == VIDEODB_CONTENT_MUSICVIDEOS)
      sql = "select count(1) from musicvideo";
    m_pDS->query( sql.c_str() );

    if (!m_pDS->eof())
      result = (m_pDS->fv(0).get_asInteger() > 0);

    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return result;
}

int CVideoDatabase::GetMusicVideoCount(const CStdString& strWhere)
{
  try
  {
    if (NULL == m_pDB.get()) return 0;
    if (NULL == m_pDS.get()) return 0;

    CStdString strSQL;
    strSQL.Format("select count(1) as nummovies from musicvideoview %s",strWhere.c_str());
    m_pDS->query( strSQL.c_str() );

    int iResult = 0;
    if (!m_pDS->eof())
      iResult = m_pDS->fv("nummovies").get_asInteger();

    m_pDS->close();
    return iResult;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
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
    info.Reset();
    if (strPath.IsEmpty()) return false;
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strPath1;
    CUtil::GetDirectory(strPath,strPath1);
    long lPathId = GetPathId(strPath1);

    if (lPathId > -1)
    {
      CStdString strSQL=FormatSQL("select path.strContent,path.strScraper,path.scanRecursive,path.useFolderNames,path.strSettings,path.noUpdate from path where path.idPath=%u",lPathId);
      m_pDS->query( strSQL.c_str() );
    }

    iFound = 1;
    if (!m_pDS->eof())
    {
      info.strContent = m_pDS->fv("path.strContent").get_asString();
      info.strPath = m_pDS->fv("path.strScraper").get_asString();
      info.settings.LoadUserXML(m_pDS->fv("path.strSettings").get_asString());

      CScraperParser parser;
      parser.Load("special://xbmc/system/scrapers/video/" + info.strPath);
      info.strLanguage = parser.GetLanguage();
      info.strTitle = parser.GetName();

      settings.parent_name = m_pDS->fv("path.useFolderNames").get_asBool();
      settings.recurse = m_pDS->fv("path.scanRecursive").get_asInteger();
      settings.noupdate = m_pDS->fv("path.noUpdate").get_asBool();
    }
    if (info.strContent.IsEmpty())
    {
      CStdString strParent;

      while (CUtil::GetParentPath(strPath1, strParent))
      {
        iFound++;

        CStdString strSQL=FormatSQL("select path.strContent,path.strScraper,path.scanRecursive,path.useFolderNames,path.strSettings,path.noUpdate from path where strPath like '%s'",strParent.c_str());
        m_pDS->query(strSQL.c_str());
        if (!m_pDS->eof())
        {
          info.strContent = m_pDS->fv("path.strContent").get_asString();
          info.strPath = m_pDS->fv("path.strScraper").get_asString();
          info.settings.LoadUserXML(m_pDS->fv("path.strSettings").get_asString());

          CScraperParser parser;
          parser.Load("special://xbmc/system/scrapers/video/" + info.strPath);
          info.strLanguage = parser.GetLanguage();
          info.strTitle = parser.GetName();

          settings.parent_name = m_pDS->fv("path.useFolderNames").get_asBool();
          settings.recurse = m_pDS->fv("path.scanRecursive").get_asInteger();
          settings.noupdate = m_pDS->fv("path.noUpdate").get_asBool();

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
      settings.noupdate = false;
      return false;
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
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
        if (!g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv("path.strPath").get_asString()),
                                                      g_settings.m_videoSources))
        {
          m_pDS->next();
          continue;
        }

      CFileItemPtr pItem(new CFileItem(m_pDS->fv("genre.strGenre").get_asString()));
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
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strSQL.c_str());
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

      CFileItemPtr pItem(new CFileItem(m_pDS->fv("genre.strGenre").get_asString()));
      CStdString strDir;
      strDir.Format("%ld/", m_pDS->fv("genre.idGenre").get_asLong());
      pItem->m_strPath="videodb://2/1/"+ strDir;
      pItem->m_bIsFolder=true;
      items.Add(pItem);
      m_pDS->next();
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strSQL.c_str());
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

      CFileItemPtr pItem(new CFileItem(m_pDS->fv("actors.strActor").get_asString()));
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
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strSQL.c_str());
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

      CFileItemPtr pItem(new CFileItem(m_pDS->fv("actors.strActor").get_asString()));
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
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strSQL.c_str());
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
      strLike = "and actors.strActor like '%%%s%%'";
    if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL=FormatSQL("select actors.idactor,actors.strActor,path.strPath from artistlinkmusicvideo,actors,musicvideo,files,path where actors.idActor=artistlinkmusicvideo.idartist and artistlinkmusicvideo.idmvideo=musicvideo.idmvideo and files.idFile = musicvideo.idFile and files.idPath = path.idPath "+strLike,strSearch.c_str());
    else
      strSQL=FormatSQL("select distinct actors.idactor,actors.strActor from artistlinkmusicvideo,actors where actors.idActor=artistlinkmusicvideo.idartist "+strLike,strSearch.c_str());
    m_pDS->query( strSQL.c_str() );

    while (!m_pDS->eof())
    {
      if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv("path.strPath").get_asString()),g_settings.m_videoSources))
        {
          m_pDS->next();
          continue;
        }

      CFileItemPtr pItem(new CFileItem(m_pDS->fv("actors.strActor").get_asString()));
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
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strSQL.c_str());
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
      strSQL=FormatSQL("select distinct genre.idgenre,genre.strgenre from genre,genrelinkmusicvideo where genrelinkmusicvideo.idgenre=genre.idgenre and genre.strGenre like '%%%s%%'", strSearch.c_str());
    m_pDS->query( strSQL.c_str() );

    while (!m_pDS->eof())
    {
      if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv("path.strPath").get_asString()),g_settings.m_videoSources))
        {
          m_pDS->next();
          continue;
        }

      CFileItemPtr pItem(new CFileItem(m_pDS->fv("genre.strGenre").get_asString()));
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
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strSQL.c_str());
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
    {
      strLike.Format("and musicvideo.c%02d",VIDEODB_ID_MUSICVIDEO_ALBUM);
      strLike += "like '%%s%%%'";
    }
    if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL=FormatSQL("select distinct musicvideo.c%02d,musicvideo.idmvideo,path.strPath from musicvideo,files,path where files.idFile = musicvideo.idFile and files.idPath = path.idPath"+strLike,VIDEODB_ID_MUSICVIDEO_ALBUM,strSearch.c_str());
    else
    {
      if (!strLike.IsEmpty())
        strLike = "where "+strLike.Mid(4);
      strSQL=FormatSQL("select distinct musicvideo.c%02d,musicvideo.idmvideo from musicvideo"+strLike,VIDEODB_ID_MUSICVIDEO_ALBUM,strSearch.c_str());
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

      CFileItemPtr pItem(new CFileItem(m_pDS->fv(0).get_asString()));
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
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strSQL.c_str());
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

      CFileItemPtr pItem(new CFileItem(m_pDS->fv(1).get_asString()+" - "+m_pDS->fv(2).get_asString()));
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
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strSQL.c_str());
  }
}

bool CVideoDatabase::GetMusicVideosByWhere(const CStdString &baseDir, const CStdString &whereClause, CFileItemList &items, bool checkLocks /*= true*/)
{
  try
  {
    DWORD time = timeGetTime();
    movieTime = 0;
    castTime = 0;

    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    // We don't use FormatSQL here, as the WHERE clause is already formatted.
    CStdString strSQL = "select * from musicvideoview " + whereClause;
    CLog::Log(LOGDEBUG, "%s query = %s", __FUNCTION__, strSQL.c_str());

    // run query
    if (!m_pDS->query(strSQL.c_str()))
      return false;
    CLog::Log(LOGDEBUG, "%s time for actual SQL query = %d", __FUNCTION__, timeGetTime() - time); time = timeGetTime();

    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return false;
    }

    // get data from returned rows
    items.Reserve(iRowsFound);
    // get songs from returned subtable
    while (!m_pDS->eof())
    {
      long lMVideoId = m_pDS->fv("idmvideo").get_asLong();
      CVideoInfoTag musicvideo = GetDetailsForMusicVideo(m_pDS);
      if (!checkLocks || g_settings.m_vecProfiles[0].getLockMode() == LOCK_MODE_EVERYONE || g_passwordManager.bMasterUser ||
          g_passwordManager.IsDatabasePathUnlocked(musicvideo.m_strPath,g_settings.m_videoSources))
      {
        CFileItemPtr item(new CFileItem(musicvideo));
        item->m_strPath.Format("%s%ld",baseDir,lMVideoId);
        item->SetOverlayImage(CGUIListItem::ICON_OVERLAY_UNWATCHED,musicvideo.m_playCount > 0);
        if (CFile::Exists(item->GetCachedFanart()))
          item->SetProperty("fanart_image",item->GetCachedFanart());
        items.Add(item);
      }
      m_pDS->next();
    }

    CLog::Log(LOGDEBUG, "%s time to retrieve from dataset = %d", __FUNCTION__, timeGetTime() - time); time = timeGetTime();

    // cleanup
    m_pDS->close();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, whereClause.c_str());
  }
  return false;
}

unsigned int CVideoDatabase::GetMusicVideoIDs(const CStdString& strWhere, vector<pair<int,long> > &songIDs)
{
  try
  {
    if (NULL == m_pDB.get()) return 0;
    if (NULL == m_pDS.get()) return 0;

    CStdString strSQL = "select distinct idmvideo from musicvideoview " + strWhere;
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
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strWhere.c_str());
  }
  return 0;
}

bool CVideoDatabase::GetRandomMusicVideo(CFileItem* item, long& lSongId, const CStdString& strWhere)
{
  try
  {
    lSongId = -1;

    int iCount = GetMusicVideoCount(strWhere);
    if (iCount <= 0)
      return false;
    int iRandom = rand() % iCount;

    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    // We don't use FormatSQL here, as the WHERE clause is already formatted.
    CStdString strSQL;
    strSQL.Format("select * from musicvideoview %s order by idMVideo limit 1 offset %i",strWhere.c_str(),iRandom);
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
    *item->GetVideoInfoTag() = GetDetailsForMusicVideo(m_pDS);
    item->m_strPath.Format("videodb://3/2/%ld",item->GetVideoInfoTag()->m_iDbId);
    lSongId = m_pDS->fv("idmvideo").get_asLong();
    item->SetLabel(item->GetVideoInfoTag()->m_strTitle);
    m_pDS->close();
    return true;
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strWhere.c_str());
  }
  return false;
}

long CVideoDatabase::GetMatchingMusicVideo(const CStdString& strArtist, const CStdString& strAlbum, const CStdString& strTitle)
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    CStdString strSQL;
    if (strAlbum.IsEmpty() && strTitle.IsEmpty())
    { // we want to return matching artists only
      if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        strSQL=FormatSQL("select distinct actors.idactor,path.strPath from artistlinkmusicvideo,actors,musicvideo,files,path where actors.idActor=artistlinkmusicvideo.idartist and artistlinkmusicvideo.idmvideo=musicvideo.idmvideo and files.idFile = musicvideo.idFile and files.idPath = path.idPath and actors.strActor like '%s'",strArtist.c_str());
      else
        strSQL=FormatSQL("select distinct actors.idactor from artistlinkmusicvideo,actors where actors.idActor=artistlinkmusicvideo.idartist and actors.strActor like '%s'",strArtist.c_str());
    }
    else
    { // we want to return the matching musicvideo
      if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        strSQL = FormatSQL("select musicvideo.idmvideo from musicvideo,file,path,artistlinkmusicvideo,actors where file.idfile=musicvideo.idfile and file.idPath=path.idPath and musicvideo.%c02d like '%s' and musicvideo.%c02d like '%s' and artistlinkmusicvideo.idmvideo=musicvideo.idmvideo and artistlinkmusicvideo.idartist = actors.idactors and actors.strActor like '%s'",VIDEODB_ID_MUSICVIDEO_ALBUM,strAlbum.c_str(),VIDEODB_ID_MUSICVIDEO_TITLE,strTitle.c_str(),strArtist.c_str());
      else
        strSQL = FormatSQL("select musicvideo.idmvideo from musicvideo join artistlinkmusicvideo on artistlinkmusicvideo.idmvideo=musicvideo.idmvideo join actors on actors.idactor=artistlinkmusicvideo.idartist where musicvideo.c%02d like '%s' and musicvideo.c%02d like '%s' and actors.strActor like '%s'",VIDEODB_ID_MUSICVIDEO_ALBUM,strAlbum.c_str(),VIDEODB_ID_MUSICVIDEO_TITLE,strTitle.c_str(),strArtist.c_str());
    }
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
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
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

      CFileItemPtr pItem(new CFileItem(m_pDS->fv(1).get_asString()));
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
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strSQL.c_str());
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

      CFileItemPtr pItem(new CFileItem(m_pDS->fv(1).get_asString()));
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
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strSQL.c_str());
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

      CFileItemPtr pItem(new CFileItem(m_pDS->fv(1).get_asString()+" ("+m_pDS->fv(4).get_asString()+")"));
      pItem->m_strPath.Format("videodb://2/2/%ld/%ld/%ld",m_pDS->fv("tvshowlinkepisode.idshow").get_asLong(),m_pDS->fv(2).get_asLong(),m_pDS->fv(0).get_asLong());
      pItem->m_bIsFolder=false;
      items.Add(pItem);
      m_pDS->next();
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strSQL.c_str());
  }
}

void CVideoDatabase::GetMusicVideosByName(const CStdString& strSearch, CFileItemList& items)
{
// Alternative searching - not quite as fast though due to
// retrieving all information
//  CStdString where = FormatSQL("where c%02d like '%s%%' or c%02d like '%% %s%%'", VIDEODB_ID_MUSICVIDEO_TITLE, strSearch.c_str(), VIDEODB_ID_MUSICVIDEO_TITLE, strSearch.c_str());
//  GetMusicVideosByWhere("videodb://3/2/", where, items);
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

      CFileItemPtr pItem(new CFileItem(m_pDS->fv(1).get_asString()));
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
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strSQL.c_str());
  }
}

void CVideoDatabase::GetEpisodesByPlot(const CStdString& strSearch, CFileItemList& items)
{
// Alternative searching - not quite as fast though due to
// retrieving all information
//  CStdString where = FormatSQL("where c%02d like '%s%%' or c%02d like '%% %s%%'", VIDEODB_ID_EPISODE_PLOT, strSearch.c_str(), VIDEODB_ID_EPISODE_PLOT, strSearch.c_str());
//  where += FormatSQL("or c%02d like '%s%%' or c%02d like '%% %s%%'", VIDEODB_ID_EPISODE_TITLE, strSearch.c_str(), VIDEODB_ID_EPISODE_TITLE, strSearch.c_str());
//  GetEpisodesByWhere("videodb://2/2/", where, items);
//  return;
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

      CFileItemPtr pItem(new CFileItem(m_pDS->fv(1).get_asString()+" ("+m_pDS->fv(4).get_asString()+")"));
      pItem->m_strPath.Format("videodb://2/2/%ld/%ld/%ld",m_pDS->fv("tvshowlinkepisode.idshow").get_asLong(),m_pDS->fv(2).get_asLong(),m_pDS->fv(0).get_asLong());
      pItem->m_bIsFolder=false;
      items.Add(pItem);
      m_pDS->next();
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strSQL.c_str());
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
      CFileItemPtr pItem(new CFileItem(m_pDS->fv("actors.strActor").get_asString()));

      pItem->m_strPath="videodb://1/5/"+ strDir;
      pItem->m_bIsFolder=true;
      items.Add(pItem);
      m_pDS->next();
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strSQL.c_str());
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
      CFileItemPtr pItem(new CFileItem(m_pDS->fv("actors.strActor").get_asString()));

      pItem->m_strPath="videodb://2/5/"+ strDir;
      pItem->m_bIsFolder=true;
      items.Add(pItem);
      m_pDS->next();
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strSQL.c_str());
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
      CFileItemPtr pItem(new CFileItem(m_pDS->fv("actors.strActor").get_asString()));

      pItem->m_strPath="videodb://3/5/"+ strDir;
      pItem->m_bIsFolder=true;
      items.Add(pItem);
      m_pDS->next();
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strSQL.c_str());
  }
}

void CVideoDatabase::CleanDatabase(IVideoInfoScannerObserver* pObserver, const vector<long>* paths)
{
  CGUIDialogProgress *progress=NULL;
  try
  {
    BeginTransaction();
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    // find all the files
    CStdString sql;
    if (paths)
    {
      if (paths->size() == 0)
      {
        RollbackTransaction();
        return;
      }

      CStdString strPaths;
      for (unsigned int i=0;i<paths->size();++i )
        strPaths.Format("%s,%u",strPaths.Mid(0).c_str(),paths->at(i));
      sql = FormatSQL("select * from files,path where files.idpath=path.idPath and path.idPath in (%s)",strPaths.Mid(1).c_str());
    }
    else
      sql = "select * from files, path where files.idPath = path.idPath";

    m_pDS->query(sql.c_str());
    if (m_pDS->num_rows() == 0) return;

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
      pObserver->OnSetTitle("");
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
      ConstructPath(fullPath,path,fileName);

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

    CLog::Log(LOGDEBUG, "%s Cleaning files table", __FUNCTION__);
    sql = "delete from files where idFile in " + filesToDelete;
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, "%s Cleaning bookmark table", __FUNCTION__);
    sql = "delete from bookmark where idFile in " + filesToDelete;
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, "%s Cleaning settings table", __FUNCTION__);
    sql = "delete from settings where idFile in " + filesToDelete;
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, "%s Cleaning stacktimes table", __FUNCTION__);
    sql = "delete from stacktimes where idFile in " + filesToDelete;
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, "%s Cleaning movie table", __FUNCTION__);
    sql = "delete from movie where idMovie in " + moviesToDelete;
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, "%s Cleaning actorlinkmovie table", __FUNCTION__);
    sql = "delete from actorlinkmovie where idMovie in " + moviesToDelete;
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, "%s Cleaning directorlinkmovie table", __FUNCTION__);
    sql = "delete from directorlinkmovie where idMovie in " + moviesToDelete;
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, "%s Cleaning writerlinkmovie table", __FUNCTION__);
    sql = "delete from writerlinkmovie where idMovie in " + moviesToDelete;
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, "%s Cleaning genrelinkmovie table", __FUNCTION__);
    sql = "delete from genrelinkmovie where idMovie in " + moviesToDelete;
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, "%s Cleaning studiolinkmovie table", __FUNCTION__);
    sql = "delete from studiolinkmovie where idMovie in " + moviesToDelete;
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, "%s Cleaning episode table", __FUNCTION__);
    sql = "delete from episode where idEpisode in " + episodesToDelete;
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, "%s Cleaning actorlinkepisode table", __FUNCTION__);
    sql = "delete from actorlinkepisode where idEpisode in " + episodesToDelete;
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, "%s Cleaning directorlinkepisode table", __FUNCTION__);
    sql = "delete from directorlinkepisode where idEpisode in " + episodesToDelete;
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, "%s Cleaning writerlinkepisode table", __FUNCTION__);
    sql = "delete from writerlinkepisode where idEpisode in " + episodesToDelete;
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, "%s Cleaning tvshowlinkepisode table", __FUNCTION__);
    sql = "delete from tvshowlinkepisode where idEpisode in " + episodesToDelete;
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, "%s Cleaning tvshow table", __FUNCTION__);
    sql = "delete from tvshow where idshow not in (select distinct idShow from tvshowlinkepisode)";
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, "%s Cleaning actorlinktvshow table", __FUNCTION__);
    sql = "delete from actorlinktvshow where idShow not in (select distinct idShow from tvshowlinkepisode)";
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, "%s Cleaning directorlinktvshow table", __FUNCTION__);
    sql = "delete from directorlinktvshow where idShow not in (select distinct idShow from tvshowlinkepisode)";
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, "%s Cleaning tvshowlinkpath table", __FUNCTION__);
    sql = "delete from tvshowlinkpath where idshow not in (select distinct idShow from tvshowlinkepisode)";
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, "%s Cleaning genrelinktvshow table", __FUNCTION__);
    sql = "delete from genrelinktvshow where idShow not in (select distinct idShow from tvshowlinkepisode)";
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, "%s Cleaning movielinktvshow table", __FUNCTION__);
    sql = "delete from movielinktvshow where idShow not in (select distinct idShow from tvshowlinkepisode)";
    m_pDS->exec(sql.c_str());
    sql = "delete from movielinktvshow where idMovie not in (select distinct idMovie from movie)";
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, "%s Cleaning musicvideo table", __FUNCTION__);
    sql = "delete from musicvideo where idmvideo in " + musicVideosToDelete;
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, "%s Cleaning artistlinkmusicvideo table", __FUNCTION__);
    sql = "delete from artistlinkmusicvideo where idMVideo in " + musicVideosToDelete;
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, "%s Cleaning directorlinkmusicvideo table" ,__FUNCTION__);
    sql = "delete from directorlinkmusicvideo where idMVideo in " + musicVideosToDelete;
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, "%s Cleaning genrelinkmusicvideo table" ,__FUNCTION__);
    sql = "delete from genrelinkmusicvideo where idMVideo in " + musicVideosToDelete;
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, "%s Cleaning studiolinkmusicvideo table", __FUNCTION__);
    sql = "delete from studiolinkmusicvideo where idMVideo in " + musicVideosToDelete;
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, "%s Cleaning path table", __FUNCTION__);
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

    CLog::Log(LOGDEBUG, "%s Cleaning genre table", __FUNCTION__);
    sql = "delete from genre where idGenre not in (select distinct idGenre from genrelinkmovie) and idGenre not in (select distinct idGenre from genrelinktvshow) and idGenre not in (select distinct idGenre from genrelinkmusicvideo)";
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, "%s Cleaning actor table of actors, directors and writers", __FUNCTION__);
    sql = "delete from actors where idActor not in (select distinct idActor from actorlinkmovie) and idActor not in (select distinct idDirector from directorlinkmovie) and idActor not in (select distinct idWriter from writerlinkmovie) and idActor not in (select distinct idActor from actorlinktvshow) and idActor not in (select distinct idActor from actorlinkepisode) and idActor not in (select distinct idDirector from directorlinktvshow) and idActor not in (select distinct idDirector from directorlinkepisode) and idActor not in (select distinct idWriter from writerlinkepisode) and idActor not in (select distinct idArtist from artistlinkmusicvideo) and idActor not in (select distinct idDirector from directorlinkmusicvideo)";
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, "%s Cleaning studio table", __FUNCTION__);
    sql = "delete from studio where idStudio not in (select distinct idStudio from studiolinkmovie) and idStudio not in (select distinct idStudio from studiolinkmusicvideo)";
    m_pDS->exec(sql.c_str());

    CommitTransaction();

    if (pObserver)
      pObserver->OnStateChanged(COMPRESSING_DATABASE);

    Compress(false);

    CUtil::DeleteVideoDatabaseDirectoryCache();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  if (progress)
    progress->Close();
}

void CVideoDatabase::DumpToDummyFiles(const CStdString &path)
{
  // get all tvshows
  CFileItemList items;
  GetTvShowsByWhere("videodb://2/2/", "", items);
  for (int i = 0; i < items.Size(); i++)
  {
    // create a folder in this directory
    CStdString showName = CUtil::MakeLegalFileName(items[i]->GetVideoInfoTag()->m_strShowTitle);
    CStdString TVFolder;
    CUtil::AddFileToFolder(path, showName, TVFolder);
    if (CDirectory::Create(TVFolder))
    { // right - grab the episodes and dump them as well
      CFileItemList episodes;
      CStdString where = FormatSQL("where idshow=%u", items[i]->GetVideoInfoTag()->m_iDbId);
      GetEpisodesByWhere("videodb://2/2/", where, episodes);
      for (int i = 0; i < episodes.Size(); i++)
      {
        CVideoInfoTag *tag = episodes[i]->GetVideoInfoTag();
        CStdString episode;
        episode.Format("%s.s%02de%02d.avi", showName.c_str(), tag->m_iSeason, tag->m_iEpisode);
        // and make a file
        CStdString episodePath;
        CUtil::AddFileToFolder(TVFolder, episode, episodePath);
        CFile file;
        if (file.OpenForWrite(episodePath))
          file.Close();
      }
    }
  }
}

void CVideoDatabase::ExportToXML(const CStdString &xmlFile, bool singleFiles /* = false */, bool images /* = false */, bool overwrite /*=false*/)
{
  if (CFile::Exists(xmlFile) && !overwrite && !singleFiles) return;

  CGUIDialogProgress *progress=NULL;
  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;
    if (NULL == m_pDS2.get()) return;

    // create a 3rd dataset as well as GetEpisodeDetails() etc. uses m_pDS2, and we need to do 3 nested queries on tv shows
    auto_ptr<Dataset> pDS;
    pDS.reset(m_pDB->CreateDataset());
    if (NULL == pDS.get()) return;

    auto_ptr<Dataset> pDS2;
    pDS2.reset(m_pDB->CreateDataset());
    if (NULL == pDS2.get()) return;

    progress = (CGUIDialogProgress *)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
    // find all movies
    CStdString sql = "select * from movieview";

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

      if (progress)
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

      if (singleFiles)
      {
        CStdString tempFile, nfoFile;
        CFileItem item(movie.m_strFileNameAndPath,false);
        CUtil::ReplaceExtension(item.GetTBNFile(), ".nfo", nfoFile);
        CUtil::AddFileToFolder(g_advancedSettings.m_cachePath, CUtil::GetFileName(nfoFile), tempFile);

        if (overwrite || !CFile::Exists(nfoFile))
        {
          if(xmlDoc.SaveFile(tempFile))
          {
            if (CFile::Cache(tempFile,nfoFile))
              CFile::Delete(tempFile);
            else
              CLog::Log(LOGERROR, "%s: Movie nfo export failed! ('%s')", __FUNCTION__, nfoFile.c_str());
          }
        }

        xmlDoc.Clear();
        TiXmlDeclaration decl("1.0", "UTF-8", "yes");
        xmlDoc.InsertEndChild(decl);

        if (images)
        {
          if (CFile::Exists(item.GetCachedVideoThumb()) && (overwrite || !CFile::Exists(item.GetTBNFile())))
            if (!CFile::Cache(item.GetCachedVideoThumb(),item.GetTBNFile()))
              CLog::Log(LOGERROR, "%s: Movie thumb export failed! ('%s' -> '%s')", __FUNCTION__, item.GetCachedVideoThumb().c_str(), item.GetTBNFile().c_str());

          CStdString strFanart;
          CUtil::ReplaceExtension(item.GetTBNFile(), "-fanart.jpg", strFanart);

          if (CFile::Exists(item.GetCachedFanart()) && (overwrite || !CFile::Exists(strFanart)))
            if (!CFile::Cache(item.GetCachedFanart(),strFanart))
              CLog::Log(LOGERROR, "%s: Movie fanart export failed! ('%s' -> '%s')", __FUNCTION__, item.GetCachedFanart().c_str(), strFanart.c_str());
        }
      }

      m_pDS->next();
      current++;
    }
    m_pDS->close();

    // find all musicvideos
    sql = "select * from musicvideoview";

    m_pDS->query(sql.c_str());

    total = m_pDS->num_rows();
    current = 0;

    while (!m_pDS->eof())
    {
      CVideoInfoTag movie = GetDetailsForMusicVideo(m_pDS);
      movie.Save(pMain, "musicvideo", !singleFiles);

      if (progress)
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

      if (singleFiles)
      {
        CStdString tempFile, nfoFile;
        CFileItem item(movie.m_strFileNameAndPath,false);
        CUtil::ReplaceExtension(item.GetTBNFile(), ".nfo", nfoFile);
        CUtil::AddFileToFolder(g_advancedSettings.m_cachePath, CUtil::GetFileName(nfoFile), tempFile);

        if (overwrite || !CFile::Exists(nfoFile))
        {
          if(xmlDoc.SaveFile(tempFile))
          {
            if (CFile::Cache(tempFile,nfoFile))
              CFile::Delete(tempFile);
            else
              CLog::Log(LOGERROR, "%s: Musicvideo nfo export failed! ('%s')", __FUNCTION__, nfoFile.c_str());
          }
        }

        xmlDoc.Clear();
        TiXmlDeclaration decl("1.0", "UTF-8", "yes");
        xmlDoc.InsertEndChild(decl);

        if (images)
        {
          if (CFile::Exists(item.GetCachedVideoThumb()) && (overwrite || !CFile::Exists(item.GetTBNFile())))
            if (!CFile::Cache(item.GetCachedVideoThumb(),item.GetTBNFile()))
              CLog::Log(LOGERROR, "%s: Musicvideo thumb export failed! ('%s' -> '%s')", __FUNCTION__, item.GetCachedVideoThumb().c_str(), item.GetTBNFile().c_str());

        }
      }
      m_pDS->next();
      current++;
    }
    m_pDS->close();

    // repeat for all tvshows
    sql = "select * from tvshowview";
    m_pDS->query(sql.c_str());

    total = m_pDS->num_rows();
    current = 0;

    while (!m_pDS->eof())
    {
      CVideoInfoTag tvshow = GetDetailsForTvShow(m_pDS, true);
      tvshow.Save(pMain, "tvshow", !singleFiles);

      if (progress)
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

      if (singleFiles)
      {
        CStdString tempFile, nfoFile;
        CFileItem item(tvshow.m_strPath,false);
        CUtil::AddFileToFolder(g_advancedSettings.m_cachePath, "tvshow.nfo", tempFile);
        CUtil::AddFileToFolder(tvshow.m_strPath, "tvshow.nfo", nfoFile);

        if (overwrite || !CFile::Exists(nfoFile))
        {
          if(xmlDoc.SaveFile(tempFile))
          {
            if (CFile::Cache(tempFile,nfoFile))
              CFile::Delete(tempFile);
            else
              CLog::Log(LOGERROR, "%s: TVshow nfo export failed! ('%s')", __FUNCTION__, nfoFile.c_str());
          }
        }

        xmlDoc.Clear();
        TiXmlDeclaration decl("1.0", "UTF-8", "yes");
        xmlDoc.InsertEndChild(decl);

        if (images)
        {
          if (CFile::Exists(item.GetCachedVideoThumb()) && (overwrite || !CFile::Exists(item.GetFolderThumb())))
            if (!CFile::Cache(item.GetCachedVideoThumb(),item.GetFolderThumb()))
              CLog::Log(LOGERROR, "%s: TVShow thumb export failed! ('%s' -> '%s')", __FUNCTION__, item.GetCachedVideoThumb().c_str(), item.GetFolderThumb().c_str());

          if (CFile::Exists(item.GetCachedFanart()) && (overwrite || !CFile::Exists(item.GetFolderThumb("fanart.jpg"))))
            if (!CFile::Cache(item.GetCachedFanart(),item.GetFolderThumb("fanart.jpg")))
              CLog::Log(LOGERROR, "%s: TVShow fanart export failed! ('%s' -> '%s')", __FUNCTION__, item.GetCachedFanart().c_str(), item.GetFolderThumb("fanart.jpg").c_str());


          // now get all available seasons from this show
          sql = FormatSQL("select distinct(c%02d) from episodeview where idShow=%i", VIDEODB_ID_EPISODE_SEASON, tvshow.m_iDbId);
          pDS2->query(sql.c_str());

          CFileItemList items;
          CStdString strDatabasePath;
          strDatabasePath.Format("videodb://2/2/%i/",tvshow.m_iDbId);

          // add "All Seasons" to list
          CFileItemPtr pItem;
          pItem.reset(new CFileItem(g_localizeStrings.Get(20366)));
          pItem->GetVideoInfoTag()->m_iSeason = -1;
          pItem->GetVideoInfoTag()->m_strPath = tvshow.m_strPath;
          items.Add(pItem);

          // loop through available season
          while (!pDS2->eof())
          {
            long lSeason = pDS2->fv(0).get_asLong();
            CStdString strLabel;
            if (lSeason == 0)
              strLabel = g_localizeStrings.Get(20381);
            else
              strLabel.Format(g_localizeStrings.Get(20358),lSeason);
            CFileItemPtr pItem(new CFileItem(strLabel));
            pItem->GetVideoInfoTag()->m_strTitle = strLabel;
            pItem->GetVideoInfoTag()->m_iSeason = lSeason;
            pItem->GetVideoInfoTag()->m_strPath = tvshow.m_strPath;
            items.Add(pItem);
            pDS2->next();
          }
          pDS2->close();

          // export season thumbs
          for (int i=0;i<items.Size();++i)
          {
            CStdString strSeasonThumb, strParent, strDest;
            int iSeason = items[i]->GetVideoInfoTag()->m_iSeason;
            if (iSeason == -1)
              strSeasonThumb = "season-all.tbn";
            else if (iSeason == 0)
              strSeasonThumb = "season-specials.tbn";
            else
              strSeasonThumb.Format("season0%i.tbn",iSeason);
            CUtil::GetParentPath(item.GetTBNFile(), strParent);
            CUtil::AddFileToFolder(strParent, strSeasonThumb, strDest);

            if (CFile::Exists(items[i]->GetCachedSeasonThumb()) && (overwrite || !CFile::Exists(strDest)))
              if (!CFile::Cache(items[i]->GetCachedSeasonThumb(),strDest))
                CLog::Log(LOGERROR, "%s: TVShow season thumb export failed! ('%s' -> '%s')", __FUNCTION__, items[i]->GetCachedSeasonThumb().c_str(), strDest.c_str());
          }

        }
      }

      // now save the episodes from this show
      sql = FormatSQL("select * from episodeview where idShow=%i",tvshow.m_iDbId);
      pDS->query(sql.c_str());

      while (!pDS->eof())
      {
        CVideoInfoTag episode = GetDetailsForEpisode(pDS, true);
        if (singleFiles)
          episode.Save(pMain, "episodedetails", !singleFiles);
        else
          episode.Save(pMain->LastChild(), "episodedetails", !singleFiles);

        if (singleFiles)
        {
          CStdString tempFile, nfoFile;
          CFileItem item(episode.m_strFileNameAndPath,false);
          CUtil::ReplaceExtension(item.GetTBNFile(), ".nfo", nfoFile);
          CUtil::AddFileToFolder(g_advancedSettings.m_cachePath, CUtil::GetFileName(nfoFile), tempFile);

          if (overwrite || !CFile::Exists(nfoFile))
          {
            if(xmlDoc.SaveFile(tempFile))
            {
              if (CFile::Cache(tempFile,nfoFile))
                CFile::Delete(tempFile);
              else
                CLog::Log(LOGERROR, "%s: Episode nfo export failed! ('%s')", __FUNCTION__, nfoFile.c_str());
            }
          }

          xmlDoc.Clear();
          TiXmlDeclaration decl("1.0", "UTF-8", "yes");
          xmlDoc.InsertEndChild(decl);

          if (images)
          {
            if (CFile::Exists(item.GetCachedVideoThumb()) && (overwrite || !CFile::Exists(item.GetTBNFile())))
              if (!CFile::Cache(item.GetCachedVideoThumb(),item.GetTBNFile()))
                CLog::Log(LOGERROR, "%s: Episode thumb export failed! ('%s' -> '%s')", __FUNCTION__, item.GetCachedVideoThumb().c_str(), item.GetTBNFile().c_str());
          }
        }
        pDS->next();
      }
      pDS->close();
      m_pDS->next();
      current++;
    }
    m_pDS->close();

    if (singleFiles && progress)
    {
      progress->SetPercentage(100);
      progress->Progress();
    }

    if (!singleFiles)
    {
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
      xmlDoc.SaveFile(xmlFile);
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }

  if (progress)
    progress->Close();
}

void CVideoDatabase::ImportFromXML(const CStdString &xmlFile)
{
  CGUIDialogProgress *progress=NULL;
  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    TiXmlDocument xmlDoc;
    if (!xmlDoc.LoadFile(xmlFile))
      return;

    TiXmlElement *root = xmlDoc.RootElement();
    if (!root) return;

    progress = (CGUIDialogProgress *)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
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
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
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
    strResult=strOpenRecordSet;
    while (!m_pDS->eof())
    {
      strResult += strOpenRecord;
      for (int i=0; i<m_pDS->fieldCount(); i++)
      {
        strResult += strOpenField + CStdString(m_pDS->fv(i).get_asString()) + strCloseField;
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
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
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

void CVideoDatabase::ConstructPath(CStdString& strDest, const CStdString& strPath, const CStdString& strFileName)
{
  if (CUtil::IsStack(strFileName) || strFileName.Mid(0,6).Equals("rar://") || strFileName.Mid(0,6).Equals("zip://"))
    strDest = strFileName;
  else
    CUtil::AddFileToFolder(strPath, strFileName, strDest);
}

void CVideoDatabase::SplitPath(const CStdString& strFileNameAndPath, CStdString& strPath, CStdString& strFileName)
{
  if (CUtil::IsStack(strFileNameAndPath) || strFileNameAndPath.Mid(0,6).Equals("rar://") || strFileNameAndPath.Mid(0,6).Equals("zip://"))
  {
    CUtil::GetParentPath(strFileNameAndPath,strPath);
    strFileName = strFileNameAndPath;
  }
  else
    CUtil::Split(strFileNameAndPath,strPath, strFileName);
}

void CVideoDatabase::InvalidatePathHash(const CStdString& strPath)
{
  SScraperInfo info;
  SScanSettings settings;
  int iFound;
  GetScraperForPath(strPath,info,settings,iFound);
  SetPathHash(strPath,"");
  if (info.strContent.Equals("tvshows") || (info.strContent.Equals("movies") && iFound != 1)) // if we scan by folder name we need to invalidate parent as well
  {
    if (info.strContent.Equals("tvshows") || settings.parent_name_root)
    {
      CStdString strParent;
      CUtil::GetParentPath(strPath,strParent);
      SetPathHash(strParent,"");
    }
  }
}

bool CVideoDatabase::CommitTransaction()
{
  if (CDatabase::CommitTransaction())
  { // number of items in the db has likely changed, so reset the infomanager cache
    g_infoManager.ResetPersistentCache();
    return true;
  }
  return false;
}

void CVideoDatabase::DeleteThumbForItem(const CStdString& strPath, bool bFolder)
{
  CFileItem item(strPath,bFolder);
  XFILE::CFile::Delete(item.GetCachedVideoThumb());
  if (item.HasVideoInfoTag())
    XFILE::CFile::Delete(item.GetCachedEpisodeThumb());
  XFILE::CFile::Delete(item.GetCachedFanart());

  // tell our GUI to completely reload all controls (as some of them
  // are likely to have had this image in use so will need refreshing)
  CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_REFRESH_THUMBS);
  m_gWindowManager.SendThreadMessage(msg);
}

