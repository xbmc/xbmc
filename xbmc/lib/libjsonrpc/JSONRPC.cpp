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
#include "PlayerActions.h"
#include "PlaylistOperations.h"
#include "FileActions.h"
#include "MusicLibrary.h"
#include "VideoLibrary.h"
#include "SystemOperations.h"
#include "XBMCOperations.h"
#include "AnnouncementManager.h"
#include <string.h>

using namespace ANNOUNCEMENT;
using namespace JSONRPC;
using namespace Json;
using namespace std;


Command CJSONRPC::m_commands[] = {
// JSON-RPC
  { "JSONRPC.Introspect",               CJSONRPC::Introspect,                   Response,     ReadData,        "Enumerates all actions and descriptions" },
  { "JSONRPC.Version",                  CJSONRPC::Version,                      Response,     ReadData,        "Retrieve the jsonrpc protocol version" },
  { "JSONRPC.Permission",               CJSONRPC::Permission,                   Response,     ReadData,        "Retrieve the clients permissions" },
  { "JSONRPC.Ping",                     CJSONRPC::Ping,                         Response,     ReadData,        "Ping responder" },
  { "JSONRPC.SetAnnouncementFlags",     CJSONRPC::SetAnnouncementFlags,         Announcing,   ControlAnnounce, "Change the announcement flags" },
  { "JSONRPC.Announce",                 CJSONRPC::Announce,                     Response,     ReadData,        "Announce to other connected clients" },

// Player
// Static methods
  { "Player.GetActivePlayers",          CPlayerActions::GetActivePlayers,       Response,     ReadData,        "Returns all active players IDs"},
// Music player
  { "MusicPlayer.PlayPause",            CPlayerActions::PlayPause,              Response,     ControlPlayback, "Pauses or unpause playback" },
  { "MusicPlayer.Stop",                 CPlayerActions::Stop,                   Response,     ControlPlayback, "Stops playback" },
  { "MusicPlayer.SkipPrevious",         CPlayerActions::SkipPrevious,           Response,     ControlPlayback, "Skips to previous item on the playlist" },
  { "MusicPlayer.SkipNext",             CPlayerActions::SkipNext,               Response,     ControlPlayback, "Skips to next item on the playlist" },

  { "MusicPlayer.BigSkipBackward",      CPlayerActions::BigSkipBackward,        Response,     ControlPlayback, "" },
  { "MusicPlayer.BigSkipForward",       CPlayerActions::BigSkipForward,         Response,     ControlPlayback, "" },
  { "MusicPlayer.SmallSkipBackward",    CPlayerActions::SmallSkipBackward,      Response,     ControlPlayback, "" },
  { "MusicPlayer.SmallSkipForward",     CPlayerActions::SmallSkipForward,       Response,     ControlPlayback, "" },

  { "MusicPlayer.Rewind",               CPlayerActions::Rewind,                 Response,     ControlPlayback, "Rewind current playback" },
  { "MusicPlayer.Forward",              CPlayerActions::Forward,                Response,     ControlPlayback, "Forward current playback" },

  { "MusicPlayer.GetTime",              CPlayerActions::GetTime,                Response,     ReadData,        "Retrieve time" },
  { "MusicPlayer.GetTimeMS",            CPlayerActions::GetTimeMS,              Response,     ReadData,        "Retrieve time in MS" },
  { "MusicPlayer.GetPercentage",        CPlayerActions::GetPercentage,          Response,     ReadData,        "Retrieve percentage" },
  { "MusicPlayer.SeekTime",             CPlayerActions::SeekTime,               Response,     ControlPlayback, "Seek to a specific time" },

  { "MusicPlayer.GetPlaylist",          CPlayerActions::GetPlaylist,            Response,     ReadData,        "Retrieve active playlist" },

  { "MusicPlayer.Record",               CPlayerActions::Record,                 Response,     ControlPlayback, "" },
// Video player
  { "VideoPlayer.PlayPause",            CPlayerActions::PlayPause,              Response,     ControlPlayback, "Pauses or unpause playback" },
  { "VideoPlayer.Stop",                 CPlayerActions::Stop,                   Response,     ControlPlayback, "Stops playback" },
  { "VideoPlayer.SkipPrevious",         CPlayerActions::SkipPrevious,           Response,     ControlPlayback, "Skips to previous item on the playlist" },
  { "VideoPlayer.SkipNext",             CPlayerActions::SkipNext,               Response,     ControlPlayback, "Skips to next item on the playlist" },

  { "VideoPlayer.BigSkipBackward",      CPlayerActions::BigSkipBackward,        Response,     ControlPlayback, "" },
  { "VideoPlayer.BigSkipForward",       CPlayerActions::BigSkipForward,         Response,     ControlPlayback, "" },
  { "VideoPlayer.SmallSkipBackward",    CPlayerActions::SmallSkipBackward,      Response,     ControlPlayback, "" },
  { "VideoPlayer.SmallSkipForward",     CPlayerActions::SmallSkipForward,       Response,     ControlPlayback, "" },

  { "VideoPlayer.Rewind",               CPlayerActions::Rewind,                 Response,     ControlPlayback, "Rewind current playback" },
  { "VideoPlayer.Forward",              CPlayerActions::Forward,                Response,     ControlPlayback, "Forward current playback" },

  { "VideoPlayer.GetTime",              CPlayerActions::GetTime,                Response,     ReadData,        "Retrieve time" },
  { "VideoPlayer.GetTimeMS",            CPlayerActions::GetTimeMS,              Response,     ReadData,        "Retrieve time in MS" },
  { "VideoPlayer.GetPercentage",        CPlayerActions::GetPercentage,          Response,     ReadData,        "Retrieve percentage" },
  { "VideoPlayer.SeekTime",             CPlayerActions::SeekTime,               Response,     ControlPlayback, "Seek to a specific time" },

  { "VideoPlayer.GetPlaylist",          CPlayerActions::GetPlaylist,            Response,     ReadData,        "Retrieve active playlist" },

// Playlist
  { "Playlist.GetItems",                CPlaylistOperations::GetItems,          Response,     ReadData,         "Retrieve items in the playlist" },
  { "Playlist.Add",                     CPlaylistOperations::Add,               Response,     ControlPlayback,  "Add items to the playlist" },
  { "Playlist.Remove",                  CPlaylistOperations::Remove,            Response,     ControlPlayback,  "Remove items in the playlist" },
  { "Playlist.Swap",                    CPlaylistOperations::Swap,              Response,     ControlPlayback,  "Swap items in the playlist" },
  { "Playlist.Shuffle",                 CPlaylistOperations::Shuffle,           Response,     ControlPlayback,  "Shuffle playlist" },

// File
// Static methods
  { "Files.GetShares",                  CFileActions::GetRootDirectory,         Response,     ReadData,        "Get the root directory of the media windows" },
  { "Files.Download",                   CFileActions::Download,                 FileDownload, ReadData,        "Specify a file to download to get info about how to download it, i.e a proper URL" },
// Object methods
  { "Files.GetDirectory",               CFileActions::GetDirectory,             Response,     ReadData,        "Retrieve the specified directory" },

// Music library
// Static/Object methods
  { "MusicLibrary.GetArtists",          CMusicLibrary::GetArtists,              Response,     ReadData,        "Retrieve all artists" },
  { "MusicLibrary.GetAlbums",           CMusicLibrary::GetAlbums,               Response,     ReadData,        "Retrieve all albums from specified artist or genre" },
  { "MusicLibrary.GetSongs",            CMusicLibrary::GetSongs,                Response,     ReadData,        "Retrieve all songs from specified album, artist or genre" },
// Object methods
  { "MusicLibrary.GetSongInfo",         CMusicLibrary::GetSongInfo,             Response,     ReadData,        "Retrieve the wanted info from the specified song" },

// Video library
  { "VideoLibrary.GetMovies",           CVideoLibrary::GetMovies,               Response,     ReadData,        "Retrieve all movies" },

  { "VideoLibrary.GetTVShows",          CVideoLibrary::GetTVShows,              Response,     ReadData,        "" },
  { "VideoLibrary.GetSeasons",          CVideoLibrary::GetSeasons,              Response,     ReadData,        "" },
  { "VideoLibrary.GetEpisodes",         CVideoLibrary::GetEpisodes,             Response,     ReadData,        "" },

  { "VideoLibrary.GetMusicVideoAlbums", CVideoLibrary::GetMusicVideoAlbums,     Response,     ReadData,        "" },
  { "VideoLibrary.GetMusicVideos",      CVideoLibrary::GetMusicVideos,          Response,     ReadData,        "" },
// Object methods
  { "VideoLibrary.GetMovieInfo",        CVideoLibrary::GetMovieInfo,            Response,     ReadData,        "" },
  { "VideoLibrary.GetTVShowInfo",       CVideoLibrary::GetTVShowInfo,           Response,     ReadData,        "" },
  { "VideoLibrary.GetEpisodeInfo",      CVideoLibrary::GetEpisodeInfo,          Response,     ReadData,        "" },
  { "VideoLibrary.GetMusicVideoInfo",   CVideoLibrary::GetMusicVideoInfo,       Response,     ReadData,        "" },

// System operations
  { "System.Shutdown",                  CSystemOperations::Shutdown,            Response,     ControlPower,    "" },
  { "System.Suspend",                   CSystemOperations::Suspend,             Response,     ControlPower,    "" },
  { "System.Hibernate",                 CSystemOperations::Hibernate,           Response,     ControlPower,    "" },
  { "System.Reboot",                    CSystemOperations::Reboot,              Response,     ControlPower,    "" },

// XBMC Operations
  { "XBMC.GetVolume",                   CXBMCOperations::GetVolume,             Response,     ReadData,        "Retrieve the current volume" },
  { "XBMC.SetVolume",                   CXBMCOperations::SetVolume,             Response,     ControlPlayback, "Set volume" },
  { "XBMC.ToggleMute",                  CXBMCOperations::ToggleMute,            Response,     ControlPlayback, "Toggle mute" },

  { "XBMC.Play",                        CXBMCOperations::Play,                  Response,     ControlPlayback, "Starts playback" },
  { "XBMC.StartSlideshow",              CXBMCOperations::StartSlideshow,        Response,     ControlPlayback, "Starts slideshow" },

  { "XBMC.Quit",                        CXBMCOperations::Quit,                  Response,     ControlPower,    "Quit xbmc" }
};

