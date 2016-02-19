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
#include "tools.h"
#include "responsepacket.h"

#include <string.h>
#include <netinet/in.h>


/* Packet format for an RR channel response:
 *
 * 4 bytes = channel ID = 1 (request/response channel)
 * 4 bytes = request ID (from serialNumber)
 * 4 bytes = length of the rest of the packet
 * ? bytes = rest of packet. depends on packet
*/

CResponsePacket::CResponsePacket(uint8_t* buffer, size_t bufSize)
  : m_sharedMemUsed(buffer != nullptr),
    m_buffer(buffer),
    m_bufSize(bufSize),
    m_bufUsed(0)
{
}

CResponsePacket::~CResponsePacket()
{
  if (!m_sharedMemUsed && m_buffer)
    free(m_buffer);
}

void CResponsePacket::initBuffers()
{
  if (m_buffer == nullptr)
  {
    m_bufSize = 512;
    m_buffer = (uint8_t*)malloc(m_bufSize);
  }
}

void CResponsePacket::init(uint32_t requestID)
{
  initBuffers();
  *((uint32_t*)(m_buffer+0)) = htonl(KODIPacket_RequestedResponse);
  *((uint32_t*)(m_buffer+4)) = htonl(requestID);
  *((uint32_t*)(m_buffer+userDataLenPos)) = 0;
  m_bufUsed = headerLength;
}

void CResponsePacket::initStatus(uint32_t opCode)
{
  initBuffers();
  *((uint32_t*)(m_buffer+0)) = htonl(KODIPacket_Status);
  *((uint32_t*)(m_buffer+4)) = htonl(opCode);
  *((uint32_t*)(m_buffer+userDataLenPos)) = 0;
  m_bufUsed = headerLength;
}

void CResponsePacket::finalise()
{
  *((uint32_t*)(m_buffer+userDataLenPos)) = htonl(m_bufUsed - headerLength);
}

bool CResponsePacket::copyin(const uint8_t* src, uint32_t len)
{
  if (!checkExtend(len))
    return false;

  memcpy(m_buffer + m_bufUsed, src, len);
  m_bufUsed += len;

  return true;
}

uint8_t* CResponsePacket::reserve(uint32_t len)
{
  if (!checkExtend(len))
    return 0;

  uint8_t* result = m_buffer + m_bufUsed;
  m_bufUsed += len;

  return result;
}

bool CResponsePacket::unreserve(uint32_t len)
{
  if(m_bufUsed < len)
    return false;

  m_bufUsed -= len;

  return true;
}

