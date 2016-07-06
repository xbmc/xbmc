/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      http://xbmc.org
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

#include "PVROperations.h"
#include "messaging/ApplicationMessenger.h"

#include "pvr/PVRManager.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/channels/PVRChannel.h"
#include "pvr/timers/PVRTimers.h"
#include "pvr/recordings/PVRRecordings.h"
#include "epg/Epg.h"
#include "epg/EpgContainer.h"
#include "utils/Variant.h"

using namespace JSONRPC;
using namespace PVR;
using namespace EPG;
using namespace KODI::MESSAGING;

JSONRPC_STATUS CPVROperations::GetProperties(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  if (!g_PVRManager.IsStarted())
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
  if (!g_PVRManager.IsStarted())
    return FailedToExecute;
  
  CPVRChannelGroupsContainer *channelGroupContainer = g_PVRChannelGroups;
  if (channelGroupContainer == NULL)
    return FailedToExecute;

  CPVRChannelGroups *channelGroups = channelGroupContainer->Get(parameterObject["channeltype"].asString().compare("radio") == 0);
  if (channelGroups == NULL)
    return FailedToExecute;

  int start, end;

  std::vector<CPVRChannelGroupPtr> groupList = channelGroups->GetMembers(true);
  HandleLimits(parameterObject, result, groupList.size(), start, end);
  for (int index = start; index < end; index++)
    FillChannelGroupDetails(groupList.at(index), parameterObject, result["channelgroups"], true);

  return OK;
}

JSONRPC_STATUS CPVROperations::GetChannelGroupDetails(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  if (!g_PVRManager.IsStarted())
    return FailedToExecute;

  CPVRChannelGroupsContainer *channelGroupContainer = g_PVRChannelGroups;
  if (channelGroupContainer == NULL)
    return FailedToExecute;
  
  CPVRChannelGroupPtr channelGroup;
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
  if (!g_PVRManager.IsStarted())
    return FailedToExecute;
  
  CPVRChannelGroupsContainer *channelGroupContainer = g_PVRChannelGroups;
  if (channelGroupContainer == NULL)
    return FailedToExecute;
  
  CPVRChannelGroupPtr channelGroup;
  CVariant id = parameterObject["channelgroupid"];
  if (id.isInteger())
    channelGroup = channelGroupContainer->GetByIdFromAll((int)id.asInteger());
  else if (id.isString())
    channelGroup = channelGroupContainer->GetGroupAll(id.asString() == "allradio");
  
  if (channelGroup == NULL)
    return InvalidParams;
  
  CFileItemList channels;
  if (channelGroup->GetMembers(channels) < 0)
    return InvalidParams;
  
  HandleFileItemList("channelid", false, "channels", channels, parameterObject, result, true);
    
  return OK;
}

JSONRPC_STATUS CPVROperations::GetChannelDetails(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  if (!g_PVRManager.IsStarted())
    return FailedToExecute;
  
  CPVRChannelGroupsContainer *channelGroupContainer = g_PVRChannelGroups;
  if (channelGroupContainer == NULL)
    return FailedToExecute;
  
  CPVRChannelPtr channel = channelGroupContainer->GetChannelById((int)parameterObject["channelid"].asInteger());
  if (channel == NULL)
    return InvalidParams;

  HandleFileItem("channelid", false, "channeldetails", CFileItemPtr(new CFileItem(channel)), parameterObject, parameterObject["properties"], result, false);
    
  return OK;
}

JSONRPC_STATUS CPVROperations::GetBroadcasts(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  if (!g_PVRManager.IsStarted())
    return FailedToExecute;

  CPVRChannelGroupsContainer *channelGroupContainer = g_PVRManager.ChannelGroups();
  if (channelGroupContainer == NULL)
    return FailedToExecute;

  CPVRChannelPtr channel = channelGroupContainer->GetChannelById((int)parameterObject["channelid"].asInteger());
  if (channel == NULL)
    return InvalidParams;

  CEpgPtr channelEpg = channel->GetEPG();
  if (!channelEpg)
    return InternalError;

  CFileItemList programFull;
  channelEpg->Get(programFull);

  HandleFileItemList("broadcastid", false, "broadcasts", programFull, parameterObject, result, programFull.Size(), true);

  return OK;
}

