/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoDatabase.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "ServiceBroker.h"
#include "dbwrappers/dataset.h"
#include "dialogs/GUIDialogExtendedProgressBar.h"
#include "dialogs/GUIDialogProgress.h"
#include "dialogs/GUIDialogYesNo.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "filesystem/MultiPathDirectory.h"
#include "filesystem/PluginDirectory.h"
#include "filesystem/StackDirectory.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "interfaces/AnnouncementManager.h"
#include "settings/MediaSourceSettings.h"
#include "storage/MediaManager.h"
#include "URL.h"
#include "Util.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include "video/VideoInfoScanner.h"

using namespace dbiplus;
using namespace XFILE;
using namespace VIDEO;
using namespace ADDON;
using namespace KODI::MESSAGING;
using namespace KODI::GUILIB;

void CVideoDatabase::CreateTables()
{
  CLog::Log(LOGINFO, "create bookmark table");
  m_pDS->exec("CREATE TABLE bookmark ( idBookmark integer primary key, idFile integer, timeInSeconds double, totalTimeInSeconds double, thumbNailImage text, player text, playerState text, type integer)\n");

  CLog::Log(LOGINFO, "create settings table");
  m_pDS->exec("CREATE TABLE settings ( idFile integer, Deinterlace bool,"
              "ViewMode integer,ZoomAmount float, PixelRatio float, VerticalShift float, AudioStream integer, SubtitleStream integer,"
              "SubtitleDelay float, SubtitlesOn bool, Brightness float, Contrast float, Gamma float,"
              "VolumeAmplification float, AudioDelay float, ResumeTime integer,"
              "Sharpness float, NoiseReduction float, NonLinStretch bool, PostProcess bool,"
              "ScalingMethod integer, DeinterlaceMode integer, StereoMode integer, StereoInvert bool, VideoStream integer,"
              "TonemapMethod integer, TonemapParam float, Orientation integer, CenterMixLevel integer)\n");

  CLog::Log(LOGINFO, "create stacktimes table");
  m_pDS->exec("CREATE TABLE stacktimes (idFile integer, times text)\n");

  CLog::Log(LOGINFO, "create genre table");
  m_pDS->exec("CREATE TABLE genre ( genre_id integer primary key, name TEXT)\n");
  m_pDS->exec("CREATE TABLE genre_link (genre_id integer, media_id integer, media_type TEXT)");

  CLog::Log(LOGINFO, "create country table");
  m_pDS->exec("CREATE TABLE country ( country_id integer primary key, name TEXT)");
  m_pDS->exec("CREATE TABLE country_link (country_id integer, media_id integer, media_type TEXT)");

  CLog::Log(LOGINFO, "create movie table");
  std::string columns = "CREATE TABLE movie ( idMovie integer primary key, idFile integer";

  for (int i = 0; i < VIDEODB_MAX_COLUMNS; i++)
    columns += StringUtils::Format(",c%02d text", i);

  columns += ", idSet integer, userrating integer, premiered text)";
  m_pDS->exec(columns);

  CLog::Log(LOGINFO, "create actor table");
  m_pDS->exec("CREATE TABLE actor ( actor_id INTEGER PRIMARY KEY, name TEXT, art_urls TEXT )");
  m_pDS->exec("CREATE TABLE actor_link(actor_id INTEGER, media_id INTEGER, media_type TEXT, role TEXT, cast_order INTEGER)");
  m_pDS->exec("CREATE TABLE director_link(actor_id INTEGER, media_id INTEGER, media_type TEXT)");
  m_pDS->exec("CREATE TABLE writer_link(actor_id INTEGER, media_id INTEGER, media_type TEXT)");

  CLog::Log(LOGINFO, "create path table");
  m_pDS->exec("CREATE TABLE path ( idPath integer primary key, strPath text, strContent text, strScraper text, strHash text, scanRecursive integer, useFolderNames bool, strSettings text, noUpdate bool, exclude bool, dateAdded text, idParentPath integer)");

  CLog::Log(LOGINFO, "create files table");
  m_pDS->exec("CREATE TABLE files ( idFile integer primary key, idPath integer, strFilename text, playCount integer, lastPlayed text, dateAdded text)");

  CLog::Log(LOGINFO, "create tvshow table");
  columns = "CREATE TABLE tvshow ( idShow integer primary key";

  for (int i = 0; i < VIDEODB_MAX_COLUMNS; i++)
    columns += StringUtils::Format(",c%02d text", i);

  columns += ", userrating integer, duration INTEGER)";
  m_pDS->exec(columns);

  CLog::Log(LOGINFO, "create episode table");
  columns = "CREATE TABLE episode ( idEpisode integer primary key, idFile integer";
  for (int i = 0; i < VIDEODB_MAX_COLUMNS; i++)
  {
    std::string column;
    if ( i == VIDEODB_ID_EPISODE_SEASON || i == VIDEODB_ID_EPISODE_EPISODE || i == VIDEODB_ID_EPISODE_BOOKMARK)
      column = StringUtils::Format(",c%02d varchar(24)", i);
    else
      column = StringUtils::Format(",c%02d text", i);

    columns += column;
  }
  columns += ", idShow integer, userrating integer, idSeason integer)";
  m_pDS->exec(columns);

  CLog::Log(LOGINFO, "create tvshowlinkpath table");
  m_pDS->exec("CREATE TABLE tvshowlinkpath (idShow integer, idPath integer)\n");

  CLog::Log(LOGINFO, "create movielinktvshow table");
  m_pDS->exec("CREATE TABLE movielinktvshow ( idMovie integer, IdShow integer)\n");

  CLog::Log(LOGINFO, "create studio table");
  m_pDS->exec("CREATE TABLE studio ( studio_id integer primary key, name TEXT)\n");
  m_pDS->exec("CREATE TABLE studio_link (studio_id integer, media_id integer, media_type TEXT)");

  CLog::Log(LOGINFO, "create musicvideo table");
  columns = "CREATE TABLE musicvideo ( idMVideo integer primary key, idFile integer";
  for (int i = 0; i < VIDEODB_MAX_COLUMNS; i++)
    columns += StringUtils::Format(",c%02d text", i);

  columns += ", userrating integer, premiered text)";
  m_pDS->exec(columns);

  CLog::Log(LOGINFO, "create streaminfo table");
  m_pDS->exec("CREATE TABLE streamdetails (idFile integer, iStreamType integer, "
    "strVideoCodec text, fVideoAspect float, iVideoWidth integer, iVideoHeight integer, "
    "strAudioCodec text, iAudioChannels integer, strAudioLanguage text, "
    "strSubtitleLanguage text, iVideoDuration integer, strStereoMode text, strVideoLanguage text)");

  CLog::Log(LOGINFO, "create sets table");
  m_pDS->exec("CREATE TABLE sets ( idSet integer primary key, strSet text, strOverview text)");

  CLog::Log(LOGINFO, "create seasons table");
  m_pDS->exec("CREATE TABLE seasons ( idSeason integer primary key, idShow integer, season integer, name text, userrating integer)");

  CLog::Log(LOGINFO, "create art table");
  m_pDS->exec("CREATE TABLE art(art_id INTEGER PRIMARY KEY, media_id INTEGER, media_type TEXT, type TEXT, url TEXT)");

  CLog::Log(LOGINFO, "create tag table");
  m_pDS->exec("CREATE TABLE tag (tag_id integer primary key, name TEXT)");
  m_pDS->exec("CREATE TABLE tag_link (tag_id integer, media_id integer, media_type TEXT)");

  CLog::Log(LOGINFO, "create rating table");
  m_pDS->exec("CREATE TABLE rating (rating_id INTEGER PRIMARY KEY, media_id INTEGER, media_type TEXT, rating_type TEXT, rating FLOAT, votes INTEGER)");

  CLog::Log(LOGINFO, "create uniqueid table");
  m_pDS->exec("CREATE TABLE uniqueid (uniqueid_id INTEGER PRIMARY KEY, media_id INTEGER, media_type TEXT, value TEXT, type TEXT)");
}

void CVideoDatabase::CreateLinkIndex(const char *table)
{
  m_pDS->exec(PrepareSQL("CREATE UNIQUE INDEX ix_%s_1 ON %s (name(255))", table, table));
  m_pDS->exec(PrepareSQL("CREATE UNIQUE INDEX ix_%s_link_1 ON %s_link (%s_id, media_type(20), media_id)", table, table, table));
  m_pDS->exec(PrepareSQL("CREATE UNIQUE INDEX ix_%s_link_2 ON %s_link (media_id, media_type(20), %s_id)", table, table, table));
  m_pDS->exec(PrepareSQL("CREATE INDEX ix_%s_link_3 ON %s_link (media_type(20))", table, table));
}

void CVideoDatabase::CreateForeignLinkIndex(const char *table, const char *foreignkey)
{
  m_pDS->exec(PrepareSQL("CREATE UNIQUE INDEX ix_%s_link_1 ON %s_link (%s_id, media_type(20), media_id)", table, table, foreignkey));
  m_pDS->exec(PrepareSQL("CREATE UNIQUE INDEX ix_%s_link_2 ON %s_link (media_id, media_type(20), %s_id)", table, table, foreignkey));
  m_pDS->exec(PrepareSQL("CREATE INDEX ix_%s_link_3 ON %s_link (media_type(20))", table, table));
}

