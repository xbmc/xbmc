/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This file is part of libRTMP.
 *
 *  libRTMP is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  libRTMP is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with libRTMP; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifdef _LINUX
#include "linux/PlatformInclude.h"
#endif

#include "rtmppacket.h"
#include "utils/log.h"

#ifdef _LINUX
  #include <arpa/inet.h>
#endif

using namespace RTMP_LIB;

RTMPPacket::RTMPPacket()
{
  Reset();
}

RTMPPacket::~RTMPPacket()
{
  FreePacket();
}

void RTMPPacket::Reset()
{
  m_headerType = 0;
  m_packetType = 0;
  m_nChannel = 0;
  m_nInfoField1 = 0; 
  m_nInfoField2 = 0; 
  m_hasAbsTimestamp = false;
  m_nBodySize = 0;
  m_nBytesRead = 0;
  m_body = NULL;
}

bool RTMPPacket::AllocPacket(int nSize)
{
  m_body = new char[nSize];
  if (!m_body)
    return false;
  memset(m_body,0,nSize);
  m_nBytesRead = 0;
  return true;
}

void RTMPPacket::FreePacket()
{
  FreePacketHeader();
  Reset();
}

void RTMPPacket::FreePacketHeader()
{
  if (m_body)
    delete [] m_body;
  m_body = NULL;
}

void RTMPPacket::Dump()
{
  CLog::Log(LOGDEBUG,"RTMP PACKET: packet type: 0x%02x. channel: 0x%02x. info 1: %d info 2: %d. Body size: %lu. body: 0x%02x", m_packetType, m_nChannel,
           m_nInfoField1, m_nInfoField2, m_nBodySize, m_body?(unsigned char)m_body[0]:0);
}
