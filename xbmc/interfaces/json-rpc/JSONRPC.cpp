/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "JSONRPC.h"

#include "ServiceBroker.h"
#include "ServiceDescription.h"
#include "TextureDatabase.h"
#include "addons/Addon.h"
#include "addons/IAddon.h"
#include "dbwrappers/DatabaseQuery.h"
#include "input/WindowTranslator.h"
#include "input/actions/ActionTranslator.h"
#include "interfaces/AnnouncementManager.h"
#include "playlists/SmartPlayList.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"

#include <string.h>

using namespace JSONRPC;

bool CJSONRPC::m_initialized = false;

void CJSONRPC::Initialize()
{
  if (m_initialized)
    return;

  // Add some types/enums at runtime
  std::vector<std::string> enumList;
  for (int addonType = ADDON::ADDON_UNKNOWN; addonType < ADDON::ADDON_MAX; addonType++)
    enumList.push_back(ADDON::CAddonInfo::TranslateType(static_cast<ADDON::TYPE>(addonType), false));
  CJSONServiceDescription::AddEnum("Addon.Types", enumList);

  enumList.clear();
  CActionTranslator::GetActions(enumList);
  CJSONServiceDescription::AddEnum("Input.Action", enumList);

  enumList.clear();
  CWindowTranslator::GetWindows(enumList);
  CJSONServiceDescription::AddEnum("GUI.Window", enumList);

  // filter-related enums
  std::vector<std::string> smartplaylistList;
  CDatabaseQueryRule::GetAvailableOperators(smartplaylistList);
  CJSONServiceDescription::AddEnum("List.Filter.Operators", smartplaylistList);

  smartplaylistList.clear();
  CSmartPlaylist::GetAvailableFields("movies", smartplaylistList);
  CJSONServiceDescription::AddEnum("List.Filter.Fields.Movies", smartplaylistList);

  smartplaylistList.clear();
  CSmartPlaylist::GetAvailableFields("tvshows", smartplaylistList);
  CJSONServiceDescription::AddEnum("List.Filter.Fields.TVShows", smartplaylistList);

  smartplaylistList.clear();
  CSmartPlaylist::GetAvailableFields("episodes", smartplaylistList);
  CJSONServiceDescription::AddEnum("List.Filter.Fields.Episodes", smartplaylistList);

  smartplaylistList.clear();
  CSmartPlaylist::GetAvailableFields("musicvideos", smartplaylistList);
  CJSONServiceDescription::AddEnum("List.Filter.Fields.MusicVideos", smartplaylistList);

  smartplaylistList.clear();
  CSmartPlaylist::GetAvailableFields("artists", smartplaylistList);
  CJSONServiceDescription::AddEnum("List.Filter.Fields.Artists", smartplaylistList);

  smartplaylistList.clear();
  CSmartPlaylist::GetAvailableFields("albums", smartplaylistList);
  CJSONServiceDescription::AddEnum("List.Filter.Fields.Albums", smartplaylistList);

  smartplaylistList.clear();
  CSmartPlaylist::GetAvailableFields("songs", smartplaylistList);
  CJSONServiceDescription::AddEnum("List.Filter.Fields.Songs", smartplaylistList);

  smartplaylistList.clear();
  CTextureRule::GetAvailableFields(smartplaylistList);
  CJSONServiceDescription::AddEnum("List.Filter.Fields.Textures", smartplaylistList);

  unsigned int size = sizeof(JSONRPC_SERVICE_TYPES) / sizeof(char*);

  for (unsigned int index = 0; index < size; index++)
    CJSONServiceDescription::AddType(JSONRPC_SERVICE_TYPES[index]);

  size = sizeof(JSONRPC_SERVICE_METHODS) / sizeof(char*);

  for (unsigned int index = 0; index < size; index++)
    CJSONServiceDescription::AddBuiltinMethod(JSONRPC_SERVICE_METHODS[index]);

  size = sizeof(JSONRPC_SERVICE_NOTIFICATIONS) / sizeof(char*);

  for (unsigned int index = 0; index < size; index++)
    CJSONServiceDescription::AddNotification(JSONRPC_SERVICE_NOTIFICATIONS[index]);

  CJSONServiceDescription::ResolveReferences();

  m_initialized = true;
  CLog::Log(LOGINFO, "JSONRPC v%s: Successfully initialized", CJSONServiceDescription::GetVersion());
}

