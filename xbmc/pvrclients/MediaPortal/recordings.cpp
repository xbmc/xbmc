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

#include "recordings.h"
#include "utils.h"

cRecording::cRecording()
{
  m_StartTime       = 0;
  m_Duration        = 0;
  m_Index           = -1;
  m_UTCdiff = GetUTCdifftime();
}

cRecording::cRecording(const PVR_RECORDINGINFO *Recording)
{

}

cRecording::~cRecording()
{
}

bool cRecording::ParseLine(const std::string& data)
{
  time_t endtime;
  struct tm timeinfo;
  int year, month ,day;
  int hour, minute, second;
  int count;

  vector<string> fields;

  Tokenize(data, fields, "|");

  if( fields.size() == 9 )
  {
    //[0] index / mediaportal recording id
    //[1] start time
    //[2] end time
    //[3] channel name
    //[4] title
    //[5] description
    //[6] stream_url
    //[7] filename (we can bypass rtsp streaming when XBMC and the TV server are on the same machine)
    //[8] lifetime (mediaportal keep until?)

    m_Index = atoi(fields[0].c_str());

    count = sscanf(fields[1].c_str(), "%d-%d-%d %d:%d:%d", &year, &month, &day, &hour, &minute, &second);

    if (count != 6)
      return false;

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

    m_StartTime = mktime (&timeinfo) + m_UTCdiff; //Start time in localtime

    if (m_StartTime < 0)
      return false;

    count = sscanf(fields[2].c_str(), "%d-%d-%d %d:%d:%d", &year, &month, &day, &hour, &minute, &second);

    if (count != 6)
      return false;

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

    endtime = mktime (&timeinfo) + m_UTCdiff; //Start time in localtime

    if (endtime < 0)
      return false;

    m_Duration = endtime - m_StartTime;

    m_channelName = fields[3];
    m_title = fields[4];
    m_description = fields[5];
    m_stream = fields[6];
    m_fileName = fields[7];
    m_lifetime = fields[8];

    return true;
  }
  else
  {
    return false;
  }
}
