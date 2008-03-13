#include "include.h"
#include "EventPacket.h"
#include "Socket.h"

using namespace EVENTPACKET;

bool CEventPacket::Parse(int datasize, const void *data)
{
  unsigned char* buf = (unsigned char *)data;
  if (datasize < HEADER_SIZE || datasize > PACKET_SIZE)
    return false;

  // check signature
  if (memcmp(data, (const void*)HEADER_SIG, HEADER_SIG_LENGTH) != 0)
    return false;

  buf += HEADER_SIG_LENGTH;

  // extract protocol version
  m_cMajVer = (*buf++);
  m_cMinVer = (*buf++);

  if (m_cMajVer != 2 && m_cMinVer != 0)
    return false;

  // get packet type
  m_eType = (PacketType)ntohs(*((uint16_t*)buf));
  
  if (m_eType < (unsigned short)PT_HELO || m_eType >= (unsigned short)PT_LAST)
    return false;
  
  // get packet sequence id
  buf += 2;
  m_iSeq  = ntohl(*((uint32_t*)buf));

  // get total message length
  buf += 4;
  m_iTotalPackets = ntohl(*((uint32_t*)buf));

  // get payload size
  buf += 4;
  m_sPayloadSize = ntohs(*((uint16_t*)buf));

  buf += 2;
  if ((m_sPayloadSize + HEADER_SIZE) != datasize)
    return false;

  // get payload
  if (m_sPayloadSize)
  {
    // forward past reserved bytes
    buf += 14;

    if (m_pPayload)
    {
      free(m_pPayload);
      m_pPayload = NULL;
    }

    m_pPayload = malloc(m_sPayloadSize);
    if (!m_pPayload)
    {
      CLog::Log(LOGERROR, "ES: Out of memory");
      return false;
    }
    memcpy(m_pPayload, buf, (size_t)m_sPayloadSize);
  }

  return  (m_bValid = true);
}

