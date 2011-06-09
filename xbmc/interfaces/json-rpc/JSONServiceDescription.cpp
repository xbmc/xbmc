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

#include <limits>
#include "ServiceDescription.h"
#include "JSONServiceDescription.h"
#include "utils/log.h"
#include "utils/StdString.h"
#include "utils/JSONVariantParser.h"
#include "JSONRPC.h"
#include "PlayerOperations.h"
#include "AVPlayerOperations.h"
#include "PicturePlayerOperations.h"
#include "AVPlaylistOperations.h"
#include "PlaylistOperations.h"
#include "FileOperations.h"
#include "AudioLibrary.h"
#include "VideoLibrary.h"
#include "SystemOperations.h"
#include "InputOperations.h"
#include "XBMCOperations.h"

using namespace std;
using namespace JSONRPC;

JSONSchemaTypeDefinition::CJsonSchemaPropertiesMap::CJsonSchemaPropertiesMap()
{
  m_propertiesmap = std::map<std::string, JSONSchemaTypeDefinition>();
}

void JSONSchemaTypeDefinition::CJsonSchemaPropertiesMap::add(JSONSchemaTypeDefinition &property)
{
  CStdString name = property.name;
  name = name.ToLower();
  m_propertiesmap[name] = property;
}

JSONSchemaTypeDefinition::CJsonSchemaPropertiesMap::JSONSchemaPropertiesIterator JSONSchemaTypeDefinition::CJsonSchemaPropertiesMap::begin() const
{
  return m_propertiesmap.begin();
}

JSONSchemaTypeDefinition::CJsonSchemaPropertiesMap::JSONSchemaPropertiesIterator JSONSchemaTypeDefinition::CJsonSchemaPropertiesMap::find(const std::string& key) const
{
  return m_propertiesmap.find(key);
}

JSONSchemaTypeDefinition::CJsonSchemaPropertiesMap::JSONSchemaPropertiesIterator JSONSchemaTypeDefinition::CJsonSchemaPropertiesMap::end() const
{
  return m_propertiesmap.end();
}

unsigned int JSONSchemaTypeDefinition::CJsonSchemaPropertiesMap::size() const
{
  return m_propertiesmap.size();
}

std::map<std::string, CVariant> CJSONServiceDescription::m_notifications = std::map<std::string, CVariant>();
CJSONServiceDescription::CJsonRpcMethodMap CJSONServiceDescription::m_actionMap;
std::map<std::string, JSONSchemaTypeDefinition> CJSONServiceDescription::m_types = std::map<std::string, JSONSchemaTypeDefinition>();

