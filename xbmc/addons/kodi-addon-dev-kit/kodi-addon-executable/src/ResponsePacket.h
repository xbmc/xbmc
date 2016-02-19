#pragma once
/*
 *      Copyright (C) 2010-2015 Team KODI
 *      http://kodi.tv
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
 *  along with KODI; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "kodi/api2/AddonLib.hpp"
#include "kodi/api2/.internal/AddonLib_internal.hpp"

#include <stdio.h>
#include <stdint.h>

class CResponsePacket
{
public:
  CResponsePacket(bool sharedMem = true);
  ~CResponsePacket();

  void      setResponse(uint8_t* packet, size_t packetLength);
  void      setStatus(uint8_t* packet, size_t packetLength);

  void      popHeader();
  void*     pop(KODI_API_Datatype type, void *value);

  bool      noResponse()                { return (m_responseData == NULL); };
  size_t    getUserDataLength()   const { return m_responseDataLength; }
  uint32_t  getChannelID()        const { return m_channelID; }
  uint32_t  getRequestID()        const { return m_requestID; }
  size_t    getPacketPos()        const { return m_packetPos; }
  size_t    getRemainingLength()  const { return m_responseDataLength - m_packetPos; }

  // If you call this, the memory becomes yours. Free with free()
  uint8_t*  stealUserData();
  uint8_t*  getUserData();
  uint8_t*  getHeader()                 { return m_header; };
  size_t    getHeaderLength() const     { return 8; };

private:
  bool      m_sharedMem;
  uint8_t   m_header[40];
  uint8_t*  m_responseData;
  uint32_t    m_responseDataLength;
  size_t    m_packetPos;
  uint32_t  m_channelID;
  uint32_t  m_requestID;
};
