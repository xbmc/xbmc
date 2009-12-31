#include "JSONRPC.h"
#include "PlayerActions.h"
#include "FileActions.h"
#include "MusicLibrary.h"
#include "VideoLibrary.h"
#include <string.h>

const JSON_ACTION commands[] = {
// JSON-RPC
  { "JSONRPC.Introspect",               MustHaveParameters, CJSONRPC::Introspect,                   "Enumerates all actions and descriptions" },
  { "JSONRPC.Version",                  NoParameters,       CJSONRPC::Version,                      "Retrieve the jsonrpc protocol version" },

// Player
// Static methods
  { "Player.GetActivePlayers",          NoParameters,       CPlayerActions::GetActivePlayers,       "Returns all active players IDs"},
// Object methods
  { "Player.Pause",                     MustHaveParameters, CPlayerActions::Pause,                  "Pauses playback" },
  { "Player.SkipNext",                  MustHaveParameters, CPlayerActions::SkipNext,               "Skips choosen player to next item on the playlist" },
  { "Player.SkipPrevious",              MustHaveParameters, CPlayerActions::SkipPrevious,           "Skips choosen player to previous item on the playlist" },

// File
// Static methods
  { "Files.GetShares",                  MustHaveParameters, CFileActions::GetRootDirectory,         "Get the root directory of the media windows" },
  { "Files.Download",                   MustHaveParameters, CFileActions::Download,                 "Specify a file to download to get info about how to download it, i.e a proper URL" },
// Object methods
  { "Files.GetDirectory",               MustHaveParameters, CFileActions::GetDirectory,             "Retrieve the specified directory" },

// Music library
// Static/Object methods
  { "MusicLibrary.GetArtists",          MayHaveParameters,  CMusicLibrary::GetArtists,              "Retrieve all artists" },
  { "MusicLibrary.GetAlbums",           MayHaveParameters,  CMusicLibrary::GetAlbums,               "Retrieve all albums from specified artist or genre" },
  { "MusicLibrary.GetSongs",            MayHaveParameters,  CMusicLibrary::GetSongs,                "Retrieve all songs from specified album, artist or genre" },
// Object methods
  { "MusicLibrary.GetSongInfo",         MustHaveParameters, CMusicLibrary::GetSongInfo,             "Retrieve the wanted info from the specified song" },

// Video library
  { "VideoLibrary.GetMovies",           MayHaveParameters,  CVideoLibrary::GetMovies,               "Retrieve all movies" },

  { "VideoLibrary.GetTVShows",          MayHaveParameters,  CVideoLibrary::GetTVShows,              "" },
  { "VideoLibrary.GetSeasons",          MustHaveParameters, CVideoLibrary::GetSeasons,              "" },
  { "VideoLibrary.GetEpisodes",         MustHaveParameters, CVideoLibrary::GetEpisodes,             "" },

  { "VideoLibrary.GetMusicVideoAlbums", MayHaveParameters,  CVideoLibrary::GetMusicVideoAlbums,     "" },
  { "VideoLibrary.GetMusicVideos",      MayHaveParameters,  CVideoLibrary::GetMusicVideos,          "" },
// Object methods
  { "VideoLibrary.GetMovieInfo",        MustHaveParameters, CVideoLibrary::GetMovieInfo,            "" },
  { "VideoLibrary.GetTVShowInfo",       MustHaveParameters, CVideoLibrary::GetTVShowInfo,           "" },
  { "VideoLibrary.GetEpisodeInfo",      MustHaveParameters, CVideoLibrary::GetEpisodeInfo,          "" },
  { "VideoLibrary.GetMusicVideoInfo",   MustHaveParameters, CVideoLibrary::GetMusicVideoInfo,       "" }

/* Planned features
"XBMC.PlayMedia" - Should take movieid, tvshowid and so on. also filepath
System.Suspend and such
*/


};

using namespace Json;
using namespace std;

ActionMap CJSONRPC::m_actionMap;

JSON_STATUS CJSONRPC::Introspect(const CStdString &method, const Value& parameterObject, Value &result)
{
  bool getCommands = parameterObject.get("getcommands", true).asBool();
  bool getDescriptions = parameterObject.get("getdescriptions", true).asBool();

  int length = sizeof(commands)/sizeof(JSON_ACTION);
  for (int i = 0; i < length; i++)
  {
    if (getCommands)
      result["commands"].append(commands[i].command);
    if (getDescriptions)
      result["commands"].append(commands[i].description);
  }

  return OK;
}

JSON_STATUS CJSONRPC::Version(const CStdString &method, const Value& parameterObject, Value &result)
{
  result["version"] = 1;

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
      outputroot["error"]["code"] = -32602;
      outputroot["error"]["message"] = "Invalid params.";
      break;
    case MethodNotFound:
      outputroot["error"]["code"] = -32601;
      outputroot["error"]["message"] = "Method not found.";
      break;
    case ParseError:
      outputroot["error"]["code"] = -32700;
      outputroot["error"]["message"] = "Parse error.";
      break;
    default:
      outputroot["error"]["code"] = -32603;
      outputroot["error"]["message"] = "Internal error.";
      break;
  }

  StyledWriter writer;
  CStdString str = writer.write(outputroot);
  return str;
}

JSON_STATUS CJSONRPC::InternalMethodCall(const CStdString& method, Value& o, Value &result)
{
  bool hasParameters = o.isMember("params");

  ActionMap::iterator iter = m_actionMap.find(method);
  if( iter != m_actionMap.end() )
  {
    if ((iter->second.parameters == MayHaveParameters) || (iter->second.parameters > 0 == hasParameters))
      return iter->second.method(method, o["params"], result);
    else
      return InvalidParams;
  }
  else
    return MethodNotFound;
}
