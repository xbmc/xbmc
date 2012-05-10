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

// Compiles with g++ WiiRemote.cpp -lcwiid -o WiiRemote
// Preferably with libcwiid >= 6.0

#include "CWIID_WiiRemote.h"

#include <unistd.h>

bool g_AllowReconnect = true;
bool g_AllowMouse     = true;
bool g_AllowNunchuck  = true;

CPacketHELO *g_Ping = NULL;

#ifndef ICON_PATH
#define ICON_PATH "../../"
#endif
std::string g_BluetoothIconPath = std::string(ICON_PATH) + std::string("/bluetooth.png");

int32_t getTicks()
{
  int32_t ticks;
  struct timeval now;
  gettimeofday(&now, NULL);
  ticks = now.tv_sec * 1000l;
  ticks += now.tv_usec / 1000l;
  return ticks;
}

CWiiRemote g_WiiRemote;

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
      g_WiiRemote.ProcessNunchuck(mesg[i].nunchuk_mesg);
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
}

#ifndef _DEBUG
/* This takes the errors generated at pre-connect and silence them as they are mostly not needed */
void CWiiRemote::ErrorCallback(struct wiimote *wiiremote, const char *str, va_list ap)
{
  //Error checking
}
#endif

//Constructor
/*This constructor is never used but it shows how one would connect to just a specific Wiiremote by Mac-Adress*/
CWiiRemote::CWiiRemote(char *wii_btaddr)
{
  SetBluetoothAddress(wii_btaddr);
  m_SamplesX = NULL;
  m_SamplesY = NULL;

  m_JoyMap   = NULL;
}

//Destructor
CWiiRemote::~CWiiRemote()
{
  if (m_connected == true)
    this->DisconnectNow(false);

  if (m_SamplesY != NULL)
    free(m_SamplesY);
  if (m_SamplesX != NULL)
    free(m_SamplesX);

  if (m_JoyMap)
    free(m_JoyMap);
}

//---------------------Public-------------------------------------------------------------------
/* Basicly this just sets up standard control bits */
void CWiiRemote::SetBluetoothAddress(const char *btaddr)
{
  static const bdaddr_t b = {{0, 0, 0, 0, 0, 0}}; /* BDADDR_ANY */
  if (btaddr != NULL)
    str2ba(btaddr, &m_btaddr);
  else
    bacpy(&m_btaddr, &b);
}

void CWiiRemote::SetSensativity(float DeadX, float DeadY, int NumSamples)
{
  m_NumSamples = NumSamples;

  m_MinX = MOUSE_MAX * DeadX;
  m_MinY = MOUSE_MAX * DeadY;

  m_MaxX = MOUSE_MAX * (1.0f + DeadX + DeadX);
  m_MaxY = MOUSE_MAX * (1.0f + DeadY + DeadY);

  if (m_SamplesY != NULL)
    delete [] m_SamplesY;
  if (m_SamplesX != NULL)
    delete [] m_SamplesX;

  m_SamplesY = new int[m_NumSamples];
  m_SamplesX = new int[m_NumSamples];
}

void CWiiRemote::SetJoystickMap(const char *JoyMap)
{
  if (m_JoyMap)
    free(m_JoyMap);
  if (JoyMap != NULL)
  {
    m_JoyMap = (char*)malloc(strlen(JoyMap) + 5);
    sprintf(m_JoyMap, "JS0:%s", JoyMap);
  }
  else
    m_JoyMap = strdup("JS0:WiiRemote");
}

