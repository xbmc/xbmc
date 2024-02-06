/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#ifndef C_API_GUI_ACTION_IDS_H
#define C_API_GUI_ACTION_IDS_H

/// @defgroup cpp_kodi_gui_Defs_action_ids enum ADDON_ACTION
/// @ingroup cpp_kodi_gui_Defs
/// @brief **Action Id's**\n
/// Actions that we have defined.
///
///@{
enum ADDON_ACTION
{
  /// @ingroup cpp_kodi_gui_key_action_ids
  ///@{

  /// @brief <b>`0  `</b>: None.
  ADDON_ACTION_NONE = 0,

  /// @brief <b>`1  `</b>: Move left.
  ADDON_ACTION_MOVE_LEFT = 1,

  /// @brief <b>`2  `</b>: Move right.
  ADDON_ACTION_MOVE_RIGHT = 2,

  /// @brief <b>`3  `</b>: Move up.
  ADDON_ACTION_MOVE_UP = 3,

  /// @brief <b>`4  `</b>: Move down.
  ADDON_ACTION_MOVE_DOWN = 4,

  /// @brief <b>`5  `</b>: Page up.
  ADDON_ACTION_PAGE_UP = 5,

  /// @brief <b>`6  `</b>: Page down.
  ADDON_ACTION_PAGE_DOWN = 6,

  /// @brief <b>`7  `</b>: Select item.
  ADDON_ACTION_SELECT_ITEM = 7,

  /// @brief <b>`8  `</b>: Highlight item.
  ADDON_ACTION_HIGHLIGHT_ITEM = 8,

  /// @brief <b>`9  `</b>: Parent directory.
  ADDON_ACTION_PARENT_DIR = 9,

  /// @brief <b>`10 `</b>: Previous menu.
  ADDON_ACTION_PREVIOUS_MENU = 10,

  /// @brief <b>`11 `</b>: Show info.
  ADDON_ACTION_SHOW_INFO = 11,

  /// @brief <b>`12 `</b>: Pause.
  ADDON_ACTION_PAUSE = 12,

  /// @brief <b>`13 `</b>: Stop.
  ADDON_ACTION_STOP = 13,

  /// @brief <b>`14 `</b>: Next item.
  ADDON_ACTION_NEXT_ITEM = 14,

  /// @brief <b>`15 `</b>: Previous item.
  ADDON_ACTION_PREV_ITEM = 15,

  /// @brief <b>`16 `</b>: Can be used to specify specific action in a window, Playback control is handled in ADDON_ACTION_PLAYER_*
  ADDON_ACTION_FORWARD = 16,

  /// @brief <b>`17 `</b>: Can be used to specify specific action in a window, Playback control is handled in ADDON_ACTION_PLAYER_*
  ADDON_ACTION_REWIND = 17,

  /// @brief <b>`18 `</b>: Toggle between GUI and movie or GUI and visualisation.
  ADDON_ACTION_SHOW_GUI = 18,

  /// @brief <b>`19 `</b>: Toggle quick-access zoom modes. Can b used in videoFullScreen.zml window id=2005
  ADDON_ACTION_ASPECT_RATIO = 19,

  /// @brief <b>`20 `</b>: Seek +1% in the movie. Can b used in videoFullScreen.xml window id=2005
  ADDON_ACTION_STEP_FORWARD = 20,

  /// @brief <b>`21 `</b>: Seek -1% in the movie. Can b used in videoFullScreen.xml window id=2005
  ADDON_ACTION_STEP_BACK = 21,

  /// @brief <b>`22 `</b>: Seek +10% in the movie. Can b used in videoFullScreen.xml window id=2005
  ADDON_ACTION_BIG_STEP_FORWARD = 22,

  /// @brief <b>`23 `</b>: Seek -10% in the movie. Can b used in videoFullScreen.xml window id=2005
  ADDON_ACTION_BIG_STEP_BACK = 23,

  /// @brief <b>`24 `</b>: Show/hide OSD. Can b used in videoFullScreen.xml window id=2005
  ADDON_ACTION_SHOW_OSD = 24,

  /// @brief <b>`25 `</b>: Turn subtitles on/off. Can b used in videoFullScreen.xml window id=2005
  ADDON_ACTION_SHOW_SUBTITLES = 25,

  /// @brief <b>`26 `</b>: Switch to next subtitle of movie. Can b used in videoFullScreen.xml window id=2005
  ADDON_ACTION_NEXT_SUBTITLE = 26,

  /// @brief <b>`27 `</b>: Show debug info for VideoPlayer
  ADDON_ACTION_PLAYER_DEBUG = 27,

