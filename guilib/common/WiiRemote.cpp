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
#include "WiiRemote.h"

// To avoid recursive include issues, we need to include these here.  I know it's naughty... --micolous (2008-02-28)
#include "../../xbmc/Application.h"
#include "../LocalizeStrings.h"
CWiiRemote g_WiiRemote;
CCriticalSection CWiiRemote::m_lock;

#ifdef CWIID_OLD
void CWiiRemote::MessageCallback(cwiid_wiimote_t *wiiremote, int mesg_count, union cwiid_mesg mesg[])
{
  MessageCallback(wiiremote, mesg_count, mesg, NULL);
}

#endif

/* The MessageCallback for the Wiiremote.
   This callback is used for error reports, mainly to see if the connection has been broken 
   This callback is also used for getting the IR sources, if this is done in update as with buttons we usually only get 1 IR source at a time wich is much harder to calculate */
void CWiiRemote::MessageCallback(cwiid_wiimote_t *wiiremote, int mesg_count, union cwiid_mesg mesg[], struct timespec *timestamp)
{
  EnterCriticalSection(m_lock);
  for (int i=0; i < mesg_count; i++)
  {
    int valid_source;
    switch (mesg[i].type) 
	  {
    case CWIID_MESG_IR:
      valid_source = 0;
      for (int j = 0; j < CWIID_IR_SRC_COUNT; j++) 
      {
        if (mesg[i].ir_mesg.src[j].valid) 
          valid_source++;
      }
      if (valid_source == 2)
      {
        g_WiiRemote.CalculateMousePointer(mesg[i].ir_mesg.src[0].pos[CWIID_X],
                                          mesg[i].ir_mesg.src[0].pos[CWIID_Y],
                                          mesg[i].ir_mesg.src[1].pos[CWIID_X],
                                          mesg[i].ir_mesg.src[1].pos[CWIID_Y]);
      }
      else if (valid_source > 2)
      { //TODO Make this care with the strenght of the sources
        g_WiiRemote.CalculateMousePointer(mesg[i].ir_mesg.src[0].pos[CWIID_X],
                                          mesg[i].ir_mesg.src[0].pos[CWIID_Y],
                                          mesg[i].ir_mesg.src[1].pos[CWIID_X],
                                          mesg[i].ir_mesg.src[1].pos[CWIID_Y]);                                             
      }
      else
        g_WiiRemote.SetIR(false);
    break;
    case CWIID_MESG_ERROR:
      g_WiiRemote.DisconnectNow(true);
    break;
    case CWIID_MESG_BTN:
      g_WiiRemote.ProcessKey(mesg[i].btn_mesg.buttons);
    break;	  
    case CWIID_MESG_STATUS:
      //Here we can figure out Extensiontypes and such
    break;
    case CWIID_MESG_NUNCHUK:
      //Not implemented
    break;
    case CWIID_MESG_CLASSIC:
      //Not implemented
    break;
    case CWIID_MESG_ACC:
      //Not implemented
    break;
    case CWIID_MESG_UNKNOWN:
    //...
    break;
    }
  }
#ifdef CWIID_OLD
  g_WiiRemote.CheckIn();
#endif  
  LeaveCriticalSection(m_lock);
}

#ifndef _DEBUG
/* This takes the errors generated at pre-connect and silence them as they are mostly not needed */
void CWiiRemote::ErrorCallback(struct wiimote *wiiremote, const char *str, va_list ap)
{
  //Error checking
}
#endif

//Constructor
/* This Constructor makes XBMC connect to any wiiremote*/
CWiiRemote::CWiiRemote()
{
  bacpy(&m_btaddr, &(*BDADDR_ANY));	
}

/*This constructor is never used but it shows how one would connect to just a specific Wiiremote by Mac-Adress*/
CWiiRemote::CWiiRemote(char *wii_btaddr)
{
  str2ba(wii_btaddr, &m_btaddr);
}

//Destructor
CWiiRemote::~CWiiRemote()
{
  if (m_connected == true)
    this->DisconnectNow(false);

  // This will be false if connectthread == NULL so we don't need to check for it
  if (m_connectThreadRunning)
  {
    m_enabled = false;                          //If we disable Wiiremote support the thread will exit when done.
    m_connectThread->WaitForThreadExit(5000);
    m_connectThread->StopThread();
    delete m_connectThread;
  }
}

