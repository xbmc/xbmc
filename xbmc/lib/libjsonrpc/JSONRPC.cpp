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
#include "PlaylistOperations.h"
#include "FileOperations.h"
#include "GUIOperations.h"
#include "MusicLibrary.h"
#include "VideoLibrary.h"
#include "SystemOperations.h"
#include "XBMCOperations.h"
#include "AnnouncementManager.h"
#include "log.h"
#include <string.h>

using namespace ANNOUNCEMENT;
using namespace JSONRPC;
using namespace Json;
using namespace std;

Command CJSONRPC::m_commands[] = {
// JSON-RPC
  { "JSONRPC.Introspect",               CJSONRPC::Introspect,                   Response,     ReadData,        "Enumerates all actions and descriptions. Parameter example {\"getdescriptions\": true, \"getpermissions\": true, \"filterbytransport\": true }. All parameters optional" },
  { "JSONRPC.Version",                  CJSONRPC::Version,                      Response,     ReadData,        "Retrieve the jsonrpc protocol version" },
  { "JSONRPC.Permission",               CJSONRPC::Permission,                   Response,     ReadData,        "Retrieve the clients permissions" },
  { "JSONRPC.Ping",                     CJSONRPC::Ping,                         Response,     ReadData,        "Ping responder" },
  { "JSONRPC.GetAnnouncementFlags",     CJSONRPC::GetAnnouncementFlags,         Announcing,   ReadData,        "Get announcement flags" },
  { "JSONRPC.SetAnnouncementFlags",     CJSONRPC::SetAnnouncementFlags,         Announcing,   ControlAnnounce, "Change the announcement flags. Parameter example {\"playback\": true, \"gui\": false }" },
  { "JSONRPC.Announce",                 CJSONRPC::Announce,                     Response,     ReadData,        "Announce to other connected clients. Parameter example {\"sender\": \"foo\", \"message\": \"bar\", \"data\": \"somedata\" }. data is optional" },

// Player
  { "Player.GetActivePlayers",          CPlayerOperations::GetActivePlayers,    Response,     ReadData,        "Returns all active players IDs"},
  { "Player.PlayPause",                 CPlayerOperations::PlayPause,           Response,     ControlPlayback, "Pauses or unpause playback of highest prioritized player" },
  { "Player.Stop",                      CPlayerOperations::Stop,                Response,     ControlPlayback, "Stops playback of highest prioritized player" },
  { "Player.SkipPrevious",              CPlayerOperations::SkipPrevious,        Response,     ControlPlayback, "Skips to previous item on the playlist of highest prioritized player" },
  { "Player.SkipNext",                  CPlayerOperations::SkipNext,            Response,     ControlPlayback, "Skips to next item on the playlist of highest prioritized player" },

// Music player
  { "MusicPlayer.PlayPause",            CAVPlayerOperations::PlayPause,         Response,     ControlPlayback, "Pauses or unpause playback" },
  { "MusicPlayer.Stop",                 CAVPlayerOperations::Stop,              Response,     ControlPlayback, "Stops playback" },
  { "MusicPlayer.SkipPrevious",         CAVPlayerOperations::SkipPrevious,      Response,     ControlPlayback, "Skips to previous item on the playlist" },
  { "MusicPlayer.SkipNext",             CAVPlayerOperations::SkipNext,          Response,     ControlPlayback, "Skips to next item on the playlist" },

  { "MusicPlayer.BigSkipBackward",      CAVPlayerOperations::BigSkipBackward,   Response,     ControlPlayback, "" },
  { "MusicPlayer.BigSkipForward",       CAVPlayerOperations::BigSkipForward,    Response,     ControlPlayback, "" },
  { "MusicPlayer.SmallSkipBackward",    CAVPlayerOperations::SmallSkipBackward, Response,     ControlPlayback, "" },
  { "MusicPlayer.SmallSkipForward",     CAVPlayerOperations::SmallSkipForward,  Response,     ControlPlayback, "" },

  { "MusicPlayer.Rewind",               CAVPlayerOperations::Rewind,            Response,     ControlPlayback, "Rewind current playback" },
  { "MusicPlayer.Forward",              CAVPlayerOperations::Forward,           Response,     ControlPlayback, "Forward current playback" },

  { "MusicPlayer.GetTime",              CAVPlayerOperations::GetTime,           Response,     ReadData,        "Retrieve time" },
  { "MusicPlayer.GetTimeMS",            CAVPlayerOperations::GetTimeMS,         Response,     ReadData,        "Retrieve time in MS" },
  { "MusicPlayer.GetPercentage",        CAVPlayerOperations::GetPercentage,     Response,     ReadData,        "Retrieve percentage" },
  { "MusicPlayer.SeekTime",             CAVPlayerOperations::SeekTime,          Response,     ControlPlayback, "Seek to a specific time. Parameter integer in MS" },

  { "MusicPlayer.GetPlaylist",          CAVPlayerOperations::GetPlaylist,       Response,     ReadData,        "Retrieve active playlist" },

  { "MusicPlayer.Record",               CAVPlayerOperations::Record,            Response,     ControlPlayback, "" },

// Video player
  { "VideoPlayer.PlayPause",            CAVPlayerOperations::PlayPause,         Response,     ControlPlayback, "Pauses or unpause playback" },
  { "VideoPlayer.Stop",                 CAVPlayerOperations::Stop,              Response,     ControlPlayback, "Stops playback" },
  { "VideoPlayer.SkipPrevious",         CAVPlayerOperations::SkipPrevious,      Response,     ControlPlayback, "Skips to previous item on the playlist" },
  { "VideoPlayer.SkipNext",             CAVPlayerOperations::SkipNext,          Response,     ControlPlayback, "Skips to next item on the playlist" },

  { "VideoPlayer.BigSkipBackward",      CAVPlayerOperations::BigSkipBackward,   Response,     ControlPlayback, "" },
  { "VideoPlayer.BigSkipForward",       CAVPlayerOperations::BigSkipForward,    Response,     ControlPlayback, "" },
  { "VideoPlayer.SmallSkipBackward",    CAVPlayerOperations::SmallSkipBackward, Response,     ControlPlayback, "" },
  { "VideoPlayer.SmallSkipForward",     CAVPlayerOperations::SmallSkipForward,  Response,     ControlPlayback, "" },

  { "VideoPlayer.Rewind",               CAVPlayerOperations::Rewind,            Response,     ControlPlayback, "Rewind current playback" },
  { "VideoPlayer.Forward",              CAVPlayerOperations::Forward,           Response,     ControlPlayback, "Forward current playback" },

  { "VideoPlayer.GetTime",              CAVPlayerOperations::GetTime,           Response,     ReadData,        "Retrieve time" },
  { "VideoPlayer.GetTimeMS",            CAVPlayerOperations::GetTimeMS,         Response,     ReadData,        "Retrieve time in MS" },
  { "VideoPlayer.GetPercentage",        CAVPlayerOperations::GetPercentage,     Response,     ReadData,        "Retrieve percentage" },
  { "VideoPlayer.SeekTime",             CAVPlayerOperations::SeekTime,          Response,     ControlPlayback, "Seek to a specific time. Parameter integer in MS" },

  { "VideoPlayer.GetPlaylist",          CAVPlayerOperations::GetPlaylist,       Response,     ReadData,        "Retrieve active playlist" },

// Picture player
  { "PicturePlayer.PlayPause",          CPicturePlayerOperations::PlayPause,    Response,     ControlPlayback, "Pauses or unpause slideshow" },
  { "PicturePlayer.Stop",               CPicturePlayerOperations::Stop,         Response,     ControlPlayback, "Stops slideshow" },
  { "PicturePlayer.SkipPrevious",       CPicturePlayerOperations::SkipPrevious, Response,     ControlPlayback, "Skips to previous picture in the slideshow" },
  { "PicturePlayer.SkipNext",           CPicturePlayerOperations::SkipNext,     Response,     ControlPlayback, "Skips to next picture in the slideshow" },

  { "PicturePlayer.MoveLeft",           CPicturePlayerOperations::MoveLeft,     Response,     ControlPlayback, "If picture is zoomed move viewport left otherwise skip previous" },
  { "PicturePlayer.MoveRight",          CPicturePlayerOperations::MoveRight,    Response,     ControlPlayback, "If picture is zoomed move viewport right otherwise skip previous" },
  { "PicturePlayer.MoveDown",           CPicturePlayerOperations::MoveDown,     Response,     ControlPlayback, "If picture is zoomed move viewport down" },
  { "PicturePlayer.MoveUp",             CPicturePlayerOperations::MoveUp,       Response,     ControlPlayback, "If picture is zoomed move viewport up" },

  { "PicturePlayer.ZoomOut",            CPicturePlayerOperations::ZoomOut,      Response,     ControlPlayback, "Zoom out once" },
  { "PicturePlayer.ZoomIn",             CPicturePlayerOperations::ZoomIn,       Response,     ControlPlayback, "Zoom in once" },
  { "PicturePlayer.Zoom",               CPicturePlayerOperations::Zoom,         Response,     ControlPlayback, "Zooms current picture. Parameter integer of zoom level" },
  { "PicturePlayer.Rotate",             CPicturePlayerOperations::Rotate,       Response,     ControlPlayback, "Rotates current picture" },

// Playlist
  { "Playlist.GetItems",                CPlaylistOperations::GetItems,          Response,     ReadData,        "Retrieve items in the playlist. Parameter example {\"playlist\": \"music\" }. playlist optional." },
  { "Playlist.Add",                     CPlaylistOperations::Add,               Response,     ControlPlayback, "Add items to the playlist. Parameter example {\"playlist\": \"music\", \"file\": \"/foo/bar.mp3\" }. playlist optional." },
  { "Playlist.Remove",                  CPlaylistOperations::Remove,            Response,     ControlPlayback, "Remove items in the playlist. Parameter example {\"playlist\": \"music\", \"item\": 0 }. playlist optional." },
  { "Playlist.Swap",                    CPlaylistOperations::Swap,              Response,     ControlPlayback, "Swap items in the playlist. Parameter example {\"playlist\": \"music\", \"item1\": 0, \"item2\": 1 }. playlist optional." },
  { "Playlist.Shuffle",                 CPlaylistOperations::Shuffle,           Response,     ControlPlayback, "Shuffle playlist" },

// File
  { "Files.GetShares",                  CFileOperations::GetRootDirectory,      Response,     ReadData,        "Get the root directory of the media windows" },
  { "Files.Download",                   CFileOperations::Download,              FileDownload, ReadData,        "Specify a file to download to get info about how to download it, i.e a proper URL" },

  { "Files.GetDirectory",               CFileOperations::GetDirectory,          Response,     ReadData,        "Retrieve the specified directory" },

// Music library
  { "MusicLibrary.GetArtists",          CMusicLibrary::GetArtists,              Response,     ReadData,        "Retrieve all artists" },
  { "MusicLibrary.GetAlbums",           CMusicLibrary::GetAlbums,               Response,     ReadData,        "Retrieve all albums from specified artist or genre" },
  { "MusicLibrary.GetSongs",            CMusicLibrary::GetSongs,                Response,     ReadData,        "Retrieve all songs from specified album, artist or genre" },

  { "MusicLibrary.GetSongInfo",         CMusicLibrary::GetSongInfo,             Response,     ReadData,        "Retrieve the wanted info from the specified song" },

// Video library
  { "VideoLibrary.GetMovies",           CVideoLibrary::GetMovies,               Response,     ReadData,        "Retrieve all movies. Parameter example { \"fields\": [\"plot\"], \"sortmethod\": \"title\", \"sortorder\": \"ascending\", \"start\": 0, \"end\": 3}. fields, sortorder, sortmethod, start and end are optional" },

  { "VideoLibrary.GetTVShows",          CVideoLibrary::GetTVShows,              Response,     ReadData,        "Parameter example { \"fields\": [\"plot\"], \"sortmethod\": \"label\", \"sortorder\": \"ascending\", \"start\": 0, \"end\": 3}. sortorder, sortmethod, start and end are optional" },
  { "VideoLibrary.GetSeasons",          CVideoLibrary::GetSeasons,              Response,     ReadData,        "Parameter example { \"tvshowid\": 0, \"fields\": [\"season\"], \"sortmethod\": \"label\", \"sortorder\": \"ascending\", \"start\": 0, \"end\": 3}. sortorder, sortmethod, start and end are optional" },
  { "VideoLibrary.GetEpisodes",         CVideoLibrary::GetEpisodes,             Response,     ReadData,        "Parameter example { \"tvshowid\": 0, \"season\": 1, \"fields\": [\"plot\"], \"sortmethod\": \"episode\", \"sortorder\": \"ascending\", \"start\": 0, \"end\": 3}. sortorder, sortmethod, start and end are optional" },

  { "VideoLibrary.GetMusicVideoAlbums", CVideoLibrary::GetMusicVideoAlbums,     Response,     ReadData,        "" },
  { "VideoLibrary.GetMusicVideos",      CVideoLibrary::GetMusicVideos,          Response,     ReadData,        "Parameter example { \"artistid\": 0, \"albumid\": 0, \"fields\": [\"plot\"], \"sortmethod\": \"artistignorethe\", \"sortorder\": \"ascending\", \"start\": 0, \"end\": 3}. sortorder, sortmethod, start and end are optional" },

  { "VideoLibrary.GetMovieInfo",        CVideoLibrary::GetMovieInfo,            Response,     ReadData,        "Parameter example { \"movieid\": 0, \"fields\": [\"plot\"] }" },
  { "VideoLibrary.GetTVShowInfo",       CVideoLibrary::GetTVShowInfo,           Response,     ReadData,        "Parameter example { \"tvshowid\": 0, \"fields\": [\"plot\"] }" },
  { "VideoLibrary.GetEpisodeInfo",      CVideoLibrary::GetEpisodeInfo,          Response,     ReadData,        "Parameter example { \"episodeinfo\": 0, \"fields\": [\"plot\"] }" },
  { "VideoLibrary.GetMusicVideoInfo",   CVideoLibrary::GetMusicVideoInfo,       Response,     ReadData,        "Parameter example { \"musicvideoid\": 0, \"fields\": [\"plot\"] }" },

// GUI Operations
  { "GUI.GetLocalizedString",           CGUIOperations::GetLocalizedString,     Response,     ReadData,        "" },

// System operations
  { "System.Shutdown",                  CSystemOperations::Shutdown,            Response,     ControlPower,    "" },
  { "System.Suspend",                   CSystemOperations::Suspend,             Response,     ControlPower,    "" },
  { "System.Hibernate",                 CSystemOperations::Hibernate,           Response,     ControlPower,    "" },
  { "System.Reboot",                    CSystemOperations::Reboot,              Response,     ControlPower,    "" },

  { "System.GetInfo",                   CSystemOperations::GetInfo,             Response,     ReadData,        "Retrieve info about the system" },

// XBMC Operations
  { "XBMC.GetVolume",                   CXBMCOperations::GetVolume,             Response,     ReadData,        "Retrieve the current volume" },
  { "XBMC.SetVolume",                   CXBMCOperations::SetVolume,             Response,     ControlPlayback, "Set volume. Parameter integer between 0 amd 100" },
  { "XBMC.ToggleMute",                  CXBMCOperations::ToggleMute,            Response,     ControlPlayback, "Toggle mute" },

  { "XBMC.Play",                        CXBMCOperations::Play,                  Response,     ControlPlayback, "Starts playback" },
  { "XBMC.StartSlideshow",              CXBMCOperations::StartSlideshow,        Response,     ControlPlayback, "Starts slideshow. Parameter example {\"directory\": \"/foo/\", \"random\": true, \"recursive\": true} or just string to recursively and random run directory" },

  { "XBMC.Log",                         CXBMCOperations::Log,                   Response,     Logging,         "Logs a line in the xbmc.log. Parameter example {\"message\": \"foo\", \"level\": \"info\"} or just a string to log message with level debug" },

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
  
  for (int i = 1; i <= OPERATION_PERMISSION_ALL; i *= 2)
  {
    if (flags & i)
      result["permission"].append(PermissionToString((OperationPermission)(flags & i)));
  }

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
  {
    if (flags & i)
      result["permission"].append(AnnouncementFlagToString((EAnnouncementFlag)(flags & i)));
  }

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

  if (client->SetAnnouncementFlags(flags))
    return GetAnnouncementFlags(method, transport, client, parameterObject, result);

  return BadPermission;
}

