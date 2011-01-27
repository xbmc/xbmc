#pragma once
/*
 *      Copyright (C) 2005-2010 Team XBMC
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

#include "utils/StdString.h"
#include <map>
#include <stdio.h>
#include <string>
#include <iostream>
#include "ITransportLayer.h"
#include "interfaces/IAnnouncer.h"
#include "jsoncpp/include/json/json.h"

namespace JSONRPC
{
  enum JSON_STATUS
  {
    OK = 0,
    ACK = -1,
    InvalidRequest = -32600,
    MethodNotFound = -32601,
    InvalidParams = -32602,
    InternalError = -32603,
    ParseError = -32700,
  //-32099..-32000 Reserved for implementation-defined server-errors.
    BadPermission = -32099,
    FailedToExecute = -32100
  };

  /* The method call needs to be perfectly threadsafe
     The method will only be called if the caller has the correct permissions. The method will need to check parameters for bad parametervalues.
  */
  typedef JSON_STATUS (*MethodCall) (const CStdString &method, ITransportLayer *transport, IClient *client, const Json::Value& parameterObject, Json::Value &result);

  enum OperationPermission
  {
    ReadData = 0x1,
    ControlPlayback = 0x2,
    ControlAnnounce = 0x4,
    ControlPower = 0x8,
    Logging = 0x10,
    ScanLibrary = 0x20,
  };

  static const int OPERATION_PERMISSION_ALL = (ReadData | ControlPlayback | ControlAnnounce | ControlPower | Logging | ScanLibrary);

  typedef struct
  {
    const char* command;
    MethodCall method;
    TransportLayerCapability transportneed;
    OperationPermission permission;
    const char* description;
  } Command;

  class CJSONRPC
  {
  public:
    static CStdString MethodCall(const CStdString &inputString, ITransportLayer *transport, IClient *client);

    static JSON_STATUS Introspect(const CStdString &method, ITransportLayer *transport, IClient *client, const Json::Value& parameterObject, Json::Value &result);
    static JSON_STATUS Version(const CStdString &method, ITransportLayer *transport, IClient *client, const Json::Value& parameterObject, Json::Value &result);
    static JSON_STATUS Permission(const CStdString &method, ITransportLayer *transport, IClient *client, const Json::Value& parameterObject, Json::Value &result);
    static JSON_STATUS Ping(const CStdString &method, ITransportLayer *transport, IClient *client, const Json::Value& parameterObject, Json::Value &result);
    static JSON_STATUS GetAnnouncementFlags(const CStdString &method, ITransportLayer *transport, IClient *client, const Json::Value& parameterObject, Json::Value &result);
    static JSON_STATUS SetAnnouncementFlags(const CStdString &method, ITransportLayer *transport, IClient *client, const Json::Value& parameterObject, Json::Value &result);
    static JSON_STATUS Announce(const CStdString &method, ITransportLayer *transport, IClient *client, const Json::Value& parameterObject, Json::Value &result);
  private:
    static bool HandleMethodCall(Json::Value& request, Json::Value& response, ITransportLayer *transport, IClient *client);
    static JSON_STATUS InternalMethodCall(const CStdString& method, Json::Value& o, Json::Value &result, ITransportLayer *transport, IClient *client);
    static inline bool IsProperJSONRPC(const Json::Value& inputroot);

    inline static void BuildResponse(const Json::Value& request, JSON_STATUS code, const Json::Value& result, Json::Value& response);
    inline static const char *PermissionToString(const OperationPermission &permission);
    inline static const char *AnnouncementFlagToString(const ANNOUNCEMENT::EAnnouncementFlag &announcement);

    class CActionMap
    {
    public:
      CActionMap(const Command commands[], int length);

      typedef std::map<CStdString, Command>::const_iterator const_iterator;
      const_iterator find(const CStdString& key) const;
      const_iterator end() const;
    private:
      std::map<CStdString, Command> m_actionmap;
    };

    static Command    m_commands[];
    static CActionMap m_actionMap;
  };
}
