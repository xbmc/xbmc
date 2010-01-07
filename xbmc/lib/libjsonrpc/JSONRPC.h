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

#include <features.h>
//#include "system.h"
#include "StdString.h"
#include <map>
#include <stdio.h>
#include <string>
#include <iostream>
#include "ITransportLayer.h"
#include "../libjsoncpp/json.h"

namespace JSONRPC
{
  enum JSON_STATUS
  {
    OK = 0,
    MethodNotFound = -32601,
    InvalidParams = -32602,
    InternalError = -32603,
    ParseError = -32700,
  //-32099..-32000 Reserved for implementation-defined server-errors.
    BadPermission = -32099 
  };

  /* The method call needs to be perfectly threadsafe
     The method will only be called if parameterObject contains data as specified. So if method doesn't support parameters it won't be called with parameters and vice versa.
  */
  typedef JSON_STATUS (*MethodCall) (const CStdString &method, ITransportLayer *transport, const Json::Value& parameterObject, Json::Value &result);

  enum OperationPermission
  {
    ReadData = 0x1,
    ControlPlayback = 0x2,
  };

  #define OPERATION_PERMISSION_ALL (ReadData | ControlPlayback)

  typedef struct
  {
    const char* command;
    MethodCall method;
    OperationPermission permission;
    const char* description;
  } JSON_ACTION;

  typedef std::map<CStdString, JSON_ACTION> ActionMap;

  class CJSONRPC
  {
  public:
    static void Initialize();
    static CStdString MethodCall(const CStdString &inputString, ITransportLayer *transport, IClient *client);

    static JSON_STATUS Introspect(const CStdString &method, ITransportLayer *transport, const Json::Value& parameterObject, Json::Value &result);
    static JSON_STATUS Version(const CStdString &method, ITransportLayer *transport, const Json::Value& parameterObject, Json::Value &result);
    static JSON_STATUS Permission(const CStdString &method, ITransportLayer *transport, const Json::Value& parameterObject, Json::Value &result);
    static JSON_STATUS Ping(const CStdString &method, ITransportLayer *transport, const Json::Value& parameterObject, Json::Value &result);
  private:
    static JSON_STATUS InternalMethodCall(const CStdString& method, Json::Value& o, Json::Value &result, ITransportLayer *transport, IClient *client);
    static ActionMap m_actionMap;
  };
}
