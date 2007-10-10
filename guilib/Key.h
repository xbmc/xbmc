/*!
 \file Key.h
 \brief 
 */

#ifndef GUILIB_KEY
#define GUILIB_KEY

#pragma once
#include "../xbmc/XBIRRemote.h"

// Analogue - don't change order
#define KEY_BUTTON_A                        256
#define KEY_BUTTON_B                        257
#define KEY_BUTTON_X                        258
#define KEY_BUTTON_Y                        259
#define KEY_BUTTON_BLACK                    260
#define KEY_BUTTON_WHITE                    261
#define KEY_BUTTON_LEFT_TRIGGER             262
#define KEY_BUTTON_RIGHT_TRIGGER            263

#define KEY_BUTTON_LEFT_THUMB_STICK         264
#define KEY_BUTTON_RIGHT_THUMB_STICK        265

#define KEY_BUTTON_RIGHT_THUMB_STICK_UP     266 // right thumb stick directions
#define KEY_BUTTON_RIGHT_THUMB_STICK_DOWN   267 // for defining different actions per direction
#define KEY_BUTTON_RIGHT_THUMB_STICK_LEFT   268
#define KEY_BUTTON_RIGHT_THUMB_STICK_RIGHT  269

// Digital - don't change order
#define KEY_BUTTON_DPAD_UP                  270
#define KEY_BUTTON_DPAD_DOWN                271
#define KEY_BUTTON_DPAD_LEFT                272
#define KEY_BUTTON_DPAD_RIGHT               273

#define KEY_BUTTON_START                    274
#define KEY_BUTTON_BACK                     275

#define KEY_BUTTON_LEFT_THUMB_BUTTON        276
#define KEY_BUTTON_RIGHT_THUMB_BUTTON       277

#define KEY_BUTTON_LEFT_ANALOG_TRIGGER      278
#define KEY_BUTTON_RIGHT_ANALOG_TRIGGER     279

#define KEY_BUTTON_LEFT_THUMB_STICK_UP      280 // left thumb stick directions
#define KEY_BUTTON_LEFT_THUMB_STICK_DOWN    281 // for defining different actions per direction
#define KEY_BUTTON_LEFT_THUMB_STICK_LEFT    282
#define KEY_BUTTON_LEFT_THUMB_STICK_RIGHT   283

#define KEY_VMOUSE          0xEFFF

// 0xF000 -> 0xF1FF is reserved for the keyboard
#define KEY_VKEY            0xF000
#define KEY_ASCII           0xF100

#define KEY_INVALID         0xFFFF

// actions that we have defined...
#define ACTION_NONE                    0
#define ACTION_MOVE_LEFT               1
#define ACTION_MOVE_RIGHT              2
#define ACTION_MOVE_UP                 3
#define ACTION_MOVE_DOWN               4
#define ACTION_PAGE_UP                 5
#define ACTION_PAGE_DOWN               6
#define ACTION_SELECT_ITEM             7
#define ACTION_HIGHLIGHT_ITEM          8
#define ACTION_PARENT_DIR              9
#define ACTION_PREVIOUS_MENU          10
#define ACTION_SHOW_INFO              11

#define ACTION_PAUSE                  12
#define ACTION_STOP                   13
#define ACTION_NEXT_ITEM              14
#define ACTION_PREV_ITEM              15
#define ACTION_FORWARD                16 // Can be used to specify specific action in a window, Playback control is handled in ACTION_PLAYER_*
#define ACTION_REWIND                 17 // Can be used to specify specific action in a window, Playback control is handled in ACTION_PLAYER_*

