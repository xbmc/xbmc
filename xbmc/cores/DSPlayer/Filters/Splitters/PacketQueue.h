/*
*      Copyright (C) 2010 Team XBMC
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

#pragma once
#include "BaseDemuxer.h"

#include <deque>

#define MIN_PACKETS_IN_QUEUE 10           // Below this is considered "drying pin"
#define MAX_PACKETS_IN_QUEUE 100

class Packet;

// FIFO Packet Queue
class CPacketQueue : public CCritSec
{
public:
  // Queue a new packet at the end of the list
  void Queue(Packet *pPacket, BOOL tryAppend = TRUE);

  // Get a packet from the beginning of the list
  Packet *Get();

  // Get the size of the queue
  size_t Size();

  // Clear the List (all elements are free'ed)
  void Clear();

  // Get access to the internal queue
  std::deque<Packet *> *GetQueue()
  {
    return &m_queue;
  }

  bool IsEmpty()
  {
    return Size() == 0;
  }
private:
  // The actual storage class
  std::deque<Packet *> m_queue;
};
