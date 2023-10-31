/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "XBDateTime.h"
#include "pvr/epg/EpgSearchData.h"

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace PVR
{
  class CPVREpgInfoTag;

  class CPVREpgSearchFilter
  {
  public:
    CPVREpgSearchFilter() = delete;

    /*!
     * @brief ctor.
     * @param bRadio the type of channels to search - if true, 'radio'. 'tv', otherwise.
     */
    explicit CPVREpgSearchFilter(bool bRadio);

    /*!
     * @brief Clear this filter.
     */
    void Reset();

    /*!
     * @brief Return the path for this filter.
     * @return the path.
     */
    std::string GetPath() const;

    /*!
     * @brief Check if a tag will be filtered or not.
     * @param tag The tag to check.
     * @return True if this tag matches the filter, false if not.
     */
    bool FilterEntry(const std::shared_ptr<const CPVREpgInfoTag>& tag) const;

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
    void SetSearchTerm(const std::string& strSearchTerm);

    void SetSearchPhrase(const std::string& strSearchPhrase);

    bool IsCaseSensitive() const { return m_bIsCaseSensitive; }
    void SetCaseSensitive(bool bIsCaseSensitive);

    bool ShouldSearchInDescription() const { return m_searchData.m_bSearchInDescription; }
    void SetSearchInDescription(bool bSearchInDescription);

    int GetGenreType() const { return m_searchData.m_iGenreType; }
    void SetGenreType(int iGenreType);

    int GetMinimumDuration() const { return m_iMinimumDuration; }
    void SetMinimumDuration(int iMinimumDuration);

    int GetMaximumDuration() const { return m_iMaximumDuration; }
    void SetMaximumDuration(int iMaximumDuration);

    bool ShouldIgnoreFinishedBroadcasts() const { return m_searchData.m_bIgnoreFinishedBroadcasts; }
    void SetIgnoreFinishedBroadcasts(bool bIgnoreFinishedBroadcasts);

    bool ShouldIgnoreFutureBroadcasts() const { return m_searchData.m_bIgnoreFutureBroadcasts; }
    void SetIgnoreFutureBroadcasts(bool bIgnoreFutureBroadcasts);

    const CDateTime& GetStartDateTime() const { return m_searchData.m_startDateTime; }
    void SetStartDateTime(const CDateTime& startDateTime);

    const CDateTime& GetEndDateTime() const { return m_searchData.m_endDateTime; }
    void SetEndDateTime(const CDateTime& endDateTime);

    bool ShouldIncludeUnknownGenres() const { return m_searchData.m_bIncludeUnknownGenres; }
    void SetIncludeUnknownGenres(bool bIncludeUnknownGenres);

    bool ShouldRemoveDuplicates() const { return m_bRemoveDuplicates; }
    void SetRemoveDuplicates(bool bRemoveDuplicates);

    int GetClientID() const { return m_iClientID; }
    void SetClientID(int iClientID);

    int GetChannelGroupID() const { return m_iChannelGroupID; }
    void SetChannelGroupID(int iChannelGroupID);

    int GetChannelUID() const { return m_iChannelUID; }
    void SetChannelUID(int iChannelUID);

    bool IsFreeToAirOnly() const { return m_bFreeToAirOnly; }
    void SetFreeToAirOnly(bool bFreeToAirOnly);

    bool ShouldIgnorePresentTimers() const { return m_bIgnorePresentTimers; }
    void SetIgnorePresentTimers(bool bIgnorePresentTimers);

    bool ShouldIgnorePresentRecordings() const { return m_bIgnorePresentRecordings; }
    void SetIgnorePresentRecordings(bool bIgnorePresentRecordings);

    int GetDatabaseId() const { return m_iDatabaseId; }
    void SetDatabaseId(int iDatabaseId);

    const std::string& GetTitle() const { return m_title; }
    void SetTitle(const std::string& title);

    const CDateTime& GetLastExecutedDateTime() const { return m_lastExecutedDateTime; }
    void SetLastExecutedDateTime(const CDateTime& lastExecutedDateTime);

    const PVREpgSearchData& GetEpgSearchData() const { return m_searchData; }
    void SetEpgSearchDataFiltered() { m_bEpgSearchDataFiltered = true; }

    bool IsChanged() const { return m_bChanged; }
    void SetChanged(bool bChanged) { m_bChanged = bChanged; }

  private:
    bool MatchGenre(const std::shared_ptr<const CPVREpgInfoTag>& tag) const;
    bool MatchDuration(const std::shared_ptr<const CPVREpgInfoTag>& tag) const;
    bool MatchStartAndEndTimes(const std::shared_ptr<const CPVREpgInfoTag>& tag) const;
    bool MatchSearchTerm(const std::shared_ptr<const CPVREpgInfoTag>& tag) const;
    bool MatchChannel(const std::shared_ptr<const CPVREpgInfoTag>& tag) const;
    bool MatchChannelGroup(const std::shared_ptr<const CPVREpgInfoTag>& tag) const;
    bool MatchFreeToAir(const std::shared_ptr<const CPVREpgInfoTag>& tag) const;
    bool MatchTimers(const std::shared_ptr<const CPVREpgInfoTag>& tag) const;
    bool MatchRecordings(const std::shared_ptr<const CPVREpgInfoTag>& tag) const;

    bool m_bChanged = false;

    PVREpgSearchData m_searchData;
    bool m_bEpgSearchDataFiltered = false;

    bool m_bIsCaseSensitive; /*!< Do a case sensitive search */
    int m_iMinimumDuration; /*!< The minimum duration for an entry */
    int m_iMaximumDuration; /*!< The maximum duration for an entry */
    bool m_bRemoveDuplicates; /*!< True to remove duplicate events, false if not */

    // PVR specific filters
    bool m_bIsRadio; /*!< True to filter radio channels only, false to tv only */
    int m_iClientID = -1; /*!< The client id */
    int m_iChannelGroupID{-1}; /*! The channel group id */
    int m_iChannelUID = -1; /*!< The channel uid */
    bool m_bFreeToAirOnly; /*!< Include free to air channels only */
    bool m_bIgnorePresentTimers; /*!< True to ignore currently present timers (future recordings), false if not */
    bool m_bIgnorePresentRecordings; /*!< True to ignore currently active recordings, false if not */

    mutable std::optional<bool> m_groupIdMatches;

    int m_iDatabaseId = -1;
    std::string m_title;
    CDateTime m_lastExecutedDateTime;
  };
}
