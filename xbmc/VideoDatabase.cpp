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

#include "VideoDatabase.h"
#include "GUIWindowVideoBase.h"
#include "utils/RegExp.h"
#include "addons/AddonManager.h"
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
#include "FileSystem/SpecialProtocol.h"
#include "GUIDialogProgress.h"
#include "GUIDialogYesNo.h"
#include "FileItem.h"
#include "AdvancedSettings.h"
#include "GUISettings.h"
#include "Settings.h"
#include "StringUtils.h"
#include "LocalizeStrings.h"
#include "utils/TimeUtils.h"
#include "utils/log.h"

using namespace std;
using namespace dbiplus;
using namespace XFILE;
using namespace VIDEO;
using namespace ADDON;

#define VIDEO_DATABASE_VIEW_TVSHOW "SELECT tvshow.*,path.strPath AS strPath," \
                                   "counts.totalcount AS totalCount,counts.watchedcount AS watchedCount," \
                                   "counts.totalcount=counts.watchedcount AS watched FROM tvshow " \
                                   "JOIN tvshowlinkpath ON tvshow.idShow=tvshowlinkpath.idShow " \
                                   "JOIN path ON path.idpath=tvshowlinkpath.idPath " \
                                   "LEFT OUTER join (" \
                                   "    SELECT tvshow.idShow AS idShow,count(1) AS totalcount,count(files.playCount) AS watchedcount FROM tvshow " \
                                   "    JOIN tvshowlinkepisode ON tvshow.idShow=tvshowlinkepisode.idShow " \
                                   "    JOIN episode ON episode.idEpisode=tvshowlinkepisode.idEpisode " \
                                   "    JOIN files ON files.idFile=episode.idFile " \
                                   "    GROUP BY tvshow.idShow" \
                                   ") counts ON tvshow.idShow=counts.idShow "

//********************************************************************************************************************************
CVideoDatabase::CVideoDatabase(void)
{
}

//********************************************************************************************************************************
CVideoDatabase::~CVideoDatabase(void)
{}

//********************************************************************************************************************************
bool CVideoDatabase::Open()
{
  return CDatabase::Open(g_advancedSettings.m_databaseVideo);
}

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

  BeginTransaction();
  try
  {
    CDatabase::CreateTables();

    CLog::Log(LOGINFO, "create bookmark table");
    m_pDS->exec("CREATE TABLE bookmark ( idBookmark integer primary key, idFile integer, timeInSeconds double, totalTimeInSeconds double, thumbNailImage text, player text, playerState text, type integer)\n");
    m_pDS->exec("CREATE INDEX ix_bookmark ON bookmark (idFile)");

    CLog::Log(LOGINFO, "create settings table");
    m_pDS->exec("CREATE TABLE settings ( idFile integer, Deinterlace bool,"
                "ViewMode integer,ZoomAmount float, PixelRatio float, AudioStream integer, SubtitleStream integer,"
                "SubtitleDelay float, SubtitlesOn bool, Brightness float, Contrast float, Gamma float,"
                "VolumeAmplification float, AudioDelay float, OutputToAllSpeakers bool, ResumeTime integer, Crop bool, CropLeft integer,"
                "CropRight integer, CropTop integer, CropBottom integer, Sharpness float, NoiseReduction float, NonLinStretch bool)\n");
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
    m_pDS->exec("CREATE TABLE path ( idPath integer primary key, strPath varchar(512), strContent text, strScraper text, strHash text, scanRecursive integer, useFolderNames bool, strSettings text, noUpdate bool)\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_path ON path ( strPath )\n");

    CLog::Log(LOGINFO, "create files table");
    m_pDS->exec("CREATE TABLE files ( idFile integer primary key, idPath integer, strFilename varchar(512), playCount integer, lastPlayed text)\n");
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

    CLog::Log(LOGINFO, "create studiolinktvshow table");
    m_pDS->exec("CREATE TABLE studiolinktvshow ( idStudio integer, idShow integer)\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_studiolinktvshow_1 ON studiolinktvshow ( idStudio, idShow)\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_studiolinktvshow_2 ON studiolinktvshow ( idShow, idStudio)\n");

    CLog::Log(LOGINFO, "create episode table");
    columns = "CREATE TABLE episode ( idEpisode integer primary key";
    for (int i = 0; i < VIDEODB_MAX_COLUMNS; i++)
    {
      CStdString column;
      if ( i == VIDEODB_ID_EPISODE_SEASON || i == VIDEODB_ID_EPISODE_EPISODE || i == VIDEODB_ID_EPISODE_BOOKMARK)
        column.Format(",c%02d varchar(24)", i);
      else
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

    CLog::Log(LOGINFO, "create streaminfo table");
    m_pDS->exec("CREATE TABLE streamdetails (idFile integer, iStreamType integer, "
      "strVideoCodec text, fVideoAspect real, iVideoWidth integer, iVideoHeight integer, "
      "strAudioCodec text, iAudioChannels integer, strAudioLanguage text, strSubtitleLanguage text)");
    m_pDS->exec("CREATE INDEX ix_streamdetails ON streamdetails (idFile)");

    CLog::Log(LOGINFO, "create episodeview");
    CStdString episodeview = FormatSQL("create view episodeview as select episode.*,files.strFileName as strFileName,"
                                       "path.strPath as strPath,files.playCount as playCount,files.lastPlayed as lastPlayed,tvshow.c%02d as strTitle,tvshow.c%02d as strStudio,tvshow.idShow as idShow,"
                                       "tvshow.c%02d as premiered, tvshow.c%02d as mpaa from episode "
                                       "join files on files.idFile=episode.idFile "
                                       "join tvshowlinkepisode on episode.idepisode=tvshowlinkepisode.idEpisode "
                                       "join tvshow on tvshow.idShow=tvshowlinkepisode.idShow "
                                       "join path on files.idPath=path.idPath",VIDEODB_ID_TV_TITLE, VIDEODB_ID_TV_STUDIOS, VIDEODB_ID_TV_PREMIERED, VIDEODB_ID_TV_MPAA);
    m_pDS->exec(episodeview.c_str());

    CLog::Log(LOGINFO, "create musicvideoview");
    m_pDS->exec("create view musicvideoview as select musicvideo.*,files.strFileName as strFileName,path.strPath as strPath,files.playCount as playCount,files.lastPlayed as lastPlayed "
                "from musicvideo join files on files.idFile=musicvideo.idFile join path on path.idPath=files.idPath");

    CLog::Log(LOGINFO, "create movieview");
    m_pDS->exec("create view movieview as select movie.*,files.strFileName as strFileName,path.strPath as strPath,files.playCount as playCount,files.lastPlayed as lastPlayed "
                "from movie join files on files.idFile=movie.idFile join path on path.idPath=files.idPath");

    CLog::Log(LOGINFO, "create sets table");
    m_pDS->exec("CREATE TABLE sets ( idSet integer primary key, strSet text)\n");

    CLog::Log(LOGINFO, "create setlinkmovie table");
    m_pDS->exec("CREATE TABLE setlinkmovie ( idSet integer, idMovie integer)\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_setlinkmovie_1 ON setlinkmovie ( idSet, idMovie)\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_setlinkmovie_2 ON setlinkmovie ( idMovie, idSet)\n");
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

//********************************************************************************************************************************
int CVideoDatabase::GetPathId(const CStdString& strPath)
{
  CStdString strSQL;
  try
  {
    int idPath=-1;
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    CStdString strPath1(strPath);
    if (CUtil::IsStack(strPath) || strPath.Mid(0,6).Equals("rar://") || strPath.Mid(0,6).Equals("zip://"))
      CUtil::GetParentPath(strPath,strPath1);

    CUtil::AddSlashAtEnd(strPath1);

    strSQL=FormatSQL("select idPath from path where strPath like '%s'",strPath1.c_str());
    m_pDS->query(strSQL.c_str());
    if (!m_pDS->eof())
      idPath = m_pDS->fv("path.idPath").get_asInt();

    m_pDS->close();
    return idPath;
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
        settings.recurse     = m_pDS->fv("scanRecursive").get_asInt();
        settings.parent_name_root = settings.parent_name && !settings.recurse;

        paths.insert(pair<CStdString,SScanSettings>(strPath,settings));
      }
      m_pDS->next();
    }
    m_pDS->close();

    // then grab all tvshow paths
    if (!m_pDS->query("select strPath,scanRecursive,useFolderNames,strContent,noUpdate from path"
                      " where ( strContent = 'tvshows'"
                      "       or idPath in (select idPath from tvshowlinkpath))"
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
          settings.recurse     = m_pDS->fv("scanRecursive").get_asInt();
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
                       " and idPath NOT in (select idPath from tvshowlinkpath)"
                       " and idPath NOT in (select idPath from files where strFileName like 'video_ts.ifo')" // dvdfolders get stacked to a single item in parent folder
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

bool CVideoDatabase::GetPathsForTvShow(int idShow, vector<int>& paths)
{
  CStdString strSQL;
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    strSQL = FormatSQL("SELECT DISTINCT idPath FROM files JOIN episode ON episode.idFile=files.idFile JOIN tvshowlinkepisode ON tvshowlinkepisode.idEpisode=episode.idEpisode WHERE tvshowlinkepisode.idShow=%i",idShow);
    m_pDS->query(strSQL.c_str());
    while (!m_pDS->eof())
    {
      paths.push_back(m_pDS->fv(0).get_asInt());
      m_pDS->next();
    }
    m_pDS->close();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s error during query: %s",__FUNCTION__, strSQL.c_str());
  }
  return false;
}

int CVideoDatabase::AddPath(const CStdString& strPath)
{
  CStdString strSQL;
  try
  {
    int idPath;
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    CStdString strPath1(strPath);
    if (CUtil::IsStack(strPath) || strPath.Mid(0,6).Equals("rar://") || strPath.Mid(0,6).Equals("zip://"))
      CUtil::GetParentPath(strPath,strPath1);

    CUtil::AddSlashAtEnd(strPath1);

    strSQL=FormatSQL("insert into path (idPath, strPath, strContent, strScraper) values (NULL,'%s','','')", strPath1.c_str());
    m_pDS->exec(strSQL.c_str());
    idPath = (int)m_pDS->lastinsertid();
    return idPath;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s unable to addpath (%s)", __FUNCTION__, strSQL.c_str());
  }
  return -1;
}

bool CVideoDatabase::GetPathHash(const CStdString &path, CStdString &hash)
{
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
int CVideoDatabase::AddFile(const CStdString& strFileNameAndPath)
{
  CStdString strSQL = "";
  try
  {
    int idFile;
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    CStdString strFileName, strPath;
    SplitPath(strFileNameAndPath,strPath,strFileName);

    int idPath=GetPathId(strPath);
    if (idPath < 0)
      idPath = AddPath(strPath);

    if (idPath < 0)
      return -1;

    CStdString strSQL=FormatSQL("select idFile from files where strFileName like '%s' and idPath=%i", strFileName.c_str(),idPath);

    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() > 0)
    {
      idFile = m_pDS->fv("idFile").get_asInt() ;
      m_pDS->close();
      return idFile;
    }
    m_pDS->close();
    strSQL=FormatSQL("insert into files (idFile,idPath,strFileName) values(NULL, %i, '%s')", idPath,strFileName.c_str());
    m_pDS->exec(strSQL.c_str());
    idFile = (int)m_pDS->lastinsertid();
    return idFile;
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
    int idPath = GetPathId(path);
    if (idPath < 0)
      idPath = AddPath(path);
    if (idPath < 0) return false;

    CStdString strSQL=FormatSQL("update path set strHash='%s' where idPath=%ld", hash.c_str(), idPath);
    m_pDS->exec(strSQL.c_str());

    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s, %s) failed", __FUNCTION__, path.c_str(), hash.c_str());
  }

  return false;
}

bool CVideoDatabase::LinkMovieToTvshow(int idMovie, int idShow, bool bRemove)
{
   try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    if (bRemove) // delete link
    {
      CStdString strSQL=FormatSQL("delete from movielinktvshow where idMovie=%i and idShow=%i", idMovie, idShow);
      m_pDS->exec(strSQL.c_str());
      return true;
    }

    CStdString strSQL=FormatSQL("insert into movielinktvshow (idShow,idMovie) values (%i,%i)", idShow,idMovie);
    m_pDS->exec(strSQL.c_str());

    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%i, %i) failed", __FUNCTION__, idMovie, idShow);
  }

  return false;
}

bool CVideoDatabase::IsLinkedToTvshow(int idMovie)
{
   try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL=FormatSQL("select * from movielinktvshow where idMovie=%i", idMovie);
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
    CLog::Log(LOGERROR, "%s (%i) failed", __FUNCTION__, idMovie);
  }

  return false;
}

bool CVideoDatabase::GetLinksToTvShow(int idMovie, vector<int>& ids)
{
   try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL=FormatSQL("select * from movielinktvshow where idMovie=%i", idMovie);
    m_pDS2->query(strSQL.c_str());
    while (!m_pDS2->eof())
    {
      ids.push_back(m_pDS2->fv(1).get_asInt());
      m_pDS2->next();
    }

    m_pDS2->close();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%i) failed", __FUNCTION__, idMovie);
  }

  return false;
}


//********************************************************************************************************************************
int CVideoDatabase::GetFileId(const CStdString& strFilenameAndPath)
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;
    CStdString strPath, strFileName;
    SplitPath(strFilenameAndPath,strPath,strFileName);

    int idPath = GetPathId(strPath);
    if (idPath < 0)
      return -1;

    CStdString strSQL;
    strSQL=FormatSQL("select idFile from files where strFileName like '%s' and idPath=%i", strFileName.c_str(),idPath);
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() > 0)
    {
      int idFile = m_pDS->fv("files.idFile").get_asInt();
      m_pDS->close();
      return idFile;
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strFilenameAndPath.c_str());
  }
  return -1;
}

int CVideoDatabase::GetFileId(const CFileItem &item)
{
  CStdString path = item.m_strPath;
  if (item.IsVideoDb() && item.HasVideoInfoTag())
    path = item.GetVideoInfoTag()->m_strFileNameAndPath;
  return GetFileId(path);
}

//********************************************************************************************************************************
int CVideoDatabase::GetMovieId(const CStdString& strFilenameAndPath)
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;
    int idMovie = -1;

    // needed for query parameters
    int idFile = GetFileId(strFilenameAndPath);
    int idPath=-1;
    CStdString strPath;
    if (idFile < 0)
    {
      CStdString strFile;
      SplitPath(strFilenameAndPath,strPath,strFile);

      // have to join movieinfo table for correct results
      idPath = GetPathId(strPath);
      if (idPath < 0 && strPath != strFilenameAndPath)
        return -1;
    }

    if (idFile == -1 && strPath != strFilenameAndPath)
      return -1;

    CStdString strSQL;
    if (idFile == -1)
      strSQL=FormatSQL("select idMovie from movie join files on files.idFile=movie.idFile where files.idPath=%i",idPath);
    else
      strSQL=FormatSQL("select idMovie from movie where idFile=%i", idFile);

    CLog::Log(LOGDEBUG, "%s (%s), query = %s", __FUNCTION__, strFilenameAndPath.c_str(), strSQL.c_str());
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() > 0)
      idMovie = m_pDS->fv("idMovie").get_asInt();
    m_pDS->close();

    return idMovie;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strFilenameAndPath.c_str());
  }
  return -1;
}

int CVideoDatabase::GetTvShowId(const CStdString& strPath)
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;
    int idTvShow = -1;

    // have to join movieinfo table for correct results
    int idPath = GetPathId(strPath);
    if (idPath < 0)
      return -1;

    CStdString strSQL;
    CStdString strPath1=strPath;
    CStdString strParent;
    int iFound=0;

    strSQL=FormatSQL("select idShow from tvshowlinkpath where tvshowlinkpath.idPath=%i",idPath);
    m_pDS->query(strSQL);
    if (!m_pDS->eof())
      iFound = 1;

    while (iFound == 0 && CUtil::GetParentPath(strPath1, strParent))
    {
      strSQL=FormatSQL("select idShow from path,tvshowlinkpath where tvshowlinkpath.idPath=path.idPath and strPath like '%s'",strParent.c_str());
      m_pDS->query(strSQL.c_str());
      if (!m_pDS->eof())
      {
        int idShow = m_pDS->fv("idShow").get_asInt();
        if (idShow != -1)
          iFound = 2;
      }
      strPath1 = strParent;
    }

    if (m_pDS->num_rows() > 0)
      idTvShow = m_pDS->fv("idShow").get_asInt();
    m_pDS->close();

    return idTvShow;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strPath.c_str());
  }
  return -1;
}

int CVideoDatabase::GetEpisodeId(const CStdString& strFilenameAndPath, int idEpisode, int idSeason) // input value is episode/season number hint - for multiparters
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    // need this due to the nested GetEpisodeInfo query
    auto_ptr<Dataset> pDS;
    pDS.reset(m_pDB->CreateDataset());
    if (NULL == pDS.get()) return -1;

    int idFile = GetFileId(strFilenameAndPath);
    if (idFile < 0)
      return -1;

    CStdString strSQL=FormatSQL("select idEpisode from episode where idFile=%i", idFile);

    CLog::Log(LOGDEBUG, "%s (%s), query = %s", __FUNCTION__, strFilenameAndPath.c_str(), strSQL.c_str());
    pDS->query(strSQL.c_str());
    if (pDS->num_rows() > 0)
    {
      if (idEpisode == -1)
        idEpisode = pDS->fv("episode.idEpisode").get_asInt();
      else // use the hint!
      {
        while (!pDS->eof())
        {
          CVideoInfoTag tag;
          int idTmpEpisode = pDS->fv("episode.idEpisode").get_asInt();
          GetEpisodeInfo(strFilenameAndPath,tag,idTmpEpisode);
          if (tag.m_iEpisode == idEpisode && (idSeason == -1 || tag.m_iSeason == idSeason)) {
            // match on the episode hint, and there's no season hint or a season hint match
            idEpisode = idTmpEpisode;
            break;
          }
          pDS->next();
        }
        if (pDS->eof())
          idEpisode = -1;
      }
    }
    else
      idEpisode = -1;

    pDS->close();

    return idEpisode;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strFilenameAndPath.c_str());
  }
  return -1;
}

int CVideoDatabase::GetMusicVideoId(const CStdString& strFilenameAndPath)
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    int idFile = GetFileId(strFilenameAndPath);
    if (idFile < 0)
      return -1;

    CStdString strSQL=FormatSQL("select idMVideo from musicvideo where idFile=%i", idFile);

    CLog::Log(LOGDEBUG, "%s (%s), query = %s", __FUNCTION__, strFilenameAndPath.c_str(), strSQL.c_str());
    m_pDS->query(strSQL.c_str());
    int idMVideo=-1;
    if (m_pDS->num_rows() > 0)
      idMVideo = m_pDS->fv("idMVideo").get_asInt();
    m_pDS->close();

    return idMVideo;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strFilenameAndPath.c_str());
  }
  return -1;
}

//********************************************************************************************************************************
int CVideoDatabase::AddMovie(const CStdString& strFilenameAndPath)
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    int idFile, idMovie=-1;
    idFile = GetFileId(strFilenameAndPath);
    if (idFile < 0)
      idFile = AddFile(strFilenameAndPath);
    else
      idMovie = GetMovieId(strFilenameAndPath);
    if (idMovie < 0)
    {
      CStdString strSQL=FormatSQL("insert into movie (idMovie, idFile) values (NULL, %i)", idFile);
      m_pDS->exec(strSQL.c_str());
      idMovie = (int)m_pDS->lastinsertid();
//      CommitTransaction();
    }

    return idMovie;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strFilenameAndPath.c_str());
  }
  return -1;
}

int CVideoDatabase::AddTvShow(const CStdString& strPath)
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    CStdString strSQL=FormatSQL("select tvshowlinkpath.idShow from path,tvshowlinkpath where path.strPath like '%s' and path.idPath=tvshowlinkpath.idPath",strPath.c_str());
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() != 0)
      return m_pDS->fv("tvshowlinkpath.idShow").get_asInt();

    strSQL=FormatSQL("insert into tvshow (idShow) values (NULL)");
    m_pDS->exec(strSQL.c_str());
    int idTvShow = (int)m_pDS->lastinsertid();

    int idPath = GetPathId(strPath);
    if (idPath < 0)
      idPath = AddPath(strPath);
    strSQL=FormatSQL("insert into tvshowlinkpath values (%i,%i)",idTvShow,idPath);
    m_pDS->exec(strSQL.c_str());

//    CommitTransaction();

    return idTvShow;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strPath.c_str());
  }
  return -1;
}

//********************************************************************************************************************************
int CVideoDatabase::AddEpisode(int idShow, const CStdString& strFilenameAndPath)
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    int idFile, idEpisode=-1;
    idFile = GetFileId(strFilenameAndPath);
    if (idFile < 0)
      idFile = AddFile(strFilenameAndPath);

    CStdString strSQL=FormatSQL("insert into episode (idEpisode, idFile) values (NULL, %i)", idFile);
    m_pDS->exec(strSQL.c_str());
    idEpisode = (int)m_pDS->lastinsertid();

    strSQL=FormatSQL("insert into tvshowlinkepisode (idShow,idEpisode) values (%i,%i)",idShow,idEpisode);
    m_pDS->exec(strSQL.c_str());
//    CommitTransaction();

    return idEpisode;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strFilenameAndPath.c_str());
  }
  return -1;
}

int CVideoDatabase::AddMusicVideo(const CStdString& strFilenameAndPath)
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    int idFile, idMVideo=-11;
    idFile = GetFileId(strFilenameAndPath);
    if (idFile < 0)
      idFile = AddFile(strFilenameAndPath);
    else
      idMVideo = GetMusicVideoId(strFilenameAndPath);
    if (idMVideo < 0)
    {
      CStdString strSQL=FormatSQL("insert into musicvideo (idMVideo, idFile) values (NULL, %i)", idFile);
      m_pDS->exec(strSQL.c_str());
      idMVideo = (int)m_pDS->lastinsertid();
    }

    return idMVideo;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strFilenameAndPath.c_str());
  }
  return -1;
}

//********************************************************************************************************************************
int CVideoDatabase::AddGenre(const CStdString& strGenre)
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
      int idGenre = (int)m_pDS->lastinsertid();
      return idGenre;
    }
    else
    {
      const field_value value = m_pDS->fv("idGenre");
      int idGenre = value.get_asInt() ;
      m_pDS->close();
      return idGenre;
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strGenre.c_str() );
  }

  return -1;
}

//********************************************************************************************************************************
int CVideoDatabase::AddActor(const CStdString& strActor, const CStdString& strThumb)
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;
    CStdString strSQL=FormatSQL("select idActor from actors where strActor like '%s'", strActor.c_str());
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      // doesnt exists, add it
      strSQL=FormatSQL("insert into actors (idActor, strActor, strThumb) values( NULL, '%s','%s')", strActor.c_str(),strThumb.c_str());
      m_pDS->exec(strSQL.c_str());
      int idActor = (int)m_pDS->lastinsertid();
      return idActor;
    }
    else
    {
      const field_value value = m_pDS->fv("idActor");
      int idActor = value.get_asInt() ;
      // update the thumb url's
      if (!strThumb.IsEmpty())
        strSQL=FormatSQL("update actors set strThumb='%s' where idActor=%i",strThumb.c_str(),idActor);
      m_pDS->close();
      return idActor;
    }

  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strActor.c_str() );
  }
  return -1;
}

int CVideoDatabase::AddSet(const CStdString& strSet)
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;
    CStdString strSQL=FormatSQL("select idSet from sets where strSet like '%s'", strSet.c_str());
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      // doesn't exist, add it
      strSQL=FormatSQL("insert into sets (idSet, strSet) values(NULL, '%s')", strSet.c_str());
      m_pDS->exec(strSQL.c_str());
      int idSet = (int)m_pDS->lastinsertid();
      return idSet;
    }
    else
    {
      const field_value value = m_pDS->fv("idSet");
      int idSet = value.get_asInt() ;
      m_pDS->close();
      return idSet;
    }

  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strSet.c_str() );
  }
  return -1;
}

int CVideoDatabase::AddStudio(const CStdString& strStudio)
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
      int idStudio = (int)m_pDS->lastinsertid();
      return idStudio;
    }
    else
    {
      const field_value value = m_pDS->fv("idStudio");
      int idStudio = value.get_asInt() ;
      m_pDS->close();
      return idStudio;
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strStudio.c_str() );
  }

  return -1;
}

