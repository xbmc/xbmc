/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "EventPacket.h"

#include "Socket.h"
#include "utils/log.h"

using namespace EVENTPACKET;

/************************************************************************/
/* CEventPacket                                                         */
/************************************************************************/
bool CEventPacket::Parse(int datasize, const void *data)
{
  unsigned char* buf = const_cast<unsigned char*>((const unsigned char *)data);
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
  uint16_t payloadSize = ntohs(*(reinterpret_cast<uint16_t*>(buf)));

  if ((payloadSize + HEADER_SIZE) != static_cast<uint16_t>(datasize))
    return false;

  // get the client's token
  buf += 2;
  m_iClientToken = ntohl(*((uint32_t*)buf));

  buf += 4;

  // get payload
  if (payloadSize > 0)
  {
    // forward past reserved bytes
    buf += 10;

    m_pPayload = std::vector<uint8_t>(buf, buf + payloadSize);
  }
  m_bValid = true;
  return true;
}

void CEventPacket::SetPayload(std::vector<uint8_t> payload)
{
  m_pPayload = std::move(payload);
}
