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
#include "PlayerOperations.h"
#include "AVPlayerOperations.h"
#include "PicturePlayerOperations.h"
#include "AVPlaylistOperations.h"
#include "PlaylistOperations.h"
#include "FileOperations.h"
#include "AudioLibrary.h"
#include "VideoLibrary.h"
#include "SystemOperations.h"
#include "XBMCOperations.h"
#include "settings/AdvancedSettings.h"
#include "interfaces/AnnouncementManager.h"
#include "utils/log.h"
#include "utils/Variant.h"
#include <string.h>
#include "ServiceDescription.h"

using namespace ANNOUNCEMENT;
using namespace JSONRPC;
using namespace Json;
using namespace std;

bool CJSONRPC::m_initialized = false;

JsonRpcMethodMap CJSONRPC::m_methodMaps[] = {
// JSON-RPC
  { "JSONRPC.Introspect",                           CJSONRPC::Introspect },
  { "JSONRPC.Version",                              CJSONRPC::Version },
  { "JSONRPC.Permission",                           CJSONRPC::Permission },
  { "JSONRPC.Ping",                                 CJSONRPC::Ping },
  { "JSONRPC.GetAnnouncementFlags",                 CJSONRPC::GetAnnouncementFlags },
  { "JSONRPC.SetAnnouncementFlags",                 CJSONRPC::SetAnnouncementFlags },
  { "JSONRPC.Announce",                             CJSONRPC::Announce },

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
  // TODO

// Audio Playlist
  // TODO

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
  { "AudioLibrary.GetAlbums",                       CAudioLibrary::GetAlbums },
  { "AudioLibrary.GetAlbumDetails",                 CAudioLibrary::GetAlbumDetails },
  { "AudioLibrary.GetSongs",                        CAudioLibrary::GetSongs },
  { "AudioLibrary.GetSongDetails",                  CAudioLibrary::GetSongDetails },
  { "AudioLibrary.GetGenres",                       CAudioLibrary::GetGenres },
  { "AudioLibrary.ScanForContent",                  CAudioLibrary::ScanForContent },

// Video Library
  { "VideoLibrary.GetMovies",                       CVideoLibrary::GetMovies },
  { "VideoLibrary.GetMovieDetails",                 CVideoLibrary::GetMovieDetails },
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

// XBMC operations
  { "XBMC.GetVolume",                               CXBMCOperations::GetVolume },
  { "XBMC.SetVolume",                               CXBMCOperations::SetVolume },
  { "XBMC.ToggleMute",                              CXBMCOperations::ToggleMute },
  { "XBMC.Play",                                    CXBMCOperations::Play },
  { "XBMC.StartSlideshow",                          CXBMCOperations::StartSlideshow },
  { "XBMC.Log",                                     CXBMCOperations::Log },
  { "XBMC.Quit",                                    CXBMCOperations::Quit }
};

