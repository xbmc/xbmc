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
#include "pvr/PVRManager.h"
#include "pvr/PVRPlaybackState.h"
#include "pvr/addons/PVRClients.h"
#include "pvr/channels/PVRChannel.h"
#include "pvr/channels/PVRChannelGroupMember.h"
#include "pvr/channels/PVRChannelGroups.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/epg/Epg.h"
#include "pvr/epg/EpgContainer.h"
#include "pvr/epg/EpgInfoTag.h"
#include "pvr/guilib/PVRGUIActionsChannels.h"
#include "pvr/guilib/PVRGUIActionsTimers.h"
#include "pvr/recordings/PVRRecordings.h"
#include "pvr/timers/PVRTimerInfoTag.h"
#include "pvr/timers/PVRTimers.h"
#include "utils/Variant.h"

#include <memory>

using namespace JSONRPC;
using namespace PVR;
using namespace KODI::MESSAGING;

JSONRPC_STATUS CPVROperations::GetProperties(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  if (!CServiceBroker::GetPVRManager().IsStarted())
    return FailedToExecute;

  CVariant properties = CVariant(CVariant::VariantTypeObject);
  for (unsigned int index = 0; index < parameterObject["properties"].size(); index++)
  {
    std::string propertyName = parameterObject["properties"][index].asString();
    CVariant property;
    JSONRPC_STATUS ret;
    if ((ret = GetPropertyValue(propertyName, property)) != OK)
      return ret;

    properties[propertyName] = property;
  }

  result = properties;

  return OK;
}

JSONRPC_STATUS CPVROperations::GetChannelGroups(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  if (!CServiceBroker::GetPVRManager().IsStarted())
    return FailedToExecute;

  std::shared_ptr<CPVRChannelGroupsContainer> channelGroupContainer = CServiceBroker::GetPVRManager().ChannelGroups();
  if (!channelGroupContainer)
    return FailedToExecute;

  CPVRChannelGroups *channelGroups = channelGroupContainer->Get(parameterObject["channeltype"].asString().compare("radio") == 0);
  if (channelGroups == NULL)
    return FailedToExecute;

  int start, end;

  std::vector<std::shared_ptr<CPVRChannelGroup>> groupList = channelGroups->GetMembers(true);
  HandleLimits(parameterObject, result, groupList.size(), start, end);
  for (int index = start; index < end; index++)
    FillChannelGroupDetails(groupList.at(index), parameterObject, result["channelgroups"], true);

  return OK;
}

JSONRPC_STATUS CPVROperations::GetChannelGroupDetails(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  if (!CServiceBroker::GetPVRManager().IsStarted())
    return FailedToExecute;

  std::shared_ptr<CPVRChannelGroupsContainer> channelGroupContainer = CServiceBroker::GetPVRManager().ChannelGroups();
  if (!channelGroupContainer)
    return FailedToExecute;

  std::shared_ptr<CPVRChannelGroup> channelGroup;
  CVariant id = parameterObject["channelgroupid"];
  if (id.isInteger())
    channelGroup = channelGroupContainer->GetByIdFromAll((int)id.asInteger());
  else if (id.isString())
    channelGroup = channelGroupContainer->GetGroupAll(id.asString() == "allradio");

  if (channelGroup == NULL)
    return InvalidParams;

  FillChannelGroupDetails(channelGroup, parameterObject, result["channelgroupdetails"], false);

  return OK;
}

JSONRPC_STATUS CPVROperations::GetChannels(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  if (!CServiceBroker::GetPVRManager().IsStarted())
    return FailedToExecute;

  std::shared_ptr<CPVRChannelGroupsContainer> channelGroupContainer = CServiceBroker::GetPVRManager().ChannelGroups();
  if (!channelGroupContainer)
    return FailedToExecute;

  std::shared_ptr<CPVRChannelGroup> channelGroup;
  CVariant id = parameterObject["channelgroupid"];
  if (id.isInteger())
    channelGroup = channelGroupContainer->GetByIdFromAll((int)id.asInteger());
  else if (id.isString())
    channelGroup = channelGroupContainer->GetGroupAll(id.asString() == "allradio");

  if (channelGroup == NULL)
    return InvalidParams;

  CFileItemList channels;
  const auto groupMembers = channelGroup->GetMembers(CPVRChannelGroup::Include::ONLY_VISIBLE);
  for (const auto& groupMember : groupMembers)
  {
    channels.Add(std::make_shared<CFileItem>(groupMember));
  }

  HandleFileItemList("channelid", false, "channels", channels, parameterObject, result, true);

  return OK;
}

