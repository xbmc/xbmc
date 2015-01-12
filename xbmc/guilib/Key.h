/*!
 \file Key.h
 \brief
 */

#ifndef GUILIB_KEY
#define GUILIB_KEY

#pragma once

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

#include <string>
#include <stdint.h>

// Reserved 0 - 255
//  XBIRRemote.h
//  XINPUT_IR_REMOTE-*

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

// 0xF000 -> 0xF200 is reserved for the keyboard; a keyboard press is either
#define KEY_VKEY            0xF000 // a virtual key/functional key e.g. cursor left
#define KEY_ASCII           0xF100 // a printable character in the range of TRUE ASCII (from 0 to 127) // FIXME make it clean and pure unicode! remove the need for KEY_ASCII
#define KEY_UNICODE         0xF200 // another printable character whose range is not included in this KEY code

// 0xE000 -> 0xEFFF is reserved for mouse actions
#define KEY_VMOUSE          0xEFFF

#define KEY_MOUSE_START            0xE000
#define KEY_MOUSE_CLICK            0xE000
#define KEY_MOUSE_RIGHTCLICK       0xE001
#define KEY_MOUSE_MIDDLECLICK      0xE002
#define KEY_MOUSE_DOUBLE_CLICK     0xE010
#define KEY_MOUSE_LONG_CLICK       0xE020
#define KEY_MOUSE_WHEEL_UP         0xE101
#define KEY_MOUSE_WHEEL_DOWN       0xE102
#define KEY_MOUSE_DRAG             0xE103
#define KEY_MOUSE_MOVE             0xE104
#define KEY_MOUSE_NOOP             0xEFFF
#define KEY_MOUSE_END              0xEFFF

// 0xD000 -> 0xD0FF is reserved for WM_APPCOMMAND messages
#define KEY_APPCOMMAND      0xD000

// 0xF000 -> 0xF0FF is reserved for mouse actions
#define KEY_TOUCH           0xF000

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
#define ACTION_ROTATE_PICTURE_CW      50 // rotate current picture clockwise during slideshow. Can be used in slideshow.xml window id=2007
#define ACTION_ROTATE_PICTURE_CCW     51 // rotate current picture counterclockwise during slideshow. Can be used in slideshow.xml window id=2007

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
#define ACTION_RENAME_ITEM          87  // rename item

#define ACTION_VOLUME_UP            88
#define ACTION_VOLUME_DOWN          89
#define ACTION_MUTE                 91
#define ACTION_NAV_BACK             92
#define ACTION_VOLAMP_UP            93
#define ACTION_VOLAMP_DOWN          94

#define ACTION_CREATE_EPISODE_BOOKMARK 95 //Creates an episode bookmark on the currently playing video file containing more than one episode
#define ACTION_CREATE_BOOKMARK         96 //Creates a bookmark of the currently playing video file

#define ACTION_CHAPTER_OR_BIG_STEP_FORWARD       97 // Goto the next chapter, if not available perform a big step forward
#define ACTION_CHAPTER_OR_BIG_STEP_BACK          98 // Goto the previous chapter, if not available perform a big step back

#define ACTION_CYCLE_SUBTITLE         99 // switch to next subtitle of movie, but will not enable/disable the subtitles. Can be used in videoFullScreen.xml window id=2005

#define ACTION_MOUSE_START            100
#define ACTION_MOUSE_LEFT_CLICK       100
#define ACTION_MOUSE_RIGHT_CLICK      101
#define ACTION_MOUSE_MIDDLE_CLICK     102
#define ACTION_MOUSE_DOUBLE_CLICK     103
#define ACTION_MOUSE_WHEEL_UP         104
#define ACTION_MOUSE_WHEEL_DOWN       105
#define ACTION_MOUSE_DRAG             106
#define ACTION_MOUSE_MOVE             107
#define ACTION_MOUSE_LONG_CLICK       108
#define ACTION_MOUSE_END              109

