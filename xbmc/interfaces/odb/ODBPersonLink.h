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

#ifndef ODBPERSONLINK_H
#define ODBPERSONLINK_H

#include <odb/core.hxx>
#include <odb/lazy-ptr.hxx>

#include <string>

#include "ODBPerson.h"
#include "ODBRole.h"

PRAGMA_DB (model version(1, 1, open))

PRAGMA_DB (object pointer(std::shared_ptr) \
                  table("person_link"))
class CODBPersonLink
{
public:
  CODBPersonLink()
  {
    m_castOrder = 0;
    m_synced = false;
  };
  
PRAGMA_DB (id auto)
  unsigned long m_idPersonLink;
  odb::lazy_shared_ptr<CODBPerson> m_person;
  odb::lazy_shared_ptr<CODBRole> m_role;
  int m_castOrder;
  
  //Members not stored in the db, used for sync ...
PRAGMA_DB (transient)
  bool m_synced;
  
private:
  friend class odb::access;
  
PRAGMA_DB (index member(m_person))
  
};

PRAGMA_DB (view object(CODBPersonLink) \
                object(CODBRole inner: CODBPersonLink::m_role) \
                object(CODBPerson: CODBPersonLink::m_person) \
                query(distinct))
struct ODBView_Artist_Roles
{
  std::shared_ptr<CODBRole> role;
};

PRAGMA_DB (view object(CODBPersonLink)
                object(CODBRole: CODBPersonLink::m_role) \
                object(CODBPerson inner: CODBPersonLink::m_person) \
                query(distinct))
struct ODBView_Person_Count
{
  PRAGMA_DB (column("count("+CODBPerson::m_idPerson+")"))
  std::size_t count;
};

#endif /* ODBPERSONLINK_H */
