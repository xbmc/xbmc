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

#include "guilib/LocalizeStrings.h"
#include "../addons/include/xbmc_pvr_types.h"
#include "EpgInfoTag.h"
#include "EpgContainer.h"
#include "EpgDatabase.h"
#include "utils/log.h"

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

bool CEpgInfoTag::operator ==(const CEpgInfoTag& right) const
{
  if (this == &right) return true;

  return (m_iBroadcastId       == right.m_iBroadcastId &&
          m_strTitle           == right.m_strTitle &&
          m_strPlotOutline     == right.m_strPlotOutline &&
          m_strPlot            == right.m_strPlot &&
          m_strGenre           == right.m_strGenre &&
          m_startTime          == right.m_startTime &&
          m_endTime            == right.m_endTime &&
          m_strIconPath        == right.m_strIconPath &&
          m_strFileNameAndPath == right.m_strFileNameAndPath &&
          m_iGenreType         == right.m_iGenreType &&
          m_iGenreSubType      == right.m_iGenreSubType &&
          m_firstAired         == right.m_firstAired &&
          m_iParentalRating    == right.m_iParentalRating &&
          m_iStarRating        == right.m_iStarRating &&
          m_bNotify            == right.m_bNotify &&
          m_iSeriesNumber      == right.m_iSeriesNumber &&
          m_iEpisodeNumber     == right.m_iEpisodeNumber &&
          m_iEpisodePart       == right.m_iEpisodePart &&
          m_iUniqueBroadcastID == right.m_iUniqueBroadcastID);
}

bool CEpgInfoTag::operator !=(const CEpgInfoTag& right) const
{
  if (this == &right) return false;

  return !(*this == right);
}

void CEpgInfoTag::Reset()
{
  m_iBroadcastId        = -1;
  m_strTitle            = "";
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
  m_iSeriesNumber       = 0;
  m_iEpisodeNumber      = 0;
  m_iEpisodePart        = 0;
  m_strEpisodeName      = "";
  m_bChanged            = false;
  m_iUniqueBroadcastID  = -1;
}

int CEpgInfoTag::GetDuration() const
{
  time_t start, end;
  m_startTime.GetAsTime(start);
  m_endTime.GetAsTime(end);
  return end - start > 0 ? end - start : 3600;
}

const CStdString &CEpgInfoTag::Title(void) const
{
  return (m_strTitle.IsEmpty()) ?
      g_localizeStrings.Get(19055) :
      m_strTitle;
}

const CEpgInfoTag *CEpgInfoTag::GetNextEvent() const
{
  return m_nextEvent;
}

const CEpgInfoTag *CEpgInfoTag::GetPreviousEvent() const
{
  return m_previousEvent;
}

const CStdString &CEpgInfoTag::ConvertGenreIdToString(int iID, int iSubID) const
{
  unsigned int iLabelId = 19499;
  switch (iID)
  {
    case EPG_EVENT_CONTENTMASK_MOVIEDRAMA:
      iLabelId = (iSubID <= 8) ? 19500 + iSubID : 19500;
      break;
    case EPG_EVENT_CONTENTMASK_NEWSCURRENTAFFAIRS:
      iLabelId = (iSubID <= 4) ? 19516 + iSubID : 19516;
      break;
    case EPG_EVENT_CONTENTMASK_SHOW:
      iLabelId = (iSubID <= 3) ? 19532 + iSubID : 19532;
      break;
    case EPG_EVENT_CONTENTMASK_SPORTS:
      iLabelId = (iSubID <= 11) ? 19548 + iSubID : 19548;
      break;
    case EPG_EVENT_CONTENTMASK_CHILDRENYOUTH:
      iLabelId = (iSubID <= 5) ? 19564 + iSubID : 19564;
      break;
    case EPG_EVENT_CONTENTMASK_MUSICBALLETDANCE:
      iLabelId = (iSubID <= 6) ? 19580 + iSubID : 19580;
      break;
    case EPG_EVENT_CONTENTMASK_ARTSCULTURE:
      iLabelId = (iSubID <= 11) ? 19596 + iSubID : 19596;
      break;
    case EPG_EVENT_CONTENTMASK_SOCIALPOLITICALECONOMICS:
      iLabelId = (iSubID <= 3) ? 19612 + iSubID : 19612;
      break;
    case EPG_EVENT_CONTENTMASK_EDUCATIONALSCIENCE:
      iLabelId = (iSubID <= 7) ? 19628 + iSubID : 19628;
      break;
    case EPG_EVENT_CONTENTMASK_LEISUREHOBBIES:
      iLabelId = (iSubID <= 7) ? 19644 + iSubID : 19644;
      break;
    case EPG_EVENT_CONTENTMASK_SPECIAL:
      iLabelId = (iSubID <= 3) ? 19660 + iSubID : 19660;
      break;
    case EPG_EVENT_CONTENTMASK_USERDEFINED:
      iLabelId = (iSubID <= 3) ? 19676 + iSubID : 19676;
      break;
    default:
      break;
  }

  return g_localizeStrings.Get(iLabelId);
}

