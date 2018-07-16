/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "XBDateTime.h"

#include "pvr/PVRTypes.h"
#include "pvr/channels/PVRChannelNumber.h"

class CFileItemList;

namespace PVR
{
  #define EPG_SEARCH_UNSET (-1)

  /** Filter to apply with on a CPVREpgInfoTag */

  class CPVREpgSearchFilter
  {
  public:
    CPVREpgSearchFilter() = delete;

    /*!
     * @brief ctor.
     * @param bRadio the type of channels to search - if true, 'radio'. 'tv', otherwise.
     */
    CPVREpgSearchFilter(bool bRadio);

    /*!
     * @brief Clear this filter.
     */
    void Reset();

    /*!
     * @brief Check if a tag will be filtered or not.
     * @param tag The tag to check.
     * @return True if this tag matches the filter, false if not.
     */
    bool FilterEntry(const CPVREpgInfoTagPtr &tag) const;

    /*!
     * @brief remove duplicates from a list of epg tags.
     * @param results the list of epg tags.
     * @return the number of items in the list after removing duplicates.
     */
    static int RemoveDuplicates(CFileItemList &results);

    /*!
     * @brief Get the type of channels to search.
     * @return true, if 'radio'. false, otherwise.
     */
    bool IsRadio() const { return m_bIsRadio; }

    const std::string &GetSearchTerm() const { return m_strSearchTerm; }
    void SetSearchTerm(const std::string &strSearchTerm) { m_strSearchTerm = strSearchTerm; }
    void SetSearchPhrase(const std::string &strSearchPhrase);

    bool IsCaseSensitive() const { return m_bIsCaseSensitive; }
    void SetCaseSensitive(bool bIsCaseSensitive) { m_bIsCaseSensitive = bIsCaseSensitive; }

    bool ShouldSearchInDescription() const { return m_bSearchInDescription; }
    void SetSearchInDescription(bool bSearchInDescription) {m_bSearchInDescription = bSearchInDescription; }

    int GetGenreType() const { return m_iGenreType; }
    void SetGenreType(int iGenreType) { m_iGenreType = iGenreType; }

    int GetGenreSubType() const { return m_iGenreSubType; }
    void SetGenreSubType(int iGenreSubType) { m_iGenreSubType = iGenreSubType; }

    int GetMinimumDuration() const { return m_iMinimumDuration; }
    void SetMinimumDuration(int iMinimumDuration) { m_iMinimumDuration = iMinimumDuration; }

    int GetMaximumDuration() const { return m_iMaximumDuration; }
    void SetMaximumDuration(int iMaximumDuration) { m_iMaximumDuration = iMaximumDuration; }

    const CDateTime &GetStartDateTime() const { return m_startDateTime; }
    void SetStartDateTime(const CDateTime &startDateTime) { m_startDateTime = startDateTime; }

    const CDateTime &GetEndDateTime() const  { return m_endDateTime; }
    void SetEndDateTime(const CDateTime &endDateTime) { m_endDateTime = endDateTime; }

    bool ShouldIncludeUnknownGenres() const { return m_bIncludeUnknownGenres; }
    void SetIncludeUnknownGenres(bool bIncludeUnknownGenres) { m_bIncludeUnknownGenres = bIncludeUnknownGenres; }

    bool ShouldRemoveDuplicates() const { return m_bRemoveDuplicates; }
    void SetRemoveDuplicates(bool bRemoveDuplicates) { m_bRemoveDuplicates = bRemoveDuplicates; }

    const CPVRChannelNumber& GetChannelNumber() const { return m_channelNumber; }
    void SetChannelNumber(const CPVRChannelNumber& channelNumber) { m_channelNumber = channelNumber; }

    bool IsFreeToAirOnly() const { return m_bFreeToAirOnly; }
    void SetFreeToAirOnly(bool bFreeToAirOnly) { m_bFreeToAirOnly = bFreeToAirOnly; }

    int GetChannelGroup() const { return m_iChannelGroup; }
    void SetChannelGroup(int iChannelGroup) { m_iChannelGroup = iChannelGroup; }

    bool ShouldIgnorePresentTimers() const { return m_bIgnorePresentTimers; }
    void SetIgnorePresentTimers(bool bIgnorePresentTimers) { m_bIgnorePresentTimers = bIgnorePresentTimers; }

    bool ShouldIgnorePresentRecordings() const { return m_bIgnorePresentRecordings; }
    void SetIgnorePresentRecordings(bool bIgnorePresentRecordings) { m_bIgnorePresentRecordings = bIgnorePresentRecordings; }

    unsigned int GetUniqueBroadcastId() const { return m_iUniqueBroadcastId; }
    void SetUniqueBroadcastId(unsigned int iUniqueBroadcastId) { m_iUniqueBroadcastId = iUniqueBroadcastId; }

  private:
    bool MatchGenre(const CPVREpgInfoTagPtr &tag) const;
    bool MatchDuration(const CPVREpgInfoTagPtr &tag) const;
    bool MatchStartAndEndTimes(const CPVREpgInfoTagPtr &tag) const;
    bool MatchSearchTerm(const CPVREpgInfoTagPtr &tag) const;
    bool MatchChannelNumber(const CPVREpgInfoTagPtr &tag) const;
    bool MatchChannelGroup(const CPVREpgInfoTagPtr &tag) const;
    bool MatchBroadcastId(const CPVREpgInfoTagPtr &tag) const;
    bool MatchChannelType(const CPVREpgInfoTagPtr &tag) const;
    bool MatchFreeToAir(const CPVREpgInfoTagPtr &tag) const;
    bool MatchTimers(const CPVREpgInfoTagPtr &tag) const;
    bool MatchRecordings(const CPVREpgInfoTagPtr &tag) const;

    std::string   m_strSearchTerm;            /*!< The term to search for */
    bool          m_bIsCaseSensitive;         /*!< Do a case sensitive search */
    bool          m_bSearchInDescription;     /*!< Search for strSearchTerm in the description too */
    int           m_iGenreType;               /*!< The genre type for an entry */
    int           m_iGenreSubType;            /*!< The genre subtype for an entry */
    int           m_iMinimumDuration;         /*!< The minimum duration for an entry */
    int           m_iMaximumDuration;         /*!< The maximum duration for an entry */
    CDateTime     m_startDateTime;            /*!< The minimum start time for an entry */
    CDateTime     m_endDateTime;              /*!< The maximum end time for an entry */
    bool          m_bIncludeUnknownGenres;    /*!< Include unknown genres or not */
    bool          m_bRemoveDuplicates;        /*!< True to remove duplicate events, false if not */
    const bool    m_bIsRadio;                 /*!< True to filter radio channels only, false to tv only */

    /* PVR specific filters */
    CPVRChannelNumber  m_channelNumber;       /*!< The channel number in the selected channel group */
    bool          m_bFreeToAirOnly;           /*!< Include free to air channels only */
    int           m_iChannelGroup;            /*!< The group this channel belongs to */
    bool          m_bIgnorePresentTimers;     /*!< True to ignore currently present timers (future recordings), false if not */
    bool          m_bIgnorePresentRecordings; /*!< True to ignore currently active recordings, false if not */
    unsigned int  m_iUniqueBroadcastId;       /*!< The broadcastid to search for */
  };
}
