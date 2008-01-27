/***************************************************************************
 *   Copyright (C) 2007 by Tobias Arrskog,,,   *
 *   topfs@tobias   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
 
#ifdef HAS_CWIID

#ifndef WII_REMOTE_H
#define WII_REMOTE_H

/* Toggle one bit */
#define ToggleBit(bf,b) (bf) = ((bf) & b) ? ((bf) & ~(b)) : ((bf) | (b))


//Settings
#define WIIREMOTE_PRIORITIZE_MOUSE                    // if uncommented this will use the mouse if both wiiremote and mouse are active
#define WIIREMOTE_IR_DEADZONE 0.2f                    // Deadzone around the edges of the IRsource range

#define WIIREMOTE_ACTIVE_LENGTH 2000                  // How long it takes after the last IR Source was seen the "mouse" will get inactive
#define WIIREMOTE_BUTTON_REPEAT_TIME 30               // How long between buttonpresses in repeat mode
#define WIIREMOTE_BUTTON_DELAY_TIME 500


#define WIIREMOTE_PRINT_ALL_ERRORS                  // Uncomment to have libcwiid drop library wide errors into the terminal (couldn't connect etc. )
#define CWIID_OLD                                     // Uncomment if the system is running cwiid that is older than 6.0 (The one from ubuntu gutsy repository is < 6.0)

//CWIID
#include <cwiid.h>
//Misc
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stdafx.h"

//Error handling
//Bluetooth specific
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
// For threading
#include "Thread.h"
//#include "../../xbmc/utils/Mutex.h"
#include "CriticalSection.h"
#include "SingleLock.h"
//Wiiremote_key.h have all the button IDs recognised by buttontranslator
#include "WiiRemote_key.h"
//CLog
#include "log.h"

class CWiiRemote : public IRunnable
{
public:
  CWiiRemote();
  CWiiRemote(char *btaddr);
  ~CWiiRemote();
	
  void Initialize();
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
  void SetMouseActive();
  void SetMouseInactive();
	
  float GetMouseX();    
  float GetMouseY();

  // Button functions
  int   GetKey();
  bool  GetNewKey();
  void  ResetKey();

  // Connect-threads startpoint  
  void  Run();   

private:
  // Connectthread
  bool Connect();  
  void CreateConnectThread();


#ifdef CWIID_OLD
  bool CheckConnection();
  int  m_LastMsgTime;
#endif  
	
  void SetRptMode();
  void SetLedState();

  void SetupWiiRemote();
		
  bool m_connected;
  bool m_enabled;
  bool m_DisconnectWhenPossible;
  bool m_connectThreadRunning;
			
  //CWIID Specific
  cwiid_wiimote_t *m_wiiremoteHandle;
  unsigned char    m_ledState;
  unsigned char    m_rptMode;
  bdaddr_t         m_btaddr;
#ifndef CWIID_OLD
  static void MessageCallback(cwiid_wiimote_t *wiiremote, int mesgCount, union cwiid_mesg mesg[], struct timespec *timestamp);
#else
  static void MessageCallback(cwiid_wiimote_t *wiiremote, int mesgCount, union cwiid_mesg mesg[]);
#endif

#ifndef WIIREMOTE_PRINT_ALL_ERRORS
/* This takes the errors generated at pre-connect and silence them as they are mostly not needed */
  static void ErrorCallback(struct wiimote *wiiremote, const char *str, va_list ap);
#endif

  //Multithreading
  CThread *m_connectThread;
  static CCriticalSection m_lock;
	
	
  // Mouse	
  bool	m_haveIRSources;
  bool  m_isActive;
  bool  m_useIRMouse;
  int   m_lastActiveTime;
	
  float	m_mouseX;
  float	m_mouseY;

  // Button
  int   m_buttonData;
  bool  m_newKey;
  int   m_lastKeyPressed;
  bool  m_buttonRepeat;
	
/* The protected functions is for the static callbacks */	
  protected:
  //Connection
  void DisconnectNow(bool startConnectThread);
	
  //Mouse
  void CalculateMousePointer(int x1, int y1, int x2, int y2);
  void SetIR(bool set);
 	
  //Button
  void ProcessKey(int Key);
 	
#ifdef CWIID_OLD
  //Disconnect check
  void CheckIn();
#endif
};

#endif // WII_REMOTE_H

extern CWiiRemote g_WiiRemote;
#endif


