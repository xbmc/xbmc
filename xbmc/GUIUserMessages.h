/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

//  GUI messages outside GuiLib
//

#include "guilib/GUIMessage.h"

//  Source related messages
constexpr const int GUI_MSG_REMOVED_MEDIA          = GUI_MSG_USER + 1;
constexpr const int GUI_MSG_UPDATE_SOURCES         = GUI_MSG_USER + 2;

//  General playlist items changed
constexpr const int GUI_MSG_PLAYLIST_CHANGED       = GUI_MSG_USER + 3;

//  Start Slideshow in my pictures lpVoid = std::string
//  Param lpVoid: std::string* that points to the Directory
//  to start the slideshow in.
constexpr const int GUI_MSG_START_SLIDESHOW        = GUI_MSG_USER + 4;

constexpr const int GUI_MSG_PLAYBACK_STARTED       = GUI_MSG_USER + 5;
constexpr const int GUI_MSG_PLAYBACK_ENDED         = GUI_MSG_USER + 6;

//  Playback stopped by user
constexpr const int GUI_MSG_PLAYBACK_STOPPED       = GUI_MSG_USER + 7;

//  Message is send by the playlistplayer when it starts a playlist
//  Parameter:
//  dwParam1 = Current Playlist, can be PLAYLIST::TYPE_MUSIC or PLAYLIST::TYPE_VIDEO
//  dwParam2 = Item started in the playlist
//  lpVoid = Playlistitem started playing
constexpr const int GUI_MSG_PLAYLISTPLAYER_STARTED  = GUI_MSG_USER + 8;

//  Message is send by playlistplayer when next/previous item is started
//  Parameter:
//  dwParam1 = Current Playlist, can be PLAYLIST::TYPE_MUSIC or PLAYLIST::TYPE_VIDEO
//  dwParam2 = LOWORD Position of the current playlistitem
//             HIWORD Position of the previous playlistitem
//  lpVoid = Current Playlistitem
constexpr const int GUI_MSG_PLAYLISTPLAYER_CHANGED  = GUI_MSG_USER + 9;

//  Message is send by the playlistplayer when the last item to play ended
//  Parameter:
//  dwParam1 = Current Playlist, can be PLAYLIST::TYPE_MUSIC or PLAYLIST::TYPE_VIDEO
//  dwParam2 = Playlistitem played when stopping
constexpr const int GUI_MSG_PLAYLISTPLAYER_STOPPED  = GUI_MSG_USER + 10;

constexpr const int GUI_MSG_LOAD_SKIN               = GUI_MSG_USER + 11;

//  Message is send by the dialog scan music
//  Parameter:
//  StringParam = Directory last scanned
constexpr const int GUI_MSG_DIRECTORY_SCANNED       = GUI_MSG_USER + 12;

constexpr const int GUI_MSG_SCAN_FINISHED           = GUI_MSG_USER + 13;

//  Player has requested the next item for caching purposes (PAPlayer)
constexpr const int GUI_MSG_QUEUE_NEXT_ITEM         = GUI_MSG_USER + 16;

//  Playback request for the trailer of a given item
constexpr const int GUI_MSG_PLAY_TRAILER = GUI_MSG_USER + 17;

// Visualisation messages when loading/unloading
constexpr const int GUI_MSG_VISUALISATION_UNLOADING = GUI_MSG_USER + 117; // sent by vis
constexpr const int GUI_MSG_VISUALISATION_LOADED    = GUI_MSG_USER + 118; // sent by vis
constexpr const int GUI_MSG_GET_VISUALISATION       = GUI_MSG_USER + 119; // request to vis for the visualisation object
constexpr const int GUI_MSG_VISUALISATION_ACTION    = GUI_MSG_USER + 120; // request the vis perform an action
constexpr const int GUI_MSG_VISUALISATION_RELOAD    = GUI_MSG_USER + 121; // request the vis to reload

constexpr const int GUI_MSG_VIDEO_MENU_STARTED      = GUI_MSG_USER + 21; // sent by VideoPlayer on entry to the menu

//  Message is sent by built-in function to alert the playlist window
//  that the user has initiated Random playback
//  dwParam1 = Current Playlist (PLAYLIST::TYPE_MUSIC or PLAYLIST::TYPE_VIDEO)
//  dwParam2 = 0 or 1 (Enabled or Disabled)
constexpr const int GUI_MSG_PLAYLISTPLAYER_RANDOM   = GUI_MSG_USER + 22;