void CWiiRemote::Initialize(CAddress Addr, int Socket)
{
  m_connected = false;
  m_lastKeyPressed          = 0;
  m_LastKey                 = 0;
  m_buttonRepeat            = false;
  m_lastKeyPressedNunchuck  = 0;
  m_LastKeyNunchuck         = 0;
  m_buttonRepeatNunchuck    = false;
  m_useIRMouse              = true;
  m_rptMode                 = 0;

  m_Socket = Socket;
  m_MyAddr = Addr;

  m_NumSamples = WIIREMOTE_SAMPLES;

  m_MaxX = WIIREMOTE_X_MAX;
  m_MaxY = WIIREMOTE_Y_MAX;
  m_MinX = WIIREMOTE_X_MIN;
  m_MinY = WIIREMOTE_Y_MIN;
#ifdef CWIID_OLD
  m_LastMsgTime = getTicks();
#endif

  //All control bits are set to false when cwiid is started
  //Report Button presses
  ToggleBit(m_rptMode, CWIID_RPT_BTN);
  if (g_AllowNunchuck)
    ToggleBit(m_rptMode, CWIID_RPT_NUNCHUK);

  //If wiiremote is used as a mouse, then report the IR sources
#ifndef CWIID_OLD  
  if (m_useIRMouse) 
#endif
    ToggleBit(m_rptMode, CWIID_RPT_IR);	

  //Have the first and fourth LED on the Wiiremote shine when connected
  ToggleBit(m_ledState, CWIID_LED1_ON);	
  ToggleBit(m_ledState, CWIID_LED4_ON);
}

/* Update is run regulary and we gather the state of the Wiiremote and see if the user have pressed on a button or moved the wiiremote
   This could have been done with callbacks instead but it doesn't look nice in C++*/
void CWiiRemote::Update()
{
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
      return;
    }
  }
#endif
}

/* Enable mouse emulation */
void CWiiRemote::EnableMouseEmulation()
{
  if (m_useIRMouse)
    return;

  m_useIRMouse = true;

#ifndef CWIID_OLD
  //We toggle IR Reporting (Save resources?)  
  if (!(m_rptMode & CWIID_RPT_IR))
    ToggleBit(m_rptMode, CWIID_RPT_IR);   
  if (m_connected)
    SetRptMode();
#endif

  CPacketLOG log(LOGDEBUG, "Enabled WiiRemote mouse emulation");
  log.Send(m_Socket, m_MyAddr);
}
/* Disable mouse emulation */
void CWiiRemote::DisableMouseEmulation()
{
  if (!m_useIRMouse)
    return;

  m_useIRMouse = false;
#ifndef CWIID_OLD
  //We toggle IR Reporting (Save resources?)  
  if (m_rptMode & CWIID_RPT_IR)
    ToggleBit(m_rptMode, CWIID_RPT_IR);
  if (m_connected)
    SetRptMode();
#endif

  CPacketLOG log(LOGDEBUG, "Disabled WiiRemote mouse emulation");
  log.Send(m_Socket, m_MyAddr);
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
    m_DisconnectWhenPossible = true;
}
		
#ifdef CWIID_OLD		
/* This function is mostly a hack as CWIID < 6.0 doesn't report on disconnects, this function is called everytime
   a message is sent to the callback (Will be once every 10 ms or so) this is to see if the connection is interupted. */
void CWiiRemote::CheckIn()
{ //This is always called from a criticalsection
  m_LastMsgTime = getTicks();
}		
#endif		

//---------------------Private-------------------------------------------------------------------
/* Connect is designed to be run in a different thread as it only 
   exits if wiiremote is either disabled or a connection is made*/