/*Command CJSONRPC::m_commands[] = {
// Video Playlist
  { "VideoPlaylist.Play",                           CAVPlaylistOperations::Play,                         Response,     ControlPlayback, "" },
  { "VideoPlaylist.SkipPrevious",                   CAVPlaylistOperations::SkipPrevious,                 Response,     ControlPlayback, "" },
  { "VideoPlaylist.SkipNext",                       CAVPlaylistOperations::SkipNext,                     Response,     ControlPlayback, "" },

  { "VideoPlaylist.GetItems",                       CAVPlaylistOperations::GetItems,                     Response,     ReadData,        "" },
  { "VideoPlaylist.Add",                            CAVPlaylistOperations::Add,                          Response,     ControlPlayback, "" },
  { "VideoPlaylist.Insert",                         CAVPlaylistOperations::Insert,                       Response,     ControlPlayback, "Insert item(s) into playlist" },
  { "VideoPlaylist.Clear",                          CAVPlaylistOperations::Clear,                        Response,     ControlPlayback, "Clear video playlist" },
  { "VideoPlaylist.Shuffle",                        CAVPlaylistOperations::Shuffle,                      Response,     ControlPlayback, "Shuffle video playlist" },
  { "VideoPlaylist.UnShuffle",                      CAVPlaylistOperations::UnShuffle,                    Response,     ControlPlayback, "UnShuffle video playlist" },
  { "VideoPlaylist.Remove",                         CAVPlaylistOperations::Remove,                       Response,     ControlPlayback, "Remove entry from playlist" },

// AudioPlaylist
  { "AudioPlaylist.Play",                           CAVPlaylistOperations::Play,                         Response,     ControlPlayback, "" },
  { "AudioPlaylist.SkipPrevious",                   CAVPlaylistOperations::SkipPrevious,                 Response,     ControlPlayback, "" },
  { "AudioPlaylist.SkipNext",                       CAVPlaylistOperations::SkipNext,                     Response,     ControlPlayback, "" },

  { "AudioPlaylist.GetItems",                       CAVPlaylistOperations::GetItems,                     Response,     ReadData,        "" },
  { "AudioPlaylist.Add",                            CAVPlaylistOperations::Add,                          Response,     ControlPlayback, "" },
  { "AudioPlaylist.Insert",                         CAVPlaylistOperations::Insert,                       Response,     ControlPlayback, "Insert item(s) into playlist" },
  { "AudioPlaylist.Clear",                          CAVPlaylistOperations::Clear,                        Response,     ControlPlayback, "Clear audio playlist" },
  { "AudioPlaylist.Shuffle",                        CAVPlaylistOperations::Shuffle,                      Response,     ControlPlayback, "Shuffle audio playlist" },
  { "AudioPlaylist.UnShuffle",                      CAVPlaylistOperations::UnShuffle,                    Response,     ControlPlayback, "UnShuffle audio playlist" },
  { "AudioPlaylist.Remove",                         CAVPlaylistOperations::Remove,                       Response,     ControlPlayback, "Remove entry from playlist" },
};*/

void CJSONRPC::Initialize()
{
  if (m_initialized)
    return;

  unsigned int size = sizeof(m_methodMaps) / sizeof(JsonRpcMethodMap);
  if (!CJSONServiceDescription::Parse(m_methodMaps, size))
    CLog::Log(LOGSEVERE, "JSONRPC: Error while parsing the json rpc service description");
  else
    CLog::Log(LOGINFO, "JSONRPC: json rpc service description successfully loaded");
  
  m_initialized = true;
}

JSON_STATUS CJSONRPC::Introspect(const CStdString &method, ITransportLayer *transport, IClient *client, const Json::Value& parameterObject, Json::Value &result)
{
  bool getDescriptions = parameterObject["getdescriptions"].asBool();
  bool getMetadata = parameterObject["getmetadata"].asBool();
  bool filterByTransport = parameterObject["filterbytransport"].asBool();

  CJSONServiceDescription::Print(result, transport, client, getDescriptions, getMetadata, filterByTransport);

  return OK;
}

JSON_STATUS CJSONRPC::Version(const CStdString &method, ITransportLayer *transport, IClient *client, const Json::Value& parameterObject, Json::Value &result)
{
  result["version"] = CJSONServiceDescription::GetVersion();

  return OK;
}

JSON_STATUS CJSONRPC::Permission(const CStdString &method, ITransportLayer *transport, IClient *client, const Json::Value& parameterObject, Json::Value &result)
{
  int flags = client->GetPermissionFlags();

  for (int i = 1; i <= OPERATION_PERMISSION_ALL; i *= 2)
    result[PermissionToString((OperationPermission)i)] = (flags & i) > 0;

  return OK;
}

JSON_STATUS CJSONRPC::Ping(const CStdString &method, ITransportLayer *transport, IClient *client, const Json::Value& parameterObject, Json::Value &result)
{
  Value temp = "pong";
  result.swap(temp);
  return OK;
}

JSON_STATUS CJSONRPC::GetAnnouncementFlags(const CStdString &method, ITransportLayer *transport, IClient *client, const Json::Value& parameterObject, Json::Value &result)
{
  int flags = client->GetAnnouncementFlags();

  for (int i = 1; i <= ANNOUNCE_ALL; i *= 2)
    result[AnnouncementFlagToString((EAnnouncementFlag)i)] = (flags & i) > 0;

  return OK;
}

