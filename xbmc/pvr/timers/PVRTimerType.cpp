/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRTimerType.h"

#include "ServiceBroker.h"
#include "guilib/LocalizeStrings.h"
#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClient.h"
#include "pvr/addons/PVRClients.h"
#include "pvr/settings/PVRTimerSettingDefinition.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <iterator>
#include <memory>
#include <string>
#include <utility>
#include <vector>

using namespace PVR;

const std::vector<std::shared_ptr<CPVRTimerType>> CPVRTimerType::GetAllTypes()
{
  std::vector<std::shared_ptr<CPVRTimerType>> allTypes =
      CServiceBroker::GetPVRManager().Clients()->GetTimerTypes();

  // Add local reminder timer types. Local reminders are always available.
  int iTypeId = PVR_TIMER_TYPE_NONE;

  // one time time-based reminder
  allTypes.emplace_back(std::make_shared<CPVRTimerType>(
      ++iTypeId, PVR_TIMER_TYPE_IS_MANUAL | PVR_TIMER_TYPE_IS_REMINDER |
                     PVR_TIMER_TYPE_SUPPORTS_CHANNELS | PVR_TIMER_TYPE_SUPPORTS_START_TIME |
                     PVR_TIMER_TYPE_SUPPORTS_END_TIME));

  // one time epg-based reminder
  allTypes.emplace_back(std::make_shared<CPVRTimerType>(
      ++iTypeId, PVR_TIMER_TYPE_IS_REMINDER | PVR_TIMER_TYPE_REQUIRES_EPG_TAG_ON_CREATE |
                     PVR_TIMER_TYPE_SUPPORTS_CHANNELS | PVR_TIMER_TYPE_SUPPORTS_START_TIME |
                     PVR_TIMER_TYPE_SUPPORTS_START_MARGIN));

  // time-based reminder rule
  allTypes.emplace_back(std::make_shared<CPVRTimerType>(
      ++iTypeId, PVR_TIMER_TYPE_IS_REPEATING | PVR_TIMER_TYPE_IS_MANUAL |
                     PVR_TIMER_TYPE_IS_REMINDER | PVR_TIMER_TYPE_SUPPORTS_ENABLE_DISABLE |
                     PVR_TIMER_TYPE_SUPPORTS_CHANNELS | PVR_TIMER_TYPE_SUPPORTS_START_TIME |
                     PVR_TIMER_TYPE_SUPPORTS_END_TIME | PVR_TIMER_TYPE_SUPPORTS_FIRST_DAY |
                     PVR_TIMER_TYPE_SUPPORTS_WEEKDAYS));

  // one time read-only time-based reminder (created by timer rule)
  allTypes.emplace_back(std::make_shared<CPVRTimerType>(
      ++iTypeId,
      PVR_TIMER_TYPE_IS_MANUAL | PVR_TIMER_TYPE_IS_REMINDER |
          PVR_TIMER_TYPE_SUPPORTS_ENABLE_DISABLE | PVR_TIMER_TYPE_SUPPORTS_CHANNELS |
          PVR_TIMER_TYPE_SUPPORTS_START_TIME | PVR_TIMER_TYPE_SUPPORTS_END_TIME,
      g_localizeStrings.Get(819))); // One time (Scheduled by timer rule)

  // epg-based reminder rule
  allTypes.emplace_back(std::make_shared<CPVRTimerType>(
      ++iTypeId, PVR_TIMER_TYPE_IS_REPEATING | PVR_TIMER_TYPE_IS_REMINDER |
                     PVR_TIMER_TYPE_SUPPORTS_ENABLE_DISABLE |
                     PVR_TIMER_TYPE_SUPPORTS_TITLE_EPG_MATCH |
                     PVR_TIMER_TYPE_SUPPORTS_FULLTEXT_EPG_MATCH | PVR_TIMER_TYPE_SUPPORTS_CHANNELS |
                     PVR_TIMER_TYPE_SUPPORTS_ANY_CHANNEL | PVR_TIMER_TYPE_SUPPORTS_START_TIME |
                     PVR_TIMER_TYPE_SUPPORTS_START_ANYTIME | PVR_TIMER_TYPE_SUPPORTS_END_TIME |
                     PVR_TIMER_TYPE_SUPPORTS_END_ANYTIME | PVR_TIMER_TYPE_SUPPORTS_FIRST_DAY |
                     PVR_TIMER_TYPE_SUPPORTS_WEEKDAYS | PVR_TIMER_TYPE_SUPPORTS_START_END_MARGIN));

  // one time read-only epg-based reminder (created by timer rule)
  allTypes.emplace_back(std::make_shared<CPVRTimerType>(
      ++iTypeId,
      PVR_TIMER_TYPE_IS_REMINDER | PVR_TIMER_TYPE_REQUIRES_EPG_TAG_ON_CREATE |
          PVR_TIMER_TYPE_SUPPORTS_ENABLE_DISABLE | PVR_TIMER_TYPE_SUPPORTS_CHANNELS |
          PVR_TIMER_TYPE_SUPPORTS_START_TIME | PVR_TIMER_TYPE_SUPPORTS_START_MARGIN,
      g_localizeStrings.Get(819))); // One time (Scheduled by timer rule)

  return allTypes;
}