#define ACTION_BACKSPACE          110
#define ACTION_SCROLL_UP          111
#define ACTION_SCROLL_DOWN        112
#define ACTION_ANALOG_FORWARD     113
#define ACTION_ANALOG_REWIND      114

#define ACTION_MOVE_ITEM_UP       115  // move item up in playlist
#define ACTION_MOVE_ITEM_DOWN     116  // move item down in playlist
#define ACTION_CONTEXT_MENU       117  // pops up the context menu


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

#define ACTION_NEXT_LETTER            140 // jump through a list or container by letter
#define ACTION_PREV_LETTER            141

#define ACTION_JUMP_SMS2              142 // jump direct to a particular letter using SMS-style input
#define ACTION_JUMP_SMS3              143
#define ACTION_JUMP_SMS4              144
#define ACTION_JUMP_SMS5              145
#define ACTION_JUMP_SMS6              146
#define ACTION_JUMP_SMS7              147
#define ACTION_JUMP_SMS8              148
#define ACTION_JUMP_SMS9              149

#define ACTION_FILTER_CLEAR           150
#define ACTION_FILTER_SMS2            151
#define ACTION_FILTER_SMS3            152
#define ACTION_FILTER_SMS4            153
#define ACTION_FILTER_SMS5            154
#define ACTION_FILTER_SMS6            155
#define ACTION_FILTER_SMS7            156
#define ACTION_FILTER_SMS8            157
#define ACTION_FILTER_SMS9            158

#define ACTION_FIRST_PAGE             159
#define ACTION_LAST_PAGE              160

#define ACTION_AUDIO_DELAY            161
#define ACTION_SUBTITLE_DELAY         162

#define ACTION_RECORD                 170

#define ACTION_PASTE                  180
#define ACTION_NEXT_CONTROL           181
#define ACTION_PREV_CONTROL           182
#define ACTION_CHANNEL_SWITCH         183
#define ACTION_CHANNEL_UP             184
#define ACTION_CHANNEL_DOWN           185
#define ACTION_NEXT_CHANNELGROUP      186
#define ACTION_PREVIOUS_CHANNELGROUP  187
#define ACTION_PVR_PLAY               188
#define ACTION_PVR_PLAY_TV            189
#define ACTION_PVR_PLAY_RADIO         190

#define ACTION_TOGGLE_FULLSCREEN      199 // switch 2 desktop resolution
#define ACTION_TOGGLE_WATCHED         200 // Toggle watched status (videos)
#define ACTION_SCAN_ITEM              201 // scan item
#define ACTION_TOGGLE_DIGITAL_ANALOG  202 // switch digital <-> analog
#define ACTION_RELOAD_KEYMAPS         203 // reloads CButtonTranslator's keymaps
#define ACTION_GUIPROFILE_BEGIN       204 // start the GUIControlProfiler running

#define ACTION_TELETEXT_RED           215 // Teletext Color buttons to control TopText
#define ACTION_TELETEXT_GREEN         216 //    "       "      "    "     "       "
#define ACTION_TELETEXT_YELLOW        217 //    "       "      "    "     "       "
#define ACTION_TELETEXT_BLUE          218 //    "       "      "    "     "       "

#define ACTION_INCREASE_PAR           219
#define ACTION_DECREASE_PAR           220

#define ACTION_VSHIFT_UP              227 // shift up video image in DVDPlayer
#define ACTION_VSHIFT_DOWN            228 // shift down video image in DVDPlayer

#define ACTION_PLAYER_PLAYPAUSE       229 // Play/pause. If playing it pauses, if paused it plays.

#define ACTION_SUBTITLE_VSHIFT_UP     230 // shift up subtitles in DVDPlayer
#define ACTION_SUBTITLE_VSHIFT_DOWN   231 // shift down subtitles in DVDPlayer
#define ACTION_SUBTITLE_ALIGN         232 // toggle vertical alignment of subtitles

#define ACTION_FILTER                 233

#define ACTION_SWITCH_PLAYER          234