#define ACTION_SHOW_GUI               18 // toggle between GUI and movie or GUI and visualisation.
#define ACTION_ASPECT_RATIO           19 // toggle quick-access zoom modes. Can b used in videoFullScreen.zml window id=2005
#define ACTION_STEP_FORWARD           20 // seek +1% in the movie. Can b used in videoFullScreen.xml window id=2005
#define ACTION_STEP_BACK              21 // seek -1% in the movie. Can b used in videoFullScreen.xml window id=2005
#define ACTION_BIG_STEP_FORWARD       22 // seek +10% in the movie. Can b used in videoFullScreen.xml window id=2005
#define ACTION_BIG_STEP_BACK          23 // seek -10% in the movie. Can b used in videoFullScreen.xml window id=2005
#define ACTION_SHOW_OSD               24 // show/hide OSD. Can b used in videoFullScreen.xml window id=2005
#define ACTION_SHOW_SUBTITLES         25 // turn subtitles on/off. Can b used in videoFullScreen.xml window id=2005
#define ACTION_NEXT_SUBTITLE          26 // switch to next subtitle of movie. Can b used in videoFullScreen.xml window id=2005
#define ACTION_SHOW_CODEC             27 // show information about file. Can b used in videoFullScreen.xml window id=2005 and in slideshow.xml window id=2007
#define ACTION_NEXT_PICTURE           28 // show next picture of slideshow. Can b used in slideshow.xml window id=2007
#define ACTION_PREV_PICTURE           29 // show previous picture of slideshow. Can b used in slideshow.xml window id=2007
#define ACTION_ZOOM_OUT               30 // zoom in picture during slideshow. Can b used in slideshow.xml window id=2007
#define ACTION_ZOOM_IN                31 // zoom out picture during slideshow. Can b used in slideshow.xml window id=2007
#define ACTION_TOGGLE_SOURCE_DEST     32 // used to toggle between source view and destination view. Can be used in myfiles.xml window id=3
#define ACTION_SHOW_PLAYLIST          33 // used to toggle between current view and playlist view. Can b used in all mymusic xml files
#define ACTION_QUEUE_ITEM             34 // used to queue a item to the playlist. Can b used in all mymusic xml files
#define ACTION_REMOVE_ITEM            35 // not used anymore
#define ACTION_SHOW_FULLSCREEN        36 // not used anymore
#define ACTION_ZOOM_LEVEL_NORMAL      37 // zoom 1x picture during slideshow. Can b used in slideshow.xml window id=2007
#define ACTION_ZOOM_LEVEL_1           38 // zoom 2x picture during slideshow. Can b used in slideshow.xml window id=2007
#define ACTION_ZOOM_LEVEL_2           39 // zoom 3x picture during slideshow. Can b used in slideshow.xml window id=2007
#define ACTION_ZOOM_LEVEL_3           40 // zoom 4x picture during slideshow. Can b used in slideshow.xml window id=2007
#define ACTION_ZOOM_LEVEL_4           41 // zoom 5x picture during slideshow. Can b used in slideshow.xml window id=2007
#define ACTION_ZOOM_LEVEL_5           42 // zoom 6x picture during slideshow. Can b used in slideshow.xml window id=2007
#define ACTION_ZOOM_LEVEL_6           43 // zoom 7x picture during slideshow. Can b used in slideshow.xml window id=2007
#define ACTION_ZOOM_LEVEL_7           44 // zoom 8x picture during slideshow. Can b used in slideshow.xml window id=2007
#define ACTION_ZOOM_LEVEL_8           45 // zoom 9x picture during slideshow. Can b used in slideshow.xml window id=2007
#define ACTION_ZOOM_LEVEL_9           46 // zoom 10x picture during slideshow. Can b used in slideshow.xml window id=2007

#define ACTION_CALIBRATE_SWAP_ARROWS  47 // select next arrow. Can b used in: settingsScreenCalibration.xml windowid=11
#define ACTION_CALIBRATE_RESET        48 // reset calibration to defaults. Can b used in: settingsScreenCalibration.xml windowid=11/settingsUICalibration.xml windowid=10
#define ACTION_ANALOG_MOVE            49 // analog thumbstick move. Can b used in: slideshow.xml window id=2007/settingsScreenCalibration.xml windowid=11/settingsUICalibration.xml windowid=10
#define ACTION_ROTATE_PICTURE         50 // rotate current picture during slideshow. Can b used in slideshow.xml window id=2007
#define ACTION_CLOSE_DIALOG           51 // action for closing the dialog. Can b used in any dialog
#define ACTION_SUBTITLE_DELAY_MIN     52 // Decrease subtitle/movie Delay.  Can b used in videoFullScreen.xml window id=2005
#define ACTION_SUBTITLE_DELAY_PLUS    53 // Increase subtitle/movie Delay.  Can b used in videoFullScreen.xml window id=2005
#define ACTION_AUDIO_DELAY_MIN        54 // Increase avsync delay.  Can b used in videoFullScreen.xml window id=2005
#define ACTION_AUDIO_DELAY_PLUS       55 // Decrease avsync delay.  Can b used in videoFullScreen.xml window id=2005
#define ACTION_AUDIO_NEXT_LANGUAGE    56 // Select next language in movie.  Can b used in videoFullScreen.xml window id=2005
#define ACTION_CHANGE_RESOLUTION      57 // switch 2 next resolution. Can b used during screen calibration settingsScreenCalibration.xml windowid=11