CJSONRPC::CActionMap CJSONRPC::m_actionMap(m_commands, sizeof(m_commands) / sizeof(m_commands[0]) );

JSON_STATUS CJSONRPC::Introspect(const CStdString &method, ITransportLayer *transport, IClient *client, const Json::Value& parameterObject, Json::Value &result)
{
  if (!(parameterObject.isObject() || parameterObject.isNull()))
    return InvalidParams;

  const Value param = parameterObject.isObject() ? parameterObject : Value(objectValue);
  bool getDescriptions = param.get("getdescriptions", true).asBool();
  bool getPermissions = param.get("getpermissions", true).asBool();
  bool filterByTransport = param.get("filterbytransport", true).asBool();

  int length = sizeof(m_commands) / sizeof(Command);
  int clientflags = client->GetPermissionFlags();
  for (int i = 0; i < length; i++)
  {
    if ((transport->GetCapabilities() & m_commands[i].transportneed) == 0 && filterByTransport)
      continue;

    Value val;

    val["command"] = m_commands[i].command;
    val["executable"] = (clientflags & m_commands[i].permission) > 0 ? true : false;
    if (getDescriptions && m_commands[i].description)
      val["description"] = m_commands[i].description;
    if (getPermissions)
      val["permission"] = PermissionToString(m_commands[i].permission);

    result["commands"].append(val);
  }

  return OK;
}

