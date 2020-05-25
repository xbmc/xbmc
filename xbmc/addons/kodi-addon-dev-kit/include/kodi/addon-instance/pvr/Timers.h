/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "General.h"
#include "../../AddonBase.h"
#include "../../c-api/addon-instance/pvr.h"

#ifdef __cplusplus

namespace kodi
{
namespace addon
{

class PVRTimer : public CStructHdl<PVRTimer, PVR_TIMER>
{
public:
  PVRTimer()
  {
    m_cStructure->iClientIndex = 0;
    m_cStructure->state = PVR_TIMER_STATE_NEW;
    m_cStructure->iTimerType = PVR_TIMER_TYPE_NONE;
    m_cStructure->iParentClientIndex = 0;
    m_cStructure->iClientChannelUid = PVR_TIMER_VALUE_NOT_AVAILABLE;
    m_cStructure->startTime = 0;
    m_cStructure->endTime = 0;
    m_cStructure->bStartAnyTime = false;
    m_cStructure->bEndAnyTime = false;
    m_cStructure->bFullTextEpgSearch = false;
    m_cStructure->iPriority = PVR_TIMER_VALUE_NOT_AVAILABLE;
    m_cStructure->iLifetime = PVR_TIMER_VALUE_NOT_AVAILABLE;
    m_cStructure->iMaxRecordings = PVR_TIMER_VALUE_NOT_AVAILABLE;
    m_cStructure->iRecordingGroup = 0;
    m_cStructure->firstDay = 0;
    m_cStructure->iWeekdays = PVR_WEEKDAY_NONE;
    m_cStructure->iPreventDuplicateEpisodes = 0;
    m_cStructure->iEpgUid = 0;
    m_cStructure->iMarginStart = 0;
    m_cStructure->iMarginEnd = 0;
    m_cStructure->iGenreType = PVR_TIMER_VALUE_NOT_AVAILABLE;
    m_cStructure->iGenreSubType = PVR_TIMER_VALUE_NOT_AVAILABLE;
  }
  PVRTimer(const PVRTimer& data) : CStructHdl(data) {}
  PVRTimer(const PVR_TIMER* data) : CStructHdl(data) {}
  PVRTimer(PVR_TIMER* data) : CStructHdl(data) {}

  void SetClientIndex(unsigned int clientIndex) { m_cStructure->iClientIndex = clientIndex; }
  unsigned int GetClientIndex() const { return m_cStructure->iClientIndex; }

  void SetState(PVR_TIMER_STATE state) { m_cStructure->state = state; }
  PVR_TIMER_STATE GetState() const { return m_cStructure->state; }

  void SetTimerType(unsigned int timerType) { m_cStructure->iTimerType = timerType; }
  unsigned int GetTimerType() const { return m_cStructure->iTimerType; }

  void SetTitle(const std::string& title)
  {
    strncpy(m_cStructure->strTitle, title.c_str(), sizeof(m_cStructure->strTitle) - 1);
  }
  std::string GetTitle() const { return m_cStructure->strTitle; }

  void SetParentClientIndex(unsigned int parentClientIndex)
  {
    m_cStructure->iParentClientIndex = parentClientIndex;
  }
  unsigned int GetParentClientIndex() const { return m_cStructure->iParentClientIndex; }

  void SetClientChannelUid(int clientChannelUid)
  {
    m_cStructure->iClientChannelUid = clientChannelUid;
  }
  int GetClientChannelUid() const { return m_cStructure->iClientChannelUid; }

  void SetStartTime(time_t startTime) { m_cStructure->startTime = startTime; }
  time_t GetStartTime() const { return m_cStructure->startTime; }

  void SetEndTime(time_t endTime) { m_cStructure->endTime = endTime; }
  time_t GetEndTime() const { return m_cStructure->endTime; }

  void SetStartAnyTime(bool startAnyTime) { m_cStructure->bStartAnyTime = startAnyTime; }
  bool GetStartAnyTime() const { return m_cStructure->bStartAnyTime; }

  void SetEndAnyTime(bool endAnyTime) { m_cStructure->bEndAnyTime = endAnyTime; }
  bool GetEndAnyTime() const { return m_cStructure->bEndAnyTime; }

  void SetEPGSearchString(const std::string& epgSearchString)
  {
    strncpy(m_cStructure->strEpgSearchString, epgSearchString.c_str(),
            sizeof(m_cStructure->strEpgSearchString) - 1);
  }
  std::string GetEPGSearchString() const { return m_cStructure->strEpgSearchString; }

