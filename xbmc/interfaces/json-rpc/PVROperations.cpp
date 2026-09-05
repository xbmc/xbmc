/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVROperations.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "XBDateTime.h"
#include "pvr/PVRManager.h"
#include "pvr/PVRPlaybackState.h"
#include "pvr/addons/PVRClients.h"
#include "pvr/channels/PVRChannel.h"
#include "pvr/channels/PVRChannelGroup.h"
#include "pvr/channels/PVRChannelGroupMember.h"
#include "pvr/channels/PVRChannelGroups.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/epg/Epg.h"
#include "pvr/epg/EpgContainer.h"
#include "pvr/epg/EpgInfoTag.h"
#include "pvr/guilib/PVRGUIActionsChannels.h"
#include "pvr/guilib/PVRGUIActionsTimers.h"
#include "pvr/providers/PVRProvider.h"
#include "pvr/providers/PVRProviders.h"
#include "pvr/recordings/PVRRecordings.h"
#include "pvr/timers/PVRTimerInfoTag.h"
#include "pvr/timers/PVRTimers.h"
#include "utils/Variant.h"

#include <memory>
#include <set>
#include <string>
#include <vector>

using namespace JSONRPC;
using namespace PVR;
using namespace KODI::MESSAGING;

JSONRPC_STATUS CPVROperations::GetProperties(const std::string& method,
                                             ITransportLayer* transport,
                                             IClient* client,
                                             const CVariant& parameterObject,
                                             CVariant& result)
{
  if (!CServiceBroker::GetPVRManager().IsStarted())
    return FailedToExecute;

  CVariant properties{CVariant::VariantTypeObject};
  for (unsigned int index = 0; index < parameterObject["properties"].size(); ++index)
  {
    const std::string propertyName{parameterObject["properties"][index].asString()};
    CVariant property;
    const JSONRPC_STATUS ret{GetPropertyValue(propertyName, property)};
    if (ret != OK)
      return ret;

    properties[propertyName] = property;
  }

  result = properties;

  return OK;
}

JSONRPC_STATUS CPVROperations::GetChannelGroups(const std::string& method,
                                                ITransportLayer* transport,
                                                IClient* client,
                                                const CVariant& parameterObject,
                                                CVariant& result)
{
  if (!CServiceBroker::GetPVRManager().IsStarted())
    return FailedToExecute;

  const std::shared_ptr<const CPVRChannelGroupsContainer> channelGroupContainer{
      CServiceBroker::GetPVRManager().ChannelGroups()};
  if (!channelGroupContainer)
    return FailedToExecute;

  const std::shared_ptr<const CPVRChannelGroups> channelGroups{
      channelGroupContainer->Get(parameterObject["channeltype"].asString() == "radio")};
  if (!channelGroups)
    return FailedToExecute;

  int start{0};
  int end{0};

  std::vector<std::shared_ptr<CPVRChannelGroup>> groupList{channelGroups->GetMembers(true)};
  HandleLimits(parameterObject, result, static_cast<int>(groupList.size()), start, end);
  for (int index = start; index < end; ++index)
    FillChannelGroupDetails(groupList.at(index), parameterObject, result["channelgroups"], true);

  return OK;
}

JSONRPC_STATUS CPVROperations::GetChannelGroupDetails(const std::string& method,
                                                      ITransportLayer* transport,
                                                      IClient* client,
                                                      const CVariant& parameterObject,
                                                      CVariant& result)
{
  if (!CServiceBroker::GetPVRManager().IsStarted())
    return FailedToExecute;

  const std::shared_ptr<const CPVRChannelGroupsContainer> channelGroupContainer{
      CServiceBroker::GetPVRManager().ChannelGroups()};
  if (!channelGroupContainer)
    return FailedToExecute;

  std::shared_ptr<const CPVRChannelGroup> channelGroup;
  const CVariant id{parameterObject["channelgroupid"]};
  if (id.isInteger())
    channelGroup = channelGroupContainer->GetByIdFromAll(static_cast<int>(id.asInteger()));
  else if (id.isString())
    channelGroup = channelGroupContainer->GetGroupAll(id.asString() == "allradio");

  if (!channelGroup)
    return NotFound;

  FillChannelGroupDetails(channelGroup, parameterObject, result["channelgroupdetails"], false);

  return OK;
}

