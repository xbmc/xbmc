/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#pragma once

// Window ID defines to make the code a bit more readable
#define WINDOW_INVALID                     9999 // do not change. value is used to avoid include in headers.
#define WINDOW_HOME                       10000
#define WINDOW_PROGRAMS                   10001
#define WINDOW_PICTURES                   10002
#define WINDOW_FILES                      10003
#define WINDOW_SETTINGS_MENU              10004
#define WINDOW_MUSIC                      10005 // virtual window to return the music start window.
#define WINDOW_VIDEOS                     10006
#define WINDOW_SYSTEM_INFORMATION         10007
#define WINDOW_TEST_PATTERN               10008
#define WINDOW_SCREEN_CALIBRATION         10011

#define WINDOW_SETTINGS_START             10012
#define WINDOW_SETTINGS_MYPICTURES        10012
#define WINDOW_SETTINGS_MYPROGRAMS        10013
#define WINDOW_SETTINGS_MYWEATHER         10014
#define WINDOW_SETTINGS_MYMUSIC           10015
#define WINDOW_SETTINGS_SYSTEM            10016
#define WINDOW_SETTINGS_MYVIDEOS          10017
#define WINDOW_SETTINGS_SERVICE           10018 // former (Eden) WINDOW_SETTINGS_NETWORK
#define WINDOW_SETTINGS_APPEARANCE        10019

#define WINDOW_SCRIPTS                    10020 // virtual window for backward compatibility
#define WINDOW_SETTINGS_MYPVR             10021

#define WINDOW_VIDEO_FILES                10024
#define WINDOW_VIDEO_NAV                  10025
#define WINDOW_VIDEO_PLAYLIST             10028

#define WINDOW_LOGIN_SCREEN               10029
#define WINDOW_SETTINGS_PROFILES          10034
#define WINDOW_SKIN_SETTINGS              10035

#define WINDOW_ADDON_BROWSER              10040

#define WINDOW_SCREENSAVER_DIM               97
#define WINDOW_DEBUG_INFO                    98
#define WINDOW_DIALOG_POINTER             10099
#define WINDOW_DIALOG_YES_NO              10100
#define WINDOW_DIALOG_PROGRESS            10101
#define WINDOW_DIALOG_KEYBOARD            10103
#define WINDOW_DIALOG_VOLUME_BAR          10104
#define WINDOW_DIALOG_SUB_MENU            10105
#define WINDOW_DIALOG_CONTEXT_MENU        10106
#define WINDOW_DIALOG_KAI_TOAST           10107
#define WINDOW_DIALOG_NUMERIC             10109
#define WINDOW_DIALOG_GAMEPAD             10110
#define WINDOW_DIALOG_BUTTON_MENU         10111
#define WINDOW_DIALOG_MUTE_BUG            10113
#define WINDOW_DIALOG_PLAYER_CONTROLS     10114
#define WINDOW_DIALOG_SEEK_BAR            10115
#define WINDOW_DIALOG_MUSIC_OSD           10120
#define WINDOW_DIALOG_VIS_SETTINGS        10121
#define WINDOW_DIALOG_VIS_PRESET_LIST     10122
#define WINDOW_DIALOG_VIDEO_OSD_SETTINGS  10123
#define WINDOW_DIALOG_AUDIO_OSD_SETTINGS  10124
#define WINDOW_DIALOG_VIDEO_BOOKMARKS     10125
#define WINDOW_DIALOG_FILE_BROWSER        10126
#define WINDOW_DIALOG_NETWORK_SETUP       10128
#define WINDOW_DIALOG_MEDIA_SOURCE        10129
#define WINDOW_DIALOG_PROFILE_SETTINGS    10130
#define WINDOW_DIALOG_LOCK_SETTINGS       10131
#define WINDOW_DIALOG_CONTENT_SETTINGS    10132
#define WINDOW_DIALOG_FAVOURITES          10134
#define WINDOW_DIALOG_SONG_INFO           10135
#define WINDOW_DIALOG_SMART_PLAYLIST_EDITOR 10136
#define WINDOW_DIALOG_SMART_PLAYLIST_RULE   10137
#define WINDOW_DIALOG_BUSY                10138
#define WINDOW_DIALOG_PICTURE_INFO        10139
#define WINDOW_DIALOG_ADDON_SETTINGS      10140
#define WINDOW_DIALOG_ACCESS_POINTS       10141
#define WINDOW_DIALOG_FULLSCREEN_INFO     10142
#define WINDOW_DIALOG_KARAOKE_SONGSELECT  10143
#define WINDOW_DIALOG_KARAOKE_SELECTOR    10144
#define WINDOW_DIALOG_SLIDER              10145
#define WINDOW_DIALOG_ADDON_INFO          10146
#define WINDOW_DIALOG_TEXT_VIEWER         10147
#define WINDOW_DIALOG_PLAY_EJECT          10148
#define WINDOW_DIALOG_PERIPHERAL_MANAGER  10149
#define WINDOW_DIALOG_PERIPHERAL_SETTINGS 10150
#define WINDOW_DIALOG_EXT_PROGRESS        10151
#define WINDOW_DIALOG_MEDIA_FILTER        10152
#define WINDOW_DIALOG_SUBTITLES           10153

