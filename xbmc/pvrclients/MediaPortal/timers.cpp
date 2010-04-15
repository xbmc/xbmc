/*
 *      Copyright (C) 2005-2010 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include <vector>
#include <stdio.h>

using namespace std;

#include "timers.h"
#include "utils.h"
/*
#define SECSINDAY  86400
*/

cTimer::cTimer()
{
  m_starttime   = 0;
  m_stoptime    = 0;
  m_index       = 0;
  m_UTCdiff     = GetUTCdifftime();
}

cTimer::~cTimer()
{
}

time_t cTimer::StartTime(void) const
{
  return m_starttime;
}

time_t cTimer::StopTime(void) const
{
  return m_stoptime;
}

bool cTimer::ParseLine(const char *s)
{
  struct tm timeinfo;
  int year, month ,day;
  int hour, minute, second;
  int count;

  vector<string> schedulefields;
  string data = s;
  uri::decode(data);

  Tokenize(data, schedulefields, "|");

  if(schedulefields.size() >= 7)
  {
    // field 0 = index
    // field 1 = start date + time
    // field 2 = end   date + time
    // field 3 = channel nr
    // field 4 = channel name
    // field 5 = program name
    // field 6 = repeat info
    // field 7 = priority

    m_index = atoi(schedulefields[0].c_str());

    count = sscanf(schedulefields[1].c_str(), "%d-%d-%d %d:%d:%d", &year, &month, &day, &hour, &minute, &second);

    if(count != 6)
      return false;

    //timeinfo = *localtime ( &rawtime );
    timeinfo.tm_hour = hour;
    timeinfo.tm_min = minute;
    timeinfo.tm_sec = second;
    timeinfo.tm_year = year - 1900;
    timeinfo.tm_mon = month - 1;
    timeinfo.tm_mday = day;
    // Make the other fields empty:
    timeinfo.tm_isdst = 0;
    timeinfo.tm_wday = 0;
    timeinfo.tm_yday = 0;

    m_starttime = mktime (&timeinfo) + m_UTCdiff; //m_StartTime should be localtime, MP TV returns UTC

    if( m_starttime < 0)
      return false;

    count = sscanf(schedulefields[2].c_str(), "%d-%d-%d %d:%d:%d", &year, &month, &day, &hour, &minute, &second);

    if( count != 6)
      return false;

    //timeinfo2 = *localtime ( &rawtime );
    timeinfo.tm_hour = hour;
    timeinfo.tm_min = minute;
    timeinfo.tm_sec = second;
    timeinfo.tm_year = year - 1900;
    timeinfo.tm_mon = month - 1;
    timeinfo.tm_mday = day;
    // Make the other fields empty:
    timeinfo.tm_isdst = 0;
    timeinfo.tm_wday = 0;
    timeinfo.tm_yday = 0;

    m_stoptime = mktime (&timeinfo) + m_UTCdiff; //m_EndTime should be localtime, MP TV returns UTC

    if( m_stoptime < 0)
      return false;

    m_channel = atoi(schedulefields[3].c_str());
    m_title = schedulefields[5];
    m_priority = atoi(schedulefields[7].c_str());

    return true;
  }
  return false;
}