bool CWiiRemote::Connect()
{
#ifndef _DEBUG
  cwiid_set_err(ErrorCallback);
#endif
  while (!m_connected)
  {
    g_Ping->Send(m_Socket, m_MyAddr);
    int flags = 0;
    ToggleBit(flags, CWIID_FLAG_MESG_IFC);
    ToggleBit(flags, CWIID_FLAG_REPEAT_BTN);

    m_wiiremoteHandle = cwiid_connect(&m_btaddr, flags);
    if (m_wiiremoteHandle != NULL)
    {
      SetupWiiRemote();
      // get battery state etc.
      cwiid_state wiiremote_state;
      int err = cwiid_get_state(m_wiiremoteHandle, &wiiremote_state);
      if (!err)
      {
        char Mesg[1024];
        sprintf(Mesg, "%i%% battery remaining", static_cast<int>(((float)(wiiremote_state.battery)/CWIID_BATTERY_MAX)*100.0));
        CPacketNOTIFICATION notification("Wii Remote connected", Mesg, ICON_PNG, g_BluetoothIconPath.c_str());
        notification.Send(m_Socket, m_MyAddr);
      }
      else
      {
        printf("Problem probing for status of WiiRemote; cwiid_get_state returned non-zero\n");
        CPacketLOG log(LOGNOTICE, "Problem probing for status of WiiRemote; cwiid_get_state returned non-zero");
        log.Send(m_Socket, m_MyAddr);
        CPacketNOTIFICATION notification("Wii Remote connected", "", ICON_PNG, g_BluetoothIconPath.c_str());
        notification.Send(m_Socket, m_MyAddr);
      }
#ifdef CWIID_OLD
      /* CheckIn to say that this is the last msg, If this isn't called it could give issues if we Connects -> Disconnect and then try to connect again 
         the CWIID_OLD hack would automaticly disconnect the wiiremote as the lastmsg is too old. */
      CheckIn();
#endif      
      m_connected = true;

      CPacketLOG log(LOGNOTICE, "Sucessfully connected a WiiRemote");
      log.Send(m_Socket, m_MyAddr);
      return true;
    }
    //Here's a good place to have a quit flag check...

  }
  return false;
}

/* Disconnect */
void CWiiRemote::DisconnectNow(bool startConnectThread)
{
  if (m_connected) //It shouldn't be enabled at the same time as it is connected
  {
    cwiid_disconnect(m_wiiremoteHandle);

    if (g_AllowReconnect)
    {
      CPacketNOTIFICATION notification("Wii Remote disconnected", "Press 1 and 2 to reconnect", ICON_PNG, g_BluetoothIconPath.c_str());
      notification.Send(m_Socket, m_MyAddr);
    }
    else
    {
      CPacketNOTIFICATION notification("Wii Remote disconnected", "", ICON_PNG, g_BluetoothIconPath.c_str());
      notification.Send(m_Socket, m_MyAddr);
    }

    CPacketLOG log(LOGNOTICE, "Sucessfully disconnected a WiiRemote");
    log.Send(m_Socket, m_MyAddr);
  }
  m_connected = false;
}

#ifdef CWIID_OLD
/* This is a harsh check if there really is a connection, It will mainly be used in CWIID < 6.0 
   as it doesn't report connect error, wich is needed to see if the Wiiremote suddenly disconnected.
   This could possible be done with bluetooth specific queries but I cannot find how to do it.  */
bool CWiiRemote::CheckConnection()
{
  if ((getTicks() - m_LastMsgTime) > 1000)
  {
    CPacketLOG log(LOGNOTICE, "Lost connection to the WiiRemote");
    log.Send(m_Socket, m_MyAddr);
    return false;
  }
  else
    return true;
}
#endif

/* Sets rpt mode when a new wiiremote is connected */
void CWiiRemote::SetupWiiRemote()
{ //Lights up the apropriate led and setups the rapport mode, so buttons and IR work
  SetRptMode();
  SetLedState();

  for (int i = 0; i < WIIREMOTE_SAMPLES; i++)
  {
    m_SamplesX[i] = 0;
    m_SamplesY[i] = 0;
  }

  if (cwiid_set_mesg_callback(m_wiiremoteHandle, MessageCallback))
  {
    CPacketLOG log(LOGERROR, "Unable to set message callback to the WiiRemote");
    log.Send(m_Socket, m_MyAddr);
  }
}

