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

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <asm/byteorder.h>

#include "config.h"
#include "requestpacket.h"
#include "vnsicommand.h"

cRequestPacket::cRequestPacket(uint32_t requestID, uint32_t opcode, uint8_t* data, uint32_t dataLength)
 : userData(data), userDataLength(dataLength), opCode(opcode), requestID(requestID)
{
  packetPos       = 0;
  ownBlock        = true;
  channelID       = 0;
  streamID        = 0;
  flag            = 0;
}

cRequestPacket::~cRequestPacket()
{
  if (!ownBlock) return; // don't free if it's a getblock

  if (userData) free(userData);
}

bool cRequestPacket::end()
{
  return (packetPos >= userDataLength);
}

int cRequestPacket::serverError()
{
  if ((packetPos == 0) && (userDataLength == 4) && !ntohl(*(uint32_t*)userData)) return 1;
  else return 0;
}

char* cRequestPacket::extract_String()
{
  if (serverError()) return NULL;

  int length = strlen((char*)&userData[packetPos]);
  if ((packetPos + length) > userDataLength) return NULL;
  char* str = new char[length + 1];
  strcpy(str, (char*)&userData[packetPos]);
  packetPos += length + 1;
  return str;
}

uint8_t cRequestPacket::extract_U8()
{
  if ((packetPos + sizeof(uint8_t)) > userDataLength) return 0;
  uint8_t uc = userData[packetPos];
  packetPos += sizeof(uint8_t);
  return uc;
}

uint32_t cRequestPacket::extract_U32()
{
  if ((packetPos + sizeof(uint32_t)) > userDataLength) return 0;
  uint32_t ul = ntohl(*(uint32_t*)&userData[packetPos]);
  packetPos += sizeof(uint32_t);
  return ul;
}

uint64_t cRequestPacket::extract_U64()
{
  if ((packetPos + sizeof(uint64_t)) > userDataLength) return 0;
  uint64_t ull = __be64_to_cpu(*(uint64_t*)&userData[packetPos]);
  packetPos += sizeof(uint64_t);
  return ull;
}

double cRequestPacket::extract_Double()
{
  if ((packetPos + sizeof(uint64_t)) > userDataLength) return 0;
  uint64_t ull = __be64_to_cpu(*(uint64_t*)&userData[packetPos]);
  double d;
  memcpy(&d,&ull,sizeof(double));
  packetPos += sizeof(uint64_t);
  return d;
}

int32_t cRequestPacket::extract_S32()
{
  if ((packetPos + sizeof(int32_t)) > userDataLength) return 0;
  int32_t l = ntohl(*(int32_t*)&userData[packetPos]);
  packetPos += sizeof(int32_t);
  return l;
}

uint8_t* cRequestPacket::getData()
{
  ownBlock = false;
  return userData;
}
