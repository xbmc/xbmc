/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "EventClient.h"

#include "EventPacket.h"
#include "ServiceBroker.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "filesystem/File.h"
#include "guilib/LocalizeStrings.h"
#include "input/keyboard/KeyIDs.h"
#include "input/keymaps/ButtonTranslator.h"
#include "input/keymaps/joysticks/GamepadTranslator.h"
#include "input/keymaps/keyboard/KeyboardTranslator.h"
#include "input/keymaps/remote/IRTranslator.h"
#include "utils/StringUtils.h"
#include "utils/TimeUtils.h"
#include "utils/log.h"
#include "windowing/GraphicContext.h"

#include <map>
#include <mutex>
#include <queue>

using namespace KODI;
using namespace EVENTCLIENT;
using namespace EVENTPACKET;

struct ButtonStateFinder
{
  explicit ButtonStateFinder(const CEventButtonState& state)
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
  std::string    m_map;
  std::string    m_button;
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
      m_iKeyCode = KEYMAP::CButtonTranslator::TranslateString(m_mapName, m_buttonName);
      if (m_iKeyCode == 0)
      {
        Reset();
        CLog::Log(LOGERROR, "ES: Could not map {} : {} to a key", m_mapName, m_buttonName);
      }
    }
  }
  else
  {
    if (m_mapName.length() > 3 &&
        (StringUtils::StartsWith(m_mapName, "JS")) )
    {
      m_joystickName = m_mapName.substr(2);  // <num>:joyname
      m_iControllerNumber = (unsigned char)(*(m_joystickName.c_str()))
        - (unsigned char)'0'; // convert <num> to int
      m_joystickName = m_joystickName.substr(2); // extract joyname
    }

    if (m_mapName.length() > 3 &&
        (StringUtils::StartsWith(m_mapName, "CC")) ) // custom map - CC:<controllerName>
    {
      m_customControllerName = m_mapName.substr(3);
    }
  }
}

/************************************************************************/
/* CEventClient                                                         */
/************************************************************************/
bool CEventClient::AddPacket(std::unique_ptr<CEventPacket> packet)
{
  if (!packet)
    return false;

  ResetTimeout();
  if ( packet->Size() > 1 )
  {
    //! @todo limit payload size
    if (m_seqPackets[packet->Sequence()])
    {
      if(!m_bSequenceError)
        CLog::Log(LOGWARNING,
                  "CEventClient::AddPacket - received packet with same sequence number ({}) as "
                  "previous packet from eventclient {}",
                  packet->Sequence(), m_deviceName);
      m_bSequenceError = true;
      m_seqPackets.erase(packet->Sequence());
    }

    unsigned int sequence = packet->Sequence();

    m_seqPackets[sequence] = std::move(packet);
    if (m_seqPackets.size() == m_seqPackets[sequence]->Size())
    {
      unsigned int iSeqPayloadSize = 0;
      for (unsigned int i = 1; i <= m_seqPackets[sequence]->Size(); i++)
      {
        iSeqPayloadSize += m_seqPackets[i]->PayloadSize();
      }

      std::vector<uint8_t> newPayload(iSeqPayloadSize);
      auto newPayloadIter = newPayload.begin();

      unsigned int packets = m_seqPackets[sequence]->Size(); // packet can be deleted in this loop
      for (unsigned int i = 1; i <= packets; i++)
      {
        newPayloadIter =
            std::copy(m_seqPackets[i]->Payload(),
                      m_seqPackets[i]->Payload() + m_seqPackets[i]->PayloadSize(), newPayloadIter);

        if (i > 1)
          m_seqPackets.erase(i);
      }
      m_seqPackets[1]->SetPayload(newPayload);
      m_readyPackets.push(std::move(m_seqPackets[1]));
      m_seqPackets.clear();
    }
  }
  else
  {
    m_readyPackets.push(std::move(packet));
  }
  return true;
}

void CEventClient::ProcessEvents()
{
  if (!m_readyPackets.empty())
  {
    while ( ! m_readyPackets.empty() )
    {
      ProcessPacket(m_readyPackets.front().get());
      if ( ! m_readyPackets.empty() ) // in case the BYE packet cleared the queues
        m_readyPackets.pop();
    }
  }
}

