#include "include.h"
#include "EventClient.h"
#include "EventPacket.h"
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
      m_readyPackets.pop();
    }
  }
}

bool CEventClient::ProcessPacket(CEventPacket *packet)
{
  if (!packet)
    return false;

  switch (packet->Type())
  {
  case PT_HELO:
    return OnPacketHELO(packet);
    
  case PT_BYE:
    return OnPacketBYE(packet);

  case PT_BUTTON:
    return OnPacketBUTTON(packet);

  default:
    break;
  }
  
  return false;
}

bool CEventClient::OnPacketHELO(CEventPacket *packet)
{
  // TODO: check it last HELO packet was received less than 5 minutes back
  //       if so, do not show notification of connection.

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
  return true;
}

bool CEventClient::OnPacketBYE(CEventPacket *packet)
{
  // TODO
  return true;
}

bool CEventClient::OnPacketBUTTON(CEventPacket *packet)
{
  // TODO
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