bool CResponsePacket::push(KODI_API_Datatype type, const void *value)
{
  switch (type)
  {
  case API_STRING:
  {
    size_t len = strlen((const char*)value) + 1;
    if (!checkExtend(len))
      return false;
    memcpy(m_buffer + m_bufUsed, (const char*)value, len);
    m_bufUsed += len;
    break;
  }
  case API_BOOLEAN:
  {
    if (!checkExtend(sizeof(bool)))
      return false;
    *((bool*)(m_buffer+m_bufUsed)) = htonl(*((bool*)value));
    m_bufUsed += sizeof(bool);
    break;
  }
  case API_CHAR:
  {
    if (!checkExtend(sizeof(char)))
      return false;
    *((char*)(m_buffer+m_bufUsed)) = htonl(*((char*)value));
    m_bufUsed += sizeof(char);
    break;
  }
  case API_SIGNED_CHAR:
  {
    if (!checkExtend(sizeof(signed char)))
      return false;
    *((signed char*)(m_buffer+m_bufUsed)) = htonl(*((signed char*)value));
    m_bufUsed += sizeof(signed char);
    break;
  }
  case API_UNSIGNED_CHAR:
  {
    if (!checkExtend(sizeof(unsigned char)))
      return false;
    *((unsigned char*)(m_buffer+m_bufUsed)) = htonl(*((unsigned char*)value));
    m_bufUsed += sizeof(unsigned char);
    break;
  }
  case API_SHORT:
  {
    if (!checkExtend(sizeof(short)))
      return false;
    *((short*)(m_buffer+m_bufUsed)) = htonl(*((short*)value));
    m_bufUsed += sizeof(short);
    break;
  }
  case API_SIGNED_SHORT:
  {
    if (!checkExtend(sizeof(signed short)))
      return false;
    *((signed short*)(m_buffer+m_bufUsed)) = htonl(*((signed short*)value));
    m_bufUsed += sizeof(signed short);
    break;
  }
  case API_UNSIGNED_SHORT:
  {
    if (!checkExtend(sizeof(unsigned short)))
      return false;
    *((unsigned short*)(m_buffer+m_bufUsed)) = htonl(*((unsigned short*)value));
    m_bufUsed += sizeof(unsigned short);
    break;
  }
  case API_INT:
  {
    if (!checkExtend(sizeof(int)))
      return false;
    *((int*)(m_buffer+m_bufUsed)) = htonl(*((int*)value));
    m_bufUsed += sizeof(int);
    break;
  }
  case API_SIGNED_INT:
  {
    if (!checkExtend(sizeof(signed int)))
      return false;
    *((signed int*)(m_buffer+m_bufUsed)) = htonl(*((signed int*)value));
    m_bufUsed += sizeof(signed int);
    break;
  }
  case API_UNSIGNED_INT:
  {
    if (!checkExtend(sizeof(unsigned int)))
      return false;
    *((unsigned int*)(m_buffer+m_bufUsed)) = htonl(*((unsigned int*)value));
    m_bufUsed += sizeof(unsigned int);
    break;
  }
  case API_LONG:
  {
    if (!checkExtend(sizeof(long)))
      return false;
    *((long*)(m_buffer+m_bufUsed)) = (*((long*)value));
    m_bufUsed += sizeof(long);
    break;
  }
  case API_SIGNED_LONG:
  {
    if (!checkExtend(sizeof(signed long)))
      return false;
    *((signed long*)(m_buffer+m_bufUsed)) = (*((signed long*)value));
    m_bufUsed += sizeof(signed long);
    break;
  }
  case API_UNSIGNED_LONG:
  {
    if (!checkExtend(sizeof(unsigned long)))
      return false;
    *((unsigned long*)(m_buffer+m_bufUsed)) = (*((unsigned long*)value));
    m_bufUsed += sizeof(unsigned long);
    break;
  }
  case API_FLOAT:
  {
    if (!checkExtend(sizeof(float)))
      return false;
    *((float*)(m_buffer+m_bufUsed)) = htonl(*((float*)value));
    m_bufUsed += sizeof(float);
    break;
  }
  case API_DOUBLE:
  {
    if (!checkExtend(sizeof(double)))
      return false;
    *((double*)(m_buffer+m_bufUsed)) = (*((double*)value));
    m_bufUsed += sizeof(double);
    break;
  }
  case API_INT8_T:
  {
    if (!checkExtend(sizeof(int8_t)))
      return false;
    *((int8_t*)(m_buffer+m_bufUsed)) = htonl(*((int8_t*)value));
    m_bufUsed += sizeof(int8_t);
    break;
  }
  case API_INT16_T:
  {
    if (!checkExtend(sizeof(int16_t)))
      return false;
    *((int16_t*)(m_buffer+m_bufUsed)) = htonl(*((int16_t*)value));
    m_bufUsed += sizeof(int16_t);
    break;
  }
  case API_INT32_T:
  {
    if (!checkExtend(sizeof(int32_t)))
      return false;
    *((int32_t*)(m_buffer+m_bufUsed)) = htonl(*((int32_t*)value));
    m_bufUsed += sizeof(int32_t);
    break;
  }
  case API_INT64_T:
  {
    if (!checkExtend(sizeof(int64_t)))
      return false;
    *((int64_t*)(m_buffer+m_bufUsed)) = (*((int64_t*)value));
    m_bufUsed += sizeof(int64_t);
    break;
  }
  case API_UINT8_T:
  {
    if (!checkExtend(sizeof(uint8_t)))
      return false;
    *((uint8_t*)(m_buffer+m_bufUsed)) = htonl(*((uint8_t*)value));
    m_bufUsed += sizeof(uint8_t);
    break;
  }
  case API_UINT16_T:
  {
    if (!checkExtend(sizeof(uint16_t)))
      return false;
    *((uint16_t*)(m_buffer+m_bufUsed)) = htonl(*((uint16_t*)value));
    m_bufUsed += sizeof(uint16_t);
    break;
  }
  case API_UINT32_T:
  {
    if (!checkExtend(sizeof(uint32_t)))
      return false;
    *((uint32_t*)(m_buffer+m_bufUsed)) = htonl(*((uint32_t*)value));
    m_bufUsed += sizeof(uint32_t);
    break;
  }
  case API_UINT64_T:
  {
    if (!checkExtend(sizeof(uint64_t)))
      return false;
    *((uint64_t*)(m_buffer+m_bufUsed)) = htonll(*((uint64_t*)value));
    m_bufUsed += sizeof(uint64_t);
    break;
  }
  default:
    break;
  }
  return false;
}

bool CResponsePacket::checkExtend(uint32_t by)
{
  if ((m_bufUsed + by) < m_bufSize)
    return true;

  if (m_sharedMemUsed)
    return false;

  if (512 > by)
    by = 512;

  uint8_t* newBuf = (uint8_t*)realloc(m_buffer, m_bufSize + by);
  if (!newBuf)
    return false;
  m_buffer = newBuf;
  m_bufSize += by;

  return true;
}
