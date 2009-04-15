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
  // TODO: Determine any other appropriate information to pass along with the slice
  struct _tag_header{
    unsigned __int64 id;
    unsigned __int64 timestamp;
    size_t data_len;
  } header;
  unsigned int data[1]; // Placeholder (TODO: This is hackish)

  unsigned char* get_data() {return (unsigned char*)&data;}
};

class CAtomicAllocator
{
public:
  CAtomicAllocator(size_t blockSize) :
    m_BlockSize(blockSize)
  {
    lf_heap_init(&m_Heap, blockSize);
  }
  
  ~CAtomicAllocator()
  {
    lf_heap_deinit(&m_Heap);
  }
  
  void* Alloc()
  {
    return lf_heap_alloc(&m_Heap);
  }
  
  void Free(void* p)
  {
    lf_heap_free(&m_Heap, p);  
  }
  
  size_t GetBlockSize() {return m_BlockSize;}
private:
  lf_heap m_Heap ;
  size_t m_BlockSize;
};

class IStreamFormatConverter
{
public:
  virtual size_t Convert(unsigned char* pIn, size_t inLen, unsigned char* pOut, size_t outLen)  = 0; // Returns bytes in output 
  virtual float GetOutputFactor() = 0; // Returns bytes output per bytes input 
};

class CSliceQueue
{
public:
  CSliceQueue(size_t sliceSize);
  virtual ~CSliceQueue();
  size_t AddData(void* pBuf, size_t bufLen);
  size_t GetData(void* pBuf, size_t bufLen);
  size_t GetTotalBytes() {return m_TotalBytes + m_RemainderSize;}
  void Clear();
  void InstallConverter(int location, IStreamFormatConverter* pConverter);
protected:
  void Push(audio_slice* pSlice);
  audio_slice* Pop(); // Does not respect remainder, so it must be private
  CAtomicAllocator* m_pAllocator;
  lf_queue m_Queue;
  size_t m_TotalBytes;
  audio_slice* m_pPartialSlice;
  size_t m_RemainderSize;
  IStreamFormatConverter* m_pInputFormatConverter;
  IStreamFormatConverter* m_pOutputFormatConverter;
};

#endif //__SLICE_QUEUE_H__