void CWiiRemote::ProcessKey(int Key)
{
  if (Key != m_LastKey)
  {
    m_LastKey = Key;
    m_lastKeyPressed = getTicks();
    m_buttonRepeat = false;
  }
  else
  {
    if (m_buttonRepeat)
    {
      if (getTicks() - m_lastKeyPressed > WIIREMOTE_BUTTON_REPEAT_TIME)
        m_lastKeyPressed = getTicks();
      else
        return;
    }
    else
    {
      if (getTicks() - m_lastKeyPressed > WIIREMOTE_BUTTON_DELAY_TIME)     
      {
        m_buttonRepeat = true;
        m_lastKeyPressed = getTicks();
      }
      else
        return;
    }
  }

  int RtnKey = -1;

  if      (Key == CWIID_BTN_UP)
    RtnKey = 1;
  else if (Key == CWIID_BTN_RIGHT)
    RtnKey = 4;
  else if (Key == CWIID_BTN_LEFT)
    RtnKey = 3;
  else if (Key == CWIID_BTN_DOWN)
    RtnKey = 2;
    
  else if (Key == CWIID_BTN_A)
    RtnKey = 5;
  else if (Key == CWIID_BTN_B)
    RtnKey = 6;
  
  else if (Key == CWIID_BTN_MINUS)
    RtnKey = 7;
  else if (Key == CWIID_BTN_PLUS)
    RtnKey = 9;

  else if (Key == CWIID_BTN_HOME)
    RtnKey = 8;
  
  else if (Key == CWIID_BTN_1)
    RtnKey = 10;
  else if (Key == CWIID_BTN_2)
    RtnKey = 11;

  if (RtnKey != -1)
  {
    CPacketBUTTON btn(RtnKey, m_JoyMap, BTN_QUEUE | BTN_NO_REPEAT);
    btn.Send(m_Socket, m_MyAddr);
  }
}

void CWiiRemote::ProcessNunchuck(struct cwiid_nunchuk_mesg &Nunchuck)
{
  if (Nunchuck.stick[0] > 135)
  { //R
    int x = (int)((((float)Nunchuck.stick[0] - 135.0f) / 95.0f) * 65535.0f);
    printf("Right: %i\n", x);
    CPacketBUTTON btn(24, m_JoyMap, (BTN_QUEUE | BTN_DOWN), x);
    btn.Send(m_Socket, m_MyAddr);
  }
  else if (Nunchuck.stick[0] < 125)
  { //L
    int x = (int)((((float)Nunchuck.stick[0] - 125.0f) / 90.0f) * -65535.0f);
    printf("Left: %i\n", x);
    CPacketBUTTON btn(23, m_JoyMap, (BTN_QUEUE | BTN_DOWN), x);
    btn.Send(m_Socket, m_MyAddr);
  }

  if (Nunchuck.stick[1] > 130)
  { //U
    int x = (int)((((float)Nunchuck.stick[1] - 130.0f) / 92.0f) * 65535.0f);
    printf("Up: %i\n", x);
    CPacketBUTTON btn(21, m_JoyMap, (BTN_QUEUE | BTN_DOWN), x);
    btn.Send(m_Socket, m_MyAddr);
  }
  else if (Nunchuck.stick[1] < 120)
  { //D
    int x = (int)((((float)Nunchuck.stick[1] - 120.0f) / 90.0f) * -65535.0f);
    printf("Down: %i\n", x);
    CPacketBUTTON btn(22, m_JoyMap, (BTN_QUEUE | BTN_DOWN), x);
    btn.Send(m_Socket, m_MyAddr);
  }

  if (Nunchuck.buttons != m_LastKeyNunchuck)
  {
    m_LastKeyNunchuck = Nunchuck.buttons;
    m_lastKeyPressedNunchuck = getTicks();
    m_buttonRepeatNunchuck = false;
  }
  else
  {
    if (m_buttonRepeatNunchuck)
    {
      if (getTicks() - m_lastKeyPressedNunchuck > WIIREMOTE_BUTTON_REPEAT_TIME)
        m_lastKeyPressedNunchuck = getTicks();
      else
        return;
    }
    else
    {
      if (getTicks() - m_lastKeyPressedNunchuck > WIIREMOTE_BUTTON_DELAY_TIME)     
      {
        m_buttonRepeatNunchuck = true;
        m_lastKeyPressedNunchuck = getTicks();
      }
      else
        return;
    }
  }

  int RtnKey = -1;

  if      (Nunchuck.buttons == CWIID_NUNCHUK_BTN_C)
    RtnKey = 25;
  else if (Nunchuck.buttons == CWIID_NUNCHUK_BTN_Z)
    RtnKey = 26;

  if (RtnKey != -1)
  {
    CPacketBUTTON btn(RtnKey, m_JoyMap, BTN_QUEUE | BTN_NO_REPEAT);
    btn.Send(m_Socket, m_MyAddr);
  }
}