  /// @brief <b>`28 `</b>: Show next picture of slideshow. Can b used in slideshow.xml window id=2007
  ADDON_ACTION_NEXT_PICTURE = 28,

  /// @brief <b>`29 `</b>: Show previous picture of slideshow. Can b used in slideshow.xml window id=2007
  ADDON_ACTION_PREV_PICTURE = 29,

  /// @brief <b>`30 `</b>: Zoom in picture during slideshow. Can b used in slideshow.xml window id=2007
  ADDON_ACTION_ZOOM_OUT = 30,

  /// @brief <b>`31 `</b>: Zoom out picture during slideshow. Can b used in slideshow.xml window id=2007
  ADDON_ACTION_ZOOM_IN = 31,

  /// @brief <b>`32 `</b>: Used to toggle between source view and destination view. Can be used in myfiles.xml window id=3
  ADDON_ACTION_TOGGLE_SOURCE_DEST = 32,

  /// @brief <b>`33 `</b>: Used to toggle between current view and playlist view. Can b used in all mymusic xml files
  ADDON_ACTION_SHOW_PLAYLIST = 33,

  /// @brief <b>`34 `</b>: Used to queue a item to the playlist. Can b used in all mymusic xml files
  ADDON_ACTION_QUEUE_ITEM = 34,

  /// @brief <b>`35 `</b>: Not used anymore
  ADDON_ACTION_REMOVE_ITEM = 35,

  /// @brief <b>`36 `</b>: Not used anymore
  ADDON_ACTION_SHOW_FULLSCREEN = 36,

  /// @brief <b>`37 `</b>: Zoom 1x picture during slideshow. Can b used in slideshow.xml window id=2007
  ADDON_ACTION_ZOOM_LEVEL_NORMAL = 37,

  /// @brief <b>`38 `</b>: Zoom 2x picture during slideshow. Can b used in slideshow.xml window id=2007
  ADDON_ACTION_ZOOM_LEVEL_1 = 38,

  /// @brief <b>`39 `</b>: Zoom 3x picture during slideshow. Can b used in slideshow.xml window id=2007
  ADDON_ACTION_ZOOM_LEVEL_2 = 39,

  /// @brief <b>`40 `</b>: Zoom 4x picture during slideshow. Can b used in slideshow.xml window id=2007
  ADDON_ACTION_ZOOM_LEVEL_3 = 40,

  /// @brief <b>`41 `</b>: Zoom 5x picture during slideshow. Can b used in slideshow.xml window id=2007
  ADDON_ACTION_ZOOM_LEVEL_4 = 41,

  /// @brief <b>`42 `</b>: Zoom 6x picture during slideshow. Can b used in slideshow.xml window id=2007
  ADDON_ACTION_ZOOM_LEVEL_5 = 42,

  /// @brief <b>`43 `</b>: Zoom 7x picture during slideshow. Can b used in slideshow.xml window id=2007
  ADDON_ACTION_ZOOM_LEVEL_6 = 43,

  /// @brief <b>`44 `</b>: Zoom 8x picture during slideshow. Can b used in slideshow.xml window id=2007
  ADDON_ACTION_ZOOM_LEVEL_7 = 44,

  /// @brief <b>`45 `</b>: Zoom 9x picture during slideshow. Can b used in slideshow.xml window id=2007
  ADDON_ACTION_ZOOM_LEVEL_8 = 45,

  /// @brief <b>`46 `</b>: Zoom 10x picture during slideshow. Can b used in slideshow.xml window id=2007
  ADDON_ACTION_ZOOM_LEVEL_9 = 46,

  /// @brief <b>`47 `</b>: Select next arrow. Can b used in: settingsScreenCalibration.xml windowid=11
  ADDON_ACTION_CALIBRATE_SWAP_ARROWS = 47,

  /// @brief <b>`48 `</b>: Reset calibration to defaults. Can b used in: `settingsScreenCalibration.xml` windowid=11/settingsUICalibration.xml windowid=10
  ADDON_ACTION_CALIBRATE_RESET = 48,

  /// @brief <b>`49 `</b>: Analog thumbstick move. Can b used in: `slideshow.xml`
  /// windowid=2007/settingsScreenCalibration.xml windowid=11/settingsUICalibration.xml
  /// windowid=10
  /// @note see also ADDON_ACTION_ANALOG_MOVE_X_LEFT, ADDON_ACTION_ANALOG_MOVE_X_RIGHT,
  /// ADDON_ACTION_ANALOG_MOVE_Y_UP, ADDON_ACTION_ANALOG_MOVE_Y_DOWN
  ADDON_ACTION_ANALOG_MOVE = 49,

