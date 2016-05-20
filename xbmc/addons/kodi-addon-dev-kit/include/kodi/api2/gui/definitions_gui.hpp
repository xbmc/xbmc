#pragma once
/*
 *      Copyright (C) 2016 Team KODI
 *      http://kodi.tv
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
 *  along with KODI; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "../definitions-all.hpp"

// NEED TO REMOVED AFTER REMOVE OF API LEVEL 1!!!
#define KODI_GUILIB_API_VERSION "5.11.0"
#define KODI_GUILIB_MIN_API_VERSION "5.10.0"
#undef ADDON_ACTION_PREVIOUS_MENU
#undef ADDON_ACTION_NAV_BACK

API_NAMESPACE

namespace KodiAPI
{
extern "C"
{

  /*!
  \defgroup CPP_KodiAPI_GUI 4. GUI
  \ingroup cpp
  \brief <b><em>Graphical functions for Windows and Dialogs to show</em></b>
  */

  //============================================================================
  ///
  /// \ingroup CPP_KodiAPI_GUI_CControlRendering_Defs CPP_KodiAPI_GUI_CWindow_Defs
  /// @{
  /// @brief Handle to use as independent pointer for GUI
  typedef void* GUIHANDLE;
  /// @}
  //----------------------------------------------------------------------------


  //============================================================================
  ///
  /// \ingroup CPP_KodiAPI_GUI_CWindow_Defs
  /// @{
  /// @brief flags for text font alignment
  ///
  /// Flags are used as bits to have several together, e.g. `ADDON_FONT_LEFT | ADDON_FONT_CENTER_Y`
  typedef enum AddonGUIFontAlignment
  {
    /// Align X left
    ADDON_FONT_LEFT = 0x00000000,
    /// Align X right
    ADDON_FONT_RIGHT = 0x00000001,
    /// Align X center
    ADDON_FONT_CENTER_X = 0x00000002,
    /// Align Y center
    ADDON_FONT_CENTER_Y = 0x00000004,
    /// Truncated text
    ADDON_FONT_TRUNCATED = 0x00000008,
    /// Justify text
    ADDON_FONT_JUSTIFIED = 0x00000010
  } AddonGUIFontAlignment;
  /// @}
  //----------------------------------------------------------------------------


  //============================================================================
  ///
  /// \ingroup CPP_KodiAPI_GUI_CListItem_Defs
  /// @{
  /// @anchor kodi_gui_listitem_iconoverlay
  /// @brief Overlay icons used on a list item
  enum AddonGUIIconOverlay
  {
    /// Value **0** - No overlay icon
    ADDON_ICON_OVERLAY_NONE = 0,
    /// Value **1** - Compressed *.rar files
    ADDON_ICON_OVERLAY_RAR,
    /// Value **2** - Compressed *.zip files
    ADDON_ICON_OVERLAY_ZIP,
    /// Value **3** - Locked files
    ADDON_ICON_OVERLAY_LOCKED,
    /// Value **4** - For not watched files
    ADDON_ICON_OVERLAY_UNWATCHED,
    /// Value **5** - For seen files
    ADDON_ICON_OVERLAY_WATCHED,
    /// Value **6** - Is on hard disk stored
    ADDON_ICON_OVERLAY_HD
  };
  /// @}
  //----------------------------------------------------------------------------


  //============================================================================
  ///
  /// \ingroup CPP_KodiAPI_GUI_CControlEdit_Defs
  /// @{
  /// @anchor AddonGUIInputType
  /// @brief Text input types used on CControlEdit
  enum AddonGUIInputType
  {
    /// Text inside edit control only readable
    ADDON_INPUT_TYPE_READONLY = -1,
    /// Normal text entries
    ADDON_INPUT_TYPE_TEXT = 0,
    /// To use on edit control only numeric numbers
    ADDON_INPUT_TYPE_NUMBER,
    /// To insert seconds
    ADDON_INPUT_TYPE_SECONDS,
    /// To insert time
    ADDON_INPUT_TYPE_TIME,
    /// To insert a date
    ADDON_INPUT_TYPE_DATE,
    /// Used for write in IP addresses
    ADDON_INPUT_TYPE_IPADDRESS,
    /// Text field used as password entry field with not visible text
    ADDON_INPUT_TYPE_PASSWORD,
    /// Text field used as password entry field with not visible text but
    /// returned as MD5 value
    ADDON_INPUT_TYPE_PASSWORD_MD5,
    /// Use text field for search purpose
    ADDON_INPUT_TYPE_SEARCH,
    /// Text field as filter
    ADDON_INPUT_TYPE_FILTER,
    /// 
    ADDON_INPUT_TYPE_PASSWORD_NUMBER_VERIFY_NEW
  };
  /// @}
  //----------------------------------------------------------------------------


  //============================================================================
  ///
  /// \ingroup kodi_addon_key_action_ids CPP_KodiAPI_GUI_CWindow_Defs
  /// @{
  /// @brief Actions that we have defined.
  ///
  enum AddonGUIActionType
  {
    ///
    ADDON_ACTION_NONE = 0,

    /// @brief Action string: <b><c>Left</c></b>
    ///
    /// Move left off a control.
    ///
    ADDON_ACTION_MOVE_LEFT = 1,

    /// @brief Action string: <b><c>Right</c></b>
    ///
    /// Move right off a control.
    ADDON_ACTION_MOVE_RIGHT = 2,

    /// @brief Action string: <b><c>Up</c></b>
    ///
    /// Move up off a control.
    ADDON_ACTION_MOVE_UP = 3,

    /// @brief Action string: <b><c>Down</c></b>
    ///
    /// Move down off a control.
    ADDON_ACTION_MOVE_DOWN = 4,

    /// @brief Action string: <b><c>PageUp</c></b>
    ///
    /// Scroll up on page in a list, thumb, or text view.
    ADDON_ACTION_PAGE_UP = 5,

    /// @brief Action string: <b><c>PageDown</c></b>
    ///
    /// Scroll down on page in a list, thumb, or text view.
    ADDON_ACTION_PAGE_DOWN = 6,

    /// @brief Action string: <b><c>Select</c></b>
    ///
    /// Select a button, or an item from a list of thumb view.
    ADDON_ACTION_SELECT_ITEM = 7,

    /// @brief Action string: <b><c>Highlight</c></b>
    ///
    /// Highlight an item in a list or thumb view.
    ADDON_ACTION_HIGHLIGHT_ITEM = 8,

    /// @brief Action string: <b><c>ParentDir</c></b>
    ///
    /// Go up a folder to the parent folder.
    ADDON_ACTION_PARENT_DIR = 9,

    /// @brief Action string: <b><c>PreviousMenu</c></b>
    ///
    /// Go back to the previous menu screen.
    ADDON_ACTION_PREVIOUS_MENU = 10,

    /// @brief Action string: <b><c>Info</c></b>
    ///
    /// Show the information about the currently highlighted item, or currently
    /// playing item.
    ADDON_ACTION_SHOW_INFO = 11,

    /// @brief Action string: <b><c>Pause</c></b>
    ///
    /// Pause the currently playing item.
    ADDON_ACTION_PAUSE = 12,

    /// @brief Action string: <b><c>Stop</c></b>
    ///
    /// Stop the currently playing item.
    ADDON_ACTION_STOP = 13,

    /// @brief Action string: <b><c>SkipNext</c></b>
    ///
    /// Skip to next video in playlist. If no playlist, then skip to next chapter
    /// in video.
    ADDON_ACTION_NEXT_ITEM = 14,

    /// @brief Action string: <b><c>SkipPrevious</c></b>
    ///
    /// Skip to previous video in playlist. If no playlist, then skip to
    /// previous chapter in video.
    ADDON_ACTION_PREV_ITEM = 15,

    /// Can be used to specify specific action in a window, Playback control is
    /// handled in ACTION_PLAYER_*
    ADDON_ACTION_FORWARD = 16,

    /// Can be used to specify specific action in a window, Playback control is
    /// handled in ACTION_PLAYER_*
    ADDON_ACTION_REWIND = 17,


    /// @brief Action string: <b><c>Fullscreen</c></b>
    ///
    /// toggle between GUI and movie or GUI and visualisation.
    ADDON_ACTION_SHOW_GUI = 18,

    /// @brief Action string: <b><c>AspectRatio</c></b>
    ///
    /// toggle quick-access zoom modes. Can b used in videoFullScreen.xml
    /// window id=2005
    ADDON_ACTION_ASPECT_RATIO = 19,

    /// @brief Action string: <b><c>StepForward</c></b>
    ///
    /// seek +1% in the movie. Can b used in videoFullScreen.xml window id=2005
    ADDON_ACTION_STEP_FORWARD = 20,

    /// @brief Action string: <b><c>StepBack</c></b>
    ///
    /// seek -1% in the movie. Can b used in videoFullScreen.xml window id=2005
    ADDON_ACTION_STEP_BACK = 21,

    /// @brief Action string: <b><c>BigStepForward</c></b>
    ///
    /// seek +10% in the movie. Can b used in videoFullScreen.xml window id=2005
    ADDON_ACTION_BIG_STEP_FORWARD = 22,

    /// @brief Action string: <b><c>BigStepBack</c></b>
    ///
    /// seek -10% in the movie. Can b used in videoFullScreen.xml window id=2005
    ADDON_ACTION_BIG_STEP_BACK = 23,

    /// @brief Action string: <b><c>OSD</c></b>
    ///
    /// show/hide OSD. Can b used in videoFullScreen.xml window id=2005
    ADDON_ACTION_SHOW_OSD = 24,

    /// @brief Action string: <b><c>ShowSubtitles</c></b>
    ///
    /// turn subtitles on/off. Can be used in videoFullScreen.xml window id=2005
    ADDON_ACTION_SHOW_SUBTITLES = 25,

    /// @brief Action string: <b><c>NextSubtitle</c></b>
    ///
    /// switch to next subtitle of movie. Can be used in videoFullScreen.xml
    /// window id=2005
    ADDON_ACTION_NEXT_SUBTITLE = 26,

    /// @brief Action string: <b><c>CodecInfo</c></b>
    ///
    /// show information about file. Can be used in videoFullScreen.xml window
    /// id=2005 and in slideshow.xml window id=2007
    ADDON_ACTION_SHOW_CODEC = 27,

    /// @brief Action string: <b><c>NextPicture</c></b>
    ///
    /// show next picture of slideshow. Can b used in slideshow.xml window id=2007
    ADDON_ACTION_NEXT_PICTURE = 28,

    /// @brief Action string: <b><c>PreviousPicture</c></b>
    ///
    /// show previous picture of slideshow. Can b used in slideshow.xml window
    /// id=2007
    ADDON_ACTION_PREV_PICTURE = 29,

    /// @brief Action string: <b><c>ZoomOut</c></b>
    ///
    /// zoom in picture during slideshow. Can b used in slideshow.xml window
    /// id=2007
    ADDON_ACTION_ZOOM_OUT = 30,

    /// @brief Action string: <b><c>ZoomIn</c></b>
    ///
    /// zoom out picture during slideshow. Can b used in slideshow.xml window
    /// id=2007
    ADDON_ACTION_ZOOM_IN = 31,

    /// used to toggle between source view and destination view. Can be used in
    /// myfiles.xml window id=3
    ADDON_ACTION_TOGGLE_SOURCE_DEST = 32,

    /// @brief Action string: <b><c>Playlist</c></b>
    ///
    /// used to toggle between current view and playlist view. Can b used in all
    /// mymusic xml files
    ADDON_ACTION_SHOW_PLAYLIST = 33,

    /// @brief Action string: <b><c>Queue</c></b>
    ///
    /// used to queue a item to the playlist. Can b used in all mymusic xml files
    ADDON_ACTION_QUEUE_ITEM = 34,

    /// not used anymore
    ADDON_ACTION_REMOVE_ITEM = 35,

    /// not used anymore
    ADDON_ACTION_SHOW_FULLSCREEN = 36,


    /// @brief Action string: <b><c>ZoomNormal</c></b>
    ///
    /// Zoom 1x picture during slideshow. Can b used in slideshow.xml window id=2007
    ADDON_ACTION_ZOOM_LEVEL_NORMAL = 37,

    /// @brief Action string: <b><c>ZoomLevel1</c></b>
    ///
    /// Zoom 2x picture during slideshow. Can b used in slideshow.xml window id=2007
    ADDON_ACTION_ZOOM_LEVEL_1 = 38,

    /// @brief Action string: <b><c>ZoomLevel2</c></b>
    ///
    /// Zoom 3x picture during slideshow. Can b used in slideshow.xml window id=2007
    ADDON_ACTION_ZOOM_LEVEL_2 = 39,

    /// @brief Action string: <b><c>ZoomLevel3</c></b>
    ///
    /// Zoom 4x picture during slideshow. Can b used in slideshow.xml window id=2007
    ADDON_ACTION_ZOOM_LEVEL_3 = 40,

    /// @brief Action string: <b><c>ZoomLevel4</c></b>
    ///
    /// Zoom 5x picture during slideshow. Can b used in slideshow.xml window id=2007
    ADDON_ACTION_ZOOM_LEVEL_4 = 41,

    /// @brief Action string: <b><c>ZoomLevel5</c></b>
    ///
    /// Zoom 6x picture during slideshow. Can b used in slideshow.xml window id=2007
    ADDON_ACTION_ZOOM_LEVEL_5 = 42,

    /// @brief Action string: <b><c>ZoomLevel6</c></b>
    ///
    /// Zoom 7x picture during slideshow. Can b used in slideshow.xml window id=2007
    ADDON_ACTION_ZOOM_LEVEL_6 = 43,

    /// @brief Action string: <b><c>ZoomLevel7</c></b>
    ///
    /// Zoom 8x picture during slideshow. Can b used in slideshow.xml window id=2007
    ADDON_ACTION_ZOOM_LEVEL_7 = 44,

    /// @brief Action string: <b><c>ZoomLevel8</c></b>
    ///
    /// Zoom 9x picture during slideshow. Can b used in slideshow.xml window id=2007
    ADDON_ACTION_ZOOM_LEVEL_8 = 45,

    /// @brief Action string: <b><c>ZoomLevel9</c></b>
    ///
    /// Zoom 10x picture during slideshow. Can b used in slideshow.xml window id=2007
    ADDON_ACTION_ZOOM_LEVEL_9 = 46,


    /// @brief Action string: <b><c>NextCalibration</c></b>
    ///
    /// select next arrow. Can b used in: settingsScreenCalibration.xml windowid=11
    ADDON_ACTION_CALIBRATE_SWAP_ARROWS = 47,

    /// @brief Action string: <b><c>ResetCalibration</c></b>
    ///
    /// reset calibration to defaults. Can be used in:
    /// settingsScreenCalibration.xml windowid=11/settingsUICalibration.xml
    /// windowid=10
    ADDON_ACTION_CALIBRATE_RESET = 48,

    /// @brief Action string: <b><c>AnalogMove</c></b>
    ///
    /// analog thumbstick move. Can be used in: slideshow.xml window
    /// id=2007/settingsScreenCalibration.xml windowid=11/settingsUICalibration.xml
    /// windowid=10, see also ACTION_ANALOG_MOVE_X, ACTION_ANALOG_MOVE_Y
    ADDON_ACTION_ANALOG_MOVE = 49,


    /// @brief Action string: <b><c>Rotate</c></b>
    ///
    /// rotate current picture clockwise during slideshow. Can be used in
    /// slideshow.xml window id=2007
    ADDON_ACTION_ROTATE_PICTURE_CW = 50,

    /// @brief Action string: <b><c>RotateCCW</c></b>
    ///
    /// rotate current picture counterclockwise during slideshow. Can be used in
    /// slideshow.xml window id=2007
    ADDON_ACTION_ROTATE_PICTURE_CCW = 51,


    /// @brief Action string: <b><c>SubtitleDelayMin</c></b>
    ///
    /// Decrease subtitle/movie Delay.  Can b used in videoFullScreen.xml window
    /// id=2005
    ADDON_ACTION_SUBTITLE_DELAY_MIN = 52,

    /// @brief Action string: <b><c>SubtitleDelayPlus</c></b>
    ///
    /// Increase subtitle/movie Delay.  Can be used in videoFullScreen.xml window
    /// id=2005
    ADDON_ACTION_SUBTITLE_DELAY_PLUS = 53,

    /// @brief Action string: <b><c>AudioDelayMinus</c></b>
    ///
    /// Increase avsync delay.  Can be used in videoFullScreen.xml window id=2005
    ADDON_ACTION_AUDIO_DELAY_MIN = 54,

    /// @brief Action string: <b><c>AudioDelayPlus</c></b>
    ///
    /// Decrease avsync delay.  Can be used in videoFullScreen.xml window id=2005
    ADDON_ACTION_AUDIO_DELAY_PLUS = 55,

    /// @brief Action string: <b><c>AudioNextLanguage</c></b>
    ///
    /// Select next language in movie.  Can be used in videoFullScreen.xml window
    /// id=2005
    ADDON_ACTION_AUDIO_NEXT_LANGUAGE = 56,

    /// @brief Action string: <b><c>NextResolution</c></b>
    ///
    /// switch 2 next resolution. Can b used during screen calibration
    /// settingsScreenCalibration.xml windowid=11
    ADDON_ACTION_CHANGE_RESOLUTION = 57,


    /// @brief Action string: <b><c>Number0</c></b>
    ///
    /// remote keys 0-9. are used by multiple windows
    ADDON_REMOTE_0 = 58,

    /// @brief Action string: <b><c>Number1</c></b>
    ///
    /// for example in videoFullScreen.xml window id=2005 you can
    ADDON_REMOTE_1 = 59,

    /// @brief Action string: <b><c>Number2</c></b>
    ///
    /// enter time (mmss) to jump to particular point in the movie
    ADDON_REMOTE_2 = 60,

    /// @brief Action string: <b><c>Number3</c></b>
    ///
    ///
    ADDON_REMOTE_3 = 61,

    /// @brief Action string: <b><c>Number4</c></b>
    ///
    /// with spincontrols you can enter 3digit number to quickly set
    ADDON_REMOTE_4 = 62,

    /// @brief Action string: <b><c>Number5</c></b>
    ///
    /// spincontrol to desired value
    ADDON_REMOTE_5 = 63,

    /// @brief Action string: <b><c>Number6</c></b>
    ///
    ///
    ADDON_REMOTE_6 = 64,

    /// @brief Action string: <b><c>Number7</c></b>
    ///
    ///
    ADDON_REMOTE_7 = 65,

    /// @brief Action string: <b><c>Number8</c></b>
    ///
    ADDON_REMOTE_8 = 66,

    /// @brief Action string: <b><c>Number9</c></b>
    ///
    ADDON_REMOTE_9 = 67,


    /// Unused at the moment
    ADDON_ACTION_PLAY = 68,

    /// @brief Action string: <b><c>SmallStepBack</c></b>
    ///
    /// jumps a few seconds back during playback of movie. Can be used in
    /// videoFullScreen.xml window id=2005
    ADDON_ACTION_SMALL_STEP_BACK = 76,

    /// @brief Action string: <b><c>FastForward</c></b>
    ///
    /// FF in current file played. global action, can be used anywhere
    ADDON_ACTION_PLAYER_FORWARD = 77,

    /// @brief Action string: <b><c>Rewind</c></b>
    ///
    /// RW in current file played. global action, can be used anywhere
    ADDON_ACTION_PLAYER_REWIND = 78,

    /// @brief Action string: <b><c>Play</c></b>
    ///
    /// Play current song. Unpauses song and sets playspeed to 1x. global action,
    /// can be used anywhere
    ADDON_ACTION_PLAYER_PLAY = 79,


    /// @brief Action string: <b><c>Delete</c></b>
    ///
    /// delete current selected item. Can be used in myfiles.xml window id=3 and
    /// in myvideoTitle.xml window id=25
    ADDON_ACTION_DELETE_ITEM = 80,

    /// @brief Action string: <b><c>Copy</c></b>
    ///
    /// copy current selected item. Can be used in myfiles.xml window id=3
    ADDON_ACTION_COPY_ITEM = 81,

    /// @brief Action string: <b><c>Move</c></b>
    ///
    /// move current selected item. Can be used in myfiles.xml window id=3
    ADDON_ACTION_MOVE_ITEM = 82,

    /// @brief Action string: <b><c>Sreenshot</c></b>
    ///
    /// take a screenshot
    ADDON_ACTION_TAKE_SCREENSHOT = 85,

    /// @brief Action string: <b><c>Rename</c></b>
    ///
    /// rename item
    ADDON_ACTION_RENAME_ITEM = 87,


    /// @brief Action string: <b><c>VolumeUp</c></b>
    ///
    /// Increase the volume of playback.
    ADDON_ACTION_VOLUME_UP = 88,

    /// @brief Action string: <b><c>VolumeDown</c></b>
    ///
    /// Decrease the volume of playback.
    ADDON_ACTION_VOLUME_DOWN = 89,

    /// @brief Action string: <b><c>VolumeAmplification</c></b>
    ///
    ///
    ADDON_ACTION_VOLAMP = 90,

    /// @brief Action string: <b><c>Mute</c></b>
    ///
    /// Mute the volume.
    ADDON_ACTION_MUTE = 91,

    /// @brief Action string: <b><c>ParentDir</c></b>
    ///
    /// Go back in navigation.
    ADDON_ACTION_NAV_BACK = 92,

    /// @brief Action string: <b><c>VolAmpUp</c></b>
    ///
    ///
    ADDON_ACTION_VOLAMP_UP = 93,

    /// @brief Action string: <b><c>VolAmpDown</c></b>
    ///
    ///
    ADDON_ACTION_VOLAMP_DOWN = 94,


    /// @brief Action string: <b><c>CreateEpisodeBookmark</c></b>
    ///
    /// Creates an episode bookmark on the currently playing video file
    /// containing more than one episode
    ADDON_ACTION_CREATE_EPISODE_BOOKMARK = 95,

    /// @brief Action string: <b><c>CreateBookmark</c></b>
    ///
    /// Creates a bookmark of the currently playing video file
    ADDON_ACTION_CREATE_BOOKMARK = 96,


    /// @brief Action string: <b><c>ChapterOrBigStepForward</c></b>
    ///
    /// Goto the next chapter, if not available perform a big step forward
    ADDON_ACTION_CHAPTER_OR_BIG_STEP_FORWARD = 97,

    /// @brief Action string: <b><c></c>ChapterOrBigStepBack</b>
    ///
    /// Goto the previous chapter, if not available perform a big step back
    ADDON_ACTION_CHAPTER_OR_BIG_STEP_BACK = 98,


    /// @brief Action string: <b><c>CycleSubtitle</c></b>
    ///
    /// switch to next subtitle of movie, but will not enable/disable the
    /// subtitles. Can be used in videoFullScreen.xml window id=2005
    ADDON_ACTION_CYCLE_SUBTITLE = 99,


    /// @brief Action string: <b><c>LeftClick</c></b>
    ///
    ///
    ADDON_ACTION_MOUSE_LEFT_CLICK = 100,

    /// @brief Action string: <b><c>RightClick</c></b>
    ///
    ///
    ADDON_ACTION_MOUSE_RIGHT_CLICK = 101,

    /// @brief Action string: <b><c>MiddleClick</c></b>
    ///
    ///
    ADDON_ACTION_MOUSE_MIDDLE_CLICK = 102,

    /// @brief Action string: <b><c>DoubleClick</c></b>
    ///
    ///
    ADDON_ACTION_MOUSE_DOUBLE_CLICK = 103,

    /// @brief Action string: <b><c>WheelUp</c></b>
    ///
    ///
    ADDON_ACTION_MOUSE_WHEEL_UP = 104,

    /// @brief Action string: <b><c>WheelDown</c></b>
    ///
    ///
    ADDON_ACTION_MOUSE_WHEEL_DOWN = 105,

    /// @brief Action string: <b><c>MouseDrag</c></b>
    ///
    ///
    ADDON_ACTION_MOUSE_DRAG = 106,

    /// @brief Action string: <b><c>MouseMove</c></b>
    ///
    ///
    ADDON_ACTION_MOUSE_MOVE = 107,

    /// @brief Action string: <b><c>LongClick</c></b>
    ///
    ///
    ADDON_ACTION_MOUSE_LONG_CLICK = 108,


    /// @brief Action string: <b><c>Backspace</c></b>
    ///
    /// Used in the virtual keyboards to delete one letter.
    ADDON_ACTION_BACKSPACE = 110,

    /// @brief Action string: <b><c>ScrollUp</c></b>
    ///
    /// Variable speed scroll up for analog keys (stick or triggers)
    ADDON_ACTION_SCROLL_UP = 111,

    /// @brief Action string: <b><c>ScrollDown</c></b>
    ///
    /// Variable speed scroll down for analog keys (stick or triggers)
    ADDON_ACTION_SCROLL_DOWN = 112,

    /// @brief Action string: <b><c>AnalogFastForward</c></b>
    ///
    ///
    ADDON_ACTION_ANALOG_FORWARD = 113,

    /// @brief Action string: <b><c>AnalogRewind</c></b>
    ///
    /// Variable speed rewind for analog keys (stick or triggers)
    ADDON_ACTION_ANALOG_REWIND = 114,


    /// @brief Action string: <b><c>MoveItemUp</c></b>
    ///
    /// move item up in playlist
    ADDON_ACTION_MOVE_ITEM_UP = 115,

    /// @brief Action string: <b><c>MoveItemDown</c></b>
    ///
    /// move item down in playlist
    ADDON_ACTION_MOVE_ITEM_DOWN = 116,

    /// @brief Action string: <b><c>ContextMenu</c></b>
    ///
    /// pops up the context menu
    ADDON_ACTION_CONTEXT_MENU = 117,


    /// @brief Action string: <b><c>Shift</c></b>
    ///
    /// Used in Virtual Keyboard to switch to upper or lower case letters
    ADDON_ACTION_SHIFT = 118,

    /// @brief Action string: <b><c>Symbols</c></b>
    ///
    /// Used in Virtual Keyboard to switch to or from symbols mode
    ADDON_ACTION_SYMBOLS = 119,

    /// @brief Action string: <b><c>CursorLeft</c></b>
    ///
    /// Used in Virtual Keyboard to move the current cursor point to the left
    ADDON_ACTION_CURSOR_LEFT = 120,

    /// @brief Action string: <b><c>CursorRight</c></b>
    ///
    /// Used in Virtual Keyboard to move the current cursor point to the right
    ADDON_ACTION_CURSOR_RIGHT = 121,


    ///
    ADDON_ACTION_BUILT_IN_FUNCTION = 122,


    /// @brief Action string: <b><c>ShowTime</c></b>
    ///
    /// displays current time, can be used in videoFullScreen.xml window id=2005
    ADDON_ACTION_SHOW_OSD_TIME = 123,

    /// @brief Action string: <b><c>AnalogSeekForward</c></b>
    ///
    /// seeks forward, and displays the seek bar.
    ADDON_ACTION_ANALOG_SEEK_FORWARD = 124,

    /// @brief Action string: <b><c>AnalogSeekBack</c></b>
    ///
    /// seeks backward, and displays the seek bar.
    ADDON_ACTION_ANALOG_SEEK_BACK = 125,


    /// @brief Action string: <b><c>ShowPreset</c></b>
    ///
    /// Shows the current visualisation preset (milkdrop/spectrum)
    ADDON_ACTION_VIS_PRESET_SHOW = 126,

    /// @brief Action string: <b><c>NextPresent</c></b>
    ///
    /// Next visualisation preset
    ADDON_ACTION_VIS_PRESET_NEXT = 128,

    /// @brief Action string: <b><c></c>PreviousPreset</b>
    ///
    /// Previous visualisation preset
    ADDON_ACTION_VIS_PRESET_PREV = 129,

    /// @brief Action string: <b><c>LockPreset</c></b>
    ///
    /// Lock the current visualisation preset
    ADDON_ACTION_VIS_PRESET_LOCK = 130,

    /// @brief Action string: <b><c>RandomPreset</c></b>
    ///
    /// Switch to a new random preset
    ADDON_ACTION_VIS_PRESET_RANDOM = 131,

    /// @brief Action string: <b><c>IncreaseVisRating</c></b>
    ///
    /// Used to increase track rating in full screen playback (press info on
    /// track and use pageplus on keyboard/remote
    ADDON_ACTION_VIS_RATE_PRESET_PLUS = 132,

    /// @brief Action string: <b><c>DecreaseVisRating</c></b>
    ///
    /// Used to decrease track rating in full screen playback (press info on
    /// track and use pageminus on keyboard/remote
    ADDON_ACTION_VIS_RATE_PRESET_MINUS = 133,


    /// @brief Action string: <b><c>ShowVideoMenu</c></b>
    ///
    /// Go to the DVD Video menu when playing a DVD.
    ADDON_ACTION_SHOW_VIDEOMENU = 134,

    /// @brief Action string: <b><c>Enter</c></b>
    ///
    /// ? (ACTION_ENTER) Not sure about all areas, but on some windows this will
    /// be the same as the "done" on screen button.
    ADDON_ACTION_ENTER = 135,


    /// @brief Action string: <b><c>IncreaseRating</c></b>
    ///
    /// Used to increase track rating in full screen playback (press info on
    /// track and use pageplus on keyboard/remote
    ADDON_ACTION_INCREASE_RATING = 136,

    /// @brief Action string: <b><c>DecreaseRating</c></b>
    ///
    /// Used to decrease track rating in full screen playback (press info on
    /// track and use pageminus on keyboard/remote
    ADDON_ACTION_DECREASE_RATING = 137,


    /// @brief Action string: <b><c>NextScene</c></b>
    ///
    /// switch to next scene/cutpoint in movie
    ADDON_ACTION_NEXT_SCENE = 138,

    /// @brief Action string: <b><c>PreviousScene</c></b>
    ///
    /// switch to previous scene/cutpoint in movie
    ADDON_ACTION_PREV_SCENE = 139,


    /// @brief Action string: <b><c>NextLetter</c></b>
    ///
    /// jump through a list or container by letter
    ADDON_ACTION_NEXT_LETTER = 140,

    /// @brief Action string: <b><c>PrevLetter</c></b>
    ///
    ///
    ADDON_ACTION_PREV_LETTER = 141,


    /// @brief Action string: <b><c>JumpSMS2</c></b>
    ///
    /// Jump through a list using SMS-style input 2 (eg press 2 twice to jump to
    /// the B's.)
    ADDON_ACTION_JUMP_SMS2 = 142,

    /// @brief Action string: <b><c>JumpSMS3</c></b>
    ///
    /// Jump through a list using SMS-style input 3 (eg press 2 twice to jump to the B's.)
    ADDON_ACTION_JUMP_SMS3 = 143,

    /// @brief Action string: <b><c>JumpSMS4</c></b>
    ///
    /// Jump through a list using SMS-style input 4 (eg press 2 twice to jump to the B's.)
    ADDON_ACTION_JUMP_SMS4 = 144,

    /// @brief Action string: <b><c>JumpSMS5</c></b>
    ///
    /// Jump through a list using SMS-style input 5 (eg press 2 twice to jump to the B's.)
    ADDON_ACTION_JUMP_SMS5 = 145,

    /// @brief Action string: <b><c>JumpSMS6</c></b>
    ///
    /// Jump through a list using SMS-style input 6 (eg press 2 twice to jump to the B's.)
    ADDON_ACTION_JUMP_SMS6 = 146,

    /// @brief Action string: <b><c>JumpSMS7</c></b>
    ///
    /// Jump through a list using SMS-style input 7 (eg press 2 twice to jump to the B's.)
    ADDON_ACTION_JUMP_SMS7 = 147,

    /// @brief Action string: <b><c>JumpSMS8</c></b>
    ///
    /// Jump through a list using SMS-style input 8 (eg press 2 twice to jump to the B's.)
    ADDON_ACTION_JUMP_SMS8 = 148,

    /// @brief Action string: <b><c>JumpSMS9</c></b>
    ///
    /// Jump through a list using SMS-style input 9 (eg press 2 twice to jump to the B's.)
    ADDON_ACTION_JUMP_SMS9 = 149,


    /// @brief Action string: <b><c>FilterClear</c></b>
    ///
    /// Filter a list in music or videos using SMS-style input.
    ADDON_ACTION_FILTER_CLEAR = 150,

    /// @brief Action string: <b><c>FilterSMS2</c></b>
    ///
    /// Filter a list in music or videos using SMS-style input.
    ADDON_ACTION_FILTER_SMS2 = 151,

    /// @brief Action string: <b><c>FilterSMS3</c></b>
    ///
    /// Filter a list in music or videos using SMS-style input.
    ADDON_ACTION_FILTER_SMS3 = 152,

    /// @brief Action string: <b><c>FilterSMS4</c></b>
    ///
    /// Filter a list in music or videos using SMS-style input.
    ADDON_ACTION_FILTER_SMS4 = 153,

    /// @brief Action string: <b><c>FilterSMS5</c></b>
    ///
    /// Filter a list in music or videos using SMS-style input.
    ADDON_ACTION_FILTER_SMS5 = 154,

    /// @brief Action string: <b><c>FilterSMS6</c></b>
    ///
    /// Filter a list in music or videos using SMS-style input.
    ADDON_ACTION_FILTER_SMS6 = 155,

    /// @brief Action string: <b><c>FilterSMS7</c></b>
    ///
    /// Filter a list in music or videos using SMS-style input.
    ADDON_ACTION_FILTER_SMS7 = 156,

    /// @brief Action string: <b><c>FilterSMS8</c></b>
    ///
    /// Filter a list in music or videos using SMS-style input.
    ADDON_ACTION_FILTER_SMS8 = 157,

    /// @brief Action string: <b><c>FilterSMS9</c></b>
    ///
    /// Filter a list in music or videos using SMS-style input.
    ADDON_ACTION_FILTER_SMS9 = 158,


    /// @brief Action string: <b><c>FirstPage</c></b>
    ///
    ///
    ADDON_ACTION_FIRST_PAGE = 159,

    /// @brief Action string: <b><c>LastPage</c></b>
    ///
    ///
    ADDON_ACTION_LAST_PAGE = 160,


    /// @brief Action string: <b><c>AudioDelay</c></b>
    ///
    ///
    ADDON_ACTION_AUDIO_DELAY = 161,

    /// @brief Action string: <b><c>SubtitleDelay</c></b>
    ///
    ///
    ADDON_ACTION_SUBTITLE_DELAY = 162,

    /// @brief Action string: <b><c>Menu</c></b>
    ///
    ///
    ADDON_ACTION_MENU = 163,


    /// @brief Action string: <b><c>Record</c></b>
    ///
    /// PVR record current playing channel or selected item in windows like EPG
    ADDON_ACTION_RECORD = 170,


    ///
    ADDON_ACTION_PASTE = 180,

    ///
    ADDON_ACTION_NEXT_CONTROL = 181,

    ///
    ADDON_ACTION_PREV_CONTROL = 182,

    /// PVR Used to switch channel. Works only if you playback TV or radio
    ADDON_ACTION_CHANNEL_SWITCH = 183,

    /// @brief Action string: <b><c>ChannelUp</c></b>
    ///
    /// PVR Used to switch up to the next channel. Works only if you playback TV
    /// or radio
    ADDON_ACTION_CHANNEL_UP = 184,

    /// @brief Action string: <b><c>ChannelDown</c></b>
    ///
    /// PVR Used to switch down to the previous channel. Works only if you
    /// playback TV or radio
    ADDON_ACTION_CHANNEL_DOWN = 185,

    /// @brief Action string: <b><c>NextChannelGroup</c></b>
    ///
    /// PVR Goes to the next channel group when using live TV. Works only within
    /// PVR windows + channel osd.
    ADDON_ACTION_NEXT_CHANNELGROUP = 186,

    /// @brief Action string: <b><c>PreviousChannelGroup</c></b>
    ///
    /// PVR Goes to the previous channel group when using live TV. Works only
    /// within PVR windows + channel osd.
    ADDON_ACTION_PREVIOUS_CHANNELGROUP = 187,

    /// @brief Action string: <b><c>PlayPVR</c></b>
    ///
    /// PVR Starts playback a TV or radio channel depends on the last played
    /// channel.
    ADDON_ACTION_PVR_PLAY = 188,

    /// @brief Action string: <b><c>PlayPVRTV</c></b>
    ///
    /// PVR Starts playback a TV channel. Will playback the last played
    /// channel if available.
    ADDON_ACTION_PVR_PLAY_TV = 189,

    /// @brief Action string: <b><c>PlayPVRRadio</c></b>
    ///
    /// PVR Starts playback a radio channel. Will playback the last played channel
    /// if available.
    ADDON_ACTION_PVR_PLAY_RADIO = 190,


    /// @brief Action string: <b><c>ToggleFullsreen</c></b>
    ///
    /// switch 2 desktop resolution
    ADDON_ACTION_TOGGLE_FULLSCREEN = 199,

    /// @brief Action string: <b><c>ToggleWatched</c></b>
    ///
    /// Toggle watched status (videos)
    ADDON_ACTION_TOGGLE_WATCHED = 200,

    /// @brief Action string: <b><c>ScanItem</c></b>
    ///
    /// scan item
    ADDON_ACTION_SCAN_ITEM = 201,

    /// @brief Action string: <b><c>AudioToggleDigital</c></b>
    ///
    /// switch digital <-> analog
    ADDON_ACTION_TOGGLE_DIGITAL_ANALOG = 202,

    /// @brief Action string: <b><c>ReloadKeymaps</c></b>
    ///
    /// reloads CButtonTranslator's keymaps
    ADDON_ACTION_RELOAD_KEYMAPS = 203,

    /// @brief Action string: <b><c>GUIProfile</c></b>
    ///
    /// start the GUIControlProfiler running
    ADDON_ACTION_GUIPROFILE_BEGIN = 204,


    /// @brief Action string: <b><c>Red</c></b>
    ///
    /// Teletext Color buttons to control TopText
    ADDON_ACTION_TELETEXT_RED = 215,

    /// @brief Action string: <b><c>Green</c></b>
    ///
    /// Teletext Color buttons to control TopText
    ADDON_ACTION_TELETEXT_GREEN = 216,

    /// @brief Action string: <b><c>Yellow</c></b>
    ///
    /// Teletext Color buttons to control TopText
    ADDON_ACTION_TELETEXT_YELLOW = 217,

    /// @brief Action string: <b><c>Blue</c></b>
    ///
    /// Teletext Color buttons to control TopText
    ADDON_ACTION_TELETEXT_BLUE = 218,


    /// @brief Action string: <b><c>IncreasePar</c></b>
    ///
    /// Used in video fullscreen to increase the pixel aspect ratio (stretch).
    ADDON_ACTION_INCREASE_PAR = 219,

    /// @brief Action string: <b><c>DecreasePar</c></b>
    ///
    /// Used in video fullscreen to decrease the pixel aspect ratio (stretch).
    ADDON_ACTION_DECREASE_PAR = 220,


    /// @brief Action string: <b><c>VerticalShiftUp</c></b>
    ///
    /// shift up video image in VideoPlayer
    ADDON_ACTION_VSHIFT_UP = 227,

    /// @brief Action string: <b><c>VerticalShiftDown</c></b>
    ///
    /// shift down video image in VideoPlayer
    ADDON_ACTION_VSHIFT_DOWN = 228,


    /// @brief Action string: <b><c>PlayPause</c></b>
    ///
    /// Play/pause. If playing it pauses, if paused it plays.
    ADDON_ACTION_PLAYER_PLAYPAUSE = 229,


    /// @brief Action string: <b><c>SubtitleShiftUp</c></b>
    ///
    /// shift up subtitles in VideoPlayer
    ADDON_ACTION_SUBTITLE_VSHIFT_UP = 230,

    /// @brief Action string: <b><c>SubtitleShiftDown</c></b>
    ///
    /// shift down subtitles in VideoPlayer
    ADDON_ACTION_SUBTITLE_VSHIFT_DOWN = 231,

    /// @brief Action string: <b><c>SubtitleAlign</c></b>
    ///
    /// toggle vertical alignment of subtitles
    ADDON_ACTION_SUBTITLE_ALIGN = 232,


    /// @brief Action string: <b><c>Filter</c></b>
    ///
    /// Opens the mediafilter dialog in the video and musiclibrary.
    ADDON_ACTION_FILTER = 233,


    /// @brief Action string: <b><c>SwitchPlayer</c></b>
    ///
    /// Select player to use, such as internal (DVDPlayer/PAPlayer),
    /// UPnP "play using" targets, external players (defined in
    /// playercorefactory.xml).
    ADDON_ACTION_SWITCH_PLAYER = 234,


    /// @brief Action string: <b><c></c>NextStereoMode</b>
    ///
    ADDON_ACTION_STEREOMODE_NEXT = 235,

    /// @brief Action string: <b><c>PreviousStereoMode</c></b>
    ///
    ADDON_ACTION_STEREOMODE_PREVIOUS = 236,

    /// @brief Action string: <b><c>ToggleStereoMode</c></b>
    ///
    /// Toggle 3D/Stereoscopic mode
    ADDON_ACTION_STEREOMODE_TOGGLE = 237,

    /// @brief Action string: <b><c>StereoMode</c></b>
    ///
    ADDON_ACTION_STEREOMODE_SELECT = 238,

    /// @brief Action string: <b><c>StereoModeToMono</c></b>
    ///
    ADDON_ACTION_STEREOMODE_TOMONO = 239,

    ///
    ADDON_ACTION_STEREOMODE_SET = 240,


    /// @brief Action string: <b><c>SettingsReset</c></b>
    ///
    ///
    ADDON_ACTION_SETTINGS_RESET = 241,

    /// @brief Action string: <b><c>SettingsLevelChange</c></b>
    ///
    ///
    ADDON_ACTION_SETTINGS_LEVEL_CHANGE = 242,


    /// show autoclosing OSD. Can b used in videoFullScreen.xml window id=2005
    ADDON_ACTION_TRIGGER_OSD = 243,

    ///
    ADDON_ACTION_INPUT_TEXT = 244,

    ///
    ADDON_ACTION_VOLUME_SET = 245,

    /// @brief Action string: <b><c>Tap</c></b>
    ///
    /// touch actions
    ADDON_ACTION_TOUCH_TAP = 401,

    /// To define the highest value possible for ADDON_ACTION_TOUCH_TAP
    ADDON_ACTION_TOUCH_TAP_TEN = 410,

    /// @brief Action string: <b><c>LongPress</c></b>
    ///
    /// touch action
    ADDON_ACTION_TOUCH_LONGPRESS = 411,

    /// To define the highest value possible for ADDON_ACTION_TOUCH_LONGPRESS
    ADDON_ACTION_TOUCH_LONGPRESS_TEN = 420,


    ///
    ADDON_ACTION_GESTURE_NOTIFY = 500,

    ///
    ADDON_ACTION_GESTURE_BEGIN = 501,

    /// @brief Action string: <b><c>ZoomGesture</c></b>
    ///
    /// sendaction with point and currentPinchScale (fingers together < 1.0 -> fingers apart > 1.0)
    ADDON_ACTION_GESTURE_ZOOM = 502,

    /// @brief Action string: <b><c>RotateGesture</c></b>
    ///
    ///
    ADDON_ACTION_GESTURE_ROTATE = 503,

    /// @brief Action string: <b><c>PanGesture</c></b>
    ///
    ///
    ADDON_ACTION_GESTURE_PAN = 504,


    /// @brief Action string: <b><c>SwipeLeft</c></b>
    ///
    /// To define the highest value possible for ADDON_ACTION_GESTURE_SWIPE_LEFT
    ADDON_ACTION_GESTURE_SWIPE_LEFT = 511,
    ADDON_ACTION_GESTURE_SWIPE_LEFT_TEN = 520,

    /// @brief Action string: <b><c>SwipeRight</c></b>
    ///
    /// To define the highest value possible for ADDON_ACTION_GESTURE_SWIPE_RIGHT
    ADDON_ACTION_GESTURE_SWIPE_RIGHT = 521,
    ADDON_ACTION_GESTURE_SWIPE_RIGHT_TEN = 530,

    /// @brief Action string: <b><c>SwipeUp</c></b>
    ///
    /// To define the highest value possible for ADDON_ACTION_GESTURE_SWIPE_UP
    ADDON_ACTION_GESTURE_SWIPE_UP = 531,
    ADDON_ACTION_GESTURE_SWIPE_UP_TEN = 540,

    /// @brief Action string: <b><c>SwipeDown</c></b>
    ///
    /// To define the highest value possible for ADDON_ACTION_GESTURE_SWIPE_DOWN
    ADDON_ACTION_GESTURE_SWIPE_DOWN = 541,
    ADDON_ACTION_GESTURE_SWIPE_DOWN_TEN = 550,


    /// @brief Action string: <b><c>AnalogMoveX</c></b>
    ///
    /// analog thumbstick move, horizontal axis; see ACTION_ANALOG_MOVE
    ADDON_ACTION_ANALOG_MOVE_X = 601,

    /// @brief Action string: <b><c>AnalogMoveY</c></b>
    ///
    /// analog thumbstick move, vertical axis; see ACTION_ANALOG_MOVE
    ADDON_ACTION_ANALOG_MOVE_Y = 602,


    /// @brief Action string: <b><c>Error</c></b>
    ///
    /// ERROR action is used to play an error sound
    ADDON_ACTION_ERROR = 998,

    /// @brief Action string: <b><c>Noop</c></b>
    ///
    /// The NOOP action can be specified to disable an input event. This is
    /// useful in user keyboard.xml etc to disable actions specified in the
    /// system mappings.
    ADDON_ACTION_NOOP = 999
  };
  /// @}

  ///
  /// \ingroup CPP_KodiAPI_GUI_CListItem_MusicInfoTag
  /// @{
  /// @brief Music info tag data transfer flags
  /// \anchor ADDON_MusicInfoTag
  ///
  /// Flags are used to identify data send from Add-on to Kodi and is used by
  /// the following functions:
  /// - \ref KodiAPI::GUI::CListItem::SetMusicInfo_BOOL "CListItem::SetMusicInfo_BOOL(...);"
  /// - \ref KodiAPI::GUI::CListItem::SetMusicInfo_INT "CListItem::SetMusicInfo_INT(...);"
  /// - \ref KodiAPI::GUI::CListItem::SetMusicInfo_UINT "CListItem::SetMusicInfo_UINT(...);"
  /// - \ref KodiAPI::GUI::CListItem::SetMusicInfo_FLOAT "CListItem::SetMusicInfo_FLOAT(...);"
  /// - \ref KodiAPI::GUI::CListItem::SetMusicInfo_STRING "CListItem::SetMusicInfo_STRING(...);"
  /// - \ref KodiAPI::GUI::CListItem::SetMusicInfo_STRING_LIST "CListItem::SetMusicInfo_STRING_LIST(...);"
  ///
  /// @note The end string of value shows the type of function to use.
  ///
  /// __Music example Values__:
  /// | Info label    | Description                                        |
  /// |--------------:|:---------------------------------------------------|
  /// | tracknumber   | integer (8)
  /// | discnumber    | integer (2)
  /// | duration      | integer (245) - duration in seconds
  /// | year          | integer (1998)
  /// | genre         | string (Rock)
  /// | album         | string (Pulse)
  /// | artist        | string (Muse)
  /// | title         | string (American Pie)
  /// | rating        | float - range is between 0 and 10
  /// | userrating    | integer - range is 1..10
  /// | lyrics        | string (On a dark desert highway...)
  /// | playcount     | integer (2) - number of times this item has been played
  /// | lastplayed    | string (%Y-%m-%d %h:%m:%s = 2009-04-05 23:16:04)
  ///
  typedef enum ADDON_MusicInfoTag
  {
    /// @brief URL of the music song.
    ///
    ADDON_MusicInfoTag_url_STRING = 1,
    /// @brief Title of the currently selected song or movie in a container.
    ///
    ADDON_MusicInfoTag_title_STRING = 2,
    /// @brief Artist of the currently selected song.
    ///
    ADDON_MusicInfoTag_artist_STRING_LIST = 3,
    /// @brief Artist of the currently selected song to show on skin.
    ///
    /// @note Only used for the way from Kodi to Add-on, not to use with
    /// CListItem::SetMusicInfo_STRING(...) to send from Add-on to
    /// Kodi.
    ADDON_MusicInfoTag_displayartist_STRING = 4,
    /// @brief Album artist name to show on skin.
    ///
    /// @note Only used for the way from Kodi to Add-on, not to use with
    /// CListItem::SetMusicInfo_STRING(...) to send from Add-on to
    /// Kodi.
    ADDON_MusicInfoTag_displayalbumartist_STRING = 5,
    /// @brief The album of the currently selected song in a container.
    ///
    ADDON_MusicInfoTag_album_STRING = 6,
    /// @brief The artist of the currently selected album in a list.
    ///
    ADDON_MusicInfoTag_albumartist_STRING_LIST = 7,
    /// @brief Genre(s) of current song.
    ///
    ADDON_MusicInfoTag_genre_STRING_LIST = 8,
    /// @brief Duration of current song.
    ///
    ADDON_MusicInfoTag_duration_INT = 9,
    /// @brief Track number of current song.
    ///
    ADDON_MusicInfoTag_track_number_INT = 10,
    /// @brief Disc Number of current song stored in ID tag info.
    ///
    ADDON_MusicInfoTag_disc_number_INT = 11,
    /// @brief Disc number of use
    ///
    ADDON_MusicInfoTag_loaded_BOOL = 12,
    /// @brief Year of release of current song.
    ///
    ADDON_MusicInfoTag_year_UINT = 13,
    /// @brief MusicBrainz metadata track ID
    ///
    ADDON_MusicInfoTag_musicbrainztrackid_STRING = 14,
    /// @brief MusicBrainz metadata artist ID
    ///
    ADDON_MusicInfoTag_musicbrainzartistid_STRING_LIST = 15,
    /// @brief MusicBrainz metadata album ID
    ///
    ADDON_MusicInfoTag_musicbrainzalbumid_STRING = 16,
    /// @brief MusicBrainz metadata Album & Artist ID
    ///
    ADDON_MusicInfoTag_musicbrainzalbumartistid_STRING_LIST = 17,
    /// @brief Media type to use for music on list item
    /// Used strings:
    /// - "music"
    /// - "song"
    /// - "album"
    /// - "artist"
    ///
    ADDON_MusicInfoTag_mediatype_STRING = 18,
    /// @brief 	Comment of current song stored in ID tag info.
    ///
    ADDON_MusicInfoTag_comment_STRING = 19,
    /// @brief Shows the moods of the currently playing artist.
    ///
    ADDON_MusicInfoTag_mood_STRING = 20,
    /// @brief Numeric Rating of current song.
    ///
    ADDON_MusicInfoTag_rating_FLOAT = 21,
    /// @brief The user rating of the currently playing item
    ///
    ADDON_MusicInfoTag_userrating_INT = 22,
    /// @brief Votes of the current song
    ///
    ADDON_MusicInfoTag_votes_INT = 23,
    /// @brief Playcount of current playing song
    ///
    ADDON_MusicInfoTag_playcount_INT = 24,
    /// @brief Last play date of current playing song
    ///
    ADDON_MusicInfoTag_lastplayed_STRING_DATE_TIME = 25,
    /// @brief The date the currently playing song was added to the library
    ///
    ADDON_MusicInfoTag_dateadded_STRING_DATE_TIME = 26,
    /// @brief Shows the release/aired date of the currently playing song
    ///
    ADDON_MusicInfoTag_releasedate_STRING_DATE_TIME = 27,
    /// @brief 	Lyrics of current song
    ///
    ADDON_MusicInfoTag_lyrics_STRING = 28,
    /// @brief Album id of currently playing song
    ///
    ADDON_MusicInfoTag_albumid_INT = 29,
    /// @brief
    ///
    ADDON_MusicInfoTag_compilation_BOOL = 30,
    /// @brief
    ///
    ADDON_MusicInfoTag_albumreleasetype_UINT = 31
  } ADDON_MusicInfoTag;
  /// @}

  ///
  /// \ingroup CPP_KodiAPI_GUI_CListItem_VideoInfoTag
  /// @{
  /// @brief Video info tag data transfer flags
  /// \anchor ADDON_VideoInfoTag
  ///
  /// Flags are used to identify data send from Add-on to Kodi and is used by
  /// the following functions:
  /// - \ref KodiAPI::GUI::CListItem::SetVideoInfo_BOOL "CListItem::SetVideoInfo_BOOL(...);"
  /// - \ref KodiAPI::GUI::CListItem::SetVideoInfo_INT "CListItem::SetVideoInfo_INT(...);"
  /// - \ref KodiAPI::GUI::CListItem::SetVideoInfo_UINT "CListItem::SetVideoInfo_UINT(...);"
  /// - \ref KodiAPI::GUI::CListItem::SetVideoInfo_FLOAT "CListItem::SetVideoInfo_FLOAT(...);"
  /// - \ref KodiAPI::GUI::CListItem::SetVideoInfo_STRING "CListItem::SetVideoInfo_STRING(...);"
  /// - \ref KodiAPI::GUI::CListItem::SetVideoInfo_STRING_LIST "CListItem::SetVideoInfo_STRING_LIST(...);"
  ///
  /// @note The end string of value shows the type of function to use.
  ///
  /// __Video example Values__:
  /// | Info label    | Description                                        |
  /// |--------------:|:---------------------------------------------------|
  /// | genre         | string (Comedy)
  /// | year          | integer (2009)
  /// | episode       | integer (4)
  /// | season        | integer (1)
  /// | top250        | integer (192)
  /// | tracknumber   | integer (3)
  /// | rating        | float (6.4) - range is 0..10
  /// | userrating    | integer (9) - range is 1..10
  /// | watched       | depreciated - use playcount instead
  /// | playcount     | integer (2) - number of times this item has been played
  /// | overlay       | integer (2) - range is `0..7`.  See \ref kodi_gui_listitem_iconoverlay "Overlay icon types" for values
  /// | cast          | list (["Michal C. Hall","Jennifer Carpenter"]) - if provided a list of tuples cast will be interpreted as castandrole
  /// | castandrole   | list of tuples ([("Michael C. Hall","Dexter"),("Jennifer Carpenter","Debra")])
  /// | director      | string (Dagur Kari)
  /// | mpaa          | string (PG-13)
  /// | plot          | string (Long Description)
  /// | plotoutline   | string (Short Description)
  /// | title         | string (Big Fan)
  /// | originaltitle | string (Big Fan)
  /// | sorttitle     | string (Big Fan)
  /// | duration      | integer (245) - duration in seconds
  /// | studio        | string (Warner Bros.)
  /// | tagline       | string (An awesome movie) - short description of movie
  /// | writer        | string (Robert D. Siegel)
  /// | tvshowtitle   | string (Heroes)
  /// | premiered     | string (2005-03-04)
  /// | status        | string (Continuing) - status of a TVshow
  /// | code          | string (tt0110293) - IMDb code
  /// | aired         | string (2008-12-07)
  /// | credits       | string (Andy Kaufman) - writing credits
  /// | lastplayed    | string (%Y-%m-%d %h:%m:%s = 2009-04-05 23:16:04)
  /// | album         | string (The Joshua Tree)
  /// | artist        | list (['U2'])
  /// | votes         | string (12345 votes)
  /// | trailer       | string (/home/user/trailer.avi)
  /// | dateadded     | string (%Y-%m-%d %h:%m:%s = 2009-04-05 23:16:04)
  /// | mediatype     | string - "video", "movie", "tvshow", "season", "episode" or "musicvideo"
  ///
  typedef enum ADDON_VideoInfoTag
  {
    /// @brief Director of current movie, if it's in the database
    ///
    ADDON_VideoInfoTag_director_STRING_LIST = 1,
    /// @brief
    ///
    ADDON_VideoInfoTag_writing_credits_STRING_LIST = 2,
    /// @brief Shows the genre of the currently selected song, album or movie
    /// in a container
    ///
    ADDON_VideoInfoTag_genre_STRING_LIST = 3,
    /// @brief Production country of current movie, if it's in the database
    ///
    ADDON_VideoInfoTag_country_STRING_LIST = 4,
    /// @brief Small Summary of current Video in a container
    ///
    ADDON_VideoInfoTag_tagline_STRING = 5,
    /// @brief Small Summary of current Video in a container
    ///
    ADDON_VideoInfoTag_plot_outline_STRING = 6,
    /// @brief Complete Text Summary of Video in a container
    ///
    ADDON_VideoInfoTag_plot_STRING = 7,
    /// @brief Title of currently playing video. If it's in the database it will
    /// return the database title, else the filename
    ///
    ADDON_VideoInfoTag_title_STRING = 8,
    /// @brief 	IMDb votes of current movie, if it's in the database
    ///
    ADDON_VideoInfoTag_votes_INT = 9,
    /// @brief 	Studio of current selected Music Video in a container
    ///
    ADDON_VideoInfoTag_studio_STRING_LIST = 10,
    /// @brief The full trailer path with filename of the currently selected
    /// movie in a container
    ///
    ADDON_VideoInfoTag_trailer_STRING = 11,
    /// @brief A concatenated string of cast members of the current movie, if
    /// it's in the database
    ///
    ADDON_VideoInfoTag_cast_DATA_LIST = 12,
    /// @brief
    ///
    ADDON_VideoInfoTag_set_STRING = 13,
    /// @brief
    ///
    ADDON_VideoInfoTag_setid_INT = 14,
    /// @brief
    ///
    ADDON_VideoInfoTag_setoverview_STRING = 15,
    /// @brief
    ///
    ADDON_VideoInfoTag_tag_STRING_LIST = 16,
    /// @brief Shows the song or movie duration of the currently selected movie
    /// in a container
    ///
    ADDON_VideoInfoTag_duration_UINT = 17,
    /// @brief Shows the filename of the currently selected song or movie in a
    /// container
    ///
    ADDON_VideoInfoTag_file_STRING = 18,
    /// @brief Shows the complete path of the currently selected song or movie
    /// in a container
    ///
    ADDON_VideoInfoTag_path_STRING = 19,
    /// @brief 	The IMDB iD of the selected Video in a container
    ///
    ADDON_VideoInfoTag_imdbnumber_STRING = 20,
    /// @brief Show the MPAA rating of the currently selected movie in a
    /// container
    ///
    ADDON_VideoInfoTag_mpaa_rating_STRING = 21,
    /// @brief Shows the full path with filename of the currently selected
    /// song or movie in a container
    ///
    ADDON_VideoInfoTag_filename_and_path_STRING = 22,
    /// @brief Shows the original title of the currently selected movie in
    /// a container
    ///
    ADDON_VideoInfoTag_original_title_STRING = 23,
    /// @brief Shows the first letter of the current file in a container
    ///
    ADDON_VideoInfoTag_sorttitle_STRING = 24,
    /// @brief
    ///
    ADDON_VideoInfoTag_episode_guide_STRING = 25,
    /// @brief Shows the release/aired date of the currently selected episode,
    /// show, movie or EPG item in a container
    ///
    ADDON_VideoInfoTag_premiered_STRING_DATE = 26,
    /// @brief
    ///
    ADDON_VideoInfoTag_status_STRING = 27,
    /// @brief
    ///
    ADDON_VideoInfoTag_production_code_STRING = 28,
    /// @brief
    ///
    ADDON_VideoInfoTag_first_aired_STRING_DATE = 29,
    /// @brief 	Shows the name value for the currently selected tvshow in the
    /// season and episode depth of the video library
    ///
    ADDON_VideoInfoTag_show_title_STRING = 30,
    /// @brief Shows the album of the currently selected song in a container
    ///
    ADDON_VideoInfoTag_album_STRING = 31,
    /// @brief Shows the artist of the currently selected song in a container
    ///
    ADDON_VideoInfoTag_artist_STRING_LIST = 32,
    /// @brief Playcount of Video in a container
    ///
    ADDON_VideoInfoTag_playcount_INT = 33,
    /// @brief Last play date of Video in a container
    ///
    ADDON_VideoInfoTag_lastplayed_STRING_DATE_TIME = 34,
    /// @brief 	Shows the IMDb top250 position of the currently selected
    /// listitem in a container.
    ///
    ADDON_VideoInfoTag_top250_INT = 35,
    /// @brief integer (23) - Only add this for items which are part of
    /// the local db. You also need to set the correct 'mediatype'!
    ///
    ADDON_VideoInfoTag_dbid_INT = 36,
    /// @brief Year of release of current movie, if it's in the database
    ///
    ADDON_VideoInfoTag_year_INT = 37,
    /// @brief Shows the season value for the currently selected tvshow
    ///
    ADDON_VideoInfoTag_season_INT = 38,
    /// @brief Shows the episode number value for the currently selected
    /// episode. It also shows the number of total, watched or unwatched
    /// episodes for the currently selected tvshow or season, based on
    /// the current watched filter.
    ///
    ADDON_VideoInfoTag_episode_INT = 39,
    /// @brief
    ///
    ADDON_VideoInfoTag_unique_id_STRING = 40,
    /// @brief Shows the IMDB rating of the currently selected movie in a
    /// container
    ///
    ADDON_VideoInfoTag_rating_FLOAT = 41,
    /// @brief Shows the user rating of the currently selected item in a
    /// container
    ///
    ADDON_VideoInfoTag_user_rating_INT = 42,
    /// @brief Shows the database id of the currently selected listitem in
    /// a container
    ///
    ADDON_VideoInfoTag_db_id_INT = 43,
    /// @brief
    ///
    ADDON_VideoInfoTag_file_id_INT = 44,
    /// @brief Shows the track number of the currently selected song in a
    /// container
    ///
    ADDON_VideoInfoTag_track_INT = 45,
    /// @brief
    ///
    ADDON_VideoInfoTag_show_link_STRING_LIST = 46,
    /// @brief
    ///
    ADDON_VideoInfoTag_resume_DATA = 47,
    /// @brief
    ///
    ADDON_VideoInfoTag_tvshow_id_INT = 48,
    /// @brief 	Shows the date the currently selected item was added to the
    /// library / Date and time of an event in the EventLog window.
    ///
    ADDON_VideoInfoTag_date_added_STRING_DATE_TIME = 49,
    /// @brief
    ///
    ADDON_VideoInfoTag_type_STRING = 50,
    /// @brief
    ///
    ADDON_VideoInfoTag_season_id_INT = 51,
    /// @brief
    ///
    ADDON_VideoInfoTag_special_sort_season_INT = 52,
    /// @brief
    ///
    ADDON_VideoInfoTag_special_sort_episode_INT = 53
  } ADDON_VideoInfoTag;
  /// @}

  ///
  /// \ingroup CPP_KodiAPI_GUI_CListItem_VideoInfoTag
  /// @{
  /// @brief Video info tag data about resume information
  ///
  /// Used on @ref ADDON_VideoInfoTag
  ///
  typedef struct ADDON_VideoInfoTag_Resume
  {
    /// @brief Current position of stream as seconds
    ///
    float       position;
    /// @brief Total size of stream as seconds
    ///
    float       total;
  } ADDON_VideoInfoTag_Resume;
  /// @}

  ///
  /// \ingroup CPP_KodiAPI_GUI_CListItem_VideoInfoTag
  /// @{
  /// @brief Video info tag data about cast informaation
  ///
  /// Used on @ref ADDON_VideoInfoTag
  ///
  typedef struct ADDON_VideoInfoTag_Cast
  {
    /// @brief Casts name
    ///
    std::string name;
    /// @brief Playing role
    ///
    std::string role;
    /// @brief Order value
    ///
    int         order;
    /// @brief Related Thumbnail URL
    ///
    std::string thumbnail;
  } ADDON_VideoInfoTag_Cast;
  /// @}

  ///
  /// \ingroup CPP_KodiAPI_GUI_CListItem_PictureInfoTag
  /// @{
  /// @brief Picture info tag data transfer flags
  /// \anchor ADDON_PictureInfoTag
  ///
  /// Flags are used to identify data send from Add-on to Kodi and is used by
  /// the following functions:
  /// - \ref KodiAPI::GUI::CListItem::SetPictureInfo_BOOL "CListItem::SetPictureInfo_BOOL(...);"
  /// - \ref KodiAPI::GUI::CListItem::SetPictureInfo_INT "CListItem::SetPictureInfo_INT(...);"
  /// - \ref KodiAPI::GUI::CListItem::SetPictureInfo_UINT "CListItem::SetPictureInfo_UINT(...);"
  /// - \ref KodiAPI::GUI::CListItem::SetPictureInfo_FLOAT "CListItem::SetPictureInfo_FLOAT(...);"
  /// - \ref KodiAPI::GUI::CListItem::SetPictureInfo_STRING "CListItem::SetPictureInfo_STRING(...);"
  /// - \ref KodiAPI::GUI::CListItem::SetPictureInfo_STRING_LIST "CListItem::SetPictureInfo_STRING_LIST(...);"
  ///
  /// @note The end string of value shows the type of function to use.
  ///
  /// __Picture example Values__:
  /// | Info label    | Description                                        |
  /// |--------------:|:---------------------------------------------------|
  /// | title         | string (In the last summer-1)
  /// | picturepath   | string (`/home/username/pictures/img001.jpg`)
  /// | exif*         | string (See \ref kodi_pictures_infotag for valid strings)
  ///
  /// @warning From exif are currently only the following supported:
  /// - ADDON_PictureInfoTag_count_INT
  /// - ADDON_PictureInfoTag_size_INT
  /// - ADDON_PictureInfoTag_title_STRING
  /// - ADDON_PictureInfoTag_picturepath_STRING
  /// - ADDON_PictureInfoTag_date_STRING_DATE_TIME
  /// - ADDON_PictureInfoTag_datetime_STRING_DATE_TIME
  /// - ADDON_PictureInfoTag_resolution_DATA
  /// All other from exif are not usable!
  typedef enum ADDON_PictureInfoTag
  {
    /// @brief The itle of the currently selected picture
    ///
    ADDON_PictureInfoTag_title_STRING = 1,
    /// @brief Shows the file size of the current picture
    ///
    ADDON_PictureInfoTag_size_INT = 2,
    /// @brief
    ///
    ADDON_PictureInfoTag_count_INT = 3,
    /// @brief Shows the filename and path of the selected picture
    ///
    ADDON_PictureInfoTag_picturepath_STRING = 4,
    /// @brief Shows the localized date of the selected picture. The short
    /// form of the date is used. The value of the EXIF DateTimeOriginal tag
    /// (hex code 0x9003) is preferred. If the DateTimeOriginal tag is not
    /// found, the value of DateTimeDigitized (hex code 0x9004) or of
    /// DateTime (hex code 0x0132) might be used.
    ///
    ADDON_PictureInfoTag_date_STRING_DATE_TIME = 5,
    /// @brief Shows the F-stop used to take the selected picture. This is
    /// the value of the EXIF FNumber tag (hex code 0x829D).
    ///
    ADDON_PictureInfoTag_aperturefnumber_FLOAT = 6,
    /// @brief Shows the manufacturer of the camera used to take the current
    /// picture. This is the value of the EXIF Make tag (hex code 0x010F).
    ///
    ADDON_PictureInfoTag_cameramake_STRING = 7,
    /// @brief Shows the manufacturer's model name or number of the camera used
    /// to take the current picture. This is the value of the EXIF Model tag
    /// (hex code 0x0110).
    ///
    ADDON_PictureInfoTag_cameramodel_STRING = 8,
    /// @brief Shows the width of the CCD in the camera used to take the current
    /// picture. This is calculated from three EXIF tags (0xA002 * 0xA210 / 0xA20e).
    ///
    ADDON_PictureInfoTag_ccdwidth_FLOAT = 9,
    /// @brief Shows a description of the selected picture. This is the value of
    /// the EXIF User Comment tag (hex code 0x9286). This is the same value as
    /// Slideshow.SlideComment.
    ///
    ADDON_PictureInfoTag_comments_STRING = 10,
    /// @brief Shows a short description of the current picture. The SlideComment,
    /// EXIFComment, or Caption values might contain a longer description. This
    /// is the value of the EXIF ImageDescription tag (hex code 0x010E).
    ///
    ADDON_PictureInfoTag_description_STRING = 11,
    /// @brief Shows the date/timestamp of the selected picture. The localized
    /// short form of the date and time is used. The value of the EXIF
    /// DateTimeOriginal tag (hex code 0x9003) is preferred. If the
    /// DateTimeOriginal tag is not found, the value of DateTimeDigitized
    /// (hex code 0x9004) or of DateTime (hex code 0x0132) might be used.
    ///
    ADDON_PictureInfoTag_datetime_STRING_DATE_TIME = 12,
    /// @brief
    ///
    ADDON_PictureInfoTag_datetimeoffsets_ten_items_INT_LIST = 13,
    /// @brief Shows the digital zoom ratio when the current picture was taken.
    /// This is the value of the EXIF .DigitalZoomRatio tag (hex code 0xA404).
    ///
    ADDON_PictureInfoTag_digitalzoomratio_FLOAT = 14,
    /// @brief Shows the distance to the subject, in meters. This is the value
    /// of the EXIF SubjectDistance tag (hex code 0x9206).
    ///
    ADDON_PictureInfoTag_distance_FLOAT = 15,
    /// @brief Shows the exposure bias of the current picture. Typically this
    /// is a number between -99.99 and 99.99. This is the value of the EXIF
    /// ExposureBiasValue tag (hex code 0x9204).
    ///
    ADDON_PictureInfoTag_exposurebias_FLOAT = 16,
    /// @brief Shows the exposure mode of the current picture. The possible
    /// values are "Automatic", "Manual", and "Auto bracketing". This is the
    /// value of the EXIF ExposureMode tag (hex code 0xA402).
    ///
    ADDON_PictureInfoTag_exposuremode_INT = 17,
    /// @brief Shows the exposure mode of the current picture. The possible
    /// values are "Automatic", "Manual", and "Auto bracketing". This is the
    /// value of the EXIF ExposureMode tag (hex code 0xA402).
    ///
    ADDON_PictureInfoTag_exposureprogram_INT = 18,
    /// @brief Shows the exposure time of the current picture, in seconds.
    /// This is the value of the EXIF ExposureTime tag (hex code 0x829A).
    /// If the ExposureTime tag is not found, the ShutterSpeedValue tag
    /// (hex code 0x9201) might be used.
    ///
    ADDON_PictureInfoTag_exposuretime_FLOAT = 19,
    /// @brief Shows the status of flash when the current picture was taken.
    /// The value will be either "Yes" or "No", and might include additional
    /// information. This is the value of the EXIF Flash tag (hex code 0x9209).
    ///
    ADDON_PictureInfoTag_flashused_INT = 20,
    /// @brief Shows the focal length of the lens, in mm. This is the value of
    /// the EXIF FocalLength tag (hex code 0x920A).
    ///
    ADDON_PictureInfoTag_focallength_FLOAT = 21,
    /// @brief
    ///
    ADDON_PictureInfoTag_focallength35mmequiv_INT = 22,
    /// @brief
    ///
    ADDON_PictureInfoTag_gpsinfopresent_BOOL = 23,
    /// @brief Shows the altitude in meters where the selected picture was taken.
    /// This is the value of the EXIF GPSInfo.GPSAltitude tag.
    ///
    ADDON_PictureInfoTag_gpsinfo_alt_STRING = 24,
    /// @brief Shows the latitude where the selected picture was taken (degrees,
    /// minutes, seconds North or South). This is the value of the EXIF
    /// GPSInfo.GPSLatitude and GPSInfo.GPSLatitudeRef tags.
    ///
    ADDON_PictureInfoTag_gpsinfo_lat_STRING = 25,
    /// @brief Shows the longitude where the selected picture was taken (degrees,
    /// minutes, seconds East or West). This is the value of the EXIF
    /// GPSInfo.GPSLongitude and GPSInfo.GPSLongitudeRef tags.
    ///
    ADDON_PictureInfoTag_gpsinfo_long_STRING = 26,
    /// @brief Shows the dimensions of the current picture (Width x Height)
    ///
    ADDON_PictureInfoTag_resolution_DATA = 27,
    /// @brief
    ///
    ADDON_PictureInfoTag_iscolor_BOOL = 28,
    /// @brief Shows the ISO speed of the camera when the current picture was
    /// taken. This is the value of the EXIF ISOSpeedRatings tag
    /// (hex code 0x8827).
    ///
    ADDON_PictureInfoTag_isoequivalent_BOOL = 29,
    /// @brief
    ///
    ADDON_PictureInfoTag_largestexifoffset_UINT = 30,
    /// @brief Shows the kind of light source when the picture was taken.
    /// Possible values include "Daylight", "Fluorescent", "Incandescent",
    /// etc. This is the value of the EXIF LightSource tag (hex code 0x9208).
    ///
    ADDON_PictureInfoTag_lightsource_INT = 31,
    /// @brief Shows the metering mode used when the current picture was
    /// taken. The possible values are "Center weight", "Spot", or "Matrix".
    /// This is the value of the EXIF MeteringMode tag (hex code 0x9207).
    ///
    ADDON_PictureInfoTag_meteringmode_INT = 32,
    /// @brief
    ///
    ADDON_PictureInfoTag_numdatetimetags_INT = 33,
    /// @brief Shows the orientation of the current picture. Possible values
    /// are "Top Left", "Top Right", "Left Top", "Right Bottom", etc. This is
    /// the value of the EXIF Orientation tag (hex code 0x0112).
    ///
    ADDON_PictureInfoTag_orientation_INT = 34,
    /// @brief Shows the process used to compress the current picture
    ///
    ADDON_PictureInfoTag_process_INT = 35,
    /// @brief
    ///
    ADDON_PictureInfoTag_thumbnailatend_BOOL = 36,
    /// @brief
    ///
    ADDON_PictureInfoTag_thumbnailoffset_UINT = 37,
    /// @brief
    ///
    ADDON_PictureInfoTag_thumbnailsize_UINT = 38,
    /// @brief
    ///
    ADDON_PictureInfoTag_thumbnailsizeoffset_INT = 39,
    /// @brief Shows the white balance mode set when the current picture was
    /// taken. The possible values are "Manual" and "Auto". This is the value
    /// of the EXIF WhiteBalance tag (hex code 0xA403).
    ///
    ADDON_PictureInfoTag_whitebalance_INT = 40,
    /// @brief Shows the name of the person involved in writing about the
    /// current picture. This is the value of the IPTC Writer tag (hex code
    /// 0x7A).
    ///
    ADDON_PictureInfoTag_author_STRING = 41,
    /// @brief Shows the name of the person who created the current picture.
    /// This is the value of the IPTC Byline tag (hex code 0x50).
    ///
    ADDON_PictureInfoTag_byline_STRING = 42,
    /// @brief Shows the title of the person who created the current picture.
    /// This is the value of the IPTC BylineTitle tag (hex code 0x55).
    ///
    ADDON_PictureInfoTag_bylinetitle_STRING = 43,
    /// @brief Shows a description of the current picture. This is the value
    /// of the IPTC Caption tag (hex code 0x78).
    ///
    ADDON_PictureInfoTag_caption_STRING = 44,
    /// @brief Shows the subject of the current picture as a category code.
    /// This is the value of the IPTC Category tag (hex code 0x0F).
    ///
    ADDON_PictureInfoTag_category_STRING = 45,
    /// @brief Shows the city where the current picture was taken. This is
    /// the value of the IPTC City tag (hex code 0x5A).
    ///
    ADDON_PictureInfoTag_city_STRING = 46,
    /// @brief Shows the urgency of the current picture. Values are 1-9. The 1
    /// is most urgent. Some image management programs use urgency to indicate
    /// picture rating, where urgency 1 is 5 stars and urgency 5 is 1 star.
    /// Urgencies 6-9 are not used for rating. This is the value of the IPTC
    /// Urgency tag (hex code 0x0A).
    ///
    ADDON_PictureInfoTag_urgency_STRING = 47,
    /// @brief Shows the copyright notice of the current picture. This is the
    /// value of the IPTC Copyright tag (hex code 0x74).
    ///
    ADDON_PictureInfoTag_copyrightnotice_STRING = 48,
    /// @brief Shows the full name of the country where the current picture
    /// was taken. This is the value of the IPTC CountryName tag (hex code
    /// 0x65).
    ///
    ADDON_PictureInfoTag_country_STRING = 49,
    /// @brief Shows the country code of the country where the current picture
    /// was taken. This is the value of the IPTC CountryCode tag (hex code 0x64).
    ///
    ADDON_PictureInfoTag_countrycode_STRING = 50,
    /// @brief Shows who provided the current picture. This is the value of the
    /// IPTC Credit tag (hex code 0x6E).
    ///
    ADDON_PictureInfoTag_credit_STRING = 51,
    /// @brief Shows the localized date of the current picture. The short form
    /// of the date is used. The value of the EXIF DateTimeOriginal tag (hex
    /// code 0x9003) is preferred. If the DateTimeOriginal tag is not found,
    /// the value of DateTimeDigitized (hex code 0x9004) or of DateTime (hex
    /// code 0x0132) might be used.
    ///
    ADDON_PictureInfoTag_date_STRING = 52,
    /// @brief Shows a synopsis of the contents of the current picture. This
    /// is the value of the IPTC Headline tag (hex code 0x69).
    ///
    ADDON_PictureInfoTag_headline_STRING = 53,
    /// @brief Shows keywords assigned to the current picture. This is the
    /// value of the IPTC Keywords tag (hex code 0x19).
    ///
    ADDON_PictureInfoTag_keywords_STRING = 54,
    /// @brief Shows a shorthand reference for the current picture. This is
    /// the value of the IPTC ObjectName tag (hex code 0x05).
    ///
    ADDON_PictureInfoTag_objectname_STRING = 55,
    /// @brief Shows the Service Identifier of a prior envelope to which the
    /// current picture refers. This is the value of the IPTC ReferenceService
    /// tag (hex code 0x2D).
    ///
    ADDON_PictureInfoTag_referenceservice_STRING = 56,
    /// @brief Shows the original owner of the current picture. This is the
    /// value of the IPTC Source tag (hex code 0x73).
    ///
    ADDON_PictureInfoTag_Source_STRING = 57,
    /// @brief Shows other editorial instructions concerning the use of the
    /// current picture. This is the value of the IPTC SpecialInstructions
    /// tag (hex code 0x28).
    ///
    ADDON_PictureInfoTag_Specialinstructions_STRING = 58,
    /// @brief Shows the State/Province where the current picture was taken.
    /// This is the value of the IPTC ProvinceState tag (hex code 0x5F).
    ///
    ADDON_PictureInfoTag_State_STRING = 59,
    /// @brief Shows supplemental category codes to further refine the subject
    /// of the current picture. This is the value of the IPTC SuppCategory
    /// tag (hex code 0x14).
    ///
    ADDON_PictureInfoTag_Supplementalcategories_STRING = 60,
    /// @brief Shows a code representing the location of original transmission
    /// of the current picture. This is the value of the IPTC
    /// TransmissionReference tag (hex code 0x67).
    ///
    ADDON_PictureInfoTag_transmissionreference_STRING = 61,
    /// @brief Shows the time when the intellectual content of the current
    /// picture was created, rather than when the picture was created. This
    /// is the value of the IPTC TimeCreated tag (hex code 0x3C).
    ///
    ADDON_PictureInfoTag_timecreated_STRING = 62,
    /// @brief Shows the location within a city where the current picture was
    /// taken - might indicate the nearest landmark. This is the value of the
    /// IPTC SubLocation tag (hex code 0x5C).
    ///
    ADDON_PictureInfoTag_Sublocation_STRING = 63,
    /// @brief Shows the color components of the current picture. This is
    /// the value of the IPTC ImageType tag (hex code 0x82).
    ///
    ADDON_PictureInfoTag_imagetype_STRING = 64
  } ADDON_PictureInfoTag;
  /// @}

} /* extern "C" */
} /* namespace KodiAPI */

END_NAMESPACE()
