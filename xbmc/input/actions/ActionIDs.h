/*
 *  Copyright (C) 2005-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

/**
 * \defgroup kodi_key_action_ids Action Id's
 * \ingroup python_xbmcgui_window_cb
 * \ingroup python_xbmcgui_action
 * @{
 * @brief Actions that we have defined.
 */

constexpr const int ACTION_NONE = 0;
constexpr const int ACTION_MOVE_LEFT = 1;
constexpr const int ACTION_MOVE_RIGHT = 2;
constexpr const int ACTION_MOVE_UP = 3;
constexpr const int ACTION_MOVE_DOWN = 4;
constexpr const int ACTION_PAGE_UP = 5;
constexpr const int ACTION_PAGE_DOWN = 6;
constexpr const int ACTION_SELECT_ITEM = 7;
constexpr const int ACTION_HIGHLIGHT_ITEM = 8;
constexpr const int ACTION_PARENT_DIR = 9;
constexpr const int ACTION_PREVIOUS_MENU = 10;
constexpr const int ACTION_SHOW_INFO = 11;

constexpr const int ACTION_PAUSE = 12;
constexpr const int ACTION_STOP = 13;
constexpr const int ACTION_NEXT_ITEM = 14;
constexpr const int ACTION_PREV_ITEM = 15;

//! Can be used to specify specific action in a window, Playback control is
//! handled in ACTION_PLAYER_*
constexpr const int ACTION_FORWARD = 16;

//! Can be used to specify specific action in a window, Playback control is
//! handled in ACTION_PLAYER_*
constexpr const int ACTION_REWIND = 17;

//! Toggle between GUI and movie or GUI and visualisation.
constexpr const int ACTION_SHOW_GUI = 18;

//! Toggle quick-access zoom modes. Can be used in videoFullScreen.zml window id=2005
constexpr const int ACTION_ASPECT_RATIO = 19;

//! Seek +1% in the movie. Can be used in videoFullScreen.xml window id=2005
constexpr const int ACTION_STEP_FORWARD = 20;

//! Seek -1% in the movie. Can be used in videoFullScreen.xml window id=2005
constexpr const int ACTION_STEP_BACK = 21;

//! Seek +10% in the movie. Can be used in videoFullScreen.xml window id=2005
constexpr const int ACTION_BIG_STEP_FORWARD = 22;

//! Seek -10% in the movie. Can be used in videoFullScreen.xml window id=2005
constexpr const int ACTION_BIG_STEP_BACK = 23;

//! Show/hide OSD. Can be used in videoFullScreen.xml window id=2005
constexpr const int ACTION_SHOW_OSD = 24;

//! Turn subtitles on/off. Can be used in videoFullScreen.xml window id=2005
constexpr const int ACTION_SHOW_SUBTITLES = 25;

//! Switch to next subtitle of movie. Can be used in videoFullScreen.xml window id=2005
constexpr const int ACTION_NEXT_SUBTITLE = 26;

//! Show debug info for VideoPlayer
constexpr const int ACTION_PLAYER_DEBUG = 27;

//! Show next picture of slideshow. Can be used in slideshow.xml window id=2007
constexpr const int ACTION_NEXT_PICTURE = 28;

//! Show previous picture of slideshow. Can be used in slideshow.xml window id=2007
constexpr const int ACTION_PREV_PICTURE = 29;

//! Zoom in picture during slideshow. Can be used in slideshow.xml window id=2007
constexpr const int ACTION_ZOOM_OUT = 30;

//! Zoom out picture during slideshow. Can be used in slideshow.xml window id=2007
constexpr const int ACTION_ZOOM_IN = 31;

//! Used to toggle between source view and destination view. Can be used in
//! myfiles.xml window < id=3
constexpr const int ACTION_TOGGLE_SOURCE_DEST = 32;

//! Used to toggle between current view and playlist view. Can be used in all mymusic xml files
constexpr const int ACTION_SHOW_PLAYLIST = 33;

//! Used to queue a item to the playlist. Can be used in all mymusic xml files
constexpr const int ACTION_QUEUE_ITEM = 34;

//! Not used anymore
constexpr const int ACTION_REMOVE_ITEM = 35;

//! Not used anymore
constexpr const int ACTION_SHOW_FULLSCREEN = 36;

