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

#include "JSONRPC.h"
#include "settings/AdvancedSettings.h"
#include "interfaces/AnnouncementManager.h"
#include "interfaces/AnnouncementUtils.h"
#include "utils/log.h"
#include "utils/Variant.h"
#include <string.h>
#include "ServiceDescription.h"

using namespace ANNOUNCEMENT;
using namespace JSONRPC;
using namespace std;

bool CJSONRPC::m_initialized = false;

void CJSONRPC::Initialize()
{
  if (m_initialized)
    return;

  unsigned int size = sizeof(JSONRPC_SERVICE_TYPES) / sizeof(char*);

  for (unsigned int index = 0; index < size; index++)
    CJSONServiceDescription::AddType(JSONRPC_SERVICE_TYPES[index]);

  size = sizeof(JSONRPC_SERVICE_METHODS) / sizeof(char*);

  for (unsigned int index = 0; index < size; index++)
    CJSONServiceDescription::AddBuiltinMethod(JSONRPC_SERVICE_METHODS[index]);

  size = sizeof(JSONRPC_SERVICE_NOTIFICATIONS) / sizeof(char*);

  for (unsigned int index = 0; index < size; index++)
    CJSONServiceDescription::AddNotification(JSONRPC_SERVICE_NOTIFICATIONS[index]);
  
  m_initialized = true;
}

JSON_STATUS CJSONRPC::Introspect(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant& parameterObject, CVariant &result)
{
  return CJSONServiceDescription::Print(result, transport, client,
    parameterObject["getdescriptions"].asBoolean(), parameterObject["getmetadata"].asBoolean(), parameterObject["filterbytransport"].asBoolean(),
    parameterObject["filter"]["id"].asString(), parameterObject["filter"]["type"].asString(), parameterObject["filter"]["getreferences"].asBoolean());
}

JSON_STATUS CJSONRPC::Version(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant& parameterObject, CVariant &result)
{
  result["version"] = CJSONServiceDescription::GetVersion();

  return OK;
}

JSON_STATUS CJSONRPC::Permission(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant& parameterObject, CVariant &result)
{
  int flags = client->GetPermissionFlags();

  for (int i = 1; i <= OPERATION_PERMISSION_ALL; i *= 2)
    result[PermissionToString((OperationPermission)i)] = (flags & i) > 0;

  return OK;
}

JSON_STATUS CJSONRPC::Ping(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant& parameterObject, CVariant &result)
{
  CVariant temp = "pong";
  result.swap(temp);
  return OK;
}

JSON_STATUS CJSONRPC::GetConfiguration(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant& parameterObject, CVariant &result)
{
  int flags = client->GetAnnouncementFlags();

  for (int i = 1; i <= ANNOUNCE_ALL; i *= 2)
    result["notifications"][CAnnouncementUtils::AnnouncementFlagToString((EAnnouncementFlag)i)] = (flags & i) > 0;

  return OK;
}

JSON_STATUS CJSONRPC::SetConfiguration(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant& parameterObject, CVariant &result)
{
  int flags = 0;
  int oldFlags = client->GetAnnouncementFlags();

  if (parameterObject.isMember("notifications"))
  {
    CVariant notifications = parameterObject["notifications"];
    if ((notifications["Player"].isNull() && (oldFlags & Player)) ||
        (notifications["Player"].isBoolean() && notifications["Player"].asBoolean()))
      flags |= Player;
    if ((notifications["GUI"].isNull() && (oldFlags & GUI)) ||
        (notifications["GUI"].isBoolean() && notifications["GUI"].asBoolean()))
      flags |= GUI;
    if ((notifications["System"].isNull() && (oldFlags & System)) ||
        (notifications["System"].isBoolean() && notifications["System"].asBoolean()))
      flags |= System;
    if ((notifications["VideoLibrary"].isNull() && (oldFlags & VideoLibrary)) ||
        (notifications["VideoLibrary"].isBoolean() && notifications["VideoLibrary"].asBoolean()))
      flags |= VideoLibrary;
    if ((notifications["AudioLibrary"].isNull() && (oldFlags & AudioLibrary)) ||
        (notifications["AudioLibrary"].isBoolean() && notifications["AudioLibrary"].asBoolean()))
      flags |= AudioLibrary;
    if ((notifications["Other"].isNull() && (oldFlags & Other)) ||
        (notifications["Other"].isBoolean() && notifications["Other"].asBoolean()))
      flags |= Other;
  }

  if (!client->SetAnnouncementFlags(flags))
    return BadPermission;

  return GetConfiguration(method, transport, client, parameterObject, result);
}

