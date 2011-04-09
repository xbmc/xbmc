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

#define MAXLIFETIME 99 //Based on VDR addon and VDR documentation. 99=Keep forever, 0=can be deleted at any time, 1..98=days to keep

class cRecordingSummary
{
private:
  std::string category;
  std::string channeldisplayname;
  std::string channelid;
  ForTheRecord::ChannelType channeltype;
  int episodenumber;
  std::string episodenumberdisplay;
  int episodenumbertotal;
  int episodepart;
  int episodeparttotal;
  bool ispartofseries;
  bool ispartialrecording;
  bool ispremiere;
  bool isrepeat;
  ForTheRecord::KeepUntilMode keepuntilmode;
  int keepuntilvalue;
  int lastwatchedposition;
  time_t lastwatchedtime;
  time_t programstarttime;
  time_t programstoptime;
  std::string rating;
  std::string recordingfileformatid;
  std::string recordingfilename;
  std::string recordingid;
  time_t recordingstarttime;
  time_t recordingstoptime;
  std::string scheduleid;
  std::string schedulename;
  ForTheRecord::SchedulePriority schedulepriority;
  int seriesnumber;
  double starrating;
  time_t starttime;
  time_t stoptime;
  std::string subtitle;
  std::string title;
  ForTheRecord::VideoAspectRatio videoaspect;

public:
  cRecordingSummary(void);
  virtual ~cRecordingSummary(void);

  bool Parse(const Json::Value& data);

  const char *Category(void) const { return category.c_str(); }
  const char *ChannelDisplayName(void) const { return channeldisplayname.c_str(); }
  const char *ChannelId(void) const { return channelid.c_str(); }
  ForTheRecord::ChannelType ChannelType(void) const { return channeltype; };
  int EpisodeNumber(void) const { return episodenumber; }
  const char *EpisodeNumberDisplay(void) const { return episodenumberdisplay.c_str(); }
  int EpisodeNumberTotal(void) const { return episodenumbertotal; }
  int EpisodePart(void) const { return episodepart; }
  int EpisodePartTotal(void) const { return episodeparttotal; }
  bool IsPartOfSeries(void) const { return ispartofseries; }
  bool IsPartialRecording(void) const { return ispartialrecording; }
  bool IsPremiere(void) const { return ispremiere; }
  bool IsRepeat(void) const { return isrepeat; }
  ForTheRecord::KeepUntilMode KeepUntilMode(void) const { return keepuntilmode; }
  int KeepUntilValue(void) const { return keepuntilvalue; }
  int LastWatchedPosition(void) const { return lastwatchedposition; }
  time_t LastWatchedTime(void) const { return lastwatchedtime; }
  time_t ProgramStartTime(void) const { return programstarttime; }
  time_t ProgramStopTime(void) const { return programstoptime; }
  const char *Rating(void) const { return rating.c_str(); }
  const char *RecordingFileFormatId(void) const { return recordingfileformatid.c_str(); }
  const char *RecordingFileName(void) const { return recordingfilename.c_str(); }
  const std::string& RecordingId(void) const { return recordingid; }
  time_t RecordingStartTime(void) const { return recordingstarttime; }
  time_t RecordingStopTime(void) const { return recordingstoptime; }
  const char *ScheduleId(void) const { return scheduleid.c_str(); }
  const char *ScheduleName(void) const { return schedulename.c_str(); }
  ForTheRecord::SchedulePriority SchedulePriority(void) const { return schedulepriority; }
  int SeriesNumber(void) const { return seriesnumber; }
  double StarRating(void) const { return starrating; }
  time_t StartTime(void) const { return starttime; }
  time_t StopTime(void) const { return stoptime; }
  const char *SubTitle(void) const { return subtitle.c_str(); }
  const char *Title(void) const { return title.c_str(); }
  ForTheRecord::VideoAspectRatio VideoAspect(void) const { return videoaspect; }
};