//! Zoom 1x picture during slideshow. Can be used in slideshow.xml window id=2007
constexpr const int ACTION_ZOOM_LEVEL_NORMAL = 37;

//! Zoom 2x picture during slideshow. Can be used in slideshow.xml window id=2007
constexpr const int ACTION_ZOOM_LEVEL_1 = 38;

//! Zoom 3x picture during slideshow. Can be used in slideshow.xml window id=2007
constexpr const int ACTION_ZOOM_LEVEL_2 = 39;

//! Zoom 4x picture during slideshow. Can be used in slideshow.xml window id=2007
constexpr const int ACTION_ZOOM_LEVEL_3 = 40;

//! Zoom 5x picture during slideshow. Can be used in slideshow.xml window id=2007
constexpr const int ACTION_ZOOM_LEVEL_4 = 41;

//! Zoom 6x picture during slideshow. Can be used in slideshow.xml window id=2007
constexpr const int ACTION_ZOOM_LEVEL_5 = 42;

//! Zoom 7x picture during slideshow. Can be used in slideshow.xml window id=2007
constexpr const int ACTION_ZOOM_LEVEL_6 = 43;

//! Zoom 8x picture during slideshow. Can be used in slideshow.xml window id=2007
constexpr const int ACTION_ZOOM_LEVEL_7 = 44;

//! Zoom 9x picture during slideshow. Can be used in slideshow.xml window id=2007
constexpr const int ACTION_ZOOM_LEVEL_8 = 45;

//< Zoom 10x picture during slideshow. Can be used in slideshow.xml window id=2007
constexpr const int ACTION_ZOOM_LEVEL_9 = 46;

//< Select next arrow. Can be used in: settingsScreenCalibration.xml windowid=11
constexpr const int ACTION_CALIBRATE_SWAP_ARROWS = 47;

//! Reset calibration to defaults. Can be used in: `settingsScreenCalibration.xml`
//! windowid=11/settingsUICalibration.xml windowid=10
constexpr const int ACTION_CALIBRATE_RESET = 48;

//! Analog thumbstick move. Can be used in: `slideshow.xml`
//! windowid=2007/settingsScreenCalibration.xml windowid=11/settingsUICalibration.xml
//! windowid=10
//! @note see also ACTION_ANALOG_MOVE_X_LEFT, ACTION_ANALOG_MOVE_X_RIGHT,
//! ACTION_ANALOG_MOVE_Y_UP, ACTION_ANALOG_MOVE_Y_DOWN
constexpr const int ACTION_ANALOG_MOVE = 49;

//! Rotate current picture clockwise during slideshow. Can be used in
//! slideshow.xml window < id=2007
constexpr const int ACTION_ROTATE_PICTURE_CW = 50;

//! Rotate current picture counterclockwise during slideshow. Can be used in
//! slideshow.xml window id=2007
constexpr const int ACTION_ROTATE_PICTURE_CCW = 51;

//! Decrease subtitle/movie Delay.  Can be used in videoFullScreen.xml window id=2005
constexpr const int ACTION_SUBTITLE_DELAY_MIN = 52;

//! Increase subtitle/movie Delay.  Can be used in videoFullScreen.xml window id=2005
constexpr const int ACTION_SUBTITLE_DELAY_PLUS = 53;

//! Increase avsync delay.  Can be used in videoFullScreen.xml window id=2005
constexpr const int ACTION_AUDIO_DELAY_MIN = 54;

//! Decrease avsync delay.  Can be used in videoFullScreen.xml window id=2005
constexpr const int ACTION_AUDIO_DELAY_PLUS = 55;

//! Select next language in movie.  Can be used in videoFullScreen.xml window id=2005
constexpr const int ACTION_AUDIO_NEXT_LANGUAGE = 56;

//! Switch 2 next resolution. Can b used during screen calibration
//! settingsScreenCalibration.xml windowid=11
constexpr const int ACTION_CHANGE_RESOLUTION = 57;

//! Remote keys 0-9 are used by multiple windows.
//!
//! For example, in videoFullScreen.xml window id=2005 you can enter
//! time (mmss) to jump to particular point in the movie.
//!
//! With spincontrols you can enter a 3-digit number to quickly set the
//! spincontrol to desired value.
//!@{
constexpr const int REMOTE_0 = 58;

//! @see REMOTE_0 about details.
constexpr const int REMOTE_1 = 59;