JSON_STATUS CJSONRPC::NotifyAll(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant& parameterObject, CVariant &result)
{
  if (parameterObject["data"].isNull())
    CAnnouncementManager::Announce(Other, parameterObject["sender"].asString().c_str(),  
      parameterObject["message"].asString().c_str());
  else
  {
    CVariant data = parameterObject["data"];
    CAnnouncementManager::Announce(Other, parameterObject["sender"].asString().c_str(),  
      parameterObject["message"].asString().c_str(), data);
  }

  return ACK;
}

CStdString CJSONRPC::MethodCall(const CStdString &inputString, ITransportLayer *transport, IClient *client)
{
  CVariant inputroot, outputroot, result;
  bool hasResponse = false;

  CLog::Log(LOGDEBUG, "JSONRPC: Incoming request: %s", inputString.c_str());
  inputroot = CJSONVariantParser::Parse((unsigned char *)inputString.c_str(), inputString.length());
  if (!inputroot.isNull())
  {
    if (inputroot.isArray())
    {
      if (inputroot.size() <= 0)
      {
        CLog::Log(LOGERROR, "JSONRPC: Empty batch call\n");
        BuildResponse(inputroot, InvalidRequest, CVariant(), outputroot);
        hasResponse = true;
      }
      else
      {
        for (CVariant::const_iterator_array itr = inputroot.begin_array(); itr != inputroot.end_array(); itr++)
        {
          CVariant response;
          if (HandleMethodCall(*itr, response, transport, client))
          {
            outputroot.append(response);
            hasResponse = true;
          }
        }
      }
    }
    else
      hasResponse = HandleMethodCall(inputroot, outputroot, transport, client);
  }
  else
  {
    CLog::Log(LOGERROR, "JSONRPC: Failed to parse '%s'\n", inputString.c_str());
    BuildResponse(inputroot, ParseError, CVariant(), outputroot);
    hasResponse = true;
  }

  CStdString str = hasResponse ? CJSONVariantWriter::Write(outputroot, g_advancedSettings.m_jsonOutputCompact) : "";
  return str;
}

bool CJSONRPC::HandleMethodCall(const CVariant& request, CVariant& response, ITransportLayer *transport, IClient *client)
{
  JSON_STATUS errorCode = OK;
  CVariant result;
  bool isNotification = false;

  if (IsProperJSONRPC(request))
  {
    isNotification = !request.isMember("id");

    CStdString methodName = request["method"].asString();
    methodName = methodName.ToLower();

    JSONRPC::MethodCall method;
    CVariant params;

    CLog::Log(LOGDEBUG, "JSONRPC: Calling %s", methodName.c_str());
    if ((errorCode = CJSONServiceDescription::CheckCall(methodName, request["params"], transport, client, isNotification, method, params)) == OK)
      errorCode = method(methodName, transport, client, params, result);
    else
      result = params;
  }
  else
  {
    CLog::Log(LOGERROR, "JSONRPC: Failed to parse '%s'\n", CJSONVariantWriter::Write(request, true).c_str());
    errorCode = InvalidRequest;
  }

  BuildResponse(request, errorCode, result, response);

  return !isNotification;
}

inline bool CJSONRPC::IsProperJSONRPC(const CVariant& inputroot)
{
  return inputroot.isObject() && inputroot.isMember("jsonrpc") && inputroot["jsonrpc"].isString() && inputroot["jsonrpc"] == CVariant("2.0") && inputroot.isMember("method") && inputroot["method"].isString() && (!inputroot.isMember("params") || inputroot["params"].isArray() || inputroot["params"].isObject());
}

inline void CJSONRPC::BuildResponse(const CVariant& request, JSON_STATUS code, const CVariant& result, CVariant& response)
{
  response["jsonrpc"] = "2.0";
  response["id"] = request.isObject() && request.isMember("id") ? request["id"] : CVariant();

  switch (code)
  {
    case OK:
      response["result"] = result;
      break;
    case ACK:
      response["result"] = "OK";
      break;
    case InvalidRequest:
      response["error"]["code"] = InvalidRequest;
      response["error"]["message"] = "Invalid request.";
      break;
    case InvalidParams:
      response["error"]["code"] = InvalidParams;
      response["error"]["message"] = "Invalid params.";
      if (!result.isNull())
        response["error"]["data"] = result;
      break;
    case MethodNotFound:
      response["error"]["code"] = MethodNotFound;
      response["error"]["message"] = "Method not found.";
      break;
    case ParseError:
      response["error"]["code"] = ParseError;
      response["error"]["message"] = "Parse error.";
      break;
    case BadPermission:
      response["error"]["code"] = BadPermission;
      response["error"]["message"] = "Bad client permission.";
      break;
    case FailedToExecute:
      response["error"]["code"] = FailedToExecute;
      response["error"]["message"] = "Failed to execute method.";
      break;
    default:
      response["error"]["code"] = InternalError;
      response["error"]["message"] = "Internal error.";
      break;
  }
}