JsonRpcMethodMap CJSONServiceDescription::m_methodMaps[] = {
// JSON-RPC
  { "JSONRPC.Introspect",                           CJSONRPC::Introspect },
  { "JSONRPC.Version",                              CJSONRPC::Version },
  { "JSONRPC.Permission",                           CJSONRPC::Permission },
  { "JSONRPC.Ping",                                 CJSONRPC::Ping },
  { "JSONRPC.GetNotificationFlags",                 CJSONRPC::GetNotificationFlags },
  { "JSONRPC.SetNotificationFlags",                 CJSONRPC::SetNotificationFlags },
  { "JSONRPC.NotifyAll",                            CJSONRPC::NotifyAll },

// Player
  { "Player.GetActivePlayers",                      CPlayerOperations::GetActivePlayers },

// Music player
  { "AudioPlayer.State",                            CAVPlayerOperations::State },
  { "AudioPlayer.PlayPause",                        CAVPlayerOperations::PlayPause },
  { "AudioPlayer.Stop",                             CAVPlayerOperations::Stop },
  { "AudioPlayer.SkipPrevious",                     CAVPlayerOperations::SkipPrevious },
  { "AudioPlayer.SkipNext",                         CAVPlayerOperations::SkipNext },

  { "AudioPlayer.BigSkipBackward",                  CAVPlayerOperations::BigSkipBackward },
  { "AudioPlayer.BigSkipForward",                   CAVPlayerOperations::BigSkipForward },
  { "AudioPlayer.SmallSkipBackward",                CAVPlayerOperations::SmallSkipBackward },
  { "AudioPlayer.SmallSkipForward",                 CAVPlayerOperations::SmallSkipForward },

  { "AudioPlayer.Rewind",                           CAVPlayerOperations::Rewind },
  { "AudioPlayer.Forward",                          CAVPlayerOperations::Forward },

  { "AudioPlayer.GetTime",                          CAVPlayerOperations::GetTime },
  { "AudioPlayer.GetPercentage",                    CAVPlayerOperations::GetPercentage },
  { "AudioPlayer.SeekTime",                         CAVPlayerOperations::SeekTime },
  { "AudioPlayer.SeekPercentage",                   CAVPlayerOperations::SeekPercentage },

// Video player
  { "VideoPlayer.State",                            CAVPlayerOperations::State },
  { "VideoPlayer.PlayPause",                        CAVPlayerOperations::PlayPause },
  { "VideoPlayer.Stop",                             CAVPlayerOperations::Stop },
  { "VideoPlayer.SkipPrevious",                     CAVPlayerOperations::SkipPrevious },
  { "VideoPlayer.SkipNext",                         CAVPlayerOperations::SkipNext },

  { "VideoPlayer.BigSkipBackward",                  CAVPlayerOperations::BigSkipBackward },
  { "VideoPlayer.BigSkipForward",                   CAVPlayerOperations::BigSkipForward },
  { "VideoPlayer.SmallSkipBackward",                CAVPlayerOperations::SmallSkipBackward },
  { "VideoPlayer.SmallSkipForward",                 CAVPlayerOperations::SmallSkipForward },

  { "VideoPlayer.Rewind",                           CAVPlayerOperations::Rewind },
  { "VideoPlayer.Forward",                          CAVPlayerOperations::Forward },

  { "VideoPlayer.GetTime",                          CAVPlayerOperations::GetTime },
  { "VideoPlayer.GetPercentage",                    CAVPlayerOperations::GetPercentage },
  { "VideoPlayer.SeekTime",                         CAVPlayerOperations::SeekTime },
  { "VideoPlayer.SeekPercentage",                   CAVPlayerOperations::SeekPercentage },

// Picture player
  { "PicturePlayer.PlayPause",                      CPicturePlayerOperations::PlayPause },
  { "PicturePlayer.Stop",                           CPicturePlayerOperations::Stop },
  { "PicturePlayer.SkipPrevious",                   CPicturePlayerOperations::SkipPrevious },
  { "PicturePlayer.SkipNext",                       CPicturePlayerOperations::SkipNext },

  { "PicturePlayer.MoveLeft",                       CPicturePlayerOperations::MoveLeft },
  { "PicturePlayer.MoveRight",                      CPicturePlayerOperations::MoveRight },
  { "PicturePlayer.MoveDown",                       CPicturePlayerOperations::MoveDown },
  { "PicturePlayer.MoveUp",                         CPicturePlayerOperations::MoveUp },

  { "PicturePlayer.ZoomOut",                        CPicturePlayerOperations::ZoomOut },
  { "PicturePlayer.ZoomIn",                         CPicturePlayerOperations::ZoomIn },
  { "PicturePlayer.Zoom",                           CPicturePlayerOperations::Zoom },
  { "PicturePlayer.Rotate",                         CPicturePlayerOperations::Rotate },

// Video Playlist
  { "VideoPlaylist.Play",                           CAVPlaylistOperations::Play },
  { "VideoPlaylist.SkipPrevious",                   CAVPlaylistOperations::SkipPrevious },
  { "VideoPlaylist.SkipNext",                       CAVPlaylistOperations::SkipNext },
  { "VideoPlaylist.GetItems",                       CAVPlaylistOperations::GetItems },
  { "VideoPlaylist.Add",                            CAVPlaylistOperations::Add },
  { "VideoPlaylist.Insert",                         CAVPlaylistOperations::Insert },
  { "VideoPlaylist.Clear",                          CAVPlaylistOperations::Clear },
  { "VideoPlaylist.Shuffle",                        CAVPlaylistOperations::Shuffle },
  { "VideoPlaylist.UnShuffle",                      CAVPlaylistOperations::UnShuffle },
  { "VideoPlaylist.Remove",                         CAVPlaylistOperations::Remove },

// AudioPlaylist
  { "AudioPlaylist.Play",                           CAVPlaylistOperations::Play },
  { "AudioPlaylist.SkipPrevious",                   CAVPlaylistOperations::SkipPrevious },
  { "AudioPlaylist.SkipNext",                       CAVPlaylistOperations::SkipNext },
  { "AudioPlaylist.GetItems",                       CAVPlaylistOperations::GetItems },
  { "AudioPlaylist.Add",                            CAVPlaylistOperations::Add },
  { "AudioPlaylist.Insert",                         CAVPlaylistOperations::Insert },
  { "AudioPlaylist.Clear",                          CAVPlaylistOperations::Clear },
  { "AudioPlaylist.Shuffle",                        CAVPlaylistOperations::Shuffle },
  { "AudioPlaylist.UnShuffle",                      CAVPlaylistOperations::UnShuffle },
  { "AudioPlaylist.Remove",                         CAVPlaylistOperations::Remove },

// Playlist
  { "Playlist.Create",                              CPlaylistOperations::Create },
  { "Playlist.Destroy",                             CPlaylistOperations::Destroy },

  { "Playlist.GetItems",                            CPlaylistOperations::GetItems },
  { "Playlist.Add",                                 CPlaylistOperations::Add },
  { "Playlist.Remove",                              CPlaylistOperations::Remove },
  { "Playlist.Swap",                                CPlaylistOperations::Swap },
  { "Playlist.Clear",                               CPlaylistOperations::Clear },
  { "Playlist.Shuffle",                             CPlaylistOperations::Shuffle },
  { "Playlist.UnShuffle",                           CPlaylistOperations::UnShuffle },

// Files
  { "Files.GetSources",                             CFileOperations::GetRootDirectory },
  { "Files.Download",                               CFileOperations::Download },
  { "Files.GetDirectory",                           CFileOperations::GetDirectory },

// Music Library
  { "AudioLibrary.GetArtists",                      CAudioLibrary::GetArtists },
  { "AudioLibrary.GetArtistDetails",                CAudioLibrary::GetArtistDetails },
  { "AudioLibrary.GetAlbums",                       CAudioLibrary::GetAlbums },
  { "AudioLibrary.GetAlbumDetails",                 CAudioLibrary::GetAlbumDetails },
  { "AudioLibrary.GetSongs",                        CAudioLibrary::GetSongs },
  { "AudioLibrary.GetSongDetails",                  CAudioLibrary::GetSongDetails },
  { "AudioLibrary.GetGenres",                       CAudioLibrary::GetGenres },
  { "AudioLibrary.ScanForContent",                  CAudioLibrary::ScanForContent },

// Video Library
  { "VideoLibrary.GetGenres",                       CVideoLibrary::GetGenres },
  { "VideoLibrary.GetMovies",                       CVideoLibrary::GetMovies },
  { "VideoLibrary.GetMovieDetails",                 CVideoLibrary::GetMovieDetails },
  { "VideoLibrary.GetMovieSets",                    CVideoLibrary::GetMovieSets },
  { "VideoLibrary.GetMovieSetDetails",              CVideoLibrary::GetMovieSetDetails },
  { "VideoLibrary.GetTVShows",                      CVideoLibrary::GetTVShows },
  { "VideoLibrary.GetTVShowDetails",                CVideoLibrary::GetTVShowDetails },
  { "VideoLibrary.GetSeasons",                      CVideoLibrary::GetSeasons },
  { "VideoLibrary.GetEpisodes",                     CVideoLibrary::GetEpisodes },
  { "VideoLibrary.GetEpisodeDetails",               CVideoLibrary::GetEpisodeDetails },
  { "VideoLibrary.GetMusicVideos",                  CVideoLibrary::GetMusicVideos },
  { "VideoLibrary.GetMusicVideoDetails",            CVideoLibrary::GetMusicVideoDetails },
  { "VideoLibrary.GetRecentlyAddedMovies",          CVideoLibrary::GetRecentlyAddedMovies },
  { "VideoLibrary.GetRecentlyAddedEpisodes",        CVideoLibrary::GetRecentlyAddedEpisodes },
  { "VideoLibrary.GetRecentlyAddedMusicVideos",     CVideoLibrary::GetRecentlyAddedMusicVideos },
  { "VideoLibrary.ScanForContent",                  CVideoLibrary::ScanForContent },

// System operations
  { "System.Shutdown",                              CSystemOperations::Shutdown },
  { "System.Suspend",                               CSystemOperations::Suspend },
  { "System.Hibernate",                             CSystemOperations::Hibernate },
  { "System.Reboot",                                CSystemOperations::Reboot },
  { "System.GetInfoLabels",                         CSystemOperations::GetInfoLabels },
  { "System.GetInfoBooleans",                       CSystemOperations::GetInfoBooleans },

// Input operations
  { "Input.Left",                                   CInputOperations::Left },
  { "Input.Right",                                  CInputOperations::Right },
  { "Input.Down",                                   CInputOperations::Down },
  { "Input.Up",                                     CInputOperations::Up },
  { "Input.Select",                                 CInputOperations::Select },
  { "Input.Back",                                   CInputOperations::Back },
  { "Input.Home",                                   CInputOperations::Home },

// XBMC operations
  { "XBMC.GetVolume",                               CXBMCOperations::GetVolume },
  { "XBMC.SetVolume",                               CXBMCOperations::SetVolume },
  { "XBMC.ToggleMute",                              CXBMCOperations::ToggleMute },
  { "XBMC.Play",                                    CXBMCOperations::Play },
  { "XBMC.StartSlideshow",                          CXBMCOperations::StartSlideshow },
  { "XBMC.Log",                                     CXBMCOperations::Log },
  { "XBMC.Quit",                                    CXBMCOperations::Quit }
};

