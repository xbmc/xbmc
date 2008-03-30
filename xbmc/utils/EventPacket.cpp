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

#include "EventPacket.h"
#include "Socket.h"
#ifdef _XBOX
#include "lib/libcdio/inttypes.h"
#endif

using namespace EVENTPACKET;

/************************************************************************/
/* CEventPacket                                                         */
/************************************************************************/
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
  m_iPayloadSize = ntohs(*((uint16_t*)buf));

  buf += 2;
  if ((m_iPayloadSize + HEADER_SIZE) != (unsigned int)datasize)
    return false;

  // get payload
  if (m_iPayloadSize)
  {
    // forward past reserved bytes
    buf += 14;

    if (m_pPayload)
    {
      free(m_pPayload);
      m_pPayload = NULL;
    }

    m_pPayload = malloc(m_iPayloadSize);
    if (!m_pPayload)
    {
      CLog::Log(LOGERROR, "ES: Out of memory");
      return false;
    }
    memcpy(m_pPayload, buf, (size_t)m_iPayloadSize);
  }

  return  (m_bValid = true);
}

#endif // HAS_EVENT_SERVER
