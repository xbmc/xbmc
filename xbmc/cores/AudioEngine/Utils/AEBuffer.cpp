/*
 *      Copyright (C) 2010-2013 Team XBMC
 *      http://xbmc.org
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

#include "AEBuffer.h"
#include <algorithm>

CAEBuffer::CAEBuffer() :
  m_buffer    (NULL),
  m_bufferSize(0   ),
  m_bufferPos (0   ),
  m_cursorPos (0   )
{
}

CAEBuffer::~CAEBuffer()
{
  DeAlloc();
}

void CAEBuffer::Alloc(const size_t size)
{
  DeAlloc();
  m_buffer     = (uint8_t*)_aligned_malloc(size, 16);
  m_bufferSize = size;
  m_bufferPos  = 0;
}

void CAEBuffer::ReAlloc(const size_t size)
{
#if defined(TARGET_WINDOWS)
  m_buffer = (uint8_t*)_aligned_realloc(m_buffer, size, 16);
#else
  uint8_t* tmp = (uint8_t*)_aligned_malloc(size, 16);
  if (m_buffer)
  {
    size_t copy = std::min(size, m_bufferSize);
    memcpy(tmp, m_buffer, copy);
    _aligned_free(m_buffer);
  }
  m_buffer = tmp;
#endif

  m_bufferSize = size;
  m_bufferPos  = std::min(m_bufferPos, m_bufferSize);
}

void CAEBuffer::DeAlloc()
{
  if (m_buffer)
    _aligned_free(m_buffer);
  m_buffer     = NULL;
  m_bufferSize = 0;
  m_bufferPos  = 0;
}
