/*
* XBoxMediaCenter
* UDP Event Server
* Copyright (c) 2008 d4rk
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "stdafx.h"

#ifdef HAS_EVENT_SERVER

#include "EventClient.h"
#include "EventPacket.h"
#include "Application.h"
#include "SingleLock.h"
#include "ButtonTranslator.h"
#include "GraphicContext.h"
#include "Key.h"
#include <map>
#include <queue>
#include "Util.h"

using namespace EVENTCLIENT;
using namespace EVENTPACKET;
using namespace std;

/************************************************************************/
/* CEventButtonState                                                    */
/************************************************************************/
void CEventButtonState::Load()
{
  if ( (m_iKeyCode == 0) )
  { 
    if ( (m_mapName.length() > 0) && (m_buttonName.length() > 0) )
    {
      if ( m_mapName.compare("KB") == 0 ) // standard keyboard map
      {
        m_iKeyCode = g_buttonTranslator.TranslateKeyboardString( m_buttonName.c_str() );
      }
      else if  ( m_mapName.compare("XG") == 0 ) // xbox gamepad map
      {
        m_iKeyCode = g_buttonTranslator.TranslateGamepadString( m_buttonName.c_str() );
      }
      else if  ( m_mapName.compare("R1") == 0 ) // xbox remote map
      {
        m_iKeyCode = g_buttonTranslator.TranslateRemoteString( m_buttonName.c_str() );
      }
      else if  ( m_mapName.compare("R2") == 0 ) // xbox unviversal remote map
      {
        m_iKeyCode = g_buttonTranslator.TranslateUniversalRemoteString( m_buttonName.c_str() );
      }
      else if ( (m_mapName.length() > 3) && 
                (m_mapName.compare(0, 2, "LI:") == 0) ) // starts with LI: ?
      {
#ifdef HAS_LIRC
        string lircDevice = m_mapName.substr(3);
        m_iKeyCode = g_buttonTranslator.TranslateLircRemoteString( lircDevice.c_str(),
                                                                   m_buttonName.c_str() );
#else
        CLog::Log(LOGERROR, "ES: LIRC support not enabled");
#endif
      }
      else
      {
        Reset(); // disable key since its invalid
        CLog::Log(LOGERROR, "ES: Could not map %s : %s to a key", m_mapName.c_str(),
                  m_buttonName.c_str());
      }
    }
  }
  else
  {
    if (m_mapName.length() > 3 && 
        (m_mapName.compare(0, 2, "JS") == 0) )
    {
      m_joystickName = m_mapName.substr(2);  // <num>:joyname
      m_iControllerNumber = (unsigned char)(*(m_joystickName.c_str()))
        - (unsigned char)'0'; // convert <num> to int
      m_joystickName = m_joystickName.substr(2); // extract joyname
    }
  }
}

/************************************************************************/
/* CEventClient                                                         */
/************************************************************************/
bool CEventClient::AddPacket(CEventPacket *packet)
{
  if (!packet)
    return false;

  ResetTimeout();
  if ( packet->Size() > 1 )
  {
    // TODO: limit payload size
    m_seqPackets[ packet->Sequence() ] = packet;
    if (m_seqPackets.size() == packet->Size())
    {
      unsigned int iSeqPayloadSize = 0;
      for (unsigned int i = 1 ; i<=packet->Size() ; i++)
      {
        iSeqPayloadSize += m_seqPackets[i]->PayloadSize();
      }
      unsigned int offset = 0;
      void *newPayload = NULL;
      newPayload = malloc(iSeqPayloadSize);
      if (newPayload)
      {
        unsigned char *payloadPtr = (unsigned char *)newPayload;
        for (unsigned int i = 1 ; i<=packet->Size() ; i++)
        {
          memcpy((void*)(payloadPtr + offset), m_seqPackets[i]->Payload(),
                 m_seqPackets[i]->PayloadSize());
          offset += m_seqPackets[i]->PayloadSize();
          if (i>1)
          {
            delete m_seqPackets[i];
            m_seqPackets[i] = NULL;
          }
        }
        m_seqPackets[1]->SetPayload(iSeqPayloadSize, newPayload);
        m_readyPackets.push(m_seqPackets[1]);
        m_seqPackets.clear();
      }
      else
      {
        CLog::Log(LOGERROR, "ES: Could not assemble packets, Out of Memory");
        FreePacketQueues();
        return false;
      }
    }
  }
  else
  {
    m_readyPackets.push(packet);
  }
  return true;
}

