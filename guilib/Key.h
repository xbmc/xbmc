/*!
  \file Key.h
  \brief 
  */

#ifndef GUILIB_KEY
#define GUILIB_KEY

#pragma once
#include "gui3d.h"
#include "../xbmc/XBIRRemote.h"

// Analogue - don't change order
#define KEY_BUTTON_A                  256
#define KEY_BUTTON_B                  257
#define KEY_BUTTON_X                  258
#define KEY_BUTTON_Y                  259
#define KEY_BUTTON_BLACK              260
#define KEY_BUTTON_WHITE              261
#define KEY_BUTTON_LEFT_TRIGGER       262
#define KEY_BUTTON_RIGHT_TRIGGER      263

#define KEY_BUTTON_LEFT_THUMB_STICK   264
#define KEY_BUTTON_RIGHT_THUMB_STICK  265

// Digital - don't change order
#define KEY_BUTTON_DPAD_UP            266
#define KEY_BUTTON_DPAD_DOWN          267
#define KEY_BUTTON_DPAD_LEFT          268
#define KEY_BUTTON_DPAD_RIGHT         269

#define KEY_BUTTON_START              270
#define KEY_BUTTON_BACK               271

#define KEY_BUTTON_LEFT_THUMB_BUTTON  272
#define KEY_BUTTON_RIGHT_THUMB_BUTTON 273

#define KEY_INVALID                   0xffff

// actions that we have defined...
#define ACTION_MOVE_LEFT               1  
#define ACTION_MOVE_RIGHT              2
#define ACTION_MOVE_UP                 3
#define ACTION_MOVE_DOWN               4
#define ACTION_PAGE_UP                 5
#define ACTION_PAGE_DOWN               6
#define ACTION_SELECT_ITEM             7
#define ACTION_HIGHLIGHT_ITEM          8
#define ACTION_PARENT_DIR              9
#define ACTION_PREVIOUS_MENU            10
#define ACTION_SHOW_INFO              11

#define ACTION_PAUSE                  12
#define ACTION_STOP                   13
#define ACTION_NEXT_ITEM              14
#define ACTION_PREV_ITEM              15
#define ACTION_FORWARD                16
#define ACTION_REWIND                 17

#define ACTION_SHOW_GUI               18 // toggle between GUI and movie or GUI and visualisation. 
#define ACTION_ASPECT_RATIO           19 // toggle between zoom/stretch/normal during a movie. Can b used in videoFullScreen.xml window id=2005 
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

#define ACTION_PLAY                 68  // Play current movie. Unpauses movie and sets playspeed to 1x.  Can b used in videoFullScreen.xml window id=2005
#define ACTION_OSD_SHOW_LEFT        69  // Move left in OSD. Can b used in videoFullScreen.xml window id=2005
#define ACTION_OSD_SHOW_RIGHT       70  // Move right in OSD. Can b used in videoFullScreen.xml window id=2005
#define ACTION_OSD_SHOW_UP          71  // Move up in OSD. Can b used in videoFullScreen.xml window id=2005
#define ACTION_OSD_SHOW_DOWN        72  // Move down in OSD. Can b used in videoFullScreen.xml window id=2005
#define ACTION_OSD_SHOW_SELECT      73  // toggle/select option in OSD. Can b used in videoFullScreen.xml window id=2005
#define ACTION_OSD_SHOW_VALUE_PLUS  74  // increase value of current option in OSD. Can b used in videoFullScreen.xml window id=2005
#define ACTION_OSD_SHOW_VALUE_MIN   75  // decrease value of current option in OSD. Can b used in videoFullScreen.xml window id=2005
#define ACTION_SMALL_STEP_BACK      76  // jumps a few seconds back during playback of movie. Can b used in videoFullScreen.xml window id=2005

#define ACTION_MUSIC_FORWARD        77  // FF in current song. global action, can be used anywhere
#define ACTION_MUSIC_REWIND         78  // RW in current song. global action, can be used anywhere
#define ACTION_MUSIC_PLAY           79  // Play current song. Unpauses song and sets playspeed to 1x. global action, can be used anywhere

#define ACTION_DELETE_ITEM          80  // delete current selected item. Can be used in myfiles.xml window id=3 and in myvideoTitle.xml window id=25
#define ACTION_COPY_ITEM            81  // copy current selected item. Can be used in myfiles.xml window id=3 
#define ACTION_MOVE_ITEM            82  // move current selected item. Can be used in myfiles.xml window id=3
#define ACTION_SHOW_MPLAYER_OSD     83  // toggles mplayers OSD. Can be used in videofullscreen.xml window id=2005
#define ACTION_OSD_HIDESUBMENU		84  // removes an OSD sub menu. Can be used in videoOSD.xml window id=2901
#define ACTION_TAKE_SCREENSHOT		85  // take a screenshot
#define ACTION_POWERDOWN		86  // restart

