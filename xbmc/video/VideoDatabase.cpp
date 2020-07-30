/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoDatabase.h"

#include "Application.h"
#include "FileItem.h"
#include "GUIInfoManager.h"
#include "GUIPassword.h"
#include "ServiceBroker.h"
#include "TextureCache.h"
#include "URL.h"
#include "Util.h"
#include "VideoInfoScanner.h"
#include "XBDateTime.h"
#include "addons/AddonManager.h"
#include "dbwrappers/dataset.h"
#include "dialogs/GUIDialogExtendedProgressBar.h"
#include "dialogs/GUIDialogKaiToast.h"
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
#include "guilib/guiinfo/GUIInfoLabels.h"
#include "interfaces/AnnouncementManager.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "playlists/SmartPlayList.h"
#include "profiles/ProfileManager.h"
#include "settings/AdvancedSettings.h"
#include "settings/MediaSettings.h"
#include "settings/MediaSourceSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "storage/MediaManager.h"
#include "threads/SystemClock.h"
#include "utils/FileUtils.h"
#include "utils/GroupUtils.h"
#include "utils/LabelFormatter.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/XMLUtils.h"
#include "utils/log.h"
#include "video/VideoDbUrl.h"
#include "video/VideoInfoTag.h"
#include "video/windows/GUIWindowVideoBase.h"

#include <algorithm>
#include <map>
#include <memory>
#include <string>
#include <vector>

using namespace dbiplus;
using namespace XFILE;
using namespace VIDEO;
using namespace ADDON;
using namespace KODI::MESSAGING;
using namespace KODI::GUILIB;

//********************************************************************************************************************************
CVideoDatabase::CVideoDatabase(void) = default;

//********************************************************************************************************************************
CVideoDatabase::~CVideoDatabase(void) = default;

//********************************************************************************************************************************
bool CVideoDatabase::Open()
{
  return CDatabase::Open(CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_databaseVideo);
}

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
  m_pDS->exec(
      "CREATE TABLE path ( idPath integer primary key, strPath text, strContent text, strScraper "
      "text, strHash text, scanRecursive integer, useFolderNames bool, strSettings text, noUpdate "
      "bool, exclude bool, allAudio bool, dateAdded text, idParentPath integer)");

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

  m_pDS->exec("CREATE UNIQUE INDEX ix_actor_1 ON actor (name(255))");
  m_pDS->exec("CREATE UNIQUE INDEX ix_actor_link_1 ON "
              "actor_link (actor_id, media_type(20), media_id, role(255))");
  m_pDS->exec("CREATE INDEX ix_actor_link_2 ON "
              "actor_link (media_id, media_type(20), actor_id)");
  m_pDS->exec("CREATE INDEX ix_actor_link_3 ON actor_link (media_type(20))");

  CreateLinkIndex("tag");
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

