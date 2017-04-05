/*
*      Copyright (C) 2017 Team Kodi
*      https://kodi.tv
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

#ifndef ODBTVSHOW_H
#define ODBTVSHOW_H

#include <odb/core.hxx>
#include <odb/lazy-ptr.hxx>
#include <odb/section.hxx>

#include <memory>
#include <string>
#include <vector>

#include "ODBRating.h"
#include "ODBGenre.h"
#include "ODBPersonLink.h"
#include "ODBStudio.h"
#include "ODBStreamDetails.h"
#include "ODBTag.h"
#include "ODBUniqueID.h"
#include "ODBArt.h"
#include "ODBDate.h"
#include "ODBSeason.h"
#include "ODBPath.h"

PRAGMA_DB (model version(1, 1, open))

PRAGMA_DB (object pointer(std::shared_ptr) \
                  table("tvshow"))

class CODBTVShow
{
public:
  CODBTVShow()
  {
    m_idTVShow = 0;
    m_title = "";
    m_plot = "";
    m_status = "";
    m_thumbUrl = "";
    m_thumbUrl_spoof = "";
    m_originalTitle = "";
    m_episodeGuide = "";
    m_fanart = "";
    m_identID = "";
    m_mpaa = "";
    m_sortTitle = "";
    m_userrating = 0;
    m_runtime = 0;
    m_synced = false;
  };
  
PRAGMA_DB (id auto)
  unsigned long m_idTVShow;
PRAGMA_DB (type("VARCHAR(255)"))
  std::string m_title;
  std::string m_plot;
  std::string m_status;
  CODBDate m_premiered;
  std::string m_thumbUrl;
  std::string m_thumbUrl_spoof;
PRAGMA_DB (type("VARCHAR(255)"))
  std::string m_originalTitle;
  std::string m_episodeGuide;
  std::string m_fanart;
  std::string m_identID;
  std::string m_mpaa;
PRAGMA_DB (type("VARCHAR(255)"))
  std::string m_sortTitle;
  int m_userrating;
  int m_runtime;
  
PRAGMA_DB (section(section_foreign))
  odb::lazy_shared_ptr<CODBRating> m_defaultRating;
PRAGMA_DB (section(section_foreign))
  std::vector< odb::lazy_shared_ptr<CODBRating> > m_ratings;
PRAGMA_DB (section(section_foreign))
  std::vector< odb::lazy_shared_ptr<CODBGenre> > m_genres;
PRAGMA_DB (section(section_foreign))
  std::vector< odb::lazy_shared_ptr<CODBArt> > m_artwork;
PRAGMA_DB (section(section_foreign))
  std::vector< odb::lazy_shared_ptr<CODBStudio> > m_studios;
PRAGMA_DB (section(section_foreign))
  std::vector< odb::lazy_shared_ptr<CODBPersonLink> > m_actors;
PRAGMA_DB (section(section_foreign))
  std::vector< odb::lazy_shared_ptr<CODBPersonLink> > m_directors;
PRAGMA_DB (section(section_foreign))
  std::vector< odb::lazy_shared_ptr<CODBTag> > m_tags;
PRAGMA_DB (section(section_foreign))
  std::vector< odb::lazy_shared_ptr<CODBUniqueID> > m_ids;
PRAGMA_DB (section(section_foreign))
  std::vector< odb::lazy_shared_ptr<CODBSeason> > m_seasons;
PRAGMA_DB (section(section_foreign))
  odb::lazy_shared_ptr<CODBUniqueID> m_defaultID;
PRAGMA_DB (section(section_foreign))
  std::vector< odb::lazy_shared_ptr<CODBPath> > m_paths;
  
PRAGMA_DB (load(lazy) update(change))
  odb::section section_foreign;
  
  //Members not stored in the db, used for sync ...
PRAGMA_DB (transient)
  bool m_synced;
  
private:
  friend class odb::access;
  
PRAGMA_DB (index member(m_title))
PRAGMA_DB (index member(m_originalTitle))
PRAGMA_DB (index member(m_sortTitle))
PRAGMA_DB (index member(m_userrating))
};

PRAGMA_DB (view object(CODBTVShow) \
                object(CODBGenre = genre: CODBTVShow::m_genres) \
                object(CODBPersonLink = director_link: CODBTVShow::m_directors) \
                object(CODBPerson = director: director_link::m_person) \
                object(CODBPersonLink = actor_ink: CODBTVShow::m_actors) \
                object(CODBPerson = actor: actor_ink::m_person) \
                object(CODBStudio = studio: CODBTVShow::m_studios) \
                object(CODBTag = tag: CODBTVShow::m_tags) \
                object(CODBRating = defaultRating: CODBTVShow::m_defaultRating) \
                object(CODBPath = path: CODBTVShow::m_paths) \
                query(distinct))
struct ODBView_TVShow
{
  std::shared_ptr<CODBTVShow> show;
};

PRAGMA_DB (view object(CODBTVShow) \
           object(CODBGenre = genre: CODBTVShow::m_genres) \
           object(CODBPersonLink = director_link: CODBTVShow::m_directors) \
           object(CODBPerson = director: director_link::m_person) \
           object(CODBPersonLink = actor_ink: CODBTVShow::m_actors) \
           object(CODBPerson = actor: actor_ink::m_person) \
           object(CODBStudio = studio: CODBTVShow::m_studios) \
           object(CODBTag = tag: CODBTVShow::m_tags) \
           object(CODBRating = defaultRating: CODBTVShow::m_defaultRating) \
           object(CODBPath = path: CODBTVShow::m_paths))
struct ODBView_TVShow_Total
{
  PRAGMA_DB (column("COUNT(DISTINCT(" + CODBTVShow::m_idTVShow + "))"))
  unsigned int total;
};

// ODBView_Season and ODBView_Episode are here to avoid forward declarations

PRAGMA_DB (view object(CODBTVShow) \
                object(CODBSeason inner: CODBTVShow::m_seasons) \
                object(CODBEpisode inner: CODBSeason::m_episodes) \
                object(CODBFile inner: CODBEpisode::m_file) \
                query(distinct))
struct ODBView_Season
{
PRAGMA_DB(column(CODBTVShow::m_idTVShow))
  unsigned long m_idTVShow;
  
PRAGMA_DB(column(CODBSeason::m_idSeason))
  unsigned long m_idSeason;
};

PRAGMA_DB (view object(CODBTVShow) \
           object(CODBSeason inner: CODBTVShow::m_seasons) \
           object(CODBEpisode inner: CODBSeason::m_episodes) \
           object(CODBFile inner: CODBEpisode::m_file))
struct ODBView_Season_Total
{
  PRAGMA_DB (column("COUNT(DISTINCT(" + CODBSeason::m_idSeason + "))"))
  unsigned int total;
};

PRAGMA_DB (view object(CODBTVShow) \
                object(CODBSeason inner: CODBTVShow::m_seasons) \
                object(CODBEpisode inner: CODBSeason::m_episodes) \
                object(CODBGenre = genre: CODBTVShow::m_genres) \
                object(CODBPersonLink = director_link: CODBEpisode::m_directors) \
                object(CODBPerson = director: director_link::m_person) \
                object(CODBPersonLink = actor_ink: CODBEpisode::m_actors) \
                object(CODBPerson = actor: actor_ink::m_person) \
                object(CODBPersonLink = writingCredit_link: CODBEpisode::m_writingCredits) \
                object(CODBPerson = writingCredit: writingCredit_link::m_person) \
                object(CODBStudio = studio: CODBTVShow::m_studios) \
                object(CODBFile = fileView: CODBEpisode::m_file) \
                object(CODBPath = pathView: fileView::m_path) \
                object(CODBStreamDetails: CODBEpisode::m_file == CODBStreamDetails::m_file) \
                object(CODBRating = defaultRating: CODBEpisode::m_defaultRating) \
                object(CODBTag = tag: CODBTVShow::m_tags) \
                query(distinct))
struct ODBView_Episode
{
  std::shared_ptr<CODBEpisode> episode;
};

PRAGMA_DB (view object(CODBTVShow) \
           object(CODBSeason inner: CODBTVShow::m_seasons) \
           object(CODBEpisode inner: CODBSeason::m_episodes) \
           object(CODBGenre = genre: CODBTVShow::m_genres) \
           object(CODBPersonLink = director_link: CODBEpisode::m_directors) \
           object(CODBPerson = director: director_link::m_person) \
           object(CODBPersonLink = actor_ink: CODBEpisode::m_actors) \
           object(CODBPerson = actor: actor_ink::m_person) \
           object(CODBPersonLink = writingCredit_link: CODBEpisode::m_writingCredits) \
           object(CODBPerson = writingCredit: writingCredit_link::m_person) \
           object(CODBStudio = studio: CODBTVShow::m_studios) \
           object(CODBFile = fileView: CODBEpisode::m_file) \
           object(CODBPath = pathView: fileView::m_path) \
           object(CODBStreamDetails: CODBEpisode::m_file == CODBStreamDetails::m_file) \
           object(CODBRating = defaultRating: CODBEpisode::m_defaultRating) \
           object(CODBTag = tag: CODBTVShow::m_tags))
struct ODBView_Episode_Total
{
  PRAGMA_DB (column("COUNT(DISTINCT(" + CODBEpisode::m_idEpisode + "))"))
  unsigned int total;
};

PRAGMA_DB (view object(CODBTVShow) \
                object(CODBPath = path inner: CODBTVShow::m_paths) \
                query(distinct))
struct ODBView_TVShow_Path
{
  std::shared_ptr<CODBTVShow> show;
  std::shared_ptr<CODBPath> path;
};

PRAGMA_DB (view object(CODBTVShow) \
                object(CODBUniqueID inner: CODBTVShow::m_ids) \
                query(distinct))
struct ODBView_TVShow_UID
{
  std::shared_ptr<CODBTVShow> show;
  std::shared_ptr<CODBUniqueID> uid;
};

PRAGMA_DB (view object(CODBTVShow) \
                object(CODBSeason inner: CODBTVShow::m_seasons) \
                query(distinct))
struct ODBView_TVShow_Seasons
{
  std::shared_ptr<CODBTVShow> show;
  std::shared_ptr<CODBSeason> season;
};

PRAGMA_DB (view object(CODBTVShow) \
                object(CODBSeason inner: CODBTVShow::m_seasons) \
                object(CODBEpisode inner: CODBSeason::m_episodes) \
                query(distinct))
struct ODBView_TVShow_Season_Episodes
{
  std::shared_ptr<CODBTVShow> show;
  std::shared_ptr<CODBSeason> season;
  std::shared_ptr<CODBEpisode> episode;
};

PRAGMA_DB (view object(CODBTVShow) \
                object(CODBSeason inner: CODBTVShow::m_seasons) \
                object(CODBEpisode inner: CODBSeason::m_episodes) \
                object(CODBFile inner: CODBEpisode::m_file) \
                query(distinct))
struct ODBView_TVShow_Counts
{
PRAGMA_DB (column("MAX(" + CODBFile::m_lastPlayed.m_ulong_date + ")"))
  unsigned long lastPlayedULong;
  
PRAGMA_DB (column("NULLIF(COUNT(" + CODBSeason::m_season + "), 0)"))
  unsigned int totalCount;
  
PRAGMA_DB (column("COUNT(" + CODBFile::m_playCount + ")"))
  unsigned int watchedCount;
  
PRAGMA_DB (column("NULLIF(COUNT(DISTINCT(" + CODBSeason::m_season + ")), 0)"))
  unsigned int totalSeasons;
  
PRAGMA_DB (column("MAX(" + CODBFile::m_dateAdded.m_ulong_date + ")"))
  unsigned long dateAddedULong;
};

PRAGMA_DB (view object(CODBTVShow) \
                object(CODBArt inner: CODBTVShow::m_artwork) \
                query(distinct))
struct ODBView_TVShow_Art
{
  std::shared_ptr<CODBArt> art;
};

PRAGMA_DB (view object(CODBTVShow) \
                object(CODBGenre = genre inner: CODBTVShow::m_genres) \
                object(CODBPath = path inner: CODBTVShow::m_paths) \
                query(distinct))
struct ODBView_TVShow_Genre
{
PRAGMA_DB (column(genre::m_idGenre))
  unsigned long m_idGenre;
PRAGMA_DB (column(genre::m_name))
  std::string m_name;
PRAGMA_DB (column(path::m_path))
  std::string m_path;
};

PRAGMA_DB (view object(CODBTVShow) \
                object(CODBStudio = studio inner: CODBTVShow::m_studios) \
                object(CODBPath = path inner: CODBTVShow::m_paths) \
                query(distinct))
struct ODBView_TVShow_Studio
{
PRAGMA_DB (column(studio::m_idStudio))
  unsigned long m_idStudio;
PRAGMA_DB (column(studio::m_name))
  std::string m_name;
PRAGMA_DB (column(path::m_path))
  std::string m_path;
};

PRAGMA_DB (view object(CODBTVShow) \
                object(CODBTag = tag inner: CODBTVShow::m_tags) \
                object(CODBPath = path inner: CODBTVShow::m_paths) \
                query(distinct))
struct ODBView_TVShow_Tag
{
PRAGMA_DB (column(tag::m_idTag))
  unsigned long m_idTag;
PRAGMA_DB (column(tag::m_name))
  std::string m_name;
PRAGMA_DB (column(path::m_path))
  std::string m_path;
};

PRAGMA_DB (view object(CODBTVShow) \
                object(CODBPersonLink = person_link inner: CODBTVShow::m_directors) \
                object(CODBPerson = person inner: person_link::m_person) \
                object(CODBPath = path inner: CODBTVShow::m_paths) \
                query(distinct))
struct ODBView_TVShow_Director
{
  std::shared_ptr<CODBPerson> person;
PRAGMA_DB (column(path::m_path))
  std::string m_path;
};

PRAGMA_DB (view object(CODBTVShow) \
                object(CODBPersonLink = person_link inner: CODBTVShow::m_actors) \
                object(CODBPerson = person inner: person_link::m_person) \
                object(CODBPath = path inner: CODBTVShow::m_paths) \
                object(CODBArt: person::m_art) \
                query(distinct))
struct ODBView_TVShow_Actor
{
  std::shared_ptr<CODBPerson> person;
PRAGMA_DB (column(path::m_path))
  std::string m_path;
PRAGMA_DB (column(CODBArt::m_url))
  std::string m_art_url;
};

PRAGMA_DB (view object(CODBTVShow))
struct ODBView_TVShow_Count
{
PRAGMA_DB (column("count(1)"))
  std::size_t count;
};

#endif /* ODBTVSHOW_H */
