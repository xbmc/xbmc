/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

/*
 *  Video Database migration code
 * 
 *  Dependency on external code must be minimized since the database migration code should provide
 *  identical results over time, regardless of changes in the dependencies.
 *  Examples of external changes: functional changes, bug fixes, constants value changes, ...
 */

#include "URL.h"
#include "VideoDatabase.h"
#include "dbwrappers/dataset.h"
#include "filesystem/MultiPathDirectory.h"
#include "guilib/LocalizeStrings.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/i18n/TableLanguageCodes.h"
#include "utils/log.h"

#include <algorithm>
#include <functional>
#include <map>
#include <memory>
#include <sstream>
#include <stdlib.h>
#include <string>
#include <vector>

using namespace dbiplus;
using namespace XFILE;

namespace KODI::DATABASE::MIGRATION
{
void InitializeVideoVersionTypeTableV123(CDatabase& db)
{
  assert(db.InTransaction());

  constexpr int VIDEO_VERSION_ID_BEGIN = 40400;
  constexpr int VIDEO_VERSION_ID_END = 40800;
  constexpr int VideoAssetTypeOwner_SYSTEM = 0;

  // The feature used localized text in the database. Avoiding the dependency on message
  // translation is not practical. Changes in translations will alter the outcome.

  try
  {
    for (int id = VIDEO_VERSION_ID_BEGIN; id <= VIDEO_VERSION_ID_END; ++id)
    {
      const std::string& type{g_localizeStrings.Get(id)};
      db.ExecuteQuery(
          db.PrepareSQL("INSERT INTO videoversiontype (id, name, owner) VALUES(%i, '%s', %i)", id,
                        type.c_str(), VideoAssetTypeOwner_SYSTEM));
    }
  }
  catch (...)
  {
    CLog::LogF(LOGERROR, "failed");
    throw;
  }
}
} // namespace KODI::DATABASE::MIGRATION

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
    std::map<std::string, int, std::less<>> paths;
    m_pDS->query("select idPath,strPath from path");
    while (!m_pDS->eof())
    {
      paths.try_emplace(m_pDS->fv(1).get_asString(), m_pDS->fv(0).get_asInt());
      m_pDS->next();
    }
    m_pDS->close();
    // run through these paths figuring out the parent path, and add to the table if found
    for (const auto& [path, pathId] : paths)
    {
      std::string parent = URIUtils::GetParentPath(path);
      auto j = paths.find(parent);
      if (j != paths.end())
        m_pDS->exec(
            PrepareSQL("UPDATE path SET idParentPath=%i WHERE idPath=%i", j->second, pathId));
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

    class CShowItem
    {
    public:
      bool operator==(const CShowItem& r) const
      {
        return (!ident.empty() && ident == r.ident) || (title == r.title && year == r.year);
      };
      int id;
      int path;
      std::string title;
      std::string year;
      std::string ident;
    };

    constexpr int LOCAL_VIDEODB_ID_TV_TITLE = 0;
    constexpr int LOCAL_VIDEODB_ID_TV_PREMIERED = 5;
    constexpr int LOCAL_VIDEODB_ID_TV_IDENT_ID = 12;

    std::string sql = PrepareSQL(
        "SELECT tvshow.idShow,idPath,c%02d,c%02d,c%02d FROM tvshow JOIN tvshowlinkpath "
        "ON tvshow.idShow = tvshowlinkpath.idShow",
        LOCAL_VIDEODB_ID_TV_TITLE, LOCAL_VIDEODB_ID_TV_PREMIERED, LOCAL_VIDEODB_ID_TV_IDENT_ID);
    m_pDS->query(sql);
    std::vector<CShowItem> shows;
    while (!m_pDS->eof())
    {
      CShowItem show;
      show.id = m_pDS->fv(0).get_asInt();
      show.path = m_pDS->fv(1).get_asInt();
      show.title = m_pDS->fv(2).get_asString();
      show.year = m_pDS->fv(3).get_asString();
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
          m_pDS->exec(
              PrepareSQL("UPDATE tvshowlinkpath SET idShow = %d WHERE idShow = %d AND idPath = %d",
                         j->id, i->id, i->path));
          // update episodes, seasons, movie links
          m_pDS->exec(PrepareSQL("UPDATE episode SET idShow = %d WHERE idShow = %d", j->id, i->id));
          m_pDS->exec(PrepareSQL("UPDATE seasons SET idShow = %d WHERE idShow = %d", j->id, i->id));
          m_pDS->exec(
              PrepareSQL("UPDATE movielinktvshow SET idShow = %d WHERE idShow = %d", j->id, i->id));
          // delete tvshow
          m_pDS->exec(PrepareSQL("DELETE FROM genrelinktvshow WHERE idShow=%i", i->id));
          m_pDS->exec(PrepareSQL("DELETE FROM actorlinktvshow WHERE idShow=%i", i->id));
          m_pDS->exec(PrepareSQL("DELETE FROM directorlinktvshow WHERE idShow=%i", i->id));
          m_pDS->exec(PrepareSQL("DELETE FROM studiolinktvshow WHERE idShow=%i", i->id));
          m_pDS->exec(PrepareSQL("DELETE FROM tvshow WHERE idShow = %d", i->id));
        }
      }
      // cleanup duplicate seasons
      m_pDS->exec("DELETE FROM seasons WHERE idSeason NOT IN (SELECT idSeason FROM (SELECT "
                  "min(idSeason) as idSeason FROM seasons GROUP BY idShow,season) AS sub)");
    }
  }
  if (iVersion < 84)
  {
    // replace any multipaths in tvshowlinkpath table

    class CShowLink
    {
    public:
      int show;
      int pathId;
      std::string path;
    };

    m_pDS->query("SELECT idShow, tvshowlinkpath.idPath, strPath FROM tvshowlinkpath JOIN path ON "
                 "tvshowlinkpath.idPath=path.idPath WHERE path.strPath LIKE 'multipath://%'");
    std::vector<CShowLink> shows;
    while (!m_pDS->eof())
    {
      CShowLink link;
      link.show = m_pDS->fv(0).get_asInt();
      link.pathId = m_pDS->fv(1).get_asInt();
      link.path = m_pDS->fv(2).get_asString();
      shows.emplace_back(std::move(link));
      m_pDS->next();
    }
    m_pDS->close();
    // update these
    for (const auto& show : shows)
    {
      std::vector<std::string> paths;
      CMultiPathDirectory::GetPaths(show.path, paths);
      for (const auto& path : paths)
      {
        const int idPath = AddPath(path, URIUtils::GetParentPath(path));
        /* we can't rely on REPLACE INTO here as analytics (indices) aren't online yet */
        if (GetSingleValue(PrepareSQL("SELECT 1 FROM tvshowlinkpath WHERE idShow=%i AND idPath=%i",
                                      show.show, idPath))
                .empty())
          m_pDS->exec(PrepareSQL("INSERT INTO tvshowlinkpath(idShow, idPath) VALUES(%i,%i)",
                                 show.show, idPath));
      }
      m_pDS->exec(PrepareSQL("DELETE FROM tvshowlinkpath WHERE idShow=%i AND idPath=%i", show.show,
                             show.pathId));
    }
  }
  if (iVersion < 85)
  {
    // drop multipaths from the path table - they're not needed for anything at all
    m_pDS->exec("DELETE FROM path WHERE strPath LIKE 'multipath://%'");
  }
  if (iVersion < 87)
  { // due to the tvshow merging above, there could be orphaned season or show art
    m_pDS->exec("DELETE from art WHERE media_type='tvshow' AND NOT EXISTS (SELECT 1 FROM tvshow "
                "WHERE tvshow.idShow = art.media_id)");
    m_pDS->exec("DELETE from art WHERE media_type='season' AND NOT EXISTS (SELECT 1 FROM seasons "
                "WHERE seasons.idSeason = art.media_id)");
  }
  if (iVersion < 91)
  {
    // create actor link table
    m_pDS->exec("CREATE TABLE actor_link(actor_id INT, media_id INT, media_type TEXT, role TEXT, "
                "cast_order INT)");
    m_pDS->exec("INSERT INTO actor_link(actor_id, media_id, media_type, role, cast_order) SELECT "
                "DISTINCT idActor, idMovie, 'movie', strRole, iOrder from actorlinkmovie");
    m_pDS->exec("INSERT INTO actor_link(actor_id, media_id, media_type, role, cast_order) SELECT "
                "DISTINCT idActor, idShow, 'tvshow', strRole, iOrder from actorlinktvshow");
    m_pDS->exec("INSERT INTO actor_link(actor_id, media_id, media_type, role, cast_order) SELECT "
                "DISTINCT idActor, idEpisode, 'episode', strRole, iOrder from actorlinkepisode");
    m_pDS->exec("DROP TABLE IF EXISTS actorlinkmovie");
    m_pDS->exec("DROP TABLE IF EXISTS actorlinktvshow");
    m_pDS->exec("DROP TABLE IF EXISTS actorlinkepisode");
    m_pDS->exec("CREATE TABLE actor(actor_id INTEGER PRIMARY KEY, name TEXT, art_urls TEXT)");
    m_pDS->exec(
        "INSERT INTO actor(actor_id, name, art_urls) SELECT idActor,strActor,strThumb FROM actors");
    m_pDS->exec("DROP TABLE IF EXISTS actors");

    // directors
    m_pDS->exec("CREATE TABLE director_link(actor_id INTEGER, media_id INTEGER, media_type TEXT)");
    m_pDS->exec("INSERT INTO director_link(actor_id, media_id, media_type) SELECT DISTINCT "
                "idDirector, idMovie, 'movie' FROM directorlinkmovie");
    m_pDS->exec("INSERT INTO director_link(actor_id, media_id, media_type) SELECT DISTINCT "
                "idDirector, idShow, 'tvshow' FROM directorlinktvshow");
    m_pDS->exec("INSERT INTO director_link(actor_id, media_id, media_type) SELECT DISTINCT "
                "idDirector, idEpisode, 'episode' FROM directorlinkepisode");
    m_pDS->exec("INSERT INTO director_link(actor_id, media_id, media_type) SELECT DISTINCT "
                "idDirector, idMVideo, 'musicvideo' FROM directorlinkmusicvideo");
    m_pDS->exec("DROP TABLE IF EXISTS directorlinkmovie");
    m_pDS->exec("DROP TABLE IF EXISTS directorlinktvshow");
    m_pDS->exec("DROP TABLE IF EXISTS directorlinkepisode");
    m_pDS->exec("DROP TABLE IF EXISTS directorlinkmusicvideo");

    // writers
    m_pDS->exec("CREATE TABLE writer_link(actor_id INTEGER, media_id INTEGER, media_type TEXT)");
    m_pDS->exec("INSERT INTO writer_link(actor_id, media_id, media_type) SELECT DISTINCT idWriter, "
                "idMovie, 'movie' FROM writerlinkmovie");
    m_pDS->exec("INSERT INTO writer_link(actor_id, media_id, media_type) SELECT DISTINCT idWriter, "
                "idEpisode, 'episode' FROM writerlinkepisode");
    m_pDS->exec("DROP TABLE IF EXISTS writerlinkmovie");
    m_pDS->exec("DROP TABLE IF EXISTS writerlinkepisode");

    // music artist
    m_pDS->exec("INSERT INTO actor_link(actor_id, media_id, media_type) SELECT DISTINCT idArtist, "
                "idMVideo, 'musicvideo' FROM artistlinkmusicvideo");
    m_pDS->exec("DROP TABLE IF EXISTS artistlinkmusicvideo");

    // studios
    m_pDS->exec("CREATE TABLE studio_link(studio_id INTEGER, media_id INTEGER, media_type TEXT)");
    m_pDS->exec("INSERT INTO studio_link(studio_id, media_id, media_type) SELECT DISTINCT "
                "idStudio, idMovie, 'movie' FROM studiolinkmovie");
    m_pDS->exec("INSERT INTO studio_link(studio_id, media_id, media_type) SELECT DISTINCT "
                "idStudio, idShow, 'tvshow' FROM studiolinktvshow");
    m_pDS->exec("INSERT INTO studio_link(studio_id, media_id, media_type) SELECT DISTINCT "
                "idStudio, idMVideo, 'musicvideo' FROM studiolinkmusicvideo");
    m_pDS->exec("DROP TABLE IF EXISTS studiolinkmovie");
    m_pDS->exec("DROP TABLE IF EXISTS studiolinktvshow");
    m_pDS->exec("DROP TABLE IF EXISTS studiolinkmusicvideo");
    m_pDS->exec("CREATE TABLE studionew(studio_id INTEGER PRIMARY KEY, name TEXT)");
    m_pDS->exec("INSERT INTO studionew(studio_id, name) SELECT idStudio,strStudio FROM studio");
    m_pDS->exec("DROP TABLE IF EXISTS studio");
    m_pDS->exec("ALTER TABLE studionew RENAME TO studio");

    // genres
    m_pDS->exec("CREATE TABLE genre_link(genre_id INTEGER, media_id INTEGER, media_type TEXT)");
    m_pDS->exec("INSERT INTO genre_link(genre_id, media_id, media_type) SELECT DISTINCT idGenre, "
                "idMovie, 'movie' FROM genrelinkmovie");
    m_pDS->exec("INSERT INTO genre_link(genre_id, media_id, media_type) SELECT DISTINCT idGenre, "
                "idShow, 'tvshow' FROM genrelinktvshow");
    m_pDS->exec("INSERT INTO genre_link(genre_id, media_id, media_type) SELECT DISTINCT idGenre, "
                "idMVideo, 'musicvideo' FROM genrelinkmusicvideo");
    m_pDS->exec("DROP TABLE IF EXISTS genrelinkmovie");
    m_pDS->exec("DROP TABLE IF EXISTS genrelinktvshow");
    m_pDS->exec("DROP TABLE IF EXISTS genrelinkmusicvideo");
    m_pDS->exec("CREATE TABLE genrenew(genre_id INTEGER PRIMARY KEY, name TEXT)");
    m_pDS->exec("INSERT INTO genrenew(genre_id, name) SELECT idGenre,strGenre FROM genre");
    m_pDS->exec("DROP TABLE IF EXISTS genre");
    m_pDS->exec("ALTER TABLE genrenew RENAME TO genre");

    // country
    m_pDS->exec("CREATE TABLE country_link(country_id INTEGER, media_id INTEGER, media_type TEXT)");
    m_pDS->exec("INSERT INTO country_link(country_id, media_id, media_type) SELECT DISTINCT "
                "idCountry, idMovie, 'movie' FROM countrylinkmovie");
    m_pDS->exec("DROP TABLE IF EXISTS countrylinkmovie");
    m_pDS->exec("CREATE TABLE countrynew(country_id INTEGER PRIMARY KEY, name TEXT)");
    m_pDS->exec(
        "INSERT INTO countrynew(country_id, name) SELECT idCountry,strCountry FROM country");
    m_pDS->exec("DROP TABLE IF EXISTS country");
    m_pDS->exec("ALTER TABLE countrynew RENAME TO country");

    // tags
    m_pDS->exec("CREATE TABLE tag_link(tag_id INTEGER, media_id INTEGER, media_type TEXT)");
    m_pDS->exec("INSERT INTO tag_link(tag_id, media_id, media_type) SELECT DISTINCT idTag, "
                "idMedia, media_type FROM taglinks");
    m_pDS->exec("DROP TABLE IF EXISTS taglinks");
    m_pDS->exec("CREATE TABLE tagnew(tag_id INTEGER PRIMARY KEY, name TEXT)");
    m_pDS->exec("INSERT INTO tagnew(tag_id, name) SELECT idTag,strTag FROM tag");
    m_pDS->exec("DROP TABLE IF EXISTS tag");
    m_pDS->exec("ALTER TABLE tagnew RENAME TO tag");
  }

  if (iVersion < 93)
  {
    // cleanup main tables
    constexpr int LOCAL_VIDEODB_MAX_COLUMNS = 24;

    std::string valuesSql;
    for (int i = 0; i < LOCAL_VIDEODB_MAX_COLUMNS; i++)
    {
      valuesSql += StringUtils::Format("c{:02} = TRIM(c{:02})", i, i);
      if (i < LOCAL_VIDEODB_MAX_COLUMNS - 1)
        valuesSql += ",";
    }
    m_pDS->exec("UPDATE episode SET " + valuesSql);
    m_pDS->exec("UPDATE movie SET " + valuesSql);
    m_pDS->exec("UPDATE musicvideo SET " + valuesSql);
    m_pDS->exec("UPDATE tvshow SET " + valuesSql);

    // cleanup additional tables
    static const std::map<std::string, std::vector<std::string>, std::less<>> additionalTablesMap =
        {{"actor", {"actor_link", "director_link", "writer_link"}},
         {"studio", {"studio_link"}},
         {"genre", {"genre_link"}},
         {"country", {"country_link"}},
         {"tag", {"tag_link"}}};
    for (const auto& [table, links] : additionalTablesMap)
    {
      std::string tablePk = table + "_id";
      std::map<int, std::string> duplicatesMinMap;
      std::map<int, std::string> duplicatesMap;

      // cleanup name
      m_pDS->exec(PrepareSQL("UPDATE %s SET name = TRIM(name)", table.c_str()));

      // shrink name to length 255
      m_pDS->exec(PrepareSQL("UPDATE %s SET name = SUBSTR(name, 1, 255) WHERE LENGTH(name) > 255",
                             table.c_str()));

      // fetch main entries
      m_pDS->query(PrepareSQL("SELECT MIN(%s), name FROM %s GROUP BY name HAVING COUNT(1) > 1",
                              tablePk.c_str(), table.c_str()));

      while (!m_pDS->eof())
      {
        duplicatesMinMap.try_emplace(m_pDS->fv(0).get_asInt(), m_pDS->fv(1).get_asString());
        m_pDS->next();
      }
      m_pDS->close();

      // fetch duplicate entries
      for (const auto& [min, name] : duplicatesMinMap)
      {
        m_pDS->query(PrepareSQL("SELECT %s FROM %s WHERE name = '%s' AND %s <> %i", tablePk.c_str(),
                                table.c_str(), name.c_str(), tablePk.c_str(), min));

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

        duplicatesMap.try_emplace(min, ids.str());
      }

      // cleanup duplicates in link tables
      for (const auto& subTable : links)
      {
        // create indexes to speed up things
        m_pDS->exec(PrepareSQL("CREATE INDEX ix_%s ON %s (%s)", subTable.c_str(), subTable.c_str(),
                               tablePk.c_str()));

        // migrate every duplicate entry to the main entry
        for (const auto& [min, name] : duplicatesMap)
        {
          m_pDS->exec(PrepareSQL("UPDATE %s SET %s = %i WHERE %s IN (%s) ", subTable.c_str(),
                                 tablePk.c_str(), min, tablePk.c_str(), name.c_str()));
        }

        // clear all duplicates in the link tables
        if (subTable == "actor_link")
        {
          // as a distinct won't work because of role and cast_order and a group by kills a
          // low powered mysql, we de-dupe it with REPLACE INTO while using the real unique index
          m_pDS->exec("CREATE TABLE temp_actor_link(actor_id INT, media_id INT, media_type TEXT, "
                      "role TEXT, cast_order INT)");
          m_pDS->exec("CREATE UNIQUE INDEX ix_temp_actor_link ON temp_actor_link (actor_id, "
                      "media_type(20), media_id)");
          m_pDS->exec("REPLACE INTO temp_actor_link SELECT * FROM actor_link");
          m_pDS->exec("DROP INDEX ix_temp_actor_link ON temp_actor_link");
        }
        else
        {
          m_pDS->exec(PrepareSQL("CREATE TABLE temp_%s AS SELECT DISTINCT * FROM %s",
                                 subTable.c_str(), subTable.c_str()));
        }

        m_pDS->exec(PrepareSQL("DROP TABLE IF EXISTS %s", subTable.c_str()));

        m_pDS->exec(
            PrepareSQL("ALTER TABLE temp_%s RENAME TO %s", subTable.c_str(), subTable.c_str()));
      }

      // delete duplicates in main table
      for (const auto& [_, name] : duplicatesMap)
      {
        m_pDS->exec(PrepareSQL("DELETE FROM %s WHERE %s IN (%s)", table.c_str(), tablePk.c_str(),
                               name.c_str()));
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
    constexpr int LOCAL_VIDEODB_ID_EPISODE_SEASON = 12;

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
                              LOCAL_VIDEODB_ID_EPISODE_SEASON, m_pDS->fv(2).get_asInt()));

      m_pDS->next();
    }
    m_pDS->close();
  }
  if (iVersion < 101)
    m_pDS->exec("ALTER TABLE seasons ADD userrating INTEGER");

  if (iVersion < 102)
  {
    constexpr int LOCAL_VIDEODB_ID_EPISODE_VOTES = 2;
    constexpr int LOCAL_VIDEODB_ID_EPISODE_RATING_ID = 3;
    constexpr int LOCAL_VIDEODB_ID_VOTES = 4;
    constexpr int LOCAL_VIDEODB_ID_RATING_ID = 5;
    constexpr int LOCAL_VIDEODB_ID_TV_VOTES = 3;
    constexpr int LOCAL_VIDEODB_ID_TV_RATING_ID = 4;

    m_pDS->exec("CREATE TABLE rating (rating_id INTEGER PRIMARY KEY, media_id INTEGER, media_type "
                "TEXT, rating_type TEXT, rating FLOAT, votes INTEGER)");

    std::string sql = PrepareSQL("SELECT DISTINCT idMovie, c%02d, c%02d FROM movie",
                                 LOCAL_VIDEODB_ID_RATING_ID, LOCAL_VIDEODB_ID_VOTES);
    m_pDS->query(sql);
    while (!m_pDS->eof())
    {
      m_pDS2->exec(PrepareSQL("INSERT INTO rating(media_id, media_type, rating_type, rating, "
                              "votes) VALUES (%i, 'movie', 'default', %f, %i)",
                              m_pDS->fv(0).get_asInt(),
                              std::strtod(m_pDS->fv(1).get_asString().c_str(), nullptr),
                              StringUtils::ReturnDigits(m_pDS->fv(2).get_asString())));
      const auto idRating = static_cast<int>(m_pDS2->lastinsertid());
      m_pDS2->exec(PrepareSQL("UPDATE movie SET c%02d=%i WHERE idMovie=%i",
                              LOCAL_VIDEODB_ID_RATING_ID, idRating, m_pDS->fv(0).get_asInt()));
      m_pDS->next();
    }
    m_pDS->close();

    sql = PrepareSQL("SELECT DISTINCT idShow, c%02d, c%02d FROM tvshow",
                     LOCAL_VIDEODB_ID_TV_RATING_ID, LOCAL_VIDEODB_ID_TV_VOTES);
    m_pDS->query(sql);
    while (!m_pDS->eof())
    {
      m_pDS2->exec(PrepareSQL("INSERT INTO rating(media_id, media_type, rating_type, rating, "
                              "votes) VALUES (%i, 'tvshow', 'default', %f, %i)",
                              m_pDS->fv(0).get_asInt(),
                              std::strtod(m_pDS->fv(1).get_asString().c_str(), nullptr),
                              StringUtils::ReturnDigits(m_pDS->fv(2).get_asString())));
      const auto idRating = static_cast<int>(m_pDS2->lastinsertid());
      m_pDS2->exec(PrepareSQL("UPDATE tvshow SET c%02d=%i WHERE idShow=%i",
                              LOCAL_VIDEODB_ID_TV_RATING_ID, idRating, m_pDS->fv(0).get_asInt()));
      m_pDS->next();
    }
    m_pDS->close();

    sql = PrepareSQL("SELECT DISTINCT idEpisode, c%02d, c%02d FROM episode",
                     LOCAL_VIDEODB_ID_EPISODE_RATING_ID, LOCAL_VIDEODB_ID_EPISODE_VOTES);
    m_pDS->query(sql);
    while (!m_pDS->eof())
    {
      m_pDS2->exec(PrepareSQL("INSERT INTO rating(media_id, media_type, rating_type, rating, "
                              "votes) VALUES (%i, 'episode', 'default', %f, %i)",
                              m_pDS->fv(0).get_asInt(),
                              std::strtod(m_pDS->fv(1).get_asString().c_str(), nullptr),
                              StringUtils::ReturnDigits(m_pDS->fv(2).get_asString())));
      const auto idRating = static_cast<int>(m_pDS2->lastinsertid());
      m_pDS2->exec(PrepareSQL("UPDATE episode SET c%02d=%i WHERE idEpisode=%i",
                              LOCAL_VIDEODB_ID_EPISODE_RATING_ID, idRating,
                              m_pDS->fv(0).get_asInt()));
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
    constexpr int LOCAL_VIDEODB_ID_EPISODE_RUNTIME = 9;

    m_pDS->exec("ALTER TABLE tvshow ADD duration INTEGER");

    std::string sql =
        PrepareSQL("SELECT episode.idShow, MAX(episode.c%02d) "
                   "FROM episode "

                   "LEFT JOIN streamdetails "
                   "ON streamdetails.idFile = episode.idFile "
                   "AND streamdetails.iStreamType = 0 " // only grab video streams

                   "WHERE episode.c%02d <> streamdetails.iVideoDuration "
                   "OR streamdetails.iVideoDuration IS NULL "
                   "GROUP BY episode.idShow",
                   LOCAL_VIDEODB_ID_EPISODE_RUNTIME, LOCAL_VIDEODB_ID_EPISODE_RUNTIME);

    m_pDS->query(sql);
    while (!m_pDS->eof())
    {
      m_pDS2->exec(PrepareSQL("UPDATE tvshow SET duration=%i WHERE idShow=%i",
                              m_pDS->fv(1).get_asInt(), m_pDS->fv(0).get_asInt()));
      m_pDS->next();
    }
    m_pDS->close();
  }

  if (iVersion < 105)
  {
    constexpr int LOCAL_VIDEODB_ID_YEAR = 7;
    constexpr int LOCAL_VIDEODB_ID_MUSICVIDEO_YEAR = 7;

    m_pDS->exec("ALTER TABLE movie ADD premiered TEXT");
    m_pDS->exec(PrepareSQL("UPDATE movie SET premiered=c%02d", LOCAL_VIDEODB_ID_YEAR));
    m_pDS->exec("ALTER TABLE musicvideo ADD premiered TEXT");
    m_pDS->exec(
        PrepareSQL("UPDATE musicvideo SET premiered=c%02d", LOCAL_VIDEODB_ID_MUSICVIDEO_YEAR));
  }

  if (iVersion < 107)
  {
    constexpr int LOCAL_VIDEODB_ID_IDENT_ID = 9;
    constexpr int LOCAL_VIDEODB_ID_TV_IDENT_ID = 12;
    constexpr int LOCAL_VIDEODB_ID_EPISODE_IDENT_ID = 20;

    // need this due to the nested GetScraperPath query
    std::unique_ptr<Dataset> pDS;
    pDS.reset(m_pDB->CreateDataset());
    if (nullptr == pDS)
      return;

    pDS->exec("CREATE TABLE uniqueid (uniqueid_id INTEGER PRIMARY KEY, media_id INTEGER, "
              "media_type TEXT, value TEXT, type TEXT)");

    for (int i = 0; i < 3; ++i)
    {
      std::string mediatype;
      std::string columnID;
      int columnUniqueID;
      switch (i)
      {
        case 0:
          mediatype = "movie";
          columnID = "idMovie";
          columnUniqueID = LOCAL_VIDEODB_ID_IDENT_ID;
          break;
        case 1:
          mediatype = "tvshow";
          columnID = "idShow";
          columnUniqueID = LOCAL_VIDEODB_ID_TV_IDENT_ID;
          break;
        case 2:
          mediatype = "episode";
          columnID = "idEpisode";
          columnUniqueID = LOCAL_VIDEODB_ID_EPISODE_IDENT_ID;
          break;
        default:
          continue;
      }
      pDS->query(PrepareSQL("SELECT %s, c%02d FROM %s", columnID.c_str(), columnUniqueID,
                            mediatype.c_str()));
      while (!pDS->eof())
      {
        std::string uniqueid = pDS->fv(1).get_asString();
        if (!uniqueid.empty())
        {
          int mediaid = pDS->fv(0).get_asInt();
          if (StringUtils::StartsWith(uniqueid, "tt"))
            m_pDS2->exec(PrepareSQL("INSERT INTO uniqueid(media_id, media_type, type, value) "
                                    "VALUES (%i, '%s', 'imdb', '%s')",
                                    mediaid, mediatype.c_str(), uniqueid.c_str()));
          else
            m_pDS2->exec(PrepareSQL("INSERT INTO uniqueid(media_id, media_type, type, value) "
                                    "VALUES (%i, '%s', 'unknown', '%s')",
                                    mediaid, mediatype.c_str(), uniqueid.c_str()));
          m_pDS2->exec(PrepareSQL("UPDATE %s SET c%02d='%i' WHERE %s=%i", mediatype.c_str(),
                                  columnUniqueID, static_cast<int>(m_pDS2->lastinsertid()),
                                  columnID.c_str(), mediaid));
        }
        pDS->next();
      }
      pDS->close();
    }
  }

  if (iVersion < 109)
  {
    m_pDS->exec("ALTER TABLE settings RENAME TO settingsold");
    m_pDS->exec(
        "CREATE TABLE settings ( idFile integer, Deinterlace bool,"
        "ViewMode integer,ZoomAmount float, PixelRatio float, VerticalShift float, AudioStream "
        "integer, SubtitleStream integer,"
        "SubtitleDelay float, SubtitlesOn bool, Brightness float, Contrast float, Gamma float,"
        "VolumeAmplification float, AudioDelay float, ResumeTime integer,"
        "Sharpness float, NoiseReduction float, NonLinStretch bool, PostProcess bool,"
        "ScalingMethod integer, DeinterlaceMode integer, StereoMode integer, StereoInvert bool, "
        "VideoStream integer)");
    m_pDS->exec("INSERT INTO settings SELECT idFile, Deinterlace, ViewMode, ZoomAmount, "
                "PixelRatio, VerticalShift, AudioStream, SubtitleStream, SubtitleDelay, "
                "SubtitlesOn, Brightness, Contrast, Gamma, VolumeAmplification, AudioDelay, "
                "ResumeTime, Sharpness, NoiseReduction, NonLinStretch, PostProcess, ScalingMethod, "
                "DeinterlaceMode, StereoMode, StereoInvert, VideoStream FROM settingsold");
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
    m_pDS->query("SELECT files.idFile, files.strFilename, path.strPath FROM files LEFT JOIN path "
                 "ON files.idPath = path.idPath WHERE files.strFilename LIKE 'plugin://%'");
    while (!m_pDS->eof())
    {
      std::string path;
      std::string fn;
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
            parentid = static_cast<int>(m_pDS2->lastinsertid());
          }
          m_pDS2->exec(PrepareSQL("INSERT INTO path (strPath, idParentPath) VALUES ('%s', %i)",
                                  path.c_str(), parentid));
          pathid = static_cast<int>(m_pDS2->lastinsertid());
        }
        m_pDS2->query(PrepareSQL("SELECT idFile FROM files WHERE strFileName='%s' AND idPath=%i",
                                 fn.c_str(), pathid));
        bool exists = !m_pDS2->eof();
        m_pDS2->close();
        if (exists)
          m_pDS2->exec(PrepareSQL("DELETE FROM files WHERE idFile=%i", m_pDS->fv(0).get_asInt()));
        else
          m_pDS2->exec(PrepareSQL("UPDATE files SET idPath=%i WHERE idFile=%i", pathid,
                                  m_pDS->fv(0).get_asInt()));
      }
      m_pDS->next();
    }
    m_pDS->close();
  }

  if (iVersion < 119)
    m_pDS->exec("ALTER TABLE path ADD allAudio bool");

  if (iVersion < 120)
    m_pDS->exec("ALTER TABLE streamdetails ADD strHdrType text");

  if (iVersion < 121)
  {
    // https://github.com/xbmc/xbmc/issues/21253 - Kodi picks up wrong "year" for PVR recording.

    m_pDS->query("SELECT idFile, strFilename FROM files WHERE strFilename LIKE '% (1969)%.pvr' OR "
                 "strFilename LIKE '% (1601)%.pvr'");
    while (!m_pDS->eof())
    {
      std::string fixedFileName = m_pDS->fv(1).get_asString();
      size_t pos = fixedFileName.find(" (1969)");
      if (pos == std::string::npos)
        pos = fixedFileName.find(" (1601)");

      if (pos != std::string::npos)
      {
        fixedFileName.erase(pos, 7);

        m_pDS2->exec(PrepareSQL("UPDATE files SET strFilename='%s' WHERE idFile=%i",
                                fixedFileName.c_str(), m_pDS->fv(0).get_asInt()));
      }
      m_pDS->next();
    }
    m_pDS->close();
  }

  if (iVersion < 123)
  {
    constexpr int VideoAssetType_VERSION = 1;
    constexpr int VIDEO_VERSION_ID_DEFAULT = 40400;

    // create videoversiontype table
    m_pDS->exec("CREATE TABLE videoversiontype (id INTEGER PRIMARY KEY, name TEXT, owner INTEGER)");
    KODI::DATABASE::MIGRATION::InitializeVideoVersionTypeTableV123(*this);

    // create videoversion table
    m_pDS->exec("CREATE TABLE videoversion (idFile INTEGER PRIMARY KEY, idMedia INTEGER, mediaType "
                "TEXT, itemType INTEGER, idType INTEGER)");
    m_pDS->exec(PrepareSQL(
        "INSERT INTO videoversion SELECT idFile, idMovie, 'movie', '%i', '%i' FROM movie",
        VideoAssetType_VERSION, VIDEO_VERSION_ID_DEFAULT));
  }

  if (iVersion < 127)
  {
    constexpr int VideoAssetType_VERSION = 1;
    constexpr int VideoAssetType_EXTRA = 2;
    constexpr int VideoAssetTypeOwner_USER = 2;
    constexpr int VIDEO_VERSION_ID_END = 40800;

    m_pDS->exec("ALTER TABLE videoversiontype ADD itemType INTEGER");

    // First, assume all types are video version types
    m_pDS->exec(PrepareSQL("UPDATE videoversiontype SET itemType = %i", VideoAssetType_VERSION));

    // Then, check current extras entries and their assigned item type and migrate it

    // get all assets with extras item type
    m_pDS->query("SELECT DISTINCT idType FROM videoversion WHERE itemType = 1");
    while (!m_pDS->eof())
    {
      const int idType{m_pDS->fv(0).get_asInt()};
      if (idType > VIDEO_VERSION_ID_END)
      {
        // user-added type for extras. change its item type to extras
        m_pDS2->exec(PrepareSQL("UPDATE videoversiontype SET itemType = %i WHERE id = %i",
                                VideoAssetType_EXTRA, idType));
      }
      else
      {
        // system type used for an extra. copy as extras item type.
        m_pDS2->query(
            PrepareSQL("SELECT itemType, name FROM videoversiontype WHERE id = %i", idType));
        if (m_pDS2->fv(0).get_asInt() == 0)
        {
          // currently a versions type, create a corresponding user-added type for extras
          m_pDS2->exec(PrepareSQL(
              "INSERT INTO videoversiontype (id, name, owner, itemType) VALUES(NULL, '%s', %i, %i)",
              m_pDS2->fv(1).get_asString().c_str(), VideoAssetTypeOwner_USER,
              VideoAssetType_EXTRA));

          // update the respective extras to use the new extras type
          const int newId{static_cast<int>(m_pDS2->lastinsertid())};
          m_pDS2->exec(
              PrepareSQL("UPDATE videoversion SET idType = %i WHERE itemType = 1 AND idType = %i",
                         newId, idType));
        }
        m_pDS2->close();
      }
      m_pDS->next();
    }
    m_pDS->close();
  }

  if (iVersion < 128)
  {
    constexpr int VideoAssetType_VERSION = 1;
    constexpr int VideoAssetTypeOwner_USER = 2;

    m_pDS->exec("CREATE TABLE videoversion_new "
                "(idFile INTEGER PRIMARY KEY, idMedia INTEGER, media_type TEXT, "
                " itemType INTEGER, idType INTEGER)");
    m_pDS->exec("INSERT INTO videoversion_new "
                " (idFile, idMedia, media_type, itemType, idType) "
                "SELECT idFile, idMedia, mediaType, itemType, idType FROM videoversion");
    m_pDS->exec("DROP TABLE videoversion");
    m_pDS->exec("ALTER TABLE videoversion_new RENAME TO videoversion");

    // Fix gap in the migration to videodb v127 for unused user-defined video version types.
    // Unfortunately due to original design we cannot tell which ones were movie versions or
    // extras and now they're all displayed in the version type selection for movies.
    // Remove them all as the better fix of providing a GUI to manage version types will not be
    // available in Omega v21. That implies the loss of the unused user-defined version names
    // created since v21 beta 2.
    m_pDS2->exec(PrepareSQL("DELETE FROM videoversiontype "
                            "WHERE id NOT IN (SELECT idType FROM videoversion) "
                            "AND owner = %i "
                            "AND itemType = %i",
                            VideoAssetTypeOwner_USER, VideoAssetType_VERSION));
  }

  if (iVersion < 131)
  {
    constexpr int VideoAssetType_VERSION = 1;
    constexpr int VideoAssetTypeOwner_USER = 2;

    // Remove quality-like predefined version types

    // Retrieve current utilization per type
    m_pDS->query("SELECT vvt.id, vvt.name, count(vv.idType) "
                 "FROM videoversiontype vvt "
                 "  LEFT JOIN videoversion vv ON vvt.id = vv.idType "
                 "WHERE vvt.id = 40405 OR vvt.id BETWEEN 40418 AND 40430 "
                 "GROUP BY vvt.id");

    while (!m_pDS->eof())
    {
      const int typeId{m_pDS->fv(0).get_asInt()};
      const std::string typeName{m_pDS->fv(1).get_asString()};
      const int versionsCount{m_pDS->fv(2).get_asInt()};

      if (versionsCount > 0)
      {
        // type used by some versions, recreate as user type and link the versions to the new id
        m_pDS2->exec(PrepareSQL(
            "INSERT INTO videoversiontype (id, name, owner, itemType) VALUES(NULL, '%s', %i, %i)",
            typeName.c_str(), VideoAssetTypeOwner_USER, VideoAssetType_VERSION));

        const int newId{static_cast<int>(m_pDS2->lastinsertid())};

        m_pDS2->exec(
            PrepareSQL("UPDATE videoversion SET idType = %i WHERE idType = %i", newId, typeId));
      }
      m_pDS2->exec(PrepareSQL("DELETE FROM videoversiontype WHERE id = %i", typeId));
      m_pDS->next();
    }
    m_pDS->close();
  }

  if (iVersion < 133)
  {
    // Remove episodes with invalid idSeason values.
    // Since 2015 they were masked from episode_view and are not going to be missed.
    // Those records would be misses in database converted in 2015 (see videodb version 99).

    m_pDS->exec("DELETE FROM episode WHERE idSeason NOT IN (SELECT idSeason from seasons)");
  }

  if (iVersion < 134)
  {
    // renumber itemType to free the value 0 for nodes navigation
    // former value 0 for versions becomes 1
    // former value 1 for extras becomes 2

    constexpr int VIDEOASSETTYPE_VERSION_OLD{0};
    constexpr int VIDEOASSETTYPE_EXTRA_OLD{1};

    m_pDS->query(
        PrepareSQL("SELECT itemType FROM videoversion WHERE itemType NOT IN (%i, %i) UNION ALL "
                   "SELECT itemType FROM videoversiontype WHERE itemType NOT IN (%i, %i) "
                   "LIMIT 1 ",
                   VIDEOASSETTYPE_VERSION_OLD, VIDEOASSETTYPE_EXTRA_OLD, VIDEOASSETTYPE_VERSION_OLD,
                   VIDEOASSETTYPE_EXTRA_OLD));
    if (!m_pDS->eof())
    {
      CLog::LogF(LOGERROR, "invalid itemType values in videoversion or videoversiontype");
      m_pDS->close();
    }

    m_pDS->exec(
        PrepareSQL("UPDATE videoversion SET itemType = itemType + 1  WHERE itemType IN (%i, %i)",
                   VIDEOASSETTYPE_VERSION_OLD, VIDEOASSETTYPE_EXTRA_OLD));
    m_pDS->exec(PrepareSQL(
        "UPDATE videoversiontype SET itemType = itemType + 1  WHERE itemType IN (%i, %i)",
        VIDEOASSETTYPE_VERSION_OLD, VIDEOASSETTYPE_EXTRA_OLD));
  }

  if (iVersion < 135)
  {
    // Fix PVR recording path encoding bug.
    m_pDS->query("SELECT idPath, strPath FROM path WHERE strPath LIKE 'pvr://recordings/%'");
    while (!m_pDS->eof())
    {
      std::string fixedPath{m_pDS->fv(1).get_asString()};
      fixedPath.erase(0, 17); // Omit "pvr://recordings/".

      // Fix special case where ? contained in directory names was treated as URL parameter.
      const size_t pos{fixedPath.find("/?")};
      if (pos != std::string::npos)
        fixedPath.erase(pos, 1);

      std::vector<std::string> segments{StringUtils::Split(fixedPath, "/")};
      for (auto& segment : segments)
        segment = CURL::Encode(segment);

      fixedPath = "pvr://recordings/";
      fixedPath += StringUtils::Join(segments, "/");

      m_pDS2->exec(PrepareSQL("UPDATE path SET strPath='%s' WHERE idPath=%i", fixedPath.c_str(),
                              m_pDS->fv(0).get_asInt()));
      m_pDS->next();
    }
    m_pDS->close();
  }

  if (iVersion < 136)
  {
    m_pDS->exec("ALTER TABLE sets ADD strOriginalSet TEXT");

    // Copy current set title for existing sets
    m_pDS->exec("UPDATE sets SET strOriginalSet = strSet");
  }

  if (iVersion < 138)
  {
    m_pDS->exec("ALTER TABLE seasons ADD plot TEXT");
  }

  if (iVersion < 139)
  {
    m_pDS->exec("ALTER TABLE movie ADD originalLanguage TEXT");
    m_pDS->exec("ALTER TABLE tvshow ADD originalLanguage TEXT");
  }

  if (iVersion < 140)
  {
    m_pDS->exec("ALTER TABLE tvshow ADD tagLine TEXT");
  }

  if (iVersion < 141)
  {
    // Convert the original language from ISO 639-2/B to BCP-47

    m_pDS->query(
        PrepareSQL("SELECT DISTINCT originalLanguage FROM movie WHERE originalLanguage <> '' "
                   "UNION "
                   "SELECT DISTINCT originalLanguage FROM tvshow WHERE originalLanguage <> ''"));
    while (!m_pDS->eof())
    {
      std::string from = m_pDS->fv(0).get_asString();

      // The current value is an ISO 639-2/B code or a code defined in AS.xml or a language Addon.
      // Convert possible ISO 639-2/B to ISO 639-1 with minimal dependencies on external code
      // The list of ISO 639-1 codes (and their mapping to ISO 639-2 code) is unlikely to change
      // so the output of the dependency should be stable over time.
      std::string iso6392Lower(from);
      StringUtils::Trim(iso6392Lower);

      // Anything with length <> 3 must have come from AS.xml or a language addon and will be left
      // as is.
      if (iso6392Lower.length() == 3)
      {
        StringUtils::ToLower(iso6392Lower);

        // BCP 47 uses the alpha-2 639-1 codes when defined for a language and alpha-3 codes from
        // 639-2/T 639-3 639-5 otherwise.
        //
        // Only few languages have different B and T forms.
        // ALL ISO 639-2/B codes that have a ISO 639-2/T equivalent also have an ISO 639-1
        // equivalent so conversion from B to T doesn't need to be considered - a conversion to
        // ISO 639-1 will happen instead.
        //
        // ISO 639-2/B that do not have an ISO 639-1 equivalent are left alone, as they are either
        // identical to the desired ISO 639-2/T code or unrecognized values that must have come
        // from AS.xml or a language addon. There is no easy way to tell which situation applies.
        const auto it = std::ranges::lower_bound(LanguageCodesByIso639_2b, iso6392Lower, {},
                                                 &ISO639::iso639_2b);
        if (it != LanguageCodesByIso639_2b.end() && it->iso639_2b == iso6392Lower)
        {
          const auto to = std::string{it->iso639_1};
          m_pDS->exec(PrepareSQL("UPDATE movie SET originalLanguage='" + to +
                                 "' WHERE originalLanguage='" + from + "'"));
          m_pDS->exec(PrepareSQL("UPDATE tvshow SET originalLanguage='" + to +
                                 "' WHERE originalLanguage='" + from + "'"));
        }
      }
      m_pDS->next();
    }
    m_pDS->close();
  }
}

int CVideoDatabase::GetSchemaVersion() const
{
  return 141;
}