JSONRPC_STATUS CPVROperations::GetBroadcastDetails(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  if (!g_PVRManager.IsStarted())
    return FailedToExecute;

  EpgSearchFilter filter;
  filter.Reset();
  filter.m_iUniqueBroadcastId = parameterObject["broadcastid"].asUnsignedInteger();

  CFileItemList broadcasts;
  int resultSize = g_EpgContainer.GetEPGSearch(broadcasts, filter);

  if (resultSize <= 0)
    return InvalidParams;
  else if (resultSize > 1)
    return InternalError;

  CFileItemPtr broadcast = broadcasts.Get(0);
  HandleFileItem("broadcastid", false, "broadcastdetails", broadcast, parameterObject, parameterObject["properties"], result, false);

  return OK;
}


JSONRPC_STATUS CPVROperations::Record(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  if (!g_PVRManager.IsStarted())
    return FailedToExecute;

  CPVRChannelPtr pChannel;
  CVariant channel = parameterObject["channel"];
  if (channel.isString() && channel.asString() == "current")
  {
    pChannel = g_PVRManager.GetCurrentChannel();
    if (!pChannel)
      return InternalError;
  }
  else if (channel.isInteger())
  {
    CPVRChannelGroupsContainer *channelGroupContainer = g_PVRManager.ChannelGroups();
    if (channelGroupContainer == NULL)
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
  bool toggle = true;
  if (record.isBoolean() && record.asBoolean() == pChannel->IsRecording())
    toggle = false;

  if (toggle)
  {
    if (!g_PVRManager.ToggleRecordingOnChannel(pChannel->ChannelID()))
      return FailedToExecute;
  }

  return ACK;
}

JSONRPC_STATUS CPVROperations::Scan(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  if (!g_PVRManager.IsStarted())
    return FailedToExecute;

  if (!g_PVRManager.IsRunningChannelScan())
    g_PVRManager.StartChannelScan();

  return ACK;
}

JSONRPC_STATUS CPVROperations::GetPropertyValue(const std::string &property, CVariant &result)
{
  bool started = g_PVRManager.IsStarted();

  if (property == "available")
    result = started;
  else if (property == "recording")
  {
    if (started)
      result = g_PVRManager.IsRecording();
    else
      result = false;
  }
  else if (property == "scanning")
  {
    if (started)
      result = g_PVRManager.IsRunningChannelScan();
    else
      result = false;
  }
  else
    return InvalidParams;

  return OK;
}

void CPVROperations::FillChannelGroupDetails(const CPVRChannelGroupPtr &channelGroup, const CVariant &parameterObject, CVariant &result, bool append /* = false */)
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
    channelGroup->GetMembers(channels);
    object["channels"] = CVariant(CVariant::VariantTypeArray);
    HandleFileItemList("channelid", false, "channels", channels, parameterObject["channels"], object, false);

    result = object;
  }
}

JSONRPC_STATUS CPVROperations::GetTimers(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  if (!g_PVRManager.IsStarted())
    return FailedToExecute;

  CPVRTimers* timers = g_PVRTimers;
  if (!timers)
    return FailedToExecute;

  CFileItemList timerList;
  timers->GetAll(timerList);

  HandleFileItemList("timerid", false, "timers", timerList, parameterObject, result, true);

  return OK;
}

JSONRPC_STATUS CPVROperations::GetTimerDetails(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  if (!g_PVRManager.IsStarted())
    return FailedToExecute;

  CPVRTimers* timers = g_PVRTimers;
  if (!timers)
    return FailedToExecute;

  CPVRTimerInfoTagPtr timer = timers->GetById((int)parameterObject["timerid"].asInteger());
  if (!timer)
    return InvalidParams;

  HandleFileItem("timerid", false, "timerdetails", CFileItemPtr(new CFileItem(timer)), parameterObject, parameterObject["properties"], result, false);

  return OK;
}

