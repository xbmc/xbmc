#ifndef __RTMP_PACKET__
#define __RTMP_PACKET__
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

#include <string>

#define RTMP_PACKET_TYPE_AUDIO 0x08
#define RTMP_PACKET_TYPE_VIDEO 0x09
#define RTMP_PACKET_TYPE_INFO  0x12

#define RTMP_MAX_HEADER_SIZE 12

namespace RTMP_LIB
{
  class RTMPPacket
  {
    public:
      RTMPPacket();
      virtual ~RTMPPacket();

      void Reset();
      bool AllocPacket(int nSize);
      void FreePacket();
      void FreePacketHeader();
      
      inline bool IsReady() { return m_nBytesRead == m_nBodySize; }
      void Dump();

      unsigned char  m_headerType;
      unsigned char  m_packetType;
      unsigned char  m_nChannel;
      int            m_nInfoField1; // 3 first bytes
      int            m_nInfoField2; // last 4 bytes in a long header
      bool           m_hasAbsTimestamp; // timestamp absolute or relative?
      unsigned long  m_nBodySize;
      unsigned long  m_nBytesRead;
      char           *m_body;
  };
};

#endif