JSONRPC_STATUS CPVROperations::GetChannelDetails(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  if (!CServiceBroker::GetPVRManager().IsStarted())
    return FailedToExecute;

  std::shared_ptr<CPVRChannelGroupsContainer> channelGroupContainer = CServiceBroker::GetPVRManager().ChannelGroups();
  if (!channelGroupContainer)
    return FailedToExecute;

  std::shared_ptr<CPVRChannel> channel = channelGroupContainer->GetChannelById(
      static_cast<int>(parameterObject["channelid"].asInteger()));
  if (channel == NULL)
    return InvalidParams;

  const std::shared_ptr<CPVRChannelGroupMember> groupMember =
      CServiceBroker::GetPVRManager().Get<PVR::GUI::Channels>().GetChannelGroupMember(channel);
  if (!groupMember)
    return InvalidParams;

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

  int start, end;

  auto clientInfos = CServiceBroker::GetPVRManager().Clients()->GetEnabledClientInfos();
  HandleLimits(parameterObject, result, clientInfos.size(), start, end);
  for (int index = start; index < end; index++)
    result["clients"].append(clientInfos[index]);

  return OK;
}

JSONRPC_STATUS CPVROperations::GetBroadcasts(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  if (!CServiceBroker::GetPVRManager().IsStarted())
    return FailedToExecute;

  std::shared_ptr<CPVRChannelGroupsContainer> channelGroupContainer = CServiceBroker::GetPVRManager().ChannelGroups();
  if (!channelGroupContainer)
    return FailedToExecute;

  std::shared_ptr<CPVRChannel> channel = channelGroupContainer->GetChannelById((int)parameterObject["channelid"].asInteger());
  if (channel == NULL)
    return InvalidParams;

  std::shared_ptr<CPVREpg> channelEpg = channel->GetEPG();
  if (!channelEpg)
    return InternalError;

  CFileItemList programFull;

  const std::vector<std::shared_ptr<CPVREpgInfoTag>> tags = channelEpg->GetTags();
  for (const auto& tag : tags)
  {
    programFull.Add(std::make_shared<CFileItem>(tag));
  }

  HandleFileItemList("broadcastid", false, "broadcasts", programFull, parameterObject, result, programFull.Size(), true);

  return OK;
}

JSONRPC_STATUS CPVROperations::GetBroadcastDetails(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  if (!CServiceBroker::GetPVRManager().IsStarted())
    return FailedToExecute;

  const std::shared_ptr<CPVREpgInfoTag> epgTag =
      CServiceBroker::GetPVRManager().EpgContainer().GetTagByDatabaseId(
          parameterObject["broadcastid"].asInteger());

  if (!epgTag)
    return InvalidParams;

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

  const std::shared_ptr<const CPVREpgInfoTag> epgTag =
      CServiceBroker::GetPVRManager().EpgContainer().GetTagByDatabaseId(
          parameterObject["broadcastid"].asInteger());

  if (!epgTag)
    return InvalidParams;

  result = epgTag->IsPlayable();

  return OK;
}