//  Message is sent by built-in function to alert the playlist window
//  that the user has initiated Repeat playback
//  dwParam1 = Current Playlist (PLAYLIST::TYPE_MUSIC or PLAYLIST::TYPE_VIDEO)
//  dwParam2 = 0 or 1 or 2 (Off, Repeat All, Repeat One)
constexpr const int GUI_MSG_PLAYLISTPLAYER_REPEAT   = GUI_MSG_USER + 23;

// Message is sent by the background info loader when it is finished with fetching a weather location.
constexpr const int GUI_MSG_WEATHER_FETCHED         = GUI_MSG_USER + 24;

// Message is sent to the screensaver window to tell that it should check the lock
constexpr const int GUI_MSG_CHECK_LOCK              = GUI_MSG_USER + 25;

// Message is sent to media windows to force a refresh
constexpr const int GUI_MSG_UPDATE                  = GUI_MSG_USER + 26;

// Message sent by filtering dialog to request a new filter be applied
constexpr const int GUI_MSG_FILTER_ITEMS            = GUI_MSG_USER + 27;

// Message sent by search dialog to request a new search be applied
constexpr const int GUI_MSG_SEARCH_UPDATE           = GUI_MSG_USER + 28;

// Message sent to tell the GUI to update a single item
constexpr const int GUI_MSG_UPDATE_ITEM             = GUI_MSG_USER + 29;

// Flags for GUI_MSG_UPDATE_ITEM message
constexpr int GUI_MSG_FLAG_UPDATE_LIST  = 0x00000001;
constexpr int GUI_MSG_FLAG_FORCE_UPDATE = 0x00000002;

// Message sent to tell the GUI to change view mode
constexpr const int GUI_MSG_CHANGE_VIEW_MODE        = GUI_MSG_USER + 30;

// Message sent to tell the GUI to change sort method/direction
constexpr const int GUI_MSG_CHANGE_SORT_METHOD      = GUI_MSG_USER + 31;
constexpr const int GUI_MSG_CHANGE_SORT_DIRECTION   = GUI_MSG_USER + 32;

// Sent from filesystem if a path is known to have changed
constexpr const int GUI_MSG_UPDATE_PATH             = GUI_MSG_USER + 33;

// Sent to tell window to initiate a search dialog
constexpr const int GUI_MSG_SEARCH                  = GUI_MSG_USER + 34;

// Sent to the AddonSetting dialogs from addons if they updated a setting
constexpr const int GUI_MSG_SETTING_UPDATED         = GUI_MSG_USER + 35;

// Message sent to CGUIWindowSlideshow to show picture
constexpr const int GUI_MSG_SHOW_PICTURE            = GUI_MSG_USER + 36;

// Sent to CGUIWindowEventLog
constexpr const int GUI_MSG_EVENT_ADDED             = GUI_MSG_USER + 39;
constexpr const int GUI_MSG_EVENT_REMOVED           = GUI_MSG_USER + 40;

// Send to RDS RadioText handlers to inform about changed data
constexpr const int GUI_MSG_UPDATE_RADIOTEXT        = GUI_MSG_USER + 41;

constexpr const int GUI_MSG_PLAYBACK_ERROR          = GUI_MSG_USER + 42;
constexpr const int GUI_MSG_PLAYBACK_AVCHANGE       = GUI_MSG_USER + 43;
constexpr const int GUI_MSG_PLAYBACK_AVSTARTED      = GUI_MSG_USER + 44;

// Sent to notify system sleep/wake
constexpr const int GUI_MSG_SYSTEM_SLEEP  			= GUI_MSG_USER + 45;
constexpr const int GUI_MSG_SYSTEM_WAKE 			= GUI_MSG_USER + 46;

constexpr const int GUI_MSG_PLAYBACK_PAUSED = GUI_MSG_USER + 47;
constexpr const int GUI_MSG_PLAYBACK_RESUMED = GUI_MSG_USER + 48;
constexpr const int GUI_MSG_PLAYBACK_SEEKED = GUI_MSG_USER + 49;
constexpr const int GUI_MSG_PLAYBACK_SPEED_CHANGED = GUI_MSG_USER + 50;
