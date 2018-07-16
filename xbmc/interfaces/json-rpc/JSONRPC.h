/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <iostream>
#include <map>
#include <stdio.h>
#include <string>

#include "JSONRPCUtils.h"
#include "JSONServiceDescription.h"

class CVariant;

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

    static void Cleanup();

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
    static std::string MethodCall(const std::string &inputString, ITransportLayer *transport, IClient *client);

    static JSONRPC_STATUS Introspect(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant& parameterObject, CVariant &result);
    static JSONRPC_STATUS Version(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant& parameterObject, CVariant &result);
    static JSONRPC_STATUS Permission(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant& parameterObject, CVariant &result);
    static JSONRPC_STATUS Ping(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant& parameterObject, CVariant &result);
    static JSONRPC_STATUS GetConfiguration(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant& parameterObject, CVariant &result);
    static JSONRPC_STATUS SetConfiguration(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant& parameterObject, CVariant &result);
    static JSONRPC_STATUS NotifyAll(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant& parameterObject, CVariant &result);

  private:
    static bool HandleMethodCall(const CVariant& request, CVariant& response, ITransportLayer *transport, IClient *client);
    static inline bool IsProperJSONRPC(const CVariant& inputroot);

    inline static void BuildResponse(const CVariant& request, JSONRPC_STATUS code, const CVariant& result, CVariant& response);

    static bool m_initialized;
  };
}
