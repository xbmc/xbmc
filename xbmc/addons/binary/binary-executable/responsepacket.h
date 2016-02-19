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

#include "addons/kodi-addon-dev-kit/include/kodi/api2/AddonLib.hpp"

#include <stdint.h>

class CResponsePacket
{
public:
  CResponsePacket(uint8_t* buffer = nullptr, size_t bufSize = 0);
  ~CResponsePacket();

  void init(uint32_t requestID);
  void initStatus(uint32_t opCode);
  void finalise();
  bool copyin(const uint8_t* src, uint32_t len);
  uint8_t* reserve(uint32_t len);
  bool unreserve(uint32_t len);

  bool push(KODI_API_Datatype type, const void *value);

  uint8_t* getPtr() { return m_buffer; }
  uint32_t getLen() { return m_bufUsed; }
  void     setLen(uint32_t len) { m_bufUsed = len; }

private:
  bool     m_sharedMemUsed;
  uint8_t* m_buffer;
  uint32_t m_bufSize;
  uint32_t m_bufUsed;

  inline void initBuffers();
  inline bool checkExtend(uint32_t by);

  const static uint32_t headerLength          = 12;
  const static uint32_t userDataLenPos        = 8;
};