// Window ID defines to make the code a bit more readable
#define WINDOW_HOME                   10000
#define WINDOW_INVALID                WINDOW_HOME-1
#define WINDOW_PROGRAMS               WINDOW_HOME+1
#define WINDOW_PICTURES               WINDOW_HOME+2
#define WINDOW_FILES                  WINDOW_HOME+3
#define WINDOW_SETTINGS_MENU		WINDOW_HOME+4
#define WINDOW_MUSIC                  WINDOW_HOME+5
#define WINDOW_VIDEOS                 WINDOW_HOME+6
#define WINDOW_SYSTEM_INFORMATION     WINDOW_HOME+7
#define WINDOW_SETTINGS_GENERAL		WINDOW_HOME+8
#define WINDOW_SETTINGS_MYVIDEOS	WINDOW_HOME+9
#define WINDOW_UI_CALIBRATION         WINDOW_HOME+10
#define WINDOW_MOVIE_CALIBRATION      WINDOW_HOME+11
#define WINDOW_SETTINGS_MYPICTURES	WINDOW_HOME+12
#define WINDOW_SETTINGS_FILTER        WINDOW_HOME+13
#define WINDOW_SETTINGS_MYMUSIC		WINDOW_HOME+14
#define WINDOW_SETTINGS_SUBTITLES     WINDOW_HOME+15 
#define WINDOW_SETTINGS_SCREENSAVER   WINDOW_HOME+16
#define WINDOW_SETTINGS_MYWEATHER     WINDOW_HOME+17
#define WINDOW_SETTINGS_OSD	      WINDOW_HOME+18
#define WINDOW_SETTINGS_SKIN		WINDOW_HOME+19
#define WINDOW_SCRIPTS                WINDOW_HOME+20
#define WINDOW_VIDEO_GENRE            WINDOW_HOME+21
#define WINDOW_VIDEO_ACTOR            WINDOW_HOME+22
#define WINDOW_VIDEO_YEAR             WINDOW_HOME+23
#define WINDOW_SETTINGS_MYPROGRAMS	WINDOW_HOME+24
#define WINDOW_VIDEO_TITLE            WINDOW_HOME+25
#define WINDOW_SETTINGS_CACHE         WINDOW_HOME+26
#define WINDOW_SETTINGS_AUTORUN       WINDOW_HOME+27
#define WINDOW_VIDEO_PLAYLIST         WINDOW_HOME+28
#define WINDOW_SETTINGS_LCD           WINDOW_HOME+29

// New windows for settings screens
#define WINDOW_SETTINGS_UI			WINDOW_HOME+30
#define WINDOW_SETTINGS_AUDIO		WINDOW_HOME+31


#define WINDOW_DIALOG_YES_NO          WINDOW_HOME+100
#define WINDOW_DIALOG_PROGRESS        WINDOW_HOME+101
#define WINDOW_DIALOG_INVITE	      WINDOW_HOME+102
#define WINDOW_DIALOG_KEYBOARD	      WINDOW_HOME+103
#define WINDOW_MUSIC_PLAYLIST         WINDOW_HOME+500
#define WINDOW_MUSIC_FILES            WINDOW_HOME+501
#define WINDOW_MUSIC_ALBUM            WINDOW_HOME+502
#define WINDOW_MUSIC_ARTIST           WINDOW_HOME+503
#define WINDOW_MUSIC_GENRE            WINDOW_HOME+504
#define WINDOW_MUSIC_TOP100           WINDOW_HOME+505
#define WINDOW_VIRTUAL_KEYBOARD       WINDOW_HOME+1000
#define WINDOW_DIALOG_SELECT          WINDOW_HOME+2000
#define WINDOW_MUSIC_INFO             WINDOW_HOME+2001
#define WINDOW_DIALOG_OK              WINDOW_HOME+2002
#define WINDOW_VIDEO_INFO             WINDOW_HOME+2003
#define WINDOW_SCRIPTS_INFO           WINDOW_HOME+2004
#define WINDOW_FULLSCREEN_VIDEO       WINDOW_HOME+2005
#define WINDOW_VISUALISATION          WINDOW_HOME+2006
#define WINDOW_SLIDESHOW              WINDOW_HOME+2007
#define WINDOW_DIALOG_FILESTACKING    WINDOW_HOME+2008
#define WINDOW_WEATHER                WINDOW_HOME+2600
#define WINDOW_BUDDIES                WINDOW_HOME+2700
#define WINDOW_SCREENSAVER            WINDOW_HOME+2900
#define WINDOW_OSD                    WINDOW_HOME+2901

// WINDOW_ID's from WINDOW_HOME+3000 to WINDOW_HOME+3099 reserved for Python

#define WINDOW_PYTHON_START	WINDOW_HOME+3000
#define WINDOW_PYTHON_END	WINDOW_HOME+3099

/*!
  \ingroup actionkeys
  \brief 
  */
struct CAction {
  WORD wID;
  float fAmount1;
  float fAmount2;
	DWORD m_dwButtonCode;
};

/*!
  \ingroup actionkeys
  \brief 
  */
class CKey
{
public:
  CKey(void);
  CKey(DWORD dwButtonCode, BYTE bLeftTrigger=0, BYTE bRightTrigger=0, float fLeftThumbX=0.0f, float fLeftThumbY=0.0f, float fRightThumbX=0.0f, float fRightThumbY=0.0f);
  CKey(const CKey& key);

  virtual ~CKey(void);
  const CKey& operator=(const CKey& key);
  DWORD       GetButtonCode() const;
  BYTE        GetLeftTrigger() const;
  BYTE        GetRightTrigger() const;
  float       GetLeftThumbX() const;
  float       GetLeftThumbY() const;
  float       GetRightThumbX() const;
  float       GetRightThumbY() const;
  
private:
  DWORD     m_dwButtonCode;
  BYTE      m_bLeftTrigger;
  BYTE      m_bRightTrigger;
  float     m_fLeftThumbX;
  float     m_fLeftThumbY;
  float     m_fRightThumbX;
  float     m_fRightThumbY;
};
#endif
