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
  int iChannelId = (int) parameterObject["channelid"].asInteger();

  if ( iChannelId > 0 )
  {
    CLog::Log(LOGDEBUG, "JSONRPC: Switch channel: %d", iChannelId);

    if ( g_PVRManager.IsStarted() )
    {
      const CPVRChannel *channel = g_PVRChannelGroups->GetByChannelIDFromAll(iChannelId);
      if ( g_PVRManager.StartPlayback(channel, false) )
      {
        return ACK;
      }
      else
      {
        return InternalError;
      }
    }
    else
    {
      CLog::Log(LOGDEBUG, "JSONRPC: PVR not started");
      return FailedToExecute;
    }
  }
  else
  {
    return InvalidParams;
  }
}

JSON_STATUS CPVROperations::ChannelUp(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CLog::Log(LOGDEBUG, "JSONRPC: Channel up");

  if ( g_PVRManager.IsStarted() && g_PVRManager.IsPlaying() && g_application.m_pPlayer )
  {
    unsigned int iNewChannelNumber(0);
    g_PVRManager.ChannelUp( &iNewChannelNumber );

    CLog::Log(LOGDEBUG, "JSONRPC: New channel %d", iNewChannelNumber);
    return ACK;
  }
  else
  {
    CLog::Log(LOGDEBUG, "JSONRPC: PVR not started");
    return FailedToExecute;
  }
}

JSON_STATUS CPVROperations::ChannelDown(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CLog::Log(LOGDEBUG, "JSONRPC: channel down");

  if ( g_PVRManager.IsStarted() && g_PVRManager.IsPlaying() && g_application.m_pPlayer )
  {
    unsigned int iNewChannelNumber(0);
    g_PVRManager.ChannelDown( &iNewChannelNumber );

    CLog::Log(LOGDEBUG, "JSONRPC: ChannelDown new channel %d", iNewChannelNumber);
    return ACK;
  }
  else
  {
    CLog::Log(LOGDEBUG, "JSONRPC: PVR not started");
    return FailedToExecute;
  }
}

JSON_STATUS CPVROperations::RecordCurrentChannel(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  if ( g_PVRManager.IsStarted() && g_PVRManager.IsPlaying() && g_application.m_pPlayer )
  {
    CPVRChannel channel;
    if ( g_PVRManager.GetCurrentChannel(&channel) )
      {
        bool bOnOff = !channel.IsRecording();
        g_PVRManager.StartRecordingOnPlayingChannel(bOnOff);
        CLog::Log(LOGDEBUG, "JSONRPC: Recording currently paying channel: %s", bOnOff ? "on" : "off" );
        return ACK;
      }
    else
      {
      CLog::Log(LOGDEBUG, "JSONRPC: Faild to recording currently paying Channel");
      return FailedToExecute;
      }
  }
  else
  {
    CLog::Log(LOGDEBUG, "JSONRPC: PVR not started or no channel is currently playing.");
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
  if ( !g_PVRManager.IsStarted() )
    return FailedToExecute;

  result = g_PVRManager.IsRunningChannelScan();

  return OK;
}

JSON_STATUS CPVROperations::IsRecording(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  if ( !g_PVRManager.IsStarted() )
    return FailedToExecute;

  result = g_PVRManager.IsRecording();

  return OK;
}

JSON_STATUS CPVROperations::ScanChannels(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  if ( !g_PVRManager.IsStarted() )
    return FailedToExecute;

  if ( !g_PVRManager.IsRunningChannelScan() )
  {
    g_PVRManager.StartChannelScan();
  }

  return ACK;
}

JSON_STATUS CPVROperations::ScheduleRecording(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  if ( g_PVRManager.IsStarted() )
    {
      int iEpgId = (int) parameterObject["epgid"].asInteger();
      int iUniqueId = (int) parameterObject["uniqueid"].asInteger();
      int iStartTime = (int) parameterObject["starttime"].asInteger();

      if ( iEpgId > 0 && iUniqueId > 0 && iStartTime > 0 )
        {
          CDateTime *startTime = new CDateTime( iStartTime );
          CEpgInfoTag *tag = g_EpgContainer.GetById(iEpgId)->GetTag(iUniqueId, *startTime);
          delete startTime;

          if ( tag )
            {
              CPVRTimerInfoTag *newTimer = CPVRTimerInfoTag::CreateFromEpg(*tag);
              bool bReturn = CPVRTimers::AddTimer(*newTimer);

              CLog::Log(LOGDEBUG, "JSONRPC: Record result %d", bReturn);

              delete newTimer;
              return ACK;
            }
          else
            {
              return InternalError;
            }
        }
      else
        {
        return InvalidParams;
        }
    }
  else
  {
    CLog::Log(LOGDEBUG, "JSONRPC: PVR not started");
    return FailedToExecute;
  }
}
