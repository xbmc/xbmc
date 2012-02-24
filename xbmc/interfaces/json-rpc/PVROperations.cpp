/*
 *      Copyright (C) 2005-2011 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "PVROperations.h"
#include "Application.h"
#include "utils/log.h"

#include "pvr/PVRManager.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/channels/PVRChannel.h"
#include "pvr/timers/PVRTimers.h"
#include "pvr/timers/PVRTimerInfoTag.h"
#include "epg/EpgInfoTag.h"
#include "epg/EpgContainer.h"

using namespace JSONRPC;
using namespace PVR;
using namespace EPG;

JSON_STATUS CPVROperations::ChannelSwitch(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  if (!g_PVRManager.IsStarted())
  {
    CLog::Log(LOGDEBUG, "JSONRPC: PVR not started");
    return FailedToExecute;
  }

  int iChannelId = (int)parameterObject["channelid"].asInteger();
  if (iChannelId <= 0)
    return InvalidParams;

  CLog::Log(LOGDEBUG, "JSONRPC: switch to channel '%d'", iChannelId);

  const CPVRChannel *channel = g_PVRChannelGroups->GetByChannelIDFromAll(iChannelId);
  if (channel == NULL)
    return InternalError;

  CPVRChannel currentChannel;
  if (g_PVRManager.GetCurrentChannel(currentChannel) && currentChannel.IsRadio() == channel->IsRadio())
    g_application.getApplicationMessenger().SendAction(CAction(ACTION_CHANNEL_SWITCH, (float)channel->ChannelNumber()));
  else
    g_application.getApplicationMessenger().MediaPlay(CFileItem(*channel));
  return ACK;
}

JSON_STATUS CPVROperations::ChannelUp(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  if (!g_PVRManager.IsStarted())
  {
    CLog::Log(LOGDEBUG, "JSONRPC: PVR not started");
    return FailedToExecute;
  }

  CLog::Log(LOGDEBUG, "JSONRPC: channel up");
  g_application.getApplicationMessenger().SendAction(CAction(ACTION_NEXT_ITEM));
  return ACK;
}

JSON_STATUS CPVROperations::ChannelDown(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  if (!g_PVRManager.IsStarted())
  {
    CLog::Log(LOGDEBUG, "JSONRPC: PVR not started");
    return FailedToExecute;
  }

  CLog::Log(LOGDEBUG, "JSONRPC: channel down");
  g_application.getApplicationMessenger().SendAction(CAction(ACTION_PREV_ITEM));
  return ACK;
}

JSON_STATUS CPVROperations::RecordCurrentChannel(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  if (!g_PVRManager.IsStarted())
  {
    CLog::Log(LOGDEBUG, "JSONRPC: PVR not started");
    return FailedToExecute;
  }

  CPVRChannel currentChannel;
  if (g_PVRManager.GetCurrentChannel(currentChannel))
  {
    bool bOnOff = !currentChannel.IsRecording();
    if (g_PVRManager.StartRecordingOnPlayingChannel(bOnOff))
    {
      CLog::Log(LOGDEBUG, "JSONRPC: set recording on currently playing channel to '%s'", bOnOff ? "on" : "off" );
      return ACK;
    }
    else
    {
      CLog::Log(LOGERROR, "JSONRPC: unable to set recording to '%s'", bOnOff ? "on" : "off" );
      return InternalError;
    }
  }
  else
  {
    CLog::Log(LOGERROR, "JSONRPC: failed to start recording - no channel is playing");
    return FailedToExecute;
  }
}

JSON_STATUS CPVROperations::IsAvailable(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  result = g_PVRManager.IsStarted();

  return OK;
}

JSON_STATUS CPVROperations::IsScanningChannels(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  if (!g_PVRManager.IsStarted())
  {
    CLog::Log(LOGDEBUG, "JSONRPC: PVR not started");
    return FailedToExecute;
  }

  result = g_PVRManager.IsRunningChannelScan();

  return OK;
}

JSON_STATUS CPVROperations::IsRecording(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  if (!g_PVRManager.IsStarted())
  {
    CLog::Log(LOGDEBUG, "JSONRPC: PVR not started");
    return FailedToExecute;
  }

  result = g_PVRManager.IsRecording();

  return OK;
}

JSON_STATUS CPVROperations::ScanChannels(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  if (!g_PVRManager.IsStarted())
  {
    CLog::Log(LOGDEBUG, "JSONRPC: PVR not started");
    return FailedToExecute;
  }

  if (!g_PVRManager.IsRunningChannelScan())
    g_PVRManager.StartChannelScan();

  return ACK;
}

JSON_STATUS CPVROperations::ScheduleRecording(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  if (!g_PVRManager.IsStarted())
  {
    CLog::Log(LOGDEBUG, "JSONRPC: PVR not started");
    return FailedToExecute;
  }

  int iEpgId     = (int)parameterObject["epgid"].asInteger();
  int iUniqueId  = (int)parameterObject["uniqueid"].asInteger();
  int iStartTime = (int)parameterObject["starttime"].asInteger();

  if (iEpgId > 0 && iUniqueId > 0 && iStartTime > 0)
  {
    CDateTime startTime(iStartTime);
    CEpgInfoTag *tag = g_EpgContainer.GetById(iEpgId)->GetTag(iUniqueId, startTime);

    if (tag && tag->HasPVRChannel())
    {
      CLog::Log(LOGDEBUG, "JSONRPC: schedule recording - channel: '%s' start: '%s' end: '%s'", tag->PVRChannelName().c_str(), tag->StartAsLocalTime().GetAsLocalizedDateTime(false, false).c_str(), tag->EndAsLocalTime().GetAsLocalizedDateTime(false, false).c_str());

      CPVRTimerInfoTag *newTimer = CPVRTimerInfoTag::CreateFromEpg(*tag);
      bool bCreated = (newTimer != NULL);
      bool bAdded = false;

      if (bCreated)
      {
        CLog::Log(LOGDEBUG, "JSONRPC: recording scheduled");
        bAdded = CPVRTimers::AddTimer(*newTimer);
      }
      else
      {
        CLog::Log(LOGERROR, "JSONRPC: failed to schedule recording");
      }
      delete newTimer;
      return bAdded ? ACK : InternalError;
    }
  }

  return InvalidParams;
}