JSON_STATUS CJSONRPC::SetAnnouncementFlags(const CStdString &method, ITransportLayer *transport, IClient *client, const Json::Value& parameterObject, Json::Value &result)
{
  int flags = 0;

  if (parameterObject["Playback"].asBool())
    flags |= Playback;
  if (parameterObject["GUI"].asBool())
    flags |= GUI;
  if (parameterObject["System"].asBool())
    flags |= System;
  if (parameterObject["Library"].asBool())
    flags |= Library;
  if (parameterObject["Other"].asBool())
    flags |= Other;

  if (client->SetAnnouncementFlags(flags))
    return GetAnnouncementFlags(method, transport, client, parameterObject, result);

  return BadPermission;
}

JSON_STATUS CJSONRPC::Announce(const CStdString &method, ITransportLayer *transport, IClient *client, const Json::Value& parameterObject, Json::Value &result)
{
  if (parameterObject["data"].isNull())
    CAnnouncementManager::Announce(Other, parameterObject["sender"].asString().c_str(),  
      parameterObject["message"].asString().c_str());
  else
  {
    CVariant data(parameterObject["data"].asString());
    CAnnouncementManager::Announce(Other, parameterObject["sender"].asString().c_str(),  
      parameterObject["message"].asString().c_str(), data);
  }

  return ACK;
}

CStdString CJSONRPC::MethodCall(const CStdString &inputString, ITransportLayer *transport, IClient *client)
{
  Value inputroot, outputroot, result;

  Reader reader;
  bool hasResponse = false;

  if (reader.parse(inputString, inputroot))
  {
    if (inputroot.isArray())
    {
      if (inputroot.size() <= 0)
      {
        CLog::Log(LOGERROR, "JSONRPC: Empty batch call\n");
        BuildResponse(inputroot, InvalidRequest, Value(), outputroot);
        hasResponse = true;
      }
      else
      {
        for (unsigned int i = 0; i < inputroot.size(); i++)
        {
          Value request = inputroot.get(i, Value());
          Value response;
          if (HandleMethodCall(request, response, transport, client))
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
    BuildResponse(inputroot, ParseError, Value(), outputroot);
    hasResponse = true;
  }

  CStdString str;
  if (hasResponse)
  {
    Writer *writer;
    if (g_advancedSettings.m_jsonOutputCompact)
      writer = new FastWriter();
    else
      writer = new StyledWriter();

    str = writer->write(outputroot);
    delete writer;
  }
  return str;
}

bool CJSONRPC::HandleMethodCall(Value& request, Value& response, ITransportLayer *transport, IClient *client)
{
  JSON_STATUS errorCode = OK;
  Value result;
  bool isAnnouncement = false;

  if (IsProperJSONRPC(request))
  {
    isAnnouncement = !request.isMember("id");

    CStdString methodName = request.get("method", "").asString();
    methodName = methodName.ToLower();

    JSONRPC::MethodCall method;
    Json::Value params;

    if ((errorCode = CJSONServiceDescription::CheckCall(methodName, request["params"], client, isAnnouncement, method, params)) == OK)
      errorCode = method(methodName, transport, client, params, result);
    else
      result = params;
  }
  else
  {
    StyledWriter writer;
    CLog::Log(LOGERROR, "JSONRPC: Failed to parse '%s'\n", writer.write(request).c_str());
    errorCode = InvalidRequest;
  }

  BuildResponse(request, errorCode, result, response);

  return !isAnnouncement;
}

inline bool CJSONRPC::IsProperJSONRPC(const Json::Value& inputroot)
{
  return inputroot.isObject() && inputroot.isMember("jsonrpc") && inputroot["jsonrpc"].isString() && inputroot.get("jsonrpc", "-1").asString() == "2.0" && inputroot.isMember("method") && inputroot["method"].isString() && (!inputroot.isMember("params") || inputroot["params"].isArray() || inputroot["params"].isObject());
}

inline void CJSONRPC::BuildResponse(const Value& request, JSON_STATUS code, const Value& result, Value& response)
{
  response["jsonrpc"] = "2.0";
  response["id"] = request.isObject() && request.isMember("id") ? request["id"] : Value();

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
