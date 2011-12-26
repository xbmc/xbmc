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

#ifndef VNSI_REQUESTPACKET_H
#define VNSI_REQUESTPACKET_H

class cRequestPacket
{
public:
  cRequestPacket(uint32_t requestID, uint32_t opcode, uint8_t* data, uint32_t dataLength);
  ~cRequestPacket();

  int  serverError();

  uint32_t  getDataLength()     { return userDataLength; }
  uint32_t  getChannelID()      { return channelID; }
  uint32_t  getRequestID()      { return requestID; }
  uint32_t  getStreamID()       { return streamID; }
  uint32_t  getFlag()           { return flag; }
  uint32_t  getOpCode()         { return opCode; }

  char*     extract_String();
  uint8_t   extract_U8();
  uint32_t  extract_U32();
  uint64_t  extract_U64();
  int32_t   extract_S32();
  double    extract_Double();

  bool      end();

  // If you call this, the memory becomes yours. Free with free()
  uint8_t* getData();

private:
  uint8_t* userData;
  uint32_t userDataLength;
  uint32_t packetPos;
  uint32_t opCode;

  uint32_t channelID;

  uint32_t requestID;
  uint32_t streamID;

  uint32_t flag; // stream only

  bool ownBlock;
};

#endif // VNSI_REQUESTPACKET_H
