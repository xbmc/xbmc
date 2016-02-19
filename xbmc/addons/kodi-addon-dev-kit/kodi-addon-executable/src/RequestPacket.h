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

class CKODIAddon_InterProcess;

class CRequestPacket
{
  public:
    CRequestPacket(uint32_t opcode,
              CKODIAddon_InterProcess*  process,
              KODI_API_Packets          type              = KODIPacket_RequestedResponse,
              bool                      setUserDataLength = false,
              size_t                    userDataLength    = 0);
    ~CRequestPacket();

    void push(KODI_API_Datatype type, const void *value);

    uint8_t*  getPtr()        const { return m_buffer;        }
    size_t    getLen()        const { return m_bufUsed;       }
    uint32_t  getChannel()    const { return m_channel;       }
    uint32_t  getSerial()     const { return m_serialNumber;  }
    uint32_t  getOpcode()     const { return m_opcode;        }
    bool      SharedMemUsed() const { return m_sharedMemUsed; }

  private:
    CKODIAddon_InterProcess*  m_process;
    bool                      m_sharedMemUsed;
    uint8_t*                  m_buffer;
    size_t                    m_bufSize;
    size_t                    m_bufUsed;
    bool                      m_lengthSet;
    uint32_t                  m_channel;
    uint32_t                  m_serialNumber;
    uint32_t                  m_opcode;

    void checkExtend(size_t by);

    static uint32_t m_serialNumberCounter;
    const static size_t headerLength    = 16;
    const static size_t userDataLenPos  = 12;
};