void CEpgInfoTag::SetUniqueBroadcastID(int iUniqueBroadcastID)
{
  if (m_iUniqueBroadcastID != iUniqueBroadcastID)
  {
    m_iUniqueBroadcastID = iUniqueBroadcastID;
    m_bChanged = true;
    UpdatePath();
  }
}

void CEpgInfoTag::SetBroadcastId(int iId)
{
  if (m_iBroadcastId != iId)
  {
    m_iBroadcastId = iId;
    m_bChanged = true;
    UpdatePath();
  }
}

const CDateTime &CEpgInfoTag::StartAsLocalTime(void) const
{
  static CDateTime tmp;
  tmp.SetFromUTCDateTime(m_startTime);

  return tmp;
}

void CEpgInfoTag::SetStartFromUTC(const CDateTime &start)
{
  if (m_startTime != start)
  {
    m_startTime = start;
    m_bChanged = true;
    UpdatePath();
  }
}

void CEpgInfoTag::SetStartFromLocalTime(const CDateTime &start)
{
  CDateTime tmp = start.GetAsUTCDateTime();
  SetStartFromUTC(tmp);
}

const CDateTime &CEpgInfoTag::EndAsLocalTime(void) const
{
  static CDateTime tmp;
  tmp.SetFromUTCDateTime(m_endTime);

  return tmp;
}

void CEpgInfoTag::SetEndFromUTC(const CDateTime &end)
{
  if (m_endTime != end)
  {
    m_endTime = end;
    m_bChanged = true;
    UpdatePath();
  }
}

void CEpgInfoTag::SetEndFromLocalTime(const CDateTime &end)
{
  CDateTime tmp = end.GetAsUTCDateTime();
  SetEndFromUTC(tmp);
}

void CEpgInfoTag::SetTitle(const CStdString &strTitle)
{
  if (m_strTitle != strTitle)
  {
    m_strTitle = strTitle;
    m_bChanged = true;
    UpdatePath();
  }
}

void CEpgInfoTag::SetPlotOutline(const CStdString &strPlotOutline)
{
  if (m_strPlotOutline != strPlotOutline)
  {
    m_strPlotOutline = strPlotOutline;
    m_bChanged = true;
    UpdatePath();
  }
}

void CEpgInfoTag::SetPlot(const CStdString &strPlot)
{
  if (m_strPlot != strPlot)
  {
    m_strPlot = strPlot;
    m_bChanged = true;
    UpdatePath();
  }
}

void CEpgInfoTag::SetGenre(int iID, int iSubID)
{
  if (m_iGenreType != iID || m_iGenreSubType != iSubID)
  {
    m_iGenreType    = iID;
    m_iGenreSubType = iSubID;
    m_strGenre      = ConvertGenreIdToString(iID, iSubID);
    m_bChanged = true;
    UpdatePath();
  }
}

const CDateTime &CEpgInfoTag::FirstAiredAsLocalTime(void) const
{
  static CDateTime tmp;
  tmp.SetFromUTCDateTime(m_firstAired);

  return tmp;
}

void CEpgInfoTag::SetFirstAiredFromUTC(const CDateTime &firstAired)
{
  if (m_firstAired != firstAired)
  {
    m_firstAired = firstAired;
    m_bChanged = true;
    UpdatePath();
  }
}

void CEpgInfoTag::SetFirstAiredFromLocalTime(const CDateTime &firstAired)
{
  CDateTime tmp = firstAired.GetAsUTCDateTime();
  SetStartFromUTC(tmp);
}

void CEpgInfoTag::SetParentalRating(int iParentalRating)
{
  if (m_iParentalRating != iParentalRating)
  {
    m_iParentalRating = iParentalRating;
    m_bChanged = true;
    UpdatePath();
  }
}

void CEpgInfoTag::SetStarRating(int iStarRating)
{
  if (m_iStarRating != iStarRating)
  {
    m_iStarRating = iStarRating;
    m_bChanged = true;
    UpdatePath();
  }
}

void CEpgInfoTag::SetNotify(bool bNotify)
{
  if (m_bNotify != bNotify)
  {
    m_bNotify = bNotify;
    m_bChanged = true;
    UpdatePath();
  }
}

void CEpgInfoTag::SetSeriesNum(int iSeriesNum)
{
  if (m_iSeriesNumber != iSeriesNum)
  {
    m_iSeriesNumber = iSeriesNum;
    m_bChanged = true;
    UpdatePath();
  }
}

