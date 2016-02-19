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

#include "RequestPacket.h"
#include "InterProcess.h"
#include "tools.h"

#include <p8-platform/sockets/tcp.h>
#include <stdexcept>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

uint32_t CRequestPacket::m_serialNumberCounter = 1;

CRequestPacket::CRequestPacket(
  uint32_t                 topcode,
  CKODIAddon_InterProcess* process,
  KODI_API_Packets         type               /* = KODIPacket_RequestedResponse*/,
  bool                     setUserDataLength  /* = false*/,
  size_t                   userDataLength     /* = 0 */)
  : m_process(process),
    m_sharedMemUsed(process->SharedMemUsed()),
    m_buffer(nullptr),
    m_bufUsed(0),
    m_lengthSet(false)
{
  if (!m_sharedMemUsed)
  {
    assert(m_buffer == nullptr);

    if (setUserDataLength)
    {
      m_bufSize       = headerLength + userDataLength;
      m_lengthSet     = true;
    }
    else
    {
      m_bufSize       = 512;
      userDataLength  = 0; // so the below will write a zero
    }

    m_buffer = (uint8_t*)malloc(m_bufSize);
    if (!m_buffer)
      throw std::bad_alloc();
  }
  else
  {
    m_buffer       = (uint8_t*)&m_process->m_sharedMem_AddonToKodi->message;
    m_bufSize      = DEFAULT_SHARED_MEM_SIZE;
    userDataLength = 0; // so the below will write a zero
  }

  m_channel       = type;
  m_serialNumber  = m_serialNumberCounter++;
  m_opcode        = topcode;

  *((uint32_t*)(m_buffer+0)) = htonl(m_channel);
  *((uint32_t*)(m_buffer+4)) = htonl(m_serialNumber);
  *((uint32_t*)(m_buffer+8)) = htonl(m_opcode);
  *((uint32_t*)(m_buffer+userDataLenPos)) = htonl(userDataLength);
  m_bufUsed = headerLength;
}

CRequestPacket::~CRequestPacket()
{
  if (!m_sharedMemUsed)
    free(m_buffer);
}

