#pragma once
/*
 *      Copyright (C) 2011 Marcel Groothuis, FHo
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

class cGuideProgram
{
private:
  std::string actors;
  std::string category;
  std::string description;
  std::string directors;
  int episodenumber;
  std::string episodenumberdisplay;
  int episodenumbertotal;
  int episodepart;
  int episodeparttotal;
  std::string guidechannelid;
  std::string guideprogramid;
  bool ischanged;
  bool isdeleted;
  bool ispremiere;
  bool isrepeat;
  time_t lastmodifiedtime;
  std::string rating;
  int seriesnumber;
  double starrating;
  time_t starttime;
  time_t stoptime;
  std::string subtitle;
  std::string title;
  ForTheRecord::VideoAspectRatio videoaspect;
public:
  cGuideProgram(void);
  virtual ~cGuideProgram(void);

  bool Parse(const Json::Value& data);

  const char *Actors(void) const { return actors.c_str(); }
  const char *Category(void) const { return category.c_str(); }
  const char *Description(void) const { return description.c_str(); }
  const char *Directors(void) const { return directors.c_str(); }
  int EpisodeNumber(void) const { return episodenumber; }
  const char *EpisodeNumberDisplay(void) const { return episodenumberdisplay.c_str(); }
  int EpisodeNumberTotal(void) const { return episodenumbertotal; }
  int EpisodePart(void) const { return episodepart; }
  int EpisodePartTotal(void) const { return episodeparttotal; }
  const std::string& GuideChannelId(void) const { return guidechannelid; }
  const std::string& GuideProgramId(void) const { return guideprogramid; }
  bool IsChanged(void) const { return ischanged; }
  bool IsDeleted(void) const { return isdeleted; }
  bool IsPremiere(void) const { return ispremiere; }
  bool IsRepeat(void) const { return isrepeat; }
  const char *Rating(void) const { return rating.c_str(); }
  int SeriesNumber(void) const { return seriesnumber; }
  double StarRating(void) const { return starrating; }
  time_t StartTime(void) const { return starttime; }
  time_t StopTime(void) const { return stoptime; }
  const char *SubTitle(void) const { return subtitle.c_str(); }
  const char *Title(void) const { return title.c_str(); }
  ForTheRecord::VideoAspectRatio VideoAspect(void) const { return videoaspect; }
};
