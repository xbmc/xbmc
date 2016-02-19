/*
 *      Copyright (C) 2010-2016 Team KODI
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

#include "ResponsePacket.h"
#include "tools.h"

#include <p8-platform/sockets/tcp.h>
#include <stdexcept>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

CResponsePacket::CResponsePacket(bool sharedMem/* = true*/)
  : m_sharedMem(sharedMem),
    m_responseData(NULL),
    m_responseDataLength(0),
    m_packetPos(0),
    m_channelID(0),
    m_requestID(0)
{
}

CResponsePacket::~CResponsePacket()
{
  if (!m_sharedMem && m_responseData)
  {
    free(m_responseData);
  }
}

void CResponsePacket::setResponse(uint8_t* tuserData, size_t tuserDataLength)
{
  m_channelID           = KODIPacket_RequestedResponse;
  m_responseData        = tuserData;
  m_responseDataLength  = tuserDataLength;
  m_packetPos           = 0;
}

void CResponsePacket::setStatus(uint8_t* tuserData, size_t tuserDataLength)
{
  m_channelID           = KODIPacket_Status;
  m_responseData        = tuserData;
  m_responseDataLength  = tuserDataLength;
  m_packetPos           = 0;
}

void CResponsePacket::popHeader()
{
  // set data pointers to header first
  m_responseData        = m_header;
  m_responseDataLength  = sizeof(m_header);
  m_packetPos           = 0;

  pop(API_UINT32_T, &m_requestID);
  pop(API_UINT32_T, &m_responseDataLength);

  m_responseData = NULL;
}