void CJSONRPC::Cleanup()
{
  CJSONServiceDescription::Cleanup();
  m_initialized = false;
}

JSONRPC_STATUS CJSONRPC::Introspect(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant& parameterObject, CVariant &result)
{
  return CJSONServiceDescription::Print(result, transport, client,
    parameterObject["getdescriptions"].asBoolean(), parameterObject["getmetadata"].asBoolean(), parameterObject["filterbytransport"].asBoolean(),
    parameterObject["filter"]["id"].asString(), parameterObject["filter"]["type"].asString(), parameterObject["filter"]["getreferences"].asBoolean());
}

JSONRPC_STATUS CJSONRPC::Version(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant& parameterObject, CVariant &result)
{
  result["version"]["major"] = 0;
  result["version"]["minor"] = 0;
  result["version"]["patch"] = 0;

  const char* version = CJSONServiceDescription::GetVersion();
  if (version != NULL)
  {
    std::vector<std::string> parts = StringUtils::Split(version, ".");
    if (!parts.empty())
      result["version"]["major"] = (int)strtol(parts[0].c_str(), NULL, 10);
    if (parts.size() > 1)
      result["version"]["minor"] = (int)strtol(parts[1].c_str(), NULL, 10);
    if (parts.size() > 2)
      result["version"]["patch"] = (int)strtol(parts[2].c_str(), NULL, 10);
  }

  return OK;
}

JSONRPC_STATUS CJSONRPC::Permission(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant& parameterObject, CVariant &result)
{
  int flags = client->GetPermissionFlags();

  for (int i = 1; i <= OPERATION_PERMISSION_ALL; i *= 2)
    result[PermissionToString((OperationPermission)i)] = (flags & i) == i;

  return OK;
}

JSONRPC_STATUS CJSONRPC::Ping(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant& parameterObject, CVariant &result)
{
  CVariant temp = "pong";
  result.swap(temp);
  return OK;
}

JSONRPC_STATUS CJSONRPC::GetConfiguration(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant& parameterObject, CVariant &result)
{
  int flags = client->GetAnnouncementFlags();

  for (int i = 1; i <= ANNOUNCEMENT::ANNOUNCE_ALL; i *= 2)
    result["notifications"][AnnouncementFlagToString((ANNOUNCEMENT::AnnouncementFlag)i)] = (flags & i) == i;

  return OK;
}

JSONRPC_STATUS CJSONRPC::SetConfiguration(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant& parameterObject, CVariant &result)
{
  int flags = 0;
  int oldFlags = client->GetAnnouncementFlags();

  if (parameterObject.isMember("notifications"))
  {
    CVariant notifications = parameterObject["notifications"];
    if ((notifications["Player"].isNull() && (oldFlags & ANNOUNCEMENT::Player)) ||
        (notifications["Player"].isBoolean() && notifications["Player"].asBoolean()))
      flags |= ANNOUNCEMENT::Player;
    if ((notifications["Playlist"].isNull() && (oldFlags & ANNOUNCEMENT::Playlist)) ||
        (notifications["Playlist"].isBoolean() && notifications["Playlist"].asBoolean()))
      flags |= ANNOUNCEMENT::Playlist;
    if ((notifications["GUI"].isNull() && (oldFlags & ANNOUNCEMENT::GUI)) ||
        (notifications["GUI"].isBoolean() && notifications["GUI"].asBoolean()))
      flags |= ANNOUNCEMENT::GUI;
    if ((notifications["System"].isNull() && (oldFlags & ANNOUNCEMENT::System)) ||
        (notifications["System"].isBoolean() && notifications["System"].asBoolean()))
      flags |= ANNOUNCEMENT::System;
    if ((notifications["VideoLibrary"].isNull() && (oldFlags & ANNOUNCEMENT::VideoLibrary)) ||
        (notifications["VideoLibrary"].isBoolean() && notifications["VideoLibrary"].asBoolean()))
      flags |= ANNOUNCEMENT::VideoLibrary;
    if ((notifications["AudioLibrary"].isNull() && (oldFlags & ANNOUNCEMENT::AudioLibrary)) ||
        (notifications["AudioLibrary"].isBoolean() && notifications["AudioLibrary"].asBoolean()))
      flags |= ANNOUNCEMENT::AudioLibrary;
    if ((notifications["Application"].isNull() && (oldFlags & ANNOUNCEMENT::Other)) ||
        (notifications["Application"].isBoolean() && notifications["Application"].asBoolean()))
      flags |= ANNOUNCEMENT::Application;
    if ((notifications["Input"].isNull() && (oldFlags & ANNOUNCEMENT::Input)) ||
        (notifications["Input"].isBoolean() && notifications["Input"].asBoolean()))
      flags |= ANNOUNCEMENT::Input;
    if ((notifications["Other"].isNull() && (oldFlags & ANNOUNCEMENT::Other)) ||
        (notifications["Other"].isBoolean() && notifications["Other"].asBoolean()))
      flags |= ANNOUNCEMENT::Other;
  }

  if (!client->SetAnnouncementFlags(flags))
    return BadPermission;

  return GetConfiguration(method, transport, client, parameterObject, result);
}