#define REMOTE_0                    58  // remote keys 0-9. are used by multiple windows
#define REMOTE_1                    59  // for example in videoFullScreen.xml window id=2005 you can
#define REMOTE_2                    60  // enter time (mmss) to jump to particular point in the movie
#define REMOTE_3                    61
#define REMOTE_4                    62  // with spincontrols you can enter 3digit number to quickly set
#define REMOTE_5                    63  // spincontrol to desired value
#define REMOTE_6                    64
#define REMOTE_7                    65
#define REMOTE_8                    66
#define REMOTE_9                    67

#define ACTION_PLAY                 68  // Unused at the moment
#define ACTION_OSD_SHOW_LEFT        69  // Move left in OSD. Can b used in videoFullScreen.xml window id=2005
#define ACTION_OSD_SHOW_RIGHT       70  // Move right in OSD. Can b used in videoFullScreen.xml window id=2005
#define ACTION_OSD_SHOW_UP          71  // Move up in OSD. Can b used in videoFullScreen.xml window id=2005
#define ACTION_OSD_SHOW_DOWN        72  // Move down in OSD. Can b used in videoFullScreen.xml window id=2005
#define ACTION_OSD_SHOW_SELECT      73  // toggle/select option in OSD. Can b used in videoFullScreen.xml window id=2005
#define ACTION_OSD_SHOW_VALUE_PLUS  74  // increase value of current option in OSD. Can b used in videoFullScreen.xml window id=2005
#define ACTION_OSD_SHOW_VALUE_MIN   75  // decrease value of current option in OSD. Can b used in videoFullScreen.xml window id=2005
#define ACTION_SMALL_STEP_BACK      76  // jumps a few seconds back during playback of movie. Can b used in videoFullScreen.xml window id=2005

#define ACTION_PLAYER_FORWARD        77  // FF in current file played. global action, can be used anywhere
#define ACTION_PLAYER_REWIND         78  // RW in current file played. global action, can be used anywhere
#define ACTION_PLAYER_PLAY           79  // Play current song. Unpauses song and sets playspeed to 1x. global action, can be used anywhere

#define ACTION_DELETE_ITEM          80  // delete current selected item. Can be used in myfiles.xml window id=3 and in myvideoTitle.xml window id=25
#define ACTION_COPY_ITEM            81  // copy current selected item. Can be used in myfiles.xml window id=3
#define ACTION_MOVE_ITEM            82  // move current selected item. Can be used in myfiles.xml window id=3
#define ACTION_SHOW_MPLAYER_OSD     83  // toggles mplayers OSD. Can be used in videofullscreen.xml window id=2005
#define ACTION_OSD_HIDESUBMENU      84  // removes an OSD sub menu. Can be used in videoOSD.xml window id=2901
#define ACTION_TAKE_SCREENSHOT      85  // take a screenshot
#define ACTION_POWERDOWN            86  // restart
#define ACTION_RENAME_ITEM          87  // rename item

#define ACTION_VOLUME_UP            88
#define ACTION_VOLUME_DOWN          89
#define ACTION_MUTE                 91

#define ACTION_MOUSE                90

#define ACTION_MOUSE_CLICK            100
#define ACTION_MOUSE_LEFT_CLICK       100
#define ACTION_MOUSE_RIGHT_CLICK      101
#define ACTION_MOUSE_MIDDLE_CLICK     102
#define ACTION_MOUSE_XBUTTON1_CLICK   103
#define ACTION_MOUSE_XBUTTON2_CLICK   104

#define ACTION_MOUSE_DOUBLE_CLICK           105
#define ACTION_MOUSE_LEFT_DOUBLE_CLICK      105
#define ACTION_MOUSE_RIGHT_DOUBLE_CLICK     106
#define ACTION_MOUSE_MIDDLE_DOUBLE_CLICK    107
#define ACTION_MOUSE_XBUTTON1_DOUBLE_CLICK  108
#define ACTION_MOUSE_XBUTTON2_DOUBLE_CLICK  109