void CVideoDatabase::CreateAnalytics()
{
  /* indexes should be added on any columns that are used in in  */
  /* a where or a join. primary key on a column is the same as a */
  /* unique index on that column, so there is no need to add any */
  /* index if no other columns are refered                       */

  /* order of indexes are important, for an index to be considered all  */
  /* columns up to the column in question have to have been specified   */
  /* select * from foolink where foo_id = 1, can not take               */
  /* advantage of a index that has been created on ( bar_id, foo_id )   */
  /* however an index on ( foo_id, bar_id ) will be considered for use  */

  CLog::Log(LOGINFO, "%s - creating indices", __FUNCTION__);
  m_pDS->exec("CREATE INDEX ix_bookmark ON bookmark (idFile, type)");
  m_pDS->exec("CREATE UNIQUE INDEX ix_settings ON settings ( idFile )\n");
  m_pDS->exec("CREATE UNIQUE INDEX ix_stacktimes ON stacktimes ( idFile )\n");
  m_pDS->exec("CREATE INDEX ix_path ON path ( strPath(255) )");
  m_pDS->exec("CREATE INDEX ix_path2 ON path ( idParentPath )");
  m_pDS->exec("CREATE INDEX ix_files ON files ( idPath, strFilename(255) )");

  m_pDS->exec("CREATE UNIQUE INDEX ix_movie_file_1 ON movie (idFile, idMovie)");
  m_pDS->exec("CREATE UNIQUE INDEX ix_movie_file_2 ON movie (idMovie, idFile)");

  m_pDS->exec("CREATE UNIQUE INDEX ix_tvshowlinkpath_1 ON tvshowlinkpath ( idShow, idPath )\n");
  m_pDS->exec("CREATE UNIQUE INDEX ix_tvshowlinkpath_2 ON tvshowlinkpath ( idPath, idShow )\n");
  m_pDS->exec("CREATE UNIQUE INDEX ix_movielinktvshow_1 ON movielinktvshow ( idShow, idMovie)\n");
  m_pDS->exec("CREATE UNIQUE INDEX ix_movielinktvshow_2 ON movielinktvshow ( idMovie, idShow)\n");

  m_pDS->exec("CREATE UNIQUE INDEX ix_episode_file_1 on episode (idEpisode, idFile)");
  m_pDS->exec("CREATE UNIQUE INDEX id_episode_file_2 on episode (idFile, idEpisode)");
  std::string createColIndex = StringUtils::Format("CREATE INDEX ix_episode_season_episode on episode (c%02d, c%02d)", VIDEODB_ID_EPISODE_SEASON, VIDEODB_ID_EPISODE_EPISODE);
  m_pDS->exec(createColIndex);
  createColIndex = StringUtils::Format("CREATE INDEX ix_episode_bookmark on episode (c%02d)", VIDEODB_ID_EPISODE_BOOKMARK);
  m_pDS->exec(createColIndex);
  m_pDS->exec("CREATE INDEX ix_episode_show1 on episode(idEpisode,idShow)");
  m_pDS->exec("CREATE INDEX ix_episode_show2 on episode(idShow,idEpisode)");

  m_pDS->exec("CREATE UNIQUE INDEX ix_musicvideo_file_1 on musicvideo (idMVideo, idFile)");
  m_pDS->exec("CREATE UNIQUE INDEX ix_musicvideo_file_2 on musicvideo (idFile, idMVideo)");

  m_pDS->exec("CREATE INDEX ixMovieBasePath ON movie ( c23(12) )");
  m_pDS->exec("CREATE INDEX ixMusicVideoBasePath ON musicvideo ( c14(12) )");
  m_pDS->exec("CREATE INDEX ixEpisodeBasePath ON episode ( c19(12) )");

  m_pDS->exec("CREATE INDEX ix_streamdetails ON streamdetails (idFile)");
  m_pDS->exec("CREATE INDEX ix_seasons ON seasons (idShow, season)");
  m_pDS->exec("CREATE INDEX ix_art ON art(media_id, media_type(20), type(20))");

  m_pDS->exec("CREATE INDEX ix_rating ON rating(media_id, media_type(20))");

  m_pDS->exec("CREATE INDEX ix_uniqueid1 ON uniqueid(media_id, media_type(20), type(20))");
  m_pDS->exec("CREATE INDEX ix_uniqueid2 ON uniqueid(media_type(20), value(20))");

  CreateLinkIndex("tag");
  CreateLinkIndex("actor");
  CreateForeignLinkIndex("director", "actor");
  CreateForeignLinkIndex("writer", "actor");
  CreateLinkIndex("studio");
  CreateLinkIndex("genre");
  CreateLinkIndex("country");

  CLog::Log(LOGINFO, "%s - creating triggers", __FUNCTION__);
  m_pDS->exec("CREATE TRIGGER delete_movie AFTER DELETE ON movie FOR EACH ROW BEGIN "
              "DELETE FROM genre_link WHERE media_id=old.idMovie AND media_type='movie'; "
              "DELETE FROM actor_link WHERE media_id=old.idMovie AND media_type='movie'; "
              "DELETE FROM director_link WHERE media_id=old.idMovie AND media_type='movie'; "
              "DELETE FROM studio_link WHERE media_id=old.idMovie AND media_type='movie'; "
              "DELETE FROM country_link WHERE media_id=old.idMovie AND media_type='movie'; "
              "DELETE FROM writer_link WHERE media_id=old.idMovie AND media_type='movie'; "
              "DELETE FROM movielinktvshow WHERE idMovie=old.idMovie; "
              "DELETE FROM art WHERE media_id=old.idMovie AND media_type='movie'; "
              "DELETE FROM tag_link WHERE media_id=old.idMovie AND media_type='movie'; "
              "DELETE FROM rating WHERE media_id=old.idMovie AND media_type='movie'; "
              "DELETE FROM uniqueid WHERE media_id=old.idMovie AND media_type='movie'; "
              "END");
  m_pDS->exec("CREATE TRIGGER delete_tvshow AFTER DELETE ON tvshow FOR EACH ROW BEGIN "
              "DELETE FROM actor_link WHERE media_id=old.idShow AND media_type='tvshow'; "
              "DELETE FROM director_link WHERE media_id=old.idShow AND media_type='tvshow'; "
              "DELETE FROM studio_link WHERE media_id=old.idShow AND media_type='tvshow'; "
              "DELETE FROM tvshowlinkpath WHERE idShow=old.idShow; "
              "DELETE FROM genre_link WHERE media_id=old.idShow AND media_type='tvshow'; "
              "DELETE FROM movielinktvshow WHERE idShow=old.idShow; "
              "DELETE FROM seasons WHERE idShow=old.idShow; "
              "DELETE FROM art WHERE media_id=old.idShow AND media_type='tvshow'; "
              "DELETE FROM tag_link WHERE media_id=old.idShow AND media_type='tvshow'; "
              "DELETE FROM rating WHERE media_id=old.idShow AND media_type='tvshow'; "
              "DELETE FROM uniqueid WHERE media_id=old.idShow AND media_type='tvshow'; "
              "END");
  m_pDS->exec("CREATE TRIGGER delete_musicvideo AFTER DELETE ON musicvideo FOR EACH ROW BEGIN "
              "DELETE FROM actor_link WHERE media_id=old.idMVideo AND media_type='musicvideo'; "
              "DELETE FROM director_link WHERE media_id=old.idMVideo AND media_type='musicvideo'; "
              "DELETE FROM genre_link WHERE media_id=old.idMVideo AND media_type='musicvideo'; "
              "DELETE FROM studio_link WHERE media_id=old.idMVideo AND media_type='musicvideo'; "
              "DELETE FROM art WHERE media_id=old.idMVideo AND media_type='musicvideo'; "
              "DELETE FROM tag_link WHERE media_id=old.idMVideo AND media_type='musicvideo'; "
              "END");
  m_pDS->exec("CREATE TRIGGER delete_episode AFTER DELETE ON episode FOR EACH ROW BEGIN "
              "DELETE FROM actor_link WHERE media_id=old.idEpisode AND media_type='episode'; "
              "DELETE FROM director_link WHERE media_id=old.idEpisode AND media_type='episode'; "
              "DELETE FROM writer_link WHERE media_id=old.idEpisode AND media_type='episode'; "
              "DELETE FROM art WHERE media_id=old.idEpisode AND media_type='episode'; "
              "DELETE FROM rating WHERE media_id=old.idEpisode AND media_type='episode'; "
              "DELETE FROM uniqueid WHERE media_id=old.idEpisode AND media_type='episode'; "
              "END");
  m_pDS->exec("CREATE TRIGGER delete_season AFTER DELETE ON seasons FOR EACH ROW BEGIN "
              "DELETE FROM art WHERE media_id=old.idSeason AND media_type='season'; "
              "END");
  m_pDS->exec("CREATE TRIGGER delete_set AFTER DELETE ON sets FOR EACH ROW BEGIN "
              "DELETE FROM art WHERE media_id=old.idSet AND media_type='set'; "
              "END");
  m_pDS->exec("CREATE TRIGGER delete_person AFTER DELETE ON actor FOR EACH ROW BEGIN "
              "DELETE FROM art WHERE media_id=old.actor_id AND media_type IN ('actor','artist','writer','director'); "
              "END");
  m_pDS->exec("CREATE TRIGGER delete_tag AFTER DELETE ON tag_link FOR EACH ROW BEGIN "
              "DELETE FROM tag WHERE tag_id=old.tag_id AND tag_id NOT IN (SELECT DISTINCT tag_id FROM tag_link); "
              "END");
  m_pDS->exec("CREATE TRIGGER delete_file AFTER DELETE ON files FOR EACH ROW BEGIN "
              "DELETE FROM bookmark WHERE idFile=old.idFile; "
              "DELETE FROM settings WHERE idFile=old.idFile; "
              "DELETE FROM stacktimes WHERE idFile=old.idFile; "
              "DELETE FROM streamdetails WHERE idFile=old.idFile; "
              "END");

  CreateViews();
}