bool CJSONServiceDescription::prepareDescription(std::string &description, CVariant &descriptionObject, std::string &name)
{
  if (description.empty())
  {
    CLog::Log(LOGERROR, "JSONRPC: Missing JSON Schema definition for \"%s\"", name.c_str());
    return false;
  }

  if (description.at(0) != '{')
  {
    CStdString json;
    json.Format("{%s}", description);
    description = json;
  }

  descriptionObject = CJSONVariantParser::Parse((const unsigned char *)description.c_str(), description.size());

  // Make sure the method description actually exists and represents an object
  if (!descriptionObject.isObject())
  {
    CLog::Log(LOGERROR, "JSONRPC: Unable to parse JSON Schema definition for \"%s\"", name.c_str());
    return false;
  }

  CVariant::const_iterator_map member = descriptionObject.begin_map();
  if (member != descriptionObject.end_map())
    name = member->first;

  if (name.empty() || !descriptionObject[name].isMember("type") || !descriptionObject[name]["type"].isString())
  {
    CLog::Log(LOGERROR, "JSONRPC: Invalid JSON Schema definition for \"%s\"", name.c_str());
    return false;
  }

  return true;
}

bool CJSONServiceDescription::addMethod(std::string &jsonMethod, MethodCall method)
{
  CVariant descriptionObject;
  std::string methodName;

  // Make sure the method description actually exists and represents an object
  if (!prepareDescription(jsonMethod, descriptionObject, methodName))
  {
    CLog::Log(LOGERROR, "JSONRPC: Invalid JSON Schema definition for method \"%s\"", methodName.c_str());
    return false;
  }

  if (m_actionMap.find(methodName) != m_actionMap.end())
  {
    CLog::Log(LOGERROR, "JSONRPC: There already is a method with the name \"%s\"", methodName.c_str());
    return false;
  }

  std::string type = GetString(descriptionObject[methodName]["type"], "");
  if (type.compare("method") != 0)
  {
    CLog::Log(LOGERROR, "JSONRPC: Invalid JSON type for method \"%s\"", methodName.c_str());
    return false;
  }

  if (method == NULL)
  {
    unsigned int size = sizeof(m_methodMaps) / sizeof(JsonRpcMethodMap);
    for (unsigned int index = 0; index < size; index++)
    {
      if (methodName.compare(m_methodMaps[index].name) == 0)
      {
        method = m_methodMaps[index].method;
        break;
      }
    }

    if (method == NULL)
    {
      CLog::Log(LOGERROR, "JSONRPC: Missing implementation for method \"%s\"", methodName.c_str());
      return false;
    }
  }

  // Parse the details of the method
  JsonRpcMethod newMethod;
  newMethod.name = methodName;
  newMethod.method = method;
  if (!parseMethod(descriptionObject[newMethod.name], newMethod))
  {
    CLog::Log(LOGERROR, "JSONRPC: Could not parse method \"%s\"", methodName.c_str());
    return false;
  }

  m_actionMap.add(newMethod);

  return true;
}

bool CJSONServiceDescription::AddType(std::string jsonType)
{
  CVariant descriptionObject;
  std::string typeName;

  if (!prepareDescription(jsonType, descriptionObject, typeName))
  {
    CLog::Log(LOGERROR, "JSONRPC: Invalid JSON Schema definition for type \"%s\"", typeName.c_str());
    return false;
  }

  if (m_types.find(typeName) != m_types.end())
  {
    CLog::Log(LOGERROR, "JSONRPC: There already is a type with the name \"%s\"", typeName.c_str());
    return false;
  }

  // Make sure the "id" attribute is correctly populated
  descriptionObject[typeName]["id"] = typeName;

  JSONSchemaTypeDefinition globalType;
  globalType.name = typeName;

  if (!parseTypeDefinition(descriptionObject[typeName], globalType, false))
  {
    CLog::Log(LOGERROR, "JSONRPC: Could not parse type \"%s\"", typeName.c_str());
    return false;
  }

  return true;
}

bool CJSONServiceDescription::AddMethod(std::string jsonMethod, MethodCall method)
{
  if (method == NULL)
  {
    CLog::Log(LOGERROR, "JSONRPC: Invalid JSONRPC method implementation");
    return false;
  }

  return addMethod(jsonMethod, method);
}

bool CJSONServiceDescription::AddBuiltinMethod(std::string jsonMethod)
{
  return addMethod(jsonMethod, NULL);
}

bool CJSONServiceDescription::AddNotification(std::string jsonNotification)
{
  CVariant descriptionObject;
  std::string notificationName;

  // Make sure the notification description actually exists and represents an object
  if (!prepareDescription(jsonNotification, descriptionObject, notificationName))
  {
    CLog::Log(LOGERROR, "JSONRPC: Invalid JSON Schema definition for notification \"%s\"", notificationName.c_str());
    return false;
  }

  if (m_notifications.find(notificationName) != m_notifications.end())
  {
    CLog::Log(LOGERROR, "JSONRPC: There already is a notification with the name \"%s\"", notificationName.c_str());
    return false;
  }

  std::string type = GetString(descriptionObject[notificationName]["type"], "");
  if (type.compare("notification") != 0)
  {
    CLog::Log(LOGERROR, "JSONRPC: Invalid JSON type for notification \"%s\"", notificationName.c_str());
    return false;
  }

  m_notifications[notificationName] = descriptionObject;

  return true;
}

int CJSONServiceDescription::GetVersion()
{
  return JSONRPC_SERVICE_VERSION;
}