void CEpgInfoTag::SetEpisodeNum(int iEpisodeNum)
{
  if (m_iEpisodeNumber != iEpisodeNum)
  {
    m_iEpisodeNumber = iEpisodeNum;
    m_bChanged = true;
    UpdatePath();
  }
}

void CEpgInfoTag::SetEpisodePart(int iEpisodePart)
{
  if (m_iEpisodePart != iEpisodePart)
  {
    m_iEpisodePart = iEpisodePart;
    m_bChanged = true;
    UpdatePath();
  }
}

void CEpgInfoTag::SetEpisodeName(const CStdString &strEpisodeName)
{
  if (m_strEpisodeName != strEpisodeName)
  {
    m_strEpisodeName = strEpisodeName;
    m_bChanged = true;
    UpdatePath();
  }
}

void CEpgInfoTag::SetIcon(const CStdString &strIconPath)
{
  if (m_strIconPath != strIconPath)
  {
    m_strIconPath = strIconPath;
    m_bChanged = true;
    UpdatePath();
  }
}

void CEpgInfoTag::SetPath(const CStdString &strFileNameAndPath)
{
  if (m_strFileNameAndPath != strFileNameAndPath)
  {
    m_strFileNameAndPath = strFileNameAndPath;
    m_bChanged = true;
  }
}

bool CEpgInfoTag::Update(const CEpgInfoTag &tag)
{
  bool bChanged = (
      m_iBroadcastId       != tag.m_iBroadcastId ||
      m_strTitle           != tag.m_strTitle ||
      m_strPlotOutline     != tag.m_strPlotOutline ||
      m_strPlot            != tag.m_strPlot ||
      m_startTime          != tag.m_startTime ||
      m_endTime            != tag.m_endTime ||
      m_iGenreType         != tag.m_iGenreType ||
      m_iGenreSubType      != tag.m_iGenreSubType ||
      m_firstAired         != tag.m_firstAired ||
      m_iParentalRating    != tag.m_iParentalRating ||
      m_iStarRating        != tag.m_iStarRating ||
      m_bNotify            != tag.m_bNotify ||
      m_iEpisodeNumber     != tag.m_iEpisodeNumber ||
      m_iEpisodePart       != tag.m_iEpisodePart ||
      m_iSeriesNumber      != tag.m_iSeriesNumber ||
      m_strEpisodeName     != tag.m_strEpisodeName ||
      m_iUniqueBroadcastID != tag.m_iUniqueBroadcastID
  );

  if (bChanged)
  {
    m_iBroadcastId       = tag.m_iBroadcastId;
    m_strTitle           = tag.m_strTitle;
    m_strPlotOutline     = tag.m_strPlotOutline;
    m_strPlot            = tag.m_strPlot;
    m_startTime          = tag.m_startTime;
    m_endTime            = tag.m_endTime;
    m_iGenreType         = tag.m_iGenreType;
    m_iGenreSubType      = tag.m_iGenreSubType;
    m_strGenre           = ConvertGenreIdToString(tag.m_iGenreType, tag.m_iGenreSubType);
    m_firstAired         = tag.m_firstAired;
    m_iParentalRating    = tag.m_iParentalRating;
    m_iStarRating        = tag.m_iStarRating;
    m_bNotify            = tag.m_bNotify;
    m_iEpisodeNumber     = tag.m_iEpisodeNumber;
    m_iEpisodePart       = tag.m_iEpisodePart;
    m_iSeriesNumber      = tag.m_iSeriesNumber;
    m_strEpisodeName     = tag.m_strEpisodeName;
    m_iUniqueBroadcastID = tag.m_iUniqueBroadcastID;

    m_bChanged = true;
    UpdatePath();
  }

  return bChanged;
}

bool CEpgInfoTag::IsActive(void) const
{
  CDateTime now = CDateTime::GetCurrentDateTime().GetAsUTCDateTime();
  return (m_startTime <= now && m_endTime > now);
}

bool CEpgInfoTag::Persist(bool bSingleUpdate /* = true */, bool bLastUpdate /* = false */)
{
  bool bReturn = false;

  if (!m_bChanged)
    return true;

  CEpgDatabase *database = g_EpgContainer.GetDatabase();
  if (!database || !database->Open())
  {
    CLog::Log(LOGERROR, "%s - could not open the database", __FUNCTION__);
    return bReturn;
  }

  int iId = database->Persist(*this, bSingleUpdate, bLastUpdate);
  if (iId >= 0)
  {
    bReturn = true;

    if (iId > 0)
    {
      m_iBroadcastId = iId;
      m_bChanged = false;
    }
  }

  database->Close();

  return bReturn;
}
