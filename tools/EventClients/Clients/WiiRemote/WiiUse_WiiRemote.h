 /*     Copyright (C) 2009 by Cory Fields
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 3 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 */

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/time.h>
//#include <math.h>
#include <unistd.h>
#ifdef DEB_PACK
#include <xbmc/xbmcclient.h>
#else
#include "../../lib/c++/xbmcclient.h"
#endif
//#ifndef WIN32
//	#include <unistd.h>
//#endif
#include "wiiuse.h"
//#define ICON_PATH "../../"
#define PORT						9777
#define MAX_WIIMOTES					1
#define OVER_180_DEG					1
#define UNDER_NEG_180_DEG				2
#define WIIMOTE_BUTTON_TWO				0x0001
#define WIIMOTE_BUTTON_ONE				0x0002
#define WIIMOTE_BUTTON_B				0x0004
#define WIIMOTE_BUTTON_A				0x0008
#define WIIMOTE_BUTTON_MINUS				0x0010
#define WIIMOTE_BUTTON_HOME				0x0080
#define WIIMOTE_BUTTON_LEFT				0x0100
#define WIIMOTE_BUTTON_RIGHT				0x0200
#define WIIMOTE_BUTTON_DOWN				0x0400
#define WIIMOTE_BUTTON_UP				0x0800
#define WIIMOTE_BUTTON_PLUS				0x1000

#define WIIMOTE_NUM_BUTTONS				11
#define KEYCODE_BUTTON_UP				1
#define KEYCODE_BUTTON_DOWN				2
#define KEYCODE_BUTTON_LEFT				3
#define KEYCODE_BUTTON_RIGHT				4
#define KEYCODE_BUTTON_A				5
#define KEYCODE_BUTTON_B				6
#define KEYCODE_BUTTON_MINUS				7
#define KEYCODE_BUTTON_HOME				8
#define KEYCODE_BUTTON_PLUS				9
#define KEYCODE_BUTTON_ONE				10
#define KEYCODE_BUTTON_TWO				11
#define KEYCODE_ROLL_NEG				33
#define KEYCODE_ROLL_POS				34
#define KEYCODE_PITCH_NEG				35
#define KEYCODE_PITCH_POS				36


#define ACTION_NONE					0
#define ACTION_ROLL					1
#define ACTION_PITCH					2

class CWiiController{
  public:

  bool m_holdableHeld;
  bool m_holdableReleased;
  bool m_repeatableHeld;
  bool m_repeatableReleased;

  unsigned short m_buttonPressed;
  unsigned short m_buttonReleased;
  unsigned short m_buttonHeld;
  unsigned short m_repeatFlags;
  unsigned short m_holdFlags;
  unsigned short m_currentAction;

  int32_t m_buttonDownTime;

  float m_abs_roll;
  float m_abs_pitch;
  float m_rel_roll;
  float m_rel_pitch;
  float m_start_roll;
  float m_start_pitch;
  char* m_joyString;


  void get_keys(wiimote* wm);
  void handleKeyPress();
  void handleRepeat();
  void handleACC(float, float);
  void handleIR();

  CWiiController()
  {
    m_joyString = "JS0:WiiRemote";
    m_repeatFlags = WIIMOTE_BUTTON_UP | WIIMOTE_BUTTON_DOWN | WIIMOTE_BUTTON_LEFT | WIIMOTE_BUTTON_RIGHT;
    m_holdFlags = WIIMOTE_BUTTON_B;
  }
};

unsigned short g_deadzone = 30;
unsigned short g_hold_button_timeout = 200;
unsigned short g_repeat_rate = 400;

int handle_disconnect(wiimote* );
int connectWiimote(wiimote** );
int32_t getTicks(void);
void EnableMotionSensing(wiimote* wm) {if (!WIIUSE_USING_ACC(wm)) wiiuse_motion_sensing(wm, 1);}
void DisableMotionSensing(wiimote* wm) {if (WIIUSE_USING_ACC(wm)) wiiuse_motion_sensing(wm, 0);}
unsigned short convert_code(unsigned short);
float smoothDeg(float, float);

CXBMCClient EventClient;