#define ACTION_BACKSPACE          110
#define ACTION_SCROLL_UP          111
#define ACTION_SCROLL_DOWN        112
#define ACTION_ANALOG_FORWARD     113
#define ACTION_ANALOG_REWIND      114

#define ACTION_MOVE_ITEM_UP       115  // move item up in playlist
#define ACTION_MOVE_ITEM_DOWN     116  // move item down in playlist
#define ACTION_CONTEXT_MENU       117	 // pops up the context menu


// stuff for virtual keyboard shortcuts
#define ACTION_SHIFT              118
#define ACTION_SYMBOLS            119
#define ACTION_CURSOR_LEFT        120
#define ACTION_CURSOR_RIGHT       121

#define ACTION_BUILT_IN_FUNCTION  122

#define ACTION_SHOW_OSD_TIME      123 // displays current time, can be used in videoFullScreen.xml window id=2005
#define ACTION_ANALOG_SEEK_FORWARD  124 // seeks forward, and displays the seek bar.
#define ACTION_ANALOG_SEEK_BACK     125 // seeks backward, and displays the seek bar.

#define ACTION_VIS_PRESET_SHOW        126
#define ACTION_VIS_PRESET_LIST        127
#define ACTION_VIS_PRESET_NEXT        128
#define ACTION_VIS_PRESET_PREV        129
#define ACTION_VIS_PRESET_LOCK        130
#define ACTION_VIS_PRESET_RANDOM      131
#define ACTION_VIS_RATE_PRESET_PLUS   132
#define ACTION_VIS_RATE_PRESET_MINUS  133

#define ACTION_SHOW_VIDEOMENU         134
#define ACTION_ENTER                  135

#define ACTION_INCREASE_RATING        136
#define ACTION_DECREASE_RATING        137

#define ACTION_NEXT_SCENE             138 // switch to next scene/cutpoint in movie
#define ACTION_PREV_SCENE             139 // switch to previous scene/cutpoint in movie

// Window ID defines to make the code a bit more readable
#define WINDOW_INVALID                     9999
#define WINDOW_HOME                       10000
#define WINDOW_PROGRAMS                   10001
#define WINDOW_PICTURES                   10002
#define WINDOW_FILES                      10003
#define WINDOW_SETTINGS_MENU              10004
#define WINDOW_MUSIC                      10005 // virtual window to return the music start window.
#define WINDOW_VIDEOS                     10006
#define WINDOW_SYSTEM_INFORMATION         10007
#define WINDOW_SCREEN_CALIBRATION          10011

#define WINDOW_SETTINGS_MYPICTURES        10012
#define WINDOW_SETTINGS_MYPROGRAMS        10013
#define WINDOW_SETTINGS_MYWEATHER         10014
#define WINDOW_SETTINGS_MYMUSIC           10015
#define WINDOW_SETTINGS_SYSTEM            10016
#define WINDOW_SETTINGS_MYVIDEOS          10017
#define WINDOW_SETTINGS_NETWORK           10018
#define WINDOW_SETTINGS_APPEARANCE        10019

#define WINDOW_SCRIPTS                    10020
#define WINDOW_VIDEO_GENRE                10021
#define WINDOW_VIDEO_ACTOR                10022
#define WINDOW_VIDEO_YEAR                 10023
#define WINDOW_VIDEO_FILES                10024
#define WINDOW_VIDEO_NAV                  10025
#define WINDOW_VIDEO_PLAYLIST             10028

#define WINDOW_LOGIN_SCREEN               10029
#define WINDOW_SETTINGS_PROFILES          10034
#define WINDOW_GAMESAVES                  10035

