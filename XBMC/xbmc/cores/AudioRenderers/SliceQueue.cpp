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

#include "stdafx.h"
#include "SliceQueue.h"

/////////////////////////////////////////////////////////////////////////////////
// CAtomicAllocator: Wrapper class for lf_heap.
////////////////////////////////////////////////////////////////////////////////
CAtomicAllocator::CAtomicAllocator(size_t blockSize) :
  m_BlockSize(blockSize)
{
  lf_heap_init(&m_Heap, blockSize);
}

CAtomicAllocator::~CAtomicAllocator()
{
  lf_heap_deinit(&m_Heap);
}

void* CAtomicAllocator::Alloc()
{
  return lf_heap_alloc(&m_Heap);
}

void CAtomicAllocator::Free(void* p)
{
  lf_heap_free(&m_Heap, p);  
}

size_t CAtomicAllocator::GetBlockSize() 
{
  return m_BlockSize;
}

//////////////////////////////////////////////////////////////////////////////////////
// CSliceQueue
//////////////////////////////////////////////////////////////////////////////////////
CSliceQueue::CSliceQueue(size_t sliceSize) :
  m_TotalBytes(0),
  m_pPartialSlice(NULL),
  m_RemainderSize(0)
{
  m_pAllocator = new CAtomicAllocator(sliceSize + offsetof(audio_slice, data));
  lf_queue_init(&m_Queue);
}

CSliceQueue::~CSliceQueue()
{
  Clear();
  lf_queue_deinit(&m_Queue);
  delete m_pAllocator;
}

void CSliceQueue::Push(audio_slice* pSlice) 
{
  if (pSlice)
  {
    lf_queue_enqueue(&m_Queue, pSlice);
    m_TotalBytes += pSlice->header.data_len;
  }
}

audio_slice* CSliceQueue::Pop()
{
  audio_slice* pSlice = (audio_slice*)lf_queue_dequeue(&m_Queue);
  if (pSlice)
    m_TotalBytes -= pSlice->header.data_len;
  return pSlice;
}

// TODO: Should we add monitoring for the fill-level?
size_t CSliceQueue::AddData(void* pBuf, size_t bufLen)
{
  size_t bytesLeft = bufLen;
  unsigned char* pData = (unsigned char*)pBuf;
  if (pBuf && bufLen)
  {
    while (bytesLeft)
    {
      // TODO: find a way to ensure success...
      audio_slice* pSlice = (audio_slice*)m_pAllocator->Alloc(); // Allocation should never fail. The heap grows automatically.
      if (!pSlice)
      {
        CLog::Log(LOGDEBUG, "CSliceQueue::AddData: Failed to allocate new slice");
        return 0;
      }
      pSlice->header.data_len = m_pAllocator->GetBlockSize() - offsetof(audio_slice, data);
      if (pSlice->header.data_len >= bytesLeft) // plenty of room. move it all in.
      {
        memcpy(pSlice->get_data(), pData, bytesLeft);
        pSlice->header.data_len = bytesLeft; // Adjust the reported size of the container
      }
      else
      {
        memcpy(pSlice->get_data(), pData, pSlice->header.data_len); // Copy all we can into this slice. More to come.
      }
      bytesLeft -= pSlice->header.data_len;
      pData += pSlice->header.data_len;
      Push(pSlice);
    }
  }
  return  bufLen - bytesLeft;
}

size_t CSliceQueue::GetData(void* pBuf, size_t bufLen)
{
  if (GetTotalBytes() < bufLen || !pBuf || !bufLen)
    return NULL;

  size_t bytesUsed = 0;
  size_t remainder = 0;
  audio_slice* pNext = NULL;
  
  // See if we can fill the request out of our partial slice (if there is one)
  if (m_RemainderSize >= bufLen)
  {
    memcpy(pBuf, m_pPartialSlice->get_data() + m_pPartialSlice->header.data_len - m_RemainderSize , bufLen);
    m_RemainderSize -= bufLen;
  }
  else // Pull what we can from the partial slice and get the rest from complete slices
  {
    // Take what we can from the partial slice (if there is one)
    if (m_RemainderSize)
    {
      memcpy(pBuf, m_pPartialSlice->get_data() + m_pPartialSlice->header.data_len - m_RemainderSize , m_RemainderSize);
      bytesUsed += m_RemainderSize;
      m_RemainderSize = 0;
    }

    // Pull slices from the fifo until we have enough data
    do // TODO: The efficiency of this loop can be improved (a lot I imagine)
    {
      pNext = Pop();
      size_t nextLen = pNext->header.data_len;
      if (bytesUsed + nextLen > bufLen) // Check for a partial slice
        remainder = nextLen - (bufLen - bytesUsed);
      memcpy((BYTE*)pBuf + bytesUsed, pNext->get_data(), nextLen - remainder);
      bytesUsed += nextLen; // Increment output size (remainder will be captured separately)
      if (!remainder)
        m_pAllocator->Free(pNext); // Free the copied slice
    } while (bytesUsed < bufLen);    
  }

  // Clean up the previous partial slice
  if (!m_RemainderSize && m_pPartialSlice)
  {
    m_pAllocator->Free(m_pPartialSlice);
    m_pPartialSlice = NULL;
  }

  // Save off the new partial slice (if there is one)
  if (remainder)
  {
    m_pPartialSlice = pNext;
    m_RemainderSize = remainder;
  }

  return bufLen;
}

size_t CSliceQueue::GetTotalBytes() 
{
  return m_TotalBytes + m_RemainderSize;
}

void CSliceQueue::Clear()
{
  while (audio_slice* pSlice = Pop())
     m_pAllocator->Free(pSlice);
  m_pAllocator->Free(m_pPartialSlice);
  m_pPartialSlice = NULL;
  m_RemainderSize = 0;
}
