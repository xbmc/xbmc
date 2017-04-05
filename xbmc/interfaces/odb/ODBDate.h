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

#ifndef ODBDATE_H
#define ODBDATE_H

#include <odb/core.hxx>

#include <string>

#include "../../linux/PlatformDefs.h"

PRAGMA_DB (model version(1, 1, open))

PRAGMA_DB (value)
class CODBDate
{
public:
  CODBDate()
  {
    m_ulong_date = 0;
    m_year = 0;
  }
  
  /*
   * ulong_date = CDateTime::GetAsULongLong()
   * date = CDateTime::GetAsDBDateTime()
   */
  CODBDate(uint64_t ulong_date, std::string date)
  {
    m_ulong_date = ulong_date;
    m_date = date;
    if (!date.empty() && date.length() >= 4)
      m_year = std::stoi(date.substr(0,4));
    else
      m_year = 0;
  }
  
  void setDateTime(uint64_t ulong_date, std::string date)
  {
    m_ulong_date = ulong_date;
    m_date = date;
    if (!date.empty() && date.length() >= 4)
      m_year = std::stoi(date.substr(0,4));
    else
      m_year = 0;
  }
  
  void clear()
  {
    m_date = "";
    m_ulong_date = 0;
    m_year = 0;
  }
  
  uint64_t m_ulong_date;
  int m_year;
PRAGMA_DB (type("VARCHAR(20)"))
  std::string m_date;
  
private:
  friend class odb::access;
  
};


#endif /* ODBDATE_H */