const std::shared_ptr<CPVRTimerType> CPVRTimerType::GetFirstAvailableType(
    const std::shared_ptr<const CPVRClient>& client)
{
  if (client)
  {
    const std::vector<std::shared_ptr<CPVRTimerType>>& types = client->GetTimerTypes();
    if (!types.empty())
    {
      return *(types.begin());
    }
  }
  return {};
}

std::shared_ptr<CPVRTimerType> CPVRTimerType::CreateFromIds(unsigned int iTypeId, int iClientId)
{
  const std::vector<std::shared_ptr<CPVRTimerType>> types = GetAllTypes();
  const auto it =
      std::find_if(types.cbegin(), types.cend(),
                   [iClientId, iTypeId](const auto& type)
                   { return type->GetClientId() == iClientId && type->GetTypeId() == iTypeId; });
  if (it != types.cend())
    return (*it);

  if (iClientId != PVR_CLIENT_INVALID_UID)
  {
    // fallback. try to obtain local timer type.
    std::shared_ptr<CPVRTimerType> type = CreateFromIds(iTypeId, PVR_CLIENT_INVALID_UID);
    if (type)
      return type;
  }

  CLog::LogF(LOGERROR, "Unable to resolve numeric timer type ({}, {})", iTypeId, iClientId);
  return {};
}

std::shared_ptr<CPVRTimerType> CPVRTimerType::CreateFromAttributes(uint64_t iMustHaveAttr,
                                                                   uint64_t iMustNotHaveAttr,
                                                                   int iClientId)
{
  const std::vector<std::shared_ptr<CPVRTimerType>> types = GetAllTypes();
  const auto it = std::find_if(types.cbegin(), types.cend(),
                               [iClientId, iMustHaveAttr, iMustNotHaveAttr](const auto& type)
                               {
                                 return type->GetClientId() == iClientId &&
                                        (type->GetAttributes() & iMustHaveAttr) == iMustHaveAttr &&
                                        (type->GetAttributes() & iMustNotHaveAttr) == 0;
                               });
  if (it != types.cend())
    return (*it);

  if (iClientId != PVR_CLIENT_INVALID_UID)
  {
    // fallback. try to obtain local timer type.
    std::shared_ptr<CPVRTimerType> type =
        CreateFromAttributes(iMustHaveAttr, iMustNotHaveAttr, PVR_CLIENT_INVALID_UID);
    if (type)
      return type;
  }

  CLog::LogF(LOGERROR, "Unable to resolve timer type (0x{:x}, 0x{:x}, {})", iMustHaveAttr,
             iMustNotHaveAttr, iClientId);
  return {};
}

CPVRTimerType::CPVRTimerType()
  : m_iTypeId(PVR_TIMER_TYPE_NONE), m_iAttributes(PVR_TIMER_TYPE_ATTRIBUTE_NONE)
{
}

CPVRTimerType::CPVRTimerType(const PVR_TIMER_TYPE& type, int iClientId)
  : m_iClientId(iClientId),
    m_iTypeId(type.iId),
    m_iAttributes(type.iAttributes),
    m_strDescription(type.strDescription ? type.strDescription : "")
{
  InitDescription();
  InitAttributeValues(type);
  InitCustomSettingDefinitions(type);
}

CPVRTimerType::CPVRTimerType(unsigned int iTypeId,
                             uint64_t iAttributes,
                             const std::string& strDescription)
  : m_iTypeId(iTypeId), m_iAttributes(iAttributes), m_strDescription(strDescription)
{
  InitDescription();
}

CPVRTimerType::~CPVRTimerType() = default;

bool CPVRTimerType::operator==(const CPVRTimerType& right) const
{
  return (m_iClientId == right.m_iClientId && m_iTypeId == right.m_iTypeId &&
          m_iAttributes == right.m_iAttributes && m_strDescription == right.m_strDescription &&
          m_priorityValues == right.m_priorityValues &&
          m_lifetimeValues == right.m_lifetimeValues &&
          m_maxRecordingsValues == right.m_maxRecordingsValues &&
          m_preventDupEpisodesValues == right.m_preventDupEpisodesValues &&
          m_recordingGroupValues == right.m_recordingGroupValues &&
          m_customSettingDefs == right.m_customSettingDefs);
}

bool CPVRTimerType::operator!=(const CPVRTimerType& right) const
{
  return !(*this == right);
}

void CPVRTimerType::Update(const CPVRTimerType& type)
{
  m_iClientId = type.m_iClientId;
  m_iTypeId = type.m_iTypeId;
  m_iAttributes = type.m_iAttributes;
  m_strDescription = type.m_strDescription;
  m_priorityValues = type.m_priorityValues;
  m_lifetimeValues = type.m_lifetimeValues;
  m_maxRecordingsValues = type.m_maxRecordingsValues;
  m_preventDupEpisodesValues = type.m_preventDupEpisodesValues;
  m_recordingGroupValues = type.m_recordingGroupValues;
  m_customSettingDefs = type.m_customSettingDefs;
}

