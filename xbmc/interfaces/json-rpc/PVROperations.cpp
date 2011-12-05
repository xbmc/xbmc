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
  if (!parameterObject["channelid"].isInteger())
    return InvalidParams;

  int iChannelId = (int) parameterObject["channelid"].asInteger();

  if (iChannelId > 0)
    {
      CLog::Log(LOGDEBUG, "JSON PVR: Switch channel: %d", iChannelId);

      if ( g_PVRManager.IsStarted() )
      {
        const CPVRChannel *channel = g_PVRChannelGroups->GetByChannelIDFromAll(iChannelId);
        if ( g_PVRManager.StartPlayback(channel, false) )
          {
            return OK;
          }
        else
          {
            return InternalError;
          }
      }
      else
      {
        CLog::Log(LOGDEBUG, "JSON PVR: failed to Switch channels. PVR not started");
        return FailedToExecute;
      }
      return OK;
    }
  else
    {
      return InvalidParams;
    }

}

JSON_STATUS CPVROperations::ChannelUp(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CLog::Log(LOGDEBUG, "JSON PVR: channel up");

  if ( g_PVRManager.IsStarted() && g_PVRManager.IsPlaying() && g_application.m_pPlayer )
  {
    unsigned int iNewChannelNumber(0);
    g_PVRManager.ChannelUp( &iNewChannelNumber );

    CLog::Log(LOGDEBUG, "JSON PVR: new channel %d", iNewChannelNumber);
    return OK;
  }
  else
  {
    CLog::Log(LOGDEBUG, "JSON PVR: PVR not started");
    return FailedToExecute;
  }

}

JSON_STATUS CPVROperations::ChannelDown(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CLog::Log(LOGDEBUG, "JSON PVR: channel down");

  if ( g_PVRManager.IsStarted() && g_PVRManager.IsPlaying() && g_application.m_pPlayer )
  {
    unsigned int iNewChannelNumber(0);
    g_PVRManager.ChannelDown( &iNewChannelNumber );

    CLog::Log(LOGDEBUG, "JSON PVR: new channel %d", iNewChannelNumber);
    return OK;
  }
  else
  {
    CLog::Log(LOGDEBUG, "JSON PVR: PVR not started");
    return FailedToExecute;
  }
}

JSON_STATUS CPVROperations::ChannelRecording(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  if (!parameterObject["on"].isBoolean())
    return InvalidParams;

  bool bOnOff = (bool) parameterObject["on"].asBoolean();

  CLog::Log(LOGDEBUG, "JSON PVR: channel recording on/off %d", bOnOff);

  if ( g_PVRManager.IsStarted() && g_PVRManager.IsPlaying() && g_application.m_pPlayer )
  {
    g_PVRManager.StartRecordingOnPlayingChannel(bOnOff);
    return OK;
  }
  else
  {
    CLog::Log(LOGDEBUG, "JSON PVR: PVR not started");
    return FailedToExecute;
  }
}

JSON_STATUS CPVROperations::ScheduleRecording(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result){

  if (!parameterObject["epgid"].isInteger() || !parameterObject["uniqueid"].isInteger() || !parameterObject["starttime"].isInteger())
    return InvalidParams;

  if ( g_PVRManager.IsStarted() )
    {
      int iEpgId = (int) parameterObject["epgid"].asInteger();
      int iUniqueId = (int) parameterObject["uniqueid"].asInteger();
      int iStartTime = (int) parameterObject["starttime"].asInteger();

      if (iEpgId > 0 && iUniqueId > 0 && iStartTime > 0)
        {
          CDateTime *startTime = new CDateTime( iStartTime );
          CEpgInfoTag *tag = g_EpgContainer.GetById(iEpgId)->GetTag(iUniqueId, *startTime);
          delete startTime;

          if ( tag )
            {
              CPVRTimerInfoTag *newTimer = CPVRTimerInfoTag::CreateFromEpg(*tag);
              bool bReturn = CPVRTimers::AddTimer(*newTimer);

              CLog::Log(LOGDEBUG, "JSON PVR: record result %d", bReturn);

              delete newTimer;
              return OK;
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
    CLog::Log(LOGDEBUG, "JSON PVR: PVR not started");
    return FailedToExecute;
  }

}
