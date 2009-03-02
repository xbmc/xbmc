/*
 *      Copyright (C) 2009 phi2039
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

#include "stdafx.h"
#include "SimpleBuffer.h"

CSimpleBuffer::CSimpleBuffer() :
  m_pBuffer(NULL),
  m_BufferOffset(0),
  m_Locked(false)
{

}

CSimpleBuffer::~CSimpleBuffer()
{
  if(m_pBuffer)
    delete[] m_pBuffer;
}

bool CSimpleBuffer::Initialize(unsigned int maxData)
{  
  if (maxData == m_BufferSize)
    Empty();
  else
  {
    if(m_pBuffer)
      delete[] m_pBuffer;

    m_BufferSize = maxData;
    m_pBuffer = new BYTE[m_BufferSize];
  }
  return true;
}

unsigned int CSimpleBuffer::Write(void* pData, size_t len)
{
  if(!m_pBuffer)
    return 0;

  if(m_BufferSize - m_BufferOffset < len)
    return 0;

  memcpy(&m_pBuffer[m_BufferOffset], pData, len);
  
  m_BufferOffset += len;
  return len;
}

void* CSimpleBuffer::GetData(unsigned int* pBytesRead)
{
  if (pBytesRead)
    *pBytesRead = m_BufferOffset;

  return m_pBuffer;
}

unsigned int CSimpleBuffer::GetLen()
{
  return m_BufferOffset;
}

unsigned int CSimpleBuffer::GetMaxLen()
{
  return m_BufferSize;
}

unsigned int CSimpleBuffer::GetSpace()
{
  return m_BufferSize - m_BufferOffset;
}

unsigned int CSimpleBuffer::ShiftUp(unsigned int bytesToShift)
{
  if(bytesToShift > m_BufferOffset)
    bytesToShift = m_BufferOffset;

  // TODO: This can be problematic if the sections overlap
  memcpy(m_pBuffer,&m_pBuffer[bytesToShift],m_BufferOffset - bytesToShift);
  m_BufferOffset -= bytesToShift;
  return bytesToShift;
}

void CSimpleBuffer::Empty()
{
  m_BufferOffset = 0;
}

void* CSimpleBuffer::Lock(size_t len)
{
  if (m_Locked || len > GetSpace())
    return false;

  m_Locked = true;

  return &m_pBuffer[m_BufferOffset];
}

void CSimpleBuffer::Unlock(size_t bytesWritten)
{
  //if (bytesWritten > GetSpace())
  //  __asm nop ; // We have a serious problem (but what can we do?) - nop is to avoid compiler complaint

  m_BufferOffset += bytesWritten;
  m_Locked = false;
}