//---------------------Public-------------------------------------------------------------------
/* This is an inherited function from IRunnable and this function will be run when a CThread is created, making this class look for a wiiremote in a multithreaded environment*/
void CWiiRemote::Run()
{
  m_connectThreadRunning = true;
  CLog::Log(LOGNOTICE, "Looking for a Wiiremote");
  Connect();
  m_connectThreadRunning = false;
  return;
}

void CWiiRemote::SetIR(bool set)
{
  m_haveIRSources = set;
}

/* Basicly this just sets up standard control bits and creates the first connect-thread*/
void CWiiRemote::Initialize()
{ //Here we will load up all MAC adresses to the given wiiremotes and poll them to see if they are visible
  m_connected = false;
  m_enabled = false;
  m_useIRMouse = true;
  m_connectThreadRunning = false;
  m_mouseX = 0;
  m_mouseY = 0;

#ifdef CWIID_OLD
  m_LastMsgTime = timeGetTime();
#endif
  
  //All control bits are set to false when cwiid is started
  //Report Button presses
  ToggleBit(m_rptMode, CWIID_RPT_BTN);
    
  //If wiiremote is used as a mouse, then report the IR sources
#ifndef CWIID_OLD  
  if (m_useIRMouse) 
#endif
    ToggleBit(m_rptMode, CWIID_RPT_IR);	
    
  //Have the first and fourth LED on the Wiiremote shine when connected
  ToggleBit(m_ledState, CWIID_LED1_ON);	
  ToggleBit(m_ledState, CWIID_LED4_ON);
	
  CSingleLock lock (m_lock);

  CLog::Log(LOGNOTICE, "Sucessfully initialized the Wiiremote Lib");
}

/* Update is run regulary and we gather the state of the Wiiremote and see if the user have pressed on a button or moved the wiiremote
   This could have been done with callbacks instead but it doesn't look nice in C++*/
void CWiiRemote::Update()
{
  EnterCriticalSection(m_lock);
  if (m_DisconnectWhenPossible)
  {//If the user have choosen to disconnect or lost comunication
    DisconnectNow(true);
    m_DisconnectWhenPossible = false;
  }
#ifdef CWIID_OLD
  if(m_connected)
  {//Here we check if the connection is suddenly broken
    if (!CheckConnection())
    {
      DisconnectNow(true);
      LeaveCriticalSection(m_lock);
      return;
    }
  }
#endif
  LeaveCriticalSection(m_lock);  
}

/*This function only returns true if a wiiremote is connected, enabled and tracking IR*/
/* m_connected implies m_enabled. m_enabled can be true when not connected but it isn't relevent*/
bool CWiiRemote::HaveIRSources()
{
  if (m_connected && m_useIRMouse)
    return m_haveIRSources;
   else
    return false;  
}

/* //This will return false if the wiiremote hasn't found an IR source in a while (Emulating a mouse that is left for a while)*/
bool CWiiRemote::isActive()
{
  if (!m_useIRMouse || !m_connected) //If we don't use the Wiiremote as a mouse, then it isn't active. ever :)
    return false;
  else
  {
    if (timeGetTime() - m_lastActiveTime < WIIREMOTE_ACTIVE_LENGTH)
      return true;
    return false;
  }
}

//Returns the location were the mousepointer should be
float CWiiRemote::GetMouseX()
{
  return m_mouseX;
}
//Returns the location were the mousepointer should be
float CWiiRemote::GetMouseY()
{
  return m_mouseY;	
}

//This will return the ID for the last pressed key and -1 if nothing is pressed at all
int  CWiiRemote::GetKey()
{
  EnterCriticalSection(m_lock);
  int RtnKey = -1;
  if (m_connected)
  {
    if      (m_buttonData == CWIID_BTN_UP)
      RtnKey = WIIREMOTE_UP;
    else if (m_buttonData == CWIID_BTN_RIGHT)
      RtnKey = WIIREMOTE_RIGHT;
    else if (m_buttonData == CWIID_BTN_LEFT)
      RtnKey = WIIREMOTE_LEFT;
    else if (m_buttonData == CWIID_BTN_DOWN)
      RtnKey = WIIREMOTE_DOWN;
      
    else if (m_buttonData == CWIID_BTN_A)
      RtnKey = WIIREMOTE_A;
    else if (m_buttonData == CWIID_BTN_B)
      RtnKey = WIIREMOTE_B;
    
    else if (m_buttonData == CWIID_BTN_MINUS)
      RtnKey = WIIREMOTE_MINUS;
    else if (m_buttonData == CWIID_BTN_PLUS)
      RtnKey =  WIIREMOTE_PLUS;
    else if (m_buttonData == CWIID_BTN_HOME)
      RtnKey = WIIREMOTE_HOME;
    
    else if (m_buttonData == CWIID_BTN_1)
      RtnKey = WIIREMOTE_1;    
    else if (m_buttonData == CWIID_BTN_2)
      RtnKey = WIIREMOTE_2;
  }
  LeaveCriticalSection(m_lock);
  return RtnKey;
}