JSON_STATUS CJSONServiceDescription::Print(CVariant &result, ITransportLayer *transport, IClient *client,
  bool printDescriptions /* = true */, bool printMetadata /* = false */, bool filterByTransport /* = true */,
  std::string filterByName /* = "" */, std::string filterByType /* = "" */, bool printReferences /* = true */)
{
  std::map<std::string, JSONSchemaTypeDefinition> types;
  CJsonRpcMethodMap methods;
  std::map<std::string, CVariant> notifications;

  int clientPermissions = client->GetPermissionFlags();
  int transportCapabilities = transport->GetCapabilities();

  if (filterByName.size() > 0)
  {
    CStdString name = filterByName;

    if (filterByType == "method")
    {
      name = name.ToLower();

      CJsonRpcMethodMap::JsonRpcMethodIterator methodIterator = m_actionMap.find(name);
      if (methodIterator != m_actionMap.end() &&
         (clientPermissions & methodIterator->second.permission) != 0 && ((transportCapabilities & methodIterator->second.transportneed) != 0 || !filterByTransport))
        methods.add(methodIterator->second);
      else
        return InvalidParams;
    }
    else if (filterByType == "namespace")
    {
      // append a . delimiter to make sure we check for a namespace
      name = name.ToLower().append(".");

      CJsonRpcMethodMap::JsonRpcMethodIterator methodIterator;
      CJsonRpcMethodMap::JsonRpcMethodIterator methodIteratorEnd = m_actionMap.end();
      for (methodIterator = m_actionMap.begin(); methodIterator != methodIteratorEnd; methodIterator++)
      {
        // Check if the given name is at the very beginning of the method name
        if (methodIterator->first.find(name) == 0 &&
           (clientPermissions & methodIterator->second.permission) != 0 && ((transportCapabilities & methodIterator->second.transportneed) != 0 || !filterByTransport))
          methods.add(methodIterator->second);
      }

      if (methods.begin() == methods.end())
        return InvalidParams;
    }
    else if (filterByType == "type")
    {
      std::map<std::string, JSONSchemaTypeDefinition>::const_iterator typeIterator = m_types.find(name);
      if (typeIterator != m_types.end())
        types[typeIterator->first] = typeIterator->second;
      else
        return InvalidParams;
    }
    else if (filterByType == "notification")
    {
      std::map<std::string, CVariant>::const_iterator notificationIterator = m_notifications.find(name);
      if (notificationIterator != m_notifications.end())
        notifications[notificationIterator->first] = notificationIterator->second;
      else
        return InvalidParams;
    }
    else
      return InvalidParams;

    // If we need to print all referenced types we have to go through all parameters etc
    if (printReferences)
    {
      std::vector<std::string> referencedTypes;

      // Loop through all printed types to get all referenced types
      std::map<std::string, JSONSchemaTypeDefinition>::const_iterator typeIterator;
      std::map<std::string, JSONSchemaTypeDefinition>::const_iterator typeIteratorEnd = types.end();
      for (typeIterator = types.begin(); typeIterator != typeIteratorEnd; typeIterator++)
        getReferencedTypes(typeIterator->second, referencedTypes);

      // Loop through all printed method's parameters to get all referenced types
      CJsonRpcMethodMap::JsonRpcMethodIterator methodIterator;
      CJsonRpcMethodMap::JsonRpcMethodIterator methodIteratorEnd = methods.end();
      for (methodIterator = methods.begin(); methodIterator != methodIteratorEnd; methodIterator++)
      {
        for (unsigned int index = 0; index < methodIterator->second.parameters.size(); index++)
          getReferencedTypes(methodIterator->second.parameters.at(index), referencedTypes);
      }

      for (unsigned int index = 0; index < referencedTypes.size(); index++)
      {
        std::map<std::string, JSONSchemaTypeDefinition>::const_iterator typeIterator = m_types.find(referencedTypes.at(index));
        if (typeIterator != m_types.end())
          types[typeIterator->first] = typeIterator->second;
      }
    }
  }
  else
  {
    types = m_types;
    methods = m_actionMap;
    notifications = m_notifications;
  }

  // Print the header
  result["id"] = JSONRPC_SERVICE_ID;
  result["version"] = JSONRPC_SERVICE_VERSION;
  result["description"] = JSONRPC_SERVICE_DESCRIPTION;

  std::map<std::string, JSONSchemaTypeDefinition>::const_iterator typeIterator;
  std::map<std::string, JSONSchemaTypeDefinition>::const_iterator typeIteratorEnd = types.end();
  for (typeIterator = types.begin(); typeIterator != typeIteratorEnd; typeIterator++)
  {
    CVariant currentType = CVariant(CVariant::VariantTypeObject);
    printType(typeIterator->second, false, true, true, printDescriptions, currentType);

    result["types"][typeIterator->first] = currentType;
  }

  // Iterate through all json rpc methods
  CJsonRpcMethodMap::JsonRpcMethodIterator methodIterator;
  CJsonRpcMethodMap::JsonRpcMethodIterator methodIteratorEnd = methods.end();
  for (methodIterator = methods.begin(); methodIterator != methodIteratorEnd; methodIterator++)
  {
    if ((clientPermissions & methodIterator->second.permission) == 0 || ((transportCapabilities & methodIterator->second.transportneed) == 0 && filterByTransport))
      continue;

    CVariant currentMethod = CVariant(CVariant::VariantTypeObject);

    currentMethod["type"] = "method";
    if (printDescriptions && !methodIterator->second.description.empty())
      currentMethod["description"] = methodIterator->second.description;
    if (printMetadata)
    {
      currentMethod["permission"] = PermissionToString(methodIterator->second.permission);
    }

    currentMethod["params"] = CVariant(CVariant::VariantTypeArray);
    for (unsigned int paramIndex = 0; paramIndex < methodIterator->second.parameters.size(); paramIndex++)
    {
      CVariant param = CVariant(CVariant::VariantTypeObject);
      printType(methodIterator->second.parameters.at(paramIndex), true, false, true, printDescriptions, param);
      currentMethod["params"].append(param);
    }

    currentMethod["returns"] = methodIterator->second.returns;

    result["methods"][methodIterator->second.name] = currentMethod;
  }

  // Print notification description
  std::map<std::string, CVariant>::const_iterator notificationIterator;
  std::map<std::string, CVariant>::const_iterator notificationIteratorEnd = notifications.end();
  for (notificationIterator = notifications.begin(); notificationIterator != notificationIteratorEnd; notificationIterator++)
    result["notifications"][notificationIterator->first] = notificationIterator->second;

  return OK;
}

JSON_STATUS CJSONServiceDescription::CheckCall(const char* const method, const CVariant &requestParameters, IClient *client, bool notification, MethodCall &methodCall, CVariant &outputParameters)
{
  CJsonRpcMethodMap::JsonRpcMethodIterator iter = m_actionMap.find(method);
  if (iter != m_actionMap.end())
  {
    if (client != NULL && (client->GetPermissionFlags() & iter->second.permission) && (!notification || (iter->second.permission & OPERATION_PERMISSION_NOTIFICATION)))
    {
      methodCall = iter->second.method;

      // Count the number of actually handled (present)
      // parameters
      unsigned int handled = 0;
      CVariant errorData = CVariant(CVariant::VariantTypeObject);
      errorData["method"] = iter->second.name;

      // Loop through all the parameters to check
      for (unsigned int i = 0; i < iter->second.parameters.size(); i++)
      {
        // Evaluate the current parameter
        JSON_STATUS status = checkParameter(requestParameters, iter->second.parameters.at(i), i, outputParameters, handled, errorData);
        if (status != OK)
        {
          // Return the error data object in the outputParameters reference
          outputParameters = errorData;
          return status;
        }
      }

      // Check if there were unnecessary parameters
      if (handled < requestParameters.size())
      {
        errorData["message"] = "Too many parameters";
        outputParameters = errorData;
        return InvalidParams;
      }

      return OK;
    }
    else
      return BadPermission;
  }
  else
    return MethodNotFound;
}

