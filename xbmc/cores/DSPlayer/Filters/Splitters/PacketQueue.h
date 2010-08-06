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
#include "streams.h"
#include <deque>
#include <limits.h>
#define MIN_PACKETS_IN_QUEUE 10           // Below this is considered "drying pin"
#define MAX_PACKETS_IN_QUEUE 100
#define INVALID_PACKET_TIME  (_I64_MIN)

// Data Packet for queue storage
class Packet
{
public:

  DWORD StreamId;
  BOOL bDiscontinuity, bSyncPoint, bAppendable;
  REFERENCE_TIME rtStart, rtStop;
  AM_MEDIA_TYPE* pmt;

  Packet() { pmt = NULL; m_pbData = NULL; bDiscontinuity = bSyncPoint = bAppendable = FALSE; rtStart = rtStop = INVALID_PACKET_TIME; m_dSize = 0; }
  virtual ~Packet() { if(pmt) DeleteMediaType(pmt); if(m_pbData) free(m_pbData); }

  // Getter
  DWORD GetDataSize() { return m_dSize; }
  BYTE *GetData() { return m_pbData; }

  // Setter
  void SetDataSize(DWORD len) { m_dSize = len; m_pbData = (BYTE *)realloc(m_pbData, len); }
  void SetData(const void* ptr, DWORD len) { SetDataSize(len); memcpy(GetData(), ptr, len); }

private:
  DWORD m_dSize;
  BYTE *m_pbData;
};

// FIFO Packet Queue
class CPacketQueue : public CCritSec
{
public:
  // Queue a new packet at the end of the list
  void Queue(Packet *pPacket);

  // Get a packet from the beginning of the list
  Packet *Get();

  // Get the size of the queue
  int Size();

  // Clear the List (all elements are free'ed)
  void Clear();
private:
  // The actual storage class
  std::deque<Packet *> m_queue;
};
