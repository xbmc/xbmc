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
#include "FileActions.h"
#include "MusicLibrary.h"
#include "VideoLibrary.h"
#include <string.h>

using namespace JSONRPC;

const JSON_ACTION commands[] = {
// JSON-RPC
  { "JSONRPC.Introspect",               CJSONRPC::Introspect,                   ReadData,        "Enumerates all actions and descriptions" },
  { "JSONRPC.Version",                  CJSONRPC::Version,                      ReadData,        "Retrieve the jsonrpc protocol version" },
  { "JSONRPC.Permission",               CJSONRPC::Permission,                   ReadData,        "Retrieve the clients permissions" },
  { "JSONRPC.Ping",                     CJSONRPC::Ping,                         ReadData,        "Ping responder" },

// Player
// Static methods
  { "Player.GetActivePlayers",          CPlayerActions::GetActivePlayers,       ReadData,        "Returns all active players IDs"},
  { "Player.GetAvailablePlayers",       CPlayerActions::GetAvailablePlayers,    ReadData,        "Returns all active players IDs"},
// Object methods
  { "Player.PlayPause",                 CPlayerActions::PlayPause,              ControlPlayback, "Pauses or unpause playback" },
  { "Player.Stop",                      CPlayerActions::Stop,                   ControlPlayback, "Stops playback" },
  { "Player.SkipPrevious",              CPlayerActions::SkipPrevious,           ControlPlayback, "Skips to previous item on the playlist" },
  { "Player.SkipNext",                  CPlayerActions::SkipNext,               ControlPlayback, "Skips to next item on the playlist" },

  { "Player.BigSkipBackward",           CPlayerActions::BigSkipBackward,        ControlPlayback, "" },
  { "Player.BigSkipForward",            CPlayerActions::BigSkipForward,         ControlPlayback, "" },
  { "Player.SmallSkipBackward",         CPlayerActions::SmallSkipBackward,      ControlPlayback, "" },
  { "Player.SmallSkipForward",          CPlayerActions::SmallSkipForward,       ControlPlayback, "" },

  { "Player.Rewind",                    CPlayerActions::Rewind,                 ControlPlayback, "Rewind current playback" },
  { "Player.Forward",                   CPlayerActions::Forward,                ControlPlayback, "Forward current playback" },

  { "Player.Record",                    CPlayerActions::Record,                 ControlPlayback, "" },

// File
// Static methods
  { "Files.GetShares",                  CFileActions::GetRootDirectory,         ReadData,        "Get the root directory of the media windows" },
  { "Files.Download",                   CFileActions::Download,                 ReadData,        "Specify a file to download to get info about how to download it, i.e a proper URL" },
// Object methods
  { "Files.GetDirectory",               CFileActions::GetDirectory,             ReadData,        "Retrieve the specified directory" },

// Music library
// Static/Object methods
  { "MusicLibrary.GetArtists",          CMusicLibrary::GetArtists,              ReadData,        "Retrieve all artists" },
  { "MusicLibrary.GetAlbums",           CMusicLibrary::GetAlbums,               ReadData,        "Retrieve all albums from specified artist or genre" },
  { "MusicLibrary.GetSongs",            CMusicLibrary::GetSongs,                ReadData,        "Retrieve all songs from specified album, artist or genre" },
// Object methods
  { "MusicLibrary.GetSongInfo",         CMusicLibrary::GetSongInfo,             ReadData,        "Retrieve the wanted info from the specified song" },

// Video library
  { "VideoLibrary.GetMovies",           CVideoLibrary::GetMovies,               ReadData,        "Retrieve all movies" },

  { "VideoLibrary.GetTVShows",          CVideoLibrary::GetTVShows,              ReadData,        "" },
  { "VideoLibrary.GetSeasons",          CVideoLibrary::GetSeasons,              ReadData,        "" },
  { "VideoLibrary.GetEpisodes",         CVideoLibrary::GetEpisodes,             ReadData,        "" },

  { "VideoLibrary.GetMusicVideoAlbums", CVideoLibrary::GetMusicVideoAlbums,     ReadData,        "" },
  { "VideoLibrary.GetMusicVideos",      CVideoLibrary::GetMusicVideos,          ReadData,        "" },
// Object methods
  { "VideoLibrary.GetMovieInfo",        CVideoLibrary::GetMovieInfo,            ReadData,        "" },
  { "VideoLibrary.GetTVShowInfo",       CVideoLibrary::GetTVShowInfo,           ReadData,        "" },
  { "VideoLibrary.GetEpisodeInfo",      CVideoLibrary::GetEpisodeInfo,          ReadData,        "" },
  { "VideoLibrary.GetMusicVideoInfo",   CVideoLibrary::GetMusicVideoInfo,       ReadData,        "" }

/* Planned features
"XBMC.PlayMedia" - Should take movieid, tvshowid and so on. also filepath
System.Suspend and such
*/


};

using namespace Json;
using namespace std;

#define ALLOWEDPERMISSION RW

ActionMap CJSONRPC::m_actionMap;

JSON_STATUS CJSONRPC::Introspect(const CStdString &method, ITransportLayer *transport, const Json::Value& parameterObject, Json::Value &result)
{
  bool getCommands = parameterObject.get("getcommands", true).asBool();
  bool getDescriptions = parameterObject.get("getdescriptions", true).asBool();
  bool getPermissions = parameterObject.get("getpermissions", true).asBool();

  int length = sizeof(commands)/sizeof(JSON_ACTION);
  for (int i = 0; i < length; i++)
  {
    Value val;

    if (getCommands)
      val["command"] = commands[i].command;
    if (getDescriptions)
      val["description"] = commands[i].description;
    if (getPermissions)
      val["permission"] = commands[i].permission == ReadData ? "ReadData" : "ControlPlayback";

    result["commands"].append(val);
  }

  return OK;
}

JSON_STATUS CJSONRPC::Version(const CStdString &method, ITransportLayer *transport, const Json::Value& parameterObject, Json::Value &result)
{
  result["version"] = 1;

  return OK;
}

JSON_STATUS CJSONRPC::Permission(const CStdString &method, ITransportLayer *transport, const Json::Value& parameterObject, Json::Value &result)
{
/*  int flags = client->GetPermissionFlags();
  if (flags & ReadData)
    result["permission"].append("ReadData");
  if (flags & ControlPlayback)
    result["permission"].append("ControlPlayback");*/

  return OK;
}

JSON_STATUS CJSONRPC::Ping(const CStdString &method, ITransportLayer *transport, const Json::Value& parameterObject, Json::Value &result)
{
  Value temp = "pong";
  result.swap(temp);
  return OK;
}

void CJSONRPC::Initialize()
{
  int length = sizeof(commands)/sizeof(JSON_ACTION);
  for (int i = 0; i < length; i++)
  {
    CStdString command = commands[i].command;
    m_actionMap[command.ToLower()] = commands[i];
  }
}

CStdString CJSONRPC::MethodCall(const CStdString &inputString, ITransportLayer *transport, IClient *client)
{
  Value inputroot, outputroot, result;

  JSON_STATUS errorCode = OK;
  Reader reader;

  if (reader.parse(inputString, inputroot) && inputroot.get("jsonrpc", "-1") == "2.0" && inputroot.isMember("method") && inputroot.isMember("id"))
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
      outputroot["error"]["message"] = "Server error.";
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
  ActionMap::iterator iter = m_actionMap.find(method);
  if( iter != m_actionMap.end() )
  {
    if (iter->second.permission & client->GetPermissionFlags())
      return iter->second.method(method, transport, o["params"], result);
    else
      return BadPermission;
  }
  else
    return MethodNotFound;
}