/* Is a new pressed since last reset_Key() was called
   reset_Key() could have been called automaticly but then it isn't as strong*/
bool CWiiRemote::GetNewKey()
{
  if (m_connected)
  { 
    if (m_newKey)    
      return true;
    else
    { 
    /* This is to emulate the Keyboard's repeat button behaviour. It takes a while to initialize the repeat and after it repeats.
       We only report that there is a new key to emulate repeat behaviour, as cwiid reports buttons differently if we have defined     CWIID_OLD or not*/
      EnterCriticalSection(m_lock);
      if (m_buttonRepeat)
      {
        if (timeGetTime() - m_lastKeyPressed > WIIREMOTE_BUTTON_REPEAT_TIME)
        {
          m_newKey = true;
          m_lastKeyPressed = timeGetTime();
        }
      }
      else
      {
        if (timeGetTime() - m_lastKeyPressed > WIIREMOTE_BUTTON_DELAY_TIME)     
        {
          m_buttonRepeat = true;
          m_newKey = true;
          m_lastKeyPressed = timeGetTime();
        }
      }
      LeaveCriticalSection(m_lock);
      return m_newKey;
    }
  }
  else
    return false;
}

/* Resets the newKey flag */
void CWiiRemote::ResetKey()
{
  EnterCriticalSection(m_lock);
  m_newKey = false;
  LeaveCriticalSection(m_lock);
}

/* Enable mouse emulation */
void CWiiRemote::EnableMouseEmulation()
{
  if (m_useIRMouse)
    return;
  EnterCriticalSection(m_lock);    
  m_useIRMouse = true;

#ifndef CWIID_OLD
  //We toggle IR Reporting (Save resources?)  
  if (!(m_rptMode & CWIID_RPT_IR))
    ToggleBit(m_rptMode, CWIID_RPT_IR);   
  if (m_connected)
    SetRptMode();
#endif
  LeaveCriticalSection(m_lock);    
  
  CLog::Log(LOGNOTICE, "Enable Wiiremote mouse emulation");
}
/* Disable mouse emulation */
void CWiiRemote::DisableMouseEmulation()
{
  if (!m_useIRMouse)
    return;
  EnterCriticalSection(m_lock);  
  m_useIRMouse = false;

#ifndef CWIID_OLD
  //We toggle IR Reporting (Save resources?)  
  if (m_rptMode & CWIID_RPT_IR)
    ToggleBit(m_rptMode, CWIID_RPT_IR);
  if (m_connected)
    SetRptMode();
#endif
  LeaveCriticalSection(m_lock);    
  
  CLog::Log(LOGNOTICE, "Disable Wiiremote mouse emulation");
}

/* This will set the mouse to active */
void CWiiRemote::SetMouseActive()
{ 
  m_lastActiveTime = timeGetTime();
}

/* This will set the mouse to inactive */
void CWiiRemote::SetMouseInactive()
{ 
  m_lastActiveTime -= WIIREMOTE_ACTIVE_LENGTH;
}

/* Is a wiiremote connected*/
bool CWiiRemote::GetConnected()
{
  return m_connected;
}

/* Disconnect ASAP*/
void CWiiRemote::Disconnect()
{ //This is always called from a criticalsection
  if (m_connected)
  {
    m_DisconnectWhenPossible = true;
  }
}
		
#ifdef CWIID_OLD		
/* This function is mostly a hack as CWIID < 6.0 doesn't report on disconnects, this function is called everytime
   a message is sent to the callback (Will be once every 10 ms or so) this is to see if the connection is interupted. */
void CWiiRemote::CheckIn()
{ //This is always called from a criticalsection
  m_LastMsgTime = timeGetTime();
}		
#endif		

/* This function will enable wiiremote support if we have a bluetooth adapter present
   Returns true if enabled */