void CPVRTimerType::InitDescription()
{
  // if no description was given, compile it
  if (m_strDescription.empty())
  {
    int id;
    if (m_iAttributes & PVR_TIMER_TYPE_IS_REPEATING)
    {
      id = (m_iAttributes & PVR_TIMER_TYPE_IS_MANUAL) ? 822 // "Timer rule"
                                                      : 823; // "Timer rule (guide-based)"
    }
    else
    {
      id = (m_iAttributes & PVR_TIMER_TYPE_IS_MANUAL) ? 820 // "One time"
                                                      : 821; // "One time (guide-based)
    }
    m_strDescription = g_localizeStrings.Get(id);
  }

  // add reminder/recording prefix
  int prefixId = (m_iAttributes & PVR_TIMER_TYPE_IS_REMINDER) ? 824 // Reminder: ...
                                                              : 825; // Recording: ...

  m_strDescription = StringUtils::Format(g_localizeStrings.Get(prefixId), m_strDescription);
}

void CPVRTimerType::InitAttributeValues(const PVR_TIMER_TYPE& type)
{
  InitPriorityValues(type);
  InitLifetimeValues(type);
  InitMaxRecordingsValues(type);
  InitPreventDuplicateEpisodesValues(type);
  InitRecordingGroupValues(type);
}

void CPVRTimerType::InitPriorityValues(const PVR_TIMER_TYPE& type)
{
  if (type.iPrioritiesSize > 0)
  {
    m_priorityValues = {type.priorities, type.iPrioritiesSize, type.iPrioritiesDefault};
  }
  else if (SupportsPriority())
  {
    // No values given by addon, but priority supported. Use default values 1..100
    std::vector<SettingIntValue> values;
    for (int i = 1; i < 101; ++i)
      values.emplace_back(std::to_string(i), i);

    m_priorityValues = {values, DEFAULT_RECORDING_PRIORITY};
  }
  else
  {
    // No priority supported.
    m_priorityValues = CPVRIntSettingValues{DEFAULT_RECORDING_PRIORITY};
  }
}

void CPVRTimerType::InitLifetimeValues(const PVR_TIMER_TYPE& type)
{
  if (type.iLifetimesSize > 0)
  {
    m_lifetimeValues = {type.lifetimes, type.iLifetimesSize, type.iLifetimesDefault};
  }
  else if (SupportsLifetime())
  {
    // No values given by addon, but lifetime supported. Use default values 1..365
    std::vector<SettingIntValue> values;
    for (int i = 1; i < 366; ++i)
    {
      values.emplace_back(StringUtils::Format(g_localizeStrings.Get(17999), i),
                          i); // "{} days"
    }
    m_lifetimeValues = {values, DEFAULT_RECORDING_LIFETIME};
  }
  else
  {
    // No lifetime supported.
    m_lifetimeValues = CPVRIntSettingValues{DEFAULT_RECORDING_LIFETIME};
  }
}

void CPVRTimerType::InitMaxRecordingsValues(const PVR_TIMER_TYPE& type)
{
  m_maxRecordingsValues = {type.maxRecordings, type.iMaxRecordingsSize, type.iMaxRecordingsDefault};
}

void CPVRTimerType::InitPreventDuplicateEpisodesValues(const PVR_TIMER_TYPE& type)
{
  if (type.iPreventDuplicateEpisodesSize > 0)
  {
    m_preventDupEpisodesValues = {type.preventDuplicateEpisodes, type.iPreventDuplicateEpisodesSize,
                                  type.iPreventDuplicateEpisodesDefault};
  }
  else if (SupportsRecordOnlyNewEpisodes())
  {
    // No values given by addon, but prevent duplicate episodes supported. Use default values 0..1
    m_preventDupEpisodesValues = {
        {{g_localizeStrings.Get(815) /* "Record all episodes" */, 0},
         {g_localizeStrings.Get(816) /* "Record only new episodes" */, 1}},
        DEFAULT_RECORDING_DUPLICATEHANDLING};
  }
  else
  {
    // No prevent duplicate episodes supported.
    m_preventDupEpisodesValues = CPVRIntSettingValues{DEFAULT_RECORDING_DUPLICATEHANDLING};
  }
}

void CPVRTimerType::InitRecordingGroupValues(const PVR_TIMER_TYPE& type)
{
  m_recordingGroupValues = {type.recordingGroup, type.iRecordingGroupSize,
                            type.iRecordingGroupDefault, 811 /* Recording group */};
}

void CPVRTimerType::InitCustomSettingDefinitions(const PVR_TIMER_TYPE& type)
{
  m_customSettingDefs = CPVRTimerSettingDefinition::CreateSettingDefinitionsList(
      m_iClientId, m_iTypeId, type.customSettingDefs, type.iCustomSettingDefsSize);
}
