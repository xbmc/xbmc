/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/kodi-dev-kit/include/kodi/c-api/addon-instance/pvr/pvr_timers.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

struct PVR_TIMER_TYPE;

namespace PVR
{
  class CPVRClient;

  static const int DEFAULT_RECORDING_PRIORITY = 50;
  static const int DEFAULT_RECORDING_LIFETIME = 99; // days
  static const unsigned int DEFAULT_RECORDING_DUPLICATEHANDLING = 0;

  class CPVRTimerType
  {
  public:
    /*!
     * @brief Return a list with all known timer types.
     * @return A list of timer types or an empty list if no types available.
     */
    static const std::vector<std::shared_ptr<CPVRTimerType>> GetAllTypes();

    /*!
     * @brief Return the first available timer type from given client.
     * @param client the PVR client.
     * @return A timer type or NULL if none available.
     */
    static const std::shared_ptr<CPVRTimerType> GetFirstAvailableType(
        const std::shared_ptr<const CPVRClient>& client);

    /*!
     * @brief Create a timer type from given timer type id and client id.
     * @param iTimerType the timer type id.
     * @param iClientId the PVR client id.
     * @return A timer type instance.
     */
    static std::shared_ptr<CPVRTimerType> CreateFromIds(unsigned int iTypeId, int iClientId);

    /*!
     * @brief Create a timer type from given timer type attributes and client id.
     * @param iMustHaveAttr a combination of PVR_TIMER_TYPE_* attributes the type to create must have.
     * @param iMustNotHaveAttr a combination of PVR_TIMER_TYPE_* attributes the type to create must not have.
     * @param iClientId the PVR client id.
     * @return A timer type instance.
     */
    static std::shared_ptr<CPVRTimerType> CreateFromAttributes(uint64_t iMustHaveAttr,
                                                               uint64_t iMustNotHaveAttr,
                                                               int iClientId);

    CPVRTimerType();
    CPVRTimerType(const PVR_TIMER_TYPE& type, int iClientId);
    CPVRTimerType(unsigned int iTypeId,
                  uint64_t iAttributes,
                  const std::string& strDescription = "");

    virtual ~CPVRTimerType();

    CPVRTimerType(const CPVRTimerType& type) = delete;
    CPVRTimerType& operator=(const CPVRTimerType& orig) = delete;

    bool operator ==(const CPVRTimerType& right) const;
    bool operator !=(const CPVRTimerType& right) const;

    /*!
     * @brief Update the data of this instance with the data given by another type instance.
     * @param type The instance containing the updated data.
     */
    void Update(const CPVRTimerType& type);

    /*!
     * @brief Get the PVR client id for this type.
     * @return The PVR client id.
     */
    int GetClientId() const { return m_iClientId; }

    /*!
     * @brief Get the numeric type id of this type.
     * @return The type id.
     */
    unsigned int GetTypeId() const { return m_iTypeId; }

    /*!
     * @brief Get the plain text (UI) description of this type.
     * @return The description.
     */
    const std::string& GetDescription() const { return m_strDescription; }

    /*!
     * @brief Get the attributes of this type.
     * @return The attributes.
     */
    uint64_t GetAttributes() const { return m_iAttributes; }

    /*!
     * @brief Check whether this type is for timer rules or one time timers.
     * @return True if type represents a timer rule, false otherwise.
     */
    bool IsTimerRule() const { return (m_iAttributes & PVR_TIMER_TYPE_IS_REPEATING) > 0; }

    /*!
     * @brief Check whether this type is for reminder timers or recording timers.
     * @return True if type represents a reminder timer, false otherwise.
     */
    bool IsReminder() const { return (m_iAttributes & PVR_TIMER_TYPE_IS_REMINDER) > 0; }

    /*!
     * @brief Check whether this type is for timer rules or one time timers.
     * @return True if type represents a one time timer, false otherwise.
     */
    bool IsOnetime() const { return !IsTimerRule(); }

    /*!
     * @brief Check whether this type is for epg-based or manual timers.
     * @return True if manual, false otherwise.
     */
    bool IsManual() const { return (m_iAttributes & PVR_TIMER_TYPE_IS_MANUAL) > 0; }

    /*!
     * @brief Check whether this type is for epg-based or manual timers.
     * @return True if epg-based, false otherwise.
     */
    bool IsEpgBased() const { return !IsManual(); }