void* CResponsePacket::pop(KODI_API_Datatype type, void* value)
{
  switch (type)
  {
  case API_STRING:
  {
    *((char**)value) = (char *)&m_responseData[m_packetPos];
    const char *end = (const char *)memchr(*((char**)value), '\0', m_responseDataLength - m_packetPos);
    if (end == NULL)
      /* string is not terminated - fail */
      throw std::out_of_range("Malformed VNSI packet");

    int length = end - *((char**)value);
    m_packetPos += length + 1;
    return *((char**)value);
  }
  case API_BOOLEAN:
  {
    if ((m_packetPos + sizeof(bool)) > m_responseDataLength)
      throw std::out_of_range("Malformed Kodi packet");
    *((bool*)value) = ntohl(*((bool*)(m_responseData+m_packetPos)));
    m_packetPos += sizeof(bool);
    break;
  }
  case API_CHAR:
  {
    if ((m_packetPos + sizeof(char)) > m_responseDataLength)
      throw std::out_of_range("Malformed Kodi packet");
    *((char*)value) = ntohl(*((char*)(m_responseData+m_packetPos)));
    m_packetPos += sizeof(char);
    break;
  }
  case API_SIGNED_CHAR:
  {
    if ((m_packetPos + sizeof(signed char)) > m_responseDataLength)
      throw std::out_of_range("Malformed Kodi packet");
    *((signed char*)value) = ntohl(*((signed char*)(m_responseData+m_packetPos)));
    m_packetPos += sizeof(signed char);
    break;
  }
  case API_UNSIGNED_CHAR:
  {
    if ((m_packetPos + sizeof(unsigned char)) > m_responseDataLength)
      throw std::out_of_range("Malformed Kodi packet");
    *((unsigned char*)value) = ntohl(*((unsigned char*)(m_responseData+m_packetPos)));
    m_packetPos += sizeof(unsigned char);
    break;
  }
  case API_SHORT:
  {
    if ((m_packetPos + sizeof(short)) > m_responseDataLength)
      throw std::out_of_range("Malformed Kodi packet");
    *((short*)value) = ntohl(*((short*)(m_responseData+m_packetPos)));
    m_packetPos += sizeof(short);
    break;
  }
  case API_SIGNED_SHORT:
  {
    if ((m_packetPos + sizeof(signed short)) > m_responseDataLength)
      throw std::out_of_range("Malformed Kodi packet");
    *((signed short*)value) = ntohl(*((signed short*)(m_responseData+m_packetPos)));
    m_packetPos += sizeof(signed short);
    break;
  }
  case API_UNSIGNED_SHORT:
  {
    if ((m_packetPos + sizeof(unsigned short)) > m_responseDataLength)
      throw std::out_of_range("Malformed Kodi packet");
    *((unsigned short*)value) = ntohl(*((unsigned short*)(m_responseData+m_packetPos)));
    m_packetPos += sizeof(unsigned short);
    break;
  }
  case API_INT:
  {
    if ((m_packetPos + sizeof(int)) > m_responseDataLength)
      throw std::out_of_range("Malformed Kodi packet");
    *((int*)value) = ntohl(*((int*)(m_responseData+m_packetPos)));
    m_packetPos += sizeof(int);
    break;
  }
  case API_SIGNED_INT:
  {
    if ((m_packetPos + sizeof(signed int)) > m_responseDataLength)
      throw std::out_of_range("Malformed Kodi packet");
    *((signed int*)value) = ntohl(*((signed int*)(m_responseData+m_packetPos)));
    m_packetPos += sizeof(signed int);
    break;
  }
  case API_UNSIGNED_INT:
  {
    if ((m_packetPos + sizeof(unsigned int)) > m_responseDataLength)
      throw std::out_of_range("Malformed Kodi packet");
    *((unsigned int*)value) = ntohl(*((unsigned int*)(m_responseData+m_packetPos)));
    m_packetPos += sizeof(unsigned int);
    break;
  }
  case API_LONG:
  {
    if ((m_packetPos + sizeof(long)) > m_responseDataLength)
      throw std::out_of_range("Malformed Kodi packet");
    *((long*)value) = ntohll(*((long*)(m_responseData+m_packetPos)));
    m_packetPos += sizeof(long);
    break;
  }
  case API_SIGNED_LONG:
  {
    if ((m_packetPos + sizeof(signed long)) > m_responseDataLength)
      throw std::out_of_range("Malformed Kodi packet");
    *((signed long*)value) = ntohll(*((signed long*)(m_responseData+m_packetPos)));
    m_packetPos += sizeof(signed long);
    break;
  }
  case API_UNSIGNED_LONG:
  {
    if ((m_packetPos + sizeof(unsigned long)) > m_responseDataLength)
      throw std::out_of_range("Malformed Kodi packet");
    *((unsigned long*)value) = ntohll(*((unsigned long*)(m_responseData+m_packetPos)));
    m_packetPos += sizeof(unsigned long);
    break;
  }
  case API_FLOAT:
  {
    if ((m_packetPos + sizeof(float)) > m_responseDataLength)
      throw std::out_of_range("Malformed Kodi packet");
    *((float*)value) = ntohl(*((float*)(m_responseData+m_packetPos)));
    m_packetPos += sizeof(float);
    break;
  }
  case API_DOUBLE:
  {
    if ((m_packetPos + sizeof(double)) > m_responseDataLength)
      throw std::out_of_range("Malformed Kodi packet");
    *((double*)value) = ntohll(*((double*)(m_responseData+m_packetPos)));
    m_packetPos += sizeof(double);
    break;
  }
  case API_LONG_DOUBLE:
  {
    if ((m_packetPos + sizeof(long double)) > m_responseDataLength)
      throw std::out_of_range("Malformed Kodi packet");
    *((long double*)value) = ntohll(*((long double*)(m_responseData+m_packetPos)));
    m_packetPos += sizeof(long double);
    break;
  }
  case API_INT8_T:
  {
    if ((m_packetPos + sizeof(int8_t)) > m_responseDataLength)
      throw std::out_of_range("Malformed Kodi packet");
    *((int8_t*)value) = (*((int8_t*)(m_responseData+m_packetPos)));
    m_packetPos += sizeof(int8_t);
    break;
  }
  case API_INT16_T:
  {
    if ((m_packetPos + sizeof(int16_t)) > m_responseDataLength)
      throw std::out_of_range("Malformed Kodi packet");
    *((int16_t*)value) = (*((int16_t*)(m_responseData+m_packetPos)));
    m_packetPos += sizeof(int16_t);
    break;
  }
  case API_INT32_T:
  {
    if ((m_packetPos + sizeof(int32_t)) > m_responseDataLength)
      throw std::out_of_range("Malformed Kodi packet");
    *((int32_t*)value) = ntohl(*((int32_t*)(m_responseData+m_packetPos)));
    m_packetPos += sizeof(int32_t);
    break;
  }
  case API_INT64_T:
  {
    if ((m_packetPos + sizeof(int64_t)) > m_responseDataLength)
      throw std::out_of_range("Malformed Kodi packet");
    *((int64_t*)value) = ntohll(*((int64_t*)(m_responseData+m_packetPos)));
    m_packetPos += sizeof(int64_t);
    break;
  }
  case API_UINT8_T:
  {
    if ((m_packetPos + sizeof(uint8_t)) > m_responseDataLength)
      throw std::out_of_range("Malformed Kodi packet");
    *((uint8_t*)value) = ntohl(*((uint8_t*)(m_responseData+m_packetPos)));
    m_packetPos += sizeof(uint8_t);
    break;
  }
  case API_UINT16_T:
  {
    if ((m_packetPos + sizeof(uint16_t)) > m_responseDataLength)
      throw std::out_of_range("Malformed Kodi packet");
    *((uint16_t*)value) = ntohl(*((uint16_t*)(m_responseData+m_packetPos)));
    m_packetPos += sizeof(uint16_t);
    break;
  }
  case API_UINT32_T:
  {
    if ((m_packetPos + sizeof(uint32_t)) > m_responseDataLength)
      throw std::out_of_range("Malformed Kodi packet");
    *((uint32_t*)value) = ntohl(*((uint32_t*)(m_responseData+m_packetPos)));
    m_packetPos += sizeof(uint32_t);
    break;
  }
  case API_UINT64_T:
  {
    if ((m_packetPos + sizeof(uint64_t)) > m_responseDataLength)
      throw std::out_of_range("Malformed Kodi packet");
    *((uint64_t*)value) = ntohll(*((uint64_t*)(m_responseData+m_packetPos)));
    m_packetPos += sizeof(uint64_t);
    break;
  }
  default:
    break;
  }
  return ((void*)value);
}

uint8_t* CResponsePacket::getUserData()
{
  return m_responseData;
}

uint8_t* CResponsePacket::stealUserData()
{
  uint8_t *result = m_responseData;
  m_responseData = NULL;
  return result;
}