JSONRPC_STATUS CPVROperations::Record(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  if (!CServiceBroker::GetPVRManager().IsStarted())
    return FailedToExecute;

  std::shared_ptr<CPVRChannel> pChannel;
  CVariant channel = parameterObject["channel"];
  if (channel.isString() && channel.asString() == "current")
  {
    pChannel = CServiceBroker::GetPVRManager().PlaybackState()->GetPlayingChannel();
    if (!pChannel)
      return InternalError;
  }
  else if (channel.isInteger())
  {
    std::shared_ptr<CPVRChannelGroupsContainer> channelGroupContainer = CServiceBroker::GetPVRManager().ChannelGroups();
    if (!channelGroupContainer)
      return FailedToExecute;

    pChannel = channelGroupContainer->GetChannelById((int)channel.asInteger());
  }
  else
    return InvalidParams;

  if (pChannel == NULL)
    return InvalidParams;
  else if (!pChannel->CanRecord())
    return FailedToExecute;

  CVariant record = parameterObject["record"];
  bool bIsRecording = CServiceBroker::GetPVRManager().Timers()->IsRecordingOnChannel(*pChannel);
  bool toggle = true;
  if (record.isBoolean() && record.asBoolean() == bIsRecording)
    toggle = false;

  if (toggle)
  {
    if (!CServiceBroker::GetPVRManager().Get<PVR::GUI::Timers>().SetRecordingOnChannel(
            pChannel, !bIsRecording))
      return FailedToExecute;
  }

  return ACK;
}

JSONRPC_STATUS CPVROperations::Scan(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  if (!CServiceBroker::GetPVRManager().IsStarted())
    return FailedToExecute;

  if (parameterObject.isMember("clientid"))
  {
    if (CServiceBroker::GetPVRManager().Get<PVR::GUI::Channels>().StartChannelScan(
            parameterObject["clientid"].asInteger()))
      return ACK;
  }
  else
  {
    if (CServiceBroker::GetPVRManager().Get<PVR::GUI::Channels>().StartChannelScan())
      return ACK;
  }

  return FailedToExecute;
}

JSONRPC_STATUS CPVROperations::GetPropertyValue(const std::string &property, CVariant &result)
{
  bool started = CServiceBroker::GetPVRManager().IsStarted();

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
  if (channelGroup == NULL)
    return;

  CVariant object(CVariant::VariantTypeObject);
  object["channelgroupid"] = channelGroup->GroupID();
  object["channeltype"] = channelGroup->IsRadio() ? "radio" : "tv";
  object["label"] = channelGroup->GroupName();

  if (append)
    result.append(object);
  else
  {
    CFileItemList channels;
    const auto groupMembers = channelGroup->GetMembers(CPVRChannelGroup::Include::ONLY_VISIBLE);
    for (const auto& groupMember : groupMembers)
    {
      channels.Add(std::make_shared<CFileItem>(groupMember));
    }

    object["channels"] = CVariant(CVariant::VariantTypeArray);
    HandleFileItemList("channelid", false, "channels", channels, parameterObject["channels"], object, false);

    result = object;
  }
}

JSONRPC_STATUS CPVROperations::GetTimers(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  if (!CServiceBroker::GetPVRManager().IsStarted())
    return FailedToExecute;

  std::shared_ptr<CPVRTimers> timers = CServiceBroker::GetPVRManager().Timers();
  if (!timers)
    return FailedToExecute;

  CFileItemList timerList;
  const std::vector<std::shared_ptr<CPVRTimerInfoTag>> tags = timers->GetAll();
  for (const auto& timer : tags)
  {
    timerList.Add(std::make_shared<CFileItem>(timer));
  }

  HandleFileItemList("timerid", false, "timers", timerList, parameterObject, result, true);

  return OK;
}

JSONRPC_STATUS CPVROperations::GetTimerDetails(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  if (!CServiceBroker::GetPVRManager().IsStarted())
    return FailedToExecute;

  std::shared_ptr<CPVRTimers> timers = CServiceBroker::GetPVRManager().Timers();
  if (!timers)
    return FailedToExecute;

  std::shared_ptr<CPVRTimerInfoTag> timer = timers->GetById((int)parameterObject["timerid"].asInteger());
  if (!timer)
    return InvalidParams;

  HandleFileItem("timerid", false, "timerdetails", std::make_shared<CFileItem>(timer),
                 parameterObject, parameterObject["properties"], result, false);

  return OK;
}

