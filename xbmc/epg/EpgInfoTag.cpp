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
#include "../addons/include/xbmc_pvr_types.h"
#include "EpgInfoTag.h"

using namespace std;

CEpgInfoTag::CEpgInfoTag(int iUniqueBroadcastId)
{
  Reset();
  m_iUniqueBroadcastID = iUniqueBroadcastId;
}

CEpgInfoTag::~CEpgInfoTag()
{
  m_Epg           = NULL;
  m_nextEvent     = NULL;
  m_previousEvent = NULL;
}

void CEpgInfoTag::Reset()
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
  m_Epg                 = NULL;
  m_iParentalRating     = 0;
  m_iStarRating         = 0;
  m_bNotify             = false;
  m_strSeriesNum        = "";
  m_strEpisodeNum       = "";
  m_strEpisodePart      = "";
  m_strEpisodeName      = "";
}

int CEpgInfoTag::GetDuration() const
{
  time_t start, end;
  m_startTime.GetAsTime(start);
  m_endTime.GetAsTime(end);
  return end - start > 0 ? end - start : 3600;
}

void CEpgInfoTag::SetGenre(int iID, int iSubID)
{
  m_iGenreType    = iID;
  m_iGenreSubType = iSubID;
  m_strGenre      = ConvertGenreIdToString(iID, iSubID);
}

const CEpgInfoTag *CEpgInfoTag::GetNextEvent() const
{
  m_Epg->Sort();

  return m_nextEvent;
}

const CEpgInfoTag *CEpgInfoTag::GetPreviousEvent() const
{
  m_Epg->Sort();

  return m_previousEvent;
}

void CEpgInfoTag::SetStart(const CDateTime &start)
{
  m_startTime = start;
  UpdatePath();
}

CStdString CEpgInfoTag::ConvertGenreIdToString(int iID, int iSubID) const
{
  CStdString str = g_localizeStrings.Get(19499);
  switch (iID)
  {
    case EVCONTENTMASK_MOVIEDRAMA:
      if (iSubID <= 8)
        str = g_localizeStrings.Get(19500 + iSubID);
      else
        str = g_localizeStrings.Get(19500) + " (undefined)";
      break;
    case EVCONTENTMASK_NEWSCURRENTAFFAIRS:
      if (iSubID <= 4)
        str = g_localizeStrings.Get(19516 + iSubID);
      else
        str = g_localizeStrings.Get(19516) + " (undefined)";
      break;
    case EVCONTENTMASK_SHOW:
      if (iSubID <= 3)
        str = g_localizeStrings.Get(19532 + iSubID);
      else
        str = g_localizeStrings.Get(19532) + " (undefined)";
      break;
    case EVCONTENTMASK_SPORTS:
      if (iSubID <= 0x0B)
        str = g_localizeStrings.Get(19548 + iSubID);
      else
        str = g_localizeStrings.Get(19548) + " (undefined)";
      break;
    case EVCONTENTMASK_CHILDRENYOUTH:
      if (iSubID <= 5)
        str = g_localizeStrings.Get(19564 + iSubID);
      else
        str = g_localizeStrings.Get(19564) + " (undefined)";
      break;
    case EVCONTENTMASK_MUSICBALLETDANCE:
      if (iSubID <= 6)
        str = g_localizeStrings.Get(19580 + iSubID);
      else
        str = g_localizeStrings.Get(19580) + " (undefined)";
      break;
    case EVCONTENTMASK_ARTSCULTURE:
      if (iSubID <= 0x0B)
        str = g_localizeStrings.Get(19596 + iSubID);
      else
        str = g_localizeStrings.Get(19596) + " (undefined)";
      break;
    case EVCONTENTMASK_SOCIALPOLITICALECONOMICS:
      if (iSubID <= 0x03)
        str = g_localizeStrings.Get(19612 + iSubID);
      else
        str = g_localizeStrings.Get(19612) + " (undefined)";
      break;
    case EVCONTENTMASK_EDUCATIONALSCIENCE:
      if (iSubID <= 0x07)
        str = g_localizeStrings.Get(19628 + iSubID);
      else
        str = g_localizeStrings.Get(19628) + " (undefined)";
      break;
    case EVCONTENTMASK_LEISUREHOBBIES:
      if (iSubID <= 0x07)
        str = g_localizeStrings.Get(19644 + iSubID);
      else
        str = g_localizeStrings.Get(19644) + " (undefined)";
      break;
    case EVCONTENTMASK_SPECIAL:
      if (iSubID <= 0x03)
        str = g_localizeStrings.Get(19660 + iSubID);
      else
        str = g_localizeStrings.Get(19660) + " (undefined)";
      break;
    case EVCONTENTMASK_USERDEFINED:
      if (iSubID <= 0x03)
        str = g_localizeStrings.Get(19676 + iSubID);
      else
        str = g_localizeStrings.Get(19676) + " (undefined)";
      break;
    default:
      break;
  }
  return str;
}

void CEpgInfoTag::Update(const CEpgInfoTag &tag)
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