void CRequestPacket::push(KODI_API_Datatype type, const void *value)
{
  switch (type)
  {
  case API_STRING:
  {
    size_t len = strlen((const char*)value) + 1;
    checkExtend(len);
    memcpy(m_buffer + m_bufUsed, (const char*)value, len);
    m_bufUsed += len;
    break;
  }
  case API_BOOLEAN:
  {
    checkExtend(sizeof(bool));
    *((bool*)(m_buffer+m_bufUsed)) = htonl(*((bool*)value));
    m_bufUsed += sizeof(bool);
    break;
  }
  case API_CHAR:
  {
    checkExtend(sizeof(char));
    *((char*)(m_buffer+m_bufUsed)) = htonl(*((char*)value));
    m_bufUsed += sizeof(char);
    break;
  }
  case API_SIGNED_CHAR:
  {
    checkExtend(sizeof(signed char));
    *((signed char*)(m_buffer+m_bufUsed)) = htonl(*((signed char*)value));
    m_bufUsed += sizeof(signed char);
    break;
  }
  case API_UNSIGNED_CHAR:
  {
    checkExtend(sizeof(unsigned char));
    *((unsigned char*)(m_buffer+m_bufUsed)) = htonl(*((unsigned char*)value));
    m_bufUsed += sizeof(unsigned char);
    break;
  }
  case API_SHORT:
  {
    checkExtend(sizeof(short));
    *((short*)(m_buffer+m_bufUsed)) = htonl(*((short*)value));
    m_bufUsed += sizeof(short);
    break;
  }
  case API_SIGNED_SHORT:
  {
    checkExtend(sizeof(signed short));
    *((signed short*)(m_buffer+m_bufUsed)) = htonl(*((signed short*)value));
    m_bufUsed += sizeof(signed short);
    break;
  }
  case API_UNSIGNED_SHORT:
  {
    checkExtend(sizeof(unsigned short));
    *((unsigned short*)(m_buffer+m_bufUsed)) = htonl(*((unsigned short*)value));
    m_bufUsed += sizeof(unsigned short);
    break;
  }
  case API_INT:
  {
    checkExtend(sizeof(int));
    *((int*)(m_buffer+m_bufUsed)) = htonl(*((int*)value));
    m_bufUsed += sizeof(int);
    break;
  }
  case API_SIGNED_INT:
  {
    checkExtend(sizeof(signed int));
    *((signed int*)(m_buffer+m_bufUsed)) = htonl(*((signed int*)value));
    m_bufUsed += sizeof(signed int);
    break;
  }
  case API_UNSIGNED_INT:
  {
    checkExtend(sizeof(unsigned int));
    *((unsigned int*)(m_buffer+m_bufUsed)) = htonl(*((unsigned int*)value));
    m_bufUsed += sizeof(unsigned int);
    break;
  }
  case API_LONG:
  {
    checkExtend(sizeof(long));
    *((long*)(m_buffer+m_bufUsed)) = (*((long*)value));
    m_bufUsed += sizeof(long);
    break;
  }
  case API_SIGNED_LONG:
  {
    checkExtend(sizeof(signed long));
    *((signed long*)(m_buffer+m_bufUsed)) = (*((signed long*)value));
    m_bufUsed += sizeof(signed long);
    break;
  }
  case API_UNSIGNED_LONG:
  {
    checkExtend(sizeof(unsigned long));
    *((unsigned long*)(m_buffer+m_bufUsed)) = (*((unsigned long*)value));
    m_bufUsed += sizeof(unsigned long);
    break;
  }
  case API_FLOAT:
  {
    checkExtend(sizeof(float));
    *((float*)(m_buffer+m_bufUsed)) = htonl(*((float*)value));
    m_bufUsed += sizeof(float);
    break;
  }
  case API_DOUBLE:
  {
    checkExtend(sizeof(double));
    *((double*)(m_buffer+m_bufUsed)) = (*((double*)value));
    m_bufUsed += sizeof(double);
    break;
  }
  case API_INT8_T:
  {
    checkExtend(sizeof(int8_t));
    *((int8_t*)(m_buffer+m_bufUsed)) = htonl(*((int8_t*)value));
    m_bufUsed += sizeof(int8_t);
    break;
  }
  case API_INT16_T:
  {
    checkExtend(sizeof(int16_t));
    *((int16_t*)(m_buffer+m_bufUsed)) = htonl(*((int16_t*)value));
    m_bufUsed += sizeof(int16_t);
    break;
  }
  case API_INT32_T:
  {
    checkExtend(sizeof(int32_t));
    *((int32_t*)(m_buffer+m_bufUsed)) = htonl(*((int32_t*)value));
    m_bufUsed += sizeof(int32_t);
    break;
  }
  case API_INT64_T:
  {
    checkExtend(sizeof(int64_t));
    *((int64_t*)(m_buffer+m_bufUsed)) = (*((int64_t*)value));
    m_bufUsed += sizeof(int64_t);
    break;
  }
  case API_UINT8_T:
  {
    checkExtend(sizeof(uint8_t));
    *((uint8_t*)(m_buffer+m_bufUsed)) = htonl(*((uint8_t*)value));
    m_bufUsed += sizeof(uint8_t);
    break;
  }
  case API_UINT16_T:
  {
    checkExtend(sizeof(uint16_t));
    *((uint16_t*)(m_buffer+m_bufUsed)) = htonl(*((uint16_t*)value));
    m_bufUsed += sizeof(uint16_t);
    break;
  }
  case API_UINT32_T:
  {
    checkExtend(sizeof(uint32_t));
    *((uint32_t*)(m_buffer+m_bufUsed)) = htonl(*((uint32_t*)value));
    m_bufUsed += sizeof(uint32_t);
    break;
  }
  case API_UINT64_T:
  {
    checkExtend(sizeof(uint64_t));
    *((uint64_t*)(m_buffer+m_bufUsed)) = htonll(*((uint64_t*)value));
    m_bufUsed += sizeof(uint64_t);
    break;
  }
  default:
    break;
  }
  if (!m_lengthSet)
    *((uint32_t*)(m_buffer+userDataLenPos)) = htonl(m_bufUsed - headerLength);
}

void CRequestPacket::checkExtend(size_t by)
{
  if (m_lengthSet)
    return;
  if ((m_bufUsed + by) <= m_bufSize)
    return;

  if (!m_sharedMemUsed)
  {
    uint8_t* newBuf = (uint8_t*)realloc(m_buffer, m_bufUsed + by);
    if (!newBuf)
    {
      newBuf = (uint8_t*)malloc(m_bufUsed + by);
      if (!newBuf)
        throw std::bad_alloc();

      memcpy(newBuf, m_buffer, m_bufUsed);
      free(m_buffer);
    }
    m_buffer  = newBuf;
    m_bufSize = m_bufUsed + by;
  }
  else
  {
    throw std::out_of_range("Shared Mem Package request out of available size");
  }
}
