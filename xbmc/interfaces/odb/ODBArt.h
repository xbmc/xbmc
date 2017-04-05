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

#ifndef KODI_ODBART_H
#define KODI_ODBART_H

#include <odb/core.hxx>
#include <odb/lazy-ptr.hxx>

#include <string>

PRAGMA_DB (model version(1, 1, open) )

PRAGMA_DB (object pointer(std::shared_ptr) \
                  table("art") )
class CODBArt
{
public:
  CODBArt()
  {
    m_idArt = 0;
    m_media_type = "";
    m_type = "";
    m_url = "";
    m_synced = false;
  };

PRAGMA_DB (id auto)
  unsigned long m_idArt;
  std::string m_media_type;
PRAGMA_DB (type("VARCHAR(255)"))
  std::string m_type;
  std::string m_url;
  
  //Members not stored in the db, used for sync ...
PRAGMA_DB(transient)
  bool m_synced;
  
private:
  friend class odb::access;
  
PRAGMA_DB(index member(m_type))

};

PRAGMA_DB (view object(CODBArt) \
                query(distinct))
struct ODBView_Art_Type
{
  std::string m_media_type;
  std::string m_type;
};

#endif //KODI_ODBART_H