bool CEventClient::GetNextAction(CEventAction &action)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  if (!m_actionQueue.empty())
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

  case EVENTPACKET::PT_MOUSE:
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
  //! @todo check it last HELO packet was received less than 5 minutes back
  //!       if so, do not show notification of connection.
  if (Greeted())
    return false;

  unsigned char *payload = (unsigned char *)packet->Payload();
  int psize = (int)packet->PayloadSize();

  // parse device name
  if (!ParseString(payload, psize, m_deviceName))
    return false;

  CLog::Log(LOGINFO, "ES: Incoming connection from {}", m_deviceName);

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
  std::string iconfile = "special://temp/helo";
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
    if (!file.OpenForWrite(iconfile, true) || file.Write((const void *)payload, psize) != psize)
    {
      CLog::Log(LOGERROR, "ES: Could not write icon file");
      m_eLogoType = LT_NONE;
    }
  }

  m_bGreeted = true;
  if (m_eLogoType == LT_NONE)
  {
    CGUIDialogKaiToast::QueueNotification(g_localizeStrings.Get(33200), m_deviceName);
  }
  else
  {
    CGUIDialogKaiToast::QueueNotification(iconfile, g_localizeStrings.Get(33200), m_deviceName);
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

  std::string map, button;
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

  if (flags & PTB_USE_NAME)
    CLog::Log(LOGDEBUG, "EventClient: button name \"{}\" map \"{}\" {}", button, map,
              active ? "pressed" : "released");
  else
    CLog::Log(LOGDEBUG, "EventClient: button code {} {}", bcode, active ? "pressed" : "released");

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
    std::unique_lock<CCriticalSection> lock(m_critSection);

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

    std::list<CEventButtonState>::reverse_iterator it;
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
        /* since modifying the list invalidates the reverse iterator */
        std::list<CEventButtonState>::iterator it2 = (++it).base();

        /* if last event had an amount, we must resend without amount */
        if (it2->m_bUseAmount && it2->m_fAmount != 0.0f)
        {
          m_buttonQueue.push_back(state);
        }

        /* if the last event was waiting for a repeat interval, it has executed already.*/
        if(it2->m_bRepeat)
        {
          if (it2->m_iNextRepeat.time_since_epoch().count() > 0)
          {
            m_buttonQueue.erase(it2);
          }
          else
          {
            it2->m_bRepeat = false;
            it2->m_bActive = false;
          }
        }

      }
      else if(active && !it->m_bActive)
      {
        m_buttonQueue.push_back(state);
        if (!state.m_bRepeat && state.m_bAxis && state.m_fAmount != 0.0f)
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
    std::unique_lock<CCriticalSection> lock(m_critSection);
    if ( flags & PTB_DOWN )
    {
      m_currentButton.m_iKeyCode   = keycode;
      m_currentButton.m_mapName    = map;
      m_currentButton.m_buttonName = button;
      m_currentButton.m_fAmount    = famount;
      m_currentButton.m_bRepeat    = (flags & PTB_NO_REPEAT)  ? false : true;
      m_currentButton.m_bAxis      = (flags & PTB_AXIS)       ? true : false;
      m_currentButton.m_iNextRepeat = {};
      m_currentButton.SetActive();
      m_currentButton.Load();
    }
    else
    {
      /* when a button is released that had amount, make sure *
       * to resend the keypress with an amount of 0           */
      if ((flags & PTB_USE_AMOUNT) && m_currentButton.m_fAmount > 0.0f)
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
    std::unique_lock<CCriticalSection> lock(m_critSection);
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
  std::string title, message;

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
  std::string iconfile = "special://temp/notification";
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
    if (!file.OpenForWrite(iconfile, true) || file.Write((const void *)payload, psize) != psize)
    {
      CLog::Log(LOGERROR, "ES: Could not write icon file");
      m_eLogoType = LT_NONE;
    }
  }

  if (m_eLogoType == LT_NONE)
  {
    CGUIDialogKaiToast::QueueNotification(title, message);
  }
  else
  {
    CGUIDialogKaiToast::QueueNotification(iconfile, title, message);
  }
  return true;
}

bool CEventClient::OnPacketLOG(CEventPacket *packet)
{
  unsigned char *payload = (unsigned char *)packet->Payload();
  int psize = (int)packet->PayloadSize();
  std::string logmsg;
  unsigned char ltype;

  if (!ParseByte(payload, psize, ltype))
    return false;
  if (!ParseString(payload, psize, logmsg))
    return false;

  CLog::Log((int)ltype, "{}", logmsg);
  return true;
}

bool CEventClient::OnPacketACTION(CEventPacket *packet)
{
  unsigned char *payload = (unsigned char *)packet->Payload();
  int psize = (int)packet->PayloadSize();
  std::string actionString;
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
      std::unique_lock<CCriticalSection> lock(m_critSection);
      m_actionQueue.emplace(actionString.c_str(), actionType);
    }
    break;

  default:
    CLog::Log(LOGDEBUG, "ES: Failed - ActionType: {} ActionString: {}", actionType, actionString);
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
  std::unique_lock<CCriticalSection> lock(m_critSection);

  while ( ! m_readyPackets.empty() )
    m_readyPackets.pop();

  m_seqPackets.clear();
}

unsigned int CEventClient::GetButtonCode(std::string& strMapName, bool& isAxis, float& amount, bool &isJoystick)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  unsigned int bcode = 0;

  if ( m_currentButton.Active() )
  {
    bcode = m_currentButton.KeyCode();
    strMapName = m_currentButton.JoystickName();
    isJoystick = true;
    if (strMapName.length() == 0)
    {
      strMapName = m_currentButton.CustomControllerName();
      isJoystick = false;
    }

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


  std::list<CEventButtonState> repeat;
  std::list<CEventButtonState>::iterator it;
  for(it = m_buttonQueue.begin(); bcode == 0 && it != m_buttonQueue.end(); ++it)
  {
    bcode        = it->KeyCode();
    strMapName   = it->JoystickName();
    isJoystick   = true;

    if (strMapName.length() == 0)
    {
      strMapName = it->CustomControllerName();
      isJoystick = false;
    }

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
  std::unique_lock<CCriticalSection> lock(m_critSection);
  if (m_bMouseMoved)
  {
    x = (m_iMouseX / 65535.0f) * CServiceBroker::GetWinSystem()->GetGfxContext().GetWidth();
    y = (m_iMouseY / 65535.0f) * CServiceBroker::GetWinSystem()->GetGfxContext().GetHeight();
    m_bMouseMoved = false;
    return true;
  }
  return false;
}

bool CEventClient::CheckButtonRepeat(std::chrono::time_point<std::chrono::steady_clock>& next)
{
  auto now = std::chrono::steady_clock::now();

  if (next.time_since_epoch().count() == 0)
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