#define WINDOW_DIALOG_YES_NO              10100
#define WINDOW_DIALOG_PROGRESS            10101
#define WINDOW_DIALOG_INVITE              10102
#define WINDOW_DIALOG_KEYBOARD            10103
#define WINDOW_DIALOG_VOLUME_BAR          10104
#define WINDOW_DIALOG_SUB_MENU            10105
#define WINDOW_DIALOG_CONTEXT_MENU        10106
#define WINDOW_DIALOG_KAI_TOAST           10107
#define WINDOW_DIALOG_HOST                10108
#define WINDOW_DIALOG_NUMERIC             10109
#define WINDOW_DIALOG_GAMEPAD             10110
#define WINDOW_DIALOG_BUTTON_MENU         10111
#define WINDOW_DIALOG_MUSIC_SCAN          10112
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
#define WINDOW_DIALOG_TRAINER_SETTINGS    10127
#define WINDOW_DIALOG_NETWORK_SETUP       10128
#define WINDOW_DIALOG_MEDIA_SOURCE        10129
#define WINDOW_DIALOG_PROFILE_SETTINGS    10130
#define WINDOW_DIALOG_LOCK_SETTINGS       10131
#define WINDOW_DIALOG_CONTENT_SETTINGS    10132
#define WINDOW_DIALOG_VIDEO_SCAN          10133
#define WINDOW_DIALOG_FAVOURITES          10134
#define WINDOW_DIALOG_SONG_INFO           10135
#define WINDOW_DIALOG_SMART_PLAYLIST_EDITOR 10136
#define WINDOW_DIALOG_SMART_PLAYLIST_RULE   10137
#define WINDOW_DIALOG_BUSY                10138
#define WINDOW_DIALOG_PICTURE_INFO        10139

#define WINDOW_MUSIC_PLAYLIST             10500
#define WINDOW_MUSIC_FILES                10501
#define WINDOW_MUSIC_NAV                  10502
#define WINDOW_MUSIC_PLAYLIST_EDITOR      10503

//#define WINDOW_VIRTUAL_KEYBOARD           11000
#define WINDOW_DIALOG_SELECT              12000
#define WINDOW_MUSIC_INFO                 12001
#define WINDOW_DIALOG_OK                  12002
#define WINDOW_VIDEO_INFO                 12003
#define WINDOW_SCRIPTS_INFO               12004
#define WINDOW_FULLSCREEN_VIDEO           12005
#define WINDOW_VISUALISATION              12006
#define WINDOW_SLIDESHOW                  12007
#define WINDOW_DIALOG_FILESTACKING        12008
#define WINDOW_WEATHER                    12600
#define WINDOW_BUDDIES                    12700
#define WINDOW_SCREENSAVER                12900
#define WINDOW_OSD                        12901

#define WINDOW_VIDEO_MENU                 12902
#define WINDOW_MUSIC_OVERLAY              12903
#define WINDOW_VIDEO_OVERLAY              12904

#define WINDOW_STARTUP                    12999 // for startup animations

// WINDOW_ID's from 13000 to 13099 reserved for Python

#define WINDOW_PYTHON_START               13000
#define WINDOW_PYTHON_END                 13099

#define ICON_TYPE_NONE          101
#define ICON_TYPE_PROGRAMS      102
#define ICON_TYPE_MUSIC         103
#define ICON_TYPE_PICTURES      104
#define ICON_TYPE_VIDEOS        105
#define ICON_TYPE_FILES         106
#define ICON_TYPE_WEATHER       107
#define ICON_TYPE_SETTINGS      109

/*!
  \ingroup actionkeys
  \brief 
  */
struct CAction
{
  WORD wID;
  float fAmount1;
  float fAmount2;
  float fRepeat;
  DWORD m_dwButtonCode;
  CStdString strAction;
};

/*!
  \ingroup actionkeys
  \brief 
  */
class CKey
{
public:
  CKey(void);
  CKey(DWORD dwButtonCode, BYTE bLeftTrigger = 0, BYTE bRightTrigger = 0, float fLeftThumbX = 0.0f, float fLeftThumbY = 0.0f, float fRightThumbX = 0.0f, float fRightThumbY = 0.0f, float fRepeat = 0.0f);
  CKey(const CKey& key);

  virtual ~CKey(void);
  const CKey& operator=(const CKey& key);
  DWORD GetButtonCode() const;
  BYTE GetLeftTrigger() const;
  BYTE GetRightTrigger() const;
  float GetLeftThumbX() const;
  float GetLeftThumbY() const;
  float GetRightThumbX() const;
  float GetRightThumbY() const;
  float GetRepeat() const;
  bool FromKeyboard() const;
  bool IsAnalogButton() const;
  bool IsIRRemote() const;
  void SetFromHttpApi(bool);
  bool GetFromHttpApi() const;

private:
  DWORD m_dwButtonCode;
  BYTE m_bLeftTrigger;
  BYTE m_bRightTrigger;
  float m_fLeftThumbX;
  float m_fLeftThumbY;
  float m_fRightThumbX;
  float m_fRightThumbY;
  float m_fRepeat; // time since last keypress
  bool m_fromHttpApi;
};
#endif