bool CWiiRemote::EnableWiiRemote()
{
  if (m_enabled)
    return true;
    
  // We check if there is a bluetooth adapter connected to the computer, hci_get_route returns -1 there aren't any
  // Note that disabled bluetooth adapters returns -1 also
  EnterCriticalSection(m_lock);
  if (hci_get_route(NULL) < 0)
  {
    m_enabled = false;
    CStdString strMsgTitle = g_localizeStrings.Get(21889);
    CStdString strMsgText = g_localizeStrings.Get(21886);
    g_application.m_guiDialogKaiToast.QueueNotification(strMsgTitle,strMsgText);
    CLog::Log(LOGERROR, "Cannot enable Wiiremote support because no bluetooth device was found");
  }
  else
  {
    m_connected = false;
    m_connectThreadRunning = false;
    m_enabled = true;  
    CLog::Log(LOGNOTICE, "Enabled Wiiremote support");
    //We could initiate the connectthread here but it will be done in the update
  }
   
  //Create a Connectthread
  CreateConnectThread();
  
  LeaveCriticalSection(m_lock);
  return m_enabled;
}

/* Disconnects any connected wiiremotes and after it disables Wiiremote support 
   Returns true on sucess */
bool CWiiRemote::DisableWiiRemote()
{
  EnterCriticalSection(m_lock);
  if (m_connected)
    DisconnectNow(false);
  
  CLog::Log(LOGNOTICE, "Disabled Wiiremote support");
  m_enabled = false;
  LeaveCriticalSection(m_lock);
  //The connectthread will disable itself
    
  return true;
}


//---------------------Private-------------------------------------------------------------------
/* Connect is designed to be run in a different thread as it only 
   exits if wiiremote is either disabled or a connection is made*/
bool CWiiRemote::Connect()
{
  if (!m_enabled) //If the Wiiremote isn't enabled we don't need to check for them
    return false;
#ifndef _DEBUG
  cwiid_set_err(ErrorCallback);
#endif
  while (!m_connected)
  {
    int flags;
    ToggleBit(flags, CWIID_FLAG_MESG_IFC);

    m_wiiremoteHandle = cwiid_connect(&m_btaddr, flags);
    if (m_wiiremoteHandle != NULL)
    {
      EnterCriticalSection(m_lock);
      SetupWiiRemote();
      // get battery state etc.
      cwiid_state wiiremote_state;
      int err = cwiid_get_state(m_wiiremoteHandle, &wiiremote_state);
      if (!err)
      {
        CStdString strMsgTitle = g_localizeStrings.Get(21887);
        CStdString strMsgTextRaw = g_localizeStrings.Get(21888);
        CStdString strMsgText;

        strMsgText.Format(strMsgTextRaw.c_str(),static_cast<int>(((float)(wiiremote_state.battery)/CWIID_BATTERY_MAX)*100.0));
        g_application.m_guiDialogKaiToast.QueueNotification(strMsgTitle,strMsgText);
      }
      else
        CLog::Log(LOGERROR, "Problem probing for status of Wiiremote; cwiid_get_state returned non-zero");
#ifdef CWIID_OLD
      /* CheckIn to say that this is the last msg, If this isn't called it could give issues if we Connects -> Disconnect and then try to connect again 
         the CWIID_OLD hack would automaticly disconnect the wiiremote as the lastmsg is too old. */
      CheckIn();
#endif      
      m_connected = true;
      LeaveCriticalSection(m_lock);
      CLog::Log(LOGNOTICE, "Sucessfully connected a Wiiremote");
      return true;
    }

    if (!m_enabled) //If the Wiiremote isn't enabled we don't need to check for them
      return false;
  }  
  return false;
}

void CWiiRemote::CreateConnectThread()
{
  if (m_enabled)
  {
    if (m_connectThread != NULL)
      m_connectThread->StopThread();
    m_connectThread = new CThread(this);
    m_connectThread->Create();
  }
}

/* Disconnect */
void CWiiRemote::DisconnectNow(bool startConnectThread)
{
  if (m_connected) //It shouldn't be enabled at the same time as it is connected
  {
    cwiid_disconnect(m_wiiremoteHandle); 
    CStdString strMsgTitle = g_localizeStrings.Get(21890);
    CStdString strMsgText = g_localizeStrings.Get(21891);
    g_application.m_guiDialogKaiToast.QueueNotification(strMsgTitle, strMsgText);
    CLog::Log(LOGNOTICE, "Sucessfully disconnected a Wiiremote");			
  }
  m_connected = false;
	
  if (startConnectThread)
    CreateConnectThread();
}