JSONRPC_STATUS CPVROperations::GetChannels(const std::string& method,
                                           ITransportLayer* transport,
                                           IClient* client,
                                           const CVariant& parameterObject,
                                           CVariant& result)
{
  if (!CServiceBroker::GetPVRManager().IsStarted())
    return FailedToExecute;

  const std::shared_ptr<const CPVRChannelGroupsContainer> channelGroupContainer{
      CServiceBroker::GetPVRManager().ChannelGroups()};
  if (!channelGroupContainer)
    return FailedToExecute;

  std::shared_ptr<const CPVRChannelGroup> channelGroup;
  const CVariant id{parameterObject["channelgroupid"]};
  if (id.isInteger())
    channelGroup = channelGroupContainer->GetByIdFromAll(static_cast<int>(id.asInteger()));
  else if (id.isString())
    channelGroup = channelGroupContainer->GetGroupAll(id.asString() == "allradio");

  if (!channelGroup)
    return NotFound;

  CFileItemList channels;
  const auto groupMembers = channelGroup->GetMembers(CPVRChannelGroup::Include::ONLY_VISIBLE);
  for (const auto& groupMember : groupMembers)
  {
    channels.Add(std::make_shared<CFileItem>(groupMember));
  }

  HandleFileItemList("channelid", false, "channels", channels, parameterObject, result, true);

  return OK;
}

JSONRPC_STATUS CPVROperations::GetChannelDetails(const std::string& method,
                                                 ITransportLayer* transport,
                                                 IClient* client,
                                                 const CVariant& parameterObject,
                                                 CVariant& result)
{
  if (!CServiceBroker::GetPVRManager().IsStarted())
    return FailedToExecute;

  const std::shared_ptr<const CPVRChannelGroupsContainer> channelGroupContainer{
      CServiceBroker::GetPVRManager().ChannelGroups()};
  if (!channelGroupContainer)
    return FailedToExecute;

  const std::shared_ptr<const CPVRChannel> channel{channelGroupContainer->GetChannelById(
      static_cast<int>(parameterObject["channelid"].asInteger()))};
  if (!channel)
    return NotFound;

  const std::shared_ptr<CPVRChannelGroupMember> groupMember{
      CServiceBroker::GetPVRManager().Get<PVR::GUI::Channels>().GetChannelGroupMember(channel)};
  if (!groupMember)
    return NotFound;

  HandleFileItem("channelid", false, "channeldetails", std::make_shared<CFileItem>(groupMember),
                 parameterObject, parameterObject["properties"], result, false);

  return OK;
}

JSONRPC_STATUS CPVROperations::GetClients(const std::string& method,
                                          ITransportLayer* transport,
                                          IClient* client,
                                          const CVariant& parameterObject,
                                          CVariant& result)
{
  if (!CServiceBroker::GetPVRManager().IsStarted())
    return FailedToExecute;

  int start{0};
  int end{0};
  auto clientInfos = CServiceBroker::GetPVRManager().Clients()->GetEnabledClientInfos();
  HandleLimits(parameterObject, result, static_cast<int>(clientInfos.size()), start, end);

  for (int index = start; index < end; ++index)
  {
    result["clients"].append(clientInfos[index]);
  }

  return OK;
}

JSONRPC_STATUS CPVROperations::GetProviders(const std::string& method,
                                            ITransportLayer* transport,
                                            IClient* client,
                                            const CVariant& parameterObject,
                                            CVariant& result)
{
  if (!CServiceBroker::GetPVRManager().IsStarted())
    return FailedToExecute;

  const std::shared_ptr<const CPVRProviders> providers{CServiceBroker::GetPVRManager().Providers()};
  if (!providers)
    return FailedToExecute;

  const std::vector<std::shared_ptr<CPVRProvider>> providerList{providers->GetProviders()};

  int start{0};
  int end{0};
  HandleLimits(parameterObject, result, static_cast<int>(providerList.size()), start, end);

  result["providers"] = CVariant{CVariant::VariantTypeArray};

  for (int index = start; index < end; ++index)
  {
    FillProviderDetails(providerList[index], parameterObject, result["providers"], true);
  }

  return OK;
}

