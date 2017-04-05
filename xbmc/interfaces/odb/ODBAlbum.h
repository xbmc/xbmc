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

#ifndef ODBALBUM_H
#define ODBALBUM_H

#include <odb/core.hxx>

#include "ODBGenre.h"
#include "ODBPersonLink.h"
#include "ODBDate.h"
#include "ODBArt.h"
#include "ODBArtistDetail.h"

#include <string>

PRAGMA_DB (model version(1, 1, open) )

PRAGMA_DB (object pointer(std::shared_ptr) \
                  table("album") )
class CODBAlbum
{
public:
  CODBAlbum()
  {
    m_year = 0;
    m_compilation = false;
    m_rating = 0.0;
    m_userrating = 0;
    m_votes = 0;
    
    m_synced = false;
  };
  
PRAGMA_DB (id auto)
  unsigned long m_idAlbum;
  std::string m_album;
  std::string m_musicBrainzAlbumID;
  int m_year;
  bool m_compilation;
  std::string m_moods;
  std::string m_styles;
  std::string m_themes;
  std::string m_review;
  std::string m_image;
  std::string m_label;
  std::string m_type;
  float m_rating;
  int m_userrating;
  CODBDate m_lastScraped;
  std::string m_releaseType;
  int m_votes;
  
  //TODO: See if we can remove those later down the road and use the below foreign objects
  std::string m_artistsString;
  std::string m_genresString;
  
PRAGMA_DB (section(section_foreign) )
  std::vector< odb::lazy_shared_ptr<CODBPersonLink> > m_artists;
PRAGMA_DB (section(section_foreign) )
  std::vector< odb::lazy_shared_ptr<CODBGenre> > m_genres;
PRAGMA_DB (section(section_foreign) )
  std::vector< odb::lazy_shared_ptr<CODBArt> > m_artwork;
  
  //Members not stored in the db, used for sync ...
PRAGMA_DB (transient)
  bool m_synced;
  
PRAGMA_DB ( load(lazy) update(change) )
  odb::section section_foreign;
  
private:
  friend class odb::access;
};

PRAGMA_DB (view object(CODBAlbum) \
                object(CODBArt inner: CODBAlbum::m_artwork) \
                query(distinct) )
struct ODBView_Album_Art
{
  std::shared_ptr<CODBArt> art;
};

PRAGMA_DB (view object(CODBAlbum) \
                object(CODBPersonLink: CODBAlbum::m_artists) \
                object(CODBPerson: CODBPersonLink::m_person) \
                query(distinct) )
struct ODBView_Albums_by_Artist
{
  std::shared_ptr<CODBAlbum> album;
};

PRAGMA_DB (view object(CODBAlbum) \
                object(CODBPersonLink: CODBAlbum::m_artists) \
                object(CODBPerson inner: CODBPersonLink::m_person) \
                query(distinct) )
struct ODBView_Artists_by_Album
{
  std::shared_ptr<CODBPersonLink> artist;
};

PRAGMA_DB (view object(CODBAlbum) \
                query(distinct) )
struct ODBView_Album_Years
{
  PRAGMA_DB (column(CODBAlbum::m_year))
  int year;
};

PRAGMA_DB (view object(CODBAlbum))
struct ODBView_Album_Count
{
  PRAGMA_DB (column("count(1)"))
  std::size_t count;
};

PRAGMA_DB (view object(CODBAlbum) \
                object(CODBPersonLink: CODBAlbum::m_artists) \
                object(CODBPerson: CODBPersonLink::m_person) \
                object(CODBRole: CODBPersonLink::m_role) \
                object(CODBArt inner: CODBPerson::m_art) \
                query(distinct) )
struct ODBView_Album_Artist_Art
{
  std::shared_ptr<CODBArt> art;
};


#endif /* ODBALBUM_H */