//********************************************************************************************************************************
int CVideoDatabase::GetPathId(const std::string& strPath)
{
  std::string strSQL;
  try
  {
    int idPath=-1;
    if (nullptr == m_pDB)
      return -1;
    if (nullptr == m_pDS)
      return -1;

    std::string strPath1(strPath);
    if (URIUtils::IsStack(strPath) || StringUtils::StartsWithNoCase(strPath, "rar://") || StringUtils::StartsWithNoCase(strPath, "zip://"))
      URIUtils::GetParentPath(strPath,strPath1);

    URIUtils::AddSlashAtEnd(strPath1);

    strSQL=PrepareSQL("select idPath from path where strPath='%s'",strPath1.c_str());
    m_pDS->query(strSQL);
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

bool CVideoDatabase::GetPaths(std::set<std::string> &paths)
{
  try
  {
    if (nullptr == m_pDB)
      return false;
    if (nullptr == m_pDS)
      return false;

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

bool CVideoDatabase::GetPathsLinkedToTvShow(int idShow, std::vector<std::string> &paths)
{
  std::string sql;
  try
  {
    sql = PrepareSQL("SELECT strPath FROM path JOIN tvshowlinkpath ON tvshowlinkpath.idPath=path.idPath WHERE idShow=%i", idShow);
    m_pDS->query(sql);
    while (!m_pDS->eof())
    {
      paths.emplace_back(m_pDS->fv(0).get_asString());
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

bool CVideoDatabase::GetPathsForTvShow(int idShow, std::set<int>& paths)
{
  std::string strSQL;
  try
  {
    if (nullptr == m_pDB)
      return false;
    if (nullptr == m_pDS)
      return false;

    // add base path
    strSQL = PrepareSQL("SELECT strPath FROM tvshow_view WHERE idShow=%i", idShow);
    if (m_pDS->query(strSQL))
      paths.insert(GetPathId(m_pDS->fv(0).get_asString()));

    // add all other known paths
    strSQL = PrepareSQL("SELECT DISTINCT idPath FROM files JOIN episode ON episode.idFile=files.idFile WHERE episode.idShow=%i",idShow);
    m_pDS->query(strSQL);
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

int CVideoDatabase::RunQuery(const std::string &sql)
{
  unsigned int time = XbmcThreads::SystemClockMillis();
  int rows = -1;
  if (m_pDS->query(sql))
  {
    rows = m_pDS->num_rows();
    if (rows == 0)
      m_pDS->close();
  }
  CLog::Log(LOGDEBUG, LOGDATABASE, "%s took %d ms for %d items query: %s", __FUNCTION__, XbmcThreads::SystemClockMillis() - time, rows, sql.c_str());
  return rows;
}

bool CVideoDatabase::GetSubPaths(const std::string &basepath, std::vector<std::pair<int, std::string>>& subpaths)
{
  std::string sql;
  try
  {
    if (!m_pDB || !m_pDS)
      return false;

    std::string path(basepath);
    URIUtils::AddSlashAtEnd(path);
    sql = PrepareSQL("SELECT idPath,strPath FROM path WHERE SUBSTR(strPath,1,%i)='%s'"
                     " AND idPath NOT IN (SELECT idPath FROM files WHERE strFileName LIKE 'video_ts.ifo')"
                     " AND idPath NOT IN (SELECT idPath FROM files WHERE strFileName LIKE 'index.bdmv')"
                     , StringUtils::utf8_strlen(path.c_str()), path.c_str());

    m_pDS->query(sql);
    while (!m_pDS->eof())
    {
      subpaths.emplace_back(m_pDS->fv(0).get_asInt(), m_pDS->fv(1).get_asString());
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

int CVideoDatabase::AddPath(const std::string& strPath, const std::string &parentPath /*= "" */, const CDateTime& dateAdded /* = CDateTime() */)
{
  std::string strSQL;
  try
  {
    int idPath = GetPathId(strPath);
    if (idPath >= 0)
      return idPath; // already have the path

    if (nullptr == m_pDB)
      return -1;
    if (nullptr == m_pDS)
      return -1;

    std::string strPath1(strPath);
    if (URIUtils::IsStack(strPath) || StringUtils::StartsWithNoCase(strPath, "rar://") || StringUtils::StartsWithNoCase(strPath, "zip://"))
      URIUtils::GetParentPath(strPath,strPath1);

    URIUtils::AddSlashAtEnd(strPath1);

    int idParentPath = GetPathId(parentPath.empty() ? URIUtils::GetParentPath(strPath1) : parentPath);

    // add the path
    if (idParentPath < 0)
    {
      if (dateAdded.IsValid())
        strSQL=PrepareSQL("insert into path (idPath, strPath, dateAdded) values (NULL, '%s', '%s')", strPath1.c_str(), dateAdded.GetAsDBDateTime().c_str());
      else
        strSQL=PrepareSQL("insert into path (idPath, strPath) values (NULL, '%s')", strPath1.c_str());
    }
    else
    {
      if (dateAdded.IsValid())
        strSQL = PrepareSQL("insert into path (idPath, strPath, dateAdded, idParentPath) values (NULL, '%s', '%s', %i)", strPath1.c_str(), dateAdded.GetAsDBDateTime().c_str(), idParentPath);
      else
        strSQL=PrepareSQL("insert into path (idPath, strPath, idParentPath) values (NULL, '%s', %i)", strPath1.c_str(), idParentPath);
    }
    m_pDS->exec(strSQL);
    idPath = (int)m_pDS->lastinsertid();
    return idPath;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s unable to addpath (%s)", __FUNCTION__, strSQL.c_str());
  }
  return -1;
}

bool CVideoDatabase::GetPathHash(const std::string &path, std::string &hash)
{
  try
  {
    if (nullptr == m_pDB)
      return false;
    if (nullptr == m_pDS)
      return false;

    std::string strSQL=PrepareSQL("select strHash from path where strPath='%s'", path.c_str());
    m_pDS->query(strSQL);
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
    if (path.empty() || m_pDB == nullptr || m_pDS == nullptr)
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
      if (m_pDS->query(strSQL) && !m_pDS->eof())
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
    std::string strParent;
    bool found = false;
    while (URIUtils::GetParentPath(strPath1, strParent))
    {
      std::string strSQL = PrepareSQL("SELECT path.strContent, path.strScraper, path.scanRecursive, path.useFolderNames, path.noUpdate, path.exclude FROM path WHERE strPath = '%s'", strParent.c_str());
      if (m_pDS->query(strSQL) && !m_pDS->eof())
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
int CVideoDatabase::AddFile(const std::string& strFileNameAndPath)
{
  std::string strSQL = "";
  try
  {
    int idFile;
    if (nullptr == m_pDB)
      return -1;
    if (nullptr == m_pDS)
      return -1;

    std::string strFileName, strPath;
    SplitPath(strFileNameAndPath,strPath,strFileName);

    int idPath = AddPath(strPath);
    if (idPath < 0)
      return -1;

    std::string strSQL=PrepareSQL("select idFile from files where strFileName='%s' and idPath=%i", strFileName.c_str(),idPath);

    m_pDS->query(strSQL);
    if (m_pDS->num_rows() > 0)
    {
      idFile = m_pDS->fv("idFile").get_asInt() ;
      m_pDS->close();
      return idFile;
    }
    m_pDS->close();

    strSQL=PrepareSQL("insert into files (idFile, idPath, strFileName) values(NULL, %i, '%s')", idPath, strFileName.c_str());
    m_pDS->exec(strSQL);
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

void CVideoDatabase::UpdateFileDateAdded(int idFile, const std::string& strFileNameAndPath, const CDateTime& dateAdded /* = CDateTime() */)
{
  if (idFile < 0 || strFileNameAndPath.empty())
    return;

  CDateTime finalDateAdded = dateAdded;
  try
  {
    if (nullptr == m_pDB)
      return;
    if (nullptr == m_pDS)
      return;

    if (!finalDateAdded.IsValid())
    {
      // Supress warnings if we have plugin source
      if (!URIUtils::IsPlugin(strFileNameAndPath))
      {
        // 1 preferring to use the files mtime(if it's valid) and only using the file's ctime if the mtime isn't valid
        if (CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_iVideoLibraryDateAdded == 1)
          finalDateAdded = CFileUtils::GetModificationDate(strFileNameAndPath, false);
        //2 using the newer datetime of the file's mtime and ctime
        else if (CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_iVideoLibraryDateAdded == 2)
          finalDateAdded = CFileUtils::GetModificationDate(strFileNameAndPath, true);
      }
      //0 using the current datetime if non of the above matches or one returns an invalid datetime
      if (!finalDateAdded.IsValid())
        finalDateAdded = CDateTime::GetCurrentDateTime();
    }

    m_pDS->exec(PrepareSQL("UPDATE files SET dateAdded='%s' WHERE idFile=%d", finalDateAdded.GetAsDBDateTime().c_str(), idFile));
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s, %s) failed", __FUNCTION__, CURL::GetRedacted(strFileNameAndPath).c_str(), finalDateAdded.GetAsDBDateTime().c_str());
  }
}

bool CVideoDatabase::SetPathHash(const std::string &path, const std::string &hash)
{
  try
  {
    if (nullptr == m_pDB)
      return false;
    if (nullptr == m_pDS)
      return false;

    int idPath = AddPath(path);
    if (idPath < 0) return false;

    std::string strSQL=PrepareSQL("update path set strHash='%s' where idPath=%ld", hash.c_str(), idPath);
    m_pDS->exec(strSQL);

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
    if (nullptr == m_pDB)
      return false;
    if (nullptr == m_pDS)
      return false;

    if (bRemove) // delete link
    {
      std::string strSQL=PrepareSQL("delete from movielinktvshow where idMovie=%i and idShow=%i", idMovie, idShow);
      m_pDS->exec(strSQL);
      return true;
    }

    std::string strSQL=PrepareSQL("insert into movielinktvshow (idShow,idMovie) values (%i,%i)", idShow,idMovie);
    m_pDS->exec(strSQL);

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
    if (nullptr == m_pDB)
      return false;
    if (nullptr == m_pDS)
      return false;

    std::string strSQL=PrepareSQL("select * from movielinktvshow where idMovie=%i", idMovie);
    m_pDS->query(strSQL);
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

bool CVideoDatabase::GetLinksToTvShow(int idMovie, std::vector<int>& ids)
{
   try
  {
    if (nullptr == m_pDB)
      return false;
    if (nullptr == m_pDS)
      return false;

    std::string strSQL=PrepareSQL("select * from movielinktvshow where idMovie=%i", idMovie);
    m_pDS2->query(strSQL);
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
int CVideoDatabase::GetFileId(const std::string& strFilenameAndPath)
{
  try
  {
    if (nullptr == m_pDB)
      return -1;
    if (nullptr == m_pDS)
      return -1;
    std::string strPath, strFileName;
    SplitPath(strFilenameAndPath,strPath,strFileName);

    int idPath = GetPathId(strPath);
    if (idPath >= 0)
    {
      std::string strSQL;
      strSQL=PrepareSQL("select idFile from files where strFileName='%s' and idPath=%i", strFileName.c_str(),idPath);
      m_pDS->query(strSQL);
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
int CVideoDatabase::GetMovieId(const std::string& strFilenameAndPath)
{
  try
  {
    if (nullptr == m_pDB)
      return -1;
    if (nullptr == m_pDS)
      return -1;
    int idMovie = -1;

    // needed for query parameters
    int idFile = GetFileId(strFilenameAndPath);
    int idPath=-1;
    std::string strPath;
    if (idFile < 0)
    {
      std::string strFile;
      SplitPath(strFilenameAndPath,strPath,strFile);

      // have to join movieinfo table for correct results
      idPath = GetPathId(strPath);
      if (idPath < 0 && strPath != strFilenameAndPath)
        return -1;
    }

    if (idFile == -1 && strPath != strFilenameAndPath)
      return -1;

    std::string strSQL;
    if (idFile == -1)
      strSQL=PrepareSQL("select idMovie from movie join files on files.idFile=movie.idFile where files.idPath=%i",idPath);
    else
      strSQL=PrepareSQL("select idMovie from movie where idFile=%i", idFile);

    CLog::Log(LOGDEBUG, LOGDATABASE, "%s (%s), query = %s", __FUNCTION__, CURL::GetRedacted(strFilenameAndPath).c_str(), strSQL.c_str());
    m_pDS->query(strSQL);
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

int CVideoDatabase::GetTvShowId(const std::string& strPath)
{
  try
  {
    if (nullptr == m_pDB)
      return -1;
    if (nullptr == m_pDS)
      return -1;
    int idTvShow = -1;

    // have to join movieinfo table for correct results
    int idPath = GetPathId(strPath);
    if (idPath < 0)
      return -1;

    std::string strSQL;
    std::string strPath1=strPath;
    std::string strParent;
    int iFound=0;

    strSQL=PrepareSQL("select idShow from tvshowlinkpath where tvshowlinkpath.idPath=%i",idPath);
    m_pDS->query(strSQL);
    if (!m_pDS->eof())
      iFound = 1;

    while (iFound == 0 && URIUtils::GetParentPath(strPath1, strParent))
    {
      strSQL=PrepareSQL("SELECT idShow FROM path INNER JOIN tvshowlinkpath ON tvshowlinkpath.idPath=path.idPath WHERE strPath='%s'",strParent.c_str());
      m_pDS->query(strSQL);
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

int CVideoDatabase::GetEpisodeId(const std::string& strFilenameAndPath, int idEpisode, int idSeason) // input value is episode/season number hint - for multiparters
{
  try
  {
    if (nullptr == m_pDB)
      return -1;
    if (nullptr == m_pDS)
      return -1;

    // need this due to the nested GetEpisodeInfo query
    std::unique_ptr<Dataset> pDS;
    pDS.reset(m_pDB->CreateDataset());
    if (nullptr == pDS)
      return -1;

    int idFile = GetFileId(strFilenameAndPath);
    if (idFile < 0)
      return -1;

    std::string strSQL=PrepareSQL("select idEpisode from episode where idFile=%i", idFile);

    CLog::Log(LOGDEBUG, LOGDATABASE, "%s (%s), query = %s", __FUNCTION__, CURL::GetRedacted(strFilenameAndPath).c_str(), strSQL.c_str());
    pDS->query(strSQL);
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
          GetEpisodeBasicInfo(strFilenameAndPath, tag, idTmpEpisode);
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

int CVideoDatabase::GetMusicVideoId(const std::string& strFilenameAndPath)
{
  try
  {
    if (nullptr == m_pDB)
      return -1;
    if (nullptr == m_pDS)
      return -1;

    int idFile = GetFileId(strFilenameAndPath);
    if (idFile < 0)
      return -1;

    std::string strSQL=PrepareSQL("select idMVideo from musicvideo where idFile=%i", idFile);

    CLog::Log(LOGDEBUG, LOGDATABASE, "%s (%s), query = %s", __FUNCTION__, CURL::GetRedacted(strFilenameAndPath).c_str(), strSQL.c_str());
    m_pDS->query(strSQL);
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
int CVideoDatabase::AddMovie(const std::string& strFilenameAndPath)
{
  try
  {
    if (nullptr == m_pDB)
      return -1;
    if (nullptr == m_pDS)
      return -1;

    int idMovie = GetMovieId(strFilenameAndPath);
    if (idMovie < 0)
    {
      int idFile = AddFile(strFilenameAndPath);
      if (idFile < 0)
        return -1;
      UpdateFileDateAdded(idFile, strFilenameAndPath);
      std::string strSQL=PrepareSQL("insert into movie (idMovie, idFile) values (NULL, %i)", idFile);
      m_pDS->exec(strSQL);
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

bool CVideoDatabase::AddPathToTvShow(int idShow, const std::string &path, const std::string &parentPath, const CDateTime& dateAdded /* = CDateTime() */)
{
  // Check if this path is already added
  int idPath = GetPathId(path);
  if (idPath < 0)
  {
    CDateTime finalDateAdded = dateAdded;
    // Skip looking at the files ctime/mtime if defined by the user through as.xml
    if (!finalDateAdded.IsValid() && CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_iVideoLibraryDateAdded > 0)
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
            finalDateAdded = *time;
        }
      }
    }

    if (!finalDateAdded.IsValid())
      finalDateAdded = CDateTime::GetCurrentDateTime();

    idPath = AddPath(path, parentPath, finalDateAdded);
  }

  return ExecuteQuery(PrepareSQL("REPLACE INTO tvshowlinkpath(idShow, idPath) VALUES (%i,%i)", idShow, idPath));
}

int CVideoDatabase::AddTvShow()
{
  if (ExecuteQuery("INSERT INTO tvshow(idShow) VALUES(NULL)"))
    return (int)m_pDS->lastinsertid();
  return -1;
}

//********************************************************************************************************************************
int CVideoDatabase::AddEpisode(int idShow, const std::string& strFilenameAndPath)
{
  try
  {
    if (nullptr == m_pDB)
      return -1;
    if (nullptr == m_pDS)
      return -1;

    int idFile = AddFile(strFilenameAndPath);
    if (idFile < 0)
      return -1;
    UpdateFileDateAdded(idFile, strFilenameAndPath);

    std::string strSQL=PrepareSQL("insert into episode (idEpisode, idFile, idShow) values (NULL, %i, %i)", idFile, idShow);
    m_pDS->exec(strSQL);
    return (int)m_pDS->lastinsertid();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strFilenameAndPath.c_str());
  }
  return -1;
}

int CVideoDatabase::AddMusicVideo(const std::string& strFilenameAndPath)
{
  try
  {
    if (nullptr == m_pDB)
      return -1;
    if (nullptr == m_pDS)
      return -1;

    int idMVideo = GetMusicVideoId(strFilenameAndPath);
    if (idMVideo < 0)
    {
      int idFile = AddFile(strFilenameAndPath);
      if (idFile < 0)
        return -1;
      UpdateFileDateAdded(idFile, strFilenameAndPath);
      std::string strSQL=PrepareSQL("insert into musicvideo (idMVideo, idFile) values (NULL, %i)", idFile);
      m_pDS->exec(strSQL);
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
int CVideoDatabase::AddToTable(const std::string& table, const std::string& firstField, const std::string& secondField, const std::string& value)
{
  try
  {
    if (nullptr == m_pDB)
      return -1;
    if (nullptr == m_pDS)
      return -1;

    std::string strSQL = PrepareSQL("select %s from %s where %s like '%s'", firstField.c_str(), table.c_str(), secondField.c_str(), value.substr(0, 255).c_str());
    m_pDS->query(strSQL);
    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      // doesn't exists, add it
      strSQL = PrepareSQL("insert into %s (%s, %s) values(NULL, '%s')", table.c_str(), firstField.c_str(), secondField.c_str(), value.substr(0, 255).c_str());
      m_pDS->exec(strSQL);
      int id = (int)m_pDS->lastinsertid();
      return id;
    }
    else
    {
      int id = m_pDS->fv(firstField.c_str()).get_asInt();
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

int CVideoDatabase::UpdateRatings(int mediaId, const char *mediaType, const RatingMap& values, const std::string& defaultRating)
{
  try
  {
    if (nullptr == m_pDB)
      return -1;
    if (nullptr == m_pDS)
      return -1;

    std::string sql = PrepareSQL("DELETE FROM rating WHERE media_id=%i AND media_type='%s'", mediaId, mediaType);
    m_pDS->exec(sql);

    return AddRatings(mediaId, mediaType, values, defaultRating);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s unable to update ratings of  (%s)", __FUNCTION__, mediaType);
  }
  return -1;
}

int CVideoDatabase::AddRatings(int mediaId, const char *mediaType, const RatingMap& values, const std::string& defaultRating)
{
  int ratingid = -1;
  try
  {
    if (nullptr == m_pDB)
      return -1;
    if (nullptr == m_pDS)
      return -1;

    for (const auto& i : values)
    {
      int id;
      std::string strSQL = PrepareSQL("SELECT rating_id FROM rating WHERE media_id=%i AND media_type='%s' AND rating_type = '%s'", mediaId, mediaType, i.first.c_str());
      m_pDS->query(strSQL);
      if (m_pDS->num_rows() == 0)
      {
        m_pDS->close();
        // doesn't exists, add it
        strSQL = PrepareSQL("INSERT INTO rating (media_id, media_type, rating_type, rating, votes) VALUES (%i, '%s', '%s', %f, %i)", mediaId, mediaType, i.first.c_str(), i.second.rating, i.second.votes);
        m_pDS->exec(strSQL);
        id = (int)m_pDS->lastinsertid();
      }
      else
      {
        id = m_pDS->fv(0).get_asInt();
        m_pDS->close();
        strSQL = PrepareSQL("UPDATE rating SET rating = %f, votes = %i WHERE rating_id = %i", i.second.rating, i.second.votes, id);
        m_pDS->exec(strSQL);
      }
      if (i.first == defaultRating)
        ratingid = id;
    }
    return ratingid;

  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%i - %s) failed", __FUNCTION__, mediaId, mediaType);
  }

  return ratingid;
}

int CVideoDatabase::UpdateUniqueIDs(int mediaId, const char *mediaType, const CVideoInfoTag& details)
{
  try
  {
    if (nullptr == m_pDB)
      return -1;
    if (nullptr == m_pDS)
      return -1;

    std::string sql = PrepareSQL("DELETE FROM uniqueid WHERE media_id=%i AND media_type='%s'", mediaId, mediaType);
    m_pDS->exec(sql);

    return AddUniqueIDs(mediaId, mediaType, details);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s unable to update unique ids of (%s)", __FUNCTION__, mediaType);
  }
  return -1;
}

int CVideoDatabase::AddUniqueIDs(int mediaId, const char *mediaType, const CVideoInfoTag& details)
{
  int uniqueid = -1;
  try
  {
    if (nullptr == m_pDB)
      return -1;
    if (nullptr == m_pDS)
      return -1;

    for (const auto& i : details.GetUniqueIDs())
    {
      int id;
      std::string strSQL = PrepareSQL("SELECT uniqueid_id FROM uniqueid WHERE media_id=%i AND media_type='%s' AND type = '%s'", mediaId, mediaType, i.first.c_str());
      m_pDS->query(strSQL);
      if (m_pDS->num_rows() == 0)
      {
        m_pDS->close();
        // doesn't exists, add it
        strSQL = PrepareSQL("INSERT INTO uniqueid (media_id, media_type, value, type) VALUES (%i, '%s', '%s', '%s')", mediaId, mediaType, i.second.c_str(), i.first.c_str());
        m_pDS->exec(strSQL);
        id = (int)m_pDS->lastinsertid();
      }
      else
      {
        id = m_pDS->fv(0).get_asInt();
        m_pDS->close();
        strSQL = PrepareSQL("UPDATE uniqueid SET value = '%s', type = '%s' WHERE uniqueid_id = %i", i.second.c_str(), i.first.c_str(), id);
        m_pDS->exec(strSQL);
      }
      if (i.first == details.GetDefaultUniqueID())
        uniqueid = id;
    }
    return uniqueid;

  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%i - %s) failed", __FUNCTION__, mediaId, mediaType);
  }

  return uniqueid;
}

int CVideoDatabase::AddSet(const std::string& strSet, const std::string& strOverview /* = "" */)
{
  if (strSet.empty())
    return -1;

  try
  {
    if (m_pDB == nullptr || m_pDS == nullptr)
      return -1;

    std::string strSQL = PrepareSQL("SELECT idSet FROM sets WHERE strSet LIKE '%s'", strSet.c_str());
    m_pDS->query(strSQL);
    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      strSQL = PrepareSQL("INSERT INTO sets (idSet, strSet, strOverview) VALUES(NULL, '%s', '%s')", strSet.c_str(), strOverview.c_str());
      m_pDS->exec(strSQL);
      int id = static_cast<int>(m_pDS->lastinsertid());
      return id;
    }
    else
    {
      int id = m_pDS->fv("idSet").get_asInt();
      m_pDS->close();
      return id;
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strSet.c_str());
  }

  return -1;
}

int CVideoDatabase::AddTag(const std::string& name)
{
  if (name.empty())
    return -1;

  return AddToTable("tag", "tag_id", "name", name);
}

int CVideoDatabase::AddActor(const std::string& name, const std::string& thumbURLs, const std::string &thumb)
{
  try
  {
    if (nullptr == m_pDB)
      return -1;
    if (nullptr == m_pDS)
      return -1;
    int idActor = -1;

    // ATTENTION: the trimming of actor names should really not be done here but after the scraping / NFO-parsing
    std::string trimmedName = name.c_str();
    StringUtils::Trim(trimmedName);

    std::string strSQL=PrepareSQL("select actor_id from actor where name like '%s'", trimmedName.substr(0, 255).c_str());
    m_pDS->query(strSQL);
    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      // doesn't exists, add it
      strSQL=PrepareSQL("insert into actor (actor_id, name, art_urls) values(NULL, '%s', '%s')", trimmedName.substr(0,255).c_str(), thumbURLs.c_str());
      m_pDS->exec(strSQL);
      idActor = (int)m_pDS->lastinsertid();
    }
    else
    {
      idActor = m_pDS->fv(0).get_asInt();
      m_pDS->close();
      // update the thumb url's
      if (!thumbURLs.empty())
      {
        strSQL=PrepareSQL("update actor set art_urls = '%s' where actor_id = %i", thumbURLs.c_str(), idActor);
        m_pDS->exec(strSQL);
      }
    }
    // add artwork
    if (!thumb.empty())
      SetArtForItem(idActor, "actor", "thumb", thumb);
    return idActor;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, name.c_str() );
  }
  return -1;
}



void CVideoDatabase::AddLinkToActor(int mediaId, const char *mediaType, int actorId, const std::string &role, int order)
{
  std::string sql = PrepareSQL("SELECT 1 FROM actor_link WHERE actor_id=%i AND "
                               "media_id=%i AND media_type='%s' AND role='%s'",
                               actorId, mediaId, mediaType, role.c_str());

  if (GetSingleValue(sql).empty())
  { // doesn't exists, add it
    sql = PrepareSQL("INSERT INTO actor_link (actor_id, media_id, media_type, role, cast_order) VALUES(%i,%i,'%s','%s',%i)", actorId, mediaId, mediaType, role.c_str(), order);
    ExecuteQuery(sql);
  }
}

void CVideoDatabase::AddToLinkTable(int mediaId, const std::string& mediaType, const std::string& table, int valueId, const char *foreignKey)
{
  const char *key = foreignKey ? foreignKey : table.c_str();
  std::string sql = PrepareSQL("SELECT 1 FROM %s_link WHERE %s_id=%i AND media_id=%i AND media_type='%s'", table.c_str(), key, valueId, mediaId, mediaType.c_str());

  if (GetSingleValue(sql).empty())
  { // doesn't exists, add it
    sql = PrepareSQL("INSERT INTO %s_link (%s_id,media_id,media_type) VALUES(%i,%i,'%s')", table.c_str(), key, valueId, mediaId, mediaType.c_str());
    ExecuteQuery(sql);
  }
}

void CVideoDatabase::RemoveFromLinkTable(int mediaId, const std::string& mediaType, const std::string& table, int valueId, const char *foreignKey)
{
  const char *key = foreignKey ? foreignKey : table.c_str();
  std::string sql = PrepareSQL("DELETE FROM %s_link WHERE %s_id=%i AND media_id=%i AND media_type='%s'", table.c_str(), key, valueId, mediaId, mediaType.c_str());

  ExecuteQuery(sql);
}

void CVideoDatabase::AddLinksToItem(int mediaId, const std::string& mediaType, const std::string& field, const std::vector<std::string>& values)
{
  for (const auto &i : values)
  {
    if (!i.empty())
    {
      int idValue = AddToTable(field, field + "_id", "name", i);
      if (idValue > -1)
        AddToLinkTable(mediaId, mediaType, field, idValue);
    }
  }
}

void CVideoDatabase::UpdateLinksToItem(int mediaId, const std::string& mediaType, const std::string& field, const std::vector<std::string>& values)
{
  std::string sql = PrepareSQL("DELETE FROM %s_link WHERE media_id=%i AND media_type='%s'", field.c_str(), mediaId, mediaType.c_str());
  m_pDS->exec(sql);

  AddLinksToItem(mediaId, mediaType, field, values);
}

void CVideoDatabase::AddActorLinksToItem(int mediaId, const std::string& mediaType, const std::string& field, const std::vector<std::string>& values)
{
  for (const auto &i : values)
  {
    if (!i.empty())
    {
      int idValue = AddActor(i, "");
      if (idValue > -1)
        AddToLinkTable(mediaId, mediaType, field, idValue, "actor");
    }
  }
}

void CVideoDatabase::UpdateActorLinksToItem(int mediaId, const std::string& mediaType, const std::string& field, const std::vector<std::string>& values)
{
  std::string sql = PrepareSQL("DELETE FROM %s_link WHERE media_id=%i AND media_type='%s'", field.c_str(), mediaId, mediaType.c_str());
  m_pDS->exec(sql);

  AddActorLinksToItem(mediaId, mediaType, field, values);
}

//****Tags****
void CVideoDatabase::AddTagToItem(int media_id, int tag_id, const std::string &type)
{
  if (type.empty())
    return;

  AddToLinkTable(media_id, type, "tag", tag_id);
}

void CVideoDatabase::RemoveTagFromItem(int media_id, int tag_id, const std::string &type)
{
  if (type.empty())
    return;

  RemoveFromLinkTable(media_id, type, "tag", tag_id);
}

void CVideoDatabase::RemoveTagsFromItem(int media_id, const std::string &type)
{
  if (type.empty())
    return;

  m_pDS2->exec(PrepareSQL("DELETE FROM tag_link WHERE media_id=%d AND media_type='%s'", media_id, type.c_str()));
}

//****Actors****
void CVideoDatabase::AddCast(int mediaId, const char *mediaType, const std::vector< SActorInfo > &cast)
{
  if (cast.empty())
    return;

  int order = std::max_element(cast.begin(), cast.end())->order;
  for (const auto &i : cast)
  {
    int idActor = AddActor(i.strName, i.thumbUrl.GetData(), i.thumb);
    AddLinkToActor(mediaId, mediaType, idActor, i.strRole, i.order >= 0 ? i.order : ++order);
  }
}

//********************************************************************************************************************************
bool CVideoDatabase::LoadVideoInfo(const std::string& strFilenameAndPath, CVideoInfoTag& details, int getDetails /* = VideoDbDetailsAll */)
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

bool CVideoDatabase::HasMovieInfo(const std::string& strFilenameAndPath)
{
  try
  {
    if (nullptr == m_pDB)
      return false;
    if (nullptr == m_pDS)
      return false;
    int idMovie = GetMovieId(strFilenameAndPath);
    return (idMovie > 0); // index of zero is also invalid
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strFilenameAndPath.c_str());
  }
  return false;
}

bool CVideoDatabase::HasTvShowInfo(const std::string& strPath)
{
  try
  {
    if (nullptr == m_pDB)
      return false;
    if (nullptr == m_pDS)
      return false;
    int idTvShow = GetTvShowId(strPath);
    return (idTvShow > 0); // index of zero is also invalid
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strPath.c_str());
  }
  return false;
}

bool CVideoDatabase::HasEpisodeInfo(const std::string& strFilenameAndPath)
{
  try
  {
    if (nullptr == m_pDB)
      return false;
    if (nullptr == m_pDS)
      return false;
    int idEpisode = GetEpisodeId(strFilenameAndPath);
    return (idEpisode > 0); // index of zero is also invalid
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strFilenameAndPath.c_str());
  }
  return false;
}

bool CVideoDatabase::HasMusicVideoInfo(const std::string& strFilenameAndPath)
{
  try
  {
    if (nullptr == m_pDB)
      return false;
    if (nullptr == m_pDS)
      return false;
    int idMVideo = GetMusicVideoId(strFilenameAndPath);
    return (idMVideo > 0); // index of zero is also invalid
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strFilenameAndPath.c_str());
  }
  return false;
}

void CVideoDatabase::DeleteDetailsForTvShow(const std::string& strPath)
{
  int idTvShow = GetTvShowId(strPath);
  if (idTvShow >= 0)
    DeleteDetailsForTvShow(idTvShow);
}

void CVideoDatabase::DeleteDetailsForTvShow(int idTvShow)
{
  try
  {
    if (nullptr == m_pDB)
      return;
    if (nullptr == m_pDS)
      return;

    std::string strSQL;
    strSQL=PrepareSQL("DELETE from genre_link WHERE media_id=%i AND media_type='tvshow'", idTvShow);
    m_pDS->exec(strSQL);

    strSQL=PrepareSQL("DELETE FROM actor_link WHERE media_id=%i AND media_type='tvshow'", idTvShow);
    m_pDS->exec(strSQL);

    strSQL=PrepareSQL("DELETE FROM director_link WHERE media_id=%i AND media_type='tvshow'", idTvShow);
    m_pDS->exec(strSQL);

    strSQL=PrepareSQL("DELETE FROM studio_link WHERE media_id=%i AND media_type='tvshow'", idTvShow);
    m_pDS->exec(strSQL);

    strSQL = PrepareSQL("DELETE FROM rating WHERE media_id=%i AND media_type='tvshow'", idTvShow);
    m_pDS->exec(strSQL);

    strSQL = PrepareSQL("DELETE FROM uniqueid WHERE media_id=%i AND media_type='tvshow'", idTvShow);
    m_pDS->exec(strSQL);

    // remove all info other than the id
    // we do this due to the way we have the link between the file + movie tables.

    std::vector<std::string> ids;
    for (int iType = VIDEODB_ID_TV_MIN + 1; iType < VIDEODB_ID_TV_MAX; iType++)
      ids.emplace_back(StringUtils::Format("c%02d=NULL", iType));

    strSQL = "update tvshow set ";
    strSQL += StringUtils::Join(ids, ", ");
    strSQL += PrepareSQL(" where idShow=%i", idTvShow);
    m_pDS->exec(strSQL);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%i) failed", __FUNCTION__, idTvShow);
  }
}

//********************************************************************************************************************************
void CVideoDatabase::GetMoviesByActor(const std::string& name, CFileItemList& items)
{
  Filter filter;
  filter.join  = "LEFT JOIN actor_link ON actor_link.media_id=movie_view.idMovie AND actor_link.media_type='movie' "
                 "LEFT JOIN actor a ON a.actor_id=actor_link.actor_id "
                 "LEFT JOIN director_link ON director_link.media_id=movie_view.idMovie AND director_link.media_type='movie' "
                 "LEFT JOIN actor d ON d.actor_id=director_link.actor_id";
  filter.where = PrepareSQL("a.name='%s' OR d.name='%s'", name.c_str(), name.c_str());
  filter.group = "movie_view.idMovie";
  GetMoviesByWhere("videodb://movies/titles/", filter, items);
}

void CVideoDatabase::GetTvShowsByActor(const std::string& name, CFileItemList& items)
{
  Filter filter;
  filter.join  = "LEFT JOIN actor_link ON actor_link.media_id=tvshow_view.idShow AND actor_link.media_type='tvshow' "
                 "LEFT JOIN actor a ON a.actor_id=actor_link.actor_id "
                 "LEFT JOIN director_link ON director_link.media_id=tvshow_view.idShow AND director_link.media_type='tvshow' "
                 "LEFT JOIN actor d ON d.actor_id=director_link.actor_id";
  filter.where = PrepareSQL("a.name='%s' OR d.name='%s'", name.c_str(), name.c_str());
  GetTvShowsByWhere("videodb://tvshows/titles/", filter, items);
}

void CVideoDatabase::GetEpisodesByActor(const std::string& name, CFileItemList& items)
{
  Filter filter;
  filter.join  = "LEFT JOIN actor_link ON actor_link.media_id=episode_view.idEpisode AND actor_link.media_type='episode' "
                 "LEFT JOIN actor a ON a.actor_id=actor_link.actor_id "
                 "LEFT JOIN director_link ON director_link.media_id=episode_view.idEpisode AND director_link.media_type='episode' "
                 "LEFT JOIN actor d ON d.actor_id=director_link.actor_id";
  filter.where = PrepareSQL("a.name='%s' OR d.name='%s'", name.c_str(), name.c_str());
  filter.group = "episode_view.idEpisode";
  GetEpisodesByWhere("videodb://tvshows/titles/", filter, items);
}

void CVideoDatabase::GetMusicVideosByArtist(const std::string& strArtist, CFileItemList& items)
{
  try
  {
    items.Clear();
    if (nullptr == m_pDB)
      return;
    if (nullptr == m_pDS)
      return;

    std::string strSQL;
    if (strArtist.empty())  //! @todo SMARTPLAYLISTS what is this here for???
      strSQL=PrepareSQL("select distinct * from musicvideo_view join actor_link on actor_link.media_id=musicvideo_view.idMVideo AND actor_link.media_type='musicvideo' join actor on actor.actor_id=actor_link.actor_id");
    else
      strSQL=PrepareSQL("select * from musicvideo_view join actor_link on actor_link.media_id=musicvideo_view.idMVideo AND actor_link.media_type='musicvideo' join actor on actor.actor_id=actor_link.actor_id where actor.name='%s'", strArtist.c_str());
    m_pDS->query( strSQL );

    while (!m_pDS->eof())
    {
      CVideoInfoTag tag = GetDetailsForMusicVideo(m_pDS);
      CFileItemPtr pItem(new CFileItem(tag));
      pItem->SetLabel(StringUtils::Join(tag.m_artist, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoItemSeparator));
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
bool CVideoDatabase::GetMovieInfo(const std::string& strFilenameAndPath, CVideoInfoTag& details, int idMovie /* = -1 */, int getDetails /* = VideoDbDetailsAll */)
{
  try
  {
    if (m_pDB == nullptr || m_pDS == nullptr)
      return false;

    if (idMovie < 0)
      idMovie = GetMovieId(strFilenameAndPath);
    if (idMovie < 0) return false;

    std::string sql = PrepareSQL("select * from movie_view where idMovie=%i", idMovie);
    if (!m_pDS->query(sql))
      return false;
    details = GetDetailsForMovie(m_pDS, getDetails);
    return !details.IsEmpty();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strFilenameAndPath.c_str());
  }
  return false;
}

//********************************************************************************************************************************
bool CVideoDatabase::GetTvShowInfo(const std::string& strPath, CVideoInfoTag& details, int idTvShow /* = -1 */, CFileItem *item /* = NULL */, int getDetails /* = VideoDbDetailsAll */)
{
  try
  {
    if (m_pDB == nullptr || m_pDS == nullptr)
      return false;

    if (idTvShow < 0)
      idTvShow = GetTvShowId(strPath);
    if (idTvShow < 0) return false;

    std::string sql = PrepareSQL("SELECT * FROM tvshow_view WHERE idShow=%i GROUP BY idShow", idTvShow);
    if (!m_pDS->query(sql))
      return false;
    details = GetDetailsForTvShow(m_pDS, getDetails, item);
    return !details.IsEmpty();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strPath.c_str());
  }
  return false;
}

bool CVideoDatabase::GetSeasonInfo(int idSeason, CVideoInfoTag& details, bool allDetails /* = true */)
{
  if (idSeason < 0)
    return false;

  try
  {
    if (!m_pDB || !m_pDS)
      return false;

    std::string sql = PrepareSQL("SELECT idSeason, idShow, season, name, userrating FROM seasons WHERE idSeason=%i", idSeason);
    if (!m_pDS->query(sql))
      return false;

    if (m_pDS->num_rows() != 1)
      return false;

    if (allDetails)
    {
      int idShow = m_pDS->fv(1).get_asInt();

      // close the current result because we are going to query the season view for all details
      m_pDS->close();

      if (idShow < 0)
        return false;

      CFileItemList seasons;
      if (!GetSeasonsNav(StringUtils::Format("videodb://tvshows/titles/%i/", idShow), seasons, -1, -1, -1, -1, idShow, false) || seasons.Size() <= 0)
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

      return false;
    }

    const int season = m_pDS->fv(2).get_asInt();
    std::string name = m_pDS->fv(3).get_asString();

    if (name.empty())
    {
      if (season == 0)
        name = g_localizeStrings.Get(20381);
      else
        name = StringUtils::Format(g_localizeStrings.Get(20358).c_str(), season);
    }

    details.m_strTitle = name;
    if (!name.empty())
      details.m_strSortTitle = name;
    details.m_iSeason = season;
    details.m_iDbId = m_pDS->fv(0).get_asInt();
    details.m_iIdSeason = details.m_iDbId;
    details.m_type = MediaTypeSeason;
    details.m_iUserRating = m_pDS->fv(4).get_asInt();
    details.m_iIdShow = m_pDS->fv(1).get_asInt();

    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%i) failed", __FUNCTION__, idSeason);
  }
  return false;
}

bool CVideoDatabase::GetEpisodeBasicInfo(const std::string& strFilenameAndPath, CVideoInfoTag& details, int idEpisode /* = -1 */)
{
  try
  {
    if (idEpisode < 0)
      idEpisode = GetEpisodeId(strFilenameAndPath);

    if (idEpisode < 0)
      return false;

    std::string sql = PrepareSQL("select * from episode where idEpisode=%i",idEpisode);
    if (!m_pDS->query(sql))
      return false;
    details = GetBasicDetailsForEpisode(m_pDS);
    return !details.IsEmpty();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strFilenameAndPath.c_str());
  }
  return false;
}

bool CVideoDatabase::GetEpisodeInfo(const std::string& strFilenameAndPath, CVideoInfoTag& details, int idEpisode /* = -1 */, int getDetails /* = VideoDbDetailsAll */)
{
  try
  {
    if (m_pDB == nullptr || m_pDS == nullptr)
      return false;

    if (idEpisode < 0)
      idEpisode = GetEpisodeId(strFilenameAndPath, details.m_iEpisode, details.m_iSeason);
    if (idEpisode < 0) return false;

    std::string sql = PrepareSQL("select * from episode_view where idEpisode=%i",idEpisode);
    if (!m_pDS->query(sql))
      return false;
    details = GetDetailsForEpisode(m_pDS, getDetails);
    return !details.IsEmpty();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strFilenameAndPath.c_str());
  }
  return false;
}

bool CVideoDatabase::GetMusicVideoInfo(const std::string& strFilenameAndPath, CVideoInfoTag& details, int idMVideo /* = -1 */, int getDetails /* = VideoDbDetailsAll */)
{
  try
  {
    if (m_pDB == nullptr || m_pDS == nullptr)
      return false;

    if (idMVideo < 0)
      idMVideo = GetMusicVideoId(strFilenameAndPath);
    if (idMVideo < 0) return false;

    std::string sql = PrepareSQL("select * from musicvideo_view where idMVideo=%i", idMVideo);
    if (!m_pDS->query(sql))
      return false;
    details = GetDetailsForMusicVideo(m_pDS, getDetails);
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

bool CVideoDatabase::GetFileInfo(const std::string& strFilenameAndPath, CVideoInfoTag& details, int idFile /* = -1 */)
{
  try
  {
    if (idFile < 0)
      idFile = GetFileId(strFilenameAndPath);
    if (idFile < 0)
      return false;

    std::string sql = PrepareSQL("SELECT * FROM files "
                                "JOIN path ON path.idPath = files.idPath "
                                "LEFT JOIN bookmark ON bookmark.idFile = files.idFile AND bookmark.type = %i "
                                "WHERE files.idFile = %i", CBookmark::RESUME, idFile);
    if (!m_pDS->query(sql))
      return false;

    details.m_iFileId = m_pDS->fv("files.idFile").get_asInt();
    details.m_strPath = m_pDS->fv("path.strPath").get_asString();
    std::string strFileName = m_pDS->fv("files.strFilename").get_asString();
    ConstructPath(details.m_strFileNameAndPath, details.m_strPath, strFileName);
    details.SetPlayCount(std::max(details.GetPlayCount(), m_pDS->fv("files.playCount").get_asInt()));
    if (!details.m_lastPlayed.IsValid())
      details.m_lastPlayed.SetFromDBDateTime(m_pDS->fv("files.lastPlayed").get_asString());
    if (!details.m_dateAdded.IsValid())
      details.m_dateAdded.SetFromDBDateTime(m_pDS->fv("files.dateAdded").get_asString());
    if (!details.GetResumePoint().IsSet())
    {
      details.SetResumePoint(m_pDS->fv("bookmark.timeInSeconds").get_asDouble(),
                             m_pDS->fv("bookmark.totalTimeInSeconds").get_asDouble(),
                             m_pDS->fv("bookmark.playerState").get_asString());
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

std::string CVideoDatabase::GetValueString(const CVideoInfoTag &details, int min, int max, const SDbTableOffsets *offsets) const
{
  std::vector<std::string> conditions;
  for (int i = min + 1; i < max; ++i)
  {
    switch (offsets[i].type)
    {
    case VIDEODB_TYPE_STRING:
      conditions.emplace_back(PrepareSQL("c%02d='%s'", i, ((const std::string*)(((const char*)&details)+offsets[i].offset))->c_str()));
      break;
    case VIDEODB_TYPE_INT:
      conditions.emplace_back(PrepareSQL("c%02d='%i'", i, *(const int*)(((const char*)&details)+offsets[i].offset)));
      break;
    case VIDEODB_TYPE_COUNT:
      {
        int value = *(const int*)(((const char*)&details)+offsets[i].offset);
        if (value)
          conditions.emplace_back(PrepareSQL("c%02d=%i", i, value));
        else
          conditions.emplace_back(PrepareSQL("c%02d=NULL", i));
      }
      break;
    case VIDEODB_TYPE_BOOL:
      conditions.emplace_back(PrepareSQL("c%02d='%s'", i, *(const bool*)(((const char*)&details)+offsets[i].offset)?"true":"false"));
      break;
    case VIDEODB_TYPE_FLOAT:
      conditions.emplace_back(PrepareSQL("c%02d='%f'", i, *(const float*)(((const char*)&details)+offsets[i].offset)));
      break;
    case VIDEODB_TYPE_STRINGARRAY:
      conditions.emplace_back(PrepareSQL("c%02d='%s'", i, StringUtils::Join(*((const std::vector<std::string>*)(((const char*)&details)+offsets[i].offset)),
                                                                          CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoItemSeparator).c_str()));
      break;
    case VIDEODB_TYPE_DATE:
      conditions.emplace_back(PrepareSQL("c%02d='%s'", i, ((const CDateTime*)(((const char*)&details)+offsets[i].offset))->GetAsDBDate().c_str()));
      break;
    case VIDEODB_TYPE_DATETIME:
      conditions.emplace_back(PrepareSQL("c%02d='%s'", i, ((const CDateTime*)(((const char*)&details)+offsets[i].offset))->GetAsDBDateTime().c_str()));
      break;
    case VIDEODB_TYPE_UNUSED: // Skip the unused field to avoid populating unused data
      continue;
    }
  }
  return StringUtils::Join(conditions, ",");
}

//********************************************************************************************************************************
int CVideoDatabase::SetDetailsForItem(CVideoInfoTag& details, const std::map<std::string, std::string> &artwork)
{
  return SetDetailsForItem(details.m_iDbId, details.m_type, details, artwork);
}

int CVideoDatabase::SetDetailsForItem(int id, const MediaType& mediaType, CVideoInfoTag& details, const std::map<std::string, std::string> &artwork)
{
  if (mediaType == MediaTypeNone)
    return -1;

  if (mediaType == MediaTypeMovie)
    return SetDetailsForMovie(details.GetPath(), details, artwork, id);
  else if (mediaType == MediaTypeVideoCollection)
    return SetDetailsForMovieSet(details, artwork, id);
  else if (mediaType == MediaTypeTvShow)
  {
    std::map<int, std::map<std::string, std::string> > seasonArtwork;
    if (!UpdateDetailsForTvShow(id, details, artwork, seasonArtwork))
      return -1;

    return id;
  }
  else if (mediaType == MediaTypeSeason)
    return SetDetailsForSeason(details, artwork, details.m_iIdShow, id);
  else if (mediaType == MediaTypeEpisode)
    return SetDetailsForEpisode(details.GetPath(), details, artwork, details.m_iIdShow, id);
  else if (mediaType == MediaTypeMusicVideo)
    return SetDetailsForMusicVideo(details.GetPath(), details, artwork, id);

  return -1;
}

int CVideoDatabase::SetDetailsForMovie(const std::string& strFilenameAndPath, CVideoInfoTag& details,
    const std::map<std::string, std::string> &artwork, int idMovie /* = -1 */)
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

    // update dateadded if it's set
    if (details.m_dateAdded.IsValid())
    {
      if (details.m_iFileId <= 0)
        details.m_iFileId = GetFileId(strFilenameAndPath);

      UpdateFileDateAdded(details.m_iFileId, strFilenameAndPath, details.m_dateAdded);
    }

    AddCast(idMovie, "movie", details.m_cast);
    AddLinksToItem(idMovie, MediaTypeMovie, "genre", details.m_genre);
    AddLinksToItem(idMovie, MediaTypeMovie, "studio", details.m_studio);
    AddLinksToItem(idMovie, MediaTypeMovie, "country", details.m_country);
    AddLinksToItem(idMovie, MediaTypeMovie, "tag", details.m_tags);
    AddActorLinksToItem(idMovie, MediaTypeMovie, "director", details.m_director);
    AddActorLinksToItem(idMovie, MediaTypeMovie, "writer", details.m_writingCredits);

    // add ratingsu
    details.m_iIdRating = AddRatings(idMovie, MediaTypeMovie, details.m_ratings, details.GetDefaultRating());

    // add unique ids
    details.m_iIdUniqueID = UpdateUniqueIDs(idMovie, MediaTypeMovie, details);

    // add set...
    int idSet = -1;
    if (!details.m_set.title.empty())
    {
      idSet = AddSet(details.m_set.title, details.m_set.overview);
      // add art if not available
      if (!HasArtForItem(idSet, MediaTypeVideoCollection))
      {
        for (const auto &it : artwork)
        {
          if (StringUtils::StartsWith(it.first, "set."))
            SetArtForItem(idSet, MediaTypeVideoCollection, it.first.substr(4), it.second);
        }
      }
    }

    if (details.HasStreamDetails())
      SetStreamDetailsForFileId(details.m_streamDetails, GetFileId(strFilenameAndPath));

    SetArtForItem(idMovie, MediaTypeMovie, artwork);

    if (!details.HasUniqueID() && details.HasYear())
    { // query DB for any movies matching online id and year
      std::string strSQL = PrepareSQL("SELECT files.playCount, files.lastPlayed "
                                      "FROM movie "
                                      "  INNER JOIN files "
                                      "    ON files.idFile=movie.idFile "
                                      "  JOIN uniqueid "
                                      "    ON movie.idMovie=uniqueid.media_id AND uniqueid.media_type='movie' AND uniqueid.value='%s'"
                                      "WHERE movie.premiered LIKE '%i%%' AND movie.idMovie!=%i AND files.playCount > 0",
                                      details.GetUniqueID().c_str(), details.GetYear(), idMovie);
      m_pDS->query(strSQL);

      if (!m_pDS->eof())
      {
        int playCount = m_pDS->fv("files.playCount").get_asInt();

        CDateTime lastPlayed;
        lastPlayed.SetFromDBDateTime(m_pDS->fv("files.lastPlayed").get_asString());

        int idFile = GetFileId(strFilenameAndPath);

        // update with playCount and lastPlayed
        strSQL = PrepareSQL("update files set playCount=%i,lastPlayed='%s' where idFile=%i", playCount, lastPlayed.GetAsDBDateTime().c_str(), idFile);
        m_pDS->exec(strSQL);
      }

      m_pDS->close();
    }
    // update our movie table (we know it was added already above)
    // and insert the new row
    std::string sql = "UPDATE movie SET " + GetValueString(details, VIDEODB_ID_MIN, VIDEODB_ID_MAX, DbMovieOffsets);
    if (idSet > 0)
      sql += PrepareSQL(", idSet = %i", idSet);
    else
      sql += ", idSet = NULL";
    if (details.m_iUserRating > 0 && details.m_iUserRating < 11)
      sql += PrepareSQL(", userrating = %i", details.m_iUserRating);
    else
      sql += ", userrating = NULL";
    if (details.HasPremiered())
      sql += PrepareSQL(", premiered = '%s'", details.GetPremiered().GetAsDBDate().c_str());
    else
      sql += PrepareSQL(", premiered = '%i'", details.GetYear());
    sql += PrepareSQL(" where idMovie=%i", idMovie);
    m_pDS->exec(sql);
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

int CVideoDatabase::UpdateDetailsForMovie(int idMovie, CVideoInfoTag& details, const std::map<std::string, std::string> &artwork, const std::set<std::string> &updatedDetails)
{
  if (idMovie < 0)
    return idMovie;

  try
  {
    CLog::Log(LOGINFO, "%s: Starting updates for movie %i", __FUNCTION__, idMovie);

    BeginTransaction();

    // process the link table updates
    if (updatedDetails.find("genre") != updatedDetails.end())
      UpdateLinksToItem(idMovie, MediaTypeMovie, "genre", details.m_genre);
    if (updatedDetails.find("studio") != updatedDetails.end())
      UpdateLinksToItem(idMovie, MediaTypeMovie, "studio", details.m_studio);
    if (updatedDetails.find("country") != updatedDetails.end())
      UpdateLinksToItem(idMovie, MediaTypeMovie, "country", details.m_country);
    if (updatedDetails.find("tag") != updatedDetails.end())
      UpdateLinksToItem(idMovie, MediaTypeMovie, "tag", details.m_tags);
    if (updatedDetails.find("director") != updatedDetails.end())
      UpdateActorLinksToItem(idMovie, MediaTypeMovie, "director", details.m_director);
    if (updatedDetails.find("writer") != updatedDetails.end())
      UpdateActorLinksToItem(idMovie, MediaTypeMovie, "writer", details.m_writingCredits);
    if (updatedDetails.find("art.altered") != updatedDetails.end())
      SetArtForItem(idMovie, MediaTypeMovie, artwork);
    if (updatedDetails.find("ratings") != updatedDetails.end())
      details.m_iIdRating = UpdateRatings(idMovie, MediaTypeMovie, details.m_ratings, details.GetDefaultRating());
    if (updatedDetails.find("uniqueid") != updatedDetails.end())
      details.m_iIdUniqueID = UpdateUniqueIDs(idMovie, MediaTypeMovie, details);
    if (updatedDetails.find("dateadded") != updatedDetails.end() && details.m_dateAdded.IsValid())
    {
      if (details.m_iFileId <= 0)
        details.m_iFileId = GetFileId(details.GetPath());

      UpdateFileDateAdded(details.m_iFileId, details.GetPath(), details.m_dateAdded);
    }

    // track if the set was updated
    int idSet = 0;
    if (updatedDetails.find("set") != updatedDetails.end())
    { // set
      idSet = -1;
      if (!details.m_set.title.empty())
      {
        idSet = AddSet(details.m_set.title, details.m_set.overview);
      }
    }

    if (updatedDetails.find("showlink") != updatedDetails.end())
    {
      // remove existing links
      std::vector<int> tvShowIds;
      GetLinksToTvShow(idMovie, tvShowIds);
      for (const auto& idTVShow : tvShowIds)
        LinkMovieToTvshow(idMovie, idTVShow, true);

      // setup links to shows if the linked shows are in the db
      for (const auto& showLink : details.m_showLink)
      {
        CFileItemList items;
        GetTvShowsByName(showLink, items);
        if (!items.IsEmpty())
          LinkMovieToTvshow(idMovie, items[0]->GetVideoInfoTag()->m_iDbId, false);
        else
          CLog::Log(LOGWARNING, "%s: Failed to link movie %s to show %s", __FUNCTION__, details.m_strTitle.c_str(), showLink.c_str());
      }
    }

    // and update the movie table
    std::string sql = "UPDATE movie SET " + GetValueString(details, VIDEODB_ID_MIN, VIDEODB_ID_MAX, DbMovieOffsets);
    if (idSet > 0)
      sql += PrepareSQL(", idSet = %i", idSet);
    else if (idSet < 0)
      sql += ", idSet = NULL";
    if (details.m_iUserRating > 0 && details.m_iUserRating < 11)
      sql += PrepareSQL(", userrating = %i", details.m_iUserRating);
    else
      sql += ", userrating = NULL";
    if (details.HasPremiered())
      sql += PrepareSQL(", premiered = '%s'", details.GetPremiered().GetAsDBDate().c_str());
    else
      sql += PrepareSQL(", premiered = '%i'", details.GetYear());
    sql += PrepareSQL(" where idMovie=%i", idMovie);
    m_pDS->exec(sql);

    CommitTransaction();

    CLog::Log(LOGINFO, "%s: Finished updates for movie %i", __FUNCTION__, idMovie);

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
      idSet = AddSet(details.m_strTitle, details.m_strPlot);
      if (idSet < 0)
      {
        RollbackTransaction();
        return -1;
      }
    }

    SetArtForItem(idSet, MediaTypeVideoCollection, artwork);

    // and insert the new row
    std::string sql = PrepareSQL("UPDATE sets SET strSet='%s', strOverview='%s' WHERE idSet=%i", details.m_strTitle.c_str(), details.m_strPlot.c_str(), idSet);
    m_pDS->exec(sql);
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
  // first try matching on uniqueid, then on title + year
  int id = -1;
  if (!details.HasUniqueID())
    id = GetDbId(PrepareSQL("SELECT idShow FROM tvshow JOIN uniqueid ON uniqueid.media_id=tvshow.idShow AND uniqueid.media_type='tvshow' WHERE uniqueid.value='%s'", details.GetUniqueID().c_str()));
  if (id < 0)
    id = GetDbId(PrepareSQL("SELECT idShow FROM tvshow WHERE c%02d='%s' AND c%02d='%s'", VIDEODB_ID_TV_TITLE, details.m_strTitle.c_str(), VIDEODB_ID_TV_PREMIERED, details.GetPremiered().GetAsDBDate().c_str()));
  return id;
}

int CVideoDatabase::SetDetailsForTvShow(const std::vector<std::pair<std::string, std::string> > &paths,
    CVideoInfoTag& details, const std::map<std::string, std::string> &artwork,
    const std::map<int, std::map<std::string, std::string> > &seasonArt, int idTvShow /*= -1 */)
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
    for (const auto &i : paths)
    {
      idTvShow = GetTvShowId(i.first);
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
  for (const auto &i : paths)
    AddPathToTvShow(idTvShow, i.first, i.second, details.m_dateAdded);

  UpdateDetailsForTvShow(idTvShow, details, artwork, seasonArt);

  return idTvShow;
}

bool CVideoDatabase::UpdateDetailsForTvShow(int idTvShow, CVideoInfoTag &details,
    const std::map<std::string, std::string> &artwork, const std::map<int, std::map<std::string, std::string>> &seasonArt)
{
  BeginTransaction();

  DeleteDetailsForTvShow(idTvShow);

  AddCast(idTvShow, "tvshow", details.m_cast);
  AddLinksToItem(idTvShow, MediaTypeTvShow, "genre", details.m_genre);
  AddLinksToItem(idTvShow, MediaTypeTvShow, "studio", details.m_studio);
  AddLinksToItem(idTvShow, MediaTypeTvShow, "tag", details.m_tags);
  AddActorLinksToItem(idTvShow, MediaTypeTvShow, "director", details.m_director);

  // add ratings
  details.m_iIdRating = AddRatings(idTvShow, MediaTypeTvShow, details.m_ratings, details.GetDefaultRating());

  // add unique ids
  details.m_iIdUniqueID = UpdateUniqueIDs(idTvShow, MediaTypeTvShow, details);

  // add "all seasons" - the rest are added in SetDetailsForEpisode
  AddSeason(idTvShow, -1);

  // add any named seasons
  for (const auto& namedSeason : details.m_namedSeasons)
  {
    // make sure the named season exists
    int seasonId = AddSeason(idTvShow, namedSeason.first, namedSeason.second);

    // get any existing details for the named season
    CVideoInfoTag season;
    if (!GetSeasonInfo(seasonId, season, false) || season.m_strSortTitle == namedSeason.second)
      continue;

    season.SetSortTitle(namedSeason.second);
    SetDetailsForSeason(season, std::map<std::string, std::string>(), idTvShow, seasonId);
  }

  SetArtForItem(idTvShow, MediaTypeTvShow, artwork);
  for (const auto &i : seasonArt)
  {
    int idSeason = AddSeason(idTvShow, i.first);
    if (idSeason > -1)
      SetArtForItem(idSeason, MediaTypeSeason, i.second);
  }

  // and insert the new row
  std::string sql = "UPDATE tvshow SET " + GetValueString(details, VIDEODB_ID_TV_MIN, VIDEODB_ID_TV_MAX, DbTvShowOffsets);
  if (details.m_iUserRating > 0 && details.m_iUserRating < 11)
    sql += PrepareSQL(", userrating = %i", details.m_iUserRating);
  else
    sql += ", userrating = NULL";
  if (details.GetDuration() > 0)
    sql += PrepareSQL(", duration = %i", details.GetDuration());
  else
    sql += ", duration = NULL";
  sql += PrepareSQL(" WHERE idShow=%i", idTvShow);
  if (ExecuteQuery(sql))
  {
    CommitTransaction();
    return true;
  }
  RollbackTransaction();
  return false;
}

int CVideoDatabase::SetDetailsForSeason(const CVideoInfoTag& details, const std::map<std::string,
    std::string> &artwork, int idShow, int idSeason /* = -1 */)
{
  if (idShow < 0 || details.m_iSeason < -1)
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
    std::string sql = PrepareSQL("UPDATE seasons SET season=%i", details.m_iSeason);
    if (!details.m_strSortTitle.empty())
      sql += PrepareSQL(", name='%s'", details.m_strSortTitle.c_str());
    if (details.m_iUserRating > 0 && details.m_iUserRating < 11)
      sql += PrepareSQL(", userrating = %i", details.m_iUserRating);
    else
      sql += ", userrating = NULL";
    sql += PrepareSQL(" WHERE idSeason=%i", idSeason);
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

int CVideoDatabase::SetDetailsForEpisode(const std::string& strFilenameAndPath, CVideoInfoTag& details,
    const std::map<std::string, std::string> &artwork, int idShow, int idEpisode)
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

    // update dateadded if it's set
    if (details.m_dateAdded.IsValid())
    {
      if (details.m_iFileId <= 0)
        details.m_iFileId = GetFileId(strFilenameAndPath);

      UpdateFileDateAdded(details.m_iFileId, strFilenameAndPath, details.m_dateAdded);
    }

    AddCast(idEpisode, "episode", details.m_cast);
    AddActorLinksToItem(idEpisode, MediaTypeEpisode, "director", details.m_director);
    AddActorLinksToItem(idEpisode, MediaTypeEpisode, "writer", details.m_writingCredits);

    // add ratings
    details.m_iIdRating = AddRatings(idEpisode, MediaTypeEpisode, details.m_ratings, details.GetDefaultRating());

    // add unique ids
    details.m_iIdUniqueID = UpdateUniqueIDs(idEpisode, MediaTypeEpisode, details);

    if (details.HasStreamDetails())
    {
      if (details.m_iFileId != -1)
        SetStreamDetailsForFileId(details.m_streamDetails, details.m_iFileId);
      else
        SetStreamDetailsForFile(details.m_streamDetails, strFilenameAndPath);
    }

    // ensure we have this season already added
    int idSeason = AddSeason(idShow, details.m_iSeason);

    SetArtForItem(idEpisode, MediaTypeEpisode, artwork);

    if (details.m_iEpisode != -1 && details.m_iSeason != -1)
    { // query DB for any episodes matching idShow, Season and Episode
      std::string strSQL = PrepareSQL("SELECT files.playCount, files.lastPlayed "
                                      "FROM episode INNER JOIN files ON files.idFile=episode.idFile "
                                      "WHERE episode.c%02d=%i AND episode.c%02d=%i AND episode.idShow=%i "
                                      "AND episode.idEpisode!=%i AND files.playCount > 0",
                                      VIDEODB_ID_EPISODE_SEASON, details.m_iSeason, VIDEODB_ID_EPISODE_EPISODE,
                                      details.m_iEpisode, idShow, idEpisode);
      m_pDS->query(strSQL);

      if (!m_pDS->eof())
      {
        int playCount = m_pDS->fv("files.playCount").get_asInt();

        CDateTime lastPlayed;
        lastPlayed.SetFromDBDateTime(m_pDS->fv("files.lastPlayed").get_asString());

        int idFile = GetFileId(strFilenameAndPath);

        // update with playCount and lastPlayed
        strSQL = PrepareSQL("update files set playCount=%i,lastPlayed='%s' where idFile=%i", playCount, lastPlayed.GetAsDBDateTime().c_str(), idFile);
        m_pDS->exec(strSQL);
      }

      m_pDS->close();
    }
    // and insert the new row
    std::string sql = "UPDATE episode SET " + GetValueString(details, VIDEODB_ID_EPISODE_MIN, VIDEODB_ID_EPISODE_MAX, DbEpisodeOffsets);
    if (details.m_iUserRating > 0 && details.m_iUserRating < 11)
      sql += PrepareSQL(", userrating = %i", details.m_iUserRating);
    else
      sql += ", userrating = NULL";
    sql += PrepareSQL(", idSeason = %i", idSeason);
    sql += PrepareSQL(" where idEpisode=%i", idEpisode);
    m_pDS->exec(sql);
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
  std::string sql = PrepareSQL("idShow=%i AND season=%i", showID, season);
  std::string id = GetSingleValue("seasons", "idSeason", sql);
  if (id.empty())
    return -1;
  return strtol(id.c_str(), NULL, 10);
}

int CVideoDatabase::AddSeason(int showID, int season, const std::string& name /* = "" */)
{
  int seasonId = GetSeasonId(showID, season);
  if (seasonId < 0)
  {
    if (ExecuteQuery(PrepareSQL("INSERT INTO seasons (idShow, season, name) VALUES(%i, %i, '%s')", showID, season, name.c_str())))
      seasonId = (int)m_pDS->lastinsertid();
  }
  return seasonId;
}

int CVideoDatabase::SetDetailsForMusicVideo(const std::string& strFilenameAndPath, const CVideoInfoTag& details,
    const std::map<std::string, std::string> &artwork, int idMVideo /* = -1 */)
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

    // update dateadded if it's set
    if (details.m_dateAdded.IsValid())
    {
      int idFile = details.m_iFileId;
      if (idFile <= 0)
        idFile = GetFileId(strFilenameAndPath);

      UpdateFileDateAdded(idFile, strFilenameAndPath, details.m_dateAdded);
    }

    AddActorLinksToItem(idMVideo, MediaTypeMusicVideo, "actor", details.m_artist);
    AddActorLinksToItem(idMVideo, MediaTypeMusicVideo, "director", details.m_director);
    AddLinksToItem(idMVideo, MediaTypeMusicVideo, "genre", details.m_genre);
    AddLinksToItem(idMVideo, MediaTypeMusicVideo, "studio", details.m_studio);
    AddLinksToItem(idMVideo, MediaTypeMusicVideo, "tag", details.m_tags);

    if (details.HasStreamDetails())
      SetStreamDetailsForFileId(details.m_streamDetails, GetFileId(strFilenameAndPath));

    SetArtForItem(idMVideo, MediaTypeMusicVideo, artwork);

    // update our movie table (we know it was added already above)
    // and insert the new row
    std::string sql = "UPDATE musicvideo SET " + GetValueString(details, VIDEODB_ID_MUSICVIDEO_MIN, VIDEODB_ID_MUSICVIDEO_MAX, DbMusicVideoOffsets);
    if (details.m_iUserRating > 0 && details.m_iUserRating < 11)
      sql += PrepareSQL(", userrating = %i", details.m_iUserRating);
    else
      sql += ", userrating = NULL";
    if (details.HasPremiered())
      sql += PrepareSQL(", premiered = '%s'", details.GetPremiered().GetAsDBDate().c_str());
    else
      sql += PrepareSQL(", premiered = '%i'", details.GetYear());
    sql += PrepareSQL(" where idMVideo=%i", idMVideo);
    m_pDS->exec(sql);
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

void CVideoDatabase::SetStreamDetailsForFile(const CStreamDetails& details, const std::string &strFileNameAndPath)
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
        "(idFile, iStreamType, strVideoCodec, fVideoAspect, iVideoWidth, iVideoHeight, iVideoDuration, strStereoMode, strVideoLanguage) "
        "VALUES (%i,%i,'%s',%f,%i,%i,%i,'%s','%s')",
        idFile, (int)CStreamDetail::VIDEO,
        details.GetVideoCodec(i).c_str(), details.GetVideoAspect(i),
        details.GetVideoWidth(i), details.GetVideoHeight(i), details.GetVideoDuration(i),
        details.GetStereoMode(i).c_str(),
        details.GetVideoLanguage(i).c_str()));
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
      std::vector<std::pair<std::string, int> > tables;
      tables.emplace_back("movie", VIDEODB_ID_RUNTIME);
      tables.emplace_back("episode", VIDEODB_ID_EPISODE_RUNTIME);
      tables.emplace_back("musicvideo", VIDEODB_ID_MUSICVIDEO_RUNTIME);
      for (const auto &i : tables)
      {
        std::string sql = PrepareSQL("update %s set c%02d=%d where idFile=%d and c%02d=''",
                                    i.first.c_str(), i.second, details.GetVideoDuration(), idFile, i.second);
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
void CVideoDatabase::GetFilePathById(int idMovie, std::string &filePath, VIDEODB_CONTENT_TYPE iType)
{
  try
  {
    if (nullptr == m_pDB)
      return;
    if (nullptr == m_pDS)
      return;

    if (idMovie < 0) return ;

    std::string strSQL;
    if (iType == VIDEODB_CONTENT_MOVIES)
      strSQL=PrepareSQL("SELECT path.strPath, files.strFileName FROM path INNER JOIN files ON path.idPath=files.idPath INNER JOIN movie ON files.idFile=movie.idFile WHERE movie.idMovie=%i ORDER BY strFilename", idMovie );
    if (iType == VIDEODB_CONTENT_EPISODES)
      strSQL=PrepareSQL("SELECT path.strPath, files.strFileName FROM path INNER JOIN files ON path.idPath=files.idPath INNER JOIN episode ON files.idFile=episode.idFile WHERE episode.idEpisode=%i ORDER BY strFilename", idMovie );
    if (iType == VIDEODB_CONTENT_TVSHOWS)
      strSQL=PrepareSQL("SELECT path.strPath FROM path INNER JOIN tvshowlinkpath ON path.idPath=tvshowlinkpath.idPath WHERE tvshowlinkpath.idShow=%i", idMovie );
    if (iType ==VIDEODB_CONTENT_MUSICVIDEOS)
      strSQL=PrepareSQL("SELECT path.strPath, files.strFileName FROM path INNER JOIN files ON path.idPath=files.idPath INNER JOIN musicvideo ON files.idFile=musicvideo.idFile WHERE musicvideo.idMVideo=%i ORDER BY strFilename", idMovie );

    m_pDS->query( strSQL );
    if (!m_pDS->eof())
    {
      if (iType != VIDEODB_CONTENT_TVSHOWS)
      {
        std::string fileName = m_pDS->fv("files.strFilename").get_asString();
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
void CVideoDatabase::GetBookMarksForFile(const std::string& strFilenameAndPath, VECBOOKMARKS& bookmarks, CBookmark::EType type /*= CBookmark::STANDARD*/, bool bAppend, long partNumber)
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
      if (nullptr == m_pDB)
        return;
      if (nullptr == m_pDS)
        return;

      std::string strSQL=PrepareSQL("select * from bookmark where idFile=%i and type=%i order by timeInSeconds", idFile, (int)type);
      m_pDS->query( strSQL );
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
          std::string strSQL2=PrepareSQL("select c%02d, c%02d from episode where c%02d=%i order by c%02d, c%02d", VIDEODB_ID_EPISODE_EPISODE, VIDEODB_ID_EPISODE_SEASON, VIDEODB_ID_EPISODE_BOOKMARK, m_pDS->fv("idBookmark").get_asInt(), VIDEODB_ID_EPISODE_SORTSEASON, VIDEODB_ID_EPISODE_SORTEPISODE);
          m_pDS2->query(strSQL2);
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

bool CVideoDatabase::GetResumeBookMark(const std::string& strFilenameAndPath, CBookmark &bookmark)
{
  VECBOOKMARKS bookmarks;
  GetBookMarksForFile(strFilenameAndPath, bookmarks, CBookmark::RESUME);
  if (!bookmarks.empty())
  {
    bookmark = bookmarks[0];
    return true;
  }
  return false;
}

void CVideoDatabase::DeleteResumeBookMark(const CFileItem& item)
{
  if (!m_pDB || !m_pDS)
    return;

  int fileID = item.GetVideoInfoTag()->m_iFileId;
  if (fileID < 0)
  {
    fileID = GetFileId(item.GetPath());
    if (fileID < 0)
      return;
  }

  try
  {
    std::string sql = PrepareSQL("delete from bookmark where idFile=%i and type=%i", fileID, CBookmark::RESUME);
    m_pDS->exec(sql);

    VIDEODB_CONTENT_TYPE iType = static_cast<VIDEODB_CONTENT_TYPE>(item.GetVideoContentType());
    std::string content;
    switch (iType)
    {
      case VIDEODB_CONTENT_MOVIES:
        content = MediaTypeMovie;
        break;
      case VIDEODB_CONTENT_EPISODES:
        content = MediaTypeEpisode;
        break;
      case VIDEODB_CONTENT_TVSHOWS:
        content = MediaTypeTvShow;
        break;
      case VIDEODB_CONTENT_MUSICVIDEOS:
        content = MediaTypeMusicVideo;
        break;
      default:
        break;
    }

    if (!content.empty())
    {
      AnnounceUpdate(content, item.GetVideoInfoTag()->m_iDbId);
    }

  }
  catch(...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, item.GetVideoInfoTag()->m_strFileNameAndPath.c_str());
  }
}

void CVideoDatabase::GetEpisodesByFile(const std::string& strFilenameAndPath, std::vector<CVideoInfoTag>& episodes)
{
  try
  {
    std::string strSQL = PrepareSQL("select * from episode_view where idFile=%i order by c%02d, c%02d asc", GetFileId(strFilenameAndPath), VIDEODB_ID_EPISODE_SORTSEASON, VIDEODB_ID_EPISODE_SORTEPISODE);
    m_pDS->query(strSQL);
    while (!m_pDS->eof())
    {
      episodes.emplace_back(GetDetailsForEpisode(m_pDS));
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
void CVideoDatabase::AddBookMarkToFile(const std::string& strFilenameAndPath, const CBookmark &bookmark, CBookmark::EType type /*= CBookmark::STANDARD*/)
{
  try
  {
    int idFile = AddFile(strFilenameAndPath);
    if (idFile < 0)
      return;
    if (nullptr == m_pDB)
      return;
    if (nullptr == m_pDS)
      return;

    std::string strSQL;
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
      m_pDS->query( strSQL );
      if (m_pDS->num_rows() != 0)
        idBookmark = m_pDS->get_field_value("idBookmark").get_asInt();
      m_pDS->close();
    }
    // update or insert depending if it existed before
    if (idBookmark >= 0 )
      strSQL=PrepareSQL("update bookmark set timeInSeconds = %f, totalTimeInSeconds = %f, thumbNailImage = '%s', player = '%s', playerState = '%s' where idBookmark = %i", bookmark.timeInSeconds, bookmark.totalTimeInSeconds, bookmark.thumbNailImage.c_str(), bookmark.player.c_str(), bookmark.playerState.c_str(), idBookmark);
    else
      strSQL=PrepareSQL("insert into bookmark (idBookmark, idFile, timeInSeconds, totalTimeInSeconds, thumbNailImage, player, playerState, type) values(NULL,%i,%f,%f,'%s','%s','%s', %i)", idFile, bookmark.timeInSeconds, bookmark.totalTimeInSeconds, bookmark.thumbNailImage.c_str(), bookmark.player.c_str(), bookmark.playerState.c_str(), (int)type);

    m_pDS->exec(strSQL);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strFilenameAndPath.c_str());
  }
}

void CVideoDatabase::ClearBookMarkOfFile(const std::string& strFilenameAndPath, CBookmark& bookmark, CBookmark::EType type /*= CBookmark::STANDARD*/)
{
  try
  {
    int idFile = GetFileId(strFilenameAndPath);
    if (idFile < 0) return ;
    if (nullptr == m_pDB)
      return;
    if (nullptr == m_pDS)
      return;

    /* a little bit uggly, we clear first bookmark that is within one second of given */
    /* should be no problem since we never add bookmarks that are closer than that   */
    double mintime = bookmark.timeInSeconds - 0.5f;
    double maxtime = bookmark.timeInSeconds + 0.5f;
    std::string strSQL = PrepareSQL("select idBookmark from bookmark where idFile=%i and type=%i and playerState like '%s' and player like '%s' and (timeInSeconds between %f and %f)", idFile, type, bookmark.playerState.c_str(), bookmark.player.c_str(), mintime, maxtime);

    m_pDS->query( strSQL );
    if (m_pDS->num_rows() != 0)
    {
      int idBookmark = m_pDS->get_field_value("idBookmark").get_asInt();
      strSQL=PrepareSQL("delete from bookmark where idBookmark=%i",idBookmark);
      m_pDS->exec(strSQL);
      if (type == CBookmark::EPISODE)
      {
        strSQL=PrepareSQL("update episode set c%02d=-1 where idFile=%i and c%02d=%i", VIDEODB_ID_EPISODE_BOOKMARK, idFile, VIDEODB_ID_EPISODE_BOOKMARK, idBookmark);
        m_pDS->exec(strSQL);
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
void CVideoDatabase::ClearBookMarksOfFile(const std::string& strFilenameAndPath, CBookmark::EType type /*= CBookmark::STANDARD*/)
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
    if (nullptr == m_pDB)
      return;
    if (nullptr == m_pDS)
      return;

    std::string strSQL=PrepareSQL("delete from bookmark where idFile=%i and type=%i", idFile, (int)type);
    m_pDS->exec(strSQL);
    if (type == CBookmark::EPISODE)
    {
      strSQL=PrepareSQL("update episode set c%02d=-1 where idFile=%i", VIDEODB_ID_EPISODE_BOOKMARK, idFile);
      m_pDS->exec(strSQL);
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
    std::string strSQL = PrepareSQL("select bookmark.* from bookmark join episode on episode.c%02d=bookmark.idBookmark where episode.idEpisode=%i", VIDEODB_ID_EPISODE_BOOKMARK, tag.m_iDbId);
    m_pDS2->query( strSQL );
    if (!m_pDS2->eof())
    {
      bookmark.timeInSeconds = m_pDS2->fv("timeInSeconds").get_asDouble();
      bookmark.totalTimeInSeconds = m_pDS2->fv("totalTimeInSeconds").get_asDouble();
      bookmark.thumbNailImage = m_pDS2->fv("thumbNailImage").get_asString();
      bookmark.playerState = m_pDS2->fv("playerState").get_asString();
      bookmark.player = m_pDS2->fv("player").get_asString();
      bookmark.type = (CBookmark::EType)m_pDS2->fv("type").get_asInt();
    }
    else
    {
      m_pDS2->close();
      return false;
    }
    m_pDS2->close();
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
    std::string strSQL = PrepareSQL("delete from bookmark where idBookmark in (select c%02d from episode where c%02d=%i and c%02d=%i and idFile=%i)", VIDEODB_ID_EPISODE_BOOKMARK, VIDEODB_ID_EPISODE_SEASON, tag.m_iSeason, VIDEODB_ID_EPISODE_EPISODE, tag.m_iEpisode, idFile);
    m_pDS->exec(strSQL);

    AddBookMarkToFile(tag.m_strFileNameAndPath, bookmark, CBookmark::EPISODE);
    int idBookmark = (int)m_pDS->lastinsertid();
    strSQL = PrepareSQL("update episode set c%02d=%i where c%02d=%i and c%02d=%i and idFile=%i", VIDEODB_ID_EPISODE_BOOKMARK, idBookmark, VIDEODB_ID_EPISODE_SEASON, tag.m_iSeason, VIDEODB_ID_EPISODE_EPISODE, tag.m_iEpisode, idFile);
    m_pDS->exec(strSQL);
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
    std::string strSQL = PrepareSQL("delete from bookmark where idBookmark in (select c%02d from episode where idEpisode=%i)", VIDEODB_ID_EPISODE_BOOKMARK, tag.m_iDbId);
    m_pDS->exec(strSQL);
    strSQL = PrepareSQL("update episode set c%02d=-1 where idEpisode=%i", VIDEODB_ID_EPISODE_BOOKMARK, tag.m_iDbId);
    m_pDS->exec(strSQL);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%i) failed", __FUNCTION__, tag.m_iDbId);
  }
}

//********************************************************************************************************************************
void CVideoDatabase::DeleteMovie(const std::string& strFilenameAndPath, bool bKeepId /* = false */)
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
    if (nullptr == m_pDB)
      return;
    if (nullptr == m_pDS)
      return;

    BeginTransaction();

    // keep the movie table entry, linking to tv shows, and bookmarks
    // so we can update the data in place
    // the ancillary tables are still purged
    if (!bKeepId)
    {
      int idFile = GetDbId(PrepareSQL("SELECT idFile FROM movie WHERE idMovie=%i", idMovie));
      std::string path = GetSingleValue(PrepareSQL("SELECT strPath FROM path JOIN files ON files.idPath=path.idPath WHERE files.idFile=%i", idFile));
      if (!path.empty())
        InvalidatePathHash(path);

      std::string strSQL = PrepareSQL("delete from movie where idMovie=%i", idMovie);
      m_pDS->exec(strSQL);
    }

    //! @todo move this below CommitTransaction() once UPnP doesn't rely on this anymore
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

void CVideoDatabase::DeleteTvShow(const std::string& strPath)
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
    if (nullptr == m_pDB)
      return;
    if (nullptr == m_pDS)
      return;

    BeginTransaction();

    std::set<int> paths;
    GetPathsForTvShow(idTvShow, paths);

    std::string strSQL=PrepareSQL("SELECT episode.idEpisode FROM episode WHERE episode.idShow=%i",idTvShow);
    m_pDS2->query(strSQL);
    while (!m_pDS2->eof())
    {
      DeleteEpisode(m_pDS2->fv(0).get_asInt(), bKeepId);
      m_pDS2->next();
    }

    DeleteDetailsForTvShow(idTvShow);

    strSQL=PrepareSQL("delete from seasons where idShow=%i", idTvShow);
    m_pDS->exec(strSQL);

    // keep tvshow table and movielink table so we can update data in place
    if (!bKeepId)
    {
      strSQL=PrepareSQL("delete from tvshow where idShow=%i", idTvShow);
      m_pDS->exec(strSQL);

      for (const auto &i : paths)
      {
        std::string path = GetSingleValue(PrepareSQL("SELECT strPath FROM path WHERE idPath=%i", i));
        if (!path.empty())
          InvalidatePathHash(path);
      }
    }

    //! @todo move this below CommitTransaction() once UPnP doesn't rely on this anymore
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
    if (m_pDB == nullptr || m_pDS == nullptr || m_pDS2 == nullptr)
      return;

    BeginTransaction();

    std::string strSQL = PrepareSQL("SELECT episode.idEpisode FROM episode "
                                    "JOIN seasons ON seasons.idSeason = %d AND episode.idShow = seasons.idShow AND episode.c%02d = seasons.season ",
                                   idSeason, VIDEODB_ID_EPISODE_SEASON);
    m_pDS2->query(strSQL);
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

void CVideoDatabase::DeleteEpisode(const std::string& strFilenameAndPath, bool bKeepId /* = false */)
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
    if (nullptr == m_pDB)
      return;
    if (nullptr == m_pDS)
      return;

    //! @todo move this below CommitTransaction() once UPnP doesn't rely on this anymore
    if (!bKeepId)
      AnnounceRemove(MediaTypeEpisode, idEpisode);

    // keep episode table entry and bookmarks so we can update the data in place
    // the ancillary tables are still purged
    if (!bKeepId)
    {
      int idFile = GetDbId(PrepareSQL("SELECT idFile FROM episode WHERE idEpisode=%i", idEpisode));
      std::string path = GetSingleValue(PrepareSQL("SELECT strPath FROM path JOIN files ON files.idPath=path.idPath WHERE files.idFile=%i", idFile));
      if (!path.empty())
        InvalidatePathHash(path);

      std::string strSQL = PrepareSQL("delete from episode where idEpisode=%i", idEpisode);
      m_pDS->exec(strSQL);
    }

  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%d) failed", __FUNCTION__, idEpisode);
  }
}

void CVideoDatabase::DeleteMusicVideo(const std::string& strFilenameAndPath, bool bKeepId /* = false */)
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
    if (nullptr == m_pDB)
      return;
    if (nullptr == m_pDS)
      return;

    BeginTransaction();

    // keep the music video table entry and bookmarks so we can update data in place
    // the ancillary tables are still purged
    if (!bKeepId)
    {
      int idFile = GetDbId(PrepareSQL("SELECT idFile FROM musicvideo WHERE idMVideo=%i", idMVideo));
      std::string path = GetSingleValue(PrepareSQL("SELECT strPath FROM path JOIN files ON files.idPath=path.idPath WHERE files.idFile=%i", idFile));
      if (!path.empty())
        InvalidatePathHash(path);

      std::string strSQL = PrepareSQL("delete from musicvideo where idMVideo=%i", idMVideo);
      m_pDS->exec(strSQL);
    }

    //! @todo move this below CommitTransaction() once UPnP doesn't rely on this anymore
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
  m_pDS->exec(PrepareSQL("DELETE FROM streamdetails WHERE idFile = %i", idFile));
}

void CVideoDatabase::DeleteSet(int idSet)
{
  try
  {
    if (nullptr == m_pDB)
      return;
    if (nullptr == m_pDS)
      return;

    std::string strSQL;
    strSQL=PrepareSQL("delete from sets where idSet = %i", idSet);
    m_pDS->exec(strSQL);
    strSQL=PrepareSQL("update movie set idSet = null where idSet = %i", idSet);
    m_pDS->exec(strSQL);
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
    if (m_pDB == nullptr || m_pDS == nullptr)
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

    std::string strSQL = PrepareSQL("DELETE FROM tag_link WHERE tag_id = %i AND media_type = '%s'", idTag, type.c_str());
    m_pDS->exec(strSQL);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%i) failed", __FUNCTION__, idTag);
  }
}

void CVideoDatabase::GetDetailsFromDB(std::unique_ptr<Dataset> &pDS, int min, int max, const SDbTableOffsets *offsets, CVideoInfoTag &details, int idxOffset)
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
      *(std::string*)(((char*)&details)+offsets[i].offset) = record->at(i+idxOffset).get_asString();
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
    {
      std::string value = record->at(i+idxOffset).get_asString();
      if (!value.empty())
        *(std::vector<std::string>*)(((char*)&details)+offsets[i].offset) = StringUtils::Split(value, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoItemSeparator);
      break;
    }
    case VIDEODB_TYPE_DATE:
      ((CDateTime*)(((char*)&details)+offsets[i].offset))->SetFromDBDate(record->at(i+idxOffset).get_asString());
      break;
    case VIDEODB_TYPE_DATETIME:
      ((CDateTime*)(((char*)&details)+offsets[i].offset))->SetFromDBDateTime(record->at(i+idxOffset).get_asString());
      break;
    case VIDEODB_TYPE_UNUSED: // Skip the unused field to avoid populating unused data
      continue;
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

  std::unique_ptr<Dataset> pDS(m_pDB->CreateDataset());
  try
  {
    std::string strSQL = PrepareSQL("SELECT * FROM streamdetails WHERE idFile = %i", tag.m_iFileId);
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
          p->m_strLanguage = pDS->fv(12).get_asString();
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
    tag.SetDuration(details.GetVideoDuration());

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
      tag.SetResumePoint(CBookmark());
      for (int i = fileList.Size() - 1; i >= 0; i--)
      {
        CBookmark bookmark;
        if (GetResumeBookMark(fileList[i]->GetPath(), bookmark))
        {
          bookmark.partNumber = (i+1); /* store part number in here */
          tag.SetResumePoint(bookmark);
          match = true;
          break;
        }
      }
    }
    else
    {
      std::string strSQL=PrepareSQL("select timeInSeconds, totalTimeInSeconds from bookmark where idFile=%i and type=%i order by timeInSeconds", tag.m_iFileId, CBookmark::RESUME);
      m_pDS2->query( strSQL );
      if (!m_pDS2->eof())
      {
        tag.SetResumePoint(m_pDS2->fv(0).get_asDouble(), m_pDS2->fv(1).get_asDouble(), "");
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

CVideoInfoTag CVideoDatabase::GetDetailsForMovie(std::unique_ptr<Dataset> &pDS, int getDetails /* = VideoDbDetailsNone */)
{
  return GetDetailsForMovie(pDS->get_sql_record(), getDetails);
}

CVideoInfoTag CVideoDatabase::GetDetailsForMovie(const dbiplus::sql_record* const record, int getDetails /* = VideoDbDetailsNone */)
{
  CVideoInfoTag details;

  if (record == NULL)
    return details;

  DWORD time = XbmcThreads::SystemClockMillis();
  int idMovie = record->at(0).get_asInt();

  GetDetailsFromDB(record, VIDEODB_ID_MIN, VIDEODB_ID_MAX, DbMovieOffsets, details);

  details.m_iDbId = idMovie;
  details.m_type = MediaTypeMovie;

  details.m_set.id = record->at(VIDEODB_DETAILS_MOVIE_SET_ID).get_asInt();
  details.m_set.title = record->at(VIDEODB_DETAILS_MOVIE_SET_NAME).get_asString();
  details.m_set.overview = record->at(VIDEODB_DETAILS_MOVIE_SET_OVERVIEW).get_asString();
  details.m_iFileId = record->at(VIDEODB_DETAILS_FILEID).get_asInt();
  details.m_strPath = record->at(VIDEODB_DETAILS_MOVIE_PATH).get_asString();
  std::string strFileName = record->at(VIDEODB_DETAILS_MOVIE_FILE).get_asString();
  ConstructPath(details.m_strFileNameAndPath,details.m_strPath,strFileName);
  details.SetPlayCount(record->at(VIDEODB_DETAILS_MOVIE_PLAYCOUNT).get_asInt());
  details.m_lastPlayed.SetFromDBDateTime(record->at(VIDEODB_DETAILS_MOVIE_LASTPLAYED).get_asString());
  details.m_dateAdded.SetFromDBDateTime(record->at(VIDEODB_DETAILS_MOVIE_DATEADDED).get_asString());
  details.SetResumePoint(record->at(VIDEODB_DETAILS_MOVIE_RESUME_TIME).get_asInt(),
                         record->at(VIDEODB_DETAILS_MOVIE_TOTAL_TIME).get_asInt(),
                         record->at(VIDEODB_DETAILS_MOVIE_PLAYER_STATE).get_asString());
  details.m_iUserRating = record->at(VIDEODB_DETAILS_MOVIE_USER_RATING).get_asInt();
  details.SetRating(record->at(VIDEODB_DETAILS_MOVIE_RATING).get_asFloat(),
                    record->at(VIDEODB_DETAILS_MOVIE_VOTES).get_asInt(),
                    record->at(VIDEODB_DETAILS_MOVIE_RATING_TYPE).get_asString(), true);
  details.SetUniqueID(record->at(VIDEODB_DETAILS_MOVIE_UNIQUEID_VALUE).get_asString(), record->at(VIDEODB_DETAILS_MOVIE_UNIQUEID_TYPE).get_asString() ,true);
  std::string premieredString = record->at(VIDEODB_DETAILS_MOVIE_PREMIERED).get_asString();
  if (premieredString.size() == 4)
    details.SetYear(record->at(VIDEODB_DETAILS_MOVIE_PREMIERED).get_asInt());
  else
    details.SetPremieredFromDBDate(premieredString);
  movieTime += XbmcThreads::SystemClockMillis() - time; time = XbmcThreads::SystemClockMillis();

  if (getDetails)
  {
    if (getDetails & VideoDbDetailsCast)
    {
      GetCast(details.m_iDbId, MediaTypeMovie, details.m_cast);
      castTime += XbmcThreads::SystemClockMillis() - time; time = XbmcThreads::SystemClockMillis();
    }

    if (getDetails & VideoDbDetailsTag)
      GetTags(details.m_iDbId, MediaTypeMovie, details.m_tags);

    if (getDetails & VideoDbDetailsRating)
      GetRatings(details.m_iDbId, MediaTypeMovie, details.m_ratings);

    if (getDetails & VideoDbDetailsUniqueID)
     GetUniqueIDs(details.m_iDbId, MediaTypeMovie, details);

    if (getDetails & VideoDbDetailsShowLink)
    {
      // create tvshowlink string
      std::vector<int> links;
      GetLinksToTvShow(idMovie, links);
      for (unsigned int i = 0; i < links.size(); ++i)
      {
        std::string strSQL = PrepareSQL("select c%02d from tvshow where idShow=%i",
          VIDEODB_ID_TV_TITLE, links[i]);
        m_pDS2->query(strSQL);
        if (!m_pDS2->eof())
          details.m_showLink.emplace_back(m_pDS2->fv(0).get_asString());
      }
      m_pDS2->close();
    }

    if (getDetails & VideoDbDetailsStream)
      GetStreamDetails(details);

    details.m_parsedDetails = getDetails;
  }
  return details;
}

CVideoInfoTag CVideoDatabase::GetDetailsForTvShow(std::unique_ptr<Dataset> &pDS, int getDetails /* = VideoDbDetailsNone */, CFileItem* item /* = NULL */)
{
  return GetDetailsForTvShow(pDS->get_sql_record(), getDetails, item);
}

CVideoInfoTag CVideoDatabase::GetDetailsForTvShow(const dbiplus::sql_record* const record, int getDetails /* = VideoDbDetailsNone */, CFileItem* item /* = NULL */)
{
  CVideoInfoTag details;

  if (record == NULL)
    return details;

  DWORD time = XbmcThreads::SystemClockMillis();
  int idTvShow = record->at(0).get_asInt();

  GetDetailsFromDB(record, VIDEODB_ID_TV_MIN, VIDEODB_ID_TV_MAX, DbTvShowOffsets, details, 1);
  details.m_bHasPremiered = details.m_premiered.IsValid();
  details.m_iDbId = idTvShow;
  details.m_type = MediaTypeTvShow;
  details.m_strPath = record->at(VIDEODB_DETAILS_TVSHOW_PATH).get_asString();
  details.m_basePath = details.m_strPath;
  details.m_parentPathID = record->at(VIDEODB_DETAILS_TVSHOW_PARENTPATHID).get_asInt();
  details.m_dateAdded.SetFromDBDateTime(record->at(VIDEODB_DETAILS_TVSHOW_DATEADDED).get_asString());
  details.m_lastPlayed.SetFromDBDateTime(record->at(VIDEODB_DETAILS_TVSHOW_LASTPLAYED).get_asString());
  details.m_iSeason = record->at(VIDEODB_DETAILS_TVSHOW_NUM_SEASONS).get_asInt();
  details.m_iEpisode = record->at(VIDEODB_DETAILS_TVSHOW_NUM_EPISODES).get_asInt();
  details.SetPlayCount(record->at(VIDEODB_DETAILS_TVSHOW_NUM_WATCHED).get_asInt());
  details.m_strShowTitle = details.m_strTitle;
  details.m_iUserRating = record->at(VIDEODB_DETAILS_TVSHOW_USER_RATING).get_asInt();
  details.SetRating(record->at(VIDEODB_DETAILS_TVSHOW_RATING).get_asFloat(),
                    record->at(VIDEODB_DETAILS_TVSHOW_VOTES).get_asInt(),
                    record->at(VIDEODB_DETAILS_TVSHOW_RATING_TYPE).get_asString(), true);
  details.SetUniqueID(record->at(VIDEODB_DETAILS_TVSHOW_UNIQUEID_VALUE).get_asString(), record->at(VIDEODB_DETAILS_TVSHOW_UNIQUEID_TYPE).get_asString(), true);
  details.SetDuration(record->at(VIDEODB_DETAILS_TVSHOW_DURATION).get_asInt());

  movieTime += XbmcThreads::SystemClockMillis() - time; time = XbmcThreads::SystemClockMillis();

  if (getDetails)
  {
    if (getDetails & VideoDbDetailsCast)
    {
      GetCast(details.m_iDbId, "tvshow", details.m_cast);
      castTime += XbmcThreads::SystemClockMillis() - time; time = XbmcThreads::SystemClockMillis();
    }

    if (getDetails & VideoDbDetailsTag)
      GetTags(details.m_iDbId, MediaTypeTvShow, details.m_tags);

    if (getDetails & VideoDbDetailsRating)
      GetRatings(details.m_iDbId, MediaTypeTvShow, details.m_ratings);

    if (getDetails & VideoDbDetailsUniqueID)
      GetUniqueIDs(details.m_iDbId, MediaTypeTvShow, details);

    details.m_parsedDetails = getDetails;
  }

  if (item != NULL)
  {
    item->m_dateTime = details.GetPremiered();
    item->SetProperty("totalseasons", details.m_iSeason);
    item->SetProperty("totalepisodes", details.m_iEpisode);
    item->SetProperty("numepisodes", details.m_iEpisode); // will be changed later to reflect watchmode setting
    item->SetProperty("watchedepisodes", details.GetPlayCount());
    item->SetProperty("unwatchedepisodes", details.m_iEpisode - details.GetPlayCount());
  }
  details.SetPlayCount((details.m_iEpisode <= details.GetPlayCount()) ? 1 : 0);

  return details;
}

CVideoInfoTag CVideoDatabase::GetBasicDetailsForEpisode(std::unique_ptr<Dataset> &pDS)
{
  return GetBasicDetailsForEpisode(pDS->get_sql_record());
}

CVideoInfoTag CVideoDatabase::GetBasicDetailsForEpisode(const dbiplus::sql_record* const record)
{
  CVideoInfoTag details;

  if (record == nullptr)
    return details;

  unsigned int time = XbmcThreads::SystemClockMillis();
  int idEpisode = record->at(0).get_asInt();

  GetDetailsFromDB(record, VIDEODB_ID_EPISODE_MIN, VIDEODB_ID_EPISODE_MAX, DbEpisodeOffsets, details);
  details.m_iDbId = idEpisode;
  details.m_type = MediaTypeEpisode;
  details.m_iFileId = record->at(VIDEODB_DETAILS_FILEID).get_asInt();
  details.m_iIdShow = record->at(VIDEODB_DETAILS_EPISODE_TVSHOW_ID).get_asInt();
  details.m_iIdSeason = record->at(VIDEODB_DETAILS_EPISODE_SEASON_ID).get_asInt();
  details.m_iUserRating = record->at(VIDEODB_DETAILS_EPISODE_USER_RATING).get_asInt();

  movieTime += XbmcThreads::SystemClockMillis() - time;
  return details;
}

CVideoInfoTag CVideoDatabase::GetDetailsForEpisode(std::unique_ptr<Dataset> &pDS, int getDetails /* = VideoDbDetailsNone */)
{
  return GetDetailsForEpisode(pDS->get_sql_record(), getDetails);
}

CVideoInfoTag CVideoDatabase::GetDetailsForEpisode(const dbiplus::sql_record* const record, int getDetails /* = VideoDbDetailsNone */)
{
  CVideoInfoTag details;

  if (record == nullptr)
    return details;

  details = GetBasicDetailsForEpisode(record);
  
  unsigned int time = XbmcThreads::SystemClockMillis();

  details.m_strPath = record->at(VIDEODB_DETAILS_EPISODE_PATH).get_asString();
  std::string strFileName = record->at(VIDEODB_DETAILS_EPISODE_FILE).get_asString();
  ConstructPath(details.m_strFileNameAndPath,details.m_strPath,strFileName);
  details.SetPlayCount(record->at(VIDEODB_DETAILS_EPISODE_PLAYCOUNT).get_asInt());
  details.m_lastPlayed.SetFromDBDateTime(record->at(VIDEODB_DETAILS_EPISODE_LASTPLAYED).get_asString());
  details.m_dateAdded.SetFromDBDateTime(record->at(VIDEODB_DETAILS_EPISODE_DATEADDED).get_asString());
  details.m_strMPAARating = record->at(VIDEODB_DETAILS_EPISODE_TVSHOW_MPAA).get_asString();
  details.m_strShowTitle = record->at(VIDEODB_DETAILS_EPISODE_TVSHOW_NAME).get_asString();
  details.m_genre = StringUtils::Split(record->at(VIDEODB_DETAILS_EPISODE_TVSHOW_GENRE).get_asString(), CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoItemSeparator);
  details.m_studio = StringUtils::Split(record->at(VIDEODB_DETAILS_EPISODE_TVSHOW_STUDIO).get_asString(), CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoItemSeparator);
  details.SetPremieredFromDBDate(record->at(VIDEODB_DETAILS_EPISODE_TVSHOW_AIRED).get_asString());
  
  details.SetResumePoint(record->at(VIDEODB_DETAILS_EPISODE_RESUME_TIME).get_asInt(),
                         record->at(VIDEODB_DETAILS_EPISODE_TOTAL_TIME).get_asInt(),
                         record->at(VIDEODB_DETAILS_EPISODE_PLAYER_STATE).get_asString());

  details.SetRating(record->at(VIDEODB_DETAILS_EPISODE_RATING).get_asFloat(),
                    record->at(VIDEODB_DETAILS_EPISODE_VOTES).get_asInt(),
                    record->at(VIDEODB_DETAILS_EPISODE_RATING_TYPE).get_asString(), true);
  details.SetUniqueID(record->at(VIDEODB_DETAILS_EPISODE_UNIQUEID_VALUE).get_asString(), record->at(VIDEODB_DETAILS_EPISODE_UNIQUEID_TYPE).get_asString(), true);
  movieTime += XbmcThreads::SystemClockMillis() - time; time = XbmcThreads::SystemClockMillis();

  if (getDetails)
  {
    if (getDetails & VideoDbDetailsCast)
    {
      GetCast(details.m_iDbId, MediaTypeEpisode, details.m_cast);
      GetCast(details.m_iIdShow, MediaTypeTvShow, details.m_cast);
      castTime += XbmcThreads::SystemClockMillis() - time; time = XbmcThreads::SystemClockMillis();
    }

    if (getDetails & VideoDbDetailsRating)
      GetRatings(details.m_iDbId, MediaTypeEpisode, details.m_ratings);

    if (getDetails & VideoDbDetailsUniqueID)
      GetUniqueIDs(details.m_iDbId, MediaTypeEpisode, details);

    if (getDetails &  VideoDbDetailsBookmark)
      GetBookMarkForEpisode(details, details.m_EpBookmark);

    if (getDetails & VideoDbDetailsStream)
      GetStreamDetails(details);

    details.m_parsedDetails = getDetails;
  }
  return details;
}

CVideoInfoTag CVideoDatabase::GetDetailsForMusicVideo(std::unique_ptr<Dataset> &pDS, int getDetails /* = VideoDbDetailsNone */)
{
  return GetDetailsForMusicVideo(pDS->get_sql_record(), getDetails);
}

CVideoInfoTag CVideoDatabase::GetDetailsForMusicVideo(const dbiplus::sql_record* const record, int getDetails /* = VideoDbDetailsNone */)
{
  CVideoInfoTag details;

  if (record == nullptr)
    return details;

  unsigned int time = XbmcThreads::SystemClockMillis();
  int idMVideo = record->at(0).get_asInt();

  GetDetailsFromDB(record, VIDEODB_ID_MUSICVIDEO_MIN, VIDEODB_ID_MUSICVIDEO_MAX, DbMusicVideoOffsets, details);
  details.m_iDbId = idMVideo;
  details.m_type = MediaTypeMusicVideo;

  details.m_iFileId = record->at(VIDEODB_DETAILS_FILEID).get_asInt();
  details.m_strPath = record->at(VIDEODB_DETAILS_MUSICVIDEO_PATH).get_asString();
  std::string strFileName = record->at(VIDEODB_DETAILS_MUSICVIDEO_FILE).get_asString();
  ConstructPath(details.m_strFileNameAndPath,details.m_strPath,strFileName);
  details.SetPlayCount(record->at(VIDEODB_DETAILS_MUSICVIDEO_PLAYCOUNT).get_asInt());
  details.m_lastPlayed.SetFromDBDateTime(record->at(VIDEODB_DETAILS_MUSICVIDEO_LASTPLAYED).get_asString());
  details.m_dateAdded.SetFromDBDateTime(record->at(VIDEODB_DETAILS_MUSICVIDEO_DATEADDED).get_asString());
  details.SetResumePoint(record->at(VIDEODB_DETAILS_MUSICVIDEO_RESUME_TIME).get_asInt(),
                         record->at(VIDEODB_DETAILS_MUSICVIDEO_TOTAL_TIME).get_asInt(),
                         record->at(VIDEODB_DETAILS_MUSICVIDEO_PLAYER_STATE).get_asString());
  details.m_iUserRating = record->at(VIDEODB_DETAILS_MUSICVIDEO_USER_RATING).get_asInt();
  std::string premieredString = record->at(VIDEODB_DETAILS_MUSICVIDEO_PREMIERED).get_asString();
  if (premieredString.size() == 4)
    details.SetYear(record->at(VIDEODB_DETAILS_MUSICVIDEO_PREMIERED).get_asInt());
  else
    details.SetPremieredFromDBDate(premieredString);

  movieTime += XbmcThreads::SystemClockMillis() - time; time = XbmcThreads::SystemClockMillis();

  if (getDetails)
  {
    if (getDetails & VideoDbDetailsTag)
      GetTags(details.m_iDbId, MediaTypeMusicVideo, details.m_tags);

    if (getDetails & VideoDbDetailsStream)
      GetStreamDetails(details);

    details.m_parsedDetails = getDetails;
  }
  return details;
}

void CVideoDatabase::GetCast(int media_id, const std::string &media_type, std::vector<SActorInfo> &cast)
{
  try
  {
    if (!m_pDB)
      return;
    if (!m_pDS2)
      return;

    std::string sql = PrepareSQL("SELECT actor.name,"
                                 "  actor_link.role,"
                                 "  actor_link.cast_order,"
                                 "  actor.art_urls,"
                                 "  art.url "
                                 "FROM actor_link"
                                 "  JOIN actor ON"
                                 "    actor_link.actor_id=actor.actor_id"
                                 "  LEFT JOIN art ON"
                                 "    art.media_id=actor.actor_id AND art.media_type='actor' AND art.type='thumb' "
                                 "WHERE actor_link.media_id=%i AND actor_link.media_type='%s'"
                                 "ORDER BY actor_link.cast_order", media_id, media_type.c_str());
    m_pDS2->query(sql);
    while (!m_pDS2->eof())
    {
      SActorInfo info;
      info.strName = m_pDS2->fv(0).get_asString();
      info.strRole = m_pDS2->fv(1).get_asString();
      info.order = m_pDS2->fv(2).get_asInt();
      info.thumbUrl.ParseFromData(m_pDS2->fv(3).get_asString());
      info.thumb = m_pDS2->fv(4).get_asString();
      cast.emplace_back(std::move(info));

      m_pDS2->next();
    }
    m_pDS2->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%i,%s) failed", __FUNCTION__, media_id, media_type.c_str());
  }
}

void CVideoDatabase::GetTags(int media_id, const std::string &media_type, std::vector<std::string> &tags)
{
  try
  {
    if (!m_pDB)
      return;
    if (!m_pDS2)
      return;

    std::string sql = PrepareSQL("SELECT tag.name FROM tag INNER JOIN tag_link ON tag_link.tag_id = tag.tag_id WHERE tag_link.media_id = %i AND tag_link.media_type = '%s' ORDER BY tag.tag_id", media_id, media_type.c_str());
    m_pDS2->query(sql);
    while (!m_pDS2->eof())
    {
      tags.emplace_back(m_pDS2->fv(0).get_asString());
      m_pDS2->next();
    }
    m_pDS2->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%i,%s) failed", __FUNCTION__, media_id, media_type.c_str());
  }
}

void CVideoDatabase::GetRatings(int media_id, const std::string &media_type, RatingMap &ratings)
{
  try
  {
    if (!m_pDB)
      return;
    if (!m_pDS2)
      return;

    std::string sql = PrepareSQL("SELECT rating.rating_type, rating.rating, rating.votes FROM rating WHERE rating.media_id = %i AND rating.media_type = '%s'", media_id, media_type.c_str());
    m_pDS2->query(sql);
    while (!m_pDS2->eof())
    {
      ratings[m_pDS2->fv(0).get_asString()] = CRating(m_pDS2->fv(1).get_asFloat(), m_pDS2->fv(2).get_asInt());
      m_pDS2->next();
    }
    m_pDS2->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%i,%s) failed", __FUNCTION__, media_id, media_type.c_str());
  }
}

void CVideoDatabase::GetUniqueIDs(int media_id, const std::string &media_type, CVideoInfoTag& details)
{
  try
  {
    if (!m_pDB)
      return;
    if (!m_pDS2)
      return;

    std::string sql = PrepareSQL("SELECT type, value FROM uniqueid WHERE media_id = %i AND media_type = '%s'", media_id, media_type.c_str());
    m_pDS2->query(sql);
    while (!m_pDS2->eof())
    {
      details.SetUniqueID(m_pDS2->fv(1).get_asString(), m_pDS2->fv(0).get_asString());
      m_pDS2->next();
    }
    m_pDS2->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%i,%s) failed", __FUNCTION__, media_id, media_type.c_str());
  }
}

bool CVideoDatabase::GetVideoSettings(const CFileItem &item, CVideoSettings &settings)
{
  return GetVideoSettings(GetFileId(item), settings);
}

/// \brief GetVideoSettings() obtains any saved video settings for the current file.
/// \retval Returns true if the settings exist, false otherwise.
bool CVideoDatabase::GetVideoSettings(const std::string &filePath, CVideoSettings &settings)
{
  return GetVideoSettings(GetFileId(filePath), settings);
}

bool CVideoDatabase::GetVideoSettings(int idFile, CVideoSettings &settings)
{
  try
  {
    if (idFile < 0) return false;
    if (nullptr == m_pDB)
      return false;
    if (nullptr == m_pDS)
      return false;

    std::string strSQL=PrepareSQL("select * from settings where settings.idFile = '%i'", idFile);
    m_pDS->query( strSQL );

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
      settings.m_InterlaceMethod = (EINTERLACEMETHOD)m_pDS->fv("Deinterlace").get_asInt();
      settings.m_VolumeAmplification = m_pDS->fv("VolumeAmplification").get_asFloat();
      settings.m_ScalingMethod = (ESCALINGMETHOD)m_pDS->fv("ScalingMethod").get_asInt();
      settings.m_StereoMode = m_pDS->fv("StereoMode").get_asInt();
      settings.m_StereoInvert = m_pDS->fv("StereoInvert").get_asBool();
      settings.m_SubtitleCached = false;
      settings.m_VideoStream = m_pDS->fv("VideoStream").get_asInt();
      settings.m_ToneMapMethod = m_pDS->fv("TonemapMethod").get_asInt();
      settings.m_ToneMapParam = m_pDS->fv("TonemapParam").get_asFloat();
      settings.m_Orientation = m_pDS->fv("Orientation").get_asInt();
      settings.m_CenterMixLevel = m_pDS->fv("CenterMixLevel").get_asInt();
      m_pDS->close();

      if (settings.m_ToneMapParam == 0.0)
      {
        settings.m_ToneMapMethod = VS_TONEMAPMETHOD_REINHARD;
        settings.m_ToneMapParam = 1.0;
      }
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

void CVideoDatabase::SetVideoSettings(const CFileItem &item, const CVideoSettings &settings)
{
  int idFile = AddFile(item);
  SetVideoSettings(idFile, settings);
}

/// \brief Sets the settings for a particular video file
void CVideoDatabase::SetVideoSettings(int idFile, const CVideoSettings &setting)
{
  try
  {
    if (nullptr == m_pDB)
      return;
    if (nullptr == m_pDS)
      return;
    if (idFile < 0)
      return;
    std::string strSQL = PrepareSQL("select * from settings where idFile=%i", idFile);
    m_pDS->query( strSQL );
    if (m_pDS->num_rows() > 0)
    {
      m_pDS->close();
      // update the item
      strSQL=PrepareSQL("update settings set Deinterlace=%i,ViewMode=%i,ZoomAmount=%f,PixelRatio=%f,VerticalShift=%f,"
                       "AudioStream=%i,SubtitleStream=%i,SubtitleDelay=%f,SubtitlesOn=%i,Brightness=%f,Contrast=%f,Gamma=%f,"
                       "VolumeAmplification=%f,AudioDelay=%f,Sharpness=%f,NoiseReduction=%f,NonLinStretch=%i,PostProcess=%i,ScalingMethod=%i,",
                       setting.m_InterlaceMethod, setting.m_ViewMode, setting.m_CustomZoomAmount, setting.m_CustomPixelRatio, setting.m_CustomVerticalShift,
                       setting.m_AudioStream, setting.m_SubtitleStream, setting.m_SubtitleDelay, setting.m_SubtitleOn,
                       setting.m_Brightness, setting.m_Contrast, setting.m_Gamma, setting.m_VolumeAmplification, setting.m_AudioDelay,
                       setting.m_Sharpness,setting.m_NoiseReduction,setting.m_CustomNonLinStretch,setting.m_PostProcess,setting.m_ScalingMethod);
      std::string strSQL2;

      strSQL2=PrepareSQL("ResumeTime=%i,StereoMode=%i,StereoInvert=%i,VideoStream=%i,TonemapMethod=%i,TonemapParam=%f where idFile=%i\n",
                         setting.m_ResumeTime, setting.m_StereoMode, setting.m_StereoInvert, setting.m_VideoStream,
                         setting.m_ToneMapMethod, setting.m_ToneMapParam, idFile);
      strSQL += strSQL2;
      m_pDS->exec(strSQL);
      return ;
    }
    else
    { // add the items
      m_pDS->close();
      strSQL= "INSERT INTO settings (idFile,Deinterlace,ViewMode,ZoomAmount,PixelRatio, VerticalShift, "
                "AudioStream,SubtitleStream,SubtitleDelay,SubtitlesOn,Brightness,"
                "Contrast,Gamma,VolumeAmplification,AudioDelay,"
                "ResumeTime,"
                "Sharpness,NoiseReduction,NonLinStretch,PostProcess,ScalingMethod,StereoMode,StereoInvert,VideoStream,TonemapMethod,TonemapParam,Orientation,CenterMixLevel) "
              "VALUES ";
      strSQL += PrepareSQL("(%i,%i,%i,%f,%f,%f,%i,%i,%f,%i,%f,%f,%f,%f,%f,%i,%f,%f,%i,%i,%i,%i,%i,%i,%i,%f,%i,%i)",
                           idFile, setting.m_InterlaceMethod, setting.m_ViewMode, setting.m_CustomZoomAmount, setting.m_CustomPixelRatio, setting.m_CustomVerticalShift,
                           setting.m_AudioStream, setting.m_SubtitleStream, setting.m_SubtitleDelay, setting.m_SubtitleOn, setting.m_Brightness,
                           setting.m_Contrast, setting.m_Gamma, setting.m_VolumeAmplification, setting.m_AudioDelay,
                           setting.m_ResumeTime,
                           setting.m_Sharpness, setting.m_NoiseReduction, setting.m_CustomNonLinStretch, setting.m_PostProcess, setting.m_ScalingMethod,
                           setting.m_StereoMode, setting.m_StereoInvert, setting.m_VideoStream, setting.m_ToneMapMethod, setting.m_ToneMapParam, setting.m_Orientation,setting.m_CenterMixLevel);
      m_pDS->exec(strSQL);
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%d) failed", __FUNCTION__, idFile);
  }
}

void CVideoDatabase::SetArtForItem(int mediaId, const MediaType &mediaType, const std::map<std::string, std::string> &art)
{
  for (const auto &i : art)
    SetArtForItem(mediaId, mediaType, i.first, i.second);
}

void CVideoDatabase::SetArtForItem(int mediaId, const MediaType &mediaType, const std::string &artType, const std::string &url)
{
  try
  {
    if (nullptr == m_pDB)
      return;
    if (nullptr == m_pDS)
      return;

    // don't set <foo>.<bar> art types - these are derivative types from parent items
    if (artType.find('.') != std::string::npos)
      return;

    std::string sql = PrepareSQL("SELECT art_id,url FROM art WHERE media_id=%i AND media_type='%s' AND type='%s'", mediaId, mediaType.c_str(), artType.c_str());
    m_pDS->query(sql);
    if (!m_pDS->eof())
    { // update
      int artId = m_pDS->fv(0).get_asInt();
      std::string oldUrl = m_pDS->fv(1).get_asString();
      m_pDS->close();
      if (oldUrl != url)
      {
        sql = PrepareSQL("UPDATE art SET url='%s' where art_id=%d", url.c_str(), artId);
        m_pDS->exec(sql);
      }
    }
    else
    { // insert
      m_pDS->close();
      sql = PrepareSQL("INSERT INTO art(media_id, media_type, type, url) VALUES (%d, '%s', '%s', '%s')", mediaId, mediaType.c_str(), artType.c_str(), url.c_str());
      m_pDS->exec(sql);
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%d, '%s', '%s', '%s') failed", __FUNCTION__, mediaId, mediaType.c_str(), artType.c_str(), url.c_str());
  }
}

bool CVideoDatabase::GetArtForItem(int mediaId, const MediaType &mediaType, std::map<std::string, std::string> &art)
{
  try
  {
    if (nullptr == m_pDB)
      return false;
    if (nullptr == m_pDS2)
      return false; // using dataset 2 as we're likely called in loops on dataset 1

    std::string sql = PrepareSQL("SELECT type,url FROM art WHERE media_id=%i AND media_type='%s'", mediaId, mediaType.c_str());
    m_pDS2->query(sql);
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

std::string CVideoDatabase::GetArtForItem(int mediaId, const MediaType &mediaType, const std::string &artType)
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
  for (const auto &i : artTypes)
    result &= RemoveArtForItem(mediaId, mediaType, i);

  return result;
}

bool CVideoDatabase::HasArtForItem(int mediaId, const MediaType &mediaType)
{
  try
  {
    if (nullptr == m_pDB)
      return false;
    if (nullptr == m_pDS2)
      return false; // using dataset 2 as we're likely called in loops on dataset 1

    std::string sql = PrepareSQL("SELECT 1 FROM art WHERE media_id=%i AND media_type='%s' LIMIT 1", mediaId, mediaType.c_str());
    m_pDS2->query(sql);
    bool result = !m_pDS2->eof();
    m_pDS2->close();
    return result;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%d) failed", __FUNCTION__, mediaId);
  }
  return false;
}

bool CVideoDatabase::GetTvShowSeasons(int showId, std::map<int, int> &seasons)
{
  try
  {
    if (nullptr == m_pDB)
      return false;
    if (nullptr == m_pDS2)
      return false; // using dataset 2 as we're likely called in loops on dataset 1

    // get all seasons for this show
    std::string sql = PrepareSQL("select idSeason,season from seasons where idShow=%i", showId);
    m_pDS2->query(sql);

    seasons.clear();
    while (!m_pDS2->eof())
    {
      seasons.insert(std::make_pair(m_pDS2->fv(1).get_asInt(), m_pDS2->fv(0).get_asInt()));
      m_pDS2->next();
    }
    m_pDS2->close();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%d) failed", __FUNCTION__, showId);
  }
  return false;
}

bool CVideoDatabase::GetTvShowNamedSeasons(int showId, std::map<int, std::string> &seasons)
{
  try
  {
    if (nullptr == m_pDB)
      return false;
    if (nullptr == m_pDS2)
      return false; // using dataset 2 as we're likely called in loops on dataset 1

    // get all named seasons for this show
    std::string sql = PrepareSQL("select season, name from seasons where season > 0 and name is not null and name <> '' and idShow = %i", showId);
    m_pDS2->query(sql);

    seasons.clear();
    while (!m_pDS2->eof())
    {
      seasons.insert(std::make_pair(m_pDS2->fv(0).get_asInt(), m_pDS2->fv(1).get_asString()));
      m_pDS2->next();
    }
    m_pDS2->close();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%d) failed", __FUNCTION__, showId);
  }
  return false;
}

bool CVideoDatabase::GetTvShowSeasonArt(int showId, std::map<int, std::map<std::string, std::string> > &seasonArt)
{
  try
  {
    if (nullptr == m_pDB)
      return false;
    if (nullptr == m_pDS2)
      return false; // using dataset 2 as we're likely called in loops on dataset 1

    std::map<int, int> seasons;
    GetTvShowSeasons(showId, seasons);

    for (const auto &i : seasons)
    {
      std::map<std::string, std::string> art;
      GetArtForItem(i.second, MediaTypeSeason, art);
      seasonArt.insert(std::make_pair(i.first,art));
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
    if (nullptr == m_pDB)
      return false;
    if (nullptr == m_pDS)
      return false;

    std::string sql = PrepareSQL("SELECT DISTINCT type FROM art WHERE media_type='%s'", mediaType.c_str());
    int numRows = RunQuery(sql);
    if (numRows <= 0)
      return numRows == 0;

    while (!m_pDS->eof())
    {
      artTypes.emplace_back(m_pDS->fv(0).get_asString());
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

namespace
{
std::vector<std::string> GetBasicItemAvailableArtTypes(int mediaId,
                                                       VIDEODB_CONTENT_TYPE dbType,
                                                       CVideoDatabase& db)
{
  std::vector<std::string> result;
  CVideoInfoTag tag = db.GetDetailsByTypeAndId(dbType, mediaId);

  //! @todo artwork: fanart stored separately, doesn't need to be
  if (tag.m_fanart.GetNumFanarts() && std::find(result.cbegin(), result.cend(), "fanart") == result.cend())
    result.emplace_back("fanart");

  // all other images
  tag.m_strPictureURL.Parse();
  for (const auto& urlEntry : tag.m_strPictureURL.GetUrls())
  {
    std::string artType = urlEntry.m_aspect;
    if (artType.empty())
      artType = tag.m_type == MediaTypeEpisode ? "thumb" : "poster";
    if (urlEntry.m_type == CScraperUrl::UrlType::General && // exclude season artwork for TV shows
        !StringUtils::StartsWith(artType, "set.") && // exclude movie set artwork for movies
        std::find(result.cbegin(), result.cend(), artType) == result.cend())
    {
      result.push_back(artType);
    }
  }
  return result;
}

std::vector<std::string> GetSeasonAvailableArtTypes(int mediaId, CVideoDatabase& db)
{
  CVideoInfoTag tag;
  db.GetSeasonInfo(mediaId, tag);

  std::vector<std::string> result;

  CVideoInfoTag sourceShow;
  db.GetTvShowInfo("", sourceShow, tag.m_iIdShow);
  sourceShow.m_strPictureURL.Parse();
  for (const auto& urlEntry : sourceShow.m_strPictureURL.GetUrls())
  {
    std::string artType = urlEntry.m_aspect;
    if (artType.empty())
      artType = "poster";
    if (urlEntry.m_type == CScraperUrl::UrlType::Season && urlEntry.m_season == tag.m_iSeason &&
        std::find(result.cbegin(), result.cend(), artType) == result.cend())
    {
      result.push_back(artType);
    }
  }
  return result;
}

std::vector<std::string> GetMovieSetAvailableArtTypes(int mediaId, CVideoDatabase& db)
{
  std::vector<std::string> result;
  CFileItemList items;
  std::string baseDir = StringUtils::Format("videodb://movies/sets/%d", mediaId);
  if (db.GetMoviesNav(baseDir, items))
  {
    for (const auto& item : items)
    {
      CVideoInfoTag* pTag = item->GetVideoInfoTag();
      pTag->m_strPictureURL.Parse();

      for (const auto& urlEntry : pTag->m_strPictureURL.GetUrls())
      {
        if (!StringUtils::StartsWith(urlEntry.m_aspect, "set."))
          continue;

        std::string artType = urlEntry.m_aspect.substr(4);
        if (std::find(result.cbegin(), result.cend(), artType) == result.cend())
          result.push_back(artType);
      }
    }
  }
  return result;
}
}

std::vector<std::string> CVideoDatabase::GetAvailableArtTypesForItem(int mediaId,
  const MediaType& mediaType)
{
  VIDEODB_CONTENT_TYPE dbType{VIDEODB_CONTENT_UNKNOWN};
  if (mediaType == MediaTypeTvShow)
    dbType = VIDEODB_CONTENT_TVSHOWS;
  else if (mediaType == MediaTypeMovie)
    dbType = VIDEODB_CONTENT_MOVIES;
  else if (mediaType == MediaTypeEpisode)
    dbType = VIDEODB_CONTENT_EPISODES;
  else if (mediaType == MediaTypeMusicVideo)
    dbType = VIDEODB_CONTENT_MUSICVIDEOS;

  if (dbType != VIDEODB_CONTENT_UNKNOWN)
    return GetBasicItemAvailableArtTypes(mediaId, dbType, *this);
  if (mediaType == MediaTypeSeason)
    return GetSeasonAvailableArtTypes(mediaId, *this);
  if (mediaType == MediaTypeVideoCollection)
    return GetMovieSetAvailableArtTypes(mediaId, *this);
  return {};
}

/// \brief GetStackTimes() obtains any saved video times for the stacked file
/// \retval Returns true if the stack times exist, false otherwise.
bool CVideoDatabase::GetStackTimes(const std::string &filePath, std::vector<uint64_t> &times)
{
  try
  {
    // obtain the FileID (if it exists)
    int idFile = GetFileId(filePath);
    if (idFile < 0) return false;
    if (nullptr == m_pDB)
      return false;
    if (nullptr == m_pDS)
      return false;
    // ok, now obtain the settings for this file
    std::string strSQL=PrepareSQL("select times from stacktimes where idFile=%i\n", idFile);
    m_pDS->query( strSQL );
    if (m_pDS->num_rows() > 0)
    { // get the video settings info
      uint64_t timeTotal = 0;
      std::vector<std::string> timeString = StringUtils::Split(m_pDS->fv("times").get_asString(), ",");
      times.clear();
      for (const auto &i : timeString)
      {
        uint64_t partTime = static_cast<uint64_t>(atof(i.c_str()) * 1000.0f);
        times.push_back(partTime); // db stores in secs, convert to msecs
        timeTotal += partTime;
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
void CVideoDatabase::SetStackTimes(const std::string& filePath, const std::vector<uint64_t> &times)
{
  try
  {
    if (nullptr == m_pDB)
      return;
    if (nullptr == m_pDS)
      return;
    int idFile = AddFile(filePath);
    if (idFile < 0)
      return;

    // delete any existing items
    m_pDS->exec( PrepareSQL("delete from stacktimes where idFile=%i", idFile) );

    // add the items
    std::string timeString = StringUtils::Format("%.3f", times[0] / 1000.0f);
    for (unsigned int i = 1; i < times.size(); i++)
      timeString += StringUtils::Format(",%.3f", times[i] / 1000.0f);

    m_pDS->exec( PrepareSQL("insert into stacktimes (idFile,times) values (%i,'%s')\n", idFile, timeString.c_str()) );
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, filePath.c_str());
  }
}

void CVideoDatabase::RemoveContentForPath(const std::string& strPath, CGUIDialogProgress *progress /* = NULL */)
{
  if(URIUtils::IsMultiPath(strPath))
  {
    std::vector<std::string> paths;
    CMultiPathDirectory::GetPaths(strPath, paths);

    for(unsigned i=0;i<paths.size();i++)
      RemoveContentForPath(paths[i], progress);
  }

  try
  {
    if (nullptr == m_pDB)
      return;
    if (nullptr == m_pDS)
      return;

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
    std::vector<std::pair<int, std::string> > paths;
    GetSubPaths(strPath, paths);
    int iCurr = 0;
    for (const auto &i : paths)
    {
      bool bMvidsChecked=false;
      if (progress)
      {
        progress->SetPercentage((int)((float)(iCurr++)/paths.size()*100.f));
        progress->Progress();
      }

      if (HasTvShowInfo(i.second))
        DeleteTvShow(i.second);
      else
      {
        std::string strSQL = PrepareSQL("select files.strFilename from files join movie on movie.idFile=files.idFile where files.idPath=%i", i.first);
        m_pDS2->query(strSQL);
        if (m_pDS2->eof())
        {
          strSQL = PrepareSQL("select files.strFilename from files join musicvideo on musicvideo.idFile=files.idFile where files.idPath=%i", i.first);
          m_pDS2->query(strSQL);
          bMvidsChecked = true;
        }
        while (!m_pDS2->eof())
        {
          std::string strMoviePath;
          std::string strFileName = m_pDS2->fv("files.strFilename").get_asString();
          ConstructPath(strMoviePath, i.second, strFileName);
          if (HasMovieInfo(strMoviePath))
            DeleteMovie(strMoviePath);
          if (HasMusicVideoInfo(strMoviePath))
            DeleteMusicVideo(strMoviePath);
          m_pDS2->next();
          if (m_pDS2->eof() && !bMvidsChecked)
          {
            strSQL =PrepareSQL("select files.strFilename from files join musicvideo on musicvideo.idFile=files.idFile where files.idPath=%i", i.first);
            m_pDS2->query(strSQL);
            bMvidsChecked = true;
          }
        }
        m_pDS2->close();
        m_pDS2->exec(PrepareSQL("update path set strContent='', strScraper='', strHash='',strSettings='',useFolderNames=0,scanRecursive=0 where idPath=%i", i.first));
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

void CVideoDatabase::SetScraperForPath(const std::string& filePath, const ScraperPtr& scraper, const VIDEO::SScanSettings& settings)
{
  // if we have a multipath, set scraper for all contained paths
  if(URIUtils::IsMultiPath(filePath))
  {
    std::vector<std::string> paths;
    CMultiPathDirectory::GetPaths(filePath, paths);

    for(unsigned i=0;i<paths.size();i++)
      SetScraperForPath(paths[i],scraper,settings);

    return;
  }

  try
  {
    if (nullptr == m_pDB)
      return;
    if (nullptr == m_pDS)
      return;

    int idPath = AddPath(filePath);
    if (idPath < 0)
      return;

    // Update
    std::string strSQL;
    if (settings.exclude)
    { //NB See note in ::GetScraperForPath about strContent=='none'
      strSQL = PrepareSQL(
          "UPDATE path SET strContent='', strScraper='', scanRecursive=0, useFolderNames=0, "
          "strSettings='', noUpdate=0, exclude=1, allAudio=%i WHERE idPath=%i",
          settings.all_ext_audio, idPath);
    }
    else if(!scraper)
    { // catch clearing content, but not excluding
      strSQL = PrepareSQL(
          "UPDATE path SET strContent='', strScraper='', scanRecursive=0, useFolderNames=0, "
          "strSettings='', noUpdate=0, exclude=0, allAudio=%i WHERE idPath=%i",
          settings.all_ext_audio, idPath);
    }
    else
    {
      std::string content = TranslateContent(scraper->Content());
      strSQL = PrepareSQL(
          "UPDATE path SET strContent='%s', strScraper='%s', scanRecursive=%i, useFolderNames=%i, "
          "strSettings='%s', noUpdate=%i, exclude=0, allAudio=%i WHERE idPath=%i",
          content.c_str(), scraper->ID().c_str(), settings.recurse, settings.parent_name,
          scraper->GetPathSettings().c_str(), settings.noupdate, settings.all_ext_audio, idPath);
    }
    m_pDS->exec(strSQL);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, filePath.c_str());
  }
}

bool CVideoDatabase::ScraperInUse(const std::string &scraperID) const
{
  try
  {
    if (nullptr == m_pDB)
      return false;
    if (nullptr == m_pDS)
      return false;

    std::string sql = PrepareSQL("select count(1) from path where strScraper='%s'", scraperID.c_str());
    if (!m_pDS->query(sql) || m_pDS->num_rows() == 0)
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
  std::string art_type;
  std::string art_url;
  int media_id;
  std::string media_type;
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
    if (nullptr == pDS)
      return;

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

  if (iVersion < 119)
    m_pDS->exec("ALTER TABLE path ADD allAudio bool");
}

int CVideoDatabase::GetSchemaVersion() const
{
  return 119;
}

bool CVideoDatabase::LookupByFolders(const std::string &path, bool shows)
{
  SScanSettings settings;
  bool foundDirectly = false;
  ScraperPtr scraper = GetScraperForPath(path, settings, foundDirectly);
  if (scraper && scraper->Content() == CONTENT_TVSHOWS && !shows)
    return false; // episodes
  return settings.parent_name_root; // shows, movies, musicvids
}

bool CVideoDatabase::GetPlayCounts(const std::string &strPath, CFileItemList &items)
{
  if(URIUtils::IsMultiPath(strPath))
  {
    std::vector<std::string> paths;
    CMultiPathDirectory::GetPaths(strPath, paths);

    bool ret = false;
    for(unsigned i=0;i<paths.size();i++)
      ret |= GetPlayCounts(paths[i], items);

    return ret;
  }
  int pathID = -1;
  if (!URIUtils::IsPlugin(strPath))
  {
    pathID = GetPathId(strPath);
    if (pathID < 0)
      return false; // path (and thus files) aren't in the database
  }

  try
  {
    // error!
    if (nullptr == m_pDB)
      return false;
    if (nullptr == m_pDS)
      return false;

    std::string sql =
      "SELECT"
      "  files.strFilename, files.playCount,"
      "  bookmark.timeInSeconds, bookmark.totalTimeInSeconds "
      "FROM files"
      "  LEFT JOIN bookmark ON"
      "    files.idFile = bookmark.idFile AND bookmark.type = %i ";

    if (URIUtils::IsPlugin(strPath))
    {
      for (auto& item : items)
      {
        if (!item || item->m_bIsFolder || !item->GetProperty("IsPlayable").asBoolean())
          continue;

        std::string path, filename;
        SplitPath(item->GetPath(), path, filename);
        m_pDS->query(PrepareSQL(sql +
          "INNER JOIN path ON files.idPath = path.idPath "
          "WHERE files.strFilename='%s' AND path.strPath='%s'",
          (int)CBookmark::RESUME, filename.c_str(), path.c_str()));

        if (!m_pDS->eof())
        {
          if (!item->GetVideoInfoTag()->IsPlayCountSet())
            item->GetVideoInfoTag()->SetPlayCount(m_pDS->fv(1).get_asInt());
          if (!item->GetVideoInfoTag()->GetResumePoint().IsSet())
            item->GetVideoInfoTag()->SetResumePoint(m_pDS->fv(2).get_asInt(), m_pDS->fv(3).get_asInt(), "");
        }
        m_pDS->close();
      }
    }
    else
    {
      //! @todo also test a single query for the above and below
      sql = PrepareSQL(sql + "WHERE files.idPath=%i", (int)CBookmark::RESUME, pathID);

      if (RunQuery(sql) <= 0)
        return false;

      items.SetFastLookup(true); // note: it's possibly quicker the other way around (map on db returned items)?
      while (!m_pDS->eof())
      {
        std::string path;
        ConstructPath(path, strPath, m_pDS->fv(0).get_asString());
        CFileItemPtr item = items.Get(path);
        if (item)
        {
          if (!items.IsPlugin() || !item->GetVideoInfoTag()->IsPlayCountSet())
            item->GetVideoInfoTag()->SetPlayCount(m_pDS->fv(1).get_asInt());

          if (!item->GetVideoInfoTag()->GetResumePoint().IsSet())
          {
            item->GetVideoInfoTag()->SetResumePoint(m_pDS->fv(2).get_asInt(), m_pDS->fv(3).get_asInt(), "");
          }
        }
        m_pDS->next();
      }
    }

    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}

int CVideoDatabase::GetPlayCount(int iFileId)
{
  if (iFileId < 0)
    return 0;  // not in db, so not watched

  try
  {
    // error!
    if (nullptr == m_pDB)
      return -1;
    if (nullptr == m_pDS)
      return -1;

    std::string strSQL = PrepareSQL("select playCount from files WHERE idFile=%i", iFileId);
    int count = 0;
    if (m_pDS->query(strSQL))
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

int CVideoDatabase::GetPlayCount(const std::string& strFilenameAndPath)
{
  return GetPlayCount(GetFileId(strFilenameAndPath));
}

int CVideoDatabase::GetPlayCount(const CFileItem &item)
{
  return GetPlayCount(GetFileId(item));
}

void CVideoDatabase::UpdateFanart(const CFileItem &item, VIDEODB_CONTENT_TYPE type)
{
  if (nullptr == m_pDB)
    return;
  if (nullptr == m_pDS)
    return;
  if (!item.HasVideoInfoTag() || item.GetVideoInfoTag()->m_iDbId < 0) return;

  std::string exec;
  if (type == VIDEODB_CONTENT_TVSHOWS)
    exec = PrepareSQL("UPDATE tvshow set c%02d='%s' WHERE idShow=%i", VIDEODB_ID_TV_FANART, item.GetVideoInfoTag()->m_fanart.m_xml.c_str(), item.GetVideoInfoTag()->m_iDbId);
  else if (type == VIDEODB_CONTENT_MOVIES)
    exec = PrepareSQL("UPDATE movie set c%02d='%s' WHERE idMovie=%i", VIDEODB_ID_FANART, item.GetVideoInfoTag()->m_fanart.m_xml.c_str(), item.GetVideoInfoTag()->m_iDbId);

  try
  {
    m_pDS->exec(exec);

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
    if (nullptr == m_pDB)
      return;
    if (nullptr == m_pDS)
      return;

    std::string strSQL;
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

    m_pDS->exec(strSQL);

    // We only need to announce changes to video items in the library
    if (item.HasVideoInfoTag() && item.GetVideoInfoTag()->m_iDbId > 0)
    {
      CVariant data;
      if (g_application.IsVideoScanning())
        data["transaction"] = true;
      // Only provide the "playcount" value if it has actually changed
      if (item.GetVideoInfoTag()->GetPlayCount() != count)
        data["playcount"] = count;
      CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::VideoLibrary, "xbmc", "OnUpdate", CFileItemPtr(new CFileItem(item)), data);
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

void CVideoDatabase::UpdateMovieTitle(int idMovie, const std::string& strNewMovieTitle, VIDEODB_CONTENT_TYPE iType)
{
  try
  {
    if (nullptr == m_pDB)
      return;
    if (nullptr == m_pDS)
      return;
    std::string content;
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
      std::string strSQL = PrepareSQL("UPDATE sets SET strSet='%s' WHERE idSet=%i", strNewMovieTitle.c_str(), idMovie );
      m_pDS->exec(strSQL);
    }

    if (!content.empty())
    {
      SetSingleValue(iType, idMovie, FieldTitle, strNewMovieTitle);
      AnnounceUpdate(content, idMovie);
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (int idMovie, const std::string& strNewMovieTitle) failed on MovieID:%i and Title:%s", __FUNCTION__, idMovie, strNewMovieTitle.c_str());
  }
}

bool CVideoDatabase::UpdateVideoSortTitle(int idDb, const std::string& strNewSortTitle, VIDEODB_CONTENT_TYPE iType /* = VIDEODB_CONTENT_MOVIES */)
{
  try
  {
    if (nullptr == m_pDB || nullptr == m_pDS)
      return false;
    if (iType != VIDEODB_CONTENT_MOVIES && iType != VIDEODB_CONTENT_TVSHOWS)
      return false;

    std::string content = MediaTypeMovie;
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
    CLog::Log(LOGERROR, "%s (int idDb, const std::string& strNewSortTitle, VIDEODB_CONTENT_TYPE iType) failed on ID: %i and Sort Title: %s", __FUNCTION__, idDb, strNewSortTitle.c_str());
  }

  return false;
}

/// \brief EraseVideoSettings() Erases the videoSettings table and reconstructs it
void CVideoDatabase::EraseVideoSettings(const CFileItem &item)
{
  int idFile = GetFileId(item);
  if (idFile < 0)
    return;

  try
  {
    std::string sql = PrepareSQL("DELETE FROM settings WHERE idFile=%i", idFile);

    CLog::Log(LOGINFO, "Deleting settings information for files %s", CURL::GetRedacted(item.GetPath()).c_str());
    m_pDS->exec(sql);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
}

void CVideoDatabase::EraseAllVideoSettings()
{
  try
  {
    std::string sql = "DELETE FROM settings";

    CLog::Log(LOGINFO, "Deleting all video settings");
    m_pDS->exec(sql);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
}

void CVideoDatabase::EraseAllVideoSettings(std::string path)
{
  std::string itemsToDelete;

  try
  {
    std::string sql = PrepareSQL("SELECT files.idFile FROM files WHERE idFile IN (SELECT idFile FROM files INNER JOIN path ON path.idPath = files.idPath AND path.strPath LIKE \"%s%%\")", path.c_str());
    m_pDS->query(sql);
    while (!m_pDS->eof())
    {
      std::string file = m_pDS->fv("files.idFile").get_asString() + ",";
      itemsToDelete += file;
      m_pDS->next();
    }
    m_pDS->close();

    if (!itemsToDelete.empty())
    {
      itemsToDelete = "(" + StringUtils::TrimRight(itemsToDelete, ",") + ")";

      sql = "DELETE FROM settings WHERE idFile IN " + itemsToDelete;
      m_pDS->exec(sql);
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
}

bool CVideoDatabase::GetGenresNav(const std::string& strBaseDir, CFileItemList& items, int idContent /* = -1 */, const Filter &filter /* = Filter() */, bool countOnly /* = false */)
{
  return GetNavCommon(strBaseDir, items, "genre", idContent, filter, countOnly);
}

bool CVideoDatabase::GetCountriesNav(const std::string& strBaseDir, CFileItemList& items, int idContent /* = -1 */, const Filter &filter /* = Filter() */, bool countOnly /* = false */)
{
  return GetNavCommon(strBaseDir, items, "country", idContent, filter, countOnly);
}

bool CVideoDatabase::GetStudiosNav(const std::string& strBaseDir, CFileItemList& items, int idContent /* = -1 */, const Filter &filter /* = Filter() */, bool countOnly /* = false */)
{
  return GetNavCommon(strBaseDir, items, "studio", idContent, filter, countOnly);
}

bool CVideoDatabase::GetNavCommon(const std::string& strBaseDir, CFileItemList& items, const char *type, int idContent /* = -1 */, const Filter &filter /* = Filter() */, bool countOnly /* = false */)
{
  try
  {
    if (nullptr == m_pDB)
      return false;
    if (nullptr == m_pDS)
      return false;

    std::string strSQL;
    Filter extFilter = filter;
    if (m_profileManager.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
    {
      std::string view, view_id, media_type, extraField, extraJoin;
      if (idContent == VIDEODB_CONTENT_MOVIES)
      {
        view       = MediaTypeMovie;
        view_id    = "idMovie";
        media_type = MediaTypeMovie;
        extraField = "files.playCount";
      }
      else if (idContent == VIDEODB_CONTENT_TVSHOWS) //this will not get tvshows with 0 episodes
      {
        view       = MediaTypeEpisode;
        view_id    = "idShow";
        media_type = MediaTypeTvShow;
        // in order to make use of FieldPlaycount in smart playlists we need an extra join
        if (StringUtils::EqualsNoCase(type, "tag"))
          extraJoin  = PrepareSQL("JOIN tvshow_view ON tvshow_view.idShow = tag_link.media_id AND tag_link.media_type='tvshow'");
      }
      else if (idContent == VIDEODB_CONTENT_MUSICVIDEOS)
      {
        view       = MediaTypeMusicVideo;
        view_id    = "idMVideo";
        media_type = MediaTypeMusicVideo;
        extraField = "files.playCount";
      }
      else
        return false;

      strSQL = "SELECT %s " + PrepareSQL("FROM %s ", type);
      extFilter.fields = PrepareSQL("%s.%s_id, %s.name, path.strPath", type, type, type);
      extFilter.AppendField(extraField);
      extFilter.AppendJoin(PrepareSQL("JOIN %s_link ON %s.%s_id = %s_link.%s_id", type, type, type, type, type));
      extFilter.AppendJoin(PrepareSQL("JOIN %s_view ON %s_link.media_id = %s_view.%s AND %s_link.media_type='%s'", view.c_str(), type, view.c_str(), view_id.c_str(), type, media_type.c_str()));
      extFilter.AppendJoin(PrepareSQL("JOIN files ON files.idFile = %s_view.idFile", view.c_str()));
      extFilter.AppendJoin("JOIN path ON path.idPath = files.idPath");
      extFilter.AppendJoin(extraJoin);
    }
    else
    {
      std::string view, view_id, media_type, extraField, extraJoin;
      if (idContent == VIDEODB_CONTENT_MOVIES)
      {
        view       = MediaTypeMovie;
        view_id    = "idMovie";
        media_type = MediaTypeMovie;
        extraField = "count(1), count(files.playCount)";
        extraJoin  = PrepareSQL("JOIN files ON files.idFile = %s_view.idFile", view.c_str());
      }
      else if (idContent == VIDEODB_CONTENT_TVSHOWS)
      {
        view       = MediaTypeTvShow;
        view_id    = "idShow";
        media_type = MediaTypeTvShow;
      }
      else if (idContent == VIDEODB_CONTENT_MUSICVIDEOS)
      {
        view       = MediaTypeMusicVideo;
        view_id    = "idMVideo";
        media_type = MediaTypeMusicVideo;
        extraField = "count(1), count(files.playCount)";
        extraJoin  = PrepareSQL("JOIN files ON files.idFile = %s_view.idFile", view.c_str());
      }
      else
        return false;

      strSQL = "SELECT %s " + PrepareSQL("FROM %s ", type);
      extFilter.fields = PrepareSQL("%s.%s_id, %s.name", type, type, type);
      extFilter.AppendField(extraField);
      extFilter.AppendJoin(PrepareSQL("JOIN %s_link ON %s.%s_id = %s_link.%s_id", type, type, type, type, type));
      extFilter.AppendJoin(PrepareSQL("JOIN %s_view ON %s_link.media_id = %s_view.%s AND %s_link.media_type='%s'",
                                      view.c_str(), type, view.c_str(), view_id.c_str(), type, media_type.c_str()));
      extFilter.AppendJoin(extraJoin);
      extFilter.AppendGroup(PrepareSQL("%s.%s_id", type, type));
    }

    if (countOnly)
    {
      extFilter.fields = PrepareSQL("COUNT(DISTINCT %s.%s_id)", type, type);
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

    if (m_profileManager.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
    {
      std::map<int, std::pair<std::string,int> > mapItems;
      while (!m_pDS->eof())
      {
        int id = m_pDS->fv(0).get_asInt();
        std::string str = m_pDS->fv(1).get_asString();

        // was this already found?
        auto it = mapItems.find(id);
        if (it == mapItems.end())
        {
          // check path
          if (g_passwordManager.IsDatabasePathUnlocked(m_pDS->fv(2).get_asString(),*CMediaSourceSettings::GetInstance().GetSources("video")))
          {
            if (idContent == VIDEODB_CONTENT_MOVIES || idContent == VIDEODB_CONTENT_MUSICVIDEOS)
              mapItems.insert(std::pair<int, std::pair<std::string,int> >(id, std::pair<std::string, int>(str,m_pDS->fv(3).get_asInt()))); //fv(3) is file.playCount
            else if (idContent == VIDEODB_CONTENT_TVSHOWS)
              mapItems.insert(std::pair<int, std::pair<std::string,int> >(id, std::pair<std::string,int>(str,0)));
          }
        }
        m_pDS->next();
      }
      m_pDS->close();

      for (const auto &i : mapItems)
      {
        CFileItemPtr pItem(new CFileItem(i.second.first));
        pItem->GetVideoInfoTag()->m_iDbId = i.first;
        pItem->GetVideoInfoTag()->m_type = type;

        CVideoDbUrl itemUrl = videoUrl;
        std::string path = StringUtils::Format("%i/", i.first);
        itemUrl.AppendPath(path);
        pItem->SetPath(itemUrl.ToString());

        pItem->m_bIsFolder = true;
        if (idContent == VIDEODB_CONTENT_MOVIES || idContent == VIDEODB_CONTENT_MUSICVIDEOS)
          pItem->GetVideoInfoTag()->SetPlayCount(i.second.second);
        if (!items.Contains(pItem->GetPath()))
        {
          pItem->SetLabelPreformatted(true);
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
        std::string path = StringUtils::Format("%i/", m_pDS->fv(0).get_asInt());
        itemUrl.AppendPath(path);
        pItem->SetPath(itemUrl.ToString());

        pItem->m_bIsFolder = true;
        pItem->SetLabelPreformatted(true);
        if (idContent == VIDEODB_CONTENT_MOVIES || idContent == VIDEODB_CONTENT_MUSICVIDEOS)
        { // fv(3) is the number of videos watched, fv(2) is the total number.  We set the playcount
          // only if the number of videos watched is equal to the total number (i.e. every video watched)
          pItem->GetVideoInfoTag()->SetPlayCount((m_pDS->fv(3).get_asInt() == m_pDS->fv(2).get_asInt()) ? 1 : 0);
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

bool CVideoDatabase::GetTagsNav(const std::string& strBaseDir, CFileItemList& items, int idContent /* = -1 */, const Filter &filter /* = Filter() */, bool countOnly /* = false */)
{
  return GetNavCommon(strBaseDir, items, "tag", idContent, filter, countOnly);
}

bool CVideoDatabase::GetSetsNav(const std::string& strBaseDir, CFileItemList& items, int idContent /* = -1 */, const Filter &filter /* = Filter() */, bool ignoreSingleMovieSets /* = false */)
{
  if (idContent != VIDEODB_CONTENT_MOVIES)
    return false;

  return GetSetsByWhere(strBaseDir, filter, items, ignoreSingleMovieSets);
}

bool CVideoDatabase::GetSetsByWhere(const std::string& strBaseDir, const Filter &filter, CFileItemList& items, bool ignoreSingleMovieSets /* = false */)
{
  try
  {
    if (nullptr == m_pDB)
      return false;
    if (nullptr == m_pDS)
      return false;

    CVideoDbUrl videoUrl;
    if (!videoUrl.FromString(strBaseDir))
      return false;

    Filter setFilter = filter;
    setFilter.join += " JOIN sets ON movie_view.idSet = sets.idSet";
    if (!setFilter.order.empty())
      setFilter.order += ",";
    setFilter.order += "sets.idSet";

    if (!GetMoviesByWhere(strBaseDir, setFilter, items))
      return false;

    CFileItemList sets;
    GroupAttribute groupingAttributes;
    const CUrlOptions::UrlOptions& options = videoUrl.GetOptions();
    auto option = options.find("ignoreSingleMovieSets");

    if (option != options.end())
    {
      groupingAttributes =
          option->second.asBoolean() ? GroupAttributeIgnoreSingleItems : GroupAttributeNone;
    }
    else
    {
      groupingAttributes =
          ignoreSingleMovieSets ? GroupAttributeIgnoreSingleItems : GroupAttributeNone;
    }

    if (!GroupUtils::Group(GroupBySet, strBaseDir, items, sets, groupingAttributes))
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

bool CVideoDatabase::GetMusicVideoAlbumsNav(const std::string& strBaseDir, CFileItemList& items, int idArtist /* = -1 */, const Filter &filter /* = Filter() */, bool countOnly /* = false */)
{
  try
  {
    if (nullptr == m_pDB)
      return false;
    if (nullptr == m_pDS)
      return false;

    CVideoDbUrl videoUrl;
    if (!videoUrl.FromString(strBaseDir))
      return false;

    std::string strSQL = "select %s from musicvideo_view ";
    Filter extFilter = filter;
    extFilter.fields = PrepareSQL("musicvideo_view.c%02d, musicvideo_view.idMVideo, actor.name", VIDEODB_ID_MUSICVIDEO_ALBUM);
    extFilter.AppendJoin(PrepareSQL("JOIN actor_link ON actor_link.media_id=musicvideo_view.idMVideo AND actor_link.media_type='musicvideo'"));
    extFilter.AppendJoin(PrepareSQL("JOIN actor ON actor.actor_id = actor_link.actor_id"));
    if (m_profileManager.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
    {
      extFilter.fields += ", path.strPath";
      extFilter.AppendJoin("join files on files.idFile = musicvideo_view.idFile join path on path.idPath = files.idPath");
    }

    if (idArtist > -1)
      videoUrl.AddOption("artistid", idArtist);

    extFilter.AppendGroup(PrepareSQL("musicvideo_view.c%02d", VIDEODB_ID_MUSICVIDEO_ALBUM));

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

    if (m_profileManager.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
    {
      std::map<int, std::pair<std::string,std::string> > mapAlbums;
      while (!m_pDS->eof())
      {
        int lidMVideo = m_pDS->fv(1).get_asInt();
        std::string strAlbum = m_pDS->fv(0).get_asString();
        auto it = mapAlbums.find(lidMVideo);
        // was this genre already found?
        if (it == mapAlbums.end())
        {
          // check path
          if (g_passwordManager.IsDatabasePathUnlocked(m_pDS->fv("path.strPath").get_asString(),*CMediaSourceSettings::GetInstance().GetSources("video")))
            mapAlbums.insert(make_pair(lidMVideo, make_pair(strAlbum,m_pDS->fv(2).get_asString())));
        }
        m_pDS->next();
      }
      m_pDS->close();

      for (const auto &i : mapAlbums)
      {
        if (!i.second.first.empty())
        {
          CFileItemPtr pItem(new CFileItem(i.second.first));

          CVideoDbUrl itemUrl = videoUrl;
          std::string path = StringUtils::Format("%i/", i.first);
          itemUrl.AppendPath(path);
          pItem->SetPath(itemUrl.ToString());

          pItem->m_bIsFolder=true;
          pItem->SetLabelPreformatted(true);
          if (!items.Contains(pItem->GetPath()))
          {
            pItem->GetVideoInfoTag()->m_artist.push_back(i.second.second);
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
          std::string path = StringUtils::Format("%i/", m_pDS->fv(1).get_asInt());
          itemUrl.AppendPath(path);
          pItem->SetPath(itemUrl.ToString());

          pItem->m_bIsFolder=true;
          pItem->SetLabelPreformatted(true);
          if (!items.Contains(pItem->GetPath()))
          {
            pItem->GetVideoInfoTag()->m_artist.emplace_back(m_pDS->fv(2).get_asString());
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

bool CVideoDatabase::GetWritersNav(const std::string& strBaseDir, CFileItemList& items, int idContent /* = -1 */, const Filter &filter /* = Filter() */, bool countOnly /* = false */)
{
  return GetPeopleNav(strBaseDir, items, "writer", idContent, filter, countOnly);
}

bool CVideoDatabase::GetDirectorsNav(const std::string& strBaseDir, CFileItemList& items, int idContent /* = -1 */, const Filter &filter /* = Filter() */, bool countOnly /* = false */)
{
  return GetPeopleNav(strBaseDir, items, "director", idContent, filter, countOnly);
}

bool CVideoDatabase::GetActorsNav(const std::string& strBaseDir, CFileItemList& items, int idContent /* = -1 */, const Filter &filter /* = Filter() */, bool countOnly /* = false */)
{
  if (GetPeopleNav(strBaseDir, items, "actor", idContent, filter, countOnly))
  { // set thumbs - ideally this should be in the normal thumb setting routines
    for (int i = 0; i < items.Size() && !countOnly; i++)
    {
      CFileItemPtr pItem = items[i];
      if (idContent == VIDEODB_CONTENT_MUSICVIDEOS)
        pItem->SetArt("icon", "DefaultArtist.png");
      else
        pItem->SetArt("icon", "DefaultActor.png");
    }
    return true;
  }
  return false;
}

bool CVideoDatabase::GetPeopleNav(const std::string& strBaseDir, CFileItemList& items, const char *type, int idContent /* = -1 */, const Filter &filter /* = Filter() */, bool countOnly /* = false */)
{
  if (nullptr == m_pDB)
    return false;
  if (nullptr == m_pDS)
    return false;

  try
  {
    //! @todo This routine (and probably others at this same level) use playcount as a reference to filter on at a later
    //!       point.  This means that we *MUST* filter these levels as you'll get double ups.  Ideally we'd allow playcount
    //!       to filter through as we normally do for tvshows to save this happening.
    //!       Also, we apply this same filtering logic to the locked or unlocked paths to prevent these from showing.
    //!       Whether or not this should happen is a tricky one - it complicates all the high level categories (everything
    //!       above titles).

    // General routine that the other actor/director/writer routines call

    // get primary genres for movies
    std::string strSQL;
    Filter extFilter = filter;
    if (m_profileManager.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
    {
      std::string view, view_id, media_type, extraField, group;
      if (idContent == VIDEODB_CONTENT_MOVIES)
      {
        view       = MediaTypeMovie;
        view_id    = "idMovie";
        media_type = MediaTypeMovie;
        extraField = "files.playCount";
      }
      else if (idContent == VIDEODB_CONTENT_TVSHOWS)
      {
        view       = MediaTypeEpisode;
        view_id    = "idShow";
        media_type = MediaTypeTvShow;
        extraField = "count(DISTINCT idShow)";
        group = "actor.actor_id";
      }
      else if (idContent == VIDEODB_CONTENT_EPISODES)
      {
        view       = MediaTypeEpisode;
        view_id    = "idEpisode";
        media_type = MediaTypeEpisode;
        extraField = "files.playCount";
      }
      else if (idContent == VIDEODB_CONTENT_MUSICVIDEOS)
      {
        view       = MediaTypeMusicVideo;
        view_id    = "idMVideo";
        media_type = MediaTypeMusicVideo;
        extraField = "files.playCount";
      }
      else
        return false;

      strSQL = "SELECT %s FROM actor ";
      extFilter.fields = "actor.actor_id, actor.name, actor.art_urls, path.strPath";
      extFilter.AppendField(extraField);
      extFilter.AppendJoin(PrepareSQL("JOIN %s_link ON actor.actor_id = %s_link.actor_id", type, type));
      extFilter.AppendJoin(PrepareSQL("JOIN %s_view ON %s_link.media_id = %s_view.%s AND %s_link.media_type='%s'", view.c_str(), type, view.c_str(), view_id.c_str(), type, media_type.c_str()));
      extFilter.AppendJoin(PrepareSQL("JOIN files ON files.idFile = %s_view.idFile", view.c_str()));
      extFilter.AppendJoin("JOIN path ON path.idPath = files.idPath");
      extFilter.AppendGroup(group);
    }
    else
    {
      std::string view, view_id, media_type, extraField, extraJoin;
      if (idContent == VIDEODB_CONTENT_MOVIES)
      {
        view       = MediaTypeMovie;
        view_id    = "idMovie";
        media_type = MediaTypeMovie;
        extraField = "count(1), count(files.playCount)";
        extraJoin  = PrepareSQL(" JOIN files ON files.idFile=%s_view.idFile", view.c_str());
      }
      else if (idContent == VIDEODB_CONTENT_TVSHOWS)
      {
        view       = MediaTypeTvShow;
        view_id    = "idShow";
        media_type = MediaTypeTvShow;
        extraField = "count(idShow)";
      }
      else if (idContent == VIDEODB_CONTENT_EPISODES)
      {
        view       = MediaTypeEpisode;
        view_id    = "idEpisode";
        media_type = MediaTypeEpisode;
        extraField = "count(1), count(files.playCount)";
        extraJoin  = PrepareSQL("JOIN files ON files.idFile = %s_view.idFile", view.c_str());
      }
      else if (idContent == VIDEODB_CONTENT_MUSICVIDEOS)
      {
        view       = MediaTypeMusicVideo;
        view_id    = "idMVideo";
        media_type = MediaTypeMusicVideo;
        extraField = "count(1), count(files.playCount)";
        extraJoin  = PrepareSQL("JOIN files ON files.idFile = %s_view.idFile", view.c_str());
      }
      else
        return false;

      strSQL ="SELECT %s FROM actor ";
      extFilter.fields = "actor.actor_id, actor.name, actor.art_urls";
      extFilter.AppendField(extraField);
      extFilter.AppendJoin(PrepareSQL("JOIN %s_link on actor.actor_id = %s_link.actor_id", type, type));
      extFilter.AppendJoin(PrepareSQL("JOIN %s_view on %s_link.media_id = %s_view.%s AND %s_link.media_type='%s'", view.c_str(), type, view.c_str(), view_id.c_str(), type, media_type.c_str()));
      extFilter.AppendJoin(extraJoin);
      extFilter.AppendGroup("actor.actor_id");
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
    if (!m_pDS->query(strSQL)) return false;
    CLog::Log(LOGDEBUG, LOGDATABASE, "%s -  query took %i ms",
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

    if (m_profileManager.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
    {
      std::map<int, CActor> mapActors;

      while (!m_pDS->eof())
      {
        int idActor = m_pDS->fv(0).get_asInt();
        CActor actor;
        actor.name = m_pDS->fv(1).get_asString();
        actor.thumb = m_pDS->fv(2).get_asString();
        if (idContent != VIDEODB_CONTENT_TVSHOWS)
        {
          actor.playcount = m_pDS->fv(3).get_asInt();
          actor.appearances = 1;
        }
        else actor.appearances = m_pDS->fv(4).get_asInt();
        auto it = mapActors.find(idActor);
        // is this actor already known?
        if (it == mapActors.end())
        {
          // check path
          if (g_passwordManager.IsDatabasePathUnlocked(m_pDS->fv("path.strPath").get_asString(),*CMediaSourceSettings::GetInstance().GetSources("video")))
            mapActors.insert(std::pair<int, CActor>(idActor, actor));
        }
        else if (idContent != VIDEODB_CONTENT_TVSHOWS)
            it->second.appearances++;
        m_pDS->next();
      }
      m_pDS->close();

      for (const auto &i : mapActors)
      {
        CFileItemPtr pItem(new CFileItem(i.second.name));

        CVideoDbUrl itemUrl = videoUrl;
        std::string path = StringUtils::Format("%i/", i.first);
        itemUrl.AppendPath(path);
        pItem->SetPath(itemUrl.ToString());

        pItem->m_bIsFolder=true;
        pItem->GetVideoInfoTag()->SetPlayCount(i.second.playcount);
        pItem->GetVideoInfoTag()->m_strPictureURL.ParseFromData(i.second.thumb);
        pItem->GetVideoInfoTag()->m_iDbId = i.first;
        pItem->GetVideoInfoTag()->m_type = type;
        pItem->GetVideoInfoTag()->m_relevance = i.second.appearances;
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
          std::string path = StringUtils::Format("%i/", m_pDS->fv(0).get_asInt());
          itemUrl.AppendPath(path);
          pItem->SetPath(itemUrl.ToString());

          pItem->m_bIsFolder=true;
          pItem->GetVideoInfoTag()->m_strPictureURL.ParseFromData(m_pDS->fv(2).get_asString());
          pItem->GetVideoInfoTag()->m_iDbId = m_pDS->fv(0).get_asInt();
          pItem->GetVideoInfoTag()->m_type = type;
          if (idContent != VIDEODB_CONTENT_TVSHOWS)
          {
            // fv(4) is the number of videos watched, fv(3) is the total number.  We set the playcount
            // only if the number of videos watched is equal to the total number (i.e. every video watched)
            pItem->GetVideoInfoTag()->SetPlayCount((m_pDS->fv(4).get_asInt() == m_pDS->fv(3).get_asInt()) ? 1 : 0);
          }
          pItem->GetVideoInfoTag()->m_relevance = m_pDS->fv(3).get_asInt();
          if (idContent == VIDEODB_CONTENT_MUSICVIDEOS)
            pItem->GetVideoInfoTag()->m_artist.emplace_back(pItem->GetLabel());
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
    CLog::Log(LOGDEBUG, LOGDATABASE, "%s item retrieval took %i ms",
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

bool CVideoDatabase::GetYearsNav(const std::string& strBaseDir, CFileItemList& items, int idContent /* = -1 */, const Filter &filter /* = Filter() */)
{
  try
  {
    if (nullptr == m_pDB)
      return false;
    if (nullptr == m_pDS)
      return false;

    std::string strSQL;
    Filter extFilter = filter;
    if (m_profileManager.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
    {
      if (idContent == VIDEODB_CONTENT_MOVIES)
      {
        strSQL = "select movie_view.premiered, path.strPath, files.playCount from movie_view ";
        extFilter.AppendJoin("join files on files.idFile = movie_view.idFile join path on files.idPath = path.idPath");
      }
      else if (idContent == VIDEODB_CONTENT_TVSHOWS)
      {
        strSQL = PrepareSQL("select tvshow_view.c%02d, path.strPath from tvshow_view ", VIDEODB_ID_TV_PREMIERED);
        extFilter.AppendJoin("join episode_view on episode_view.idShow = tvshow_view.idShow join files on files.idFile = episode_view.idFile join path on files.idPath = path.idPath");
      }
      else if (idContent == VIDEODB_CONTENT_MUSICVIDEOS)
      {
        strSQL = "select musicvideo_view.premiered, path.strPath, files.playCount from musicvideo_view ";
        extFilter.AppendJoin("join files on files.idFile = musicvideo_view.idFile join path on files.idPath = path.idPath");
      }
      else
        return false;
    }
    else
    {
      std::string group;
      if (idContent == VIDEODB_CONTENT_MOVIES)
      {
        strSQL = "select movie_view.premiered, count(1), count(files.playCount) from movie_view ";
        extFilter.AppendJoin("join files on files.idFile = movie_view.idFile");
        extFilter.AppendGroup("movie_view.premiered");
      }
      else if (idContent == VIDEODB_CONTENT_TVSHOWS)
      {
        strSQL = PrepareSQL("select distinct tvshow_view.c%02d from tvshow_view", VIDEODB_ID_TV_PREMIERED);
        extFilter.AppendGroup(PrepareSQL("tvshow_view.c%02d", VIDEODB_ID_TV_PREMIERED));
      }
      else if (idContent == VIDEODB_CONTENT_MUSICVIDEOS)
      {
        strSQL = "select musicvideo_view.premiered, count(1), count(files.playCount) from musicvideo_view ";
        extFilter.AppendJoin("join files on files.idFile = musicvideo_view.idFile");
        extFilter.AppendGroup("musicvideo_view.premiered");
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

    if (m_profileManager.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
    {
      std::map<int, std::pair<std::string,int> > mapYears;
      while (!m_pDS->eof())
      {
        int lYear = 0;
        std::string dateString = m_pDS->fv(0).get_asString();
        if (dateString.size() == 4)
          lYear = m_pDS->fv(0).get_asInt();
        else
        {
          CDateTime time;
          time.SetFromDateString(dateString);
          if (time.IsValid())
            lYear = time.GetYear();
        }
        auto it = mapYears.find(lYear);
        if (it == mapYears.end())
        {
          // check path
          if (g_passwordManager.IsDatabasePathUnlocked(m_pDS->fv("path.strPath").get_asString(),*CMediaSourceSettings::GetInstance().GetSources("video")))
          {
            std::string year = StringUtils::Format("%d", lYear);
            if (idContent == VIDEODB_CONTENT_MOVIES || idContent == VIDEODB_CONTENT_MUSICVIDEOS)
              mapYears.insert(std::pair<int, std::pair<std::string,int> >(lYear, std::pair<std::string,int>(year,m_pDS->fv(2).get_asInt())));
            else
              mapYears.insert(std::pair<int, std::pair<std::string,int> >(lYear, std::pair<std::string,int>(year,0)));
          }
        }
        m_pDS->next();
      }
      m_pDS->close();

      for (const auto &i : mapYears)
      {
        if (i.first == 0)
          continue;
        CFileItemPtr pItem(new CFileItem(i.second.first));

        CVideoDbUrl itemUrl = videoUrl;
        std::string path = StringUtils::Format("%i/", i.first);
        itemUrl.AppendPath(path);
        pItem->SetPath(itemUrl.ToString());

        pItem->m_bIsFolder=true;
        if (idContent == VIDEODB_CONTENT_MOVIES || idContent == VIDEODB_CONTENT_MUSICVIDEOS)
          pItem->GetVideoInfoTag()->SetPlayCount(i.second.second);
        items.Add(pItem);
      }
    }
    else
    {
      while (!m_pDS->eof())
      {
        int lYear = 0;
        std::string strLabel = m_pDS->fv(0).get_asString();
        if (strLabel.size() == 4)
          lYear = m_pDS->fv(0).get_asInt();
        else
        {
          CDateTime time;
          time.SetFromDateString(strLabel);
          if (time.IsValid())
          {
            lYear = time.GetYear();
            strLabel = StringUtils::Format("%i", lYear);
          }
        }
        if (lYear == 0)
        {
          m_pDS->next();
          continue;
        }
        CFileItemPtr pItem(new CFileItem(strLabel));

        CVideoDbUrl itemUrl = videoUrl;
        std::string path = StringUtils::Format("%i/", lYear);
        itemUrl.AppendPath(path);
        pItem->SetPath(itemUrl.ToString());

        pItem->m_bIsFolder=true;
        if (idContent == VIDEODB_CONTENT_MOVIES || idContent == VIDEODB_CONTENT_MUSICVIDEOS)
        {
          // fv(2) is the number of videos watched, fv(1) is the total number.  We set the playcount
          // only if the number of videos watched is equal to the total number (i.e. every video watched)
          pItem->GetVideoInfoTag()->SetPlayCount((m_pDS->fv(2).get_asInt() == m_pDS->fv(1).get_asInt()) ? 1 : 0);
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

bool CVideoDatabase::GetSeasonsNav(const std::string& strBaseDir, CFileItemList& items, int idActor, int idDirector, int idGenre, int idYear, int idShow, bool getLinkedMovies /* = true */)
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
    movieFilter.join = PrepareSQL("join movielinktvshow on movielinktvshow.idMovie=movie_view.idMovie");
    movieFilter.where = PrepareSQL("movielinktvshow.idShow = %i", idShow);
    CFileItemList movieItems;
    GetMoviesByWhere("videodb://movies/titles/", movieFilter, movieItems);

    if (movieItems.Size() > 0)
      items.Append(movieItems);
  }

  return true;
}

bool CVideoDatabase::GetSeasonsByWhere(const std::string& strBaseDir, const Filter &filter, CFileItemList& items, bool appendFullShowPath /* = true */, const SortDescription &sortDescription /* = SortDescription() */)
{
  try
  {
    if (nullptr == m_pDB)
      return false;
    if (nullptr == m_pDS)
      return false;

    int total = -1;

    std::string strSQL = "SELECT %s FROM season_view ";
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

    std::set<std::pair<int, int>> mapSeasons;
    while (!m_pDS->eof())
    {
      int id = m_pDS->fv(VIDEODB_ID_SEASON_ID).get_asInt();
      int showId = m_pDS->fv(VIDEODB_ID_SEASON_TVSHOW_ID).get_asInt();
      int iSeason = m_pDS->fv(VIDEODB_ID_SEASON_NUMBER).get_asInt();
      std::string name = m_pDS->fv(VIDEODB_ID_SEASON_NAME).get_asString();
      std::string path = m_pDS->fv(VIDEODB_ID_SEASON_TVSHOW_PATH).get_asString();

      if (mapSeasons.find(std::make_pair(showId, iSeason)) == mapSeasons.end() &&
         (m_profileManager.GetMasterProfile().getLockMode() == LOCK_MODE_EVERYONE || g_passwordManager.bMasterUser ||
          g_passwordManager.IsDatabasePathUnlocked(path, *CMediaSourceSettings::GetInstance().GetSources("video"))))
      {
        mapSeasons.insert(std::make_pair(showId, iSeason));

        std::string strLabel = name;
        if (strLabel.empty())
        {
          if (iSeason == 0)
            strLabel = g_localizeStrings.Get(20381);
          else
            strLabel = StringUtils::Format(g_localizeStrings.Get(20358).c_str(), iSeason);
        }
        CFileItemPtr pItem(new CFileItem(strLabel));

        CVideoDbUrl itemUrl = videoUrl;
        std::string strDir;
        if (appendFullShowPath)
          strDir += StringUtils::Format("%d/", showId);
        strDir += StringUtils::Format("%d/", iSeason);
        itemUrl.AppendPath(strDir);
        pItem->SetPath(itemUrl.ToString());

        pItem->m_bIsFolder = true;
        pItem->GetVideoInfoTag()->m_strTitle = strLabel;
        if (!name.empty())
          pItem->GetVideoInfoTag()->m_strSortTitle = name;
        pItem->GetVideoInfoTag()->m_iSeason = iSeason;
        pItem->GetVideoInfoTag()->m_iDbId = id;
        pItem->GetVideoInfoTag()->m_iIdSeason = id;
        pItem->GetVideoInfoTag()->m_type = MediaTypeSeason;
        pItem->GetVideoInfoTag()->m_strPath = path;
        pItem->GetVideoInfoTag()->m_strShowTitle = m_pDS->fv(VIDEODB_ID_SEASON_TVSHOW_TITLE).get_asString();
        pItem->GetVideoInfoTag()->m_strPlot = m_pDS->fv(VIDEODB_ID_SEASON_TVSHOW_PLOT).get_asString();
        pItem->GetVideoInfoTag()->SetPremieredFromDBDate(m_pDS->fv(VIDEODB_ID_SEASON_TVSHOW_PREMIERED).get_asString());
        pItem->GetVideoInfoTag()->m_firstAired.SetFromDBDate(m_pDS->fv(VIDEODB_ID_SEASON_PREMIERED).get_asString());
        pItem->GetVideoInfoTag()->m_iUserRating = m_pDS->fv(VIDEODB_ID_SEASON_USER_RATING).get_asInt();
        // season premiered date based on first episode airdate associated to the season
        // tvshow premiered date is used as a fallback
        if (pItem->GetVideoInfoTag()->m_firstAired.IsValid())
          pItem->GetVideoInfoTag()->SetPremiered(pItem->GetVideoInfoTag()->m_firstAired);
        else if (pItem->GetVideoInfoTag()->HasPremiered())
          pItem->GetVideoInfoTag()->SetPremiered(pItem->GetVideoInfoTag()->GetPremiered());
        else if (pItem->GetVideoInfoTag()->HasYear())
          pItem->GetVideoInfoTag()->SetYear(pItem->GetVideoInfoTag()->GetYear());
        pItem->GetVideoInfoTag()->m_genre = StringUtils::Split(m_pDS->fv(VIDEODB_ID_SEASON_TVSHOW_GENRE).get_asString(), CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoItemSeparator);
        pItem->GetVideoInfoTag()->m_studio = StringUtils::Split(m_pDS->fv(VIDEODB_ID_SEASON_TVSHOW_STUDIO).get_asString(), CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoItemSeparator);
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
        pItem->GetVideoInfoTag()->SetPlayCount((totalEpisodes == watchedEpisodes) ? 1 : 0);
        pItem->SetOverlayImage(CGUIListItem::ICON_OVERLAY_UNWATCHED, (pItem->GetVideoInfoTag()->GetPlayCount() > 0) && (pItem->GetVideoInfoTag()->m_iEpisode > 0));

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

bool CVideoDatabase::GetSortedVideos(const MediaType &mediaType, const std::string& strBaseDir, const SortDescription &sortDescription, CFileItemList& items, const Filter &filter /* = Filter() */)
{
  if (nullptr == m_pDB || nullptr == m_pDS)
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
      sortDescription.sortBy == SortByUserRating ||
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

  items.SetContent(CMediaTypes::ToPlural(mediaType));
  return success;
}

bool CVideoDatabase::GetItems(const std::string &strBaseDir, CFileItemList &items, const Filter &filter /* = Filter() */, const SortDescription &sortDescription /* = SortDescription() */)
{
  CVideoDbUrl videoUrl;
  if (!videoUrl.FromString(strBaseDir))
    return false;

  return GetItems(strBaseDir, videoUrl.GetType(), videoUrl.GetItemType(), items, filter, sortDescription);
}

bool CVideoDatabase::GetItems(const std::string &strBaseDir, const std::string &mediaType, const std::string &itemType, CFileItemList &items, const Filter &filter /* = Filter() */, const SortDescription &sortDescription /* = SortDescription() */)
{
  VIDEODB_CONTENT_TYPE contentType;
  if (StringUtils::EqualsNoCase(mediaType, "movies"))
    contentType = VIDEODB_CONTENT_MOVIES;
  else if (StringUtils::EqualsNoCase(mediaType, "tvshows"))
  {
    if (StringUtils::EqualsNoCase(itemType, "episodes"))
      contentType = VIDEODB_CONTENT_EPISODES;
    else
      contentType = VIDEODB_CONTENT_TVSHOWS;
  }
  else if (StringUtils::EqualsNoCase(mediaType, "musicvideos"))
    contentType = VIDEODB_CONTENT_MUSICVIDEOS;
  else
    return false;

  return GetItems(strBaseDir, contentType, itemType, items, filter, sortDescription);
}

bool CVideoDatabase::GetItems(const std::string &strBaseDir, VIDEODB_CONTENT_TYPE mediaType, const std::string &itemType, CFileItemList &items, const Filter &filter /* = Filter() */, const SortDescription &sortDescription /* = SortDescription() */)
{
  if (StringUtils::EqualsNoCase(itemType, "movies") && (mediaType == VIDEODB_CONTENT_MOVIES || mediaType == VIDEODB_CONTENT_MOVIE_SETS))
    return GetMoviesByWhere(strBaseDir, filter, items, sortDescription);
  else if (StringUtils::EqualsNoCase(itemType, "tvshows") && mediaType == VIDEODB_CONTENT_TVSHOWS)
  {
    Filter extFilter = filter;
    if (!CServiceBroker::GetSettingsComponent()->GetSettings()->
        GetBool(CSettings::SETTING_VIDEOLIBRARY_SHOWEMPTYTVSHOWS))
      extFilter.AppendWhere("totalCount IS NOT NULL AND totalCount > 0");
    return GetTvShowsByWhere(strBaseDir, extFilter, items, sortDescription);
  }
  else if (StringUtils::EqualsNoCase(itemType, "musicvideos") && mediaType == VIDEODB_CONTENT_MUSICVIDEOS)
    return GetMusicVideosByWhere(strBaseDir, filter, items, true, sortDescription);
  else if (StringUtils::EqualsNoCase(itemType, "episodes") && mediaType == VIDEODB_CONTENT_EPISODES)
    return GetEpisodesByWhere(strBaseDir, filter, items, true, sortDescription);
  else if (StringUtils::EqualsNoCase(itemType, "seasons") && mediaType == VIDEODB_CONTENT_TVSHOWS)
    return GetSeasonsNav(strBaseDir, items);
  else if (StringUtils::EqualsNoCase(itemType, "genres"))
    return GetGenresNav(strBaseDir, items, mediaType, filter);
  else if (StringUtils::EqualsNoCase(itemType, "years"))
    return GetYearsNav(strBaseDir, items, mediaType, filter);
  else if (StringUtils::EqualsNoCase(itemType, "actors"))
    return GetActorsNav(strBaseDir, items, mediaType, filter);
  else if (StringUtils::EqualsNoCase(itemType, "directors"))
    return GetDirectorsNav(strBaseDir, items, mediaType, filter);
  else if (StringUtils::EqualsNoCase(itemType, "writers"))
    return GetWritersNav(strBaseDir, items, mediaType, filter);
  else if (StringUtils::EqualsNoCase(itemType, "studios"))
    return GetStudiosNav(strBaseDir, items, mediaType, filter);
  else if (StringUtils::EqualsNoCase(itemType, "sets"))
    return GetSetsNav(strBaseDir, items, mediaType, filter, !CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_VIDEOLIBRARY_GROUPSINGLEITEMSETS));
  else if (StringUtils::EqualsNoCase(itemType, "countries"))
    return GetCountriesNav(strBaseDir, items, mediaType, filter);
  else if (StringUtils::EqualsNoCase(itemType, "tags"))
    return GetTagsNav(strBaseDir, items, mediaType, filter);
  else if (StringUtils::EqualsNoCase(itemType, "artists") && mediaType == VIDEODB_CONTENT_MUSICVIDEOS)
    return GetActorsNav(strBaseDir, items, mediaType, filter);
  else if (StringUtils::EqualsNoCase(itemType, "albums") && mediaType == VIDEODB_CONTENT_MUSICVIDEOS)
    return GetMusicVideoAlbumsNav(strBaseDir, items, -1, filter);

  return false;
}

std::string CVideoDatabase::GetItemById(const std::string &itemType, int id)
{
  if (StringUtils::EqualsNoCase(itemType, "genres"))
    return GetGenreById(id);
  else if (StringUtils::EqualsNoCase(itemType, "years"))
    return StringUtils::Format("%d", id);
  else if (StringUtils::EqualsNoCase(itemType, "actors") ||
           StringUtils::EqualsNoCase(itemType, "directors") ||
           StringUtils::EqualsNoCase(itemType, "artists"))
    return GetPersonById(id);
  else if (StringUtils::EqualsNoCase(itemType, "studios"))
    return GetStudioById(id);
  else if (StringUtils::EqualsNoCase(itemType, "sets"))
    return GetSetById(id);
  else if (StringUtils::EqualsNoCase(itemType, "countries"))
    return GetCountryById(id);
  else if (StringUtils::EqualsNoCase(itemType, "tags"))
    return GetTagById(id);
  else if (StringUtils::EqualsNoCase(itemType, "albums"))
    return GetMusicVideoAlbumById(id);

  return "";
}

bool CVideoDatabase::GetMoviesNav(const std::string& strBaseDir, CFileItemList& items,
                                  int idGenre /* = -1 */, int idYear /* = -1 */, int idActor /* = -1 */, int idDirector /* = -1 */,
                                  int idStudio /* = -1 */, int idCountry /* = -1 */, int idSet /* = -1 */, int idTag /* = -1 */,
                                  const SortDescription &sortDescription /* = SortDescription() */, int getDetails /* = VideoDbDetailsNone */)
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
  return GetMoviesByWhere(videoUrl.ToString(), filter, items, sortDescription, getDetails);
}

bool CVideoDatabase::GetMoviesByWhere(const std::string& strBaseDir, const Filter &filter, CFileItemList& items, const SortDescription &sortDescription /* = SortDescription() */, int getDetails /* = VideoDbDetailsNone */)
{
  try
  {
    movieTime = 0;
    castTime = 0;

    if (nullptr == m_pDB)
      return false;
    if (nullptr == m_pDS)
      return false;

    // parse the base path to get additional filters
    CVideoDbUrl videoUrl;
    Filter extFilter = filter;
    SortDescription sorting = sortDescription;
    if (!videoUrl.FromString(strBaseDir) || !GetFilter(videoUrl, extFilter, sorting))
      return false;

    int total = -1;

    std::string strSQL = "select %s from movie_view ";
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
    for (const auto &i : results)
    {
      unsigned int targetRow = (unsigned int)i.at(FieldRow).asInteger();
      const dbiplus::sql_record* const record = data.at(targetRow);

      CVideoInfoTag movie = GetDetailsForMovie(record, getDetails);
      if (m_profileManager.GetMasterProfile().getLockMode() == LOCK_MODE_EVERYONE ||
          g_passwordManager.bMasterUser                                   ||
          g_passwordManager.IsDatabasePathUnlocked(movie.m_strPath, *CMediaSourceSettings::GetInstance().GetSources("video")))
      {
        CFileItemPtr pItem(new CFileItem(movie));

        CVideoDbUrl itemUrl = videoUrl;
        std::string path = StringUtils::Format("%i", movie.m_iDbId);
        itemUrl.AppendPath(path);
        pItem->SetPath(itemUrl.ToString());
        pItem->SetDynPath(movie.m_strFileNameAndPath);

        pItem->SetOverlayImage(CGUIListItem::ICON_OVERLAY_UNWATCHED,movie.GetPlayCount() > 0);
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

bool CVideoDatabase::GetTvShowsNav(const std::string& strBaseDir, CFileItemList& items,
                                  int idGenre /* = -1 */, int idYear /* = -1 */, int idActor /* = -1 */, int idDirector /* = -1 */, int idStudio /* = -1 */, int idTag /* = -1 */,
                                  const SortDescription &sortDescription /* = SortDescription() */, int getDetails /* = VideoDbDetailsNone */)
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
  if (!CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_VIDEOLIBRARY_SHOWEMPTYTVSHOWS))
    filter.AppendWhere("totalCount IS NOT NULL AND totalCount > 0");
  return GetTvShowsByWhere(videoUrl.ToString(), filter, items, sortDescription, getDetails);
}

bool CVideoDatabase::GetTvShowsByWhere(const std::string& strBaseDir, const Filter &filter, CFileItemList& items, const SortDescription &sortDescription /* = SortDescription() */, int getDetails /* = VideoDbDetailsNone */)
{
  try
  {
    movieTime = 0;

    if (nullptr == m_pDB)
      return false;
    if (nullptr == m_pDS)
      return false;

    int total = -1;

    std::string strSQL = "SELECT %s FROM tvshow_view ";
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
    if (!SortUtils::SortFromDataset(sorting, MediaTypeTvShow, m_pDS, results))
      return false;

    // get data from returned rows
    items.Reserve(results.size());
    const query_data &data = m_pDS->get_result_set().records;
    for (const auto &i : results)
    {
      unsigned int targetRow = (unsigned int)i.at(FieldRow).asInteger();
      const dbiplus::sql_record* const record = data.at(targetRow);

      CFileItemPtr pItem(new CFileItem());
      CVideoInfoTag movie = GetDetailsForTvShow(record, getDetails, pItem.get());
      if (m_profileManager.GetMasterProfile().getLockMode() == LOCK_MODE_EVERYONE ||
           g_passwordManager.bMasterUser                                     ||
           g_passwordManager.IsDatabasePathUnlocked(movie.m_strPath, *CMediaSourceSettings::GetInstance().GetSources("video")))
      {
        pItem->SetFromVideoInfoTag(movie);

        CVideoDbUrl itemUrl = videoUrl;
        std::string path = StringUtils::Format("%i/", record->at(0).get_asInt());
        itemUrl.AppendPath(path);
        pItem->SetPath(itemUrl.ToString());

        pItem->SetOverlayImage(CGUIListItem::ICON_OVERLAY_UNWATCHED, (pItem->GetVideoInfoTag()->GetPlayCount() > 0) && (pItem->GetVideoInfoTag()->m_iEpisode > 0));
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

bool CVideoDatabase::GetEpisodesNav(const std::string& strBaseDir, CFileItemList& items, int idGenre, int idYear, int idActor, int idDirector, int idShow, int idSeason, const SortDescription &sortDescription /* = SortDescription() */, int getDetails /* = VideoDbDetailsNone */)
{
  CVideoDbUrl videoUrl;
  if (!videoUrl.FromString(strBaseDir))
    return false;

  std::string strIn;
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
  bool ret = GetEpisodesByWhere(videoUrl.ToString(), filter, items, false, sortDescription, getDetails);

  if (idSeason == -1 && idShow != -1)
  { // add any linked movies
    Filter movieFilter;
    movieFilter.join  = PrepareSQL("join movielinktvshow on movielinktvshow.idMovie=movie_view.idMovie");
    movieFilter.where = PrepareSQL("movielinktvshow.idShow = %i", idShow);
    CFileItemList movieItems;
    GetMoviesByWhere("videodb://movies/titles/", movieFilter, movieItems);

    if (movieItems.Size() > 0)
      items.Append(movieItems);
  }

  return ret;
}

bool CVideoDatabase::GetEpisodesByWhere(const std::string& strBaseDir, const Filter &filter, CFileItemList& items, bool appendFullShowPath /* = true */, const SortDescription &sortDescription /* = SortDescription() */, int getDetails /* = VideoDbDetailsNone */)
{
  try
  {
    movieTime = 0;
    castTime = 0;

    if (nullptr == m_pDB)
      return false;
    if (nullptr == m_pDS)
      return false;

    int total = -1;

    std::string strSQL = "select %s from episode_view ";
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
    for (const auto &i : results)
    {
      unsigned int targetRow = (unsigned int)i.at(FieldRow).asInteger();
      const dbiplus::sql_record* const record = data.at(targetRow);

      CVideoInfoTag episode = GetDetailsForEpisode(record, getDetails);
      if (m_profileManager.GetMasterProfile().getLockMode() == LOCK_MODE_EVERYONE ||
          g_passwordManager.bMasterUser                                     ||
          g_passwordManager.IsDatabasePathUnlocked(episode.m_strPath, *CMediaSourceSettings::GetInstance().GetSources("video")))
      {
        CFileItemPtr pItem(new CFileItem(episode));
        formatter.FormatLabel(pItem.get());

        int idEpisode = record->at(0).get_asInt();

        CVideoDbUrl itemUrl = videoUrl;
        std::string path;
        if (appendFullShowPath && videoUrl.GetItemType() != "episodes")
          path = StringUtils::Format("%i/%i/%i", record->at(VIDEODB_DETAILS_EPISODE_TVSHOW_ID).get_asInt(), episode.m_iSeason, idEpisode);
        else
          path = StringUtils::Format("%i", idEpisode);
        itemUrl.AppendPath(path);
        pItem->SetPath(itemUrl.ToString());
        pItem->SetDynPath(episode.m_strFileNameAndPath);

        pItem->SetOverlayImage(CGUIListItem::ICON_OVERLAY_UNWATCHED, episode.GetPlayCount() > 0);
        pItem->m_dateTime = episode.m_firstAired;
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

bool CVideoDatabase::GetMusicVideosNav(const std::string& strBaseDir, CFileItemList& items, int idGenre, int idYear, int idArtist, int idDirector, int idStudio, int idAlbum, int idTag /* = -1 */, const SortDescription &sortDescription /* = SortDescription() */, int getDetails /* = VideoDbDetailsNone */)
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
  return GetMusicVideosByWhere(videoUrl.ToString(), filter, items, true, sortDescription, getDetails);
}

bool CVideoDatabase::GetRecentlyAddedMoviesNav(const std::string& strBaseDir, CFileItemList& items, unsigned int limit /* = 0 */, int getDetails /* = VideoDbDetailsNone */)
{
  Filter filter;
  filter.order = "dateAdded desc, idMovie desc";
  filter.limit = PrepareSQL("%u", limit ? limit : CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_iVideoLibraryRecentlyAddedItems);
  return GetMoviesByWhere(strBaseDir, filter, items, SortDescription(), getDetails);
}

bool CVideoDatabase::GetRecentlyAddedEpisodesNav(const std::string& strBaseDir, CFileItemList& items, unsigned int limit /* = 0 */, int getDetails /* = VideoDbDetailsNone */)
{
  Filter filter;
  filter.order = "dateAdded desc, idEpisode desc";
  filter.limit = PrepareSQL("%u", limit ? limit : CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_iVideoLibraryRecentlyAddedItems);
  return GetEpisodesByWhere(strBaseDir, filter, items, false, SortDescription(), getDetails);
}

bool CVideoDatabase::GetRecentlyAddedMusicVideosNav(const std::string& strBaseDir, CFileItemList& items, unsigned int limit /* = 0 */, int getDetails /* = VideoDbDetailsNone */)
{
  Filter filter;
  filter.order = "dateAdded desc, idMVideo desc";
  filter.limit = PrepareSQL("%u", limit ? limit : CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_iVideoLibraryRecentlyAddedItems);
  return GetMusicVideosByWhere(strBaseDir, filter, items, true, SortDescription(), getDetails);
}

bool CVideoDatabase::GetInProgressTvShowsNav(const std::string& strBaseDir, CFileItemList& items, unsigned int limit /* = 0 */, int getDetails /* = VideoDbDetailsNone */)
{
  Filter filter;
  filter.order = PrepareSQL("c%02d", VIDEODB_ID_TV_TITLE);
  filter.where = "watchedCount != 0 AND totalCount != watchedCount";
  return GetTvShowsByWhere(strBaseDir, filter, items, SortDescription(), getDetails);
}

std::string CVideoDatabase::GetGenreById(int id)
{
  return GetSingleValue("genre", "name", PrepareSQL("genre_id=%i", id));
}

std::string CVideoDatabase::GetCountryById(int id)
{
  return GetSingleValue("country", "name", PrepareSQL("country_id=%i", id));
}

std::string CVideoDatabase::GetSetById(int id)
{
  return GetSingleValue("sets", "strSet", PrepareSQL("idSet=%i", id));
}

std::string CVideoDatabase::GetTagById(int id)
{
  return GetSingleValue("tag", "name", PrepareSQL("tag_id = %i", id));
}

std::string CVideoDatabase::GetPersonById(int id)
{
  return GetSingleValue("actor", "name", PrepareSQL("actor_id=%i", id));
}

std::string CVideoDatabase::GetStudioById(int id)
{
  return GetSingleValue("studio", "name", PrepareSQL("studio_id=%i", id));
}

std::string CVideoDatabase::GetTvShowTitleById(int id)
{
  return GetSingleValue("tvshow", PrepareSQL("c%02d", VIDEODB_ID_TV_TITLE), PrepareSQL("idShow=%i", id));
}

std::string CVideoDatabase::GetMusicVideoAlbumById(int id)
{
  return GetSingleValue("musicvideo", PrepareSQL("c%02d", VIDEODB_ID_MUSICVIDEO_ALBUM), PrepareSQL("idMVideo=%i", id));
}

bool CVideoDatabase::HasSets() const
{
  try
  {
    if (nullptr == m_pDB)
      return false;
    if (nullptr == m_pDS)
      return false;

    m_pDS->query("SELECT movie_view.idSet,COUNT(1) AS c FROM movie_view "
                 "JOIN sets ON sets.idSet = movie_view.idSet "
                 "GROUP BY movie_view.idSet HAVING c>1");

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
    if (nullptr == m_pDB)
      return false;
    if (nullptr == m_pDS2)
      return false;

    // make sure we use m_pDS2, as this is called in loops using m_pDS
    std::string strSQL=PrepareSQL("select idShow from episode where idEpisode=%i", idEpisode);
    m_pDS2->query( strSQL );

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
  std::string id = GetSingleValue("episode", column, PrepareSQL("idEpisode=%i", idEpisode));
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
    if (nullptr == m_pDB)
      return false;
    if (nullptr == m_pDS)
      return false;

    std::string sql;
    if (type == VIDEODB_CONTENT_MOVIES)
      sql = "select count(1) from movie";
    else if (type == VIDEODB_CONTENT_TVSHOWS)
      sql = "select count(1) from tvshow";
    else if (type == VIDEODB_CONTENT_MUSICVIDEOS)
      sql = "select count(1) from musicvideo";
    m_pDS->query( sql );

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

ScraperPtr CVideoDatabase::GetScraperForPath( const std::string& strPath )
{
  SScanSettings settings;
  return GetScraperForPath(strPath, settings);
}

ScraperPtr CVideoDatabase::GetScraperForPath(const std::string& strPath, SScanSettings& settings)
{
  bool dummy;
  return GetScraperForPath(strPath, settings, dummy);
}

ScraperPtr CVideoDatabase::GetScraperForPath(const std::string& strPath, SScanSettings& settings, bool& foundDirectly)
{
  foundDirectly = false;
  try
  {
    if (strPath.empty() || !m_pDB || !m_pDS)
      return ScraperPtr();

    ScraperPtr scraper;
    std::string strPath2;

    if (URIUtils::IsMultiPath(strPath))
      strPath2 = CMultiPathDirectory::GetFirstPath(strPath);
    else
      strPath2 = strPath;

    std::string strPath1 = URIUtils::GetDirectory(strPath2);
    int idPath = GetPathId(strPath1);

    if (idPath > -1)
    {
      std::string strSQL = PrepareSQL(
          "SELECT path.strContent, path.strScraper, path.scanRecursive, path.useFolderNames, "
          "path.strSettings, path.noUpdate, path.exclude, path.allAudio FROM path WHERE idPath=%i",
          idPath);
      m_pDS->query( strSQL );
    }

    int iFound = 1;
    CONTENT_TYPE content = CONTENT_NONE;
    if (!m_pDS->eof())
    { // path is stored in db

      settings.all_ext_audio = m_pDS->fv("path.allAudio").get_asBool();

      if (m_pDS->fv("path.exclude").get_asBool())
      {
        settings.exclude = true;
        m_pDS->close();
        return ScraperPtr();
      }
      settings.exclude = false;

      // try and ascertain scraper for this path
      std::string strcontent = m_pDS->fv("path.strContent").get_asString();
      StringUtils::ToLower(strcontent);
      content = TranslateContent(strcontent);

      //FIXME paths stored should not have empty strContent
      //assert(content != CONTENT_NONE);
      std::string scraperID = m_pDS->fv("path.strScraper").get_asString();

      AddonPtr addon;
      if (!scraperID.empty() && CServiceBroker::GetAddonMgr().GetAddon(scraperID, addon))
      {
        scraper = std::dynamic_pointer_cast<CScraper>(addon);
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
      std::string strParent;
      while (URIUtils::GetParentPath(strPath1, strParent))
      {
        iFound++;

        std::string strSQL =
            PrepareSQL("SELECT path.strContent, path.strScraper, path.scanRecursive, "
                       "path.useFolderNames, path.strSettings, path.noUpdate, path.exclude, "
                       "path.allAudio FROM path WHERE strPath='%s'",
                       strParent.c_str());
        m_pDS->query(strSQL);

        CONTENT_TYPE content = CONTENT_NONE;
        if (!m_pDS->eof())
        {
          settings.all_ext_audio = m_pDS->fv("path.allAudio").get_asBool();
          std::string strcontent = m_pDS->fv("path.strContent").get_asString();
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
              CServiceBroker::GetAddonMgr().GetAddon(m_pDS->fv("path.strScraper").get_asString(), addon))
          {
            scraper = std::dynamic_pointer_cast<CScraper>(addon);
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

bool CVideoDatabase::GetUseAllExternalAudioForVideo(const std::string& videoPath)
{
  // Find longest configured source path for given video path
  std::string strSQL = PrepareSQL("SELECT allAudio FROM path WHERE allAudio IS NOT NULL AND "
                                  "instr('%s', strPath) = 1 ORDER BY length(strPath) DESC LIMIT 1",
                                  videoPath.c_str());
  m_pDS->query(strSQL);

  if (!m_pDS->eof())
    return m_pDS->fv("allAudio").get_asBool();

  return false;
}

std::string CVideoDatabase::GetContentForPath(const std::string& strPath)
{
  SScanSettings settings;
  bool foundDirectly = false;
  ScraperPtr scraper = GetScraperForPath(strPath, settings, foundDirectly);
  if (scraper)
  {
    if (scraper->Content() == CONTENT_TVSHOWS)
    {
      // check for episodes or seasons.  Assumptions are:
      // 1. if episodes are in the path then we're in episodes.
      // 2. if no episodes are found, and content was set directly on this path, then we're in shows.
      // 3. if no episodes are found, and content was not set directly on this path, we're in seasons (assumes tvshows/seasons/episodes)
      std::string sql = "SELECT COUNT(*) FROM episode_view ";

      if (foundDirectly)
        sql += PrepareSQL("WHERE strPath = '%s'", strPath.c_str());
      else
        sql += PrepareSQL("WHERE strPath LIKE '%s%%'", strPath.c_str());

      m_pDS->query( sql );
      if (m_pDS->num_rows() && m_pDS->fv(0).get_asInt() > 0)
        return "episodes";
      return foundDirectly ? "tvshows" : "seasons";
    }
    return TranslateContent(scraper->Content());
  }
  return "";
}

void CVideoDatabase::GetMovieGenresByName(const std::string& strSearch, CFileItemList& items)
{
  std::string strSQL;

  try
  {
    if (nullptr == m_pDB)
      return;
    if (nullptr == m_pDS)
      return;

    if (m_profileManager.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL=PrepareSQL("SELECT genre.genre_id, genre.name, path.strPath FROM genre INNER JOIN genre_link ON genre_link.genre_id = genre.genre_id INNER JOIN movie ON (genre_link.media_type='movie' = genre_link.media_id=movie.idMovie) INNER JOIN files ON files.idFile=movie.idFile INNER JOIN path ON path.idPath=files.idPath WHERE genre.name LIKE '%%%s%%'",strSearch.c_str());
    else
      strSQL=PrepareSQL("SELECT DISTINCT genre.genre_id, genre.name FROM genre INNER JOIN genre_link ON genre_link.genre_id=genre.genre_id WHERE genre_link.media_type='movie' AND name LIKE '%%%s%%'", strSearch.c_str());
    m_pDS->query( strSQL );

    while (!m_pDS->eof())
    {
      if (m_profileManager.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(m_pDS->fv("path.strPath").get_asString(),
                                                      *CMediaSourceSettings::GetInstance().GetSources("video")))
        {
          m_pDS->next();
          continue;
        }

      CFileItemPtr pItem(new CFileItem(m_pDS->fv(1).get_asString()));
      std::string strDir = StringUtils::Format("%i/", m_pDS->fv(0).get_asInt());
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

void CVideoDatabase::GetMovieCountriesByName(const std::string& strSearch, CFileItemList& items)
{
  std::string strSQL;

  try
  {
    if (nullptr == m_pDB)
      return;
    if (nullptr == m_pDS)
      return;

    if (m_profileManager.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL=PrepareSQL("SELECT country.country_id, country.name, path.strPath FROM country INNER JOIN country_link ON country_link.country_id=country.country_id INNER JOIN movie ON country_link.media_id=movie.idMovie INNER JOIN files ON files.idFile=movie.idFile INNER JOIN path ON path.idPath=files.idPath WHERE country_link.media_type='movie' AND country.name LIKE '%%%s%%'", strSearch.c_str());
    else
      strSQL=PrepareSQL("SELECT DISTINCT country.country_id, country.name FROM country INNER JOIN country_link ON country_link.country_id=country.country_id WHERE country_link.media_type='movie' AND name like '%%%s%%'", strSearch.c_str());
    m_pDS->query( strSQL );

    while (!m_pDS->eof())
    {
      if (m_profileManager.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(m_pDS->fv("path.strPath").get_asString(),
                                                      *CMediaSourceSettings::GetInstance().GetSources("video")))
        {
          m_pDS->next();
          continue;
        }

      CFileItemPtr pItem(new CFileItem(m_pDS->fv(1).get_asString()));
      std::string strDir = StringUtils::Format("%i/", m_pDS->fv(0).get_asInt());
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

void CVideoDatabase::GetTvShowGenresByName(const std::string& strSearch, CFileItemList& items)
{
  std::string strSQL;

  try
  {
    if (nullptr == m_pDB)
      return;
    if (nullptr == m_pDS)
      return;

    if (m_profileManager.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL=PrepareSQL("SELECT genre.genre_id, genre.name, path.strPath FROM genre INNER JOIN genre_link ON genre_link.genre_id=genre.genre_id INNER JOIN tvshow ON genre_link.media_id=tvshow.idShow INNER JOIN tvshowlinkpath ON tvshowlinkpath.idShow=tvshow.idShow INNER JOIN path ON path.idPath=tvshowlinkpath.idPath WHERE genre_link.media_type='tvshow' AND genre.name LIKE '%%%s%%'", strSearch.c_str());
    else
      strSQL=PrepareSQL("SELECT DISTINCT genre.genre_id, genre.name FROM genre INNER JOIN genre_link ON genre_link.genre_id=genre.genre_id WHERE genre_link.media_type='tvshow' AND name LIKE '%%%s%%'", strSearch.c_str());
    m_pDS->query( strSQL );

    while (!m_pDS->eof())
    {
      if (m_profileManager.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(m_pDS->fv("path.strPath").get_asString(),*CMediaSourceSettings::GetInstance().GetSources("video")))
        {
          m_pDS->next();
          continue;
        }

      CFileItemPtr pItem(new CFileItem(m_pDS->fv(1).get_asString()));
      std::string strDir = StringUtils::Format("%i/", m_pDS->fv(0).get_asInt());
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

void CVideoDatabase::GetMovieActorsByName(const std::string& strSearch, CFileItemList& items)
{
  std::string strSQL;

  try
  {
    if (nullptr == m_pDB)
      return;
    if (nullptr == m_pDS)
      return;

    if (m_profileManager.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL=PrepareSQL("SELECT actor.actor_id, actor.name, path.strPath FROM actor INNER JOIN actor_link ON actor_link.actor_id=actor.actor_id INNER JOIN movie ON actor_link.media_id=movie.idMovie INNER JOIN files ON files.idFile=movie.idFile INNER JOIN path ON path.idPath=files.idPath WHERE actor_link.media_type='movie' AND actor.name LIKE '%%%s%%'", strSearch.c_str());
    else
      strSQL=PrepareSQL("SELECT DISTINCT actor.actor_id, actor.name FROM actor INNER JOIN actor_link ON actor_link.actor_id=actor.actor_id INNER JOIN movie ON actor_link.media_id=movie.idMovie WHERE actor_link.media_type='movie' AND actor.name LIKE '%%%s%%'", strSearch.c_str());
    m_pDS->query( strSQL );

    while (!m_pDS->eof())
    {
      if (m_profileManager.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(m_pDS->fv("path.strPath").get_asString(),*CMediaSourceSettings::GetInstance().GetSources("video")))
        {
          m_pDS->next();
          continue;
        }

      CFileItemPtr pItem(new CFileItem(m_pDS->fv(1).get_asString()));
      std::string strDir = StringUtils::Format("%i/", m_pDS->fv(0).get_asInt());
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

void CVideoDatabase::GetTvShowsActorsByName(const std::string& strSearch, CFileItemList& items)
{
  std::string strSQL;

  try
  {
    if (nullptr == m_pDB)
      return;
    if (nullptr == m_pDS)
      return;

    if (m_profileManager.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL=PrepareSQL("SELECT actor.actor_id, actor.name, path.strPath FROM actor INNER JOIN actor_link ON actor_link.actor_id=actor.actor_id INNER JOIN tvshow ON actor_link.media_id=tvshow.idShow INNER JOIN tvshowlinkpath ON tvshowlinkpath.idPath=tvshow.idShow INNER JOIN path ON path.idPath=tvshowlinkpath.idPath WHERE actor_link.media_type='tvshow' AND actor.name LIKE '%%%s%%'", strSearch.c_str());
    else
      strSQL=PrepareSQL("SELECT DISTINCT actor.actor_id, actor.name FROM actor INNER JOIN actor_link ON actor_link.actor_id=actor.actor_id INNER JOIN tvshow ON actor_link.media_id=tvshow.idShow WHERE actor_link.media_type='tvshow' AND actor.name LIKE '%%%s%%'",strSearch.c_str());
    m_pDS->query( strSQL );

    while (!m_pDS->eof())
    {
      if (m_profileManager.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(m_pDS->fv("path.strPath").get_asString(),*CMediaSourceSettings::GetInstance().GetSources("video")))
        {
          m_pDS->next();
          continue;
        }

      CFileItemPtr pItem(new CFileItem(m_pDS->fv(1).get_asString()));
      std::string strDir = StringUtils::Format("%i/", m_pDS->fv(0).get_asInt());
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

void CVideoDatabase::GetMusicVideoArtistsByName(const std::string& strSearch, CFileItemList& items)
{
  std::string strSQL;

  try
  {
    if (nullptr == m_pDB)
      return;
    if (nullptr == m_pDS)
      return;

    std::string strLike;
    if (!strSearch.empty())
      strLike = "and actor.name like '%%%s%%'";
    if (m_profileManager.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL=PrepareSQL("SELECT actor.actor_id, actor.name, path.strPath FROM actor INNER JOIN actor_link ON actor_link.actor_id=actor.actor_id INNER JOIN musicvideo ON actor_link.media_id=musicvideo.idMVideo INNER JOIN files ON files.idFile=musicvideo.idFile INNER JOIN path ON path.idPath=files.idPath WHERE actor_link.media_type='musicvideo' "+strLike, strSearch.c_str());
    else
      strSQL=PrepareSQL("SELECT DISTINCT actor.actor_id, actor.name from actor INNER JOIN actor_link ON actor_link.actor_id=actor.actor_id WHERE actor_link.media_type='musicvideo' "+strLike,strSearch.c_str());
    m_pDS->query( strSQL );

    while (!m_pDS->eof())
    {
      if (m_profileManager.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(m_pDS->fv("path.strPath").get_asString(),*CMediaSourceSettings::GetInstance().GetSources("video")))
        {
          m_pDS->next();
          continue;
        }

      CFileItemPtr pItem(new CFileItem(m_pDS->fv(1).get_asString()));
      std::string strDir = StringUtils::Format("%i/", m_pDS->fv(0).get_asInt());
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

void CVideoDatabase::GetMusicVideoGenresByName(const std::string& strSearch, CFileItemList& items)
{
  std::string strSQL;

  try
  {
    if (nullptr == m_pDB)
      return;
    if (nullptr == m_pDS)
      return;

    if (m_profileManager.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL=PrepareSQL("SELECT genre.genre_id, genre.name, path.strPath FROM genre INNER JOIN genre_link ON genre_link.genre_id=genre.genre_id INNER JOIN musicvideo ON genre_link.media_id=musicvideo.idMVideo INNER JOIN files ON files.idFile=musicvideo.idFile INNER JOIN path ON path.idPath=files.idPath WHERE genre_link.media_type='musicvideo' AND genre.name LIKE '%%%s%%'", strSearch.c_str());
    else
      strSQL=PrepareSQL("SELECT DISTINCT genre.genre_id, genre.name FROM genre INNER JOIN genre_link ON genre_link.genre_id=genre.genre_id WHERE genre_link.media_type='musicvideo' AND genre.name LIKE '%%%s%%'", strSearch.c_str());
    m_pDS->query( strSQL );

    while (!m_pDS->eof())
    {
      if (m_profileManager.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(m_pDS->fv("path.strPath").get_asString(),*CMediaSourceSettings::GetInstance().GetSources("video")))
        {
          m_pDS->next();
          continue;
        }

      CFileItemPtr pItem(new CFileItem(m_pDS->fv(1).get_asString()));
      std::string strDir = StringUtils::Format("%i/", m_pDS->fv(0).get_asInt());
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

void CVideoDatabase::GetMusicVideoAlbumsByName(const std::string& strSearch, CFileItemList& items)
{
  std::string strSQL;

  try
  {
    if (nullptr == m_pDB)
      return;
    if (nullptr == m_pDS)
      return;

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

    m_pDS->query( strSQL );

    while (!m_pDS->eof())
    {
      if (m_pDS->fv(0).get_asString().empty())
      {
        m_pDS->next();
        continue;
      }

      if (m_profileManager.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(m_pDS->fv(2).get_asString(),*CMediaSourceSettings::GetInstance().GetSources("video")))
        {
          m_pDS->next();
          continue;
        }

      CFileItemPtr pItem(new CFileItem(m_pDS->fv(0).get_asString()));
      std::string strDir = StringUtils::Format("%i", m_pDS->fv(1).get_asInt());
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

void CVideoDatabase::GetMusicVideosByAlbum(const std::string& strSearch, CFileItemList& items)
{
  std::string strSQL;

  try
  {
    if (nullptr == m_pDB)
      return;
    if (nullptr == m_pDS)
      return;

    if (m_profileManager.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL = PrepareSQL("SELECT musicvideo.idMVideo, musicvideo.c%02d,musicvideo.c%02d, path.strPath FROM musicvideo INNER JOIN files ON files.idFile=musicvideo.idFile INNER JOIN path ON path.idPath=files.idPath WHERE musicvideo.c%02d LIKE '%%%s%%'", VIDEODB_ID_MUSICVIDEO_ALBUM, VIDEODB_ID_MUSICVIDEO_TITLE, VIDEODB_ID_MUSICVIDEO_ALBUM, strSearch.c_str());
    else
      strSQL = PrepareSQL("select musicvideo.idMVideo,musicvideo.c%02d,musicvideo.c%02d from musicvideo where musicvideo.c%02d like '%%%s%%'",VIDEODB_ID_MUSICVIDEO_ALBUM,VIDEODB_ID_MUSICVIDEO_TITLE,VIDEODB_ID_MUSICVIDEO_ALBUM,strSearch.c_str());
    m_pDS->query( strSQL );

    while (!m_pDS->eof())
    {
      if (m_profileManager.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(m_pDS->fv("path.strPath").get_asString(),*CMediaSourceSettings::GetInstance().GetSources("video")))
        {
          m_pDS->next();
          continue;
        }

      CFileItemPtr pItem(new CFileItem(m_pDS->fv(1).get_asString()+" - "+m_pDS->fv(2).get_asString()));
      std::string strDir = StringUtils::Format("3/2/%i",m_pDS->fv("musicvideo.idMVideo").get_asInt());

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

bool CVideoDatabase::GetMusicVideosByWhere(const std::string &baseDir, const Filter &filter, CFileItemList &items, bool checkLocks /*= true*/, const SortDescription &sortDescription /* = SortDescription() */, int getDetails /* = VideoDbDetailsNone */)
{
  try
  {
    movieTime = 0;
    castTime = 0;

    if (nullptr == m_pDB)
      return false;
    if (nullptr == m_pDS)
      return false;

    int total = -1;

    std::string strSQL = "select %s from musicvideo_view ";
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
    for (const auto &i : results)
    {
      unsigned int targetRow = (unsigned int)i.at(FieldRow).asInteger();
      const dbiplus::sql_record* const record = data.at(targetRow);

      CVideoInfoTag musicvideo = GetDetailsForMusicVideo(record, getDetails);
      if (!checkLocks || m_profileManager.GetMasterProfile().getLockMode() == LOCK_MODE_EVERYONE || g_passwordManager.bMasterUser ||
          g_passwordManager.IsDatabasePathUnlocked(musicvideo.m_strPath, *CMediaSourceSettings::GetInstance().GetSources("video")))
      {
        CFileItemPtr item(new CFileItem(musicvideo));

        CVideoDbUrl itemUrl = videoUrl;
        std::string path = StringUtils::Format("%i", record->at(0).get_asInt());
        itemUrl.AppendPath(path);
        item->SetPath(itemUrl.ToString());

        item->SetOverlayImage(CGUIListItem::ICON_OVERLAY_UNWATCHED, musicvideo.GetPlayCount() > 0);
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

unsigned int CVideoDatabase::GetRandomMusicVideoIDs(const std::string& strWhere, std::vector<std::pair<int,int> > &songIDs)
{
  try
  {
    if (nullptr == m_pDB)
      return 0;
    if (nullptr == m_pDS)
      return 0;

    std::string strSQL = "select distinct idMVideo from musicvideo_view";
    if (!strWhere.empty())
      strSQL += " where " + strWhere;
    strSQL += PrepareSQL(" ORDER BY RANDOM()");

    if (!m_pDS->query(strSQL)) return 0;
    songIDs.clear();
    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      return 0;
    }
    songIDs.reserve(m_pDS->num_rows());
    while (!m_pDS->eof())
    {
      songIDs.push_back(std::make_pair<int,int>(2,m_pDS->fv(0).get_asInt()));
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

int CVideoDatabase::GetMatchingMusicVideo(const std::string& strArtist, const std::string& strAlbum, const std::string& strTitle)
{
  try
  {
    if (nullptr == m_pDB)
      return -1;
    if (nullptr == m_pDS)
      return -1;

    std::string strSQL;
    if (strAlbum.empty() && strTitle.empty())
    { // we want to return matching artists only
      if (m_profileManager.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        strSQL=PrepareSQL("SELECT DISTINCT actor.actor_id, path.strPath FROM actor INNER JOIN actor_link ON actor_link.actor_id=actor.actor_id INNER JOIN musicvideo ON actor_link.media_id=musicvideo.idMVideo INNER JOIN files ON files.idFile=musicvideo.idFile INNER JOIN path ON path.idPath=files.idPath WHERE actor_link.media_type='musicvideo' AND actor.name like '%s'", strArtist.c_str());
      else
        strSQL=PrepareSQL("SELECT DISTINCT actor.actor_id FROM actor INNER JOIN actor_link ON actor_link.actor_id=actor.actor_id WHERE actor_link.media_type='musicvideo' AND actor.name LIKE '%s'", strArtist.c_str());
    }
    else
    { // we want to return the matching musicvideo
      if (m_profileManager.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        strSQL = PrepareSQL("SELECT musicvideo.idMVideo FROM actor INNER JOIN actor_link ON actor_link.actor_id=actor.actor_id INNER JOIN musicvideo ON actor_link.media_id=musicvideo.idMVideo INNER JOIN files ON files.idFile=musicvideo.idFile INNER JOIN path ON path.idPath=files.idPath WHERE actor_link.media_type='musicvideo' AND musicvideo.c%02d LIKE '%s' AND musicvideo.c%02d LIKE '%s' AND actor.name LIKE '%s'", VIDEODB_ID_MUSICVIDEO_ALBUM, strAlbum.c_str(), VIDEODB_ID_MUSICVIDEO_TITLE, strTitle.c_str(), strArtist.c_str());
      else
        strSQL = PrepareSQL("select musicvideo.idMVideo from musicvideo join actor_link on actor_link.media_id=musicvideo.idMVideo AND actor_link.media_type='musicvideo' join actor on actor.actor_id=actor_link.actor_id where musicvideo.c%02d like '%s' and musicvideo.c%02d like '%s' and actor.name like '%s'",VIDEODB_ID_MUSICVIDEO_ALBUM,strAlbum.c_str(),VIDEODB_ID_MUSICVIDEO_TITLE,strTitle.c_str(),strArtist.c_str());
    }
    m_pDS->query( strSQL );

    if (m_pDS->eof())
      return -1;

    if (m_profileManager.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      if (!g_passwordManager.IsDatabasePathUnlocked(m_pDS->fv("path.strPath").get_asString(),*CMediaSourceSettings::GetInstance().GetSources("video")))
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

void CVideoDatabase::GetMoviesByName(const std::string& strSearch, CFileItemList& items)
{
  std::string strSQL;

  try
  {
    if (nullptr == m_pDB)
      return;
    if (nullptr == m_pDS)
      return;

    if (m_profileManager.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL = PrepareSQL("SELECT movie.idMovie, movie.c%02d, path.strPath, movie.idSet FROM movie INNER JOIN files ON files.idFile=movie.idFile INNER JOIN path ON path.idPath=files.idPath WHERE movie.c%02d LIKE '%%%s%%'", VIDEODB_ID_TITLE, VIDEODB_ID_TITLE, strSearch.c_str());
    else
      strSQL = PrepareSQL("select movie.idMovie,movie.c%02d, movie.idSet from movie where movie.c%02d like '%%%s%%'",VIDEODB_ID_TITLE,VIDEODB_ID_TITLE,strSearch.c_str());
    m_pDS->query( strSQL );

    while (!m_pDS->eof())
    {
      if (m_profileManager.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(m_pDS->fv("path.strPath").get_asString(),*CMediaSourceSettings::GetInstance().GetSources("video")))
        {
          m_pDS->next();
          continue;
        }

      int movieId = m_pDS->fv("movie.idMovie").get_asInt();
      int setId = m_pDS->fv("movie.idSet").get_asInt();
      CFileItemPtr pItem(new CFileItem(m_pDS->fv(1).get_asString()));
      std::string path;
      if (setId <= 0 || !CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_VIDEOLIBRARY_GROUPMOVIESETS))
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

void CVideoDatabase::GetTvShowsByName(const std::string& strSearch, CFileItemList& items)
{
  std::string strSQL;

  try
  {
    if (nullptr == m_pDB)
      return;
    if (nullptr == m_pDS)
      return;

    if (m_profileManager.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL = PrepareSQL("SELECT tvshow.idShow, tvshow.c%02d, path.strPath FROM tvshow INNER JOIN tvshowlinkpath ON tvshowlinkpath.idShow=tvshow.idShow INNER JOIN path ON path.idPath=tvshowlinkpath.idPath WHERE tvshow.c%02d LIKE '%%%s%%'", VIDEODB_ID_TV_TITLE, VIDEODB_ID_TV_TITLE, strSearch.c_str());
    else
      strSQL = PrepareSQL("select tvshow.idShow,tvshow.c%02d from tvshow where tvshow.c%02d like '%%%s%%'",VIDEODB_ID_TV_TITLE,VIDEODB_ID_TV_TITLE,strSearch.c_str());
    m_pDS->query( strSQL );

    while (!m_pDS->eof())
    {
      if (m_profileManager.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(m_pDS->fv("path.strPath").get_asString(),*CMediaSourceSettings::GetInstance().GetSources("video")))
        {
          m_pDS->next();
          continue;
        }

      CFileItemPtr pItem(new CFileItem(m_pDS->fv(1).get_asString()));
      std::string strDir = StringUtils::Format("tvshows/titles/%i/", m_pDS->fv("tvshow.idShow").get_asInt());

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

void CVideoDatabase::GetEpisodesByName(const std::string& strSearch, CFileItemList& items)
{
  std::string strSQL;

  try
  {
    if (nullptr == m_pDB)
      return;
    if (nullptr == m_pDS)
      return;

    if (m_profileManager.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL = PrepareSQL("SELECT episode.idEpisode, episode.c%02d, episode.c%02d, episode.idShow, tvshow.c%02d, path.strPath FROM episode INNER JOIN tvshow ON tvshow.idShow=episode.idShow INNER JOIN files ON files.idFile=episode.idFile INNER JOIN path ON path.idPath=files.idPath WHERE episode.c%02d LIKE '%%%s%%'", VIDEODB_ID_EPISODE_TITLE, VIDEODB_ID_EPISODE_SEASON, VIDEODB_ID_TV_TITLE, VIDEODB_ID_EPISODE_TITLE, strSearch.c_str());
    else
      strSQL = PrepareSQL("SELECT episode.idEpisode, episode.c%02d, episode.c%02d, episode.idShow, tvshow.c%02d FROM episode INNER JOIN tvshow ON tvshow.idShow=episode.idShow WHERE episode.c%02d like '%%%s%%'", VIDEODB_ID_EPISODE_TITLE, VIDEODB_ID_EPISODE_SEASON, VIDEODB_ID_TV_TITLE, VIDEODB_ID_EPISODE_TITLE, strSearch.c_str());
    m_pDS->query( strSQL );

    while (!m_pDS->eof())
    {
      if (m_profileManager.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(m_pDS->fv("path.strPath").get_asString(),*CMediaSourceSettings::GetInstance().GetSources("video")))
        {
          m_pDS->next();
          continue;
        }

      CFileItemPtr pItem(new CFileItem(m_pDS->fv(1).get_asString()+" ("+m_pDS->fv(4).get_asString()+")"));
      std::string path = StringUtils::Format("videodb://tvshows/titles/%i/%i/%i",m_pDS->fv("episode.idShow").get_asInt(),m_pDS->fv(2).get_asInt(),m_pDS->fv(0).get_asInt());
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

void CVideoDatabase::GetMusicVideosByName(const std::string& strSearch, CFileItemList& items)
{
// Alternative searching - not quite as fast though due to
// retrieving all information
//  Filter filter(PrepareSQL("c%02d like '%s%%' or c%02d like '%% %s%%'", VIDEODB_ID_MUSICVIDEO_TITLE, strSearch.c_str(), VIDEODB_ID_MUSICVIDEO_TITLE, strSearch.c_str()));
//  GetMusicVideosByWhere("videodb://musicvideos/titles/", filter, items);
  std::string strSQL;

  try
  {
    if (nullptr == m_pDB)
      return;
    if (nullptr == m_pDS)
      return;

    if (m_profileManager.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL = PrepareSQL("SELECT musicvideo.idMVideo, musicvideo.c%02d, path.strPath FROM musicvideo INNER JOIN files ON files.idFile=musicvideo.idFile INNER JOIN path ON path.idPath=files.idPath WHERE musicvideo.c%02d LIKE '%%%s%%'", VIDEODB_ID_MUSICVIDEO_TITLE, VIDEODB_ID_MUSICVIDEO_TITLE, strSearch.c_str());
    else
      strSQL = PrepareSQL("select musicvideo.idMVideo,musicvideo.c%02d from musicvideo where musicvideo.c%02d like '%%%s%%'",VIDEODB_ID_MUSICVIDEO_TITLE,VIDEODB_ID_MUSICVIDEO_TITLE,strSearch.c_str());
    m_pDS->query( strSQL );

    while (!m_pDS->eof())
    {
      if (m_profileManager.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(m_pDS->fv("path.strPath").get_asString(),*CMediaSourceSettings::GetInstance().GetSources("video")))
        {
          m_pDS->next();
          continue;
        }

      CFileItemPtr pItem(new CFileItem(m_pDS->fv(1).get_asString()));
      std::string strDir = StringUtils::Format("3/2/%i",m_pDS->fv("musicvideo.idMVideo").get_asInt());

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

void CVideoDatabase::GetEpisodesByPlot(const std::string& strSearch, CFileItemList& items)
{
// Alternative searching - not quite as fast though due to
// retrieving all information
//  Filter filter;
//  filter.where = PrepareSQL("c%02d like '%s%%' or c%02d like '%% %s%%'", VIDEODB_ID_EPISODE_PLOT, strSearch.c_str(), VIDEODB_ID_EPISODE_PLOT, strSearch.c_str());
//  filter.where += PrepareSQL("or c%02d like '%s%%' or c%02d like '%% %s%%'", VIDEODB_ID_EPISODE_TITLE, strSearch.c_str(), VIDEODB_ID_EPISODE_TITLE, strSearch.c_str());
//  GetEpisodesByWhere("videodb://tvshows/titles/", filter, items);
//  return;
  std::string strSQL;

  try
  {
    if (nullptr == m_pDB)
      return;
    if (nullptr == m_pDS)
      return;

    if (m_profileManager.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL = PrepareSQL("SELECT episode.idEpisode, episode.c%02d, episode.c%02d, episode.idShow, tvshow.c%02d, path.strPath FROM episode INNER JOIN tvshow ON tvshow.idShow=episode.idShow INNER JOIN files ON files.idFile=episode.idFile INNER JOIN path ON path.idPath=files.idPath WHERE episode.c%02d LIKE '%%%s%%'", VIDEODB_ID_EPISODE_TITLE, VIDEODB_ID_EPISODE_SEASON, VIDEODB_ID_TV_TITLE, VIDEODB_ID_EPISODE_PLOT, strSearch.c_str());
    else
      strSQL = PrepareSQL("SELECT episode.idEpisode, episode.c%02d, episode.c%02d, episode.idShow, tvshow.c%02d FROM episode INNER JOIN tvshow ON tvshow.idShow=episode.idShow WHERE episode.c%02d LIKE '%%%s%%'", VIDEODB_ID_EPISODE_TITLE, VIDEODB_ID_EPISODE_SEASON, VIDEODB_ID_TV_TITLE, VIDEODB_ID_EPISODE_PLOT, strSearch.c_str());
    m_pDS->query( strSQL );

    while (!m_pDS->eof())
    {
      if (m_profileManager.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(m_pDS->fv("path.strPath").get_asString(),*CMediaSourceSettings::GetInstance().GetSources("video")))
        {
          m_pDS->next();
          continue;
        }

      CFileItemPtr pItem(new CFileItem(m_pDS->fv(1).get_asString()+" ("+m_pDS->fv(4).get_asString()+")"));
      std::string path = StringUtils::Format("videodb://tvshows/titles/%i/%i/%i",m_pDS->fv("episode.idShow").get_asInt(),m_pDS->fv(2).get_asInt(),m_pDS->fv(0).get_asInt());
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

void CVideoDatabase::GetMoviesByPlot(const std::string& strSearch, CFileItemList& items)
{
  std::string strSQL;

  try
  {
    if (nullptr == m_pDB)
      return;
    if (nullptr == m_pDS)
      return;

    if (m_profileManager.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL = PrepareSQL("select movie.idMovie, movie.c%02d, path.strPath FROM movie INNER JOIN files ON files.idFile=movie.idFile INNER JOIN path ON path.idPath=files.idPath WHERE movie.c%02d LIKE '%%%s%%' OR movie.c%02d LIKE '%%%s%%' OR movie.c%02d LIKE '%%%s%%'", VIDEODB_ID_TITLE,VIDEODB_ID_PLOT, strSearch.c_str(), VIDEODB_ID_PLOTOUTLINE, strSearch.c_str(), VIDEODB_ID_TAGLINE,strSearch.c_str());
    else
      strSQL = PrepareSQL("SELECT movie.idMovie, movie.c%02d FROM movie WHERE (movie.c%02d LIKE '%%%s%%' OR movie.c%02d LIKE '%%%s%%' OR movie.c%02d LIKE '%%%s%%')", VIDEODB_ID_TITLE, VIDEODB_ID_PLOT, strSearch.c_str(), VIDEODB_ID_PLOTOUTLINE, strSearch.c_str(), VIDEODB_ID_TAGLINE, strSearch.c_str());

    m_pDS->query( strSQL );

    while (!m_pDS->eof())
    {
      if (m_profileManager.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(m_pDS->fv(2).get_asString(),*CMediaSourceSettings::GetInstance().GetSources("video")))
        {
          m_pDS->next();
          continue;
        }

      CFileItemPtr pItem(new CFileItem(m_pDS->fv(1).get_asString()));
      std::string path = StringUtils::Format("videodb://movies/titles/%i", m_pDS->fv(0).get_asInt());
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

void CVideoDatabase::GetMovieDirectorsByName(const std::string& strSearch, CFileItemList& items)
{
  std::string strSQL;

  try
  {
    if (nullptr == m_pDB)
      return;
    if (nullptr == m_pDS)
      return;

    if (m_profileManager.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL = PrepareSQL("SELECT DISTINCT director_link.actor_id, actor.name, path.strPath FROM movie INNER JOIN director_link ON (director_link.media_id=movie.idMovie AND director_link.media_type='movie') INNER JOIN actor ON actor.actor_id=director_link.actor_id INNER JOIN files ON files.idFile=movie.idFile INNER JOIN path ON path.idPath=files.idPath WHERE actor.name LIKE '%%%s%%'", strSearch.c_str());
    else
      strSQL = PrepareSQL("SELECT DISTINCT director_link.actor_id, actor.name FROM actor INNER JOIN director_link ON director_link.actor_id=actor.actor_id INNER JOIN movie ON director_link.media_id=movie.idMovie WHERE director_link.media_type='movie' AND actor.name like '%%%s%%'", strSearch.c_str());

    m_pDS->query( strSQL );

    while (!m_pDS->eof())
    {
      if (m_profileManager.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(m_pDS->fv("path.strPath").get_asString(),*CMediaSourceSettings::GetInstance().GetSources("video")))
        {
          m_pDS->next();
          continue;
        }

      std::string strDir = StringUtils::Format("%i/", m_pDS->fv(0).get_asInt());
      CFileItemPtr pItem(new CFileItem(m_pDS->fv(1).get_asString()));

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

void CVideoDatabase::GetTvShowsDirectorsByName(const std::string& strSearch, CFileItemList& items)
{
  std::string strSQL;

  try
  {
    if (nullptr == m_pDB)
      return;
    if (nullptr == m_pDS)
      return;

    if (m_profileManager.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL = PrepareSQL("SELECT DISTINCT director_link.actor_id, actor.name, path.strPath FROM actor INNER JOIN director_link ON director_link.actor_id=actor.actor_id INNER JOIN tvshow ON director_link.media_id=tvshow.idShow INNER JOIN tvshowlinkpath ON tvshowlinkpath.idShow=tvshow.idShow INNER JOIN path ON path.idPath=tvshowlinkpath.idPath WHERE director_link.media_type='tvshow' AND actor.name LIKE '%%%s%%'", strSearch.c_str());
    else
      strSQL = PrepareSQL("SELECT DISTINCT director_link.actor_id, actor.name FROM actor INNER JOIN director_link ON director_link.actor_id=actor.actor_id INNER JOIN tvshow ON director_link.media_id=tvshow.idShow WHERE director_link.media_type='tvshow' AND actor.name LIKE '%%%s%%'", strSearch.c_str());

    m_pDS->query( strSQL );

    while (!m_pDS->eof())
    {
      if (m_profileManager.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(m_pDS->fv("path.strPath").get_asString(),*CMediaSourceSettings::GetInstance().GetSources("video")))
        {
          m_pDS->next();
          continue;
        }

      std::string strDir = StringUtils::Format("%i/", m_pDS->fv(0).get_asInt());
      CFileItemPtr pItem(new CFileItem(m_pDS->fv(1).get_asString()));

      pItem->SetPath("videodb://tvshows/directors/"+ strDir);
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

void CVideoDatabase::GetMusicVideoDirectorsByName(const std::string& strSearch, CFileItemList& items)
{
  std::string strSQL;

  try
  {
    if (nullptr == m_pDB)
      return;
    if (nullptr == m_pDS)
      return;

    if (m_profileManager.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL = PrepareSQL("SELECT DISTINCT director_link.actor_id, actor.name, path.strPath FROM actor INNER JOIN director_link ON director_link.actor_id=actor.actor_id INNER JOIN musicvideo ON director_link.media_id=musicvideo.idMVideo INNER JOIN files ON files.idFile=musicvideo.idFile INNER JOIN path ON path.idPath=files.idPath WHERE director_link.media_type='musicvideo' AND actor.name LIKE '%%%s%%'", strSearch.c_str());
    else
      strSQL = PrepareSQL("SELECT DISTINCT director_link.actor_id, actor.name FROM actor INNER JOIN director_link ON director_link.actor_id=actor.actor_id INNER JOIN musicvideo ON director_link.media_id=musicvideo.idMVideo WHERE director_link.media_type='musicvideo' AND actor.name LIKE '%%%s%%'", strSearch.c_str());

    m_pDS->query( strSQL );

    while (!m_pDS->eof())
    {
      if (m_profileManager.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(m_pDS->fv("path.strPath").get_asString(),*CMediaSourceSettings::GetInstance().GetSources("video")))
        {
          m_pDS->next();
          continue;
        }

      std::string strDir = StringUtils::Format("%i/", m_pDS->fv(0).get_asInt());
      CFileItemPtr pItem(new CFileItem(m_pDS->fv(1).get_asString()));

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

void CVideoDatabase::CleanDatabase(CGUIDialogProgressBarHandle* handle, const std::set<int>& paths, bool showProgress)
{
  CGUIDialogProgress *progress=NULL;
  try
  {
    if (nullptr == m_pDB)
      return;
    if (nullptr == m_pDS)
      return;
    if (nullptr == m_pDS2)
      return;

    unsigned int time = XbmcThreads::SystemClockMillis();
    CLog::Log(LOGINFO, "%s: Starting videodatabase cleanup ..", __FUNCTION__);
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

    // For directory caching to work properly, we need to sort the files by path
    sql += " ORDER BY path.strPath";

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
    CServiceBroker::GetMediaManager().GetRemovableDrives(videoSources);

    int total = m_pDS2->num_rows();
    int current = 0;
    std::string lastDir;
    bool gotDir = true;

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
        // Only consider keeping this file if not optical and belonging to a (matching) source
        bool bIsSource;
        if (!URIUtils::IsOnDVD(fullPath) &&
            CUtil::GetMatchingSource(fullPath, videoSources, bIsSource) >= 0)
        {
          const std::string pathDir = URIUtils::GetDirectory(fullPath);

          // Cache file's directory in case it's different from the previous file
          if (lastDir != pathDir)
          {
            lastDir = pathDir;
            CFileItemList items; // Dummy list
            gotDir = CDirectory::GetDirectory(pathDir, items, "", DIR_FLAG_NO_FILE_DIRS |
                                              DIR_FLAG_NO_FILE_INFO);
          }

          // Keep existing files
          if (gotDir && CFile::Exists(fullPath, true))
            del = false;
        }
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
    CLog::Log(LOGINFO, "%s: Cleaning videodatabase done. Operation took %s", __FUNCTION__,
              StringUtils::SecondsToTimeString(time / 1000).c_str());

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
  CServiceBroker::GetMediaManager().GetRemovableDrives(videoSources);

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

void CVideoDatabase::DumpToDummyFiles(const std::string &path)
{
  // get all tvshows
  CFileItemList items;
  GetTvShowsByWhere("videodb://tvshows/titles/", CDatabase::Filter(), items);
  std::string showPath = URIUtils::AddFileToFolder(path, "shows");
  CDirectory::Create(showPath);
  for (int i = 0; i < items.Size(); i++)
  {
    // create a folder in this directory
    std::string showName = CUtil::MakeLegalFileName(items[i]->GetVideoInfoTag()->m_strShowTitle);
    std::string TVFolder = URIUtils::AddFileToFolder(showPath, showName);
    if (CDirectory::Create(TVFolder))
    { // right - grab the episodes and dump them as well
      CFileItemList episodes;
      Filter filter(PrepareSQL("idShow=%i", items[i]->GetVideoInfoTag()->m_iDbId));
      GetEpisodesByWhere("videodb://tvshows/titles/", filter, episodes);
      for (int i = 0; i < episodes.Size(); i++)
      {
        CVideoInfoTag *tag = episodes[i]->GetVideoInfoTag();
        std::string episode = StringUtils::Format("%s.s%02de%02d.avi", showName.c_str(), tag->m_iSeason, tag->m_iEpisode);
        // and make a file
        std::string episodePath = URIUtils::AddFileToFolder(TVFolder, episode);
        CFile file;
        if (file.OpenForWrite(episodePath))
          file.Close();
      }
    }
  }
  // get all movies
  items.Clear();
  GetMoviesByWhere("videodb://movies/titles/", CDatabase::Filter(), items);
  std::string moviePath = URIUtils::AddFileToFolder(path, "movies");
  CDirectory::Create(moviePath);
  for (int i = 0; i < items.Size(); i++)
  {
    CVideoInfoTag *tag = items[i]->GetVideoInfoTag();
    std::string movie = StringUtils::Format("%s.avi", tag->m_strTitle.c_str());
    CFile file;
    if (file.OpenForWrite(URIUtils::AddFileToFolder(moviePath, movie)))
      file.Close();
  }
}

void CVideoDatabase::ExportToXML(const std::string &path, bool singleFile /* = true */, bool images /* = false */, bool actorThumbs /* false */, bool overwrite /*=false*/)
{
  int iFailCount = 0;
  CGUIDialogProgress *progress=NULL;
  try
  {
    if (nullptr == m_pDB)
      return;
    if (nullptr == m_pDS)
      return;
    if (nullptr == m_pDS2)
      return;

    // create a 3rd dataset as well as GetEpisodeDetails() etc. uses m_pDS2, and we need to do 3 nested queries on tv shows
    std::unique_ptr<Dataset> pDS;
    pDS.reset(m_pDB->CreateDataset());
    if (nullptr == pDS)
      return;

    std::unique_ptr<Dataset> pDS2;
    pDS2.reset(m_pDB->CreateDataset());
    if (nullptr == pDS2)
      return;

    // if we're exporting to a single folder, we export thumbs as well
    std::string exportRoot = URIUtils::AddFileToFolder(path, "kodi_videodb_" + CDateTime::GetCurrentDateTime().GetAsDBDate());
    std::string xmlFile = URIUtils::AddFileToFolder(exportRoot, "videodb.xml");
    std::string actorsDir = URIUtils::AddFileToFolder(exportRoot, "actors");
    std::string moviesDir = URIUtils::AddFileToFolder(exportRoot, "movies");
    std::string movieSetsDir = URIUtils::AddFileToFolder(exportRoot, "moviesets");
    std::string musicvideosDir = URIUtils::AddFileToFolder(exportRoot, "musicvideos");
    std::string tvshowsDir = URIUtils::AddFileToFolder(exportRoot, "tvshows");
    if (singleFile)
    {
      images = true;
      overwrite = false;
      actorThumbs = true;
      CDirectory::Remove(exportRoot);
      CDirectory::Create(exportRoot);
      CDirectory::Create(actorsDir);
      CDirectory::Create(moviesDir);
      CDirectory::Create(movieSetsDir);
      CDirectory::Create(musicvideosDir);
      CDirectory::Create(tvshowsDir);
    }

    progress = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogProgress>(WINDOW_DIALOG_PROGRESS);
    // find all movies
    std::string sql = "select * from movie_view";

    m_pDS->query(sql);

    if (progress)
    {
      progress->SetHeading(CVariant{647});
      progress->SetLine(0, CVariant{650});
      progress->SetLine(1, CVariant{""});
      progress->SetLine(2, CVariant{""});
      progress->SetPercentage(0);
      progress->Open();
      progress->ShowProgressBar(true);
    }

    int total = m_pDS->num_rows();
    int current = 0;

    // create our xml document
    CXBMCTinyXML xmlDoc;
    TiXmlDeclaration decl("1.0", "UTF-8", "yes");
    xmlDoc.InsertEndChild(decl);
    TiXmlNode *pMain = NULL;
    if (!singleFile)
      pMain = &xmlDoc;
    else
    {
      TiXmlElement xmlMainElement("videodb");
      pMain = xmlDoc.InsertEndChild(xmlMainElement);
      XMLUtils::SetInt(pMain,"version", GetExportVersion());
    }

    while (!m_pDS->eof())
    {
      CVideoInfoTag movie = GetDetailsForMovie(m_pDS, VideoDbDetailsAll);
      // strip paths to make them relative
      if (StringUtils::StartsWith(movie.m_strTrailer, movie.m_strPath))
        movie.m_strTrailer = movie.m_strTrailer.substr(movie.m_strPath.size());
      std::map<std::string, std::string> artwork;
      if (GetArtForItem(movie.m_iDbId, movie.m_type, artwork) && singleFile)
      {
        TiXmlElement additionalNode("art");
        for (const auto &i : artwork)
          XMLUtils::SetString(&additionalNode, i.first.c_str(), i.second);
        movie.Save(pMain, "movie", true, &additionalNode);
      }
      else
        movie.Save(pMain, "movie", singleFile);

      // reset old skip state
      bool bSkip = false;

      if (progress)
      {
        progress->SetLine(1, CVariant{movie.m_strTitle});
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
      if (!singleFile && CUtil::SupportsWriteFileOperations(movie.m_strFileNameAndPath))
      {
        if (!item.Exists(false))
        {
          CLog::Log(LOGINFO, "%s - Not exporting item %s as it does not exist", __FUNCTION__, movie.m_strFileNameAndPath.c_str());
          bSkip = true;
        }
        else
        {
          std::string nfoFile(URIUtils::ReplaceExtension(item.GetTBNFile(), ".nfo"));

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
              CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Error, g_localizeStrings.Get(20302), nfoFile);
              iFailCount++;
            }
          }
        }
      }
      if (!singleFile)
      {
        xmlDoc.Clear();
        TiXmlDeclaration decl("1.0", "UTF-8", "yes");
        xmlDoc.InsertEndChild(decl);
      }

      if (images && !bSkip)
      {
        if (singleFile)
        {
          std::string strFileName(movie.m_strTitle);
          if (movie.HasYear())
            strFileName += StringUtils::Format("_%i", movie.GetYear());
          item.SetPath(GetSafeFile(moviesDir, strFileName) + ".avi");
        }
        for (const auto &i : artwork)
        {
          std::string savedThumb = item.GetLocalArt(i.first, false);
          CTextureCache::GetInstance().Export(i.second, savedThumb, overwrite);
        }
        if (actorThumbs)
          ExportActorThumbs(actorsDir, movie, !singleFile, overwrite);
      }
      m_pDS->next();
      current++;
    }
    m_pDS->close();

    if (!singleFile)
      movieSetsDir = CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(
          CSettings::SETTING_VIDEOLIBRARY_MOVIESETSFOLDER);
    if (images && !movieSetsDir.empty())
    {
      // find all movie sets
      sql = "select idSet, strSet from sets";

      m_pDS->query(sql);

      total = m_pDS->num_rows();
      current = 0;

      while (!m_pDS->eof())
      {
        std::string title = m_pDS->fv("strSet").get_asString();

        if (progress)
        {
          progress->SetLine(1, CVariant{title});
          progress->SetPercentage(current * 100 / total);
          progress->Progress();
          if (progress->IsCanceled())
          {
            progress->Close();
            m_pDS->close();
            return;
          }
        }

        std::string itemPath = URIUtils::AddFileToFolder(movieSetsDir,
            CUtil::MakeLegalFileName(title, LEGAL_WIN32_COMPAT));
        if (CDirectory::Exists(itemPath) || CDirectory::Create(itemPath))
        {
          std::map<std::string, std::string> artwork;
          GetArtForItem(m_pDS->fv("idSet").get_asInt(), MediaTypeVideoCollection, artwork);
          for (const auto& art : artwork)
          {
            std::string savedThumb = URIUtils::AddFileToFolder(itemPath, art.first);
            CTextureCache::GetInstance().Export(art.second, savedThumb, overwrite);
          }
        }
        else
          CLog::Log(LOGDEBUG,
            "CVideoDatabase::%s - Not exporting movie set '%s' as could not create folder '%s'",
            __FUNCTION__, title.c_str(), itemPath.c_str());
        m_pDS->next();
        current++;
      }
      m_pDS->close();
    }

    // find all musicvideos
    sql = "select * from musicvideo_view";

    m_pDS->query(sql);

    total = m_pDS->num_rows();
    current = 0;

    while (!m_pDS->eof())
    {
      CVideoInfoTag movie = GetDetailsForMusicVideo(m_pDS, VideoDbDetailsAll);
      std::map<std::string, std::string> artwork;
      if (GetArtForItem(movie.m_iDbId, movie.m_type, artwork) && singleFile)
      {
        TiXmlElement additionalNode("art");
        for (const auto &i : artwork)
          XMLUtils::SetString(&additionalNode, i.first.c_str(), i.second);
        movie.Save(pMain, "musicvideo", true, &additionalNode);
      }
      else
        movie.Save(pMain, "musicvideo", singleFile);

      // reset old skip state
      bool bSkip = false;

      if (progress)
      {
        progress->SetLine(1, CVariant{movie.m_strTitle});
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
      if (!singleFile && CUtil::SupportsWriteFileOperations(movie.m_strFileNameAndPath))
      {
        if (!item.Exists(false))
        {
          CLog::Log(LOGINFO, "%s - Not exporting item %s as it does not exist", __FUNCTION__, movie.m_strFileNameAndPath.c_str());
          bSkip = true;
        }
        else
        {
          std::string nfoFile(URIUtils::ReplaceExtension(item.GetTBNFile(), ".nfo"));

          if (overwrite || !CFile::Exists(nfoFile, false))
          {
            if(!xmlDoc.SaveFile(nfoFile))
            {
              CLog::Log(LOGERROR, "%s: Musicvideo nfo export failed! ('%s')", __FUNCTION__, nfoFile.c_str());
              CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Error, g_localizeStrings.Get(20302), nfoFile);
              iFailCount++;
            }
          }
        }
      }
      if (!singleFile)
      {
        xmlDoc.Clear();
        TiXmlDeclaration decl("1.0", "UTF-8", "yes");
        xmlDoc.InsertEndChild(decl);
      }
      if (images && !bSkip)
      {
        if (singleFile)
        {
          std::string strFileName(StringUtils::Join(movie.m_artist, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoItemSeparator) + "." + movie.m_strTitle);
          if (movie.HasYear())
            strFileName += StringUtils::Format("_%i", movie.GetYear());
          item.SetPath(GetSafeFile(musicvideosDir, strFileName) + ".avi");
        }
        for (const auto &i : artwork)
        {
          std::string savedThumb = item.GetLocalArt(i.first, false);
          CTextureCache::GetInstance().Export(i.second, savedThumb, overwrite);
        }
      }
      m_pDS->next();
      current++;
    }
    m_pDS->close();

    // repeat for all tvshows
    sql = "SELECT * FROM tvshow_view";
    m_pDS->query(sql);

    total = m_pDS->num_rows();
    current = 0;

    while (!m_pDS->eof())
    {
      CVideoInfoTag tvshow = GetDetailsForTvShow(m_pDS, VideoDbDetailsAll);
      GetTvShowNamedSeasons(tvshow.m_iDbId, tvshow.m_namedSeasons);

      std::map<int, std::map<std::string, std::string> > seasonArt;
      GetTvShowSeasonArt(tvshow.m_iDbId, seasonArt);

      std::map<std::string, std::string> artwork;
      if (GetArtForItem(tvshow.m_iDbId, tvshow.m_type, artwork) && singleFile)
      {
        TiXmlElement additionalNode("art");
        for (const auto &i : artwork)
          XMLUtils::SetString(&additionalNode, i.first.c_str(), i.second);
        for (const auto &i : seasonArt)
        {
          TiXmlElement seasonNode("season");
          seasonNode.SetAttribute("num", i.first);
          for (const auto &j : i.second)
            XMLUtils::SetString(&seasonNode, j.first.c_str(), j.second);
          additionalNode.InsertEndChild(seasonNode);
        }
        tvshow.Save(pMain, "tvshow", true, &additionalNode);
      }
      else
        tvshow.Save(pMain, "tvshow", singleFile);

      // reset old skip state
      bool bSkip = false;

      if (progress)
      {
        progress->SetLine(1, CVariant{tvshow.m_strTitle});
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
      if (!singleFile && CUtil::SupportsWriteFileOperations(tvshow.m_strPath))
      {
        if (!item.Exists(false))
        {
          CLog::Log(LOGINFO, "%s - Not exporting item %s as it does not exist", __FUNCTION__, tvshow.m_strPath.c_str());
          bSkip = true;
        }
        else
        {
          std::string nfoFile = URIUtils::AddFileToFolder(tvshow.m_strPath, "tvshow.nfo");

          if (overwrite || !CFile::Exists(nfoFile, false))
          {
            if(!xmlDoc.SaveFile(nfoFile))
            {
              CLog::Log(LOGERROR, "%s: TVShow nfo export failed! ('%s')", __FUNCTION__, nfoFile.c_str());
              CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Error, g_localizeStrings.Get(20302), nfoFile);
              iFailCount++;
            }
          }
        }
      }
      if (!singleFile)
      {
        xmlDoc.Clear();
        TiXmlDeclaration decl("1.0", "UTF-8", "yes");
        xmlDoc.InsertEndChild(decl);
      }
      if (images && !bSkip)
      {
        if (singleFile)
          item.SetPath(GetSafeFile(tvshowsDir, tvshow.m_strTitle));

        for (const auto &i : artwork)
        {
          std::string savedThumb = item.GetLocalArt(i.first, true);
          CTextureCache::GetInstance().Export(i.second, savedThumb, overwrite);
        }

        if (actorThumbs)
          ExportActorThumbs(actorsDir, tvshow, !singleFile, overwrite);

        // export season thumbs
        for (const auto &i : seasonArt)
        {
          std::string seasonThumb;
          if (i.first == -1)
            seasonThumb = "season-all";
          else if (i.first == 0)
            seasonThumb = "season-specials";
          else
            seasonThumb = StringUtils::Format("season%02i", i.first);
          for (const auto &j : i.second)
          {
            std::string savedThumb(item.GetLocalArt(seasonThumb + "-" + j.first, true));
            if (!i.second.empty())
              CTextureCache::GetInstance().Export(j.second, savedThumb, overwrite);
          }
        }
      }

      // now save the episodes from this show
      sql = PrepareSQL("select * from episode_view where idShow=%i order by strFileName, idEpisode",tvshow.m_iDbId);
      pDS->query(sql);
      std::string showDir(item.GetPath());

      while (!pDS->eof())
      {
        CVideoInfoTag episode = GetDetailsForEpisode(pDS, VideoDbDetailsAll);
        std::map<std::string, std::string> artwork;
        if (GetArtForItem(episode.m_iDbId, MediaTypeEpisode, artwork) && singleFile)
        {
          TiXmlElement additionalNode("art");
          for (const auto &i : artwork)
            XMLUtils::SetString(&additionalNode, i.first.c_str(), i.second);
          episode.Save(pMain->LastChild(), "episodedetails", true, &additionalNode);
        }
        else if (singleFile)
          episode.Save(pMain->LastChild(), "episodedetails", singleFile);
        else
          episode.Save(pMain, "episodedetails", singleFile);
        pDS->next();
        // multi-episode files need dumping to the same XML
        while (!singleFile && !pDS->eof() &&
               episode.m_iFileId == pDS->fv("idFile").get_asInt())
        {
          episode = GetDetailsForEpisode(pDS, VideoDbDetailsAll);
          episode.Save(pMain, "episodedetails", singleFile);
          pDS->next();
        }

        // reset old skip state
        bool bSkip = false;

        CFileItem item(episode.m_strFileNameAndPath, false);
        if (!singleFile && CUtil::SupportsWriteFileOperations(episode.m_strFileNameAndPath))
        {
          if (!item.Exists(false))
          {
            CLog::Log(LOGINFO, "%s - Not exporting item %s as it does not exist", __FUNCTION__, episode.m_strFileNameAndPath.c_str());
            bSkip = true;
          }
          else
          {
            std::string nfoFile(URIUtils::ReplaceExtension(item.GetTBNFile(), ".nfo"));

            if (overwrite || !CFile::Exists(nfoFile, false))
            {
              if(!xmlDoc.SaveFile(nfoFile))
              {
                CLog::Log(LOGERROR, "%s: Episode nfo export failed! ('%s')", __FUNCTION__, nfoFile.c_str());
                CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Error, g_localizeStrings.Get(20302), nfoFile);
                iFailCount++;
              }
            }
          }
        }
        if (!singleFile)
        {
          xmlDoc.Clear();
          TiXmlDeclaration decl("1.0", "UTF-8", "yes");
          xmlDoc.InsertEndChild(decl);
        }

        if (images && !bSkip)
        {
          if (singleFile)
          {
            std::string epName = StringUtils::Format("s%02ie%02i.avi", episode.m_iSeason, episode.m_iEpisode);
            item.SetPath(URIUtils::AddFileToFolder(showDir, epName));
          }
          for (const auto &i : artwork)
          {
            std::string savedThumb = item.GetLocalArt(i.first, false);
            CTextureCache::GetInstance().Export(i.second, savedThumb, overwrite);
          }
          if (actorThumbs)
            ExportActorThumbs(actorsDir, episode, !singleFile, overwrite);
        }
      }
      pDS->close();
      m_pDS->next();
      current++;
    }
    m_pDS->close();

    if (!singleFile && progress)
    {
      progress->SetPercentage(100);
      progress->Progress();
    }

    if (singleFile)
    {
      // now dump path info
      std::set<std::string> paths;
      GetPaths(paths);
      TiXmlElement xmlPathElement("paths");
      TiXmlNode *pPaths = pMain->InsertEndChild(xmlPathElement);
      for (const auto &i : paths)
      {
        bool foundDirectly = false;
        SScanSettings settings;
        ScraperPtr info = GetScraperForPath(i, settings, foundDirectly);
        if (info && foundDirectly)
        {
          TiXmlElement xmlPathElement2("path");
          TiXmlNode *pPath = pPaths->InsertEndChild(xmlPathElement2);
          XMLUtils::SetString(pPath,"url", i);
          XMLUtils::SetInt(pPath,"scanrecursive", settings.recurse);
          XMLUtils::SetBoolean(pPath,"usefoldernames", settings.parent_name);
          XMLUtils::SetString(pPath,"content", TranslateContent(info->Content()));
          XMLUtils::SetString(pPath,"scraperpath", info->ID());
        }
      }
      xmlDoc.SaveFile(xmlFile);
    }
    CVariant data;
    if (singleFile)
    {
      data["root"] = exportRoot;
      data["file"] = xmlFile;
      if (iFailCount > 0)
        data["failcount"] = iFailCount;
    }
    CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::VideoLibrary, "xbmc", "OnExport", data);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
    iFailCount++;
  }

  if (progress)
    progress->Close();

  if (iFailCount > 0)
    HELPERS::ShowOKDialogText(CVariant{647}, CVariant{StringUtils::Format(g_localizeStrings.Get(15011).c_str(), iFailCount)});
}

void CVideoDatabase::ExportActorThumbs(const std::string &strDir, const CVideoInfoTag &tag, bool singleFiles, bool overwrite /*=false*/)
{
  std::string strPath(strDir);
  if (singleFiles)
  {
    strPath = URIUtils::AddFileToFolder(tag.m_strPath, ".actors");
    if (!CDirectory::Exists(strPath))
    {
      CDirectory::Create(strPath);
      CFile::SetHidden(strPath, true);
    }
  }

  for (const auto &i : tag.m_cast)
  {
    CFileItem item;
    item.SetLabel(i.strName);
    if (!i.thumb.empty())
    {
      std::string thumbFile(GetSafeFile(strPath, i.strName));
      CTextureCache::GetInstance().Export(i.thumb, thumbFile, overwrite);
    }
  }
}

void CVideoDatabase::ImportFromXML(const std::string &path)
{
  CGUIDialogProgress *progress=NULL;
  try
  {
    if (nullptr == m_pDB)
      return;
    if (nullptr == m_pDS)
      return;

    CXBMCTinyXML xmlDoc;
    if (!xmlDoc.LoadFile(URIUtils::AddFileToFolder(path, "videodb.xml")))
      return;

    TiXmlElement *root = xmlDoc.RootElement();
    if (!root) return;

    progress = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogProgress>(WINDOW_DIALOG_PROGRESS);
    if (progress)
    {
      progress->SetHeading(CVariant{648});
      progress->SetLine(0, CVariant{649});
      progress->SetLine(1, CVariant{330});
      progress->SetLine(2, CVariant{""});
      progress->SetPercentage(0);
      progress->Open();
      progress->ShowProgressBar(true);
    }

    int iVersion = 0;
    XMLUtils::GetInt(root, "version", iVersion);

    CLog::Log(LOGINFO, "%s: Starting import (export version = %i)", __FUNCTION__, iVersion);

    TiXmlElement *movie = root->FirstChildElement();
    int current = 0;
    int total = 0;
    // first count the number of items...
    while (movie)
    {
      if (StringUtils::CompareNoCase(movie->Value(), MediaTypeMovie, 5) == 0 ||
          StringUtils::CompareNoCase(movie->Value(), MediaTypeTvShow, 6) == 0 ||
          StringUtils::CompareNoCase(movie->Value(), MediaTypeMusicVideo, 10) == 0)
        total++;
      movie = movie->NextSiblingElement();
    }

    std::string actorsDir(URIUtils::AddFileToFolder(path, "actors"));
    std::string moviesDir(URIUtils::AddFileToFolder(path, "movies"));
    std::string movieSetsDir(URIUtils::AddFileToFolder(path, "moviesets"));
    std::string musicvideosDir(URIUtils::AddFileToFolder(path, "musicvideos"));
    std::string tvshowsDir(URIUtils::AddFileToFolder(path, "tvshows"));
    CVideoInfoScanner scanner;
    // add paths first (so we have scraper settings available)
    TiXmlElement *path = root->FirstChildElement("paths");
    path = path->FirstChildElement();
    while (path)
    {
      std::string strPath;
      if (XMLUtils::GetString(path,"url",strPath) && !strPath.empty())
        AddPath(strPath);

      std::string content;
      if (XMLUtils::GetString(path,"content", content) && !content.empty())
      { // check the scraper exists, if so store the path
        AddonPtr addon;
        std::string id;
        XMLUtils::GetString(path,"scraperpath",id);
        if (CServiceBroker::GetAddonMgr().GetAddon(id, addon))
        {
          SScanSettings settings;
          ScraperPtr scraper = std::dynamic_pointer_cast<CScraper>(addon);
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
      if (StringUtils::CompareNoCase(movie->Value(), MediaTypeMovie, 5) == 0)
      {
        info.Load(movie);
        CFileItem item(info);
        bool useFolders = info.m_basePath.empty() ? LookupByFolders(item.GetPath()) : false;
        std::string filename = info.m_strTitle;
        if (info.HasYear())
          filename += StringUtils::Format("_%i", info.GetYear());
        CFileItem artItem(item);
        artItem.SetPath(GetSafeFile(moviesDir, filename) + ".avi");
        scanner.GetArtwork(&artItem, CONTENT_MOVIES, useFolders, true, actorsDir);
        item.SetArt(artItem.GetArt());
        if (!item.GetVideoInfoTag()->m_set.title.empty())
        {
          std::string setPath = URIUtils::AddFileToFolder(movieSetsDir,
              CUtil::MakeLegalFileName(item.GetVideoInfoTag()->m_set.title, LEGAL_WIN32_COMPAT));
          if (CDirectory::Exists(setPath))
          {
            CGUIListItem::ArtMap setArt;
            CFileItem artItem(setPath, true);
            for (const auto& artType : CVideoThumbLoader::GetArtTypes(MediaTypeVideoCollection))
            {
              std::string artPath = CVideoThumbLoader::GetLocalArt(artItem, artType, true);
              if (!artPath.empty())
              {
                setArt[artType] = artPath;
              }
            }
            item.AppendArt(setArt, "set");
          }
        }
        scanner.AddVideo(&item, CONTENT_MOVIES, useFolders, true, NULL, true);
        current++;
      }
      else if (StringUtils::CompareNoCase(movie->Value(), MediaTypeMusicVideo, 10) == 0)
      {
        info.Load(movie);
        CFileItem item(info);
        bool useFolders = info.m_basePath.empty() ? LookupByFolders(item.GetPath()) : false;
        std::string filename = StringUtils::Join(info.m_artist, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoItemSeparator) + "." + info.m_strTitle;
        if (info.HasYear())
          filename += StringUtils::Format("_%i", info.GetYear());
        CFileItem artItem(item);
        artItem.SetPath(GetSafeFile(musicvideosDir, filename) + ".avi");
        scanner.GetArtwork(&artItem, CONTENT_MUSICVIDEOS, useFolders, true, actorsDir);
        item.SetArt(artItem.GetArt());
        scanner.AddVideo(&item, CONTENT_MUSICVIDEOS, useFolders, true, NULL, true);
        current++;
      }
      else if (StringUtils::CompareNoCase(movie->Value(), MediaTypeTvShow, 6) == 0)
      {
        // load the TV show in.  NOTE: This deletes all episodes under the TV Show, which may not be
        // what we desire.  It may make better sense to only delete (or even better, update) the show information
        info.Load(movie);
        URIUtils::AddSlashAtEnd(info.m_strPath);
        DeleteTvShow(info.m_strPath);
        CFileItem showItem(info);
        bool useFolders = info.m_basePath.empty() ? LookupByFolders(showItem.GetPath(), true) : false;
        CFileItem artItem(showItem);
        std::string artPath(GetSafeFile(tvshowsDir, info.m_strTitle));
        artItem.SetPath(artPath);
        scanner.GetArtwork(&artItem, CONTENT_TVSHOWS, useFolders, true, actorsDir);
        showItem.SetArt(artItem.GetArt());
        int showID = scanner.AddVideo(&showItem, CONTENT_TVSHOWS, useFolders, true, NULL, true);
        // season artwork
        std::map<int, std::map<std::string, std::string> > seasonArt;
        artItem.GetVideoInfoTag()->m_strPath = artPath;
        scanner.GetSeasonThumbs(*artItem.GetVideoInfoTag(), seasonArt, CVideoThumbLoader::GetArtTypes(MediaTypeSeason), true);
        for (const auto &i : seasonArt)
        {
          int seasonID = AddSeason(showID, i.first);
          SetArtForItem(seasonID, MediaTypeSeason, i.second);
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
          std::string filename = StringUtils::Format("s%02ie%02i.avi", info.m_iSeason, info.m_iEpisode);
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
        progress->SetLine(2, CVariant{info.m_strTitle});
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

bool CVideoDatabase::ImportArtFromXML(const TiXmlNode *node, std::map<std::string, std::string> &artwork)
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

void CVideoDatabase::ConstructPath(std::string& strDest, const std::string& strPath, const std::string& strFileName)
{
  if (URIUtils::IsStack(strFileName) ||
      URIUtils::IsInArchive(strFileName) || URIUtils::IsPlugin(strPath))
    strDest = strFileName;
  else
    strDest = URIUtils::AddFileToFolder(strPath, strFileName);
}

void CVideoDatabase::SplitPath(const std::string& strFileNameAndPath, std::string& strPath, std::string& strFileName)
{
  if (URIUtils::IsStack(strFileNameAndPath) || StringUtils::StartsWithNoCase(strFileNameAndPath, "rar://") || StringUtils::StartsWithNoCase(strFileNameAndPath, "zip://"))
  {
    URIUtils::GetParentPath(strFileNameAndPath,strPath);
    strFileName = strFileNameAndPath;
  }
  else if (URIUtils::IsPlugin(strFileNameAndPath))
  {
    CURL url(strFileNameAndPath);
    strPath = url.GetOptions().empty() ? url.GetWithoutFilename() : url.GetWithoutOptions();
    strFileName = strFileNameAndPath;
  }
  else
  {
    URIUtils::Split(strFileNameAndPath, strPath, strFileName);
    // Keep protocol options as part of the path
    if (URIUtils::IsURL(strFileNameAndPath))
    {
      CURL url(strFileNameAndPath);
      if (!url.GetProtocolOptions().empty())
        strPath += "|" + url.GetProtocolOptions();
    }
  }
}

void CVideoDatabase::InvalidatePathHash(const std::string& strPath)
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
      std::string strParent;
      if (URIUtils::GetParentPath(strPath, strParent) && (!URIUtils::IsPlugin(strPath) || !CURL(strParent).GetHostName().empty()))
        SetPathHash(strParent, "");
    }
  }
}

bool CVideoDatabase::CommitTransaction()
{
  if (CDatabase::CommitTransaction())
  { // number of items in the db has likely changed, so recalculate
    GUIINFO::CLibraryGUIInfo& guiInfo = CServiceBroker::GetGUI()->GetInfoManager().GetInfoProviders().GetLibraryInfoProvider();
    guiInfo.SetLibraryBool(LIBRARY_HAS_MOVIES, HasContent(VIDEODB_CONTENT_MOVIES));
    guiInfo.SetLibraryBool(LIBRARY_HAS_TVSHOWS, HasContent(VIDEODB_CONTENT_TVSHOWS));
    guiInfo.SetLibraryBool(LIBRARY_HAS_MUSICVIDEOS, HasContent(VIDEODB_CONTENT_MUSICVIDEOS));
    return true;
  }
  return false;
}

bool CVideoDatabase::SetSingleValue(VIDEODB_CONTENT_TYPE type, int dbId, int dbField, const std::string &strValue)
{
  std::string strSQL;
  try
  {
    if (nullptr == m_pDB || nullptr == m_pDS)
      return false;

    std::string strTable, strField;
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
    if (nullptr == m_pDB || nullptr == m_pDS)
      return false;

    sql = PrepareSQL("UPDATE %s SET %s='%s'", table.c_str(), fieldName.c_str(), strValue.c_str());
    if (!conditionName.empty())
      sql += PrepareSQL(" WHERE %s=%u", conditionName.c_str(), conditionValue);
    if (m_pDS->exec(sql) == 0)
      return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, sql.c_str());
  }
  return false;
}

std::string CVideoDatabase::GetSafeFile(const std::string &dir, const std::string &name) const
{
  std::string safeThumb(name);
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
  CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::VideoLibrary, "xbmc", "OnRemove", data);
}

void CVideoDatabase::AnnounceUpdate(std::string content, int id)
{
  CVariant data;
  data["type"] = content;
  data["id"] = id;
  CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::VideoLibrary, "xbmc", "OnUpdate", data);
}

bool CVideoDatabase::GetItemsForPath(const std::string &content, const std::string &strPath, CFileItemList &items)
{
  std::string path(strPath);

  if(URIUtils::IsMultiPath(path))
  {
    std::vector<std::string> paths;
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

void CVideoDatabase::AppendIdLinkFilter(const char* field, const char *table, const MediaType& mediaType, const char *view, const char *viewKey, const CUrlOptions::UrlOptions& options, Filter &filter)
{
  auto option = options.find((std::string)field + "id");
  if (option == options.end())
    return;

  filter.AppendJoin(PrepareSQL("JOIN %s_link ON %s_link.media_id=%s_view.%s AND %s_link.media_type='%s'", field, field, view, viewKey, field, mediaType.c_str()));
  filter.AppendWhere(PrepareSQL("%s_link.%s_id = %i", field, table, (int)option->second.asInteger()));
}

void CVideoDatabase::AppendLinkFilter(const char* field, const char *table, const MediaType& mediaType, const char *view, const char *viewKey, const CUrlOptions::UrlOptions& options, Filter &filter)
{
  auto option = options.find(field);
  if (option == options.end())
    return;

  filter.AppendJoin(PrepareSQL("JOIN %s_link ON %s_link.media_id=%s_view.%s AND %s_link.media_type='%s'", field, field, view, viewKey, field, mediaType.c_str()));
  filter.AppendJoin(PrepareSQL("JOIN %s ON %s.%s_id=%s_link.%s_id", table, table, field, table, field));
  filter.AppendWhere(PrepareSQL("%s.name like '%s'", table, option->second.asString().c_str()));
}

bool CVideoDatabase::GetFilter(CDbUrl &videoUrl, Filter &filter, SortDescription &sorting)
{
  if (!videoUrl.IsValid())
    return false;

  std::string type = videoUrl.GetType();
  std::string itemType = ((const CVideoDbUrl &)videoUrl).GetItemType();
  const CUrlOptions::UrlOptions& options = videoUrl.GetOptions();

  if (type == "movies")
  {
    AppendIdLinkFilter("genre", "genre", "movie", "movie", "idMovie", options, filter);
    AppendLinkFilter("genre", "genre", "movie", "movie", "idMovie", options, filter);

    AppendIdLinkFilter("country", "country", "movie", "movie", "idMovie", options, filter);
    AppendLinkFilter("country", "country", "movie", "movie", "idMovie", options, filter);

    AppendIdLinkFilter("studio", "studio", "movie", "movie", "idMovie", options, filter);
    AppendLinkFilter("studio", "studio", "movie", "movie", "idMovie", options, filter);

    AppendIdLinkFilter("director", "actor", "movie", "movie", "idMovie", options, filter);
    AppendLinkFilter("director", "actor", "movie", "movie", "idMovie", options, filter);

    auto option = options.find("year");
    if (option != options.end())
      filter.AppendWhere(PrepareSQL("movie_view.premiered like '%i%%'", (int)option->second.asInteger()));

    AppendIdLinkFilter("actor", "actor", "movie", "movie", "idMovie", options, filter);
    AppendLinkFilter("actor", "actor", "movie", "movie", "idMovie", options, filter);

    option = options.find("setid");
    if (option != options.end())
      filter.AppendWhere(PrepareSQL("movie_view.idSet = %i", (int)option->second.asInteger()));

    option = options.find("set");
    if (option != options.end())
      filter.AppendWhere(PrepareSQL("movie_view.strSet LIKE '%s'", option->second.asString().c_str()));

    AppendIdLinkFilter("tag", "tag", "movie", "movie", "idMovie", options, filter);
    AppendLinkFilter("tag", "tag", "movie", "movie", "idMovie", options, filter);
  }
  else if (type == "tvshows")
  {
    if (itemType == "tvshows")
    {
      AppendIdLinkFilter("genre", "genre", "tvshow", "tvshow", "idShow", options, filter);
      AppendLinkFilter("genre", "genre", "tvshow", "tvshow", "idShow", options, filter);

      AppendIdLinkFilter("studio", "studio", "tvshow", "tvshow", "idShow", options, filter);
      AppendLinkFilter("studio", "studio", "tvshow", "tvshow", "idShow", options, filter);

      AppendIdLinkFilter("director", "actor", "tvshow", "tvshow", "idShow", options, filter);

      auto option = options.find("year");
      if (option != options.end())
        filter.AppendWhere(PrepareSQL("tvshow_view.c%02d like '%%%i%%'", VIDEODB_ID_TV_PREMIERED, (int)option->second.asInteger()));

      AppendIdLinkFilter("actor", "actor", "tvshow", "tvshow", "idShow", options, filter);
      AppendLinkFilter("actor", "actor", "tvshow", "tvshow", "idShow", options, filter);

      AppendIdLinkFilter("tag", "tag", "tvshow", "tvshow", "idShow", options, filter);
      AppendLinkFilter("tag", "tag", "tvshow", "tvshow", "idShow", options, filter);
    }
    else if (itemType == "seasons")
    {
      auto option = options.find("tvshowid");
      if (option != options.end())
        filter.AppendWhere(PrepareSQL("season_view.idShow = %i", (int)option->second.asInteger()));

      AppendIdLinkFilter("genre", "genre", "tvshow", "season", "idShow", options, filter);

      AppendIdLinkFilter("director", "actor", "tvshow", "season", "idShow", options, filter);

      option = options.find("year");
      if (option != options.end())
        filter.AppendWhere(PrepareSQL("season_view.premiered like '%%%i%%'", (int)option->second.asInteger()));

      AppendIdLinkFilter("actor", "actor", "tvshow", "season", "idShow", options, filter);
    }
    else if (itemType == "episodes")
    {
      int idShow = -1;
      auto option = options.find("tvshowid");
      if (option != options.end())
        idShow = (int)option->second.asInteger();

      int season = -1;
      option = options.find("season");
      if (option != options.end())
        season = (int)option->second.asInteger();

      if (idShow > -1)
      {
        bool condition = false;

        AppendIdLinkFilter("genre", "genre", "tvshow", "episode", "idShow", options, filter);
        AppendLinkFilter("genre", "genre", "tvshow", "episode", "idShow", options, filter);

        AppendIdLinkFilter("director", "actor", "tvshow", "episode", "idShow", options, filter);
        AppendLinkFilter("director", "actor", "tvshow", "episode", "idShow", options, filter);

        option = options.find("year");
        if (option != options.end())
        {
          condition = true;
          filter.AppendWhere(PrepareSQL("episode_view.idShow = %i and episode_view.premiered like '%%%i%%'", idShow, (int)option->second.asInteger()));
        }

        AppendIdLinkFilter("actor", "actor", "tvshow", "episode", "idShow", options, filter);
        AppendLinkFilter("actor", "actor", "tvshow", "episode", "idShow", options, filter);

        if (!condition)
          filter.AppendWhere(PrepareSQL("episode_view.idShow = %i", idShow));

        if (season > -1)
        {
          if (season == 0) // season = 0 indicates a special - we grab all specials here (see below)
            filter.AppendWhere(PrepareSQL("episode_view.c%02d = %i", VIDEODB_ID_EPISODE_SEASON, season));
          else
            filter.AppendWhere(PrepareSQL("(episode_view.c%02d = %i or (episode_view.c%02d = 0 and (episode_view.c%02d = 0 or episode_view.c%02d = %i)))",
              VIDEODB_ID_EPISODE_SEASON, season, VIDEODB_ID_EPISODE_SEASON, VIDEODB_ID_EPISODE_SORTSEASON, VIDEODB_ID_EPISODE_SORTSEASON, season));
        }
      }
      else
      {
        option = options.find("year");
        if (option != options.end())
          filter.AppendWhere(PrepareSQL("episode_view.premiered like '%%%i%%'", (int)option->second.asInteger()));

        AppendIdLinkFilter("director", "actor", "episode", "episode", "idEpisode", options, filter);
        AppendLinkFilter("director", "actor", "episode", "episode", "idEpisode", options, filter);
      }
    }
  }
  else if (type == "musicvideos")
  {
    AppendIdLinkFilter("genre", "genre", "musicvideo", "musicvideo", "idMVideo", options, filter);
    AppendLinkFilter("genre", "genre", "musicvideo", "musicvideo", "idMVideo", options, filter);

    AppendIdLinkFilter("studio", "studio", "musicvideo", "musicvideo", "idMVideo", options, filter);
    AppendLinkFilter("studio", "studio", "musicvideo", "musicvideo", "idMVideo", options, filter);

    AppendIdLinkFilter("director", "actor", "musicvideo", "musicvideo", "idMVideo", options, filter);
    AppendLinkFilter("director", "actor", "musicvideo", "musicvideo", "idMVideo", options, filter);

    auto option = options.find("year");
    if (option != options.end())
      filter.AppendWhere(PrepareSQL("musicvideo_view.premiered like '%i%%'", (int)option->second.asInteger()));

    option = options.find("artistid");
    if (option != options.end())
    {
      if (itemType != "albums")
        filter.AppendJoin(PrepareSQL("JOIN actor_link ON actor_link.media_id=musicvideo_view.idMVideo AND actor_link.media_type='musicvideo'"));
      filter.AppendWhere(PrepareSQL("actor_link.actor_id = %i", (int)option->second.asInteger()));
    }

    option = options.find("artist");
    if (option != options.end())
    {
      if (itemType != "albums")
      {
        filter.AppendJoin(PrepareSQL("JOIN actor_link ON actor_link.media_id=musicvideo_view.idMVideo AND actor_link.media_type='musicvideo'"));
        filter.AppendJoin(PrepareSQL("JOIN actor ON actor.actor_id=actor_link.actor_id"));
      }
      filter.AppendWhere(PrepareSQL("actor.name LIKE '%s'", option->second.asString().c_str()));
    }

    option = options.find("albumid");
    if (option != options.end())
      filter.AppendWhere(PrepareSQL("musicvideo_view.c%02d = (select c%02d from musicvideo where idMVideo = %i)", VIDEODB_ID_MUSICVIDEO_ALBUM, VIDEODB_ID_MUSICVIDEO_ALBUM, (int)option->second.asInteger()));

    AppendIdLinkFilter("tag", "tag", "musicvideo", "musicvideo", "idMVideo", options, filter);
    AppendLinkFilter("tag", "tag", "musicvideo", "musicvideo", "idMVideo", options, filter);
  }
  else
    return false;

  auto option = options.find("xsp");
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
      if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_FILELISTS_IGNORETHEWHENSORTING))
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

bool CVideoDatabase::SetVideoUserRating(int dbId, int rating, const MediaType& mediaType)
{
  try
  {
    if (nullptr == m_pDB)
      return false;
    if (nullptr == m_pDS)
      return false;

    if (mediaType == MediaTypeNone)
      return false;

    std::string sql;
    if (mediaType == MediaTypeMovie)
      sql = PrepareSQL("UPDATE movie SET userrating=%i WHERE idMovie = %i", rating, dbId);
    else if (mediaType == MediaTypeEpisode)
      sql = PrepareSQL("UPDATE episode SET userrating=%i WHERE idEpisode = %i", rating, dbId);
    else if (mediaType == MediaTypeMusicVideo)
      sql = PrepareSQL("UPDATE musicvideo SET userrating=%i WHERE idMVideo = %i", rating, dbId);
    else if (mediaType == MediaTypeTvShow)
      sql = PrepareSQL("UPDATE tvshow SET userrating=%i WHERE idShow = %i", rating, dbId);
    else if (mediaType == MediaTypeSeason)
      sql = PrepareSQL("UPDATE seasons SET userrating=%i WHERE idSeason = %i", rating, dbId);

    m_pDS->exec(sql);
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%i, %s, %i) failed", __FUNCTION__, dbId, mediaType.c_str(), rating);
  }
  return false;
}