void CEventClient::ExecuteEvents()
{
  if (m_readyPackets.size() > 0)
  {
    while ( ! m_readyPackets.empty() )
    {
      ProcessPacket( m_readyPackets.front() );
      if ( ! m_readyPackets.empty() ) // in case the BYE packet cleared the queues
      {
        delete m_readyPackets.front();
        m_readyPackets.pop();
      }
    }
  }
}

bool CEventClient::ProcessPacket(CEventPacket *packet)
{
  if (!packet)
    return false;

  bool valid = false;

  switch (packet->Type())
  {
  case PT_HELO:
    valid = OnPacketHELO(packet);
    break;

  case PT_BYE:
    valid = OnPacketBYE(packet);
    break;

  case PT_BUTTON:
    valid = OnPacketBUTTON(packet);
    break;

  case PT_MOUSE:
    valid = OnPacketMOUSE(packet);
    break;

  case PT_NOTIFICATION:
    valid = OnPacketNOTIFICATION(packet);
    break;

  case PT_PING:
    valid = true;
    break;

  case PT_LOG:
    valid = OnPacketLOG(packet);
    break;

  case PT_ACTION:
    valid = OnPacketACTION(packet);
    break;

  default:
    CLog::Log(LOGDEBUG, "ES: Got Unknown Packet");
    break;
  }

  if (valid)
    ResetTimeout();

  return valid;
}

bool CEventClient::OnPacketHELO(CEventPacket *packet)
{
  // TODO: check it last HELO packet was received less than 5 minutes back
  //       if so, do not show notification of connection.
  if (Greeted())
    return false;

  unsigned char *payload = (unsigned char *)packet->Payload();
  int psize = (int)packet->PayloadSize();

  // parse device name
  if (!ParseString(payload, psize, m_deviceName))
    return false;

  CLog::Log(LOGNOTICE, "ES: Incoming connection from %s", m_deviceName.c_str());

  // icon type
  unsigned char ltype;
  if (!ParseByte(payload, psize, ltype))
    return false;
  m_eLogoType = (LogoType)ltype;

  // client's port (if any)
  unsigned short dport;
  if (!ParseUInt16(payload, psize, dport))
    return false;
  m_iRemotePort = (unsigned int)dport;

  // 2 x reserved uint32 (8 bytes)
  unsigned int reserved;
  ParseUInt32(payload, psize, reserved);
  ParseUInt32(payload, psize, reserved);

  // image data if any
  string iconfile = "Z:\\helo";
  if (m_eLogoType != LT_NONE && psize>0)
  {
    switch (m_eLogoType)
    {
    case LT_JPEG:
      iconfile += ".jpg";
      break;

    case LT_GIF:
      iconfile += ".gif";
      break;

    default:
      iconfile += ".png";
      break;
    }
    FILE * f = fopen(_P(iconfile.c_str()), "wb");
    if (f)
    {
      fwrite((const void *)payload, psize, 1, f);
      fclose(f);
    }
    else
    {
      CLog::Log(LOGERROR, "ES: Could not write icon file");
      m_eLogoType = LT_NONE;
    }
  }

  m_bGreeted = true;
  if (m_eLogoType == LT_NONE)
  {
    g_application.m_guiDialogKaiToast.QueueNotification("Detected New Connection",
                                                        m_deviceName.c_str());
  }
  else
  {
    g_application.m_guiDialogKaiToast.QueueNotification(iconfile.c_str(),
                                                        "Detected New Connection",
                                                        m_deviceName.c_str());
  }
  return true;
}

bool CEventClient::OnPacketBYE(CEventPacket *packet)
{
  if (!Greeted())
    return false;

  m_bGreeted = false;
  FreePacketQueues();
  m_currentButton.Reset();
  
  return true;
}