JSONRPC_STATUS CPVROperations::GetProviderDetails(const std::string& method,
                                                  ITransportLayer* transport,
                                                  IClient* client,
                                                  const CVariant& parameterObject,
                                                  CVariant& result)
{
  if (!CServiceBroker::GetPVRManager().IsStarted())
    return FailedToExecute;

  const std::shared_ptr<const CPVRProviders> providers{CServiceBroker::GetPVRManager().Providers()};
  if (!providers)
    return FailedToExecute;

  const std::shared_ptr<const CPVRProvider> provider{
      providers->GetById(static_cast<int>(parameterObject["providerid"].asInteger()))};
  if (!provider)
    return NotFound;

  FillProviderDetails(provider, parameterObject, result["providerdetails"], false);

  return OK;
}

void CPVROperations::FillProviderDetails(const std::shared_ptr<const CPVRProvider>& provider,
                                         const CVariant& parameterObject,
                                         CVariant& result,
                                         bool append /* = false */)
{
  CVariant object{CVariant::VariantTypeObject};
  object["providerid"] = provider->GetDatabaseId();
  object["label"] = provider->GetName();

  std::set<std::string> fields{RequestedFields(parameterObject)};

  // A provider has no file item, so the serialized values are all there is to answer from.
  FillDetails(provider.get(), {}, fields, object);

  if (append)
    result.append(object);
  else
    result = object;
}

JSONRPC_STATUS CPVROperations::GetBroadcasts(const std::string& method,
                                             ITransportLayer* transport,
                                             IClient* client,
                                             const CVariant& parameterObject,
                                             CVariant& result)
{
  if (!CServiceBroker::GetPVRManager().IsStarted())
    return FailedToExecute;

  const std::shared_ptr<const CPVRChannelGroupsContainer> channelGroupContainer{
      CServiceBroker::GetPVRManager().ChannelGroups()};
  if (!channelGroupContainer)
    return FailedToExecute;

  CDateTime start;
  CDateTime end;
  const JSONRPC_STATUS rangeStatus{ParseTimeRange(parameterObject, false, start, end)};
  if (rangeStatus != OK)
    return rangeStatus;

  const std::shared_ptr<const CPVRChannel> channel{channelGroupContainer->GetChannelById(
      static_cast<int>(parameterObject["channelid"].asInteger()))};
  if (!channel)
    return NotFound;

  const std::shared_ptr<const CPVREpg> channelEpg{channel->GetEPG()};
  if (!channelEpg)
    return InternalError;

  CFileItemList programFull;
  for (const auto& tag : GetBroadcastsInRange(*channelEpg, start, end))
  {
    programFull.Add(std::make_shared<CFileItem>(tag));
  }

  HandleFileItemList("broadcastid", false, "broadcasts", programFull, parameterObject, result,
                     programFull.Size(), true);

  return OK;
}

JSONRPC_STATUS CPVROperations::GetBroadcastsByChannelGroup(const std::string& method,
                                                           ITransportLayer* transport,
                                                           IClient* client,
                                                           const CVariant& parameterObject,
                                                           CVariant& result)
{
  if (!CServiceBroker::GetPVRManager().IsStarted())
    return FailedToExecute;

  const std::shared_ptr<const CPVRChannelGroupsContainer> channelGroupContainer{
      CServiceBroker::GetPVRManager().ChannelGroups()};
  if (!channelGroupContainer)
    return FailedToExecute;

  CDateTime start;
  CDateTime end;
  const JSONRPC_STATUS rangeStatus{ParseTimeRange(parameterObject, true, start, end)};
  if (rangeStatus != OK)
    return rangeStatus;

  std::shared_ptr<const CPVRChannelGroup> channelGroup;
  const CVariant id{parameterObject["channelgroupid"]};
  if (id.isInteger())
    channelGroup = channelGroupContainer->GetByIdFromAll(static_cast<int>(id.asInteger()));
  else if (id.isString())
    channelGroup = channelGroupContainer->GetGroupAll(id.asString() == "allradio");

  if (!channelGroup)
    return NotFound;

  result["channels"] = CVariant{CVariant::VariantTypeArray};

  const auto groupMembers = channelGroup->GetMembers(CPVRChannelGroup::Include::ONLY_VISIBLE);
  for (const auto& groupMember : groupMembers)
  {
    const std::shared_ptr<const CPVRChannel> channel{groupMember->Channel()};
    if (!channel)
      continue;

    CVariant entry{CVariant::VariantTypeObject};
    entry["channelid"] = channel->ChannelID();
    entry["broadcasts"] = CVariant{CVariant::VariantTypeArray};

    const std::shared_ptr<const CPVREpg> channelEpg{channel->GetEPG()};
    if (channelEpg)
    {
      for (const auto& tag : GetBroadcastsInRange(*channelEpg, start, end))
      {
        HandleFileItem("broadcastid", false, "broadcasts", std::make_shared<CFileItem>(tag),
                       parameterObject, parameterObject["properties"], entry, true);
      }
    }

    result["channels"].append(std::move(entry));
  }

  return OK;
}

