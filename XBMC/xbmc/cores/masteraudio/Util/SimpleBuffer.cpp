/*
 *      Copyright (C) 2009 Team XBMC
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

#include "stdafx.h"
#include "SimpleBuffer.h"

/////////////////////////////////////////////////
/// \brief The default constructor for CSimpleBuffer.
/////////////////////////////////////////////////
CSimpleBuffer::CSimpleBuffer() :
  m_pBuffer(NULL),
  m_BufferOffset(0),
  m_Locked(false)
{

}

/////////////////////////////////////////////////
/// \brief The destructor for CSimpleBuffer.
/////////////////////////////////////////////////
CSimpleBuffer::~CSimpleBuffer()
{
  delete[] m_pBuffer;
}

//////////////////////////////////////////////////////////////////////////////
/// \brief  Allocates memory and resets the internal state of the buffer.
/// \param[in] size Maximum number of bytes able to be stored by the buffer
/// \return true if the buffer was successfully created, false otherwise
//////////////////////////////////////////////////////////////////////////////
bool CSimpleBuffer::Initialize(size_t size)
{  
  if (size == m_BufferSize)
  {
    // Use the same buffer, just reset the state
    Empty();
  }
  else
  {
    if(m_pBuffer)
      delete[] m_pBuffer;

    m_BufferSize = size;
    m_pBuffer = new BYTE[m_BufferSize];
  }
  return true;
}

//////////////////////////////////////////////////////////////////////////////
/// \brief
/// \param[in] pData
/// \param[in] len
/// \return
//////////////////////////////////////////////////////////////////////////////
size_t CSimpleBuffer::Write(void* pData, size_t len)
{
  if(!m_pBuffer)
    return 0;

  if(m_BufferSize - m_BufferOffset < len)
    return 0;

  memcpy(&m_pBuffer[m_BufferOffset], pData, len);
  
  m_BufferOffset += len;
  return len;
}

//////////////////////////////////////////////////////////////////////////////
/// \brief
/// \param[out] pBytesRead
/// \return
//////////////////////////////////////////////////////////////////////////////
void* CSimpleBuffer::GetData(size_t* pBytesRead)
{
  if (pBytesRead)
    *pBytesRead = m_BufferOffset;

  return m_pBuffer;
}

//////////////////////////////////////////////////////////////////////////////
/// \brief
/// \return
//////////////////////////////////////////////////////////////////////////////
size_t CSimpleBuffer::GetLen()
{
  return m_BufferOffset;
}

//////////////////////////////////////////////////////////////////////////////
/// \brief
/// \return
//////////////////////////////////////////////////////////////////////////////
size_t CSimpleBuffer::GetMaxLen()
{
  return m_BufferSize;
}

//////////////////////////////////////////////////////////////////////////////
/// \brief
/// \return
//////////////////////////////////////////////////////////////////////////////
size_t CSimpleBuffer::GetSpace()
{
  return m_BufferSize - m_BufferOffset;
}

//////////////////////////////////////////////////////////////////////////////
/// \brief
/// \param[in] bytesToShift
/// \return
//////////////////////////////////////////////////////////////////////////////
size_t CSimpleBuffer::ShiftUp(size_t bytesToShift)
{
  if(bytesToShift > m_BufferOffset)
    bytesToShift = m_BufferOffset;

  // TODO: This can be problematic if the sections overlap (i.e. remaining data len > shift len). Probably need multiple writes to be safe.
  memcpy(m_pBuffer,&m_pBuffer[bytesToShift],m_BufferOffset - bytesToShift);
  m_BufferOffset -= bytesToShift;
  return bytesToShift;
}

//////////////////////////////////////////////////////////////////////////////
/// \brief
//////////////////////////////////////////////////////////////////////////////
void CSimpleBuffer::Empty()
{
  m_BufferOffset = 0;
}

//////////////////////////////////////////////////////////////////////////////
/// \brief
/// \param[in] len
/// \return
//////////////////////////////////////////////////////////////////////////////
void* CSimpleBuffer::Lock(size_t len)
{
  if (m_Locked || len > GetSpace())
    return false;

  m_Locked = true;

  return &m_pBuffer[m_BufferOffset];
}

//////////////////////////////////////////////////////////////////////////////
/// \brief
/// \param[in] bytesWritten
//////////////////////////////////////////////////////////////////////////////
void CSimpleBuffer::Unlock(size_t bytesWritten)
{
  //if (bytesWritten > GetSpace())
  //  __asm nop ; // We have a serious problem (but what can we do?) - nop is to avoid compiler complaint

  m_BufferOffset += bytesWritten;
  m_Locked = false;
}