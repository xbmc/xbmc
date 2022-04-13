/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "XBDateTime.h"
#include "pvr/channels/PVRChannelNumber.h"
#include "pvr/epg/EpgSearchData.h"

#include <memory>
#include <string>
#include <vector>

namespace PVR
{
  class CPVREpgInfoTag;

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
    bool FilterEntry(const std::shared_ptr<CPVREpgInfoTag>& tag) const;

    /*!
     * @brief remove duplicates from a list of epg tags.
     * @param results The list of epg tags.
     */
    static void RemoveDuplicates(std::vector<std::shared_ptr<CPVREpgInfoTag>>& results);

    /*!
     * @brief Get the type of channels to search.
     * @return true, if 'radio'. false, otherwise.
     */
    bool IsRadio() const { return m_bIsRadio; }

    const std::string& GetSearchTerm() const { return m_searchData.m_strSearchTerm; }
    void SetSearchTerm(const std::string& strSearchTerm)
    {
      m_searchData.m_strSearchTerm = strSearchTerm;
    }
    void SetSearchPhrase(const std::string& strSearchPhrase);

    bool IsCaseSensitive() const { return m_bIsCaseSensitive; }
    void SetCaseSensitive(bool bIsCaseSensitive) { m_bIsCaseSensitive = bIsCaseSensitive; }

    bool ShouldSearchInDescription() const { return m_searchData.m_bSearchInDescription; }
    void SetSearchInDescription(bool bSearchInDescription)
    {
      m_searchData.m_bSearchInDescription = bSearchInDescription;
    }

    int GetGenreType() const { return m_searchData.m_iGenreType; }
    void SetGenreType(int iGenreType) { m_searchData.m_iGenreType = iGenreType; }

    int GetMinimumDuration() const { return m_iMinimumDuration; }
    void SetMinimumDuration(int iMinimumDuration) { m_iMinimumDuration = iMinimumDuration; }

    int GetMaximumDuration() const { return m_iMaximumDuration; }
    void SetMaximumDuration(int iMaximumDuration) { m_iMaximumDuration = iMaximumDuration; }

    const CDateTime& GetStartDateTime() const { return m_searchData.m_startDateTime; }
    void SetStartDateTime(const CDateTime& startDateTime)
    {
      m_searchData.m_startDateTime = startDateTime;
    }

    const CDateTime& GetEndDateTime() const { return m_searchData.m_endDateTime; }
    void SetEndDateTime(const CDateTime& endDateTime) { m_searchData.m_endDateTime = endDateTime; }

    bool ShouldIncludeUnknownGenres() const { return m_searchData.m_bIncludeUnknownGenres; }
    void SetIncludeUnknownGenres(bool bIncludeUnknownGenres)
    {
      m_searchData.m_bIncludeUnknownGenres = bIncludeUnknownGenres;
    }

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

    unsigned int GetUniqueBroadcastId() const { return m_searchData.m_iUniqueBroadcastId; }
    void SetUniqueBroadcastId(unsigned int iUniqueBroadcastId)
    {
      m_searchData.m_iUniqueBroadcastId = iUniqueBroadcastId;
    }

    const PVREpgSearchData& GetEpgSearchData() const { return m_searchData; }
    void SetEpgSearchDataFiltered() { m_bEpgSearchDataFiltered = true; }

  private:
    bool MatchGenre(const std::shared_ptr<CPVREpgInfoTag>& tag) const;
    bool MatchDuration(const std::shared_ptr<CPVREpgInfoTag>& tag) const;
    bool MatchStartAndEndTimes(const std::shared_ptr<CPVREpgInfoTag>& tag) const;
    bool MatchSearchTerm(const std::shared_ptr<CPVREpgInfoTag>& tag) const;
    bool MatchChannelNumber(const std::shared_ptr<CPVREpgInfoTag>& tag) const;
    bool MatchChannelGroup(const std::shared_ptr<CPVREpgInfoTag>& tag) const;
    bool MatchBroadcastId(const std::shared_ptr<CPVREpgInfoTag>& tag) const;
    bool MatchChannelType(const std::shared_ptr<CPVREpgInfoTag>& tag) const;
    bool MatchFreeToAir(const std::shared_ptr<CPVREpgInfoTag>& tag) const;
    bool MatchTimers(const std::shared_ptr<CPVREpgInfoTag>& tag) const;
    bool MatchRecordings(const std::shared_ptr<CPVREpgInfoTag>& tag) const;

    PVREpgSearchData m_searchData;
    bool m_bEpgSearchDataFiltered = false;

    bool m_bIsCaseSensitive; /*!< Do a case sensitive search */
    int m_iMinimumDuration; /*!< The minimum duration for an entry */
    int m_iMaximumDuration; /*!< The maximum duration for an entry */
    bool m_bRemoveDuplicates; /*!< True to remove duplicate events, false if not */

    // PVR specific filters
    const bool m_bIsRadio; /*!< True to filter radio channels only, false to tv only */
    CPVRChannelNumber m_channelNumber; /*!< The channel number in the selected channel group */
    bool m_bFreeToAirOnly; /*!< Include free to air channels only */
    int m_iChannelGroup; /*!< The group this channel belongs to */
    bool m_bIgnorePresentTimers; /*!< True to ignore currently present timers (future recordings), false if not */
    bool m_bIgnorePresentRecordings; /*!< True to ignore currently active recordings, false if not */
  };
}