    /*!
     * @brief Check whether this type is for epg-based timer rules.
     * @return True if epg-based timer rule, false otherwise.
     */
    bool IsEpgBasedTimerRule() const { return IsEpgBased() && IsTimerRule(); }

    /*!
     * @brief Check whether this type is for one time epg-based timers.
     * @return True if one time epg-based, false otherwise.
     */
    bool IsEpgBasedOnetime() const { return IsEpgBased() && IsOnetime(); }

    /*!
     * @brief Check whether this type is for manual timer rules.
     * @return True if manual timer rule, false otherwise.
     */
    bool IsManualTimerRule() const { return IsManual() && IsTimerRule(); }

    /*!
     * @brief Check whether this type is for one time manual timers.
     * @return True if one time manual, false otherwise.
     */
    bool IsManualOnetime() const { return IsManual() && IsOnetime(); }

    /*!
     * @brief Check whether this type is readonly (must not be modified after initial creation).
     * @return True if readonly, false otherwise.
     */
    bool IsReadOnly() const { return (m_iAttributes & PVR_TIMER_TYPE_IS_READONLY) > 0; }

    /*!
     * @brief Check whether this type allows deletion.
     * @return True if type allows deletion, false otherwise.
     */
    bool AllowsDelete() const { return !IsReadOnly() || SupportsReadOnlyDelete(); }

    /*!
     * @brief Check whether this type forbids creation of new timers of this type.
     * @return True if new instances are forbidden, false otherwise.
     */
    bool ForbidsNewInstances() const { return (m_iAttributes & PVR_TIMER_TYPE_FORBIDS_NEW_INSTANCES) > 0; }

    /*!
     * @brief Check whether this timer type is forbidden when epg tag info is present.
     * @return True if new instances are forbidden when epg info is present, false otherwise.
     */
    bool ForbidsEpgTagOnCreate() const { return (m_iAttributes & PVR_TIMER_TYPE_FORBIDS_EPG_TAG_ON_CREATE) > 0; }

    /*!
     * @brief Check whether this timer type requires epg tag info to be present.
     * @return True if new instances require EPG info, false otherwise.
     */
    bool RequiresEpgTagOnCreate() const { return (m_iAttributes & (PVR_TIMER_TYPE_REQUIRES_EPG_TAG_ON_CREATE |
                                                                   PVR_TIMER_TYPE_REQUIRES_EPG_SERIES_ON_CREATE |
                                                                   PVR_TIMER_TYPE_REQUIRES_EPG_SERIESLINK_ON_CREATE)) > 0; }

    /*!
     * @brief Check whether this timer type requires epg tag info including series attributes to be present.
     * @return True if new instances require an EPG tag with series attributes, false otherwise.
     */
    bool RequiresEpgSeriesOnCreate() const { return (m_iAttributes & PVR_TIMER_TYPE_REQUIRES_EPG_SERIES_ON_CREATE) > 0; }

    /*!
     * @brief Check whether this timer type requires epg tag info including a series link to be present.
     * @return True if new instances require an EPG tag with a series link, false otherwise.
     */
    bool RequiresEpgSeriesLinkOnCreate() const { return (m_iAttributes & PVR_TIMER_TYPE_REQUIRES_EPG_SERIESLINK_ON_CREATE) > 0; }

    /*!
     * @brief Check whether this type supports the "enabling/disabling" of timers of its type.
     * @return True if "enabling/disabling" feature is supported, false otherwise.
     */
    bool SupportsEnableDisable() const { return (m_iAttributes & PVR_TIMER_TYPE_SUPPORTS_ENABLE_DISABLE) > 0; }

    /*!
     * @brief Check whether this type supports channels.
     * @return True if channels are supported, false otherwise.
     */
    bool SupportsChannels() const { return (m_iAttributes & PVR_TIMER_TYPE_SUPPORTS_CHANNELS) > 0; }

    /*!
     * @brief Check whether this type supports start time.
     * @return True if start time values are supported, false otherwise.
     */
    bool SupportsStartTime() const { return (m_iAttributes & PVR_TIMER_TYPE_SUPPORTS_START_TIME) > 0; }

    /*!
     * @brief Check whether this type supports end time.
     * @return True if end time values are supported, false otherwise.
     */
    bool SupportsEndTime() const { return (m_iAttributes & PVR_TIMER_TYPE_SUPPORTS_END_TIME) > 0; }
    /*!
     * @brief Check whether this type supports start any time.
     * @return True if start any time is supported, false otherwise.
     */
    bool SupportsStartAnyTime() const { return (m_iAttributes & PVR_TIMER_TYPE_SUPPORTS_START_ANYTIME) > 0; }

