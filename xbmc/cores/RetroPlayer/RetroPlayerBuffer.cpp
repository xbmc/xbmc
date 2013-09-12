/*
 *      Copyright (C) 2013 Team XBMC
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

#include "RetroPlayerBuffer.h"

void CRetroPlayerPacketBase::Assign(const uint8_t *data, unsigned int size)
{
  buffer.clear();
  buffer.insert(buffer.begin(), data, data + size);
}

CRetroPlayerBuffer::CRetroPlayerBuffer() : m_acquiredIndex(-1)
{
  // Verify the subclass doesn't report full for an empty buffer (prevents infinite loop below)
  assert(!IsFull());
}

CRetroPlayerBuffer::~CRetroPlayerBuffer()
{
  for (std::vector<CRetroPlayerPacketBase*>::iterator it = m_packets.begin(); it != m_packets.end(); ++it)
    delete *it;
}

unsigned int CRetroPlayerBuffer::GetSize() const
{
  unsigned int size = 0;
  for (std::deque<unsigned int>::const_iterator it = m_indices.begin(); it != m_indices.end(); ++it)
  {
    unsigned int index = *it;
    assert(index < m_packets.size());
    size += m_packets[index]->buffer.size();
  }
  return size;
}

unsigned int CRetroPlayerBuffer::GetCount() const
{
  return m_indices.size();
}

void CRetroPlayerBuffer::Purge()
{
  // Purge old data
  while (IsFull())
  {
    unsigned int index = m_indices.front();
    m_packets[index]->Clear();
    m_indices.pop_front();
  }
}