/* Tell cwiid wich data will be reported */
void CWiiRemote::SetRptMode()
{ //Sets our wiiremote to report something, for example IR, Buttons
#ifdef CWIID_OLD
  if (cwiid_command(m_wiiremoteHandle, CWIID_CMD_RPT_MODE, m_rptMode))
#else  
  if (cwiid_set_rpt_mode(m_wiiremoteHandle, m_rptMode))
#endif
  {
    CPacketLOG log(LOGERROR, "Error setting WiiRemote report mode");
    log.Send(m_Socket, m_MyAddr);
  }
}
/* Tell cwiid the LED states */
void CWiiRemote::SetLedState()
{ //Sets our leds on the wiiremote
#ifdef CWIID_OLD
  if (cwiid_command(m_wiiremoteHandle, CWIID_CMD_LED, m_ledState))
#else  
  if (cwiid_set_led(m_wiiremoteHandle, m_ledState))
#endif
  {
    CPacketLOG log(LOGERROR, "Error setting WiiRemote LED state");
    log.Send(m_Socket, m_MyAddr);
  }
}

/* Calculate the mousepointer from 2 IR sources (Default) */
void CWiiRemote::CalculateMousePointer(int x1, int y1, int x2, int y2)
{
  int x3, y3;

  x3 = ( (x1 + x2) / 2 );
  y3 = ( (y1 + y2) / 2 );

  x3 = (int)( ((float)x3 / (float)CWIID_IR_X_MAX) * m_MaxX);
  y3 = (int)( ((float)y3 / (float)CWIID_IR_Y_MAX) * m_MaxY);

  x3 = (int)(x3 - m_MinX);
  y3 = (int)(y3 - m_MinY);

  if      (x3 < MOUSE_MIN)  x3 = MOUSE_MIN;
  else if (x3 > MOUSE_MAX)  x3 = MOUSE_MAX;

  if      (y3 < MOUSE_MIN)  y3 = MOUSE_MIN;
  else if (y3 > MOUSE_MAX)  y3 = MOUSE_MAX;

  x3 = MOUSE_MAX - x3;

  if (m_NumSamples == 1)
  {
    CPacketMOUSE mouse(x3, y3);
    mouse.Send(m_Socket, m_MyAddr);
    return;
  }
  else
  {
    for (int i = m_NumSamples; i > 0; i--)
    {
      m_SamplesX[i] =  m_SamplesX[i-1];
      m_SamplesY[i] =  m_SamplesY[i-1];
    }

    m_SamplesX[0] = x3;
    m_SamplesY[0] = y3;

    long x4 = 0, y4 = 0;

    for (int i = 0; i < m_NumSamples; i++)
    {
      x4 += m_SamplesX[i];
      y4 += m_SamplesY[i];
    }
    CPacketMOUSE mouse((x4 / m_NumSamples), (y4 / m_NumSamples));
    mouse.Send(m_Socket, m_MyAddr);
  }
}
















void PrintHelp(const char *Prog)
{
  printf("Commands:\n");
  printf("\t--disable-mouseemulation\n\t--disable-reconnect\n\t--disable-nunchuck\n");
  printf("\t--address ADDRESS\n\t--port PORT\n");
  printf("\t--btaddr MACADDRESS\n");
  printf("\t--deadzone-x DEADX          | Number between 0 - 100 (Default: %i)\n", (int)(DEADZONE_X * 100));
  printf("\t--deadzone-y DEADY          | Number between 0 - 100 (Default: %i)\n", (int)(DEADZONE_Y * 100));
  printf("\t--deadzone DEAD             | Sets both X and Y too the number\n");
  printf("\t--smoothing-samples SAMPLE  | Number 1 counts as Off (Default: %i)\n", WIIREMOTE_SAMPLES);
  printf("\t--joystick-map JOYMAP       | The string ID for the joymap (Default: WiiRemote)\n");
}