  /// @brief <b>`50 `</b>: Rotate current picture clockwise during slideshow. Can be used in slideshow.xml window id=2007
  ADDON_ACTION_ROTATE_PICTURE_CW = 50,

  /// @brief <b>`51 `</b>: Rotate current picture counterclockwise during slideshow. Can be used in slideshow.xml window id=2007
  ADDON_ACTION_ROTATE_PICTURE_CCW = 51,

  /// @brief <b>`52 `</b>: Decrease subtitle/movie Delay.  Can b used in videoFullScreen.xml window id=2005
  ADDON_ACTION_SUBTITLE_DELAY_MIN = 52,

  /// @brief <b>`53 `</b>: Increase subtitle/movie Delay.  Can b used in videoFullScreen.xml window id=2005
  ADDON_ACTION_SUBTITLE_DELAY_PLUS = 53,

  /// @brief <b>`54 `</b>: Increase avsync delay.  Can b used in videoFullScreen.xml window id=2005
  ADDON_ACTION_AUDIO_DELAY_MIN = 54,

  /// @brief <b>`55 `</b>: Decrease avsync delay.  Can b used in videoFullScreen.xml window id=2005
  ADDON_ACTION_AUDIO_DELAY_PLUS = 55,

  /// @brief <b>`56 `</b>: Select next language in movie.  Can b used in videoFullScreen.xml window id=2005
  ADDON_ACTION_AUDIO_NEXT_LANGUAGE = 56,

  /// @brief <b>`57 `</b>: Switch 2 next resolution. Can b used during screen calibration settingsScreenCalibration.xml windowid=11
  ADDON_ACTION_CHANGE_RESOLUTION = 57,

  /// @brief <b>`58 `</b>: remote keys 0-9. are used by multiple windows
  /// for example in videoFullScreen.xml window id=2005 you can
  /// enter time (mmss) to jump to particular point in the movie
  /// with spincontrols you can enter 3digit number to quickly set
  /// spincontrol to desired value
  ///
  /// Remote key 0
  ADDON_ACTION_REMOTE_0 = 58,

  /// @brief <b>`59 `</b>: Remote key 1
  ADDON_ACTION_REMOTE_1 = 59,

  /// @brief <b>`60 `</b>: Remote key 2
  ADDON_ACTION_REMOTE_2 = 60,

  /// @brief <b>`61 `</b>: Remote key 3
  ADDON_ACTION_REMOTE_3 = 61,

  /// @brief <b>`62 `</b>: Remote key 4
  ADDON_ACTION_REMOTE_4 = 62,

  /// @brief <b>`63 `</b>: Remote key 5
  ADDON_ACTION_REMOTE_5 = 63,

  /// @brief <b>`64 `</b>: Remote key 6
  ADDON_ACTION_REMOTE_6 = 64,

  /// @brief <b>`65 `</b>: Remote key 7
  ADDON_ACTION_REMOTE_7 = 65,

  /// @brief <b>`66 `</b>: Remote key 8
  ADDON_ACTION_REMOTE_8 = 66,

  /// @brief <b>`67 `</b>: Remote key 9
  ADDON_ACTION_REMOTE_9 = 67,

  /// @brief <b>`69 `</b>: Show player process info (video decoder, pixel format, pvr signal strength and the like
  ADDON_ACTION_PLAYER_PROCESS_INFO = 69,

  /// @brief <b>`70 `</b>: Program select.
  ADDON_ACTION_PLAYER_PROGRAM_SELECT = 70,

  /// @brief <b>`71 `</b>: Resolution select.
  ADDON_ACTION_PLAYER_RESOLUTION_SELECT = 71,

  /// @brief <b>`76 `</b>: Jumps a few seconds back during playback of movie. Can b used in videoFullScreen.xml window id=2005
  ADDON_ACTION_SMALL_STEP_BACK = 76,

  /// @brief <b>`77 `</b>: FF in current file played. global action, can be used anywhere
  ADDON_ACTION_PLAYER_FORWARD = 77,

  /// @brief <b>`78 `</b>: RW in current file played. global action, can be used anywhere
  ADDON_ACTION_PLAYER_REWIND = 78,

  /// @brief <b>`79 `</b>: Play current song. Unpauses song and sets playspeed to 1x. global action, can be used anywhere
  ADDON_ACTION_PLAYER_PLAY = 79,

  /// @brief <b>`80 `</b>: Delete current selected item. Can be used in myfiles.xml window id=3 and in myvideoTitle.xml window id=25
  ADDON_ACTION_DELETE_ITEM = 80,

  /// @brief <b>`81 `</b>: Copy current selected item. Can be used in myfiles.xml window id=3
  ADDON_ACTION_COPY_ITEM = 81,