JSON_STATUS CJSONRPC::Announce(const CStdString &method, ITransportLayer *transport, IClient *client, const Json::Value& parameterObject, Json::Value &result)
{
  if (!parameterObject.isObject() || !parameterObject.isMember("sender") || !parameterObject.isMember("message"))
    return InvalidParams;

  CAnnouncementManager::Announce(Other, parameterObject["sender"].asString().c_str(), parameterObject["message"].asString().c_str(), parameterObject.isMember("data") ? parameterObject["sender"].asString().c_str() : NULL);

  return ACK;
}

CStdString CJSONRPC::MethodCall(const CStdString &inputString, ITransportLayer *transport, IClient *client)
{
  Value inputroot, outputroot, result;

  JSON_STATUS errorCode = OK;
  Reader reader;

  if (reader.parse(inputString, inputroot) && IsProperJSONRPC(inputroot))
  {
    CStdString method = inputroot.get("method", "").asString();
    method = method.ToLower();
    errorCode = InternalMethodCall(method, inputroot, result, transport, client);
  }
  else
  {
    CLog::Log(LOGERROR, "JSONRPC: Failed to parse '%s'\n", inputString.c_str());
    errorCode = ParseError;
  }

  outputroot["jsonrpc"] = "2.0";
  outputroot["id"] = inputroot.get("id", 0);

  switch (errorCode)
  {
    case OK:
      outputroot["result"] = result;
      break;
    case ACK:
      outputroot["result"] = "OK";
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

inline bool CJSONRPC::IsProperJSONRPC(const Json::Value& inputroot)
{
  return inputroot.isObject() && inputroot.isMember("jsonrpc") && inputroot["jsonrpc"].isString() && inputroot.get("jsonrpc", "-1").asString() == "2.0" && inputroot.isMember("method") && inputroot["method"].isString() && inputroot.isMember("id");
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
  case Logging:
    return "Logging";
  default:
    return "Unkown";
  }
}

inline const char *CJSONRPC::AnnouncementFlagToString(const EAnnouncementFlag &announcement)
{
  switch (announcement)
  {
  case Playback:
    return "Playback";
  case GUI:
    return "GUI";
  case System:
    return "System";
  case Other:
    return "Other";
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
