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

#ifndef ODBSEASON_H
#define ODBSEASON_H

#include <odb/core.hxx>
#include <odb/lazy-ptr.hxx>
#include <odb/section.hxx>

#include <memory>
#include <string>
#include <vector>

#include "ODBEpisode.h"
#include "ODBArt.h"

PRAGMA_DB (model version(1, 1, open))

PRAGMA_DB (object pointer(std::shared_ptr) \
                  table("season"))
class CODBSeason
{
public:
  CODBSeason()
  {
    m_name = "";
    m_season = -1;
    m_userrating = 0;
    m_synced = false;
  }
  
PRAGMA_DB (id auto)
  unsigned long m_idSeason;
PRAGMA_DB (type("VARCHAR(255)"))
  std::string m_name;
  int m_season;
  int m_userrating;
  CODBDate m_firstAired;
  
PRAGMA_DB (section(section_foreign))
  std::vector< odb::lazy_shared_ptr<CODBEpisode> > m_episodes;
PRAGMA_DB (section(section_foreign))
  std::vector< odb::lazy_shared_ptr<CODBArt> > m_artwork;
  
PRAGMA_DB (load(lazy) update(change))
  odb::section section_foreign;
  
  //Members not stored in the db, used for sync ...
PRAGMA_DB (transient)
  bool m_synced;
  
private:
  friend class odb::access;
  
PRAGMA_DB (index member(m_name))
};

PRAGMA_DB (view object(CODBSeason) \
                object(CODBEpisode inner: CODBSeason::m_episodes) \
                object(CODBFile: CODBEpisode::m_file) \
                query(distinct))
struct ODBView_Season_Episodes
{
  std::shared_ptr<CODBSeason> season;
  std::shared_ptr<CODBEpisode> episode;
};

PRAGMA_DB (view object(CODBSeason) \
                object(CODBArt inner: CODBSeason::m_artwork) \
                query(distinct))
struct ODBView_Season_Art
{
  std::shared_ptr<CODBArt> art;
};


#endif /* ODBSEASON_H */