#define WINDOW_MUSIC_PLAYLIST             10500
#define WINDOW_MUSIC_FILES                10501
#define WINDOW_MUSIC_NAV                  10502
#define WINDOW_MUSIC_PLAYLIST_EDITOR      10503

#define WINDOW_DIALOG_OSD_TELETEXT        10600

// PVR related Window and Dialog ID's
#define WINDOW_DIALOG_PVR_GUIDE_INFO      10602
#define WINDOW_DIALOG_PVR_RECORDING_INFO  10603
#define WINDOW_DIALOG_PVR_TIMER_SETTING   10604
#define WINDOW_DIALOG_PVR_GROUP_MANAGER   10605
#define WINDOW_DIALOG_PVR_CHANNEL_MANAGER 10606
#define WINDOW_DIALOG_PVR_GUIDE_SEARCH    10607
#define WINDOW_DIALOG_PVR_CHANNEL_SCAN    10608
#define WINDOW_DIALOG_PVR_UPDATE_PROGRESS 10609
#define WINDOW_DIALOG_PVR_OSD_CHANNELS    10610
#define WINDOW_DIALOG_PVR_OSD_GUIDE       10611
#define WINDOW_FULLSCREEN_LIVETV          10614 // virtual window for PVR specific keymap bindings in fullscreen playback (which internally uses WINDOW_FULLSCREEN_VIDEO)
#define WINDOW_TV_CHANNELS                10615
#define WINDOW_TV_RECORDINGS              10616
#define WINDOW_TV_GUIDE                   10617
#define WINDOW_TV_TIMERS                  10618
#define WINDOW_TV_SEARCH                  10619
#define WINDOW_RADIO_CHANNELS             10620
#define WINDOW_RADIO_RECORDINGS           10621
#define WINDOW_RADIO_GUIDE                10622
#define WINDOW_RADIO_TIMERS               10623
#define WINDOW_RADIO_SEARCH               10624
#define WINDOW_FULLSCREEN_RADIO           10625 // virtual window for PVR radio specific keymaps with fallback to WINDOW_VISUALISATION

//#define WINDOW_VIRTUAL_KEYBOARD           11000
// WINDOW_ID's from 11100 to 11199 reserved for Skins

#define WINDOW_DIALOG_SELECT              12000
#define WINDOW_DIALOG_MUSIC_INFO          12001
#define WINDOW_DIALOG_OK                  12002
#define WINDOW_DIALOG_VIDEO_INFO          12003
#define WINDOW_FULLSCREEN_VIDEO           12005
#define WINDOW_VISUALISATION              12006
#define WINDOW_SLIDESHOW                  12007
#define WINDOW_DIALOG_FILESTACKING        12008
#define WINDOW_KARAOKELYRICS              12009
#define WINDOW_WEATHER                    12600
#define WINDOW_SCREENSAVER                12900
#define WINDOW_DIALOG_VIDEO_OSD           12901

#define WINDOW_VIDEO_MENU                 12902
#define WINDOW_DIALOG_MUSIC_OVERLAY       12903
#define WINDOW_DIALOG_VIDEO_OVERLAY       12904
#define WINDOW_VIDEO_TIME_SEEK            12905 // virtual window for time seeking during fullscreen video

#define WINDOW_START                      12998 // first window to load
#define WINDOW_STARTUP_ANIM               12999 // for startup animations

// WINDOW_ID's from 13000 to 13099 reserved for Python

#define WINDOW_PYTHON_START               13000
#define WINDOW_PYTHON_END                 13099

// WINDOW_ID's from 14000 to 14099 reserved for Addons

#define WINDOW_ADDON_START                14000
#define WINDOW_ADDON_END                  14099