JSON_STATUS CJSONRPC::Version(const CStdString &method, ITransportLayer *transport, IClient *client, const Json::Value& parameterObject, Json::Value &result)
{
  result["version"] = 1;

  return OK;
}

JSON_STATUS CJSONRPC::Permission(const CStdString &method, ITransportLayer *transport, IClient *client, const Json::Value& parameterObject, Json::Value &result)
{
  int flags = client->GetPermissionFlags();
  if (flags & ReadData)
    result["permission"].append("ReadData");
  if (flags & ControlPlayback)
    result["permission"].append("ControlPlayback");
  if (flags & ControlAnnounce)
    result["permission"].append("ControlAnnounce");
  if (flags & ControlPower)
    result["permission"].append("ControlPower");

  return OK;
}

JSON_STATUS CJSONRPC::Ping(const CStdString &method, ITransportLayer *transport, IClient *client, const Json::Value& parameterObject, Json::Value &result)
{
  Value temp = "pong";
  result.swap(temp);
  return OK;
}

JSON_STATUS CJSONRPC::SetAnnouncementFlags(const CStdString &method, ITransportLayer *transport, IClient *client, const Json::Value& parameterObject, Json::Value &result)
{
  if (!parameterObject.isObject())
    return InvalidParams;

  int flags = 0;

  if (parameterObject.get("playback", false).asBool())
    flags |= Playback;
  else if (parameterObject.get("gui", false).asBool())
    flags |= GUI;
  else if (parameterObject.get("system", false).asBool())
    flags |= System;
  else if (parameterObject.get("other", false).asBool())
    flags |= Other;

  return client->SetAnnouncementFlags(flags) ? OK : BadPermission;
}

