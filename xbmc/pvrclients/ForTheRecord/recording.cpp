/*
*      Copyright (C) 2011 Marcel Groothuis, Fho
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
#include "recording.h"
#include "client.h"

cRecording::cRecording(void)
{
  actors = "";
  category = "";
  channeldisplayname = "";
  channelid = "";
  channeltype = ForTheRecord::Television;
  description = "";
  director = "";
  episodenumber = 0;
  episodenumberdisplay = "";
  episodenumbertotal = 0;
  episodepart = 0;
  episodeparttotal = 0;
  ischanged = false;
  ispartofseries = false;
  ispartialrecording = false;
  ispremiere = false;
  isrepeat = false;
  keepuntilmode = ForTheRecord::UntilSpaceIsNeeded;
  keepuntilvalue = 0;
  lastwatchedposition = 0;
  lastwatchedtime = 0;
  programstarttime = 0;
  programstoptime = 0;
  rating = "";
  recordingfileformatid = "";
  recordingfilename = "";
  recordingid = "";
  recordingstarttime = 0;
  recordingstoptime = 0;
  scheduleid = "";
  schedulename = "";
  schedulepriority = ForTheRecord::Normal;
  seriesnumber = 0;
  starrating = 0.0;
  starttime = 0;
  stoptime = 0;
  subtitle = "";
  thumbnailfilename = "";
  title = "";
  videoaspect = ForTheRecord::Unknown;
}

cRecording::~cRecording(void)
{
}

bool cRecording::Parse(const Json::Value& data)
{
  int offset;
  std::string t;
  actors = data["Actors"].asString();
  category = data["Category"].asString();
  channeldisplayname = data["ChannelDisplayName"].asString();
  channelid = data["ChannelId"].asString();
  channeltype = (ForTheRecord::ChannelType) data["ChannelType"].asInt();
  description = data["Description"].asString();
  director = data["Director"].asString();
  episodenumber = data["EpisodeNumber"].asInt();
  episodenumberdisplay = data["EpisodeNumberDisplay"].asString();
  episodenumbertotal = data["EpisodeNumberTotal"].asInt();
  episodepart = data["EpisodePart"].asInt();
  episodeparttotal = data["EpisodePartTotal"].asInt();
  ischanged = data["IsChanged"].asBool();
  ispartofseries = data["IsPartOfSeries"].asBool();
  ispartialrecording = data["IsPartialRecording"].asBool();
  ispremiere = data["IsPremiere"].asBool();
  isrepeat = data["IsRepeat"].asBool();
  keepuntilmode = (ForTheRecord::KeepUntilMode) data["KeepUntilMode"].asInt();
  keepuntilvalue = data["KeepUntilValue"].asInt();
  lastwatchedposition = data["LastWatchedPosition"].asInt();
  t = data["LastWatchedTime"].asString();
  lastwatchedtime = ForTheRecord::WCFDateToTimeT(t, offset);
  t = data["ProgramStartTime"].asString();
  programstarttime = ForTheRecord::WCFDateToTimeT(t, offset);
  t = data["ProgramStopTime"].asString();
  programstoptime = ForTheRecord::WCFDateToTimeT(t, offset);
  rating = data["Rating"].asString();
  recordingfileformatid = data["RecordingFileFormatId"].asString();
  recordingfilename = data["RecordingFileName"].asString();
  recordingid = data["RecordingId"].asString();
  t = data["RecordingStartTime"].asString();
  recordingstarttime = ForTheRecord::WCFDateToTimeT(t, offset);
  t = data["RecordingStopTime"].asString();
  recordingstoptime = ForTheRecord::WCFDateToTimeT(t, offset);
  scheduleid = data["ScheduleId"].asString();
  schedulename = data["ScheduleName"].asString();
  schedulepriority = (ForTheRecord::SchedulePriority) data["SchedulePriority"].asInt();
  seriesnumber = data["SeriesNumber"].asInt();
  starrating = data["StarRating"].asDouble();
  t = data["StartTime"].asString();
  starttime = ForTheRecord::WCFDateToTimeT(t, offset);
  t = data["StopTime"].asString();
  stoptime = ForTheRecord::WCFDateToTimeT(t, offset);
  subtitle = data["SubTitle"].asString();
  thumbnailfilename = data["ThumbnailFileName"].asString();
  title = data["Title"].asString();
  videoaspect = (ForTheRecord::VideoAspectRatio) data["VideoAspect"].asInt();
  std::string CIFSname = recordingfilename;
  std::string SMBPrefix = "smb://";
  if (g_szUser.length() > 0)
  {
    SMBPrefix += g_szUser;
    if (g_szPass.length() > 0)
    {
      SMBPrefix += ":" + g_szPass;
    }
  }
  else
  {
    SMBPrefix += "Guest";
  }
  SMBPrefix += "@";
  size_t found;
  while ((found = CIFSname.find("\\")) != std::string::npos)
  {
    CIFSname.replace(found, 1, "/");
  }
  CIFSname.erase(0,2);
  CIFSname.insert(0, SMBPrefix);
  cifsrecordingfilename = CIFSname;  

  return true;
}
