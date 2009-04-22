#include <queue>
/*
 *      Copyright (C) 2005-2009 Team XBMC
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

#ifndef __SLICE_QUEUE_H__
#define __SLICE_QUEUE_H__

#include <utils/LockFree.h>

struct audio_slice
{
  struct _tag_header{
    unsigned __int64 timestamp; // Currently not used
    size_t data_len;
  } header;
  unsigned int data[1];
  unsigned char* get_data() {return (unsigned char*)&data;}
};

class CAtomicAllocator
{
public:
  CAtomicAllocator(size_t blockSize);
  ~CAtomicAllocator();
  void* Alloc();
  void Free(void* p);
  size_t GetBlockSize();
private:
  lf_heap m_Heap ;
  size_t m_BlockSize;
};

class CSliceQueue
{
public:
  CSliceQueue(size_t sliceSize);
  virtual ~CSliceQueue();
  size_t AddData(void* pBuf, size_t bufLen);
  size_t GetData(void* pBuf, size_t bufLen);
  size_t GetTotalBytes();
  void Clear();
protected:
  void Push(audio_slice* pSlice);
  audio_slice* Pop(); // Does not respect remainder, so it must be private
  CAtomicAllocator* m_pAllocator;
  lf_queue m_Queue;
  size_t m_TotalBytes;
  audio_slice* m_pPartialSlice;
  size_t m_RemainderSize;
};

#endif //__SLICE_QUEUE_H__