CFileItemPtr CPVROperations::GetBroadcastFromBroadcastid(unsigned int broadcastid)
{
  EpgSearchFilter filter;
  filter.Reset();
  filter.m_iUniqueBroadcastId = broadcastid;

  CFileItemList broadcasts;
  int resultSize = g_EpgContainer.GetEPGSearch(broadcasts, filter);

  if (resultSize != 1)
    return CFileItemPtr();

  return broadcasts.Get(0);
}

JSONRPC_STATUS CPVROperations::AddTimer(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  if (!g_PVRManager.IsStarted())
    return FailedToExecute;

  CFileItemPtr broadcast = CPVROperations::GetBroadcastFromBroadcastid(parameterObject["broadcastid"].asUnsignedInteger());
  if (!broadcast)
    return InvalidParams;

  if (!broadcast->HasEPGInfoTag())
    return InvalidParams;

  CEpgInfoTagPtr epgTag = broadcast->GetEPGInfoTag();

  if (!epgTag)
    return InvalidParams;

  if (epgTag->HasTimer())
    return InvalidParams;

  CPVRTimerInfoTagPtr newTimer = CPVRTimerInfoTag::CreateFromEpg(epgTag, parameterObject["timerrule"].asBoolean(false));
  if (newTimer)
  {
    if (g_PVRTimers->AddTimer(newTimer))
      return ACK;
  }
  return FailedToExecute;
}


JSONRPC_STATUS CPVROperations::DeleteTimer(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  if (!g_PVRManager.IsStarted())
    return FailedToExecute;

  CPVRTimers* timers = g_PVRTimers;

  CPVRTimerInfoTagPtr timer = timers->GetById(parameterObject["timerid"].asInteger());
  if (!timer)
    return InvalidParams;

  if (timers->DeleteTimer(timer, timer->IsRecording(), false))
    return ACK;

  return FailedToExecute;
}

JSONRPC_STATUS CPVROperations::ToggleTimer(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  if (!g_PVRManager.IsStarted())
    return FailedToExecute;

  CFileItemPtr broadcast = CPVROperations::GetBroadcastFromBroadcastid(parameterObject["broadcastid"].asUnsignedInteger());
  if (!broadcast)
    return InvalidParams;

  if (!broadcast->HasEPGInfoTag())
    return InvalidParams;

  CEpgInfoTagPtr epgTag = broadcast->GetEPGInfoTag();

  if (!epgTag)
    return InvalidParams;

  bool timerrule = parameterObject["timerrule"].asBoolean(false);
  bool sentOkay = false;
  CPVRTimerInfoTagPtr timer(epgTag->Timer());
  if (timer)
  {
    if (timerrule)
      timer = g_PVRTimers->GetTimerRule(timer);

    if (timer)
      sentOkay = g_PVRTimers->DeleteTimer(timer, timer->IsRecording(), false);
  }
  else
  {
    timer = CPVRTimerInfoTag::CreateFromEpg(epgTag, timerrule);
    sentOkay = g_PVRTimers->AddTimer(timer);
  }

  if (sentOkay)
    return ACK;

  return FailedToExecute;
}

JSONRPC_STATUS CPVROperations::GetRecordings(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  if (!g_PVRManager.IsStarted())
    return FailedToExecute;

  CPVRRecordings* recordings = g_PVRRecordings;
  if (!recordings)
    return FailedToExecute;

  CFileItemList recordingsList;
  recordings->GetAll(recordingsList);

  HandleFileItemList("recordingid", true, "recordings", recordingsList, parameterObject, result, true);

  return OK;
}

JSONRPC_STATUS CPVROperations::GetRecordingDetails(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  if (!g_PVRManager.IsStarted())
    return FailedToExecute;

  CPVRRecordings* recordings = g_PVRRecordings;
  if (!recordings)
    return FailedToExecute;

  CFileItemPtr recording = recordings->GetById((int)parameterObject["recordingid"].asInteger());
  if (!recording)
    return InvalidParams;

  HandleFileItem("recordingid", true, "recordingdetails", recording, parameterObject, parameterObject["properties"], result, false);

  return OK;
}