JSONRPC_STATUS CJSONRPC::NotifyAll(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant& parameterObject, CVariant &result)
{
  if (parameterObject["data"].isNull())
    CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::Other, parameterObject["sender"].asString().c_str(),
      parameterObject["message"].asString().c_str());
  else
  {
    CVariant data = parameterObject["data"];
    CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::Other, parameterObject["sender"].asString().c_str(),
      parameterObject["message"].asString().c_str(), data);
  }

  return ACK;
}

std::string CJSONRPC::MethodCall(const std::string &inputString, ITransportLayer *transport, IClient *client)
{
  CVariant inputroot, outputroot, result;
  bool hasResponse = false;

  CLog::Log(LOGDEBUG, LOGJSONRPC, "JSONRPC: Incoming request: %s", inputString.c_str());

  if (CJSONVariantParser::Parse(inputString, inputroot) && !inputroot.isNull())
  {
    if (inputroot.isArray())
    {
      if (inputroot.size() <= 0)
      {
        CLog::Log(LOGERROR, "JSONRPC: Empty batch call");
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
    CLog::Log(LOGERROR, "JSONRPC: Failed to parse '%s'", inputString.c_str());
    BuildResponse(inputroot, ParseError, CVariant(), outputroot);
    hasResponse = true;
  }

  std::string str;
  if (hasResponse)
    CJSONVariantWriter::Write(outputroot, str, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_jsonOutputCompact);

  return str;
}

bool CJSONRPC::HandleMethodCall(const CVariant& request, CVariant& response, ITransportLayer *transport, IClient *client)
{
  JSONRPC_STATUS errorCode = OK;
  CVariant result;
  bool isNotification = false;

  if (IsProperJSONRPC(request))
  {
    isNotification = !request.isMember("id");

    std::string methodName = request["method"].asString();
    StringUtils::ToLower(methodName);

    JSONRPC::MethodCall method;
    CVariant params;

    if ((errorCode = CJSONServiceDescription::CheckCall(methodName.c_str(), request["params"], transport, client, isNotification, method, params)) == OK)
      errorCode = method(methodName, transport, client, params, result);
    else
      result = params;
  }
  else
  {
    std::string str;
    CJSONVariantWriter::Write(request, str, true);

    CLog::Log(LOGERROR, "JSONRPC: Failed to parse '%s'", str.c_str());
    errorCode = InvalidRequest;
  }

  BuildResponse(request, errorCode, result, response);

  return !isNotification;
}

inline bool CJSONRPC::IsProperJSONRPC(const CVariant& inputroot)
{
  return inputroot.isMember("jsonrpc") && inputroot["jsonrpc"].isString() && inputroot["jsonrpc"] == CVariant("2.0") && inputroot.isMember("method") && inputroot["method"].isString() && (!inputroot.isMember("params") || inputroot["params"].isArray() || inputroot["params"].isObject());
}

inline void CJSONRPC::BuildResponse(const CVariant& request, JSONRPC_STATUS code, const CVariant& result, CVariant& response)
{
  response["jsonrpc"] = "2.0";
  response["id"] = request.isMember("id") ? request["id"] : CVariant();

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
