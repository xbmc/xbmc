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
#include "guideprogram.h"

cGuideProgram::cGuideProgram(void)
{
  actors = "";
  category = "";
  description = "";
  directors = "";
  episodenumber = 0;
  episodenumberdisplay = "";
  episodenumbertotal = 0;
  episodepart = 0;
  episodeparttotal = 0;
  guidechannelid = "";
  guideprogramid = "";
  ischanged = false;
  isdeleted = false;
  ispremiere = false;
  isrepeat = false;
  lastmodifiedtime = 0;
  rating = "";
  seriesnumber = 0;
  starrating = 0.0;
  starttime = 0;
  stoptime = 0;
  subtitle = "";
  title = "";
  videoaspect = ForTheRecord::Unknown;
}

cGuideProgram::~cGuideProgram(void)
{
}

bool cGuideProgram::Parse(const Json::Value& data)
{
  int offset;
  std::string t;
  //actors = data["Actors"].   .asString();
  category = data["Category"].asString();
  description = data["Description"].asString();
  //directors = data["Directors"].asString();
  bool zz = data["Directors"].isArray();
  episodenumber = data["EpisodeNumber"].asInt();
  episodenumberdisplay = data["EpisodeNumberDisplay"].asString();
  episodenumbertotal = data["EpisodeNumberTotal"].asInt();
  episodepart = data["EpisodePart"].asInt();
  episodeparttotal = data["EpisodePartTotal"].asInt();
  guidechannelid = data["GuideChannelId"].asString();
  guideprogramid = data["GuideProgramId"].asString();
  ischanged = data["IsChanged"].asBool();
  isdeleted = data["IsDeleted"].asBool();
  ispremiere = data["IsPremiere"].asBool();
  isrepeat = data["IsRepeat"].asBool();
  t = data["LastModifiedTime"].asString();
  lastmodifiedtime = ForTheRecord::WCFDateToTimeT(t, offset);
  lastmodifiedtime += ((offset/100)*3600);
  rating = data["Rating"].asString();
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