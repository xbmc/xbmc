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
#pragma once

#include "threads/CriticalSection.h"
#include "threads/SingleLock.h"

#include <assert.h>
#include <deque>
#include <stdint.h>
#include <vector>

/**
 * Audio and video packets used by RetroPlayer's buffers are composed of packet
 * data and (optionally) additional metadata about the packet (such as video
 * frame dimensions). To allow both metadata type templating and object
 * inheritance, we use the type erasure pattern (similar to boost::any) to
 * store the metadata polymorphically in a common base class. For a good
 * overview, see http://www.artima.com/cppsource/type_erasure.html.
 */
class CRetroPlayerPacketBase
{
public:
  CRetroPlayerPacketBase() { }
  CRetroPlayerPacketBase(const uint8_t *data, unsigned int size) { Assign(data, size); }
  virtual ~CRetroPlayerPacketBase() { }

  // Assign() lets us re-use buffer for new data and avoid allocation for every new packet
  void Assign(const uint8_t *data, unsigned int size);
  void Clear() { buffer.clear(); }
  bool IsEmpty() const { return buffer.empty(); }

  std::vector<uint8_t> buffer;
};

/**
 * The templated metadata container.
 */
template <typename MetaType>
class CRetroPlayerPacket : public CRetroPlayerPacketBase
{
public:
  CRetroPlayerPacket() { }
  CRetroPlayerPacket(const uint8_t *data, unsigned int size, const MetaType &meta) { Assign(data, size, meta); }
  virtual ~CRetroPlayerPacket() { }

  void Assign(const uint8_t *data, unsigned int size, const MetaType &meta);

  MetaType             meta;
};

template <typename MetaType>
void CRetroPlayerPacket<MetaType>::Assign(const uint8_t *data, unsigned int size, const MetaType &meta)
{
  CRetroPlayerPacketBase::Assign(data, size);
  this->meta = meta;
}

/**
 * Audio and video buffers in RetroPlayer are out-of-order paged ring buffers,
 * where each page holds a single AV packet. A separate queue structure is used
 * to track the order of the packets, allowing for single-copy and avoiding the
 * need to constantly allocate small blocks of memory for each packet. This
 * strategy is well-suited for RetroPlayer, as game clients generally deliver
 * identically-sized packets at a relatively uniform rate. All public methods
 * are thread-safe, which removes the need for syncronization strategies in
 * RetroPlayerAudio and RetroPlayerVideo.
 *
 * Subclasses provide IsFull() to indicate that incoming packets should
 * overwrite existing data, allowing them to determine the size of the buffer
 * (e.g. by packet count or buffer size). 
 */
class CRetroPlayerBuffer
{
public:
  CRetroPlayerBuffer();
  virtual ~CRetroPlayerBuffer();

  /**
   * Returns a pointer to the next packet, or NULL if there is no new data
   * availabe. The returned packet is "acquired", and will no longer be used to
   * calculate GetSize() or GetCount(). The pointer is valid until the next
   * call to GetPacket(), and doesn't need to be freed.
   * @param pPacket (out) - pointer that gets assigned the packet
   */
  template <typename PacketType>
  void GetPacket(PacketType *&pPacket);

  /**
   * Add a packet of data to the buffer. If the buffer is full, older data is
   * discarded until the buffer is no longer full.
   */
  template <typename MetaType>
  void AddPacket(const uint8_t *data, unsigned int size, const MetaType &meta);

protected:
  /**
   * Return true if the buffer is full (excluding acquired packet). Function
   * body provided for linking purposes, must be overridden by subclass.
   */
  virtual bool IsFull() const { return false; }

  /**
   * Returns the summed size of the packets' buffers (excluding the acquired
   * packet).
   */
  unsigned int GetSize() const;

  /**
   * Returns the number of packets being tracked by the buffer (excluding the
   * acquired packet).
   */
  unsigned int GetCount() const;

private:
  /**
   * Remove old packets until IsFull() no longer returns true.
   */
  void Purge();

  /**
   * Get the first empty packet in m_packets. m_packets is extended if
   * necessary.
   * @param pPacket (out) - pointer to the first empty packet, or NULL if
   *        extending m_packet fails.
   * @param index (out) - index of pPacket, only valid if pPacket is non-NULL
   */
  template <typename PacketType>
  void GetEmptyPacket(PacketType *&pPacket, unsigned int &index);

  // Must store pointers, as a returned reference could be invalidated by increasing the vector size.
  std::vector<CRetroPlayerPacketBase*> m_packets;
  std::deque<unsigned int>  m_indices;
  int                       m_acquiredIndex; // The index of the packet currently "checked out" by GetPacket()
  CCriticalSection          m_critSection;
};

template <typename PacketType>
void CRetroPlayerBuffer::GetPacket(PacketType *&pPacket)
{
  CSingleLock lock(m_critSection);

  // Clear any packets previously "checked out"
  if (m_acquiredIndex >= 0)
  {
    assert((unsigned int)m_acquiredIndex < m_packets.size());
    m_packets[m_acquiredIndex]->Clear();
    m_acquiredIndex = -1;
  }

  // Return the first packet in the queue
  if (!m_indices.empty())
  {
    m_acquiredIndex = m_indices.front();
    m_indices.pop_front();
    // OK if dynamic_cast returns NULL (shouldn't happen)
    pPacket = dynamic_cast<PacketType*>(m_packets[m_acquiredIndex]);
  }
  else
  {
    pPacket = NULL;
  }
}

template <typename MetaType>
void CRetroPlayerBuffer::AddPacket(const uint8_t *data, unsigned int size, const MetaType &meta)
{
  if (!data || !size)
    return;

  CSingleLock lock(m_critSection);

  Purge();

  CRetroPlayerPacket<MetaType> *pPacket;
  unsigned int index;
  GetEmptyPacket(pPacket, index);
  if (pPacket)
  {
    pPacket->Assign(data, size, meta);
    m_indices.push_back(index);
  }
}

template <typename PacketType>
void CRetroPlayerBuffer::GetEmptyPacket(PacketType *&pPacket, unsigned int &index)
{
  // Look for an empty packet
  unsigned int i = 0;
  for (std::vector<CRetroPlayerPacketBase*>::iterator it = m_packets.begin(); it != m_packets.end(); ++it, i++)
  {
    // Skip the acquired packet
    if ((int)i == m_acquiredIndex)
      continue;

    PacketType *packet = dynamic_cast<PacketType*>(*it);
    if (packet && packet->IsEmpty())
    {
      pPacket = packet;
      index = i;
      return;
    }
  }

  // No empty packets found, allocate a new one
  pPacket = new PacketType;
  if (pPacket)
  {
    m_packets.push_back(pPacket);
    index = m_packets.size() - 1;
  }
}
