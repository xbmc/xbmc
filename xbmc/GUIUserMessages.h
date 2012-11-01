//  GUI messages outside GuiLib
//
#pragma once

/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "guilib/GUIMessage.h"

//  Source related messages
#define GUI_MSG_REMOVED_MEDIA           GUI_MSG_USER + 1
#define GUI_MSG_UPDATE_SOURCES          GUI_MSG_USER + 2

//  General playlist items changed
#define GUI_MSG_PLAYLIST_CHANGED        GUI_MSG_USER + 3

//  Start Slideshow in my pictures lpVoid = CStdString
//  Param lpVoid: CStdString* that points to the Directory
//  to start the slideshow in.
#define GUI_MSG_START_SLIDESHOW         GUI_MSG_USER + 4

#define GUI_MSG_PLAYBACK_STARTED        GUI_MSG_USER + 5
#define GUI_MSG_PLAYBACK_ENDED          GUI_MSG_USER + 6

//  Playback stopped by user
#define GUI_MSG_PLAYBACK_STOPPED        GUI_MSG_USER + 7

//  Message is send by the playlistplayer when it starts a playlist
//  Parameter:
//  dwParam1 = Current Playlist, can be PLAYLIST_MUSIC, PLAYLIST_TEMP_MUSIC, PLAYLIST_VIDEO or PLAYLIST_TEMP_VIDEO
//  dwParam2 = Item started in the playlist
//  lpVoid = Playlistitem started playing
#define GUI_MSG_PLAYLISTPLAYER_STARTED  GUI_MSG_USER + 8

//  Message is send by playlistplayer when next/previous item is started
//  Parameter:
//  dwParam1 = Current Playlist, can be PLAYLIST_MUSIC, PLAYLIST_TEMP_MUSIC, PLAYLIST_VIDEO or PLAYLIST_TEMP_VIDEO
//  dwParam2 = LOWORD Position of the current playlistitem
//             HIWORD Position of the previous playlistitem
//  lpVoid = Current Playlistitem
#define GUI_MSG_PLAYLISTPLAYER_CHANGED  GUI_MSG_USER + 9

//  Message is send by the playlistplayer when the last item to play ended
//  Parameter:
//  dwParam1 = Current Playlist, can be PLAYLIST_MUSIC, PLAYLIST_TEMP_MUSIC, PLAYLIST_VIDEO or PLAYLIST_TEMP_VIDEO
//  dwParam2 = Playlistitem played when stopping
#define GUI_MSG_PLAYLISTPLAYER_STOPPED  GUI_MSG_USER + 10

#define GUI_MSG_LOAD_SKIN               GUI_MSG_USER + 11

//  Message is send by the dialog scan music
//  Parameter:
//  StringParam = Directory last scanned
#define GUI_MSG_DIRECTORY_SCANNED       GUI_MSG_USER + 12

#define GUI_MSG_SCAN_FINISHED           GUI_MSG_USER + 13

//  Player has requested the next item for caching purposes (PAPlayer)
#define GUI_MSG_QUEUE_NEXT_ITEM         GUI_MSG_USER + 16

// Visualisation messages when loading/unloading
#define GUI_MSG_VISUALISATION_UNLOADING GUI_MSG_USER + 117 // sent by vis
#define GUI_MSG_VISUALISATION_LOADED    GUI_MSG_USER + 118 // sent by vis
#define GUI_MSG_GET_VISUALISATION       GUI_MSG_USER + 119 // request to vis for the visualisation object
#define GUI_MSG_VISUALISATION_ACTION    GUI_MSG_USER + 120 // request the vis perform an action
#define GUI_MSG_VISUALISATION_RELOAD    GUI_MSG_USER + 121 // request the vis to reload

#define GUI_MSG_VIDEO_MENU_STARTED      GUI_MSG_USER + 21 // sent by dvdplayer on entry to the menu

//  Message is sent by built-in function to alert the playlist window
//  that the user has initiated Random playback
//  dwParam1 = Current Playlist (PLAYLIST_MUSIC, PLAYLIST_TEMP_MUSIC, PLAYLIST_VIDEO or PLAYLIST_TEMP_VIDEO)
//  dwParam2 = 0 or 1 (Enabled or Disabled)
#define GUI_MSG_PLAYLISTPLAYER_RANDOM   GUI_MSG_USER + 22

//  Message is sent by built-in function to alert the playlist window
//  that the user has initiated Repeat playback
//  dwParam1 = Current Playlist (PLAYLIST_MUSIC, PLAYLIST_TEMP_MUSIC, PLAYLIST_VIDEO or PLAYLIST_TEMP_VIDEO)
//  dwParam2 = 0 or 1 or 2 (Off, Repeat All, Repeat One)
#define GUI_MSG_PLAYLISTPLAYER_REPEAT   GUI_MSG_USER + 23

// Message is sent by the background info loader when it is finished with fetching a weather location.
#define GUI_MSG_WEATHER_FETCHED       GUI_MSG_USER + 24

// Message is sent to the screensaver window to tell that it should check the lock
#define GUI_MSG_CHECK_LOCK            GUI_MSG_USER + 25

// Message is sent to media windows to force a refresh
#define GUI_MSG_UPDATE                GUI_MSG_USER + 26

// Message sent by filtering dialog to request a new filter be applied
#define GUI_MSG_FILTER_ITEMS          GUI_MSG_USER + 27

// Message sent by search dialog to request a new search be applied
#define GUI_MSG_SEARCH_UPDATE         GUI_MSG_USER + 28

// Message sent to tell the GUI to update a single item
#define GUI_MSG_UPDATE_ITEM           GUI_MSG_USER + 29

// Message sent to tell the GUI to change view mode
#define GUI_MSG_CHANGE_VIEW_MODE      GUI_MSG_USER + 30

// Message sent to tell the GUI to change sort method/direction
#define GUI_MSG_CHANGE_SORT_METHOD    GUI_MSG_USER + 31
#define GUI_MSG_CHANGE_SORT_DIRECTION GUI_MSG_USER + 32

// Sent from filesystem if a path is known to have changed
#define GUI_MSG_UPDATE_PATH           GUI_MSG_USER + 33

// Sent to tell window to initiate a search dialog
#define GUI_MSG_SEARCH                GUI_MSG_USER + 34

// Sent to the AddonSetting dialogs from addons if they updated a setting
#define GUI_MSG_SETTING_UPDATED       GUI_MSG_USER + 35