  void SetFullTextEpgSearch(bool fullTextEpgSearch)
  {
    m_cStructure->bFullTextEpgSearch = fullTextEpgSearch;
  }
  bool GetFullTextEpgSearch() const { return m_cStructure->bFullTextEpgSearch; }

  void SetDirectory(const std::string& directory)
  {
    strncpy(m_cStructure->strDirectory, directory.c_str(), sizeof(m_cStructure->strDirectory) - 1);
  }
  std::string GetDirectory() const { return m_cStructure->strDirectory; }

  void SetSummary(const std::string& summary)
  {
    strncpy(m_cStructure->strSummary, summary.c_str(), sizeof(m_cStructure->strSummary) - 1);
  }
  std::string GetSummary() const { return m_cStructure->strSummary; }

  void SetPriority(int priority) { m_cStructure->iPriority = priority; }
  int GetPriority() const { return m_cStructure->iPriority; }

  void SetLifetime(int priority) { m_cStructure->iLifetime = priority; }
  int GetLifetime() const { return m_cStructure->iLifetime; }

  void SetMaxRecordings(int maxRecordings) { m_cStructure->iMaxRecordings = maxRecordings; }
  int GetMaxRecordings() const { return m_cStructure->iMaxRecordings; }

  void SetRecordingGroup(unsigned int recordingGroup)
  {
    m_cStructure->iRecordingGroup = recordingGroup;
  }
  unsigned int GetRecordingGroup() const { return m_cStructure->iRecordingGroup; }

  void SetFirstDay(time_t firstDay) { m_cStructure->firstDay = firstDay; }
  time_t GetFirstDay() const { return m_cStructure->firstDay; }

  void SetWeekdays(unsigned int weekdays) { m_cStructure->iWeekdays = weekdays; }
  unsigned int GetWeekdays() const { return m_cStructure->iWeekdays; }

  void SetPreventDuplicateEpisodes(unsigned int preventDuplicateEpisodes)
  {
    m_cStructure->iPreventDuplicateEpisodes = preventDuplicateEpisodes;
  }
  unsigned int GetPreventDuplicateEpisodes() const
  {
    return m_cStructure->iPreventDuplicateEpisodes;
  }

  void SetEPGUid(unsigned int epgUid) { m_cStructure->iEpgUid = epgUid; }
  unsigned int GetEPGUid() const { return m_cStructure->iEpgUid; }

  void SetMarginStart(unsigned int marginStart) { m_cStructure->iMarginStart = marginStart; }
  unsigned int GetMarginStart() const { return m_cStructure->iMarginStart; }

  void SetMarginEnd(unsigned int marginEnd) { m_cStructure->iMarginEnd = marginEnd; }
  unsigned int GetMarginEnd() const { return m_cStructure->iMarginEnd; }

  void SetGenreType(int genreType) { m_cStructure->iGenreType = genreType; }
  int GetGenreType() const { return m_cStructure->iGenreType; }

  void SetGenreSubType(int genreSubType) { m_cStructure->iGenreSubType = genreSubType; }
  int GetGenreSubType() const { return m_cStructure->iGenreSubType; }

  void SetSeriesLink(const std::string& seriesLink)
  {
    strncpy(m_cStructure->strSeriesLink, seriesLink.c_str(),
            sizeof(m_cStructure->strSeriesLink) - 1);
  }

  std::string GetSeriesLink() const { return m_cStructure->strSeriesLink; }
};

class PVRTimersResultSet
{
public:
  PVRTimersResultSet() = delete;
  PVRTimersResultSet(const AddonInstance_PVR* instance, ADDON_HANDLE handle)
    : m_instance(instance), m_handle(handle)
  {
  }

  void Add(const kodi::addon::PVRTimer& tag)
  {
    m_instance->toKodi->TransferTimerEntry(m_instance->toKodi->kodiInstance, m_handle, tag);
  }

private:
  const AddonInstance_PVR* m_instance = nullptr;
  const ADDON_HANDLE m_handle;
};

class PVRTimerType : public CStructHdl<PVRTimerType, PVR_TIMER_TYPE>
{
public:
  PVRTimerType()
  {
    memset(m_cStructure, 0, sizeof(PVR_TIMER_TYPE));
    m_cStructure->iPrioritiesDefault = -1;
    m_cStructure->iLifetimesDefault = -1;
    m_cStructure->iPreventDuplicateEpisodesDefault = -1;
    m_cStructure->iRecordingGroupDefault = -1;
    m_cStructure->iMaxRecordingsDefault = -1;
  }
  PVRTimerType(const PVRTimerType& type) : CStructHdl(type) {}
  PVRTimerType(const PVR_TIMER_TYPE* type) : CStructHdl(type) {}
  PVRTimerType(PVR_TIMER_TYPE* type) : CStructHdl(type) {}