//! @see REMOTE_0 about details.
constexpr const int REMOTE_2 = 60;

//! @see REMOTE_0 about details.
constexpr const int REMOTE_3 = 61;

//! @see REMOTE_0 about details.
constexpr const int REMOTE_4 = 62;

//! @see REMOTE_0 about details.
constexpr const int REMOTE_5 = 63;

//! @see REMOTE_0 about details.
constexpr const int REMOTE_6 = 64;

//! @see REMOTE_0 about details.
constexpr const int REMOTE_7 = 65;

//! @see REMOTE_0 about details.
constexpr const int REMOTE_8 = 66;

//! @see REMOTE_0 about details.
constexpr const int REMOTE_9 = 67;
//!@}

//! Show player process info (video decoder, pixel format, pvr signal strength
//! and the like
constexpr const int ACTION_PLAYER_PROCESS_INFO = 69;

constexpr const int ACTION_PLAYER_PROGRAM_SELECT = 70;

constexpr const int ACTION_PLAYER_RESOLUTION_SELECT = 71;

//! Jumps a few seconds back during playback of movie. Can be used in videoFullScreen.xml
//! window id=2005
constexpr const int ACTION_SMALL_STEP_BACK = 76;

//! FF in current file played. global action, can be used anywhere
constexpr const int ACTION_PLAYER_FORWARD = 77;

//! RW in current file played. global action, can be used anywhere
constexpr const int ACTION_PLAYER_REWIND = 78;

//! Play current song. Unpauses song and sets playspeed to 1x. global action,
//! can be used anywhere
constexpr const int ACTION_PLAYER_PLAY = 79;

//! Delete current selected item. Can be used in myfiles.xml window id=3 and in
//! myvideoTitle.xml window id=25
constexpr const int ACTION_DELETE_ITEM = 80;

//! Copy current selected item. Can be used in myfiles.xml window id=3
constexpr const int ACTION_COPY_ITEM = 81;

//! Move current selected item. Can be used in myfiles.xml window id=3
constexpr const int ACTION_MOVE_ITEM = 82;

//! Take a screenshot
constexpr const int ACTION_TAKE_SCREENSHOT = 85;

//! Rename item
constexpr const int ACTION_RENAME_ITEM = 87;

constexpr const int ACTION_VOLUME_UP = 88;
constexpr const int ACTION_VOLUME_DOWN = 89;
constexpr const int ACTION_VOLAMP = 90;
constexpr const int ACTION_MUTE = 91;
constexpr const int ACTION_NAV_BACK = 92;
constexpr const int ACTION_VOLAMP_UP = 93;
constexpr const int ACTION_VOLAMP_DOWN = 94;

//! Creates an episode bookmark on the currently playing video file containing
//! more than one episode
constexpr const int ACTION_CREATE_EPISODE_BOOKMARK = 95;

//! Creates a bookmark of the currently playing video file
constexpr const int ACTION_CREATE_BOOKMARK = 96;

//! Goto the next chapter, if not available perform a big step forward
constexpr const int ACTION_CHAPTER_OR_BIG_STEP_FORWARD = 97;

//! Goto the previous chapter, if not available perform a big step back
constexpr const int ACTION_CHAPTER_OR_BIG_STEP_BACK = 98;

//! Switch to next subtitle of movie, but will not enable/disable the subtitles.
//! Can be used in videoFullScreen.xml window id=2005
constexpr const int ACTION_CYCLE_SUBTITLE = 99;

constexpr const int ACTION_MOUSE_START = 100;
constexpr const int ACTION_MOUSE_LEFT_CLICK = 100;
constexpr const int ACTION_MOUSE_RIGHT_CLICK = 101;
constexpr const int ACTION_MOUSE_MIDDLE_CLICK = 102;
constexpr const int ACTION_MOUSE_DOUBLE_CLICK = 103;
constexpr const int ACTION_MOUSE_WHEEL_UP = 104;
constexpr const int ACTION_MOUSE_WHEEL_DOWN = 105;
constexpr const int ACTION_MOUSE_DRAG = 106;
constexpr const int ACTION_MOUSE_MOVE = 107;
constexpr const int ACTION_MOUSE_LONG_CLICK = 108;
constexpr const int ACTION_MOUSE_DRAG_END = 109;
constexpr const int ACTION_MOUSE_END = 109;

