#ifndef GUILIB_KEY
#define GUILIB_KEY

#pragma once
#include "gui3d.h"
#include "../xbmc/XBIRRemote.h"

#define KEY_BUTTON_A									256
#define KEY_BUTTON_B									257
#define KEY_BUTTON_X									258
#define KEY_BUTTON_Y									259
#define KEY_BUTTON_WHITE							260
#define KEY_BUTTON_BLACK							261
#define KEY_BUTTON_BACK								262
#define KEY_BUTTON_START							263

#define KEY_BUTTON_DPAD_DOWN					264
#define KEY_BUTTON_DPAD_UP						265
#define KEY_BUTTON_DPAD_LEFT					266
#define KEY_BUTTON_DPAD_RIGHT					267

#define KEY_BUTTON_LEFT_TRIGGER				268
#define KEY_BUTTON_RIGHT_TRIGGER			269

#define KEY_BUTTON_LEFT_THUMB_BUTTON	270
#define KEY_BUTTON_RIGHT_THUMB_BUTTON	271
#define KEY_BUTTON_LEFT_THUMB_STICK   272
#define KEY_BUTTON_RIGHT_THUMB_STICK	273

#define KEY_INVALID										0xffff

// actions that we have defined...
#define ACTION_MOVE_LEFT							 1	
#define ACTION_MOVE_RIGHT							 2
#define ACTION_MOVE_UP								 3
#define ACTION_MOVE_DOWN							 4
#define ACTION_PAGE_UP								 5
#define ACTION_PAGE_DOWN							 6
#define ACTION_SELECT_ITEM						 7
#define ACTION_HIGHLIGHT_ITEM					 8
#define ACTION_PARENT_DIR							 9
#define ACTION_PREVIOUS_MENU						10
#define ACTION_SHOW_INFO							11

#define ACTION_PAUSE									12
#define ACTION_STOP										13
#define ACTION_NEXT_ITEM							14
#define ACTION_PREV_ITEM							15
#define ACTION_FORWARD								16
#define ACTION_REWIND									17

#define ACTION_SHOW_GUI								18
#define ACTION_ASPECT_RATIO						19
#define ACTION_STEP_FORWARD						20
#define ACTION_STEP_BACK							21
#define ACTION_BIG_STEP_FORWARD				22
#define ACTION_BIG_STEP_BACK					23
#define ACTION_SHOW_OSD								24
#define ACTION_SHOW_SUBTITLES					25
#define ACTION_NEXT_SUBTITLE					26
#define ACTION_SHOW_CODEC							27
#define ACTION_NEXT_PICTURE						28
#define ACTION_PREV_PICTURE						29
#define ACTION_ZOOM_OUT								30
#define ACTION_ZOOM_IN								31
#define ACTION_TOGGLE_SOURCE_DEST			32
#define ACTION_SHOW_PLAYLIST					33
#define ACTION_QUEUE_ITEM							34
#define	ACTION_REMOVE_ITEM						35
#define ACTION_SHOW_FULLSCREEN				36
#define ACTION_ZOOM_LEVEL_NORMAL			37
#define ACTION_ZOOM_LEVEL_1						38
#define ACTION_ZOOM_LEVEL_2						39
#define ACTION_ZOOM_LEVEL_3						40
#define ACTION_ZOOM_LEVEL_4						41
#define ACTION_ZOOM_LEVEL_5						42
#define ACTION_ZOOM_LEVEL_6						43
#define ACTION_ZOOM_LEVEL_7						44
#define ACTION_ZOOM_LEVEL_8						45
#define ACTION_ZOOM_LEVEL_9						46
#define ACTION_CALIBRATE_SWAP_ARROWS	47
#define ACTION_CALIBRATE_RESET				48
#define ACTION_ANALOG_MOVE						49
#define ACTION_ROTATE_PICTURE					50
#define ACTION_CLOSE_DIALOG						51
#define ACTION_SUBTITLE_DELAY_MIN			52
#define ACTION_SUBTITLE_DELAY_PLUS		53
#define ACTION_AUDIO_DELAY_MIN				54
#define ACTION_AUDIO_DELAY_PLUS				55
#define ACTION_AUDIO_NEXT_LANGUAGE		56
#define ACTION_CHANGE_RESOLUTION		57

// Window ID defines to make the code a bit more readable
#define WINDOW_INVALID				-1
#define WINDOW_HOME					0
#define WINDOW_PROGRAMS				1
#define WINDOW_PICTURES				2
#define WINDOW_FILES				3
#define WINDOW_SETTINGS				4
#define WINDOW_MUSIC				5
#define WINDOW_VIDEOS				6
#define WINDOW_SETTINGS_GENERAL		8
#define WINDOW_SETTINGS_SCREEN		9
#define WINDOW_UI_CALIBRATION		10
#define WINDOW_MOVIE_CALIBRATION	11
#define WINDOW_SETTINGS_SLIDESHOW	12
#define WINDOW_SETTINGS_FILTER		13
#define WINDOW_SETTINGS_MUSIC		14
#define WINDOW_SCRIPTS				20
#define WINDOW_DIALOG_YES_NO		100
#define WINDOW_DIALOG_PROGRESS		101
#define WINDOW_VIRTUAL_KEYBOARD		1000
#define WINDOW_DIALOG_SELECT		2000
#define WINDOW_MUSIC_INFO			2001
#define WINDOW_DIALOG_OK			2002
#define WINDOW_VIDEO_INFO			2003
#define WINDOW_FULLSCREEN_VIDEO		2005
#define WINDOW_VISUALISATION		2006
#define WINDOW_SLIDESHOW			2007

struct CAction {
	WORD wID;
	float fAmount1;
	float fAmount2;
};

class CKey
{
public:
  CKey(void);
  CKey(DWORD dwButtonCode, BYTE bLeftTrigger=0, BYTE bRightTrigger=0, float fLeftThumbX=0.0f, float fLeftThumbY=0.0f, float fRightThumbX=0.0f, float fRightThumbY=0.0f);
  CKey(const CKey& key);

  virtual ~CKey(void);
  const CKey& operator=(const CKey& key);
	DWORD				GetButtonCode() const;
	BYTE				GetLeftTrigger() const;
	BYTE				GetRightTrigger() const;
	float				GetLeftThumbX() const;
	float				GetLeftThumbY() const;
	float				GetRightThumbX() const;
	float				GetRightThumbY() const;
	
private:
	DWORD			m_dwButtonCode;
	BYTE			m_bLeftTrigger;
	BYTE			m_bRightTrigger;
	float			m_fLeftThumbX;
	float			m_fLeftThumbY;
	float			m_fRightThumbX;
	float			m_fRightThumbY;
};
#endif