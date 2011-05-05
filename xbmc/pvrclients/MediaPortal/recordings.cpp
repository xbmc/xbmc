/*
 *      Copyright (C) 2005-2010 Team XBMC
 *      http://www.xbmc.org
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
  //m_UTCdiff = GetUTCdifftime();
}

cRecording::cRecording(const PVR_RECORDING *Recording)
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
  string filePath;

  vector<string> fields;

  Tokenize(data, fields, "|");

  if( fields.size() >= 9 )
  {
    //[0] index / mediaportal recording id
    //[1] start time
    //[2] end time
    //[3] channel name
    //[4] title
    //[5] description
    //[6] stream_url (resolved hostname if requested)
    //[7] filename (we can bypass rtsp streaming when XBMC and the TV server are on the same machine)
    //[8] lifetime (mediaportal keep until?)
    //[9] (optional) original stream_url when resolve hostnames is enabled

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

    m_StartTime = mktime (&timeinfo); // + m_UTCdiff; //Start time in localtime

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

    endtime = mktime (&timeinfo); // + m_UTCdiff; //Start time in localtime

    if (endtime < 0)
      return false;

    m_Duration = endtime - m_StartTime;

    m_channelName = fields[3];
    m_title = fields[4];
    m_description = fields[5];
    m_stream = fields[6];
    m_filePath = fields[7];

    // TODO: fill lifetime with data from MP TV Server
    // From the VDR documentation (VDR is used by Alwinus as basis for the XBMC
    // PVR framework:
    // "The lifetime (int) value corresponds to the the number of days (0..99)
    // a recording made through this timer is guaranteed to remain on disk
    // before it is automatically removed to free up space for a new recording.
    // Note that setting this parameter to very high values for all recordings
    // may soon fill up the entire disk and cause new recordings to fail due to
    // low disk space. The special value 99 means that this recording will live
    // forever, and a value of 0 means that this recording can be deleted any
    // time if a recording with a higher priority needs disk space."
    m_lifetime = fields[8];

    if( m_filePath.length() > 0 )
    {
      size_t found = m_filePath.find_last_of("/\\");
      if (found != string::npos)
      {
        m_fileName = m_filePath.substr(found+1);
        m_directory = m_filePath.substr(0, found+1);
      }
      else
      {
        m_fileName = m_filePath;
        m_directory = "";
      }
    }
    else
    {
      m_fileName = "";
      m_directory = "";
    }


    if (fields.size() == 10) // Since 1.0.8.0
    {
      m_originalurl = fields[9];
    } else {
      m_originalurl = fields[6];
    }

    return true;
  }
  else
  {
    return false;
  }
}

void cRecording::SetDirectory( string& directory )
{
  m_directory = directory;
  m_filePath = m_directory + m_fileName;
}
