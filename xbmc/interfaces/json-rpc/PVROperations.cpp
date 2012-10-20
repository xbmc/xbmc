/*
 *      Copyright (C) 2012 Team XBMC
 *      http://www.xbmc.org
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
#include "ApplicationMessenger.h"
#include "utils/log.h"

#include "pvr/PVRManager.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/channels/PVRChannel.h"
#include "pvr/timers/PVRTimers.h"
#include "pvr/timers/PVRTimerInfoTag.h"
#include "epg/Epg.h"
#include "epg/EpgContainer.h"

using namespace std;
using namespace JSONRPC;
using namespace PVR;
using namespace EPG;

JSONRPC_STATUS CPVROperations::GetProperties(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  if (!g_PVRManager.IsStarted())
    return FailedToExecute;
  
  CVariant properties = CVariant(CVariant::VariantTypeObject);
  for (unsigned int index = 0; index < parameterObject["properties"].size(); index++)
  {
    CStdString propertyName = parameterObject["properties"][index].asString();
    CVariant property;
    JSONRPC_STATUS ret;
    if ((ret = GetPropertyValue(propertyName, property)) != OK)
      return ret;

    properties[propertyName] = property;
  }

  result = properties;

  return OK;
}

JSONRPC_STATUS CPVROperations::GetChannelGroups(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
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

  vector<CPVRChannelGroupPtr> groupList = channelGroups->GetMembers();
  HandleLimits(parameterObject, result, groupList.size(), start, end);
  for (int index = start; index < end; index++)
    FillChannelGroupDetails(groupList.at(index), parameterObject, result["channelgroups"], true);

  return OK;
}

JSONRPC_STATUS CPVROperations::GetChannelGroupDetails(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
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

JSONRPC_STATUS CPVROperations::GetChannels(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
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
  
  HandleFileItemList("channelid", false, "channels", channels, parameterObject, result, false);
    
  return OK;
}

JSONRPC_STATUS CPVROperations::GetChannelDetails(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  if (!g_PVRManager.IsStarted())
    return FailedToExecute;
  
  CPVRChannelGroupsContainer *channelGroupContainer = g_PVRChannelGroups;
  if (channelGroupContainer == NULL)
    return FailedToExecute;
  
  CPVRChannelPtr channel = channelGroupContainer->GetChannelById((int)parameterObject["channelid"].asInteger());
  if (channel == NULL)
    return InvalidParams;

  HandleFileItem("channelid", false, "channeldetails", CFileItemPtr(new CFileItem(*channel)), parameterObject, parameterObject["properties"], result, false);
    
  return OK;
}

JSONRPC_STATUS CPVROperations::Record(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  if (!g_PVRManager.IsStarted())
    return FailedToExecute;

  CPVRChannelPtr pChannel;
  CVariant channel = parameterObject["channel"];
  if (channel.isString() && channel.asString() == "current")
  {
    if (!g_PVRManager.GetCurrentChannel(pChannel))
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

JSONRPC_STATUS CPVROperations::Scan(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  if (!g_PVRManager.IsStarted())
    return FailedToExecute;

  if (!g_PVRManager.IsRunningChannelScan())
    g_PVRManager.StartChannelScan();

  return ACK;
}

JSONRPC_STATUS CPVROperations::GetPropertyValue(const CStdString &property, CVariant &result)
{
  bool started = g_PVRManager.IsStarted();

  if (property.Equals("available"))
    result = started;
  else if (property.Equals("recording"))
  {
    if (started)
      result = g_PVRManager.IsRecording();
    else
      result = false;
  }
  else if (property.Equals("scanning"))
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