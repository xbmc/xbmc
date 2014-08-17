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

#if (defined HAVE_CONFIG_H) && (!defined TARGET_WINDOWS)
  #include "config.h"
#endif

#include "ApplicationMessenger.h"
#include "threads/SystemClock.h"
#include "VideoDatabase.h"
#include "video/windows/GUIWindowVideoBase.h"
#include "utils/RegExp.h"
#include "addons/AddonManager.h"
#include "GUIInfoManager.h"
#include "Util.h"
#include "utils/URIUtils.h"
#include "utils/XMLUtils.h"
#include "GUIPassword.h"
#include "filesystem/StackDirectory.h"
#include "filesystem/MultiPathDirectory.h"
#include "VideoInfoScanner.h"
#include "guilib/GUIWindowManager.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "dialogs/GUIDialogExtendedProgressBar.h"
#include "dialogs/GUIDialogProgress.h"
#include "dialogs/GUIDialogYesNo.h"
#include "FileItem.h"
#include "profiles/ProfilesManager.h"
#include "settings/AdvancedSettings.h"
#include "settings/MediaSettings.h"
#include "settings/MediaSourceSettings.h"
#include "settings/Settings.h"
#include "utils/StringUtils.h"
#include "guilib/LocalizeStrings.h"
#include "utils/TimeUtils.h"
#include "utils/log.h"
#include "TextureCache.h"
#include "addons/AddonInstaller.h"
#include "interfaces/AnnouncementManager.h"
#include "dbwrappers/dataset.h"
#include "utils/LabelFormatter.h"
#include "XBDateTime.h"
#include "URL.h"
#include "video/VideoDbUrl.h"
#include "playlists/SmartPlayList.h"
#include "utils/GroupUtils.h"
#include "Application.h"

using namespace std;
using namespace dbiplus;
using namespace XFILE;
using namespace VIDEO;
using namespace ADDON;

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

void CVideoDatabase::CreateTables()
{
  CLog::Log(LOGINFO, "create bookmark table");
  m_pDS->exec("CREATE TABLE bookmark ( idBookmark integer primary key, idFile integer, timeInSeconds double, totalTimeInSeconds double, thumbNailImage text, player text, playerState text, type integer)\n");

  CLog::Log(LOGINFO, "create settings table");
  m_pDS->exec("CREATE TABLE settings ( idFile integer, Deinterlace bool,"
              "ViewMode integer,ZoomAmount float, PixelRatio float, VerticalShift float, AudioStream integer, SubtitleStream integer,"
              "SubtitleDelay float, SubtitlesOn bool, Brightness float, Contrast float, Gamma float,"
              "VolumeAmplification float, AudioDelay float, OutputToAllSpeakers bool, ResumeTime integer, Crop bool, CropLeft integer,"
              "CropRight integer, CropTop integer, CropBottom integer, Sharpness float, NoiseReduction float, NonLinStretch bool, PostProcess bool,"
              "ScalingMethod integer, DeinterlaceMode integer, StereoMode integer, StereoInvert bool)\n");

  CLog::Log(LOGINFO, "create stacktimes table");
  m_pDS->exec("CREATE TABLE stacktimes (idFile integer, times text)\n");

  CLog::Log(LOGINFO, "create genre table");
  m_pDS->exec("CREATE TABLE genre ( idGenre integer primary key, strGenre text)\n");

  CLog::Log(LOGINFO, "create genrelinkmovie table");
  m_pDS->exec("CREATE TABLE genrelinkmovie ( idGenre integer, idMovie integer)\n");

  CLog::Log(LOGINFO, "create country table");
  m_pDS->exec("CREATE TABLE country ( idCountry integer primary key, strCountry text)\n");

  CLog::Log(LOGINFO, "create countrylinkmovie table");
  m_pDS->exec("CREATE TABLE countrylinkmovie ( idCountry integer, idMovie integer)\n");

  CLog::Log(LOGINFO, "create movie table");
  CStdString columns = "CREATE TABLE movie ( idMovie integer primary key, idFile integer";

  for (int i = 0; i < VIDEODB_MAX_COLUMNS; i++)
    columns += StringUtils::Format(",c%02d text", i);

  columns += ", idSet integer)";
  m_pDS->exec(columns.c_str());

  CLog::Log(LOGINFO, "create actorlinkmovie table");
  m_pDS->exec("CREATE TABLE actorlinkmovie ( idActor integer, idMovie integer, strRole text, iOrder integer)\n");

  CLog::Log(LOGINFO, "create directorlinkmovie table");
  m_pDS->exec("CREATE TABLE directorlinkmovie ( idDirector integer, idMovie integer)\n");

  CLog::Log(LOGINFO, "create writerlinkmovie table");
  m_pDS->exec("CREATE TABLE writerlinkmovie ( idWriter integer, idMovie integer)\n");

  CLog::Log(LOGINFO, "create actors table");
  m_pDS->exec("CREATE TABLE actors ( idActor integer primary key, strActor text, strThumb text )\n");

  CLog::Log(LOGINFO, "create path table");
  m_pDS->exec("CREATE TABLE path ( idPath integer primary key, strPath text, strContent text, strScraper text, strHash text, scanRecursive integer, useFolderNames bool, strSettings text, noUpdate bool, exclude bool, dateAdded text, idParentPath integer)");

  CLog::Log(LOGINFO, "create files table");
  m_pDS->exec("CREATE TABLE files ( idFile integer primary key, idPath integer, strFilename text, playCount integer, lastPlayed text, dateAdded text)");

  CLog::Log(LOGINFO, "create tvshow table");
  columns = "CREATE TABLE tvshow ( idShow integer primary key";

  for (int i = 0; i < VIDEODB_MAX_COLUMNS; i++)
    columns += StringUtils::Format(",c%02d text", i);;

  columns += ")";
  m_pDS->exec(columns.c_str());

  CLog::Log(LOGINFO, "create directorlinktvshow table");
  m_pDS->exec("CREATE TABLE directorlinktvshow ( idDirector integer, idShow integer)\n");

  CLog::Log(LOGINFO, "create actorlinktvshow table");
  m_pDS->exec("CREATE TABLE actorlinktvshow ( idActor integer, idShow integer, strRole text, iOrder integer)\n");

  CLog::Log(LOGINFO, "create studiolinktvshow table");
  m_pDS->exec("CREATE TABLE studiolinktvshow ( idStudio integer, idShow integer)\n");

  CLog::Log(LOGINFO, "create episode table");
  columns = "CREATE TABLE episode ( idEpisode integer primary key, idFile integer";
  for (int i = 0; i < VIDEODB_MAX_COLUMNS; i++)
  {
    CStdString column;
    if ( i == VIDEODB_ID_EPISODE_SEASON || i == VIDEODB_ID_EPISODE_EPISODE || i == VIDEODB_ID_EPISODE_BOOKMARK)
      column = StringUtils::Format(",c%02d varchar(24)", i);
    else
      column = StringUtils::Format(",c%02d text", i);

    columns += column;
  }
  columns += ", idShow integer)";
  m_pDS->exec(columns.c_str());

  CLog::Log(LOGINFO, "create tvshowlinkpath table");
  m_pDS->exec("CREATE TABLE tvshowlinkpath (idShow integer, idPath integer)\n");

  CLog::Log(LOGINFO, "create actorlinkepisode table");
  m_pDS->exec("CREATE TABLE actorlinkepisode ( idActor integer, idEpisode integer, strRole text, iOrder integer)\n");

  CLog::Log(LOGINFO, "create directorlinkepisode table");
  m_pDS->exec("CREATE TABLE directorlinkepisode ( idDirector integer, idEpisode integer)\n");

  CLog::Log(LOGINFO, "create writerlinkepisode table");
  m_pDS->exec("CREATE TABLE writerlinkepisode ( idWriter integer, idEpisode integer)\n");

  CLog::Log(LOGINFO, "create genrelinktvshow table");
  m_pDS->exec("CREATE TABLE genrelinktvshow ( idGenre integer, idShow integer)\n");

  CLog::Log(LOGINFO, "create movielinktvshow table");
  m_pDS->exec("CREATE TABLE movielinktvshow ( idMovie integer, IdShow integer)\n");

  CLog::Log(LOGINFO, "create studio table");
  m_pDS->exec("CREATE TABLE studio ( idStudio integer primary key, strStudio text)\n");

  CLog::Log(LOGINFO, "create studiolinkmovie table");
  m_pDS->exec("CREATE TABLE studiolinkmovie ( idStudio integer, idMovie integer)\n");

  CLog::Log(LOGINFO, "create musicvideo table");
  columns = "CREATE TABLE musicvideo ( idMVideo integer primary key, idFile integer";
  for (int i = 0; i < VIDEODB_MAX_COLUMNS; i++)
    columns += StringUtils::Format(",c%02d text", i);;

  columns += ")";
  m_pDS->exec(columns.c_str());

  CLog::Log(LOGINFO, "create artistlinkmusicvideo table");
  m_pDS->exec("CREATE TABLE artistlinkmusicvideo ( idArtist integer, idMVideo integer)\n");

  CLog::Log(LOGINFO, "create genrelinkmusicvideo table");
  m_pDS->exec("CREATE TABLE genrelinkmusicvideo ( idGenre integer, idMVideo integer)\n");

  CLog::Log(LOGINFO, "create studiolinkmusicvideo table");
  m_pDS->exec("CREATE TABLE studiolinkmusicvideo ( idStudio integer, idMVideo integer)\n");

  CLog::Log(LOGINFO, "create directorlinkmusicvideo table");
  m_pDS->exec("CREATE TABLE directorlinkmusicvideo ( idDirector integer, idMVideo integer)\n");

  CLog::Log(LOGINFO, "create streaminfo table");
  m_pDS->exec("CREATE TABLE streamdetails (idFile integer, iStreamType integer, "
    "strVideoCodec text, fVideoAspect float, iVideoWidth integer, iVideoHeight integer, "
    "strAudioCodec text, iAudioChannels integer, strAudioLanguage text, strSubtitleLanguage text, iVideoDuration integer, strStereoMode text)");

 CLog::Log(LOGINFO, "create sets table");
  m_pDS->exec("CREATE TABLE sets ( idSet integer primary key, strSet text)\n");

  CLog::Log(LOGINFO, "create seasons table");
  m_pDS->exec("CREATE TABLE seasons ( idSeason integer primary key, idShow integer, season integer)");

  CLog::Log(LOGINFO, "create art table");
  m_pDS->exec("CREATE TABLE art(art_id INTEGER PRIMARY KEY, media_id INTEGER, media_type TEXT, type TEXT, url TEXT)");

  CLog::Log(LOGINFO, "create tag table");
  m_pDS->exec("CREATE TABLE tag (idTag integer primary key, strTag text)");

  CLog::Log(LOGINFO, "create taglinks table");
  m_pDS->exec("CREATE TABLE taglinks (idTag integer, idMedia integer, media_type TEXT)");
}

void CVideoDatabase::CreateAnalytics()
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

  CLog::Log(LOGINFO, "%s - creating indicies", __FUNCTION__);
  m_pDS->exec("CREATE INDEX ix_bookmark ON bookmark (idFile, type)");
  m_pDS->exec("CREATE UNIQUE INDEX ix_settings ON settings ( idFile )\n");
  m_pDS->exec("CREATE UNIQUE INDEX ix_stacktimes ON stacktimes ( idFile )\n");
  m_pDS->exec("CREATE INDEX ix_path ON path ( strPath(255) )");
  m_pDS->exec("CREATE INDEX ix_path2 ON path ( idParentPath )");
  m_pDS->exec("CREATE INDEX ix_files ON files ( idPath, strFilename(255) )");

  m_pDS->exec("CREATE UNIQUE INDEX ix_genrelinkmovie_1 ON genrelinkmovie ( idGenre, idMovie)\n");
  m_pDS->exec("CREATE UNIQUE INDEX ix_genrelinkmovie_2 ON genrelinkmovie ( idMovie, idGenre)\n");
  m_pDS->exec("CREATE UNIQUE INDEX ix_countrylinkmovie_1 ON countrylinkmovie ( idCountry, idMovie)\n");
  m_pDS->exec("CREATE UNIQUE INDEX ix_countrylinkmovie_2 ON countrylinkmovie ( idMovie, idCountry)\n");
  m_pDS->exec("CREATE UNIQUE INDEX ix_movie_file_1 ON movie (idFile, idMovie)");
  m_pDS->exec("CREATE UNIQUE INDEX ix_movie_file_2 ON movie (idMovie, idFile)");
  m_pDS->exec("CREATE UNIQUE INDEX ix_actorlinkmovie_1 ON actorlinkmovie ( idActor, idMovie )\n");
  m_pDS->exec("CREATE UNIQUE INDEX ix_actorlinkmovie_2 ON actorlinkmovie ( idMovie, idActor )\n");
  m_pDS->exec("CREATE UNIQUE INDEX ix_directorlinkmovie_1 ON directorlinkmovie ( idDirector, idMovie )\n");
  m_pDS->exec("CREATE UNIQUE INDEX ix_directorlinkmovie_2 ON directorlinkmovie ( idMovie, idDirector )\n");
  m_pDS->exec("CREATE UNIQUE INDEX ix_writerlinkmovie_1 ON writerlinkmovie ( idWriter, idMovie )\n");
  m_pDS->exec("CREATE UNIQUE INDEX ix_writerlinkmovie_2 ON writerlinkmovie ( idMovie, idWriter )\n");
  m_pDS->exec("CREATE UNIQUE INDEX ix_studiolinkmovie_1 ON studiolinkmovie ( idStudio, idMovie)\n");
  m_pDS->exec("CREATE UNIQUE INDEX ix_studiolinkmovie_2 ON studiolinkmovie ( idMovie, idStudio)\n");

  m_pDS->exec("CREATE UNIQUE INDEX ix_directorlinktvshow_1 ON directorlinktvshow ( idDirector, idShow )\n");
  m_pDS->exec("CREATE UNIQUE INDEX ix_directorlinktvshow_2 ON directorlinktvshow ( idShow, idDirector )\n");
  m_pDS->exec("CREATE UNIQUE INDEX ix_actorlinktvshow_1 ON actorlinktvshow ( idActor, idShow )\n");
  m_pDS->exec("CREATE UNIQUE INDEX ix_actorlinktvshow_2 ON actorlinktvshow ( idShow, idActor )\n");
  m_pDS->exec("CREATE UNIQUE INDEX ix_studiolinktvshow_1 ON studiolinktvshow ( idStudio, idShow)\n");
  m_pDS->exec("CREATE UNIQUE INDEX ix_studiolinktvshow_2 ON studiolinktvshow ( idShow, idStudio)\n");
  m_pDS->exec("CREATE UNIQUE INDEX ix_tvshowlinkpath_1 ON tvshowlinkpath ( idShow, idPath )\n");
  m_pDS->exec("CREATE UNIQUE INDEX ix_tvshowlinkpath_2 ON tvshowlinkpath ( idPath, idShow )\n");
  m_pDS->exec("CREATE UNIQUE INDEX ix_movielinktvshow_1 ON movielinktvshow ( idShow, idMovie)\n");
  m_pDS->exec("CREATE UNIQUE INDEX ix_movielinktvshow_2 ON movielinktvshow ( idMovie, idShow)\n");

  m_pDS->exec("CREATE UNIQUE INDEX ix_episode_file_1 on episode (idEpisode, idFile)");
  m_pDS->exec("CREATE UNIQUE INDEX id_episode_file_2 on episode (idFile, idEpisode)");
  CStdString createColIndex = StringUtils::Format("CREATE INDEX ix_episode_season_episode on episode (c%02d, c%02d)", VIDEODB_ID_EPISODE_SEASON, VIDEODB_ID_EPISODE_EPISODE);
  m_pDS->exec(createColIndex.c_str());
  createColIndex = StringUtils::Format("CREATE INDEX ix_episode_bookmark on episode (c%02d)", VIDEODB_ID_EPISODE_BOOKMARK);
  m_pDS->exec(createColIndex.c_str());
  m_pDS->exec("CREATE INDEX ix_episode_show1 on episode(idEpisode,idShow)");
  m_pDS->exec("CREATE INDEX ix_episode_show2 on episode(idShow,idEpisode)");
  m_pDS->exec("CREATE UNIQUE INDEX ix_actorlinkepisode_1 ON actorlinkepisode ( idActor, idEpisode )\n");
  m_pDS->exec("CREATE UNIQUE INDEX ix_actorlinkepisode_2 ON actorlinkepisode ( idEpisode, idActor )\n");
  m_pDS->exec("CREATE UNIQUE INDEX ix_directorlinkepisode_1 ON directorlinkepisode ( idDirector, idEpisode )\n");
  m_pDS->exec("CREATE UNIQUE INDEX ix_directorlinkepisode_2 ON directorlinkepisode ( idEpisode, idDirector )\n");
  m_pDS->exec("CREATE UNIQUE INDEX ix_writerlinkepisode_1 ON writerlinkepisode ( idWriter, idEpisode )\n");
  m_pDS->exec("CREATE UNIQUE INDEX ix_writerlinkepisode_2 ON writerlinkepisode ( idEpisode, idWriter )\n");
  m_pDS->exec("CREATE UNIQUE INDEX ix_genrelinktvshow_1 ON genrelinktvshow ( idGenre, idShow)\n");
  m_pDS->exec("CREATE UNIQUE INDEX ix_genrelinktvshow_2 ON genrelinktvshow ( idShow, idGenre)\n");

  m_pDS->exec("CREATE UNIQUE INDEX ix_musicvideo_file_1 on musicvideo (idMVideo, idFile)");
  m_pDS->exec("CREATE UNIQUE INDEX ix_musicvideo_file_2 on musicvideo (idFile, idMVideo)");
  m_pDS->exec("CREATE UNIQUE INDEX ix_artistlinkmusicvideo_1 ON artistlinkmusicvideo ( idArtist, idMVideo)\n");
  m_pDS->exec("CREATE UNIQUE INDEX ix_artistlinkmusicvideo_2 ON artistlinkmusicvideo ( idMVideo, idArtist)\n");
  m_pDS->exec("CREATE UNIQUE INDEX ix_genrelinkmusicvideo_1 ON genrelinkmusicvideo ( idGenre, idMVideo)\n");
  m_pDS->exec("CREATE UNIQUE INDEX ix_genrelinkmusicvideo_2 ON genrelinkmusicvideo ( idMVideo, idGenre)\n");
  m_pDS->exec("CREATE UNIQUE INDEX ix_studiolinkmusicvideo_1 ON studiolinkmusicvideo ( idStudio, idMVideo)\n");
  m_pDS->exec("CREATE UNIQUE INDEX ix_studiolinkmusicvideo_2 ON studiolinkmusicvideo ( idMVideo, idStudio)\n");
  m_pDS->exec("CREATE UNIQUE INDEX ix_directorlinkmusicvideo_1 ON directorlinkmusicvideo ( idDirector, idMVideo )\n");
  m_pDS->exec("CREATE UNIQUE INDEX ix_directorlinkmusicvideo_2 ON directorlinkmusicvideo ( idMVideo, idDirector )\n");

  m_pDS->exec("CREATE INDEX ixMovieBasePath ON movie ( c23(12) )");
  m_pDS->exec("CREATE INDEX ixMusicVideoBasePath ON musicvideo ( c14(12) )");
  m_pDS->exec("CREATE INDEX ixEpisodeBasePath ON episode ( c19(12) )");

  m_pDS->exec("CREATE INDEX ix_streamdetails ON streamdetails (idFile)");
  m_pDS->exec("CREATE INDEX ix_seasons ON seasons (idShow, season)");
  m_pDS->exec("CREATE INDEX ix_art ON art(media_id, media_type(20), type(20))");

  m_pDS->exec("CREATE UNIQUE INDEX ix_tag_1 ON tag (strTag(255))");
  m_pDS->exec("CREATE UNIQUE INDEX ix_taglinks_1 ON taglinks (idTag, media_type(20), idMedia)");
  m_pDS->exec("CREATE UNIQUE INDEX ix_taglinks_2 ON taglinks (idMedia, media_type(20), idTag)");
  m_pDS->exec("CREATE INDEX ix_taglinks_3 ON taglinks (media_type(20))");

  CLog::Log(LOGINFO, "%s - creating triggers", __FUNCTION__);
  m_pDS->exec("CREATE TRIGGER delete_movie AFTER DELETE ON movie FOR EACH ROW BEGIN "
              "DELETE FROM genrelinkmovie WHERE idMovie=old.idMovie; "
              "DELETE FROM actorlinkmovie WHERE idMovie=old.idMovie; "
              "DELETE FROM directorlinkmovie WHERE idMovie=old.idMovie; "
              "DELETE FROM studiolinkmovie WHERE idMovie=old.idMovie; "
              "DELETE FROM countrylinkmovie WHERE idMovie=old.idMovie; "
              "DELETE FROM writerlinkmovie WHERE idMovie=old.idMovie; "
              "DELETE FROM movielinktvshow WHERE idMovie=old.idMovie; "
              "DELETE FROM art WHERE media_id=old.idMovie AND media_type='movie'; "
              "DELETE FROM taglinks WHERE idMedia=old.idMovie AND media_type='movie'; "
              "END");
  m_pDS->exec("CREATE TRIGGER delete_tvshow AFTER DELETE ON tvshow FOR EACH ROW BEGIN "
              "DELETE FROM actorlinktvshow WHERE idShow=old.idShow; "
              "DELETE FROM directorlinktvshow WHERE idShow=old.idShow; "
              "DELETE FROM tvshowlinkpath WHERE idShow=old.idShow; "
              "DELETE FROM genrelinktvshow WHERE idShow=old.idShow; "
              "DELETE FROM movielinktvshow WHERE idShow=old.idShow; "
              "DELETE FROM seasons WHERE idShow=old.idShow; "
              "DELETE FROM art WHERE media_id=old.idShow AND media_type='tvshow'; "
              "DELETE FROM taglinks WHERE idMedia=old.idShow AND media_type='tvshow'; "
              "END");
  m_pDS->exec("CREATE TRIGGER delete_musicvideo AFTER DELETE ON musicvideo FOR EACH ROW BEGIN "
              "DELETE FROM artistlinkmusicvideo WHERE idMVideo=old.idMVideo; "
              "DELETE FROM directorlinkmusicvideo WHERE idMVideo=old.idMVideo; "
              "DELETE FROM genrelinkmusicvideo WHERE idMVideo=old.idMVideo; "
              "DELETE FROM studiolinkmusicvideo WHERE idMVideo=old.idMVideo; "
              "DELETE FROM art WHERE media_id=old.idMVideo AND media_type='musicvideo'; "
              "DELETE FROM taglinks WHERE idMedia=old.idMVideo AND media_type='musicvideo'; "
              "END");
  m_pDS->exec("CREATE TRIGGER delete_episode AFTER DELETE ON episode FOR EACH ROW BEGIN "
              "DELETE FROM actorlinkepisode WHERE idEpisode=old.idEpisode; "
              "DELETE FROM directorlinkepisode WHERE idEpisode=old.idEpisode; "
              "DELETE FROM writerlinkepisode WHERE idEpisode=old.idEpisode; "
              "DELETE FROM art WHERE media_id=old.idEpisode AND media_type='episode'; "
              "END");
  m_pDS->exec("CREATE TRIGGER delete_season AFTER DELETE ON seasons FOR EACH ROW BEGIN "
              "DELETE FROM art WHERE media_id=old.idSeason AND media_type='season'; "
              "END");
  m_pDS->exec("CREATE TRIGGER delete_set AFTER DELETE ON sets FOR EACH ROW BEGIN "
              "DELETE FROM art WHERE media_id=old.idSet AND media_type='set'; "
              "END");
  m_pDS->exec("CREATE TRIGGER delete_person AFTER DELETE ON actors FOR EACH ROW BEGIN "
              "DELETE FROM art WHERE media_id=old.idActor AND media_type IN ('actor','artist','writer','director'); "
              "END");
  m_pDS->exec("CREATE TRIGGER delete_tag AFTER DELETE ON taglinks FOR EACH ROW BEGIN "
              "DELETE FROM tag WHERE idTag=old.idTag AND idTag NOT IN (SELECT DISTINCT idTag FROM taglinks); "
              "END");

  CreateViews();
}

void CVideoDatabase::CreateViews()
{
  CLog::Log(LOGINFO, "create episodeview");
  CStdString episodeview = PrepareSQL("CREATE VIEW episodeview AS SELECT "
                                      "  episode.*,"
                                      "  files.strFileName AS strFileName,"
                                      "  path.strPath AS strPath,"
                                      "  files.playCount AS playCount,"
                                      "  files.lastPlayed AS lastPlayed,"
                                      "  files.dateAdded AS dateAdded,"
                                      "  tvshow.c%02d AS strTitle,"
                                      "  tvshow.c%02d AS strStudio,"
                                      "  tvshow.c%02d AS premiered,"
                                      "  tvshow.c%02d AS mpaa,"
                                      "  bookmark.timeInSeconds AS resumeTimeInSeconds, "
                                      "  bookmark.totalTimeInSeconds AS totalTimeInSeconds, "
                                      "  seasons.idSeason AS idSeason "
                                      "FROM episode"
                                      "  JOIN files ON"
                                      "    files.idFile=episode.idFile"
                                      "  JOIN tvshow ON"
                                      "    tvshow.idShow=episode.idShow"
                                      "  LEFT JOIN seasons ON"
                                      "    seasons.idShow=episode.idShow AND seasons.season=episode.c%02d"
                                      "  JOIN path ON"
                                      "    files.idPath=path.idPath"
                                      "  LEFT JOIN bookmark ON"
                                      "    bookmark.idFile=episode.idFile AND bookmark.type=1", VIDEODB_ID_TV_TITLE, VIDEODB_ID_TV_STUDIOS, VIDEODB_ID_TV_PREMIERED, VIDEODB_ID_TV_MPAA,VIDEODB_ID_EPISODE_SEASON);
  m_pDS->exec(episodeview.c_str());

  /* NOTE: The tvshowview needs to have "GROUP BY tvshow.idShow" added to any usage if you wish to
           avoid duplicates due to multiple paths per tvshow (from the join on tvshowlinkpath) */
  CLog::Log(LOGINFO, "create tvshowcounts");
  CStdString tvshowcounts = PrepareSQL("CREATE VIEW tvshowcounts AS SELECT "
                                       "      tvshow.idShow AS idShow,"
                                       "      MAX(files.lastPlayed) AS lastPlayed,"
                                       "      NULLIF(COUNT(episode.c12), 0) AS totalCount,"
                                       "      COUNT(files.playCount) AS watchedcount,"
                                       "      NULLIF(COUNT(DISTINCT(episode.c12)), 0) AS totalSeasons, "
                                       "      MAX(files.dateAdded) as dateAdded "
                                       "    FROM tvshow"
                                       "      LEFT JOIN episode ON"
                                       "        episode.idShow=tvshow.idShow"
                                       "      LEFT JOIN files ON"
                                       "        files.idFile=episode.idFile "
                                       "    GROUP BY tvshow.idShow");
  m_pDS->exec(tvshowcounts.c_str());

  CLog::Log(LOGINFO, "create tvshowview");
  CStdString tvshowview = PrepareSQL("CREATE VIEW tvshowview AS SELECT "
                                     "  tvshow.*,"
                                     "  path.idParentPath AS idParentPath,"
                                     "  path.strPath AS strPath,"
                                     "  tvshowcounts.dateAdded AS dateAdded,"
                                     "  lastPlayed, totalCount, watchedcount, totalSeasons "
                                     "FROM tvshow"
                                     "  LEFT JOIN tvshowlinkpath ON"
                                     "    tvshowlinkpath.idShow=tvshow.idShow"
                                     "  LEFT JOIN path ON"
                                     "    path.idPath=tvshowlinkpath.idPath"
                                     "  INNER JOIN tvshowcounts ON"
                                     "  tvshow.idShow = tvshowcounts.idShow");
  m_pDS->exec(tvshowview.c_str());

  CLog::Log(LOGINFO, "create seasonview");
  CStdString seasonview = PrepareSQL("CREATE VIEW seasonview AS SELECT "
                                     "  seasons.*, "
                                     "  tvshowview.strPath AS strPath,"
                                     "  tvshowview.c%02d AS showTitle,"
                                     "  tvshowview.c%02d AS plot,"
                                     "  tvshowview.c%02d AS premiered,"
                                     "  tvshowview.c%02d AS genre,"
                                     "  tvshowview.c%02d AS strStudio,"
                                     "  tvshowview.c%02d AS mpaa,"
                                     "  count(DISTINCT episodeview.idEpisode) AS episodes,"
                                     "  count(files.playCount) AS playCount "
                                     "FROM seasons"
                                     "  JOIN tvshowview ON"
                                     "    tvshowview.idShow = seasons.idShow"
                                     "  JOIN episodeview ON"
                                     "    episodeview.idShow = seasons.idShow AND episodeview.c%02d = seasons.season"
                                     "  JOIN files ON"
                                     "    files.idFile = episodeview.idFile "
                                     "GROUP BY seasons.idSeason",
                                     VIDEODB_ID_TV_TITLE, VIDEODB_ID_TV_PLOT, VIDEODB_ID_TV_PREMIERED,
                                     VIDEODB_ID_TV_GENRE, VIDEODB_ID_TV_STUDIOS, VIDEODB_ID_TV_MPAA,
                                     VIDEODB_ID_EPISODE_SEASON);
  m_pDS->exec(seasonview.c_str());

  CLog::Log(LOGINFO, "create musicvideoview");
  m_pDS->exec("CREATE VIEW musicvideoview AS SELECT"
              "  musicvideo.*,"
              "  files.strFileName as strFileName,"
              "  path.strPath as strPath,"
              "  files.playCount as playCount,"
              "  files.lastPlayed as lastPlayed,"
              "  files.dateAdded as dateAdded, "
              "  bookmark.timeInSeconds AS resumeTimeInSeconds, "
              "  bookmark.totalTimeInSeconds AS totalTimeInSeconds "
              "FROM musicvideo"
              "  JOIN files ON"
              "    files.idFile=musicvideo.idFile"
              "  JOIN path ON"
              "    path.idPath=files.idPath"
              "  LEFT JOIN bookmark ON"
              "    bookmark.idFile=musicvideo.idFile AND bookmark.type=1");

  CLog::Log(LOGINFO, "create movieview");
  m_pDS->exec("CREATE VIEW movieview AS SELECT"
              "  movie.*,"
              "  sets.strSet AS strSet,"
              "  files.strFileName AS strFileName,"
              "  path.strPath AS strPath,"
              "  files.playCount AS playCount,"
              "  files.lastPlayed AS lastPlayed, "
              "  files.dateAdded AS dateAdded, "
              "  bookmark.timeInSeconds AS resumeTimeInSeconds, "
              "  bookmark.totalTimeInSeconds AS totalTimeInSeconds "
              "FROM movie"
              "  LEFT JOIN sets ON"
              "    sets.idSet = movie.idSet"
              "  JOIN files ON"
              "    files.idFile=movie.idFile"
              "  JOIN path ON"
              "    path.idPath=files.idPath"
              "  LEFT JOIN bookmark ON"
              "    bookmark.idFile=movie.idFile AND bookmark.type=1");
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
    if (URIUtils::IsStack(strPath) || StringUtils::StartsWithNoCase(strPath, "rar://") || StringUtils::StartsWithNoCase(strPath, "zip://"))
      URIUtils::GetParentPath(strPath,strPath1);

    URIUtils::AddSlashAtEnd(strPath1);

    strSQL=PrepareSQL("select idPath from path where strPath='%s'",strPath1.c_str());
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

bool CVideoDatabase::GetPaths(set<CStdString> &paths)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    paths.clear();

    // grab all paths with movie content set
    if (!m_pDS->query("select strPath,noUpdate from path"
                      " where (strContent = 'movies' or strContent = 'musicvideos')"
                      " and strPath NOT like 'multipath://%%'"
                      " order by strPath"))
      return false;

    while (!m_pDS->eof())
    {
      if (!m_pDS->fv("noUpdate").get_asBool())
        paths.insert(m_pDS->fv("strPath").get_asString());
      m_pDS->next();
    }
    m_pDS->close();

    // then grab all tvshow paths
    if (!m_pDS->query("select strPath,noUpdate from path"
                      " where ( strContent = 'tvshows'"
                      "       or idPath in (select idPath from tvshowlinkpath))"
                      " and strPath NOT like 'multipath://%%'"
                      " order by strPath"))
      return false;

    while (!m_pDS->eof())
    {
      if (!m_pDS->fv("noUpdate").get_asBool())
        paths.insert(m_pDS->fv("strPath").get_asString());
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
                       " and idPath NOT in (select idPath from files where strFileName like 'video_ts.ifo')" // dvd folders get stacked to a single item in parent folder
                       " and idPath NOT in (select idPath from files where strFileName like 'index.bdmv')" // bluray folders get stacked to a single item in parent folder
                       " and strPath NOT like 'multipath://%%'"
                       " and strContent NOT in ('movies', 'tvshows', 'None')" // these have been added above
                       " order by strPath"))

      return false;
    while (!m_pDS->eof())
    {
      if (!m_pDS->fv("noUpdate").get_asBool())
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

bool CVideoDatabase::GetPathsLinkedToTvShow(int idShow, vector<string> &paths)
{
  string sql;
  try
  {
    sql = PrepareSQL("SELECT strPath FROM path JOIN tvshowlinkpath ON tvshowlinkpath.idPath=path.idPath WHERE idShow=%i", idShow);
    m_pDS->query(sql.c_str());
    while (!m_pDS->eof())
    {
      paths.push_back(m_pDS->fv(0).get_asString());
      m_pDS->next();
    }
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s error during query: %s",__FUNCTION__, sql.c_str());
  }
  return false;
}

bool CVideoDatabase::GetPathsForTvShow(int idShow, set<int>& paths)
{
  CStdString strSQL;
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    strSQL = PrepareSQL("SELECT DISTINCT idPath FROM files JOIN episode ON episode.idFile=files.idFile WHERE episode.idShow=%i",idShow);
    m_pDS->query(strSQL.c_str());
    while (!m_pDS->eof())
    {
      paths.insert(m_pDS->fv(0).get_asInt());
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

int CVideoDatabase::RunQuery(const CStdString &sql)
{
  unsigned int time = XbmcThreads::SystemClockMillis();
  int rows = -1;
  if (m_pDS->query(sql.c_str()))
  {
    rows = m_pDS->num_rows();
    if (rows == 0)
      m_pDS->close();
  }
  CLog::Log(LOGDEBUG, "%s took %d ms for %d items query: %s", __FUNCTION__, XbmcThreads::SystemClockMillis() - time, rows, sql.c_str());
  return rows;
}

bool CVideoDatabase::GetSubPaths(const CStdString &basepath, vector< pair<int,string> >& subpaths)
{
  CStdString sql;
  try
  {
    if (!m_pDB.get() || !m_pDS.get())
      return false;

    CStdString path(basepath);
    URIUtils::AddSlashAtEnd(path);
    sql = PrepareSQL("SELECT idPath,strPath FROM path WHERE SUBSTR(strPath,1,%i)='%s'", StringUtils::utf8_strlen(path.c_str()), path.c_str());
    m_pDS->query(sql.c_str());
    while (!m_pDS->eof())
    {
      subpaths.push_back(make_pair(m_pDS->fv(0).get_asInt(), m_pDS->fv(1).get_asString()));
      m_pDS->next();
    }
    m_pDS->close();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s error during query: %s",__FUNCTION__, sql.c_str());
  }
  return false;
}

int CVideoDatabase::AddPath(const CStdString& strPath, const CStdString &parentPath /*= "" */, const CStdString &strDateAdded /*= "" */)
{
  CStdString strSQL;
  try
  {
    int idPath = GetPathId(strPath);
    if (idPath >= 0)
      return idPath; // already have the path

    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    CStdString strPath1(strPath);
    if (URIUtils::IsStack(strPath) || StringUtils::StartsWithNoCase(strPath, "rar://") || StringUtils::StartsWithNoCase(strPath, "zip://"))
      URIUtils::GetParentPath(strPath,strPath1);

    URIUtils::AddSlashAtEnd(strPath1);

    int idParentPath = GetPathId(parentPath.empty() ? (CStdString)URIUtils::GetParentPath(strPath1) : parentPath);

    // add the path
    if (idParentPath < 0)
    {
      if (!strDateAdded.empty())
        strSQL=PrepareSQL("insert into path (idPath, strPath, dateAdded) values (NULL, '%s', '%s')", strPath1.c_str(), strDateAdded.c_str());
      else
        strSQL=PrepareSQL("insert into path (idPath, strPath) values (NULL, '%s')", strPath1.c_str());
    }
    else
    {
      if (!strDateAdded.empty())
        strSQL=PrepareSQL("insert into path (idPath, strPath, dateAdded, idParentPath) values (NULL, '%s', '%s', %i)", strPath1.c_str(), strDateAdded.c_str(), idParentPath);
      else
        strSQL=PrepareSQL("insert into path (idPath, strPath, idParentPath) values (NULL, '%s', %i)", strPath1.c_str(), idParentPath);
    }
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

bool CVideoDatabase::GetSourcePath(const std::string &path, std::string &sourcePath)
{
  SScanSettings dummy;
  return GetSourcePath(path, sourcePath, dummy);
}

bool CVideoDatabase::GetSourcePath(const std::string &path, std::string &sourcePath, SScanSettings& settings)
{
  try
  {
    if (path.empty() ||
        m_pDB.get() == NULL || m_pDS.get() == NULL)
      return false;

    std::string strPath2;

    if (URIUtils::IsMultiPath(path))
      strPath2 = CMultiPathDirectory::GetFirstPath(path);
    else
      strPath2 = path;

    std::string strPath1 = URIUtils::GetDirectory(strPath2);
    int idPath = GetPathId(strPath1);

    if (idPath > -1)
    {
      // check if the given path already is a source itself
      std::string strSQL = PrepareSQL("SELECT path.useFolderNames, path.scanRecursive, path.noUpdate, path.exclude FROM path WHERE "
                                        "path.idPath = %i AND "
                                        "path.strContent IS NOT NULL AND path.strContent != '' AND "
                                        "path.strScraper IS NOT NULL AND path.strScraper != ''", idPath);
      if (m_pDS->query(strSQL.c_str()) && !m_pDS->eof())
      {
        settings.parent_name_root = settings.parent_name = m_pDS->fv(0).get_asBool();
        settings.recurse = m_pDS->fv(1).get_asInt();
        settings.noupdate = m_pDS->fv(2).get_asBool();
        settings.exclude = m_pDS->fv(3).get_asBool();

        m_pDS->close();
        sourcePath = path;
        return true;
      }
    }

    // look for parent paths until there is one which is a source
    CStdString strParent;
    bool found = false;
    while (URIUtils::GetParentPath(strPath1, strParent))
    {
      std::string strSQL = PrepareSQL("SELECT path.strContent, path.strScraper, path.scanRecursive, path.useFolderNames, path.noUpdate, path.exclude FROM path WHERE strPath = '%s'", strParent.c_str());
      if (m_pDS->query(strSQL.c_str()) && !m_pDS->eof())
      {
        std::string strContent = m_pDS->fv(0).get_asString();
        std::string strScraper = m_pDS->fv(1).get_asString();
        if (!strContent.empty() && !strScraper.empty())
        {
          settings.parent_name_root = settings.parent_name = m_pDS->fv(2).get_asBool();
          settings.recurse = m_pDS->fv(3).get_asInt();
          settings.noupdate = m_pDS->fv(4).get_asBool();
          settings.exclude = m_pDS->fv(5).get_asBool();
          found = true;
          break;
        }
      }

      strPath1 = strParent;
    }
    m_pDS->close();

    if (found)
    {
      sourcePath = strParent;
      return true;
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
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

    int idPath = AddPath(strPath);
    if (idPath < 0)
      return -1;

    CStdString strSQL=PrepareSQL("select idFile from files where strFileName='%s' and idPath=%i", strFileName.c_str(),idPath);

    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() > 0)
    {
      idFile = m_pDS->fv("idFile").get_asInt() ;
      m_pDS->close();
      return idFile;
    }
    m_pDS->close();

    strSQL=PrepareSQL("insert into files (idFile, idPath, strFileName) values(NULL, %i, '%s')", idPath, strFileName.c_str());
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

int CVideoDatabase::AddFile(const CFileItem& item)
{
  if (item.IsVideoDb() && item.HasVideoInfoTag())
  {
    if (item.GetVideoInfoTag()->m_iFileId != -1)
      return item.GetVideoInfoTag()->m_iFileId;
    else
      return AddFile(item.GetVideoInfoTag()->m_strFileNameAndPath);
  }
  return AddFile(item.GetPath());
}

void CVideoDatabase::UpdateFileDateAdded(int idFile, const CStdString& strFileNameAndPath)
{
  if (idFile < 0 || strFileNameAndPath.empty())
    return;
    
  CStdString strSQL = "";
  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    CStdString file = strFileNameAndPath;
    if (URIUtils::IsStack(strFileNameAndPath))
      file = CStackDirectory::GetFirstStackedFile(strFileNameAndPath);

    if (URIUtils::IsInArchive(file))
      file = CURL(file).GetHostName();

    CDateTime dateAdded;
    // Skip looking at the files ctime/mtime if defined by the user through as.xml
    if (g_advancedSettings.m_iVideoLibraryDateAdded > 0)
    {
      // Let's try to get the modification datetime
      struct __stat64 buffer;
      if (CFile::Stat(file, &buffer) == 0 && (buffer.st_mtime != 0 || buffer.st_ctime !=0))
      {
        time_t now = time(NULL);
        time_t addedTime;
        // Prefer the modification time if it's valid
        if (g_advancedSettings.m_iVideoLibraryDateAdded == 1)
        {
          if (buffer.st_mtime != 0 && (time_t)buffer.st_mtime <= now)
            addedTime = (time_t)buffer.st_mtime;
          else
            addedTime = (time_t)buffer.st_ctime;
        }
        // Use the newer of the creation and modification time
        else
        {
          addedTime = max((time_t)buffer.st_ctime, (time_t)buffer.st_mtime);
          // if the newer of the two dates is in the future, we try it with the older one
          if (addedTime > now)
            addedTime = min((time_t)buffer.st_ctime, (time_t)buffer.st_mtime);
        }

        // make sure the datetime does is not in the future
        if (addedTime <= now)
        {
          struct tm *time;
#ifdef HAVE_LOCALTIME_R
          struct tm result = {};
          time = localtime_r(&addedTime, &result);
#else
          time = localtime(&addedTime);
#endif
          if (time)
            dateAdded = *time;
        }
      }
    }

    if (!dateAdded.IsValid())
      dateAdded = CDateTime::GetCurrentDateTime();

    strSQL = PrepareSQL("update files set dateAdded='%s' where idFile=%d", dateAdded.GetAsDBDateTime().c_str(), idFile);
    m_pDS->exec(strSQL.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s unable to update dateadded for file (%s)", __FUNCTION__, strSQL.c_str());
  }
}

bool CVideoDatabase::SetPathHash(const CStdString &path, const CStdString &hash)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

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

bool CVideoDatabase::LinkMovieToTvshow(int idMovie, int idShow, bool bRemove)
{
   try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    if (bRemove) // delete link
    {
      CStdString strSQL=PrepareSQL("delete from movielinktvshow where idMovie=%i and idShow=%i", idMovie, idShow);
      m_pDS->exec(strSQL.c_str());
      return true;
    }

    CStdString strSQL=PrepareSQL("insert into movielinktvshow (idShow,idMovie) values (%i,%i)", idShow,idMovie);
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

    CStdString strSQL=PrepareSQL("select * from movielinktvshow where idMovie=%i", idMovie);
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

    CStdString strSQL=PrepareSQL("select * from movielinktvshow where idMovie=%i", idMovie);
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
    if (idPath >= 0)
    {
      CStdString strSQL;
      strSQL=PrepareSQL("select idFile from files where strFileName='%s' and idPath=%i", strFileName.c_str(),idPath);
      m_pDS->query(strSQL.c_str());
      if (m_pDS->num_rows() > 0)
      {
        int idFile = m_pDS->fv("files.idFile").get_asInt();
        m_pDS->close();
        return idFile;
      }
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
  if (item.IsVideoDb() && item.HasVideoInfoTag())
  {
    if (item.GetVideoInfoTag()->m_iFileId != -1)
      return item.GetVideoInfoTag()->m_iFileId;
    else
      return GetFileId(item.GetVideoInfoTag()->m_strFileNameAndPath);
  }
  return GetFileId(item.GetPath());
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
      strSQL=PrepareSQL("select idMovie from movie join files on files.idFile=movie.idFile where files.idPath=%i",idPath);
    else
      strSQL=PrepareSQL("select idMovie from movie where idFile=%i", idFile);

    CLog::Log(LOGDEBUG, "%s (%s), query = %s", __FUNCTION__, CURL::GetRedacted(strFilenameAndPath).c_str(), strSQL.c_str());
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

    strSQL=PrepareSQL("select idShow from tvshowlinkpath where tvshowlinkpath.idPath=%i",idPath);
    m_pDS->query(strSQL);
    if (!m_pDS->eof())
      iFound = 1;

    while (iFound == 0 && URIUtils::GetParentPath(strPath1, strParent))
    {
      strSQL=PrepareSQL("select idShow from path,tvshowlinkpath where tvshowlinkpath.idPath=path.idPath and strPath='%s'",strParent.c_str());
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

    CStdString strSQL=PrepareSQL("select idEpisode from episode where idFile=%i", idFile);

    CLog::Log(LOGDEBUG, "%s (%s), query = %s", __FUNCTION__, CURL::GetRedacted(strFilenameAndPath).c_str(), strSQL.c_str());
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

    CStdString strSQL=PrepareSQL("select idMVideo from musicvideo where idFile=%i", idFile);

    CLog::Log(LOGDEBUG, "%s (%s), query = %s", __FUNCTION__, CURL::GetRedacted(strFilenameAndPath).c_str(), strSQL.c_str());
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

    int idMovie = GetMovieId(strFilenameAndPath);
    if (idMovie < 0)
    {
      int idFile = AddFile(strFilenameAndPath);
      if (idFile < 0)
        return -1;
      UpdateFileDateAdded(idFile, strFilenameAndPath);
      CStdString strSQL=PrepareSQL("insert into movie (idMovie, idFile) values (NULL, %i)", idFile);
      m_pDS->exec(strSQL.c_str());
      idMovie = (int)m_pDS->lastinsertid();
    }

    return idMovie;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strFilenameAndPath.c_str());
  }
  return -1;
}

bool CVideoDatabase::AddPathToTvShow(int idShow, const std::string &path, const std::string &parentPath)
{
  // Check if this path is already added
  int idPath = GetPathId(path);
  if (idPath < 0)
  {
    // Get the creation datetime of the tvshow directory
    CDateTime dateAdded;

    // Skip looking at the files ctime/mtime if defined by the user through as.xml
    if (g_advancedSettings.m_iVideoLibraryDateAdded > 0)
    {
      struct __stat64 buffer;
      if (XFILE::CFile::Stat(path, &buffer) == 0)
      {
        time_t now = time(NULL);
        // Make sure we have a valid date (i.e. not in the future)
        if ((time_t)buffer.st_ctime <= now)
        {
          struct tm *time;
#ifdef HAVE_LOCALTIME_R
          struct tm result = {};
          time = localtime_r((const time_t*)&buffer.st_ctime, &result);
#else
          time = localtime((const time_t*)&buffer.st_ctime);
#endif
          if (time)
            dateAdded = *time;
        }
      }
    }

    if (!dateAdded.IsValid())
      dateAdded = CDateTime::GetCurrentDateTime();

    idPath = AddPath(path, parentPath, dateAdded.GetAsDBDateTime());
  }

  return ExecuteQuery(PrepareSQL("REPLACE INTO tvshowlinkpath(idShow, idPath) VALUES (%i,%i)", idShow, idPath));
}

int CVideoDatabase::AddTvShow()
{
  if (ExecuteQuery("INSERT INTO tvshow(idShow) VALUES(NULL)"))
    return m_pDS->lastinsertid();
  return -1;
}

//********************************************************************************************************************************
int CVideoDatabase::AddEpisode(int idShow, const CStdString& strFilenameAndPath)
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    int idFile = AddFile(strFilenameAndPath);
    if (idFile < 0)
      return -1;
    UpdateFileDateAdded(idFile, strFilenameAndPath);

    CStdString strSQL=PrepareSQL("insert into episode (idEpisode, idFile, idShow) values (NULL, %i, %i)", idFile, idShow);
    m_pDS->exec(strSQL.c_str());
    return (int)m_pDS->lastinsertid();
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

    int idMVideo = GetMusicVideoId(strFilenameAndPath);
    if (idMVideo < 0)
    {
      int idFile = AddFile(strFilenameAndPath);
      if (idFile < 0)
        return -1;
      UpdateFileDateAdded(idFile, strFilenameAndPath);
      CStdString strSQL=PrepareSQL("insert into musicvideo (idMVideo, idFile) values (NULL, %i)", idFile);
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
int CVideoDatabase::AddToTable(const CStdString& table, const CStdString& firstField, const CStdString& secondField, const CStdString& value)
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    CStdString strSQL = PrepareSQL("select %s from %s where %s like '%s'", firstField.c_str(), table.c_str(), secondField.c_str(), value.c_str());
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      // doesnt exists, add it
      strSQL = PrepareSQL("insert into %s (%s, %s) values(NULL, '%s')", table.c_str(), firstField.c_str(), secondField.c_str(), value.c_str());      
      m_pDS->exec(strSQL.c_str());
      int id = (int)m_pDS->lastinsertid();
      return id;
    }
    else
    {
      int id = m_pDS->fv(firstField).get_asInt();
      m_pDS->close();
      return id;
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, value.c_str() );
  }

  return -1;
}

int CVideoDatabase::AddSet(const CStdString& strSet)
{
  if (strSet.empty())
    return -1;

  return AddToTable("sets", "idSet", "strSet", strSet);
}

int CVideoDatabase::AddTag(const std::string& tag)
{
  if (tag.empty())
    return -1;

  return AddToTable("tag", "idTag", "strTag", tag);
}

int CVideoDatabase::AddGenre(const CStdString& strGenre)
{
  return AddToTable("genre", "idGenre", "strGenre", strGenre);
}

int CVideoDatabase::AddStudio(const CStdString& strStudio)
{
  return AddToTable("studio", "idStudio", "strStudio", strStudio);
}

//********************************************************************************************************************************
int CVideoDatabase::AddCountry(const CStdString& strCountry)
{
  return AddToTable("country", "idCountry", "strCountry", strCountry);
}

int CVideoDatabase::AddActor(const CStdString& strActor, const CStdString& thumbURLs, const CStdString &thumb)
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;
    int idActor = -1;
    CStdString strSQL=PrepareSQL("select idActor from actors where strActor like '%s'", strActor.c_str());
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      // doesnt exists, add it
      strSQL=PrepareSQL("insert into actors (idActor, strActor, strThumb) values( NULL, '%s','%s')", strActor.c_str(),thumbURLs.c_str());
      m_pDS->exec(strSQL.c_str());
      idActor = (int)m_pDS->lastinsertid();
    }
    else
    {
      idActor = m_pDS->fv("idActor").get_asInt();
      m_pDS->close();
      // update the thumb url's
      if (!thumbURLs.empty())
      {
        strSQL=PrepareSQL("update actors set strThumb='%s' where idActor=%i",thumbURLs.c_str(),idActor);
        m_pDS->exec(strSQL.c_str());
      }
    }
    // add artwork
    if (!thumb.empty())
      SetArtForItem(idActor, "actor", "thumb", thumb);
    return idActor;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strActor.c_str() );
  }
  return -1;
}



void CVideoDatabase::AddLinkToActor(const char *table, int actorID, const char *field, int secondID, const CStdString &role, int order)
{
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    CStdString strSQL=PrepareSQL("select * from actorlink%s where idActor=%i and id%s=%i", table, actorID, field, secondID);
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() == 0)
    {
      // doesnt exists, add it
      strSQL=PrepareSQL("insert into actorlink%s (idActor, id%s, strRole, iOrder) values(%i,%i,'%s',%i)", table, field, actorID, secondID, role.c_str(), order);
      m_pDS->exec(strSQL.c_str());
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
}

void CVideoDatabase::AddToLinkTable(const char *table, const char *firstField, int firstID, const char *secondField, int secondID, const char *typeField /* = NULL */, const char *type /* = NULL */)
{
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    CStdString strSQL = PrepareSQL("select * from %s where %s=%i and %s=%i", table, firstField, firstID, secondField, secondID);
    if (typeField != NULL && type != NULL)
      strSQL += PrepareSQL(" and %s='%s'", typeField, type);
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() == 0)
    {
      // doesnt exists, add it
      if (typeField == NULL || type == NULL)
        strSQL = PrepareSQL("insert into %s (%s,%s) values(%i,%i)", table, firstField, secondField, firstID, secondID);
      else
        strSQL = PrepareSQL("insert into %s (%s,%s,%s) values(%i,%i,'%s')", table, firstField, secondField, typeField, firstID, secondID, type);
      m_pDS->exec(strSQL.c_str());
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
}

void CVideoDatabase::RemoveFromLinkTable(const char *table, const char *firstField, int firstID, const char *secondField, int secondID, const char *typeField /* = NULL */, const char *type /* = NULL */)
{
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    CStdString strSQL = PrepareSQL("DELETE FROM %s WHERE %s = %i AND %s = %i", table, firstField, firstID, secondField, secondID);
    if (typeField != NULL && type != NULL)
      strSQL += PrepareSQL(" AND %s='%s'", typeField, type);
    m_pDS->exec(strSQL.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
}

void CVideoDatabase::UpdateLinkTable(int mediaId, const std::string& mediaType, const std::string& field, const std::vector<std::string>& values)
{
  std::string sql = PrepareSQL("delete from %slink%s where id%s=%i", field.c_str(), mediaType.c_str(), mediaType.c_str(), mediaId);
  m_pDS->exec(sql);

  for (std::vector<std::string>::const_iterator i = values.begin(); i != values.end(); ++i)
  {
    if (!i->empty())
    {
      int idValue = AddToTable(field, "id" + field, "str" + field, *i);
      if (idValue > -1)
        AddToLinkTable((field + "link" + mediaType).c_str(), ("id" + field).c_str(), idValue, ("id" + mediaType).c_str(), mediaId);
    }
  }
}

void CVideoDatabase::UpdateActorLinkTable(int mediaId, const std::string& mediaType, const std::string& field, const std::vector<std::string>& values)
{
  std::string sql = PrepareSQL("delete from %slink%s where id%s=%i", field.c_str(), mediaType.c_str(), mediaType.c_str(), mediaId);
  m_pDS->exec(sql);

  for (std::vector<std::string>::const_iterator i = values.begin(); i != values.end(); ++i)
  {
    if (!i->empty())
    {
      int idValue = AddActor(*i, "");
      if (idValue > -1)
        AddToLinkTable((field + "link" + mediaType).c_str(), ("id" + field).c_str(), idValue, ("id" + mediaType).c_str(), mediaId);
    }
  }
}

//****Tags****
void CVideoDatabase::AddTagToItem(int idMovie, int idTag, const std::string &type)
{
  if (type.empty())
    return;

  AddToLinkTable("taglinks", "idTag", idTag, "idMedia", idMovie, "media_type", type.c_str());
}

void CVideoDatabase::RemoveTagFromItem(int idItem, int idTag, const std::string &type)
{
  if (type.empty())
    return;

  RemoveFromLinkTable("taglinks", "idTag", idTag, "idMedia", idItem, "media_type", type.c_str());
}

void CVideoDatabase::RemoveTagsFromItem(int idItem, const std::string &type)
{
  if (type.empty())
    return;

  m_pDS2->exec(PrepareSQL("DELETE FROM taglinks WHERE idMedia=%d AND media_type='%s'", idItem, type.c_str()));
}

//****Actors****
void CVideoDatabase::AddCast(int idMedia, const char *table, const char *field, const std::vector< SActorInfo > &cast)
{
  if (cast.empty())
    return;

  int order = std::max_element(cast.begin(), cast.end())->order;
  for (CVideoInfoTag::iCast it = cast.begin(); it != cast.end(); ++it)
  {
    int idActor = AddActor(it->strName, it->thumbUrl.m_xml, it->thumb);
    AddLinkToActor(table, idActor, field, idMedia, it->strRole, it->order >= 0 ? it->order : ++order);
  }
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

//****Country****
void CVideoDatabase::AddCountryToMovie(int idMovie, int idCountry)
{
  AddToLinkTable("countrylinkmovie", "idCountry", idCountry, "idMovie", idMovie);
}

//********************************************************************************************************************************
bool CVideoDatabase::LoadVideoInfo(const CStdString& strFilenameAndPath, CVideoInfoTag& details)
{
  if (GetMovieInfo(strFilenameAndPath, details))
    return true;
  if (GetEpisodeInfo(strFilenameAndPath, details))
    return true;
  if (GetMusicVideoInfo(strFilenameAndPath, details))
    return true;
  if (GetFileInfo(strFilenameAndPath, details))
    return true;

  return false;
}

bool CVideoDatabase::HasMovieInfo(const CStdString& strFilenameAndPath)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    int idMovie = GetMovieId(strFilenameAndPath);
    return (idMovie > 0); // index of zero is also invalid
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
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strFilenameAndPath.c_str());
  }
  return false;
}

void CVideoDatabase::DeleteDetailsForTvShow(const CStdString& strPath)
{
  int idTvShow = GetTvShowId(strPath);
  if (idTvShow >= 0)
    DeleteDetailsForTvShow(idTvShow);
}

void CVideoDatabase::DeleteDetailsForTvShow(int idTvShow)
{
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    CStdString strSQL;
    strSQL=PrepareSQL("delete from genrelinktvshow where idShow=%i", idTvShow);
    m_pDS->exec(strSQL.c_str());

    strSQL=PrepareSQL("delete from actorlinktvshow where idShow=%i", idTvShow);
    m_pDS->exec(strSQL.c_str());

    strSQL=PrepareSQL("delete from directorlinktvshow where idShow=%i", idTvShow);
    m_pDS->exec(strSQL.c_str());

    strSQL=PrepareSQL("delete from studiolinktvshow where idShow=%i", idTvShow);
    m_pDS->exec(strSQL.c_str());

    // remove all info other than the id
    // we do this due to the way we have the link between the file + movie tables.

    std::vector<string> ids;
    for (int iType = VIDEODB_ID_TV_MIN + 1; iType < VIDEODB_ID_TV_MAX; iType++)
      ids.push_back(StringUtils::Format("c%02d=NULL", iType));

    strSQL = "update tvshow set ";
    strSQL += StringUtils::Join(ids, ", ");
    strSQL += PrepareSQL(" where idShow=%i", idTvShow);
    m_pDS->exec(strSQL.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%i) failed", __FUNCTION__, idTvShow);
  }
}

//********************************************************************************************************************************
void CVideoDatabase::GetMoviesByActor(const CStdString& strActor, CFileItemList& items)
{
  Filter filter;
  filter.join  = "LEFT JOIN actorlinkmovie ON actorlinkmovie.idMovie=movieview.idMovie "
                 "LEFT JOIN actors a ON a.idActor=actorlinkmovie.idActor "
                 "LEFT JOIN directorlinkmovie ON directorlinkmovie.idMovie=movieview.idMovie "
                 "LEFT JOIN actors d ON d.idActor=directorlinkmovie.idDirector";
  filter.where = PrepareSQL("a.strActor='%s' OR d.strActor='%s'", strActor.c_str(), strActor.c_str());
  filter.group = "movieview.idMovie";
  GetMoviesByWhere("videodb://movies/titles/", filter, items);
}

void CVideoDatabase::GetTvShowsByActor(const CStdString& strActor, CFileItemList& items)
{
  Filter filter;
  filter.join  = "LEFT JOIN actorlinktvshow ON actorlinktvshow.idShow=tvshowview.idShow "
                 "LEFT JOIN actors a ON a.idActor=actorlinktvshow.idActor "
                 "LEFT JOIN directorlinktvshow ON directorlinktvshow.idShow=tvshowview.idShow "
                 "LEFT JOIN actors d ON d.idActor=directorlinktvshow.idDirector";
  filter.where = PrepareSQL("a.strActor='%s' OR d.strActor='%s'", strActor.c_str(), strActor.c_str());
  filter.group = "tvshowview.idShow";
  GetTvShowsByWhere("videodb://tvshows/titles/", filter, items);
}

void CVideoDatabase::GetEpisodesByActor(const CStdString& strActor, CFileItemList& items)
{
  Filter filter;
  filter.join  = "LEFT JOIN actorlinkepisode ON actorlinkepisode.idEpisode=episodeview.idEpisode "
                 "LEFT JOIN actors a ON a.idActor=actorlinkepisode.idActor "
                 "LEFT JOIN directorlinkepisode ON directorlinkepisode.idEpisode=episodeview.idEpisode "
                 "LEFT JOIN actors d ON d.idActor=directorlinkepisode.idDirector";
  filter.where = PrepareSQL("a.strActor='%s' OR d.strActor='%s'", strActor.c_str(), strActor.c_str());
  filter.group = "episodeview.idEpisode";
  GetEpisodesByWhere("videodb://tvshows/titles/", filter, items);
}

void CVideoDatabase::GetMusicVideosByArtist(const CStdString& strArtist, CFileItemList& items)
{
  try
  {
    items.Clear();
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    CStdString strSQL;
    if (strArtist.empty())  // TODO: SMARTPLAYLISTS what is this here for???
      strSQL=PrepareSQL("select distinct * from musicvideoview join artistlinkmusicvideo on artistlinkmusicvideo.idMVideo=musicvideoview.idMVideo join actors on actors.idActor=artistlinkmusicvideo.idArtist");
    else
      strSQL=PrepareSQL("select * from musicvideoview join artistlinkmusicvideo on artistlinkmusicvideo.idMVideo=musicvideoview.idMVideo join actors on actors.idActor=artistlinkmusicvideo.idArtist where actors.strActor='%s'", strArtist.c_str());
    m_pDS->query( strSQL.c_str() );

    while (!m_pDS->eof())
    {
      CVideoInfoTag tag = GetDetailsForMusicVideo(m_pDS);
      CFileItemPtr pItem(new CFileItem(tag));
      pItem->SetLabel(StringUtils::Join(tag.m_artist, g_advancedSettings.m_videoItemSeparator));
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
bool CVideoDatabase::GetMovieInfo(const CStdString& strFilenameAndPath, CVideoInfoTag& details, int idMovie /* = -1 */)
{
  try
  {
    // TODO: Optimize this - no need for all the queries!
    if (idMovie < 0)
      idMovie = GetMovieId(strFilenameAndPath);
    if (idMovie < 0) return false;

    CStdString sql = PrepareSQL("select * from movieview where idMovie=%i", idMovie);
    if (!m_pDS->query(sql.c_str()))
      return false;
    details = GetDetailsForMovie(m_pDS, true);
    return !details.IsEmpty();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strFilenameAndPath.c_str());
  }
  return false;
}

//********************************************************************************************************************************
bool CVideoDatabase::GetTvShowInfo(const CStdString& strPath, CVideoInfoTag& details, int idTvShow /* = -1 */, CFileItem *item /* = NULL */)
{
  try
  {
    if (idTvShow < 0)
      idTvShow = GetTvShowId(strPath);
    if (idTvShow < 0) return false;

    CStdString sql = PrepareSQL("SELECT * FROM tvshowview WHERE idShow=%i GROUP BY idShow", idTvShow);
    if (!m_pDS->query(sql.c_str()))
      return false;
    details = GetDetailsForTvShow(m_pDS, true, item);
    return !details.IsEmpty();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strPath.c_str());
  }
  return false;
}

bool CVideoDatabase::GetSeasonInfo(int idSeason, CVideoInfoTag& details)
{
  if (idSeason < 0)
    return false;

  try
  {
    if (!m_pDB.get() || !m_pDS.get())
      return false;

    CStdString sql = PrepareSQL("SELECT idShow FROM seasons WHERE idSeason=%i", idSeason);
    if (!m_pDS->query(sql.c_str()))
      return false;

    int idShow = -1;
    if (m_pDS->num_rows() == 1)
      idShow = m_pDS->fv(0).get_asInt();

    m_pDS->close();

    if (idShow < 0)
      return false;

    CFileItemList seasons;
    if (!GetSeasonsNav(StringUtils::Format("videodb://tvshows/titles/%ld/", idShow), seasons, -1, -1, -1, -1, idShow, false) || seasons.Size() <= 0)
      return false;

    for (int index = 0; index < seasons.Size(); index++)
    {
      const CFileItemPtr season = seasons.Get(index);
      if (season->HasVideoInfoTag() && season->GetVideoInfoTag()->m_iDbId == idSeason && season->GetVideoInfoTag()->m_iIdShow == idShow)
      {
        details = *season->GetVideoInfoTag();
        return true;
      }
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%i) failed", __FUNCTION__, idSeason);
  }
  return false;
}

bool CVideoDatabase::GetEpisodeInfo(const CStdString& strFilenameAndPath, CVideoInfoTag& details, int idEpisode /* = -1 */)
{
  try
  {
    // TODO: Optimize this - no need for all the queries!
    if (idEpisode < 0)
      idEpisode = GetEpisodeId(strFilenameAndPath);
    if (idEpisode < 0) return false;

    CStdString sql = PrepareSQL("select * from episodeview where idEpisode=%i",idEpisode);
    if (!m_pDS->query(sql.c_str()))
      return false;
    details = GetDetailsForEpisode(m_pDS, true);
    return !details.IsEmpty();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strFilenameAndPath.c_str());
  }
  return false;
}

bool CVideoDatabase::GetMusicVideoInfo(const CStdString& strFilenameAndPath, CVideoInfoTag& details, int idMVideo /* = -1 */)
{
  try
  {
    // TODO: Optimize this - no need for all the queries!
    if (idMVideo < 0)
      idMVideo = GetMusicVideoId(strFilenameAndPath);
    if (idMVideo < 0) return false;

    CStdString sql = PrepareSQL("select * from musicvideoview where idMVideo=%i", idMVideo);
    if (!m_pDS->query(sql.c_str()))
      return false;
    details = GetDetailsForMusicVideo(m_pDS, true);
    return !details.IsEmpty();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strFilenameAndPath.c_str());
  }
  return false;
}

bool CVideoDatabase::GetSetInfo(int idSet, CVideoInfoTag& details)
{
  try
  {
    if (idSet < 0)
      return false;

    Filter filter;
    filter.where = PrepareSQL("sets.idSet=%d", idSet);
    CFileItemList items;
    if (!GetSetsByWhere("videodb://movies/sets/", filter, items) ||
        items.Size() != 1 ||
        !items[0]->HasVideoInfoTag())
      return false;

    details = *(items[0]->GetVideoInfoTag());
    return !details.IsEmpty();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%d) failed", __FUNCTION__, idSet);
  }
  return false;
}

bool CVideoDatabase::GetFileInfo(const CStdString& strFilenameAndPath, CVideoInfoTag& details, int idFile /* = -1 */)
{
  try
  {
    if (idFile < 0)
      idFile = GetFileId(strFilenameAndPath);
    if (idFile < 0)
      return false;

    CStdString sql = PrepareSQL("SELECT * FROM files "
                                "JOIN path ON path.idPath = files.idPath "
                                "LEFT JOIN bookmark ON bookmark.idFile = files.idFile AND bookmark.type = %i "
                                "WHERE files.idFile = %i", CBookmark::RESUME, idFile);
    if (!m_pDS->query(sql.c_str()))
      return false;

    details.m_iFileId = m_pDS->fv("files.idFile").get_asInt();
    details.m_strPath = m_pDS->fv("path.strPath").get_asString();
    CStdString strFileName = m_pDS->fv("files.strFilename").get_asString();
    ConstructPath(details.m_strFileNameAndPath, details.m_strPath, strFileName);
    details.m_playCount = max(details.m_playCount, m_pDS->fv("files.playCount").get_asInt());
    if (!details.m_lastPlayed.IsValid())
      details.m_lastPlayed.SetFromDBDateTime(m_pDS->fv("files.lastPlayed").get_asString());
    if (!details.m_dateAdded.IsValid())
      details.m_dateAdded.SetFromDBDateTime(m_pDS->fv("files.dateAdded").get_asString());
    if (!details.m_resumePoint.IsSet())
    {
      details.m_resumePoint.timeInSeconds = m_pDS->fv("bookmark.timeInSeconds").get_asInt();
      details.m_resumePoint.totalTimeInSeconds = m_pDS->fv("bookmark.totalTimeInSeconds").get_asInt();
      details.m_resumePoint.type = CBookmark::RESUME;
    }

    // get streamdetails
    GetStreamDetails(details);

    return !details.IsEmpty();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strFilenameAndPath.c_str());
  }
  return false;
}

void CVideoDatabase::AddGenreAndDirectorsAndStudios(const CVideoInfoTag& details, vector<int>& vecDirectors, vector<int>& vecGenres, vector<int>& vecStudios)
{
  // add all directors
  for (unsigned int i = 0; i < details.m_director.size(); i++)
    vecDirectors.push_back(AddActor(details.m_director[i],""));

  // add all genres
  for (unsigned int i = 0; i < details.m_genre.size(); i++)
    vecGenres.push_back(AddGenre(details.m_genre[i]));
  // add all studios
  for (unsigned int i = 0; i < details.m_studio.size(); i++)
    vecStudios.push_back(AddStudio(details.m_studio[i]));
}

CStdString CVideoDatabase::GetValueString(const CVideoInfoTag &details, int min, int max, const SDbTableOffsets *offsets) const
{
  std::vector<std::string> conditions;
  for (int i = min + 1; i < max; ++i)
  {
    switch (offsets[i].type)
    {
    case VIDEODB_TYPE_STRING:
      conditions.push_back(PrepareSQL("c%02d='%s'", i, ((CStdString*)(((char*)&details)+offsets[i].offset))->c_str()));
      break;
    case VIDEODB_TYPE_INT:
      conditions.push_back(PrepareSQL("c%02d='%i'", i, *(int*)(((char*)&details)+offsets[i].offset)));
      break;
    case VIDEODB_TYPE_COUNT:
      {
        int value = *(int*)(((char*)&details)+offsets[i].offset);
        if (value)
          conditions.push_back(PrepareSQL("c%02d=%i", i, value));
        else
          conditions.push_back(PrepareSQL("c%02d=NULL", i));
      }
      break;
    case VIDEODB_TYPE_BOOL:
      conditions.push_back(PrepareSQL("c%02d='%s'", i, *(bool*)(((char*)&details)+offsets[i].offset)?"true":"false"));
      break;
    case VIDEODB_TYPE_FLOAT:
      conditions.push_back(PrepareSQL("c%02d='%f'", i, *(float*)(((char*)&details)+offsets[i].offset)));
      break;
    case VIDEODB_TYPE_STRINGARRAY:
      conditions.push_back(PrepareSQL("c%02d='%s'", i, StringUtils::Join(*((std::vector<std::string>*)(((char*)&details)+offsets[i].offset)),
                                                                          g_advancedSettings.m_videoItemSeparator).c_str()));
      break;
    case VIDEODB_TYPE_DATE:
      conditions.push_back(PrepareSQL("c%02d='%s'", i, ((CDateTime*)(((char*)&details)+offsets[i].offset))->GetAsDBDate().c_str()));
      break;
    case VIDEODB_TYPE_DATETIME:
      conditions.push_back(PrepareSQL("c%02d='%s'", i, ((CDateTime*)(((char*)&details)+offsets[i].offset))->GetAsDBDateTime().c_str()));
      break;
    }
  }
  return StringUtils::Join(conditions, ",");
}

//********************************************************************************************************************************
int CVideoDatabase::SetDetailsForMovie(const CStdString& strFilenameAndPath, const CVideoInfoTag& details, const map<string, string> &artwork, int idMovie /* = -1 */)
{
  try
  {
    BeginTransaction();

    if (idMovie < 0)
      idMovie = GetMovieId(strFilenameAndPath);

    if (idMovie > -1)
      DeleteMovie(idMovie, true); // true to keep the table entry
    else
    {
      // only add a new movie if we don't already have a valid idMovie
      // (DeleteMovie is called with bKeepId == true so the movie won't
      // be removed from the movie table)
      idMovie = AddMovie(strFilenameAndPath);
      if (idMovie < 0)
      {
        RollbackTransaction();
        return idMovie;
      }
    }

    vector<int> vecDirectors;
    vector<int> vecGenres;
    vector<int> vecStudios;
    AddGenreAndDirectorsAndStudios(details,vecDirectors,vecGenres,vecStudios);

    for (unsigned int i = 0; i < vecGenres.size(); ++i)
      AddGenreToMovie(idMovie, vecGenres[i]);

    for (unsigned int i = 0; i < vecDirectors.size(); ++i)
      AddDirectorToMovie(idMovie, vecDirectors[i]);

    for (unsigned int i = 0; i < vecStudios.size(); ++i)
      AddStudioToMovie(idMovie, vecStudios[i]);

    // add writers...
    for (unsigned int i = 0; i < details.m_writingCredits.size(); i++)
      AddWriterToMovie(idMovie, AddActor(details.m_writingCredits[i],""));

    AddCast(idMovie, "movie", "movie", details.m_cast);

    // add set...
    int idSet = -1;
    if (!details.m_strSet.empty())
    {
      idSet = AddSet(details.m_strSet);
      // add art if not available
      map<string, string> setArt;
      if (!GetArtForItem(idSet, MediaTypeVideoCollection, setArt))
        SetArtForItem(idSet, MediaTypeVideoCollection, artwork);
    }

    // add tags...
    for (unsigned int i = 0; i < details.m_tags.size(); i++)
    {
      int idTag = AddTag(details.m_tags[i]);
      AddTagToItem(idMovie, idTag, MediaTypeMovie);
    }

    // add countries...
    for (unsigned int i = 0; i < details.m_country.size(); i++)
      AddCountryToMovie(idMovie, AddCountry(details.m_country[i]));

    if (details.HasStreamDetails())
      SetStreamDetailsForFileId(details.m_streamDetails, GetFileId(strFilenameAndPath));

    SetArtForItem(idMovie, MediaTypeMovie, artwork);

    if (!details.m_strIMDBNumber.empty() && details.m_iYear)
    { // query DB for any movies matching imdbid and year
      CStdString strSQL = PrepareSQL("select files.playCount, files.lastPlayed from movie,files where files.idFile=movie.idFile and movie.c%02d='%s' and movie.c%02d=%i and movie.idMovie!=%i and files.playCount > 0", VIDEODB_ID_IDENT, details.m_strIMDBNumber.c_str(), VIDEODB_ID_YEAR, details.m_iYear, idMovie);
      m_pDS->query(strSQL.c_str());

      if (!m_pDS->eof())
      {
        int playCount = m_pDS->fv("files.playCount").get_asInt();

        CDateTime lastPlayed;
        lastPlayed.SetFromDBDateTime(m_pDS->fv("files.lastPlayed").get_asString());

        int idFile = GetFileId(strFilenameAndPath);

        // update with playCount and lastPlayed
        strSQL = PrepareSQL("update files set playCount=%i,lastPlayed='%s' where idFile=%i", playCount, lastPlayed.GetAsDBDateTime().c_str(), idFile);
        m_pDS->exec(strSQL.c_str());
      }

      m_pDS->close();
    }
    // update our movie table (we know it was added already above)
    // and insert the new row
    CStdString sql = "update movie set " + GetValueString(details, VIDEODB_ID_MIN, VIDEODB_ID_MAX, DbMovieOffsets);
    if (idSet > 0)
      sql += PrepareSQL(", idSet = %i", idSet);
    else
      sql += ", idSet = NULL";
    sql += PrepareSQL(" where idMovie=%i", idMovie);
    m_pDS->exec(sql.c_str());
    CommitTransaction();

    return idMovie;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strFilenameAndPath.c_str());
  }
  RollbackTransaction();
  return -1;
}

int CVideoDatabase::UpdateDetailsForMovie(int idMovie, const CVideoInfoTag& details, const std::map<std::string, std::string> &artwork, const std::set<std::string> &updatedDetails)
{
  if (idMovie < 0)
    return idMovie;

  try
  {
    CLog::Log(LOGDEBUG, "%s: Starting updates for movie %i", __FUNCTION__, idMovie);

    BeginTransaction();

    // process the link table updates
    if (updatedDetails.find("genre") != updatedDetails.end())
      UpdateLinkTable(idMovie, "movie", "genre", details.m_genre);
    if (updatedDetails.find("studio") != updatedDetails.end())
      UpdateLinkTable(idMovie, "movie", "studio", details.m_studio);
    if (updatedDetails.find("country") != updatedDetails.end())
      UpdateLinkTable(idMovie, "movie", "country", details.m_country);
    if (updatedDetails.find("director") != updatedDetails.end())
      UpdateActorLinkTable(idMovie, "movie", "director", details.m_director);
    if (updatedDetails.find("writer") != updatedDetails.end())
      UpdateActorLinkTable(idMovie, "movie", "writer", details.m_writingCredits);
    if (updatedDetails.find("tag") != updatedDetails.end())
    {
      RemoveTagsFromItem(idMovie, "movie");
      for (unsigned int i = 0; i < details.m_tags.size(); i++)
        AddTagToItem(idMovie, AddTag(details.m_tags[i]), "movie");
    }
    if (updatedDetails.find("art.altered") != updatedDetails.end())
      SetArtForItem(idMovie, "movie", artwork);

    // track if the set was updated
    int idSet = 0;
    if (updatedDetails.find("set") != updatedDetails.end())
    { // set
      idSet = -1;
      if (!details.m_strSet.empty())
      {
        idSet = AddSet(details.m_strSet);
        // add art if not available
        map<string, string> setArt;
        if (!GetArtForItem(idSet, "set", setArt))
          SetArtForItem(idSet, "set", artwork);
      }
    }

    // and update the movie table
    std::string sql = "update movie set " + GetValueString(details, VIDEODB_ID_MIN, VIDEODB_ID_MAX, DbMovieOffsets);
    if (idSet > 0)
      sql += PrepareSQL(", idSet = %i", idSet);
    else if (idSet < 0)
      sql += ", idSet = NULL";
    sql += PrepareSQL(" where idMovie=%i", idMovie);
    m_pDS->exec(sql);

    CommitTransaction();

    CLog::Log(LOGDEBUG, "%s: Finished updates for movie %i", __FUNCTION__, idMovie);

    return idMovie;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%i) failed", __FUNCTION__, idMovie);
  }
  RollbackTransaction();
  return -1;
}

int CVideoDatabase::SetDetailsForMovieSet(const CVideoInfoTag& details, const std::map<std::string, std::string> &artwork, int idSet /* = -1 */)
{
  if (details.m_strTitle.empty())
    return -1;

  try
  {
    BeginTransaction();
    if (idSet < 0)
    {
      idSet = AddSet(details.m_strTitle);
      if (idSet < 0)
      {
        RollbackTransaction();
        return -1;
      }
    }

    SetArtForItem(idSet, MediaTypeVideoCollection, artwork);

    // and insert the new row
    CStdString sql = PrepareSQL("UPDATE sets SET strSet='%s' WHERE idSet=%i", details.m_strTitle.c_str(), idSet);
    m_pDS->exec(sql.c_str());
    CommitTransaction();

    return idSet;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%i) failed", __FUNCTION__, idSet);
  }
  RollbackTransaction();
  return -1;
}

int CVideoDatabase::GetMatchingTvShow(const CVideoInfoTag &details)
{
  // first try matching on VIDEODB_ID_TV_IDENT, then on title + year
  int id = -1;
  if (!details.m_strIMDBNumber.empty())
    id = GetDbId(PrepareSQL("SELECT idShow from tvshow WHERE c%02d='%s'", VIDEODB_ID_TV_IDENT, details.m_strIMDBNumber.c_str()));
  if (id < 0)
    id = GetDbId(PrepareSQL("SELECT idShow FROM tvshow WHERE c%02d='%s' AND c%02d='%s'", VIDEODB_ID_TV_TITLE, details.m_strTitle.c_str(), VIDEODB_ID_TV_PREMIERED, details.m_premiered.GetAsDBDate().c_str()));
  return id;
}

int CVideoDatabase::SetDetailsForTvShow(const vector< pair<string, string> > &paths, const CVideoInfoTag& details, const map<string, string> &artwork, const map<int, map<string, string> > &seasonArt, int idTvShow /*= -1 */)
{

  /*
   The steps are as follows.
   1. Check if the tvshow is found on any of the given paths.  If found, we have the show id.
   2. Search for a matching show.  If found, we have the show id.
   3. If we don't have the id, add a new show.
   4. Add the paths to the show.
   5. Add details for the show.
   */

  if (idTvShow < 0)
  {
    for (vector< pair<string, string> >::const_iterator i = paths.begin(); i != paths.end(); ++i)
    {
      idTvShow = GetTvShowId(i->first);
      if (idTvShow > -1)
        break;
    }
  }
  if (idTvShow < 0)
    idTvShow = GetMatchingTvShow(details);
  if (idTvShow < 0)
  {
    idTvShow = AddTvShow();
    if (idTvShow < 0)
      return -1;
  }

  // add any paths to the tvshow
  for (vector< pair<string, string> >::const_iterator i = paths.begin(); i != paths.end(); ++i)
    AddPathToTvShow(idTvShow, i->first, i->second);

  UpdateDetailsForTvShow(idTvShow, details, artwork, seasonArt);

  return idTvShow;
}

bool CVideoDatabase::UpdateDetailsForTvShow(int idTvShow, const CVideoInfoTag &details, const map<string, string> &artwork, const map<int, map<string, string> > &seasonArt)
{
  BeginTransaction();

  DeleteDetailsForTvShow(idTvShow);

  vector<int> vecDirectors;
  vector<int> vecGenres;
  vector<int> vecStudios;
  AddGenreAndDirectorsAndStudios(details,vecDirectors,vecGenres,vecStudios);

  AddCast(idTvShow, "tvshow", "show", details.m_cast);

  unsigned int i;
  for (i = 0; i < vecGenres.size(); ++i)
    AddGenreToTvShow(idTvShow, vecGenres[i]);

  for (i = 0; i < vecDirectors.size(); ++i)
    AddDirectorToTvShow(idTvShow, vecDirectors[i]);

  for (i = 0; i < vecStudios.size(); ++i)
    AddStudioToTvShow(idTvShow, vecStudios[i]);

  // add tags...
  for (unsigned int i = 0; i < details.m_tags.size(); i++)
  {
    int idTag = AddTag(details.m_tags[i]);
    AddTagToItem(idTvShow, idTag, MediaTypeTvShow);
  }

  // add "all seasons" - the rest are added in SetDetailsForEpisode
  AddSeason(idTvShow, -1);

  SetArtForItem(idTvShow, MediaTypeTvShow, artwork);
  for (map<int, map<string, string> >::const_iterator i = seasonArt.begin(); i != seasonArt.end(); ++i)
  {
    int idSeason = AddSeason(idTvShow, i->first);
    if (idSeason > -1)
      SetArtForItem(idSeason, MediaTypeSeason, i->second);
  }

  // and insert the new row
  std::string sql = "UPDATE tvshow SET " + GetValueString(details, VIDEODB_ID_TV_MIN, VIDEODB_ID_TV_MAX, DbTvShowOffsets);
  sql += PrepareSQL(" WHERE idShow=%i", idTvShow);
  if (ExecuteQuery(sql))
  {
    CommitTransaction();
    return true;
  }
  RollbackTransaction();
  return false;
}

int CVideoDatabase::SetDetailsForSeason(const CVideoInfoTag& details, const std::map<std::string, std::string> &artwork, int idShow, int idSeason /* = -1 */)
{
  if (idShow < 0 || details.m_iSeason < 0)
    return -1;

   try
  {
    BeginTransaction();
    if (idSeason < 0)
    {
      idSeason = AddSeason(idShow, details.m_iSeason);
      if (idSeason < 0)
      {
        RollbackTransaction();
        return -1;
      }
    }

    SetArtForItem(idSeason, MediaTypeSeason, artwork);

    // and insert the new row
    CStdString sql = PrepareSQL("UPDATE seasons SET season=%i WHERE idSeason=%i", details.m_iSeason, idSeason);
    m_pDS->exec(sql.c_str());
    CommitTransaction();

    return idSeason;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%i) failed", __FUNCTION__, idSeason);
  }
  RollbackTransaction();
  return -1;
}

int CVideoDatabase::SetDetailsForEpisode(const CStdString& strFilenameAndPath, const CVideoInfoTag& details, const map<string, string> &artwork, int idShow, int idEpisode)
{
  try
  {
    BeginTransaction();
    if (idEpisode < 0)
      idEpisode = GetEpisodeId(strFilenameAndPath);

    if (idEpisode > 0)
      DeleteEpisode(idEpisode, true); // true to keep the table entry
    else
    {
      // only add a new episode if we don't already have a valid idEpisode
      // (DeleteEpisode is called with bKeepId == true so the episode won't
      // be removed from the episode table)
      idEpisode = AddEpisode(idShow,strFilenameAndPath);
      if (idEpisode < 0)
      {
        RollbackTransaction();
        return -1;
      }
    }

    vector<int> vecDirectors;
    vector<int> vecGenres;
    vector<int> vecStudios;
    AddGenreAndDirectorsAndStudios(details,vecDirectors,vecGenres,vecStudios);

    AddCast(idEpisode, "episode", "episode", details.m_cast);

    // add writers...
    for (unsigned int i = 0; i < details.m_writingCredits.size(); i++)
      AddWriterToEpisode(idEpisode, AddActor(details.m_writingCredits[i],""));

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

    // ensure we have this season already added
    AddSeason(idShow, details.m_iSeason);

    SetArtForItem(idEpisode, MediaTypeEpisode, artwork);

    if (details.m_iEpisode != -1 && details.m_iSeason != -1)
    { // query DB for any episodes matching idShow, Season and Episode
      CStdString strSQL = PrepareSQL("select files.playCount, files.lastPlayed from episode, files where files.idFile=episode.idFile and episode.c%02d=%i and episode.c%02d=%i AND episode.idShow=%i and episode.idEpisode!=%i and files.playCount > 0",VIDEODB_ID_EPISODE_SEASON, details.m_iSeason, VIDEODB_ID_EPISODE_EPISODE, details.m_iEpisode, idShow, idEpisode);
      m_pDS->query(strSQL.c_str());

      if (!m_pDS->eof())
      {
        int playCount = m_pDS->fv("files.playCount").get_asInt();

        CDateTime lastPlayed;
        lastPlayed.SetFromDBDateTime(m_pDS->fv("files.lastPlayed").get_asString());

        int idFile = GetFileId(strFilenameAndPath);

        // update with playCount and lastPlayed
        strSQL = PrepareSQL("update files set playCount=%i,lastPlayed='%s' where idFile=%i", playCount, lastPlayed.GetAsDBDateTime().c_str(), idFile);
        m_pDS->exec(strSQL.c_str());
      }

      m_pDS->close();
    }
    // and insert the new row
    CStdString sql = "update episode set " + GetValueString(details, VIDEODB_ID_EPISODE_MIN, VIDEODB_ID_EPISODE_MAX, DbEpisodeOffsets);
    sql += PrepareSQL(" where idEpisode=%i", idEpisode);
    m_pDS->exec(sql.c_str());
    CommitTransaction();

    return idEpisode;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strFilenameAndPath.c_str());
  }
  RollbackTransaction();
  return -1;
}

int CVideoDatabase::GetSeasonId(int showID, int season)
{
  CStdString sql = PrepareSQL("idShow=%i AND season=%i", showID, season);
  CStdString id = GetSingleValue("seasons", "idSeason", sql);
  if (id.empty())
    return -1;
  return strtol(id.c_str(), NULL, 10);
}

int CVideoDatabase::AddSeason(int showID, int season)
{
  int seasonId = GetSeasonId(showID, season);
  if (seasonId < 0)
  {
    if (ExecuteQuery(PrepareSQL("INSERT INTO seasons (idShow,season) VALUES(%i,%i)", showID, season)))
      seasonId = (int)m_pDS->lastinsertid();
  }
  return seasonId;
}

int CVideoDatabase::SetDetailsForMusicVideo(const CStdString& strFilenameAndPath, const CVideoInfoTag& details, const map<string, string> &artwork, int idMVideo /* = -1 */)
{
  try
  {
    BeginTransaction();

    if (idMVideo < 0)
      idMVideo = GetMusicVideoId(strFilenameAndPath);

    if (idMVideo > -1)
      DeleteMusicVideo(idMVideo, true); // Keep id
    else
    {
      // only add a new musicvideo if we don't already have a valid idMVideo
      // (DeleteMusicVideo is called with bKeepId == true so the musicvideo won't
      // be removed from the musicvideo table)
      idMVideo = AddMusicVideo(strFilenameAndPath);
      if (idMVideo < 0)
      {
        RollbackTransaction();
        return -1;
      }
    }

    vector<int> vecDirectors;
    vector<int> vecGenres;
    vector<int> vecStudios;
    AddGenreAndDirectorsAndStudios(details,vecDirectors,vecGenres,vecStudios);

    // add artists...
    if (!details.m_artist.empty())
    {
      for (unsigned int i = 0; i < details.m_artist.size(); i++)
      {
        CStdString artist = details.m_artist[i];
        StringUtils::Trim(artist);
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

    // add tags...
    for (unsigned int i = 0; i < details.m_tags.size(); i++)
    {
      int idTag = AddTag(details.m_tags[i]);
      AddTagToItem(idMVideo, idTag, MediaTypeMusicVideo);
    }

    if (details.HasStreamDetails())
      SetStreamDetailsForFileId(details.m_streamDetails, GetFileId(strFilenameAndPath));

    SetArtForItem(idMVideo, MediaTypeMusicVideo, artwork);

    // update our movie table (we know it was added already above)
    // and insert the new row
    CStdString sql = "update musicvideo set " + GetValueString(details, VIDEODB_ID_MUSICVIDEO_MIN, VIDEODB_ID_MUSICVIDEO_MAX, DbMusicVideoOffsets);
    sql += PrepareSQL(" where idMVideo=%i", idMVideo);
    m_pDS->exec(sql.c_str());
    CommitTransaction();

    return idMVideo;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strFilenameAndPath.c_str());
  }
  RollbackTransaction();
  return -1;
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
    m_pDS->exec(PrepareSQL("DELETE FROM streamdetails WHERE idFile = %i", idFile));

    for (int i=1; i<=details.GetVideoStreamCount(); i++)
    {
      m_pDS->exec(PrepareSQL("INSERT INTO streamdetails "
        "(idFile, iStreamType, strVideoCodec, fVideoAspect, iVideoWidth, iVideoHeight, iVideoDuration, strStereoMode) "
        "VALUES (%i,%i,'%s',%f,%i,%i,%i,'%s')",
        idFile, (int)CStreamDetail::VIDEO,
        details.GetVideoCodec(i).c_str(), details.GetVideoAspect(i),
        details.GetVideoWidth(i), details.GetVideoHeight(i), details.GetVideoDuration(i),
        details.GetStereoMode(i).c_str()));
    }
    for (int i=1; i<=details.GetAudioStreamCount(); i++)
    {
      m_pDS->exec(PrepareSQL("INSERT INTO streamdetails "
        "(idFile, iStreamType, strAudioCodec, iAudioChannels, strAudioLanguage) "
        "VALUES (%i,%i,'%s',%i,'%s')",
        idFile, (int)CStreamDetail::AUDIO,
        details.GetAudioCodec(i).c_str(), details.GetAudioChannels(i),
        details.GetAudioLanguage(i).c_str()));
    }
    for (int i=1; i<=details.GetSubtitleStreamCount(); i++)
    {
      m_pDS->exec(PrepareSQL("INSERT INTO streamdetails "
        "(idFile, iStreamType, strSubtitleLanguage) "
        "VALUES (%i,%i,'%s')",
        idFile, (int)CStreamDetail::SUBTITLE,
        details.GetSubtitleLanguage(i).c_str()));
    }

    // update the runtime information, if empty
    if (details.GetVideoDuration())
    {
      vector< pair<string, int> > tables;
      tables.push_back(make_pair("movie", VIDEODB_ID_RUNTIME));
      tables.push_back(make_pair("episode", VIDEODB_ID_EPISODE_RUNTIME));
      tables.push_back(make_pair("musicvideo", VIDEODB_ID_MUSICVIDEO_RUNTIME));
      for (vector< pair<string, int> >::iterator i = tables.begin(); i != tables.end(); ++i)
      {
        CStdString sql = PrepareSQL("update %s set c%02d=%d where idFile=%d and c%02d=''",
                                    i->first.c_str(), i->second, details.GetVideoDuration(), idFile, i->second);
        m_pDS->exec(sql);
      }
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
      strSQL=PrepareSQL("select path.strPath,files.strFileName from path, files, movie where path.idPath=files.idPath and files.idFile=movie.idFile and movie.idMovie=%i order by strFilename", idMovie );
    if (iType == VIDEODB_CONTENT_EPISODES)
      strSQL=PrepareSQL("select path.strPath,files.strFileName from path, files, episode where path.idPath=files.idPath and files.idFile=episode.idFile and episode.idEpisode=%i order by strFilename", idMovie );
    if (iType == VIDEODB_CONTENT_TVSHOWS)
      strSQL=PrepareSQL("select path.strPath from path,tvshowlinkpath where path.idPath=tvshowlinkpath.idPath and tvshowlinkpath.idShow=%i", idMovie );
    if (iType ==VIDEODB_CONTENT_MUSICVIDEOS)
      strSQL=PrepareSQL("select path.strPath,files.strFileName from path, files, musicvideo where path.idPath=files.idPath and files.idFile=musicvideo.idFile and musicvideo.idMVideo=%i order by strFilename", idMovie );

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
void CVideoDatabase::GetBookMarksForFile(const CStdString& strFilenameAndPath, VECBOOKMARKS& bookmarks, CBookmark::EType type /*= CBookmark::STANDARD*/, bool bAppend, long partNumber)
{
  try
  {
    if (URIUtils::IsStack(strFilenameAndPath) && CFileItem(CStackDirectory::GetFirstStackedFile(strFilenameAndPath),false).IsDiscImage())
    {
      CStackDirectory dir;
      CFileItemList fileList;
      const CURL pathToUrl(strFilenameAndPath);
      dir.GetDirectory(pathToUrl, fileList);
      if (!bAppend)
        bookmarks.clear();
      for (int i = fileList.Size() - 1; i >= 0; i--) // put the bookmarks of the highest part first in the list
        GetBookMarksForFile(fileList[i]->GetPath(), bookmarks, type, true, (i+1));
    }
    else
    {
      int idFile = GetFileId(strFilenameAndPath);
      if (idFile < 0) return ;
      if (!bAppend)
        bookmarks.erase(bookmarks.begin(), bookmarks.end());
      if (NULL == m_pDB.get()) return ;
      if (NULL == m_pDS.get()) return ;

      CStdString strSQL=PrepareSQL("select * from bookmark where idFile=%i and type=%i order by timeInSeconds", idFile, (int)type);
      m_pDS->query( strSQL.c_str() );
      while (!m_pDS->eof())
      {
        CBookmark bookmark;
        bookmark.timeInSeconds = m_pDS->fv("timeInSeconds").get_asDouble();
        bookmark.partNumber = partNumber;
        bookmark.totalTimeInSeconds = m_pDS->fv("totalTimeInSeconds").get_asDouble();
        bookmark.thumbNailImage = m_pDS->fv("thumbNailImage").get_asString();
        bookmark.playerState = m_pDS->fv("playerState").get_asString();
        bookmark.player = m_pDS->fv("player").get_asString();
        bookmark.type = type;
        if (type == CBookmark::EPISODE)
        {
          CStdString strSQL2=PrepareSQL("select c%02d, c%02d from episode where c%02d=%i order by c%02d, c%02d", VIDEODB_ID_EPISODE_EPISODE, VIDEODB_ID_EPISODE_SEASON, VIDEODB_ID_EPISODE_BOOKMARK, m_pDS->fv("idBookmark").get_asInt(), VIDEODB_ID_EPISODE_SORTSEASON, VIDEODB_ID_EPISODE_SORTEPISODE);
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
    CStdString sql = PrepareSQL("delete from bookmark where idFile=%i and type=%i", fileID, CBookmark::RESUME);
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
    CStdString strSQL = PrepareSQL("select * from episodeview where idFile=%i order by c%02d, c%02d asc", GetFileId(strFilenameAndPath), VIDEODB_ID_EPISODE_SORTSEASON, VIDEODB_ID_EPISODE_SORTEPISODE);
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
    int idFile = AddFile(strFilenameAndPath);
    if (idFile < 0)
      return;
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    CStdString strSQL;
    int idBookmark=-1;
    if (type == CBookmark::RESUME) // get the same resume mark bookmark each time type
    {
      strSQL=PrepareSQL("select idBookmark from bookmark where idFile=%i and type=1", idFile);
    }
    else if (type == CBookmark::STANDARD) // get the same bookmark again, and update. not sure here as a dvd can have same time in multiple places, state will differ thou
    {
      /* get a bookmark within the same time as previous */
      double mintime = bookmark.timeInSeconds - 0.5f;
      double maxtime = bookmark.timeInSeconds + 0.5f;
      strSQL=PrepareSQL("select idBookmark from bookmark where idFile=%i and type=%i and (timeInSeconds between %f and %f) and playerState='%s'", idFile, (int)type, mintime, maxtime, bookmark.playerState.c_str());
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
      strSQL=PrepareSQL("update bookmark set timeInSeconds = %f, totalTimeInSeconds = %f, thumbNailImage = '%s', player = '%s', playerState = '%s' where idBookmark = %i", bookmark.timeInSeconds, bookmark.totalTimeInSeconds, bookmark.thumbNailImage.c_str(), bookmark.player.c_str(), bookmark.playerState.c_str(), idBookmark);
    else
      strSQL=PrepareSQL("insert into bookmark (idBookmark, idFile, timeInSeconds, totalTimeInSeconds, thumbNailImage, player, playerState, type) values(NULL,%i,%f,%f,'%s','%s','%s', %i)", idFile, bookmark.timeInSeconds, bookmark.totalTimeInSeconds, bookmark.thumbNailImage.c_str(), bookmark.player.c_str(), bookmark.playerState.c_str(), (int)type);

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
    CStdString strSQL = PrepareSQL("select idBookmark from bookmark where idFile=%i and type=%i and playerState like '%s' and player like '%s' and (timeInSeconds between %f and %f)", idFile, type, bookmark.playerState.c_str(), bookmark.player.c_str(), mintime, maxtime);

    m_pDS->query( strSQL.c_str() );
    if (m_pDS->num_rows() != 0)
    {
      int idBookmark = m_pDS->get_field_value("idBookmark").get_asInt();
      strSQL=PrepareSQL("delete from bookmark where idBookmark=%i",idBookmark);
      m_pDS->exec(strSQL.c_str());
      if (type == CBookmark::EPISODE)
      {
        strSQL=PrepareSQL("update episode set c%02d=-1 where idFile=%i and c%02d=%i", VIDEODB_ID_EPISODE_BOOKMARK, idFile, VIDEODB_ID_EPISODE_BOOKMARK, idBookmark);
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
  int idFile = GetFileId(strFilenameAndPath);
  if (idFile >= 0)
    return ClearBookMarksOfFile(idFile, type);
}

void CVideoDatabase::ClearBookMarksOfFile(int idFile, CBookmark::EType type /*= CBookmark::STANDARD*/)
{
  if (idFile < 0)
    return;

  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    CStdString strSQL=PrepareSQL("delete from bookmark where idFile=%i and type=%i", idFile, (int)type);
    m_pDS->exec(strSQL.c_str());
    if (type == CBookmark::EPISODE)
    {
      strSQL=PrepareSQL("update episode set c%02d=-1 where idFile=%i", VIDEODB_ID_EPISODE_BOOKMARK, idFile);
      m_pDS->exec(strSQL.c_str());
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%d) failed", __FUNCTION__, idFile);
  }
}


bool CVideoDatabase::GetBookMarkForEpisode(const CVideoInfoTag& tag, CBookmark& bookmark)
{
  try
  {
    CStdString strSQL = PrepareSQL("select bookmark.* from bookmark join episode on episode.c%02d=bookmark.idBookmark where episode.idEpisode=%i", VIDEODB_ID_EPISODE_BOOKMARK, tag.m_iDbId);
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
    CStdString strSQL = PrepareSQL("delete from bookmark where idBookmark in (select c%02d from episode where c%02d=%i and c%02d=%i and idFile=%i)", VIDEODB_ID_EPISODE_BOOKMARK, VIDEODB_ID_EPISODE_SEASON, tag.m_iSeason, VIDEODB_ID_EPISODE_EPISODE, tag.m_iEpisode, idFile);
    m_pDS->exec(strSQL.c_str());

    AddBookMarkToFile(tag.m_strFileNameAndPath, bookmark, CBookmark::EPISODE);
    int idBookmark = (int)m_pDS->lastinsertid();
    strSQL = PrepareSQL("update episode set c%02d=%i where c%02d=%i and c%02d=%i and idFile=%i", VIDEODB_ID_EPISODE_BOOKMARK, idBookmark, VIDEODB_ID_EPISODE_SEASON, tag.m_iSeason, VIDEODB_ID_EPISODE_EPISODE, tag.m_iEpisode, idFile);
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
    CStdString strSQL = PrepareSQL("delete from bookmark where idBookmark in (select c%02d from episode where idEpisode=%i)", VIDEODB_ID_EPISODE_BOOKMARK, tag.m_iDbId);
    m_pDS->exec(strSQL.c_str());
    strSQL = PrepareSQL("update episode set c%02d=-1 where idEpisode=%i", VIDEODB_ID_EPISODE_BOOKMARK, tag.m_iDbId);
    m_pDS->exec(strSQL.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%i) failed", __FUNCTION__, tag.m_iDbId);
  }
}

//********************************************************************************************************************************
void CVideoDatabase::DeleteMovie(const CStdString& strFilenameAndPath, bool bKeepId /* = false */)
{
  int idMovie = GetMovieId(strFilenameAndPath);
  if (idMovie > -1)
    DeleteMovie(idMovie, bKeepId);
}

void CVideoDatabase::DeleteMovie(int idMovie, bool bKeepId /* = false */)
{
  if (idMovie < 0)
    return;

  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    BeginTransaction();

    CStdString strSQL;
    strSQL=PrepareSQL("delete from genrelinkmovie where idMovie=%i", idMovie);
    m_pDS->exec(strSQL.c_str());

    strSQL=PrepareSQL("delete from actorlinkmovie where idMovie=%i", idMovie);
    m_pDS->exec(strSQL.c_str());

    strSQL=PrepareSQL("delete from directorlinkmovie where idMovie=%i", idMovie);
    m_pDS->exec(strSQL.c_str());

    strSQL=PrepareSQL("delete from studiolinkmovie where idMovie=%i", idMovie);
    m_pDS->exec(strSQL.c_str());

    strSQL=PrepareSQL("delete from countrylinkmovie where idMovie=%i", idMovie);
    m_pDS->exec(strSQL.c_str());

    strSQL=PrepareSQL("delete from writerlinkmovie where idMovie=%i", idMovie);
    m_pDS->exec(strSQL.c_str());

    // keep the movie table entry, linking to tv shows, and bookmarks
    // so we can update the data in place
    // the ancilliary tables are still purged
    if (!bKeepId)
    {
      strSQL=PrepareSQL("delete from movie where idMovie=%i", idMovie);
      m_pDS->exec(strSQL.c_str());

      // TODO: Why are we invalidating paths here?
      int idFile = GetDbId(PrepareSQL("SELECT idFile FROM movie WHERE idMovie=%i", idMovie));
      std::string path = GetSingleValue(PrepareSQL("SELECT strPath FROM path JOIN files ON files.idPath=path.idPath WHERE files.idFile=%i", idFile));
      if (!path.empty())
        InvalidatePathHash(path);
    }

    //TODO: move this below CommitTransaction() once UPnP doesn't rely on this anymore
    if (!bKeepId)
      AnnounceRemove(MediaTypeMovie, idMovie);

    CommitTransaction();

  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
    RollbackTransaction();
  }
}

void CVideoDatabase::DeleteTvShow(const CStdString& strPath)
{
  int idTvShow = GetTvShowId(strPath);
  if (idTvShow >= 0)
    DeleteTvShow(idTvShow);
}

void CVideoDatabase::DeleteTvShow(int idTvShow, bool bKeepId /* = false */)
{
  if (idTvShow < 0)
    return;

  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    BeginTransaction();

    CStdString strSQL=PrepareSQL("SELECT episode.idEpisode FROM episode WHERE episode.idShow=%i",idTvShow);
    m_pDS2->query(strSQL.c_str());
    while (!m_pDS2->eof())
    {
      DeleteEpisode(m_pDS2->fv(0).get_asInt(), bKeepId);
      m_pDS2->next();
    }

    DeleteDetailsForTvShow(idTvShow);

    strSQL=PrepareSQL("delete from seasons where idShow=%i", idTvShow);
    m_pDS->exec(strSQL.c_str());

    // keep tvshow table and movielink table so we can update data in place
    if (!bKeepId)
    {
      strSQL=PrepareSQL("delete from tvshow where idShow=%i", idTvShow);
      m_pDS->exec(strSQL.c_str());

      strSQL=PrepareSQL("delete from tvshowlinkpath where idShow=%i", idTvShow);
      m_pDS->exec(strSQL.c_str());

      strSQL=PrepareSQL("delete from movielinktvshow where idShow=%i", idTvShow);
      m_pDS->exec(strSQL.c_str());

      // TODO: why do we invalidate the path hash here??
      set<int> paths;
      GetPathsForTvShow(idTvShow, paths);
      for (set<int>::const_iterator i = paths.begin(); i != paths.end(); ++i)
      {
        std::string path = GetSingleValue(PrepareSQL("SELECT strPath FROM path WHERE idPath=%i", *i));
        if (!path.empty())
          InvalidatePathHash(path);
      }
    }

    //TODO: move this below CommitTransaction() once UPnP doesn't rely on this anymore
    if (!bKeepId)
      AnnounceRemove(MediaTypeTvShow, idTvShow);

    CommitTransaction();

  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%d) failed", __FUNCTION__, idTvShow);
    RollbackTransaction();
  }
}

void CVideoDatabase::DeleteSeason(int idSeason, bool bKeepId /* = false */)
{
  if (idSeason < 0)
    return;

  try
  {
    if (m_pDB.get() == NULL ||
        m_pDS.get() == NULL ||
        m_pDS2.get() == NULL)
      return;

    BeginTransaction();

    CStdString strSQL = PrepareSQL("SELECT episode.idEpisode FROM episode "
                                   "JOIN seasons ON seasons.idSeason = %d AND episode.idShow = seasons.idShow AND episode.c%02d = seasons.season ",
                                   idSeason, VIDEODB_ID_EPISODE_SEASON);
    m_pDS2->query(strSQL.c_str());
    while (!m_pDS2->eof())
    {
      DeleteEpisode(m_pDS2->fv(0).get_asInt(), bKeepId);
      m_pDS2->next();
    }

    ExecuteQuery(PrepareSQL("DELETE FROM seasons WHERE idSeason = %i", idSeason));

    CommitTransaction();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%d) failed", __FUNCTION__, idSeason);
    RollbackTransaction();
  }
}

void CVideoDatabase::DeleteEpisode(const CStdString& strFilenameAndPath, bool bKeepId /* = false */)
{
  int idEpisode = GetEpisodeId(strFilenameAndPath);
  if (idEpisode > -1)
    DeleteEpisode(idEpisode, bKeepId);
}

void CVideoDatabase::DeleteEpisode(int idEpisode, bool bKeepId /* = false */)
{
  if (idEpisode < 0)
    return;

  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    //TODO: move this below CommitTransaction() once UPnP doesn't rely on this anymore
    if (!bKeepId)
      AnnounceRemove(MediaTypeEpisode, idEpisode);

    CStdString strSQL;
    strSQL=PrepareSQL("delete from actorlinkepisode where idEpisode=%i", idEpisode);
    m_pDS->exec(strSQL.c_str());

    strSQL=PrepareSQL("delete from directorlinkepisode where idEpisode=%i", idEpisode);
    m_pDS->exec(strSQL.c_str());

    strSQL=PrepareSQL("delete from writerlinkepisode where idEpisode=%i", idEpisode);
    m_pDS->exec(strSQL.c_str());

    // keep episode table entry and bookmarks so we can update the data in place
    // the ancilliary tables are still purged
    if (!bKeepId)
    {
      strSQL=PrepareSQL("delete from episode where idEpisode=%i", idEpisode);
      m_pDS->exec(strSQL.c_str());
    }

  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%d) failed", __FUNCTION__, idEpisode);
  }
}

void CVideoDatabase::DeleteMusicVideo(const CStdString& strFilenameAndPath, bool bKeepId /* = false */)
{
  int idMVideo = GetMusicVideoId(strFilenameAndPath);
  if (idMVideo > -1)
    DeleteMusicVideo(idMVideo, bKeepId);
}

void CVideoDatabase::DeleteMusicVideo(int idMVideo, bool bKeepId /* = false */)
{
  if (idMVideo < 0)
    return;

  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    BeginTransaction();

    CStdString strSQL;
    strSQL=PrepareSQL("delete from genrelinkmusicvideo where idMVideo=%i", idMVideo);
    m_pDS->exec(strSQL.c_str());

    strSQL=PrepareSQL("delete from artistlinkmusicvideo where idMVideo=%i", idMVideo);
    m_pDS->exec(strSQL.c_str());

    strSQL=PrepareSQL("delete from directorlinkmusicvideo where idMVideo=%i", idMVideo);
    m_pDS->exec(strSQL.c_str());

    strSQL=PrepareSQL("delete from studiolinkmusicvideo where idMVideo=%i", idMVideo);
    m_pDS->exec(strSQL.c_str());

    // keep the music video table entry and bookmarks so we can update data in place
    // the ancilliary tables are still purged
    if (!bKeepId)
    {
      strSQL=PrepareSQL("delete from musicvideo where idMVideo=%i", idMVideo);
      m_pDS->exec(strSQL.c_str());

      // TODO: Why are we invalidating paths here?
      int idFile = GetDbId(PrepareSQL("SELECT idFile FROM musicvideo WHERE idMVideo=%i", idMVideo));
      std::string path = GetSingleValue(PrepareSQL("SELECT strPath FROM path JOIN files ON files.idPath=path.idPath WHERE files.idFile=%i", idFile));
      if (!path.empty())
        InvalidatePathHash(path);
    }

    //TODO: move this below CommitTransaction() once UPnP doesn't rely on this anymore
    if (!bKeepId)
      AnnounceRemove(MediaTypeMusicVideo, idMVideo);

    CommitTransaction();

  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
    RollbackTransaction();
  }
}

int CVideoDatabase::GetDbId(const std::string &query)
{
  std::string result = GetSingleValue(query);
  if (!result.empty())
  {
    int idDb = strtol(result.c_str(), NULL, 10);
    if (idDb > 0)
      return idDb;
  }
  return -1;
}

void CVideoDatabase::DeleteStreamDetails(int idFile)
{
    m_pDS->exec(PrepareSQL("delete from streamdetails where idFile=%i", idFile));
}

void CVideoDatabase::DeleteSet(int idSet)
{
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    CStdString strSQL;
    strSQL=PrepareSQL("delete from sets where idSet = %i", idSet);
    m_pDS->exec(strSQL.c_str());
    strSQL=PrepareSQL("update movie set idSet = null where idSet = %i", idSet);
    m_pDS->exec(strSQL.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%i) failed", __FUNCTION__, idSet);
  }
}

void CVideoDatabase::ClearMovieSet(int idMovie)
{
  SetMovieSet(idMovie, -1);
}

void CVideoDatabase::SetMovieSet(int idMovie, int idSet)
{
  if (idSet >= 0)
    ExecuteQuery(PrepareSQL("update movie set idSet = %i where idMovie = %i", idSet, idMovie));
  else
    ExecuteQuery(PrepareSQL("update movie set idSet = null where idMovie = %i", idMovie));
}

void CVideoDatabase::DeleteTag(int idTag, VIDEODB_CONTENT_TYPE mediaType)
{
  try
  {
    if (m_pDB.get() == NULL || m_pDS.get() == NULL)
      return;

    std::string type;
    if (mediaType == VIDEODB_CONTENT_MOVIES)
      type = MediaTypeMovie;
    else if (mediaType == VIDEODB_CONTENT_TVSHOWS)
      type = MediaTypeTvShow;
    else if (mediaType == VIDEODB_CONTENT_MUSICVIDEOS)
      type = MediaTypeMusicVideo;
    else
      return;

    CStdString strSQL;
    strSQL = PrepareSQL("DELETE FROM taglinks WHERE idTag = %i AND media_type = '%s'", idTag, type.c_str());
    m_pDS->exec(strSQL.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%i) failed", __FUNCTION__, idTag);
  }
}

void CVideoDatabase::GetDetailsFromDB(auto_ptr<Dataset> &pDS, int min, int max, const SDbTableOffsets *offsets, CVideoInfoTag &details, int idxOffset)
{
  GetDetailsFromDB(pDS->get_sql_record(), min, max, offsets, details, idxOffset);
}

void CVideoDatabase::GetDetailsFromDB(const dbiplus::sql_record* const record, int min, int max, const SDbTableOffsets *offsets, CVideoInfoTag &details, int idxOffset)
{
  for (int i = min + 1; i < max; i++)
  {
    switch (offsets[i].type)
    {
    case VIDEODB_TYPE_STRING:
      *(CStdString*)(((char*)&details)+offsets[i].offset) = record->at(i+idxOffset).get_asString();
      break;
    case VIDEODB_TYPE_INT:
    case VIDEODB_TYPE_COUNT:
      *(int*)(((char*)&details)+offsets[i].offset) = record->at(i+idxOffset).get_asInt();
      break;
    case VIDEODB_TYPE_BOOL:
      *(bool*)(((char*)&details)+offsets[i].offset) = record->at(i+idxOffset).get_asBool();
      break;
    case VIDEODB_TYPE_FLOAT:
      *(float*)(((char*)&details)+offsets[i].offset) = record->at(i+idxOffset).get_asFloat();
      break;
    case VIDEODB_TYPE_STRINGARRAY:
      *(std::vector<std::string>*)(((char*)&details)+offsets[i].offset) = StringUtils::Split(record->at(i+idxOffset).get_asString(), g_advancedSettings.m_videoItemSeparator);
      break;
    case VIDEODB_TYPE_DATE:
      ((CDateTime*)(((char*)&details)+offsets[i].offset))->SetFromDBDate(record->at(i+idxOffset).get_asString());
      break;
    case VIDEODB_TYPE_DATETIME:
      ((CDateTime*)(((char*)&details)+offsets[i].offset))->SetFromDBDateTime(record->at(i+idxOffset).get_asString());
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
      break;
    default:
      break;
  }

  return details;
}

bool CVideoDatabase::GetStreamDetails(CFileItem& item)
{
  // Note that this function (possibly) creates VideoInfoTags for items that don't have one yet!
  int fileId = -1;

  if (item.HasVideoInfoTag())
    fileId = item.GetVideoInfoTag()->m_iFileId;

  if (fileId < 0)
    fileId = GetFileId(item);

  if (fileId < 0)
    return false;

  // Have a file id, get stream details if available (creates tag either way)
  item.GetVideoInfoTag()->m_iFileId = fileId;
  return GetStreamDetails(*item.GetVideoInfoTag());
}

bool CVideoDatabase::GetStreamDetails(CVideoInfoTag& tag) const
{
  if (tag.m_iFileId < 0)
    return false;

  bool retVal = false;

  CStreamDetails& details = tag.m_streamDetails;
  details.Reset();

  auto_ptr<Dataset> pDS(m_pDB->CreateDataset());
  try
  {
    CStdString strSQL = PrepareSQL("SELECT * FROM streamdetails WHERE idFile = %i", tag.m_iFileId);
    pDS->query(strSQL);

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
          p->m_iDuration = pDS->fv(10).get_asInt();
          p->m_strStereoMode = pDS->fv(11).get_asString();
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
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%i) failed", __FUNCTION__, tag.m_iFileId);
  }
  details.DetermineBestStreams();

  if (details.GetVideoDuration() > 0)
    tag.m_duration = details.GetVideoDuration();

  return retVal;
}
 
bool CVideoDatabase::GetResumePoint(CVideoInfoTag& tag)
{
  if (tag.m_iFileId < 0)
    return false;

  bool match = false;

  try
  {
    if (URIUtils::IsStack(tag.m_strFileNameAndPath) && CFileItem(CStackDirectory::GetFirstStackedFile(tag.m_strFileNameAndPath),false).IsDiscImage())
    {
      CStackDirectory dir;
      CFileItemList fileList;
      const CURL pathToUrl(tag.m_strFileNameAndPath);
      dir.GetDirectory(pathToUrl, fileList);
      tag.m_resumePoint.Reset();
      for (int i = fileList.Size() - 1; i >= 0; i--)
      {
        CBookmark bookmark;
        if (GetResumeBookMark(fileList[i]->GetPath(), bookmark))
        {
          tag.m_resumePoint = bookmark;
          tag.m_resumePoint.partNumber = (i+1); /* store part number in here */
          match = true;
          break;
        }
      }
    }
    else
    {
      CStdString strSQL=PrepareSQL("select timeInSeconds, totalTimeInSeconds from bookmark where idFile=%i and type=%i order by timeInSeconds", tag.m_iFileId, CBookmark::RESUME);
      m_pDS2->query( strSQL.c_str() );
      if (!m_pDS2->eof())
      {
        tag.m_resumePoint.timeInSeconds = m_pDS2->fv(0).get_asDouble();
        tag.m_resumePoint.totalTimeInSeconds = m_pDS2->fv(1).get_asDouble();
        tag.m_resumePoint.partNumber = 0; // regular files or non-iso stacks don't need partNumber
        tag.m_resumePoint.type = CBookmark::RESUME;
        match = true;
      }
      m_pDS2->close();
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, tag.m_strFileNameAndPath.c_str());
  }

  return match;
}

CVideoInfoTag CVideoDatabase::GetDetailsForMovie(auto_ptr<Dataset> &pDS, bool getDetails /* = false */)
{
  return GetDetailsForMovie(pDS->get_sql_record(), getDetails);
}

CVideoInfoTag CVideoDatabase::GetDetailsForMovie(const dbiplus::sql_record* const record, bool getDetails /* = false */)
{
  CVideoInfoTag details;

  if (record == NULL)
    return details;

  DWORD time = XbmcThreads::SystemClockMillis();
  int idMovie = record->at(0).get_asInt();

  GetDetailsFromDB(record, VIDEODB_ID_MIN, VIDEODB_ID_MAX, DbMovieOffsets, details);

  details.m_iDbId = idMovie;
  details.m_type = MediaTypeMovie;
  
  details.m_iSetId = record->at(VIDEODB_DETAILS_MOVIE_SET_ID).get_asInt();
  details.m_strSet = record->at(VIDEODB_DETAILS_MOVIE_SET_NAME).get_asString();
  details.m_iFileId = record->at(VIDEODB_DETAILS_FILEID).get_asInt();
  details.m_strPath = record->at(VIDEODB_DETAILS_MOVIE_PATH).get_asString();
  CStdString strFileName = record->at(VIDEODB_DETAILS_MOVIE_FILE).get_asString();
  ConstructPath(details.m_strFileNameAndPath,details.m_strPath,strFileName);
  details.m_playCount = record->at(VIDEODB_DETAILS_MOVIE_PLAYCOUNT).get_asInt();
  details.m_lastPlayed.SetFromDBDateTime(record->at(VIDEODB_DETAILS_MOVIE_LASTPLAYED).get_asString());
  details.m_dateAdded.SetFromDBDateTime(record->at(VIDEODB_DETAILS_MOVIE_DATEADDED).get_asString());
  details.m_resumePoint.timeInSeconds = record->at(VIDEODB_DETAILS_MOVIE_RESUME_TIME).get_asInt();
  details.m_resumePoint.totalTimeInSeconds = record->at(VIDEODB_DETAILS_MOVIE_TOTAL_TIME).get_asInt();
  details.m_resumePoint.type = CBookmark::RESUME;

  movieTime += XbmcThreads::SystemClockMillis() - time; time = XbmcThreads::SystemClockMillis();

  if (getDetails)
  {
    GetCast("movie", "idMovie", details.m_iDbId, details.m_cast);

    castTime += XbmcThreads::SystemClockMillis() - time; time = XbmcThreads::SystemClockMillis();
    details.m_strPictureURL.Parse();

    // get tags
    CStdString strSQL = PrepareSQL("SELECT tag.strTag FROM tag, taglinks WHERE taglinks.idMedia = %i AND taglinks.media_type = 'movie' AND taglinks.idTag = tag.idTag ORDER BY tag.idTag", idMovie);
    m_pDS2->query(strSQL.c_str());
    while (!m_pDS2->eof())
    {
      details.m_tags.push_back(m_pDS2->fv("tag.strTag").get_asString());
      m_pDS2->next();
    }

    // create tvshowlink string
    vector<int> links;
    GetLinksToTvShow(idMovie,links);
    for (unsigned int i=0;i<links.size();++i)
    {
      CStdString strSQL = PrepareSQL("select c%02d from tvshow where idShow=%i",
                         VIDEODB_ID_TV_TITLE,links[i]);
      m_pDS2->query(strSQL.c_str());
      if (!m_pDS2->eof())
        details.m_showLink.push_back(m_pDS2->fv(0).get_asString());
    }
    m_pDS2->close();

    // get streamdetails
    GetStreamDetails(details);
  }
  return details;
}

CVideoInfoTag CVideoDatabase::GetDetailsForTvShow(auto_ptr<Dataset> &pDS, bool getDetails /* = false */, CFileItem* item /* = NULL */)
{
  return GetDetailsForTvShow(pDS->get_sql_record(), getDetails, item);
}

CVideoInfoTag CVideoDatabase::GetDetailsForTvShow(const dbiplus::sql_record* const record, bool getDetails /* = false */, CFileItem* item /* = NULL */)
{
  CVideoInfoTag details;

  if (record == NULL)
    return details;

  DWORD time = XbmcThreads::SystemClockMillis();
  int idTvShow = record->at(0).get_asInt();

  GetDetailsFromDB(record, VIDEODB_ID_TV_MIN, VIDEODB_ID_TV_MAX, DbTvShowOffsets, details, 1);
  details.m_iDbId = idTvShow;
  details.m_type = MediaTypeTvShow;
  details.m_strPath = record->at(VIDEODB_DETAILS_TVSHOW_PATH).get_asString();
  details.m_basePath = details.m_strPath;
  details.m_parentPathID = record->at(VIDEODB_DETAILS_TVSHOW_PARENTPATHID).get_asInt();
  details.m_dateAdded.SetFromDBDateTime(record->at(VIDEODB_DETAILS_TVSHOW_DATEADDED).get_asString());
  details.m_lastPlayed.SetFromDBDateTime(record->at(VIDEODB_DETAILS_TVSHOW_LASTPLAYED).get_asString());
  details.m_iEpisode = record->at(VIDEODB_DETAILS_TVSHOW_NUM_EPISODES).get_asInt();
  details.m_playCount = record->at(VIDEODB_DETAILS_TVSHOW_NUM_WATCHED).get_asInt();
  details.m_strShowTitle = details.m_strTitle;
  if (details.m_premiered.IsValid())
    details.m_iYear = details.m_premiered.GetYear();

  movieTime += XbmcThreads::SystemClockMillis() - time; time = XbmcThreads::SystemClockMillis();

  if (getDetails)
  {
    GetCast("tvshow", "idShow", details.m_iDbId, details.m_cast);

    // get tags
    CStdString strSQL = PrepareSQL("SELECT tag.strTag FROM tag, taglinks WHERE taglinks.idMedia = %i AND taglinks.media_type = 'tvshow' AND taglinks.idTag = tag.idTag ORDER BY tag.idTag", idTvShow);
    m_pDS2->query(strSQL.c_str());
    while (!m_pDS2->eof())
    {
      details.m_tags.push_back(m_pDS2->fv("tag.strTag").get_asString());
      m_pDS2->next();
    }

    castTime += XbmcThreads::SystemClockMillis() - time; time = XbmcThreads::SystemClockMillis();
    details.m_strPictureURL.Parse();
  }

  if (item != NULL)
  {
    item->m_dateTime = details.m_premiered;
    item->SetProperty("totalseasons", record->at(VIDEODB_DETAILS_TVSHOW_NUM_SEASONS).get_asInt());
    item->SetProperty("totalepisodes", details.m_iEpisode);
    item->SetProperty("numepisodes", details.m_iEpisode); // will be changed later to reflect watchmode setting
    item->SetProperty("watchedepisodes", details.m_playCount);
    item->SetProperty("unwatchedepisodes", details.m_iEpisode - details.m_playCount);
  }
  details.m_playCount = (details.m_iEpisode <= details.m_playCount) ? 1 : 0;

  return details;
}

CVideoInfoTag CVideoDatabase::GetDetailsForEpisode(auto_ptr<Dataset> &pDS, bool getDetails /* = false */)
{
  return GetDetailsForEpisode(pDS->get_sql_record(), getDetails);
}

CVideoInfoTag CVideoDatabase::GetDetailsForEpisode(const dbiplus::sql_record* const record, bool getDetails /* = false */)
{
  CVideoInfoTag details;

  if (record == NULL)
    return details;

  DWORD time = XbmcThreads::SystemClockMillis();
  int idEpisode = record->at(0).get_asInt();

  GetDetailsFromDB(record, VIDEODB_ID_EPISODE_MIN, VIDEODB_ID_EPISODE_MAX, DbEpisodeOffsets, details);
  details.m_iDbId = idEpisode;
  details.m_type = MediaTypeEpisode;
  details.m_iFileId = record->at(VIDEODB_DETAILS_FILEID).get_asInt();
  details.m_strPath = record->at(VIDEODB_DETAILS_EPISODE_PATH).get_asString();
  CStdString strFileName = record->at(VIDEODB_DETAILS_EPISODE_FILE).get_asString();
  ConstructPath(details.m_strFileNameAndPath,details.m_strPath,strFileName);
  details.m_playCount = record->at(VIDEODB_DETAILS_EPISODE_PLAYCOUNT).get_asInt();
  details.m_lastPlayed.SetFromDBDateTime(record->at(VIDEODB_DETAILS_EPISODE_LASTPLAYED).get_asString());
  details.m_dateAdded.SetFromDBDateTime(record->at(VIDEODB_DETAILS_EPISODE_DATEADDED).get_asString());
  details.m_strMPAARating = record->at(VIDEODB_DETAILS_EPISODE_TVSHOW_MPAA).get_asString();
  details.m_strShowTitle = record->at(VIDEODB_DETAILS_EPISODE_TVSHOW_NAME).get_asString();
  details.m_studio = StringUtils::Split(record->at(VIDEODB_DETAILS_EPISODE_TVSHOW_STUDIO).get_asString(), g_advancedSettings.m_videoItemSeparator);
  details.m_premiered.SetFromDBDate(record->at(VIDEODB_DETAILS_EPISODE_TVSHOW_AIRED).get_asString());
  details.m_iIdShow = record->at(VIDEODB_DETAILS_EPISODE_TVSHOW_ID).get_asInt();
  details.m_iIdSeason = record->at(VIDEODB_DETAILS_EPISODE_SEASON_ID).get_asInt();

  details.m_resumePoint.timeInSeconds = record->at(VIDEODB_DETAILS_EPISODE_RESUME_TIME).get_asInt();
  details.m_resumePoint.totalTimeInSeconds = record->at(VIDEODB_DETAILS_EPISODE_TOTAL_TIME).get_asInt();
  details.m_resumePoint.type = CBookmark::RESUME;

  movieTime += XbmcThreads::SystemClockMillis() - time; time = XbmcThreads::SystemClockMillis();

  if (getDetails)
  {
    GetCast("episode", "idEpisode", details.m_iDbId, details.m_cast);
    GetCast("tvshow", "idShow", details.m_iIdShow, details.m_cast);

    castTime += XbmcThreads::SystemClockMillis() - time; time = XbmcThreads::SystemClockMillis();
    details.m_strPictureURL.Parse();
    CStdString strSQL = PrepareSQL("select * from bookmark join episode on episode.c%02d=bookmark.idBookmark where episode.idEpisode=%i and bookmark.type=%i", VIDEODB_ID_EPISODE_BOOKMARK,details.m_iDbId,CBookmark::EPISODE);
    m_pDS2->query(strSQL.c_str());
    if (!m_pDS2->eof())
      details.m_fEpBookmark = m_pDS2->fv("bookmark.timeInSeconds").get_asFloat();
    m_pDS2->close();

    // get streamdetails
    GetStreamDetails(details);
  }
  return details;
}

CVideoInfoTag CVideoDatabase::GetDetailsForMusicVideo(auto_ptr<Dataset> &pDS, bool getDetails /* = false */)
{
  return GetDetailsForMusicVideo(pDS->get_sql_record(), getDetails);
}

CVideoInfoTag CVideoDatabase::GetDetailsForMusicVideo(const dbiplus::sql_record* const record, bool getDetails /* = false */)
{
  CVideoInfoTag details;

  unsigned int time = XbmcThreads::SystemClockMillis();
  int idMVideo = record->at(0).get_asInt();

  GetDetailsFromDB(record, VIDEODB_ID_MUSICVIDEO_MIN, VIDEODB_ID_MUSICVIDEO_MAX, DbMusicVideoOffsets, details);
  details.m_iDbId = idMVideo;
  details.m_type = MediaTypeMusicVideo;
  
  details.m_iFileId = record->at(VIDEODB_DETAILS_FILEID).get_asInt();
  details.m_strPath = record->at(VIDEODB_DETAILS_MUSICVIDEO_PATH).get_asString();
  CStdString strFileName = record->at(VIDEODB_DETAILS_MUSICVIDEO_FILE).get_asString();
  ConstructPath(details.m_strFileNameAndPath,details.m_strPath,strFileName);
  details.m_playCount = record->at(VIDEODB_DETAILS_MUSICVIDEO_PLAYCOUNT).get_asInt();
  details.m_lastPlayed.SetFromDBDateTime(record->at(VIDEODB_DETAILS_MUSICVIDEO_LASTPLAYED).get_asString());
  details.m_dateAdded.SetFromDBDateTime(record->at(VIDEODB_DETAILS_MUSICVIDEO_DATEADDED).get_asString());
  details.m_resumePoint.timeInSeconds = record->at(VIDEODB_DETAILS_MUSICVIDEO_RESUME_TIME).get_asInt();
  details.m_resumePoint.totalTimeInSeconds = record->at(VIDEODB_DETAILS_MUSICVIDEO_TOTAL_TIME).get_asInt();
  details.m_resumePoint.type = CBookmark::RESUME;

  movieTime += XbmcThreads::SystemClockMillis() - time; time = XbmcThreads::SystemClockMillis();

  if (getDetails)
  {
    // get tags
    CStdString strSQL = PrepareSQL("SELECT tag.strTag FROM tag, taglinks WHERE taglinks.idMedia = %i AND taglinks.media_type = 'musicvideo' AND taglinks.idTag = tag.idTag ORDER BY tag.idTag", idMVideo);
    m_pDS2->query(strSQL.c_str());
    while (!m_pDS2->eof())
    {
      details.m_tags.push_back(m_pDS2->fv("tag.strTag").get_asString());
      m_pDS2->next();
    }
    m_pDS2->close();

    details.m_strPictureURL.Parse();

    // get streamdetails
    GetStreamDetails(details);
  }
  return details;
}

void CVideoDatabase::GetCast(const CStdString &table, const CStdString &table_id, int type_id, vector<SActorInfo> &cast)
{
  try
  {
    if (!m_pDB.get()) return;
    if (!m_pDS2.get()) return;

    CStdString sql = PrepareSQL("SELECT actors.strActor,"
                                "  actorlink%s.strRole,"
                                "  actorlink%s.iOrder,"
                                "  actors.strThumb,"
                                "  art.url "
                                "FROM actorlink%s"
                                "  JOIN actors ON"
                                "    actorlink%s.idActor=actors.idActor"
                                "  LEFT JOIN art ON"
                                "    art.media_id=actors.idActor AND art.media_type='actor' AND art.type='thumb' "
                                "WHERE actorlink%s.%s=%i "
                                "ORDER BY actorlink%s.iOrder",table.c_str(), table.c_str(), table.c_str(), table.c_str(), table.c_str(), table_id.c_str(), type_id, table.c_str());
    m_pDS2->query(sql.c_str());
    while (!m_pDS2->eof())
    {
      SActorInfo info;
      info.strName = m_pDS2->fv(0).get_asString();
      bool found = false;
      for (vector<SActorInfo>::iterator i = cast.begin(); i != cast.end(); ++i)
      {
        if (i->strName == info.strName)
        {
          found = true;
          break;
        }
      }
      if (!found)
      {
        info.strRole = m_pDS2->fv(1).get_asString();
        info.order = m_pDS2->fv(2).get_asInt();
        info.thumbUrl.ParseString(m_pDS2->fv(3).get_asString());
        info.thumb = m_pDS2->fv(4).get_asString();
        cast.push_back(info);
      }
      m_pDS2->next();
    }
    m_pDS2->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%s,%s,%i) failed", __FUNCTION__, table.c_str(), table_id.c_str(), type_id);
  }
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
    URIUtils::Split(strFilenameAndPath, strPath, strFileName);
    CStdString strSQL=PrepareSQL("select * from settings, files, path where settings.idFile=files.idFile and path.idPath=files.idPath and path.strPath='%s' and files.strFileName='%s'", strPath.c_str() , strFileName.c_str());
#else
    int idFile = GetFileId(strFilenameAndPath);
    if (idFile < 0) return false;
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    // ok, now obtain the settings for this file
    CStdString strSQL=PrepareSQL("select * from settings where settings.idFile = '%i'", idFile);
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
      settings.m_PostProcess = m_pDS->fv("PostProcess").get_asBool();
      settings.m_Sharpness = m_pDS->fv("Sharpness").get_asFloat();
      settings.m_CustomZoomAmount = m_pDS->fv("ZoomAmount").get_asFloat();
      settings.m_CustomVerticalShift = m_pDS->fv("VerticalShift").get_asFloat();
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
      settings.m_DeinterlaceMode = (EDEINTERLACEMODE)m_pDS->fv("DeinterlaceMode").get_asInt();
      settings.m_InterlaceMethod = (EINTERLACEMETHOD)m_pDS->fv("Deinterlace").get_asInt();
      settings.m_VolumeAmplification = m_pDS->fv("VolumeAmplification").get_asFloat();
      settings.m_OutputToAllSpeakers = m_pDS->fv("OutputToAllSpeakers").get_asBool();
      settings.m_ScalingMethod = (ESCALINGMETHOD)m_pDS->fv("ScalingMethod").get_asInt();
      settings.m_StereoMode = m_pDS->fv("StereoMode").get_asInt();
      settings.m_StereoInvert = m_pDS->fv("StereoInvert").get_asBool();
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
    int idFile = AddFile(strFilenameAndPath);
    if (idFile < 0)
      return;
    CStdString strSQL = StringUtils::Format("select * from settings where idFile=%i", idFile);
    m_pDS->query( strSQL.c_str() );
    if (m_pDS->num_rows() > 0)
    {
      m_pDS->close();
      // update the item
      strSQL=PrepareSQL("update settings set Deinterlace=%i,ViewMode=%i,ZoomAmount=%f,PixelRatio=%f,VerticalShift=%f,"
                       "AudioStream=%i,SubtitleStream=%i,SubtitleDelay=%f,SubtitlesOn=%i,Brightness=%f,Contrast=%f,Gamma=%f,"
                       "VolumeAmplification=%f,AudioDelay=%f,OutputToAllSpeakers=%i,Sharpness=%f,NoiseReduction=%f,NonLinStretch=%i,PostProcess=%i,ScalingMethod=%i,"
                       "DeinterlaceMode=%i,",
                       setting.m_InterlaceMethod, setting.m_ViewMode, setting.m_CustomZoomAmount, setting.m_CustomPixelRatio, setting.m_CustomVerticalShift,
                       setting.m_AudioStream, setting.m_SubtitleStream, setting.m_SubtitleDelay, setting.m_SubtitleOn,
                       setting.m_Brightness, setting.m_Contrast, setting.m_Gamma, setting.m_VolumeAmplification, setting.m_AudioDelay,
                       setting.m_OutputToAllSpeakers,setting.m_Sharpness,setting.m_NoiseReduction,setting.m_CustomNonLinStretch,setting.m_PostProcess,setting.m_ScalingMethod,
                       setting.m_DeinterlaceMode);
      CStdString strSQL2;
      strSQL2=PrepareSQL("ResumeTime=%i,Crop=%i,CropLeft=%i,CropRight=%i,CropTop=%i,CropBottom=%i,StereoMode=%i,StereoInvert=%i where idFile=%i\n", setting.m_ResumeTime, setting.m_Crop, setting.m_CropLeft, setting.m_CropRight, setting.m_CropTop, setting.m_CropBottom, setting.m_StereoMode, setting.m_StereoInvert, idFile);
      strSQL += strSQL2;
      m_pDS->exec(strSQL.c_str());
      return ;
    }
    else
    { // add the items
      m_pDS->close();
      strSQL= "INSERT INTO settings (idFile,Deinterlace,ViewMode,ZoomAmount,PixelRatio, VerticalShift, "
                "AudioStream,SubtitleStream,SubtitleDelay,SubtitlesOn,Brightness,"
                "Contrast,Gamma,VolumeAmplification,AudioDelay,OutputToAllSpeakers,"
                "ResumeTime,Crop,CropLeft,CropRight,CropTop,CropBottom,"
                "Sharpness,NoiseReduction,NonLinStretch,PostProcess,ScalingMethod,DeinterlaceMode,StereoMode,StereoInvert) "
              "VALUES ";
      strSQL += PrepareSQL("(%i,%i,%i,%f,%f,%f,%i,%i,%f,%i,%f,%f,%f,%f,%f,%i,%i,%i,%i,%i,%i,%i,%f,%f,%i,%i,%i,%i,%i,%i)",
                           idFile, setting.m_InterlaceMethod, setting.m_ViewMode, setting.m_CustomZoomAmount, setting.m_CustomPixelRatio, setting.m_CustomVerticalShift,
                           setting.m_AudioStream, setting.m_SubtitleStream, setting.m_SubtitleDelay, setting.m_SubtitleOn, setting.m_Brightness,
                           setting.m_Contrast, setting.m_Gamma, setting.m_VolumeAmplification, setting.m_AudioDelay, setting.m_OutputToAllSpeakers,
                           setting.m_ResumeTime, setting.m_Crop, setting.m_CropLeft, setting.m_CropRight, setting.m_CropTop, setting.m_CropBottom,
                           setting.m_Sharpness, setting.m_NoiseReduction, setting.m_CustomNonLinStretch, setting.m_PostProcess, setting.m_ScalingMethod,
                           setting.m_DeinterlaceMode, setting.m_StereoMode, setting.m_StereoInvert);
      m_pDS->exec(strSQL.c_str());
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strFilenameAndPath.c_str());
  }
}

void CVideoDatabase::SetArtForItem(int mediaId, const MediaType &mediaType, const map<string, string> &art)
{
  for (map<string, string>::const_iterator i = art.begin(); i != art.end(); ++i)
    SetArtForItem(mediaId, mediaType, i->first, i->second);
}

void CVideoDatabase::SetArtForItem(int mediaId, const MediaType &mediaType, const string &artType, const string &url)
{
  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    // don't set <foo>.<bar> art types - these are derivative types from parent items
    if (artType.find('.') != string::npos)
      return;

    CStdString sql = PrepareSQL("SELECT art_id,url FROM art WHERE media_id=%i AND media_type='%s' AND type='%s'", mediaId, mediaType.c_str(), artType.c_str());
    m_pDS->query(sql.c_str());
    if (!m_pDS->eof())
    { // update
      int artId = m_pDS->fv(0).get_asInt();
      std::string oldUrl = m_pDS->fv(1).get_asString();
      m_pDS->close();
      if (oldUrl != url)
      {
        sql = PrepareSQL("UPDATE art SET url='%s' where art_id=%d", url.c_str(), artId);
        m_pDS->exec(sql.c_str());
      }
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

bool CVideoDatabase::GetArtForItem(int mediaId, const MediaType &mediaType, map<string, string> &art)
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

string CVideoDatabase::GetArtForItem(int mediaId, const MediaType &mediaType, const string &artType)
{
  std::string query = PrepareSQL("SELECT url FROM art WHERE media_id=%i AND media_type='%s' AND type='%s'", mediaId, mediaType.c_str(), artType.c_str());
  return GetSingleValue(query, m_pDS2);
}

bool CVideoDatabase::RemoveArtForItem(int mediaId, const MediaType &mediaType, const std::string &artType)
{
  return ExecuteQuery(PrepareSQL("DELETE FROM art WHERE media_id=%i AND media_type='%s' AND type='%s'", mediaId, mediaType.c_str(), artType.c_str()));
}

bool CVideoDatabase::RemoveArtForItem(int mediaId, const MediaType &mediaType, const std::set<std::string> &artTypes)
{
  bool result = true;
  for (set<string>::const_iterator i = artTypes.begin(); i != artTypes.end(); ++i)
    result &= RemoveArtForItem(mediaId, mediaType, *i);

  return result;
}

bool CVideoDatabase::GetTvShowSeasonArt(int showId, map<int, map<string, string> > &seasonArt)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS2.get()) return false; // using dataset 2 as we're likely called in loops on dataset 1

    // get all seasons for this show
    CStdString sql = PrepareSQL("select idSeason,season from seasons where idShow=%i", showId);
    m_pDS2->query(sql.c_str());

    vector< pair<int, int> > seasons;
    while (!m_pDS2->eof())
    {
      seasons.push_back(make_pair(m_pDS2->fv(0).get_asInt(), m_pDS2->fv(1).get_asInt()));
      m_pDS2->next();
    }
    m_pDS2->close();

    for (vector< pair<int,int> >::const_iterator i = seasons.begin(); i != seasons.end(); ++i)
    {
      map<string, string> art;
      GetArtForItem(i->first, MediaTypeSeason, art);
      seasonArt.insert(make_pair(i->second,art));
    }
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%d) failed", __FUNCTION__, showId);
  }
  return false;
}

bool CVideoDatabase::GetArtTypes(const MediaType &mediaType, std::vector<std::string> &artTypes)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString sql = PrepareSQL("SELECT DISTINCT type FROM art WHERE media_type='%s'", mediaType.c_str());
    int numRows = RunQuery(sql);
    if (numRows <= 0)
      return numRows == 0;

    while (!m_pDS->eof())
    {
      artTypes.push_back(m_pDS->fv(0).get_asString());
      m_pDS->next();
    }
    m_pDS->close();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%s) failed", __FUNCTION__, mediaType.c_str());
  }
  return false;
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
    CStdString strSQL=PrepareSQL("select times from stacktimes where idFile=%i\n", idFile);
    m_pDS->query( strSQL.c_str() );
    if (m_pDS->num_rows() > 0)
    { // get the video settings info
      int timeTotal = 0;
      vector<string> timeString = StringUtils::Split(m_pDS->fv("times").get_asString(), ",");
      times.clear();
      for (vector<string>::const_iterator i = timeString.begin(); i != timeString.end(); ++i)
      {
        times.push_back(atoi(i->c_str()));
        timeTotal += atoi(i->c_str());
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
    int idFile = AddFile(filePath);
    if (idFile < 0)
      return;

    // delete any existing items
    m_pDS->exec( PrepareSQL("delete from stacktimes where idFile=%i", idFile) );

    // add the items
    CStdString timeString = StringUtils::Format("%i", times[0]);
    for (unsigned int i = 1; i < times.size(); i++)
      timeString += StringUtils::Format(",%i", times[i]);

    m_pDS->exec( PrepareSQL("insert into stacktimes (idFile,times) values (%i,'%s')\n", idFile, timeString.c_str()) );
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, filePath.c_str());
  }
}

void CVideoDatabase::RemoveContentForPath(const CStdString& strPath, CGUIDialogProgress *progress /* = NULL */)
{
  if(URIUtils::IsMultiPath(strPath))
  {
    vector<std::string> paths;
    CMultiPathDirectory::GetPaths(strPath, paths);

    for(unsigned i=0;i<paths.size();i++)
      RemoveContentForPath(paths[i], progress);
  }

  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

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
    vector< pair<int,string> > paths;
    GetSubPaths(strPath, paths);
    int iCurr = 0;
    for (vector< pair<int, string> >::const_iterator i = paths.begin(); i != paths.end(); ++i)
    {
      bool bMvidsChecked=false;
      if (progress)
      {
        progress->SetPercentage((int)((float)(iCurr++)/paths.size()*100.f));
        progress->Progress();
      }

      if (HasTvShowInfo(i->second))
        DeleteTvShow(i->second);
      else
      {
        CStdString strSQL = PrepareSQL("select files.strFilename from files join movie on movie.idFile=files.idFile where files.idPath=%i", i->first);
        m_pDS2->query(strSQL.c_str());
        if (m_pDS2->eof())
        {
          strSQL = PrepareSQL("select files.strFilename from files join musicvideo on musicvideo.idFile=files.idFile where files.idPath=%i", i->first);
          m_pDS2->query(strSQL.c_str());
          bMvidsChecked = true;
        }
        while (!m_pDS2->eof())
        {
          CStdString strMoviePath;
          CStdString strFileName = m_pDS2->fv("files.strFilename").get_asString();
          ConstructPath(strMoviePath, i->second, strFileName);
          if (HasMovieInfo(strMoviePath))
            DeleteMovie(strMoviePath);
          if (HasMusicVideoInfo(strMoviePath))
            DeleteMusicVideo(strMoviePath);
          m_pDS2->next();
          if (m_pDS2->eof() && !bMvidsChecked)
          {
            strSQL =PrepareSQL("select files.strFilename from files join musicvideo on musicvideo.idFile=files.idFile where files.idPath=%i", i->first);
            m_pDS2->query(strSQL.c_str());
            bMvidsChecked = true;
          }
        }
        m_pDS2->close();
        m_pDS2->exec(PrepareSQL("update path set strContent='', strScraper='', strHash='',strSettings='',useFolderNames=0,scanRecursive=0 where idPath=%i", i->first));
      }
    }
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
  // if we have a multipath, set scraper for all contained paths
  if(URIUtils::IsMultiPath(filePath))
  {
    vector<std::string> paths;
    CMultiPathDirectory::GetPaths(filePath, paths);

    for(unsigned i=0;i<paths.size();i++)
      SetScraperForPath(paths[i],scraper,settings);

    return;
  }

  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    int idPath = AddPath(filePath);
    if (idPath < 0)
      return;

    // Update
    CStdString strSQL;
    if (settings.exclude)
    { //NB See note in ::GetScraperForPath about strContent=='none'
      strSQL=PrepareSQL("update path set strContent='', strScraper='', scanRecursive=0, useFolderNames=0, strSettings='', noUpdate=0 , exclude=1 where idPath=%i", idPath);
    }
    else if(!scraper)
    { // catch clearing content, but not excluding
      strSQL=PrepareSQL("update path set strContent='', strScraper='', scanRecursive=0, useFolderNames=0, strSettings='', noUpdate=0, exclude=0 where idPath=%i", idPath);
    }
    else
    {
      CStdString content = TranslateContent(scraper->Content());
      strSQL=PrepareSQL("update path set strContent='%s', strScraper='%s', scanRecursive=%i, useFolderNames=%i, strSettings='%s', noUpdate=%i, exclude=0 where idPath=%i", content.c_str(), scraper->ID().c_str(),settings.recurse,settings.parent_name,scraper->GetPathSettings().c_str(),settings.noupdate, idPath);
    }
    m_pDS->exec(strSQL.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, filePath.c_str());
  }
}

bool CVideoDatabase::ScraperInUse(const CStdString &scraperID) const
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString sql = PrepareSQL("select count(1) from path where strScraper='%s'", scraperID.c_str());
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

class CArtItem
{
public:
  CArtItem() { art_id = 0; media_id = 0; };
  int art_id;
  string art_type;
  string art_url;
  int media_id;
  string media_type;
};

// used for database update to v83
class CShowItem
{
public:
  bool operator==(const CShowItem &r) const
  {
    return (!ident.empty() && ident == r.ident) || (title == r.title && year == r.year);
  };
  int    id;
  int    path;
  string title;
  string year;
  string ident;
};

// used for database update to v84
class CShowLink
{
public:
  int show;
  int pathId;
  std::string path;
};

void CVideoDatabase::UpdateTables(int iVersion)
{
  if (iVersion < 61)
  {
    m_pDS->exec("ALTER TABLE path ADD dateAdded text");
    m_pDS->exec("ALTER TABLE files ADD dateAdded text");
  }
  if (iVersion < 62)
  { // add seasons table
    m_pDS->exec("CREATE TABLE seasons ( idSeason integer primary key, idShow integer, season integer)");
    // insert all seasons for each show
    m_pDS->query("SELECT idShow FROM tvshow");
    while (!m_pDS->eof())
    {
      CStdString sql = PrepareSQL("INSERT INTO seasons (idShow,season)"
                                  "  SELECT DISTINCT"
                                  "    idShow,c%02d"
                                  "  FROM"
                                  "    episode"
                                  "  JOIN tvshowlinkepisode ON"
                                  "    episode.idEpisode=tvshowlinkepisode.idEpisode"
                                  "  WHERE idShow=%i", VIDEODB_ID_EPISODE_SEASON, m_pDS->fv(0).get_asInt());
      m_pDS2->exec(sql.c_str());
      // and the "all seasons node"
      sql = PrepareSQL("INSERT INTO seasons (idShow,season) VALUES(%i,-1)", m_pDS->fv(0).get_asInt());
      m_pDS2->exec(sql.c_str());
      m_pDS->next();
    }
  }
  if (iVersion < 63)
  { // add art table
    m_pDS->exec("CREATE TABLE art(art_id INTEGER PRIMARY KEY, media_id INTEGER, media_type TEXT, type TEXT, url TEXT)");

    CMediaSettings::Get().SetVideoNeedsUpdate(63);
    CSettings::Get().Save();
  }
  if (iVersion < 64)
  { // add idShow to episode table
    m_pDS->exec("ALTER TABLE episode ADD idShow integer");
    m_pDS->query("SELECT idEpisode FROM episode");
    while (!m_pDS->eof())
    {
      int idEpisode = m_pDS->fv(0).get_asInt();
      CStdString update = PrepareSQL("UPDATE episode SET idShow=(SELECT idShow FROM tvshowlinkepisode WHERE idEpisode=%d) WHERE idEpisode=%d", idEpisode, idEpisode);
      m_pDS2->exec(update.c_str());
      m_pDS->next();
    }
    m_pDS->exec("DROP TABLE tvshowlinkepisode");
  }
  if (iVersion < 67)
  {
    m_pDS->exec("CREATE TABLE tag (idTag integer primary key, strTag text)");
    m_pDS->exec("CREATE TABLE taglinks (idTag integer, idMedia integer, media_type TEXT)");
  }
  if (iVersion < 68)
  { // add idSet to movie table
    m_pDS->exec("ALTER TABLE movie ADD idSet integer");
    m_pDS->query("SELECT idMovie FROM movie");
    while (!m_pDS->eof())
    {
      int idMovie = m_pDS->fv(0).get_asInt();
      CStdString sql = PrepareSQL("UPDATE movie SET idSet=(SELECT idSet FROM setlinkmovie WHERE idMovie = %d LIMIT 1) WHERE idMovie = %d", idMovie, idMovie);
      m_pDS2->exec(sql.c_str());
      m_pDS->next();
    }
    m_pDS->exec("DROP TABLE IF EXISTS setlinkmovie");
  }
  if (iVersion < 70)
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
  if (iVersion < 71)
  { // update URL encoded paths
    m_pDS->query("select idFile, strFilename from files");
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
      bool update = URIUtils::UpdateUrlEncoding(filename) &&
                    (!m_pDS->query(PrepareSQL("SELECT idFile FROM files WHERE strFilename = '%s'", filename.c_str()).c_str()) || m_pDS->num_rows() <= 0);
      m_pDS->close();

      if (update)
        m_pDS->exec(PrepareSQL("UPDATE files SET strFilename='%s' WHERE idFile=%d", filename.c_str(), i->first));
    }
  }
  if (iVersion < 72)
  { // Update thumb to poster or banner as applicable
    CTextureDatabase db;
    if (db.Open())
    {
      m_pDS->query("select art_id,url,media_id,media_type from art where type='thumb' and media_type in ('movie', 'musicvideo', 'tvshow', 'season', 'set')");
      vector<CArtItem> art;
      while (!m_pDS->eof())
      {
        CTextureDetails details;
        if (db.GetCachedTexture(m_pDS->fv(1).get_asString(), details))
        {
          CArtItem item;
          item.art_id = m_pDS->fv(0).get_asInt();
          item.art_url = m_pDS->fv(1).get_asString();
          item.art_type = CVideoInfoScanner::GetArtTypeFromSize(details.width, details.height);
          item.media_id = m_pDS->fv(2).get_asInt();
          item.media_type = m_pDS->fv(3).get_asString();
          if (item.art_type != "thumb")
            art.push_back(item);
        }
        m_pDS->next();
      }
      m_pDS->close();
      for (vector<CArtItem>::iterator i = art.begin(); i != art.end(); ++i)
      {
        if (GetArtForItem(i->media_id, i->media_type, i->art_type).empty())
          m_pDS->exec(PrepareSQL("update art set type='%s' where art_id=%d", i->art_type.c_str(), i->art_id));
        else
          m_pDS->exec(PrepareSQL("delete from art where art_id=%d", i->art_id));
      }
    }
  }
  if (iVersion < 74)
  { // update the runtime columns
    vector< pair<string, int> > tables;
    tables.push_back(make_pair("movie", VIDEODB_ID_RUNTIME));
    tables.push_back(make_pair("episode", VIDEODB_ID_EPISODE_RUNTIME));
    tables.push_back(make_pair("mvideo", VIDEODB_ID_MUSICVIDEO_RUNTIME));
    for (vector< pair<string, int> >::iterator i = tables.begin(); i != tables.end(); ++i)
    {
      CStdString sql = PrepareSQL("select id%s,c%02d from %s where c%02d != ''", i->first.c_str(), i->second, (i->first=="mvideo")?"musicvideo":i->first.c_str(), i->second);
      m_pDS->query(sql.c_str());
      vector< pair<int, int> > videos;
      while (!m_pDS->eof())
      {
        int duration = CVideoInfoTag::GetDurationFromMinuteString(m_pDS->fv(1).get_asString());
        if (duration)
          videos.push_back(make_pair(m_pDS->fv(0).get_asInt(), duration));
        m_pDS->next();
      }
      m_pDS->close();
      for (vector< pair<int, int> >::iterator j = videos.begin(); j != videos.end(); ++j)
        m_pDS->exec(PrepareSQL("update %s set c%02d=%d where id%s=%d", (i->first=="mvideo")?"musicvideo":i->first.c_str(), i->second, j->second, i->first.c_str(), j->first));
    }
  }
  if (iVersion < 76)
  {
    m_pDS->exec("ALTER TABLE settings ADD StereoMode integer");
    m_pDS->exec("ALTER TABLE settings ADD StereoInvert bool");
  }
  if (iVersion < 77)
    m_pDS->exec("ALTER TABLE streamdetails ADD strStereoMode text");

  if (iVersion < 81)
  { // add idParentPath to path table
    m_pDS->exec("ALTER TABLE path ADD idParentPath integer");
    map<string, int> paths;
    m_pDS->query("select idPath,strPath from path");
    while (!m_pDS->eof())
    {
      paths.insert(make_pair(m_pDS->fv(1).get_asString(), m_pDS->fv(0).get_asInt()));
      m_pDS->next();
    }
    m_pDS->close();
    // run through these paths figuring out the parent path, and add to the table if found
    for (map<string, int>::const_iterator i = paths.begin(); i != paths.end(); ++i)
    {
      std::string parent = URIUtils::GetParentPath(i->first);
      map<string, int>::const_iterator j = paths.find(parent);
      if (j != paths.end())
        m_pDS->exec(PrepareSQL("UPDATE path SET idParentPath=%i WHERE idPath=%i", j->second, i->second));
    }
  }
  if (iVersion < 82)
  {
    // drop parent path id and basePath from tvshow table
    m_pDS->exec("UPDATE tvshow SET c16=NULL,c17=NULL");
  }
  if (iVersion < 83)
  {
    // drop duplicates in tvshow table, and update tvshowlinkpath accordingly
    CStdString sql = PrepareSQL("SELECT tvshow.idShow,idPath,c%02d,c%02d,c%02d FROM tvshow JOIN tvshowlinkpath ON tvshow.idShow = tvshowlinkpath.idShow", VIDEODB_ID_TV_TITLE, VIDEODB_ID_TV_PREMIERED, VIDEODB_ID_TV_IDENT);
    m_pDS->query(sql.c_str());
    vector<CShowItem> shows;
    while (!m_pDS->eof())
    {
      CShowItem show;
      show.id    = m_pDS->fv(0).get_asInt();
      show.path  = m_pDS->fv(1).get_asInt();
      show.title = m_pDS->fv(2).get_asString();
      show.year  = m_pDS->fv(3).get_asString();
      show.ident = m_pDS->fv(4).get_asString();
      shows.push_back(show);
      m_pDS->next();
    }
    m_pDS->close();
    if (!shows.empty())
    {
      for (vector<CShowItem>::iterator i = shows.begin() + 1; i != shows.end(); ++i)
      {
        // has this show been found before?
        vector<CShowItem>::const_iterator j = find(shows.begin(), i, *i);
        if (j != i)
        { // this is a duplicate
          // update the tvshowlinkpath table
          m_pDS->exec(PrepareSQL("UPDATE tvshowlinkpath SET idShow = %d WHERE idShow = %d AND idPath = %d", j->id, i->id, i->path));
          // update episodes, seasons, movie links
          m_pDS->exec(PrepareSQL("UPDATE episode SET idShow = %d WHERE idShow = %d", j->id, i->id));
          m_pDS->exec(PrepareSQL("UPDATE seasons SET idShow = %d WHERE idShow = %d", j->id, i->id));
          m_pDS->exec(PrepareSQL("UPDATE movielinktvshow SET idShow = %d WHERE idShow = %d", j->id, i->id));
          // delete tvshow
          m_pDS->exec(PrepareSQL("DELETE FROM genrelinktvshow WHERE idShow=%i", i->id));
          m_pDS->exec(PrepareSQL("DELETE FROM actorlinktvshow WHERE idShow=%i", i->id));
          m_pDS->exec(PrepareSQL("DELETE FROM directorlinktvshow WHERE idShow=%i", i->id));
          m_pDS->exec(PrepareSQL("DELETE FROM studiolinktvshow WHERE idShow=%i", i->id));
          m_pDS->exec(PrepareSQL("DELETE FROM tvshow WHERE idShow = %d", i->id));
        }
      }
      // cleanup duplicate seasons
      m_pDS->exec("DELETE FROM seasons WHERE idSeason NOT IN (SELECT idSeason FROM (SELECT min(idSeason) FROM seasons GROUP BY idShow,season) AS sub)");
    }
  }
  if (iVersion < 84)
  { // replace any multipaths in tvshowlinkpath table
    m_pDS->query("SELECT idShow, tvshowlinkpath.idPath, strPath FROM tvshowlinkpath JOIN path ON tvshowlinkpath.idPath=path.idPath WHERE path.strPath LIKE 'multipath://%'");
    vector<CShowLink> shows;
    while (!m_pDS->eof())
    {
      CShowLink link;
      link.show   = m_pDS->fv(0).get_asInt();
      link.pathId = m_pDS->fv(1).get_asInt();
      link.path   = m_pDS->fv(2).get_asString();
      shows.push_back(link);
      m_pDS->next();
    }
    m_pDS->close();
    // update these
    for (vector<CShowLink>::const_iterator i = shows.begin(); i != shows.end(); ++i)
    {
      vector<string> paths;
      CMultiPathDirectory::GetPaths(i->path, paths);
      for (vector<string>::const_iterator j = paths.begin(); j != paths.end(); ++j)
      {
        int idPath = AddPath(*j, URIUtils::GetParentPath(*j));
        /* we can't rely on REPLACE INTO here as analytics (indices) aren't online yet */
        if (GetSingleValue(PrepareSQL("SELECT 1 FROM tvshowlinkpath WHERE idShow=%i AND idPath=%i", i->show, idPath)).empty())
          m_pDS->exec(PrepareSQL("INSERT INTO tvshowlinkpath(idShow, idPath) VALUES(%i,%i)", i->show, idPath));
      }
      m_pDS->exec(PrepareSQL("DELETE FROM tvshowlinkpath WHERE idShow=%i AND idPath=%i", i->show, i->pathId));
    }
  }
  if (iVersion < 85)
  {
    // drop multipaths from the path table - they're not needed for anything at all
    m_pDS->exec("DELETE FROM path WHERE strPath LIKE 'multipath://%'");
  }
  if (iVersion < 87)
  { // due to the tvshow merging above, there could be orphaned season or show art
    m_pDS->exec("DELETE from art WHERE media_type='tvshow' AND NOT EXISTS (SELECT 1 FROM tvshow WHERE tvshow.idShow = art.media_id)");
    m_pDS->exec("DELETE from art WHERE media_type='season' AND NOT EXISTS (SELECT 1 FROM seasons WHERE seasons.idSeason = art.media_id)");
  }
}

int CVideoDatabase::GetSchemaVersion() const
{
  return 89;
}

bool CVideoDatabase::LookupByFolders(const CStdString &path, bool shows)
{
  SScanSettings settings;
  bool foundDirectly = false;
  ScraperPtr scraper = GetScraperForPath(path, settings, foundDirectly);
  if (scraper && scraper->Content() == CONTENT_TVSHOWS && !shows)
    return false; // episodes
  return settings.parent_name_root; // shows, movies, musicvids
}

bool CVideoDatabase::GetPlayCounts(const CStdString &strPath, CFileItemList &items)
{
  if(URIUtils::IsMultiPath(strPath))
  {
    vector<std::string> paths;
    CMultiPathDirectory::GetPaths(strPath, paths);

    bool ret = false;
    for(unsigned i=0;i<paths.size();i++)
      ret |= GetPlayCounts(paths[i], items);

    return ret;
  }
  int pathID;
  if (URIUtils::IsPlugin(strPath))
  {
    CURL url(strPath);
    pathID = GetPathId(url.GetWithoutFilename());
  }
  else
    pathID = GetPathId(strPath);
  if (pathID < 0)
    return false; // path (and thus files) aren't in the database

  try
  {
    // error!
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    // TODO: also test a single query for the above and below
    CStdString sql = PrepareSQL(
      "SELECT"
      "  files.strFilename, files.playCount,"
      "  bookmark.timeInSeconds, bookmark.totalTimeInSeconds "
      "FROM files"
      "  LEFT JOIN bookmark ON"
      "    files.idFile = bookmark.idFile AND bookmark.type = %i"
      "  WHERE files.idPath=%i", (int)CBookmark::RESUME, pathID);

    if (RunQuery(sql) <= 0)
      return false;

    items.SetFastLookup(true); // note: it's possibly quicker the other way around (map on db returned items)?
    while (!m_pDS->eof())
    {
      CStdString path;
      ConstructPath(path, strPath, m_pDS->fv(0).get_asString());
      CFileItemPtr item = items.Get(path);
      if (item)
      {
        item->GetVideoInfoTag()->m_playCount = m_pDS->fv(1).get_asInt();
        if (!item->GetVideoInfoTag()->m_resumePoint.IsSet())
        {
          item->GetVideoInfoTag()->m_resumePoint.timeInSeconds = m_pDS->fv(2).get_asInt();
          item->GetVideoInfoTag()->m_resumePoint.totalTimeInSeconds = m_pDS->fv(3).get_asInt();
          item->GetVideoInfoTag()->m_resumePoint.type = CBookmark::RESUME;
        }
      }
      m_pDS->next();
    }
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}

int CVideoDatabase::GetPlayCount(const CFileItem &item)
{
  int id = GetFileId(item);
  if (id < 0)
    return 0;  // not in db, so not watched

  try
  {
    // error!
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    CStdString strSQL = PrepareSQL("select playCount from files WHERE idFile=%i", id);
    int count = 0;
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
    exec = PrepareSQL("UPDATE tvshow set c%02d='%s' WHERE idShow=%i", VIDEODB_ID_TV_FANART, item.GetVideoInfoTag()->m_fanart.m_xml.c_str(), item.GetVideoInfoTag()->m_iDbId);
  else if (type == VIDEODB_CONTENT_MOVIES)
    exec = PrepareSQL("UPDATE movie set c%02d='%s' WHERE idMovie=%i", VIDEODB_ID_FANART, item.GetVideoInfoTag()->m_fanart.m_xml.c_str(), item.GetVideoInfoTag()->m_iDbId);

  try
  {
    m_pDS->exec(exec.c_str());

    if (type == VIDEODB_CONTENT_TVSHOWS)
      AnnounceUpdate(MediaTypeTvShow, item.GetVideoInfoTag()->m_iDbId);
    else if (type == VIDEODB_CONTENT_MOVIES)
      AnnounceUpdate(MediaTypeMovie, item.GetVideoInfoTag()->m_iDbId);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s - error updating fanart for %s", __FUNCTION__, item.GetPath().c_str());
  }
}

void CVideoDatabase::SetPlayCount(const CFileItem &item, int count, const CDateTime &date)
{
  int id;
  if (item.HasProperty("original_listitem_url") &&
      URIUtils::IsPlugin(item.GetProperty("original_listitem_url").asString()))
  {
    CFileItem item2(item);
    item2.SetPath(item.GetProperty("original_listitem_url").asString());
    id = AddFile(item2);
  }
  else
    id = AddFile(item);
  if (id < 0)
    return;

  // and mark as watched
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    CStdString strSQL;
    if (count)
    {
      if (!date.IsValid())
        strSQL = PrepareSQL("update files set playCount=%i,lastPlayed='%s' where idFile=%i", count, CDateTime::GetCurrentDateTime().GetAsDBDateTime().c_str(), id);
      else
        strSQL = PrepareSQL("update files set playCount=%i,lastPlayed='%s' where idFile=%i", count, date.GetAsDBDateTime().c_str(), id);
    }
    else
    {
      if (!date.IsValid())
        strSQL = PrepareSQL("update files set playCount=NULL,lastPlayed=NULL where idFile=%i", id);
      else
        strSQL = PrepareSQL("update files set playCount=NULL,lastPlayed='%s' where idFile=%i", date.GetAsDBDateTime().c_str(), id);
    }

    m_pDS->exec(strSQL.c_str());

    // We only need to announce changes to video items in the library
    if (item.HasVideoInfoTag() && item.GetVideoInfoTag()->m_iDbId > 0)
    {
      CVariant data;
      if (g_application.IsVideoScanning())
        data["transaction"] = true;
      // Only provide the "playcount" value if it has actually changed
      if (item.GetVideoInfoTag()->m_playCount != count)
        data["playcount"] = count;
      ANNOUNCEMENT::CAnnouncementManager::Get().Announce(ANNOUNCEMENT::VideoLibrary, "xbmc", "OnUpdate", CFileItemPtr(new CFileItem(item)), data);
    }
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

void CVideoDatabase::UpdateLastPlayed(const CFileItem &item)
{
  SetPlayCount(item, GetPlayCount(item), CDateTime::GetCurrentDateTime());
}

void CVideoDatabase::UpdateMovieTitle(int idMovie, const CStdString& strNewMovieTitle, VIDEODB_CONTENT_TYPE iType)
{
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;
    CStdString content;
    if (iType == VIDEODB_CONTENT_MOVIES)
    {
      CLog::Log(LOGINFO, "Changing Movie:id:%i New Title:%s", idMovie, strNewMovieTitle.c_str());
      content = MediaTypeMovie;
    }
    else if (iType == VIDEODB_CONTENT_EPISODES)
    {
      CLog::Log(LOGINFO, "Changing Episode:id:%i New Title:%s", idMovie, strNewMovieTitle.c_str());
      content = MediaTypeEpisode;
    }
    else if (iType == VIDEODB_CONTENT_TVSHOWS)
    {
      CLog::Log(LOGINFO, "Changing TvShow:id:%i New Title:%s", idMovie, strNewMovieTitle.c_str());
      content = MediaTypeTvShow;
    }
    else if (iType == VIDEODB_CONTENT_MUSICVIDEOS)
    {
      CLog::Log(LOGINFO, "Changing MusicVideo:id:%i New Title:%s", idMovie, strNewMovieTitle.c_str());
      content = MediaTypeMusicVideo;
    }
    else if (iType == VIDEODB_CONTENT_MOVIE_SETS)
    {
      CLog::Log(LOGINFO, "Changing Movie set:id:%i New Title:%s", idMovie, strNewMovieTitle.c_str());
      CStdString strSQL = PrepareSQL("UPDATE sets SET strSet='%s' WHERE idSet=%i", strNewMovieTitle.c_str(), idMovie );
      m_pDS->exec(strSQL.c_str());
    }

    if (!content.empty())
    {
      SetSingleValue(iType, idMovie, FieldTitle, strNewMovieTitle);
      AnnounceUpdate(content, idMovie);
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (int idMovie, const CStdString& strNewMovieTitle) failed on MovieID:%i and Title:%s", __FUNCTION__, idMovie, strNewMovieTitle.c_str());
  }
}

bool CVideoDatabase::UpdateVideoSortTitle(int idDb, const CStdString& strNewSortTitle, VIDEODB_CONTENT_TYPE iType /* = VIDEODB_CONTENT_MOVIES */)
{
  try
  {
    if (NULL == m_pDB.get() || NULL == m_pDS.get())
      return false;
    if (iType != VIDEODB_CONTENT_MOVIES && iType != VIDEODB_CONTENT_TVSHOWS)
      return false;

    CStdString content = MediaTypeMovie;
    if (iType == VIDEODB_CONTENT_TVSHOWS)
      content = MediaTypeTvShow;

    if (SetSingleValue(iType, idDb, FieldSortTitle, strNewSortTitle))
    {
      AnnounceUpdate(content, idDb);
      return true;
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (int idDb, const CStdString& strNewSortTitle, VIDEODB_CONTENT_TYPE iType) failed on ID: %i and Sort Title: %s", __FUNCTION__, idDb, strNewSortTitle.c_str());
  }

  return false;
}

/// \brief EraseVideoSettings() Erases the videoSettings table and reconstructs it
void CVideoDatabase::EraseVideoSettings(const std::string &path /* = ""*/)
{
  try
  {
    std::string sql = "DELETE FROM settings";

    if (!path.empty())
    {
      Filter pathFilter;
      pathFilter.AppendWhere(PrepareSQL("idFile IN (SELECT idFile FROM files INNER JOIN path ON path.idPath = files.idPath AND path.strPath LIKE \"%s%%\")", path.c_str()));
      sql += std::string(" WHERE ") + pathFilter.where.c_str();

      CLog::Log(LOGINFO, "Deleting settings information for all files under %s", path.c_str());
    }
    else
      CLog::Log(LOGINFO, "Deleting settings information for all files");
    
    m_pDS->exec(sql);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
}

bool CVideoDatabase::GetGenresNav(const CStdString& strBaseDir, CFileItemList& items, int idContent /* = -1 */, const Filter &filter /* = Filter() */, bool countOnly /* = false */)
{
  return GetNavCommon(strBaseDir, items, "genre", idContent, filter, countOnly);
}

bool CVideoDatabase::GetCountriesNav(const CStdString& strBaseDir, CFileItemList& items, int idContent /* = -1 */, const Filter &filter /* = Filter() */, bool countOnly /* = false */)
{
  return GetNavCommon(strBaseDir, items, "country", idContent, filter, countOnly);
}

bool CVideoDatabase::GetStudiosNav(const CStdString& strBaseDir, CFileItemList& items, int idContent /* = -1 */, const Filter &filter /* = Filter() */, bool countOnly /* = false */)
{
  return GetNavCommon(strBaseDir, items, "studio", idContent, filter, countOnly);
}

bool CVideoDatabase::GetNavCommon(const CStdString& strBaseDir, CFileItemList& items, const CStdString &type, int idContent /* = -1 */, const Filter &filter /* = Filter() */, bool countOnly /* = false */)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL;
    Filter extFilter = filter;
    if (CProfilesManager::Get().GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
    {
      if (idContent == VIDEODB_CONTENT_MOVIES)
      {
        strSQL = "select %s " + PrepareSQL("from %s ", type.c_str());
        extFilter.fields = PrepareSQL("%s.id%s, %s.str%s, path.strPath, files.playCount", type.c_str(), type.c_str(), type.c_str(), type.c_str());
        extFilter.AppendJoin(PrepareSQL("join %slinkmovie on %s.id%s = %slinkmovie.id%s join movieview on %slinkmovie.idMovie = movieview.idMovie join files on files.idFile = movieview.idFile join path on path.idPath = files.idPath",
                                        type.c_str(), type.c_str(), type.c_str(), type.c_str(), type.c_str(), type.c_str()));
      }
      else if (idContent == VIDEODB_CONTENT_TVSHOWS) //this will not get tvshows with 0 episodes
      {
        strSQL = "select %s " + PrepareSQL("from %s ", type.c_str());
        extFilter.fields = PrepareSQL("%s.id%s, %s.str%s, path.strPath", type.c_str(), type.c_str(), type.c_str(), type.c_str());
        extFilter.AppendJoin(PrepareSQL("join %slinktvshow on %s.id%s = %slinktvshow.id%s join episodeview on %slinktvshow.idShow = episodeview.idShow join files on files.idFile = episodeview.idFile join path on path.idPath = files.idPath",
                                        type.c_str(), type.c_str(), type.c_str(), type.c_str(), type.c_str(), type.c_str()));
      }
      else if (idContent == VIDEODB_CONTENT_MUSICVIDEOS)
      {
        strSQL = "select %s " + PrepareSQL("from %s ", type.c_str());
        extFilter.fields = PrepareSQL("%s.id%s, %s.str%s, path.strPath, files.playCount", type.c_str(), type.c_str(), type.c_str(), type.c_str());
        extFilter.AppendJoin(PrepareSQL("join %slinkmusicvideo on %s.id%s = %slinkmusicvideo.id%s join musicvideoview on %slinkmusicvideo.idMVideo = musicvideoview.idMVideo join files on files.idFile = musicvideoview.idFile join path on path.idPath = files.idPath",
                                        type.c_str(), type.c_str(), type.c_str(), type.c_str(), type.c_str(), type.c_str()));
      }
      else
        return false;
    }
    else
    {
      if (idContent == VIDEODB_CONTENT_MOVIES)
      {
        strSQL = "select %s " + PrepareSQL("from %s ", type.c_str());
        extFilter.fields = PrepareSQL("%s.id%s, %s.str%s, count(1), count(files.playCount)", type.c_str(), type.c_str(), type.c_str(), type.c_str());
        extFilter.AppendJoin(PrepareSQL("join %slinkmovie on %s.id%s = %slinkmovie.id%s join movieview on %slinkmovie.idMovie = movieview.idMovie join files on files.idFile = movieview.idFile",
                                        type.c_str(), type.c_str(), type.c_str(), type.c_str(), type.c_str(), type.c_str()));
        extFilter.AppendGroup(PrepareSQL("%s.id%s", type.c_str(), type.c_str()));
      }
      else if (idContent == VIDEODB_CONTENT_TVSHOWS)
      {
        strSQL = "select %s " + PrepareSQL("from %s ", type.c_str());
        extFilter.fields = PrepareSQL("distinct %s.id%s, %s.str%s", type.c_str(), type.c_str(), type.c_str(), type.c_str());
        extFilter.AppendJoin(PrepareSQL("join %slinktvshow on %s.id%s = %slinktvshow.id%s join tvshowview on %slinktvshow.idShow = tvshowview.idShow",
                                        type.c_str(), type.c_str(), type.c_str(), type.c_str(), type.c_str(), type.c_str()));
      }
      else if (idContent == VIDEODB_CONTENT_MUSICVIDEOS)
      {
        strSQL = "select %s " + PrepareSQL("from %s ", type.c_str());
        extFilter.fields = PrepareSQL("%s.id%s, %s.str%s, count(1), count(files.playCount)", type.c_str(), type.c_str(), type.c_str(), type.c_str());
        extFilter.AppendJoin(PrepareSQL("join %slinkmusicvideo on %s.id%s = %slinkmusicvideo.id%s join musicvideoview on %slinkmusicvideo.idMVideo = musicvideoview.idMVideo join files on files.idFile = musicvideoview.idFile",
                                        type.c_str(), type.c_str(), type.c_str(), type.c_str(), type.c_str(), type.c_str()));
        extFilter.AppendGroup(PrepareSQL("%s.id%s", type.c_str(), type.c_str()));
      }
      else
        return false;
    }

    if (countOnly)
    {
      extFilter.fields = PrepareSQL("COUNT(DISTINCT %s.id%s)", type.c_str(), type.c_str());
      extFilter.group.clear();
      extFilter.order.clear();
    }
    strSQL = StringUtils::Format(strSQL.c_str(), !extFilter.fields.empty() ? extFilter.fields.c_str() : "*");

    CVideoDbUrl videoUrl;
    if (!BuildSQL(strBaseDir, strSQL, extFilter, strSQL, videoUrl))
      return false;

    int iRowsFound = RunQuery(strSQL);
    if (iRowsFound <= 0)
      return iRowsFound == 0;

    if (countOnly)
    {
      CFileItemPtr pItem(new CFileItem());
      pItem->SetProperty("total", iRowsFound == 1 ? m_pDS->fv(0).get_asInt() : iRowsFound);
      items.Add(pItem);

      m_pDS->close();
      return true;
    }

    if (CProfilesManager::Get().GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
    {
      map<int, pair<CStdString,int> > mapItems;
      map<int, pair<CStdString,int> >::iterator it;
      while (!m_pDS->eof())
      {
        int id = m_pDS->fv(0).get_asInt();
        CStdString str = m_pDS->fv(1).get_asString();

        // was this already found?
        it = mapItems.find(id);
        if (it == mapItems.end())
        {
          // check path
          if (g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv(2).get_asString()),*CMediaSourceSettings::Get().GetSources("video")))
          {
            if (idContent == VIDEODB_CONTENT_MOVIES || idContent == VIDEODB_CONTENT_MUSICVIDEOS)
              mapItems.insert(pair<int, pair<CStdString,int> >(id, pair<CStdString,int>(str,m_pDS->fv(3).get_asInt()))); //fv(3) is file.playCount
            else if (idContent == VIDEODB_CONTENT_TVSHOWS)
              mapItems.insert(pair<int, pair<CStdString,int> >(id, pair<CStdString,int>(str,0)));
          }
        }
        m_pDS->next();
      }
      m_pDS->close();

      for (it = mapItems.begin(); it != mapItems.end(); ++it)
      {
        CFileItemPtr pItem(new CFileItem(it->second.first));
        pItem->GetVideoInfoTag()->m_iDbId = it->first;
        pItem->GetVideoInfoTag()->m_type = type;

        CVideoDbUrl itemUrl = videoUrl;
        CStdString path = StringUtils::Format("%ld/", it->first);
        itemUrl.AppendPath(path);
        pItem->SetPath(itemUrl.ToString());

        pItem->m_bIsFolder = true;
        if (idContent == VIDEODB_CONTENT_MOVIES || idContent == VIDEODB_CONTENT_MUSICVIDEOS)
          pItem->GetVideoInfoTag()->m_playCount = it->second.second;
        if (!items.Contains(pItem->GetPath()))
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
        CFileItemPtr pItem(new CFileItem(m_pDS->fv(1).get_asString()));
        pItem->GetVideoInfoTag()->m_iDbId = m_pDS->fv(0).get_asInt();
        pItem->GetVideoInfoTag()->m_type = type;

        CVideoDbUrl itemUrl = videoUrl;
        CStdString path = StringUtils::Format("%ld/", m_pDS->fv(0).get_asInt());
        itemUrl.AppendPath(path);
        pItem->SetPath(itemUrl.ToString());

        pItem->m_bIsFolder = true;
        pItem->SetLabelPreformated(true);
        if (idContent == VIDEODB_CONTENT_MOVIES || idContent == VIDEODB_CONTENT_MUSICVIDEOS)
        { // fv(3) is the number of videos watched, fv(2) is the total number.  We set the playcount
          // only if the number of videos watched is equal to the total number (i.e. every video watched)
          pItem->GetVideoInfoTag()->m_playCount = (m_pDS->fv(3).get_asInt() == m_pDS->fv(2).get_asInt()) ? 1 : 0;
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

bool CVideoDatabase::GetTagsNav(const CStdString& strBaseDir, CFileItemList& items, int idContent /* = -1 */, const Filter &filter /* = Filter() */, bool countOnly /* = false */)
{
  CStdString mediaType;
  if (idContent == VIDEODB_CONTENT_MOVIES)
    mediaType = MediaTypeMovie;
  else if (idContent == VIDEODB_CONTENT_TVSHOWS)
    mediaType = MediaTypeTvShow;
  else if (idContent == VIDEODB_CONTENT_MUSICVIDEOS)
    mediaType = MediaTypeMusicVideo;
  else
    return false;

  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL = "SELECT %s FROM taglinks ";

    Filter extFilter = filter;
    extFilter.fields = "tag.idTag, tag.strTag";
    extFilter.AppendJoin("JOIN tag ON tag.idTag = taglinks.idTag");

    if (idContent == (int)VIDEODB_CONTENT_MOVIES)
      extFilter.AppendJoin("JOIN movieview ON movieview.idMovie = taglinks.idMedia");

    extFilter.AppendWhere(PrepareSQL("taglinks.media_type = '%s'", mediaType.c_str()));
    extFilter.AppendGroup("taglinks.idTag");

    if (countOnly)
    {
      extFilter.fields = "COUNT(DISTINCT taglinks.idTag)";
      extFilter.group.clear();
      extFilter.order.clear();
    }
    strSQL = StringUtils::Format(strSQL.c_str(), !extFilter.fields.empty() ? extFilter.fields.c_str() : "*");

    // parse the base path to get additional filters
    CVideoDbUrl videoUrl;
    if (!BuildSQL(strBaseDir, strSQL, extFilter, strSQL, videoUrl))
      return false;

    int iRowsFound = RunQuery(strSQL);
    if (iRowsFound <= 0)
      return iRowsFound == 0;

    if (countOnly)
    {
      CFileItemPtr pItem(new CFileItem());
      pItem->SetProperty("total", iRowsFound == 1 ? m_pDS->fv(0).get_asInt() : iRowsFound);
      items.Add(pItem);

      m_pDS->close();
      return true;
    }

    while (!m_pDS->eof())
    {
      int idTag = m_pDS->fv(0).get_asInt();

      CFileItemPtr pItem(new CFileItem(m_pDS->fv(1).get_asString()));
      pItem->m_bIsFolder = true;
      pItem->GetVideoInfoTag()->m_iDbId = idTag;
      pItem->GetVideoInfoTag()->m_type = "tag";

      CVideoDbUrl itemUrl = videoUrl;
      CStdString path = StringUtils::Format("%ld/", idTag);
      itemUrl.AppendPath(path);
      pItem->SetPath(itemUrl.ToString());

      if (!items.Contains(pItem->GetPath()))
      {
        pItem->SetLabelPreformated(true);
        items.Add(pItem);
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

bool CVideoDatabase::GetSetsNav(const CStdString& strBaseDir, CFileItemList& items, int idContent /* = -1 */, const Filter &filter /* = Filter() */, bool ignoreSingleMovieSets /* = false */)
{
  if (idContent != VIDEODB_CONTENT_MOVIES)
    return false;

  return GetSetsByWhere(strBaseDir, filter, items, ignoreSingleMovieSets);
}

bool CVideoDatabase::GetSetsByWhere(const CStdString& strBaseDir, const Filter &filter, CFileItemList& items, bool ignoreSingleMovieSets /* = false */)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CVideoDbUrl videoUrl;
    if (!videoUrl.FromString(strBaseDir))
      return false;

    Filter setFilter = filter;
    setFilter.join += " JOIN sets ON movieview.idSet = sets.idSet";
    if (!setFilter.order.empty())
      setFilter.order += ",";
    setFilter.order += "sets.idSet";

    if (!GetMoviesByWhere(strBaseDir, setFilter, items))
      return false;

    CFileItemList sets;
    if (!GroupUtils::Group(GroupBySet, strBaseDir, items, sets))
      return false;

    items.ClearItems();
    items.Append(sets);

    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}

bool CVideoDatabase::GetMusicVideoAlbumsNav(const CStdString& strBaseDir, CFileItemList& items, int idArtist /* = -1 */, const Filter &filter /* = Filter() */, bool countOnly /* = false */)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CVideoDbUrl videoUrl;
    if (!videoUrl.FromString(strBaseDir))
      return false;

    CStdString strSQL = "select %s from musicvideoview ";
    Filter extFilter = filter;
    extFilter.fields = PrepareSQL("musicvideoview.c%02d, musicvideoview.idMVideo, actors.strActor", VIDEODB_ID_MUSICVIDEO_ALBUM);
    extFilter.AppendJoin(PrepareSQL("join artistlinkmusicvideo on artistlinkmusicvideo.idMVideo = musicvideoview.idMVideo"));
    extFilter.AppendJoin(PrepareSQL("join actors on actors.idActor = artistlinkmusicvideo.idArtist"));
    if (CProfilesManager::Get().GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
    {
      extFilter.fields += ", path.strPath";
      extFilter.AppendJoin("join files on files.idFile = musicvideoview.idFile join path on path.idPath = files.idPath");
    }

    if (idArtist > -1)
      videoUrl.AddOption("artistid", idArtist);

    extFilter.AppendGroup(PrepareSQL("musicvideoview.c%02d", VIDEODB_ID_MUSICVIDEO_ALBUM));

    if (countOnly)
    {
      extFilter.fields = "COUNT(1)";
      extFilter.group.clear();
      extFilter.order.clear();
    }
    strSQL = StringUtils::Format(strSQL.c_str(), !extFilter.fields.empty() ? extFilter.fields.c_str() : "*");

    if (!BuildSQL(videoUrl.ToString(), strSQL, extFilter, strSQL, videoUrl))
      return false;

    int iRowsFound = RunQuery(strSQL);
    if (iRowsFound <= 0)
      return iRowsFound == 0;

    if (countOnly)
    {
      CFileItemPtr pItem(new CFileItem());
      pItem->SetProperty("total", iRowsFound == 1 ? m_pDS->fv(0).get_asInt() : iRowsFound);
      items.Add(pItem);

      m_pDS->close();
      return true;
    }

    if (CProfilesManager::Get().GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
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
          if (g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv("path.strPath").get_asString()),*CMediaSourceSettings::Get().GetSources("video")))
            mapAlbums.insert(make_pair(lidMVideo, make_pair(strAlbum,m_pDS->fv(2).get_asString())));
        }
        m_pDS->next();
      }
      m_pDS->close();

      for (it=mapAlbums.begin();it != mapAlbums.end();++it)
      {
        if (!it->second.first.empty())
        {
          CFileItemPtr pItem(new CFileItem(it->second.first));

          CVideoDbUrl itemUrl = videoUrl;
          CStdString path = StringUtils::Format("%ld/", it->first);
          itemUrl.AppendPath(path);
          pItem->SetPath(itemUrl.ToString());

          pItem->m_bIsFolder=true;
          pItem->SetLabelPreformated(true);
          if (!items.Contains(pItem->GetPath()))
          {
            pItem->GetVideoInfoTag()->m_artist.push_back(it->second.second);
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

          CVideoDbUrl itemUrl = videoUrl;
          CStdString path = StringUtils::Format("%ld/", m_pDS->fv(1).get_asInt());
          itemUrl.AppendPath(path);
          pItem->SetPath(itemUrl.ToString());

          pItem->m_bIsFolder=true;
          pItem->SetLabelPreformated(true);
          if (!items.Contains(pItem->GetPath()))
          {
            pItem->GetVideoInfoTag()->m_artist.push_back(m_pDS->fv(2).get_asString());
            items.Add(pItem);
          }
        }
        m_pDS->next();
      }
      m_pDS->close();
    }

//    CLog::Log(LOGDEBUG, __FUNCTION__" Time: %d ms", XbmcThreads::SystemClockMillis() - time);
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}

bool CVideoDatabase::GetWritersNav(const CStdString& strBaseDir, CFileItemList& items, int idContent /* = -1 */, const Filter &filter /* = Filter() */, bool countOnly /* = false */)
{
  return GetPeopleNav(strBaseDir, items, "writer", idContent, filter, countOnly);
}

bool CVideoDatabase::GetDirectorsNav(const CStdString& strBaseDir, CFileItemList& items, int idContent /* = -1 */, const Filter &filter /* = Filter() */, bool countOnly /* = false */)
{
  return GetPeopleNav(strBaseDir, items, "director", idContent, filter, countOnly);
}

bool CVideoDatabase::GetActorsNav(const CStdString& strBaseDir, CFileItemList& items, int idContent /* = -1 */, const Filter &filter /* = Filter() */, bool countOnly /* = false */)
{
  if (GetPeopleNav(strBaseDir, items, (idContent == VIDEODB_CONTENT_MUSICVIDEOS) ? "artist" : "actor", idContent, filter, countOnly))
  { // set thumbs - ideally this should be in the normal thumb setting routines
    for (int i = 0; i < items.Size() && !countOnly; i++)
    {
      CFileItemPtr pItem = items[i];
      if (idContent == VIDEODB_CONTENT_MUSICVIDEOS)
        pItem->SetIconImage("DefaultArtist.png");
      else
        pItem->SetIconImage("DefaultActor.png");
    }
    return true;
  }
  return false;
}

bool CVideoDatabase::GetPeopleNav(const CStdString& strBaseDir, CFileItemList& items, const CStdString &type, int idContent /* = -1 */, const Filter &filter /* = Filter() */, bool countOnly /* = false */)
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
    Filter extFilter = filter;
    if (CProfilesManager::Get().GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
    {
      if (idContent == VIDEODB_CONTENT_MOVIES)
      {
        strSQL = "select %s from actors ";
        extFilter.fields = "actors.idActor, actors.strActor, actors.strThumb, path.strPath, files.playCount";
        extFilter.AppendJoin(PrepareSQL("join %slinkmovie on actors.idActor = %slinkmovie.id%s join movieview on %slinkmovie.idMovie = movieview.idMovie join files on files.idFile = movieview.idFile join path on path.idPath = files.idPath",
                                        type.c_str(), type.c_str(), type.c_str(), type.c_str()));
      }
      else if (idContent == VIDEODB_CONTENT_TVSHOWS)
      {
        strSQL = "select %s from actors ";
        extFilter.fields = "actors.idActor, actors.strActor, actors.strThumb, path.strPath";
        extFilter.AppendJoin(PrepareSQL("join %slinktvshow on actors.idActor = %slinktvshow.id%s join episodeview on %slinktvshow.idShow = episodeview.idShow join files on files.idFile = episodeview.idFile join path on path.idPath = files.idPath",
                                        type.c_str(), type.c_str(), type.c_str(), type.c_str()));
      }
      else if (idContent == VIDEODB_CONTENT_EPISODES)
      {
        strSQL = "select %s from actors ";
        extFilter.fields = "actors.idActor, actors.strActor, actors.strThumb, path.strPath, files.playCount";
        extFilter.AppendJoin(PrepareSQL("join %slinkepisode on actors.idActor = %slinkepisode.id%s join episodeview on %slinkepisode.idEpisode = episodeview.idEpisode join files on files.idFile = episodeview.idFile join path on path.idPath = files.idPath",
                                        type.c_str(), type.c_str(), type.c_str(), type.c_str()));
      }
      else if (idContent == VIDEODB_CONTENT_MUSICVIDEOS)
      {
        strSQL = "select %s from actors ";
        extFilter.fields = "actors.idActor, actors.strActor, actors.strThumb, path.strPath, files.playCount";
        extFilter.AppendJoin(PrepareSQL("join %slinkmusicvideo on actors.idActor = %slinkmusicvideo.id%s join musicvideoview on %slinkmusicvideo.idMVideo = musicvideoview.idMVideo join files on files.idFile = musicvideoview.idFile join path on path.idPath = files.idPath",
                                        type.c_str(), type.c_str(), type.c_str(), type.c_str()));
      }
      else
        return false;
    }
    else
    {
      if (idContent == VIDEODB_CONTENT_MOVIES)
      {
        strSQL ="select %s from actors ";
        extFilter.fields = "actors.idActor, actors.strActor, actors.strThumb, count(1), count(files.playCount)";
        extFilter.AppendJoin(PrepareSQL("join %slinkmovie on actors.idActor = %slinkmovie.id%s join movieview on %slinkmovie.idMovie = movieview.idMovie join files on files.idFile = movieview.idFile",
                                        type.c_str(), type.c_str(), type.c_str(), type.c_str()));
        extFilter.AppendGroup("actors.idActor");
      }
      else if (idContent == VIDEODB_CONTENT_TVSHOWS)
      {
        strSQL = "select %s " + PrepareSQL("from actors, %slinktvshow, tvshowview ", type.c_str());
        extFilter.fields = "distinct actors.idActor, actors.strActor, actors.strThumb";
        extFilter.AppendWhere(PrepareSQL("actors.idActor = %slinktvshow.id%s and %slinktvshow.idShow = tvshowview.idShow",
                                         type.c_str(), type.c_str(), type.c_str()));
      }
      else if (idContent == VIDEODB_CONTENT_EPISODES)
      {
        strSQL = "select %s " + PrepareSQL("from %slinkepisode, actors, episodeview, files ", type.c_str());
        extFilter.fields = "actors.idActor, actors.strActor, actors.strThumb, count(1), count(files.playCount)";
        extFilter.AppendWhere(PrepareSQL("actors.idActor = %slinkepisode.id%s and %slinkepisode.idEpisode = episodeview.idEpisode and files.idFile = episodeview.idFile",
                                         type.c_str(), type.c_str(), type.c_str()));
        extFilter.AppendGroup("actors.idActor");
      }
      else if (idContent == VIDEODB_CONTENT_MUSICVIDEOS)
      {
        strSQL = "select %s from actors ";
        extFilter.fields = "actors.idActor, actors.strActor, actors.strThumb, count(1), count(files.playCount)";
        extFilter.AppendJoin(PrepareSQL("join %slinkmusicvideo on actors.idActor = %slinkmusicvideo.id%s join musicvideoview on %slinkmusicvideo.idMVideo = musicvideoview.idMVideo join files on files.idFile = musicvideoview.idFile",
                                        type.c_str(), type.c_str(), type.c_str(), type.c_str()));
        extFilter.AppendGroup("actors.idActor");
      }
      else
        return false;
    }

    if (countOnly)
    {
      extFilter.fields = "COUNT(1)";
      extFilter.group.clear();
      extFilter.order.clear();
    }
    strSQL = StringUtils::Format(strSQL.c_str(), !extFilter.fields.empty() ? extFilter.fields.c_str() : "*");

    CVideoDbUrl videoUrl;
    if (!BuildSQL(strBaseDir, strSQL, extFilter, strSQL, videoUrl))
      return false;

    // run query
    unsigned int time = XbmcThreads::SystemClockMillis();
    if (!m_pDS->query(strSQL.c_str())) return false;
    CLog::Log(LOGDEBUG, "%s -  query took %i ms",
              __FUNCTION__, XbmcThreads::SystemClockMillis() - time); time = XbmcThreads::SystemClockMillis();
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

    if (CProfilesManager::Get().GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
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
          if (g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv("path.strPath").get_asString()),*CMediaSourceSettings::Get().GetSources("video")))
            mapActors.insert(pair<int, CActor>(idActor, actor));
        }
        m_pDS->next();
      }
      m_pDS->close();

      for (it=mapActors.begin();it != mapActors.end();++it)
      {
        CFileItemPtr pItem(new CFileItem(it->second.name));

        CVideoDbUrl itemUrl = videoUrl;
        CStdString path = StringUtils::Format("%ld/", it->first);
        itemUrl.AppendPath(path);
        pItem->SetPath(itemUrl.ToString());

        pItem->m_bIsFolder=true;
        pItem->GetVideoInfoTag()->m_playCount = it->second.playcount;
        pItem->GetVideoInfoTag()->m_strPictureURL.ParseString(it->second.thumb);
        pItem->GetVideoInfoTag()->m_iDbId = it->first;
        pItem->GetVideoInfoTag()->m_type = type;
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

          CVideoDbUrl itemUrl = videoUrl;
          CStdString path = StringUtils::Format("%ld/", m_pDS->fv(0).get_asInt());
          itemUrl.AppendPath(path);
          pItem->SetPath(itemUrl.ToString());

          pItem->m_bIsFolder=true;
          pItem->GetVideoInfoTag()->m_strPictureURL.ParseString(m_pDS->fv(2).get_asString());
          pItem->GetVideoInfoTag()->m_iDbId = m_pDS->fv(0).get_asInt();
          pItem->GetVideoInfoTag()->m_type = type;
          if (idContent != VIDEODB_CONTENT_TVSHOWS)
          {
            // fv(4) is the number of videos watched, fv(3) is the total number.  We set the playcount
            // only if the number of videos watched is equal to the total number (i.e. every video watched)
            pItem->GetVideoInfoTag()->m_playCount = (m_pDS->fv(4).get_asInt() == m_pDS->fv(3).get_asInt()) ? 1 : 0;
          }
          if (idContent == VIDEODB_CONTENT_MUSICVIDEOS)
            pItem->GetVideoInfoTag()->m_artist.push_back(pItem->GetLabel());
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
              __FUNCTION__, XbmcThreads::SystemClockMillis() - time); time = XbmcThreads::SystemClockMillis();

    return true;
  }
  catch (...)
  {
    m_pDS->close();
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}

bool CVideoDatabase::GetYearsNav(const CStdString& strBaseDir, CFileItemList& items, int idContent /* = -1 */, const Filter &filter /* = Filter() */)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL;
    Filter extFilter = filter;
    if (CProfilesManager::Get().GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
    {
      if (idContent == VIDEODB_CONTENT_MOVIES)
      {
        strSQL = PrepareSQL("select movieview.c%02d, path.strPath, files.playCount from movieview ", VIDEODB_ID_YEAR);
        extFilter.AppendJoin("join files on files.idFile = movieview.idFile join path on files.idPath = path.idPath");
      }
      else if (idContent == VIDEODB_CONTENT_TVSHOWS)
      {
        strSQL = PrepareSQL("select tvshowview.c%02d, path.strPath from tvshowview ", VIDEODB_ID_TV_PREMIERED);
        extFilter.AppendJoin("join episodeview on episodeview.idShow = tvshowview.idShow join files on files.idFile = episodeview.idFile join path on files.idPath = path.idPath");
      }
      else if (idContent == VIDEODB_CONTENT_MUSICVIDEOS)
      {
        strSQL = PrepareSQL("select musicvideoview.c%02d, path.strPath, files.playCount from musicvideoview ", VIDEODB_ID_MUSICVIDEO_YEAR);
        extFilter.AppendJoin("join files on files.idFile = musicvideoview.idFile join path on files.idPath = path.idPath");
      }
      else
        return false;
    }
    else
    {
      CStdString group;
      if (idContent == VIDEODB_CONTENT_MOVIES)
      {
        strSQL = PrepareSQL("select movieview.c%02d, count(1), count(files.playCount) from movieview ", VIDEODB_ID_YEAR);
        extFilter.AppendJoin("join files on files.idFile = movieview.idFile");
        extFilter.AppendGroup(PrepareSQL("movieview.c%02d", VIDEODB_ID_YEAR));
      }
      else if (idContent == VIDEODB_CONTENT_TVSHOWS)
      {
        strSQL = PrepareSQL("select distinct tvshowview.c%02d from tvshowview", VIDEODB_ID_TV_PREMIERED);
        extFilter.AppendGroup(PrepareSQL("tvshowview.c%02d", VIDEODB_ID_TV_PREMIERED));
      }
      else if (idContent == VIDEODB_CONTENT_MUSICVIDEOS)
      {
        strSQL = PrepareSQL("select musicvideoview.c%02d, count(1), count(files.playCount) from musicvideoview ", VIDEODB_ID_MUSICVIDEO_YEAR);
        extFilter.AppendJoin("join files on files.idFile = musicvideoview.idFile");
        extFilter.AppendGroup(PrepareSQL("musicvideoview.c%02d", VIDEODB_ID_MUSICVIDEO_YEAR));
      }
      else
        return false;
    }

    CVideoDbUrl videoUrl;
    if (!BuildSQL(strBaseDir, strSQL, extFilter, strSQL, videoUrl))
      return false;

    int iRowsFound = RunQuery(strSQL);
    if (iRowsFound <= 0)
      return iRowsFound == 0;

    if (CProfilesManager::Get().GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
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
          if (g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv("path.strPath").get_asString()),*CMediaSourceSettings::Get().GetSources("video")))
          {
            CStdString year = StringUtils::Format("%d", lYear);
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

        CVideoDbUrl itemUrl = videoUrl;
        CStdString path = StringUtils::Format("%ld/", it->first);
        itemUrl.AppendPath(path);
        pItem->SetPath(itemUrl.ToString());

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
          strLabel = StringUtils::Format("%i",lYear);
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

        CVideoDbUrl itemUrl = videoUrl;
        CStdString path = StringUtils::Format("%ld/", lYear);
        itemUrl.AppendPath(path);
        pItem->SetPath(itemUrl.ToString());

        pItem->m_bIsFolder=true;
        if (idContent == VIDEODB_CONTENT_MOVIES || idContent == VIDEODB_CONTENT_MUSICVIDEOS)
        {
          // fv(2) is the number of videos watched, fv(1) is the total number.  We set the playcount
          // only if the number of videos watched is equal to the total number (i.e. every video watched)
          pItem->GetVideoInfoTag()->m_playCount = (m_pDS->fv(2).get_asInt() == m_pDS->fv(1).get_asInt()) ? 1 : 0;
        }

        // take care of dupes ..
        if (!items.Contains(pItem->GetPath()))
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

bool CVideoDatabase::GetSeasonsNav(const CStdString& strBaseDir, CFileItemList& items, int idActor, int idDirector, int idGenre, int idYear, int idShow, bool getLinkedMovies /* = true */)
{
  // parse the base path to get additional filters
  CVideoDbUrl videoUrl;
  if (!videoUrl.FromString(strBaseDir))
    return false;

  if (idShow != -1)
    videoUrl.AddOption("tvshowid", idShow);
  if (idActor != -1)
    videoUrl.AddOption("actorid", idActor);
  else if (idDirector != -1)
    videoUrl.AddOption("directorid", idDirector);
  else if (idGenre != -1)
    videoUrl.AddOption("genreid", idGenre);
  else if (idYear != -1)
    videoUrl.AddOption("year", idYear);

  if (!GetSeasonsByWhere(videoUrl.ToString(), Filter(), items, false))
    return false;

  // now add any linked movies
  if (getLinkedMovies && idShow != -1)
  {
    Filter movieFilter;
    movieFilter.join = PrepareSQL("join movielinktvshow on movielinktvshow.idMovie=movieview.idMovie");
    movieFilter.where = PrepareSQL("movielinktvshow.idShow = %i", idShow);
    CFileItemList movieItems;
    GetMoviesByWhere("videodb://movies/titles/", movieFilter, movieItems);

    if (movieItems.Size() > 0)
      items.Append(movieItems);
  }

  return true;
}

bool CVideoDatabase::GetSeasonsByWhere(const CStdString& strBaseDir, const Filter &filter, CFileItemList& items, bool appendFullShowPath /* = true */, const SortDescription &sortDescription /* = SortDescription() */)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    // parse the base path to get additional filters
    CVideoDbUrl videoUrl;
    if (!videoUrl.FromString(strBaseDir))
      return false;

    CStdString strSQL = "SELECT * FROM seasonview ";

    Filter extFilter = filter;
    if (!BuildSQL(strBaseDir, strSQL, extFilter, strSQL, videoUrl))
      return false;

    int iRowsFound = RunQuery(strSQL);
    if (iRowsFound <= 0)
      return iRowsFound == 0;

    set< std::pair<int, int> > mapSeasons;
    while (!m_pDS->eof())
    {
      int id = m_pDS->fv(VIDEODB_ID_SEASON_ID).get_asInt();
      int showId = m_pDS->fv(VIDEODB_ID_SEASON_TVSHOW_ID).get_asInt();
      int iSeason = m_pDS->fv(VIDEODB_ID_SEASON_NUMBER).get_asInt();
      std::string path = m_pDS->fv(VIDEODB_ID_SEASON_TVSHOW_PATH).get_asString();

      if (mapSeasons.find(std::make_pair(showId, iSeason)) == mapSeasons.end() &&
         (CProfilesManager::Get().GetMasterProfile().getLockMode() == LOCK_MODE_EVERYONE || g_passwordManager.bMasterUser ||
          g_passwordManager.IsDatabasePathUnlocked(path, *CMediaSourceSettings::Get().GetSources("video"))))
      {
        mapSeasons.insert(std::make_pair(showId, iSeason));

        CStdString strLabel;
        if (iSeason == 0)
          strLabel = g_localizeStrings.Get(20381);
        else
          strLabel = StringUtils::Format(g_localizeStrings.Get(20358).c_str(), iSeason);
        CFileItemPtr pItem(new CFileItem(strLabel));

        CVideoDbUrl itemUrl = videoUrl;
        CStdString strDir;
        if (appendFullShowPath)
          strDir += StringUtils::Format("%d/", showId);
        strDir += StringUtils::Format("%d/", iSeason);
        itemUrl.AppendPath(strDir);
        pItem->SetPath(itemUrl.ToString());

        pItem->m_bIsFolder = true;
        pItem->GetVideoInfoTag()->m_strTitle = strLabel;
        pItem->GetVideoInfoTag()->m_iSeason = iSeason;
        pItem->GetVideoInfoTag()->m_iDbId = id;
        pItem->GetVideoInfoTag()->m_type = MediaTypeSeason;
        pItem->GetVideoInfoTag()->m_strPath = path;
        pItem->GetVideoInfoTag()->m_strShowTitle = m_pDS->fv(VIDEODB_ID_SEASON_TVSHOW_TITLE).get_asString();
        pItem->GetVideoInfoTag()->m_strPlot = m_pDS->fv(VIDEODB_ID_SEASON_TVSHOW_PLOT).get_asString();
        pItem->GetVideoInfoTag()->m_premiered.SetFromDBDate(m_pDS->fv(VIDEODB_ID_SEASON_TVSHOW_PREMIERED).get_asString());
        if (pItem->GetVideoInfoTag()->m_premiered.IsValid())
          pItem->GetVideoInfoTag()->m_iYear = pItem->GetVideoInfoTag()->m_premiered.GetYear();
        pItem->GetVideoInfoTag()->m_genre = StringUtils::Split(m_pDS->fv(VIDEODB_ID_SEASON_TVSHOW_GENRE).get_asString(), g_advancedSettings.m_videoItemSeparator);
        pItem->GetVideoInfoTag()->m_studio = StringUtils::Split(m_pDS->fv(VIDEODB_ID_SEASON_TVSHOW_STUDIO).get_asString(), g_advancedSettings.m_videoItemSeparator);
        pItem->GetVideoInfoTag()->m_strMPAARating = m_pDS->fv(VIDEODB_ID_SEASON_TVSHOW_MPAA).get_asString();
        pItem->GetVideoInfoTag()->m_iIdShow = showId;

        int totalEpisodes = m_pDS->fv(VIDEODB_ID_SEASON_EPISODES_TOTAL).get_asInt();
        int watchedEpisodes = m_pDS->fv(VIDEODB_ID_SEASON_EPISODES_WATCHED).get_asInt();
        pItem->GetVideoInfoTag()->m_iEpisode = totalEpisodes;
        pItem->SetProperty("totalepisodes", totalEpisodes);
        pItem->SetProperty("numepisodes", totalEpisodes); // will be changed later to reflect watchmode setting
        pItem->SetProperty("watchedepisodes", watchedEpisodes);
        pItem->SetProperty("unwatchedepisodes", totalEpisodes - watchedEpisodes);
        if (iSeason == 0)
          pItem->SetProperty("isspecial", true);
        pItem->GetVideoInfoTag()->m_playCount = (totalEpisodes == watchedEpisodes) ? 1 : 0;
        pItem->SetOverlayImage(CGUIListItem::ICON_OVERLAY_UNWATCHED, (pItem->GetVideoInfoTag()->m_playCount > 0) && (pItem->GetVideoInfoTag()->m_iEpisode > 0));

        items.Add(pItem);
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

bool CVideoDatabase::GetSortedVideos(const MediaType &mediaType, const CStdString& strBaseDir, const SortDescription &sortDescription, CFileItemList& items, const Filter &filter /* = Filter() */)
{
  if (NULL == m_pDB.get() || NULL == m_pDS.get())
    return false;

  if (mediaType != MediaTypeMovie && mediaType != MediaTypeTvShow && mediaType != MediaTypeEpisode && mediaType != MediaTypeMusicVideo)
    return false;

  SortDescription sorting = sortDescription;
  if (sortDescription.sortBy == SortByFile ||
      sortDescription.sortBy == SortByTitle ||
      sortDescription.sortBy == SortBySortTitle ||
      sortDescription.sortBy == SortByLabel ||
      sortDescription.sortBy == SortByDateAdded ||
      sortDescription.sortBy == SortByRating ||
      sortDescription.sortBy == SortByYear ||
      sortDescription.sortBy == SortByLastPlayed ||
      sortDescription.sortBy == SortByPlaycount)
    sorting.sortAttributes = (SortAttribute)(sortDescription.sortAttributes | SortAttributeIgnoreFolders);

  bool success = false;
  if (mediaType == MediaTypeMovie)
    success = GetMoviesByWhere(strBaseDir, filter, items, sorting);
  else if (mediaType == MediaTypeTvShow)
    success = GetTvShowsByWhere(strBaseDir, filter, items, sorting);
  else if (mediaType == MediaTypeEpisode)
    success = GetEpisodesByWhere(strBaseDir, filter, items, true, sorting);
  else if (mediaType == MediaTypeMusicVideo)
    success = GetMusicVideosByWhere(strBaseDir, filter, items, true, sorting);
  else
    return false;

  items.SetContent(MediaTypes::ToPlural(mediaType));
  return success;
}

bool CVideoDatabase::GetItems(const CStdString &strBaseDir, CFileItemList &items, const Filter &filter /* = Filter() */, const SortDescription &sortDescription /* = SortDescription() */)
{
  CVideoDbUrl videoUrl;
  if (!videoUrl.FromString(strBaseDir))
    return false;

  return GetItems(strBaseDir, videoUrl.GetType(), videoUrl.GetItemType(), items, filter, sortDescription);
}

bool CVideoDatabase::GetItems(const CStdString &strBaseDir, const CStdString &mediaType, const CStdString &itemType, CFileItemList &items, const Filter &filter /* = Filter() */, const SortDescription &sortDescription /* = SortDescription() */)
{
  VIDEODB_CONTENT_TYPE contentType;
  if (mediaType.Equals("movies"))
    contentType = VIDEODB_CONTENT_MOVIES;
  else if (mediaType.Equals("tvshows"))
  {
    if (itemType.Equals("episodes"))
      contentType = VIDEODB_CONTENT_EPISODES;
    else
      contentType = VIDEODB_CONTENT_TVSHOWS;
  }
  else if (mediaType.Equals("musicvideos"))
    contentType = VIDEODB_CONTENT_MUSICVIDEOS;
  else
    return false;

  return GetItems(strBaseDir, contentType, itemType, items, filter, sortDescription);
}

bool CVideoDatabase::GetItems(const CStdString &strBaseDir, VIDEODB_CONTENT_TYPE mediaType, const CStdString &itemType, CFileItemList &items, const Filter &filter /* = Filter() */, const SortDescription &sortDescription /* = SortDescription() */)
{
  if (itemType.Equals("movies") && (mediaType == VIDEODB_CONTENT_MOVIES || mediaType == VIDEODB_CONTENT_MOVIE_SETS))
    return GetMoviesByWhere(strBaseDir, filter, items, sortDescription);
  else if (itemType.Equals("tvshows") && mediaType == VIDEODB_CONTENT_TVSHOWS)
    return GetTvShowsByWhere(strBaseDir, filter, items, sortDescription);
  else if (itemType.Equals("musicvideos") && mediaType == VIDEODB_CONTENT_MUSICVIDEOS)
    return GetMusicVideosByWhere(strBaseDir, filter, items, true, sortDescription);
  else if (itemType.Equals("episodes") && mediaType == VIDEODB_CONTENT_EPISODES)
    return GetEpisodesByWhere(strBaseDir, filter, items, true, sortDescription);
  else if (itemType.Equals("seasons") && mediaType == VIDEODB_CONTENT_TVSHOWS)
    return GetSeasonsNav(strBaseDir, items);
  else if (itemType.Equals("genres"))
    return GetGenresNav(strBaseDir, items, mediaType, filter);
  else if (itemType.Equals("years"))
    return GetYearsNav(strBaseDir, items, mediaType, filter);
  else if (itemType.Equals("actors"))
    return GetActorsNav(strBaseDir, items, mediaType, filter);
  else if (itemType.Equals("directors"))
    return GetDirectorsNav(strBaseDir, items, mediaType, filter);
  else if (itemType.Equals("writers"))
    return GetWritersNav(strBaseDir, items, mediaType, filter);
  else if (itemType.Equals("studios"))
    return GetStudiosNav(strBaseDir, items, mediaType, filter);
  else if (itemType.Equals("sets"))
    return GetSetsNav(strBaseDir, items, mediaType, filter);
  else if (itemType.Equals("countries"))
    return GetCountriesNav(strBaseDir, items, mediaType, filter);
  else if (itemType.Equals("tags"))
    return GetTagsNav(strBaseDir, items, mediaType, filter);
  else if (itemType.Equals("artists") && mediaType == VIDEODB_CONTENT_MUSICVIDEOS)
    return GetActorsNav(strBaseDir, items, mediaType, filter);
  else if (itemType.Equals("albums") && mediaType == VIDEODB_CONTENT_MUSICVIDEOS)
    return GetMusicVideoAlbumsNav(strBaseDir, items, -1, filter);

  return false;
}

CStdString CVideoDatabase::GetItemById(const CStdString &itemType, int id)
{
  if (itemType.Equals("genres"))
    return GetGenreById(id);
  else if (itemType.Equals("years"))
    return StringUtils::Format("%d", id);
  else if (itemType.Equals("actors") || itemType.Equals("directors") || itemType.Equals("artists"))
    return GetPersonById(id);
  else if (itemType.Equals("studios"))
    return GetStudioById(id);
  else if (itemType.Equals("sets"))
    return GetSetById(id);
  else if (itemType.Equals("countries"))
    return GetCountryById(id);
  else if (itemType.Equals("tags"))
    return GetTagById(id);
  else if (itemType.Equals("albums"))
    return GetMusicVideoAlbumById(id);

  return "";
}

bool CVideoDatabase::GetMoviesNav(const CStdString& strBaseDir, CFileItemList& items,
                                  int idGenre /* = -1 */, int idYear /* = -1 */, int idActor /* = -1 */, int idDirector /* = -1 */,
                                  int idStudio /* = -1 */, int idCountry /* = -1 */, int idSet /* = -1 */, int idTag /* = -1 */,
                                  const SortDescription &sortDescription /* = SortDescription() */)
{
  CVideoDbUrl videoUrl;
  if (!videoUrl.FromString(strBaseDir))
    return false;

  if (idGenre > 0)
    videoUrl.AddOption("genreid", idGenre);
  else if (idCountry > 0)
    videoUrl.AddOption("countryid", idCountry);
  else if (idStudio > 0)
    videoUrl.AddOption("studioid", idStudio);
  else if (idDirector > 0)
    videoUrl.AddOption("directorid", idDirector);
  else if (idYear > 0)
    videoUrl.AddOption("year", idYear);
  else if (idActor > 0)
    videoUrl.AddOption("actorid", idActor);
  else if (idSet > 0)
    videoUrl.AddOption("setid", idSet);
  else if (idTag > 0)
    videoUrl.AddOption("tagid", idTag);

  Filter filter;
  return GetMoviesByWhere(videoUrl.ToString(), filter, items, sortDescription);
}

bool CVideoDatabase::GetMoviesByWhere(const CStdString& strBaseDir, const Filter &filter, CFileItemList& items, const SortDescription &sortDescription /* = SortDescription() */)
{
  try
  {
    movieTime = 0;
    castTime = 0;

    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    // parse the base path to get additional filters
    CVideoDbUrl videoUrl;
    Filter extFilter = filter;
    SortDescription sorting = sortDescription;
    if (!videoUrl.FromString(strBaseDir) || !GetFilter(videoUrl, extFilter, sorting))
      return false;

    int total = -1;

    std::string strSQL = "select %s from movieview ";
    std::string strSQLExtra;
    if (!CDatabase::BuildSQL(strSQLExtra, extFilter, strSQLExtra))
      return false;

    // Apply the limiting directly here if there's no special sorting but limiting
    if (extFilter.limit.empty() &&
        sorting.sortBy == SortByNone &&
       (sorting.limitStart > 0 || sorting.limitEnd > 0))
    {
      total = (int)strtol(GetSingleValue(PrepareSQL(strSQL, "COUNT(1)") + strSQLExtra, m_pDS).c_str(), NULL, 10);
      strSQLExtra += DatabaseUtils::BuildLimitClause(sorting.limitEnd, sorting.limitStart);
    }

    strSQL = PrepareSQL(strSQL, !extFilter.fields.empty() ? extFilter.fields.c_str() : "*") + strSQLExtra;

    int iRowsFound = RunQuery(strSQL);
    if (iRowsFound <= 0)
      return iRowsFound == 0;

    // store the total value of items as a property
    if (total < iRowsFound)
      total = iRowsFound;
    items.SetProperty("total", total);
    
    DatabaseResults results;
    results.reserve(iRowsFound);

    if (!SortUtils::SortFromDataset(sortDescription, MediaTypeMovie, m_pDS, results))
      return false;

    // get data from returned rows
    items.Reserve(results.size());
    const query_data &data = m_pDS->get_result_set().records;
    for (DatabaseResults::const_iterator it = results.begin(); it != results.end(); it++)
    {
      unsigned int targetRow = (unsigned int)it->at(FieldRow).asInteger();
      const dbiplus::sql_record* const record = data.at(targetRow);

      CVideoInfoTag movie = GetDetailsForMovie(record);
      if (CProfilesManager::Get().GetMasterProfile().getLockMode() == LOCK_MODE_EVERYONE ||
          g_passwordManager.bMasterUser                                   ||
          g_passwordManager.IsDatabasePathUnlocked(movie.m_strPath, *CMediaSourceSettings::Get().GetSources("video")))
      {
        CFileItemPtr pItem(new CFileItem(movie));

        CVideoDbUrl itemUrl = videoUrl;
        CStdString path = StringUtils::Format("%ld", movie.m_iDbId);
        itemUrl.AppendPath(path);
        pItem->SetPath(itemUrl.ToString());

        pItem->SetOverlayImage(CGUIListItem::ICON_OVERLAY_UNWATCHED,movie.m_playCount > 0);
        items.Add(pItem);
      }
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

bool CVideoDatabase::GetTvShowsNav(const CStdString& strBaseDir, CFileItemList& items,
                                  int idGenre /* = -1 */, int idYear /* = -1 */, int idActor /* = -1 */, int idDirector /* = -1 */, int idStudio /* = -1 */, int idTag /* = -1 */,
                                  const SortDescription &sortDescription /* = SortDescription() */)
{
  CVideoDbUrl videoUrl;
  if (!videoUrl.FromString(strBaseDir))
    return false;

  if (idGenre != -1)
    videoUrl.AddOption("genreid", idGenre);
  else if (idStudio != -1)
    videoUrl.AddOption("studioid", idStudio);
  else if (idDirector != -1)
    videoUrl.AddOption("directorid", idDirector);
  else if (idYear != -1)
    videoUrl.AddOption("year", idYear);
  else if (idActor != -1)
    videoUrl.AddOption("actorid", idActor);
  else if (idTag != -1)
    videoUrl.AddOption("tagid", idTag);

  Filter filter;
  return GetTvShowsByWhere(videoUrl.ToString(), filter, items, sortDescription);
}

bool CVideoDatabase::GetTvShowsByWhere(const CStdString& strBaseDir, const Filter &filter, CFileItemList& items, const SortDescription &sortDescription /* = SortDescription() */)
{
  try
  {
    movieTime = 0;

    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    int total = -1;
    
    std::string strSQL = "SELECT %s FROM tvshowview ";
    CVideoDbUrl videoUrl;
    std::string strSQLExtra;
    Filter extFilter = filter;
    extFilter.AppendGroup("tvshowview.idShow");
    SortDescription sorting = sortDescription;
    if (!BuildSQL(strBaseDir, strSQLExtra, extFilter, strSQLExtra, videoUrl, sorting))
      return false;

    // Apply the limiting directly here if there's no special sorting but limiting
    if (extFilter.limit.empty() &&
        sorting.sortBy == SortByNone &&
        (sorting.limitStart > 0 || sorting.limitEnd > 0))
    {
      total = (int)strtol(GetSingleValue(PrepareSQL(strSQL, "COUNT(1)") + strSQLExtra, m_pDS).c_str(), NULL, 10);
      strSQLExtra += DatabaseUtils::BuildLimitClause(sorting.limitEnd, sorting.limitStart);
    }

    strSQL = PrepareSQL(strSQL, !extFilter.fields.empty() ? extFilter.fields.c_str() : "*") + strSQLExtra;

    int iRowsFound = RunQuery(strSQL);
    if (iRowsFound <= 0)
      return iRowsFound == 0;

    // store the total value of items as a property
    if (total < iRowsFound)
      total = iRowsFound;
    items.SetProperty("total", total);
    
    DatabaseResults results;
    results.reserve(iRowsFound);
    if (!SortUtils::SortFromDataset(sorting, MediaTypeTvShow, m_pDS, results))
      return false;

    // get data from returned rows
    items.Reserve(results.size());
    const query_data &data = m_pDS->get_result_set().records;
    for (DatabaseResults::const_iterator it = results.begin(); it != results.end(); it++)
    {
      unsigned int targetRow = (unsigned int)it->at(FieldRow).asInteger();
      const dbiplus::sql_record* const record = data.at(targetRow);
      
      CFileItemPtr pItem(new CFileItem());
      CVideoInfoTag movie = GetDetailsForTvShow(record, false, pItem.get());
      if ((CProfilesManager::Get().GetMasterProfile().getLockMode() == LOCK_MODE_EVERYONE ||
           g_passwordManager.bMasterUser                                     ||
           g_passwordManager.IsDatabasePathUnlocked(movie.m_strPath, *CMediaSourceSettings::Get().GetSources("video"))) &&
          (!g_advancedSettings.m_bVideoLibraryHideEmptySeries || movie.m_iEpisode > 0))
      {
        pItem->SetFromVideoInfoTag(movie);

        CVideoDbUrl itemUrl = videoUrl;
        CStdString path = StringUtils::Format("%ld/", record->at(0).get_asInt());
        itemUrl.AppendPath(path);
        pItem->SetPath(itemUrl.ToString());

        pItem->SetOverlayImage(CGUIListItem::ICON_OVERLAY_UNWATCHED, (pItem->GetVideoInfoTag()->m_playCount > 0) && (pItem->GetVideoInfoTag()->m_iEpisode > 0));
        items.Add(pItem);
      }
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

bool CVideoDatabase::GetEpisodesNav(const CStdString& strBaseDir, CFileItemList& items, int idGenre, int idYear, int idActor, int idDirector, int idShow, int idSeason, const SortDescription &sortDescription /* = SortDescription() */)
{
  CVideoDbUrl videoUrl;
  if (!videoUrl.FromString(strBaseDir))
    return false;

  CStdString strIn;
  if (idShow != -1)
  {
    videoUrl.AddOption("tvshowid", idShow);
    if (idSeason >= 0)
      videoUrl.AddOption("season", idSeason);

    if (idGenre != -1)
      videoUrl.AddOption("genreid", idGenre);
    else if (idYear !=-1)
      videoUrl.AddOption("year", idYear);
    else if (idActor != -1)
      videoUrl.AddOption("actorid", idActor);
  }
  else if (idYear != -1)
    videoUrl.AddOption("year", idYear);
  
  if (idDirector != -1)
    videoUrl.AddOption("directorid", idDirector);

  Filter filter;
  bool ret = GetEpisodesByWhere(videoUrl.ToString(), filter, items, false, sortDescription);

  if (idSeason == -1 && idShow != -1)
  { // add any linked movies
    Filter movieFilter;
    movieFilter.join  = PrepareSQL("join movielinktvshow on movielinktvshow.idMovie=movieview.idMovie");
    movieFilter.where = PrepareSQL("movielinktvshow.idShow = %i", idShow);
    CFileItemList movieItems;
    GetMoviesByWhere("videodb://movies/titles/", movieFilter, movieItems);

    if (movieItems.Size() > 0)
      items.Append(movieItems);
  }

  return ret;
}

bool CVideoDatabase::GetEpisodesByWhere(const CStdString& strBaseDir, const Filter &filter, CFileItemList& items, bool appendFullShowPath /* = true */, const SortDescription &sortDescription /* = SortDescription() */)
{
  try
  {
    movieTime = 0;
    castTime = 0;

    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    int total = -1;
    
    std::string strSQL = "select %s from episodeview ";
    CVideoDbUrl videoUrl;
    std::string strSQLExtra;
    Filter extFilter = filter;
    SortDescription sorting = sortDescription;
    if (!BuildSQL(strBaseDir, strSQLExtra, extFilter, strSQLExtra, videoUrl, sorting))
      return false;

    // Apply the limiting directly here if there's no special sorting but limiting
    if (extFilter.limit.empty() &&
      sorting.sortBy == SortByNone &&
      (sorting.limitStart > 0 || sorting.limitEnd > 0))
    {
      total = (int)strtol(GetSingleValue(PrepareSQL(strSQL, "COUNT(1)") + strSQLExtra, m_pDS).c_str(), NULL, 10);
      strSQLExtra += DatabaseUtils::BuildLimitClause(sorting.limitEnd, sorting.limitStart);
    }

    strSQL = PrepareSQL(strSQL, !extFilter.fields.empty() ? extFilter.fields.c_str() : "*") + strSQLExtra;

    int iRowsFound = RunQuery(strSQL);
    if (iRowsFound <= 0)
      return iRowsFound == 0;

    // store the total value of items as a property
    if (total < iRowsFound)
      total = iRowsFound;
    items.SetProperty("total", total);
    
    DatabaseResults results;
    results.reserve(iRowsFound);
    if (!SortUtils::SortFromDataset(sorting, MediaTypeEpisode, m_pDS, results))
      return false;
    
    // get data from returned rows
    items.Reserve(results.size());
    CLabelFormatter formatter("%H. %T", "");

    const query_data &data = m_pDS->get_result_set().records;
    for (DatabaseResults::const_iterator it = results.begin(); it != results.end(); it++)
    {
      unsigned int targetRow = (unsigned int)it->at(FieldRow).asInteger();
      const dbiplus::sql_record* const record = data.at(targetRow);

      CVideoInfoTag movie = GetDetailsForEpisode(record);
      if (CProfilesManager::Get().GetMasterProfile().getLockMode() == LOCK_MODE_EVERYONE ||
          g_passwordManager.bMasterUser                                     ||
          g_passwordManager.IsDatabasePathUnlocked(movie.m_strPath, *CMediaSourceSettings::Get().GetSources("video")))
      {
        CFileItemPtr pItem(new CFileItem(movie));
        formatter.FormatLabel(pItem.get());
      
        int idEpisode = record->at(0).get_asInt();

        CVideoDbUrl itemUrl = videoUrl;
        CStdString path;
        if (appendFullShowPath && videoUrl.GetItemType() != "episodes")
          path = StringUtils::Format("%ld/%ld/%ld", record->at(VIDEODB_DETAILS_EPISODE_TVSHOW_ID).get_asInt(), movie.m_iSeason, idEpisode);
        else
          path = StringUtils::Format("%ld", idEpisode);
        itemUrl.AppendPath(path);
        pItem->SetPath(itemUrl.ToString());

        pItem->SetOverlayImage(CGUIListItem::ICON_OVERLAY_UNWATCHED, movie.m_playCount > 0);
        pItem->m_dateTime = movie.m_firstAired;
        pItem->GetVideoInfoTag()->m_iYear = pItem->m_dateTime.GetYear();
        items.Add(pItem);
      }
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

bool CVideoDatabase::GetMusicVideosNav(const CStdString& strBaseDir, CFileItemList& items, int idGenre, int idYear, int idArtist, int idDirector, int idStudio, int idAlbum, int idTag /* = -1 */, const SortDescription &sortDescription /* = SortDescription() */)
{
  CVideoDbUrl videoUrl;
  if (!videoUrl.FromString(strBaseDir))
    return false;

  if (idGenre != -1)
    videoUrl.AddOption("genreid", idGenre);
  else if (idStudio != -1)
    videoUrl.AddOption("studioid", idStudio);
  else if (idDirector != -1)
    videoUrl.AddOption("directorid", idDirector);
  else if (idYear !=-1)
    videoUrl.AddOption("year", idYear);
  else if (idArtist != -1)
    videoUrl.AddOption("artistid", idArtist);
  else if (idTag != -1)
    videoUrl.AddOption("tagid", idTag);
  if (idAlbum != -1)
    videoUrl.AddOption("albumid", idAlbum);

  Filter filter;
  return GetMusicVideosByWhere(videoUrl.ToString(), filter, items, true, sortDescription);
}

bool CVideoDatabase::GetRecentlyAddedMoviesNav(const CStdString& strBaseDir, CFileItemList& items, unsigned int limit)
{
  Filter filter;
  filter.order = "dateAdded desc, idMovie desc";
  filter.limit = PrepareSQL("%u", limit ? limit : g_advancedSettings.m_iVideoLibraryRecentlyAddedItems);
  return GetMoviesByWhere(strBaseDir, filter, items);
}

bool CVideoDatabase::GetRecentlyAddedEpisodesNav(const CStdString& strBaseDir, CFileItemList& items, unsigned int limit)
{
  Filter filter;
  filter.order = "dateAdded desc, idEpisode desc";
  filter.limit = PrepareSQL("%u", limit ? limit : g_advancedSettings.m_iVideoLibraryRecentlyAddedItems);
  return GetEpisodesByWhere(strBaseDir, filter, items, false);
}

bool CVideoDatabase::GetRecentlyAddedMusicVideosNav(const CStdString& strBaseDir, CFileItemList& items, unsigned int limit)
{
  Filter filter;
  filter.order = "dateAdded desc, idMVideo desc";
  filter.limit = PrepareSQL("%u", limit ? limit : g_advancedSettings.m_iVideoLibraryRecentlyAddedItems);
  return GetMusicVideosByWhere(strBaseDir, filter, items);
}

CStdString CVideoDatabase::GetGenreById(int id)
{
  return GetSingleValue("genre", "strGenre", PrepareSQL("idGenre=%i", id));
}

CStdString CVideoDatabase::GetCountryById(int id)
{
  return GetSingleValue("country", "strCountry", PrepareSQL("idCountry=%i", id));
}

CStdString CVideoDatabase::GetSetById(int id)
{
  return GetSingleValue("sets", "strSet", PrepareSQL("idSet=%i", id));
}

CStdString CVideoDatabase::GetTagById(int id)
{
  return GetSingleValue("tag", "strTag", PrepareSQL("idTag = %i", id));
}

CStdString CVideoDatabase::GetPersonById(int id)
{
  return GetSingleValue("actors", "strActor", PrepareSQL("idActor=%i", id));
}

CStdString CVideoDatabase::GetStudioById(int id)
{
  return GetSingleValue("studio", "strStudio", PrepareSQL("idStudio=%i", id));
}

CStdString CVideoDatabase::GetTvShowTitleById(int id)
{
  return GetSingleValue("tvshow", PrepareSQL("c%02d", VIDEODB_ID_TV_TITLE), PrepareSQL("idShow=%i", id));
}

CStdString CVideoDatabase::GetMusicVideoAlbumById(int id)
{
  return GetSingleValue("musicvideo", PrepareSQL("c%02d", VIDEODB_ID_MUSICVIDEO_ALBUM), PrepareSQL("idMVideo=%i", id));
}

bool CVideoDatabase::HasSets() const
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    m_pDS->query("SELECT movieview.idSet,COUNT(1) AS c FROM movieview "
                 "JOIN sets ON sets.idSet = movieview.idSet "
                 "GROUP BY movieview.idSet HAVING c>1");

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

int CVideoDatabase::GetTvShowForEpisode(int idEpisode)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS2.get()) return false;

    // make sure we use m_pDS2, as this is called in loops using m_pDS
    CStdString strSQL=PrepareSQL("select idShow from episode where idEpisode=%i", idEpisode);
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

int CVideoDatabase::GetSeasonForEpisode(int idEpisode)
{
  char column[5];
  sprintf(column, "c%0d", VIDEODB_ID_EPISODE_SEASON);
  CStdString id = GetSingleValue("episode", column, PrepareSQL("idEpisode=%i", idEpisode));
  if (id.empty())
    return -1;
  return atoi(id.c_str());
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

    CStdString strSQL = StringUtils::Format("select count(1) as nummovies from musicvideoview where %s",strWhere.c_str());
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

ScraperPtr CVideoDatabase::GetScraperForPath( const CStdString& strPath )
{
  SScanSettings settings;
  return GetScraperForPath(strPath, settings);
}

ScraperPtr CVideoDatabase::GetScraperForPath(const CStdString& strPath, SScanSettings& settings)
{
  bool dummy;
  return GetScraperForPath(strPath, settings, dummy);
}

ScraperPtr CVideoDatabase::GetScraperForPath(const CStdString& strPath, SScanSettings& settings, bool& foundDirectly)
{
  foundDirectly = false;
  try
  {
    if (strPath.empty() || !m_pDB.get() || !m_pDS.get()) return ScraperPtr();

    ScraperPtr scraper;
    CStdString strPath2;

    if (URIUtils::IsMultiPath(strPath))
      strPath2 = CMultiPathDirectory::GetFirstPath(strPath);
    else
      strPath2 = strPath;

    CStdString strPath1 = URIUtils::GetDirectory(strPath2);
    int idPath = GetPathId(strPath1);

    if (idPath > -1)
    {
      CStdString strSQL=PrepareSQL("select path.strContent,path.strScraper,path.scanRecursive,path.useFolderNames,path.strSettings,path.noUpdate,path.exclude from path where path.idPath=%i",idPath);
      m_pDS->query( strSQL.c_str() );
    }

    int iFound = 1;
    CONTENT_TYPE content = CONTENT_NONE;
    if (!m_pDS->eof())
    { // path is stored in db

      if (m_pDS->fv("path.exclude").get_asBool())
      {
        settings.exclude = true;
        m_pDS->close();
        return ScraperPtr();
      }
      settings.exclude = false;

      // try and ascertain scraper for this path
      CStdString strcontent = m_pDS->fv("path.strContent").get_asString();
      StringUtils::ToLower(strcontent);
      content = TranslateContent(strcontent);

      //FIXME paths stored should not have empty strContent
      //assert(content != CONTENT_NONE);
      CStdString scraperID = m_pDS->fv("path.strScraper").get_asString();

      AddonPtr addon;
      if (!scraperID.empty() &&
        CAddonMgr::Get().GetAddon(scraperID, addon))
      {
        scraper = boost::dynamic_pointer_cast<CScraper>(addon->Clone());
        if (!scraper)
          return ScraperPtr();

        // store this path's content & settings
        scraper->SetPathSettings(content, m_pDS->fv("path.strSettings").get_asString());
        settings.parent_name = m_pDS->fv("path.useFolderNames").get_asBool();
        settings.recurse = m_pDS->fv("path.scanRecursive").get_asInt();
        settings.noupdate = m_pDS->fv("path.noUpdate").get_asBool();
      }
    }

    if (content == CONTENT_NONE)
    { // this path is not saved in db
      // we must drill up until a scraper is configured
      CStdString strParent;
      while (URIUtils::GetParentPath(strPath1, strParent))
      {
        iFound++;

        CStdString strSQL=PrepareSQL("select path.strContent,path.strScraper,path.scanRecursive,path.useFolderNames,path.strSettings,path.noUpdate, path.exclude from path where strPath='%s'",strParent.c_str());
        m_pDS->query(strSQL.c_str());

        CONTENT_TYPE content = CONTENT_NONE;
        if (!m_pDS->eof())
        {

          CStdString strcontent = m_pDS->fv("path.strContent").get_asString();
          StringUtils::ToLower(strcontent);
          if (m_pDS->fv("path.exclude").get_asBool())
          {
            settings.exclude = true;
            scraper.reset();
            m_pDS->close();
            break;
          }

          content = TranslateContent(strcontent);

          AddonPtr addon;
          if (content != CONTENT_NONE &&
              CAddonMgr::Get().GetAddon(m_pDS->fv("path.strScraper").get_asString(), addon))
          {
            scraper = boost::dynamic_pointer_cast<CScraper>(addon->Clone());
            scraper->SetPathSettings(content, m_pDS->fv("path.strSettings").get_asString());
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
      return ScraperPtr();

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
      return ScraperPtr();
    }
    foundDirectly = (iFound == 1);
    return scraper;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return ScraperPtr();
}

CStdString CVideoDatabase::GetContentForPath(const CStdString& strPath)
{
  SScanSettings settings;
  bool foundDirectly = false;
  ScraperPtr scraper = GetScraperForPath(strPath, settings, foundDirectly);
  if (scraper)
  {
    if (scraper->Content() == CONTENT_TVSHOWS)
    { // check for episodes or seasons.  Assumptions are:
      // 1. if episodes are in the path then we're in episodes.
      // 2. if no episodes are found, and content was set directly on this path, then we're in shows.
      // 3. if no episodes are found, and content was not set directly on this path, we're in seasons (assumes tvshows/seasons/episodes)
      CStdString sql = PrepareSQL("select count(1) from episodeview where strPath = '%s' limit 1", strPath.c_str());
      m_pDS->query( sql.c_str() );
      if (m_pDS->num_rows() && m_pDS->fv(0).get_asInt() > 0)
        return "episodes";
      return foundDirectly ? "tvshows" : "seasons";
    }
    return TranslateContent(scraper->Content());
  }
  return "";
}

void CVideoDatabase::GetMovieGenresByName(const CStdString& strSearch, CFileItemList& items)
{
  CStdString strSQL;

  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    if (CProfilesManager::Get().GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL=PrepareSQL("select genre.idGenre,genre.strGenre,path.strPath from genre,genrelinkmovie,movie,path,files where genre.idGenre=genrelinkmovie.idGenre and genrelinkmovie.idMovie=movie.idMovie and files.idFile=movie.idFile and path.idPath=files.idPath and genre.strGenre like '%%%s%%'",strSearch.c_str());
    else
      strSQL=PrepareSQL("select distinct genre.idGenre,genre.strGenre from genre,genrelinkmovie where genrelinkmovie.idGenre=genre.idGenre and strGenre like '%%%s%%'", strSearch.c_str());
    m_pDS->query( strSQL.c_str() );

    while (!m_pDS->eof())
    {
      if (CProfilesManager::Get().GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv("path.strPath").get_asString()),
                                                      *CMediaSourceSettings::Get().GetSources("video")))
        {
          m_pDS->next();
          continue;
        }

      CFileItemPtr pItem(new CFileItem(m_pDS->fv("genre.strGenre").get_asString()));
      CStdString strDir = StringUtils::Format("%ld/", m_pDS->fv("genre.idGenre").get_asInt());
      pItem->SetPath("videodb://movies/genres/"+ strDir);
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

void CVideoDatabase::GetMovieCountriesByName(const CStdString& strSearch, CFileItemList& items)
{
  CStdString strSQL;

  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    if (CProfilesManager::Get().GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL=PrepareSQL("select country.idCountry,country.strCountry,path.strPath from country,countrylinkmovie,movie,path,files where country.idCountry=countrylinkmovie.idCountry and countrylinkmovie.idMovie=movie.idMovie and files.idFile=movie.idFile and path.idPath=files.idPath and country.strCountry like '%%%s%%'",strSearch.c_str());
    else
      strSQL=PrepareSQL("select distinct country.idCountry,country.strCountry from country,countrylinkmovie where countrylinkmovie.idCountry=country.idCountry and strCountry like '%%%s%%'", strSearch.c_str());
    m_pDS->query( strSQL.c_str() );

    while (!m_pDS->eof())
    {
      if (CProfilesManager::Get().GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv("path.strPath").get_asString()),
                                                      *CMediaSourceSettings::Get().GetSources("video")))
        {
          m_pDS->next();
          continue;
        }

      CFileItemPtr pItem(new CFileItem(m_pDS->fv("country.strCountry").get_asString()));
      CStdString strDir = StringUtils::Format("%ld/", m_pDS->fv("country.idCountry").get_asInt());
      pItem->SetPath("videodb://movies/genres/"+ strDir);
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

    if (CProfilesManager::Get().GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL=PrepareSQL("select genre.idGenre,genre.strGenre,path.strPath from genre,genrelinktvshow,tvshow,path,tvshowlinkpath where genre.idGenre=genrelinktvshow.idGenre and genrelinktvshow.idShow=tvshow.idShow and path.idPath=tvshowlinkpath.idPath and tvshowlinkpath.idShow=tvshow.idShow and genre.strGenre like '%%%s%%'",strSearch.c_str());
    else
      strSQL=PrepareSQL("select distinct genre.idGenre,genre.strGenre from genre,genrelinktvshow where genrelinktvshow.idGenre=genre.idGenre and strGenre like '%%%s%%'", strSearch.c_str());
    m_pDS->query( strSQL.c_str() );

    while (!m_pDS->eof())
    {
      if (CProfilesManager::Get().GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv("path.strPath").get_asString()),*CMediaSourceSettings::Get().GetSources("video")))
        {
          m_pDS->next();
          continue;
        }

      CFileItemPtr pItem(new CFileItem(m_pDS->fv("genre.strGenre").get_asString()));
      CStdString strDir = StringUtils::Format("%ld/", m_pDS->fv("genre.idGenre").get_asInt());
      pItem->SetPath("videodb://tvshows/genres/"+ strDir);
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

    if (CProfilesManager::Get().GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL=PrepareSQL("select actors.idActor,actors.strActor,path.strPath from actorlinkmovie,actors,movie,files,path where actors.idActor=actorlinkmovie.idActor and actorlinkmovie.idMovie=movie.idMovie and files.idFile=movie.idFile and files.idPath=path.idPath and actors.strActor like '%%%s%%'",strSearch.c_str());
    else
      strSQL=PrepareSQL("select distinct actors.idActor,actors.strActor from actorlinkmovie,actors,movie where actors.idActor=actorlinkmovie.idActor and actorlinkmovie.idMovie=movie.idMovie and actors.strActor like '%%%s%%'",strSearch.c_str());
    m_pDS->query( strSQL.c_str() );

    while (!m_pDS->eof())
    {
      if (CProfilesManager::Get().GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv("path.strPath").get_asString()),*CMediaSourceSettings::Get().GetSources("video")))
        {
          m_pDS->next();
          continue;
        }

      CFileItemPtr pItem(new CFileItem(m_pDS->fv("actors.strActor").get_asString()));
      CStdString strDir = StringUtils::Format("%ld/", m_pDS->fv("actors.idActor").get_asInt());
      pItem->SetPath("videodb://movies/actors/"+ strDir);
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

    if (CProfilesManager::Get().GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL=PrepareSQL("select actors.idActor,actors.strActor,path.strPath from actorlinktvshow,actors,tvshow,path,tvshowlinkpath where actors.idActor=actorlinktvshow.idActor and actorlinktvshow.idShow=tvshow.idShow and tvshowlinkpath.idPath=tvshow.idShow and tvshowlinkpath.idPath=path.idPath and actors.strActor like '%%%s%%'",strSearch.c_str());
    else
      strSQL=PrepareSQL("select distinct actors.idActor,actors.strActor from actorlinktvshow,actors,tvshow where actors.idActor=actorlinktvshow.idActor and actorlinktvshow.idShow=tvshow.idShow and actors.strActor like '%%%s%%'",strSearch.c_str());
    m_pDS->query( strSQL.c_str() );

    while (!m_pDS->eof())
    {
      if (CProfilesManager::Get().GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv("path.strPath").get_asString()),*CMediaSourceSettings::Get().GetSources("video")))
        {
          m_pDS->next();
          continue;
        }

      CFileItemPtr pItem(new CFileItem(m_pDS->fv("actors.strActor").get_asString()));
      CStdString strDir = StringUtils::Format("%ld/", m_pDS->fv("actors.idActor").get_asInt());
      pItem->SetPath("videodb://tvshows/actors/"+ strDir);
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
    if (!strSearch.empty())
      strLike = "and actors.strActor like '%%%s%%'";
    if (CProfilesManager::Get().GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL=PrepareSQL("select actors.idActor,actors.strActor,path.strPath from artistlinkmusicvideo,actors,musicvideo,files,path where actors.idActor=artistlinkmusicvideo.idArtist and artistlinkmusicvideo.idMVideo=musicvideo.idMVideo and files.idFile=musicvideo.idFile and files.idPath=path.idPath "+strLike,strSearch.c_str());
    else
      strSQL=PrepareSQL("select distinct actors.idActor,actors.strActor from artistlinkmusicvideo,actors where actors.idActor=artistlinkmusicvideo.idArtist "+strLike,strSearch.c_str());
    m_pDS->query( strSQL.c_str() );

    while (!m_pDS->eof())
    {
      if (CProfilesManager::Get().GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv("path.strPath").get_asString()),*CMediaSourceSettings::Get().GetSources("video")))
        {
          m_pDS->next();
          continue;
        }

      CFileItemPtr pItem(new CFileItem(m_pDS->fv("actors.strActor").get_asString()));
      CStdString strDir = StringUtils::Format("%ld/", m_pDS->fv("actors.idActor").get_asInt());
      pItem->SetPath("videodb://musicvideos/artists/"+ strDir);
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

    if (CProfilesManager::Get().GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL=PrepareSQL("select genre.idGenre,genre.strGenre,path.strPath from genre,genrelinkmusicvideo,musicvideo,path,files where genre.idGenre=genrelinkmusicvideo.idGenre and genrelinkmusicvideo.idMVideo = musicvideo.idMVideo and files.idFile=musicvideo.idFile and path.idPath=files.idPath and genre.strGenre like '%%%s%%'",strSearch.c_str());
    else
      strSQL=PrepareSQL("select distinct genre.idGenre,genre.strGenre from genre,genrelinkmusicvideo where genrelinkmusicvideo.idGenre=genre.idGenre and genre.strGenre like '%%%s%%'", strSearch.c_str());
    m_pDS->query( strSQL.c_str() );

    while (!m_pDS->eof())
    {
      if (CProfilesManager::Get().GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv("path.strPath").get_asString()),*CMediaSourceSettings::Get().GetSources("video")))
        {
          m_pDS->next();
          continue;
        }

      CFileItemPtr pItem(new CFileItem(m_pDS->fv("genre.strGenre").get_asString()));
      CStdString strDir = StringUtils::Format("%ld/", m_pDS->fv("genre.idGenre").get_asInt());
      pItem->SetPath("videodb://musicvideos/genres/"+ strDir);
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

    strSQL = StringUtils::Format("SELECT DISTINCT"
                                 "  musicvideo.c%02d,"
                                 "  musicvideo.idMVideo,"
                                 "  path.strPath"
                                 " FROM"
                                 "  musicvideo"
                                 " JOIN files ON"
                                 "  files.idFile=musicvideo.idFile"
                                 " JOIN path ON"
                                 "  path.idPath=files.idPath", VIDEODB_ID_MUSICVIDEO_ALBUM);
    if (!strSearch.empty())
      strSQL += PrepareSQL(" WHERE musicvideo.c%02d like '%%%s%%'",VIDEODB_ID_MUSICVIDEO_ALBUM, strSearch.c_str());

    m_pDS->query( strSQL.c_str() );

    while (!m_pDS->eof())
    {
      if (m_pDS->fv(0).get_asString().empty())
      {
        m_pDS->next();
        continue;
      }

      if (CProfilesManager::Get().GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv(2).get_asString()),*CMediaSourceSettings::Get().GetSources("video")))
        {
          m_pDS->next();
          continue;
        }

      CFileItemPtr pItem(new CFileItem(m_pDS->fv(0).get_asString()));
      CStdString strDir = StringUtils::Format("%ld", m_pDS->fv(1).get_asInt());
      pItem->SetPath("videodb://musicvideos/titles/"+ strDir);
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

    if (CProfilesManager::Get().GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL = PrepareSQL("select musicvideo.idMVideo,musicvideo.c%02d,musicvideo.c%02d,path.strPath from musicvideo,files,path where files.idFile=musicvideo.idFile and files.idPath=path.idPath and musicvideo.c%02d like '%%%s%%'",VIDEODB_ID_MUSICVIDEO_ALBUM,VIDEODB_ID_MUSICVIDEO_TITLE,VIDEODB_ID_MUSICVIDEO_ALBUM,strSearch.c_str());
    else
      strSQL = PrepareSQL("select musicvideo.idMVideo,musicvideo.c%02d,musicvideo.c%02d from musicvideo where musicvideo.c%02d like '%%%s%%'",VIDEODB_ID_MUSICVIDEO_ALBUM,VIDEODB_ID_MUSICVIDEO_TITLE,VIDEODB_ID_MUSICVIDEO_ALBUM,strSearch.c_str());
    m_pDS->query( strSQL.c_str() );

    while (!m_pDS->eof())
    {
      if (CProfilesManager::Get().GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv("path.strPath").get_asString()),*CMediaSourceSettings::Get().GetSources("video")))
        {
          m_pDS->next();
          continue;
        }

      CFileItemPtr pItem(new CFileItem(m_pDS->fv(1).get_asString()+" - "+m_pDS->fv(2).get_asString()));
      CStdString strDir = StringUtils::Format("3/2/%ld",m_pDS->fv("musicvideo.idMVideo").get_asInt());

      pItem->SetPath("videodb://"+ strDir);
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

bool CVideoDatabase::GetMusicVideosByWhere(const CStdString &baseDir, const Filter &filter, CFileItemList &items, bool checkLocks /*= true*/, const SortDescription &sortDescription /* = SortDescription() */)
{
  try
  {
    movieTime = 0;
    castTime = 0;

    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    int total = -1;
    
    std::string strSQL = "select %s from musicvideoview ";
    CVideoDbUrl videoUrl;
    std::string strSQLExtra;
    Filter extFilter = filter;
    SortDescription sorting = sortDescription;
    if (!BuildSQL(baseDir, strSQLExtra, extFilter, strSQLExtra, videoUrl, sorting))
      return false;

    // Apply the limiting directly here if there's no special sorting but limiting
    if (extFilter.limit.empty() &&
      sorting.sortBy == SortByNone &&
      (sorting.limitStart > 0 || sorting.limitEnd > 0))
    {
      total = (int)strtol(GetSingleValue(PrepareSQL(strSQL, "COUNT(1)") + strSQLExtra, m_pDS).c_str(), NULL, 10);
      strSQLExtra += DatabaseUtils::BuildLimitClause(sorting.limitEnd, sorting.limitStart);
    }

    strSQL = PrepareSQL(strSQL, !extFilter.fields.empty() ? extFilter.fields.c_str() : "*") + strSQLExtra;

    int iRowsFound = RunQuery(strSQL);
    if (iRowsFound <= 0)
      return iRowsFound == 0;

    // store the total value of items as a property
    if (total < iRowsFound)
      total = iRowsFound;
    items.SetProperty("total", total);
    
    DatabaseResults results;
    results.reserve(iRowsFound);
    if (!SortUtils::SortFromDataset(sorting, MediaTypeMusicVideo, m_pDS, results))
      return false;
    
    // get data from returned rows
    items.Reserve(results.size());
    // get songs from returned subtable
    const query_data &data = m_pDS->get_result_set().records;
    for (DatabaseResults::const_iterator it = results.begin(); it != results.end(); it++)
    {
      unsigned int targetRow = (unsigned int)it->at(FieldRow).asInteger();
      const dbiplus::sql_record* const record = data.at(targetRow);
      
      CVideoInfoTag musicvideo = GetDetailsForMusicVideo(record);
      if (!checkLocks || CProfilesManager::Get().GetMasterProfile().getLockMode() == LOCK_MODE_EVERYONE || g_passwordManager.bMasterUser ||
          g_passwordManager.IsDatabasePathUnlocked(musicvideo.m_strPath, *CMediaSourceSettings::Get().GetSources("video")))
      {
        CFileItemPtr item(new CFileItem(musicvideo));

        CVideoDbUrl itemUrl = videoUrl;
        CStdString path = StringUtils::Format("%ld", record->at(0).get_asInt());
        itemUrl.AppendPath(path);
        item->SetPath(itemUrl.ToString());

        item->SetOverlayImage(CGUIListItem::ICON_OVERLAY_UNWATCHED, musicvideo.m_playCount > 0);
        items.Add(item);
      }
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

    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    // We don't use PrepareSQL here, as the WHERE clause is already formatted.
    CStdString strSQL = StringUtils::Format("select * from musicvideoview where %s", strWhere.c_str());
    strSQL += PrepareSQL(" order by RANDOM() limit 1");
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
    CStdString path = StringUtils::Format("videodb://musicvideos/titles/%ld",item->GetVideoInfoTag()->m_iDbId);
    item->SetPath(path);
    idSong = m_pDS->fv("idMVideo").get_asInt();
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
    if (strAlbum.empty() && strTitle.empty())
    { // we want to return matching artists only
      if (CProfilesManager::Get().GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        strSQL=PrepareSQL("select distinct actors.idActor,path.strPath from artistlinkmusicvideo,actors,musicvideo,files,path where actors.idActor=artistlinkmusicvideo.idArtist and artistlinkmusicvideo.idMVideo=musicvideo.idMVideo and files.idFile=musicvideo.idFile and files.idPath=path.idPath and actors.strActor like '%s'",strArtist.c_str());
      else
        strSQL=PrepareSQL("select distinct actors.idActor from artistlinkmusicvideo,actors where actors.idActor=artistlinkmusicvideo.idArtist and actors.strActor like '%s'",strArtist.c_str());
    }
    else
    { // we want to return the matching musicvideo
      if (CProfilesManager::Get().GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        strSQL = PrepareSQL("select musicvideo.idMVideo from musicvideo,files,path,artistlinkmusicvideo,actors where files.idFile=musicvideo.idFile and files.idPath=path.idPath and musicvideo.c%02d like '%s' and musicvideo.c%02d like '%s' and artistlinkmusicvideo.idMVideo=musicvideo.idMVideo and artistlinkmusicvideo.idArtist=actors.idActor and actors.strActor like '%s'",VIDEODB_ID_MUSICVIDEO_ALBUM,strAlbum.c_str(),VIDEODB_ID_MUSICVIDEO_TITLE,strTitle.c_str(),strArtist.c_str());
      else
        strSQL = PrepareSQL("select musicvideo.idMVideo from musicvideo join artistlinkmusicvideo on artistlinkmusicvideo.idMVideo=musicvideo.idMVideo join actors on actors.idActor=artistlinkmusicvideo.idArtist where musicvideo.c%02d like '%s' and musicvideo.c%02d like '%s' and actors.strActor like '%s'",VIDEODB_ID_MUSICVIDEO_ALBUM,strAlbum.c_str(),VIDEODB_ID_MUSICVIDEO_TITLE,strTitle.c_str(),strArtist.c_str());
    }
    m_pDS->query( strSQL.c_str() );

    if (m_pDS->eof())
      return -1;

    if (CProfilesManager::Get().GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      if (!g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv("path.strPath").get_asString()),*CMediaSourceSettings::Get().GetSources("video")))
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

    if (CProfilesManager::Get().GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL = PrepareSQL("select movie.idMovie,movie.c%02d,path.strPath, movie.idSet from movie,files,path where files.idFile=movie.idFile and files.idPath=path.idPath and movie.c%02d like '%%%s%%'",VIDEODB_ID_TITLE,VIDEODB_ID_TITLE,strSearch.c_str());
    else
      strSQL = PrepareSQL("select movie.idMovie,movie.c%02d, movie.idSet from movie where movie.c%02d like '%%%s%%'",VIDEODB_ID_TITLE,VIDEODB_ID_TITLE,strSearch.c_str());
    m_pDS->query( strSQL.c_str() );

    while (!m_pDS->eof())
    {
      if (CProfilesManager::Get().GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv("path.strPath").get_asString()),*CMediaSourceSettings::Get().GetSources("video")))
        {
          m_pDS->next();
          continue;
        }

      int movieId = m_pDS->fv("movie.idMovie").get_asInt();
      int setId = m_pDS->fv("movie.idSet").get_asInt();
      CFileItemPtr pItem(new CFileItem(m_pDS->fv(1).get_asString()));
      CStdString path;
      if (setId <= 0 || !CSettings::Get().GetBool("videolibrary.groupmoviesets"))
        path = StringUtils::Format("videodb://movies/titles/%i", movieId);
      else
        path = StringUtils::Format("videodb://movies/sets/%i/%i", setId, movieId);
      pItem->SetPath(path);
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

    if (CProfilesManager::Get().GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL = PrepareSQL("select tvshow.idShow,tvshow.c%02d,path.strPath from tvshow,path,tvshowlinkpath where tvshowlinkpath.idPath=path.idPath and tvshowlinkpath.idShow=tvshow.idShow and tvshow.c%02d like '%%%s%%'",VIDEODB_ID_TV_TITLE,VIDEODB_ID_TV_TITLE,strSearch.c_str());
    else
      strSQL = PrepareSQL("select tvshow.idShow,tvshow.c%02d from tvshow where tvshow.c%02d like '%%%s%%'",VIDEODB_ID_TV_TITLE,VIDEODB_ID_TV_TITLE,strSearch.c_str());
    m_pDS->query( strSQL.c_str() );

    while (!m_pDS->eof())
    {
      if (CProfilesManager::Get().GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv("path.strPath").get_asString()),*CMediaSourceSettings::Get().GetSources("video")))
        {
          m_pDS->next();
          continue;
        }

      CFileItemPtr pItem(new CFileItem(m_pDS->fv(1).get_asString()));
      CStdString strDir = StringUtils::Format("tvshows/titles/%ld/", m_pDS->fv("tvshow.idShow").get_asInt());

      pItem->SetPath("videodb://"+ strDir);
      pItem->m_bIsFolder=true;
      pItem->GetVideoInfoTag()->m_iDbId = m_pDS->fv("tvshow.idShow").get_asInt();
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

    if (CProfilesManager::Get().GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL = PrepareSQL("select episode.idEpisode,episode.c%02d,episode.c%02d,episode.idShow,tvshow.c%02d,path.strPath from episode,files,path,tvshow where files.idFile=episode.idFile and episode.idShow=tvshow.idShow and files.idPath=path.idPath and episode.c%02d like '%%%s%%'",VIDEODB_ID_EPISODE_TITLE,VIDEODB_ID_EPISODE_SEASON,VIDEODB_ID_TV_TITLE,VIDEODB_ID_EPISODE_TITLE,strSearch.c_str());
    else
      strSQL = PrepareSQL("select episode.idEpisode,episode.c%02d,episode.c%02d,episode.idShow,tvshow.c%02d from episode,tvshow where tvshow.idShow=episode.idShow and episode.c%02d like '%%%s%%'",VIDEODB_ID_EPISODE_TITLE,VIDEODB_ID_EPISODE_SEASON,VIDEODB_ID_TV_TITLE,VIDEODB_ID_EPISODE_TITLE,strSearch.c_str());
    m_pDS->query( strSQL.c_str() );

    while (!m_pDS->eof())
    {
      if (CProfilesManager::Get().GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv("path.strPath").get_asString()),*CMediaSourceSettings::Get().GetSources("video")))
        {
          m_pDS->next();
          continue;
        }

      CFileItemPtr pItem(new CFileItem(m_pDS->fv(1).get_asString()+" ("+m_pDS->fv(4).get_asString()+")"));
      CStdString path = StringUtils::Format("videodb://tvshows/titles/%ld/%ld/%ld",m_pDS->fv("episode.idShow").get_asInt(),m_pDS->fv(2).get_asInt(),m_pDS->fv(0).get_asInt());
      pItem->SetPath(path);
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
//  Filter filter(PrepareSQL("c%02d like '%s%%' or c%02d like '%% %s%%'", VIDEODB_ID_MUSICVIDEO_TITLE, strSearch.c_str(), VIDEODB_ID_MUSICVIDEO_TITLE, strSearch.c_str()));
//  GetMusicVideosByWhere("videodb://musicvideos/titles/", filter, items);
  CStdString strSQL;

  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    if (CProfilesManager::Get().GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL = PrepareSQL("select musicvideo.idMVideo,musicvideo.c%02d,path.strPath from musicvideo,files,path where files.idFile=musicvideo.idFile and files.idPath=path.idPath and musicvideo.c%02d like '%%%s%%'",VIDEODB_ID_MUSICVIDEO_TITLE,VIDEODB_ID_MUSICVIDEO_TITLE,strSearch.c_str());
    else
      strSQL = PrepareSQL("select musicvideo.idMVideo,musicvideo.c%02d from musicvideo where musicvideo.c%02d like '%%%s%%'",VIDEODB_ID_MUSICVIDEO_TITLE,VIDEODB_ID_MUSICVIDEO_TITLE,strSearch.c_str());
    m_pDS->query( strSQL.c_str() );

    while (!m_pDS->eof())
    {
      if (CProfilesManager::Get().GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv("path.strPath").get_asString()),*CMediaSourceSettings::Get().GetSources("video")))
        {
          m_pDS->next();
          continue;
        }

      CFileItemPtr pItem(new CFileItem(m_pDS->fv(1).get_asString()));
      CStdString strDir = StringUtils::Format("3/2/%ld",m_pDS->fv("musicvideo.idMVideo").get_asInt());

      pItem->SetPath("videodb://"+ strDir);
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
//  Filter filter;
//  filter.where = PrepareSQL("c%02d like '%s%%' or c%02d like '%% %s%%'", VIDEODB_ID_EPISODE_PLOT, strSearch.c_str(), VIDEODB_ID_EPISODE_PLOT, strSearch.c_str());
//  filter.where += PrepareSQL("or c%02d like '%s%%' or c%02d like '%% %s%%'", VIDEODB_ID_EPISODE_TITLE, strSearch.c_str(), VIDEODB_ID_EPISODE_TITLE, strSearch.c_str());
//  GetEpisodesByWhere("videodb://tvshows/titles/", filter, items);
//  return;
  CStdString strSQL;

  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    if (CProfilesManager::Get().GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL = PrepareSQL("select episode.idEpisode,episode.c%02d,episode.c%02d,episode.idShow,tvshow.c%02d,path.strPath from episode,files,path,tvshow where files.idFile=episode.idFile and files.idPath=path.idPath and tvshow.idShow=episode.idShow and episode.c%02d like '%%%s%%'",VIDEODB_ID_EPISODE_TITLE,VIDEODB_ID_EPISODE_SEASON,VIDEODB_ID_TV_TITLE,VIDEODB_ID_EPISODE_PLOT,strSearch.c_str());
    else
      strSQL = PrepareSQL("select episode.idEpisode,episode.c%02d,episode.c%02d,episode.idShow,tvshow.c%02d from episode,tvshow where tvshow.idShow=episode.idShow and episode.c%02d like '%%%s%%'",VIDEODB_ID_EPISODE_TITLE,VIDEODB_ID_EPISODE_SEASON,VIDEODB_ID_TV_TITLE,VIDEODB_ID_EPISODE_PLOT,strSearch.c_str());
    m_pDS->query( strSQL.c_str() );

    while (!m_pDS->eof())
    {
      if (CProfilesManager::Get().GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv("path.strPath").get_asString()),*CMediaSourceSettings::Get().GetSources("video")))
        {
          m_pDS->next();
          continue;
        }

      CFileItemPtr pItem(new CFileItem(m_pDS->fv(1).get_asString()+" ("+m_pDS->fv(4).get_asString()+")"));
      CStdString path = StringUtils::Format("videodb://tvshows/titles/%ld/%ld/%ld",m_pDS->fv("episode.idShow").get_asInt(),m_pDS->fv(2).get_asInt(),m_pDS->fv(0).get_asInt());
      pItem->SetPath(path);
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

    if (CProfilesManager::Get().GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL = PrepareSQL("select movie.idMovie, movie.c%02d, path.strPath from movie,files,path where files.idFile=movie.idFile and files.idPath=path.idPath and (movie.c%02d like '%%%s%%' or movie.c%02d like '%%%s%%' or movie.c%02d like '%%%s%%')",VIDEODB_ID_TITLE,VIDEODB_ID_PLOT,strSearch.c_str(),VIDEODB_ID_PLOTOUTLINE,strSearch.c_str(),VIDEODB_ID_TAGLINE,strSearch.c_str());
    else
      strSQL = PrepareSQL("select movie.idMovie, movie.c%02d from movie where (movie.c%02d like '%%%s%%' or movie.c%02d like '%%%s%%' or movie.c%02d like '%%%s%%')",VIDEODB_ID_TITLE,VIDEODB_ID_PLOT,strSearch.c_str(),VIDEODB_ID_PLOTOUTLINE,strSearch.c_str(),VIDEODB_ID_TAGLINE,strSearch.c_str());

    m_pDS->query( strSQL.c_str() );

    while (!m_pDS->eof())
    {
      if (CProfilesManager::Get().GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv(2).get_asString()),*CMediaSourceSettings::Get().GetSources("video")))
        {
          m_pDS->next();
          continue;
        }

      CFileItemPtr pItem(new CFileItem(m_pDS->fv(1).get_asString()));
      CStdString path = StringUtils::Format("videodb://movies/titles/%ld", m_pDS->fv(0).get_asInt());
      pItem->SetPath(path);
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

    if (CProfilesManager::Get().GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL = PrepareSQL("select distinct directorlinkmovie.idDirector,actors.strActor,path.strPath from movie,files,path,actors,directorlinkmovie where files.idFile=movie.idFile and files.idPath=path.idPath and directorlinkmovie.idMovie=movie.idMovie and directorlinkmovie.idDirector=actors.idActor and actors.strActor like '%%%s%%'",strSearch.c_str());
    else
      strSQL = PrepareSQL("select distinct directorlinkmovie.idDirector,actors.strActor from movie,actors,directorlinkmovie where directorlinkmovie.idMovie=movie.idMovie and directorlinkmovie.idDirector=actors.idActor and actors.strActor like '%%%s%%'",strSearch.c_str());

    m_pDS->query( strSQL.c_str() );

    while (!m_pDS->eof())
    {
      if (CProfilesManager::Get().GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv("path.strPath").get_asString()),*CMediaSourceSettings::Get().GetSources("video")))
        {
          m_pDS->next();
          continue;
        }

      CStdString strDir = StringUtils::Format("%ld/", m_pDS->fv("directorlinkmovie.idDirector").get_asInt());
      CFileItemPtr pItem(new CFileItem(m_pDS->fv("actors.strActor").get_asString()));

      pItem->SetPath("videodb://movies/directors/"+ strDir);
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

    if (CProfilesManager::Get().GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL = PrepareSQL("select distinct directorlinktvshow.idDirector,actors.strActor,path.strPath from tvshow,path,actors,directorlinktvshow,tvshowlinkpath where tvshowlinkpath.idPath=path.idPath and tvshowlinkpath.idShow=tvshow.idShow and directorlinktvshow.idShow=tvshow.idShow and directorlinktvshow.idDirector=actors.idActor and actors.strActor like '%%%s%%'",strSearch.c_str());
    else
      strSQL = PrepareSQL("select distinct directorlinktvshow.idDirector,actors.strActor from tvshow,actors,directorlinktvshow where directorlinktvshow.idShow=tvshow.idShow and directorlinktvshow.idDirector=actors.idActor and actors.strActor like '%%%s%%'",strSearch.c_str());

    m_pDS->query( strSQL.c_str() );

    while (!m_pDS->eof())
    {
      if (CProfilesManager::Get().GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv("path.strPath").get_asString()),*CMediaSourceSettings::Get().GetSources("video")))
        {
          m_pDS->next();
          continue;
        }

      CStdString strDir = StringUtils::Format("%ld/", m_pDS->fv("directorlinktvshow.idDirector").get_asInt());
      CFileItemPtr pItem(new CFileItem(m_pDS->fv("actors.strActor").get_asString()));

      pItem->SetPath("videodb://tvshows/studios/"+ strDir);
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

    if (CProfilesManager::Get().GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL = PrepareSQL("select distinct directorlinkmusicvideo.idDirector,actors.strActor,path.strPath from musicvideo,files,path,actors,directorlinkmusicvideo where files.idFile=musicvideo.idFile and files.idPath=path.idPath and directorlinkmusicvideo.idMVideo=musicvideo.idMVideo and directorlinkmusicvideo.idDirector=actors.idActor and actors.strActor like '%%%s%%'",strSearch.c_str());
    else
      strSQL = PrepareSQL("select distinct directorlinkmusicvideo.idDirector,actors.strActor from musicvideo,actors,directorlinkmusicvideo where directorlinkmusicvideo.idMVideo=musicvideo.idMVideo and directorlinkmusicvideo.idDirector=actors.idActor and actors.strActor like '%%%s%%'",strSearch.c_str());

    m_pDS->query( strSQL.c_str() );

    while (!m_pDS->eof())
    {
      if (CProfilesManager::Get().GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv("path.strPath").get_asString()),*CMediaSourceSettings::Get().GetSources("video")))
        {
          m_pDS->next();
          continue;
        }

      CStdString strDir = StringUtils::Format("%ld/", m_pDS->fv("directorlinkmusicvideo.idDirector").get_asInt());
      CFileItemPtr pItem(new CFileItem(m_pDS->fv("actors.strActor").get_asString()));

      pItem->SetPath("videodb://musicvideos/albums/"+ strDir);
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

void CVideoDatabase::CleanDatabase(CGUIDialogProgressBarHandle* handle, const set<int>* paths, bool showProgress)
{
  CGUIDialogProgress *progress=NULL;
  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    unsigned int time = XbmcThreads::SystemClockMillis();
    CLog::Log(LOGNOTICE, "%s: Starting videodatabase cleanup ..", __FUNCTION__);
    ANNOUNCEMENT::CAnnouncementManager::Get().Announce(ANNOUNCEMENT::VideoLibrary, "xbmc", "OnCleanStarted");

    BeginTransaction();

    // find all the files
    CStdString sql = "SELECT files.idFile, files.strFileName, path.strPath FROM files, path WHERE files.idPath = path.idPath";
    if (paths)
    {
      if (paths->empty())
      {
        RollbackTransaction();
        ANNOUNCEMENT::CAnnouncementManager::Get().Announce(ANNOUNCEMENT::VideoLibrary, "xbmc", "OnCleanFinished");
        return;
      }

      CStdString strPaths;
      for (std::set<int>::const_iterator it = paths->begin(); it != paths->end(); ++it)
        strPaths += StringUtils::Format(",%i", *it);
      sql += PrepareSQL(" AND path.idPath IN (%s)", strPaths.substr(1).c_str());
    }

    m_pDS->query(sql.c_str());
    if (m_pDS->num_rows() == 0) return;

    if (handle)
    {
      handle->SetTitle(g_localizeStrings.Get(700));
      handle->SetText("");
    }
    else if (showProgress)
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

    std::string filesToTestForDelete;

    int total = m_pDS->num_rows();
    int current = 0;

    while (!m_pDS->eof())
    {
      std::string path = m_pDS->fv("path.strPath").get_asString();
      std::string fileName = m_pDS->fv("files.strFileName").get_asString();
      CStdString fullPath;
      ConstructPath(fullPath, path, fileName);

      // get the first stacked file
      if (URIUtils::IsStack(fullPath))
        fullPath = CStackDirectory::GetFirstStackedFile(fullPath);

      // remove optical, non-existing files
      if (URIUtils::IsOnDVD(fullPath) || !CFile::Exists(fullPath, false))
        filesToTestForDelete += m_pDS->fv("files.idFile").get_asString() + ",";

      if (handle == NULL && progress != NULL)
      {
        int percentage = current * 100 / total;
        if (percentage > progress->GetPercentage())
        {
          progress->SetPercentage(percentage);
          progress->Progress();
        }
        if (progress->IsCanceled())
        {
          progress->Close();
          m_pDS->close();
          ANNOUNCEMENT::CAnnouncementManager::Get().Announce(ANNOUNCEMENT::VideoLibrary, "xbmc", "OnCleanFinished");
          return;
        }
      }
      else if (handle != NULL)
        handle->SetPercentage(current * 100 / (float)total);

      m_pDS->next();
      current++;
    }
    m_pDS->close();

    std::string filesToDelete;

    // Add any files that don't have a valid idPath entry to the filesToDelete list.
    m_pDS->query("SELECT files.idFile FROM files WHERE NOT EXISTS (SELECT 1 FROM path WHERE path.idPath = files.idPath)");
    while (!m_pDS->eof())
    {
      string file = m_pDS->fv("files.idFile").get_asString() + ",";
      filesToTestForDelete += file;
      filesToDelete += file;

      m_pDS->next();
    }
    m_pDS->close();

    std::map<int, bool> pathsDeleteDecisions;
    std::vector<int> movieIDs;
    std::vector<int> tvshowIDs;
    std::vector<int> episodeIDs;
    std::vector<int> musicVideoIDs;

    if (!filesToTestForDelete.empty())
    {
      StringUtils::TrimRight(filesToTestForDelete, ",");

      movieIDs = CleanMediaType(MediaTypeMovie, filesToTestForDelete, pathsDeleteDecisions, filesToDelete, !showProgress);
      episodeIDs = CleanMediaType(MediaTypeEpisode, filesToTestForDelete, pathsDeleteDecisions, filesToDelete, !showProgress);
      musicVideoIDs = CleanMediaType(MediaTypeMusicVideo, filesToTestForDelete, pathsDeleteDecisions, filesToDelete, !showProgress);
    }

    if (progress != NULL)
    {
      progress->SetPercentage(100);
      progress->Progress();
    }

    if (!filesToDelete.empty())
    {
      filesToDelete = "(" + StringUtils::TrimRight(filesToDelete, ",") + ")";

      CLog::Log(LOGDEBUG, "%s: Cleaning files table", __FUNCTION__);
      sql = "DELETE FROM files WHERE idFile IN " + filesToDelete;
      m_pDS->exec(sql.c_str());

      CLog::Log(LOGDEBUG, "%s: Cleaning streamdetails table", __FUNCTION__);
      sql = "DELETE FROM streamdetails WHERE idFile IN " + filesToDelete;
      m_pDS->exec(sql.c_str());

      CLog::Log(LOGDEBUG, "%s: Cleaning bookmark table", __FUNCTION__);
      sql = "DELETE FROM bookmark WHERE idFile IN " + filesToDelete;
      m_pDS->exec(sql.c_str());

      CLog::Log(LOGDEBUG, "%s: Cleaning settings table", __FUNCTION__);
      sql = "DELETE FROM settings WHERE idFile IN " + filesToDelete;
      m_pDS->exec(sql.c_str());

      CLog::Log(LOGDEBUG, "%s: Cleaning stacktimes table", __FUNCTION__);
      sql = "DELETE FROM stacktimes WHERE idFile IN " + filesToDelete;
      m_pDS->exec(sql.c_str());
    }

    if (!movieIDs.empty())
    {
      std::string moviesToDelete;
      for (std::vector<int>::const_iterator it = movieIDs.begin(); it != movieIDs.end(); ++it)
        moviesToDelete += StringUtils::Format("%i,", *it);
      moviesToDelete = "(" + StringUtils::TrimRight(moviesToDelete, ",") + ")";

      CLog::Log(LOGDEBUG, "%s: Cleaning movie table", __FUNCTION__);
      sql = "DELETE FROM movie WHERE idMovie IN " + moviesToDelete;
      m_pDS->exec(sql.c_str());
    }

    if (!episodeIDs.empty())
    {
      std::string episodesToDelete;
      for (std::vector<int>::const_iterator it = episodeIDs.begin(); it != episodeIDs.end(); ++it)
        episodesToDelete += StringUtils::Format("%i,", *it);
      episodesToDelete = "(" + StringUtils::TrimRight(episodesToDelete, ",") + ")";

      CLog::Log(LOGDEBUG, "%s: Cleaning episode table", __FUNCTION__);
      sql = "DELETE FROM episode WHERE idEpisode IN " + episodesToDelete;
      m_pDS->exec(sql.c_str());
    }

    CLog::Log(LOGDEBUG, "%s: Cleaning paths that don't exist and have content set...", __FUNCTION__);
    sql = "SELECT path.idPath, path.strPath FROM path "
            "WHERE NOT ((strContent IS NULL OR strContent = '') "
                   "AND (strSettings IS NULL OR strSettings = '') "
                   "AND (strHash IS NULL OR strHash = '') "
                   "AND (exclude IS NULL OR exclude != 1))";
    m_pDS->query(sql.c_str());
    std::string strIds;
    while (!m_pDS->eof())
    {
      std::map<int, bool>::const_iterator pathsDeleteDecision = pathsDeleteDecisions.find(m_pDS->fv(0).get_asInt());
      if ((pathsDeleteDecision != pathsDeleteDecisions.end() && pathsDeleteDecision->second) ||
          (pathsDeleteDecision == pathsDeleteDecisions.end() && !CDirectory::Exists(m_pDS->fv(1).get_asString(), false)))
        strIds += m_pDS->fv(0).get_asString() + ",";

      m_pDS->next();
    }
    m_pDS->close();

    if (!strIds.empty())
    {
      sql = PrepareSQL("DELETE FROM path WHERE idPath IN (%s)", StringUtils::TrimRight(strIds, ",").c_str());
      m_pDS->exec(sql.c_str());
      sql = "DELETE FROM tvshowlinkpath WHERE NOT EXISTS (SELECT 1 FROM path WHERE path.idPath = tvshowlinkpath.idPath)";
      m_pDS->exec(sql.c_str());
    }

    CLog::Log(LOGDEBUG, "%s: Cleaning tvshow table", __FUNCTION__);
    sql = "DELETE FROM tvshow WHERE NOT EXISTS (SELECT 1 FROM tvshowlinkpath WHERE tvshowlinkpath.idShow = tvshow.idShow)";
    m_pDS->exec(sql.c_str());

    std::string tvshowsToDelete;
    sql = "SELECT tvshow.idShow FROM tvshow "
            "JOIN tvshowlinkpath ON tvshow.idShow = tvshowlinkpath.idShow "
            "JOIN path ON path.idPath = tvshowlinkpath.idPath "
          "WHERE NOT EXISTS (SELECT 1 FROM episode WHERE episode.idShow = tvshow.idShow) "
            "AND (path.strContent IS NULL OR path.strContent = '')";
    m_pDS->query(sql.c_str());
    while (!m_pDS->eof())
    {
      tvshowIDs.push_back(m_pDS->fv(0).get_asInt());
      tvshowsToDelete += m_pDS->fv(0).get_asString() + ",";
      m_pDS->next();
    }
    m_pDS->close();
    if (!tvshowsToDelete.empty())
    {
      sql = "DELETE FROM tvshow WHERE idShow IN (" + StringUtils::TrimRight(tvshowsToDelete, ",") + ")";
      m_pDS->exec(sql.c_str());
    }

    if (!musicVideoIDs.empty())
    {
      std::string musicVideosToDelete;
      for (std::vector<int>::const_iterator it = musicVideoIDs.begin(); it != musicVideoIDs.end(); ++it)
        musicVideosToDelete += StringUtils::Format("%i,", *it);
      musicVideosToDelete = "(" + StringUtils::TrimRight(musicVideosToDelete, ",") + ")";

      CLog::Log(LOGDEBUG, "%s: Cleaning musicvideo table", __FUNCTION__);
      sql = "DELETE FROM musicvideo WHERE idMVideo IN " + musicVideosToDelete;
      m_pDS->exec(sql.c_str());
    }

    CLog::Log(LOGDEBUG, "%s: Cleaning path table", __FUNCTION__);
    sql = StringUtils::Format("DELETE FROM path "
                                "WHERE (strContent IS NULL OR strContent = '') "
                                  "AND (strSettings IS NULL OR strSettings = '') "
                                  "AND (strHash IS NULL OR strHash = '') "
                                  "AND (exclude IS NULL OR exclude != 1) "
                                  "AND (idParentPath IS NULL OR NOT EXISTS (SELECT 1 FROM (SELECT idPath FROM path) as parentPath WHERE parentPath.idPath = path.idParentPath)) " // MySQL only fix (#5007)
                                  "AND NOT EXISTS (SELECT 1 FROM files WHERE files.idPath = path.idPath) "
                                  "AND NOT EXISTS (SELECT 1 FROM tvshowlinkpath WHERE tvshowlinkpath.idPath = path.idPath) "
                                  "AND NOT EXISTS (SELECT 1 FROM movie WHERE movie.c%02d = path.idPath) "
                                  "AND NOT EXISTS (SELECT 1 FROM episode WHERE episode.c%02d = path.idPath) "
                                  "AND NOT EXISTS (SELECT 1 FROM musicvideo WHERE musicvideo.c%02d = path.idPath)"
                , VIDEODB_ID_PARENTPATHID, VIDEODB_ID_EPISODE_PARENTPATHID, VIDEODB_ID_MUSICVIDEO_PARENTPATHID );
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, "%s: Cleaning genre table", __FUNCTION__);
    sql = "DELETE FROM genre "
            "WHERE NOT EXISTS (SELECT 1 FROM genrelinkmovie WHERE genrelinkmovie.idGenre = genre.idGenre) "
              "AND NOT EXISTS (SELECT 1 FROM genrelinktvshow WHERE genrelinktvshow.idGenre = genre.idGenre) "
              "AND NOT EXISTS (SELECT 1 FROM genrelinkmusicvideo WHERE genrelinkmusicvideo.idGenre = genre.idGenre)";
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, "%s: Cleaning country table", __FUNCTION__);
    sql = "DELETE FROM country WHERE NOT EXISTS (SELECT 1 FROM countrylinkmovie WHERE countrylinkmovie.idCountry = country.idCountry)";
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, "%s: Cleaning actor table of actors, directors and writers", __FUNCTION__);
    sql = "DELETE FROM actors "
            "WHERE NOT EXISTS (SELECT 1 FROM actorlinkmovie WHERE actorlinkmovie.idActor = actors.idActor) "
              "AND NOT EXISTS (SELECT 1 FROM directorlinkmovie WHERE directorlinkmovie.idDirector = actors.idActor) "
              "AND NOT EXISTS (SELECT 1 FROM writerlinkmovie WHERE writerlinkmovie.idWriter = actors.idActor) "
              "AND NOT EXISTS (SELECT 1 FROM actorlinktvshow WHERE actorlinktvshow.idActor = actors.idActor) "
              "AND NOT EXISTS (SELECT 1 FROM actorlinkepisode WHERE actorlinkepisode.idActor = actors.idActor) "
              "AND NOT EXISTS (SELECT 1 FROM directorlinktvshow WHERE directorlinktvshow.idDirector = actors.idActor) "
              "AND NOT EXISTS (SELECT 1 FROM directorlinkepisode WHERE directorlinkepisode.idDirector = actors.idActor) "
              "AND NOT EXISTS (SELECT 1 FROM writerlinkepisode WHERE writerlinkepisode.idWriter = actors.idActor) "
              "AND NOT EXISTS (SELECT 1 FROM artistlinkmusicvideo WHERE artistlinkmusicvideo.idArtist = actors.idActor) "
              "AND NOT EXISTS (SELECT 1 FROM directorlinkmusicvideo WHERE directorlinkmusicvideo.idDirector = actors.idActor)";
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, "%s: Cleaning studio table", __FUNCTION__);
    sql = "DELETE FROM studio "
            "WHERE NOT EXISTS (SELECT 1 FROM studiolinkmovie WHERE studiolinkmovie.idStudio = studio.idStudio) "
              "AND NOT EXISTS (SELECT 1 FROM studiolinkmusicvideo WHERE studiolinkmusicvideo.idStudio = studio.idStudio) "
              "AND NOT EXISTS (SELECT 1 FROM studiolinktvshow WHERE studiolinktvshow.idStudio = studio.idStudio)";
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, "%s: Cleaning set table", __FUNCTION__);
    sql = "DELETE FROM sets WHERE NOT EXISTS (SELECT 1 FROM movie WHERE movie.idSet = sets.idSet)";
    m_pDS->exec(sql.c_str());

    CommitTransaction();

    if (handle)
      handle->SetTitle(g_localizeStrings.Get(331));

    Compress(false);

    CUtil::DeleteVideoDatabaseDirectoryCache();

    time = XbmcThreads::SystemClockMillis() - time;
    CLog::Log(LOGNOTICE, "%s: Cleaning videodatabase done. Operation took %s", __FUNCTION__, StringUtils::SecondsToTimeString(time / 1000).c_str());

    for (std::vector<int>::const_iterator it = movieIDs.begin(); it != movieIDs.end(); ++it)
      AnnounceRemove(MediaTypeMovie, *it, true);

    for (std::vector<int>::const_iterator it = episodeIDs.begin(); it != episodeIDs.end(); ++it)
      AnnounceRemove(MediaTypeEpisode, *it, true);

    for (std::vector<int>::const_iterator it = tvshowIDs.begin(); it != tvshowIDs.end(); ++it)
      AnnounceRemove(MediaTypeTvShow, *it, true);

    for (std::vector<int>::const_iterator it = musicVideoIDs.begin(); it != musicVideoIDs.end(); ++it)
      AnnounceRemove(MediaTypeMusicVideo, *it, true);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
    RollbackTransaction();
  }
  if (progress)
    progress->Close();

  ANNOUNCEMENT::CAnnouncementManager::Get().Announce(ANNOUNCEMENT::VideoLibrary, "xbmc", "OnCleanFinished");
}

std::vector<int> CVideoDatabase::CleanMediaType(const std::string &mediaType, const std::string &cleanableFileIDs,
                                                std::map<int, bool> &pathsDeleteDecisions, std::string &deletedFileIDs, bool silent)
{
  std::vector<int> cleanedIDs;
  if (mediaType.empty() || cleanableFileIDs.empty())
    return cleanedIDs;

  std::string table = mediaType;
  std::string idField;
  std::string parentPathIdField;
  bool isEpisode = false;
  if (mediaType == MediaTypeMovie)
  {
    idField = "idMovie";
    parentPathIdField = StringUtils::Format("%s.c%02d", table.c_str(), VIDEODB_ID_PARENTPATHID);
  }
  else if (mediaType == MediaTypeEpisode)
  {
    idField = "idEpisode";
    parentPathIdField = "showPath.idParentPath";
    isEpisode = true;
  }
  else if (mediaType == MediaTypeMusicVideo)
  {
    idField = "idMVideo";
    parentPathIdField = StringUtils::Format("%s.c%02d", table.c_str(), VIDEODB_ID_MUSICVIDEO_PARENTPATHID);
  }
  else
    return cleanedIDs;

  // now grab them media items
  std::string sql = PrepareSQL("SELECT %s.%s, %s.idFile, path.idPath, parentPath.strPath FROM %s "
                                 "JOIN files ON files.idFile = %s.idFile "
                                 "JOIN path ON path.idPath = files.idPath ",
                               table.c_str(), idField.c_str(), table.c_str(), table.c_str(),
                               table.c_str());

  if (isEpisode)
    sql += "JOIN tvshowlinkpath ON tvshowlinkpath.idShow = episode.idShow JOIN path AS showPath ON showPath.idPath=tvshowlinkpath.idPath ";

  sql += PrepareSQL("JOIN path as parentPath ON parentPath.idPath = %s "
                    "WHERE %s.idFile IN (%s)",
                    parentPathIdField.c_str(),
                    table.c_str(), cleanableFileIDs.c_str());

  // map of parent path ID to boolean pair (if not exists and user choice)
  std::map<int, std::pair<bool, bool> > sourcePathsDeleteDecisions;
  m_pDS2->query(sql.c_str());
  while (!m_pDS2->eof())
  {
    std::string parentPath = m_pDS2->fv(3).get_asString();

    // try to find the source path the parent path belongs to
    SScanSettings scanSettings;
    std::string sourcePath;
    GetSourcePath(parentPath, sourcePath, scanSettings);
    int sourcePathID = GetPathId(sourcePath);
    std::map<int, std::pair<bool, bool> >::const_iterator sourcePathsDeleteDecision = sourcePathsDeleteDecisions.find(sourcePathID);
    bool del = true;
    if (sourcePathsDeleteDecision == sourcePathsDeleteDecisions.end())
    {
      bool sourcePathNotExists = !CDirectory::Exists(sourcePath, false);
      // if the parent path exists, the file will be deleted without asking
      // if the parent path doesn't exist, ask the user whether to remove all items it contained
      if (sourcePathNotExists)
      {
        // in silent mode assume that the files are just temporarily missing
        if (silent)
          del = false;
        else
        {
          CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*)g_windowManager.GetWindow(WINDOW_DIALOG_YES_NO);
          if (pDialog != NULL)
          {
            CURL sourceUrl(sourcePath);
            pDialog->SetHeading(15012);
            pDialog->SetText(StringUtils::Format(g_localizeStrings.Get(15013).c_str(), sourceUrl.GetWithoutUserDetails().c_str()));
            pDialog->SetChoice(0, 15015);
            pDialog->SetChoice(1, 15014);

            //send message and wait for user input
            ThreadMessage tMsg = { TMSG_DIALOG_DOMODAL, WINDOW_DIALOG_YES_NO, g_windowManager.GetActiveWindow() };
            CApplicationMessenger::Get().SendMessage(tMsg, true);

            del = !pDialog->IsConfirmed();
          }
        }
      }

      sourcePathsDeleteDecisions.insert(make_pair(sourcePathID, make_pair(sourcePathNotExists, del)));
      pathsDeleteDecisions.insert(make_pair(sourcePathID, sourcePathNotExists && del));
    }
    // the only reason not to delete the file is if the parent path doesn't
    // exist and the user decided to delete all the items it contained
    else if (sourcePathsDeleteDecision->second.first &&
             !sourcePathsDeleteDecision->second.second)
      del = false;

    if (scanSettings.parent_name)
      pathsDeleteDecisions.insert(make_pair(m_pDS2->fv(2).get_asInt(), del));

    if (del)
    {
      deletedFileIDs += m_pDS2->fv(1).get_asString() + ",";
      cleanedIDs.push_back(m_pDS2->fv(0).get_asInt());
    }

    m_pDS2->next();
  }
  m_pDS2->close();

  return cleanedIDs;
}

void CVideoDatabase::DumpToDummyFiles(const CStdString &path)
{
  // get all tvshows
  CFileItemList items;
  GetTvShowsByWhere("videodb://tvshows/titles/", "", items);
  CStdString showPath = URIUtils::AddFileToFolder(path, "shows");
  CDirectory::Create(showPath);
  for (int i = 0; i < items.Size(); i++)
  {
    // create a folder in this directory
    CStdString showName = CUtil::MakeLegalFileName(items[i]->GetVideoInfoTag()->m_strShowTitle);
    CStdString TVFolder = URIUtils::AddFileToFolder(showPath, showName);
    if (CDirectory::Create(TVFolder))
    { // right - grab the episodes and dump them as well
      CFileItemList episodes;
      Filter filter(PrepareSQL("idShow=%i", items[i]->GetVideoInfoTag()->m_iDbId));
      GetEpisodesByWhere("videodb://tvshows/titles/", filter, episodes);
      for (int i = 0; i < episodes.Size(); i++)
      {
        CVideoInfoTag *tag = episodes[i]->GetVideoInfoTag();
        CStdString episode = StringUtils::Format("%s.s%02de%02d.avi", showName.c_str(), tag->m_iSeason, tag->m_iEpisode);
        // and make a file
        CStdString episodePath = URIUtils::AddFileToFolder(TVFolder, episode);
        CFile file;
        if (file.OpenForWrite(episodePath))
          file.Close();
      }
    }
  }
  // get all movies
  items.Clear();
  GetMoviesByWhere("videodb://movies/titles/", "", items);
  CStdString moviePath = URIUtils::AddFileToFolder(path, "movies");
  CDirectory::Create(moviePath);
  for (int i = 0; i < items.Size(); i++)
  {
    CVideoInfoTag *tag = items[i]->GetVideoInfoTag();
    CStdString movie = StringUtils::Format("%s.avi", tag->m_strTitle.c_str());
    CFile file;
    if (file.OpenForWrite(URIUtils::AddFileToFolder(moviePath, movie)))
      file.Close();
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
    CStdString exportRoot = URIUtils::AddFileToFolder(path, "xbmc_videodb_" + CDateTime::GetCurrentDateTime().GetAsDBDate());
    CStdString xmlFile = URIUtils::AddFileToFolder(exportRoot, "videodb.xml");
    CStdString actorsDir = URIUtils::AddFileToFolder(exportRoot, "actors");
    CStdString moviesDir = URIUtils::AddFileToFolder(exportRoot, "movies");
    CStdString musicvideosDir = URIUtils::AddFileToFolder(exportRoot, "musicvideos");
    CStdString tvshowsDir = URIUtils::AddFileToFolder(exportRoot, "tvshows");
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
    CXBMCTinyXML xmlDoc;
    TiXmlDeclaration decl("1.0", "UTF-8", "yes");
    xmlDoc.InsertEndChild(decl);
    TiXmlNode *pMain = NULL;
    if (singleFiles)
      pMain = &xmlDoc;
    else
    {
      TiXmlElement xmlMainElement("videodb");
      pMain = xmlDoc.InsertEndChild(xmlMainElement);
      XMLUtils::SetInt(pMain,"version", GetExportVersion());
    }

    while (!m_pDS->eof())
    {
      CVideoInfoTag movie = GetDetailsForMovie(m_pDS, true);
      // strip paths to make them relative
      if (StringUtils::StartsWith(movie.m_strTrailer, movie.m_strPath))
        movie.m_strTrailer = movie.m_strTrailer.substr(movie.m_strPath.size());
      map<string, string> artwork;
      if (GetArtForItem(movie.m_iDbId, movie.m_type, artwork) && !singleFiles)
      {
        TiXmlElement additionalNode("art");
        for (map<string, string>::const_iterator i = artwork.begin(); i != artwork.end(); ++i)
          XMLUtils::SetString(&additionalNode, i->first.c_str(), i->second);
        movie.Save(pMain, "movie", true, &additionalNode);
      }
      else
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
      if (singleFiles && CUtil::SupportsWriteFileOperations(movie.m_strFileNameAndPath))
      {
        if (!item.Exists(false))
        {
          CLog::Log(LOGDEBUG, "%s - Not exporting item %s as it does not exist", __FUNCTION__, movie.m_strFileNameAndPath.c_str());
          bSkip = true;
        }
        else
        {
          CStdString nfoFile(URIUtils::ReplaceExtension(item.GetTBNFile(), ".nfo"));

          if (item.IsOpticalMediaFile())
          {
            nfoFile = URIUtils::AddFileToFolder(
                                    URIUtils::GetParentPath(nfoFile),
                                    URIUtils::GetFileName(nfoFile));
          }

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
        }
      }
      if (singleFiles)
      {
        xmlDoc.Clear();
        TiXmlDeclaration decl("1.0", "UTF-8", "yes");
        xmlDoc.InsertEndChild(decl);
      }

      if (images && !bSkip)
      {
        if (!singleFiles)
        {
          CStdString strFileName(movie.m_strTitle);
          if (movie.m_iYear > 0)
            strFileName += StringUtils::Format("_%i", movie.m_iYear);
          item.SetPath(GetSafeFile(moviesDir, strFileName) + ".avi");
        }
        for (map<string, string>::const_iterator i = artwork.begin(); i != artwork.end(); ++i)
        {
          CStdString savedThumb = item.GetLocalArt(i->first, false);
          CTextureCache::Get().Export(i->second, savedThumb, overwrite);
        }
        if (actorThumbs)
          ExportActorThumbs(actorsDir, movie, singleFiles, overwrite);
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
      CVideoInfoTag movie = GetDetailsForMusicVideo(m_pDS, true);
      map<string, string> artwork;
      if (GetArtForItem(movie.m_iDbId, movie.m_type, artwork) && !singleFiles)
      {
        TiXmlElement additionalNode("art");
        for (map<string, string>::const_iterator i = artwork.begin(); i != artwork.end(); ++i)
          XMLUtils::SetString(&additionalNode, i->first.c_str(), i->second);
        movie.Save(pMain, "musicvideo", true, &additionalNode);
      }
      else
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
      if (singleFiles && CUtil::SupportsWriteFileOperations(movie.m_strFileNameAndPath))
      {
        if (!item.Exists(false))
        {
          CLog::Log(LOGDEBUG, "%s - Not exporting item %s as it does not exist", __FUNCTION__, movie.m_strFileNameAndPath.c_str());
          bSkip = true;
        }
        else
        {
          CStdString nfoFile(URIUtils::ReplaceExtension(item.GetTBNFile(), ".nfo"));

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
        }
      }
      if (singleFiles)
      {
        xmlDoc.Clear();
        TiXmlDeclaration decl("1.0", "UTF-8", "yes");
        xmlDoc.InsertEndChild(decl);
      }
      if (images && !bSkip)
      {
        if (!singleFiles)
        {
          CStdString strFileName((CStdString)StringUtils::Join(movie.m_artist, g_advancedSettings.m_videoItemSeparator) + "." + movie.m_strTitle);
          if (movie.m_iYear > 0)
            strFileName += StringUtils::Format("_%i", movie.m_iYear);
          item.SetPath(GetSafeFile(moviesDir, strFileName) + ".avi");
        }
        for (map<string, string>::const_iterator i = artwork.begin(); i != artwork.end(); ++i)
        {
          CStdString savedThumb = item.GetLocalArt(i->first, false);
          CTextureCache::Get().Export(i->second, savedThumb, overwrite);
        }
      }
      m_pDS->next();
      current++;
    }
    m_pDS->close();

    // repeat for all tvshows
    sql = "SELECT * FROM tvshowview GROUP BY tvshowview.idShow";
    m_pDS->query(sql.c_str());

    total = m_pDS->num_rows();
    current = 0;

    while (!m_pDS->eof())
    {
      CVideoInfoTag tvshow = GetDetailsForTvShow(m_pDS, true);

      map<int, map<string, string> > seasonArt;
      GetTvShowSeasonArt(tvshow.m_iDbId, seasonArt);

      map<string, string> artwork;
      if (GetArtForItem(tvshow.m_iDbId, tvshow.m_type, artwork) && !singleFiles)
      {
        TiXmlElement additionalNode("art");
        for (map<string, string>::const_iterator i = artwork.begin(); i != artwork.end(); ++i)
          XMLUtils::SetString(&additionalNode, i->first.c_str(), i->second);
        for (map<int, map<string, string> >::const_iterator i = seasonArt.begin(); i != seasonArt.end(); ++i)
        {
          TiXmlElement seasonNode("season");
          seasonNode.SetAttribute("num", i->first);
          for (map<string, string>::const_iterator j = i->second.begin(); j != i->second.end(); ++j)
            XMLUtils::SetString(&seasonNode, j->first.c_str(), j->second);
          additionalNode.InsertEndChild(seasonNode);
        }
        tvshow.Save(pMain, "tvshow", true, &additionalNode);
      }
      else
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
      if (singleFiles && CUtil::SupportsWriteFileOperations(tvshow.m_strPath))
      {
        if (!item.Exists(false))
        {
          CLog::Log(LOGDEBUG, "%s - Not exporting item %s as it does not exist", __FUNCTION__, tvshow.m_strPath.c_str());
          bSkip = true;
        }
        else
        {
          CStdString nfoFile = URIUtils::AddFileToFolder(tvshow.m_strPath, "tvshow.nfo");

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
        }
      }
      if (singleFiles)
      {
        xmlDoc.Clear();
        TiXmlDeclaration decl("1.0", "UTF-8", "yes");
        xmlDoc.InsertEndChild(decl);
      }
      if (images && !bSkip)
      {
        if (!singleFiles)
          item.SetPath(GetSafeFile(tvshowsDir, tvshow.m_strTitle));

        for (map<string, string>::const_iterator i = artwork.begin(); i != artwork.end(); ++i)
        {
          CStdString savedThumb = item.GetLocalArt(i->first, true);
          CTextureCache::Get().Export(i->second, savedThumb, overwrite);
        }

        if (actorThumbs)
          ExportActorThumbs(actorsDir, tvshow, singleFiles, overwrite);

        // export season thumbs
        for (map<int, map<string, string> >::const_iterator i = seasonArt.begin(); i != seasonArt.end(); ++i)
        {
          string seasonThumb;
          if (i->first == -1)
            seasonThumb = "season-all";
          else if (i->first == 0)
            seasonThumb = "season-specials";
          else
            seasonThumb = StringUtils::Format("season%02i", i->first);
          for (map<string, string>::const_iterator j = i->second.begin(); j != i->second.end(); j++)
          {
            CStdString savedThumb(item.GetLocalArt(seasonThumb + "-" + j->first, true));
            if (!i->second.empty())
              CTextureCache::Get().Export(j->second, savedThumb, overwrite);
          }
        }
      }

      // now save the episodes from this show
      sql = PrepareSQL("select * from episodeview where idShow=%i order by strFileName, idEpisode",tvshow.m_iDbId);
      pDS->query(sql.c_str());
      CStdString showDir(item.GetPath());

      while (!pDS->eof())
      {
        CVideoInfoTag episode = GetDetailsForEpisode(pDS, true);
        map<string, string> artwork;
        if (GetArtForItem(episode.m_iDbId, MediaTypeEpisode, artwork) && !singleFiles)
        {
          TiXmlElement additionalNode("art");
          for (map<string, string>::const_iterator i = artwork.begin(); i != artwork.end(); ++i)
            XMLUtils::SetString(&additionalNode, i->first.c_str(), i->second);
          episode.Save(pMain->LastChild(), "episodedetails", true, &additionalNode);
        }
        else if (!singleFiles)
          episode.Save(pMain->LastChild(), "episodedetails", !singleFiles);
        else
          episode.Save(pMain, "episodedetails", !singleFiles);
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
        if (singleFiles && CUtil::SupportsWriteFileOperations(episode.m_strFileNameAndPath))
        {
          if (!item.Exists(false))
          {
            CLog::Log(LOGDEBUG, "%s - Not exporting item %s as it does not exist", __FUNCTION__, episode.m_strFileNameAndPath.c_str());
            bSkip = true;
          }
          else
          {
            CStdString nfoFile(URIUtils::ReplaceExtension(item.GetTBNFile(), ".nfo"));

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
          }
        }
        if (singleFiles)
        {
          xmlDoc.Clear();
          TiXmlDeclaration decl("1.0", "UTF-8", "yes");
          xmlDoc.InsertEndChild(decl);
        }

        if (images && !bSkip)
        {
          if (!singleFiles)
          {
            CStdString epName = StringUtils::Format("s%02ie%02i.avi", episode.m_iSeason, episode.m_iEpisode);
            item.SetPath(URIUtils::AddFileToFolder(showDir, epName));
          }
          for (map<string, string>::const_iterator i = artwork.begin(); i != artwork.end(); ++i)
          {
            CStdString savedThumb = item.GetLocalArt(i->first, false);
            CTextureCache::Get().Export(i->second, savedThumb, overwrite);
          }
          if (actorThumbs)
            ExportActorThumbs(actorsDir, episode, singleFiles, overwrite);
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
      set<CStdString> paths;
      GetPaths(paths);
      TiXmlElement xmlPathElement("paths");
      TiXmlNode *pPaths = pMain->InsertEndChild(xmlPathElement);
      for( set<CStdString>::iterator iter = paths.begin(); iter != paths.end(); ++iter)
      {
        bool foundDirectly = false;
        SScanSettings settings;
        ScraperPtr info = GetScraperForPath(*iter, settings, foundDirectly);
        if (info && foundDirectly)
        {
          TiXmlElement xmlPathElement2("path");
          TiXmlNode *pPath = pPaths->InsertEndChild(xmlPathElement2);
          XMLUtils::SetString(pPath,"url", *iter);
          XMLUtils::SetInt(pPath,"scanrecursive", settings.recurse);
          XMLUtils::SetBoolean(pPath,"usefoldernames", settings.parent_name);
          XMLUtils::SetString(pPath,"content", TranslateContent(info->Content()));
          XMLUtils::SetString(pPath,"scraperpath", info->ID());
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

void CVideoDatabase::ExportActorThumbs(const CStdString &strDir, const CVideoInfoTag &tag, bool singleFiles, bool overwrite /*=false*/)
{
  CStdString strPath(strDir);
  if (singleFiles)
  {
    strPath = URIUtils::AddFileToFolder(tag.m_strPath, ".actors");
    if (!CDirectory::Exists(strPath))
    {
      CDirectory::Create(strPath);
      CFile::SetHidden(strPath, true);
    }
  }

  for (CVideoInfoTag::iCast iter = tag.m_cast.begin();iter != tag.m_cast.end();++iter)
  {
    CFileItem item;
    item.SetLabel(iter->strName);
    if (!iter->thumb.empty())
    {
      CStdString thumbFile(GetSafeFile(strPath, iter->strName));
      CTextureCache::Get().Export(iter->thumb, thumbFile, overwrite);
    }
  }
}

bool CVideoDatabase::ExportSkipEntry(const CStdString &nfoFile)
{
  CStdString strParent;
  URIUtils::GetParentPath(nfoFile,strParent);
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

    CXBMCTinyXML xmlDoc;
    if (!xmlDoc.LoadFile(URIUtils::AddFileToFolder(path, "videodb.xml")))
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

    int iVersion = 0;
    XMLUtils::GetInt(root, "version", iVersion);

    CLog::Log(LOGDEBUG, "%s: Starting import (export version = %i)", __FUNCTION__, iVersion);

    TiXmlElement *movie = root->FirstChildElement();
    int current = 0;
    int total = 0;
    // first count the number of items...
    while (movie)
    {
      if (strnicmp(movie->Value(), MediaTypeMovie, 5)==0 ||
          strnicmp(movie->Value(), MediaTypeTvShow, 6)==0 ||
          strnicmp(movie->Value(), MediaTypeMusicVideo,10)==0 )
        total++;
      movie = movie->NextSiblingElement();
    }

    CStdString actorsDir(URIUtils::AddFileToFolder(path, "actors"));
    CStdString moviesDir(URIUtils::AddFileToFolder(path, "movies"));
    CStdString musicvideosDir(URIUtils::AddFileToFolder(path, "musicvideos"));
    CStdString tvshowsDir(URIUtils::AddFileToFolder(path, "tvshows"));
    CVideoInfoScanner scanner;
    // add paths first (so we have scraper settings available)
    TiXmlElement *path = root->FirstChildElement("paths");
    path = path->FirstChildElement();
    while (path)
    {
      CStdString strPath;
      if (XMLUtils::GetString(path,"url",strPath) && !strPath.empty())
        AddPath(strPath);

      CStdString content;
      if (XMLUtils::GetString(path,"content", content) && !content.empty())
      { // check the scraper exists, if so store the path
        AddonPtr addon;
        CStdString id;
        XMLUtils::GetString(path,"scraperpath",id);
        if (CAddonMgr::Get().GetAddon(id, addon))
        {
          SScanSettings settings;
          ScraperPtr scraper = boost::dynamic_pointer_cast<CScraper>(addon);
          // FIXME: scraper settings are not exported?
          scraper->SetPathSettings(TranslateContent(content), "");
          XMLUtils::GetInt(path,"scanrecursive",settings.recurse);
          XMLUtils::GetBoolean(path,"usefoldernames",settings.parent_name);
          SetScraperForPath(strPath,scraper,settings);
        }
      }
      path = path->NextSiblingElement();
    }
    movie = root->FirstChildElement();
    while (movie)
    {
      CVideoInfoTag info;
      if (strnicmp(movie->Value(), MediaTypeMovie, 5) == 0)
      {
        info.Load(movie);
        CFileItem item(info);
        bool useFolders = info.m_basePath.empty() ? LookupByFolders(item.GetPath()) : false;
        CStdString filename = info.m_strTitle;
        if (info.m_iYear > 0)
          filename += StringUtils::Format("_%i", info.m_iYear);
        CFileItem artItem(item);
        artItem.SetPath(GetSafeFile(moviesDir, filename) + ".avi");
        scanner.GetArtwork(&artItem, CONTENT_MOVIES, useFolders, true, actorsDir);
        item.SetArt(artItem.GetArt());
        scanner.AddVideo(&item, CONTENT_MOVIES, useFolders, true, NULL, true);
        current++;
      }
      else if (strnicmp(movie->Value(), MediaTypeMusicVideo, 10) == 0)
      {
        info.Load(movie);
        CFileItem item(info);
        bool useFolders = info.m_basePath.empty() ? LookupByFolders(item.GetPath()) : false;
        CStdString filename = (CStdString)StringUtils::Join(info.m_artist, g_advancedSettings.m_videoItemSeparator) + "." + info.m_strTitle;
        if (info.m_iYear > 0)
          filename += StringUtils::Format("_%i", info.m_iYear);
        CFileItem artItem(item);
        artItem.SetPath(GetSafeFile(musicvideosDir, filename) + ".avi");
        scanner.GetArtwork(&artItem, CONTENT_MUSICVIDEOS, useFolders, true, actorsDir);
        item.SetArt(artItem.GetArt());
        scanner.AddVideo(&item, CONTENT_MUSICVIDEOS, useFolders, true, NULL, true);
        current++;
      }
      else if (strnicmp(movie->Value(), MediaTypeTvShow, 6) == 0)
      {
        // load the TV show in.  NOTE: This deletes all episodes under the TV Show, which may not be
        // what we desire.  It may make better sense to only delete (or even better, update) the show information
        info.Load(movie);
        URIUtils::AddSlashAtEnd(info.m_strPath);
        DeleteTvShow(info.m_strPath);
        CFileItem showItem(info);
        bool useFolders = info.m_basePath.empty() ? LookupByFolders(showItem.GetPath(), true) : false;
        CFileItem artItem(showItem);
        CStdString artPath(GetSafeFile(tvshowsDir, info.m_strTitle));
        artItem.SetPath(artPath);
        scanner.GetArtwork(&artItem, CONTENT_TVSHOWS, useFolders, true, actorsDir);
        showItem.SetArt(artItem.GetArt());
        int showID = scanner.AddVideo(&showItem, CONTENT_TVSHOWS, useFolders, true, NULL, true);
        // season artwork
        map<int, map<string, string> > seasonArt;
        artItem.GetVideoInfoTag()->m_strPath = artPath;
        scanner.GetSeasonThumbs(*artItem.GetVideoInfoTag(), seasonArt, CVideoThumbLoader::GetArtTypes(MediaTypeSeason), true);
        for (map<int, map<string, string> >::iterator i = seasonArt.begin(); i != seasonArt.end(); ++i)
        {
          int seasonID = AddSeason(showID, i->first);
          SetArtForItem(seasonID, MediaTypeSeason, i->second);
        }
        current++;
        // now load the episodes
        TiXmlElement *episode = movie->FirstChildElement("episodedetails");
        while (episode)
        {
          // no need to delete the episode info, due to the above deletion
          CVideoInfoTag info;
          info.Load(episode);
          CFileItem item(info);
          CStdString filename = StringUtils::Format("s%02ie%02i.avi", info.m_iSeason, info.m_iEpisode);
          CFileItem artItem(item);
          artItem.SetPath(GetSafeFile(artPath, filename));
          scanner.GetArtwork(&artItem, CONTENT_TVSHOWS, useFolders, true, actorsDir);
          item.SetArt(artItem.GetArt());
          scanner.AddVideo(&item,CONTENT_TVSHOWS, false, false, showItem.GetVideoInfoTag(), true);
          episode = episode->NextSiblingElement("episodedetails");
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
          return;
        }
      }
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  if (progress)
    progress->Close();
}

bool CVideoDatabase::ImportArtFromXML(const TiXmlNode *node, map<string, string> &artwork)
{
  if (!node) return false;
  const TiXmlNode *art = node->FirstChild();
  while (art && art->FirstChild())
  {
    artwork.insert(make_pair(art->ValueStr(), art->FirstChild()->ValueStr()));
    art = art->NextSibling();
  }
  return !artwork.empty();
}

void CVideoDatabase::ConstructPath(CStdString& strDest, const CStdString& strPath, const CStdString& strFileName)
{
  if (URIUtils::IsStack(strFileName) || 
      URIUtils::IsInArchive(strFileName) || URIUtils::IsPlugin(strPath))
    strDest = strFileName;
  else
    strDest = URIUtils::AddFileToFolder(strPath, strFileName);
}

void CVideoDatabase::SplitPath(const CStdString& strFileNameAndPath, CStdString& strPath, CStdString& strFileName)
{
  if (URIUtils::IsStack(strFileNameAndPath) || StringUtils::StartsWithNoCase(strFileNameAndPath, "rar://") || StringUtils::StartsWithNoCase(strFileNameAndPath, "zip://"))
  {
    URIUtils::GetParentPath(strFileNameAndPath,strPath);
    strFileName = strFileNameAndPath;
  }
  else if (URIUtils::IsPlugin(strFileNameAndPath))
  {
    CURL url(strFileNameAndPath);
    strPath = url.GetWithoutFilename();
    strFileName = strFileNameAndPath;
  }
  else
    URIUtils::Split(strFileNameAndPath,strPath, strFileName);
}

void CVideoDatabase::InvalidatePathHash(const CStdString& strPath)
{
  SScanSettings settings;
  bool foundDirectly;
  ScraperPtr info = GetScraperForPath(strPath,settings,foundDirectly);
  SetPathHash(strPath,"");
  if (!info)
    return;
  if (info->Content() == CONTENT_TVSHOWS || (info->Content() == CONTENT_MOVIES && !foundDirectly)) // if we scan by folder name we need to invalidate parent as well
  {
    if (info->Content() == CONTENT_TVSHOWS || settings.parent_name_root)
    {
      CStdString strParent;
      URIUtils::GetParentPath(strPath,strParent);
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

bool CVideoDatabase::SetSingleValue(VIDEODB_CONTENT_TYPE type, int dbId, int dbField, const std::string &strValue)
{
  string strSQL;
  try
  {
    if (NULL == m_pDB.get() || NULL == m_pDS.get())
      return false;

    string strTable, strField;
    if (type == VIDEODB_CONTENT_MOVIES)
    {
      strTable = "movie";
      strField = "idMovie";
    }
    else if (type == VIDEODB_CONTENT_TVSHOWS)
    {
      strTable = "tvshow";
      strField = "idShow";
    }
    else if (type == VIDEODB_CONTENT_EPISODES)
    {
      strTable = "episode";
      strField = "idEpisode";
    }
    else if (type == VIDEODB_CONTENT_MUSICVIDEOS)
    {
      strTable = "musicvideo";
      strField = "idMVideo";
    }

    if (strTable.empty())
      return false;

    return SetSingleValue(strTable, StringUtils::Format("c%02u", dbField), strValue, strField, dbId);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strSQL.c_str());
  }
  return false;
}

bool CVideoDatabase::SetSingleValue(VIDEODB_CONTENT_TYPE type, int dbId, Field dbField, const std::string &strValue)
{
  MediaType mediaType = DatabaseUtils::MediaTypeFromVideoContentType(type);
  if (mediaType == MediaTypeNone)
    return false;

  int dbFieldIndex = DatabaseUtils::GetField(dbField, mediaType);
  if (dbFieldIndex < 0)
    return false;

  return SetSingleValue(type, dbId, dbFieldIndex, strValue);
}

bool CVideoDatabase::SetSingleValue(const std::string &table, const std::string &fieldName, const std::string &strValue,
                                    const std::string &conditionName /* = "" */, int conditionValue /* = -1 */)
{
  if (table.empty() || fieldName.empty())
    return false;

  std::string sql;
  try
  {
    if (NULL == m_pDB.get() || NULL == m_pDS.get())
      return false;

    sql = PrepareSQL("UPDATE %s SET %s='%s'", table.c_str(), fieldName.c_str(), strValue.c_str());
    if (!conditionName.empty())
      sql += PrepareSQL(" WHERE %s=%u", conditionName.c_str(), conditionValue);
    if (m_pDS->exec(sql.c_str()) == 0)
      return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, sql.c_str());
  }
  return false;
}

CStdString CVideoDatabase::GetSafeFile(const CStdString &dir, const CStdString &name) const
{
  CStdString safeThumb(name);
  StringUtils::Replace(safeThumb, ' ', '_');
  return URIUtils::AddFileToFolder(dir, CUtil::MakeLegalFileName(safeThumb));
}

void CVideoDatabase::AnnounceRemove(std::string content, int id, bool scanning /* = false */)
{
  CVariant data;
  data["type"] = content;
  data["id"] = id;
  if (scanning)
    data["transaction"] = true;
  ANNOUNCEMENT::CAnnouncementManager::Get().Announce(ANNOUNCEMENT::VideoLibrary, "xbmc", "OnRemove", data);
}

void CVideoDatabase::AnnounceUpdate(std::string content, int id)
{
  CVariant data;
  data["type"] = content;
  data["id"] = id;
  ANNOUNCEMENT::CAnnouncementManager::Get().Announce(ANNOUNCEMENT::VideoLibrary, "xbmc", "OnUpdate", data);
}

bool CVideoDatabase::GetItemsForPath(const CStdString &content, const CStdString &strPath, CFileItemList &items)
{
  CStdString path(strPath);
  
  if(URIUtils::IsMultiPath(path))
  {
    vector<std::string> paths;
    CMultiPathDirectory::GetPaths(path, paths);

    for(unsigned i=0;i<paths.size();i++)
      GetItemsForPath(content, paths[i], items);

    return items.Size() > 0;
  }
  
  int pathID = GetPathId(path);
  if (pathID < 0)
    return false;

  if (content == "movies")
  {
    Filter filter(PrepareSQL("c%02d=%d", VIDEODB_ID_PARENTPATHID, pathID));
    GetMoviesByWhere("videodb://movies/titles/", filter, items);
  }
  else if (content == "episodes")
  {
    Filter filter(PrepareSQL("c%02d=%d", VIDEODB_ID_EPISODE_PARENTPATHID, pathID));
    GetEpisodesByWhere("videodb://tvshows/titles/", filter, items);
  }
  else if (content == "tvshows")
  {
    Filter filter(PrepareSQL("idParentPath=%d", pathID));
    GetTvShowsByWhere("videodb://tvshows/titles/", filter, items);
  }
  else if (content == "musicvideos")
  {
    Filter filter(PrepareSQL("c%02d=%d", VIDEODB_ID_MUSICVIDEO_PARENTPATHID, pathID));
    GetMusicVideosByWhere("videodb://musicvideos/titles/", filter, items);
  }
  for (int i = 0; i < items.Size(); i++)
    items[i]->SetPath(items[i]->GetVideoInfoTag()->m_basePath);
  return items.Size() > 0;
}

bool CVideoDatabase::GetFilter(CDbUrl &videoUrl, Filter &filter, SortDescription &sorting)
{
  if (!videoUrl.IsValid())
    return false;

  std::string type = videoUrl.GetType();
  std::string itemType = ((const CVideoDbUrl &)videoUrl).GetItemType();
  const CUrlOptions::UrlOptions& options = videoUrl.GetOptions();
  CUrlOptions::UrlOptions::const_iterator option;

  if (type == "movies")
  {
    option = options.find("genreid");
    if (option != options.end())
    {
      filter.AppendJoin(PrepareSQL("join genrelinkmovie on genrelinkmovie.idMovie = movieview.idMovie"));
      filter.AppendWhere(PrepareSQL("genrelinkmovie.idGenre = %i", (int)option->second.asInteger()));
    }

    option = options.find("genre");
    if (option != options.end())
    {
      filter.AppendJoin(PrepareSQL("join genrelinkmovie on genrelinkmovie.idMovie = movieview.idMovie join genre on genre.idGenre = genrelinkmovie.idGenre"));
      filter.AppendWhere(PrepareSQL("genre.strGenre like '%s'", option->second.asString().c_str()));
    }

    option = options.find("countryid");
    if (option != options.end())
    {
      filter.AppendJoin(PrepareSQL("join countrylinkmovie on countrylinkmovie.idMovie = movieview.idMovie"));
      filter.AppendWhere(PrepareSQL("countrylinkmovie.idCountry = %i", (int)option->second.asInteger()));
    }

    option = options.find("country");
    if (option != options.end())
    {
      filter.AppendJoin(PrepareSQL("join countrylinkmovie on countrylinkmovie.idMovie = movieview.idMovie join country on country.idCountry = countrylinkmovie.idCountry"));
      filter.AppendWhere(PrepareSQL("country.strCountry like '%s'", option->second.asString().c_str()));
    }

    option = options.find("studioid");
    if (option != options.end())
    {
      filter.AppendJoin(PrepareSQL("join studiolinkmovie on studiolinkmovie.idMovie = movieview.idMovie"));
      filter.AppendWhere(PrepareSQL("studiolinkmovie.idStudio = %i", (int)option->second.asInteger()));
    }

    option = options.find("studio");
    if (option != options.end())
    {
      filter.AppendJoin(PrepareSQL("join studiolinkmovie on studiolinkmovie.idMovie = movieview.idMovie join studio on studio.idStudio = studiolinkmovie.idStudio"));
      filter.AppendWhere(PrepareSQL("studio.strStudio like '%s'", option->second.asString().c_str()));
    }

    option = options.find("directorid");
    if (option != options.end())
    {
      filter.AppendJoin(PrepareSQL("join directorlinkmovie on directorlinkmovie.idMovie = movieview.idMovie"));
      filter.AppendWhere(PrepareSQL("directorlinkmovie.idDirector = %i", (int)option->second.asInteger()));
    }

    option = options.find("director");
    if (option != options.end())
    {
      filter.AppendJoin(PrepareSQL("join directorlinkmovie on directorlinkmovie.idMovie = movieview.idMovie join actors on actors.idActor = directorlinkmovie.idDirector"));
      filter.AppendWhere(PrepareSQL("actors.strActor like '%s'", option->second.asString().c_str()));
    }

    option = options.find("year");
    if (option != options.end())
      filter.AppendWhere(PrepareSQL("movieview.c%02d = '%i'", VIDEODB_ID_YEAR, (int)option->second.asInteger()));

    option = options.find("actorid");
    if (option != options.end())
    {
      filter.AppendJoin(PrepareSQL("join actorlinkmovie on actorlinkmovie.idMovie = movieview.idMovie"));
      filter.AppendWhere(PrepareSQL("actorlinkmovie.idActor = %i", (int)option->second.asInteger()));
    }

    option = options.find("actor");
    if (option != options.end())
    {
      filter.AppendJoin(PrepareSQL("join actorlinkmovie on actorlinkmovie.idMovie = movieview.idMovie join actors on actors.idActor = actorlinkmovie.idActor"));
      filter.AppendWhere(PrepareSQL("actors.strActor like '%s'", option->second.asString().c_str()));
    }

    option = options.find("setid");
    if (option != options.end())
      filter.AppendWhere(PrepareSQL("movieview.idSet = %i", (int)option->second.asInteger()));

    option = options.find("set");
    if (option != options.end())
    {
      filter.AppendJoin(PrepareSQL("join setlinkmovie on setlinkmovie.idMovie = movieview.idMovie join sets on sets.idSet = setlinkmovie.idSet"));
      filter.AppendWhere(PrepareSQL("sets.strSet like '%s'", option->second.asString().c_str()));
    }

    option = options.find("tagid");
    if (option != options.end())
    {
      filter.AppendJoin(PrepareSQL("join taglinks on taglinks.idMedia = movieview.idMovie AND taglinks.media_type = 'movie'"));
      filter.AppendWhere(PrepareSQL("taglinks.idTag = %i", (int)option->second.asInteger()));
    }

    option = options.find("tag");
    if (option != options.end())
    {
      filter.AppendJoin(PrepareSQL("join taglinks on taglinks.idMedia = movieview.idMovie AND taglinks.media_type = 'movie' join tag on tag.idTag = taglinks.idTag"));
      filter.AppendWhere(PrepareSQL("tag.strTag like '%s'", option->second.asString().c_str()));
    }
  }
  else if (type == "tvshows")
  {
    if (itemType == "tvshows")
    {
      option = options.find("genreid");
      if (option != options.end())
      {
        filter.AppendJoin(PrepareSQL("join genrelinktvshow on genrelinktvshow.idShow = tvshowview.idShow"));
        filter.AppendWhere(PrepareSQL("genrelinktvshow.idGenre = %i", (int)option->second.asInteger()));
      }

      option = options.find("genre");
      if (option != options.end())
      {
        filter.AppendJoin(PrepareSQL("join genrelinktvshow on genrelinktvshow.idShow = tvshowview.idShow join genre on genre.idGenre = genrelinktvshow.idGenre"));
        filter.AppendWhere(PrepareSQL("genre.strGenre like '%s'", option->second.asString().c_str()));
      }

      option = options.find("studioid");
      if (option != options.end())
      {
        filter.AppendJoin(PrepareSQL("join studiolinktvshow on studiolinktvshow.idShow = tvshowview.idShow"));
        filter.AppendWhere(PrepareSQL("studiolinktvshow.idStudio = %i", (int)option->second.asInteger()));
      }

      option = options.find("studio");
      if (option != options.end())
      {
        filter.AppendJoin(PrepareSQL("join studiolinktvshow on studiolinktvshow.idShow = tvshowview.idShow join studio on studio.idStudio = studiolinktvshow.idStudio"));
        filter.AppendWhere(PrepareSQL("studio.strStudio like '%s'", option->second.asString().c_str()));
      }

      option = options.find("directorid");
      if (option != options.end())
      {
        filter.AppendJoin(PrepareSQL("join directorlinktvshow on directorlinktvshow.idShow = tvshowview.idShow"));
        filter.AppendWhere(PrepareSQL("directorlinktvshow.idDirector = %i", (int)option->second.asInteger()));
      }

      option = options.find("year");
      if (option != options.end())
        filter.AppendWhere(PrepareSQL("tvshowview.c%02d like '%%%i%%'", VIDEODB_ID_TV_PREMIERED, (int)option->second.asInteger()));

      option = options.find("actorid");
      if (option != options.end())
      {
        filter.AppendJoin(PrepareSQL("join actorlinktvshow on actorlinktvshow.idShow = tvshowview.idShow"));
        filter.AppendWhere(PrepareSQL("actorlinktvshow.idActor = %i", (int)option->second.asInteger()));
      }

      option = options.find("actor");
      if (option != options.end())
      {
        filter.AppendJoin(PrepareSQL("join actorlinktvshow on actorlinktvshow.idShow = tvshowview.idShow join actors on actors.idActor = actorlinktvshow.idActor"));
        filter.AppendWhere(PrepareSQL("actors.strActor like '%s'", option->second.asString().c_str()));
      }

      option = options.find("tagid");
      if (option != options.end())
      {
        filter.AppendJoin(PrepareSQL("join taglinks on taglinks.idMedia = tvshowview.idShow AND taglinks.media_type = 'tvshow'"));
        filter.AppendWhere(PrepareSQL("taglinks.idTag = %i", (int)option->second.asInteger()));
      }

      option = options.find("tag");
      if (option != options.end())
      {
        filter.AppendJoin(PrepareSQL("join taglinks on taglinks.idMedia = tvshowview.idShow AND taglinks.media_type = 'tvshow' join tag on tag.idTag = taglinks.idTag"));
        filter.AppendWhere(PrepareSQL("tag.strTag like '%s'", option->second.asString().c_str()));
      }
    }
    else if (itemType == "seasons")
    {
      option = options.find("tvshowid");
      if (option != options.end())
        filter.AppendWhere(PrepareSQL("seasonview.idShow = %i", (int)option->second.asInteger()));

      option = options.find("genreid");
      if (option != options.end())
      {
        filter.AppendJoin(PrepareSQL("join genrelinktvshow on genrelinktvshow.idShow = seasonview.idShow"));
        filter.AppendWhere(PrepareSQL("genrelinktvshow.idGenre = %i", (int)option->second.asInteger()));
      }

      option = options.find("directorid");
      if (option != options.end())
      {
        filter.AppendJoin(PrepareSQL("join directorlinktvshow on directorlinktvshow.idShow = seasonview.idShow"));
        filter.AppendWhere(PrepareSQL("directorlinktvshow.idDirector = %i", (int)option->second.asInteger()));
      }
      
      option = options.find("year");
      if (option != options.end())
        filter.AppendWhere(PrepareSQL("seasonview.premiered like '%%%i%%'", (int)option->second.asInteger()));

      option = options.find("actorid");
      if (option != options.end())
      {
        filter.AppendJoin(PrepareSQL("join actorlinktvshow on actorlinktvshow.idShow = seasonview.idShow"));
        filter.AppendWhere(PrepareSQL("actorlinktvshow.idActor = %i", (int)option->second.asInteger()));
      }
    }
    else if (itemType == "episodes")
    {
      int idShow = -1;
      option = options.find("tvshowid");
      if (option != options.end())
        idShow = (int)option->second.asInteger();

      int season = -1;
      option = options.find("season");
      if (option != options.end())
        season = (int)option->second.asInteger();

      if (idShow > -1)
      {
        bool condition = false;

        option = options.find("genreid");
        if (option != options.end())
        {
          condition = true;
          filter.AppendJoin(PrepareSQL("join genrelinktvshow on genrelinktvshow.idShow = episodeview.idShow"));
          filter.AppendWhere(PrepareSQL("episodeview.idShow = %i and genrelinktvshow.idGenre = %i", idShow, (int)option->second.asInteger()));
        }

        option = options.find("genre");
        if (option != options.end())
        {
          condition = true;
          filter.AppendJoin(PrepareSQL("join genrelinktvshow on genrelinktvshow.idShow = episodeview.idShow join genre on genre.idGenre = genrelinktvshow.idGenre"));
          filter.AppendWhere(PrepareSQL("episodeview.idShow = %i and genre.strGenre like '%s'", idShow, option->second.asString().c_str()));
        }

        option = options.find("directorid");
        if (option != options.end())
        {
          condition = true;
          filter.AppendJoin(PrepareSQL("join directorlinktvshow on directorlinktvshow.idShow = episodeview.idShow"));
          filter.AppendWhere(PrepareSQL("episodeview.idShow = %i and directorlinktvshow.idDirector = %i", idShow, (int)option->second.asInteger()));
        }

        option = options.find("director");
        if (option != options.end())
        {
          condition = true;
          filter.AppendJoin(PrepareSQL("join directorlinktvshow on directorlinktvshow.idShow = episodeview.idShow join actors on actors.idActor = directorlinktvshow.idDirector"));
          filter.AppendWhere(PrepareSQL("episodeview.idShow = %i and actors.strActor like '%s'", idShow, option->second.asString().c_str()));
        }
      
        option = options.find("year");
        if (option != options.end())
        {
          condition = true;
          filter.AppendWhere(PrepareSQL("episodeview.idShow = %i and episodeview.premiered like '%%%i%%'", idShow, (int)option->second.asInteger()));
        }

        option = options.find("actorid");
        if (option != options.end())
        {
          condition = true;
          filter.AppendJoin(PrepareSQL("join actorlinktvshow on actorlinktvshow.idShow = episodeview.idShow"));
          filter.AppendWhere(PrepareSQL("episodeview.idShow = %i and actorlinktvshow.idActor = %i", idShow, (int)option->second.asInteger()));
        }

        option = options.find("actor");
        if (option != options.end())
        {
          condition = true;
          filter.AppendJoin(PrepareSQL("join actorlinktvshow on actorlinktvshow.idShow = episodeview.idShow join actors on actors.idActor = actorlinktvshow.idActor"));
          filter.AppendWhere(PrepareSQL("episodeview.idShow = %i and actors.strActor = '%s'", idShow, option->second.asString().c_str()));
        }

        if (!condition)
          filter.AppendWhere(PrepareSQL("episodeview.idShow = %i", idShow));

        if (season > -1)
        {
          if (season == 0) // season = 0 indicates a special - we grab all specials here (see below)
            filter.AppendWhere(PrepareSQL("episodeview.c%02d = %i", VIDEODB_ID_EPISODE_SEASON, season));
          else
            filter.AppendWhere(PrepareSQL("(episodeview.c%02d = %i or (episodeview.c%02d = 0 and (episodeview.c%02d = 0 or episodeview.c%02d = %i)))",
              VIDEODB_ID_EPISODE_SEASON, season, VIDEODB_ID_EPISODE_SEASON, VIDEODB_ID_EPISODE_SORTSEASON, VIDEODB_ID_EPISODE_SORTSEASON, season));
        }
      }
      else
      {
        option = options.find("year");
        if (option != options.end())
          filter.AppendWhere(PrepareSQL("episodeview.premiered like '%%%i%%'", (int)option->second.asInteger()));

        option = options.find("directorid");
        if (option != options.end())
        {
          filter.AppendJoin(PrepareSQL("join directorlinkepisode on directorlinkepisode.idEpisode = episodeview.idEpisode"));
          filter.AppendWhere(PrepareSQL("directorlinkepisode.idDirector = %i", (int)option->second.asInteger()));
        }

        option = options.find("director");
        if (option != options.end())
        {
          filter.AppendJoin(PrepareSQL("join directorlinkepisode on directorlinkepisode.idEpisode = episodeview.idEpisode join actors on actors.idActor = directorlinktvshow.idDirector"));
          filter.AppendWhere(PrepareSQL("actors.strActor = %s", option->second.asString().c_str()));
        }
      }
    }
  }
  else if (type == "musicvideos")
  {
    option = options.find("genreid");
    if (option != options.end())
    {
      filter.AppendJoin(PrepareSQL("join genrelinkmusicvideo on genrelinkmusicvideo.idMVideo = musicvideoview.idMVideo"));
      filter.AppendWhere(PrepareSQL("genrelinkmusicvideo.idGenre = %i", (int)option->second.asInteger()));
    }

    option = options.find("genre");
    if (option != options.end())
    {
      filter.AppendJoin(PrepareSQL("join genrelinkmusicvideo on genrelinkmusicvideo.idMVideo = musicvideoview.idMVideo join genre on genre.idGenre = genrelinkmusicvideo.idGenre"));
      filter.AppendWhere(PrepareSQL("genre.strGenre like '%s'", option->second.asString().c_str()));
    }

    option = options.find("studioid");
    if (option != options.end())
    {
      filter.AppendJoin(PrepareSQL("join studiolinkmusicvideo on studiolinkmusicvideo.idMVideo = musicvideoview.idMVideo"));
      filter.AppendWhere(PrepareSQL("studiolinkmusicvideo.idStudio = %i", (int)option->second.asInteger()));
    }

    option = options.find("studio");
    if (option != options.end())
    {
      filter.AppendJoin(PrepareSQL("join studiolinkmusicvideo on studiolinkmusicvideo.idMVideo = musicvideoview.idMVideo join studio on studio.idStudio = studiolinkmusicvideo.idStudio"));
      filter.AppendWhere(PrepareSQL("studio.strStudio like '%s'", option->second.asString().c_str()));
    }

    option = options.find("directorid");
    if (option != options.end())
    {
      filter.AppendJoin(PrepareSQL("join directorlinkmusicvideo on directorlinkmusicvideo.idMVideo = musicvideoview.idMVideo"));
      filter.AppendWhere(PrepareSQL("directorlinkmusicvideo.idDirector = %i", (int)option->second.asInteger()));
    }

    option = options.find("director");
    if (option != options.end())
    {
      filter.AppendJoin(PrepareSQL("join directorlinkmusicvideo on directorlinkmusicvideo.idMVideo = musicvideoview.idMVideo join actors on actors.idActor = directorlinkmusicvideo.idDirector"));
      filter.AppendWhere(PrepareSQL("actors.strActor like '%s'", option->second.asString().c_str()));
    }

    option = options.find("year");
    if (option != options.end())
      filter.AppendWhere(PrepareSQL("musicvideoview.c%02d = '%i'",VIDEODB_ID_MUSICVIDEO_YEAR, (int)option->second.asInteger()));

    option = options.find("artistid");
    if (option != options.end())
    {
      if (itemType != "albums")
        filter.AppendJoin(PrepareSQL("join artistlinkmusicvideo on artistlinkmusicvideo.idMVideo = musicvideoview.idMVideo"));
      filter.AppendWhere(PrepareSQL("artistlinkmusicvideo.idArtist = %i", (int)option->second.asInteger()));
    }

    option = options.find("artist");
    if (option != options.end())
    {
      if (itemType != "albums")
      {
        filter.AppendJoin(PrepareSQL("join artistlinkmusicvideo on artistlinkmusicvideo.idMVideo = musicvideoview.idMVideo"));
        filter.AppendJoin(PrepareSQL("join actors on actors.idActor = artistlinkmusicvideo.idArtist"));
      }
      filter.AppendWhere(PrepareSQL("actors.strActor like '%s'", option->second.asString().c_str()));
    }

    option = options.find("albumid");
    if (option != options.end())
      filter.AppendWhere(PrepareSQL("musicvideoview.c%02d = (select c%02d from musicvideo where idMVideo = %i)", VIDEODB_ID_MUSICVIDEO_ALBUM, VIDEODB_ID_MUSICVIDEO_ALBUM, (int)option->second.asInteger()));

    option = options.find("tagid");
    if (option != options.end())
    {
      filter.AppendJoin(PrepareSQL("join taglinks on taglinks.idMedia = musicvideoview.idMVideo AND taglinks.media_type = 'musicvideo'"));
      filter.AppendWhere(PrepareSQL("taglinks.idTag = %i", (int)option->second.asInteger()));
    }

    option = options.find("tag");
    if (option != options.end())
    {
      filter.AppendJoin(PrepareSQL("join taglinks on taglinks.idMedia = musicvideoview.idMVideo AND taglinks.media_type = 'musicvideo' join tag on tag.idTag = taglinks.idTag"));
      filter.AppendWhere(PrepareSQL("tag.strTag like '%s'", option->second.asString().c_str()));
    }
  }
  else
    return false;

  option = options.find("xsp");
  if (option != options.end())
  {
    CSmartPlaylist xsp;
    if (!xsp.LoadFromJson(option->second.asString()))
      return false;

    // check if the filter playlist matches the item type
    if (xsp.GetType() == itemType ||
       (xsp.GetGroup() == itemType && !xsp.IsGroupMixed()) ||
        // handle episode listings with videodb://tvshows/titles/ which get the rest
        // of the path (season and episodeid) appended later
       (xsp.GetType() == "episodes" && itemType == "tvshows"))
    {
      std::set<std::string> playlists;
      filter.AppendWhere(xsp.GetWhereClause(*this, playlists));

      if (xsp.GetLimit() > 0)
        sorting.limitEnd = xsp.GetLimit();
      if (xsp.GetOrder() != SortByNone)
        sorting.sortBy = xsp.GetOrder();
      if (xsp.GetOrderDirection() != SortOrderNone)
        sorting.sortOrder = xsp.GetOrderDirection();
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
    if (xspFilter.GetType() == itemType)
    {
      std::set<std::string> playlists;
      filter.AppendWhere(xspFilter.GetWhereClause(*this, playlists));
    }
    // remove the filter if it doesn't match the item type
    else
      videoUrl.RemoveOption("filter");
  }

  return true;
}