  /// @brief <b>`82 `</b>: move current selected item. Can be used in myfiles.xml window id=3
  ADDON_ACTION_MOVE_ITEM = 82,

  /// @brief <b>`85 `</b>: Take a screenshot.
  ADDON_ACTION_TAKE_SCREENSHOT = 85,

  /// @brief <b>`87 `</b>: Rename item.
  ADDON_ACTION_RENAME_ITEM = 87,

  /// @brief <b>`87 `</b>: Volume up.
  ADDON_ACTION_VOLUME_UP = 88,

  /// @brief <b>`87 `</b>: Volume down.
  ADDON_ACTION_VOLUME_DOWN = 89,

  /// @brief <b>`90 `</b>: Volume amplication.
  ADDON_ACTION_VOLAMP = 90,

  /// @brief <b>`90 `</b>: Mute.
  ADDON_ACTION_MUTE = 91,

  /// @brief <b>`90 `</b>: Nav back.
  ADDON_ACTION_NAV_BACK = 92,

  /// @brief <b>`90 `</b>: Volume amp up,
  ADDON_ACTION_VOLAMP_UP = 93,

  /// @brief <b>`94 `</b>: Volume amp down.
  ADDON_ACTION_VOLAMP_DOWN = 94,

  /// @brief <b>`95 `</b>: Creates an episode bookmark on the currently playing video file containing more than one
  /// episode
  ADDON_ACTION_CREATE_EPISODE_BOOKMARK = 95,

  /// @brief <b>`96 `</b>: Creates a bookmark of the currently playing video file
  ADDON_ACTION_CREATE_BOOKMARK = 96,

  /// @brief <b>`97 `</b>: Goto the next chapter, if not available perform a big step forward
  ADDON_ACTION_CHAPTER_OR_BIG_STEP_FORWARD = 97,

  /// @brief <b>`98 `</b>: Goto the previous chapter, if not available perform a big step back
  ADDON_ACTION_CHAPTER_OR_BIG_STEP_BACK = 98,

  /// @brief <b>`99 `</b>: Switch to next subtitle of movie, but will not enable/disable the subtitles. Can be used
  /// in videoFullScreen.xml window id=2005
  ADDON_ACTION_CYCLE_SUBTITLE = 99,

  /// @brief <b>`100`</b>: Mouse action values start.
  ///
  /// Ends with @ref ADDON_ACTION_MOUSE_END.
  ADDON_ACTION_MOUSE_START = 100,

  /// @brief <b>`100`</b>: Mouse left click.
  ADDON_ACTION_MOUSE_LEFT_CLICK = 100,

  /// @brief <b>`101`</b>: Mouse right click.
  ADDON_ACTION_MOUSE_RIGHT_CLICK = 101,

  /// @brief <b>`102`</b>: Mouse middle click.
  ADDON_ACTION_MOUSE_MIDDLE_CLICK = 102,

  /// @brief <b>`103`</b>: Mouse double click.
  ADDON_ACTION_MOUSE_DOUBLE_CLICK = 103,

  /// @brief <b>`104`</b>: Mouse wheel up.
  ADDON_ACTION_MOUSE_WHEEL_UP = 104,

  /// @brief <b>`105`</b>: Mouse wheel down.
  ADDON_ACTION_MOUSE_WHEEL_DOWN = 105,

  /// @brief <b>`106`</b>: Mouse drag.
  ADDON_ACTION_MOUSE_DRAG = 106,

  /// @brief <b>`107`</b>: Mouse move.
  ADDON_ACTION_MOUSE_MOVE = 107,

  /// @brief <b>`108`</b>: Mouse long click.
  ADDON_ACTION_MOUSE_LONG_CLICK = 108,

  /// @brief <b>`109`</b>: Mouse drag end.
  ADDON_ACTION_MOUSE_DRAG_END = 109,

  /// @brief <b>`109`</b>: Mouse action values end.
  ///
  /// Starts with @ref ADDON_ACTION_MOUSE_START.
  ADDON_ACTION_MOUSE_END = 109,

  /// @brief <b>`110`</b>: Backspace.
  ADDON_ACTION_BACKSPACE = 110,

  /// @brief <b>`111`</b>: Scroll up.
  ADDON_ACTION_SCROLL_UP = 111,

  /// @brief <b>`112`</b>: Scroll down.
  ADDON_ACTION_SCROLL_DOWN = 112,

  /// @brief <b>`113`</b>: Analog forward.
  ADDON_ACTION_ANALOG_FORWARD = 113,

  /// @brief <b>`114`</b>: Analog rewind.
  ADDON_ACTION_ANALOG_REWIND = 114,