JSONRPC_STATUS CPVROperations::GetBroadcastDetails(const std::string& method,
                                                   ITransportLayer* transport,
                                                   IClient* client,
                                                   const CVariant& parameterObject,
                                                   CVariant& result)
{
  if (!CServiceBroker::GetPVRManager().IsStarted())
    return FailedToExecute;

  const std::shared_ptr<CPVREpgInfoTag> epgTag{
      CServiceBroker::GetPVRManager().EpgContainer().GetTagByDatabaseId(
          static_cast<int>(parameterObject["broadcastid"].asInteger()))};

  if (!epgTag)
    return NotFound;

  HandleFileItem("broadcastid", false, "broadcastdetails", std::make_shared<CFileItem>(epgTag),
                 parameterObject, parameterObject["properties"], result, false);

  return OK;
}

JSONRPC_STATUS CPVROperations::GetBroadcastIsPlayable(const std::string& method,
                                                      ITransportLayer* transport,
                                                      IClient* client,
                                                      const CVariant& parameterObject,
                                                      CVariant& result)
{
  if (!CServiceBroker::GetPVRManager().IsStarted())
    return FailedToExecute;

  const std::shared_ptr<const CPVREpgInfoTag> epgTag{
      CServiceBroker::GetPVRManager().EpgContainer().GetTagByDatabaseId(
          static_cast<int>(parameterObject["broadcastid"].asInteger()))};

  if (!epgTag)
    return NotFound;

  result = epgTag->IsPlayable();

  return OK;
}

JSONRPC_STATUS CPVROperations::GetPlayableBroadcasts(const std::string& method,
                                                     ITransportLayer* transport,
                                                     IClient* client,
                                                     const CVariant& parameterObject,
                                                     CVariant& result)
{
  if (!CServiceBroker::GetPVRManager().IsStarted())
    return FailedToExecute;

  const std::shared_ptr<const CPVRChannelGroupsContainer> channelGroupContainer{
      CServiceBroker::GetPVRManager().ChannelGroups()};
  if (!channelGroupContainer)
    return FailedToExecute;

  CDateTime start;
  CDateTime end;
  const JSONRPC_STATUS rangeStatus{ParseTimeRange(parameterObject, true, start, end)};
  if (rangeStatus != OK)
    return rangeStatus;

  const std::shared_ptr<const CPVRChannel> channel{channelGroupContainer->GetChannelById(
      static_cast<int>(parameterObject["channelid"].asInteger()))};
  if (!channel)
    return NotFound;

  const std::shared_ptr<const CPVREpg> channelEpg{channel->GetEPG()};
  if (!channelEpg)
    return InternalError;

  const std::vector<std::shared_ptr<CPVREpgInfoTag>> tagsInRange{
      GetBroadcastsInRange(*channelEpg, start, end)};

  // Resolving playability costs a call into the client per tag, so bound how many are
  // examined rather than how many are returned.
  int first{0};
  int last{0};
  HandleLimits(parameterObject, result, static_cast<int>(tagsInRange.size()), first, last);

  result["broadcastids"] = CVariant{CVariant::VariantTypeArray};

  for (int index = first; index < last; ++index)
  {
    const std::shared_ptr<const CPVREpgInfoTag>& tag{tagsInRange[index]};
    if (tag->IsPlayable())
    {
      result["broadcastids"].append(tag->DatabaseID());
    }
  }

  return OK;
}

