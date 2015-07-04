#pragma once
/*
 *      Copyright (C) 2012-2015 Team Kodi
 *      http://kodi.tv
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <memory>
#include <vector>
#include <string>

struct PVR_TIMER_TYPE;

namespace PVR
{
  class CPVRTimerType;
  typedef std::shared_ptr<CPVRTimerType> CPVRTimerTypePtr;

  class CPVRTimerType
  {
  public:

    /*!
     * @brief Return a list with all known timer types.
     * @return A list of timer types or an empty list if no types available.
     */
    static const std::vector<CPVRTimerTypePtr> GetAllTypes();

    /*!
     * @brief Return the first available timer type.
     * @return A timer type or NULL if none available.
     */
    static const CPVRTimerTypePtr GetFirstAvailableType();

    /*!
     * @brief Create a timer type from given timer type id and client id.
     * @param iTimerType the timer type id.
     * @param iClientId the PVR client id.
     * @return A timer type instance.
     */
    static CPVRTimerTypePtr CreateFromIds(unsigned int iTypeId, int iClientId);

    /*!
     * @brief Create a timer type from given timer type attributes and client id.
     * @param iMustHaveAttr a combination of PVR_TIMER_TYPE_* attributes the type to create must have.
     * @param iMustNotHaveAttr a combination of PVR_TIMER_TYPE_* attributes the type to create must not have.
     * @param iClientId the PVR client id.
     * @return A timer type instance.
     */
    static CPVRTimerTypePtr CreateFromAttributes(unsigned int iMustHaveAttr, unsigned int iMustNotHaveAttr, int iClientId);

    CPVRTimerType();
    CPVRTimerType(const PVR_TIMER_TYPE &type, int iClientId);

    virtual ~CPVRTimerType();

    CPVRTimerType(const CPVRTimerType &type) = delete;
    CPVRTimerType &operator=(const CPVRTimerType &orig) = delete;

    bool operator ==(const CPVRTimerType &right) const;
    bool operator !=(const CPVRTimerType &right) const;

    /*!
     * @brief Get the PVR client id for this type.
     * @return The PVR client id.
     */
    int GetClientId() const { return m_iClientId; }

    /*!
     * @brief Get the numeric type id of this type.
     * @return The type id.
     */
    int GetTypeId() const { return m_iTypeId; }

    /*!
     * @brief Get the plain text (UI) description of this type.
     * @return The description.
     */
    const std::string& GetDescription() const { return m_strDescription; }

    /*!
     * @brief Check whether this type is for repeating ore one time timers.
     * @return True if repeating, false otherwise.
     */
    bool IsRepeating() const { return (m_iAttributes & PVR_TIMER_TYPE_IS_REPEATING) > 0; }

    /*!
     * @brief Check whether this type is for repeating ore one time timers.
     * @return True if one time, false otherwise.
     */
    bool IsOnetime() const { return !IsRepeating(); }

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
     * @brief Check whether this type is for repeating epg-based timers.
     * @return True if repeating epg-based, false otherwise.
     */
    bool IsRepeatingEpgBased() const { return IsRepeating() && IsEpgBased(); }

    /*!
     * @brief Check whether this type is for one time epg-based timers.
     * @return True if one time epg-based, false otherwise.
     */
    bool IsOnetimeEpgBased() const { return IsOnetime() && IsEpgBased(); }

    /*!
     * @brief Check whether this type is for repeating manual timers.
     * @return True if repeating manual, false otherwise.
     */
    bool IsRepeatingManual() const { return IsRepeating() && IsManual(); }

    /*!
     * @brief Check whether this type is for one time manual timers.
     * @return True if one time manual, false otherwise.
     */
    bool IsOnetimeManual() const { return IsOnetime() && IsManual(); }

    /*!
     * @brief Check whether this type is readonly (must not be modified after initial creation).
     * @return True if readonly, false otherwise.
     */
    bool IsReadOnly() const { return (m_iAttributes & PVR_TIMER_TYPE_IS_READONLY) > 0; }

    /*!
     * @brief Check whether this type forbids creation of new timers of this type.
     * @return True if new instances are forbidden, false otherwise.
     */
    bool ForbidsNewInstances() const { return (m_iAttributes & PVR_TIMER_TYPE_FORBIDS_NEW_INSTANCES) > 0; }

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
     * @brief Check whether this type supports start time and end time.
     * @return True if start time and end time values is supported, false otherwise.
     */
    bool SupportsStartEndTime() const { return (m_iAttributes & PVR_TIMER_TYPE_SUPPORTS_START_END_TIME) > 0; }

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
     * @brief Check whether this type supports pre and post record time.
     * @return True if pre and post record time is supported, false otherwise.
     */
    bool SupportsStartEndMargin() const { return (m_iAttributes & PVR_TIMER_TYPE_SUPPORTS_START_END_MARGIN) > 0; }

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
     * @brief Obtain a list with all possible values for the priority attribute.
     * @param list out, the list with the values or an empty list, if priority is not supported by this type.
     */
    void GetPriorityValues(std::vector< std::pair<std::string, int> > &list) const;

    /*!
     * @brief Obtain the default value for the priority attribute.
     * @return the default value.
     */
    int GetPriorityDefault() const { return m_iPriorityDefault; }

    /*!
     * @brief Obtain a list with all possible values for the lifetime attribute.
     * @param list out, the list with the values or an empty list, if liftime is not supported by this type.
     */
    void GetLifetimeValues(std::vector< std::pair<std::string, int> > &list) const;

    /*!
     * @brief Obtain the default value for the lifetime attribute.
     * @return the default value.
     */
    int GetLifetimeDefault() const { return m_iLifetimeDefault; }

    /*!
     * @brief Obtain a list with all possible values for the duplicate episode prevention attribute.
     * @param list out, the list with the values or an empty list, if duplicate episode prevention is not supported by this type.
     */
    void GetPreventDuplicateEpisodesValues(std::vector< std::pair<std::string, int> > &list) const;

    /*!
     * @brief Obtain the default value for the duplicate episode prevention attribute.
     * @return the default value.
     */
    int GetPreventDuplicateEpisodesDefault() const { return m_iPreventDupEpisodesDefault; }

    /*!
     * @brief Obtain a list with all possible values for the recording group attribute.
     * @param list out, the list with the values or an empty list, if recording group is not supported by this type.
     */
    void GetRecordingGroupValues(std::vector< std::pair<std::string, int> > &list) const;

    /*!
     * @brief Obtain the default value for the Recording Group attribute.
     * @return the default value.
     */
    int GetRecordingGroupDefault() const { return m_iRecordingGroupDefault; }


  private:
    void InitAttributeValues(const PVR_TIMER_TYPE &type);
    void InitPriorityValues(const PVR_TIMER_TYPE &type);
    void InitLifetimeValues(const PVR_TIMER_TYPE &type);
    void InitPreventDuplicateEpisodesValues(const PVR_TIMER_TYPE &type);
    void InitRecordingGroupValues(const PVR_TIMER_TYPE &type);

    int           m_iClientId;
    unsigned int  m_iTypeId;
    unsigned int  m_iAttributes;
    std::string   m_strDescription;
    std::vector< std::pair<std::string, int> > m_priorityValues;
    int           m_iPriorityDefault;
    std::vector< std::pair<std::string, int> > m_lifetimeValues;
    int           m_iLifetimeDefault;
    std::vector< std::pair<std::string, int> > m_preventDupEpisodesValues;
    unsigned int  m_iPreventDupEpisodesDefault;
    std::vector< std::pair<std::string, int> > m_recordingGroupValues;
    unsigned int  m_iRecordingGroupDefault;
  };
}