bool CEventClient::OnPacketBUTTON(CEventPacket *packet)
{
  unsigned char *payload = (unsigned char *)packet->Payload();
  int psize = (int)packet->PayloadSize();

  string map, button;
  unsigned short flags;
  unsigned short bcode;
  unsigned short amount;

  // parse the button code
  if (!ParseUInt16(payload, psize, bcode))
    return false;

  // parse flags
  if (!ParseUInt16(payload, psize, flags))
    return false;

  // parse amount
  if (!ParseUInt16(payload, psize, amount))
    return false;

  // parse the map to use
  if (!ParseString(payload, psize, map))
    return false;

  // parse button name
  if (flags & PTB_USE_NAME)
  {
    if (!ParseString(payload, psize, button))
      return false;
  }

  if (flags & PTB_QUEUE)
  {
    CSingleLock lock(m_critSection);
    m_buttonQueue.push (
      new CEventButtonState( (flags & PTB_USE_NAME) ? 0 : 
                             ( (flags & PTB_VKEY) ? (bcode|KEY_VKEY) : bcode ),
                             map,
                             button,
                             (float)amount/65535.0f*2.0-1.0,
                             (flags & PTB_AXIS),
                             false /* queued buttons cannot be repeated */ )
      );
  }
  else
  {
    CSingleLock lock(m_critSection);
    if ( flags & PTB_DOWN )
    {
      m_currentButton.m_iKeyCode   = (flags & PTB_USE_NAME) ? 0 :  // use name? if so no bcode
        ( (flags & PTB_VKEY) ? (bcode|KEY_VKEY) : bcode );         // not name, use vkey?
      m_currentButton.m_mapName    = map;
      m_currentButton.m_buttonName = button;
      m_currentButton.m_fAmount    = (flags & PTB_USE_AMOUNT) ? (amount/65535.0f*2.0-1.0) : 1.0f;
      m_currentButton.m_bRepeat    = (flags & PTB_NO_REPEAT)  ? false : true;
      m_currentButton.m_bAxis      = (flags & PTB_AXIS)       ? true : false;
      m_currentButton.SetActive();
      m_currentButton.Load();
      m_iNextRepeat = 0;
    }
    else
    {
      m_currentButton.Reset();
    }
  }
  return true;
}

bool CEventClient::OnPacketMOUSE(CEventPacket *packet)
{
  unsigned char *payload = (unsigned char *)packet->Payload();
  int psize = (int)packet->PayloadSize();
  unsigned char flags;
  unsigned short mx, my;

  // parse flags
  if (!ParseByte(payload, psize, flags))
    return false;

  // parse x position
  if (!ParseUInt16(payload, psize, mx))
    return false;

  // parse x position
  if (!ParseUInt16(payload, psize, my))
    return false;

  {
    CSingleLock lock(m_critSection);
    if ( flags & PTM_ABSOLUTE )
    {
      m_iMouseX = mx;
      m_iMouseY = my;
      m_bMouseMoved = true;
    }
  }

  return true;
}

bool CEventClient::OnPacketNOTIFICATION(CEventPacket *packet)
{
  unsigned char *payload = (unsigned char *)packet->Payload();
  int psize = (int)packet->PayloadSize();
  string title, message;

  // parse caption
  if (!ParseString(payload, psize, title))
    return false;

  // parse message
  if (!ParseString(payload, psize, message))
    return false;

  // icon type
  unsigned char ltype;
  if (!ParseByte(payload, psize, ltype))
    return false;
  m_eLogoType = (LogoType)ltype;

  // reserved uint32
  unsigned int reserved;
  ParseUInt32(payload, psize, reserved);

  // image data if any
  string iconfile = "Z:\\notification";
  if (m_eLogoType != LT_NONE && psize>0)
  {
    switch (m_eLogoType)
    {
    case LT_JPEG:
      iconfile += ".jpg";
      break;

    case LT_GIF:
      iconfile += ".gif";
      break;

    default:
      iconfile += ".png";
      break;
    }

    FILE * f = fopen(_P(iconfile.c_str()), "wb");
    if (f)
    {
      fwrite((const void *)payload, psize, 1, f);
      fclose(f);
    }
    else
    {
      CLog::Log(LOGERROR, "ES: Could not write icon file");
      m_eLogoType = LT_NONE;
    }
  }

  if (m_eLogoType == LT_NONE)
  {
    g_application.m_guiDialogKaiToast.QueueNotification(title.c_str(),
                                                        message.c_str());
  }
  else
  {
    g_application.m_guiDialogKaiToast.QueueNotification(iconfile.c_str(),
                                                        title.c_str(),
                                                        message.c_str());
  }
  return true;
}

bool CEventClient::OnPacketLOG(CEventPacket *packet)
{
  unsigned char *payload = (unsigned char *)packet->Payload();
  int psize = (int)packet->PayloadSize();
  string logmsg;
  unsigned char ltype;

  if (!ParseByte(payload, psize, ltype))
    return false;
  if (!ParseString(payload, psize, logmsg))
    return false;

  CLog::Log((int)ltype, logmsg.c_str());
  return true;
}

