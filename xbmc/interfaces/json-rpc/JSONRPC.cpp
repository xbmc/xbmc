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
//  { "PicturePlayer.PlayPause",                      CPicturePlayerOperations::PlayPause },
//  { "PicturePlayer.Stop",                           CPicturePlayerOperations::Stop },
//  { "PicturePlayer.SkipPrevious",                   CPicturePlayerOperations::SkipPrevious },
//  { "PicturePlayer.SkipNext",                       CPicturePlayerOperations::SkipNext },
//
//  { "PicturePlayer.MoveLeft",                       CPicturePlayerOperations::MoveLeft },
//  { "PicturePlayer.MoveRight",                      CPicturePlayerOperations::MoveRight },
//  { "PicturePlayer.MoveDown",                       CPicturePlayerOperations::MoveDown },
//  { "PicturePlayer.MoveUp",                         CPicturePlayerOperations::MoveUp },
//
//  { "PicturePlayer.ZoomOut",                        CPicturePlayerOperations::ZoomOut },
//  { "PicturePlayer.ZoomIn",                         CPicturePlayerOperations::ZoomIn },
//  { "PicturePlayer.Zoom",                           CPicturePlayerOperations::Zoom },
//  { "PicturePlayer.Rotate",                         CPicturePlayerOperations::Rotate },

// Video Playlist
  // TODO

// Audio Playlist
  // TODO

// Playlist
//  { "Playlist.Create",                              CPlaylistOperations::Create },
//  { "Playlist.Destroy",                             CPlaylistOperations::Destroy },
//
//  { "Playlist.GetItems",                            CPlaylistOperations::GetItems },
//  { "Playlist.Add",                                 CPlaylistOperations::Add },
//  { "Playlist.Remove",                              CPlaylistOperations::Remove },
//  { "Playlist.Swap",                                CPlaylistOperations::Swap },
//  { "Playlist.Shuffle",                             CPlaylistOperations::Shuffle },
//  { "Playlist.UnShuffle",                           CPlaylistOperations::UnShuffle },

// Files
//  { "Files.GetSources",                             CFileOperations::GetRootDirectory },
//  { "Files.Download",                               CFileOperations::Download },
//  { "Files.GetDirectory",                           CFileOperations::GetDirectory },

// Music Library
  // TODO

// Video Library
  // TODO

// System operations
//  { "System.Shutdown",                              CSystemOperations::Shutdown },
//  { "System.Suspend",                               CSystemOperations::Suspend },
//  { "System.Hibernate",                             CSystemOperations::Hibernate },
//  { "System.Reboot",                                CSystemOperations::Reboot },
//  { "System.GetInfoLabels",                         CSystemOperations::GetInfoLabels },
//  { "System.GetInfoBooleans",                       CSystemOperations::GetInfoBooleans },

// XBMC operations
//  { "XBMC.GetVolume",                               CXBMCOperations::GetVolume },
//  { "XBMC.SetVolume",                               CXBMCOperations::SetVolume },
//  { "XBMC.ToggleMute",                              CXBMCOperations::ToggleMute },
//  { "XBMC.Play",                                    CXBMCOperations::Play },
//  { "XBMC.StartSlideshow",                          CXBMCOperations::StartSlideshow },
//  { "XBMC.Log",                                     CXBMCOperations::Log },
//  { "XBMC.Quit",                                    CXBMCOperations::Quit }
};