  void SetId(unsigned int id) { m_cStructure->iId = id; }
  unsigned int GetId() const { return m_cStructure->iId; }

  void SetAttributes(uint64_t attributes) { m_cStructure->iAttributes = attributes; }
  uint64_t GetAttributes() const { return m_cStructure->iAttributes; }

  void SetDescription(const std::string& description)
  {
    strncpy(m_cStructure->strDescription, description.c_str(),
            sizeof(m_cStructure->strDescription) - 1);
  }
  std::string GetDescription() const { return m_cStructure->strDescription; }

  void SetPriorities(const std::vector<PVRTypeIntValue>& priorities,
                     int prioritiesDefault = -1)
  {
    m_cStructure->iPrioritiesSize = priorities.size();
    for (unsigned int i = 0;
         i < m_cStructure->iPrioritiesSize && i < sizeof(m_cStructure->priorities); ++i)
    {
      m_cStructure->priorities[i].iValue = priorities[i].GetCStructure()->iValue;
      strncpy(m_cStructure->priorities[i].strDescription,
              priorities[i].GetCStructure()->strDescription,
              sizeof(m_cStructure->priorities[i].strDescription) - 1);
    }
    if (prioritiesDefault != -1)
      m_cStructure->iPrioritiesDefault = prioritiesDefault;
  }
  std::vector<PVRTypeIntValue> GetPriorities() const
  {
    std::vector<PVRTypeIntValue> ret;
    for (unsigned int i = 0; i < m_cStructure->iPrioritiesSize; ++i)
      ret.emplace_back(m_cStructure->priorities[i].iValue,
                       m_cStructure->priorities[i].strDescription);
    return ret;
  }

  void SetPrioritiesDefault(int prioritiesDefault)
  {
    m_cStructure->iPrioritiesDefault = prioritiesDefault;
  }

  /// @brief To get with @ref SetPrioritiesDefault changed values.
  int GetPrioritiesDefault() const { return m_cStructure->iPrioritiesDefault; }

  void SetLifetimes(const std::vector<PVRTypeIntValue>& lifetimes, int lifetimesDefault = -1)
  {
    m_cStructure->iLifetimesSize = lifetimes.size();
    for (unsigned int i = 0;
         i < m_cStructure->iLifetimesSize && i < sizeof(m_cStructure->lifetimes); ++i)
    {
      m_cStructure->lifetimes[i].iValue = lifetimes[i].GetCStructure()->iValue;
      strncpy(m_cStructure->lifetimes[i].strDescription,
              lifetimes[i].GetCStructure()->strDescription,
              sizeof(m_cStructure->lifetimes[i].strDescription) - 1);
    }
    if (lifetimesDefault != -1)
      m_cStructure->iLifetimesDefault = lifetimesDefault;
  }
  std::vector<PVRTypeIntValue> GetLifetimes() const
  {
    std::vector<PVRTypeIntValue> ret;
    for (unsigned int i = 0; i < m_cStructure->iLifetimesSize; ++i)
      ret.emplace_back(m_cStructure->lifetimes[i].iValue,
                       m_cStructure->lifetimes[i].strDescription);
    return ret;
  }

  void SetLifetimesDefault(int lifetimesDefault)
  {
    m_cStructure->iLifetimesDefault = lifetimesDefault;
  }
  int GetLifetimesDefault() const { return m_cStructure->iLifetimesDefault; }

