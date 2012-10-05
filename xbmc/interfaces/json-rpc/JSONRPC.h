#pragma once
/*
 *      Copyright (C) 2005-2012 Team XBMC
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

#include <iostream>
#include <map>
#include <stdio.h>
#include <string>

#include "JSONRPCUtils.h"
#include "JSONServiceDescription.h"
#include "interfaces/IAnnouncer.h"
#include "utils/StdString.h"

namespace JSONRPC
{
  /*!
   \ingroup jsonrpc
   \brief JSON RPC handler

   Sets up and manages all needed information to process
   JSON-RPC requests and answering with the appropriate
   JSON-RPC response (actual response or error message).
   */
  class CJSONRPC
  {
  public:
    /*!
     \brief Initializes the JSON-RPC handler
     */
    static void Initialize();

    /*
     \brief Handles an incoming JSON-RPC request
     \param inputString received JSON-RPC request
     \param transport Transport protocol on which the request arrived
     \param client Client which sent the request
     \return JSON-RPC response to be sent back to the client

     Parses the received input string for the called method and provided
     parameters. If the request does not conform to the JSON-RPC 2.0
     specification an error is returned. Otherwise the parameters provided
     in the request are checked for validity and completeness. If the request
     is valid and the requested method exists it is called and executed.
     */
    static CStdString MethodCall(const CStdString &inputString, ITransportLayer *transport, IClient *client);

    static JSONRPC_STATUS Introspect(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant& parameterObject, CVariant &result);
    static JSONRPC_STATUS Version(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant& parameterObject, CVariant &result);
    static JSONRPC_STATUS Permission(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant& parameterObject, CVariant &result);
    static JSONRPC_STATUS Ping(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant& parameterObject, CVariant &result);
    static JSONRPC_STATUS GetConfiguration(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant& parameterObject, CVariant &result);
    static JSONRPC_STATUS SetConfiguration(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant& parameterObject, CVariant &result);
    static JSONRPC_STATUS NotifyAll(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant& parameterObject, CVariant &result);
  
  private:
    static void setup();
    static bool HandleMethodCall(const CVariant& request, CVariant& response, ITransportLayer *transport, IClient *client);
    static inline bool IsProperJSONRPC(const CVariant& inputroot);

    inline static void BuildResponse(const CVariant& request, JSONRPC_STATUS code, const CVariant& result, CVariant& response);

    static bool m_initialized;
  };
}