void CJSONServiceDescription::printType(const JSONSchemaTypeDefinition &type, bool isParameter, bool isGlobal, bool printDefault, bool printDescriptions, CVariant &output)
{
  bool typeReference = false;

  // Printing general fields
  if (isParameter)
    output["name"] = type.name;

  if (isGlobal)
    output["id"] = type.ID;
  else if (!type.ID.empty())
  {
    output["$ref"] = type.ID;
    typeReference = true;
  }

  if (printDescriptions && !type.description.empty())
    output["description"] = type.description;

  if (isParameter || printDefault)
  {
    if (!type.optional)
      output["required"] = true;
    if (type.optional && type.type != ObjectValue && type.type != ArrayValue)
      output["default"] = type.defaultValue;
  }

  if (!typeReference)
  {
    SchemaValueTypeToJson(type.type, output["type"]);

    // Printing enum field
    if (type.enums.size() > 0)
    {
      output["enums"] = CVariant(CVariant::VariantTypeArray);
      for (unsigned int enumIndex = 0; enumIndex < type.enums.size(); enumIndex++)
        output["enums"].append(type.enums.at(enumIndex));
    }

    // Printing integer/number fields
    if (HasType(type.type, IntegerValue) || HasType(type.type, NumberValue))
    {
      if (HasType(type.type, NumberValue))
      {
        if (type.minimum > -numeric_limits<double>::max())
          output["minimum"] = type.minimum;
        if (type.maximum < numeric_limits<double>::max())
          output["maximum"] = type.maximum;
      }
      else
      {
        if (type.minimum > numeric_limits<int>::min())
          output["minimum"] = (int)type.minimum;
        if (type.maximum < numeric_limits<int>::max())
          output["maximum"] = (int)type.maximum;
      }

      if (type.exclusiveMinimum)
        output["exclusiveMinimum"] = true;
      if (type.exclusiveMaximum)
        output["exclusiveMaximum"] = true;
      if (type.divisibleBy > 0)
        output["divisibleBy"] = type.divisibleBy;
    }

    // Print array fields
    if (HasType(type.type, ArrayValue))
    {
      if (type.items.size() == 1)
      {
        printType(type.items.at(0), false, false, false, printDescriptions, output["items"]);
      }
      else if (type.items.size() > 1)
      {
        output["items"] = CVariant(CVariant::VariantTypeArray);
        for (unsigned int itemIndex = 0; itemIndex < type.items.size(); itemIndex++)
        {
          CVariant item = CVariant(CVariant::VariantTypeObject);
          printType(type.items.at(itemIndex), false, false, false, printDescriptions, item);
          output["items"].append(item);
        }
      }

      if (type.minItems > 0)
        output["minItems"] = type.minItems;
      if (type.maxItems > 0)
        output["maxItems"] = type.maxItems;

      if (type.additionalItems.size() == 1)
      {
        printType(type.additionalItems.at(0), false, false, false, printDescriptions, output["additionalItems"]);
      }
      else if (type.additionalItems.size() > 1)
      {
        output["additionalItems"] = CVariant(CVariant::VariantTypeArray);
        for (unsigned int addItemIndex = 0; addItemIndex < type.additionalItems.size(); addItemIndex++)
        {
          CVariant item = CVariant(CVariant::VariantTypeObject);
          printType(type.additionalItems.at(addItemIndex), false, false, false, printDescriptions, item);
          output["additionalItems"].append(item);
        }
      }

      if (type.uniqueItems)
        output["uniqueItems"] = true;
    }

    // Print object fields
    if (HasType(type.type, ObjectValue) && type.properties.size() > 0)
    {
      output["properties"] = CVariant(CVariant::VariantTypeObject);

      JSONSchemaTypeDefinition::CJsonSchemaPropertiesMap::JSONSchemaPropertiesIterator propertiesEnd = type.properties.end();
      JSONSchemaTypeDefinition::CJsonSchemaPropertiesMap::JSONSchemaPropertiesIterator propertiesIterator;
      for (propertiesIterator = type.properties.begin(); propertiesIterator != propertiesEnd; propertiesIterator++)
      {
        printType(propertiesIterator->second, false, false, true, printDescriptions, output["properties"][propertiesIterator->first]);
      }
    }
  }
}

JSON_STATUS CJSONServiceDescription::checkParameter(const CVariant &requestParameters, const JSONSchemaTypeDefinition &type, unsigned int position, CVariant &outputParameters, unsigned int &handled, CVariant &errorData)
{
  // Let's check if the parameter has been provided
  if (ParameterExists(requestParameters, type.name, position))
  {
    // Get the parameter
    CVariant parameterValue = GetParameter(requestParameters, type.name, position);

    // Evaluate the type of the parameter
    JSON_STATUS status = checkType(parameterValue, type, outputParameters[type.name], errorData["stack"]);
    if (status != OK)
      return status;

    // The parameter was present and valid
    handled++;
  }
  // If the parameter has not been provided but is optional
  // we can use its default value
  else if (type.optional)
    outputParameters[type.name] = type.defaultValue;
  // The parameter is required but has not been provided => invalid
  else
  {
    errorData["stack"]["name"] = type.name;
    SchemaValueTypeToJson(type.type, errorData["stack"]["type"]);
    errorData["stack"]["message"] = "Missing parameter";
    return InvalidParams;
  }

  return OK;
}