JSONRPC_STATUS CPVROperations::Record(const std::string& method,
                                      ITransportLayer* transport,
                                      IClient* client,
                                      const CVariant& parameterObject,
                                      CVariant& result)
{
  if (!CServiceBroker::GetPVRManager().IsStarted())
    return FailedToExecute;

  std::shared_ptr<CPVRChannel> pChannel;
  const CVariant channel{parameterObject["channel"]};
  if (channel.isString() && channel.asString() == "current")
  {
    pChannel = CServiceBroker::GetPVRManager().PlaybackState()->GetPlayingChannel();
    if (!pChannel)
      return InternalError;
  }
  else if (channel.isInteger())
  {
    const std::shared_ptr<const CPVRChannelGroupsContainer> channelGroupContainer{
        CServiceBroker::GetPVRManager().ChannelGroups()};
    if (!channelGroupContainer)
      return FailedToExecute;

    pChannel = channelGroupContainer->GetChannelById(static_cast<int>(channel.asInteger()));
  }
  else
    return InvalidParams;

  if (!pChannel)
    return NotFound;
  else if (!pChannel->CanRecord())
    return FailedToExecute;

  const CVariant record{parameterObject["record"]};
  const bool isRecording{CServiceBroker::GetPVRManager().Timers()->IsRecordingOnChannel(*pChannel)};
  bool toggle = true;
  if (record.isBoolean() && record.asBoolean() == isRecording)
    toggle = false;

  if (toggle)
  {
    if (!CServiceBroker::GetPVRManager().Get<PVR::GUI::Timers>().SetRecordingOnChannel(
            pChannel, !isRecording))
      return FailedToExecute;
  }

  return ACK;
}

JSONRPC_STATUS CPVROperations::Scan(const std::string& method,
                                    ITransportLayer* transport,
                                    IClient* client,
                                    const CVariant& parameterObject,
                                    CVariant& result)
{
  if (!CServiceBroker::GetPVRManager().IsStarted())
    return FailedToExecute;

  if (parameterObject.isMember("clientid"))
  {
    if (CServiceBroker::GetPVRManager().Get<PVR::GUI::Channels>().StartChannelScan(
            static_cast<int>(parameterObject["clientid"].asInteger())))
      return ACK;
  }
  else
  {
    if (CServiceBroker::GetPVRManager().Get<PVR::GUI::Channels>().StartChannelScan())
      return ACK;
  }

  return FailedToExecute;
}

JSONRPC_STATUS CPVROperations::GetPropertyValue(const std::string& property, CVariant& result)
{
  const bool started{CServiceBroker::GetPVRManager().IsStarted()};

  if (property == "available")
    result = started;
  else if (property == "recording")
  {
    if (started)
      result = CServiceBroker::GetPVRManager().PlaybackState()->IsRecording();
    else
      result = false;
  }
  else if (property == "scanning")
  {
    if (started)
      result = CServiceBroker::GetPVRManager().Get<PVR::GUI::Channels>().IsRunningChannelScan();
    else
      result = false;
  }
  else
    return InvalidParams;

  return OK;
}

void CPVROperations::FillChannelGroupDetails(
    const std::shared_ptr<const CPVRChannelGroup>& channelGroup,
    const CVariant& parameterObject,
    CVariant& result,
    bool append /* = false */)
{
  if (!channelGroup)
    return;

  CVariant object{CVariant::VariantTypeObject};
  object["channelgroupid"] = channelGroup->GroupID();
  object["channeltype"] = channelGroup->IsRadio() ? "radio" : "tv";
  object["label"] = channelGroup->GroupName();

  if (append)
    result.append(object);
  else
  {
    CFileItemList channels;
    const auto groupMembers{channelGroup->GetMembers(CPVRChannelGroup::Include::ONLY_VISIBLE)};
    for (const auto& groupMember : groupMembers)
    {
      channels.Add(std::make_shared<CFileItem>(groupMember));
    }

    object["channels"] = CVariant(CVariant::VariantTypeArray);
    HandleFileItemList("channelid", false, "channels", channels, parameterObject["channels"],
                       object, false);

    result = object;
  }
}