#define ACTION_STEREOMODE_NEXT        235
#define ACTION_STEREOMODE_PREVIOUS    236
#define ACTION_STEREOMODE_TOGGLE      237 // turns 3d mode on/off
#define ACTION_STEREOMODE_SELECT      238
#define ACTION_STEREOMODE_TOMONO      239
#define ACTION_STEREOMODE_SET         240

#define ACTION_SETTINGS_RESET         241
#define ACTION_SETTINGS_LEVEL_CHANGE  242

#define ACTION_TRIGGER_OSD            243 // show autoclosing OSD. Can b used in videoFullScreen.xml window id=2005
#define ACTION_INPUT_TEXT             244

// touch actions
#define ACTION_TOUCH_TAP              401
#define ACTION_TOUCH_TAP_TEN          410
#define ACTION_TOUCH_LONGPRESS        411
#define ACTION_TOUCH_LONGPRESS_TEN    420

#define ACTION_GESTURE_NOTIFY         500
#define ACTION_GESTURE_BEGIN          501
#define ACTION_GESTURE_ZOOM           502 //sendaction with point and currentPinchScale (fingers together < 1.0 -> fingers apart > 1.0)
#define ACTION_GESTURE_ROTATE         503
#define ACTION_GESTURE_PAN            504

#define ACTION_GESTURE_SWIPE_LEFT       511
#define ACTION_GESTURE_SWIPE_LEFT_TEN   520
#define ACTION_GESTURE_SWIPE_RIGHT      521
#define ACTION_GESTURE_SWIPE_RIGHT_TEN  530
#define ACTION_GESTURE_SWIPE_UP         531
#define ACTION_GESTURE_SWIPE_UP_TEN     540
#define ACTION_GESTURE_SWIPE_DOWN       541
#define ACTION_GESTURE_SWIPE_DOWN_TEN   550
// 5xx is reserved for additional gesture actions
#define ACTION_GESTURE_END            599

// The NOOP action can be specified to disable an input event. This is
// useful in user keyboard.xml etc to disable actions specified in the
// system mappings.
#define ACTION_NOOP                   999

#define ICON_TYPE_NONE          101
#define ICON_TYPE_PROGRAMS      102
#define ICON_TYPE_MUSIC         103
#define ICON_TYPE_PICTURES      104
#define ICON_TYPE_VIDEOS        105
#define ICON_TYPE_FILES         106
#define ICON_TYPE_WEATHER       107
#define ICON_TYPE_SETTINGS      109

#ifndef SWIG

class CKey;

/*!
  \ingroup actionkeys
  \brief class encapsulating information regarding a particular user action to be sent to windows and controls
  */
class CAction
{
public:
  CAction(int actionID, float amount1 = 1.0f, float amount2 = 0.0f, const std::string &name = "", unsigned int holdTime = 0);
  CAction(int actionID, wchar_t unicode);
  CAction(int actionID, unsigned int state, float posX, float posY, float offsetX, float offsetY, const std::string &name = "");
  CAction(int actionID, const std::string &name, const CKey &key);
  CAction(int actionID, const std::string &name);

  /*! \brief Identifier of the action
   \return id of the action
   */
  int GetID() const { return m_id; };

  /*! \brief Is this an action from the mouse
   \return true if this is a mouse action, false otherwise
   */
  bool IsMouse() const { return (m_id >= ACTION_MOUSE_START && m_id <= ACTION_MOUSE_END); };

  bool IsGesture() const { return (m_id >= ACTION_GESTURE_NOTIFY && m_id <= ACTION_GESTURE_END); };

  /*! \brief Human-readable name of the action
   \return name of the action
   */
  const std::string &GetName() const { return m_name; };
  
  /*! \brief Text of the action if any
   \return text payload of this action.
   */
  const std::string &GetText() const { return m_text; };
  
  /*! \brief Set the text payload of the action
   \param text to be set
   */
  void SetText(const std::string &text) { m_text = text; };

