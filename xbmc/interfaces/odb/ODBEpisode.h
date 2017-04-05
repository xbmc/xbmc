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

#ifndef ODBEPISODE_H
#define ODBEPISODE_H

#include <odb/core.hxx>
#include <odb/lazy-ptr.hxx>
#include <odb/section.hxx>

#include <memory>
#include <string>
#include <vector>

#include "ODBFile.h"
#include "ODBRating.h"
#include "ODBDate.h"
#include "ODBBookmark.h"
#include "ODBPersonLink.h"
#include "ODBUniqueID.h"
#include "ODBArt.h"
#include "ODBStreamDetails.h"

PRAGMA_DB (model version(1, 1, open))

PRAGMA_DB (object pointer(std::shared_ptr) \
                  table("episode"))

class CODBEpisode
{
public:
  CODBEpisode()
  {
    m_idEpisode = 0;
    m_title = "";
    m_plot = "";
    m_thumbUrl = "";
    m_thumbUrl_spoofed = "";
    m_runtime = 0;
    m_productionCode = "";
    m_episode = -1;
    m_originalTitle = "";
    m_sortSeason = -1;
    m_sortEpisode = -1;
    m_identId = 0;
    m_userrating = 0;
    m_idShow = 0;
  }
  
PRAGMA_DB (id auto)
  unsigned long m_idEpisode;
PRAGMA_DB (type("VARCHAR(255)"))
  std::string m_title;
  std::string m_plot;
  CODBDate m_aired;
  std::string m_thumbUrl;
  std::string m_thumbUrl_spoofed;
  int m_runtime;
  std::string m_productionCode;
  int m_episode;
  std::string m_originalTitle;
  int m_sortSeason;
  int m_sortEpisode;
  int m_identId;
  int m_userrating;
  
  //Temporary Show ID as used in CVideoInfoScanner:1264
  //See if this can be refactored away when odb is fully integrated
  unsigned long m_idShow;
  
PRAGMA_DB (section(section_foreign))
  odb::lazy_shared_ptr<CODBFile> m_file;
PRAGMA_DB (section(section_foreign))
  odb::lazy_shared_ptr<CODBRating> m_defaultRating;
PRAGMA_DB (section(section_foreign))
  std::vector< odb::lazy_shared_ptr<CODBRating> > m_ratings;
PRAGMA_DB (section(section_foreign))
  std::vector< odb::lazy_shared_ptr<CODBPersonLink> > m_credits;
PRAGMA_DB (section(section_foreign))
  std::vector< odb::lazy_shared_ptr<CODBPersonLink> > m_directors;
PRAGMA_DB (section(section_foreign))
  std::vector< odb::lazy_shared_ptr<CODBPersonLink> > m_writingCredits;
PRAGMA_DB (section(section_foreign))
  std::vector< odb::lazy_shared_ptr<CODBArt> > m_artwork;
PRAGMA_DB (section(section_foreign))
  std::vector< odb::lazy_shared_ptr<CODBPersonLink> > m_actors;
PRAGMA_DB (section(section_foreign))
  odb::lazy_shared_ptr<CODBPath> m_basepath;
PRAGMA_DB (section(section_foreign))
  std::vector< odb::lazy_shared_ptr<CODBUniqueID> > m_ids;
PRAGMA_DB (section(section_foreign))
  odb::lazy_shared_ptr<CODBUniqueID> m_defaultID;
PRAGMA_DB (section(section_foreign))
  std::vector< odb::lazy_shared_ptr<CODBBookmark> > m_bookmarks;
PRAGMA_DB (section(section_foreign))
  odb::lazy_shared_ptr<CODBBookmark> m_resumeBookmark;
  
PRAGMA_DB (load(lazy) update(change))
  odb::section section_foreign;
  
  //Members not stored in the db, used for sync ...
PRAGMA_DB (transient)
  bool m_synced;
  
private:
  friend class odb::access;
  
PRAGMA_DB (index member(m_title))
PRAGMA_DB (index member(m_sortSeason))
PRAGMA_DB (index member(m_sortEpisode))
PRAGMA_DB (index member(m_identId))
PRAGMA_DB (index member(m_userrating))
};

PRAGMA_DB (view object(CODBEpisode) \
                object(CODBArt inner: CODBEpisode::m_artwork) \
                query(distinct))
struct ODBView_Episode_Art
{
  std::shared_ptr<CODBArt> art;
};

#endif /* ODBEPISODE_H */
