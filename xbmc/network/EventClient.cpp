/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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

#include "threads/SystemClock.h"
#include "system.h"

#ifdef HAS_EVENT_SERVER

#include "EventClient.h"
#include "EventPacket.h"
#include "threads/SingleLock.h"
#include "input/ButtonTranslator.h"
#include <map>
#include <queue>
#include "filesystem/File.h"
#include "utils/log.h"
#include "utils/TimeUtils.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "guilib/GraphicContext.h"
#include "guilib/LocalizeStrings.h"

using namespace EVENTCLIENT;
using namespace EVENTPACKET;
using namespace std;


struct ButtonStateFinder
{
  ButtonStateFinder(const CEventButtonState& state)
    : m_keycode(state.m_iKeyCode)
    , m_map(state.m_mapName)
    , m_button(state.m_buttonName)
  {}

  bool operator()(const CEventButtonState& state)
  {
    return state.m_mapName    == m_map
        && state.m_iKeyCode   == m_keycode
        && state.m_buttonName == m_button;
  }
  private:
  unsigned short m_keycode;
  string    m_map;
  string    m_button;
};

/************************************************************************/
/* CEventButtonState                                                    */
/************************************************************************/
void CEventButtonState::Load()
{
  if ( m_iKeyCode == 0 )
  {
    if ( (m_mapName.length() > 0) && (m_buttonName.length() > 0) )
    {
      if ( m_mapName.compare("KB") == 0 ) // standard keyboard map
      {
        m_iKeyCode = CButtonTranslator::TranslateKeyboardString( m_buttonName.c_str() );
      }
      else if  ( m_mapName.compare("XG") == 0 ) // xbox gamepad map
      {
        m_iKeyCode = CButtonTranslator::TranslateGamepadString( m_buttonName.c_str() );
      }
      else if  ( m_mapName.compare("R1") == 0 ) // xbox remote map
      {
        m_iKeyCode = CButtonTranslator::TranslateRemoteString( m_buttonName.c_str() );
      }
      else if  ( m_mapName.compare("R2") == 0 ) // xbox unviversal remote map
      {
        m_iKeyCode = CButtonTranslator::TranslateUniversalRemoteString( m_buttonName.c_str() );
      }
      else if ( (m_mapName.length() > 3) &&
                (m_mapName.compare(0, 3, "LI:") == 0) ) // starts with LI: ?
      {
#if defined(HAS_LIRC) || defined(HAS_IRSERVERSUITE)
        string lircDevice = m_mapName.substr(3);
        m_iKeyCode = CButtonTranslator::GetInstance().TranslateLircRemoteString( lircDevice.c_str(),
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
    if (m_seqPackets[ packet->Sequence() ])
    {
      if(!m_bSequenceError)
        CLog::Log(LOGWARNING, "CEventClient::AddPacket - received packet with same sequence number (%d) as previous packet from eventclient %s", packet->Sequence(), m_deviceName.c_str());
      m_bSequenceError = true;
      delete m_seqPackets[ packet->Sequence() ];
    }

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
        unsigned int packets = packet->Size(); // packet can be deleted in this loop
        for (unsigned int i = 1 ; i<=packets ; i++)
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

void CEventClient::ProcessEvents()
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

bool CEventClient::GetNextAction(CEventAction &action)
{
  CSingleLock lock(m_critSection);
  if (m_actionQueue.size() > 0)
  {
    // grab the next action in line
    action = m_actionQueue.front();
    m_actionQueue.pop();
    return true;
  }
  else
  {
    // we got nothing
    return false;
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
  string iconfile = "special://temp/helo";
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
    XFILE::CFile file;
    if (file.OpenForWrite(iconfile, true))
    {
      file.Write((const void *)payload, psize);
      file.Close();
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
    CGUIDialogKaiToast::QueueNotification(g_localizeStrings.Get(33200),
                                          m_deviceName.c_str());
  }
  else
  {
    CGUIDialogKaiToast::QueueNotification(iconfile.c_str(),
                                          g_localizeStrings.Get(33200),
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

  unsigned int keycode;
  if(flags & PTB_USE_NAME)
    keycode = 0;
  else if(flags & PTB_VKEY)
    keycode = bcode|KEY_VKEY;
  else if(flags & PTB_UNICODE)
    keycode = bcode|ES_FLAG_UNICODE;
  else
    keycode = bcode;

  float famount = 0;
  bool active = (flags & PTB_DOWN) ? true : false;

  if(flags & PTB_USE_AMOUNT)
  {
    if(flags & PTB_AXIS)
      famount = (float)amount/65535.0f*2.0f-1.0f;
    else
      famount = (float)amount/65535.0f;
  }
  else
    famount = (active ? 1.0f : 0.0f);

  if(flags & PTB_QUEUE)
  {
    /* find the last queued item of this type */
    CSingleLock lock(m_critSection);

    CEventButtonState state( keycode,
                             map,
                             button,
                             famount,
                             (flags & (PTB_AXIS|PTB_AXISSINGLE)) ? true  : false,
                             (flags & PTB_NO_REPEAT)             ? false : true,
                             (flags & PTB_USE_AMOUNT)            ? true : false );

    /* correct non active events so they work with rest of code */
    if(!active)
    {
      state.m_bActive = false;
      state.m_bRepeat = false;
      state.m_fAmount = 0.0;
    }

    list<CEventButtonState>::reverse_iterator it;
    it = find_if( m_buttonQueue.rbegin() , m_buttonQueue.rend(), ButtonStateFinder(state));

    if(it == m_buttonQueue.rend())
    {
      if(active)
        m_buttonQueue.push_back(state);
    }
    else
    {
      if(!active && it->m_bActive)
      {
        /* since modifying the list invalidates the referse iteratator */
        list<CEventButtonState>::iterator it2 = (++it).base();

        /* if last event had an amount, we must resend without amount */
        if(it2->m_bUseAmount && it2->m_fAmount != 0.0)
          m_buttonQueue.push_back(state);

        /* if the last event was waiting for a repeat interval, it has executed already.*/
        if(it2->m_bRepeat)
        {
          if(it2->m_iNextRepeat > 0)
            m_buttonQueue.erase(it2);
          else
            it2->m_bRepeat = false;
        }

      }
      else if(active && !it->m_bActive)
      {
        m_buttonQueue.push_back(state);
        if(!state.m_bRepeat && state.m_bAxis && state.m_fAmount != 0.0)
        {
          state.m_bActive = false;
          state.m_bRepeat = false;
          state.m_fAmount = 0.0;
          m_buttonQueue.push_back(state);
        }
      }
      else
        it->m_fAmount = state.m_fAmount;
    }
  }
  else
  {
    CSingleLock lock(m_critSection);
    if ( flags & PTB_DOWN )
    {
      m_currentButton.m_iKeyCode   = keycode;
      m_currentButton.m_mapName    = map;
      m_currentButton.m_buttonName = button;
      m_currentButton.m_fAmount    = famount;
      m_currentButton.m_bRepeat    = (flags & PTB_NO_REPEAT)  ? false : true;
      m_currentButton.m_bAxis      = (flags & PTB_AXIS)       ? true : false;
      m_currentButton.m_iNextRepeat = 0;
      m_currentButton.SetActive();
      m_currentButton.Load();
    }
    else
    {
      /* when a button is released that had amount, make sure *
       * to resend the keypress with an amount of 0           */
      if((flags & PTB_USE_AMOUNT) && m_currentButton.m_fAmount > 0.0)
      {
        CEventButtonState state( m_currentButton.m_iKeyCode,
                                 m_currentButton.m_mapName,
                                 m_currentButton.m_buttonName,
                                 0.0,
                                 m_currentButton.m_bAxis,
                                 false,
                                 true );

        m_buttonQueue.push_back (state);
      }
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
  string iconfile = "special://temp/notification";
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

    XFILE::CFile file;
    if (file.OpenForWrite(iconfile, true))
    {
      file.Write((const void *)payload, psize);
      file.Close();
    }
    else
    {
      CLog::Log(LOGERROR, "ES: Could not write icon file");
      m_eLogoType = LT_NONE;
    }
  }

  if (m_eLogoType == LT_NONE)
  {
    CGUIDialogKaiToast::QueueNotification(title.c_str(),
                                          message.c_str());
  }
  else
  {
    CGUIDialogKaiToast::QueueNotification(iconfile.c_str(),
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

  CLog::Log((int)ltype, "%s", logmsg.c_str());
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
  case AT_BUTTON:
    {
      CSingleLock lock(m_critSection);
      m_actionQueue.push(CEventAction(actionString.c_str(), actionType));
    }
    break;

  default:
    CLog::Log(LOGDEBUG, "ES: Failed - ActionType: %i ActionString: %s", actionType, actionString.c_str());
    return false;
    break;
  }
  return true;
}

bool CEventClient::ParseString(unsigned char* &payload, int &psize, string& parsedVal)
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

unsigned int CEventClient::GetButtonCode(string& joystickName, bool& isAxis, float& amount)
{
  CSingleLock lock(m_critSection);
  unsigned int bcode = 0;

  if ( m_currentButton.Active() )
  {
    bcode = m_currentButton.KeyCode();
    joystickName = m_currentButton.JoystickName();
    isAxis = m_currentButton.Axis();
    amount = m_currentButton.Amount();

    if ( ! m_currentButton.Repeat() )
      m_currentButton.Reset();
    else
    {
      if ( ! CheckButtonRepeat(m_currentButton.m_iNextRepeat) )
        bcode = 0;
    }
    return bcode;
  }

  if(m_buttonQueue.empty())
    return 0;


  list<CEventButtonState> repeat;
  list<CEventButtonState>::iterator it;
  for(it = m_buttonQueue.begin(); bcode == 0 && it != m_buttonQueue.end(); it++)
  {
    bcode        = it->KeyCode();
    joystickName = it->JoystickName();
    isAxis       = it->Axis();
    amount       = it->Amount();

    if(it->Repeat())
    {
      /* MUST update m_iNextRepeat before resend */
      bool skip = !it->Axis() && !CheckButtonRepeat(it->m_iNextRepeat);

      repeat.push_back(*it);
      if(skip)
      {
        bcode = 0;
        continue;
      }
    }
  }

  m_buttonQueue.erase(m_buttonQueue.begin(), it);
  m_buttonQueue.insert(m_buttonQueue.end(), repeat.begin(), repeat.end());
  return bcode;
}

bool CEventClient::GetMousePos(float& x, float& y)
{
  CSingleLock lock(m_critSection);
  if (m_bMouseMoved)
  {
    x = (float)((m_iMouseX / 65535.0f) *
                (g_graphicsContext.GetViewWindow().x2
                 -g_graphicsContext.GetViewWindow().x1));
    y = (float)((m_iMouseY / 65535.0f) *
                (g_graphicsContext.GetViewWindow().y2
                 -g_graphicsContext.GetViewWindow().y1));
    m_bMouseMoved = false;
    return true;
  }
  return false;
}

bool CEventClient::CheckButtonRepeat(unsigned int &next)
{
  unsigned int now = XbmcThreads::SystemClockMillis();

  if ( next == 0 )
  {
    next = now + m_iRepeatDelay;
    return true;
  }
  else if ( now > next )
  {
    next = now + m_iRepeatSpeed;
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