  /// @brief <b>`115`</b>: move item up in playlist
  ADDON_ACTION_MOVE_ITEM_UP = 115,

  /// @brief <b>`116`</b>: move item down in playlist
  ADDON_ACTION_MOVE_ITEM_DOWN = 116,

  /// @brief <b>`117`</b>: pops up the context menu
  ADDON_ACTION_CONTEXT_MENU = 117,

  /// @brief <b>`118`</b>: stuff for virtual keyboard shortcuts
  ADDON_ACTION_SHIFT = 118,

  /// @brief <b>`119`</b>: stuff for virtual keyboard shortcuts
  ADDON_ACTION_SYMBOLS = 119,

  /// @brief <b>`120`</b>: stuff for virtual keyboard shortcuts
  ADDON_ACTION_CURSOR_LEFT = 120,

  /// @brief <b>`121`</b>: stuff for virtual keyboard shortcuts
  ADDON_ACTION_CURSOR_RIGHT = 121,

  /// @brief <b>`122`</b>: Build in function
  ADDON_ACTION_BUILT_IN_FUNCTION = 122,

  /// @brief <b>`114`</b>: Displays current time, can be used in videoFullScreen.xml window id=2005
  ADDON_ACTION_SHOW_OSD_TIME = 123,

  /// @brief <b>`124`</b>: Seeks forward, and displays the seek bar.
  ADDON_ACTION_ANALOG_SEEK_FORWARD = 124,

  /// @brief <b>`125`</b>: Seeks backward, and displays the seek bar.
  ADDON_ACTION_ANALOG_SEEK_BACK = 125,

  /// @brief <b>`126`</b>: Visualization preset show.
  ADDON_ACTION_VIS_PRESET_SHOW = 126,

  /// @brief <b>`128`</b>: Visualization preset next.
  ADDON_ACTION_VIS_PRESET_NEXT = 128,

  /// @brief <b>`129`</b>: Visualization preset previous.
  ADDON_ACTION_VIS_PRESET_PREV = 129,

  /// @brief <b>`130`</b>: Visualization preset lock.
  ADDON_ACTION_VIS_PRESET_LOCK = 130,

  /// @brief <b>`131`</b>: Visualization preset random.
  ADDON_ACTION_VIS_PRESET_RANDOM = 131,

  /// @brief <b>`132`</b>: Visualization preset plus.
  ADDON_ACTION_VIS_RATE_PRESET_PLUS = 132,

  /// @brief <b>`133`</b>: Visualization preset minus.
  ADDON_ACTION_VIS_RATE_PRESET_MINUS = 133,

  /// @brief <b>`134`</b>: Show Videomenu
  ADDON_ACTION_SHOW_VIDEOMENU = 134,

  /// @brief <b>`135`</b>: Enter.
  ADDON_ACTION_ENTER = 135,

  /// @brief <b>`136`</b>: Increase rating.
  ADDON_ACTION_INCREASE_RATING = 136,

  /// @brief <b>`137`</b>: Decrease rating.
  ADDON_ACTION_DECREASE_RATING = 137,

  /// @brief <b>`138`</b>: Switch to next scene/cutpoint in movie.
  ADDON_ACTION_NEXT_SCENE = 138,

  /// @brief <b>`139`</b>: Switch to previous scene/cutpoint in movie.
  ADDON_ACTION_PREV_SCENE = 139,

  /// @brief <b>`140`</b>: Jump through a list or container to next letter.
  ADDON_ACTION_NEXT_LETTER = 140,

  /// @brief <b>`141`</b>: Jump through a list or container to previous letter.
  ADDON_ACTION_PREV_LETTER = 141,

  /// @brief <b>`142`</b>: Jump direct to a particular letter using SMS-style input
  ///
  /// Jump to SMS2.
  ADDON_ACTION_JUMP_SMS2 = 142,

  /// @brief <b>`143`</b>: Jump to SMS3.
  ADDON_ACTION_JUMP_SMS3 = 143,

  /// @brief <b>`144`</b>: Jump to SMS4.
  ADDON_ACTION_JUMP_SMS4 = 144,

  /// @brief <b>`145`</b>: Jump to SMS5.
  ADDON_ACTION_JUMP_SMS5 = 145,

  /// @brief <b>`146`</b>: Jump to SMS6.
  ADDON_ACTION_JUMP_SMS6 = 146,

  /// @brief <b>`147`</b>: Jump to SMS7.
  ADDON_ACTION_JUMP_SMS7 = 147,

  /// @brief <b>`148`</b>: Jump to SMS8.
  ADDON_ACTION_JUMP_SMS8 = 148,

  /// @brief <b>`149`</b>: Jump to SMS9.
  ADDON_ACTION_JUMP_SMS9 = 149,