bool CEventClient::OnPacketACTION(CEventPacket *packet)
{
  unsigned char *payload = (unsigned char *)packet->Payload();
  int psize = (int)packet->PayloadSize();
  string actionString;
  unsigned char actionType;

  if (!ParseByte(payload, psize, actionType))
    return false;
  if (!ParseString(payload, psize, actionString))
    return false;

  switch(actionType)
  {
  case AT_EXEC_BUILTIN:
    CUtil::ExecBuiltIn(actionString);
    break;

  default:
    CLog::Log(LOGDEBUG, "ES: Failed - ActionType: %i ActionString: %s", actionType, actionString.c_str());
    return false;
    break;
  }
  return true;
}

bool CEventClient::ParseString(unsigned char* &payload, int &psize, std::string& parsedVal)
{
  if (psize <= 0)
    return false;

  unsigned char *pos = (unsigned char *)memchr((void*)payload, (int)'\0', psize);
  if (!pos)
    return false;

  parsedVal = (char*)payload;
  psize -= ((pos - payload) + 1);
  payload = pos+1;
  return true;
}

bool CEventClient::ParseByte(unsigned char* &payload, int &psize, unsigned char& parsedVal)
{
  if (psize <= 0)
    return false;

  parsedVal = *payload;
  payload++;
  psize--;
  return true;
}

bool CEventClient::ParseUInt32(unsigned char* &payload, int &psize, unsigned int& parsedVal)
{
  if (psize < 4)
    return false;

  parsedVal = ntohl(*((unsigned int *)payload));
  payload+=4;
  psize-=4;
  return true;
}

bool CEventClient::ParseUInt16(unsigned char* &payload, int &psize, unsigned short& parsedVal)
{
  if (psize < 2)
    return false;

  parsedVal = ntohs(*((unsigned short *)payload));
  payload+=2;
  psize-=2;
  return true;
}

void CEventClient::FreePacketQueues()
{
  CSingleLock lock(m_critSection);
  while ( ! m_readyPackets.empty() )
  {
    delete m_readyPackets.front();
    m_readyPackets.pop();
  }

  map<unsigned int, EVENTPACKET::CEventPacket*>::iterator iter = m_seqPackets.begin();
  while (iter != m_seqPackets.end())
  {
    if (iter->second)
    {
      delete iter->second;
    }
    iter++;
  }
  m_seqPackets.clear();
}

unsigned short CEventClient::GetButtonCode(std::string& joystickName, bool& isAxis, float& amount)
{
  CSingleLock lock(m_critSection);
  unsigned short bcode = 0;

  if ( ! m_buttonQueue.empty() )
  {
    CEventButtonState *btn = m_buttonQueue.front();
    m_buttonQueue.pop();

    if ( btn )
    {
      if (btn->Active())
      {
        bcode = btn->KeyCode();
        joystickName = btn->JoystickName();
        isAxis = btn->Axis();
        amount = btn->Amount();
      }
      delete btn;
    }
  }
  else if ( m_currentButton.Active() )
  {
    bcode = m_currentButton.KeyCode();
    joystickName = m_currentButton.JoystickName();
    isAxis = m_currentButton.Axis();
    amount = m_currentButton.Amount();
    
    if ( ! m_currentButton.Repeat() )
      m_currentButton.Reset();
    else
    {
      if ( ! CheckButtonRepeat() )
      {
        bcode = 0;
      }
    }
  }
  return bcode;
}

bool CEventClient::GetMousePos(float& x, float& y)
{
  CSingleLock lock(m_critSection);
  if (m_bMouseMoved)
  {
    x = (float)((m_iMouseX / 65535.0f) * 
                (g_graphicsContext.GetViewWindow().right
                 -g_graphicsContext.GetViewWindow().left));
    y = (float)((m_iMouseY / 65535.0f) * 
                (g_graphicsContext.GetViewWindow().bottom
                 -g_graphicsContext.GetViewWindow().top));
    m_bMouseMoved = false;
    return true;
  }
  return false;
}

bool CEventClient::CheckButtonRepeat()
{
  unsigned int now = timeGetTime();

  if ( m_iNextRepeat == 0 )
  {
    m_iNextRepeat = now + m_iRepeatDelay;
    return true;
  }
  else if ( now > m_iNextRepeat )
  {
    m_iNextRepeat = now + m_iRepeatSpeed;
    return true;
  }
  return false;
}

bool CEventClient::Alive() const
{
  // 60 seconds timeout
  if ( (time(NULL) - m_lastPing) > 60 )
    return false;
  return true;
}

#endif // HAS_EVENT_SERVER