#ifdef CWIID_OLD
/* This is a harsh check if there really is a connection, It will mainly be used in CWIID < 6.0 
   as it doesn't report connect error, wich is needed to see if the Wiiremote suddenly disconnected.
   This could possible be done with bluetooth specific queries but I cannot find how to do it.  */
bool CWiiRemote::CheckConnection()
{
  if ((timeGetTime() - m_LastMsgTime) > 1000)
    return false;
  else
    return true;
}
#endif

/* Sets rpt mode when a new wiiremote is connected */
void CWiiRemote::SetupWiiRemote()
{ //Lights up the apropriate led and setups the rapport mode, so buttons and IR work
  SetRptMode();
  SetLedState();
  
  if (cwiid_set_mesg_callback(m_wiiremoteHandle, MessageCallback))
    CLog::Log(LOGERROR, "Unable to set message callback to the Wiiremote");
}

void CWiiRemote::ProcessKey(int Key)
{
  if (Key != m_buttonData)
  {
    m_buttonData = Key;
    m_newKey = true;
    m_buttonRepeat = false;
    m_lastKeyPressed = timeGetTime();
  }  
}

/* Tell cwiid wich data will be reported */
void CWiiRemote::SetRptMode()
{ //Sets our wiiremote to report something, for example IR, Buttons
#ifdef CWIID_OLD
  if (cwiid_command(m_wiiremoteHandle, CWIID_CMD_RPT_MODE, m_rptMode))
    CLog::Log(LOGERROR, "Error setting Wiiremote report mode");
#else  
  if (cwiid_set_rpt_mode(m_wiiremoteHandle, m_rptMode))
    CLog::Log(LOGERROR, "Error setting Wiiremote report mode");
#endif
}
/* Tell cwiid the LED states */
void CWiiRemote::SetLedState()
{ //Sets our leds on the wiiremote
#ifdef CWIID_OLD
  if (cwiid_command(m_wiiremoteHandle, CWIID_CMD_LED, m_ledState))
    CLog::Log(LOGERROR, "Error setting Wiiremote LED state");
#else  
  if (cwiid_set_led(m_wiiremoteHandle, m_ledState))
    CLog::Log(LOGERROR, "Error setting Wiiremote LED state");
#endif
}

/* Calculate the mousepointer from 2 IR sources (Default) */
void CWiiRemote::CalculateMousePointer(int x1, int y1, int x2, int y2)
{
  float x3, y3;
  //Get the middle of the 2 points
  x3 = CWIID_IR_X_MAX - ( (x1 + x2) / 2 );
  y3 = CWIID_IR_Y_MAX - ( (y1 + y2) / 2 );
	
  //Calculate a procentage 0.0f - 1.0f
  x3 = ((float)x3 / (float)CWIID_IR_X_MAX);
  y3 = ((float)y3 / (float)CWIID_IR_Y_MAX);
		
  //Have a safezone at the edge of the IR-remote's sight
  if (x3 < WIIREMOTE_IR_DEADZONE)
    x3 = WIIREMOTE_IR_DEADZONE;
  else if (x3 > (1.0f - WIIREMOTE_IR_DEADZONE))
    x3 = (1.0f - WIIREMOTE_IR_DEADZONE);

  if (y3 < WIIREMOTE_IR_DEADZONE)
    y3 = WIIREMOTE_IR_DEADZONE;
  else if (y3 >  (1.0f - WIIREMOTE_IR_DEADZONE))
    y3 =  (1.0f - WIIREMOTE_IR_DEADZONE);
	  
  // Stretch the values inside the deadzone to 0.0f - 1.0f  
  x3 = x3 - WIIREMOTE_IR_DEADZONE;
  y3 = y3 - WIIREMOTE_IR_DEADZONE;
	
  x3 = x3 / (1.0f - ( 2.0f * WIIREMOTE_IR_DEADZONE ) );
  y3 = y3 / (1.0f - ( 2.0f * WIIREMOTE_IR_DEADZONE ) );
	
  y3 = 1.0f - y3; //Flips the Y axis

  //TODO these calculation is easy to follow although they are hardly optimized	
  m_mouseX = x3;
  m_mouseY = y3;
  m_lastActiveTime = timeGetTime(); 
  m_haveIRSources = true;
}
#endif
