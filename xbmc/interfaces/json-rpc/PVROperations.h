/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "FileItemHandler.h"

#include <memory>
#include <vector>

class CDateTime;
class CVariant;

namespace PVR
{
class CPVRChannelGroup;
class CPVREpg;
class CPVREpgInfoTag;
class CPVRProvider;
}

namespace JSONRPC
{
  class CPVROperations : public CFileItemHandler
  {
  public:
    static JSONRPC_STATUS GetProperties(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS GetChannelGroups(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS GetChannelGroupDetails(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS GetChannels(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS GetChannelDetails(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS GetClients(const std::string& method,
                                     ITransportLayer* transport,
                                     IClient* client,
                                     const CVariant& parameterObject,
                                     CVariant& result);
    static JSONRPC_STATUS GetProviders(const std::string& method,
                                       ITransportLayer* transport,
                                       IClient* client,
                                       const CVariant& parameterObject,
                                       CVariant& result);
    static JSONRPC_STATUS GetProviderDetails(const std::string& method,
                                             ITransportLayer* transport,
                                             IClient* client,
                                             const CVariant& parameterObject,
                                             CVariant& result);
    static JSONRPC_STATUS GetBroadcasts(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS GetBroadcastsByChannelGroup(const std::string& method,
                                                      ITransportLayer* transport,
                                                      IClient* client,
                                                      const CVariant& parameterObject,
                                                      CVariant& result);
    static JSONRPC_STATUS GetBroadcastDetails(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS GetBroadcastIsPlayable(const std::string& method,
                                                 ITransportLayer* transport,
                                                 IClient* client,
                                                 const CVariant& parameterObject,
                                                 CVariant& result);
    static JSONRPC_STATUS GetPlayableBroadcasts(const std::string& method,
                                                ITransportLayer* transport,
                                                IClient* client,
                                                const CVariant& parameterObject,
                                                CVariant& result);
    static JSONRPC_STATUS GetTimers(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS GetTimerDetails(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS GetRecordings(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS GetRecordingDetails(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);

    static JSONRPC_STATUS AddTimer(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS DeleteTimer(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS ToggleTimer(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);

    static JSONRPC_STATUS Record(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS Scan(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);

    static std::shared_ptr<CFileItem> GetRecordingFileItem(int recordingId);

  protected:
    /*!
     \brief Read the optional or required starttime/endtime pair of a request

     Both absent leaves start and end invalid (no range) when not required; otherwise both
     must parse and end must not precede start.
     */
    static JSONRPC_STATUS ParseTimeRange(const CVariant& parameterObject,
                                         bool required,
                                         CDateTime& start,
                                         CDateTime& end);

    /*!
     \brief The broadcasts of an EPG overlapping [start, end), or all of them for an invalid range
     */
    static std::vector<std::shared_ptr<PVR::CPVREpgInfoTag>> GetBroadcastsInRange(
        const PVR::CPVREpg& epg, const CDateTime& start, const CDateTime& end);

  private:
    static JSONRPC_STATUS GetPropertyValue(const std::string &property, CVariant &result);
    static void FillChannelGroupDetails(
        const std::shared_ptr<const PVR::CPVRChannelGroup>& channelGroup,
        const CVariant& parameterObject,
        CVariant& result,
        bool append = false);
    static void FillProviderDetails(const std::shared_ptr<const PVR::CPVRProvider>& provider,
                                    const CVariant& parameterObject,
                                    CVariant& result,
                                    bool append = false);
  };
}
