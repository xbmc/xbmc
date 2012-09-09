#pragma once

/*
 *      Copyright (C) 2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "XBDateTime.h"

class CFileItemList;

namespace EPG
{
  class CEpgInfoTag;

  #define EPG_SEARCH_UNSET (-1)

  /** Filter to apply with on a CEpgInfoTag */

  struct EpgSearchFilter
  {
    static int FilterRecordings(CFileItemList &results);
    static int FilterTimers(CFileItemList &results);

    /*!
     * @brief Clear this filter.
     */
    virtual void Reset();

    /*!
     * @brief Check if a tag will be filtered or not.
     * @param tag The tag to check.
     * @return True if this tag matches the filter, false if not.
     */
    virtual bool FilterEntry(const CEpgInfoTag &tag) const;

    virtual bool MatchGenre(const CEpgInfoTag &tag) const;
    virtual bool MatchDuration(const CEpgInfoTag &tag) const;
    virtual bool MatchStartAndEndTimes(const CEpgInfoTag &tag) const;
    virtual bool MatchSearchTerm(const CEpgInfoTag &tag) const;
    virtual bool MatchChannelNumber(const CEpgInfoTag &tag) const;
    virtual bool MatchChannelGroup(const CEpgInfoTag &tag) const;

    static int RemoveDuplicates(CFileItemList &results);

    CStdString    m_strSearchTerm;            /*!< The term to search for */
    bool          m_bIsCaseSensitive;         /*!< Do a case sensitive search */
    bool          m_bSearchInDescription;     /*!< Search for strSearchTerm in the description too */
    int           m_iGenreType;               /*!< The genre type for an entry */
    int           m_iGenreSubType;            /*!< The genre subtype for an entry */
    int           m_iMinimumDuration;         /*!< The minimum duration for an entry */
    int           m_iMaximumDuration;         /*!< The maximum duration for an entry */
    CDateTime     m_startDateTime;            /*!< The minimum start time for an entry */
    CDateTime     m_endDateTime;              /*!< The maximum end time for an entry */
    bool          m_bIncludeUnknownGenres;    /*!< Include unknown genres or not */
    bool          m_bPreventRepeats;          /*!< True to remove repeating events, false if not */

    /* PVR specific filters */
    int           m_iChannelNumber;           /*!< The channel number in the selected channel group */
    bool          m_bFTAOnly;                 /*!< Free to air only or not */
    int           m_iChannelGroup;            /*!< The group this channel belongs to */
    bool          m_bIgnorePresentTimers;     /*!< True to ignore currently present timers (future recordings), false if not */
    bool          m_bIgnorePresentRecordings; /*!< True to ignore currently active recordings, false if not */
  };
}
