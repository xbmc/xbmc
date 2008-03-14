#include "stdafx.h"
#include "Util.h"
#include "EventClient.h"
#include "EventPacket.h"
#include "Application.h"
#include <map>
#include <queue>

using namespace EVENTCLIENT;
using namespace EVENTPACKET;
using namespace std;

bool CEventClient::AddPacket(CEventPacket *packet)
{
  if (!packet)
    return false;

  if ( packet->Size() > 1 )
  {
    m_seqPackets[ packet->Sequence() ] = packet;
    m_iSeqPayloadSize += packet->PayloadSize();
    if (m_seqPackets.size() == packet->Size())
    {
      unsigned int offset = 0;
      void *newPayload = NULL;
      newPayload = malloc(m_iSeqPayloadSize);
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
        m_seqPackets[1]->SetPayload(m_iSeqPayloadSize, newPayload);
        m_readyPackets.push(m_seqPackets[1]);
        m_seqPackets.clear();
        m_iSeqPayloadSize = 0;
      }
      else
      {
        m_iSeqPayloadSize = 0;
        CLog::Log(LOGERROR, "ES: Could not assemble packets, Out of Memory");
        FreeQueues();
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
      delete m_readyPackets.front();
      m_readyPackets.pop();
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

  default:
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
    fwrite((const void *)payload, psize, 1, f);
    fclose(f);
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
  FreeQueues();

  return true;
}

bool CEventClient::OnPacketBUTTON(CEventPacket *packet)
{
  if (!Greeted())
    return false;

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

  // TODO: optimize so that the next two are only if !(flags & 0x1)

  // parse the map to use
  if (!ParseString(payload, psize, map))
    return false;

  // parse button name
  if (!ParseString(payload, psize, button))
    return false;

  return true;
}

bool CEventClient::OnPacketMOUSE(CEventPacket *packet)
{
  if (!Greeted())
    return false;
  // TODO
  return true;
}

bool CEventClient::OnPacketNOTIFICATION(CEventPacket *packet)
{
  if (!Greeted())
    return false;

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
    fwrite((const void *)payload, psize, 1, f);
    fclose(f);
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

void CEventClient::FreeQueues()
{
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