JSON_STATUS CJSONServiceDescription::checkType(const CVariant &value, const JSONSchemaTypeDefinition &type, CVariant &outputValue, CVariant &errorData)
{
  if (!type.name.empty())
    errorData["name"] = type.name;
  SchemaValueTypeToJson(type.type, errorData["type"]);
  CStdString errorMessage;

  // Let's check the type of the provided parameter
  if (!IsType(value, type.type))
  {
    CLog::Log(LOGWARNING, "JSONRPC: Type mismatch in type %s", type.name.c_str());
    errorMessage.Format("Invalid type %s received", ValueTypeToString(value.type()));
    errorData["message"] = errorMessage.c_str();
    return InvalidParams;
  }
  else if (value.isNull() && !HasType(type.type, NullValue))
  {
    CLog::Log(LOGWARNING, "JSONRPC: Value is NULL in type %s", type.name.c_str());
    errorData["message"] = "Received value is null";
    return InvalidParams;
  }

  // If it is an array we need to
  // - check the type of every element ("items")
  // - check if they need to be unique ("uniqueItems")
  if (HasType(type.type, ArrayValue) && value.isArray())
  {
    outputValue = CVariant(CVariant::VariantTypeArray);
    // Check the number of items against minItems and maxItems
    if ((type.minItems > 0 && value.size() < type.minItems) || (type.maxItems > 0 && value.size() > type.maxItems))
    {
      CLog::Log(LOGWARNING, "JSONRPC: Number of array elements does not match minItems and/or maxItems in type %s", type.name.c_str());
      if (type.minItems > 0 && type.maxItems > 0)
        errorMessage.Format("Between %d and %d array items expected but %d received", type.minItems, type.maxItems, value.size());
      else if (type.minItems > 0)
        errorMessage.Format("At least %d array items expected but only %d received", type.minItems, value.size());
      else
        errorMessage.Format("Only %d array items expected but %d received", type.maxItems, value.size());
      errorData["message"] = errorMessage.c_str();
      return InvalidParams;
    }

    if (type.items.size() == 0)
    {
      outputValue = value;
    }
    else if (type.items.size() == 1)
    {
      JSONSchemaTypeDefinition itemType = type.items.at(0);

      // Loop through all array elements
      for (unsigned int arrayIndex = 0; arrayIndex < value.size(); arrayIndex++)
      {
        CVariant temp;
        JSON_STATUS status = checkType(value[arrayIndex], itemType, temp, errorData["property"]);
        outputValue.push_back(temp);
        if (status != OK)
        {
          CLog::Log(LOGWARNING, "JSONRPC: Array element at index %u does not match in type %s", arrayIndex, type.name.c_str());
          errorMessage.Format("array element at index %u does not match", arrayIndex);
          errorData["message"] = errorMessage.c_str();
          return status;
        }
      }
    }
    // We have more than one element in "items"
    // so we have tuple typing, which means that
    // every element in the value array must match
    // with the type at the same position in the
    // "items" array
    else
    {
      // If the number of elements in the value array
      // does not match the number of elements in the
      // "items" array and additional items are not
      // allowed there is no need to check every element
      if (value.size() < type.items.size() || (value.size() != type.items.size() && type.additionalItems.size() == 0))
      {
        CLog::Log(LOGWARNING, "JSONRPC: One of the array elements does not match in type %s", type.name.c_str());
        errorMessage.Format("%d array elements expected but %d received", type.items.size(), value.size());
        errorData["message"] = errorMessage.c_str();
        return InvalidParams;
      }

      // Loop through all array elements until there
      // are either no more schemas in the "items"
      // array or no more elements in the value's array
      unsigned int arrayIndex;
      for (arrayIndex = 0; arrayIndex < min(type.items.size(), (size_t)value.size()); arrayIndex++)
      {
        JSON_STATUS status = checkType(value[arrayIndex], type.items.at(arrayIndex), outputValue[arrayIndex], errorData["property"]);
        if (status != OK)
        {
          CLog::Log(LOGWARNING, "JSONRPC: Array element at index %u does not match with items schema in type %s", arrayIndex, type.name.c_str());
          return status;
        }
      }

      if (type.additionalItems.size() > 0)
      {
        // Loop through the rest of the elements
        // in the array and check them against the
        // "additionalItems"
        for (; arrayIndex < value.size(); arrayIndex++)
        {
          bool ok = false;
          for (unsigned int additionalIndex = 0; additionalIndex < type.additionalItems.size(); additionalIndex++)
          {
            CVariant dummyError;
            if (checkType(value[arrayIndex], type.additionalItems.at(additionalIndex), outputValue[arrayIndex], dummyError) == OK)
            {
              ok = true;
              break;
            }
          }

          if (!ok)
          {
            CLog::Log(LOGWARNING, "JSONRPC: Array contains non-conforming additional items in type %s", type.name.c_str());
            errorMessage.Format("Array element at index %u does not match the \"additionalItems\" schema", arrayIndex);
            errorData["message"] = errorMessage.c_str();
            return InvalidParams;
          }
        }
      }
    }

    // If every array element is unique we need to check each one
    if (type.uniqueItems)
    {
      for (unsigned int checkingIndex = 0; checkingIndex < outputValue.size(); checkingIndex++)
      {
        for (unsigned int checkedIndex = checkingIndex + 1; checkedIndex < outputValue.size(); checkedIndex++)
        {
          // If two elements are the same they are not unique
          if (outputValue[checkingIndex] == outputValue[checkedIndex])
          {
            CLog::Log(LOGWARNING, "JSONRPC: Not unique array element at index %u and %u in type %s", checkingIndex, checkedIndex, type.name.c_str());
            errorMessage.Format("Array element at index %u is not unique (same as array element at index %u)", checkingIndex, checkedIndex);
            errorData["message"] = errorMessage.c_str();
            return InvalidParams;
          }
        }
      }
    }

    return OK;
  }

  // If it is an object we need to check every element
  // against the defined "properties"
  if (HasType(type.type, ObjectValue) && value.isObject())
  {
    unsigned int handled = 0;
    JSONSchemaTypeDefinition::CJsonSchemaPropertiesMap::JSONSchemaPropertiesIterator propertiesEnd = type.properties.end();
    JSONSchemaTypeDefinition::CJsonSchemaPropertiesMap::JSONSchemaPropertiesIterator propertiesIterator;
    for (propertiesIterator = type.properties.begin(); propertiesIterator != propertiesEnd; propertiesIterator++)
    {
      if (value.isMember(propertiesIterator->first))
      {
        JSON_STATUS status = checkType(value[propertiesIterator->first], propertiesIterator->second, outputValue[propertiesIterator->first], errorData["property"]);
        if (status != OK)
        {
          CLog::Log(LOGWARNING, "JSONRPC: Invalid property \"%s\" in type %s", propertiesIterator->first.c_str(), type.name.c_str());
          return status;
        }
        handled++;
      }
      else if (propertiesIterator->second.optional)
        outputValue[propertiesIterator->first] = propertiesIterator->second.defaultValue;
      else
      {
        CLog::Log(LOGWARNING, "JSONRPC: Missing property \"%s\" in type %s", propertiesIterator->first.c_str(), type.name.c_str());
        errorData["property"]["name"] = propertiesIterator->first.c_str();
        errorData["property"]["type"] = SchemaValueTypeToString(propertiesIterator->second.type);
        errorData["message"] = "Missing property";
        return InvalidParams;
      }
    }

    // Additional properties are not allowed
    if (handled < value.size())
    {
      errorData["message"] = "Unexpected additional properties received";
      return InvalidParams;
    }

    return OK;
  }

  // It's neither an array nor an object

  // If it can only take certain values ("enum")
  // we need to check against those
  if (type.enums.size() > 0)
  {
    bool valid = false;
    for (std::vector<CVariant>::const_iterator enumItr = type.enums.begin(); enumItr != type.enums.end(); enumItr++)
    {
      if (*enumItr == value)
      {
        valid = true;
        break;
      }
    }

    if (!valid)
    {
      CLog::Log(LOGWARNING, "JSONRPC: Value does not match any of the enum values in type %s", type.name.c_str());
      errorData["message"] = "Received value does not match any of the defined enum values";
      return InvalidParams;
    }
  }

  // If we have a number or an integer type, we need
  // to check the minimum and maximum values
  if ((HasType(type.type, NumberValue) || HasType(type.type, IntegerValue)) && value.isDouble())
  {
    double numberValue = value.asDouble();
    // Check minimum
    if ((type.exclusiveMinimum && numberValue <= type.minimum) || (!type.exclusiveMinimum && numberValue < type.minimum) ||
    // Check maximum
        (type.exclusiveMaximum && numberValue >= type.maximum) || (!type.exclusiveMaximum && numberValue > type.maximum))        
    {
      CLog::Log(LOGWARNING, "JSONRPC: Value does not lay between minimum and maximum in type %s", type.name.c_str());
      errorMessage.Format("Value between %f (%s) and %f (%s) expected but %f received", 
        type.minimum, type.exclusiveMinimum ? "exclusive" : "inclusive", type.maximum, type.exclusiveMaximum ? "exclusive" : "inclusive", numberValue);
      errorData["message"] = errorMessage.c_str();
      return InvalidParams;
    }
    // Check divisibleBy
    else if ((HasType(type.type, IntegerValue) && type.divisibleBy > 0 && ((int)numberValue % type.divisibleBy) != 0))
    {
      CLog::Log(LOGWARNING, "JSONRPC: Value does not meet divisibleBy requirements in type %s", type.name.c_str());
      errorMessage.Format("Value should be divisible by %d but %d received", type.divisibleBy, (int)numberValue);
      errorData["message"] = errorMessage.c_str();
      return InvalidParams;
    }
  }

  // Otherwise it can have any value
  outputValue = value;
  return OK;
}