/*Command CJSONRPC::m_commands[] = {
// JSON-RPC
  { "JSONRPC.Introspect",                           CJSONRPC::Introspect,                                Response,     ReadData,        "Enumerates all actions and descriptions. Parameter example {\"getdescriptions\": true, \"getpermissions\": true, \"filterbytransport\": true }. All parameters optional" },
  { "JSONRPC.Version",                              CJSONRPC::Version,                                   Response,     ReadData,        "Retrieve the jsonrpc protocol version" },
  { "JSONRPC.Permission",                           CJSONRPC::Permission,                                Response,     ReadData,        "Retrieve the clients permissions" },
  { "JSONRPC.Ping",                                 CJSONRPC::Ping,                                      Response,     ReadData,        "Ping responder" },
  { "JSONRPC.GetAnnouncementFlags",                 CJSONRPC::GetAnnouncementFlags,                      Announcing,   ReadData,        "Get announcement flags" },
  { "JSONRPC.SetAnnouncementFlags",                 CJSONRPC::SetAnnouncementFlags,                      Announcing,   ControlAnnounce, "Change the announcement flags. Parameter example {\"playback\": true, \"gui\": false }" },
  { "JSONRPC.Announce",                             CJSONRPC::Announce,                                  Response,     ReadData,        "Announce to other connected clients. Parameter example {\"sender\": \"foo\", \"message\": \"bar\", \"data\": \"somedata\" }. data is optional" },

// Player
  { "Player.GetActivePlayers",                      CPlayerOperations::GetActivePlayers,                 Response,     ReadData,        "Returns all active players IDs"},

// Music player
  { "AudioPlayer.State",                            CAVPlayerOperations::State,                          Response,     ReadData,        "Returns Current Playback state"},
  { "AudioPlayer.PlayPause",                        CAVPlayerOperations::PlayPause,                      Response,     ControlPlayback, "Pauses or unpause playback, returns new state" },
  { "AudioPlayer.Stop",                             CAVPlayerOperations::Stop,                           Response,     ControlPlayback, "Stops playback" },
  { "AudioPlayer.SkipPrevious",                     CAVPlayerOperations::SkipPrevious,                   Response,     ControlPlayback, "Skips to previous item on the playlist" },
  { "AudioPlayer.SkipNext",                         CAVPlayerOperations::SkipNext,                       Response,     ControlPlayback, "Skips to next item on the playlist" },

  { "AudioPlayer.BigSkipBackward",                  CAVPlayerOperations::BigSkipBackward,                Response,     ControlPlayback, "" },
  { "AudioPlayer.BigSkipForward",                   CAVPlayerOperations::BigSkipForward,                 Response,     ControlPlayback, "" },
  { "AudioPlayer.SmallSkipBackward",                CAVPlayerOperations::SmallSkipBackward,              Response,     ControlPlayback, "" },
  { "AudioPlayer.SmallSkipForward",                 CAVPlayerOperations::SmallSkipForward,               Response,     ControlPlayback, "" },

  { "AudioPlayer.Rewind",                           CAVPlayerOperations::Rewind,                         Response,     ControlPlayback, "Rewind current playback" },
  { "AudioPlayer.Forward",                          CAVPlayerOperations::Forward,                        Response,     ControlPlayback, "Forward current playback" },

  { "AudioPlayer.GetTime",                          CAVPlayerOperations::GetTime,                        Response,     ReadData,        "Retrieve time" },
  { "AudioPlayer.GetTimeMS",                        CAVPlayerOperations::GetTimeMS,                      Response,     ReadData,        "Retrieve time in MS" },
  { "AudioPlayer.GetPercentage",                    CAVPlayerOperations::GetPercentage,                  Response,     ReadData,        "Retrieve percentage" },
  { "AudioPlayer.SeekTime",                         CAVPlayerOperations::SeekTime,                       Response,     ControlPlayback, "Seek to a specific time. Parameter integer in seconds" },
  { "AudioPlayer.SeekPercentage",                   CAVPlayerOperations::SeekPercentage,                 Response,     ControlPlayback, "Seek to a specific percentage. Parameter float or integer from 0 to 100" },

  { "AudioPlayer.Record",                           CAVPlayerOperations::Record,                         Response,     ControlPlayback, "" },

// Video player
  { "VideoPlayer.State",                            CAVPlayerOperations::State,                          Response,     ReadData,        "Returns Current Playback state"},
  { "VideoPlayer.PlayPause",                        CAVPlayerOperations::PlayPause,                      Response,     ControlPlayback, "Pauses or unpause playback, returns new state" },
  { "VideoPlayer.Stop",                             CAVPlayerOperations::Stop,                           Response,     ControlPlayback, "Stops playback" },
  { "VideoPlayer.SkipPrevious",                     CAVPlayerOperations::SkipPrevious,                   Response,     ControlPlayback, "Skips to previous item on the playlist" },
  { "VideoPlayer.SkipNext",                         CAVPlayerOperations::SkipNext,                       Response,     ControlPlayback, "Skips to next item on the playlist" },

  { "VideoPlayer.BigSkipBackward",                  CAVPlayerOperations::BigSkipBackward,                Response,     ControlPlayback, "" },
  { "VideoPlayer.BigSkipForward",                   CAVPlayerOperations::BigSkipForward,                 Response,     ControlPlayback, "" },
  { "VideoPlayer.SmallSkipBackward",                CAVPlayerOperations::SmallSkipBackward,              Response,     ControlPlayback, "" },
  { "VideoPlayer.SmallSkipForward",                 CAVPlayerOperations::SmallSkipForward,               Response,     ControlPlayback, "" },

  { "VideoPlayer.Rewind",                           CAVPlayerOperations::Rewind,                         Response,     ControlPlayback, "Rewind current playback" },
  { "VideoPlayer.Forward",                          CAVPlayerOperations::Forward,                        Response,     ControlPlayback, "Forward current playback" },

  { "VideoPlayer.GetTime",                          CAVPlayerOperations::GetTime,                        Response,     ReadData,        "Retrieve time" },
  { "VideoPlayer.GetTimeMS",                        CAVPlayerOperations::GetTimeMS,                      Response,     ReadData,        "Retrieve time in MS" },
  { "VideoPlayer.GetPercentage",                    CAVPlayerOperations::GetPercentage,                  Response,     ReadData,        "Retrieve percentage" },
  { "VideoPlayer.SeekTime",                         CAVPlayerOperations::SeekTime,                       Response,     ControlPlayback, "Seek to a specific time. Parameter integer in seconds" },
  { "VideoPlayer.SeekPercentage",                   CAVPlayerOperations::SeekPercentage,                 Response,     ControlPlayback, "Seek to a specific percentage. Parameter float or integer from 0 to 100" },

// Picture player
  { "PicturePlayer.PlayPause",                      CPicturePlayerOperations::PlayPause,                 Response,     ControlPlayback, "Pauses or unpause slideshow" },
  { "PicturePlayer.Stop",                           CPicturePlayerOperations::Stop,                      Response,     ControlPlayback, "Stops slideshow" },
  { "PicturePlayer.SkipPrevious",                   CPicturePlayerOperations::SkipPrevious,              Response,     ControlPlayback, "Skips to previous picture in the slideshow" },
  { "PicturePlayer.SkipNext",                       CPicturePlayerOperations::SkipNext,                  Response,     ControlPlayback, "Skips to next picture in the slideshow" },

  { "PicturePlayer.MoveLeft",                       CPicturePlayerOperations::MoveLeft,                  Response,     ControlPlayback, "If picture is zoomed move viewport left otherwise skip previous" },
  { "PicturePlayer.MoveRight",                      CPicturePlayerOperations::MoveRight,                 Response,     ControlPlayback, "If picture is zoomed move viewport right otherwise skip previous" },
  { "PicturePlayer.MoveDown",                       CPicturePlayerOperations::MoveDown,                  Response,     ControlPlayback, "If picture is zoomed move viewport down" },
  { "PicturePlayer.MoveUp",                         CPicturePlayerOperations::MoveUp,                    Response,     ControlPlayback, "If picture is zoomed move viewport up" },

  { "PicturePlayer.ZoomOut",                        CPicturePlayerOperations::ZoomOut,                   Response,     ControlPlayback, "Zoom out once" },
  { "PicturePlayer.ZoomIn",                         CPicturePlayerOperations::ZoomIn,                    Response,     ControlPlayback, "Zoom in once" },
  { "PicturePlayer.Zoom",                           CPicturePlayerOperations::Zoom,                      Response,     ControlPlayback, "Zooms current picture. Parameter integer of zoom level" },
  { "PicturePlayer.Rotate",                         CPicturePlayerOperations::Rotate,                    Response,     ControlPlayback, "Rotates current picture" },

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

// Playlist
  { "Playlist.Create",                              CPlaylistOperations::Create,                         Response,     ReadData,        "Creates a virtual playlist from a given one from a file" },
  { "Playlist.Destroy",                             CPlaylistOperations::Destroy,                        Response,     ReadData,        "Destroys a virtual playlist" },

  { "Playlist.GetItems",                            CPlaylistOperations::GetItems,                       Response,     ReadData,        "Retrieve items in the playlist. Parameter example {\"playlist\": \"music\" }. playlist optional." },
  { "Playlist.Add",                                 CPlaylistOperations::Add,                            Response,     ControlPlayback, "Add items to the playlist. Parameter example {\"playlist\": \"music\", \"file\": \"/foo/bar.mp3\" }. playlist optional." },
  { "Playlist.Remove",                              CPlaylistOperations::Remove,                         Response,     ControlPlayback, "Remove items in the playlist. Parameter example {\"playlist\": \"music\", \"item\": 0 }. playlist optional." },
  { "Playlist.Swap",                                CPlaylistOperations::Swap,                           Response,     ControlPlayback, "Swap items in the playlist. Parameter example {\"playlist\": \"music\", \"item1\": 0, \"item2\": 1 }. playlist optional." },
  { "Playlist.Shuffle",                             CPlaylistOperations::Shuffle,                        Response,     ControlPlayback, "Shuffle playlist" },
  { "Playlist.UnShuffle",                           CPlaylistOperations::UnShuffle,                      Response,     ControlPlayback, "UnShuffle playlist" },

// Video library
  { "VideoLibrary.GetMovies",                       CVideoLibrary::GetMovies,                            Response,     ReadData,        "Retrieve all movies. Parameter example { \"fields\": [\"plot\"], \"sortmethod\": \"title\", \"sortorder\": \"ascending\", \"start\": 0, \"end\": 3}. fields, sortorder, sortmethod, start and end are optional" },
  { "VideoLibrary.GetMovieDetails",                 CVideoLibrary::GetMovieDetails,                      Response,     ReadData,        "Retrieve details about a specific movie. Parameter example { \"fields\": [\"plot\"], \"movieid\": 12}. fields is optional" },

  { "VideoLibrary.GetTVShows",                      CVideoLibrary::GetTVShows,                           Response,     ReadData,        "Parameter example { \"fields\": [\"plot\"], \"sortmethod\": \"label\", \"sortorder\": \"ascending\", \"start\": 0, \"end\": 3}. sortorder, sortmethod, start and end are optional" },
  { "VideoLibrary.GetTVShowDetails",                CVideoLibrary::GetTVShowDetails,                     Response,     ReadData,        "Retrieve details about a specific tv show. Parameter example { \"fields\": [\"plot\"], \"tvshowid\": 12}. fields is optional" },
  { "VideoLibrary.GetSeasons",                      CVideoLibrary::GetSeasons,                           Response,     ReadData,        "Parameter example { \"tvshowid\": 0, \"fields\": [\"season\"], \"sortmethod\": \"label\", \"sortorder\": \"ascending\", \"start\": 0, \"end\": 3}. sortorder, sortmethod, start and end are optional" },
  { "VideoLibrary.GetEpisodes",                     CVideoLibrary::GetEpisodes,                          Response,     ReadData,        "Parameter example { \"tvshowid\": 0, \"season\": 1, \"fields\": [\"plot\"], \"sortmethod\": \"episode\", \"sortorder\": \"ascending\", \"start\": 0, \"end\": 3}. sortorder, sortmethod, start and end are optional" },
  { "VideoLibrary.GetEpisodeDetails",               CVideoLibrary::GetEpisodeDetails,                    Response,     ReadData,        "Retrieve details about a specific tv show episode. Parameter example { \"fields\": [\"plot\"], \"episodeid\": 12}. fields is optional" },

  { "VideoLibrary.GetMusicVideos",                  CVideoLibrary::GetMusicVideos,                       Response,     ReadData,        "Parameter example { \"artistid\": 0, \"albumid\": 0, \"fields\": [\"plot\"], \"sortmethod\": \"artistignorethe\", \"sortorder\": \"ascending\", \"start\": 0, \"end\": 3}. sortorder, sortmethod, start and end are optional" },
  { "VideoLibrary.GetMusicVideoDetails",            CVideoLibrary::GetMusicVideoDetails,                 Response,     ReadData,        "Retrieve details about a specific music video. Parameter example { \"fields\": [\"plot\"], \"musicvideoid\": 12}. fields is optional" },

  { "VideoLibrary.GetRecentlyAddedMovies",          CVideoLibrary::GetRecentlyAddedMovies,               Response,     ReadData,        "Retrieve all recently added movies. Parameter example { \"fields\": [\"plot\"], \"sortmethod\": \"title\", \"sortorder\": \"ascending\", \"start\": 0, \"end\": 3}. fields, sortorder, sortmethod, start and end are optional" },
  { "VideoLibrary.GetRecentlyAddedEpisodes",        CVideoLibrary::GetRecentlyAddedEpisodes,             Response,     ReadData,        "Retrieve all recently added episodes. Parameter example { \"fields\": [\"plot\"], \"sortmethod\": \"title\", \"sortorder\": \"ascending\", \"start\": 0, \"end\": 3}. fields, sortorder, sortmethod, start and end are optional" },
  { "VideoLibrary.GetRecentlyAddedMusicVideos",     CVideoLibrary::GetRecentlyAddedMusicVideos,          Response,     ReadData,        "Retrieve all recently added music videos. Parameter example { \"fields\": [\"plot\"], \"sortmethod\": \"title\", \"sortorder\": \"ascending\", \"start\": 0, \"end\": 3}. fields, sortorder, sortmethod, start and end are optional" },

  { "VideoLibrary.ScanForContent",                  CVideoLibrary::ScanForContent,                       Response,     ScanLibrary,     "" },

// System operations
  { "System.Shutdown",                              CSystemOperations::Shutdown,                         Response,     ControlPower,    "" },
  { "System.Suspend",                               CSystemOperations::Suspend,                          Response,     ControlPower,    "" },
  { "System.Hibernate",                             CSystemOperations::Hibernate,                        Response,     ControlPower,    "" },
  { "System.Reboot",                                CSystemOperations::Reboot,                           Response,     ControlPower,    "" },

  { "System.GetInfoLabels",                         CSystemOperations::GetInfoLabels,                    Response,     ReadData,        "Retrieve info labels about the system" },
  { "System.GetInfoBooleans",                       CSystemOperations::GetInfoBooleans,                  Response,     ReadData,        "Retrieve info booleans about the system" },

// XBMC Operations
  { "XBMC.GetVolume",                               CXBMCOperations::GetVolume,                          Response,     ReadData,        "Retrieve the current volume" },
  { "XBMC.SetVolume",                               CXBMCOperations::SetVolume,                          Response,     ControlPlayback, "Set volume. Parameter integer between 0 amd 100" },
  { "XBMC.ToggleMute",                              CXBMCOperations::ToggleMute,                         Response,     ControlPlayback, "Toggle mute" },

  { "XBMC.Play",                                    CXBMCOperations::Play,                               Response,     ControlPlayback, "Starts playback" },
  { "XBMC.StartSlideshow",                          CXBMCOperations::StartSlideshow,                     Response,     ControlPlayback, "Starts slideshow. Parameter example {\"directory\": \"/foo/\", \"random\": true, \"recursive\": true} or just string to recursively and random run directory" },

  { "XBMC.Log",                                     CXBMCOperations::Log,                                Response,     Logging,         "Logs a line in the xbmc.log. Parameter example {\"message\": \"foo\", \"level\": \"info\"} or just a string to log message with level debug" },

  { "XBMC.Quit",                                    CXBMCOperations::Quit,                               Response,     ControlPower,    "Quit xbmc" }
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
