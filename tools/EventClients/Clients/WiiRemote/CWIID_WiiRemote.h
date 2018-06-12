/*
 *      Copyright (C) 2007 by Tobias Arrskog
 *      topfs@tobias
 *
 *      Copyright (C) 2007-2013 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

/* Toggle one bit */
#define ToggleBit(bf,b) (bf) = ((bf) & b) ? ((bf) & ~(b)) : ((bf) | (b))

//Settings
#define WIIREMOTE_SAMPLES 16

#define DEADZONE_Y 0.3f
#define DEADZONE_X 0.5f

#define MOUSE_MAX 65535
#define MOUSE_MIN 0

//The work area is from 0 - MAX but the one sent to XBMC is MIN - (MIN + MOUSE_MAX)
#define WIIREMOTE_X_MIN MOUSE_MAX * DEADZONE_X
#define WIIREMOTE_Y_MIN MOUSE_MAX * DEADZONE_Y

#define WIIREMOTE_X_MAX MOUSE_MAX * (1.0f + DEADZONE_X + DEADZONE_X)
#define WIIREMOTE_Y_MAX MOUSE_MAX * (1.0f + DEADZONE_Y + DEADZONE_Y)

#define WIIREMOTE_BUTTON_REPEAT_TIME 30               // How long between buttonpresses in repeat mode
#define WIIREMOTE_BUTTON_DELAY_TIME 500
//#define CWIID_OLD                                     // Uncomment if the system is running cwiid that is older than 6.0 (The one from ubuntu gutsy repository is < 6.0)

//CWIID
#include <cwiid.h>
//Bluetooth specific
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
// UDP Client
#ifdef DEB_PACK
#include <xbmc/xbmcclient.h>
#else
#include "../../lib/c++/xbmcclient.h"
#endif
/*#include <stdio.h>*/
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <string>

class CWiiRemote
{
public:
  CWiiRemote(char *btaddr = NULL);
  ~CWiiRemote();

  void Initialize(CAddress Addr, int Socket);
  void Disconnect();
  bool GetConnected();

  bool EnableWiiRemote();
  bool DisableWiiRemote();

  void Update();

  // Mouse functions
  bool HaveIRSources();
  bool isActive();
  void EnableMouseEmulation();
  void DisableMouseEmulation();

  bool Connect();

  void SetBluetoothAddress(const char * btaddr);
  void SetSensitivity(float DeadX, float DeadY, int Samples);
  void SetJoystickMap(const char *JoyMap);
private:
  int  m_NumSamples;
  int  *m_SamplesX;
  int  *m_SamplesY;

  float m_MaxX;
  float m_MaxY;
  float m_MinX;
  float m_MinY;
#ifdef CWIID_OLD
  bool CheckConnection();
  int  m_LastMsgTime;
#endif
  char *m_JoyMap;
  int  m_lastKeyPressed;
  int  m_LastKey;
  bool m_buttonRepeat;

  int  m_lastKeyPressedNunchuck;
  int  m_LastKeyNunchuck;
  bool m_buttonRepeatNunchuck;

  void SetRptMode();
  void SetLedState();

  void SetupWiiRemote();

  bool m_connected;

  bool m_DisconnectWhenPossible;
  bool m_connectThreadRunning;

  //CWIID Specific
  cwiid_wiimote_t *m_wiiremoteHandle;
  unsigned char    m_ledState;
  unsigned char    m_rptMode;
  bdaddr_t         m_btaddr;

  static void MessageCallback(cwiid_wiimote_t *wiiremote, int mesgCount, union cwiid_mesg mesg[], struct timespec *timestamp);
#ifdef CWIID_OLD
  static void MessageCallback(cwiid_wiimote_t *wiiremote, int mesgCount, union cwiid_mesg mesg[]);
#endif

#ifndef _DEBUG
/* This takes the errors generated at pre-connect and silence them as they are mostly not needed */
  static void ErrorCallback(struct wiimote *wiiremote, const char *str, va_list ap);
#endif

	int   m_Socket;
  CAddress m_MyAddr;

  // Mouse
  bool	m_haveIRSources;
  bool  m_isActive;
  bool  m_useIRMouse;
  int   m_lastActiveTime;

/* The protected functions is for the static callbacks */
  protected:
  //Connection
  void DisconnectNow(bool startConnectThread);

  //Mouse
  void CalculateMousePointer(int x1, int y1, int x2, int y2);
//  void SetIR(bool set);

  //Button
  void ProcessKey(int Key);

  //Nunchuck
  void ProcessNunchuck(struct cwiid_nunchuk_mesg &Nunchuck);
#ifdef CWIID_OLD
  //Disconnect check
  void CheckIn();
#endif
};

extern CWiiRemote g_WiiRemote;