bool CJSONServiceDescription::parseMethod(const CVariant &value, JsonRpcMethod &method)
{
  // Parse XBMC specific information about the method
  method.transportneed = StringToTransportLayer(value.isMember("transport") ? value["transport"].asString() : "");
  method.permission = StringToPermission(value.isMember("permission") ? value["permission"].asString() : "");
  method.description = GetString(value["description"], "");

  // Check whether there are parameters defined
  if (value.isMember("params") && value["params"].isArray())
  {
    // Loop through all defined parameters
    for (unsigned int paramIndex = 0; paramIndex < value["params"].size(); paramIndex++)
    {
      CVariant parameter = value["params"][paramIndex];
      // If the parameter definition does not contain a valid "name" or
      // "type" element we will ignore it
      if (!parameter.isMember("name") || !parameter["name"].isString() ||
          (!parameter.isMember("type") && !parameter.isMember("$ref")) ||
          (parameter.isMember("type") && !parameter["type"].isString() &&
          !parameter["type"].isArray()) || (parameter.isMember("$ref") &&
          !parameter["$ref"].isString()))
      {
        CLog::Log(LOGWARNING, "JSONRPC: Method %s has a badly defined parameter", method.name.c_str());
        return false;
      }

      // Parse the parameter and add it to the list
      // of defined parameters
      JSONSchemaTypeDefinition param;
      if (!parseParameter(parameter, param))
        return false;
      method.parameters.push_back(param);
    }
  }
    
  // Parse the return value of the method
  parseReturn(value, method.returns);

  return true;
}

bool CJSONServiceDescription::parseParameter(CVariant &value, JSONSchemaTypeDefinition &parameter)
{
  parameter.name = GetString(value["name"], "");

  // Parse the type and default value of the parameter
  return parseTypeDefinition(value, parameter, true);
}

bool CJSONServiceDescription::parseTypeDefinition(const CVariant &value, JSONSchemaTypeDefinition &type, bool isParameter)
{
  bool isReferenceType = false;
  bool hasReference = false;

  // Check if the type of the parameter defines a json reference
  // to a type defined somewhere else
  if (value.isMember("$ref") && value["$ref"].isString())
  {
    // Get the name of the referenced type
    std::string refType = value["$ref"].asString();
    // Check if the referenced type exists
    std::map<std::string, JSONSchemaTypeDefinition>::const_iterator iter = m_types.find(refType);
    if (refType.length() <= 0 || iter == m_types.end())
    {
      CLog::Log(LOGDEBUG, "JSONRPC: JSON schema type %s references an unknown type %s", type.name.c_str(), refType.c_str());
      return false;
    }
    
    std::string typeName = type.name;
    type = iter->second;
    if (!typeName.empty())
      type.name = typeName;
    hasReference = true;
  }
  else if (value.isMember("id") && value["id"].isString())
  {
    type.ID = GetString(value["id"], "");
    isReferenceType = true;
  }

  // Check if the "required" field has been defined
  type.optional = value.isMember("required") && value["required"].isBoolean() ? !value["required"].asBoolean() : true;

  // Get the "description"
  if (!hasReference || (value.isMember("description") && value["description"].isString()))
    type.description = GetString(value["description"], "");

  if (hasReference)
  {
    // If there is a specific default value, read it
    if (value.isMember("default") && IsType(value["default"], type.type))
    {
      bool ok = false;
      if (type.enums.size() >= 0)
        ok = true;
      // If the type has an enum definition we must make
      // sure that the default value is a valid enum value
      else
      {
        for (std::vector<CVariant>::const_iterator itr = type.enums.begin(); itr != type.enums.end(); itr++)
        {
          if (value["default"] == *itr)
          {
            ok = true;
            break;
          }
        }
      }

      if (ok)
        type.defaultValue = value["default"];
    }

    return true;
  }

  // Get the defined type of the parameter
  if (value["type"].isArray())
  {
    int parsedType = 0;
    // If the defined type is an array, we have
    // to handle a union type
    for (unsigned int typeIndex = 0; typeIndex < value["type"].size(); typeIndex++)
    {
      // If the type is a string try to parse it
      if (value["type"][typeIndex].isString())
        parsedType |= StringToSchemaValueType(value["type"][typeIndex].asString());
      else
        CLog::Log(LOGWARNING, "JSONRPC: Invalid type in union type definition of type %s", type.name.c_str());
    }

    type.type = (JSONSchemaType)parsedType;

    // If the type has not been set yet
    // set it to "any"
    if (type.type == 0)
      type.type = AnyValue;
  }
  else
    type.type = value["type"].isString() ? StringToSchemaValueType(value["type"].asString()) : AnyValue;

  if (type.type == ObjectValue)
  {
    // If the type definition is of type "object"
    // and has a "properties" definition we need
    // to handle these as well
    if (value.isMember("properties") && value["properties"].isObject())
    {
      // Get all child elements of the "properties"
      // object and loop through them
      for (CVariant::const_iterator_map itr = value["properties"].begin_map(); itr != value["properties"].end_map(); itr++)
      {
        // Create a new type definition, store the name
        // of the current property into it, parse it
        // recursively and add its default value
        // to the current type's default value
        JSONSchemaTypeDefinition propertyType;
        propertyType.name = itr->first;
        if (!parseTypeDefinition(itr->second, propertyType, false))
          return false;
        type.defaultValue[itr->first] = propertyType.defaultValue;
        type.properties.add(propertyType);
      }
    }
  }
  else 
  {
    // If the defined parameter is an array
    // we need to check for detailed definitions
    // of the array items
    if (type.type == ArrayValue)
    {
      // Check for "uniqueItems" field
      if (value.isMember("uniqueItems") && value["uniqueItems"].isBoolean())
        type.uniqueItems = value["uniqueItems"].asBoolean();
      else
        type.uniqueItems = false;

      // Check for "additionalItems" field
      if (value.isMember("additionalItems"))
      {
        // If it is an object, there is only one schema for it
        if (value["additionalItems"].isObject())
        {
          JSONSchemaTypeDefinition additionalItem;

          if (parseTypeDefinition(value["additionalItems"], additionalItem, false))
            type.additionalItems.push_back(additionalItem);
        }
        // If it is an array there may be multiple schema definitions
        else if (value["additionalItems"].isArray())
        {
          for (unsigned int itemIndex = 0; itemIndex < value["additionalItems"].size(); itemIndex++)
          {
            JSONSchemaTypeDefinition additionalItem;

            if (parseTypeDefinition(value["additionalItems"][itemIndex], additionalItem, false))
              type.additionalItems.push_back(additionalItem);
          }
        }
        // If it is not a (array of) schema and not a bool (default value is false)
        // it has an invalid value
        else if (!value["additionalItems"].isBoolean())
          CLog::Log(LOGWARNING, "Invalid \"additionalItems\" value for type %s", type.name.c_str());
      }

      // If the "items" field is a single object
      // we can parse that directly
      if (value.isMember("items"))
      {
        if (value["items"].isObject())
        {
          JSONSchemaTypeDefinition item;

          if (!parseTypeDefinition(value["items"], item, false))
            return false;
          type.items.push_back(item);
        }
        // Otherwise if it is an array we need to
        // parse all elements and store them
        else if (value["items"].isArray())
        {
          for (CVariant::const_iterator_array itemItr = value["items"].begin_array(); itemItr != value["items"].end_array(); itemItr++)
          {
            JSONSchemaTypeDefinition item;

            if (!parseTypeDefinition(*itemItr, item, false))
              return false;
            type.items.push_back(item);
          }
        }
      }

      type.minItems = (unsigned int)value["minItems"].asUnsignedInteger(0);
      type.maxItems = (unsigned int)value["maxItems"].asUnsignedInteger(0);
    }
    // The type is whether an object nor an array
    else 
    {
      if ((type.type & NumberValue) == NumberValue || (type.type  & IntegerValue) == IntegerValue)
      {
        if ((type.type & NumberValue) == NumberValue)
        {
          type.minimum = value["minimum"].asDouble(-numeric_limits<double>::max());
          type.maximum = value["maximum"].asDouble(numeric_limits<double>::max());
        }
        else if ((type.type  & IntegerValue) == IntegerValue)
        {
          type.minimum = (double)value["minimum"].asInteger(numeric_limits<int>::min());
          type.maximum = (double)value["maximum"].asInteger(numeric_limits<int>::max());
        }

        type.exclusiveMinimum = value["exclusiveMinimum"].asBoolean(false);
        type.exclusiveMaximum = value["exclusiveMaximum"].asBoolean(false);
        type.divisibleBy = (unsigned int)value["divisibleBy"].asUnsignedInteger(0);
      }

      // If the type definition is neither an
      // "object" nor an "array" we can check
      // for an "enum" definition
      if (value.isMember("enum") && value["enum"].isArray())
      {
        // Loop through all elements in the "enum" array
        for (CVariant::const_iterator_array enumItr = value["enum"].begin_array(); enumItr != value["enum"].end_array(); enumItr++)
        {
          // Check for duplicates and eliminate them
          bool approved = true;
          for (unsigned int approvedIndex = 0; approvedIndex < type.enums.size(); approvedIndex++)
          {
            if (*enumItr == type.enums.at(approvedIndex))
            {
              approved = false;
              break;
            }
          }

          // Only add the current item to the enum value 
          // list if it is not duplicate
          if (approved)
            type.enums.push_back(*enumItr);
        }
      }
    }

    // If there is a definition for a default value and its type
    // matches the type of the parameter we can parse it
    bool ok = false;
    if (value.isMember("default") && IsType(value["default"], type.type))
    {
      if (type.enums.size() >= 0)
        ok = true;
      // If the type has an enum definition we must make
      // sure that the default value is a valid enum value
      else
      {
        for (std::vector<CVariant>::const_iterator itr = type.enums.begin(); itr != type.enums.end(); itr++)
        {
          if (value["default"] == *itr)
          {
            ok = true;
            break;
          }
        }
      }
    }

    if (ok)
      type.defaultValue = value["default"];
    else
    {
      // If the type of the default value definition does not
      // match the type of the parameter we have to log this
      if (value.isMember("default") && !IsType(value["default"], type.type))
        CLog::Log(LOGWARNING, "JSONRPC: Parameter %s has an invalid default value", type.name.c_str());
      
      // If the type contains an "enum" we need to get the
      // default value from the first enum value
      if (type.enums.size() > 0)
        type.defaultValue = type.enums.at(0);
      // otherwise set a default value instead
      else
        SetDefaultValue(type.defaultValue, type.type);
    }
  }

  if (isReferenceType)
    addReferenceTypeDefinition(type);

  return true;
}