  /// @brief <b>`150`</b>: Filter clear.
  ADDON_ACTION_FILTER_CLEAR = 150,

  /// @brief <b>`151`</b>: Filter SMS2.
  ADDON_ACTION_FILTER_SMS2 = 151,

  /// @brief <b>`152`</b>: Filter SMS3.
  ADDON_ACTION_FILTER_SMS3 = 152,

  /// @brief <b>`153`</b>: Filter SMS4.
  ADDON_ACTION_FILTER_SMS4 = 153,

  /// @brief <b>`154`</b>: Filter SMS5.
  ADDON_ACTION_FILTER_SMS5 = 154,

  /// @brief <b>`155`</b>: Filter SMS6.
  ADDON_ACTION_FILTER_SMS6 = 155,

  /// @brief <b>`156`</b>: Filter SMS7.
  ADDON_ACTION_FILTER_SMS7 = 156,

  /// @brief <b>`157`</b>: Filter SMS8.
  ADDON_ACTION_FILTER_SMS8 = 157,

  /// @brief <b>`158`</b>: Filter SMS9.
  ADDON_ACTION_FILTER_SMS9 = 158,

  /// @brief <b>`159`</b>: First page.
  ADDON_ACTION_FIRST_PAGE = 159,

  /// @brief <b>`160`</b>: Last page.
  ADDON_ACTION_LAST_PAGE = 160,

  /// @brief <b>`161`</b>: Audio delay.
  ADDON_ACTION_AUDIO_DELAY = 161,

  /// @brief <b>`162`</b>: Subtitle delay.
  ADDON_ACTION_SUBTITLE_DELAY = 162,

  /// @brief <b>`163`</b>: Menu.
  ADDON_ACTION_MENU = 163,

  /// @brief <b>`164`</b>: Set rating.
  ADDON_ACTION_SET_RATING = 164,

  /// @brief <b>`170`</b>: Record.
  ADDON_ACTION_RECORD = 170,

  /// @brief <b>`180`</b>: Paste.
  ADDON_ACTION_PASTE = 180,

  /// @brief <b>`181`</b>: Next control.
  ADDON_ACTION_NEXT_CONTROL = 181,

  /// @brief <b>`182`</b>: Previous control.
  ADDON_ACTION_PREV_CONTROL = 182,

  /// @brief <b>`183`</b>: Channel switch.
  ADDON_ACTION_CHANNEL_SWITCH = 183,

  /// @brief <b>`184`</b>: Channel up.
  ADDON_ACTION_CHANNEL_UP = 184,

  /// @brief <b>`185`</b>: Channel down.
  ADDON_ACTION_CHANNEL_DOWN = 185,

  /// @brief <b>`186`</b>: Next channel group.
  ADDON_ACTION_NEXT_CHANNELGROUP = 186,

  /// @brief <b>`187`</b>: Previous channel group.
  ADDON_ACTION_PREVIOUS_CHANNELGROUP = 187,

  /// @brief <b>`188`</b>: PVR play.
  ADDON_ACTION_PVR_PLAY = 188,

  /// @brief <b>`189`</b>: PVR play TV.
  ADDON_ACTION_PVR_PLAY_TV = 189,

  /// @brief <b>`190`</b>: PVR play radio.
  ADDON_ACTION_PVR_PLAY_RADIO = 190,

  /// @brief <b>`191`</b>: PVR show timer rule.
  ADDON_ACTION_PVR_SHOW_TIMER_RULE = 191,

  /// @brief <b>`192`</b>: Channel number sep
  ADDON_ACTION_CHANNEL_NUMBER_SEP = 192,

  /// @brief <b>`193`</b>: PVR announce reminders
  ADDON_ACTION_PVR_ANNOUNCE_REMINDERS = 193,

  /// @brief <b>`199`</b>: Switch 2 desktop resolution
  ADDON_ACTION_TOGGLE_FULLSCREEN = 199,

  /// @brief <b>`200`</b>: Toggle watched status (videos)
  ADDON_ACTION_TOGGLE_WATCHED = 200,

  /// @brief <b>`201`</b>: Scan item
  ADDON_ACTION_SCAN_ITEM = 201,

  /// @brief <b>`202`</b>: Switch digital <-> analog
  ADDON_ACTION_TOGGLE_DIGITAL_ANALOG = 202,

  /// @brief <b>`203`</b>: Reloads CButtonTranslator's keymaps
  ADDON_ACTION_RELOAD_KEYMAPS = 203,

  /// @brief <b>`204`</b>: Start the GUIControlProfiler running
  ADDON_ACTION_GUIPROFILE_BEGIN = 204,