constexpr const int ACTION_BACKSPACE = 110;
constexpr const int ACTION_SCROLL_UP = 111;
constexpr const int ACTION_SCROLL_DOWN = 112;
constexpr const int ACTION_ANALOG_FORWARD = 113;
constexpr const int ACTION_ANALOG_REWIND = 114;

constexpr const int ACTION_MOVE_ITEM_UP = 115; //!< move item up in playlist
constexpr const int ACTION_MOVE_ITEM_DOWN = 116; //!< move item down in playlist
constexpr const int ACTION_CONTEXT_MENU = 117; //!< pops up the context menu

// stuff for virtual keyboard shortcuts
constexpr const int ACTION_SHIFT = 118; //!< stuff for virtual keyboard shortcuts
constexpr const int ACTION_SYMBOLS = 119; //!< stuff for virtual keyboard shortcuts
constexpr const int ACTION_CURSOR_LEFT = 120; //!< stuff for virtual keyboard shortcuts
constexpr const int ACTION_CURSOR_RIGHT = 121; //!< stuff for virtual keyboard shortcuts

constexpr const int ACTION_BUILT_IN_FUNCTION = 122;

//! Displays current time, can be used in videoFullScreen.xml window id=2005
constexpr const int ACTION_SHOW_OSD_TIME = 123;

constexpr const int ACTION_ANALOG_SEEK_FORWARD = 124; //!< seeks forward, and displays the seek bar.
constexpr const int ACTION_ANALOG_SEEK_BACK = 125; //!< seeks backward, and displays the seek bar.

constexpr const int ACTION_VIS_PRESET_SHOW = 126;
constexpr const int ACTION_VIS_PRESET_NEXT = 128;
constexpr const int ACTION_VIS_PRESET_PREV = 129;
constexpr const int ACTION_VIS_PRESET_LOCK = 130;
constexpr const int ACTION_VIS_PRESET_RANDOM = 131;
constexpr const int ACTION_VIS_RATE_PRESET_PLUS = 132;
constexpr const int ACTION_VIS_RATE_PRESET_MINUS = 133;

constexpr const int ACTION_SHOW_VIDEOMENU = 134;
constexpr const int ACTION_ENTER = 135;

constexpr const int ACTION_INCREASE_RATING = 136;
constexpr const int ACTION_DECREASE_RATING = 137;

constexpr const int ACTION_NEXT_SCENE = 138; //!< switch to next scene/cutpoint in movie
constexpr const int ACTION_PREV_SCENE = 139; //!< switch to previous scene/cutpoint in movie

constexpr const int ACTION_NEXT_LETTER = 140; //!< jump through a list or container by letter
constexpr const int ACTION_PREV_LETTER = 141;

//! Jump direct to a particular letter using SMS-style input
constexpr const int ACTION_JUMP_SMS2 = 142;
constexpr const int ACTION_JUMP_SMS3 = 143;
constexpr const int ACTION_JUMP_SMS4 = 144;
constexpr const int ACTION_JUMP_SMS5 = 145;
constexpr const int ACTION_JUMP_SMS6 = 146;
constexpr const int ACTION_JUMP_SMS7 = 147;
constexpr const int ACTION_JUMP_SMS8 = 148;
constexpr const int ACTION_JUMP_SMS9 = 149;

constexpr const int ACTION_FILTER_CLEAR = 150;
constexpr const int ACTION_FILTER_SMS2 = 151;
constexpr const int ACTION_FILTER_SMS3 = 152;
constexpr const int ACTION_FILTER_SMS4 = 153;
constexpr const int ACTION_FILTER_SMS5 = 154;
constexpr const int ACTION_FILTER_SMS6 = 155;
constexpr const int ACTION_FILTER_SMS7 = 156;
constexpr const int ACTION_FILTER_SMS8 = 157;
constexpr const int ACTION_FILTER_SMS9 = 158;

constexpr const int ACTION_FIRST_PAGE = 159;
constexpr const int ACTION_LAST_PAGE = 160;

constexpr const int ACTION_AUDIO_DELAY = 161;
constexpr const int ACTION_SUBTITLE_DELAY = 162;
constexpr const int ACTION_MENU = 163;

constexpr const int ACTION_SET_RATING = 164;