void CJSONServiceDescription::parseReturn(const CVariant &value, CVariant &returns)
{
  // Only parse the "returns" definition if there is one
  if (value.isMember("returns"))
    returns = value["returns"];
}

void CJSONServiceDescription::addReferenceTypeDefinition(JSONSchemaTypeDefinition &typeDefinition)
{
  // If the given json value is no object or does not contain an "id" field
  // of type string it is no valid type definition
  if (typeDefinition.ID.empty())
    return;

  // If the id has already been defined we ignore the type definition
  if (m_types.find(typeDefinition.ID) != m_types.end())
    return;

  // Add the type to the list of type definitions
  m_types[typeDefinition.ID] = typeDefinition;
}

void CJSONServiceDescription::getReferencedTypes(const JSONSchemaTypeDefinition &type, std::vector<std::string> &referencedTypes)
{
  // If the current type is a referenceable object, we can add it to the list
  if (type.ID.size() > 0)
  {
    for (unsigned int index = 0; index < referencedTypes.size(); index++)
    {
      // The referenceable object has already been added to the list so we can just skip it
      if (type.ID == referencedTypes.at(index))
        return;
    }

    referencedTypes.push_back(type.ID);
  }

  // If the current type is an object we need to check its properties
  if (HasType(type.type, ObjectValue))
  {
    JSONSchemaTypeDefinition::CJsonSchemaPropertiesMap::JSONSchemaPropertiesIterator iter;
    JSONSchemaTypeDefinition::CJsonSchemaPropertiesMap::JSONSchemaPropertiesIterator iterEnd = type.properties.end();
    for (iter = type.properties.begin(); iter != iterEnd; iter++)
      getReferencedTypes(iter->second, referencedTypes);
  }
  // If the current type is an array we need to check its items
  if (HasType(type.type, ArrayValue))
  {
    unsigned int index;
    for (index = 0; index < type.items.size(); index++)
      getReferencedTypes(type.items.at(index), referencedTypes);

    for (index = 0; index < type.additionalItems.size(); index++)
      getReferencedTypes(type.additionalItems.at(index), referencedTypes);
  }
}

CJSONServiceDescription::CJsonRpcMethodMap::CJsonRpcMethodMap()
{
  m_actionmap = std::map<std::string, JsonRpcMethod>();
}

void CJSONServiceDescription::CJsonRpcMethodMap::add(const JsonRpcMethod &method)
{
  CStdString name = method.name;
  name = name.ToLower();
  m_actionmap[name] = method;
}

CJSONServiceDescription::CJsonRpcMethodMap::JsonRpcMethodIterator CJSONServiceDescription::CJsonRpcMethodMap::begin() const
{
  return m_actionmap.begin();
}

CJSONServiceDescription::CJsonRpcMethodMap::JsonRpcMethodIterator CJSONServiceDescription::CJsonRpcMethodMap::find(const std::string& key) const
{
  return m_actionmap.find(key);
}

CJSONServiceDescription::CJsonRpcMethodMap::JsonRpcMethodIterator CJSONServiceDescription::CJsonRpcMethodMap::end() const
{
  return m_actionmap.end();
}
