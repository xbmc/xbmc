/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "dbwrappers/dataset.h"
#include "guilib/LocalizeStrings.h"
#include "media/MediaType.h"
#include "utils/StringUtils.h"
#include "utils/log.h"
#include "video/VideoDatabase.h"
#include "video/VideoDatabaseColumns.h"
#include "video/VideoManagerTypes.h"

void CVideoDatabase::InitializeVideoVersionTypeTable()
{
  assert(m_pDB->in_transaction());

  try
  {
    for (int id = VIDEO_VERSION_ID_BEGIN; id <= VIDEO_VERSION_ID_END; ++id)
    {
      // Exclude removed pre-populated "quality" values
      if (id == 40405 || (id >= 40418 && id <= 40430))
        continue;

      const std::string& type{g_localizeStrings.Get(id)};
      m_pDS->exec(PrepareSQL(
          "INSERT INTO videoversiontype (id, name, owner, itemType) VALUES(%i, '%s', %i, %i)", id,
          type.c_str(), VideoAssetTypeOwner::SYSTEM, VideoAssetType::VERSION));
    }
  }
  catch (...)
  {
    CLog::LogF(LOGERROR, "failed");
    throw;
  }
}
void CVideoDatabase::CreateTables()
{
  CLog::Log(LOGINFO, "create bookmark table");
  m_pDS->exec("CREATE TABLE bookmark ( idBookmark integer primary key, idFile integer, "
              "timeInSeconds double, totalTimeInSeconds double, thumbNailImage text, player text, "
              "playerState text, type integer)\n");

  CLog::Log(LOGINFO, "create settings table");
  m_pDS->exec(
      "CREATE TABLE settings ( idFile integer, Deinterlace bool,"
      "ViewMode integer,ZoomAmount float, PixelRatio float, VerticalShift float, AudioStream "
      "integer, SubtitleStream integer,"
      "SubtitleDelay float, SubtitlesOn bool, Brightness float, Contrast float, Gamma float,"
      "VolumeAmplification float, AudioDelay float, ResumeTime integer,"
      "Sharpness float, NoiseReduction float, NonLinStretch bool, PostProcess bool,"
      "ScalingMethod integer, DeinterlaceMode integer, StereoMode integer, StereoInvert bool, "
      "VideoStream integer,"
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
    columns += StringUtils::Format(",c{:02} text", i);

  columns += ", idSet integer, userrating integer, premiered text, originalLanguage text)";
  m_pDS->exec(columns);

  CLog::Log(LOGINFO, "create actor table");
  m_pDS->exec("CREATE TABLE actor ( actor_id INTEGER PRIMARY KEY, name TEXT, art_urls TEXT )");
  m_pDS->exec("CREATE TABLE actor_link(actor_id INTEGER, media_id INTEGER, media_type TEXT, role "
              "TEXT, cast_order INTEGER)");
  m_pDS->exec("CREATE TABLE director_link(actor_id INTEGER, media_id INTEGER, media_type TEXT)");
  m_pDS->exec("CREATE TABLE writer_link(actor_id INTEGER, media_id INTEGER, media_type TEXT)");

  CLog::Log(LOGINFO, "create path table");
  m_pDS->exec(
      "CREATE TABLE path ( idPath integer primary key, strPath text, strContent text, strScraper "
      "text, strHash text, scanRecursive integer, useFolderNames bool, strSettings text, noUpdate "
      "bool, exclude bool, allAudio bool, dateAdded text, idParentPath integer)");

  CLog::Log(LOGINFO, "create files table");
  m_pDS->exec("CREATE TABLE files ( idFile integer primary key, idPath integer, strFilename text, "
              "playCount integer, lastPlayed text, dateAdded text)");

  CLog::Log(LOGINFO, "create tvshow table");
  columns = "CREATE TABLE tvshow ( idShow integer primary key";

  for (int i = 0; i < VIDEODB_MAX_COLUMNS; i++)
    columns += StringUtils::Format(",c{:02} text", i);

  columns += ", userrating INTEGER, duration INTEGER, originalLanguage TEXT, tagLine TEXT)";
  m_pDS->exec(columns);

  CLog::Log(LOGINFO, "create episode table");
  columns = "CREATE TABLE episode ( idEpisode integer primary key, idFile integer";
  for (int i = 0; i < VIDEODB_MAX_COLUMNS; i++)
  {
    std::string column;
    if (i == VIDEODB_ID_EPISODE_SEASON || i == VIDEODB_ID_EPISODE_EPISODE ||
        i == VIDEODB_ID_EPISODE_BOOKMARK)
      column = StringUtils::Format(",c{:02} varchar(24)", i);
    else
      column = StringUtils::Format(",c{:02} text", i);

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
    columns += StringUtils::Format(",c{:02} text", i);

  columns += ", userrating integer, premiered text)";
  m_pDS->exec(columns);

  CLog::Log(LOGINFO, "create streaminfo table");
  m_pDS->exec("CREATE TABLE streamdetails (idFile integer, iStreamType integer, "
              "strVideoCodec text, fVideoAspect float, iVideoWidth integer, iVideoHeight integer, "
              "strAudioCodec text, iAudioChannels integer, strAudioLanguage text, "
              "strSubtitleLanguage text, iVideoDuration integer, strStereoMode text, "
              "strVideoLanguage text, "
              "strHdrType text)");

  CLog::Log(LOGINFO, "create sets table");
  m_pDS->exec("CREATE TABLE sets ( idSet integer primary key, strSet text, strOverview text, "
              "strOriginalSet text)");

  CLog::Log(LOGINFO, "create seasons table");
  m_pDS->exec("CREATE TABLE seasons ( idSeason integer primary key, idShow integer, season "
              "integer, name text, userrating integer, plot TEXT)");

  CLog::Log(LOGINFO, "create art table");
  m_pDS->exec("CREATE TABLE art(art_id INTEGER PRIMARY KEY, media_id INTEGER, media_type TEXT, "
              "type TEXT, url TEXT)");

  CLog::Log(LOGINFO, "create tag table");
  m_pDS->exec("CREATE TABLE tag (tag_id integer primary key, name TEXT)");
  m_pDS->exec("CREATE TABLE tag_link (tag_id integer, media_id integer, media_type TEXT)");

  CLog::Log(LOGINFO, "create rating table");
  m_pDS->exec("CREATE TABLE rating (rating_id INTEGER PRIMARY KEY, media_id INTEGER, media_type "
              "TEXT, rating_type TEXT, rating FLOAT, votes INTEGER)");

  CLog::Log(LOGINFO, "create uniqueid table");
  m_pDS->exec("CREATE TABLE uniqueid (uniqueid_id INTEGER PRIMARY KEY, media_id INTEGER, "
              "media_type TEXT, value TEXT, type TEXT)");

  CLog::Log(LOGINFO, "create videoversiontype table");
  m_pDS->exec("CREATE TABLE videoversiontype (id INTEGER PRIMARY KEY, name TEXT, owner INTEGER, "
              "itemType INTEGER)");
  CLog::Log(LOGINFO, "populate videoversiontype table");
  InitializeVideoVersionTypeTable();

  CLog::Log(LOGINFO, "create videoversion table");
  m_pDS->exec("CREATE TABLE videoversion (idFile INTEGER PRIMARY KEY, idMedia INTEGER, media_type "
              "TEXT, itemType INTEGER, idType INTEGER)");
}

void CVideoDatabase::CreateLinkIndex(const char* table)
{
  m_pDS->exec(PrepareSQL("CREATE UNIQUE INDEX ix_%s_1 ON %s (name(255))", table, table));
  m_pDS->exec(
      PrepareSQL("CREATE UNIQUE INDEX ix_%s_link_1 ON %s_link (%s_id, media_type(20), media_id)",
                 table, table, table));
  m_pDS->exec(
      PrepareSQL("CREATE UNIQUE INDEX ix_%s_link_2 ON %s_link (media_id, media_type(20), %s_id)",
                 table, table, table));
  m_pDS->exec(PrepareSQL("CREATE INDEX ix_%s_link_3 ON %s_link (media_type(20))", table, table));
}

void CVideoDatabase::CreateForeignLinkIndex(const char* table, const char* foreignkey)
{
  m_pDS->exec(
      PrepareSQL("CREATE UNIQUE INDEX ix_%s_link_1 ON %s_link (%s_id, media_type(20), media_id)",
                 table, table, foreignkey));
  m_pDS->exec(
      PrepareSQL("CREATE UNIQUE INDEX ix_%s_link_2 ON %s_link (media_id, media_type(20), %s_id)",
                 table, table, foreignkey));
  m_pDS->exec(PrepareSQL("CREATE INDEX ix_%s_link_3 ON %s_link (media_type(20))", table, table));
}

void CVideoDatabase::CreateAnalytics()
{
  /* indexes should be added on any columns that are used in     */
  /* a where or a join. primary key on a column is the same as a */
  /* unique index on that column, so there is no need to add any */
  /* index if no other columns are referred                      */

  /* order of indexes are important, for an index to be considered all  */
  /* columns up to the column in question have to have been specified   */
  /* select * from foolink where foo_id = 1, can not take               */
  /* advantage of a index that has been created on ( bar_id, foo_id )   */
  /* however an index on ( foo_id, bar_id ) will be considered for use  */

  CLog::Log(LOGINFO, "Creating indices");
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
  std::string createColIndex =
      StringUtils::Format("CREATE INDEX ix_episode_season_episode on episode (c{:02}, c{:02})",
                          VIDEODB_ID_EPISODE_SEASON, VIDEODB_ID_EPISODE_EPISODE);
  m_pDS->exec(createColIndex);
  createColIndex = StringUtils::Format("CREATE INDEX ix_episode_bookmark on episode (c{:02})",
                                       VIDEODB_ID_EPISODE_BOOKMARK);
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

  m_pDS->exec("CREATE INDEX ix_videoversion ON videoversion (idMedia, media_type(20))");

  m_pDS->exec(PrepareSQL("CREATE INDEX ix_movie_title ON movie (c%02d(255))", VIDEODB_ID_TITLE));

  m_pDS->exec(PrepareSQL("CREATE INDEX ix_tvshow_title ON tvshow (c%02d(255), c%02d(10))",
                         VIDEODB_ID_TV_TITLE, VIDEODB_ID_TV_PREMIERED));

  CreateLinkIndex("tag");
  CreateForeignLinkIndex("director", "actor");
  CreateForeignLinkIndex("writer", "actor");
  CreateLinkIndex("studio");
  CreateLinkIndex("genre");
  CreateLinkIndex("country");

  CLog::Log(LOGINFO, "Creating triggers");
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
              "DELETE FROM videoversion "
              "WHERE idFile=old.idFile AND idMedia=old.idMovie AND media_type='movie'; "
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
              "DELETE FROM uniqueid WHERE media_id=old.idMVideo AND media_type='musicvideo'; "
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
              "DELETE FROM art WHERE media_id=old.actor_id AND media_type IN "
              "('actor','artist','writer','director'); "
              "END");
  m_pDS->exec("CREATE TRIGGER delete_tag AFTER DELETE ON tag_link FOR EACH ROW BEGIN "
              "DELETE FROM tag WHERE tag_id=old.tag_id AND tag_id NOT IN (SELECT DISTINCT tag_id "
              "FROM tag_link); "
              "END");
  m_pDS->exec("CREATE TRIGGER delete_file AFTER DELETE ON files FOR EACH ROW BEGIN "
              "DELETE FROM bookmark WHERE idFile=old.idFile; "
              "DELETE FROM settings WHERE idFile=old.idFile; "
              "DELETE FROM stacktimes WHERE idFile=old.idFile; "
              "DELETE FROM streamdetails WHERE idFile=old.idFile; "
              "DELETE FROM videoversion WHERE idFile=old.idFile; "
              "DELETE FROM art WHERE media_id=old.idFile AND media_type='videoversion'; "
              "END");
  m_pDS->exec("CREATE TRIGGER delete_videoversion AFTER DELETE ON videoversion FOR EACH ROW BEGIN "
              "DELETE FROM art WHERE media_id=old.idFile AND media_type='videoversion'; "
              "DELETE FROM streamdetails WHERE idFile=old.idFile; "
              "END");

  CreateViews();
}

void CVideoDatabase::CreateViews()
{
  CLog::Log(LOGINFO, "create episode_view");
  std::string episodeview = PrepareSQL(
      "CREATE VIEW episode_view AS SELECT "
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
      "  tvshow.originalLanguage, "
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
      "  JOIN path ON"
      "    files.idPath=path.idPath"
      "  LEFT JOIN bookmark ON"
      "    bookmark.idFile=episode.idFile AND bookmark.type=1"
      "  LEFT JOIN rating ON"
      "    rating.rating_id=episode.c%02d"
      "  LEFT JOIN uniqueid ON"
      "    uniqueid.uniqueid_id=episode.c%02d",
      VIDEODB_ID_TV_TITLE, VIDEODB_ID_TV_GENRE, VIDEODB_ID_TV_STUDIOS, VIDEODB_ID_TV_PREMIERED,
      VIDEODB_ID_TV_MPAA, VIDEODB_ID_EPISODE_RATING_ID, VIDEODB_ID_EPISODE_IDENT_ID);
  m_pDS->exec(episodeview);

  CLog::Log(LOGINFO, "create tvshowcounts");
  // clang-format off
  const std::string tvshowcounts = PrepareSQL("CREATE VIEW tvshowcounts AS SELECT "
                                       "      tvshow.idShow AS idShow,"
                                       "      MAX(files.lastPlayed) AS lastPlayed,"
                                       "      NULLIF(COUNT(episode.c12), 0) AS totalCount,"
                                       "      COUNT(files.playCount) AS watchedcount,"
                                       "      NULLIF(COUNT(DISTINCT(episode.c12)), 0) AS totalSeasons, "
                                       "      MAX(files.dateAdded) as dateAdded, "
                                       "      COUNT(bookmark.type) AS inProgressCount "
                                       "    FROM tvshow"
                                       "      LEFT JOIN episode ON"
                                       "        episode.idShow=tvshow.idShow"
                                       "      LEFT JOIN files ON"
                                       "        files.idFile=episode.idFile "
                                       "      LEFT JOIN bookmark ON"
                                       "        bookmark.idFile=files.idFile AND bookmark.type=1 "
                                       "GROUP BY tvshow.idShow");
  // clang-format on
  m_pDS->exec(tvshowcounts);

  CLog::Log(LOGINFO, "create tvshowlinkpath_minview");
  // This view only exists to workaround a limitation in MySQL <5.7 which is not able to
  // perform subqueries in joins.
  // Also, the correct solution is to remove the path information altogether, since a
  // TV series can always have multiple paths. It is used in the GUI at the moment, but
  // such usage should be removed together with this view and the path columns in tvshow_view.
  //!@todo Remove the hacky selection of a semi-random path for tvshows from the queries and UI
  const std::string tvshowlinkpathview = PrepareSQL("CREATE VIEW tvshowlinkpath_minview AS SELECT "
                                                    "  idShow, "
                                                    "  min(idPath) AS idPath "
                                                    "FROM tvshowlinkpath "
                                                    "GROUP BY idShow");
  m_pDS->exec(tvshowlinkpathview);

  CLog::Log(LOGINFO, "create tvshow_view");
  // clang-format off
  const std::string tvshowview = PrepareSQL("CREATE VIEW tvshow_view AS SELECT "
                                     "  tvshow.*,"
                                     "  path.idParentPath AS idParentPath,"
                                     "  path.strPath AS strPath,"
                                     "  tvshowcounts.dateAdded AS dateAdded,"
                                     "  lastPlayed, totalCount, watchedcount, totalSeasons, "
                                     "  rating.rating AS rating, "
                                     "  rating.votes AS votes, "
                                     "  rating.rating_type AS rating_type, "
                                     "  uniqueid.value AS uniqueid_value, "
                                     "  uniqueid.type AS uniqueid_type, "
                                     "  tvshowcounts.inProgressCount AS inProgressCount "
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
  // clang-format on
  m_pDS->exec(tvshowview);

  CLog::Log(LOGINFO, "create season_view");
  // clang-format off
  const std::string seasonview = PrepareSQL("CREATE VIEW season_view AS SELECT "
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
                                     "  min(episode.c%02d) AS aired, "
                                     "  count(bookmark.type) AS inProgressCount, "
                                     "  seasons.plot AS seasonPlot "
                                     "FROM seasons"
                                     "  JOIN tvshow_view ON"
                                     "    tvshow_view.idShow = seasons.idShow"
                                     "  JOIN episode ON"
                                     "    episode.idShow = seasons.idShow AND episode.c%02d = seasons.season"
                                     "  JOIN files ON"
                                     "    files.idFile = episode.idFile "
                                     "  LEFT JOIN bookmark ON"
                                     "    bookmark.idFile = files.idFile AND bookmark.type = 1 "
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
                                     "         tvshow_view.c%02d,"
                                     "         seasons.plot ",
                                     VIDEODB_ID_TV_TITLE, VIDEODB_ID_TV_PLOT, VIDEODB_ID_TV_PREMIERED,
                                     VIDEODB_ID_TV_GENRE, VIDEODB_ID_TV_STUDIOS, VIDEODB_ID_TV_MPAA,
                                     VIDEODB_ID_EPISODE_AIRED, VIDEODB_ID_EPISODE_SEASON,
                                     VIDEODB_ID_TV_TITLE, VIDEODB_ID_TV_PLOT, VIDEODB_ID_TV_PREMIERED,
                                     VIDEODB_ID_TV_GENRE, VIDEODB_ID_TV_STUDIOS, VIDEODB_ID_TV_MPAA);
  // clang-format on
  m_pDS->exec(seasonview);

  CLog::Log(LOGINFO, "create musicvideo_view");
  m_pDS->exec(PrepareSQL("CREATE VIEW musicvideo_view AS SELECT"
                         "  musicvideo.*,"
                         "  files.strFileName as strFileName,"
                         "  path.strPath as strPath,"
                         "  files.playCount as playCount,"
                         "  files.lastPlayed as lastPlayed,"
                         "  files.dateAdded as dateAdded, "
                         "  bookmark.timeInSeconds AS resumeTimeInSeconds, "
                         "  bookmark.totalTimeInSeconds AS totalTimeInSeconds, "
                         "  bookmark.playerState AS playerState, "
                         "  uniqueid.value AS uniqueid_value, "
                         "  uniqueid.type AS uniqueid_type "
                         "FROM musicvideo"
                         "  JOIN files ON"
                         "    files.idFile=musicvideo.idFile"
                         "  JOIN path ON"
                         "    path.idPath=files.idPath"
                         "  LEFT JOIN bookmark ON"
                         "    bookmark.idFile=musicvideo.idFile AND bookmark.type=1"
                         "  LEFT JOIN uniqueid ON"
                         "    uniqueid.uniqueid_id=musicvideo.c%02d",
                         VIDEODB_ID_MUSICVIDEO_IDENT_ID));

  CLog::Log(LOGINFO, "create movie_view");

  const std::string movieview = PrepareSQL(
      "CREATE VIEW movie_view AS SELECT"
      "  movie.*,"
      "  sets.strSet AS strSet,"
      "  sets.strOverview AS strSetOverview,"
      "  sets.strOriginalSet as strOriginalSet,"
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
      "  uniqueid.type AS uniqueid_type, "
      "  EXISTS( "
      "    SELECT 1 "
      "    FROM  videoversion vv "
      "    WHERE vv.idMedia = movie.idMovie "
      "    AND   vv.media_type = '%s' "
      "    AND   vv.itemType = %i "
      "    AND   vv.idFile <> movie.idFile "
      "  ) AS hasVideoVersions, "
      "  EXISTS( "
      "    SELECT 1 "
      "    FROM  videoversion vv "
      "    WHERE vv.idMedia = movie.idMovie "
      "    AND   vv.media_type = '%s' "
      "    AND   vv.itemType = %i "
      "  ) AS hasVideoExtras, "
      "  CASE "
      "    WHEN vv.idFile = movie.idFile AND vv.itemType = %i THEN 1 "
      "    ELSE 0 "
      "  END AS isDefaultVersion, "
      "  vv.idFile AS videoVersionIdFile, "
      "  vvt.id AS videoVersionTypeId,"
      "  vvt.name AS videoVersionTypeName,"
      "  vvt.itemType AS videoVersionTypeItemType "
      "FROM movie"
      "  LEFT JOIN sets ON"
      "    sets.idSet = movie.idSet"
      "  LEFT JOIN rating ON"
      "    rating.rating_id = movie.c%02d"
      "  LEFT JOIN uniqueid ON"
      "    uniqueid.uniqueid_id = movie.c%02d"
      "  LEFT JOIN videoversion vv ON"
      "    vv.idMedia = movie.idMovie AND vv.media_type = '%s' "
      "  JOIN videoversiontype vvt ON"
      "    vvt.id = vv.idType AND vvt.itemType = vv.itemType"
      "  JOIN files ON"
      "    files.idFile = vv.idFile"
      "  JOIN path ON"
      "    path.idPath = files.idPath"
      "  LEFT JOIN bookmark ON"
      "    bookmark.idFile = vv.idFile AND bookmark.type = 1",
      MediaTypeMovie, VideoAssetType::VERSION, MediaTypeMovie, VideoAssetType::EXTRA,
      VideoAssetType::VERSION, VIDEODB_ID_RATING_ID, VIDEODB_ID_IDENT_ID, MediaTypeMovie);

  m_pDS->exec(movieview);
}
