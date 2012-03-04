/*
 *      Copyright (C) 2007 Chris Tallon
 *      Copyright (C) 2010 Alwin Esch (Team XBMC)
 *      Copyright (C) 2011 Alexander Pipelka
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
#include <string.h>

#include "XVDRResponsePacket.h"
#include "xvdrcommand.h"
#include "tools.h"
#include "client.h"

extern "C" {
#include "libTcpSocket/os-dependent_socket.h"
}

#ifdef HAVE_ZLIB
#include "zlib.h"
#endif

using namespace ADDON;

cXVDRResponsePacket::cXVDRResponsePacket()
{
  userDataLength  = 0;
  packetPos       = 0;
  userData        = NULL;
  ownBlock        = true;
  channelID       = 0;
  requestID       = 0;
  streamID        = 0;
}

cXVDRResponsePacket::~cXVDRResponsePacket()
{
  if (!ownBlock) return; // don't free if it's a getblock

  if (userData) free(userData);
}

void cXVDRResponsePacket::setResponse(uint32_t trequestID, uint8_t* tuserData, uint32_t tuserDataLength)
{
  channelID       = XVDR_CHANNEL_REQUEST_RESPONSE;
  requestID       = trequestID;
  userData        = tuserData;
  userDataLength  = tuserDataLength;
}

void cXVDRResponsePacket::setStatus(uint32_t trequestID, uint8_t* tuserData, uint32_t tuserDataLength)
{
  channelID       = XVDR_CHANNEL_STATUS;
  requestID       = trequestID;
  userData        = tuserData;
  userDataLength  = tuserDataLength;
}

void cXVDRResponsePacket::setStream(uint32_t topcodeID, uint32_t tstreamID, uint32_t tduration, int64_t tdts, int64_t tpts, uint8_t* tuserData, uint32_t tuserDataLength)
{
  channelID       = XVDR_CHANNEL_STREAM;
  opcodeID        = topcodeID;
  streamID        = tstreamID;
  duration        = tduration;
  dts             = tdts;
  pts             = tpts;
  userData        = tuserData;
  userDataLength  = tuserDataLength;
}

bool cXVDRResponsePacket::end()
{
  return (packetPos >= userDataLength);
}

int cXVDRResponsePacket::serverError()
{
  if ((packetPos == 0) && (userDataLength == 4) && !ntohl(*(uint32_t*)userData)) return 1;
  else return 0;
}

void cXVDRResponsePacket::ConvertToUTF8(std::string& value)
{
}

char* cXVDRResponsePacket::extract_String()
{
  std::string s;
  if(!extract_String(s))
    return NULL;

  char* str = new char[s.size()+1];
  strcpy(str, s.c_str());

  return str;
}

bool cXVDRResponsePacket::extract_String(std::string& value)
{
  value.clear();
  if (serverError()) return false;

  int length = strlen((char*)&userData[packetPos]);
  if ((packetPos + length) > userDataLength) return NULL;
  value = (char*)&userData[packetPos];
  packetPos += length + 1;

  return true;
}

uint8_t cXVDRResponsePacket::extract_U8()
{
  if ((packetPos + sizeof(uint8_t)) > userDataLength) return 0;
  uint8_t uc = userData[packetPos];
  packetPos += sizeof(uint8_t);
  return uc;
}

uint32_t cXVDRResponsePacket::extract_U32()
{
  if ((packetPos + sizeof(uint32_t)) > userDataLength) return 0;
  uint32_t ul = ntohl(*(uint32_t*)&userData[packetPos]);
  packetPos += sizeof(uint32_t);
  return ul;
}

uint64_t cXVDRResponsePacket::extract_U64()
{
  if ((packetPos + sizeof(uint64_t)) > userDataLength) return 0;
  uint64_t ull = ntohll(*(uint64_t*)&userData[packetPos]);
  packetPos += sizeof(uint64_t);
  return ull;
}

double cXVDRResponsePacket::extract_Double()
{
  if ((packetPos + sizeof(uint64_t)) > userDataLength) return 0;
  uint64_t ull = ntohll(*(uint64_t*)&userData[packetPos]);
  double d;
  memcpy(&d,&ull,sizeof(double));
  packetPos += sizeof(uint64_t);
  return d;
}

int32_t cXVDRResponsePacket::extract_S32()
{
  if ((packetPos + sizeof(int32_t)) > userDataLength) return 0;
  int32_t l = ntohl(*(int32_t*)&userData[packetPos]);
  packetPos += sizeof(int32_t);
  return l;
}

int64_t cXVDRResponsePacket::extract_S64()
{
  if ((packetPos + sizeof(int64_t)) > userDataLength) return 0;
  int64_t ll = ntohll(*(int64_t*)&userData[packetPos]);
  packetPos += sizeof(int64_t);
  return ll;
}

uint8_t* cXVDRResponsePacket::getUserData()
{
  ownBlock = false;
  return userData;
}

bool cXVDRResponsePacket::uncompress()
{
#ifdef HAVE_ZLIB
  XBMC->Log(LOG_DEBUG, "Uncompressing packet (%i bytes) ...", userDataLength - 4);

  uLongf original_size = ntohl(*(uint32_t*)&userData[0]);
  uint8_t* buffer = (uint8_t*)malloc(original_size);

  if(buffer == NULL)
    return false;

  if(::uncompress(buffer, &original_size, userData + 4, userDataLength - 4) == Z_OK)
  {
    free(userData);
    userData        = buffer;
    userDataLength  = original_size;
    XBMC->Log(LOG_DEBUG, "Done. (Now %i bytes)", original_size);
    return true;
  }
  else
    XBMC->Log(LOG_DEBUG, "Failed!");

  free(buffer);
#endif
  return false;
}
