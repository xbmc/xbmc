#pragma once
/*
 *      Copyright (C) 2010-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "system.h"

#ifdef _DEBUG
#include "utils/StdString.h" /* needed for ASSERT */
#endif

/**
 * This class wraps a block of 16 byte aligned memory for simple buffer
 * operations, if _DEBUG is defined then size is always verified.
 */
class CAEBuffer
{
private:
  uint8_t *m_buffer;
  size_t   m_bufferSize;
  size_t   m_bufferPos;
  size_t   m_cursorPos;

public:
  CAEBuffer();
  ~CAEBuffer();

  /* initialize methods */
  void Alloc  (const size_t size);
  void ReAlloc(const size_t size);
  void DeAlloc();

  /* usage methods */
  inline size_t Size () { return m_bufferSize; }
  inline size_t Used () { return m_bufferPos ; }
  inline size_t Free () { return m_bufferSize - m_bufferPos; }
  inline void   Empty() { m_bufferPos = 0; }

  /* write methods */
  inline void Write(const void *src, const size_t size)
  {
  #ifdef _DEBUG
    ASSERT(src);
    ASSERT(size <= m_bufferSize);
  #endif
    memcpy(m_buffer, src, size);
    m_bufferPos = 0;
  }

  inline void Push(const void *src, const size_t size)
  {
  #ifdef _DEBUG
    ASSERT(src);
    ASSERT(size <= m_bufferSize - m_bufferPos);
  #endif
    memcpy(m_buffer + m_bufferPos, src, size);
    m_bufferPos += size;
  }

  inline void UnShift(const void *src, const size_t size)
  {
  #ifdef _DEBUG
    ASSERT(src);
    ASSERT(size < m_bufferSize - m_bufferPos);
  #endif
    memmove(m_buffer + size, m_buffer, m_bufferSize - size);
    memcpy (m_buffer, src, size);
    m_bufferPos += size;
  }

  inline void* Take(const size_t size)
  {
  #ifdef _DEBUG
    ASSERT(size <= m_bufferSize - m_bufferPos);
  #endif

    void* ret = m_buffer + m_bufferPos;
    m_bufferPos += size;
    return ret;
  }

  /* raw methods */
  inline void* Raw(const size_t size)
  {
  #ifdef _DEBUG
    ASSERT(size <= m_bufferSize);
  #endif
    return m_buffer;
  }

  /* read methods */
  inline void Read(void *dst, const size_t size)
  {
  #ifdef _DEBUG
    ASSERT(size <= m_bufferSize);
    ASSERT(dst);
  #endif
    memcpy(dst, m_buffer, size);
  }

  inline void Pop(void *dst, const size_t size)
  {
  #ifdef _DEBUG
    ASSERT(size <= m_bufferPos);
  #endif
    m_bufferPos -= size;
    if (dst)
      memcpy(dst, m_buffer + m_bufferPos, size);
  }

  inline void Shift(void *dst, const size_t size)
  {
  #ifdef _DEBUG
    ASSERT(size <= m_bufferPos);
  #endif
    if (dst)
      memcpy(dst, m_buffer, size);
    // we can just reset m_bufferPos
    // if there is nothing else inside.
    if ((m_bufferPos != size) && size <= m_bufferPos)
      memmove(m_buffer, m_buffer + size, m_bufferSize - size);
    if (size <= m_bufferPos)
      m_bufferPos -= size;
  }

  /* cursor methods */
  inline void CursorReset()
  {
    m_cursorPos = 0;
  }

  inline size_t CursorOffset()
  {
    return m_cursorPos;
  }

  inline bool CursorEnd()
  {
    return m_cursorPos == m_bufferPos;
  }

  inline void CursorSeek (const size_t pos )
  {
  #ifdef _DEBUG
    ASSERT(pos <= m_bufferSize);
  #endif
    m_cursorPos = pos;
  }

  inline void* CursorRead(const size_t size)
  {
  #ifdef _DEBUG
    ASSERT(m_cursorPos + size <= m_bufferPos);
  #endif
    uint8_t* out = m_buffer + m_cursorPos;
    m_cursorPos += size;
    return out;
  }
};