int main(int argc, char **argv)
{
  char *Address = NULL;
  char *btaddr  = NULL;
  int  Port = 9777;

  int NumSamples = WIIREMOTE_SAMPLES;
  float DeadX    = DEADZONE_X;
  float DeadY    = DEADZONE_Y;

  char *JoyMap = NULL;

  for (int i = 0; i < argc; i++)
  {
    if (strcmp(argv[i], "--help") == 0)
    {
      PrintHelp(argv[0]);
      return 0;
    }
    else if (strcmp(argv[i], "--disable-mouseemulation") == 0)
      g_AllowMouse = false;
    else if (strcmp(argv[i], "--disable-reconnect") == 0)
      g_AllowReconnect = false;
    else if (strcmp(argv[i], "--disable-nunchuck") == 0)
      g_AllowNunchuck = false;
    else if (strcmp(argv[i], "--address") == 0 && ((i + 1) <= argc))
      Address = argv[i + 1];
    else if (strcmp(argv[i], "--port") == 0 && ((i + 1) <= argc))
      Port = atoi(argv[i + 1]);
    else if (strcmp(argv[i], "--btaddr") == 0 && ((i + 1) <= argc))
      btaddr = argv[i + 1];
    else if (strcmp(argv[i], "--deadzone-x") == 0 && ((i + 1) <= argc))
      DeadX = ((float)atoi(argv[i + 1]) / 100.0f);
    else if (strcmp(argv[i], "--deadzone-y") == 0 && ((i + 1) <= argc))
      DeadY = ((float)atoi(argv[i + 1]) / 100.0f);
    else if (strcmp(argv[i], "--deadzone") == 0 && ((i + 1) <= argc))
      DeadX = DeadY = ((float)atoi(argv[i + 1]) / 100.0f);
    else if (strcmp(argv[i], "--smoothing-samples") == 0 && ((i + 1) <= argc))
      NumSamples = atoi(argv[i + 1]);
    else if (strcmp(argv[i], "--joystick-map") == 0 && ((i + 1) <= argc))
      JoyMap = argv[i + 1];
  }

  if (NumSamples < 1 || DeadX < 0 || DeadY < 0 || DeadX > 1 || DeadY > 1)
  {
    PrintHelp(argv[0]);
    return -1;
  }

  CAddress my_addr(Address, Port); // Address => localhost on 9777
  int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0)
  {
    printf("Error creating socket\n");
    return -1;
  }

  if (hci_get_route(NULL) < 0)
  {
    CPacketLOG log(LOGERROR, "Error No bluetooth device");
    log.Send(sockfd, my_addr);
    return -1;
  }
  g_Ping = new CPacketHELO("WiiRemote", ICON_PNG, g_BluetoothIconPath.c_str());
  g_WiiRemote.Initialize(my_addr, sockfd);
  g_WiiRemote.SetBluetoothAddress(btaddr);
  g_WiiRemote.SetSensativity(DeadX, DeadY, NumSamples);
  g_WiiRemote.SetSensativity(DeadX, DeadY, NumSamples);
  g_WiiRemote.SetJoystickMap(JoyMap);
  if (g_AllowMouse)
    g_WiiRemote.EnableMouseEmulation();
  else
    g_WiiRemote.DisableMouseEmulation();
 
  g_Ping->Send(sockfd, my_addr);
  bool HaveConnected = false;
  while (true)
  {
    bool Connected = g_WiiRemote.GetConnected();

    while (!Connected)
    {
      if (HaveConnected && !g_AllowReconnect)
        exit(0);

      Connected = g_WiiRemote.Connect();
      HaveConnected = true;
    }
#ifdef CWIID_OLD
//  Update the state of the WiiRemote more often when we have the old lib due too it not telling when disconnected..
    sleep (5);
#else
    sleep (15);
#endif
    g_Ping->Send(sockfd, my_addr);
    g_WiiRemote.Update();
  }
}
