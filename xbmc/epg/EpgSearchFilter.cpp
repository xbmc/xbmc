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
#include "TextSearch.h"
#include "../addons/include/xbmc_pvr_types.h"

#include "EpgSearchFilter.h"
#include "EpgContainer.h"

using namespace std;

void EpgSearchFilter::Reset()
{
  m_strSearchTerm            = "";
  m_bIsCaseSensitive         = false;
  m_bSearchInDescription     = false;
  m_iGenreType               = -1;
  m_iGenreSubType            = -1;
  m_iMinimumDuration         = -1;
  m_iMaximumDuration         = -1;
  g_EpgContainer.GetFirstEPGDate().GetAsSystemTime(m_startDate);
  m_startDate.wHour          = 0;
  m_startDate.wMinute        = 0;
  g_EpgContainer.GetLastEPGDate().GetAsSystemTime(m_endDate);
  m_endDate.wHour            = 23;
  m_endDate.wMinute          = 59;
  m_startTime                = m_startDate;
  m_startTime.wHour          = 0;
  m_startTime.wMinute        = 0;
  m_endTime                  = m_endDate;
  m_endTime.wHour            = 23;
  m_endTime.wMinute          = 59;
  m_bIncludeUnknownGenres    = true;
  m_bIgnorePresentTimers     = true;
  m_bIgnorePresentRecordings = true;
  m_bPreventRepeats          = false;
}

bool EpgSearchFilter::FilterEntry(const CEpgInfoTag &tag) const
{
  if (m_iGenreType != -1)
  {
    if (tag.GenreType() != m_iGenreType &&
        (!m_bIncludeUnknownGenres &&
        ((tag.GenreType() < EPG_EVENT_CONTENTMASK_USERDEFINED || tag.GenreType() >= EPG_EVENT_CONTENTMASK_MOVIEDRAMA))))
    {
      return false;
    }
  }
  if (m_iMinimumDuration != -1)
  {
    if (tag.GetDuration() < (m_iMinimumDuration*60))
      return false;
  }
  if (m_iMaximumDuration != -1)
  {
    if (tag.GetDuration() > (m_iMaximumDuration*60))
      return false;
  }

  int timeTag = (tag.StartAsLocalTime().GetHour()*60 + tag.StartAsLocalTime().GetMinute());

  if (timeTag < (m_startTime.wHour*60 + m_startTime.wMinute))
    return false;

  if (timeTag > (m_endTime.wHour*60 + m_endTime.wMinute))
    return false;

  if (tag.StartAsLocalTime() < m_startDate)
    return false;

  if (tag.StartAsLocalTime() > m_endDate)
    return false;

  if (m_strSearchTerm != "")
  {
    cTextSearch search(tag.Title(), m_strSearchTerm, m_bIsCaseSensitive);
    if (!search.DoSearch())
    {
      if (m_bSearchInDescription)
      {
        search.SetText(tag.PlotOutline(), m_strSearchTerm, m_bIsCaseSensitive);
        if (!search.DoSearch())
        {
          search.SetText(tag.Plot(), m_strSearchTerm, m_bIsCaseSensitive);
          if (!search.DoSearch())
            return false;
        }
      }
      else
        return false;
    }
  }
  return true;
}
