#include "include.h"
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

  if ( packet->Size() != packet->Sequence() )
  {
    m_seqPackets[ packet->Sequence() ] = packet;
    // TODO: combine seq and insert into queue
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

  // logo type
  unsigned char ltype;
  if (!ParseByte(payload, psize, ltype))
    return false;
  m_eLogoType = (LogoType)ltype;

  if (m_eLogoType != LT_NONE)
  {
    // TODO
  }
  m_bGreeted = true;
  g_application.m_guiDialogKaiToast.QueueNotification("Detected New Connection", m_deviceName.c_str());
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
  
  // parse device name
  if (!ParseString(payload, psize, title))
    return false;

  // parse message
  if (!ParseString(payload, psize, message))
    return false;

  g_application.m_guiDialogKaiToast.QueueNotification(title.c_str(), message.c_str());
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