constexpr const int ACTION_RECORD = 170;

constexpr const int ACTION_PASTE = 180;
constexpr const int ACTION_NEXT_CONTROL = 181;
constexpr const int ACTION_PREV_CONTROL = 182;
constexpr const int ACTION_CHANNEL_SWITCH = 183;
constexpr const int ACTION_CHANNEL_UP = 184;
constexpr const int ACTION_CHANNEL_DOWN = 185;
constexpr const int ACTION_NEXT_CHANNELGROUP = 186;
constexpr const int ACTION_PREVIOUS_CHANNELGROUP = 187;
constexpr const int ACTION_PVR_PLAY = 188;
constexpr const int ACTION_PVR_PLAY_TV = 189;
constexpr const int ACTION_PVR_PLAY_RADIO = 190;
constexpr const int ACTION_PVR_SHOW_TIMER_RULE = 191;
constexpr const int ACTION_CHANNEL_NUMBER_SEP = 192;
constexpr const int ACTION_PVR_ANNOUNCE_REMINDERS = 193;

constexpr const int ACTION_TOGGLE_FULLSCREEN = 199; //!< switch 2 desktop resolution
constexpr const int ACTION_TOGGLE_WATCHED = 200; //!< Toggle watched status (videos)
constexpr const int ACTION_SCAN_ITEM = 201; //!< scan item
constexpr const int ACTION_TOGGLE_DIGITAL_ANALOG = 202; //!< switch digital <-> analog
constexpr const int ACTION_RELOAD_KEYMAPS = 203; //!< reloads CButtonTranslator's keymaps
constexpr const int ACTION_GUIPROFILE_BEGIN = 204; //!< start the GUIControlProfiler running

//! Teletext Color button <b>Red</b> to control TopText
constexpr const int ACTION_TELETEXT_RED = 215;

//! Teletext Color button <b>Green</b> to control TopText
constexpr const int ACTION_TELETEXT_GREEN = 216;

//! Teletext Color button <b>Yellow</b> to control TopText
constexpr const int ACTION_TELETEXT_YELLOW = 217;

//! Teletext Color button <b>Blue</b> to control TopText
constexpr const int ACTION_TELETEXT_BLUE = 218;

constexpr const int ACTION_INCREASE_PAR = 219;
constexpr const int ACTION_DECREASE_PAR = 220;

constexpr const int ACTION_VSHIFT_UP = 227; //!< shift up video image in VideoPlayer
constexpr const int ACTION_VSHIFT_DOWN = 228; //!< shift down video image in VideoPlayer

//! Play/pause. If playing it pauses, if paused it plays.
constexpr const int ACTION_PLAYER_PLAYPAUSE = 229;

constexpr const int ACTION_SUBTITLE_VSHIFT_UP = 230; //!< shift up subtitles in VideoPlayer
constexpr const int ACTION_SUBTITLE_VSHIFT_DOWN = 231; //!< shift down subtitles in VideoPlayer
constexpr const int ACTION_SUBTITLE_ALIGN = 232; //!< toggle vertical alignment of subtitles

constexpr const int ACTION_FILTER = 233;

constexpr const int ACTION_SWITCH_PLAYER = 234;

constexpr const int ACTION_STEREOMODE_NEXT = 235;
constexpr const int ACTION_STEREOMODE_PREVIOUS = 236;
constexpr const int ACTION_STEREOMODE_TOGGLE = 237; //!< turns 3d mode on/off
constexpr const int ACTION_STEREOMODE_SELECT = 238;
constexpr const int ACTION_STEREOMODE_TOMONO = 239;
constexpr const int ACTION_STEREOMODE_SET = 240;

constexpr const int ACTION_SETTINGS_RESET = 241;
constexpr const int ACTION_SETTINGS_LEVEL_CHANGE = 242;

//! Show autoclosing OSD. Can be used in videoFullScreen.xml window id=2005
constexpr const int ACTION_TRIGGER_OSD = 243;
constexpr const int ACTION_INPUT_TEXT = 244;
constexpr const int ACTION_VOLUME_SET = 245;
constexpr const int ACTION_TOGGLE_COMMSKIP = 246;

//! Browse for subtitle. Can be used in videofullscreen
constexpr const int ACTION_BROWSE_SUBTITLE = 247;

constexpr const int ACTION_PLAYER_RESET = 248; //!< Send a reset command to the active game