  /// @brief <b>`215`</b>: Teletext Color button <b>Red</b> to control TopText
  ADDON_ACTION_TELETEXT_RED = 215,

  /// @brief <b>`216`</b>: Teletext Color button <b>Green</b> to control TopText
  ADDON_ACTION_TELETEXT_GREEN = 216,

  /// @brief <b>`217`</b>: Teletext Color button <b>Yellow</b> to control TopText
  ADDON_ACTION_TELETEXT_YELLOW = 217,

  /// @brief <b>`218`</b>: Teletext Color button <b>Blue</b> to control TopText
  ADDON_ACTION_TELETEXT_BLUE = 218,

  /// @brief <b>`219`</b>: Increase par.
  ADDON_ACTION_INCREASE_PAR = 219,

  /// @brief <b>`220`</b>: Decrease par.
  ADDON_ACTION_DECREASE_PAR = 220,

  /// @brief <b>`227`</b>: Shift up video image in VideoPlayer
  ADDON_ACTION_VSHIFT_UP = 227,

  /// @brief <b>`228`</b>: Shift down video image in VideoPlayer
  ADDON_ACTION_VSHIFT_DOWN = 228,

  /// @brief <b>`229`</b>: Play/pause. If playing it pauses, if paused it plays.
  ADDON_ACTION_PLAYER_PLAYPAUSE = 229,

  /// @brief <b>`230`</b>: Shift up subtitles in VideoPlayer
  ADDON_ACTION_SUBTITLE_VSHIFT_UP = 230,

  /// @brief <b>`231`</b>: Shift down subtitles in VideoPlayer
  ADDON_ACTION_SUBTITLE_VSHIFT_DOWN = 231,

  /// @brief <b>`232`</b>: Toggle vertical alignment of subtitles
  ADDON_ACTION_SUBTITLE_ALIGN = 232,

  /// @brief <b>`233`</b>: Filter.
  ADDON_ACTION_FILTER = 233,

  /// @brief <b>`234`</b>: Switch player.
  ADDON_ACTION_SWITCH_PLAYER = 234,

  /// @brief <b>`235`</b>: Stereo mode next.
  ADDON_ACTION_STEREOMODE_NEXT = 235,

  /// @brief <b>`236`</b>: Stereo mode previous.
  ADDON_ACTION_STEREOMODE_PREVIOUS = 236,

  /// @brief <b>`237`</b>: Turns 3d mode on/off.
  ADDON_ACTION_STEREOMODE_TOGGLE = 237,

  /// @brief <b>`238`</b>: Stereo mode select.
  ADDON_ACTION_STEREOMODE_SELECT = 238,

  /// @brief <b>`239`</b>: Stereo mode to mono.
  ADDON_ACTION_STEREOMODE_TOMONO = 239,

  /// @brief <b>`240`</b>: Stereo mode set.
  ADDON_ACTION_STEREOMODE_SET = 240,

  /// @brief <b>`241`</b>: Settings reset.
  ADDON_ACTION_SETTINGS_RESET = 241,

  /// @brief <b>`242`</b>: Settings level change.
  ADDON_ACTION_SETTINGS_LEVEL_CHANGE = 242,

  /// @brief <b>`243`</b>: Show autoclosing OSD. Can b used in videoFullScreen.xml window id=2005
  ADDON_ACTION_TRIGGER_OSD = 243,

  /// @brief <b>`244`</b>: Input text.
  ADDON_ACTION_INPUT_TEXT = 244,

  /// @brief <b>`245`</b>: Volume set.
  ADDON_ACTION_VOLUME_SET = 245,

  /// @brief <b>`246`</b>: Toggle commercial skip.
  ADDON_ACTION_TOGGLE_COMMSKIP = 246,

  /// @brief <b>`247`</b>: Browse for subtitle. Can be used in videofullscreen
  ADDON_ACTION_BROWSE_SUBTITLE = 247,

  /// @brief <b>`248`</b>: Send a reset command to the active game
  ADDON_ACTION_PLAYER_RESET = 248,

  /// @brief <b>`249`</b>: Toggle font. Used in TextViewer dialog
  ADDON_ACTION_TOGGLE_FONT = 249,

  /// @brief <b>`250`</b>: Cycle video streams. Used in videofullscreen.
  ADDON_ACTION_VIDEO_NEXT_STREAM = 250,

  /// @brief <b>`251`</b>: Used to queue an item to the next position in the playlist
  ADDON_ACTION_QUEUE_ITEM_NEXT = 251,

  /// @brief <b>`260`</b>: Toggle display HDR on/off
  ADDON_ACTION_HDR_TOGGLE = 260,