  /*! \brief Get an amount associated with this action
   \param zero-based index of amount to retrieve, defaults to 0
   \return an amount associated with this action
   */
  float GetAmount(unsigned int index = 0) const { return (index < max_amounts) ? m_amount[index] : 0; };

  /*! \brief Unicode value associated with this action
   \return unicode value associated with this action, for keyboard input.
   */
  wchar_t GetUnicode() const { return m_unicode; };

  /*! \brief Time in ms that the key has been held
   \return time that the key has been held down in ms.
   */
  unsigned int GetHoldTime() const { return m_holdTime; };

  /*! \brief Time since last repeat in ms
   \return time since last repeat in ms. Returns 0 if unknown.
   */
  float GetRepeat() const { return m_repeat; };

  /*! \brief Button code that triggered this action
   \return button code
   */
  unsigned int GetButtonCode() const { return m_buttonCode; };

private:
  int          m_id;
  std::string   m_name;

  static const unsigned int max_amounts = 4; // Must be at least 4.
  float        m_amount[max_amounts];

  float        m_repeat;
  unsigned int m_holdTime;
  unsigned int m_buttonCode;
  wchar_t      m_unicode;
  std::string  m_text;
};

/*!
  \ingroup actionkeys, mouse
  \brief Simple class for mouse events
  */
class CMouseEvent
{
public:
  CMouseEvent(int actionID, int state = 0, float offsetX = 0, float offsetY = 0)
  {
    m_id = actionID;
    m_state = state;
    m_offsetX = offsetX;
    m_offsetY = offsetY;
  };

  int    m_id;
  int    m_state;
  float  m_offsetX;
  float  m_offsetY;
};

/*!
  \ingroup actionkeys
  \brief
  */
class CKey
{
public:
  CKey(void);
  CKey(uint32_t buttonCode, uint8_t leftTrigger = 0, uint8_t rightTrigger = 0, float leftThumbX = 0.0f, float leftThumbY = 0.0f, float rightThumbX = 0.0f, float rightThumbY = 0.0f, float repeat = 0.0f);
  CKey(uint32_t buttonCode, unsigned int held);
  CKey(uint8_t vkey, wchar_t unicode, char ascii, uint32_t modifiers, unsigned int held);
  CKey(const CKey& key);

  virtual ~CKey(void);
  CKey& operator=(const CKey& key);
  uint8_t GetLeftTrigger() const;
  uint8_t GetRightTrigger() const;
  float GetLeftThumbX() const;
  float GetLeftThumbY() const;
  float GetRightThumbX() const;
  float GetRightThumbY() const;
  float GetRepeat() const;
  bool FromKeyboard() const;
  bool IsAnalogButton() const;
  bool IsIRRemote() const;
  void SetFromService(bool fromService);
  bool GetFromService() const { return m_fromService; }

  inline uint32_t GetButtonCode() const { return m_buttonCode; }
  inline uint8_t  GetVKey() const       { return m_vkey; }
  inline wchar_t  GetUnicode() const    { return m_unicode; }
  inline char     GetAscii() const      { return m_ascii; }
  inline uint32_t GetModifiers() const  { return m_modifiers; };
  inline unsigned int GetHeld() const   { return m_held; }

  enum Modifier {
    MODIFIER_CTRL  = 0x00010000,
    MODIFIER_SHIFT = 0x00020000,
    MODIFIER_ALT   = 0x00040000,
    MODIFIER_RALT  = 0x00080000,
    MODIFIER_SUPER = 0x00100000,
    MODIFIER_META  = 0X00200000
  };

private:
  void Reset();

  uint32_t m_buttonCode;
  uint8_t  m_vkey;
  wchar_t  m_unicode;
  char     m_ascii;
  uint32_t m_modifiers;
  unsigned int m_held;

  uint8_t m_leftTrigger;
  uint8_t m_rightTrigger;
  float m_leftThumbX;
  float m_leftThumbY;
  float m_rightThumbX;
  float m_rightThumbY;
  float m_repeat; // time since last keypress
  bool m_fromService;
};
#endif //undef SWIG

#endif

