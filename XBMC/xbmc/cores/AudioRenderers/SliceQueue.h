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

#include <utils/Atomics.h>
#include "/usr/include/malloc/malloc.h"

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

extern malloc_zone_t* g_CoreAudioPrivateZone;
extern long g_CoreAudioAllocatorCount;

class CSliceAllocator
{
public:
  CSliceAllocator() : 
    m_NextSliceId(0), 
    m_LastFreedId(0),
    m_Lock(0)
  {
    if (AtomicIncrement32(&g_CoreAudioAllocatorCount) == 1) // We're first, create the memory allocation zone
    {
      CLog::Log(LOGDEBUG, "CoreAudio: Creating private malloc zone");
      g_CoreAudioPrivateZone = malloc_create_zone(512 * 1024, 0);
      if (g_CoreAudioPrivateZone)
        malloc_set_zone_name(g_CoreAudioPrivateZone, "CoreAudioRendererZone");
    }
  }
  
  virtual ~CSliceAllocator()
  {
      if (!AtomicDecrement32(&g_CoreAudioAllocatorCount)) // We're the last, Tear down the memory allocation zone
      {
        CLog::Log(LOGDEBUG, "CoreAudio: Destroying private malloc zone");
        if (g_CoreAudioPrivateZone)
        {
          malloc_destroy_zone(g_CoreAudioPrivateZone);
          g_CoreAudioPrivateZone = 0;
        }
      }
  }
  
  audio_slice* NewSlice(size_t len)
  {
    unsigned int time = timeGetTime();
    audio_slice* p = (audio_slice*)malloc_zone_malloc(g_CoreAudioPrivateZone, sizeof(audio_slice::_tag_header) + len);
    unsigned int deltaTime = timeGetTime() - time;
    if (deltaTime > 1)
      CLog::Log(LOGDEBUG, "CoreAudio: Malloc delay = %u", deltaTime);
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
      unsigned int time = timeGetTime();
      malloc_zone_free(g_CoreAudioPrivateZone, pSlice);
      unsigned int deltaTime = timeGetTime() - time;
      if (deltaTime > 1)
        CLog::Log(LOGDEBUG, "CoreAudio: Free delay = %u", deltaTime);
      //CLog::Log(LOGDEBUG,"Freeing slice %u. %u bytes", (Uint32)pSlice->header.id, pSlice->header.data_len);    
    }
  }
  
protected:
  __int64 m_NextSliceId;
  __int64 m_LastFreedId;
  long m_Lock;
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
  CSliceAllocator* m_pAllocator;
  std::queue<audio_slice*> m_Slices;
  size_t m_TotalBytes;
  audio_slice* m_pPartialSlice;
  size_t m_RemainderSize;
  long m_QueueLock;
};

#endif //__SLICE_QUEUE_H__