JSONRPC_STATUS CPVROperations::GetTimers(const std::string& method,
                                         ITransportLayer* transport,
                                         IClient* client,
                                         const CVariant& parameterObject,
                                         CVariant& result)
{
  if (!CServiceBroker::GetPVRManager().IsStarted())
    return FailedToExecute;

  const std::shared_ptr<const CPVRTimers> timers{CServiceBroker::GetPVRManager().Timers()};
  if (!timers)
    return FailedToExecute;

  CFileItemList timerList;
  const std::vector<std::shared_ptr<CPVRTimerInfoTag>> tags{timers->GetAll()};
  for (const auto& timer : tags)
  {
    timerList.Add(std::make_shared<CFileItem>(timer));
  }

  HandleFileItemList("timerid", false, "timers", timerList, parameterObject, result, true);

  return OK;
}

JSONRPC_STATUS CPVROperations::GetTimerDetails(const std::string& method,
                                               ITransportLayer* transport,
                                               IClient* client,
                                               const CVariant& parameterObject,
                                               CVariant& result)
{
  if (!CServiceBroker::GetPVRManager().IsStarted())
    return FailedToExecute;

  const std::shared_ptr<const CPVRTimers> timers{CServiceBroker::GetPVRManager().Timers()};
  if (!timers)
    return FailedToExecute;

  const std::shared_ptr<CPVRTimerInfoTag> timer{
      timers->GetById(static_cast<int>(parameterObject["timerid"].asInteger()))};
  if (!timer)
    return NotFound;

  HandleFileItem("timerid", false, "timerdetails", std::make_shared<CFileItem>(timer),
                 parameterObject, parameterObject["properties"], result, false);

  return OK;
}

JSONRPC_STATUS CPVROperations::AddTimer(const std::string& method,
                                        ITransportLayer* transport,
                                        IClient* client,
                                        const CVariant& parameterObject,
                                        CVariant& result)
{
  if (!CServiceBroker::GetPVRManager().IsStarted())
    return FailedToExecute;

  const std::shared_ptr<CPVREpgInfoTag> epgTag{
      CServiceBroker::GetPVRManager().EpgContainer().GetTagByDatabaseId(
          static_cast<int>(parameterObject["broadcastid"].asInteger()))};

  if (!epgTag)
    return NotFound;

  if (CServiceBroker::GetPVRManager().Timers()->GetTimerForEpgTag(epgTag))
    return InvalidParams;

  const std::shared_ptr<CPVRTimerInfoTag> newTimer{
      CPVRTimerInfoTag::CreateFromEpg(epgTag, parameterObject["timerrule"].asBoolean(false),
                                      parameterObject["reminder"].asBoolean(false))};
  if (newTimer)
  {
    if (CServiceBroker::GetPVRManager().Get<PVR::GUI::Timers>().AddTimer(newTimer))
      return ACK;
  }
  return FailedToExecute;
}

JSONRPC_STATUS CPVROperations::DeleteTimer(const std::string& method,
                                           ITransportLayer* transport,
                                           IClient* client,
                                           const CVariant& parameterObject,
                                           CVariant& result)
{
  if (!CServiceBroker::GetPVRManager().IsStarted())
    return FailedToExecute;

  const std::shared_ptr<CPVRTimers> timers{CServiceBroker::GetPVRManager().Timers()};
  if (!timers)
    return FailedToExecute;

  const std::shared_ptr<CPVRTimerInfoTag> timer{
      timers->GetById(static_cast<int>(parameterObject["timerid"].asInteger()))};
  if (!timer)
    return NotFound;

  if (timers->DeleteTimer(timer, timer->IsRecording(), false) == TimerOperationResult::OK)
    return ACK;

  return FailedToExecute;
}

JSONRPC_STATUS CPVROperations::ToggleTimer(const std::string& method,
                                           ITransportLayer* transport,
                                           IClient* client,
                                           const CVariant& parameterObject,
                                           CVariant& result)
{
  if (!CServiceBroker::GetPVRManager().IsStarted())
    return FailedToExecute;

  const std::shared_ptr<CPVREpgInfoTag> epgTag{
      CServiceBroker::GetPVRManager().EpgContainer().GetTagByDatabaseId(
          static_cast<int>(parameterObject["broadcastid"].asInteger()))};

  if (!epgTag)
    return NotFound;

  const std::shared_ptr<CPVRTimers> timers{CServiceBroker::GetPVRManager().Timers()};
  if (!timers)
    return FailedToExecute;

  const bool timerrule{parameterObject["timerrule"].asBoolean(false)};
  bool sentOkay = false;
  std::shared_ptr<CPVRTimerInfoTag> timer{timers->GetTimerForEpgTag(epgTag)};
  if (timer)
  {
    if (timerrule)
      timer = timers->GetTimerRule(timer);

    if (timer)
      sentOkay =
          (timers->DeleteTimer(timer, timer->IsRecording(), false) == TimerOperationResult::OK);
  }
  else
  {
    timer = CPVRTimerInfoTag::CreateFromEpg(epgTag, timerrule);
    if (!timer)
      return InvalidParams;

    sentOkay = CServiceBroker::GetPVRManager().Get<PVR::GUI::Timers>().AddTimer(timer);
  }

  if (sentOkay)
    return ACK;

  return FailedToExecute;
}

