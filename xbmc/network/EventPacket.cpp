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

#include "system.h"

#ifdef HAS_EVENT_SERVER

#include "EventPacket.h"
#include "Socket.h"
#include "utils/log.h"

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

  if ((m_iPayloadSize + HEADER_SIZE) != (unsigned int)datasize)
    return false;

  // get the client's token
  buf += 2;
  m_iClientToken = ntohl(*((uint32_t*)buf));

  buf += 4;

  // get payload
  if (m_iPayloadSize)
  {
    // forward past reserved bytes
    buf += 10;

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
  m_bValid = true;
  return true;
}

#endif // HAS_EVENT_SERVER
