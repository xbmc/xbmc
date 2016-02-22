/*
 *      vdr-plugin-vnsi - XBMC server plugin for VDR
 *
 *      Copyright (C) 2007 Chris Tallon
 *      Copyright (C) 2010 Alwin Esch (Team XBMC)
 *      Copyright (C) 2010, 2011 Alexander Pipelka
 *
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

/*
 * This code is taken from VOMP for VDR plugin.
 */

#ifndef VNSI_RESPONSEPACKET_H
#define VNSI_RESPONSEPACKET_H

class cResponsePacket
{
public:
  cResponsePacket();
  ~cResponsePacket();

  bool init(uint32_t requestID);
  bool initScan(uint32_t opCode);
  bool initStatus(uint32_t opCode);
  bool initStream(uint32_t opCode, uint32_t streamID, uint32_t duration, int64_t dts, int64_t pts);
  void finalise();
  void finaliseStream();
  bool copyin(const uint8_t* src, uint32_t len);
  uint8_t* reserve(uint32_t len);
  bool unreserve(uint32_t len);

  bool add_String(const char* string);
  bool add_U32(uint32_t ul);
  bool add_S32(int32_t l);
  bool add_U8(uint8_t c);
  bool add_U64(uint64_t ull);
  bool add_double(double d);

  uint8_t* getPtr() { return buffer; }
  uint32_t getLen() { return bufUsed; }

private:
  uint8_t* buffer;
  uint32_t bufSize;
  uint32_t bufUsed;

  void initBuffers();
  bool checkExtend(uint32_t by);

  const static uint32_t headerLength          = 12;
  const static uint32_t userDataLenPos        = 8;
  const static uint32_t headerLengthStream    = 36;
  const static uint32_t userDataLenPosStream  = 32;
};

#endif // VNSI_RESPONSEPACKET_H