void CVideoDatabase::AddLinkToActor(const char *table, int actorID, const char *secondField, int secondID, const CStdString &role)
{
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    CStdString strSQL=FormatSQL("select * from %s where idActor=%i and %s=%i", table, actorID, secondField, secondID);
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

void CVideoDatabase::AddToLinkTable(const char *table, const char *firstField, int firstID, const char *secondField, int secondID)
{
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    CStdString strSQL=FormatSQL("select * from %s where %s=%i and %s=%i", table, firstField, firstID, secondField, secondID);
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

//****Sets****
void CVideoDatabase::AddSetToMovie(int idMovie, int idSet)
{
  AddToLinkTable("setlinkmovie", "idSet", idSet, "idMovie", idMovie);
}

//****Actors****
void CVideoDatabase::AddActorToMovie(int idMovie, int idActor, const CStdString& strRole)
{
  AddLinkToActor("actorlinkmovie", idActor, "idMovie", idMovie, strRole);
}

void CVideoDatabase::AddActorToTvShow(int idTvShow, int idActor, const CStdString& strRole)
{
  AddLinkToActor("actorlinktvshow", idActor, "idShow", idTvShow, strRole);
}

void CVideoDatabase::AddActorToEpisode(int idEpisode, int idActor, const CStdString& strRole)
{
  AddLinkToActor("actorlinkepisode", idActor, "idEpisode", idEpisode, strRole);
}

void CVideoDatabase::AddArtistToMusicVideo(int idMVideo, int idArtist)
{
  AddToLinkTable("artistlinkmusicvideo", "idArtist", idArtist, "idMVideo", idMVideo);
}

//****Directors + Writers****
void CVideoDatabase::AddDirectorToMovie(int idMovie, int idDirector)
{
  AddToLinkTable("directorlinkmovie", "idDirector", idDirector, "idMovie", idMovie);
}

void CVideoDatabase::AddDirectorToTvShow(int idTvShow, int idDirector)
{
  AddToLinkTable("directorlinktvshow", "idDirector", idDirector, "idShow", idTvShow);
}

void CVideoDatabase::AddWriterToEpisode(int idEpisode, int idWriter)
{
  AddToLinkTable("writerlinkepisode", "idWriter", idWriter, "idEpisode", idEpisode);
}

void CVideoDatabase::AddWriterToMovie(int idMovie, int idWriter)
{
  AddToLinkTable("writerlinkmovie", "idWriter", idWriter, "idMovie", idMovie);
}

void CVideoDatabase::AddDirectorToEpisode(int idEpisode, int idDirector)
{
  AddToLinkTable("directorlinkepisode", "idDirector", idDirector, "idEpisode", idEpisode);
}

void CVideoDatabase::AddDirectorToMusicVideo(int idMVideo, int idDirector)
{
  AddToLinkTable("directorlinkmusicvideo", "idDirector", idDirector, "idMVideo", idMVideo);
}

//****Studios****
void CVideoDatabase::AddStudioToMovie(int idMovie, int idStudio)
{
  AddToLinkTable("studiolinkmovie", "idStudio", idStudio, "idMovie", idMovie);
}

void CVideoDatabase::AddStudioToTvShow(int idTvShow, int idStudio)
{
  AddToLinkTable("studiolinktvshow", "idStudio", idStudio, "idShow", idTvShow);
}

void CVideoDatabase::AddStudioToMusicVideo(int idMVideo, int idStudio)
{
  AddToLinkTable("studiolinkmusicvideo", "idStudio", idStudio, "idMVideo", idMVideo);
}

//****Genres****
void CVideoDatabase::AddGenreToMovie(int idMovie, int idGenre)
{
  AddToLinkTable("genrelinkmovie", "idGenre", idGenre, "idMovie", idMovie);
}

void CVideoDatabase::AddGenreToTvShow(int idTvShow, int idGenre)
{
  AddToLinkTable("genrelinktvshow", "idGenre", idGenre, "idShow", idTvShow);
}

void CVideoDatabase::AddGenreToMusicVideo(int idMVideo, int idGenre)
{
  AddToLinkTable("genrelinkmusicvideo", "idGenre", idGenre, "idMVideo", idMVideo);
}

//********************************************************************************************************************************
bool CVideoDatabase::HasMovieInfo(const CStdString& strFilenameAndPath)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    int idMovie = GetMovieId(strFilenameAndPath);
    return (idMovie > 0); // index of zero is also invalid

    // work in progress
    if (idMovie > 0)
    {
      // get title.  if no title, the id was "deleted" for in-place update
      CVideoInfoTag details;
      GetMovieInfo(strFilenameAndPath, details, idMovie);
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
    int idTvShow = GetTvShowId(strPath);
    return (idTvShow > 0); // index of zero is also invalid

    // work in progress
    if (idTvShow > 0)
    {
      // get title. if no title, the id was "deleted" for in-place update
      CVideoInfoTag details;
      GetTvShowInfo(strPath, details, idTvShow);
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
    int idEpisode = GetEpisodeId(strFilenameAndPath);
    return (idEpisode > 0); // index of zero is also invalid

    // work in progress
    if (idEpisode > 0)
    {
      // get title.  if no title, the id was "deleted" for in-place update
      CVideoInfoTag details;
      GetEpisodeInfo(strFilenameAndPath, details, idEpisode);
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
    int idMVideo = GetMusicVideoId(strFilenameAndPath);
    return (idMVideo > 0); // index of zero is also invalid

    // work in progress
    if (idMVideo > 0)
    {
      // get title.  if no title, the id was "deleted" for in-place update
      CVideoInfoTag details;
      GetMusicVideoInfo(strFilenameAndPath, details, idMVideo);
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

    int idTvShow = GetTvShowId(strPath);
    if ( idTvShow < 0) return ;

    CFileItemList items;
    CStdString strPath2;
    strPath2.Format("videodb://2/2/%i/",idTvShow);
    GetSeasonsNav(strPath2,items,-1,-1,-1,-1,idTvShow);
    for( int i=0;i<items.Size();++i )
      XFILE::CFile::Delete(items[i]->GetCachedSeasonThumb());

    DeleteThumbForItem(strPath,true);

    CStdString strSQL;
    strSQL=FormatSQL("delete from genrelinktvshow where idShow=%i", idTvShow);
    m_pDS->exec(strSQL.c_str());

    strSQL=FormatSQL("delete from actorlinktvshow where idShow=%i", idTvShow);
    m_pDS->exec(strSQL.c_str());

    strSQL=FormatSQL("delete from directorlinktvshow where idShow=%i", idTvShow);
    m_pDS->exec(strSQL.c_str());

    strSQL=FormatSQL("delete from studiolinktvshow where idShow=%i", idTvShow);
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
    strSQL = strSQL.Mid(0, strSQL.size() - 1) + FormatSQL(" where idShow=%i", idTvShow);
    m_pDS->exec(strSQL.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strPath.c_str());
  }
}

//********************************************************************************************************************************
void CVideoDatabase::GetMoviesByActor(const CStdString& strActor, CFileItemList& items)
{
  CStdString where;
  where.Format("join actorlinkmovie on actorlinkmovie.idMovie=movieview.idMovie "
               "join actors on actors.idActor=actorlinkmovie.idActor "
               "where actors.strActor='%s'", strActor.c_str());
  GetMoviesByWhere("videodb://1/2/", where, "", items);
}

void CVideoDatabase::GetTvShowsByActor(const CStdString& strActor, CFileItemList& items)
{
  CStdString where = FormatSQL("join actorlinktvshow on actorlinktvshow.idShow=tvshow.idShow "
                               "join actors on actors.idActor=actorlinktvshow.idActor "
                               "where actors.strActor='%s'", strActor.c_str());
  GetTvShowsByWhere("videodb://2/2/", where, items);
}

void CVideoDatabase::GetEpisodesByActor(const CStdString& strActor, CFileItemList& items)
{
  CStdString where = FormatSQL("join actorlinkepisode on actorlinkepisode.idEpisode=episodeview.idEpisode "
                               "join actors on actors.idActor=actorlinkepisode.idActor "
                               "where actors.strActor='%s'", strActor.c_str());
  GetEpisodesByWhere("videodb://2/2/", where, items);
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
      strSQL=FormatSQL("select distinct * from musicvideoview join artistlinkmusicvideo on artistlinkmusicvideo.idMVideo=musicvideoview.idMVideo join actors on actors.idActor=artistlinkmusicvideo.idArtist");
    else
      strSQL=FormatSQL("select * from musicvideoview join artistlinkmusicvideo on artistlinkmusicvideo.idMVideo=musicvideoview.idMVideo join actors on actors.idActor=artistlinkmusicvideo.idArtist where actors.strActor='%s'", strArtist.c_str());
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
void CVideoDatabase::GetMovieInfo(const CStdString& strFilenameAndPath, CVideoInfoTag& details, int idMovie /* = -1 */)
{
  try
  {
    // TODO: Optimize this - no need for all the queries!
    if (idMovie < 0)
      idMovie = GetMovieId(strFilenameAndPath);
    if (idMovie < 0) return ;

    CStdString sql = FormatSQL("select * from movieview where idMovie=%i", idMovie);
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
void CVideoDatabase::GetTvShowInfo(const CStdString& strPath, CVideoInfoTag& details, int idTvShow /* = -1 */)
{
  try
  {
    if (idTvShow < 0)
      idTvShow = GetTvShowId(strPath);
    if (idTvShow < 0) return ;

    CStdString sql = FormatSQL(VIDEO_DATABASE_VIEW_TVSHOW " WHERE tvshow.idShow=%i", idTvShow);
    if (!m_pDS->query(sql.c_str()))
      return;
    details = GetDetailsForTvShow(m_pDS, true);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strPath.c_str());
  }
}

bool CVideoDatabase::GetEpisodeInfo(const CStdString& strFilenameAndPath, CVideoInfoTag& details, int idEpisode /* = -1 */)
{
  try
  {
    // TODO: Optimize this - no need for all the queries!
    if (idEpisode < 0)
      idEpisode = GetEpisodeId(strFilenameAndPath);
    if (idEpisode < 0) return false;

    CStdString sql = FormatSQL("select * from episodeview where idEpisode=%i",idEpisode);
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

void CVideoDatabase::GetMusicVideoInfo(const CStdString& strFilenameAndPath, CVideoInfoTag& details, int idMVideo /* = -1 */)
{
  try
  {
    // TODO: Optimize this - no need for all the queries!
    if (idMVideo < 0)
      idMVideo = GetMusicVideoId(strFilenameAndPath);
    if (idMVideo < 0) return ;

    CStdString sql = FormatSQL("select * from musicvideoview where idMVideo=%i", idMVideo);
    if (!m_pDS->query(sql.c_str()))
      return;
    details = GetDetailsForMusicVideo(m_pDS);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strFilenameAndPath.c_str());
  }
}

void CVideoDatabase::AddGenreAndDirectorsAndStudios(const CVideoInfoTag& details, vector<int>& vecDirectors, vector<int>& vecGenres, vector<int>& vecStudios)
{
  // add all directors
  if (!details.m_strDirector.IsEmpty())
  {
    CStdStringArray directors;
    StringUtils::SplitString(details.m_strDirector, g_advancedSettings.m_videoItemSeparator, directors);
    for (unsigned int i = 0; i < directors.size(); i++)
    {
      CStdString strDirector(directors[i]);
      strDirector.Trim();
      int idDirector = AddActor(strDirector,"");
      vecDirectors.push_back(idDirector);
    }
  }

  // add all genres
  if (!details.m_strGenre.IsEmpty())
  {
    CStdStringArray genres;
    StringUtils::SplitString(details.m_strGenre, g_advancedSettings.m_videoItemSeparator, genres);
    for (unsigned int i = 0; i < genres.size(); i++)
    {
      CStdString strGenre(genres[i]);
      strGenre.Trim();
      int idGenre = AddGenre(strGenre);
      vecGenres.push_back(idGenre);
    }
  }
  // add all studios
  if (!details.m_strStudio.IsEmpty())
  {
    CStdStringArray studios;
    StringUtils::SplitString(details.m_strStudio, g_advancedSettings.m_videoItemSeparator, studios);
    for (unsigned int i = 0; i < studios.size(); i++)
    {
      CStdString strStudio(studios[i]);
      strStudio.Trim();
      int idStudio = AddStudio(strStudio);
      vecStudios.push_back(idStudio);
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
int CVideoDatabase::SetDetailsForMovie(const CStdString& strFilenameAndPath, const CVideoInfoTag& details)
{
  try
  {
    CVideoInfoTag info = details;

    int idFile = GetFileId(strFilenameAndPath);

    if (idFile < 0)
      idFile = AddFile(strFilenameAndPath);

    int idMovie = GetMovieId(strFilenameAndPath);

    if (idMovie > -1)
      DeleteMovie(strFilenameAndPath, true); // true to keep the table entry

    BeginTransaction();

    idMovie = AddMovie(strFilenameAndPath);
    if (idMovie < 0)
    {
      CommitTransaction();
      return idMovie;
    }

    vector<int> vecDirectors;
    vector<int> vecGenres;
    vector<int> vecStudios;
    AddGenreAndDirectorsAndStudios(info,vecDirectors,vecGenres,vecStudios);

    for (unsigned int i = 0; i < vecGenres.size(); ++i)
      AddGenreToMovie(idMovie, vecGenres[i]);

    for (unsigned int i = 0; i < vecDirectors.size(); ++i)
      AddDirectorToMovie(idMovie, vecDirectors[i]);

    for (unsigned int i = 0; i < vecStudios.size(); ++i)
      AddStudioToMovie(idMovie, vecStudios[i]);

    // add writers...
    if (!info.m_strWritingCredits.IsEmpty())
    {
      CStdStringArray writers;
      StringUtils::SplitString(info.m_strWritingCredits, g_advancedSettings.m_videoItemSeparator, writers);
      for (unsigned int i = 0; i < writers.size(); i++)
      {
        CStdString writer(writers[i]);
        writer.Trim();
        int idWriter = AddActor(writer,"");
        AddWriterToMovie(idMovie, idWriter );
      }
    }

    // add cast...
    for (CVideoInfoTag::iCast it = info.m_cast.begin(); it != info.m_cast.end(); ++it)
    {
      int idActor = AddActor(it->strName,it->thumbUrl.m_xml);
      AddActorToMovie(idMovie, idActor, it->strRole);
    }

    // add sets...
    if (!info.m_strSet.IsEmpty())
    {
      CStdStringArray sets;
      StringUtils::SplitString(info.m_strSet, g_advancedSettings.m_videoItemSeparator, sets);
      for (unsigned int i = 0; i < sets.size(); i++)
      {
        CStdString set(sets[i]);
        set.Trim();
        int idSet = AddSet(set);
        AddSetToMovie(idMovie, idSet);
      }
    }

    if (details.HasStreamDetails())
      SetStreamDetailsForFileId(details.m_streamDetails, idFile);

    // update our movie table (we know it was added already above)
    // and insert the new row
    CStdString sql = "update movie set " + GetValueString(info, VIDEODB_ID_MIN, VIDEODB_ID_MAX, DbMovieOffsets);
    sql += FormatSQL(" where idMovie=%i", idMovie);
    m_pDS->exec(sql.c_str());
    CommitTransaction();
    return idMovie;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strFilenameAndPath.c_str());
  }
  return -1;
}

int CVideoDatabase::SetDetailsForTvShow(const CStdString& strPath, const CVideoInfoTag& details)
{
  try
  {
    if (!m_pDB.get() || !m_pDS.get())
    {
      CLog::Log(LOGERROR, "%s: called without database open", __FUNCTION__);
      return -1;
    }

    BeginTransaction();

    int idTvShow = GetTvShowId(strPath);
    if (idTvShow < 0)
      idTvShow = AddTvShow(strPath);

    vector<int> vecDirectors;
    vector<int> vecGenres;
    vector<int> vecStudios;
    AddGenreAndDirectorsAndStudios(details,vecDirectors,vecGenres,vecStudios);

    // add cast...
    for (CVideoInfoTag::iCast it = details.m_cast.begin(); it != details.m_cast.end(); ++it)
    {
      int idActor = AddActor(it->strName,it->thumbUrl.m_xml);
      AddActorToTvShow(idTvShow, idActor, it->strRole);
    }

    unsigned int i;
    for (i = 0; i < vecGenres.size(); ++i)
    {
      AddGenreToTvShow(idTvShow, vecGenres[i]);
    }

    for (i = 0; i < vecDirectors.size(); ++i)
    {
      AddDirectorToTvShow(idTvShow, vecDirectors[i]);
    }

    for (i = 0; i < vecStudios.size(); ++i)
    {
      AddStudioToTvShow(idTvShow, vecStudios[i]);
    }

    // and insert the new row
    CStdString sql = "update tvshow set " + GetValueString(details, VIDEODB_ID_TV_MIN, VIDEODB_ID_TV_MAX, DbTvShowOffsets);
    sql += FormatSQL("where idShow=%i", idTvShow);
    m_pDS->exec(sql.c_str());
    CommitTransaction();
    return idTvShow;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strPath.c_str());
  }

  return -1;
}

int CVideoDatabase::SetDetailsForEpisode(const CStdString& strFilenameAndPath, const CVideoInfoTag& details, int idShow, int idEpisode)
{
  try
  {
    BeginTransaction();
    if (idEpisode == -1)
    {
      idEpisode = GetEpisodeId(strFilenameAndPath);
      if (idEpisode > 0)
        DeleteEpisode(strFilenameAndPath,idEpisode);

      idEpisode = AddEpisode(idShow,strFilenameAndPath);
      if (idEpisode < 0)
      {
        CommitTransaction();
        return -1;
      }
    }

    vector<int> vecDirectors;
    vector<int> vecGenres;
    vector<int> vecStudios;
    AddGenreAndDirectorsAndStudios(details,vecDirectors,vecGenres,vecStudios);

    // add cast...
    for (CVideoInfoTag::iCast it = details.m_cast.begin(); it != details.m_cast.end(); ++it)
    {
      int idActor = AddActor(it->strName,it->thumbUrl.m_xml);
      AddActorToEpisode(idEpisode, idActor, it->strRole);
    }

    // add writers...
    if (!details.m_strWritingCredits.IsEmpty())
    {
      CStdStringArray writers;
      StringUtils::SplitString(details.m_strWritingCredits, g_advancedSettings.m_videoItemSeparator, writers);
      for (unsigned int i = 0; i < writers.size(); i++)
      {
        CStdString writer(writers[i]);
        writer.Trim();
        int idWriter = AddActor(writer,"");
        AddWriterToEpisode(idEpisode, idWriter );
      }
    }

    for (unsigned int i = 0; i < vecDirectors.size(); ++i)
    {
      AddDirectorToEpisode(idEpisode, vecDirectors[i]);
    }

    if (details.HasStreamDetails())
    {
      if (details.m_iFileId != -1)
        SetStreamDetailsForFileId(details.m_streamDetails, details.m_iFileId);
      else
        SetStreamDetailsForFile(details.m_streamDetails, strFilenameAndPath);
    }

    // and insert the new row
    CStdString sql = "update episode set " + GetValueString(details, VIDEODB_ID_EPISODE_MIN, VIDEODB_ID_EPISODE_MAX, DbEpisodeOffsets);
    sql += FormatSQL("where idEpisode=%i", idEpisode);
    m_pDS->exec(sql.c_str());
    CommitTransaction();
    return idEpisode;
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

    int idFile = GetFileId(strFilenameAndPath);
    if (idFile < 0)
      idFile = AddFile(strFilenameAndPath);
    int idMVideo = GetMusicVideoId(strFilenameAndPath);
    if (idMVideo > -1)
    {
      DeleteMusicVideo(strFilenameAndPath);
    }
    idMVideo = AddMusicVideo(strFilenameAndPath);
    if (idMVideo < 0)
    {
      CommitTransaction();
      return;
    }

    vector<int> vecDirectors;
    vector<int> vecGenres;
    vector<int> vecStudios;
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
        int idArtist = AddActor(artist,"");
        AddArtistToMusicVideo(idMVideo, idArtist);
      }
    }

    unsigned int i;
    for (i = 0; i < vecGenres.size(); ++i)
    {
      AddGenreToMusicVideo(idMVideo, vecGenres[i]);
    }

    for (i = 0; i < vecDirectors.size(); ++i)
    {
      AddDirectorToMusicVideo(idMVideo, vecDirectors[i]);
    }

    for (i = 0; i < vecStudios.size(); ++i)
    {
      AddStudioToMusicVideo(idMVideo, vecStudios[i]);
    }

    if (details.HasStreamDetails())
      SetStreamDetailsForFileId(details.m_streamDetails, idFile);

    // update our movie table (we know it was added already above)
    // and insert the new row
    CStdString sql = "update musicvideo set " + GetValueString(details, VIDEODB_ID_MUSICVIDEO_MIN, VIDEODB_ID_MUSICVIDEO_MAX, DbMusicVideoOffsets);
    sql += FormatSQL(" where idMVideo=%i", idMVideo);
    m_pDS->exec(sql.c_str());
    CommitTransaction();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strFilenameAndPath.c_str());
  }
}

void CVideoDatabase::SetStreamDetailsForFile(const CStreamDetails& details, const CStdString &strFileNameAndPath)
{
  // AddFile checks to make sure the file isn't already in the DB first
  int idFile = AddFile(strFileNameAndPath);
  if (idFile < 0)
    return;
  SetStreamDetailsForFileId(details, idFile);
}

void CVideoDatabase::SetStreamDetailsForFileId(const CStreamDetails& details, int idFile)
{
  if (idFile < 0)
    return;

  try
  {
    BeginTransaction();
    m_pDS->exec(FormatSQL("DELETE FROM streamdetails WHERE idFile = %i", idFile));

    for (int i=1; i<=details.GetVideoStreamCount(); i++)
    {
      m_pDS->exec(FormatSQL("INSERT INTO streamdetails "
        "(idFile, iStreamType, strVideoCodec, fVideoAspect, iVideoWidth, iVideoHeight) "
        "VALUES (%i,%i,'%s',%f,%i,%i)",
        idFile, (int)CStreamDetail::VIDEO,
        details.GetVideoCodec(i).c_str(), details.GetVideoAspect(i),
        details.GetVideoWidth(i), details.GetVideoHeight(i)));
    }
    for (int i=1; i<=details.GetAudioStreamCount(); i++)
    {
      m_pDS->exec(FormatSQL("INSERT INTO streamdetails "
        "(idFile, iStreamType, strAudioCodec, iAudioChannels, strAudioLanguage) "
        "VALUES (%i,%i,'%s',%i,'%s')",
        idFile, (int)CStreamDetail::AUDIO,
        details.GetAudioCodec(i).c_str(), details.GetAudioChannels(i),
        details.GetAudioLanguage(i).c_str()));
    }
    for (int i=1; i<=details.GetSubtitleStreamCount(); i++)
    {
      m_pDS->exec(FormatSQL("INSERT INTO streamdetails "
        "(idFile, iStreamType, strSubtitleLanguage) "
        "VALUES (%i,%i,'%s')",
        idFile, (int)CStreamDetail::SUBTITLE,
        details.GetSubtitleLanguage(i).c_str()));
    }

    CommitTransaction();
  }
  catch (...)
  {
    RollbackTransaction();
    CLog::Log(LOGERROR, "%s (%i) failed", __FUNCTION__, idFile);
  }
}

//********************************************************************************************************************************
void CVideoDatabase::GetFilePathById(int idMovie, CStdString &filePath, VIDEODB_CONTENT_TYPE iType)
{
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    if (idMovie < 0) return ;

    CStdString strSQL;
    if (iType == VIDEODB_CONTENT_MOVIES)
      strSQL=FormatSQL("select path.strPath,files.strFileName from path, files, movie where path.idPath=files.idPath and files.idFile=movie.idFile and movie.idMovie=%i order by strFilename", idMovie );
    if (iType == VIDEODB_CONTENT_EPISODES)
      strSQL=FormatSQL("select path.strPath,files.strFileName from path, files, episode where path.idPath=files.idPath and files.idFile=episode.idFile and episode.idEpisode=%i order by strFilename", idMovie );
    if (iType == VIDEODB_CONTENT_TVSHOWS)
      strSQL=FormatSQL("select path.strPath from path,tvshowlinkpath where path.idPath=tvshowlinkpath.idPath and tvshowlinkpath.idShow=%i", idMovie );
    if (iType ==VIDEODB_CONTENT_MUSICVIDEOS)
      strSQL=FormatSQL("select path.strPath,files.strFileName from path, files, musicvideo where path.idPath=files.idPath and files.idFile=musicvideo.idFile and musicvideo.idMVideo=%i order by strFilename", idMovie );

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
    int idFile = GetFileId(strFilenameAndPath);
    if (idFile < 0) return ;
    if (!bAppend)
      bookmarks.erase(bookmarks.begin(), bookmarks.end());
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    CStdString strSQL=FormatSQL("select * from bookmark where idFile=%i and type=%i order by timeInSeconds", idFile, (int)type);
    m_pDS->query( strSQL.c_str() );
    while (!m_pDS->eof())
    {
      CBookmark bookmark;
      bookmark.timeInSeconds = m_pDS->fv("timeInSeconds").get_asDouble();
      bookmark.totalTimeInSeconds = m_pDS->fv("totalTimeInSeconds").get_asDouble();
      bookmark.thumbNailImage = m_pDS->fv("thumbNailImage").get_asString();
      bookmark.playerState = m_pDS->fv("playerState").get_asString();
      bookmark.player = m_pDS->fv("player").get_asString();
      bookmark.type = type;
      if (type == CBookmark::EPISODE)
      {
        CStdString strSQL2=FormatSQL("select c%02d, c%02d from episode where c%02d=%i order by c%02d, c%02d", VIDEODB_ID_EPISODE_EPISODE, VIDEODB_ID_EPISODE_SEASON, VIDEODB_ID_EPISODE_BOOKMARK, m_pDS->fv("idBookmark").get_asInt(), VIDEODB_ID_EPISODE_SORTSEASON, VIDEODB_ID_EPISODE_SORTEPISODE);
        m_pDS2->query(strSQL2.c_str());
        bookmark.episodeNumber = m_pDS2->fv(0).get_asInt();
        bookmark.seasonNumber = m_pDS2->fv(1).get_asInt();
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
    int idFile = GetFileId(strFilenameAndPath);
    if (idFile < 0)
    {
      // Doesn't exist in the database yet - add it.
      // TODO: It doesn't appear to me that the CDLabel parameter or the subtitles
      // parameter is anywhere in use in XBMC.
      idFile = AddFile(strFilenameAndPath);
      if (idFile < 0)
        return;
    }
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    CStdString strSQL;
    int idBookmark=-1;
    if (type == CBookmark::RESUME) // get the same resume mark bookmark each time type
    {
      strSQL=FormatSQL("select idBookmark from bookmark where idFile=%i and type=1", idFile);
    }
    else if (type == CBookmark::STANDARD) // get the same bookmark again, and update. not sure here as a dvd can have same time in multiple places, state will differ thou
    {
      /* get a bookmark within the same time as previous */
      double mintime = bookmark.timeInSeconds - 0.5f;
      double maxtime = bookmark.timeInSeconds + 0.5f;
      strSQL=FormatSQL("select idBookmark from bookmark where idFile=%i and type=%i and (timeInSeconds between %f and %f) and playerState='%s'", idFile, (int)type, mintime, maxtime, bookmark.playerState.c_str());
    }

    if (type != CBookmark::EPISODE)
    {
      // get current id
      m_pDS->query( strSQL.c_str() );
      if (m_pDS->num_rows() != 0)
        idBookmark = m_pDS->get_field_value("idBookmark").get_asInt();
      m_pDS->close();
    }
    // update or insert depending if it existed before
    if (idBookmark >= 0 )
      strSQL=FormatSQL("update bookmark set timeInSeconds = %f, totalTimeInSeconds = %f, thumbNailImage = '%s', player = '%s', playerState = '%s' where idBookmark = %i", bookmark.timeInSeconds, bookmark.totalTimeInSeconds, bookmark.thumbNailImage.c_str(), bookmark.player.c_str(), bookmark.playerState.c_str(), idBookmark);
    else
      strSQL=FormatSQL("insert into bookmark (idBookmark, idFile, timeInSeconds, totalTimeInSeconds, thumbNailImage, player, playerState, type) values(NULL,%i,%f,%f,'%s','%s','%s', %i)", idFile, bookmark.timeInSeconds, bookmark.totalTimeInSeconds, bookmark.thumbNailImage.c_str(), bookmark.player.c_str(), bookmark.playerState.c_str(), (int)type);

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
    int idFile = GetFileId(strFilenameAndPath);
    if (idFile < 0) return ;
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    /* a litle bit uggly, we clear first bookmark that is within one second of given */
    /* should be no problem since we never add bookmarks that are closer than that   */
    double mintime = bookmark.timeInSeconds - 0.5f;
    double maxtime = bookmark.timeInSeconds + 0.5f;
    CStdString strSQL = FormatSQL("select idBookmark from bookmark where idFile=%i and type=%i and playerState like '%s' and player like '%s' and (timeInSeconds between %f and %f)", idFile, type, bookmark.playerState.c_str(), bookmark.player.c_str(), mintime, maxtime);

    m_pDS->query( strSQL.c_str() );
    if (m_pDS->num_rows() != 0)
    {
      int idBookmark = m_pDS->get_field_value("idBookmark").get_asInt();
      strSQL=FormatSQL("delete from bookmark where idBookmark=%i",idBookmark);
      m_pDS->exec(strSQL.c_str());
      if (type == CBookmark::EPISODE)
      {
        strSQL=FormatSQL("update episode set c%02d=-1 where idFile=%i and c%02d=%i", VIDEODB_ID_EPISODE_BOOKMARK, idFile, VIDEODB_ID_EPISODE_BOOKMARK, idBookmark);
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
    int idFile = GetFileId(strFilenameAndPath);
    if (idFile < 0) return ;
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    CStdString strSQL=FormatSQL("delete from bookmark where idFile=%i and type=%i", idFile, (int)type);
    m_pDS->exec(strSQL.c_str());
    if (type == CBookmark::EPISODE)
    {
      strSQL=FormatSQL("update episode set c%02d=-1 where idFile=%i", VIDEODB_ID_EPISODE_BOOKMARK, idFile);
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
      bookmark.totalTimeInSeconds = m_pDS->fv("totalTimeInSeconds").get_asDouble();
      bookmark.thumbNailImage = m_pDS->fv("thumbNailImage").get_asString();
      bookmark.playerState = m_pDS->fv("playerState").get_asString();
      bookmark.player = m_pDS->fv("player").get_asString();
      bookmark.type = (CBookmark::EType)m_pDS->fv("type").get_asInt();
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
    int idFile = GetFileId(tag.m_strFileNameAndPath);
    // delete the current episode for the selected episode number
    CStdString strSQL = FormatSQL("delete from bookmark where idBookmark in (select c%02d from episode where c%02d=%i and c%02d=%i and idFile=%i)", VIDEODB_ID_EPISODE_BOOKMARK, VIDEODB_ID_EPISODE_SEASON, tag.m_iSeason, VIDEODB_ID_EPISODE_EPISODE, tag.m_iEpisode, idFile);
    m_pDS->exec(strSQL.c_str());

    AddBookMarkToFile(tag.m_strFileNameAndPath, bookmark, CBookmark::EPISODE);
    int idBookmark = (int)m_pDS->lastinsertid();
    strSQL = FormatSQL("update episode set c%02d=%i where c%02d=%i and c%02d=%i and idFile=%i", VIDEODB_ID_EPISODE_BOOKMARK, idBookmark, VIDEODB_ID_EPISODE_SEASON, tag.m_iSeason, VIDEODB_ID_EPISODE_EPISODE, tag.m_iEpisode, idFile);
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
    int idMovie = GetMovieId(strFilenameAndPath);
    if (idMovie < 0)
    {
      return ;
    }

    int idFile = GetFileId(strFilenameAndPath);
    if (idFile < 0)
      return ;

    BeginTransaction();

    CStdString strSQL;
    strSQL=FormatSQL("delete from genrelinkmovie where idMovie=%i", idMovie);
    m_pDS->exec(strSQL.c_str());

    strSQL=FormatSQL("delete from actorlinkmovie where idMovie=%i", idMovie);
    m_pDS->exec(strSQL.c_str());

    strSQL=FormatSQL("delete from directorlinkmovie where idMovie=%i", idMovie);
    m_pDS->exec(strSQL.c_str());

    strSQL=FormatSQL("delete from studiolinkmovie where idMovie=%i", idMovie);
    m_pDS->exec(strSQL.c_str());

    strSQL=FormatSQL("delete from setlinkmovie where idMovie=%i", idMovie);
    m_pDS->exec(strSQL.c_str());

    if (!bKeepThumb)
      DeleteThumbForItem(strFilenameAndPath,false);

    DeleteStreamDetails(idFile);

    // keep the movie table entry, linking to tv shows, and bookmarks
    // so we can update the data in place
    // the ancilliary tables are still purged
    if (!bKeepId)
    {
      ClearBookMarksOfFile(strFilenameAndPath);

      strSQL=FormatSQL("delete from movie where idMovie=%i", idMovie);
      m_pDS->exec(strSQL.c_str());

      strSQL=FormatSQL("delete from movielinktvshow where idMovie=%i", idMovie);
      m_pDS->exec(strSQL.c_str());
    }
    /*
    // work in progress
    else
    {
      // clear the title
      strSQL=FormatSQL("update movie set c%02d=NULL where idmovie=%i", VIDEODB_ID_TITLE, idMovie);
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
    int idTvShow=-1;
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;
    idTvShow = GetTvShowId(strPath);
    if (idTvShow < 0)
    {
      return ;
    }

    BeginTransaction();

    CStdString strSQL=FormatSQL("select tvshowlinkepisode.idEpisode,path.strPath,files.strFileName from tvshowlinkepisode,path,files,episode where tvshowlinkepisode.idShow=%i and tvshowlinkepisode.idEpisode=episode.idEpisode and episode.idFile=files.idFile and files.idPath=path.idPath",idTvShow);
    m_pDS2->query(strSQL.c_str());
    while (!m_pDS2->eof())
    {
      CStdString strFilenameAndPath;
      CStdString strPath = m_pDS2->fv("path.strPath").get_asString();
      CStdString strFileName = m_pDS2->fv("files.strFilename").get_asString();
      ConstructPath(strFilenameAndPath, strPath, strFileName);
      DeleteEpisode(strFilenameAndPath, m_pDS2->fv(0).get_asInt(), bKeepId);
      m_pDS2->next();
    }

    strSQL=FormatSQL("delete from genrelinktvshow where idShow=%i", idTvShow);
    m_pDS->exec(strSQL.c_str());

    strSQL=FormatSQL("delete from actorlinktvshow where idShow=%i", idTvShow);
    m_pDS->exec(strSQL.c_str());

    strSQL=FormatSQL("delete from directorlinktvshow where idShow=%i", idTvShow);
    m_pDS->exec(strSQL.c_str());

    strSQL=FormatSQL("delete from tvshowlinkpath where idShow=%i", idTvShow);
    m_pDS->exec(strSQL.c_str());

    strSQL=FormatSQL("delete from studiolinktvshow where idShow=%i", idTvShow);
    m_pDS->exec(strSQL.c_str());

    if (!bKeepThumb)
      DeleteThumbForItem(strPath,true);

    // keep tvshow table and movielink table so we can update data in place
    if (!bKeepId)
    {
      strSQL=FormatSQL("delete from tvshow where idShow=%i", idTvShow);
      m_pDS->exec(strSQL.c_str());

      strSQL=FormatSQL("delete from movielinktvshow where idShow=%i", idTvShow);
      m_pDS->exec(strSQL.c_str());
    }

    InvalidatePathHash(strPath);

    CommitTransaction();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strPath.c_str());
  }
}

void CVideoDatabase::DeleteEpisode(const CStdString& strFilenameAndPath, int idEpisode, bool bKeepId /* = false */, bool bKeepThumb /* = false */)
{
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;
    if (idEpisode < 0)
    {
      idEpisode = GetEpisodeId(strFilenameAndPath);
      if (idEpisode < 0)
      {
        return ;
      }
    }

    int idFile = GetFileId(strFilenameAndPath);
    if (idFile < 0)
      return ;

    CStdString strSQL;
    strSQL=FormatSQL("delete from actorlinkepisode where idEpisode=%i", idEpisode);
    m_pDS->exec(strSQL.c_str());

    strSQL=FormatSQL("delete from directorlinkepisode where idEpisode=%i", idEpisode);
    m_pDS->exec(strSQL.c_str());

    strSQL=FormatSQL("select tvshowlinkepisode.idshow from tvshowlinkepisode where idEpisode=%i",idEpisode);
    m_pDS->query(strSQL.c_str());

    strSQL=FormatSQL("delete from tvshowlinkepisode where idEpisode=%i", idEpisode);
    m_pDS->exec(strSQL.c_str());

    if (!bKeepThumb)
      DeleteThumbForItem(strFilenameAndPath, false, idEpisode);

    DeleteStreamDetails(idFile);

    // keep episode table entry and bookmarks so we can update the data in place
    // the ancilliary tables are still purged
    if (!bKeepId)
    {
      ClearBookMarksOfFile(strFilenameAndPath);

      strSQL=FormatSQL("delete from episode where idEpisode=%i", idEpisode);
      m_pDS->exec(strSQL.c_str());
    }

  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strFilenameAndPath.c_str());
  }
}

void CVideoDatabase::DeleteMusicVideo(const CStdString& strFilenameAndPath, bool bKeepId /* = false */, bool bKeepThumb /* = false */)
{
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;
    int idMVideo = GetMusicVideoId(strFilenameAndPath);
    if (idMVideo < 0)
    {
      return ;
    }

    int idFile = GetFileId(strFilenameAndPath);
    if (idFile < 0)
      return ;

    BeginTransaction();

    CStdString strSQL;
    strSQL=FormatSQL("delete from genrelinkmusicvideo where idMVideo=%i", idMVideo);
    m_pDS->exec(strSQL.c_str());

    strSQL=FormatSQL("delete from artistlinkmusicvideo where idMVideo=%i", idMVideo);
    m_pDS->exec(strSQL.c_str());

    strSQL=FormatSQL("delete from directorlinkmusicvideo where idMVideo=%i", idMVideo);
    m_pDS->exec(strSQL.c_str());

    strSQL=FormatSQL("delete from studiolinkmusicvideo where idMVideo=%i", idMVideo);
    m_pDS->exec(strSQL.c_str());

    if (!bKeepThumb)
      DeleteThumbForItem(strFilenameAndPath,false);

    DeleteStreamDetails(idFile);

    // keep the music video table entry and bookmarks so we can update data in place
    // the ancilliary tables are still purged
    if (!bKeepId)
    {
      ClearBookMarksOfFile(strFilenameAndPath);

      strSQL=FormatSQL("delete from musicvideo where idMVideo=%i", idMVideo);
      m_pDS->exec(strSQL.c_str());
    }
    /*
    // work in progress
    else
    {
      // clear the title
      strSQL=FormatSQL("update musicvideo set c%02d=NULL where idmvideo=%i", VIDEODB_ID_MUSICVIDEO_TITLE, idMVideo);
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

void CVideoDatabase::DeleteStreamDetails(int idFile)
{
    m_pDS->exec(FormatSQL("delete from streamdetails where idFile=%i", idFile));
}

void CVideoDatabase::DeleteSet(int idSet)
{
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    CStdString strSQL;
    strSQL=FormatSQL("delete from sets where idSet=%i", idSet);
    m_pDS->exec(strSQL.c_str());
    strSQL=FormatSQL("delete from setlinkmovie where idSet=%i", idSet);
    m_pDS->exec(strSQL.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%i) failed", __FUNCTION__, idSet);
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
      *(int*)(((char*)&details)+offsets[i].offset) = pDS->fv(i+1).get_asInt();
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

CVideoInfoTag CVideoDatabase::GetDetailsByTypeAndId(VIDEODB_CONTENT_TYPE type, int id)
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
    default:
      break;
  }

  return details;
}

bool CVideoDatabase::GetStreamDetailsForFileId(CStreamDetails& details, int idFile) const
{
  if (idFile < 0)
    return false;

  bool retVal = false;

  dbiplus::Dataset *pDS = m_pDB->CreateDataset();
  CStdString strSQL = FormatSQL("SELECT * FROM streamdetails WHERE idFile = %i", idFile);
  pDS->query(strSQL);

  details.Reset();
  while (!pDS->eof())
  {
    CStreamDetail::StreamType e = (CStreamDetail::StreamType)pDS->fv(1).get_asInt();
    switch (e)
    {
    case CStreamDetail::VIDEO:
      {
        CStreamDetailVideo *p = new CStreamDetailVideo();
        p->m_strCodec = pDS->fv(2).get_asString();
        p->m_fAspect = pDS->fv(3).get_asFloat();
        p->m_iWidth = pDS->fv(4).get_asInt();
        p->m_iHeight = pDS->fv(5).get_asInt();
        details.AddStream(p);
        retVal = true;
        break;
      }
    case CStreamDetail::AUDIO:
      {
        CStreamDetailAudio *p = new CStreamDetailAudio();
        p->m_strCodec = pDS->fv(6).get_asString();
        if (pDS->fv(7).get_isNull())
          p->m_iChannels = -1;
        else
          p->m_iChannels = pDS->fv(7).get_asInt();
        p->m_strLanguage = pDS->fv(8).get_asString();
        details.AddStream(p);
        retVal = true;
        break;
      }
    case CStreamDetail::SUBTITLE:
      {
        CStreamDetailSubtitle *p = new CStreamDetailSubtitle();
        p->m_strLanguage = pDS->fv(9).get_asString();
        details.AddStream(p);
        retVal = true;
        break;
      }
    }

    pDS->next();
  }

  pDS->close();
  details.DetermineBestStreams();

  return retVal;
}

CVideoInfoTag CVideoDatabase::GetDetailsForMovie(auto_ptr<Dataset> &pDS, bool needsCast /* = false */)
{
  CVideoInfoTag details;
  details.Reset();

  DWORD time = CTimeUtils::GetTimeMS();
  int idMovie = pDS->fv(0).get_asInt();

  GetDetailsFromDB(pDS, VIDEODB_ID_MIN, VIDEODB_ID_MAX, DbMovieOffsets, details);

  details.m_iDbId = idMovie;
  GetCommonDetails(pDS, details);
  movieTime += CTimeUtils::GetTimeMS() - time; time = CTimeUtils::GetTimeMS();

  GetStreamDetailsForFileId(details.m_streamDetails, details.m_iFileId);

  if (needsCast)
  {
    // create cast string
    CStdString strSQL = FormatSQL("SELECT actors.strActor,actorlinkmovie.strRole,actors.strThumb FROM actorlinkmovie,actors WHERE actorlinkmovie.idMovie=%i AND actorlinkmovie.idActor=actors.idActor ORDER BY actors.idActor",idMovie);
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
    castTime += CTimeUtils::GetTimeMS() - time; time = CTimeUtils::GetTimeMS();
    details.m_strPictureURL.Parse();

    // create sets string
    strSQL = FormatSQL("SELECT sets.strSet FROM sets,setlinkmovie WHERE setlinkmovie.idMovie=%i AND setlinkmovie.idSet=sets.idSet ORDER BY sets.idSet",idMovie);
    m_pDS2->query(strSQL.c_str());
    while (!m_pDS2->eof())
    {
      CStdString setName = m_pDS2->fv("sets.strSet").get_asString();
      if (!details.m_strSet.IsEmpty())
        details.m_strSet += g_advancedSettings.m_videoItemSeparator;
      details.m_strSet += setName;
      m_pDS2->next();
    }

    // create tvshowlink string
    vector<int> links;
    GetLinksToTvShow(idMovie,links);
    for (unsigned int i=0;i<links.size();++i)
    {
      strSQL = FormatSQL("select c%02d from tvshow where idShow=%i",
                         VIDEODB_ID_TV_TITLE,links[i]);
      m_pDS2->query(strSQL.c_str());
      if (!m_pDS2->eof())
      {
        if (!details.m_strShowLink.IsEmpty())
          details.m_strShowLink += g_advancedSettings.m_videoItemSeparator;
        details.m_strShowLink += m_pDS2->fv(0).get_asString();
      }
    }
    m_pDS2->close();
  }
  return details;
}

CVideoInfoTag CVideoDatabase::GetDetailsForTvShow(auto_ptr<Dataset> &pDS, bool needsCast /* = false */)
{
  CVideoInfoTag details;
  details.Reset();

  DWORD time = CTimeUtils::GetTimeMS();
  int idTvShow = pDS->fv(0).get_asInt();

  GetDetailsFromDB(pDS, VIDEODB_ID_TV_MIN, VIDEODB_ID_TV_MAX, DbTvShowOffsets, details);
  details.m_iDbId = idTvShow;
  details.m_strPath = pDS->fv(VIDEODB_DETAILS_TVSHOW_PATH).get_asString();
  details.m_iEpisode = m_pDS->fv(VIDEODB_DETAILS_TVSHOW_NUM_EPISODES).get_asInt();
  details.m_playCount = m_pDS->fv(VIDEODB_DETAILS_TVSHOW_NUM_WATCHED).get_asInt();
  details.m_strShowTitle = details.m_strTitle;

  movieTime += CTimeUtils::GetTimeMS() - time; time = CTimeUtils::GetTimeMS();

  if (needsCast)
  {
    // create cast string
    CStdString strSQL = FormatSQL("select actors.strActor,actorlinktvshow.strRole,actors.strThumb from actorlinktvshow,actors where actorlinktvshow.idShow=%i and actorlinktvshow.idActor = actors.idActor",idTvShow);
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
    castTime += CTimeUtils::GetTimeMS() - time; time = CTimeUtils::GetTimeMS();
    details.m_strPictureURL.Parse();
  }
  return details;
}

CVideoInfoTag CVideoDatabase::GetDetailsForEpisode(auto_ptr<Dataset> &pDS, bool needsCast /* = false */)
{
  CVideoInfoTag details;
  details.Reset();

  DWORD time = CTimeUtils::GetTimeMS();
  int idEpisode = pDS->fv(0).get_asInt();

  GetDetailsFromDB(pDS, VIDEODB_ID_EPISODE_MIN, VIDEODB_ID_EPISODE_MAX, DbEpisodeOffsets, details);
  details.m_iDbId = idEpisode;
  GetCommonDetails(pDS, details);
  movieTime += CTimeUtils::GetTimeMS() - time; time = CTimeUtils::GetTimeMS();

  details.m_strMPAARating = pDS->fv(VIDEODB_DETAILS_EPISODE_TVSHOW_MPAA).get_asString();
  details.m_strShowTitle = pDS->fv(VIDEODB_DETAILS_EPISODE_TVSHOW_NAME).get_asString();
  details.m_strStudio = pDS->fv(VIDEODB_DETAILS_EPISODE_TVSHOW_STUDIO).get_asString();

  GetStreamDetailsForFileId(details.m_streamDetails, details.m_iFileId);

  if (needsCast)
  {
    // create cast string
    CStdString strSQL = FormatSQL("select actors.strActor,actorlinkepisode.strRole,actors.strThumb from actorlinkepisode,actors where actorlinkepisode.idEpisode=%i and actorlinkepisode.idActor = actors.idActor",idEpisode);
    m_pDS2->query(strSQL.c_str());
    bool showCast=false;
    while (!m_pDS2->eof() || !showCast)
    {
      if (!m_pDS2->eof())
      {
        SActorInfo info;
        info.strName = m_pDS2->fv("actors.strActor").get_asString();
        info.strRole = m_pDS2->fv("actorlinkepisode.strRole").get_asString();
        info.thumbUrl.ParseString(m_pDS2->fv("actors.strThumb").get_asString());
        details.m_cast.push_back(info);
        m_pDS2->next();
      }
      if (m_pDS2->eof() && !showCast)
      {
        showCast = true;
        int idShow = GetTvShowForEpisode(details.m_iDbId);
        if (idShow > -1)
        {
          strSQL = FormatSQL("select actors.strActor,actorlinktvshow.strRole,actors.strThumb from actorlinktvshow,actors where actorlinktvshow.idShow=%i and actorlinktvshow.idActor = actors.idActor",idShow);
          m_pDS2->query(strSQL.c_str());
        }
      }
    }
    castTime += CTimeUtils::GetTimeMS() - time; time = CTimeUtils::GetTimeMS();
    details.m_strPictureURL.Parse();
    strSQL=FormatSQL("select * from bookmark join episode on episode.c%02d=bookmark.idBookmark where episode.idEpisode=%i and bookmark.type=%i", VIDEODB_ID_EPISODE_BOOKMARK,details.m_iDbId,CBookmark::EPISODE);
    m_pDS2->query(strSQL.c_str());
    if (!m_pDS2->eof())
      details.m_fEpBookmark = m_pDS2->fv("bookmark.timeInSeconds").get_asFloat();
    m_pDS2->close();
  }
  return details;
}

CVideoInfoTag CVideoDatabase::GetDetailsForMusicVideo(auto_ptr<Dataset> &pDS)
{
  CVideoInfoTag details;
  details.Reset();

  DWORD time = CTimeUtils::GetTimeMS();
  int idMovie = pDS->fv(0).get_asInt();

  GetDetailsFromDB(pDS, VIDEODB_ID_MUSICVIDEO_MIN, VIDEODB_ID_MUSICVIDEO_MAX, DbMusicVideoOffsets, details);
  details.m_iDbId = idMovie;
  GetCommonDetails(pDS, details);
  movieTime += CTimeUtils::GetTimeMS() - time; time = CTimeUtils::GetTimeMS();

  GetStreamDetailsForFileId(details.m_streamDetails, details.m_iFileId);

  details.m_strPictureURL.Parse();
  return details;
}

void CVideoDatabase::GetCommonDetails(auto_ptr<Dataset> &pDS, CVideoInfoTag &details)
{
  details.m_iFileId = pDS->fv(VIDEODB_DETAILS_FILEID).get_asInt();
  details.m_strPath = pDS->fv(VIDEODB_DETAILS_PATH).get_asString();
  CStdString strFileName = pDS->fv(VIDEODB_DETAILS_FILE).get_asString();
  ConstructPath(details.m_strFileNameAndPath,details.m_strPath,strFileName);
  details.m_playCount = pDS->fv(VIDEODB_DETAILS_PLAYCOUNT).get_asInt();
  details.m_lastPlayed = pDS->fv(VIDEODB_DETAILS_LASTPLAYED).get_asString();
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
    CStdString strSQL=FormatSQL("select * from settings, files, path where settings.idFile=files.idFile and path.idPath=files.idPath and path.strPath like '%s' and files.strFileName like '%s'", strPath.c_str() , strFileName.c_str());
#else
    int idFile = GetFileId(strFilenameAndPath);
    if (idFile < 0) return false;
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    // ok, now obtain the settings for this file
    CStdString strSQL=FormatSQL("select * from settings where settings.idFile = '%i'", idFile);
#endif
    m_pDS->query( strSQL.c_str() );
    if (m_pDS->num_rows() > 0)
    { // get the video settings info
      settings.m_AudioDelay = m_pDS->fv("AudioDelay").get_asFloat();
      settings.m_AudioStream = m_pDS->fv("AudioStream").get_asInt();
      settings.m_Brightness = m_pDS->fv("Brightness").get_asFloat();
      settings.m_Contrast = m_pDS->fv("Contrast").get_asFloat();
      settings.m_CustomPixelRatio = m_pDS->fv("PixelRatio").get_asFloat();
      settings.m_CustomNonLinStretch = m_pDS->fv("NonLinStretch").get_asBool();
      settings.m_NoiseReduction = m_pDS->fv("NoiseReduction").get_asFloat();
      settings.m_Sharpness = m_pDS->fv("Sharpness").get_asFloat();
      settings.m_CustomZoomAmount = m_pDS->fv("ZoomAmount").get_asFloat();
      settings.m_Gamma = m_pDS->fv("Gamma").get_asFloat();
      settings.m_SubtitleDelay = m_pDS->fv("SubtitleDelay").get_asFloat();
      settings.m_SubtitleOn = m_pDS->fv("SubtitlesOn").get_asBool();
      settings.m_SubtitleStream = m_pDS->fv("SubtitleStream").get_asInt();
      settings.m_ViewMode = m_pDS->fv("ViewMode").get_asInt();
      settings.m_ResumeTime = m_pDS->fv("ResumeTime").get_asInt();
      settings.m_Crop = m_pDS->fv("Crop").get_asBool();
      settings.m_CropLeft = m_pDS->fv("CropLeft").get_asInt();
      settings.m_CropRight = m_pDS->fv("CropRight").get_asInt();
      settings.m_CropTop = m_pDS->fv("CropTop").get_asInt();
      settings.m_CropBottom = m_pDS->fv("CropBottom").get_asInt();
      settings.m_InterlaceMethod = (EINTERLACEMETHOD)m_pDS->fv("Deinterlace").get_asInt();
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
    int idFile = GetFileId(strFilenameAndPath);
    if (idFile < 0)
    { // no files found - we have to add one
      idFile = AddFile(strFilenameAndPath);
      if (idFile < 0) return ;
    }
    CStdString strSQL;
    strSQL.Format("select * from settings where idFile=%i", idFile);
    m_pDS->query( strSQL.c_str() );
    if (m_pDS->num_rows() > 0)
    {
      m_pDS->close();
      // update the item
      strSQL=FormatSQL("update settings set Deinterlace=%i,ViewMode=%i,ZoomAmount=%f,PixelRatio=%f,"
                       "AudioStream=%i,SubtitleStream=%i,SubtitleDelay=%f,SubtitlesOn=%i,Brightness=%f,Contrast=%f,Gamma=%f,"
                       "VolumeAmplification=%f,AudioDelay=%f,OutputToAllSpeakers=%i,Sharpness=%f,NoiseReduction=%f,NonLinStretch=%i,",
                       setting.m_InterlaceMethod, setting.m_ViewMode, setting.m_CustomZoomAmount, setting.m_CustomPixelRatio,
                       setting.m_AudioStream, setting.m_SubtitleStream, setting.m_SubtitleDelay, setting.m_SubtitleOn,
                       setting.m_Brightness, setting.m_Contrast, setting.m_Gamma, setting.m_VolumeAmplification, setting.m_AudioDelay,
                       setting.m_OutputToAllSpeakers,setting.m_Sharpness,setting.m_NoiseReduction,setting.m_CustomNonLinStretch);
      CStdString strSQL2;
      strSQL2=FormatSQL("ResumeTime=%i,Crop=%i,CropLeft=%i,CropRight=%i,CropTop=%i,CropBottom=%i where idFile=%i\n", setting.m_ResumeTime, setting.m_Crop, setting.m_CropLeft, setting.m_CropRight, setting.m_CropTop, setting.m_CropBottom, idFile);
      strSQL += strSQL2;
      m_pDS->exec(strSQL.c_str());
      return ;
    }
    else
    { // add the items
      m_pDS->close();
      strSQL=FormatSQL("insert into settings ( idFile,Deinterlace,ViewMode,ZoomAmount,PixelRatio,"
                       "AudioStream,SubtitleStream,SubtitleDelay,SubtitlesOn,Brightness,Contrast,Gamma,"
                       "VolumeAmplification,AudioDelay,OutputToAllSpeakers,ResumeTime,Crop,CropLeft,CropRight,CropTop,CropBottom,Sharpness,NoiseReduction,NonLinStretch)"
                       " values (%i,%i,%i,%f,%f,%i,%i,%f,%i,%f,%f,%f,%f,%f,%i,",
                       idFile, setting.m_InterlaceMethod, setting.m_ViewMode, setting.m_CustomZoomAmount, setting.m_CustomPixelRatio,
                       setting.m_AudioStream, setting.m_SubtitleStream, setting.m_SubtitleDelay, setting.m_SubtitleOn,
                       setting.m_Brightness, setting.m_Contrast, setting.m_Gamma, setting.m_VolumeAmplification, setting.m_AudioDelay,
                       setting.m_CustomNonLinStretch);
      CStdString strSQL2;
      strSQL2=FormatSQL("%i,%i,%i,%i,%i,%i,%i,%f,%f)\n", setting.m_OutputToAllSpeakers, setting.m_ResumeTime, setting.m_Crop, setting.m_CropLeft, setting.m_CropRight,
                    setting.m_CropTop, setting.m_CropBottom, setting.m_Sharpness, setting.m_NoiseReduction);
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
bool CVideoDatabase::GetStackTimes(const CStdString &filePath, vector<int> &times)
{
  try
  {
    // obtain the FileID (if it exists)
    int idFile = GetFileId(filePath);
    if (idFile < 0) return false;
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    // ok, now obtain the settings for this file
    CStdString strSQL=FormatSQL("select times from stacktimes where idFile=%i\n", idFile);
    m_pDS->query( strSQL.c_str() );
    if (m_pDS->num_rows() > 0)
    { // get the video settings info
      CStdStringArray timeString;
      int timeTotal = 0;
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
void CVideoDatabase::SetStackTimes(const CStdString& filePath, vector<int> &times)
{
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;
    int idFile = GetFileId(filePath);
    if (idFile < 0)
    { // no files found - we have to add one
      idFile = AddFile(filePath);
      if (idFile < 0) return ;
    }

    // delete any existing items
    m_pDS->exec( FormatSQL("delete from stacktimes where idFile=%i", idFile) );

    // add the items
    CStdString timeString;
    timeString.Format("%i", times[0]);
    for (unsigned int i = 1; i < times.size(); i++)
    {
      CStdString time;
      time.Format(",%i", times[i]);
      timeString += time;
    }
    m_pDS->exec( FormatSQL("insert into stacktimes (idFile,times) values (%i,'%s')\n", idFile, timeString.c_str()) );
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
    progress = (CGUIDialogProgress *)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
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
      int idPath = pDS->fv("path.idPath").get_asInt();
      CStdString strCurrPath = pDS->fv("path.strPath").get_asString();
      if (HasTvShowInfo(strCurrPath))
        DeleteTvShow(strCurrPath);
      else
      {
        strSQL=FormatSQL("select files.strFilename from files join movie on movie.idFile=files.idFile where files.idPath=%i",idPath);
        m_pDS2->query(strSQL.c_str());
        if (m_pDS2->eof())
        {
          strSQL=FormatSQL("select files.strFilename from files join musicvideo on musicvideo.idFile=files.idFile where files.idPath=%i",idPath);
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
            strSQL=FormatSQL("select files.strFilename from files join musicvideo on musicvideo.idFile=files.idFile where files.idPath=%i",idPath);
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

void CVideoDatabase::SetScraperForPath(const CStdString& filePath, const ScraperPtr& scraper, const VIDEO::SScanSettings& settings)
{
  // if we have a multipath, set scraper for all contained paths too
  if(CUtil::IsMultiPath(filePath))
  {
    vector<CStdString> paths;
    CMultiPathDirectory::GetPaths(filePath, paths);

    for(unsigned i=0;i<paths.size();i++)
      SetScraperForPath(paths[i],scraper,settings);
  }

  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;
    int idPath = GetPathId(filePath);
    if (idPath < 0)
    { // no path found - we have to add one
      idPath = AddPath(filePath);
      if (idPath < 0) return ;
    }

    // Update
    CStdString strSQL;
    if (settings.exclude)
    { //NB See note in ::GetScraperForPath about strContent=='none'
      strSQL=FormatSQL("update path set strContent='none', strScraper='', scanRecursive=0, useFolderNames=0, strSettings='', noUpdate=0 where idPath=%i", idPath);
    }
    else if(!scraper)
    { // catch clearing content, but not excluding
      strSQL=FormatSQL("update path set strContent='', strScraper='', scanRecursive=0, useFolderNames=0, strSettings='', noUpdate=0 where idPath=%i", idPath);
    }
    else
    {
      assert(scraper->Parent());
      CStdString content = TranslateContent(scraper->Content());
      strSQL=FormatSQL("update path set strContent='%s', strScraper='%s', scanRecursive=%i, useFolderNames=%i, strSettings='%s', noUpdate=%i where idPath=%i", content.c_str(), scraper->Parent()->ID().c_str(),settings.recurse,settings.parent_name,scraper->GetSettings().c_str(),settings.noupdate, idPath);
    }
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
        int idEpisode = m_pDS->fv("files.idEpisode").get_asInt();
        int idMovie = m_pDS->fv("files.idMovie").get_asInt();
        if (idEpisode > -1)
        {
          strSQL=FormatSQL("update episode set idFile=%i where idEpisode=%i",m_pDS->fv("files.idFile").get_asInt(),idEpisode);
        }
        if (idMovie > -1)
          strSQL=FormatSQL("update movie set idFile=%i where idMovie=%i",m_pDS->fv("files.idFile").get_asInt(),idMovie);

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
        strSQL = FormatSQL("update path set strPath='%s' where idPath=%i",strPath.c_str(),m_pDS->fv("path.idPath").get_asInt());
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
                                    "join path on path.idPath=tvshowlinkpath.idPath "
                                    "left outer join ("
                                    "    select tvshow.idShow as idShow,count(1) as totalcount,count(episode.c%02d) as watchedcount from tvshow "
                                    "    join tvshowlinkepisode on tvshow.idShow=tvshowlinkepisode.idShow "
                                    "    join episode on episode.idEpisode=tvshowlinkepisode.idEpisode "
                                    "    group by tvshow.idShow"
                                    ") counts on tvshow.idShow=counts.idShow", VIDEODB_ID_EPISODE_PLAYCOUNT);
      m_pDS->exec(showview.c_str());
      CStdString episodeview = FormatSQL("create view episodeview as select episode.*,files.strFileName as strFileName,"
                                         "path.strPath as strPath,tvshow.c%02d as strTitle,tvshow.idShow as idShow,"
                                         "tvshow.c%02d as premiered from episode "
                                         "join files on files.idFile=episode.idFile "
                                         "join tvshowlinkepisode on episode.idEpisode=tvshowlinkepisode.idEpisode "
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
      vector< pair<int, CStdString> > writingCredits;
      while (!m_pDS->eof())
      {
        writingCredits.push_back(pair<int, CStdString>(m_pDS->fv(0).get_asInt(), m_pDS->fv(1).get_asString()));
        m_pDS->next();
      }
      m_pDS->close();
      for (unsigned int i = 0; i < writingCredits.size(); i++)
      {
        int idEpisode = writingCredits[i].first;
        if (writingCredits[i].second.size())
        {
          CStdStringArray writers;
          StringUtils::SplitString(writingCredits[i].second, g_advancedSettings.m_videoItemSeparator, writers);
          for (unsigned int i = 0; i < writers.size(); i++)
          {
            CStdString writer(writers[i]);
            writer.Trim();
            int idWriter = AddActor(writer,"");
            AddWriterToEpisode(idEpisode, idWriter);
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
      vector< pair<int, CStdString> > writingCredits;
      while (!m_pDS->eof())
      {
        writingCredits.push_back(pair<int, CStdString>(m_pDS->fv(0).get_asInt(), m_pDS->fv(1).get_asString()));
        m_pDS->next();
      }
      m_pDS->close();
      for (unsigned int i = 0; i < writingCredits.size(); i++)
      {
        int idMovie = writingCredits[i].first;
        if (writingCredits[i].second.size())
        {
          CStdStringArray writers;
          StringUtils::SplitString(writingCredits[i].second, g_advancedSettings.m_videoItemSeparator, writers);
          for (unsigned int i = 0; i < writers.size(); i++)
          {
            CStdString writer(writers[i]);
            writer.Trim();
            int idWriter = AddActor(writer,"");
            AddWriterToMovie(idMovie, idWriter);
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
    if (iVersion < 24)
    { // add extra columns to the file table
      m_pDS->exec("alter table files add playCount integer");
      m_pDS->exec("alter table files add lastPlayed text");
      // drop the views
      m_pDS->exec("drop view movieview");
      m_pDS->exec("create view movieview as select movie.*,files.strFileName as strFileName,path.strPath as strPath,files.playCount as playCount,files.lastPlayed as lastPlayed "
                  "from movie join files on files.idFile=movie.idFile join path on path.idPath=files.idPath");
      m_pDS->exec("drop view musicvideoview");
      m_pDS->exec("create view musicvideoview as select musicvideo.*,files.strFileName as strFileName,path.strPath as strPath,files.playCount as playCount,files.lastPlayed as lastPlayed "
                  "from musicvideo join files on files.idFile=musicvideo.idFile join path on path.idPath=files.idPath");
      m_pDS->exec("drop view episodeview");
      CStdString episodeview = FormatSQL("create view episodeview as select episode.*,files.strFileName as strFileName,"
                                         "path.strPath as strPath,files.playCount as playCount,files.lastPlayed as lastPlayed,tvshow.c%02d as strTitle,tvshow.idShow as idShow,"
                                         "tvshow.c%02d as premiered from episode "
                                         "join files on files.idFile=episode.idFile "
                                         "join tvshowlinkepisode on episode.idEpisode=tvshowlinkepisode.idEpisode "
                                         "join tvshow on tvshow.idShow=tvshowlinkepisode.idShow "
                                         "join path on files.idPath=path.idPath",VIDEODB_ID_TV_TITLE, VIDEODB_ID_TV_PREMIERED);
      m_pDS->exec(episodeview.c_str());
      m_pDS->exec("drop view tvshowview");
      CStdString showview=FormatSQL("create view tvshowview as select tvshow.*,path.strPath as strPath,"
                                    "counts.totalcount as totalCount,counts.watchedcount as watchedCount,"
                                    "counts.totalcount=counts.watchedcount as watched from tvshow "
                                    "join tvshowlinkpath on tvshow.idShow=tvshowlinkpath.idShow "
                                    "join path on path.idPath=tvshowlinkpath.idPath "
                                    "left outer join ("
                                    "    select tvshow.idShow as idShow,count(1) as totalcount,count(files.playCount) as watchedcount from tvshow "
                                    "    join tvshowlinkepisode on tvshow.idShow=tvshowlinkepisode.idShow "
                                    "    join episode on episode.idEpisode=tvshowlinkepisode.idEpisode "
                                    "    join files on files.idFile=episode.idFile "
                                    "    group by tvshow.idShow"
                                    ") counts on tvshow.idShow=counts.idShow");
      m_pDS->exec(showview.c_str());
      // and fill the new playcount fields
      CStdString sql;
      sql = FormatSQL("update files set playCount = (select movie.c10 from movie where movie.idFile = files.idFile) " // NOTE: c10 was the old playcount field which is now reused
                      "where exists (select movie.c10 from movie where movie.idFile = files.idFile)");
      m_pDS->exec(sql.c_str());
      sql = FormatSQL("update files set playCount = (select episode.c%02d from episode where episode.idFile = files.idFile) "
        "where exists (select episode.c%02d from episode where episode.idFile = files.idFile)", VIDEODB_ID_EPISODE_PLAYCOUNT, VIDEODB_ID_EPISODE_PLAYCOUNT);
      m_pDS->exec(sql.c_str());
      sql = FormatSQL("update files set playCount = (select musicvideo.c%02d from musicvideo where musicvideo.idFile = files.idFile) "
        "where exists (select musicvideo.c%02d from musicvideo where musicvideo.idFile = files.idFile)", VIDEODB_ID_MUSICVIDEO_PLAYCOUNT, VIDEODB_ID_MUSICVIDEO_PLAYCOUNT);
      m_pDS->exec(sql.c_str());
      // and clear out the old fields
      sql = FormatSQL("update movie set c10=NULL"); // NOTE: c10 was the old playcount field which has now been reused
      m_pDS->exec(sql.c_str());
      sql = FormatSQL("update episode set c%02d=NULL", VIDEODB_ID_EPISODE_PLAYCOUNT);
      m_pDS->exec(sql.c_str());
      sql = FormatSQL("update musicvideo set c%02d=NULL", VIDEODB_ID_MUSICVIDEO_PLAYCOUNT);
      m_pDS->exec(sql.c_str());
    }
    if (iVersion < 25)
    {
      m_pDS->exec("alter table settings add Sharpness float");
      m_pDS->exec("alter table settings add NoiseReduction float");
    }
    if (iVersion < 26)
    {
      CLog::Log(LOGINFO, "create studiolinktvshow table");
      m_pDS->exec("CREATE TABLE studiolinktvshow ( idStudio integer, idShow integer)\n");
      m_pDS->exec("CREATE UNIQUE INDEX ix_studiolinktvshow_1 ON studiolinktvshow ( idStudio, idShow)\n");
      m_pDS->exec("CREATE UNIQUE INDEX ix_studiolinktvshow_2 ON studiolinktvshow ( idShow, idStudio)\n");
    }
    if (iVersion < 27)
    {
      m_pDS->exec("drop view episodeview");
      CStdString episodeview = FormatSQL("create view episodeview as select episode.*,files.strFileName as strFileName,"
                                         "path.strPath as strPath,files.playCount as playCount,files.lastPlayed as lastPlayed,tvshow.c%02d as strTitle,tvshow.c%02d as strStudio,tvshow.idShow as idShow,"
                                         "tvshow.c%02d as premiered from episode "
                                         "join files on files.idFile=episode.idFile "
                                         "join tvshowlinkepisode on episode.idEpisode=tvshowlinkepisode.idEpisode "
                                         "join tvshow on tvshow.idShow=tvshowlinkepisode.idShow "
                                         "join path on files.idPath=path.idPath",VIDEODB_ID_TV_TITLE, VIDEODB_ID_TV_STUDIOS, VIDEODB_ID_TV_PREMIERED);
      m_pDS->exec(episodeview.c_str());
    }
    if (iVersion < 28)
    {
      m_pDS->exec("CREATE TABLE streamdetails (idFile integer, iStreamType integer, "
        "strVideoCodec text, fVideoAspect real, iVideoWidth integer, iVideoHeight integer, "
        "strAudioCodec text, iAudioChannels integer, strAudioLanguage text, strSubtitleLanguage text)");
      m_pDS->exec("CREATE INDEX ix_streamdetails ON streamdetails (idFile)");
    }
    if (iVersion < 29)
    {
      m_pDS->exec("alter table bookmark add totalTimeInSeconds double");
    }
    if (iVersion < 30)
    {
      m_pDS->exec("drop view episodeview");
      CStdString episodeview = FormatSQL("create view episodeview as select episode.*,files.strFileName as strFileName,"
                                       "path.strPath as strPath,files.playCount as playCount,files.lastPlayed as lastPlayed,tvshow.c%02d as strTitle,tvshow.c%02d as strStudio,tvshow.idShow as idShow,"
                                       "tvshow.c%02d as premiered, tvshow.c%02d as mpaa from episode "
                                       "join files on files.idFile=episode.idFile "
                                       "join tvshowlinkepisode on episode.idEpisode=tvshowlinkepisode.idEpisode "
                                       "join tvshow on tvshow.idShow=tvshowlinkepisode.idShow "
                                       "join path on files.idPath=path.idPath",VIDEODB_ID_TV_TITLE, VIDEODB_ID_TV_STUDIOS, VIDEODB_ID_TV_PREMIERED, VIDEODB_ID_TV_MPAA);
      m_pDS->exec(episodeview.c_str());
    }
    if (iVersion < 31)
    {
      const char* tag1[] = {"idMovie","idShow","idEpisode","idMVideo","idActor"};
      const char* tag2[] = {"c08","c06","c06","c02","strThumb"};
      const char* tag3[] = {"movie","tvshow","episode","musicvideo","actors"};
      for (int i=0;i<5;++i)
      {
        CStdString strSQL=FormatSQL("select %s,%s from %s",
                                    tag1[i],tag2[i],tag3[i]);
        m_pDS->query(strSQL.c_str());
        while (!m_pDS->eof())
        {
          TiXmlDocument doc;
          doc.Parse(m_pDS->fv(1).get_asString().c_str());
          if (!doc.RootElement() || !doc.RootElement()->FirstChildElement("thumb"))
          {
            m_pDS->next();
            continue;
          }
          const TiXmlElement* thumb = doc.RootElement()->FirstChildElement("thumb");
          stringstream str;
          while (thumb)
          {
            str << *thumb;
            thumb = thumb->NextSiblingElement("thumb");
          }
          CStdString strSQL = FormatSQL("update %s set %s='%s' where %s=%i",
                                        tag3[i],tag2[i],
                                        str.str().c_str(),tag1[i],
                                        m_pDS->fv(0).get_asInt());
          m_pDS2->exec(strSQL.c_str());
          m_pDS->next();
        }
      }
    }
    if (iVersion < 32)
    {
      CStdString sql;
      sql = FormatSQL("UPDATE movie SET c%02d=NULL", VIDEODB_ID_SORTTITLE);
      m_pDS->exec(sql.c_str());
    }
    if ( iVersion < 33 )
    {
      CLog::Log(LOGINFO, "create sets table");
      m_pDS->exec("CREATE TABLE sets ( idSet integer primary key, strSet text)\n");

      CLog::Log(LOGINFO, "create setlinkmovie table");
      m_pDS->exec("CREATE TABLE setlinkmovie ( idSet integer, idMovie integer)\n");
      m_pDS->exec("CREATE UNIQUE INDEX ix_setlinkmovie_1 ON setlinkmovie ( idSet, idMovie)\n");
      m_pDS->exec("CREATE UNIQUE INDEX ix_setlinkmovie_2 ON setlinkmovie ( idMovie, idSet)\n");
    }
    if (iVersion < 34)
    {
      // fake bump to correct tvdb urls
      CStdString strSQL = FormatSQL("update tvshow set c%02d = replace(c%02d,'images.thetvdb.com','thetvdb.com')",VIDEODB_ID_TV_THUMBURL,VIDEODB_ID_TV_THUMBURL);
      m_pDS->exec(strSQL.c_str());
      strSQL = FormatSQL("update tvshow set c%02d = replace(c%02d,'images.thetvdb.com','thetvdb.com')",VIDEODB_ID_TV_FANART,VIDEODB_ID_TV_FANART);
      m_pDS->exec(strSQL.c_str());
      strSQL = FormatSQL("update actors set strThumb = replace(strThumb,'images.thetvdb.com','thetvdb.com')");
      m_pDS->exec(strSQL.c_str());
      strSQL = FormatSQL("update episode set c%02d = replace(c%02d,'images.thetvdb.com','thetvdb.com')",VIDEODB_ID_EPISODE_THUMBURL,VIDEODB_ID_EPISODE_THUMBURL);
      m_pDS->exec(strSQL.c_str());
    }
    if (iVersion < 35)
    {
      m_pDS->exec("alter table settings add NonLinStretch bool");
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

int CVideoDatabase::GetPlayCount(const CFileItem &item)
{
  int id = GetFileId(item);
  if (id < 0)
    return -1;  // not in db

  try
  {
    // error!
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    CStdString strSQL = FormatSQL("select playCount from files WHERE idFile=%i", id);
    int count = -1;
    if (m_pDS->query(strSQL.c_str()))
    {
      // there should only ever be one row returned
      if (m_pDS->num_rows() == 1)
        count = m_pDS->fv(0).get_asInt();
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
    exec = FormatSQL("UPDATE tvshow set c%02d='%s' WHERE idShow=%i", VIDEODB_ID_TV_FANART, item.GetVideoInfoTag()->m_fanart.m_xml.c_str(), item.GetVideoInfoTag()->m_iDbId);
  else if (type == VIDEODB_CONTENT_MOVIES)
    exec = FormatSQL("UPDATE movie set c%02d='%s' WHERE idMovie=%i", VIDEODB_ID_FANART, item.GetVideoInfoTag()->m_fanart.m_xml.c_str(), item.GetVideoInfoTag()->m_iDbId);

  try
  {
    m_pDS->exec(exec.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s - error updating fanart for %s", __FUNCTION__, item.m_strPath.c_str());
  }
}

void CVideoDatabase::SetPlayCount(const CFileItem &item, int count, const CStdString &date)
{
  int id = GetFileId(item);
  if (id < 0)
    return;  // not in db

  // and mark as watched
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    CStdString strSQL;
    if (count)
    {
      if (date.IsEmpty())
        strSQL = FormatSQL("update files set playCount=%i,lastPlayed=CURRENT_TIMESTAMP where idFile=%i", count, id);
      else
        strSQL = FormatSQL("update files set playCount=%i,lastPlayed='%s' where idFile=%i", count, date.c_str(), id);
    }
    else
      strSQL = FormatSQL("update files set playCount=NULL,lastPlayed=NULL where idFile=%i", id);

    m_pDS->exec(strSQL.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
}

void CVideoDatabase::IncrementPlayCount(const CFileItem &item)
{
  SetPlayCount(item, GetPlayCount(item) + 1);
}

void CVideoDatabase::UpdateMovieTitle(int idMovie, const CStdString& strNewMovieTitle, VIDEODB_CONTENT_TYPE iType)
{
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;
    CStdString strSQL;
    if (iType == VIDEODB_CONTENT_MOVIES)
    {
      CLog::Log(LOGINFO, "Changing Movie:id:%i New Title:%s", idMovie, strNewMovieTitle.c_str());
      strSQL = FormatSQL("UPDATE movie SET c%02d='%s' WHERE idMovie=%i", VIDEODB_ID_TITLE, strNewMovieTitle.c_str(), idMovie );
    }
    else if (iType == VIDEODB_CONTENT_EPISODES)
    {
      CLog::Log(LOGINFO, "Changing Episode:id:%i New Title:%s", idMovie, strNewMovieTitle.c_str());
      strSQL = FormatSQL("UPDATE episode SET c%02d='%s' WHERE idEpisode=%i", VIDEODB_ID_EPISODE_TITLE, strNewMovieTitle.c_str(), idMovie );
    }
    else if (iType == VIDEODB_CONTENT_TVSHOWS)
    {
      CLog::Log(LOGINFO, "Changing TvShow:id:%i New Title:%s", idMovie, strNewMovieTitle.c_str());
      strSQL = FormatSQL("UPDATE tvshow SET c%02d='%s' WHERE idShow=%i", VIDEODB_ID_TV_TITLE, strNewMovieTitle.c_str(), idMovie );
    }
    else if (iType == VIDEODB_CONTENT_MUSICVIDEOS)
    {
      CLog::Log(LOGINFO, "Changing MusicVideo:id:%i New Title:%s", idMovie, strNewMovieTitle.c_str());
      strSQL = FormatSQL("UPDATE musicvideo SET c%02d='%s' WHERE idMVideo=%i", VIDEODB_ID_MUSICVIDEO_TITLE, strNewMovieTitle.c_str(), idMovie );
    }
    else if (iType == VIDEODB_CONTENT_MOVIE_SETS)
    {
      CLog::Log(LOGINFO, "Changing Movie set:id:%i New Title:%s", idMovie, strNewMovieTitle.c_str());
      strSQL = FormatSQL("UPDATE sets SET strSet='%s' WHERE idSet=%i", strNewMovieTitle.c_str(), idMovie );
    }
    m_pDS->exec(strSQL.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (int idMovie, const CStdString& strNewMovieTitle) failed on MovieID:%i and Title:%s", __FUNCTION__, idMovie, strNewMovieTitle.c_str());
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

bool CVideoDatabase::GetGenresNav(const CStdString& strBaseDir, CFileItemList& items, int idContent)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    // get primary genres for movies
    CStdString strSQL;
    if (g_settings.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
    {
      if (idContent == VIDEODB_CONTENT_MOVIES)
        strSQL=FormatSQL("select genre.idgenre,genre.strgenre,path.strPath,files.playCount from genre join genrelinkmovie on genre.idGenre=genrelinkmovie.idGenre join movie on genrelinkmovie.idMovie = movie.idMovie join files on files.idFile=movie.idFile join path on path.idPath = files.idPath");
      else if (idContent == VIDEODB_CONTENT_TVSHOWS)
        strSQL=FormatSQL("select genre.idgenre,genre.strgenre,path.strPath from genre join genrelinktvshow on genre.idGenre=genrelinkTvShow.idGenre join tvshow on genrelinkTvShow.idShow = tvshow.idShow join tvshowlinkpath on tvshowlinkpath.idShow=tvshow.idShow join files on files.idPath=tvshowlinkpath.idPath join path on path.idPath = files.idPath");
      else if (idContent == VIDEODB_CONTENT_MUSICVIDEOS)
        strSQL=FormatSQL("select genre.idgenre,genre.strgenre,path.strPath,files.playCount from genre join genrelinkmusicvideo on genre.idGenre=genrelinkmusicvideo.idGenre join musicvideo on genrelinkmusicvideo.idMVideo = musicvideo.idMVideo join files on musicvideo.idFile=files.idFile join path on path.idPath = files.idPath");
    }
    else
    {
      CStdString group;
      if (idContent == VIDEODB_CONTENT_MOVIES)
      {
        strSQL=FormatSQL("select genre.idgenre,genre.strgenre,count(1),count(files.playCount) from genre join genrelinkmovie on genre.idGenre=genrelinkmovie.idGenre join movie on genrelinkmovie.idMovie=movie.idMovie join files on files.idFile=movie.idFile ");
        group = " group by genre.idgenre";
      }
      else if (idContent == VIDEODB_CONTENT_TVSHOWS)
        strSQL=FormatSQL("select distinct genre.idGenre,genre.strGenre from genre join genrelinktvshow on genre.idGenre=genrelinkTvShow.idGenre join tvshow on genrelinkTvShow.idShow=tvshow.idShow");
      else if (idContent == VIDEODB_CONTENT_MUSICVIDEOS)
      {
        strSQL=FormatSQL("select genre.idGenre,genre.strGenre,count(1),count(files.playCount) from genre join genrelinkmusicvideo on genre.idGenre=genrelinkmusicvideo.idGenre join musicvideo on genrelinkmusicvideo.idMVideo=musicvideo.idMVideo join files on files.idFile=musicvideo.idFile ");
        group = " group by genre.idGenre";
      }
      strSQL += group;
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

    if (g_settings.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
    {
      map<int, pair<CStdString,int> > mapGenres;
      map<int, pair<CStdString,int> >::iterator it;
      while (!m_pDS->eof())
      {
        int idGenre = m_pDS->fv("genre.idGenre").get_asInt();
        CStdString strGenre = m_pDS->fv("genre.strGenre").get_asString();
        it = mapGenres.find(idGenre);
        // was this genre already found?
        if (it == mapGenres.end())
        {
          // check path
          if (g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv("path.strPath").get_asString()),g_settings.m_videoSources))
          {
            if (idContent == VIDEODB_CONTENT_MOVIES || idContent == VIDEODB_CONTENT_MUSICVIDEOS)
              mapGenres.insert(pair<int, pair<CStdString,int> >(idGenre, pair<CStdString,int>(strGenre,m_pDS->fv(3).get_asInt())));
            else
              mapGenres.insert(pair<int, pair<CStdString,int> >(idGenre, pair<CStdString,int>(strGenre,0)));
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
        CFileItemPtr pItem(new CFileItem(m_pDS->fv("genre.strGenre").get_asString()));
        CStdString strDir;
        strDir.Format("%ld/", m_pDS->fv("genre.idGenre").get_asInt());
        pItem->m_strPath=strBaseDir + strDir;
        pItem->m_bIsFolder=true;
        pItem->SetLabelPreformated(true);
        if (idContent == VIDEODB_CONTENT_MOVIES || idContent==VIDEODB_CONTENT_MUSICVIDEOS)
        {
          // fv(3) is the number of videos watched, fv(2) is the total number.  We set the playcount
          // only if the number of videos watched is equal to the total number (i.e. every video watched)
          pItem->GetVideoInfoTag()->m_playCount = (m_pDS->fv(3).get_asInt() == m_pDS->fv(2).get_asInt()) ? 1 : 0;
        }
        items.Add(pItem);
        m_pDS->next();
      }
      m_pDS->close();
    }

//    CLog::Log(LOGDEBUG, "%s Time: %d ms", CTimeUtils::GetTimeMS() - time);
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}

bool CVideoDatabase::GetStudiosNav(const CStdString& strBaseDir, CFileItemList& items, int idContent)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL;
    if (g_settings.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
    {
      if (idContent == VIDEODB_CONTENT_MOVIES)
        strSQL=FormatSQL("select studio.idStudio,studio.strStudio,path.strPath,files.playCount from studio join studiolinkmovie on studio.idStudio=studiolinkmovie.idStudio join movie on studiolinkmovie.idMovie = movie.idMovie join files on files.idFile=movie.idFile join path on path.idPath=files.idPath");
      else if (idContent == VIDEODB_CONTENT_MUSICVIDEOS)
        strSQL=FormatSQL("select studio.idStudio,studio.strStudio,path.strPath,files.playCount from studio join studiolinkmusicvideo studio.idStudio=studiolinkmusicvideo.idStudio join musicvideo on studiolinkmusicvideo.idMVideo=musicvideo.idMVideo join files on files.idFile=musicvideo.idFile join path on path.idPath=files.idPath");
      else if (idContent == VIDEODB_CONTENT_TVSHOWS)
        strSQL=FormatSQL("select studio.idStudio,studio.strStudio,path.strPath from studio join studiolinktvshow on studio.idStudio=studiolinktvshow.idStudio join tvshow on studiolinktvshow.idShow=tvshow.idShow join tvshowlinkepisode on tvshowlinkepisode.idShow=tvshow.idShow join episode on episode.idEpisode=tvshowlinkepisode.idEpisode join files on files.idFile=episode.idFile join path on path.idPath=files.idPath");
    }
    else
    {
      if (idContent == VIDEODB_CONTENT_MOVIES)
      {
        strSQL=FormatSQL("select studio.idStudio,studio.strStudio,count(1),count(files.playCount) from studio join studiolinkmovie on studio.idStudio=studiolinkmovie.idStudio join movie on studiolinkmovie.idMovie=movie.idMovie join files on files.idFile=movie.idFile");
        strSQL += " group by studio.idStudio";
      }
      else if (idContent == VIDEODB_CONTENT_MUSICVIDEOS)
      {
        strSQL=FormatSQL("select studio.idStudio,studio.strStudio,count(1),count(files.playCount) from studio join studiolinkmusicvideo on studio.idStudio=studiolinkmusicvideo.idStudio join musicvideo on studiolinkmusicvideo.idMVideo = musicvideo.idMVideo join files on files.idFile=musicvideo.idFile");
        strSQL += " group by studio.idStudio";
      }
      else if (idContent == VIDEODB_CONTENT_TVSHOWS)
        strSQL=FormatSQL("select distinct studio.idStudio, studio.strStudio from studio join studiolinktvshow on studio.idStudio=studiolinkTvShow.idStudio join tvshow on studiolinkTvShow.idShow=tvshow.idShow");
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

    if (g_settings.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
    {
      map<int, pair<CStdString,int> > mapStudios;
      map<int, pair<CStdString,int> >::iterator it;
      while (!m_pDS->eof())
      {
        int idStudio = m_pDS->fv("studio.idStudio").get_asInt();
        CStdString strStudio = m_pDS->fv("studio.strStudio").get_asString();
        it = mapStudios.find(idStudio);
        // was this genre already found?
        if (it == mapStudios.end())
        {
          // check path
          if (g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv("path.strPath").get_asString()),g_settings.m_videoSources))
          {
            if (idContent == VIDEODB_CONTENT_MOVIES || idContent == VIDEODB_CONTENT_MUSICVIDEOS)
              mapStudios.insert(pair<int, pair<CStdString,int> >(idStudio, pair<CStdString,int>(strStudio,m_pDS->fv(3).get_asInt())));
            else
              mapStudios.insert(pair<int, pair<CStdString,int> >(idStudio, pair<CStdString,int>(strStudio,0)));
          }
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
        CFileItemPtr pItem(new CFileItem(m_pDS->fv("studio.strStudio").get_asString()));
        CStdString strDir;
        strDir.Format("%ld/", m_pDS->fv("studio.idStudio").get_asInt());
        pItem->m_strPath=strBaseDir + strDir;
        pItem->m_bIsFolder=true;
        pItem->SetLabelPreformated(true);
        if (idContent == VIDEODB_CONTENT_MOVIES || idContent == VIDEODB_CONTENT_MUSICVIDEOS)
        {
          // fv(3) is the number of videos watched, fv(2) is the total number.  We set the playcount
          // only if the number of videos watched is equal to the total number (i.e. every video watched)
          pItem->GetVideoInfoTag()->m_playCount = (m_pDS->fv(3).get_asInt() == m_pDS->fv(2).get_asInt()) ? 1 : 0;
        }
        items.Add(pItem);
        m_pDS->next();
      }
      m_pDS->close();
    }

//    CLog::Log(LOGDEBUG, __FUNCTION__" Time: %d ms", CTimeUtils::GetTimeMS() - time);
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}

bool CVideoDatabase::GetSetsNav(const CStdString& strBaseDir, CFileItemList& items, int idContent, const CStdString &where)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    // get primary sets for movies
    CStdString strSQL;
    if (g_settings.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
    {
      if (idContent == VIDEODB_CONTENT_MOVIES)
        strSQL=FormatSQL("select sets.idSet,sets.strSet,path.strPath,files.playCount from sets join setlinkmovie on sets.idSet=setlinkmovie.idSet join movie on setlinkmovie.idMovie=movie.idMovie join files on files.idFile=movie.idFile join path on path.idPath=files.idPath");
      strSQL += where;
    }
    else
    {
      CStdString group;
      if (idContent == VIDEODB_CONTENT_MOVIES)
      {
        strSQL=FormatSQL("select sets.idSet,sets.strSet,count(1),count(files.playCount) from sets join setlinkmovie on sets.idSet=setlinkmovie.idSet join movie on setlinkmovie.idMovie=movie.idMovie join files on files.idFile=movie.idFile ");
        group = " group by sets.idSet";
      }
      strSQL += where;
      strSQL += group;
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

    if (g_settings.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
    {
      map<int, pair<CStdString,int> > mapSets;
      map<int, pair<CStdString,int> >::iterator it;
      while (!m_pDS->eof())
      {
        int idSet = m_pDS->fv("sets.idSet").get_asInt();
        CStdString strSet = m_pDS->fv("sets.strSet").get_asString();
        it = mapSets.find(idSet);
        // was this set already found?
        if (it == mapSets.end())
        {
          // check path
          if (g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv("path.strPath").get_asString()),g_settings.m_videoSources))
          {
            if (idContent == VIDEODB_CONTENT_MOVIES || idContent == VIDEODB_CONTENT_MUSICVIDEOS)
              mapSets.insert(pair<int, pair<CStdString,int> >(idSet, pair<CStdString,int>(strSet,m_pDS->fv(3).get_asInt())));
            else
              mapSets.insert(pair<int, pair<CStdString,int> >(idSet, pair<CStdString,int>(strSet,0)));
          }
        }
        m_pDS->next();
      }
      m_pDS->close();

      for (it=mapSets.begin();it != mapSets.end();++it)
      {
        CFileItemPtr pItem(new CFileItem(it->second.first));
        CStdString strDir;
        strDir.Format("%ld/", it->first);
        pItem->m_strPath=strBaseDir + strDir;
        pItem->m_bIsFolder=true;
        if (idContent == VIDEODB_CONTENT_MOVIES || idContent == VIDEODB_CONTENT_MUSICVIDEOS)
        {
          pItem->GetVideoInfoTag()->m_playCount = it->second.second;
          pItem->GetVideoInfoTag()->m_strTitle = pItem->GetLabel();
        }
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
        CFileItemPtr pItem(new CFileItem(m_pDS->fv("sets.strSet").get_asString()));
        CStdString strDir;
        strDir.Format("%ld/", m_pDS->fv("sets.idSet").get_asInt());
        pItem->m_strPath=strBaseDir + strDir;
        pItem->m_bIsFolder=true;
        pItem->SetLabelPreformated(true);
        bool thumb=false,fanart=false;
        if (CFile::Exists(pItem->GetCachedVideoThumb()))
        {
          pItem->SetThumbnailImage(pItem->GetCachedVideoThumb());
          thumb = true;
        }
        if (CFile::Exists(pItem->GetCachedFanart()))
        {
          pItem->SetProperty("fanart_image",pItem->GetCachedFanart());
          fanart = true;
        }
        if (!thumb || !fanart) // use the first item's thumb
        {
          CStdString strSQL = FormatSQL("select strPath, strFileName from movieview join setlinkmovie on setlinkmovie.idMovie=movieview.idMovie where setlinkmovie.idSet=%u", m_pDS->fv("sets.idSet").get_asInt());
          m_pDS2->query(strSQL.c_str());
          if (!m_pDS2->eof())
          {
            CStdString path;
            ConstructPath(path,m_pDS2->fv(0).get_asString(),m_pDS2->fv(1).get_asString());
            CFileItem item(path,false);
            if (!thumb && CFile::Exists(item.GetCachedVideoThumb()))
              pItem->SetThumbnailImage(item.GetCachedVideoThumb());
            if (!fanart && CFile::Exists(item.GetCachedFanart()))
              pItem->SetProperty("fanart_image",item.GetCachedFanart());
            m_pDS2->close();
          }
        }
        if (idContent == VIDEODB_CONTENT_MOVIES || idContent==VIDEODB_CONTENT_MUSICVIDEOS)
        {
          // fv(3) is the number of videos watched, fv(2) is the total number.  We set the playcount
          // only if the number of videos watched is equal to the total number (i.e. every video watched)
          pItem->GetVideoInfoTag()->m_playCount = (m_pDS->fv(3).get_asInt() == m_pDS->fv(2).get_asInt()) ? 1 : 0;
          pItem->GetVideoInfoTag()->m_strTitle = pItem->GetLabel();
        }
        items.Add(pItem);
        m_pDS->next();
      }
      m_pDS->close();
    }

//    CLog::Log(LOGDEBUG, "%s Time: %d ms", CTimeUtils::GetTimeMS() - time);
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}

bool CVideoDatabase::GetMusicVideoAlbumsNav(const CStdString& strBaseDir, CFileItemList& items, int idArtist)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL;
    if (g_settings.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
    {
      strSQL=FormatSQL("select musicvideo.c%02d,musicvideo.idMVideo,actors.strActor,path.strPath from musicvideo,path,files join artistlinkmusicvideo on artistlinkmusicvideo.idMVideo=musicvideo.idMVideo join actors on actors.idActor=artistlinkmusicvideo.idArtist join files on files.idFile=musicvideo.idFile join path on path.idPath=files.idPath",VIDEODB_ID_MUSICVIDEO_ALBUM);
    }
    else
    {
      strSQL=FormatSQL("select musicvideo.c%02d,musicvideo.idMVideo,actors.strActor from musicvideo join artistlinkmusicvideo on artistlinkmusicvideo.idMVideo=musicvideo.idMVideo join actors on actors.idActor=artistlinkmusicvideo.idArtist",VIDEODB_ID_MUSICVIDEO_ALBUM);
    }
    if (idArtist > -1)
      strSQL += FormatSQL(" where artistlinkmusicvideo.idArtist=%i",idArtist);

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

    if (g_settings.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
    {
      map<int, pair<CStdString,CStdString> > mapAlbums;
      map<int, pair<CStdString,CStdString> >::iterator it;
      while (!m_pDS->eof())
      {
        int lidMVideo = m_pDS->fv(1).get_asInt();
        CStdString strAlbum = m_pDS->fv(0).get_asString();
        it = mapAlbums.find(lidMVideo);
        // was this genre already found?
        if (it == mapAlbums.end())
        {
          // check path
          if (g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv("path.strPath").get_asString()),g_settings.m_videoSources))
            mapAlbums.insert(make_pair(lidMVideo, make_pair(strAlbum,m_pDS->fv(2).get_asString())));
        }
        m_pDS->next();
      }
      m_pDS->close();

      for (it=mapAlbums.begin();it != mapAlbums.end();++it)
      {
        if (!it->second.first.IsEmpty())
        {
          CFileItemPtr pItem(new CFileItem(it->second.first));
          CStdString strDir;
          strDir.Format("%ld/", it->first);
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
        }
      }
    }
    else
    {
      while (!m_pDS->eof())
      {
        if (!m_pDS->fv(0).get_asString().empty())
        {
          CFileItemPtr pItem(new CFileItem(m_pDS->fv(0).get_asString()));
          CStdString strDir;
          strDir.Format("%ld/", m_pDS->fv(1).get_asInt());
          pItem->m_strPath=strBaseDir + strDir;
          pItem->m_bIsFolder=true;
          pItem->SetLabelPreformated(true);
          if (!items.Contains(pItem->m_strPath))
          {
            pItem->GetVideoInfoTag()->m_strArtist = m_pDS->fv(2).get_asString();
            CStdString strThumb = CUtil::GetCachedAlbumThumb(pItem->GetLabel(),m_pDS->fv(2).get_asString());
            if (CFile::Exists(strThumb))
              pItem->SetThumbnailImage(strThumb);
            items.Add(pItem);
          }
        }
        m_pDS->next();
      }
      m_pDS->close();
    }

//    CLog::Log(LOGDEBUG, __FUNCTION__" Time: %d ms", CTimeUtils::GetTimeMS() - time);
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}

bool CVideoDatabase::GetWritersNav(const CStdString& strBaseDir, CFileItemList& items, int idContent)
{
  return GetPeopleNav(strBaseDir, items, "studio", idContent);
}

bool CVideoDatabase::GetDirectorsNav(const CStdString& strBaseDir, CFileItemList& items, int idContent)
{
  return GetPeopleNav(strBaseDir, items, "director", idContent);
}

bool CVideoDatabase::GetActorsNav(const CStdString& strBaseDir, CFileItemList& items, int idContent)
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
        pItem->SetIconImage("DefaultArtist.png");
      }
      else
      {
        if (CFile::Exists(pItem->GetCachedActorThumb()))
          pItem->SetThumbnailImage(pItem->GetCachedActorThumb());
        pItem->SetIconImage("DefaultActor.png");
      }
    }
    return true;
  }
  return false;
}

bool CVideoDatabase::GetPeopleNav(const CStdString& strBaseDir, CFileItemList& items, const CStdString &type, int idContent)
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
    if (g_settings.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
    {
      if (idContent == VIDEODB_CONTENT_MOVIES)
        strSQL=FormatSQL("select actors.idActor,actors.strActor,actors.strThumb,path.strPath,files.playCount from actors join %slinkmovie on actors.idActor=%slinkmovie.id%s join movie on %slinkmovie.idMovie=movie.idMovie join files on files.idFile=movie.idFile join path on path.idPath=files.idPath", type.c_str(), type.c_str(), type.c_str(), type.c_str());
      else if (idContent == VIDEODB_CONTENT_TVSHOWS)
        strSQL=FormatSQL("select actors.idActor,actors.strActor,actors.strThumb,path.strPath from actors join %slinktvshow on actors.idActor=%slinktvshow.id%s join tvshow on %slinktvshow.idShow = tvshow.idShow join tvshowlinkepisode on tvshowlinkepisode.idShow=tvshow.idShow join episode on episode.idEpisode=tvshowlinkepisode.idEpisode join files on files.idFile=episode.idFile join path on path.idPath = files.idPath", type.c_str(), type.c_str(), type.c_str(), type.c_str());
      else if (idContent == VIDEODB_CONTENT_EPISODES)
        strSQL=FormatSQL("select actors.idActor,actors.strActor,actors.strThumb,path.strPath,files.playCount from actors join %slinkepisode on actors.idActor=%slinkepisode.id%s join episode on %slinkepisode.idEpisode = episode.idEpisode join files on files.idFile=episode.idFile join path on path.idPath = files.idPath", type.c_str(), type.c_str(), type.c_str(), type.c_str());
      else if (idContent == VIDEODB_CONTENT_MUSICVIDEOS)
        strSQL=FormatSQL("select actors.idActor,actors.strActor,actors.strThumb,path.strPath,files.playCount from actors join %slinkmusicvideo on actors.idActor=%slinkmusicvideo.id%s join musicvideo on %slinkmusicvideo.idMVideo = musicvideo.idMVideo join files on files.idFile=musicvideo.idFile join path on path.idPath = files.idPath", type.c_str(), type.c_str(), type.c_str(), type.c_str());
    }
    else
    {
      if (idContent == VIDEODB_CONTENT_MOVIES)
      {
        strSQL=FormatSQL("select actors.idActor,actors.strActor,actors.strThumb,count(1),count(files.playCount) from actors join %slinkmovie on actors.idActor=%slinkmovie.id%s join movie on %slinkmovie.idMovie=movie.idMovie join files on files.idFile=movie.idFile", type.c_str(), type.c_str(), type.c_str(), type.c_str());
        strSQL += " group by actors.idActor";
      }
      else if (idContent == VIDEODB_CONTENT_TVSHOWS)
        strSQL=FormatSQL("select distinct actors.idActor,actors.strActor,actors.strThumb from actors,%slinktvshow,tvshow where actors.idActor=%slinktvshow.id%s and %slinktvshow.idShow=tvshow.idShow", type.c_str(), type.c_str(), type.c_str(), type.c_str());
      else if (idContent == VIDEODB_CONTENT_EPISODES)
        strSQL=FormatSQL("select actors.idActor,actors.strActor,actors.strThumb,count(1),count(files.playCount) from %slinkepisode,actors,episode,files where actors.idActor=%slinkepisode.id%s and %slinkepisode.idEpisode=episode.idEpisode and files.idFile=episode.idFile group by actors.idActor", type.c_str(), type.c_str(), type.c_str(), type.c_str());
      else if (idContent == VIDEODB_CONTENT_MUSICVIDEOS)
      {
        strSQL=FormatSQL("select actors.idActor,actors.strActor,actors.strThumb,count(1),count(files.playCount) from actors join %slinkmusicvideo on actors.idActor=%slinkmusicvideo.id%s join musicvideo on %slinkmusicvideo.idMVideo=musicvideo.idMVideo join files on files.idFile=musicvideo.idFile", type.c_str(), type.c_str(), type.c_str(), type.c_str());
        strSQL += " group by actors.idActor";
      }
    }

    // run query
    unsigned int time = CTimeUtils::GetTimeMS();
    if (!m_pDS->query(strSQL.c_str())) return false;
    CLog::Log(LOGDEBUG, "%s -  query took %i ms",
              __FUNCTION__, CTimeUtils::GetTimeMS() - time); time = CTimeUtils::GetTimeMS();
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return true;
    }

    if (g_settings.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
    {
      map<int, CActor> mapActors;
      map<int, CActor>::iterator it;

      while (!m_pDS->eof())
      {
        int idActor = m_pDS->fv(0).get_asInt();
        CActor actor;
        actor.name = m_pDS->fv(1).get_asString();
        actor.thumb = m_pDS->fv(2).get_asString();
        if (idContent != VIDEODB_CONTENT_TVSHOWS)
          actor.playcount = m_pDS->fv(3).get_asInt();
        it = mapActors.find(idActor);
        // is this actor already known?
        if (it == mapActors.end())
        {
          // check path
          if (g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv("path.strPath").get_asString()),g_settings.m_videoSources))
            mapActors.insert(pair<int, CActor>(idActor, actor));
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
          strDir.Format("%ld/", m_pDS->fv(0).get_asInt());
          pItem->m_strPath=strBaseDir + strDir;
          pItem->m_bIsFolder=true;
          pItem->GetVideoInfoTag()->m_strPictureURL.ParseString(m_pDS->fv(2).get_asString());
          if (idContent != VIDEODB_CONTENT_TVSHOWS)
          {
            // fv(4) is the number of videos watched, fv(3) is the total number.  We set the playcount
            // only if the number of videos watched is equal to the total number (i.e. every video watched)
            pItem->GetVideoInfoTag()->m_playCount = (m_pDS->fv(4).get_asInt() == m_pDS->fv(3).get_asInt()) ? 1 : 0;
          }
          if (idContent == VIDEODB_CONTENT_MUSICVIDEOS)
            pItem->GetVideoInfoTag()->m_strArtist = pItem->GetLabel();
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
    CLog::Log(LOGDEBUG, "%s item retrieval took %i ms",
              __FUNCTION__, CTimeUtils::GetTimeMS() - time); time = CTimeUtils::GetTimeMS();

    return true;
  }
  catch (...)
  {
    m_pDS->close();
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}



bool CVideoDatabase::GetYearsNav(const CStdString& strBaseDir, CFileItemList& items, int idContent)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL;
    if (g_settings.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
    {
      if (idContent == VIDEODB_CONTENT_MOVIES)
        strSQL = FormatSQL("select movie.c%02d,path.strPath,files.playCount from movie join files on files.idFile=movie.idFile join path on files.idPath = path.idPath", VIDEODB_ID_YEAR);
      else if (idContent == VIDEODB_CONTENT_TVSHOWS)
        strSQL = FormatSQL("select tvshow.c%02d,path.strPath from tvshow join files on files.idFile=episode.idFile join episode on episode.idEpisode=tvshowlinkepisode.idEpisode join tvshowlinkepisode on tvshow.idShow = tvshowlinkepisode.idShow join path on files.idPath = path.idPath", VIDEODB_ID_TV_PREMIERED);
      else if (idContent == VIDEODB_CONTENT_MUSICVIDEOS)
        strSQL = FormatSQL("select musicvideo.c%02d,path.strPath,files.playCount from musicvideo join files on files.idFile=musicvideo.idFile join path on files.idPath = path.idPath", VIDEODB_ID_MUSICVIDEO_YEAR);
    }
    else
    {
      CStdString group;
      if (idContent == VIDEODB_CONTENT_MOVIES)
      {
        strSQL = FormatSQL("select movie.c%02d,count(1),count(files.playCount) from movie join files on files.idFile=movie.idFile", VIDEODB_ID_YEAR);
        group = FormatSQL(" group by movie.c%02d", VIDEODB_ID_YEAR);
      }
      else if (idContent == VIDEODB_CONTENT_TVSHOWS)
        strSQL = FormatSQL("select distinct tvshow.c%02d from tvshow", VIDEODB_ID_TV_PREMIERED);
      else if (idContent == VIDEODB_CONTENT_MUSICVIDEOS)
      {
        strSQL = FormatSQL("select musicvideo.c%02d,count(1),count(files.playCount) from musicvideo join files on files.idFile=musicvideo.idFile", VIDEODB_ID_MUSICVIDEO_YEAR);
        group = FormatSQL(" group by musicvideo.c%02d", VIDEODB_ID_MUSICVIDEO_YEAR);
      }
      strSQL += group;
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

    if (g_settings.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
    {
      map<int, pair<CStdString,int> > mapYears;
      map<int, pair<CStdString,int> >::iterator it;
      while (!m_pDS->eof())
      {
        int lYear = 0;
        if (idContent == VIDEODB_CONTENT_TVSHOWS)
        {
          CDateTime time;
          time.SetFromDateString(m_pDS->fv(0).get_asString());
          lYear = time.GetYear();
        }
        else if (idContent == VIDEODB_CONTENT_MOVIES || idContent == VIDEODB_CONTENT_MUSICVIDEOS)
          lYear = m_pDS->fv(0).get_asInt();
        it = mapYears.find(lYear);
        if (it == mapYears.end())
        {
          // check path
          if (g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv("path.strPath").get_asString()),g_settings.m_videoSources))
          {
            CStdString year;
            year.Format("%d", lYear);
            if (idContent == VIDEODB_CONTENT_MOVIES || idContent == VIDEODB_CONTENT_MUSICVIDEOS)
              mapYears.insert(pair<int, pair<CStdString,int> >(lYear, pair<CStdString,int>(year,m_pDS->fv(2).get_asInt())));
            else
              mapYears.insert(pair<int, pair<CStdString,int> >(lYear, pair<CStdString,int>(year,0)));
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
        int lYear = 0;
        CStdString strLabel;
        if (idContent == VIDEODB_CONTENT_TVSHOWS)
        {
          CDateTime time;
          time.SetFromDateString(m_pDS->fv(0).get_asString());
          lYear = time.GetYear();
          strLabel.Format("%i",lYear);
        }
        else if (idContent == VIDEODB_CONTENT_MOVIES || idContent == VIDEODB_CONTENT_MUSICVIDEOS)
        {
          lYear = m_pDS->fv(0).get_asInt();
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
          pItem->GetVideoInfoTag()->m_playCount = (m_pDS->fv(2).get_asInt() == m_pDS->fv(1).get_asInt()) ? 1 : 0;
        }

        // take care of dupes ..
        if (!items.Contains(pItem->m_strPath))
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

bool CVideoDatabase::GetStackedTvShowList(int idShow, CStdString& strIn)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    // look for duplicate show titles and stack them into a list
    if (idShow == -1)
      return false;
    CStdString strSQL = FormatSQL("select idShow from tvshow where c00 like (select c00 from tvshow where idShow=%i) order by idShow", idShow);
    CLog::Log(LOGDEBUG, "%s query: %s", __FUNCTION__, strSQL.c_str());
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRows = m_pDS->num_rows();
    if (iRows == 0) return false; // this should never happen!
    if (iRows > 1)
    { // more than one show, so stack them up
      strIn = "IN (";
      while (!m_pDS->eof())
      {
        strIn += FormatSQL("%i,", m_pDS->fv(0).get_asInt());
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

bool CVideoDatabase::GetSeasonsNav(const CStdString& strBaseDir, CFileItemList& items, int idActor, int idDirector, int idGenre, int idYear, int idShow)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strIn = FormatSQL("= %i", idShow);
    GetStackedTvShowList(idShow, strIn);

    CStdString strSQL = FormatSQL("select episode.c%02d,path.strPath,tvshow.c%02d,tvshow.c%02d,count(1),count(files.playCount) from episode join tvshowlinkepisode on tvshowlinkepisode.idEpisode=episode.idEpisode join tvshow on tvshow.idShow=tvshowlinkepisode.idShow join files on files.idFile=episode.idFile ", VIDEODB_ID_EPISODE_SEASON, VIDEODB_ID_TV_TITLE, VIDEODB_ID_TV_GENRE);
    CStdString joins = FormatSQL(" join tvshowlinkpath on tvshowlinkpath.idShow = tvshow.idShow join path on path.idPath = tvshowlinkpath.idPath where tvshow.idShow %s ", strIn.c_str());
    CStdString extraJoins, extraWhere;
    if (idActor != -1)
    {
      extraJoins = "join actorlinktvshow on actorlinktvshow.idShow=tvshow.idShow";
      extraWhere = FormatSQL("and actorlinktvshow.idActor=%i", idActor);
    }
    else if (idDirector != -1)
    {
      extraJoins = "join directorlinktvshow on directorlinktvshow.idShow=tvshow.idShow";
      extraWhere = FormatSQL("and directorlinktvshow.idDirector=%i",idDirector);
    }
    else if (idGenre != -1)
    {
      extraJoins = "join genrelinktvshow on genrelinktvshow.idShow=tvshow.idShow";
      extraWhere = FormatSQL("and genrelinktvshow.idGenre=%i", idGenre);
    }
    else if (idYear != -1)
    {
      extraWhere = FormatSQL("and tvshow.c%02d like '%%%i%%'", VIDEODB_ID_TV_PREMIERED, idYear);
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

    if (g_settings.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
    {
      map<int, CSeason> mapSeasons;
      map<int, CSeason>::iterator it;
      while (!m_pDS->eof())
      {
        int iSeason = m_pDS->fv(0).get_asInt();
        it = mapSeasons.find(iSeason);
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
          season.genre = m_pDS->fv(3).get_asString();
          season.numEpisodes = m_pDS->fv(4).get_asInt();
          season.numWatched = m_pDS->fv(5).get_asInt();
          mapSeasons.insert(make_pair(iSeason, season));
        }
        m_pDS->next();
      }
      m_pDS->close();

      for (it=mapSeasons.begin();it != mapSeasons.end();++it)
      {
        int iSeason = it->first;
        CStdString strLabel;
        if (iSeason == 0)
          strLabel = g_localizeStrings.Get(20381);
        else
          strLabel.Format(g_localizeStrings.Get(20358),iSeason);
        CFileItemPtr pItem(new CFileItem(strLabel));
        CStdString strDir;
        strDir.Format("%ld/", it->first);
        pItem->m_strPath=strBaseDir + strDir;
        pItem->m_bIsFolder=true;
        pItem->GetVideoInfoTag()->m_strTitle = strLabel;
        pItem->GetVideoInfoTag()->m_iSeason = iSeason;
        pItem->GetVideoInfoTag()->m_iDbId = idShow;
        pItem->GetVideoInfoTag()->m_strPath = it->second.path;
        pItem->GetVideoInfoTag()->m_strGenre = it->second.genre;
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
        int iSeason = m_pDS->fv(0).get_asInt();
        CStdString strLabel;
        if (iSeason == 0)
          strLabel = g_localizeStrings.Get(20381);
        else
          strLabel.Format(g_localizeStrings.Get(20358),iSeason);
        CFileItemPtr pItem(new CFileItem(strLabel));
        CStdString strDir;
        strDir.Format("%ld/", iSeason);
        pItem->m_strPath=strBaseDir + strDir;
        pItem->m_bIsFolder=true;
        pItem->GetVideoInfoTag()->m_strTitle = strLabel;
        pItem->GetVideoInfoTag()->m_iSeason = iSeason;
        pItem->GetVideoInfoTag()->m_iDbId = idShow;
        pItem->GetVideoInfoTag()->m_strPath = m_pDS->fv(1).get_asString();
        pItem->GetVideoInfoTag()->m_strGenre = m_pDS->fv(3).get_asString();
        pItem->GetVideoInfoTag()->m_strShowTitle = showTitle;
        int totalEpisodes = m_pDS->fv(4).get_asInt();
        int watchedEpisodes = m_pDS->fv(5).get_asInt();
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
    GetMoviesByWhere("videodb://1/2/", where, "", items);
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}

bool CVideoDatabase::GetMoviesNav(const CStdString& strBaseDir, CFileItemList& items, int idGenre, int idYear, int idActor, int idDirector, int idStudio, int idSet)
{
  CStdString where;
  if (idGenre != -1)
    where = FormatSQL("join genrelinkmovie on genrelinkmovie.idMovie=movieview.idMovie where genrelinkmovie.idGenre=%i", idGenre);
  else if (idStudio != -1)
    where = FormatSQL("join studiolinkmovie on studiolinkmovie.idMovie=movieview.idMovie where studiolinkmovie.idStudio=%i", idStudio);
  else if (idDirector != -1)
    where = FormatSQL("join directorlinkmovie on directorlinkmovie.idMovie=movieview.idMovie where directorlinkmovie.idDirector=%i", idDirector);
  else if (idYear !=-1)
    where = FormatSQL("where c%02d='%i'",VIDEODB_ID_YEAR,idYear);
  else if (idActor != -1)
    where = FormatSQL("join actorlinkmovie on actorlinkmovie.idMovie=movieview.idMovie join actors on actors.idActor=actorlinkmovie.idActor where actors.idActor=%i",idActor);
  else if (idSet != -1)
    where = FormatSQL("join setlinkmovie on setlinkmovie.idMovie=movieview.idMovie where setlinkmovie.idSet=%u",idSet);

  return GetMoviesByWhere(strBaseDir, where, "", items, idSet == -1);
}

bool CVideoDatabase::GetMoviesByWhere(const CStdString& strBaseDir, const CStdString &where, const CStdString &order, CFileItemList& items, bool fetchSets)
{
  try
  {
    DWORD time = CTimeUtils::GetTimeMS();
    movieTime = 0;
    castTime = 0;

    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL = "select * from movieview ";
    if (fetchSets)
    {
      // not getting a set, so grab all sets that match this where clause first
      CStdString setsWhere;
      if (where.size())
        setsWhere = " where movie.idMovie in (select movieview.idMovie from movieview " + where + ")";
      GetSetsNav("videodb://1/7/", items, VIDEODB_CONTENT_MOVIES, setsWhere);
      if (where.size())
        strSQL += where + FormatSQL(" and movieview.idMovie NOT in (select idMovie from setlinkmovie)");
      else
        strSQL += FormatSQL("where movieview.idMovie NOT in (select idMovie from setlinkmovie)");
    }
    else
      strSQL += where;

    if (order.size())
      strSQL += " " + order;

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
              CTimeUtils::GetTimeMS() - time); time = CTimeUtils::GetTimeMS();

    // get data from returned rows
    items.Reserve(iRowsFound);
    while (!m_pDS->eof())
    {
      CVideoInfoTag movie = GetDetailsForMovie(m_pDS);
      if (g_settings.GetMasterProfile().getLockMode() == LOCK_MODE_EVERYONE ||
          g_passwordManager.bMasterUser                                   ||
          g_passwordManager.IsDatabasePathUnlocked(movie.m_strPath, g_settings.m_videoSources))
      {
        CFileItemPtr pItem(new CFileItem(movie));
        pItem->m_strPath.Format("%s%ld", strBaseDir.c_str(), movie.m_iDbId);
        pItem->SetOverlayImage(CGUIListItem::ICON_OVERLAY_UNWATCHED,movie.m_playCount > 0);
        items.Add(pItem);
      }
      m_pDS->next();
    }

    CLog::Log(LOGDEBUG,"Time to retrieve movies from dataset = %d",
              CTimeUtils::GetTimeMS() - time);

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

bool CVideoDatabase::GetTvShowsNav(const CStdString& strBaseDir, CFileItemList& items, int idGenre, int idYear, int idActor, int idDirector, int idStudio)
{
  CStdString where;
  if (idGenre != -1)
    where = FormatSQL("join genrelinktvshow on genrelinktvshow.idShow=tvshow.idShow where genrelinktvshow.idGenre=%i ", idGenre);
  else if (idStudio != -1)
    where = FormatSQL("join studiolinktvshow on studiolinktvshow.idShow=tvshow.idShow where studiolinktvshow.idStudio=%i", idStudio);
  else if (idDirector != -1)
    where = FormatSQL("join directorlinktvshow on directorlinktvshow.idShow=tvshow.idShow where directorlinktvshow.idDirector=%i", idDirector);
  else if (idYear != -1)
    where = FormatSQL("where c%02d like '%%%i%%'", VIDEODB_ID_TV_PREMIERED,idYear);
  else if (idActor != -1)
    where = FormatSQL("join actorlinktvshow on actorlinktvshow.idShow=tvshow.idShow join actors on actors.idActor=actorlinktvshow.idActor where actors.idActor=%i",idActor);

  return GetTvShowsByWhere(strBaseDir, where, items);
}

bool CVideoDatabase::GetTvShowsByWhere(const CStdString& strBaseDir, const CStdString &where, CFileItemList& items)
{
  try
  {
    DWORD time = CTimeUtils::GetTimeMS();
    movieTime = 0;

    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL = VIDEO_DATABASE_VIEW_TVSHOW + where;
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
              CTimeUtils::GetTimeMS() - time); time = CTimeUtils::GetTimeMS();

    // get data from returned rows
    items.Reserve(iRowsFound);

    while (!m_pDS->eof())
    {
      int idShow = m_pDS->fv("tvshow.idShow").get_asInt();

      CVideoInfoTag movie = GetDetailsForTvShow(m_pDS, false);
      if (!g_advancedSettings.m_bVideoLibraryHideEmptySeries || movie.m_iEpisode > 0)
      {
        CFileItemPtr pItem(new CFileItem(movie));
        pItem->m_strPath.Format("%s%ld/", strBaseDir.c_str(), idShow);
        pItem->m_dateTime.SetFromDateString(movie.m_strPremiered);
        pItem->GetVideoInfoTag()->m_iYear = pItem->m_dateTime.GetYear();
        pItem->SetProperty("watchedepisodes", movie.m_playCount);
        pItem->SetProperty("unwatchedepisodes", movie.m_iEpisode - movie.m_playCount);
        pItem->GetVideoInfoTag()->m_playCount = (movie.m_iEpisode == movie.m_playCount) ? 1 : 0;
        pItem->SetOverlayImage(CGUIListItem::ICON_OVERLAY_UNWATCHED, (pItem->GetVideoInfoTag()->m_playCount > 0) && (pItem->GetVideoInfoTag()->m_iEpisode > 0));
        items.Add(pItem);
      }
      m_pDS->next();
    }

    CLog::Log(LOGDEBUG,"Time to retrieve tvshows from dataset = %d",
              CTimeUtils::GetTimeMS() - time);

    CStdString order(where);
    bool maintainOrder = (size_t)order.ToLower().Find("order by") != CStdString::npos;
    Stack(items, VIDEODB_CONTENT_TVSHOWS, maintainOrder);

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
            if (jItem->GetVideoInfoTag()->m_strPremiered != 
                pItem->GetVideoInfoTag()->m_strPremiered)
            {
              j++;
              continue;
            }
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
          pItem->SetOverlayImage(CGUIListItem::ICON_OVERLAY_UNWATCHED, (pItem->GetVideoInfoTag()->m_playCount > 0) && (pItem->GetVideoInfoTag()->m_iEpisode > 0));
          if (!strFanArt.IsEmpty())
            pItem->SetProperty("fanart_image", strFanArt);
        }
        // increment i to j which is the next item
        i = j;
      }
    }
    break;
    // We currently don't stack episodes (No call here in GetEpisodesByWhere()), but this code is left
    // so that if we eventually want to stack during scan we can utilize it.
    /*
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

                // episodes dont have fanart yet
                //if (strFanArt.IsEmpty())
                //  strFanArt = jItem->GetProperty("fanart_image");

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

          // episodes dont have fanart yet
          //if (!strFanArt.IsEmpty())
          //  pItem->SetProperty("fanart_image", strFanArt);
        }
        // increment i to j which is the next item
        i = j;
      }
    }
    break;*/

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

bool CVideoDatabase::GetEpisodesNav(const CStdString& strBaseDir, CFileItemList& items, int idGenre, int idYear, int idActor, int idDirector, int idShow, int idSeason)
{
  CStdString strIn = FormatSQL("= %i", idShow);
  GetStackedTvShowList(idShow, strIn);
  CStdString where = FormatSQL("where idShow %s",strIn.c_str());
  if (idGenre != -1)
    where = FormatSQL("join genrelinktvshow on genrelinktvshow.idShow=episodeview.idShow where episodeview.idShow=%i and genrelinktvshow.idGenre=%i",idShow,idGenre);
  else if (idDirector != -1)
    where = FormatSQL("join directorlinktvshow on directorlinktvshow.idShow=episodeview.idShow where episodeview.idShow=%i and directorlinktvshow.idDirector=%i",idShow,idDirector);
  else if (idYear !=-1)
    where=FormatSQL("where idShow=%i and premiered like '%%%i%%'",idShow,idYear);
  else if (idActor != -1)
    where = FormatSQL("join actorlinktvshow on actorlinktvshow.idShow=episodeview.idShow where episodeview.idShow=%i and actorlinktvshow.idActor=%i",idShow,idActor);

  if (idSeason != -1)
  {
    if (idSeason != 0) // season = 0 indicates a special - we grab all specials here (see below)
      where += FormatSQL(" and (c%02d=%i or (c%02d=0 and (c%02d=0 or c%02d=%i)))",VIDEODB_ID_EPISODE_SEASON,idSeason,VIDEODB_ID_EPISODE_SEASON, VIDEODB_ID_EPISODE_SORTSEASON, VIDEODB_ID_EPISODE_SORTSEASON,idSeason);
    else
      where += FormatSQL(" and c%02d=%i",VIDEODB_ID_EPISODE_SEASON,idSeason);
  }

  // we always append show, season + episode in GetEpisodesByWhere
  CStdString parent, grandParent;
  CUtil::GetParentPath(strBaseDir,parent);
  CUtil::GetParentPath(parent,grandParent);

  bool ret = GetEpisodesByWhere(grandParent, where, items);

  if (idSeason == -1)
  { // add any linked movies
    CStdString where = FormatSQL("join movielinktvshow on movielinktvshow.idMovie=movieview.idMovie where movielinktvshow.idShow %s", strIn.c_str());
    GetMoviesByWhere("videodb://1/2/", where, "", items);
  }
  return ret;
}

bool CVideoDatabase::GetEpisodesByWhere(const CStdString& strBaseDir, const CStdString &where, CFileItemList& items, bool appendFullShowPath /* = true */)
{
  try
  {
    DWORD time = CTimeUtils::GetTimeMS();
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
              CTimeUtils::GetTimeMS() - time); time = CTimeUtils::GetTimeMS();

    // get data from returned rows
    items.Reserve(iRowsFound);
    while (!m_pDS->eof())
    {
      int idEpisode = m_pDS->fv("idEpisode").get_asInt();
      int idShow = m_pDS->fv("idShow").get_asInt();

      CVideoInfoTag movie = GetDetailsForEpisode(m_pDS);
      CFileItemPtr pItem(new CFileItem(movie));
      if (appendFullShowPath)
        pItem->m_strPath.Format("%s%ld/%ld/%ld",strBaseDir.c_str(), idShow, movie.m_iSeason,idEpisode);
      else
        pItem->m_strPath.Format("%s%ld",strBaseDir.c_str(), idEpisode);

      pItem->SetOverlayImage(CGUIListItem::ICON_OVERLAY_UNWATCHED,movie.m_playCount > 0);
      pItem->m_dateTime.SetFromDateString(movie.m_strFirstAired);
      pItem->GetVideoInfoTag()->m_iYear = pItem->m_dateTime.GetYear();
      items.Add(pItem);

      m_pDS->next();
    }

    CLog::Log(LOGDEBUG,"Time to retrieve episodes from dataset = %d",
              CTimeUtils::GetTimeMS() - time);

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


bool CVideoDatabase::GetMusicVideosNav(const CStdString& strBaseDir, CFileItemList& items, int idGenre, int idYear, int idArtist, int idDirector, int idStudio, int idAlbum)
{
  CStdString where;
  if (idGenre != -1)
    where = FormatSQL("join genrelinkmusicvideo on genrelinkmusicvideo.idMVideo=musicvideoview.idMVideo where genrelinkmusicvideo.idGenre=%i", idGenre);
  else if (idStudio != -1)
    where = FormatSQL("join studiolinkmusicvideo on studiolinkmusicvideo.idMVideo=musicvideoview.idMVideo where studiolinkmusicvideo.idAtudio=%i", idStudio);
  else if (idDirector != -1)
    where = FormatSQL("join directorlinkmusicvideo on directorlinkmusicvideo.idMVideo=musicvideoview.idMVideo where directorlinkmusicvideo.idDirector=%i", idDirector);
  else if (idYear !=-1)
    where = FormatSQL("where c%02d='%i'",VIDEODB_ID_MUSICVIDEO_YEAR,idYear);
  else if (idArtist != -1)
    where = FormatSQL("join artistlinkmusicvideo on artistlinkmusicvideo.idMVideo=musicvideoview.idMVideo join actors on actors.idActor=artistlinkmusicvideo.idArtist where actors.idActor=%i",idArtist);
  if (idAlbum != -1)
   {
    CStdString str2 = FormatSQL(" musicvideoview.c%02d=(select c%02d from musicvideo where idMVideo=%i)",VIDEODB_ID_MUSICVIDEO_ALBUM,VIDEODB_ID_MUSICVIDEO_ALBUM,idAlbum);
    if (where.IsEmpty())
      where.Format(" %s%s","where",str2.c_str());
    else
      where.Format(" %s %s%s",where.Mid(0).c_str(),"and",str2.c_str());
  }

  return GetMusicVideosByWhere(strBaseDir, where, items);
}

bool CVideoDatabase::GetRecentlyAddedMoviesNav(const CStdString& strBaseDir, CFileItemList& items)
{
  CStdString order = FormatSQL("order by idMovie desc limit %i", g_advancedSettings.m_iVideoLibraryRecentlyAddedItems);
  return GetMoviesByWhere(strBaseDir, "", order, items);
}

bool CVideoDatabase::GetRecentlyAddedEpisodesNav(const CStdString& strBaseDir, CFileItemList& items)
{
  CStdString where = FormatSQL("order by idEpisode desc limit %i", g_advancedSettings.m_iVideoLibraryRecentlyAddedItems);
  return GetEpisodesByWhere(strBaseDir, where, items, false);
}

bool CVideoDatabase::GetRecentlyAddedMusicVideosNav(const CStdString& strBaseDir, CFileItemList& items)
{
  CStdString where = FormatSQL("order by idMVideo desc limit %i", g_advancedSettings.m_iVideoLibraryRecentlyAddedItems);
  return GetMusicVideosByWhere(strBaseDir, where, items);
}

bool CVideoDatabase::GetGenreById(int idGenre, CStdString& strGenre)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL=FormatSQL("select genre.strGenre from genre where genre.idGenre=%i", idGenre);
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

bool CVideoDatabase::HasSets() const
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    m_pDS->query( "select idSet from sets" );
    bool bResult = (m_pDS->num_rows() > 0);
    m_pDS->close();
    return bResult;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}

bool CVideoDatabase::GetSetById(int idSet, CStdString& strSet)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL=FormatSQL("select sets.strSet from sets where sets.idSet=%i", idSet);
    m_pDS->query( strSQL.c_str() );

    bool bResult = false;
    if (!m_pDS->eof())
    {
      strSet  = m_pDS->fv("sets.strSet").get_asString();
      bResult = true;
    }
    m_pDS->close();
    return bResult;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strSet.c_str());
  }
  return false;
}

int CVideoDatabase::GetTvShowForEpisode(int idEpisode)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS2.get()) return false;

    // make sure we use m_pDS2, as this is called in loops using m_pDS
    CStdString strSQL=FormatSQL("select idShow from tvshowlinkepisode where idEpisode=%i", idEpisode);
    m_pDS2->query( strSQL.c_str() );

    int result=-1;
    if (!m_pDS2->eof())
      result=m_pDS2->fv(0).get_asInt();
    m_pDS2->close();

    return result;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%i) failed", __FUNCTION__, idEpisode);
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
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString sql;
    if (type == VIDEODB_CONTENT_MOVIES)
      sql = "select count(1) from movie";
    else if (type == VIDEODB_CONTENT_TVSHOWS)
      sql = "select count(1) from tvshow";
    else if (type == VIDEODB_CONTENT_MUSICVIDEOS)
      sql = "select count(1) from musicvideo";
    m_pDS->query( sql.c_str() );

    if (!m_pDS->eof())
      result = (m_pDS->fv(0).get_asInt() > 0);

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
      iResult = m_pDS->fv("nummovies").get_asInt();

    m_pDS->close();
    return iResult;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return 0;
}

bool CVideoDatabase::GetScraperForPath( const CStdString& strPath, ScraperPtr& scraper )
{
  int iDummy;
  return GetScraperForPath(strPath, scraper, iDummy);
}

bool CVideoDatabase::GetScraperForPath(const CStdString& strPath, ScraperPtr& scraper, int& iFound)
{
  SScanSettings settings;
  return GetScraperForPath(strPath, scraper, settings, iFound);
}

bool CVideoDatabase::GetScraperForPath(const CStdString& strPath, ScraperPtr& scraper, SScanSettings& settings)
{
  int iDummy;
  return GetScraperForPath(strPath, scraper, settings, iDummy);
}

bool CVideoDatabase::GetScraperForPath(const CStdString& strPath, ScraperPtr& scraper, SScanSettings& settings, int& iFound)
{
  try
  {
    if (strPath.IsEmpty()) return false;
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strPath1;
    CStdString strPath2(strPath);

    if (CUtil::IsMultiPath(strPath))
      strPath2 = CMultiPathDirectory::GetFirstPath(strPath);

    CUtil::GetDirectory(strPath2,strPath1);
    int idPath = GetPathId(strPath1);

    if (idPath > -1)
    {
      CStdString strSQL=FormatSQL("select path.strContent,path.strScraper,path.scanRecursive,path.useFolderNames,path.strSettings,path.noUpdate from path where path.idPath=%i",idPath);
      m_pDS->query( strSQL.c_str() );
    }

    iFound = 1;
    CONTENT_TYPE content = CONTENT_NONE;
    if (!m_pDS->eof())
    { // path is stored in db

      //!!!
      //FIXME confusion arises here as VIDEODB_CONTENT_NONE has no relation to strContent == 'none'.
      // Here, we are referring to paths which are explicitly excluded from scraping, by
      // having settings.exclude == true
      //!!
      CStdString strcontent = m_pDS->fv("path.strContent").get_asString();
      strcontent.ToLower();
      if (strcontent.Equals("none"))
      {
        settings.exclude = true;
        scraper.reset();
        m_pDS->close();
        return false;
      }

      // path is not excluded
      // try and ascertain scraper for this path
      content = TranslateContent(strcontent);

      //FIXME paths stored should not have empty strContent
      //assert(content != CONTENT_NONE);
      CStdString scraperID = m_pDS->fv("path.strScraper").get_asString();

      AddonPtr addon;
      if (!scraperID.empty() &&
        CAddonMgr::Get()->GetAddon(scraperID, addon, ADDON::ADDON_SCRAPER))
      {
        scraper = boost::dynamic_pointer_cast<CScraper>(addon->Clone(addon));
        if (!scraper)
          return false;

        // store this path's content & settings
        scraper->m_pathContent = content;
        scraper->LoadUserXML(m_pDS->fv("path.strSettings").get_asString());
        settings.parent_name = m_pDS->fv("path.useFolderNames").get_asBool();
        settings.recurse = m_pDS->fv("path.scanRecursive").get_asInt();
        settings.noupdate = m_pDS->fv("path.noUpdate").get_asBool();
      }
    }

    if (content == CONTENT_NONE)
    { // this path is not saved in db
      // we must drill up until a scraper is configured
      CStdString strParent;
      while (CUtil::GetParentPath(strPath1, strParent))
      {
        iFound++;

        CStdString strSQL=FormatSQL("select path.strContent,path.strScraper,path.scanRecursive,path.useFolderNames,path.strSettings,path.noUpdate from path where strPath like '%s'",strParent.c_str());
        m_pDS->query(strSQL.c_str());

        CONTENT_TYPE content = CONTENT_NONE;
        if (!m_pDS->eof())
        {

          CStdString strcontent = m_pDS->fv("path.strContent").get_asString();
          strcontent.ToLower();
          if (strcontent.Equals("none"))
          {
            settings.exclude = true;
            scraper.reset();
            m_pDS->close();
            break;
          }

          content = TranslateContent(strcontent);

          AddonPtr addon;
          if (content != CONTENT_NONE &&
              CAddonMgr::Get()->GetAddon(m_pDS->fv("path.strScraper").get_asString(), addon, ADDON::ADDON_SCRAPER))
          {
            scraper = boost::dynamic_pointer_cast<CScraper>(addon->Clone(addon));
            scraper->m_pathContent = content;
            scraper->LoadUserXML(m_pDS->fv("path.strSettings").get_asString());
            settings.parent_name = m_pDS->fv("path.useFolderNames").get_asBool();
            settings.recurse = m_pDS->fv("path.scanRecursive").get_asInt();
            settings.noupdate = m_pDS->fv("path.noUpdate").get_asBool();
            settings.exclude = false;
            break;
          }
        }
        strPath1 = strParent;
      }
    }
    m_pDS->close();

    if (!scraper || scraper->Content() == CONTENT_NONE)
      return false;

    if (scraper->Content() == CONTENT_TVSHOWS)
    {
      settings.recurse = 0;
      if(settings.parent_name) // single show
      {
        settings.parent_name_root = settings.parent_name = (iFound == 1);
      }
      else // show root
      {
        settings.parent_name_root = settings.parent_name = (iFound == 2);
      }
    }
    else if (scraper->Content() == CONTENT_MOVIES)
    {
      settings.recurse = settings.recurse - (iFound-1);
      settings.parent_name_root = settings.parent_name && (!settings.recurse || iFound > 1);
    }
    else if (scraper->Content() == CONTENT_MUSICVIDEOS)
    {
      settings.recurse = settings.recurse - (iFound-1);
      settings.parent_name_root = settings.parent_name && (!settings.recurse || iFound > 1);
    }
    else
    {
      iFound = 0;
      return false;
    }
    return true;
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

    if (g_settings.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL=FormatSQL("select genre.idGenre,genre.strGenre,path.strPath from genre,genrelinkmovie,movie,path,files where genre.idGenre=genrelinkmovie.idGenre and genrelinkmovie.idMovie=movie.idMovie and files.idFile=movie.idFile and path.idPath=files.idPath and genre.strGenre like '%%%s%%'",strSearch.c_str());
    else
      strSQL=FormatSQL("select distinct genre.idGenre,genre.strGenre from genre,genrelinkmovie where genrelinkmovie.idGenre=genre.idGenre and strGenre like '%%%s%%'", strSearch.c_str());
    m_pDS->query( strSQL.c_str() );

    while (!m_pDS->eof())
    {
      if (g_settings.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv("path.strPath").get_asString()),
                                                      g_settings.m_videoSources))
        {
          m_pDS->next();
          continue;
        }

      CFileItemPtr pItem(new CFileItem(m_pDS->fv("genre.strGenre").get_asString()));
      CStdString strDir;
      strDir.Format("%ld/", m_pDS->fv("genre.idGenre").get_asInt());
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

    if (g_settings.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL=FormatSQL("select genre.idGenre,genre.strGenre,path.strPath from genre,genrelinktvshow,tvshow,path,tvshowlinkpath where genre.idGenre=genrelinktvshow.idGenre and genrelinktvshow.idShow=tvshow.idShow and path.idPath=tvshowlinkpath.idPath and tvshowlinkpath.idShow=tvshow.idShow and genre.strGenre like '%%%s%%'",strSearch.c_str());
    else
      strSQL=FormatSQL("select distinct genre.idGenre,genre.strGenre from genre,genrelinktvshow where genrelinktvshow.idGenre=genre.idGenre and strGenre like '%%%s%%'", strSearch.c_str());
    m_pDS->query( strSQL.c_str() );

    while (!m_pDS->eof())
    {
      if (g_settings.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv("path.strPath").get_asString()),g_settings.m_videoSources))
        {
          m_pDS->next();
          continue;
        }

      CFileItemPtr pItem(new CFileItem(m_pDS->fv("genre.strGenre").get_asString()));
      CStdString strDir;
      strDir.Format("%ld/", m_pDS->fv("genre.idGenre").get_asInt());
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

    if (g_settings.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL=FormatSQL("select actors.idactor,actors.strActor,path.strPath from actorlinkmovie,actors,movie,files,path where actors.idActor=actorlinkmovie.idActor and actorlinkmovie.idMovie=movie.idMovie and files.idFile=movie.idFile and files.idPath=path.idPath and actors.strActor like '%%%s%%'",strSearch.c_str());
    else
      strSQL=FormatSQL("select actors.idactor,actors.strActor from actorlinkmovie,actors,movie where actors.idActor=actorlinkmovie.idActor and actorlinkmovie.idMovie=movie.idMovie and actors.strActor like '%%%s%%'",strSearch.c_str());
    m_pDS->query( strSQL.c_str() );

    while (!m_pDS->eof())
    {
      if (g_settings.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv("path.strPath").get_asString()),g_settings.m_videoSources))
        {
          m_pDS->next();
          continue;
        }

      CFileItemPtr pItem(new CFileItem(m_pDS->fv("actors.strActor").get_asString()));
      CStdString strDir;
      strDir.Format("%ld/", m_pDS->fv("actors.idActor").get_asInt());
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

    if (g_settings.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL=FormatSQL("select actors.idActor,actors.strActor,path.strPath from actorlinktvshow,actors,tvshow,path,tvshowlinkpath where actors.idActor=actorlinktvshow.idActor and actorlinktvshow.idShow=tvshow.idShow and tvshowlinkpath.idPath=tvshow.idShow and tvshowlinkpath.idPath=path.idPath and actors.strActor like '%%%s%%'",strSearch.c_str());
    else
      strSQL=FormatSQL("select actors.idactor,actors.strActor from actorlinktvshow,actors,tvshow where actors.idActor=actorlinktvshow.idActor and actorlinktvshow.idShow=tvshow.idShow and actors.strActor like '%%%s%%'",strSearch.c_str());
    m_pDS->query( strSQL.c_str() );

    while (!m_pDS->eof())
    {
      if (g_settings.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv("path.strPath").get_asString()),g_settings.m_videoSources))
        {
          m_pDS->next();
          continue;
        }

      CFileItemPtr pItem(new CFileItem(m_pDS->fv("actors.strActor").get_asString()));
      CStdString strDir;
      strDir.Format("%ld/", m_pDS->fv("actors.idActor").get_asInt());
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
    if (g_settings.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL=FormatSQL("select actors.idactor,actors.strActor,path.strPath from artistlinkmusicvideo,actors,musicvideo,files,path where actors.idActor=artistlinkmusicvideo.idArtist and artistlinkmusicvideo.idMVideo=musicvideo.idMVideo and files.idFile=musicvideo.idFile and files.idPath=path.idPath "+strLike,strSearch.c_str());
    else
      strSQL=FormatSQL("select distinct actors.idactor,actors.strActor from artistlinkmusicvideo,actors where actors.idActor=artistlinkmusicvideo.idArtist "+strLike,strSearch.c_str());
    m_pDS->query( strSQL.c_str() );

    while (!m_pDS->eof())
    {
      if (g_settings.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv("path.strPath").get_asString()),g_settings.m_videoSources))
        {
          m_pDS->next();
          continue;
        }

      CFileItemPtr pItem(new CFileItem(m_pDS->fv("actors.strActor").get_asString()));
      CStdString strDir;
      strDir.Format("%ld/", m_pDS->fv("actors.idActor").get_asInt());
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

    if (g_settings.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL=FormatSQL("select genre.idGenre,genre.strGenre,path.strPath from genre,genrelinkmusicvideo,musicvideo,path,files where genre.idGenre=genrelinkmusicvideo.idGenre and genrelinkmusicvideo.idMVideo = musicvideo.idMVideo and files.idFile=musicvideo.idFile and path.idPath=files.idPath and genre.strGenre like '%%%s%%'",strSearch.c_str());
    else
      strSQL=FormatSQL("select distinct genre.idGenre,genre.strGenre from genre,genrelinkmusicvideo where genrelinkmusicvideo.idGenre=genre.idGenre and genre.strGenre like '%%%s%%'", strSearch.c_str());
    m_pDS->query( strSQL.c_str() );

    while (!m_pDS->eof())
    {
      if (g_settings.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv("path.strPath").get_asString()),g_settings.m_videoSources))
        {
          m_pDS->next();
          continue;
        }

      CFileItemPtr pItem(new CFileItem(m_pDS->fv("genre.strGenre").get_asString()));
      CStdString strDir;
      strDir.Format("%ld/", m_pDS->fv("genre.idGenre").get_asInt());
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
    if (g_settings.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL=FormatSQL("select distinct musicvideo.c%02d,musicvideo.idMVideo,path.strPath from musicvideo,files,path where files.idFile=musicvideo.idFile and files.idPath=path.idPath"+strLike,VIDEODB_ID_MUSICVIDEO_ALBUM,strSearch.c_str());
    else
    {
      if (!strLike.IsEmpty())
        strLike = "where "+strLike.Mid(4);
      strSQL=FormatSQL("select distinct musicvideo.c%02d,musicvideo.idMVideo from musicvideo"+strLike,VIDEODB_ID_MUSICVIDEO_ALBUM,strSearch.c_str());
    }
    m_pDS->query( strSQL.c_str() );

    while (!m_pDS->eof())
    {
      if (m_pDS->fv(0).get_asString().empty())
      {
        m_pDS->next();
        continue;
      }

      if (g_settings.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv("path.strPath").get_asString()),g_settings.m_videoSources))
        {
          m_pDS->next();
          continue;
        }

      CFileItemPtr pItem(new CFileItem(m_pDS->fv(0).get_asString()));
      CStdString strDir;
      strDir.Format("%ld", m_pDS->fv(1).get_asInt());
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

    if (g_settings.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL = FormatSQL("select musicvideo.idMVideo,musicvideo.c%02d,musicvideo.c%02d,path.strPath from musicvideo,files,path where files.idFile=musicvideo.idFile and files.idPath=path.idPath and musicvideo.c%02d like '%%%s%%'",VIDEODB_ID_MUSICVIDEO_ALBUM,VIDEODB_ID_MUSICVIDEO_TITLE,VIDEODB_ID_MUSICVIDEO_ALBUM,strSearch.c_str());
    else
      strSQL = FormatSQL("select musicvideo.idmvideo,musicvideo.c%02d,musicvideo.c%02d from musicvideo where musicvideo.c%02d like '%%%s%%'",VIDEODB_ID_MUSICVIDEO_ALBUM,VIDEODB_ID_MUSICVIDEO_TITLE,VIDEODB_ID_MUSICVIDEO_ALBUM,strSearch.c_str());
    m_pDS->query( strSQL.c_str() );

    while (!m_pDS->eof())
    {
      if (g_settings.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv("path.strPath").get_asString()),g_settings.m_videoSources))
        {
          m_pDS->next();
          continue;
        }

      CFileItemPtr pItem(new CFileItem(m_pDS->fv(1).get_asString()+" - "+m_pDS->fv(2).get_asString()));
      CStdString strDir;
      strDir.Format("3/2/%ld",m_pDS->fv("musicvideo.idMVideo").get_asInt());

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
    DWORD time = CTimeUtils::GetTimeMS();
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
    CLog::Log(LOGDEBUG, "%s time for actual SQL query = %d", __FUNCTION__, CTimeUtils::GetTimeMS() - time); time = CTimeUtils::GetTimeMS();

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
      int idMVideo = m_pDS->fv("idMVideo").get_asInt();
      CVideoInfoTag musicvideo = GetDetailsForMusicVideo(m_pDS);
      if (!checkLocks || g_settings.GetMasterProfile().getLockMode() == LOCK_MODE_EVERYONE || g_passwordManager.bMasterUser ||
          g_passwordManager.IsDatabasePathUnlocked(musicvideo.m_strPath,g_settings.m_videoSources))
      {
        CFileItemPtr item(new CFileItem(musicvideo));
        item->m_strPath.Format("%s%ld",baseDir,idMVideo);
        item->SetOverlayImage(CGUIListItem::ICON_OVERLAY_UNWATCHED,musicvideo.m_playCount > 0);
        items.Add(item);
      }
      m_pDS->next();
    }

    CLog::Log(LOGDEBUG, "%s time to retrieve from dataset = %d", __FUNCTION__, CTimeUtils::GetTimeMS() - time); time = CTimeUtils::GetTimeMS();

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

unsigned int CVideoDatabase::GetMusicVideoIDs(const CStdString& strWhere, vector<pair<int,int> > &songIDs)
{
  try
  {
    if (NULL == m_pDB.get()) return 0;
    if (NULL == m_pDS.get()) return 0;

    CStdString strSQL = "select distinct idMVideo from musicvideoview " + strWhere;
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
      songIDs.push_back(make_pair<int,int>(2,m_pDS->fv(0).get_asInt()));
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

bool CVideoDatabase::GetRandomMusicVideo(CFileItem* item, int& idSong, const CStdString& strWhere)
{
  try
  {
    idSong = -1;

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
    idSong = m_pDS->fv("idmvideo").get_asInt();
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

int CVideoDatabase::GetMatchingMusicVideo(const CStdString& strArtist, const CStdString& strAlbum, const CStdString& strTitle)
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    CStdString strSQL;
    if (strAlbum.IsEmpty() && strTitle.IsEmpty())
    { // we want to return matching artists only
      if (g_settings.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        strSQL=FormatSQL("select distinct actors.idActor,path.strPath from artistlinkmusicvideo,actors,musicvideo,files,path where actors.idActor=artistlinkmusicvideo.idArtist and artistlinkmusicvideo.idMVideo=musicvideo.idMVideo and files.idFile=musicvideo.idFile and files.idPath=path.idPath and actors.strActor like '%s'",strArtist.c_str());
      else
        strSQL=FormatSQL("select distinct actors.idActor from artistlinkmusicvideo,actors where actors.idActor=artistlinkmusicvideo.idArtist and actors.strActor like '%s'",strArtist.c_str());
    }
    else
    { // we want to return the matching musicvideo
      if (g_settings.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        strSQL = FormatSQL("select musicvideo.idMVideo from musicvideo,files,path,artistlinkmusicvideo,actors where files.idFile=musicvideo.idFile and files.idPath=path.idPath and musicvideo.%c02d like '%s' and musicvideo.%c02d like '%s' and artistlinkmusicvideo.idMVideo=musicvideo.idMVideo and artistlinkmusicvideo.idArtist=actors.idActors and actors.strActor like '%s'",VIDEODB_ID_MUSICVIDEO_ALBUM,strAlbum.c_str(),VIDEODB_ID_MUSICVIDEO_TITLE,strTitle.c_str(),strArtist.c_str());
      else
        strSQL = FormatSQL("select musicvideo.idMVideo from musicvideo join artistlinkmusicvideo on artistlinkmusicvideo.idMVideo=musicvideo.idMVideo join actors on actors.idActor=artistlinkmusicvideo.idArtist where musicvideo.c%02d like '%s' and musicvideo.c%02d like '%s' and actors.strActor like '%s'",VIDEODB_ID_MUSICVIDEO_ALBUM,strAlbum.c_str(),VIDEODB_ID_MUSICVIDEO_TITLE,strTitle.c_str(),strArtist.c_str());
    }
    m_pDS->query( strSQL.c_str() );

    if (m_pDS->eof())
      return -1;

    if (g_settings.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      if (!g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv("path.strPath").get_asString()),g_settings.m_videoSources))
      {
        m_pDS->close();
        return -1;
      }

    int lResult = m_pDS->fv(0).get_asInt();
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

    if (g_settings.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL = FormatSQL("select movie.idMovie,movie.c%02d,path.strPath from movie,files,path where files.idFile=movie.idFile and files.idPath=path.idPath and movie.c%02d like '%%%s%%'",VIDEODB_ID_TITLE,VIDEODB_ID_TITLE,strSearch.c_str());
    else
      strSQL = FormatSQL("select movie.idMovie,movie.c%02d from movie where movie.c%02d like '%%%s%%'",VIDEODB_ID_TITLE,VIDEODB_ID_TITLE,strSearch.c_str());
    m_pDS->query( strSQL.c_str() );

    while (!m_pDS->eof())
    {
      if (g_settings.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv("path.strPath").get_asString()),g_settings.m_videoSources))
        {
          m_pDS->next();
          continue;
        }

      int movieId = m_pDS->fv("movie.idMovie").get_asInt();
      CStdString strSQL2 = FormatSQL("select idSet from setlinkmovie where idMovie=%i",movieId); 
      m_pDS2->query(strSQL2.c_str());
      CFileItemPtr pItem(new CFileItem(m_pDS->fv(1).get_asString()));
      if (m_pDS2->eof())
        pItem->m_strPath.Format("videodb://1/2/%i",movieId);
      else
        pItem->m_strPath.Format("videodb://1/7/%i/%i",m_pDS2->fv(0).get_asInt(),movieId);

      pItem->m_bIsFolder=false;
      items.Add(pItem);
      m_pDS2->close();
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

    if (g_settings.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL = FormatSQL("select tvshow.idShow,tvshow.c%02d,path.strPath from tvshow,path,tvshowlinkpath where tvshowlinkpath.idPath=path.idPath and tvshowlinkpath.idShow=tvshow.idShow and tvshow.c%02d like '%%%s%%'",VIDEODB_ID_TV_TITLE,VIDEODB_ID_TV_TITLE,strSearch.c_str());
    else
      strSQL = FormatSQL("select tvshow.idShow,tvshow.c%02d from tvshow where tvshow.c%02d like '%%%s%%'",VIDEODB_ID_TV_TITLE,VIDEODB_ID_TV_TITLE,strSearch.c_str());
    m_pDS->query( strSQL.c_str() );

    while (!m_pDS->eof())
    {
      if (g_settings.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv("path.strPath").get_asString()),g_settings.m_videoSources))
        {
          m_pDS->next();
          continue;
        }

      CFileItemPtr pItem(new CFileItem(m_pDS->fv(1).get_asString()));
      CStdString strDir;
      strDir.Format("2/2/%ld/", m_pDS->fv("tvshow.idShow").get_asInt());

      pItem->m_strPath="videodb://"+ strDir;
      pItem->m_bIsFolder=true;
      pItem->GetVideoInfoTag()->m_iDbId = m_pDS->fv("tvshow.idshow").get_asInt();
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

    if (g_settings.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL = FormatSQL("select episode.idEpisode,episode.c%02d,episode.c%02d,tvshowlinkepisode.idShow,tvshow.c%02d,path.strPath from episode,files,path,tvshowlinkepisode,tvshow where files.idFile=episode.idFile and tvshowlinkepisode.idEpisode=episode.idEpisode and tvshowlinkepisode.idShow=tvshow.idShow and files.idPath=path.idPath and episode.c%02d like '%%%s%%'",VIDEODB_ID_EPISODE_TITLE,VIDEODB_ID_EPISODE_SEASON,VIDEODB_ID_TV_TITLE,VIDEODB_ID_EPISODE_TITLE,strSearch.c_str());
    else
      strSQL = FormatSQL("select episode.idEpisode,episode.c%02d,episode.c%02d,tvshowlinkepisode.idShow,tvshow.c%02d from episode,tvshowlinkepisode,tvshow where tvshowlinkepisode.idEpisode=episode.idEpisode and tvshow.idShow=tvshowlinkepisode.idShow and episode.c%02d like '%%%s%%'",VIDEODB_ID_EPISODE_TITLE,VIDEODB_ID_EPISODE_SEASON,VIDEODB_ID_TV_TITLE,VIDEODB_ID_EPISODE_TITLE,strSearch.c_str());
    m_pDS->query( strSQL.c_str() );

    while (!m_pDS->eof())
    {
      if (g_settings.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv("path.strPath").get_asString()),g_settings.m_videoSources))
        {
          m_pDS->next();
          continue;
        }

      CFileItemPtr pItem(new CFileItem(m_pDS->fv(1).get_asString()+" ("+m_pDS->fv(4).get_asString()+")"));
      pItem->m_strPath.Format("videodb://2/2/%ld/%ld/%ld",m_pDS->fv("tvshowlinkepisode.idShow").get_asInt(),m_pDS->fv(2).get_asInt(),m_pDS->fv(0).get_asInt());
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

    if (g_settings.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL = FormatSQL("select musicvideo.idMVideo,musicvideo.c%02d,path.strPath from musicvideo,files,path where files.idFile=musicvideo.idFile and files.idPath=path.idPath and musicvideo.c%02d like '%%%s%%'",VIDEODB_ID_MUSICVIDEO_TITLE,VIDEODB_ID_MUSICVIDEO_TITLE,strSearch.c_str());
    else
      strSQL = FormatSQL("select musicvideo.idMVideo,musicvideo.c%02d from musicvideo where musicvideo.c%02d like '%%%s%%'",VIDEODB_ID_MUSICVIDEO_TITLE,VIDEODB_ID_MUSICVIDEO_TITLE,strSearch.c_str());
    m_pDS->query( strSQL.c_str() );

    while (!m_pDS->eof())
    {
      if (g_settings.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv("path.strPath").get_asString()),g_settings.m_videoSources))
        {
          m_pDS->next();
          continue;
        }

      CFileItemPtr pItem(new CFileItem(m_pDS->fv(1).get_asString()));
      CStdString strDir;
      strDir.Format("3/2/%ld",m_pDS->fv("musicvideo.idMVideo").get_asInt());

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

    if (g_settings.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL = FormatSQL("select episode.idEpisode,episode.c%02d,episode.c%02d,tvshowlinkepisode.idShow,tvshow.c%02d,path.strPath from episode,files,path,tvshowlinkepisode,tvshow where files.idFile=episode.idFile and tvshowlinkepisode.idEpisode=episode.idEpisode and files.idPath=path.idPath and tvshow.idShow=tvshowlinkepisode.idShow and episode.c%02d like '%%%s%%'",VIDEODB_ID_EPISODE_TITLE,VIDEODB_ID_EPISODE_SEASON,VIDEODB_ID_TV_TITLE,VIDEODB_ID_EPISODE_PLOT,strSearch.c_str());
    else
      strSQL = FormatSQL("select episode.idEpisode,episode.c%02d,episode.c%02d,tvshowlinkepisode.idShow,tvshow.c%02d from episode,tvshowlinkepisode,tvshow where tvshowlinkepisode.idEpisode=episode.idEpisode and tvshow.idShow=tvshowlinkepisode.idShow and episode.c%02d like '%%%s%%'",VIDEODB_ID_EPISODE_TITLE,VIDEODB_ID_EPISODE_SEASON,VIDEODB_ID_TV_TITLE,VIDEODB_ID_EPISODE_PLOT,strSearch.c_str());
    m_pDS->query( strSQL.c_str() );

    while (!m_pDS->eof())
    {
      if (g_settings.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv("path.strPath").get_asString()),g_settings.m_videoSources))
        {
          m_pDS->next();
          continue;
        }

      CFileItemPtr pItem(new CFileItem(m_pDS->fv(1).get_asString()+" ("+m_pDS->fv(4).get_asString()+")"));
      pItem->m_strPath.Format("videodb://2/2/%ld/%ld/%ld",m_pDS->fv("tvshowlinkepisode.idShow").get_asInt(),m_pDS->fv(2).get_asInt(),m_pDS->fv(0).get_asInt());
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

void CVideoDatabase::GetMoviesByPlot(const CStdString& strSearch, CFileItemList& items)
{
  CStdString strSQL;

  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    if (g_settings.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL = FormatSQL("select movie.idMovie, movie.c%02d, path.strPath from movie,files,path where files.idFile=movie.idFile and files.idPath=path.idPath and (movie.c%02d like '%%%s%%' or movie.c%02d like '%%%s%%' or movie.c%02d like '%%%s%%')",VIDEODB_ID_TITLE,VIDEODB_ID_PLOT,strSearch.c_str(),VIDEODB_ID_PLOTOUTLINE,strSearch.c_str(),VIDEODB_ID_TAGLINE,strSearch.c_str());
    else
      strSQL = FormatSQL("select movie.idMovie, movie.c%02d from movie where (movie.c%02d like '%%%s%%' or movie.c%02d like '%%%s%%' or movie.c%02d like '%%%s%%')",VIDEODB_ID_TITLE,VIDEODB_ID_PLOT,strSearch.c_str(),VIDEODB_ID_PLOTOUTLINE,strSearch.c_str(),VIDEODB_ID_TAGLINE,strSearch.c_str());

    m_pDS->query( strSQL.c_str() );

    while (!m_pDS->eof())
    {
      if (g_settings.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv(2).get_asString()),g_settings.m_videoSources))
        {
          m_pDS->next();
          continue;
        }

      CFileItemPtr pItem(new CFileItem(m_pDS->fv(1).get_asString()));
      pItem->m_strPath.Format("videodb://1/2/%ld", m_pDS->fv(0).get_asInt());
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

    if (g_settings.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL = FormatSQL("select directorlinkmovie.idDirector,actors.strActor,path.strPath from movie,files,path,actors,directorlinkmovie where files.idFile=movie.idFile and files.idPath=path.idPath and directorlinkmovie.idMovie=movie.idMovie and directorlinkmovie.idDirector=actors.idActor and actors.strActor like '%%%s%%'",strSearch.c_str());
    else
      strSQL = FormatSQL("select directorlinkmovie.idDirector,actors.strActor from movie,actors,directorlinkmovie where directorlinkmovie.idMovie=movie.idMovie and directorlinkmovie.idDirector=actors.idActor and actors.strActor like '%%%s%%'",strSearch.c_str());

    m_pDS->query( strSQL.c_str() );

    while (!m_pDS->eof())
    {
      if (g_settings.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv("path.strPath").get_asString()),g_settings.m_videoSources))
        {
          m_pDS->next();
          continue;
        }

      CStdString strDir;
      strDir.Format("%ld/", m_pDS->fv("directorlinkmovie.idDirector").get_asInt());
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

    if (g_settings.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL = FormatSQL("select directorlinktvshow.idDirector,actors.strActor,path.strPath from tvshow,path,actors,directorlinktvshow,tvshowlinkpath where tvshowlinkpath.idPath=path.idPath and tvshowlinkpath.idShow=tvshow.idShow and directorlinktvshow.idShow=tvshow.idShow and directorlinktvshow.idDirector=actors.idActor and actors.strActor like '%%%s%%'",strSearch.c_str());
    else
      strSQL = FormatSQL("select directorlinktvshow.idDirector,actors.strActor from tvshow,actors,directorlinktvshow where directorlinktvshow.idShow=tvshow.idShow and directorlinktvshow.idDirector=actors.idActor and actors.strActor like '%%%s%%'",strSearch.c_str());

    m_pDS->query( strSQL.c_str() );

    while (!m_pDS->eof())
    {
      if (g_settings.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv("path.strPath").get_asString()),g_settings.m_videoSources))
        {
          m_pDS->next();
          continue;
        }

      CStdString strDir;
      strDir.Format("%ld/", m_pDS->fv("directorlinktvshow.idDirector").get_asInt());
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

    if (g_settings.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL = FormatSQL("select directorlinkmusicvideo.idDirector,actors.strActor,path.strPath from musicvideo,files,path,actors,directorlinkmusicvideo where files.idFile=musicvideo.idFile and files.idPath=path.idPath and directorlinkmusicvideo.idMVideo=musicvideo.idMVideo and directorlinkmusicvideo.idDirector=actors.idActor and actors.strActor like '%%%s%%'",strSearch.c_str());
    else
      strSQL = FormatSQL("select directorlinkmusicvideo.idDirector,actors.strActor from musicvideo,actors,directorlinkmusicvideo where directorlinkmusicvideo.idMVideo=musicvideo.idMVideo and directorlinkmusicvideo.idDirector=actors.idActor and actors.strActor like '%%%s%%'",strSearch.c_str());

    m_pDS->query( strSQL.c_str() );

    while (!m_pDS->eof())
    {
      if (g_settings.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv("path.strPath").get_asString()),g_settings.m_videoSources))
        {
          m_pDS->next();
          continue;
        }

      CStdString strDir;
      strDir.Format("%ld/", m_pDS->fv("directorlinkmusicvideo.idDirector").get_asInt());
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

void CVideoDatabase::CleanDatabase(IVideoInfoScannerObserver* pObserver, const vector<int>* paths)
{
  CGUIDialogProgress *progress=NULL;
  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    BeginTransaction();

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
        strPaths.Format("%s,%i",strPaths.Mid(0).c_str(),paths->at(i));
      sql = FormatSQL("select * from files,path where files.idPath=path.idPath and path.idPath in (%s)",strPaths.Mid(1).c_str());
    }
    else
      sql = "select * from files, path where files.idPath = path.idPath";

    m_pDS->query(sql.c_str());
    if (m_pDS->num_rows() == 0) return;

    if (!pObserver)
    {
      progress = (CGUIDialogProgress *)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
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
          url.GetProtocol() == "http" ||
          url.GetProtocol() == "https" ||
          !CFile::Exists(fullPath, false))
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

    CLog::Log(LOGDEBUG, "%s Cleaning streamdetails table", __FUNCTION__);
    sql = "delete from streamdetails where idFile in " + filesToDelete;
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

    CLog::Log(LOGDEBUG, "%s Cleaning setlinkmovie table", __FUNCTION__);
    sql = "delete from setlinkmovie where idMovie in " + moviesToDelete;
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, "Cleaning paths that don't exist and don't have content set...");
    sql = "select * from path where strContent not like ''";
    m_pDS->query(sql.c_str());
    CStdString strIds;
    while (!m_pDS->eof())
    {
      if (!CDirectory::Exists(m_pDS->fv("path.strPath").get_asString()))
        strIds.Format("%s %i,",strIds.Mid(0),m_pDS->fv("path.idPath").get_asInt()); // mid since we cannot format the same string
      m_pDS->next();
    }
    if (!strIds.IsEmpty())
    {
      strIds.TrimLeft(" ");
      strIds.TrimRight(",");
      sql = FormatSQL("delete from path where idPath in (%s)",strIds.c_str());
      m_pDS->exec(sql.c_str());
      sql = FormatSQL("delete from tvshowlinkpath where idPath in (%s)",strIds.c_str());
      m_pDS->exec(sql.c_str());
    }

    CLog::Log(LOGDEBUG, "%s Cleaning tvshow table", __FUNCTION__);
    sql = "delete from tvshow where idShow not in (select idShow from tvshowlinkpath)";
    m_pDS->exec(sql.c_str());
    sql = "delete from tvshow where idShow in (select tvshow.idShow from tvshow "
                                               "join tvshowlinkpath on tvshow.idShow=tvshowlinkpath.idShow "
                                               "join path on path.idPath=tvshowlinkpath.idPath "
                                               "where tvshow.idShow not in (select idShow from tvshowlinkepisode) "
                                               "and path.strContent == '')";
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, "%s Cleaning actorlinktvshow table", __FUNCTION__);
    sql = "delete from actorlinktvshow where idShow not in (select idShow from tvshow)";
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, "%s Cleaning directorlinktvshow table", __FUNCTION__);
    sql = "delete from directorlinktvshow where idShow not in (select idShow from tvshow)";
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, "%s Cleaning tvshowlinkpath table", __FUNCTION__);
    sql = "delete from tvshowlinkpath where idShow not in (select idShow from tvshow)";
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, "%s Cleaning genrelinktvshow table", __FUNCTION__);
    sql = "delete from genrelinktvshow where idShow not in (select idShow from tvshow)";
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, "%s Cleaning movielinktvshow table", __FUNCTION__);
    sql = "delete from movielinktvshow where idShow not in (select idShow from tvshow)";
    m_pDS->exec(sql.c_str());
    sql = "delete from movielinktvshow where idMovie not in (select distinct idMovie from movie)";
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, "%s Cleaning musicvideo table", __FUNCTION__);
    sql = "delete from musicvideo where idMVideo in " + musicVideosToDelete;
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

    CLog::Log(LOGDEBUG, "%s Cleaning genre table", __FUNCTION__);
    sql = "delete from genre where idGenre not in (select distinct idGenre from genrelinkmovie) and idGenre not in (select distinct idGenre from genrelinktvshow) and idGenre not in (select distinct idGenre from genrelinkmusicvideo)";
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, "%s Cleaning actor table of actors, directors and writers", __FUNCTION__);
    sql = "delete from actors where idActor not in (select distinct idActor from actorlinkmovie) and idActor not in (select distinct idDirector from directorlinkmovie) and idActor not in (select distinct idWriter from writerlinkmovie) and idActor not in (select distinct idActor from actorlinktvshow) and idActor not in (select distinct idActor from actorlinkepisode) and idActor not in (select distinct idDirector from directorlinktvshow) and idActor not in (select distinct idDirector from directorlinkepisode) and idActor not in (select distinct idWriter from writerlinkepisode) and idActor not in (select distinct idArtist from artistlinkmusicvideo) and idActor not in (select distinct idDirector from directorlinkmusicvideo)";
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, "%s Cleaning studio table", __FUNCTION__);
    sql = "delete from studio where idStudio not in (select distinct idStudio from studiolinkmovie) and idStudio not in (select distinct idStudio from studiolinkmusicvideo) and idStudio not in (select distinct idStudio from studiolinktvshow)";
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, "%s Cleaning set table", __FUNCTION__);
    sql = "delete from sets where idSet not in (select distinct idSet from setlinkmovie)";
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
      CStdString where = FormatSQL("where idShow=%i", items[i]->GetVideoInfoTag()->m_iDbId);
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

void CVideoDatabase::ExportToXML(const CStdString &path, bool singleFiles /* = false */, bool images /* = false */, bool actorThumbs /* false */, bool overwrite /*=false*/)
{
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

    // if we're exporting to a single folder, we export thumbs as well
    CStdString exportRoot = CUtil::AddFileToFolder(path, "xbmc_videodb_" + CDateTime::GetCurrentDateTime().GetAsDBDate());
    CStdString xmlFile = CUtil::AddFileToFolder(exportRoot, "videodb.xml");
    CStdString actorsDir = CUtil::AddFileToFolder(exportRoot, "actors");
    CStdString moviesDir = CUtil::AddFileToFolder(exportRoot, "movies");
    CStdString musicvideosDir = CUtil::AddFileToFolder(exportRoot, "musicvideos");
    CStdString tvshowsDir = CUtil::AddFileToFolder(exportRoot, "tvshows");
    if (!singleFiles)
    {
      images = true;
      overwrite = false;
      actorThumbs = true;
      CDirectory::Remove(exportRoot);
      CDirectory::Create(exportRoot);
      CDirectory::Create(actorsDir);
      CDirectory::Create(moviesDir);
      CDirectory::Create(musicvideosDir);
      CDirectory::Create(tvshowsDir);
    }

    progress = (CGUIDialogProgress *)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
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
      // strip paths to make them relative
      if (movie.m_strTrailer.Mid(0,movie.m_strPath.size()).Equals(movie.m_strPath))
        movie.m_strTrailer = movie.m_strTrailer.Mid(movie.m_strPath.size());
      movie.Save(pMain, "movie", !singleFiles);

      // reset old skip state
      bool bSkip = false;

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

      CFileItem item(movie.m_strFileNameAndPath,false);
      CFileItem saveItem(item);
      if (!singleFiles)
        saveItem = CFileItem(GetSafeFile(moviesDir, movie.m_strTitle) + ".avi", false);
      if (singleFiles && CUtil::IsWritable(movie.m_strFileNameAndPath))
      {
        if (!item.Exists(false))
        {
          CLog::Log(LOGDEBUG, "%s - Not exporting item %s as it does not exist", __FUNCTION__, movie.m_strFileNameAndPath.c_str());
          bSkip = true;
        }
        else
        {
          CStdString nfoFile(CUtil::ReplaceExtension(item.GetTBNFile(), ".nfo"));

          if (overwrite || !CFile::Exists(nfoFile, false))
          {
            if(!xmlDoc.SaveFile(nfoFile))
            {
              CLog::Log(LOGERROR, "%s: Movie nfo export failed! ('%s')", __FUNCTION__, nfoFile.c_str());
              bSkip = ExportSkipEntry(nfoFile);
              if (!bSkip)
              {
                if (progress)
                {
                  progress->Close();
                  m_pDS->close();
                  return;
                }
              }
            }
          }

          xmlDoc.Clear();
          TiXmlDeclaration decl("1.0", "UTF-8", "yes");
          xmlDoc.InsertEndChild(decl);
        }
      }

      if (images && !bSkip)
      {
        CStdString cachedThumb(GetCachedThumb(item));
        CStdString savedThumb(saveItem.GetTBNFile());
        if (!cachedThumb.IsEmpty() && (overwrite || !CFile::Exists(savedThumb, false)))
          if (!CFile::Cache(cachedThumb, savedThumb))
            CLog::Log(LOGERROR, "%s: Movie thumb export failed! ('%s' -> '%s')", __FUNCTION__, cachedThumb.c_str(), savedThumb.c_str());
        
        CStdString cachedFanart(item.GetCachedFanart());
        CStdString savedFanart(CUtil::ReplaceExtension(savedThumb, "-fanart.jpg"));
        
        if (CFile::Exists(cachedFanart, false))
          if (!CFile::Cache(cachedFanart, savedFanart))
            CLog::Log(LOGERROR, "%s: Movie fanart export failed! ('%s' -> '%s')", __FUNCTION__, cachedFanart.c_str(), savedFanart.c_str());
        
        if (actorThumbs)
        {
          if (singleFiles)
            ExportActorThumbs(movie, overwrite);
          else
            ExportActorThumbs(actorsDir, movie, overwrite);
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

      // reset old skip state
      bool bSkip = false;

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

      CFileItem item(movie.m_strFileNameAndPath,false);
      CFileItem saveItem(item);
      if (!singleFiles)
        saveItem = CFileItem(GetSafeFile(musicvideosDir, movie.m_strArtist + "." + movie.m_strTitle) + ".avi", false);
      if (CUtil::IsWritable(movie.m_strFileNameAndPath) && singleFiles)
      {
        if (!item.Exists(false))
        {
          CLog::Log(LOGDEBUG, "%s - Not exporting item %s as it does not exist", __FUNCTION__, movie.m_strFileNameAndPath.c_str());
          bSkip = true;
        }
        else
        {
          CStdString nfoFile(CUtil::ReplaceExtension(item.GetTBNFile(), ".nfo"));

          if (overwrite || !CFile::Exists(nfoFile, false))
          {
            if(!xmlDoc.SaveFile(nfoFile))
            {
              CLog::Log(LOGERROR, "%s: Musicvideo nfo export failed! ('%s')", __FUNCTION__, nfoFile.c_str());
              bSkip = ExportSkipEntry(nfoFile);
              if (!bSkip)
              {
                if (progress)
                {
                  progress->Close();
                  m_pDS->close();
                  return;
                }
              }
            }
          }

          xmlDoc.Clear();
          TiXmlDeclaration decl("1.0", "UTF-8", "yes");
          xmlDoc.InsertEndChild(decl);
        }
      }
      if (images && !bSkip)
      {
        CStdString cachedThumb(GetCachedThumb(item));
        CStdString savedThumb(saveItem.GetTBNFile());
        if (!cachedThumb.IsEmpty() && (overwrite || !CFile::Exists(savedThumb, false)))
          if (!CFile::Cache(cachedThumb, savedThumb))
            CLog::Log(LOGERROR, "%s: Musicvideo thumb export failed! ('%s' -> '%s')", __FUNCTION__, cachedThumb.c_str(), savedThumb.c_str());
      }
      m_pDS->next();
      current++;
    }
    m_pDS->close();

    // repeat for all tvshows
    sql = VIDEO_DATABASE_VIEW_TVSHOW;
    m_pDS->query(sql.c_str());

    total = m_pDS->num_rows();
    current = 0;

    while (!m_pDS->eof())
    {
      CVideoInfoTag tvshow = GetDetailsForTvShow(m_pDS, true);
      tvshow.Save(pMain, "tvshow", !singleFiles);

      // reset old skip state
      bool bSkip = false;

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


      CFileItem item(tvshow.m_strPath, true);
      CFileItem saveItem(item);
      if (!singleFiles)
      {
        saveItem = CFileItem(GetSafeFile(tvshowsDir, tvshow.m_strShowTitle), true);
        CDirectory::Create(item.m_strPath);
      }
      if (singleFiles && CUtil::IsWritable(tvshow.m_strPath))
      {
        if (!item.Exists(false))
        {
          CLog::Log(LOGDEBUG, "%s - Not exporting item %s as it does not exist", __FUNCTION__, tvshow.m_strPath.c_str());
          bSkip = true;
        }
        else
        {
          CStdString nfoFile;
          CUtil::AddFileToFolder(tvshow.m_strPath, "tvshow.nfo", nfoFile);

          if (overwrite || !CFile::Exists(nfoFile, false))
          {
            if(!xmlDoc.SaveFile(nfoFile))
            {
              CLog::Log(LOGERROR, "%s: TVShow nfo export failed! ('%s')", __FUNCTION__, nfoFile.c_str());
              bSkip = ExportSkipEntry(nfoFile);
              if (!bSkip)
              {
                if (progress)
                {
                  progress->Close();
                  m_pDS->close();
                  return;
                }
              }
            }
          }

          xmlDoc.Clear();
          TiXmlDeclaration decl("1.0", "UTF-8", "yes");
          xmlDoc.InsertEndChild(decl);
        }
      }
      if (images && !bSkip)
      {
        CStdString cachedThumb(GetCachedThumb(item));
        CStdString savedThumb(saveItem.GetFolderThumb());
        if (!cachedThumb.IsEmpty() && (overwrite || !CFile::Exists(savedThumb, false)))
          if (!CFile::Cache(cachedThumb, savedThumb))
            CLog::Log(LOGERROR, "%s: TVShow thumb export failed! ('%s' -> '%s')", __FUNCTION__, cachedThumb.c_str(), savedThumb.c_str());

        CStdString cachedFanart(item.GetCachedFanart());
        CStdString savedFanart(saveItem.GetFolderThumb("fanart.jpg"));
        if (CFile::Exists(cachedFanart, false) && (overwrite || !CFile::Exists(savedFanart, false)))
          if (!CFile::Cache(cachedFanart, savedFanart))
            CLog::Log(LOGERROR, "%s: TVShow fanart export failed! ('%s' -> '%s')", __FUNCTION__, cachedFanart.c_str(), savedFanart.c_str());

        if (actorThumbs)
        {
          if (singleFiles)
            ExportActorThumbs(tvshow, overwrite);
          else
            ExportActorThumbs(actorsDir, tvshow, overwrite);
        }

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
          int iSeason = pDS2->fv(0).get_asInt();
          CStdString strLabel;
          if (iSeason == 0)
            strLabel = g_localizeStrings.Get(20381);
          else
            strLabel.Format(g_localizeStrings.Get(20358),iSeason);
          CFileItemPtr pItem(new CFileItem(strLabel));
          pItem->GetVideoInfoTag()->m_strTitle = strLabel;
          pItem->GetVideoInfoTag()->m_iSeason = iSeason;
          pItem->GetVideoInfoTag()->m_strPath = tvshow.m_strPath;
          items.Add(pItem);
          pDS2->next();
        }
        pDS2->close();

        // export season thumbs
        for (int i=0;i<items.Size();++i)
        {
          CStdString strSeasonThumb, strParent;
          int iSeason = items[i]->GetVideoInfoTag()->m_iSeason;
          if (iSeason == -1)
            strSeasonThumb = "season-all.tbn";
          else if (iSeason == 0)
            strSeasonThumb = "season-specials.tbn";
          else
            strSeasonThumb.Format("season%02i.tbn",iSeason);
          CUtil::GetParentPath(saveItem.GetFolderThumb(), strParent);
          
          CStdString cachedThumb(items[i]->GetCachedSeasonThumb());
          CStdString savedThumb(CUtil::AddFileToFolder(strParent, strSeasonThumb));

          if (CFile::Exists(cachedThumb, false) && (overwrite || !CFile::Exists(savedThumb, false)))
            if (!CFile::Cache(cachedThumb, savedThumb))
              CLog::Log(LOGERROR, "%s: TVShow season thumb export failed ('%s' -> '%s')", __FUNCTION__, cachedThumb.c_str(), savedThumb.c_str());
        }
      }
      
      // now save the episodes from this show
      sql = FormatSQL("select * from episodeview where idShow=%i order by strFileName, idEpisode",tvshow.m_iDbId);
      pDS->query(sql.c_str());
      CStdString showDir(saveItem.m_strPath);

      while (!pDS->eof())
      {
        CVideoInfoTag episode = GetDetailsForEpisode(pDS, true);
        if (singleFiles)
          episode.Save(pMain, "episodedetails", !singleFiles);
        else
          episode.Save(pMain->LastChild(), "episodedetails", !singleFiles);
        pDS->next();
        // multi-episode files need dumping to the same XML
        while (singleFiles && !pDS->eof() &&
               episode.m_iFileId == pDS->fv("idFile").get_asInt())
        {
          episode = GetDetailsForEpisode(pDS, true);
          episode.Save(pMain, "episodedetails", !singleFiles);
          pDS->next();
        }

        // reset old skip state
        bool bSkip = false;

        CFileItem item(episode.m_strFileNameAndPath, false);
        CFileItem saveItem(item);
        if (!singleFiles)
        {
          CStdString epName;
          epName.Format("s%02ie%02i.avi", episode.m_iSeason, episode.m_iEpisode);
          saveItem = CFileItem(CUtil::AddFileToFolder(showDir, epName), false);
        }
        if (singleFiles)
        {
          if (!item.Exists(false))
          {
            CLog::Log(LOGDEBUG, "%s - Not exporting item %s as it does not exist", __FUNCTION__, episode.m_strFileNameAndPath.c_str());
            bSkip = true;
          }
          else
          {
            CStdString nfoFile(CUtil::ReplaceExtension(item.GetTBNFile(), ".nfo"));

            if (overwrite || !CFile::Exists(nfoFile, false))
            {
              if(!xmlDoc.SaveFile(nfoFile))
              {
                CLog::Log(LOGERROR, "%s: Episode nfo export failed! ('%s')", __FUNCTION__, nfoFile.c_str());
                bSkip = ExportSkipEntry(nfoFile);
                if (!bSkip)
                {
                  if (progress)
                  {
                    progress->Close();
                    m_pDS->close();
                    return;
                  }
                }
              }
            }

            xmlDoc.Clear();
            TiXmlDeclaration decl("1.0", "UTF-8", "yes");
            xmlDoc.InsertEndChild(decl);
          }
        }

        if (images && !bSkip)
        {
          CStdString cachedThumb(GetCachedThumb(item));
          CStdString savedThumb(saveItem.GetTBNFile());
          if (!cachedThumb.IsEmpty() && (overwrite || !CFile::Exists(savedThumb, false)))
            if (!CFile::Cache(cachedThumb, savedThumb))
              CLog::Log(LOGERROR, "%s: Episode thumb export failed! ('%s' -> '%s')", __FUNCTION__, cachedThumb.c_str(), savedThumb.c_str());

          if (actorThumbs)
            ExportActorThumbs(episode, overwrite);
        }
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
        ScraperPtr info;
        int iFound=0;
        if (GetScraperForPath(iter->first,info,iFound) && iFound == 1)
        {
          TiXmlElement xmlPathElement2("path");
          TiXmlNode *pPath = pPaths->InsertEndChild(xmlPathElement2);
          XMLUtils::SetString(pPath,"url",iter->first);
          XMLUtils::SetInt(pPath,"scanrecursive",iter->second.recurse);
          XMLUtils::SetBoolean(pPath,"usefoldernames",iter->second.parent_name);
          XMLUtils::SetString(pPath,"content", TranslateContent(info->Content()));
          XMLUtils::SetString(pPath,"scraperpath",info->ID());
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

void CVideoDatabase::ExportActorThumbs(const CVideoInfoTag& tag, bool overwrite /*=false*/)
{
  CStdString strDir = CUtil::AddFileToFolder(tag.m_strPath,".actors");
  if (!CDirectory::Exists(strDir))
  {
    CDirectory::Create(strDir);
    CFile::SetHidden(strDir, true);
  }
  ExportActorThumbs(strDir, tag, overwrite);
}

void CVideoDatabase::ExportActorThumbs(const CStdString &strDir, const CVideoInfoTag &tag, bool overwrite /*=false*/)
{
  for (CVideoInfoTag::iCast iter = tag.m_cast.begin();iter != tag.m_cast.end();++iter)
  {
    CFileItem item;
    item.SetLabel(iter->strName);
    CStdString strThumb = item.GetCachedActorThumb();
    if (CFile::Exists(strThumb))
    {
      CStdString thumbFile(GetSafeFile(strDir, iter->strName) + ".tbn");
      if (overwrite || !CFile::Exists(thumbFile))
        if (!CFile::Cache(strThumb, thumbFile))
          CLog::Log(LOGERROR, "%s: Actor thumb export failed! ('%s' -> '%s')", __FUNCTION__, strThumb.c_str(), thumbFile.c_str());
    }
  }
}

CStdString CVideoDatabase::GetCachedThumb(const CFileItem& item) const
{
  CStdString cachedThumb(item.GetCachedVideoThumb());
  if (!CFile::Exists(cachedThumb) && g_advancedSettings.m_bVideoLibraryExportAutoThumbs)
  {
    CStdString strPath, strFileName;
    CUtil::Split(cachedThumb, strPath, strFileName);
    cachedThumb = strPath + "auto-" + strFileName;
  }

  if (CFile::Exists(cachedThumb))
    return cachedThumb;
  else
    return "";
}

bool CVideoDatabase::ExportSkipEntry(const CStdString &nfoFile)
{
  CStdString strParent;
  CUtil::GetParentPath(nfoFile,strParent);
  CLog::Log(LOGERROR, "%s: Unable to write to '%s'!", __FUNCTION__, strParent.c_str());

  bool bSkip = CGUIDialogYesNo::ShowAndGetInput(g_localizeStrings.Get(647), g_localizeStrings.Get(20302), strParent.c_str(), g_localizeStrings.Get(20303));

  if (bSkip)
    CLog::Log(LOGERROR, "%s: Skipping export of '%s' as requested", __FUNCTION__, nfoFile.c_str());
  else
    CLog::Log(LOGERROR, "%s: Export failed! Canceling as requested", __FUNCTION__);

  return bSkip;
}

void CVideoDatabase::ImportFromXML(const CStdString &path)
{
  CGUIDialogProgress *progress=NULL;
  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    TiXmlDocument xmlDoc;
    if (!xmlDoc.LoadFile(CUtil::AddFileToFolder(path, "videodb.xml")))
      return;

    TiXmlElement *root = xmlDoc.RootElement();
    if (!root) return;

    progress = (CGUIDialogProgress *)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
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

    CStdString actorsDir(CUtil::AddFileToFolder(path, "actors"));
    CStdString moviesDir(CUtil::AddFileToFolder(path, "movies"));
    CStdString musicvideosDir(CUtil::AddFileToFolder(path, "musicvideos"));
    CStdString tvshowsDir(CUtil::AddFileToFolder(path, "tvshows"));
    CVideoInfoScanner scanner;
    set<CStdString> actors;
    movie = root->FirstChildElement();
    while (movie)
    {
      CVideoInfoTag info;
      if (strnicmp(movie->Value(), "movie", 5) == 0)
      {
        info.Load(movie);
        CFileItem item(info);
        scanner.AddMovie(&item,CONTENT_MOVIES,info);
        SetPlayCount(item, info.m_playCount, info.m_lastPlayed);
        CStdString file(GetSafeFile(moviesDir, info.m_strTitle));
        CFile::Cache(file + ".tbn", item.GetCachedVideoThumb());
        CFile::Cache(file + "-fanart.jpg", item.GetCachedFanart());
        for (CVideoInfoTag::iCast i = info.m_cast.begin(); i != info.m_cast.end(); ++i)
          actors.insert(i->strName);
        current++;
      }
      else if (strnicmp(movie->Value(), "musicvideo", 10) == 0)
      {
        info.Load(movie);
        CFileItem item(info);
        scanner.AddMovie(&item,CONTENT_MUSICVIDEOS,info);
        SetPlayCount(item, info.m_playCount, info.m_lastPlayed);
        CStdString file(GetSafeFile(musicvideosDir, info.m_strArtist + "." + info.m_strTitle));
        CFile::Cache(file + ".tbn", item.GetCachedVideoThumb());
        current++;
      }
      else if (strnicmp(movie->Value(), "tvshow", 6) == 0)
      {
        // load the TV show in.  NOTE: This deletes all episodes under the TV Show, which may not be
        // what we desire.  It may make better sense to only delete (or even better, update) the show information
        info.Load(movie);
        CUtil::AddSlashAtEnd(info.m_strPath);
        DeleteTvShow(info.m_strPath);
        CFileItem item(info);
        int showID = scanner.AddMovie(&item,CONTENT_TVSHOWS,info);
        current++;
        CStdString showDir(GetSafeFile(tvshowsDir, info.m_strTitle));
        CFile::Cache(CUtil::AddFileToFolder(showDir, "folder.jpg"), item.GetCachedVideoThumb());
        CFile::Cache(CUtil::AddFileToFolder(showDir, "fanart.jpg"), item.GetCachedFanart());
        for (CVideoInfoTag::iCast i = info.m_cast.begin(); i != info.m_cast.end(); ++i)
          actors.insert(i->strName);
        // now load the episodes
        TiXmlElement *episode = movie->FirstChildElement("episodedetails");
        while (episode)
        {
          // no need to delete the episode info, due to the above deletion
          CVideoInfoTag info;
          info.Load(episode);
          CFileItem item(info);
          scanner.AddMovie(&item,CONTENT_TVSHOWS,info,showID);
          SetPlayCount(item, info.m_playCount, info.m_lastPlayed);
          CStdString file;
          file.Format("s%02ie%02i.tbn", info.m_iSeason, info.m_iEpisode);
          CFile::Cache(CUtil::AddFileToFolder(showDir, file), item.GetCachedVideoThumb());
          for (CVideoInfoTag::iCast i = info.m_cast.begin(); i != info.m_cast.end(); ++i)
            actors.insert(i->strName);
          episode = episode->NextSiblingElement("episodedetails");
        }
        // and fetch season thumbs
        scanner.FetchSeasonThumbs(showID, showDir, false, true);
      }
      else if (strnicmp(movie->Value(), "paths", 5) == 0)
      {
        const TiXmlElement* path = movie->FirstChildElement("path");
        while (path)
        {
          CStdString strPath;
          XMLUtils::GetString(path,"url",strPath);
          CStdString content;

          if (XMLUtils::GetString(path,"content", content))
          { // check the scraper exists, if so store the path
            AddonPtr addon;
            CStdString uuid;

            if (!XMLUtils::GetString(path,"scraperID",uuid))
            { // support pre addons exports
              XMLUtils::GetString(path, "scraperpath", uuid);
              uuid = CUtil::GetFileName(uuid);
            }

            if (CAddonMgr::Get()->GetAddon(uuid, addon, ADDON::ADDON_SCRAPER))
            {
              SScanSettings settings;
              ScraperPtr scraper = boost::dynamic_pointer_cast<CScraper>(addon);
              scraper->m_pathContent = TranslateContent(content);
              XMLUtils::GetInt(path,"scanrecursive",settings.recurse);
              XMLUtils::GetBoolean(path,"usefoldernames",settings.parent_name);
              SetScraperForPath(strPath,scraper,settings);
            }
          }
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
    // cache any actor thumbs
    for (set<CStdString>::iterator i = actors.begin(); i != actors.end(); i++)
    {
      CFileItem item;
      item.SetLabel(*i);
      CStdString savedThumb(GetSafeFile(actorsDir, *i) + ".tbn");
      CStdString cachedThumb = item.GetCachedActorThumb();
      CFile::Cache(savedThumb, cachedThumb);
    }
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

bool CVideoDatabase::ArbitraryExec(const CStdString& strExec)
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

void CVideoDatabase::ConstructPath(CStdString& strDest, const CStdString& strPath, const CStdString& strFileName)
{
  if (CUtil::IsStack(strFileName) || CUtil::IsInArchive(strFileName))
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
  ScraperPtr info;
  SScanSettings settings;
  int iFound;
  GetScraperForPath(strPath,info,settings,iFound);
  SetPathHash(strPath,"");
  if (!info)
    return;
  if (info->Content() == CONTENT_TVSHOWS || (info->Content() == CONTENT_MOVIES && iFound != 1)) // if we scan by folder name we need to invalidate parent as well
  {
    if (info->Content() == CONTENT_TVSHOWS || settings.parent_name_root)
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
  { // number of items in the db has likely changed, so recalculate
    g_infoManager.SetLibraryBool(LIBRARY_HAS_MOVIES, HasContent(VIDEODB_CONTENT_MOVIES));
    g_infoManager.SetLibraryBool(LIBRARY_HAS_TVSHOWS, HasContent(VIDEODB_CONTENT_TVSHOWS));
    g_infoManager.SetLibraryBool(LIBRARY_HAS_MUSICVIDEOS, HasContent(VIDEODB_CONTENT_MUSICVIDEOS));
    return true;
  }
  return false;
}

void CVideoDatabase::DeleteThumbForItem(const CStdString& strPath, bool bFolder, int idEpisode)
{
  CFileItem item(strPath,bFolder);
  if (idEpisode > 0)
  {
    item.m_strPath = item.GetVideoInfoTag()->m_strFileNameAndPath;
    if (CFile::Exists(item.GetCachedEpisodeThumb()))
      XFILE::CFile::Delete(item.GetCachedEpisodeThumb());
    else
      XFILE::CFile::Delete(item.GetCachedVideoThumb());
  }
  else
  {
    XFILE::CFile::Delete(item.GetCachedVideoThumb());
    XFILE::CFile::Delete(item.GetCachedFanart());
  }

  // tell our GUI to completely reload all controls (as some of them
  // are likely to have had this image in use so will need refreshing)
  CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_REFRESH_THUMBS);
  g_windowManager.SendThreadMessage(msg);
}

void CVideoDatabase::SetDetail(const CStdString& strDetail, int id, int field,
                               VIDEODB_CONTENT_TYPE type)
{
  if (NULL == m_pDB.get()) return;
  if (NULL == m_pDS.get()) return;

  CStdString strTable, strField;
  if (type == VIDEODB_CONTENT_MOVIES)
  {
    strTable = "movies";
    strField = "idMovie";
  }
  if (type == VIDEODB_CONTENT_TVSHOWS)
  {
    strTable = "tvshows";
    strField = "idShow";
  }
  if (type == VIDEODB_CONTENT_MUSICVIDEOS)
  {
    strTable = "musicvideos";
    strField = "idMVideo";
  }

  if (strTable.IsEmpty())
    return;

  CStdString strSQL = FormatSQL("update %s set c%02u='%s' where %s=%u",
                                strTable.c_str(),field,strField.c_str(),id);
  m_pDS->exec(strSQL.c_str());
}

CStdString CVideoDatabase::GetSafeFile(const CStdString &dir, const CStdString &name) const
{
  CStdString safeThumb(name);
  safeThumb.Replace(' ', '_');
  return CUtil::AddFileToFolder(dir, safeThumb);
}
