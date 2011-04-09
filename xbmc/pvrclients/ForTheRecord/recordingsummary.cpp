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
#include "recordingsummary.h"

cRecordingSummary::cRecordingSummary(void)
{
  category = "";
  channeldisplayname = "";
  channelid = "";
  channeltype = ForTheRecord::Television;
  episodenumber = 0;
  episodenumberdisplay = "";
  episodenumbertotal = 0;
  episodepart = 0;
  episodeparttotal = 0;
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
  title = "";
  videoaspect = ForTheRecord::Unknown;
}

cRecordingSummary::~cRecordingSummary(void)
{
}

bool cRecordingSummary::Parse(const Json::Value& data)
{
  int offset;
  std::string t;
  category = data["Category"].asString();
  channeldisplayname = data["ChannelDisplayName"].asString();
  channelid = data["ChannelId"].asString();
  channeltype = (ForTheRecord::ChannelType) data["ChannelType"].asInt();
  episodenumber = data["EpisodeNumber"].asInt();
  episodenumberdisplay = data["EpisodeNumberDisplay"].asString();
  episodenumbertotal = data["EpisodeNumberTotal"].asInt();
  episodepart = data["EpisodePart"].asInt();
  episodeparttotal = data["EpisodePartTotal"].asInt();
  ispartofseries = data["IsPartOfSeries"].asBool();
  ispartialrecording = data["IsPartialRecording"].asBool();
  ispremiere = data["IsPremiere"].asBool();
  isrepeat = data["IsRepeat"].asBool();
  keepuntilmode = (ForTheRecord::KeepUntilMode) data["KeepUntilMode"].asInt();
  keepuntilvalue = data["KeepUntilValue"].asInt();
  lastwatchedposition = data["LastWatchedPosition"].asInt();
  t = data["LastWatchedTime"].asString();
  lastwatchedtime = ForTheRecord::WCFDateToTimeT(t, offset);
  lastwatchedtime += ((offset/100)*3600);
  t = data["ProgramStartTime"].asString();
  programstarttime = ForTheRecord::WCFDateToTimeT(t, offset);
  programstarttime += ((offset/100)*3600);
  t = data["ProgramStopTime"].asString();
  programstoptime = ForTheRecord::WCFDateToTimeT(t, offset);
  programstoptime += ((offset/100)*3600);
  rating = data["Rating"].asString();
  recordingfileformatid = data["RecordingFileFormatId"].asString();
  recordingfilename = data["RecordingFileName"].asString();
  recordingid = data["RecordingId"].asString();
  t = data["RecordingStartTime"].asString();
  recordingstarttime = ForTheRecord::WCFDateToTimeT(t, offset);
  recordingstarttime += ((offset/100)*3600);
  t = data["RecordingStopTime"].asString();
  recordingstoptime = ForTheRecord::WCFDateToTimeT(t, offset);
  recordingstoptime += ((offset/100)*3600);
  scheduleid = data["ScheduleId"].asString();
  schedulename = data["ScheduleName"].asString();
  schedulepriority = (ForTheRecord::SchedulePriority) data["SchedulePriority"].asInt();
  seriesnumber = data["SeriesNumber"].asInt();
  starrating = data["StarRating"].asDouble();
  t = data["StartTime"].asString();
  starttime = ForTheRecord::WCFDateToTimeT(t, offset);
  starttime += ((offset/100)*3600);
  t = data["StopTime"].asString();
  stoptime = ForTheRecord::WCFDateToTimeT(t, offset);
  stoptime += ((offset/100)*3600);
  subtitle = data["SubTitle"].asString();
  title = data["Title"].asString();
  videoaspect = (ForTheRecord::VideoAspectRatio) data["VideoAspect"].asInt();

  return true;
}