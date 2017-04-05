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

#ifndef ODBMOVIE_H
#define ODBMOVIE_H

#include <odb/core.hxx>
#include <odb/lazy-ptr.hxx>
#include <odb/section.hxx>

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "ODBBookmark.h"
#include "ODBCountry.h"
#include "ODBFile.h"
#include "ODBRating.h"
#include "ODBGenre.h"
#include "ODBPersonLink.h"
#include "ODBSet.h"
#include "ODBStudio.h"
#include "ODBStreamDetails.h"
#include "ODBTag.h"
#include "ODBUniqueID.h"
#include "ODBArt.h"
#include "ODBDate.h"
#include "ODBTVShow.h"


PRAGMA_DB (model version(1, 1, open))

PRAGMA_DB (object pointer(std::shared_ptr) \
                  table("movie"))
class CODBMovie
{
public:
  CODBMovie()
  {
    m_title = "";
    m_plot = "";
    m_plotoutline = "";
    m_tagline = "";
    m_credits = "";
    m_thumbUrl = "";
    m_sortTitle = "";
    m_runtime = 0;
    m_mpaa = "";
    m_top250 = 0;
    m_originalTitle = "";
    m_thumbUrl_spoof = "";
    m_trailer = "";
    m_fanart = "";
    m_userrating = 0;
    m_synced = false;
  };
  
PRAGMA_DB (id auto)
  unsigned long m_idMovie;
PRAGMA_DB (type("VARCHAR(255)"))
  std::string m_title;
  std::string m_plot;
  std::string m_plotoutline;
  std::string m_tagline;
  std::string m_credits;
  std::string m_thumbUrl;
PRAGMA_DB (type("VARCHAR(255)"))
  std::string m_sortTitle;
  int m_runtime;
  std::string m_mpaa;
  int m_top250;
  int m_userrating;
  std::string m_trailer;
  std::string m_fanart;
  CODBDate m_premiered;
PRAGMA_DB (type("VARCHAR(255)"))
  std::string m_originalTitle;
  std::string m_thumbUrl_spoof;
PRAGMA_DB (section(section_foreign))
  odb::lazy_shared_ptr<CODBFile> m_file;
PRAGMA_DB (section(section_foreign))
  odb::lazy_shared_ptr<CODBPath> m_basePath;
PRAGMA_DB (section(section_foreign))
  odb::lazy_shared_ptr<CODBPath> m_parentPath;
PRAGMA_DB (section(section_foreign))
  odb::lazy_shared_ptr<CODBRating> m_defaultRating;
PRAGMA_DB (section(section_foreign))
  std::vector< odb::lazy_shared_ptr<CODBRating> > m_ratings;
PRAGMA_DB (section(section_foreign))
  std::vector< odb::lazy_shared_ptr<CODBGenre> > m_genres;
PRAGMA_DB (section(section_foreign))
  std::vector< odb::lazy_shared_ptr<CODBPersonLink> > m_directors;
PRAGMA_DB (section(section_foreign))
  std::vector< odb::lazy_shared_ptr<CODBPersonLink> > m_actors;
PRAGMA_DB (section(section_foreign))
  std::vector< odb::lazy_shared_ptr<CODBPersonLink> > m_writingCredits;
PRAGMA_DB (section(section_foreign))
  std::vector< odb::lazy_shared_ptr<CODBStudio> > m_studios;
PRAGMA_DB (section(section_foreign))
  std::vector< odb::lazy_shared_ptr<CODBTag> > m_tags;
PRAGMA_DB (section(section_foreign))
  std::vector< odb::lazy_shared_ptr<CODBCountry> > m_countries;
PRAGMA_DB (section(section_foreign))
  odb::lazy_shared_ptr<CODBSet> m_set;
PRAGMA_DB (section(section_foreign))
  std::vector< odb::lazy_shared_ptr<CODBBookmark> > m_bookmarks;
PRAGMA_DB (section(section_foreign))
  odb::lazy_shared_ptr<CODBBookmark> m_resumeBookmark;
PRAGMA_DB (section(section_foreign))
  std::vector< odb::lazy_shared_ptr<CODBUniqueID> > m_ids;
PRAGMA_DB (section(section_foreign))
  odb::lazy_shared_ptr<CODBUniqueID> m_defaultID;
PRAGMA_DB (section(section_foreign))
  std::vector<odb::lazy_shared_ptr<CODBTVShow>> m_linkedTVShows;
PRAGMA_DB (section(section_artwork))
  std::vector< odb::lazy_shared_ptr<CODBArt> > m_artwork;
  
PRAGMA_DB (load(lazy) update(change))
  odb::section section_foreign;
  
PRAGMA_DB (load(lazy) update(change))
  odb::section section_artwork;
  
