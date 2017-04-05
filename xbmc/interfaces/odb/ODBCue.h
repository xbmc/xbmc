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

#ifndef ODBCUE_H
#define ODBCUE_H

#include <odb/core.hxx>

#include "ODBFile.h"

#include <string>

PRAGMA_DB (model version(1, 1, open))

PRAGMA_DB (object pointer(std::shared_ptr) \
                  table("cue"))
class CODBCue
{
public:
  CODBCue()
  {
    m_synced = false;
  };
  
PRAGMA_DB (id auto)
  unsigned long m_idCue;
  odb::lazy_shared_ptr<CODBFile> m_file;
  std::string m_cuesheet;
  
  //Members not stored in the db, used for sync ...
PRAGMA_DB (transient)
  bool m_synced;
  
private:
  friend class odb::access;
};

PRAGMA_DB (view object(CODBCue) \
                object(CODBFile: CODBCue::m_file) \
                object(CODBPath: CODBFile::m_path) \
                query(distinct))
struct ODBView_Cue_Path
{
  std::shared_ptr<CODBCue> cue;
};

#endif /* ODBCUE_H */
