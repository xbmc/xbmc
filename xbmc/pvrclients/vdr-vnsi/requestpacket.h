#pragma once
/*
 *      Copyright (C) 2010 Alwin Esch (Team XBMC)
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include <stdint.h>

class cRequestPacket
{
  public:
    cRequestPacket();
    ~cRequestPacket();

    bool init(uint32_t opcode, bool stream = false, bool setUserDataLength = false, uint32_t userDataLength = 0);
    bool add_String(const char* string);
    bool add_U8(uint8_t c);
    bool add_U32(uint32_t ul);
    bool add_S32(int32_t l);
    bool add_U64(uint64_t ull);

    uint8_t* getPtr() { return buffer; }
    uint32_t getLen() { return bufUsed; }
    uint32_t getChannel() { return channel; }
    uint32_t getSerial() { return serialNumber; }

    uint32_t getOpcode() { return opcode; }

  private:
    static uint32_t serialNumberCounter;

    uint8_t* buffer;
    uint32_t bufSize;
    uint32_t bufUsed;
    bool lengthSet;

    uint32_t channel;
    uint32_t serialNumber;

    uint32_t opcode;

    bool checkExtend(uint32_t by);

    const static uint32_t headerLength = 16;
    const static uint32_t userDataLenPos = 12;
};
