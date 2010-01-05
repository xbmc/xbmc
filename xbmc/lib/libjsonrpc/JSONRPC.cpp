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

const JSON_ACTION commands[] = {
// JSON-RPC
  { "JSONRPC.Introspect",               CJSONRPC::Introspect,                   RO, "Enumerates all actions and descriptions" },
  { "JSONRPC.Version",                  CJSONRPC::Version,                      RO, "Retrieve the jsonrpc protocol version" },
  { "JSONRPC.Permission",               CJSONRPC::Permission,                   RO, "Retrieve the clients permissions" },
  { "JSONRPC.Ping",                     CJSONRPC::Ping,                         RO, "Ping responder" },

// Player
// Static methods
  { "Player.GetActivePlayers",          CPlayerActions::GetActivePlayers,       RO, "Returns all active players IDs"},
  { "Player.GetAvailablePlayers",       CPlayerActions::GetAvailablePlayers,    RO, "Returns all active players IDs"},
// Object methods
  { "Player.PlayPause",                 CPlayerActions::PlayPause,              RW, "Pauses or unpause playback" },
  { "Player.Stop",                      CPlayerActions::Stop,                   RW, "Stops playback" },
  { "Player.SkipPrevious",              CPlayerActions::SkipPrevious,           RW, "Skips to previous item on the playlist" },
  { "Player.SkipNext",                  CPlayerActions::SkipNext,               RW, "Skips to next item on the playlist" },

  { "Player.BigSkipBackward",           CPlayerActions::BigSkipBackward,        RW, "" },
  { "Player.BigSkipForward",            CPlayerActions::BigSkipForward,         RW, "" },
  { "Player.SmallSkipBackward",         CPlayerActions::SmallSkipBackward,      RW, "" },
  { "Player.SmallSkipForward",          CPlayerActions::SmallSkipForward,       RW, "" },

  { "Player.Rewind",                    CPlayerActions::Rewind,                 RW, "Rewind current playback" },
  { "Player.Forward",                   CPlayerActions::Forward,                RW, "Forward current playback" },

  { "Player.Record",                    CPlayerActions::Record,                 RW, "" },

// File
// Static methods
  { "Files.GetShares",                  CFileActions::GetRootDirectory,         RO, "Get the root directory of the media windows" },
  { "Files.Download",                   CFileActions::Download,                 RO, "Specify a file to download to get info about how to download it, i.e a proper URL" },
// Object methods
  { "Files.GetDirectory",               CFileActions::GetDirectory,             RO, "Retrieve the specified directory" },

// Music library
// Static/Object methods
  { "MusicLibrary.GetArtists",          CMusicLibrary::GetArtists,              RO, "Retrieve all artists" },
  { "MusicLibrary.GetAlbums",           CMusicLibrary::GetAlbums,               RO, "Retrieve all albums from specified artist or genre" },
  { "MusicLibrary.GetSongs",            CMusicLibrary::GetSongs,                RO, "Retrieve all songs from specified album, artist or genre" },
// Object methods
  { "MusicLibrary.GetSongInfo",         CMusicLibrary::GetSongInfo,             RO, "Retrieve the wanted info from the specified song" },

// Video library
  { "VideoLibrary.GetMovies",           CVideoLibrary::GetMovies,               RO, "Retrieve all movies" },

  { "VideoLibrary.GetTVShows",          CVideoLibrary::GetTVShows,              RO, "" },
  { "VideoLibrary.GetSeasons",          CVideoLibrary::GetSeasons,              RO, "" },
  { "VideoLibrary.GetEpisodes",         CVideoLibrary::GetEpisodes,             RO, "" },

  { "VideoLibrary.GetMusicVideoAlbums", CVideoLibrary::GetMusicVideoAlbums,     RO, "" },
  { "VideoLibrary.GetMusicVideos",      CVideoLibrary::GetMusicVideos,          RO, "" },
// Object methods
  { "VideoLibrary.GetMovieInfo",        CVideoLibrary::GetMovieInfo,            RO, "" },
  { "VideoLibrary.GetTVShowInfo",       CVideoLibrary::GetTVShowInfo,           RO, "" },
  { "VideoLibrary.GetEpisodeInfo",      CVideoLibrary::GetEpisodeInfo,          RO, "" },
  { "VideoLibrary.GetMusicVideoInfo",   CVideoLibrary::GetMusicVideoInfo,       RO, "" }

/* Planned features
"XBMC.PlayMedia" - Should take movieid, tvshowid and so on. also filepath
System.Suspend and such
*/


};

using namespace Json;
using namespace std;

#define ALLOWEDPERMISSION RW

ActionMap CJSONRPC::m_actionMap;

JSON_STATUS CJSONRPC::Introspect(const CStdString &method, const Value& parameterObject, Value &result)
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
      val["permission"] = commands[i].permission == RW ? "RW" : "RO";

    result["commands"].append(val);
  }

  return OK;
}

JSON_STATUS CJSONRPC::Version(const CStdString &method, const Value& parameterObject, Value &result)
{
  result["version"] = 1;

  return OK;
}

JSON_STATUS CJSONRPC::Permission(const CStdString &method, const Value& parameterObject, Value &result)
{
  result["permission"] = (ALLOWEDPERMISSION == RW ? "RW" : "RO");

  return OK;
}

JSON_STATUS CJSONRPC::Ping(const CStdString &method, const Value& parameterObject, Value &result)
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

CStdString CJSONRPC::MethodCall(const CStdString &inputString)
{
  Value inputroot, outputroot, result;

  JSON_STATUS errorCode = OK;
  Reader reader;

  if (reader.parse(inputString, inputroot) && inputroot.get("jsonrpc", "-1") == "2.0" && inputroot.isMember("method") && inputroot.isMember("id"))
  {
    CStdString method = inputroot.get("method", "").asString();
    method = method.ToLower();
    errorCode = InternalMethodCall(method, inputroot, result);
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

JSON_STATUS CJSONRPC::InternalMethodCall(const CStdString& method, Value& o, Value &result)
{
  ActionMap::iterator iter = m_actionMap.find(method);
  if( iter != m_actionMap.end() )
  {
    if (iter->second.permission == RO || iter->second.permission == ALLOWEDPERMISSION)
      return iter->second.method(method, o["params"], result);
    else
      return BadPermission;
  }
  else
    return MethodNotFound;
}