JSONRPC_STATUS CPVROperations::GetRecordings(const std::string& method,
                                             ITransportLayer* transport,
                                             IClient* client,
                                             const CVariant& parameterObject,
                                             CVariant& result)
{
  if (!CServiceBroker::GetPVRManager().IsStarted())
    return FailedToExecute;

  const std::shared_ptr<const CPVRRecordings> recordings{
      CServiceBroker::GetPVRManager().Recordings()};
  if (!recordings)
    return FailedToExecute;

  CFileItemList recordingsList;
  const std::vector<std::shared_ptr<CPVRRecording>> recs{recordings->GetAll()};
  for (const auto& recording : recs)
  {
    recordingsList.Add(std::make_shared<CFileItem>(recording));
  }

  HandleFileItemList("recordingid", true, "recordings", recordingsList, parameterObject, result,
                     true);

  return OK;
}

JSONRPC_STATUS CPVROperations::GetRecordingDetails(const std::string& method,
                                                   ITransportLayer* transport,
                                                   IClient* client,
                                                   const CVariant& parameterObject,
                                                   CVariant& result)
{
  if (!CServiceBroker::GetPVRManager().IsStarted())
    return FailedToExecute;

  const std::shared_ptr<const CPVRRecordings> recordings{
      CServiceBroker::GetPVRManager().Recordings()};
  if (!recordings)
    return FailedToExecute;

  const std::shared_ptr<CPVRRecording> recording{
      recordings->GetById(static_cast<int>(parameterObject["recordingid"].asInteger()))};
  if (!recording)
    return NotFound;

  HandleFileItem("recordingid", true, "recordingdetails", std::make_shared<CFileItem>(recording),
                 parameterObject, parameterObject["properties"], result, false);

  return OK;
}

std::shared_ptr<CFileItem> CPVROperations::GetRecordingFileItem(int recordingId)
{
  if (CServiceBroker::GetPVRManager().IsStarted())
  {
    const std::shared_ptr<const CPVRRecordings> recordings{
        CServiceBroker::GetPVRManager().Recordings()};

    if (recordings)
    {
      const std::shared_ptr<CPVRRecording> recording{recordings->GetById(recordingId)};
      if (recording)
        return std::make_shared<CFileItem>(recording);
    }
  }

  return {};
}

JSONRPC_STATUS CPVROperations::ParseTimeRange(const CVariant& parameterObject,
                                              bool required,
                                              CDateTime& start,
                                              CDateTime& end)
{
  const std::string startTime{parameterObject["starttime"].asString()};
  const std::string endTime{parameterObject["endtime"].asString()};

  if (startTime.empty() && endTime.empty())
  {
    if (required)
      return InvalidParams;

    start.SetValid(false);
    end.SetValid(false);
    return OK;
  }

  if (!start.SetFromDBDateTime(startTime) || !end.SetFromDBDateTime(endTime) || end < start)
    return InvalidParams;

  return OK;
}

std::vector<std::shared_ptr<CPVREpgInfoTag>> CPVROperations::GetBroadcastsInRange(
    const CPVREpg& epg, const CDateTime& start, const CDateTime& end)
{
  if (!start.IsValid() || !end.IsValid())
    return epg.GetTags();

  // the ranged query fills the gaps between broadcasts with placeholder tags
  std::vector<std::shared_ptr<CPVREpgInfoTag>> tags{epg.GetTimeline(start, end, start, end)};
  std::erase_if(tags, [](const std::shared_ptr<CPVREpgInfoTag>& tag) { return tag->IsGapTag(); });
  return tags;
}
