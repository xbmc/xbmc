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

#ifndef ODBSTACKTIME_H
#define ODBSTACKTIME_H

#include <odb/core.hxx>
#include <odb/lazy-ptr.hxx>

#include <string>

#include "ODBFile.h"

PRAGMA_DB (model version(1, 1, open))

PRAGMA_DB (object pointer(std::shared_ptr) \
                  table("stacktimes"))
class CODBStacktime
{
public:
  CODBStacktime()
  {
    m_times = "";
  };
  
PRAGMA_DB (id auto)
  unsigned long m_idStacktime;
  odb::lazy_shared_ptr<CODBFile> m_file;
  std::string m_times;
  
private:
  friend class odb::access;
  
PRAGMA_DB (index member(m_file))
  
};


#endif /* ODBSTACKTIME_H */