JSONRPC_STATUS CPVROperations::AddTimer(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  if (!CServiceBroker::GetPVRManager().IsStarted())
    return FailedToExecute;

  const std::shared_ptr<CPVREpgInfoTag> epgTag =
      CServiceBroker::GetPVRManager().EpgContainer().GetTagByDatabaseId(
          parameterObject["broadcastid"].asInteger());

  if (!epgTag)
    return InvalidParams;

  if (CServiceBroker::GetPVRManager().Timers()->GetTimerForEpgTag(epgTag))
    return InvalidParams;

  const std::shared_ptr<CPVRTimerInfoTag> newTimer =
      CPVRTimerInfoTag::CreateFromEpg(epgTag, parameterObject["timerrule"].asBoolean(false),
                                      parameterObject["reminder"].asBoolean(false));
  if (newTimer)
  {
    if (CServiceBroker::GetPVRManager().Get<PVR::GUI::Timers>().AddTimer(newTimer))
      return ACK;
  }
  return FailedToExecute;
}


JSONRPC_STATUS CPVROperations::DeleteTimer(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  if (!CServiceBroker::GetPVRManager().IsStarted())
    return FailedToExecute;

  std::shared_ptr<CPVRTimers> timers = CServiceBroker::GetPVRManager().Timers();
  if (!timers)
    return FailedToExecute;

  std::shared_ptr<CPVRTimerInfoTag> timer = timers->GetById(parameterObject["timerid"].asInteger());
  if (!timer)
    return InvalidParams;

  if (timers->DeleteTimer(timer, timer->IsRecording(), false) == TimerOperationResult::OK)
    return ACK;

  return FailedToExecute;
}

JSONRPC_STATUS CPVROperations::ToggleTimer(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  if (!CServiceBroker::GetPVRManager().IsStarted())
    return FailedToExecute;

  const std::shared_ptr<CPVREpgInfoTag> epgTag =
      CServiceBroker::GetPVRManager().EpgContainer().GetTagByDatabaseId(
          parameterObject["broadcastid"].asInteger());

  if (!epgTag)
    return InvalidParams;

  bool timerrule = parameterObject["timerrule"].asBoolean(false);
  bool sentOkay = false;
  std::shared_ptr<CPVRTimerInfoTag> timer = CServiceBroker::GetPVRManager().Timers()->GetTimerForEpgTag(epgTag);
  if (timer)
  {
    if (timerrule)
      timer = CServiceBroker::GetPVRManager().Timers()->GetTimerRule(timer);

    if (timer)
      sentOkay = (CServiceBroker::GetPVRManager().Timers()->DeleteTimer(timer, timer->IsRecording(), false) == TimerOperationResult::OK);
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

JSONRPC_STATUS CPVROperations::GetRecordings(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  if (!CServiceBroker::GetPVRManager().IsStarted())
    return FailedToExecute;

  std::shared_ptr<CPVRRecordings> recordings = CServiceBroker::GetPVRManager().Recordings();
  if (!recordings)
    return FailedToExecute;

  CFileItemList recordingsList;
  const std::vector<std::shared_ptr<CPVRRecording>> recs = recordings->GetAll();
  for (const auto& recording : recs)
  {
    recordingsList.Add(std::make_shared<CFileItem>(recording));
  }

  HandleFileItemList("recordingid", true, "recordings", recordingsList, parameterObject, result, true);

  return OK;
}

JSONRPC_STATUS CPVROperations::GetRecordingDetails(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  if (!CServiceBroker::GetPVRManager().IsStarted())
    return FailedToExecute;

  std::shared_ptr<CPVRRecordings> recordings = CServiceBroker::GetPVRManager().Recordings();
  if (!recordings)
    return FailedToExecute;

  const std::shared_ptr<CPVRRecording> recording = recordings->GetById(static_cast<int>(parameterObject["recordingid"].asInteger()));
  if (!recording)
    return InvalidParams;

  HandleFileItem("recordingid", true, "recordingdetails", std::make_shared<CFileItem>(recording), parameterObject, parameterObject["properties"], result, false);

  return OK;
}

std::shared_ptr<CFileItem> CPVROperations::GetRecordingFileItem(int recordingId)
{
  if (CServiceBroker::GetPVRManager().IsStarted())
  {
    const std::shared_ptr<PVR::CPVRRecordings> recordings =
        CServiceBroker::GetPVRManager().Recordings();

    if (recordings)
    {
      const std::shared_ptr<PVR::CPVRRecording> recording = recordings->GetById(recordingId);
      if (recording)
        return std::make_shared<CFileItem>(recording);
    }
  }

  return {};
}