void CVideoDatabase::CreateViews()
{
  CLog::Log(LOGINFO, "create episode_view");
  std::string episodeview = PrepareSQL("CREATE VIEW episode_view AS SELECT "
                                      "  episode.*,"
                                      "  files.strFileName AS strFileName,"
                                      "  path.strPath AS strPath,"
                                      "  files.playCount AS playCount,"
                                      "  files.lastPlayed AS lastPlayed,"
                                      "  files.dateAdded AS dateAdded,"
                                      "  tvshow.c%02d AS strTitle,"
                                      "  tvshow.c%02d AS genre,"
                                      "  tvshow.c%02d AS studio,"
                                      "  tvshow.c%02d AS premiered,"
                                      "  tvshow.c%02d AS mpaa,"
                                      "  bookmark.timeInSeconds AS resumeTimeInSeconds, "
                                      "  bookmark.totalTimeInSeconds AS totalTimeInSeconds, "
                                      "  bookmark.playerState AS playerState, "
                                      "  rating.rating AS rating, "
                                      "  rating.votes AS votes, "
                                      "  rating.rating_type AS rating_type, "
                                      "  uniqueid.value AS uniqueid_value, "
                                      "  uniqueid.type AS uniqueid_type "
                                      "FROM episode"
                                      "  JOIN files ON"
                                      "    files.idFile=episode.idFile"
                                      "  JOIN tvshow ON"
                                      "    tvshow.idShow=episode.idShow"
                                      "  JOIN seasons ON"
                                      "    seasons.idSeason=episode.idSeason"
                                      "  JOIN path ON"
                                      "    files.idPath=path.idPath"
                                      "  LEFT JOIN bookmark ON"
                                      "    bookmark.idFile=episode.idFile AND bookmark.type=1"
                                      "  LEFT JOIN rating ON"
                                      "    rating.rating_id=episode.c%02d"
                                      "  LEFT JOIN uniqueid ON"
                                      "    uniqueid.uniqueid_id=episode.c%02d",
                                      VIDEODB_ID_TV_TITLE, VIDEODB_ID_TV_GENRE,
                                      VIDEODB_ID_TV_STUDIOS, VIDEODB_ID_TV_PREMIERED,
                                      VIDEODB_ID_TV_MPAA, VIDEODB_ID_EPISODE_RATING_ID,
                                      VIDEODB_ID_EPISODE_IDENT_ID);
  m_pDS->exec(episodeview);

  CLog::Log(LOGINFO, "create tvshowcounts");
  std::string tvshowcounts = PrepareSQL("CREATE VIEW tvshowcounts AS SELECT "
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
                                       "GROUP BY tvshow.idShow");
  m_pDS->exec(tvshowcounts);

  CLog::Log(LOGINFO, "create tvshowlinkpath_minview");
  // This view only exists to workaround a limitation in MySQL <5.7 which is not able to
  // perform subqueries in joins.
  // Also, the correct solution is to remove the path information altogether, since a
  // TV series can always have multiple paths. It is used in the GUI at the moment, but
  // such usage should be removed together with this view and the path columns in tvshow_view.
  //!@todo Remove the hacky selection of a semi-random path for tvshows from the queries and UI
  std::string tvshowlinkpathview = PrepareSQL("CREATE VIEW tvshowlinkpath_minview AS SELECT "
                                             "  idShow, "
                                             "  min(idPath) AS idPath "
                                             "FROM tvshowlinkpath "
                                             "GROUP BY idShow");
  m_pDS->exec(tvshowlinkpathview);

  CLog::Log(LOGINFO, "create tvshow_view");
  std::string tvshowview = PrepareSQL("CREATE VIEW tvshow_view AS SELECT "
                                     "  tvshow.*,"
                                     "  path.idParentPath AS idParentPath,"
                                     "  path.strPath AS strPath,"
                                     "  tvshowcounts.dateAdded AS dateAdded,"
                                     "  lastPlayed, totalCount, watchedcount, totalSeasons, "
                                     "  rating.rating AS rating, "
                                     "  rating.votes AS votes, "
                                     "  rating.rating_type AS rating_type, "
                                     "  uniqueid.value AS uniqueid_value, "
                                     "  uniqueid.type AS uniqueid_type "
                                     "FROM tvshow"
                                     "  LEFT JOIN tvshowlinkpath_minview ON "
                                     "    tvshowlinkpath_minview.idShow=tvshow.idShow"
                                     "  LEFT JOIN path ON"
                                     "    path.idPath=tvshowlinkpath_minview.idPath"
                                     "  INNER JOIN tvshowcounts ON"
                                     "    tvshow.idShow = tvshowcounts.idShow "
                                     "  LEFT JOIN rating ON"
                                     "    rating.rating_id=tvshow.c%02d "
                                     "  LEFT JOIN uniqueid ON"
                                     "    uniqueid.uniqueid_id=tvshow.c%02d ",
                                     VIDEODB_ID_TV_RATING_ID, VIDEODB_ID_TV_IDENT_ID);
  m_pDS->exec(tvshowview);
  
  CLog::Log(LOGINFO, "create season_view");
  std::string seasonview = PrepareSQL("CREATE VIEW season_view AS SELECT "
                                     "  seasons.idSeason AS idSeason,"
                                     "  seasons.idShow AS idShow,"
                                     "  seasons.season AS season,"
                                     "  seasons.name AS name,"
                                     "  seasons.userrating AS userrating,"
                                     "  tvshow_view.strPath AS strPath,"
                                     "  tvshow_view.c%02d AS showTitle,"
                                     "  tvshow_view.c%02d AS plot,"
                                     "  tvshow_view.c%02d AS premiered,"
                                     "  tvshow_view.c%02d AS genre,"
                                     "  tvshow_view.c%02d AS studio,"
                                     "  tvshow_view.c%02d AS mpaa,"
                                     "  count(DISTINCT episode.idEpisode) AS episodes,"
                                     "  count(files.playCount) AS playCount,"
                                     "  min(episode.c%02d) AS aired "
                                     "FROM seasons"
                                     "  JOIN tvshow_view ON"
                                     "    tvshow_view.idShow = seasons.idShow"
                                     "  JOIN episode ON"
                                     "    episode.idShow = seasons.idShow AND episode.c%02d = seasons.season"
                                     "  JOIN files ON"
                                     "    files.idFile = episode.idFile "
                                     "GROUP BY seasons.idSeason,"
                                     "         seasons.idShow,"
                                     "         seasons.season,"
                                     "         seasons.name,"
                                     "         seasons.userrating,"
                                     "         tvshow_view.strPath,"
                                     "         tvshow_view.c%02d,"
                                     "         tvshow_view.c%02d,"
                                     "         tvshow_view.c%02d,"
                                     "         tvshow_view.c%02d,"
                                     "         tvshow_view.c%02d,"
                                     "         tvshow_view.c%02d ",
                                     VIDEODB_ID_TV_TITLE, VIDEODB_ID_TV_PLOT, VIDEODB_ID_TV_PREMIERED,
                                     VIDEODB_ID_TV_GENRE, VIDEODB_ID_TV_STUDIOS, VIDEODB_ID_TV_MPAA,
                                     VIDEODB_ID_EPISODE_AIRED, VIDEODB_ID_EPISODE_SEASON,
                                     VIDEODB_ID_TV_TITLE, VIDEODB_ID_TV_PLOT, VIDEODB_ID_TV_PREMIERED,
                                     VIDEODB_ID_TV_GENRE, VIDEODB_ID_TV_STUDIOS, VIDEODB_ID_TV_MPAA);
  m_pDS->exec(seasonview);

  CLog::Log(LOGINFO, "create musicvideo_view");
  m_pDS->exec("CREATE VIEW musicvideo_view AS SELECT"
              "  musicvideo.*,"
              "  files.strFileName as strFileName,"
              "  path.strPath as strPath,"
              "  files.playCount as playCount,"
              "  files.lastPlayed as lastPlayed,"
              "  files.dateAdded as dateAdded, "
              "  bookmark.timeInSeconds AS resumeTimeInSeconds, "
              "  bookmark.totalTimeInSeconds AS totalTimeInSeconds, "
              "  bookmark.playerState AS playerState "
              "FROM musicvideo"
              "  JOIN files ON"
              "    files.idFile=musicvideo.idFile"
              "  JOIN path ON"
              "    path.idPath=files.idPath"
              "  LEFT JOIN bookmark ON"
              "    bookmark.idFile=musicvideo.idFile AND bookmark.type=1");

  CLog::Log(LOGINFO, "create movie_view");

  std::string movieview = PrepareSQL("CREATE VIEW movie_view AS SELECT"
                                      "  movie.*,"
                                      "  sets.strSet AS strSet,"
                                      "  sets.strOverview AS strSetOverview,"
                                      "  files.strFileName AS strFileName,"
                                      "  path.strPath AS strPath,"
                                      "  files.playCount AS playCount,"
                                      "  files.lastPlayed AS lastPlayed, "
                                      "  files.dateAdded AS dateAdded, "
                                      "  bookmark.timeInSeconds AS resumeTimeInSeconds, "
                                      "  bookmark.totalTimeInSeconds AS totalTimeInSeconds, "
                                      "  bookmark.playerState AS playerState, "
                                      "  rating.rating AS rating, "
                                      "  rating.votes AS votes, "
                                      "  rating.rating_type AS rating_type, "
                                      "  uniqueid.value AS uniqueid_value, "
                                      "  uniqueid.type AS uniqueid_type "
                                      "FROM movie"
                                      "  LEFT JOIN sets ON"
                                      "    sets.idSet = movie.idSet"
                                      "  JOIN files ON"
                                      "    files.idFile=movie.idFile"
                                      "  JOIN path ON"
                                      "    path.idPath=files.idPath"
                                      "  LEFT JOIN bookmark ON"
                                      "    bookmark.idFile=movie.idFile AND bookmark.type=1"
                                      "  LEFT JOIN rating ON"
                                      "    rating.rating_id=movie.c%02d"
                                      "  LEFT JOIN uniqueid ON"
                                      "    uniqueid.uniqueid_id=movie.c%02d",
                                      VIDEODB_ID_RATING_ID, VIDEODB_ID_IDENT_ID);
  m_pDS->exec(movieview);
}

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
  std::string title;
  std::string year;
  std::string ident;
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
  // Important: DO NOT use CREATE TABLE [...] AS SELECT [...] - it does not work on MySQL with GTID consistency enforced

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
    std::map<std::string, int> paths;
    m_pDS->query("select idPath,strPath from path");
    while (!m_pDS->eof())
    {
      paths.insert(make_pair(m_pDS->fv(1).get_asString(), m_pDS->fv(0).get_asInt()));
      m_pDS->next();
    }
    m_pDS->close();
    // run through these paths figuring out the parent path, and add to the table if found
    for (const auto &i : paths)
    {
      std::string parent = URIUtils::GetParentPath(i.first);
      auto j = paths.find(parent);
      if (j != paths.end())
        m_pDS->exec(PrepareSQL("UPDATE path SET idParentPath=%i WHERE idPath=%i", j->second, i.second));
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
    std::string sql = PrepareSQL("SELECT tvshow.idShow,idPath,c%02d,c%02d,c%02d FROM tvshow JOIN tvshowlinkpath ON tvshow.idShow = tvshowlinkpath.idShow", VIDEODB_ID_TV_TITLE, VIDEODB_ID_TV_PREMIERED, VIDEODB_ID_TV_IDENT_ID);
    m_pDS->query(sql);
    std::vector<CShowItem> shows;
    while (!m_pDS->eof())
    {
      CShowItem show;
      show.id    = m_pDS->fv(0).get_asInt();
      show.path  = m_pDS->fv(1).get_asInt();
      show.title = m_pDS->fv(2).get_asString();
      show.year  = m_pDS->fv(3).get_asString();
      show.ident = m_pDS->fv(4).get_asString();
      shows.emplace_back(std::move(show));
      m_pDS->next();
    }
    m_pDS->close();
    if (!shows.empty())
    {
      for (auto i = shows.begin() + 1; i != shows.end(); ++i)
      {
        // has this show been found before?
        auto j = find(shows.begin(), i, *i);
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
      m_pDS->exec("DELETE FROM seasons WHERE idSeason NOT IN (SELECT idSeason FROM (SELECT min(idSeason) as idSeason FROM seasons GROUP BY idShow,season) AS sub)");
    }
  }
  if (iVersion < 84)
  { // replace any multipaths in tvshowlinkpath table
    m_pDS->query("SELECT idShow, tvshowlinkpath.idPath, strPath FROM tvshowlinkpath JOIN path ON tvshowlinkpath.idPath=path.idPath WHERE path.strPath LIKE 'multipath://%'");
    std::vector<CShowLink> shows;
    while (!m_pDS->eof())
    {
      CShowLink link;
      link.show   = m_pDS->fv(0).get_asInt();
      link.pathId = m_pDS->fv(1).get_asInt();
      link.path   = m_pDS->fv(2).get_asString();
      shows.emplace_back(std::move(link));
      m_pDS->next();
    }
    m_pDS->close();
    // update these
    for (auto i = shows.begin(); i != shows.end(); ++i)
    {
      std::vector<std::string> paths;
      CMultiPathDirectory::GetPaths(i->path, paths);
      for (auto j = paths.begin(); j != paths.end(); ++j)
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
  if (iVersion < 91)
  {
    // create actor link table
    m_pDS->exec("CREATE TABLE actor_link(actor_id INT, media_id INT, media_type TEXT, role TEXT, cast_order INT)");
    m_pDS->exec("INSERT INTO actor_link(actor_id, media_id, media_type, role, cast_order) SELECT DISTINCT idActor, idMovie, 'movie', strRole, iOrder from actorlinkmovie");
    m_pDS->exec("INSERT INTO actor_link(actor_id, media_id, media_type, role, cast_order) SELECT DISTINCT idActor, idShow, 'tvshow', strRole, iOrder from actorlinktvshow");
    m_pDS->exec("INSERT INTO actor_link(actor_id, media_id, media_type, role, cast_order) SELECT DISTINCT idActor, idEpisode, 'episode', strRole, iOrder from actorlinkepisode");
    m_pDS->exec("DROP TABLE IF EXISTS actorlinkmovie");
    m_pDS->exec("DROP TABLE IF EXISTS actorlinktvshow");
    m_pDS->exec("DROP TABLE IF EXISTS actorlinkepisode");
    m_pDS->exec("CREATE TABLE actor(actor_id INTEGER PRIMARY KEY, name TEXT, art_urls TEXT)");
    m_pDS->exec("INSERT INTO actor(actor_id, name, art_urls) SELECT idActor,strActor,strThumb FROM actors");
    m_pDS->exec("DROP TABLE IF EXISTS actors");

    // directors
    m_pDS->exec("CREATE TABLE director_link(actor_id INTEGER, media_id INTEGER, media_type TEXT)");
    m_pDS->exec("INSERT INTO director_link(actor_id, media_id, media_type) SELECT DISTINCT idDirector, idMovie, 'movie' FROM directorlinkmovie");
    m_pDS->exec("INSERT INTO director_link(actor_id, media_id, media_type) SELECT DISTINCT idDirector, idShow, 'tvshow' FROM directorlinktvshow");
    m_pDS->exec("INSERT INTO director_link(actor_id, media_id, media_type) SELECT DISTINCT idDirector, idEpisode, 'episode' FROM directorlinkepisode");
    m_pDS->exec("INSERT INTO director_link(actor_id, media_id, media_type) SELECT DISTINCT idDirector, idMVideo, 'musicvideo' FROM directorlinkmusicvideo");
    m_pDS->exec("DROP TABLE IF EXISTS directorlinkmovie");
    m_pDS->exec("DROP TABLE IF EXISTS directorlinktvshow");
    m_pDS->exec("DROP TABLE IF EXISTS directorlinkepisode");
    m_pDS->exec("DROP TABLE IF EXISTS directorlinkmusicvideo");

    // writers
    m_pDS->exec("CREATE TABLE writer_link(actor_id INTEGER, media_id INTEGER, media_type TEXT)");
    m_pDS->exec("INSERT INTO writer_link(actor_id, media_id, media_type) SELECT DISTINCT idWriter, idMovie, 'movie' FROM writerlinkmovie");
    m_pDS->exec("INSERT INTO writer_link(actor_id, media_id, media_type) SELECT DISTINCT idWriter, idEpisode, 'episode' FROM writerlinkepisode");
    m_pDS->exec("DROP TABLE IF EXISTS writerlinkmovie");
    m_pDS->exec("DROP TABLE IF EXISTS writerlinkepisode");

    // music artist
    m_pDS->exec("INSERT INTO actor_link(actor_id, media_id, media_type) SELECT DISTINCT idArtist, idMVideo, 'musicvideo' FROM artistlinkmusicvideo");
    m_pDS->exec("DROP TABLE IF EXISTS artistlinkmusicvideo");

    // studios
    m_pDS->exec("CREATE TABLE studio_link(studio_id INTEGER, media_id INTEGER, media_type TEXT)");
    m_pDS->exec("INSERT INTO studio_link(studio_id, media_id, media_type) SELECT DISTINCT idStudio, idMovie, 'movie' FROM studiolinkmovie");
    m_pDS->exec("INSERT INTO studio_link(studio_id, media_id, media_type) SELECT DISTINCT idStudio, idShow, 'tvshow' FROM studiolinktvshow");
    m_pDS->exec("INSERT INTO studio_link(studio_id, media_id, media_type) SELECT DISTINCT idStudio, idMVideo, 'musicvideo' FROM studiolinkmusicvideo");
    m_pDS->exec("DROP TABLE IF EXISTS studiolinkmovie");
    m_pDS->exec("DROP TABLE IF EXISTS studiolinktvshow");
    m_pDS->exec("DROP TABLE IF EXISTS studiolinkmusicvideo");
    m_pDS->exec("CREATE TABLE studionew(studio_id INTEGER PRIMARY KEY, name TEXT)");
    m_pDS->exec("INSERT INTO studionew(studio_id, name) SELECT idStudio,strStudio FROM studio");
    m_pDS->exec("DROP TABLE IF EXISTS studio");
    m_pDS->exec("ALTER TABLE studionew RENAME TO studio");

    // genres
    m_pDS->exec("CREATE TABLE genre_link(genre_id INTEGER, media_id INTEGER, media_type TEXT)");
    m_pDS->exec("INSERT INTO genre_link(genre_id, media_id, media_type) SELECT DISTINCT idGenre, idMovie, 'movie' FROM genrelinkmovie");
    m_pDS->exec("INSERT INTO genre_link(genre_id, media_id, media_type) SELECT DISTINCT idGenre, idShow, 'tvshow' FROM genrelinktvshow");
    m_pDS->exec("INSERT INTO genre_link(genre_id, media_id, media_type) SELECT DISTINCT idGenre, idMVideo, 'musicvideo' FROM genrelinkmusicvideo");
    m_pDS->exec("DROP TABLE IF EXISTS genrelinkmovie");
    m_pDS->exec("DROP TABLE IF EXISTS genrelinktvshow");
    m_pDS->exec("DROP TABLE IF EXISTS genrelinkmusicvideo");
    m_pDS->exec("CREATE TABLE genrenew(genre_id INTEGER PRIMARY KEY, name TEXT)");
    m_pDS->exec("INSERT INTO genrenew(genre_id, name) SELECT idGenre,strGenre FROM genre");
    m_pDS->exec("DROP TABLE IF EXISTS genre");
    m_pDS->exec("ALTER TABLE genrenew RENAME TO genre");

    // country
    m_pDS->exec("CREATE TABLE country_link(country_id INTEGER, media_id INTEGER, media_type TEXT)");
    m_pDS->exec("INSERT INTO country_link(country_id, media_id, media_type) SELECT DISTINCT idCountry, idMovie, 'movie' FROM countrylinkmovie");
    m_pDS->exec("DROP TABLE IF EXISTS countrylinkmovie");
    m_pDS->exec("CREATE TABLE countrynew(country_id INTEGER PRIMARY KEY, name TEXT)");
    m_pDS->exec("INSERT INTO countrynew(country_id, name) SELECT idCountry,strCountry FROM country");
    m_pDS->exec("DROP TABLE IF EXISTS country");
    m_pDS->exec("ALTER TABLE countrynew RENAME TO country");

    // tags
    m_pDS->exec("CREATE TABLE tag_link(tag_id INTEGER, media_id INTEGER, media_type TEXT)");
    m_pDS->exec("INSERT INTO tag_link(tag_id, media_id, media_type) SELECT DISTINCT idTag, idMedia, media_type FROM taglinks");
    m_pDS->exec("DROP TABLE IF EXISTS taglinks");
    m_pDS->exec("CREATE TABLE tagnew(tag_id INTEGER PRIMARY KEY, name TEXT)");
    m_pDS->exec("INSERT INTO tagnew(tag_id, name) SELECT idTag,strTag FROM tag");
    m_pDS->exec("DROP TABLE IF EXISTS tag");
    m_pDS->exec("ALTER TABLE tagnew RENAME TO tag");
  }

  if (iVersion < 93)
  {
    // cleanup main tables
    std::string valuesSql;
    for(int i = 0; i < VIDEODB_MAX_COLUMNS; i++)
    {
      valuesSql += StringUtils::Format("c%02d = TRIM(c%02d)", i, i);
      if (i < VIDEODB_MAX_COLUMNS - 1)
        valuesSql += ",";
    }
    m_pDS->exec("UPDATE episode SET " + valuesSql);
    m_pDS->exec("UPDATE movie SET " + valuesSql);
    m_pDS->exec("UPDATE musicvideo SET " + valuesSql);
    m_pDS->exec("UPDATE tvshow SET " + valuesSql);

    // cleanup additional tables
    std::map<std::string, std::vector<std::string>> additionalTablesMap = {
      {"actor", {"actor_link", "director_link", "writer_link"}},
      {"studio", {"studio_link"}},
      {"genre", {"genre_link"}},
      {"country", {"country_link"}},
      {"tag", {"tag_link"}}
    };
    for (const auto& additionalTableEntry : additionalTablesMap)
    {
      std::string table = additionalTableEntry.first;
      std::string tablePk = additionalTableEntry.first + "_id";
      std::map<int, std::string> duplicatesMinMap;
      std::map<int, std::string> duplicatesMap;

      // cleanup name
      m_pDS->exec(PrepareSQL("UPDATE %s SET name = TRIM(name)",
                             table.c_str()));

      // shrink name to length 255
      m_pDS->exec(PrepareSQL("UPDATE %s SET name = SUBSTR(name, 1, 255) WHERE LENGTH(name) > 255",
                             table.c_str()));

      // fetch main entries
      m_pDS->query(PrepareSQL("SELECT MIN(%s), name FROM %s GROUP BY name HAVING COUNT(1) > 1",
                              tablePk.c_str(), table.c_str()));

      while (!m_pDS->eof())
      {
        duplicatesMinMap.insert(std::make_pair(m_pDS->fv(0).get_asInt(), m_pDS->fv(1).get_asString()));
        m_pDS->next();
      }
      m_pDS->close();

      // fetch duplicate entries
      for (const auto& entry : duplicatesMinMap)
      {
        m_pDS->query(PrepareSQL("SELECT %s FROM %s WHERE name = '%s' AND %s <> %i",
                                tablePk.c_str(), table.c_str(),
                                entry.second.c_str(), tablePk.c_str(), entry.first));

        std::stringstream ids;
        while (!m_pDS->eof())
        {
          int id = m_pDS->fv(0).get_asInt();
          m_pDS->next();

          ids << id;
          if (!m_pDS->eof())
            ids << ",";
        }
        m_pDS->close();

        duplicatesMap.insert(std::make_pair(entry.first, ids.str()));
      }

      // cleanup duplicates in link tables
      for (const auto& subTable : additionalTableEntry.second)
      {
        // create indexes to speed up things
        m_pDS->exec(PrepareSQL("CREATE INDEX ix_%s ON %s (%s)",
                               subTable.c_str(), subTable.c_str(), tablePk.c_str()));

        // migrate every duplicate entry to the main entry
        for (const auto& entry : duplicatesMap)
        {
          m_pDS->exec(PrepareSQL("UPDATE %s SET %s = %i WHERE %s IN (%s) ",
                                 subTable.c_str(), tablePk.c_str(), entry.first,
                                 tablePk.c_str(), entry.second.c_str()));
        }

        // clear all duplicates in the link tables
        if (subTable == "actor_link")
        {
          // as a distinct won't work because of role and cast_order and a group by kills a
          // low powered mysql, we de-dupe it with REPLACE INTO while using the real unique index
          m_pDS->exec("CREATE TABLE temp_actor_link(actor_id INT, media_id INT, media_type TEXT, role TEXT, cast_order INT)");
          m_pDS->exec("CREATE UNIQUE INDEX ix_temp_actor_link ON temp_actor_link (actor_id, media_type(20), media_id)");
          m_pDS->exec("REPLACE INTO temp_actor_link SELECT * FROM actor_link");
          m_pDS->exec("DROP INDEX ix_temp_actor_link ON temp_actor_link");
        }
        else
        {
          m_pDS->exec(PrepareSQL("CREATE TABLE temp_%s AS SELECT DISTINCT * FROM %s",
                                 subTable.c_str(), subTable.c_str()));
        }

        m_pDS->exec(PrepareSQL("DROP TABLE IF EXISTS %s",
                               subTable.c_str()));

        m_pDS->exec(PrepareSQL("ALTER TABLE temp_%s RENAME TO %s",
                               subTable.c_str(), subTable.c_str()));
      }

      // delete duplicates in main table
      for (const auto& entry : duplicatesMap)
      {
        m_pDS->exec(PrepareSQL("DELETE FROM %s WHERE %s IN (%s)",
                               table.c_str(), tablePk.c_str(), entry.second.c_str()));
      }
    }
  }

  if (iVersion < 96)
  {
    m_pDS->exec("ALTER TABLE movie ADD userrating integer");
    m_pDS->exec("ALTER TABLE episode ADD userrating integer");
    m_pDS->exec("ALTER TABLE tvshow ADD userrating integer");
    m_pDS->exec("ALTER TABLE musicvideo ADD userrating integer");
  }

  if (iVersion < 97)
    m_pDS->exec("ALTER TABLE sets ADD strOverview TEXT");

  if (iVersion < 98)
    m_pDS->exec("ALTER TABLE seasons ADD name text");

  if (iVersion < 99)
  {
    // Add idSeason to episode table, so we don't have to join via idShow and season in the future
    m_pDS->exec("ALTER TABLE episode ADD idSeason integer");

    m_pDS->query("SELECT idSeason, idShow, season FROM seasons");
    while (!m_pDS->eof())
    {
      m_pDS2->exec(PrepareSQL("UPDATE episode "
        "SET idSeason = %d "
        "WHERE "
        "episode.idShow = %d AND "
        "episode.c%02d = %d",
        m_pDS->fv(0).get_asInt(), m_pDS->fv(1).get_asInt(),
        VIDEODB_ID_EPISODE_SEASON, m_pDS->fv(2).get_asInt()));

      m_pDS->next();
    }
  }
  if (iVersion < 101)
    m_pDS->exec("ALTER TABLE seasons ADD userrating INTEGER");

  if (iVersion < 102)
  {
    m_pDS->exec("CREATE TABLE rating (rating_id INTEGER PRIMARY KEY, media_id INTEGER, media_type TEXT, rating_type TEXT, rating FLOAT, votes INTEGER)");

    std::string sql = PrepareSQL("SELECT DISTINCT idMovie, c%02d, c%02d FROM movie", VIDEODB_ID_RATING_ID, VIDEODB_ID_VOTES);
    m_pDS->query(sql);
    while (!m_pDS->eof())
    {
      m_pDS2->exec(PrepareSQL("INSERT INTO rating(media_id, media_type, rating_type, rating, votes) VALUES (%i, 'movie', 'default', %f, %i)", m_pDS->fv(0).get_asInt(), (float)strtod(m_pDS->fv(1).get_asString().c_str(), NULL), StringUtils::ReturnDigits(m_pDS->fv(2).get_asString())));
      int idRating = (int)m_pDS2->lastinsertid();
      m_pDS2->exec(PrepareSQL("UPDATE movie SET c%02d=%i WHERE idMovie=%i", VIDEODB_ID_RATING_ID, idRating, m_pDS->fv(0).get_asInt()));
      m_pDS->next();
    }
    m_pDS->close();

    sql = PrepareSQL("SELECT DISTINCT idShow, c%02d, c%02d FROM tvshow", VIDEODB_ID_TV_RATING_ID, VIDEODB_ID_TV_VOTES);
    m_pDS->query(sql);
    while (!m_pDS->eof())
    {
      m_pDS2->exec(PrepareSQL("INSERT INTO rating(media_id, media_type, rating_type, rating, votes) VALUES (%i, 'tvshow', 'default', %f, %i)", m_pDS->fv(0).get_asInt(), (float)strtod(m_pDS->fv(1).get_asString().c_str(), NULL), StringUtils::ReturnDigits(m_pDS->fv(2).get_asString())));
      int idRating = (int)m_pDS2->lastinsertid();
      m_pDS2->exec(PrepareSQL("UPDATE tvshow SET c%02d=%i WHERE idShow=%i", VIDEODB_ID_TV_RATING_ID, idRating, m_pDS->fv(0).get_asInt()));
      m_pDS->next();
    }
    m_pDS->close();

    sql = PrepareSQL("SELECT DISTINCT idEpisode, c%02d, c%02d FROM episode", VIDEODB_ID_EPISODE_RATING_ID, VIDEODB_ID_EPISODE_VOTES);
    m_pDS->query(sql);
    while (!m_pDS->eof())
    {
      m_pDS2->exec(PrepareSQL("INSERT INTO rating(media_id, media_type, rating_type, rating, votes) VALUES (%i, 'episode', 'default', %f, %i)", m_pDS->fv(0).get_asInt(), (float)strtod(m_pDS->fv(1).get_asString().c_str(), NULL), StringUtils::ReturnDigits(m_pDS->fv(2).get_asString())));
      int idRating = (int)m_pDS2->lastinsertid();
      m_pDS2->exec(PrepareSQL("UPDATE episode SET c%02d=%i WHERE idEpisode=%i", VIDEODB_ID_EPISODE_RATING_ID, idRating, m_pDS->fv(0).get_asInt()));
      m_pDS->next();
    }
    m_pDS->close();
  }

  if (iVersion < 103)
  {
    m_pDS->exec("ALTER TABLE settings ADD VideoStream integer");
    m_pDS->exec("ALTER TABLE streamdetails ADD strVideoLanguage text");
  }

  if (iVersion < 104)
  {
    m_pDS->exec("ALTER TABLE tvshow ADD duration INTEGER");

    std::string sql = PrepareSQL( "SELECT episode.idShow, MAX(episode.c%02d) "
                                  "FROM episode "

                                  "LEFT JOIN streamdetails "
                                  "ON streamdetails.idFile = episode.idFile "
                                  "AND streamdetails.iStreamType = 0 " // only grab video streams

                                  "WHERE episode.c%02d <> streamdetails.iVideoDuration "
                                  "OR streamdetails.iVideoDuration IS NULL "
                                  "GROUP BY episode.idShow", VIDEODB_ID_EPISODE_RUNTIME, VIDEODB_ID_EPISODE_RUNTIME);

    m_pDS->query(sql);
    while (!m_pDS->eof())
    {
      m_pDS2->exec(PrepareSQL("UPDATE tvshow SET duration=%i WHERE idShow=%i", m_pDS->fv(1).get_asInt(), m_pDS->fv(0).get_asInt()));
      m_pDS->next();
    }
    m_pDS->close();
  }

  if (iVersion < 105)
  {
    m_pDS->exec("ALTER TABLE movie ADD premiered TEXT");
    m_pDS->exec(PrepareSQL("UPDATE movie SET premiered=c%02d", VIDEODB_ID_YEAR));
    m_pDS->exec("ALTER TABLE musicvideo ADD premiered TEXT");
    m_pDS->exec(PrepareSQL("UPDATE musicvideo SET premiered=c%02d", VIDEODB_ID_MUSICVIDEO_YEAR));
  }

  if (iVersion < 107)
  {
    // need this due to the nested GetScraperPath query
    std::unique_ptr<Dataset> pDS;
    pDS.reset(m_pDB->CreateDataset());
    if (NULL == pDS.get()) return;

    pDS->exec("CREATE TABLE uniqueid (uniqueid_id INTEGER PRIMARY KEY, media_id INTEGER, media_type TEXT, value TEXT, type TEXT)");

    for (int i = 0; i < 3; ++i)
    {
      std::string mediatype, columnID;
      int columnUniqueID;
      switch (i)
      {
      case (0):
        mediatype = "movie";
        columnID = "idMovie";
        columnUniqueID = VIDEODB_ID_IDENT_ID;
        break;
      case (1):
        mediatype = "tvshow";
        columnID = "idShow";
        columnUniqueID = VIDEODB_ID_TV_IDENT_ID;
        break;
      case (2):
        mediatype = "episode";
        columnID = "idEpisode";
        columnUniqueID = VIDEODB_ID_EPISODE_IDENT_ID;
        break;
      default:
        continue;
      }
      pDS->query(PrepareSQL("SELECT %s, c%02d FROM %s", columnID.c_str(), columnUniqueID, mediatype.c_str()));
      while (!pDS->eof())
      {
        std::string uniqueid = pDS->fv(1).get_asString();
        if (!uniqueid.empty())
        {
          int mediaid = pDS->fv(0).get_asInt();
          if (StringUtils::StartsWith(uniqueid, "tt"))
            m_pDS2->exec(PrepareSQL("INSERT INTO uniqueid(media_id, media_type, type, value) VALUES (%i, '%s', 'imdb', '%s')", mediaid, mediatype.c_str(), uniqueid.c_str()));
          else
            m_pDS2->exec(PrepareSQL("INSERT INTO uniqueid(media_id, media_type, type, value) VALUES (%i, '%s', 'unknown', '%s')", mediaid, mediatype.c_str(), uniqueid.c_str()));
          m_pDS2->exec(PrepareSQL("UPDATE %s SET c%02d='%i' WHERE %s=%i", mediatype.c_str(), columnUniqueID, (int)m_pDS2->lastinsertid(), columnID.c_str(), mediaid));
        }
        pDS->next();
      }
      pDS->close();
    }
  }

  if (iVersion < 109)
  {
    m_pDS->exec("ALTER TABLE settings RENAME TO settingsold");
    m_pDS->exec("CREATE TABLE settings ( idFile integer, Deinterlace bool,"
                "ViewMode integer,ZoomAmount float, PixelRatio float, VerticalShift float, AudioStream integer, SubtitleStream integer,"
                "SubtitleDelay float, SubtitlesOn bool, Brightness float, Contrast float, Gamma float,"
                "VolumeAmplification float, AudioDelay float, ResumeTime integer,"
                "Sharpness float, NoiseReduction float, NonLinStretch bool, PostProcess bool,"
                "ScalingMethod integer, DeinterlaceMode integer, StereoMode integer, StereoInvert bool, VideoStream integer)");
    m_pDS->exec("INSERT INTO settings SELECT idFile, Deinterlace, ViewMode, ZoomAmount, PixelRatio, VerticalShift, AudioStream, SubtitleStream, SubtitleDelay, SubtitlesOn, Brightness, Contrast, Gamma, VolumeAmplification, AudioDelay, ResumeTime, Sharpness, NoiseReduction, NonLinStretch, PostProcess, ScalingMethod, DeinterlaceMode, StereoMode, StereoInvert, VideoStream FROM settingsold");
    m_pDS->exec("DROP TABLE settingsold");
  }

  if (iVersion < 110)
  {
    m_pDS->exec("ALTER TABLE settings ADD TonemapMethod integer");
    m_pDS->exec("ALTER TABLE settings ADD TonemapParam float");
  }

  if (iVersion < 111)
    m_pDS->exec("ALTER TABLE settings ADD Orientation integer");

  if (iVersion < 112)
    m_pDS->exec("ALTER TABLE settings ADD CenterMixLevel integer");

  if (iVersion < 113)
  {
    // fb9c25f5 and e5f6d204 changed the behavior of path splitting for plugin URIs (previously it would only use the root)
    // Re-split paths for plugin files in order to maintain watched state etc.
    m_pDS->query("SELECT files.idFile, files.strFilename, path.strPath FROM files LEFT JOIN path ON files.idPath = path.idPath WHERE files.strFilename LIKE 'plugin://%'");
    while (!m_pDS->eof())
    {
      std::string path, fn;
      SplitPath(m_pDS->fv(1).get_asString(), path, fn);
      if (path != m_pDS->fv(2).get_asString())
      {
        int pathid = -1;
        m_pDS2->query(PrepareSQL("SELECT idPath FROM path WHERE strPath='%s'", path.c_str()));
        if (!m_pDS2->eof())
          pathid = m_pDS2->fv(0).get_asInt();
        m_pDS2->close();
        if (pathid < 0)
        {
          std::string parent = URIUtils::GetParentPath(path);
          int parentid = -1;
          m_pDS2->query(PrepareSQL("SELECT idPath FROM path WHERE strPath='%s'", parent.c_str()));
          if (!m_pDS2->eof())
            parentid = m_pDS2->fv(0).get_asInt();
          m_pDS2->close();
          if (parentid < 0)
          {
            m_pDS2->exec(PrepareSQL("INSERT INTO path (strPath) VALUES ('%s')", parent.c_str()));
            parentid = (int)m_pDS2->lastinsertid();
          }
          m_pDS2->exec(PrepareSQL("INSERT INTO path (strPath, idParentPath) VALUES ('%s', %i)", path.c_str(), parentid));
          pathid = (int)m_pDS2->lastinsertid();
        }
        m_pDS2->query(PrepareSQL("SELECT idFile FROM files WHERE strFileName='%s' AND idPath=%i", fn.c_str(), pathid));
        bool exists = !m_pDS2->eof();
        m_pDS2->close();
        if (exists)
          m_pDS2->exec(PrepareSQL("DELETE FROM files WHERE idFile=%i", m_pDS->fv(0).get_asInt()));
        else
          m_pDS2->exec(PrepareSQL("UPDATE files SET idPath=%i WHERE idFile=%i", pathid, m_pDS->fv(0).get_asInt()));
      }
      m_pDS->next();
    }
    m_pDS->close();
  }
}

void CVideoDatabase::CleanDatabase(CGUIDialogProgressBarHandle* handle, const std::set<int>& paths, bool showProgress)
{
  CGUIDialogProgress *progress=NULL;
  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;
    if (NULL == m_pDS2.get()) return;

    unsigned int time = XbmcThreads::SystemClockMillis();
    CLog::Log(LOGNOTICE, "%s: Starting videodatabase cleanup ..", __FUNCTION__);
    CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::VideoLibrary, "xbmc", "OnCleanStarted");

    BeginTransaction();

    // find all the files
    std::string sql = "SELECT files.idFile, files.strFileName, path.strPath FROM files INNER JOIN path ON path.idPath=files.idPath";
    if (!paths.empty())
    {
      std::string strPaths;
      for (const auto &i : paths)
        strPaths += StringUtils::Format(",%i", i);
      sql += PrepareSQL(" AND path.idPath IN (%s)", strPaths.substr(1).c_str());
    }

    m_pDS2->query(sql);
    if (m_pDS2->num_rows() == 0) return;

    if (handle)
    {
      handle->SetTitle(g_localizeStrings.Get(700));
      handle->SetText("");
    }
    else if (showProgress)
    {
      progress = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogProgress>(WINDOW_DIALOG_PROGRESS);
      if (progress)
      {
        progress->SetHeading(CVariant{700});
        progress->SetLine(0, CVariant{""});
        progress->SetLine(1, CVariant{313});
        progress->SetLine(2, CVariant{330});
        progress->SetPercentage(0);
        progress->Open();
        progress->ShowProgressBar(true);
      }
    }

    std::string filesToTestForDelete;
    VECSOURCES videoSources(*CMediaSourceSettings::GetInstance().GetSources("video"));
    g_mediaManager.GetRemovableDrives(videoSources);

    int total = m_pDS2->num_rows();
    int current = 0;

    while (!m_pDS2->eof())
    {
      std::string path = m_pDS2->fv("path.strPath").get_asString();
      std::string fileName = m_pDS2->fv("files.strFileName").get_asString();
      std::string fullPath;
      ConstructPath(fullPath, path, fileName);

      // get the first stacked file
      if (URIUtils::IsStack(fullPath))
        fullPath = CStackDirectory::GetFirstStackedFile(fullPath);

      // get the actual archive path
      if (URIUtils::IsInArchive(fullPath))
        fullPath = CURL(fullPath).GetHostName();

      bool del = true;
      if (URIUtils::IsPlugin(fullPath))
      {
        SScanSettings settings;
        bool foundDirectly = false;
        ScraperPtr scraper = GetScraperForPath(fullPath, settings, foundDirectly);
        if (scraper && CPluginDirectory::CheckExists(TranslateContent(scraper->Content()), fullPath))
          del = false;
      }
      else
      {
        // remove optical, non-existing files, files with no matching source
        bool bIsSource;
        if (!URIUtils::IsOnDVD(fullPath) && CFile::Exists(fullPath, false) &&
            CUtil::GetMatchingSource(fullPath, videoSources, bIsSource) >= 0)
          del = false;
      }
      if (del)
        filesToTestForDelete += m_pDS2->fv("files.idFile").get_asString() + ",";

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
          m_pDS2->close();
          CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::VideoLibrary, "xbmc", "OnCleanFinished");
          return;
        }
      }
      else if (handle != NULL)
        handle->SetPercentage(current * 100 / (float)total);

      m_pDS2->next();
      current++;
    }
    m_pDS2->close();

    std::string filesToDelete;

    // Add any files that don't have a valid idPath entry to the filesToDelete list.
    m_pDS->query("SELECT files.idFile FROM files WHERE NOT EXISTS (SELECT 1 FROM path WHERE path.idPath = files.idPath)");
    while (!m_pDS->eof())
    {
      std::string file = m_pDS->fv("files.idFile").get_asString() + ",";
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

      // Clean hashes of all paths that files are deleted from
      // Otherwise there is a mismatch between the path contents and the hash in the
      // database, leading to potentially missed items on re-scan (if deleted files are
      // later re-added to a source)
      CLog::LogF(LOGDEBUG, LOGDATABASE, "Cleaning path hashes");
      m_pDS->query("SELECT DISTINCT strPath FROM path JOIN files ON files.idPath=path.idPath WHERE files.idFile IN " + filesToDelete);
      int pathHashCount = m_pDS->num_rows();
      while (!m_pDS->eof())
      {
        InvalidatePathHash(m_pDS->fv("strPath").get_asString());
        m_pDS->next();
      }
      CLog::LogF(LOGDEBUG, LOGDATABASE, "Cleaned {} path hashes", pathHashCount);

      CLog::Log(LOGDEBUG, LOGDATABASE, "%s: Cleaning files table", __FUNCTION__);
      sql = "DELETE FROM files WHERE idFile IN " + filesToDelete;
      m_pDS->exec(sql);
    }

    if (!movieIDs.empty())
    {
      std::string moviesToDelete;
      for (const auto &i : movieIDs)
        moviesToDelete += StringUtils::Format("%i,", i);
      moviesToDelete = "(" + StringUtils::TrimRight(moviesToDelete, ",") + ")";

      CLog::Log(LOGDEBUG, LOGDATABASE, "%s: Cleaning movie table", __FUNCTION__);
      sql = "DELETE FROM movie WHERE idMovie IN " + moviesToDelete;
      m_pDS->exec(sql);
    }

    if (!episodeIDs.empty())
    {
      std::string episodesToDelete;
      for (const auto &i : episodeIDs)
        episodesToDelete += StringUtils::Format("%i,", i);
      episodesToDelete = "(" + StringUtils::TrimRight(episodesToDelete, ",") + ")";

      CLog::Log(LOGDEBUG, LOGDATABASE, "%s: Cleaning episode table", __FUNCTION__);
      sql = "DELETE FROM episode WHERE idEpisode IN " + episodesToDelete;
      m_pDS->exec(sql);
    }

    CLog::Log(LOGDEBUG, LOGDATABASE, "%s: Cleaning paths that don't exist and have content set...", __FUNCTION__);
    sql = "SELECT path.idPath, path.strPath, path.idParentPath FROM path "
            "WHERE NOT ((strContent IS NULL OR strContent = '') "
                   "AND (strSettings IS NULL OR strSettings = '') "
                   "AND (strHash IS NULL OR strHash = '') "
                   "AND (exclude IS NULL OR exclude != 1))";
    m_pDS2->query(sql);
    std::string strIds;
    while (!m_pDS2->eof())
    {
      auto pathsDeleteDecision = pathsDeleteDecisions.find(m_pDS2->fv(0).get_asInt());
      // Check if we have a decision for the parent path
      auto pathsDeleteDecisionByParent = pathsDeleteDecisions.find(m_pDS2->fv(2).get_asInt());
      std::string path = m_pDS2->fv(1).get_asString();

      bool exists = false;
      if (URIUtils::IsPlugin(path))
      {
        SScanSettings settings;
        bool foundDirectly = false;
        ScraperPtr scraper = GetScraperForPath(path, settings, foundDirectly);
        if (scraper && CPluginDirectory::CheckExists(TranslateContent(scraper->Content()), path))
          exists = true;
      }
      else
        exists = CDirectory::Exists(path, false);

      if (((pathsDeleteDecision != pathsDeleteDecisions.end() && pathsDeleteDecision->second) ||
           (pathsDeleteDecision == pathsDeleteDecisions.end() && !exists)) &&
          ((pathsDeleteDecisionByParent != pathsDeleteDecisions.end() && pathsDeleteDecisionByParent->second) ||
           (pathsDeleteDecisionByParent == pathsDeleteDecisions.end())))
        strIds += m_pDS2->fv(0).get_asString() + ",";

      m_pDS2->next();
    }
    m_pDS2->close();

    if (!strIds.empty())
    {
      sql = PrepareSQL("DELETE FROM path WHERE idPath IN (%s)", StringUtils::TrimRight(strIds, ",").c_str());
      m_pDS->exec(sql);
      sql = "DELETE FROM tvshowlinkpath WHERE NOT EXISTS (SELECT 1 FROM path WHERE path.idPath = tvshowlinkpath.idPath)";
      m_pDS->exec(sql);
    }

    CLog::Log(LOGDEBUG, LOGDATABASE, "%s: Cleaning tvshow table", __FUNCTION__);

    std::string tvshowsToDelete;
    sql = "SELECT idShow FROM tvshow WHERE NOT EXISTS (SELECT 1 FROM tvshowlinkpath WHERE tvshowlinkpath.idShow = tvshow.idShow)";
    m_pDS->query(sql);
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
      m_pDS->exec(sql);
    }

    if (!musicVideoIDs.empty())
    {
      std::string musicVideosToDelete;
      for (const auto &i : musicVideoIDs)
        musicVideosToDelete += StringUtils::Format("%i,", i);
      musicVideosToDelete = "(" + StringUtils::TrimRight(musicVideosToDelete, ",") + ")";

      CLog::Log(LOGDEBUG, LOGDATABASE, "%s: Cleaning musicvideo table", __FUNCTION__);
      sql = "DELETE FROM musicvideo WHERE idMVideo IN " + musicVideosToDelete;
      m_pDS->exec(sql);
    }

    CLog::Log(LOGDEBUG, LOGDATABASE, "%s: Cleaning path table", __FUNCTION__);
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
    m_pDS->exec(sql);

    CLog::Log(LOGDEBUG, LOGDATABASE, "%s: Cleaning genre table", __FUNCTION__);
    sql = "DELETE FROM genre "
            "WHERE NOT EXISTS (SELECT 1 FROM genre_link WHERE genre_link.genre_id = genre.genre_id)";
    m_pDS->exec(sql);

    CLog::Log(LOGDEBUG, LOGDATABASE, "%s: Cleaning country table", __FUNCTION__);
    sql = "DELETE FROM country WHERE NOT EXISTS (SELECT 1 FROM country_link WHERE country_link.country_id = country.country_id)";
    m_pDS->exec(sql);

    CLog::Log(LOGDEBUG, LOGDATABASE, "%s: Cleaning actor table of actors, directors and writers", __FUNCTION__);
    sql = "DELETE FROM actor "
            "WHERE NOT EXISTS (SELECT 1 FROM actor_link WHERE actor_link.actor_id = actor.actor_id) "
              "AND NOT EXISTS (SELECT 1 FROM director_link WHERE director_link.actor_id = actor.actor_id) "
              "AND NOT EXISTS (SELECT 1 FROM writer_link WHERE writer_link.actor_id = actor.actor_id)";
    m_pDS->exec(sql);

    CLog::Log(LOGDEBUG, LOGDATABASE, "%s: Cleaning studio table", __FUNCTION__);
    sql = "DELETE FROM studio "
            "WHERE NOT EXISTS (SELECT 1 FROM studio_link WHERE studio_link.studio_id = studio.studio_id)";
    m_pDS->exec(sql);

    CLog::Log(LOGDEBUG, LOGDATABASE, "%s: Cleaning set table", __FUNCTION__);
    sql = "DELETE FROM sets WHERE NOT EXISTS (SELECT 1 FROM movie WHERE movie.idSet = sets.idSet)";
    m_pDS->exec(sql);

    CommitTransaction();

    if (handle)
      handle->SetTitle(g_localizeStrings.Get(331));

    Compress(false);

    CUtil::DeleteVideoDatabaseDirectoryCache();

    time = XbmcThreads::SystemClockMillis() - time;
    CLog::Log(LOGNOTICE, "%s: Cleaning videodatabase done. Operation took %s", __FUNCTION__, StringUtils::SecondsToTimeString(time / 1000).c_str());

    for (const auto &i : movieIDs)
      AnnounceRemove(MediaTypeMovie, i, true);

    for (const auto &i : episodeIDs)
      AnnounceRemove(MediaTypeEpisode, i, true);

    for (const auto &i : tvshowIDs)
      AnnounceRemove(MediaTypeTvShow, i, true);

    for (const auto &i : musicVideoIDs)
      AnnounceRemove(MediaTypeMusicVideo, i, true);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
    RollbackTransaction();
  }
  if (progress)
    progress->Close();

  CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::VideoLibrary, "xbmc", "OnCleanFinished");
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

  sql += PrepareSQL("LEFT JOIN path as parentPath ON parentPath.idPath = %s "
                    "WHERE %s.idFile IN (%s)",
                    parentPathIdField.c_str(),
                    table.c_str(), cleanableFileIDs.c_str());

  VECSOURCES videoSources(*CMediaSourceSettings::GetInstance().GetSources("video"));
  g_mediaManager.GetRemovableDrives(videoSources);

  // map of parent path ID to boolean pair (if not exists and user choice)
  std::map<int, std::pair<bool, bool> > sourcePathsDeleteDecisions;
  m_pDS2->query(sql);
  while (!m_pDS2->eof())
  {
    bool del = true;
    if (m_pDS2->fv(3).get_isNull() == false)
    {
      std::string parentPath = m_pDS2->fv(3).get_asString();

      // try to find the source path the parent path belongs to
      SScanSettings scanSettings;
      std::string sourcePath;
      GetSourcePath(parentPath, sourcePath, scanSettings);

      bool bIsSourceName;
      bool sourceNotFound = (CUtil::GetMatchingSource(parentPath, videoSources, bIsSourceName) < 0);

      if (sourceNotFound && sourcePath.empty())
        sourcePath = parentPath;

      int sourcePathID = GetPathId(sourcePath);
      auto sourcePathsDeleteDecision = sourcePathsDeleteDecisions.find(sourcePathID);
      if (sourcePathsDeleteDecision == sourcePathsDeleteDecisions.end())
      {
        bool sourcePathNotExists = (sourceNotFound || !CDirectory::Exists(sourcePath, false));
        // if the parent path exists, the file will be deleted without asking
        // if the parent path doesn't exist or does not belong to a valid media source,
        // ask the user whether to remove all items it contained
        if (sourcePathNotExists)
        {
          // in silent mode assume that the files are just temporarily missing
          if (silent)
            del = false;
          else
          {
            CGUIDialogYesNo* pDialog = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogYesNo>(WINDOW_DIALOG_YES_NO);
            if (pDialog != NULL)
            {
              CURL sourceUrl(sourcePath);
              pDialog->SetHeading(CVariant{15012});
              pDialog->SetText(CVariant{StringUtils::Format(g_localizeStrings.Get(15013).c_str(), sourceUrl.GetWithoutUserDetails().c_str())});
              pDialog->SetChoice(0, CVariant{15015});
              pDialog->SetChoice(1, CVariant{15014});
              pDialog->Open();

              del = !pDialog->IsConfirmed();
            }
          }
        }

        sourcePathsDeleteDecisions.insert(std::make_pair(sourcePathID, std::make_pair(sourcePathNotExists, del)));
        pathsDeleteDecisions.insert(std::make_pair(sourcePathID, sourcePathNotExists && del));
      }
      // the only reason not to delete the file is if the parent path doesn't
      // exist and the user decided to delete all the items it contained
      else if (sourcePathsDeleteDecision->second.first &&
               !sourcePathsDeleteDecision->second.second)
        del = false;

      if (scanSettings.parent_name)
        pathsDeleteDecisions.insert(std::make_pair(m_pDS2->fv(2).get_asInt(), del));
    }

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