  //Members not stored in the db, used for sync ...
PRAGMA_DB (transient)
  bool m_synced;
  
private:
  friend class odb::access;
  
PRAGMA_DB (index member(m_file))
PRAGMA_DB (index member(m_title))
PRAGMA_DB (index member(m_defaultRating))
PRAGMA_DB (index member(m_sortTitle))
PRAGMA_DB (index member(m_originalTitle))
PRAGMA_DB (index member(m_basePath))
PRAGMA_DB (index member(m_parentPath))
PRAGMA_DB (index member(m_set))
PRAGMA_DB (index member(m_userrating))
PRAGMA_DB (index member(m_premiered))
PRAGMA_DB (index member(m_defaultID))
};

PRAGMA_DB (view \
  object(CODBMovie) \
  object(CODBGenre = genre: CODBMovie::m_genres) \
  object(CODBPersonLink = director_link: CODBMovie::m_directors) \
  object(CODBPerson = director: director_link::m_person) \
  object(CODBPersonLink = actor_ink: CODBMovie::m_actors) \
  object(CODBPerson = actor: actor_ink::m_person) \
  object(CODBPersonLink = writingCredit_link: CODBMovie::m_writingCredits) \
  object(CODBPerson = writingCredit: writingCredit_link::m_person) \
  object(CODBStudio = studio: CODBMovie::m_studios) \
  object(CODBTag = tag: CODBMovie::m_tags) \
  object(CODBCountry = country: CODBMovie::m_countries) \
  object(CODBSet = set: CODBMovie::m_set) \
  object(CODBBookmark = bookmark: CODBMovie::m_bookmarks) \
  object(CODBFile = fileView: CODBMovie::m_file) \
  object(CODBPath = pathView: fileView::m_path) \
  object(CODBStreamDetails: CODBMovie::m_file == CODBStreamDetails::m_file) \
  object(CODBRating = defaultRating: CODBMovie::m_defaultRating) \
  query(distinct))
struct ODBView_Movie
{
  std::shared_ptr<CODBMovie> movie;
};

PRAGMA_DB (view \
  object(CODBMovie) \
  object(CODBGenre = genre inner: CODBMovie::m_genres) \
  object(CODBFile = file inner: CODBMovie::m_file) \
  object(CODBPath = path inner: file::m_path) \
  query(distinct))
struct ODBView_Movie_Genre
{
  PRAGMA_DB (column(genre::m_idGenre))
  unsigned long m_idGenre;
  PRAGMA_DB (column(genre::m_name))
  std::string m_name;
  PRAGMA_DB (column(file::m_playCount))
  unsigned int m_playCount;
  PRAGMA_DB (column(path::m_path))
  std::string m_path;
};

PRAGMA_DB (view \
  object(CODBMovie) \
  object(CODBCountry = country inner: CODBMovie::m_countries) \
  object(CODBFile = file inner: CODBMovie::m_file) \
  object(CODBPath = path inner: file::m_path) \
  query(distinct))
