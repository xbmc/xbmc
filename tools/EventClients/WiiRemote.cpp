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

#include "WiiRemote.h"


bool g_AllowReconnect = true;
bool g_AllowMouse     = true;

long getTicks()
{
  long ticks;
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
}

//Destructor
CWiiRemote::~CWiiRemote()
{
  if (m_connected == true)
    this->DisconnectNow(false);
}

//---------------------Public-------------------------------------------------------------------
/* Basicly this just sets up standard control bits */
void CWiiRemote::SetBluetoothAddress(const char *btaddr)
{
  if (btaddr != NULL)
    str2ba(btaddr, &m_btaddr);
  else
    bacpy(&m_btaddr, &(*BDADDR_ANY));
}

void CWiiRemote::Initialize(CAddress Addr, int Socket)
{
  m_connected = false;
  m_lastKeyPressed = 0;
  m_LastKey = 0;
  m_buttonRepeat = false;
  m_useIRMouse = true;
  m_rptMode = 0;

  m_Socket = Socket;
  m_MyAddr = Addr;

#ifdef CWIID_OLD
  m_LastMsgTime = getTicks();
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

  printf("Enabled WiiRemote mouse emulation\n");
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

  printf("Disabled WiiRemote mouse emulation\n");
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
    CPacketPING ping;
    ping.Send(m_Socket, m_MyAddr);
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
        CPacketNOTIFICATION notification("Wii Remote connected", Mesg, ICON_PNG, "icons/bluetooth.png");
        notification.Send(m_Socket, m_MyAddr);
      }
      else
      {
        printf("Problem probing for status of Wiiremote; cwiid_get_state returned non-zero\n");
        CPacketNOTIFICATION notification("Wii Remote connected", "", ICON_PNG, "icons/bluetooth.png");
        notification.Send(m_Socket, m_MyAddr);
      }
#ifdef CWIID_OLD
      /* CheckIn to say that this is the last msg, If this isn't called it could give issues if we Connects -> Disconnect and then try to connect again 
         the CWIID_OLD hack would automaticly disconnect the wiiremote as the lastmsg is too old. */
      CheckIn();
#endif      
      m_connected = true;

      printf("Sucessfully connected a WiiRemote\n");
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
      CPacketNOTIFICATION notification("Wii Remote disconnected", "Press 1 and 2 to reconnect", ICON_PNG, "icons/bluetooth.png");
      notification.Send(m_Socket, m_MyAddr);
    }
    else
    {
      CPacketNOTIFICATION notification("Wii Remote disconnected", "", ICON_PNG, "icons/bluetooth.png");
      notification.Send(m_Socket, m_MyAddr);
    }

    printf("Sucessfully disconnected a Wiiremote\n");
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
    printf("Lost connection to the WiiRemote\n");
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
  
  if (cwiid_set_mesg_callback(m_wiiremoteHandle, MessageCallback))
    printf("Unable to set message callback to the Wiiremote\n");
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

  char *RtnKey = NULL;

  if      (Key == CWIID_BTN_UP)
    RtnKey = "up";
  else if (Key == CWIID_BTN_RIGHT)
    RtnKey = "right";
  else if (Key == CWIID_BTN_LEFT)
    RtnKey = "left";
  else if (Key == CWIID_BTN_DOWN)
    RtnKey = "down";
    
  else if (Key == CWIID_BTN_A)
    RtnKey = "return";
  else if (Key == CWIID_BTN_B)
    RtnKey = "escape";
  
  else if (Key == CWIID_BTN_MINUS)
    RtnKey = "volume_down";
  else if (Key == CWIID_BTN_PLUS)
    RtnKey = "volume_up";

  else if (Key == CWIID_BTN_HOME)
    RtnKey = "browser_home";
  
  else if (Key == CWIID_BTN_1)
    RtnKey = "menu";
  else if (Key == CWIID_BTN_2)
    RtnKey = "tab";

  if (RtnKey != NULL)
  {
    CPacketBUTTON btn(RtnKey, "KB", true);
    btn.Send(m_Socket, m_MyAddr);
//    m_LastKey = Key;
//    m_lastKeyPressed = getTicks();
  }
}

/* Tell cwiid wich data will be reported */
void CWiiRemote::SetRptMode()
{ //Sets our wiiremote to report something, for example IR, Buttons
#ifdef CWIID_OLD
  if (cwiid_command(m_wiiremoteHandle, CWIID_CMD_RPT_MODE, m_rptMode))
    printf("Error setting Wiiremote report mode\n");
#else  
  if (cwiid_set_rpt_mode(m_wiiremoteHandle, m_rptMode))
    printf("Error setting Wiiremote report mode\n");
#endif
}
/* Tell cwiid the LED states */
void CWiiRemote::SetLedState()
{ //Sets our leds on the wiiremote
#ifdef CWIID_OLD
  if (cwiid_command(m_wiiremoteHandle, CWIID_CMD_LED, m_ledState))
    printf("Error setting Wiiremote LED state\n");
#else  
  if (cwiid_set_led(m_wiiremoteHandle, m_ledState))
    printf("Error setting Wiiremote LED state\n");
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

  m_lastActiveTime = getTicks(); 
  m_haveIRSources = true;

  int x4 = (int)(x3 * 65535);
  int y4 = (int)(y3 * 65535);

  CPacketMOUSE mouse(x4, y4);
  mouse.Send(m_Socket, m_MyAddr);
}

















void PrintHelp(const char *Prog)
{
  printf("Commands:\n");
  printf("\t--disable-mouseemulation\n\t--disable-reconnect\n");
  printf("\t--address ADDRESS\n\t--port PORT\n");
  printf("\t--btaddr MACADDRESS\n");
}

int main(int argc, char **argv)
{
  char *Address = NULL;
  char *btaddr  = NULL;
  int  Port = 9777;

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
    else if (strcmp(argv[i], "--address") == 0 && ((i + 1) <= argc))
      Address = argv[i + 1];
    else if (strcmp(argv[i], "--port") == 0 && ((i + 1) <= argc))
      Port = atoi(argv[i + 1]);
    else if (strcmp(argv[i], "--btaddr") == 0 && ((i + 1) <= argc))
      btaddr = argv[i + 1];
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
    printf("Error No bluetooth device\n");
    return -1;
  }

  g_WiiRemote.Initialize(my_addr, sockfd);
  g_WiiRemote.SetBluetoothAddress(btaddr);
  if (g_AllowMouse)
    g_WiiRemote.EnableMouseEmulation();
  else
    g_WiiRemote.DisableMouseEmulation();
 
  CPacketHELO HeloPackage("WiiRemote", ICON_PNG, "icons/bluetooth.png");
  HeloPackage.Send(sockfd, my_addr);
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
    CPacketPING ping;
    ping.Send(sockfd, my_addr);
    g_WiiRemote.Update();
  }
}
