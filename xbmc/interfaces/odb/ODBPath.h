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

#ifndef ODBPATH_H
#define ODBPATH_H

#include <odb/core.hxx>
#include <odb/lazy-ptr.hxx>

#include <string>

#include "ODBDate.h"

PRAGMA_DB (model version(1, 1, open))

PRAGMA_DB (object pointer(std::shared_ptr) \
                  table("path"))
class CODBPath
{
public:
  CODBPath()
  {
    m_path = "";
    m_content = "";
    m_scraper = "";
    m_hash = "";
    m_scanRecursive = 0;
    m_useFolderNames = false;
    m_settings = "";
    m_noUpdate = false;
    m_exclude =false;
    m_synced = false;
  };
  
PRAGMA_DB (id auto)
  unsigned long m_idPath;
PRAGMA_DB (type("VARCHAR(255)"))
  std::string m_path;
PRAGMA_DB (type("VARCHAR(255)"))
  std::string m_content;
PRAGMA_DB (type("VARCHAR(255)"))
  std::string m_scraper;
PRAGMA_DB (type("VARCHAR(255)"))
  std::string m_hash;
  int m_scanRecursive;
  bool m_useFolderNames;
  std::string m_settings;
  bool m_noUpdate;
  bool m_exclude;
  CODBDate m_dateAdded;
  odb::lazy_shared_ptr<CODBPath> m_parentPath;

  //Members not stored in the db, used for sync ...
PRAGMA_DB (transient)
  bool m_synced;
  
private:
  friend class odb::access;
  
PRAGMA_DB (index member(m_path))
PRAGMA_DB (index member(m_content))
PRAGMA_DB (index member(m_scraper))
PRAGMA_DB (index member(m_hash))
PRAGMA_DB (index member(m_scanRecursive))
PRAGMA_DB (index member(m_noUpdate))
PRAGMA_DB (index member(m_exclude))
PRAGMA_DB (index member(m_dateAdded))
PRAGMA_DB (index member(m_parentPath))
};


#endif /* ODBPATH_H */