struct ODBView_Movie_Country
{
PRAGMA_DB (column(country::m_idCountry))
  unsigned long m_idCountry;
PRAGMA_DB (column(country::m_name))
  std::string m_name;
PRAGMA_DB (column(file::m_playCount))
  unsigned int m_playCount;
PRAGMA_DB (column(path::m_path))
  std::string m_path;
};

PRAGMA_DB (view \
  object(CODBMovie) \
  object(CODBTag = tag inner: CODBMovie::m_tags) \
  object(CODBFile = file inner: CODBMovie::m_file) \
  object(CODBPath = path inner: file::m_path) \
  query(distinct))
struct ODBView_Movie_Tag
{
PRAGMA_DB (column(tag::m_idTag))
  unsigned long m_idTag;
PRAGMA_DB (column(tag::m_name))
  std::string m_name;
PRAGMA_DB (column(file::m_playCount))
  unsigned int m_playCount;
PRAGMA_DB (column(path::m_path))
  std::string m_path;
};

PRAGMA_DB (view \
  object(CODBMovie) \
  object(CODBPersonLink = person_link inner: CODBMovie::m_directors) \
  object(CODBPerson = person inner: person_link::m_person) \
  object(CODBFile = file inner: CODBMovie::m_file) \
  object(CODBPath = path inner: file::m_path) \
  query(distinct))
struct ODBView_Movie_Director
{
  std::shared_ptr<CODBPerson> person;
PRAGMA_DB (column(file::m_playCount))
  unsigned int m_playCount;
PRAGMA_DB (column(path::m_path))
  std::string m_path;
};

PRAGMA_DB (view \
  object(CODBMovie) \
  object(CODBPersonLink = person_link inner: CODBMovie::m_actors) \
  object(CODBPerson = person inner: person_link::m_person) \
  object(CODBFile = file inner: CODBMovie::m_file) \
  object(CODBPath = path inner: file::m_path) \
  query(distinct))
struct ODBView_Movie_Actor
{
  std::shared_ptr<CODBPerson> person;
PRAGMA_DB (column(file::m_playCount))
  unsigned int m_playCount;
PRAGMA_DB (column(path::m_path))
  std::string m_path;
};

PRAGMA_DB (view \
  object(CODBMovie) \
  object(CODBStudio = studio inner: CODBMovie::m_studios) \
  object(CODBFile = file inner: CODBMovie::m_file) \
  object(CODBPath = path inner: file::m_path) \
  query(distinct))
struct ODBView_Movie_Studio
{
PRAGMA_DB (column(studio::m_idStudio))
  unsigned long m_idStudio;
PRAGMA_DB (column(studio::m_name))
  std::string m_name;
PRAGMA_DB (column(file::m_playCount))
  unsigned int m_playCount;
PRAGMA_DB (column(path::m_path))
  std::string m_path;
};

PRAGMA_DB (view object(CODBMovie))
struct ODBView_Movie_Count
{
PRAGMA_DB (column("count(1)"))
  std::size_t count;
};

PRAGMA_DB (view object(CODBMovie) \
  object(CODBFile inner: CODBMovie::m_file) \
  object(CODBPath inner: CODBFile::m_path))
struct ODBView_Movie_File_Path
{
  std::shared_ptr<CODBMovie> movie;
  std::shared_ptr<CODBFile> file;
  std::shared_ptr<CODBPath> path;
};

PRAGMA_DB (view object(CODBMovie) \
  object(CODBFile inner: CODBMovie::m_file) \
  object(CODBUniqueID inner: CODBMovie::m_file))
struct ODBView_Movie_File_UID
{
  std::shared_ptr<CODBMovie> movie;
  std::shared_ptr<CODBFile> file;
  std::shared_ptr<CODBUniqueID> uid;
};

PRAGMA_DB (view object(CODBMovie) \
  object(CODBArt inner: CODBMovie::m_artwork) \
  query(distinct))
struct ODBView_Movie_Art
{
  std::shared_ptr<CODBArt> art;
};

#endif /* ODBMOVIE_H */
