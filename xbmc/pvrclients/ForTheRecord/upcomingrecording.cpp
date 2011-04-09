/*
 *      Copyright (C) 2011 Fred Hoogduin
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
#include "utils.h"
#include <stdlib.h>
#include <string.h>
#include "upcomingrecording.h"

cUpcomingRecording::cUpcomingRecording(void)
{
  channeldisplayname = "";
  channelid = "";
  date = 0;
  starttime = 0;
  stoptime = 0;
  title = "";
  isactive = false;
  isrecording = false;
}

cUpcomingRecording::~cUpcomingRecording(void)
{
}
bool cUpcomingRecording::Parse(const Json::Value& data)
{
  int offset;
  std::string t;

  date = 0;
  t = data["StartTime"].asString();
  starttime = ForTheRecord::WCFDateToTimeT(t, offset);
  starttime += ((offset/100)*3600);
  t = data["StopTime"].asString();
  stoptime = ForTheRecord::WCFDateToTimeT(t, offset);
  stoptime += ((offset/100)*3600);
  prerecordseconds = data["PreRecordSeconds"].asInt();
  postrecordseconds = data["PostRecordSeconds"].asInt();
  title = data["Title"].asString();
  isactive = true;
  isrecording = false;
  upcomingprogramid = data["UpcomingProgramId"].asString();
  scheduleid = data["ScheduleId"].asString();

  // From the Program class pickup the C# Channel class
  Json::Value channelobject;
  channelobject = data["Channel"];

  // And -finally- our channel id
  channelid = channelobject["ChannelId"].asString();
  channeldisplayname = channelobject["DisplayName"].asString();

  return true;
}