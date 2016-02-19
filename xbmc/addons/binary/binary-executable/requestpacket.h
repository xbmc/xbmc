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

#include "addons/kodi-addon-dev-kit/include/kodi/api2/.internal/AddonLib_internal.hpp"

#include <stddef.h>
#include <stdint.h>
#include <stdexcept>

class MalformedAddonPacket : public std::runtime_error {
public:
  MalformedAddonPacket()
    :std::runtime_error("Malformed Addon packet") {}
};

class CRequestPacket
{
public:
  CRequestPacket(uint32_t requestID, uint32_t opcode, uint8_t* data, size_t dataLength, bool sharedMem = false);
  ~CRequestPacket();

  inline size_t   getDataLength() const { return m_userDataLength; }
  inline uint32_t getRequestID()  const { return m_requestID; }
  inline uint32_t getOpCode()     const { return m_opCode; }

  void* pop(KODI_API_Datatype type, void* value);

  bool      end() const;

  // If you call this, the memory becomes yours. Free with free()
  uint8_t* getData();

private:
  uint8_t*  m_userData;
  size_t    m_userDataLength;
  size_t    m_packetPos;
  uint32_t  m_opCode;
  uint32_t  m_requestID;
  bool      m_sharedMem;
};