    /*!
     * @brief Check whether this type supports end any time.
     * @return True if end any time is supported, false otherwise.
     */
    bool SupportsEndAnyTime() const { return (m_iAttributes & PVR_TIMER_TYPE_SUPPORTS_END_ANYTIME) > 0; }

    /*!
     * @brief Check whether this type supports matching a search string against epg episode title.
     * @return True if title matching is supported, false otherwise.
     */
    bool SupportsEpgTitleMatch() const { return (m_iAttributes & (PVR_TIMER_TYPE_SUPPORTS_TITLE_EPG_MATCH | PVR_TIMER_TYPE_SUPPORTS_FULLTEXT_EPG_MATCH)) > 0; }

    /*!
     * @brief Check whether this type supports matching a search string against extended (fulltext) epg data. This
              includes title matching.
     * @return True if fulltext matching is supported, false otherwise.
     */
    bool SupportsEpgFulltextMatch() const { return (m_iAttributes & PVR_TIMER_TYPE_SUPPORTS_FULLTEXT_EPG_MATCH) > 0; }

    /*!
     * @brief Check whether this type supports a first day the timer is active.
     * @return True if first day is supported, false otherwise.
     */
    bool SupportsFirstDay() const { return (m_iAttributes & PVR_TIMER_TYPE_SUPPORTS_FIRST_DAY) > 0; }

    /*!
     * @brief Check whether this type supports weekdays for timer schedules.
     * @return True if weekdays are supported, false otherwise.
     */
    bool SupportsWeekdays() const { return (m_iAttributes & PVR_TIMER_TYPE_SUPPORTS_WEEKDAYS) > 0; }

    /*!
     * @brief Check whether this type supports the "record only new episodes" feature.
     * @return True if the "record only new episodes" feature is supported, false otherwise.
     */
    bool SupportsRecordOnlyNewEpisodes() const { return (m_iAttributes & PVR_TIMER_TYPE_SUPPORTS_RECORD_ONLY_NEW_EPISODES) > 0; }

    /*!
     * @brief Check whether this type supports pre record time.
     * @return True if pre record time is supported, false otherwise.
     */
    bool SupportsStartMargin() const
    {
      return (m_iAttributes & PVR_TIMER_TYPE_SUPPORTS_START_MARGIN) > 0 ||
             (m_iAttributes & PVR_TIMER_TYPE_SUPPORTS_START_END_MARGIN) > 0;
    }

    /*!
     * @brief Check whether this type supports post record time.
     * @return True if post record time is supported, false otherwise.
     */
    bool SupportsEndMargin() const
    {
      return (m_iAttributes & PVR_TIMER_TYPE_SUPPORTS_END_MARGIN) > 0 ||
             (m_iAttributes & PVR_TIMER_TYPE_SUPPORTS_START_END_MARGIN) > 0;
    }

    /*!
     * @brief Check whether this type supports recording priorities.
     * @return True if recording priority is supported, false otherwise.
     */
    bool SupportsPriority() const { return (m_iAttributes & PVR_TIMER_TYPE_SUPPORTS_PRIORITY) > 0; }

    /*!
     * @brief Check whether this type supports lifetime for recordings.
     * @return True if recording lifetime is supported, false otherwise.
     */
    bool SupportsLifetime() const { return (m_iAttributes & PVR_TIMER_TYPE_SUPPORTS_LIFETIME) > 0; }

    /*!
     * @brief Check whether this type supports MaxRecordings for recordings.
     * @return True if MaxRecordings is supported, false otherwise.
     */
    bool SupportsMaxRecordings() const { return (m_iAttributes & PVR_TIMER_TYPE_SUPPORTS_MAX_RECORDINGS) > 0; }

    /*!
     * @brief Check whether this type supports user specified recording folders.
     * @return True if recording folders are supported, false otherwise.
     */
    bool SupportsRecordingFolders() const { return (m_iAttributes & PVR_TIMER_TYPE_SUPPORTS_RECORDING_FOLDERS) > 0; }

    /*!
     * @brief Check whether this type supports recording groups.
     * @return True if recording groups are supported, false otherwise.
     */
    bool SupportsRecordingGroup() const { return (m_iAttributes & PVR_TIMER_TYPE_SUPPORTS_RECORDING_GROUP) > 0; }

