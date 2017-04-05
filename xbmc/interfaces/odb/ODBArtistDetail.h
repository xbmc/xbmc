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

#ifndef ODBARTISTDETAIL_H
#define ODBARTISTDETAIL_H

#include <odb/core.hxx>

#include "ODBPerson.h"
#include "ODBDate.h"
#include "ODBGenre.h"
#include "ODBInfoSetting.h"

#include <string>

PRAGMA_DB (model version(1, 1, open))

PRAGMA_DB (object pointer(std::shared_ptr) \
                  table("artist_details"))
class CODBArtistDetail
{
public:
  CODBArtistDetail()
  {
    m_scrapedMBID = false;
    m_synced = false;
  };
  
PRAGMA_DB (id auto)
  unsigned long m_idArtistDetail;
  std::string m_musicBrainzArtistId;
  std::string m_born;
  std::string m_formed;
  std::string m_moods;
  std::string m_styles;
  std::string m_instruments;
  std::string m_biography;
  std::string m_died;
  std::string m_disbanded;
  std::string m_yearsActive;
  CODBDate m_lastScraped;
  std::string m_image;
  std::string m_fanart;
  bool m_scrapedMBID;

PRAGMA_DB (section(section_foreign))
  odb::lazy_shared_ptr<CODBPerson> m_person;
PRAGMA_DB (section(section_foreign))
  std::vector<odb::lazy_shared_ptr<CODBGenre> > m_genres;
PRAGMA_DB (section(section_foreign))
  odb::lazy_shared_ptr<CODBInfoSetting> m_infoSetting;
  
  //Members not stored in the db, used for sync ...
PRAGMA_DB (transient)
  bool m_synced;
  
PRAGMA_DB (load(lazy) update(change))
  odb::section section_foreign;
  
private:
  friend class odb::access;
};

PRAGMA_DB (view object(CODBArtistDetail) \
                object(CODBPerson inner: CODBArtistDetail::m_person) \
                query(distinct))
struct ODBView_Artist_Details
{
  std::shared_ptr<CODBPerson> person;
  std::shared_ptr<CODBArtistDetail> details;
};

#endif /* ODBARTISTDETAIL_H */
