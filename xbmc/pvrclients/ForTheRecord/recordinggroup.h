#pragma once
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

#include "libXBMC_pvr.h"
#include <string>
#include <json/json.h>
#include "fortherecordrpc.h"

class cRecordingGroup
{
private:
  std::string category;
  std::string channeldisplayname;
  std::string channelid;
  ForTheRecord::ChannelType channeltype;
  bool isrecording;
  time_t latestprogramstarttime;
  std::string programtitle;
  ForTheRecord::RecordingGroupMode recordinggroupmode;
  int recordingscount;
  std::string scheduleid;
  std::string schedulename;
  ForTheRecord::SchedulePriority schedulepriority;

public:
  cRecordingGroup(void);
  virtual ~cRecordingGroup(void);

  bool Parse(const Json::Value& data);

  const char *Category(void) const { return category.c_str(); }
  const char *ChannelDisplayName(void) const { return channeldisplayname.c_str(); }
  const char *ChannelID(void) const { return channelid.c_str(); }
  ForTheRecord::ChannelType ChannelType(void) const { return channeltype; }
  bool IsRecording(void) const { return isrecording; }
  time_t LatestProgramStartTime(void) const { return latestprogramstarttime; }
  const std::string& ProgramTitle(void) const { return programtitle; }
  ForTheRecord::RecordingGroupMode RecordingGroupMode(void) const { return recordinggroupmode; }
  int RecordingsCount(void) const { return recordingscount; }
  const char *ScheduleId(void) const { return scheduleid.c_str(); }
  const char *ScheduleName(void) const { return schedulename.c_str(); }
  ForTheRecord::SchedulePriority SchedulePriority(void) const { return schedulepriority; }
};
