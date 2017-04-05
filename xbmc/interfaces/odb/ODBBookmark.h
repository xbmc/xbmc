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

#ifndef ODBBOOKMARK_H
#define ODBBOOKMARK_H

#include <odb/core.hxx>
#include <odb/lazy-ptr.hxx>

#include <string>

#include "ODBFile.h"

PRAGMA_DB (model version(1, 1, open))

PRAGMA_DB (object pointer(std::shared_ptr) \
                  table("bookmark"))
class CODBBookmark
{
public:
  CODBBookmark()
  {
    m_timeInSeconds = 0;
    m_totalTimeInSeconds = 0;
    m_thumbNailImage = "";
    m_player = "";
    m_type = 0;
    m_synced = false;
  };
  
PRAGMA_DB (id auto)
  unsigned long m_idBookmark;
  odb::lazy_shared_ptr<CODBFile> m_file;
  double m_timeInSeconds;
  double m_totalTimeInSeconds;
  std::string m_thumbNailImage;
  std::string m_player;
  std::string m_playerState;
  int m_type;
  
  //Members not stored in the db, used for sync ...
PRAGMA_DB (transient)
  bool m_synced;
  
private:
  friend class odb::access;
  
PRAGMA_DB (index member(m_file))
PRAGMA_DB (index member(m_type))
  
};

#endif /* ODBBOOKMARK_H */
