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
struct audio_slice
{
  // TODO: Determine any other appropriate information to pass along with the slice
  struct _tag_header{
    unsigned __int64 id;
    unsigned __int64 timestamp;
    size_t data_len;
  } header;
  unsigned int data[1]; // Placeholder (TODO: This is hackish)

  unsigned char* get_data() {return (unsigned char*)&data;}
};

class CSliceAllocator
{
public:
  CSliceAllocator() : m_NextSliceId(0), m_LastFreedId(0) {}
  virtual ~CSliceAllocator() {};
  audio_slice* NewSlice(size_t len)
  {
    audio_slice* p = (audio_slice*)(new BYTE[sizeof(audio_slice::_tag_header) + len]);
    p->header.data_len = len;
    p->header.id = ++m_NextSliceId;
    return p;
  }
  void FreeSlice(audio_slice* pSlice)
  {
    if (pSlice)
    {
      if (pSlice->header.id != m_LastFreedId + 1)
        CLog::Log(LOGDEBUG,"Freed slices out of order");
      m_LastFreedId = pSlice->header.id;
      delete[] pSlice->get_data();
      //CLog::Log(LOGDEBUG,"Freeing slice %u. %u bytes", (Uint32)pSlice->header.id, pSlice->header.data_len);    
    }
  }
protected:
  __int64 m_NextSliceId;
  __int64 m_LastFreedId;
};

class CSliceQueue
{
public:
  CSliceQueue();
  virtual ~CSliceQueue();
  void Push(audio_slice* pSlice);
  audio_slice* GetSlice(size_t align, size_t maxSize);
  size_t AddData(void* pBuf, size_t bufLen);
  size_t GetData(void* pBuf, size_t bufLen);
  size_t GetTotalBytes() {return m_TotalBytes + m_RemainderSize;}
  void Clear();
protected:
  audio_slice* Pop(); // Does not respect remainder, so it must be private
  CSliceAllocator m_Allocator;
  std::queue<audio_slice*> m_Slices;
  size_t m_TotalBytes;
  audio_slice* m_pPartialSlice;
  size_t m_RemainderSize;
};

#endif //__SLICE_QUEUE_H__