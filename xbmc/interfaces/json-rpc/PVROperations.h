#pragma once
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

#ifndef JSON_RPC_FILEITEMHANDLER_H_INCLUDED
#define JSON_RPC_FILEITEMHANDLER_H_INCLUDED
#include "FileItemHandler.h"
#endif

#ifndef JSON_RPC_UTILS_STDSTRING_H_INCLUDED
#define JSON_RPC_UTILS_STDSTRING_H_INCLUDED
#include "utils/StdString.h"
#endif

#ifndef JSON_RPC_PVR_CHANNELS_PVRCHANNELGROUP_H_INCLUDED
#define JSON_RPC_PVR_CHANNELS_PVRCHANNELGROUP_H_INCLUDED
#include "pvr/channels/PVRChannelGroup.h"
#endif


namespace JSONRPC
{
  class CPVROperations : public CFileItemHandler
  {
  public:
    static JSONRPC_STATUS GetProperties(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS GetChannelGroups(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS GetChannelGroupDetails(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS GetChannels(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS GetChannelDetails(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS GetBroadcasts(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS GetBroadcastDetails(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS GetTimers(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS GetTimerDetails(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS GetRecordings(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS GetRecordingDetails(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);

    static JSONRPC_STATUS Record(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS Scan(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);

  private:
    static JSONRPC_STATUS GetPropertyValue(const CStdString &property, CVariant &result);
    static void FillChannelGroupDetails(const PVR::CPVRChannelGroupPtr &channelGroup, const CVariant &parameterObject, CVariant &result, bool append = false);
  };
}