    /*!
     * @brief Check whether this type supports 'any channel', for example for defining a timer rule that should match any channel instead of a particular channel.
     * @return True if any channel is supported, false otherwise.
     */
    bool SupportsAnyChannel() const { return (m_iAttributes & PVR_TIMER_TYPE_SUPPORTS_ANY_CHANNEL) > 0; }

    /*!
     * @brief Check whether this type supports deletion of an otherwise read-only timer.
     * @return True if read-only deletion is supported, false otherwise.
     */
    bool SupportsReadOnlyDelete() const { return (m_iAttributes & PVR_TIMER_TYPE_SUPPORTS_READONLY_DELETE) > 0; }

    /*!
     * @brief Obtain a list with all possible values for the priority attribute.
     * @param list out, the list with the values or an empty list, if priority is not supported by this type.
     */
    void GetPriorityValues(std::vector<std::pair<std::string, int>>& list) const;

    /*!
     * @brief Obtain the default value for the priority attribute.
     * @return the default value.
     */
    int GetPriorityDefault() const { return m_iPriorityDefault; }

    /*!
     * @brief Obtain a list with all possible values for the lifetime attribute.
     * @param list out, the list with the values or an empty list, if lifetime is not supported by this type.
     */
    void GetLifetimeValues(std::vector<std::pair<std::string, int>>& list) const;

    /*!
     * @brief Obtain the default value for the lifetime attribute.
     * @return the default value.
     */
    int GetLifetimeDefault() const { return m_iLifetimeDefault; }

    /*!
     * @brief Obtain a list with all possible values for the MaxRecordings attribute.
     * @param list out, the list with the values or an empty list, if MaxRecordings is not supported by this type.
     */
    void GetMaxRecordingsValues(std::vector<std::pair<std::string, int>>& list) const;

    /*!
     * @brief Obtain the default value for the MaxRecordings attribute.
     * @return the default value.
     */
    int GetMaxRecordingsDefault() const { return m_iMaxRecordingsDefault; }

    /*!
     * @brief Obtain a list with all possible values for the duplicate episode prevention attribute.
     * @param list out, the list with the values or an empty list, if duplicate episode prevention is not supported by this type.
     */
    void GetPreventDuplicateEpisodesValues(std::vector<std::pair<std::string, int>>& list) const;

    /*!
     * @brief Obtain the default value for the duplicate episode prevention attribute.
     * @return the default value.
     */
    int GetPreventDuplicateEpisodesDefault() const { return m_iPreventDupEpisodesDefault; }

    /*!
     * @brief Obtain a list with all possible values for the recording group attribute.
     * @param list out, the list with the values or an empty list, if recording group is not supported by this type.
     */
    void GetRecordingGroupValues(std::vector<std::pair<std::string, int>>& list) const;

    /*!
     * @brief Obtain the default value for the Recording Group attribute.
     * @return the default value.
     */
    int GetRecordingGroupDefault() const { return m_iRecordingGroupDefault; }

  private:
    void InitDescription();
    void InitAttributeValues(const PVR_TIMER_TYPE& type);
    void InitPriorityValues(const PVR_TIMER_TYPE& type);
    void InitLifetimeValues(const PVR_TIMER_TYPE& type);
    void InitMaxRecordingsValues(const PVR_TIMER_TYPE& type);
    void InitPreventDuplicateEpisodesValues(const PVR_TIMER_TYPE& type);
    void InitRecordingGroupValues(const PVR_TIMER_TYPE& type);

    int m_iClientId = -1;
    unsigned int m_iTypeId;
    uint64_t m_iAttributes;
    std::string m_strDescription;
    std::vector< std::pair<std::string, int> > m_priorityValues;
    int m_iPriorityDefault = DEFAULT_RECORDING_PRIORITY;
    std::vector< std::pair<std::string, int> > m_lifetimeValues;
    int m_iLifetimeDefault = DEFAULT_RECORDING_LIFETIME;
    std::vector< std::pair<std::string, int> > m_maxRecordingsValues;
    int m_iMaxRecordingsDefault = 0;
    std::vector< std::pair<std::string, int> > m_preventDupEpisodesValues;
    unsigned int m_iPreventDupEpisodesDefault = DEFAULT_RECORDING_DUPLICATEHANDLING;
    std::vector< std::pair<std::string, int> > m_recordingGroupValues;
    unsigned int m_iRecordingGroupDefault = 0;
  };
}