constexpr const int ACTION_TOGGLE_FONT = 249; //!< Toggle font. Used in TextViewer dialog

//! Cycle video streams. Used in videofullscreen.
constexpr const int ACTION_VIDEO_NEXT_STREAM = 250;

//! Used to queue an item to the next position in the playlist
constexpr const int ACTION_QUEUE_ITEM_NEXT = 251;

constexpr const int ACTION_HDR_TOGGLE = 260; //!< Toggle display HDR on/off

constexpr const int ACTION_CYCLE_TONEMAP_METHOD = 261; //!< Switch to next tonemap method

//! Show debug info for video (source format, metadata, shaders, render flags and output format)
constexpr const int ACTION_PLAYER_DEBUG_VIDEO = 262;

//! Keyboard is composing a key (sequence started by a dead key press)
constexpr const int ACTION_KEYBOARD_COMPOSING_KEY = 263;
//! Keyboard has canceled the key composition
constexpr const int ACTION_KEYBOARD_COMPOSING_KEY_CANCELLED = 264;
//! Keyboard has finishing the key composition
constexpr const int ACTION_KEYBOARD_COMPOSING_KEY_FINISHED = 265;

//! Tempo change in current file played. global action, can be used anywhere
constexpr const int ACTION_PLAYER_INCREASE_TEMPO = 266;
constexpr const int ACTION_PLAYER_DECREASE_TEMPO = 267;

// Voice actions
constexpr const int ACTION_VOICE_RECOGNIZE = 300;

// Touch actions
constexpr const int ACTION_TOUCH_TAP = 401; //!< touch actions
constexpr const int ACTION_TOUCH_TAP_TEN = 410; //!< touch actions
constexpr const int ACTION_TOUCH_LONGPRESS = 411; //!< touch actions
constexpr const int ACTION_TOUCH_LONGPRESS_TEN = 420; //!< touch actions

constexpr const int ACTION_GESTURE_NOTIFY = 500;
constexpr const int ACTION_GESTURE_BEGIN = 501;

//! sendaction with point and currentPinchScale (fingers together < 1.0 ->
//! fingers apart  > 1.0)
constexpr const int ACTION_GESTURE_ZOOM = 502;
constexpr const int ACTION_GESTURE_ROTATE = 503;
constexpr const int ACTION_GESTURE_PAN = 504;
constexpr const int ACTION_GESTURE_ABORT = 505; //!< gesture was interrupted in unspecified state

constexpr const int ACTION_GESTURE_SWIPE_LEFT = 511;
constexpr const int ACTION_GESTURE_SWIPE_LEFT_TEN = 520;
constexpr const int ACTION_GESTURE_SWIPE_RIGHT = 521;
constexpr const int ACTION_GESTURE_SWIPE_RIGHT_TEN = 530;
constexpr const int ACTION_GESTURE_SWIPE_UP = 531;
constexpr const int ACTION_GESTURE_SWIPE_UP_TEN = 540;
constexpr const int ACTION_GESTURE_SWIPE_DOWN = 541;
constexpr const int ACTION_GESTURE_SWIPE_DOWN_TEN = 550;

//! 5xx is reserved for additional gesture actions
constexpr const int ACTION_GESTURE_END = 599;

/*!
 * @brief Other, non-gesture actions
 */
///@{

//!< analog thumbstick move, horizontal axis, left; see ACTION_ANALOG_MOVE
constexpr const int ACTION_ANALOG_MOVE_X_LEFT = 601;

//!< analog thumbstick move, horizontal axis, right; see ACTION_ANALOG_MOVE
constexpr const int ACTION_ANALOG_MOVE_X_RIGHT = 602;

//!< analog thumbstick move, vertical axis, up; see ACTION_ANALOG_MOVE
constexpr const int ACTION_ANALOG_MOVE_Y_UP = 603;

//!< analog thumbstick move, vertical axis, down; see ACTION_ANALOG_MOVE
constexpr const int ACTION_ANALOG_MOVE_Y_DOWN = 604;

///@}

// The NOOP action can be specified to disable an input event. This is
// useful in user keyboard.xml etc to disable actions specified in the
// system mappings. ERROR action is used to play an error sound
constexpr const int ACTION_ERROR = 998;
constexpr const int ACTION_NOOP = 999;