  /// @brief <b>`266`</b>: Tempo increase in current file played. global action, can be used anywhere
  ADDON_ACTION_PLAYER_INCREASE_TEMPO = 266,

  /// @brief <b>`267`</b>: Tempo decrease in current file played. global action, can be used anywhere
  ADDON_ACTION_PLAYER_DECREASE_TEMPO = 267,

  /// @brief <b>`300`</b>: Voice actions
  ADDON_ACTION_VOICE_RECOGNIZE = 300,

  // Number 347 used om front by ADDON_ACTION_BROWSE_SUBTITLE

  /// @brief <b>`401`</b>: Touch actions
  ADDON_ACTION_TOUCH_TAP = 401,

  /// @brief <b>`410`</b>: Touch actions
  ADDON_ACTION_TOUCH_TAP_TEN = 410,

  /// @brief <b>`411`</b>: Touch actions
  ADDON_ACTION_TOUCH_LONGPRESS = 411,

  /// @brief <b>`412`</b>: Touch actions
  ADDON_ACTION_TOUCH_LONGPRESS_TEN = 420,

  /// @brief <b>`500`</b>: Gesture notify.
  ADDON_ACTION_GESTURE_NOTIFY = 500,

  /// @brief <b>`501`</b>: Gesture begin.
  ADDON_ACTION_GESTURE_BEGIN = 501,

  /// @brief <b>`502`</b>: Send action with point and currentPinchScale (fingers together < 1.0 -> fingers apart > 1.0)
  ADDON_ACTION_GESTURE_ZOOM = 502,

  /// @brief <b>`503`</b>: Gesture rotate.
  ADDON_ACTION_GESTURE_ROTATE = 503,

  /// @brief <b>`504`</b>: Gesture pan.
  ADDON_ACTION_GESTURE_PAN = 504,

  /// @brief <b>`505`</b>: Gesture was interrupted in unspecified state
  ADDON_ACTION_GESTURE_ABORT = 505,

  /// @brief <b>`511`</b>: Gesture swipe left.
  ADDON_ACTION_GESTURE_SWIPE_LEFT = 511,

  /// @brief <b>`520`</b>: Gesture swipe left ten
  ADDON_ACTION_GESTURE_SWIPE_LEFT_TEN = 520,

  /// @brief <b>`521`</b>: Gesture swipe right
  ADDON_ACTION_GESTURE_SWIPE_RIGHT = 521,

  /// @brief <b>`530`</b>: Gesture swipe right ten
  ADDON_ACTION_GESTURE_SWIPE_RIGHT_TEN = 530,

  /// @brief <b>`531`</b>: Gesture swipe up
  ADDON_ACTION_GESTURE_SWIPE_UP = 531,

  /// @brief <b>`540`</b>: Gesture swipe up ten
  ADDON_ACTION_GESTURE_SWIPE_UP_TEN = 540,

  /// @brief <b>`541`</b>: Gesture swipe down.
  ADDON_ACTION_GESTURE_SWIPE_DOWN = 541,

  /// @brief <b>`550`</b>: Gesture swipe down ten.
  ADDON_ACTION_GESTURE_SWIPE_DOWN_TEN = 550,

  /// @brief <b>`599`</b>: 5xx is reserved for additional gesture actions
  ADDON_ACTION_GESTURE_END = 599,

  // other, non-gesture actions

  /// @brief <b>`601`</b>: Analog thumbstick move, horizontal axis, left; see ADDON_ACTION_ANALOG_MOVE
  ADDON_ACTION_ANALOG_MOVE_X_LEFT = 601,

  /// @brief <b>`602`</b>: Analog thumbstick move, horizontal axis, right; see ADDON_ACTION_ANALOG_MOVE
  ADDON_ACTION_ANALOG_MOVE_X_RIGHT = 602,

  /// @brief <b>`603`</b>: Analog thumbstick move, vertical axis, up; see ADDON_ACTION_ANALOG_MOVE
  ADDON_ACTION_ANALOG_MOVE_Y_UP = 603,

  /// @brief <b>`604`</b>: Analog thumbstick move, vertical axis, down; see ADDON_ACTION_ANALOG_MOVE
  ADDON_ACTION_ANALOG_MOVE_Y_DOWN = 604,

  /// @brief <b>`998`</b>: ERROR action is used to play an error sound.
  ADDON_ACTION_ERROR = 998,

  /// @brief <b>`999`</b>: The NOOP action can be specified to disable an input event. This is
  /// useful in user keyboard.xml etc to disable actions specified in the
  /// system mappings.
  ADDON_ACTION_NOOP = 999
  ///@}
};
///@}

#endif /* !C_API_GUI_ACTION_IDS_H */
