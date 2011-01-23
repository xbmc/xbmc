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

#include "LocalizeStrings.h"

#include "PVREpgInfoTag.h"
#include "PVRTimers.h"
#include "PVRTimerInfoTag.h"

using namespace std;

CPVREpgInfoTag::CPVREpgInfoTag(int iUniqueBroadcastId)
{
  Reset();
  m_iUniqueBroadcastID = iUniqueBroadcastId;
}

CPVREpgInfoTag::~CPVREpgInfoTag()
{
  m_Epg           = NULL;
  m_nextEvent     = NULL;
  m_previousEvent = NULL;
}

void CPVREpgInfoTag::Reset()
{
  m_iBroadcastId        = -1;
  m_strTitle            = g_localizeStrings.Get(19055);
  m_strGenre            = "";
  m_strPlotOutline      = "";
  m_strPlot             = "";
  m_iGenreType          = 0;
  m_iGenreSubType       = 0;
  m_strFileNameAndPath  = "";
  m_strIconPath         = "";
  m_isRecording         = false;
  m_Timer               = NULL;
  m_Epg                 = NULL;
  m_iParentalRating     = 0;
  m_iStarRating         = 0;
  m_notify              = false;
  m_seriesNum           = "";
  m_episodeNum          = "";
  m_episodePart         = "";
  m_episodeName         = "";
}

void CPVREpgInfoTag::SetTimer(const CPVRTimerInfoTag *Timer)
{
  if (!Timer)
    m_Timer = NULL;

  m_Timer = Timer;
}

bool CPVREpgInfoTag::HasTimer(void) const
{
  for (unsigned int i = 0; i < PVRTimers.size(); ++i)
  {
    if (PVRTimers[i].EpgInfoTag() == this)
      return true;
  }
  return false;
}

int CPVREpgInfoTag::GetDuration() const
{
  time_t start, end;
  m_startTime.GetAsTime(start);
  m_endTime.GetAsTime(end);
  return end - start > 0 ? end - start : 3600;
}

void CPVREpgInfoTag::SetGenre(int ID, int subID)
{
  m_iGenreType    = ID;
  m_iGenreSubType = subID;
  m_strGenre      = ConvertGenreIdToString(ID, subID);
}

const CPVREpgInfoTag *CPVREpgInfoTag::GetNextEvent() const
{
  m_Epg->Sort();

  return m_nextEvent;
}

const CPVREpgInfoTag *CPVREpgInfoTag::GetPreviousEvent() const
{
  m_Epg->Sort();

  return m_previousEvent;
}

void CPVREpgInfoTag::SetStart(CDateTime Start)
{
  m_startTime = Start;
  UpdatePath();
}

void CPVREpgInfoTag::UpdatePath()
{
  if (!m_Epg)
    return;

  CStdString path;
  path.Format("pvr://guide/channel-%04i/%s.epg", m_Epg->Channel()->ChannelNumber(), m_startTime.GetAsDBDateTime().c_str());
  SetPath(path);
}

CStdString CPVREpgInfoTag::ConvertGenreIdToString(int ID, int subID) const
{
  CStdString str = g_localizeStrings.Get(19499);
  switch (ID)
  {
    case EVCONTENTMASK_MOVIEDRAMA:
      if (subID <= 8)
        str = g_localizeStrings.Get(19500 + subID);
      else
        str = g_localizeStrings.Get(19500) + " (undefined)";
      break;
    case EVCONTENTMASK_NEWSCURRENTAFFAIRS:
      if (subID <= 4)
        str = g_localizeStrings.Get(19516 + subID);
      else
        str = g_localizeStrings.Get(19516) + " (undefined)";
      break;
    case EVCONTENTMASK_SHOW:
      if (subID <= 3)
        str = g_localizeStrings.Get(19532 + subID);
      else
        str = g_localizeStrings.Get(19532) + " (undefined)";
      break;
    case EVCONTENTMASK_SPORTS:
      if (subID <= 0x0B)
        str = g_localizeStrings.Get(19548 + subID);
      else
        str = g_localizeStrings.Get(19548) + " (undefined)";
      break;
    case EVCONTENTMASK_CHILDRENYOUTH:
      if (subID <= 5)
        str = g_localizeStrings.Get(19564 + subID);
      else
        str = g_localizeStrings.Get(19564) + " (undefined)";
      break;
    case EVCONTENTMASK_MUSICBALLETDANCE:
      if (subID <= 6)
        str = g_localizeStrings.Get(19580 + subID);
      else
        str = g_localizeStrings.Get(19580) + " (undefined)";
      break;
    case EVCONTENTMASK_ARTSCULTURE:
      if (subID <= 0x0B)
        str = g_localizeStrings.Get(19596 + subID);
      else
        str = g_localizeStrings.Get(19596) + " (undefined)";
      break;
    case EVCONTENTMASK_SOCIALPOLITICALECONOMICS:
      if (subID <= 0x03)
        str = g_localizeStrings.Get(19612 + subID);
      else
        str = g_localizeStrings.Get(19612) + " (undefined)";
      break;
    case EVCONTENTMASK_EDUCATIONALSCIENCE:
      if (subID <= 0x07)
        str = g_localizeStrings.Get(19628 + subID);
      else
        str = g_localizeStrings.Get(19628) + " (undefined)";
      break;
    case EVCONTENTMASK_LEISUREHOBBIES:
      if (subID <= 0x07)
        str = g_localizeStrings.Get(19644 + subID);
      else
        str = g_localizeStrings.Get(19644) + " (undefined)";
      break;
    case EVCONTENTMASK_SPECIAL:
      if (subID <= 0x03)
        str = g_localizeStrings.Get(19660 + subID);
      else
        str = g_localizeStrings.Get(19660) + " (undefined)";
      break;
    case EVCONTENTMASK_USERDEFINED:
      if (subID <= 0x03)
        str = g_localizeStrings.Get(19676 + subID);
      else
        str = g_localizeStrings.Get(19676) + " (undefined)";
      break;
    default:
      break;
  }
  return str;
}

void CPVREpgInfoTag::Update(const CPVREpgInfoTag &tag)
{
  SetBroadcastId(tag.BroadcastId());
  SetTitle(tag.Title());
  SetPlotOutline(tag.PlotOutline());
  SetPlot(tag.Plot());
  SetStart(tag.Start());
  SetEnd(tag.End());
  SetGenre(tag.GenreType(), tag.GenreSubType());
  SetFirstAired(tag.FirstAired());
  SetParentalRating(tag.ParentalRating());
  SetStarRating(tag.StarRating());
  SetNotify(tag.Notify());
  SetEpisodeNum(tag.EpisodeNum());
  SetEpisodePart(tag.EpisodePart());
  SetEpisodeName(tag.EpisodeName());
}

void CPVREpgInfoTag::Update(const PVR_PROGINFO *data)
{
  SetStart((time_t)data->starttime);
  SetEnd((time_t)data->endtime);
  SetTitle(data->title);
  SetPlotOutline(data->subtitle);
  SetPlot(data->description);
  SetGenre(data->genre_type, data->genre_sub_type);
  SetParentalRating(data->parental_rating);
//  SetIcon(m_Channel->IconPath());
}