JSON_STATUS CJSONRPC::Announce(const CStdString &method, ITransportLayer *transport, IClient *client, const Json::Value& parameterObject, Json::Value &result)
{
  if (!parameterObject.isObject() || !parameterObject.isMember("sender") || !parameterObject.isMember("message"))
    return InvalidParams;

  CAnnouncementManager::Announce(Other, parameterObject["sender"].asString().c_str(), parameterObject["message"].asString().c_str(), parameterObject.isMember("data") ? parameterObject["sender"].asString().c_str() : NULL);

  Value temp = "OK";
  result.swap(temp);
  return OK;
}

CStdString CJSONRPC::MethodCall(const CStdString &inputString, ITransportLayer *transport, IClient *client)
{
  Value inputroot, outputroot, result;

  JSON_STATUS errorCode = OK;
  Reader reader;

  if (reader.parse(inputString, inputroot) && inputroot.isObject() && inputroot.get("jsonrpc", "-1").asString() == "2.0" && inputroot.isMember("method") && inputroot.isMember("id"))
  {
    CStdString method = inputroot.get("method", "").asString();
    method = method.ToLower();
    errorCode = InternalMethodCall(method, inputroot, result, transport, client);
  }
  else
    errorCode = ParseError;

  outputroot["jsonrpc"] = "2.0";
  outputroot["id"] = inputroot.get("id", 0);

  switch (errorCode)
  {
    case OK:
      outputroot["result"] = result;
      break;
    case InvalidParams:
      outputroot["error"]["code"] = InvalidParams;
      outputroot["error"]["message"] = "Invalid params.";
      break;
    case MethodNotFound:
      outputroot["error"]["code"] = MethodNotFound;
      outputroot["error"]["message"] = "Method not found.";
      break;
    case ParseError:
      outputroot["error"]["code"] = ParseError;
      outputroot["error"]["message"] = "Parse error.";
      break;
    case BadPermission:
      outputroot["error"]["code"] = BadPermission;
      outputroot["error"]["message"] = "Bad client permission.";
      break;
    case FailedToExecute:
      outputroot["error"]["code"] = FailedToExecute;
      outputroot["error"]["message"] = "Failed to execute method.";
      break;
    default:
      outputroot["error"]["code"] = InternalError;
      outputroot["error"]["message"] = "Internal error.";
      break;
  }

  StyledWriter writer;
  CStdString str = writer.write(outputroot);
  return str;
}

JSON_STATUS CJSONRPC::InternalMethodCall(const CStdString& method, Value& o, Value &result, ITransportLayer *transport, IClient *client)
{
  CActionMap::const_iterator iter = m_actionMap.find(method);
  if( iter != m_actionMap.end() )
  {
    if (client->GetPermissionFlags() & iter->second.permission)
      return iter->second.method(method, transport, client, o["params"], result);
    else
      return BadPermission;
  }
  else
    return MethodNotFound;
}

inline const char *CJSONRPC::PermissionToString(const OperationPermission &permission)
{
  switch (permission)
  {
  case ReadData:
    return "ReadData";
  case ControlPlayback:
    return "ControlPlayback";
  case ControlAnnounce:
    return "ControlAnnounce";
  case ControlPower:
    return "ControlPower";
  default:
    return "Unkown";
  }
}

CJSONRPC::CActionMap::CActionMap(const Command commands[], int length)
{
  for (int i = 0; i < length; i++)
  {
    CStdString command = commands[i].command;
    command = command.ToLower();
    m_actionmap[command] = commands[i];
  }
}

CJSONRPC::CActionMap::const_iterator CJSONRPC::CActionMap::find(const CStdString& key) const
{
  return m_actionmap.find(key);
}

CJSONRPC::CActionMap::const_iterator CJSONRPC::CActionMap::end() const
{
  return m_actionmap.end();
}