  void SetPreventDuplicateEpisodes(
      const std::vector<PVRTypeIntValue>& preventDuplicateEpisodes,
      int preventDuplicateEpisodesDefault = -1)
  {
    m_cStructure->iPreventDuplicateEpisodesSize = preventDuplicateEpisodes.size();
    for (unsigned int i = 0; i < m_cStructure->iPreventDuplicateEpisodesSize &&
                             i < sizeof(m_cStructure->preventDuplicateEpisodes);
         ++i)
    {
      m_cStructure->preventDuplicateEpisodes[i].iValue =
          preventDuplicateEpisodes[i].GetCStructure()->iValue;
      strncpy(m_cStructure->preventDuplicateEpisodes[i].strDescription,
              preventDuplicateEpisodes[i].GetCStructure()->strDescription,
              sizeof(m_cStructure->preventDuplicateEpisodes[i].strDescription) - 1);
    }
    if (preventDuplicateEpisodesDefault != -1)
      m_cStructure->iPreventDuplicateEpisodesDefault = preventDuplicateEpisodesDefault;
  }
  std::vector<PVRTypeIntValue> GetPreventDuplicateEpisodes() const
  {
    std::vector<PVRTypeIntValue> ret;
    for (unsigned int i = 0; i < m_cStructure->iPreventDuplicateEpisodesSize; ++i)
      ret.emplace_back(m_cStructure->preventDuplicateEpisodes[i].iValue,
                       m_cStructure->preventDuplicateEpisodes[i].strDescription);
    return ret;
  }

  void SetPreventDuplicateEpisodesDefault(int preventDuplicateEpisodesDefault)
  {
    m_cStructure->iPreventDuplicateEpisodesDefault = preventDuplicateEpisodesDefault;
  }
  int GetPreventDuplicateEpisodesDefault() const
  {
    return m_cStructure->iPreventDuplicateEpisodesDefault;
  }

  void SetRecordingGroups(const std::vector<PVRTypeIntValue>& recordingGroup,
                          int recordingGroupDefault = -1)
  {
    m_cStructure->iRecordingGroupSize = recordingGroup.size();
    for (unsigned int i = 0;
         i < m_cStructure->iRecordingGroupSize && i < sizeof(m_cStructure->recordingGroup); ++i)
    {
      m_cStructure->recordingGroup[i].iValue = recordingGroup[i].GetCStructure()->iValue;
      strncpy(m_cStructure->recordingGroup[i].strDescription,
              recordingGroup[i].GetCStructure()->strDescription,
              sizeof(m_cStructure->recordingGroup[i].strDescription) - 1);
    }
    if (recordingGroupDefault != -1)
      m_cStructure->iRecordingGroupDefault = recordingGroupDefault;
  }
  std::vector<PVRTypeIntValue> GetRecordingGroups() const
  {
    std::vector<PVRTypeIntValue> ret;
    for (unsigned int i = 0; i < m_cStructure->iRecordingGroupSize; ++i)
      ret.emplace_back(m_cStructure->recordingGroup[i].iValue,
                       m_cStructure->recordingGroup[i].strDescription);
    return ret;
  }

  void SetRecordingGroupDefault(int recordingGroupDefault)
  {
    m_cStructure->iRecordingGroupDefault = recordingGroupDefault;
  }
  int GetRecordingGroupDefault() const { return m_cStructure->iRecordingGroupDefault; }

  void SetMaxRecordings(const std::vector<PVRTypeIntValue>& maxRecordings,
                        int maxRecordingsDefault = -1)
  {
    m_cStructure->iMaxRecordingsSize = maxRecordings.size();
    for (unsigned int i = 0;
         i < m_cStructure->iMaxRecordingsSize && i < sizeof(m_cStructure->maxRecordings); ++i)
    {
      m_cStructure->maxRecordings[i].iValue = maxRecordings[i].GetCStructure()->iValue;
      strncpy(m_cStructure->maxRecordings[i].strDescription,
              maxRecordings[i].GetCStructure()->strDescription,
              sizeof(m_cStructure->maxRecordings[i].strDescription) - 1);
    }
    if (maxRecordingsDefault != -1)
      m_cStructure->iMaxRecordingsDefault = maxRecordingsDefault;
  }
  std::vector<PVRTypeIntValue> GetMaxRecordings() const
  {
    std::vector<PVRTypeIntValue> ret;
    for (unsigned int i = 0; i < m_cStructure->iMaxRecordingsSize; ++i)
      ret.emplace_back(m_cStructure->maxRecordings[i].iValue,
                       m_cStructure->maxRecordings[i].strDescription);
    return ret;
  }

  void SetMaxRecordingsDefault(int maxRecordingsDefault)
  {
    m_cStructure->iMaxRecordingsDefault = maxRecordingsDefault;
  }
  int GetMaxRecordingsDefault() const { return m_cStructure->iMaxRecordingsDefault; }
};

} /* namespace addon */
} /* namespace kodi */

#endif /* __cplusplus */
