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

#define BLOCK_SIZE 2048

// TODO: BLOCK_SIZE should be dependent on caller
// CSliceQueue
//////////////////////////////////////////////////////////////////////////////////////
CSliceQueue::CSliceQueue(size_t sliceSize) :
  m_TotalBytes(0),
  m_pPartialSlice(NULL),
  m_RemainderSize(0)
{
  m_pAllocator = new CAtomicAllocator(sliceSize);
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

size_t CSliceQueue::AddData(void* pBuf, size_t bufLen)
{
  size_t bytesLeft = bufLen;
  unsigned char* pData = (unsigned char*)pBuf;
  if (pBuf && bufLen)
  {
    while (bytesLeft)
    {
      // TODO: How should we behave here? Can we grow the allocator?
      audio_slice* pSlice = (audio_slice*)m_pAllocator->Alloc();
      if (!pSlice)
        break;
      
      pSlice->header.data_len = BLOCK_SIZE;
      if (pSlice->header.data_len >= bytesLeft) // plenty of room. move it all out.
      {
        memcpy(pSlice->get_data(), pData, bytesLeft);
        pSlice->header.data_len = bytesLeft; // Adjust the reported size of the container
      }
      else
      {
        memcpy(pSlice->get_data(), pData, pSlice->header.data_len); // Copy all we can into this slice
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
  
  unsigned int profileTime[5] = {0};  
  unsigned int loopCount = 0;
  
  profileTime[0] = timeGetTime();
  // See if we can fill the request out of our partial slice (if there is one)
  if (m_RemainderSize >= bufLen)
  {
    memcpy(pBuf, m_pPartialSlice->get_data() + m_pPartialSlice->header.data_len - m_RemainderSize , bufLen);
    m_RemainderSize -= bufLen;
    
    profileTime[1] = profileTime[2] = profileTime[3] = timeGetTime();
  }
  else // Pull what we can from the partial slice and get the rest from complete slices
  {
    profileTime[1] = timeGetTime();
    // Take what we can from the partial slice (if there is one)
    if (m_RemainderSize)
    {
      memcpy(pBuf, m_pPartialSlice->get_data() + m_pPartialSlice->header.data_len - m_RemainderSize , m_RemainderSize);
      bytesUsed += m_RemainderSize;
      m_RemainderSize = 0;
    }

    profileTime[2] = timeGetTime();
    // Pull slices from the fifo until we have enough data
    do // TODO: The efficiency of this loop can be improved (a lot I imagine)
    {
      unsigned int microProfile = timeGetTime();
      pNext = Pop();
      unsigned int microDelta = timeGetTime() - microProfile;
      size_t nextLen = pNext->header.data_len;
      if (bytesUsed + nextLen > bufLen) // Check for a partial slice
        remainder = nextLen - (bufLen - bytesUsed);
      memcpy((BYTE*)pBuf + bytesUsed, pNext->get_data(), nextLen - remainder);
      bytesUsed += nextLen; // Increment output size (remainder will be captured separately)
      if (!remainder)
        m_pAllocator->Free(pNext); // Free the copied slice
      if (microDelta > 1)
        CLog::Log(LOGDEBUG, "CoreAudio: Microtime = %u.", microDelta);
      loopCount++;
    } while (bytesUsed < bufLen);    
    profileTime[3] = timeGetTime(); 
  }

  // Clean up the previous partial slice
  if (!m_RemainderSize && m_pPartialSlice)
  {
    m_pAllocator->Free(m_pPartialSlice);
    m_pPartialSlice = NULL;
  }

  // Save off a new partial slice (if there is one)
  if (remainder)
  {
    m_pPartialSlice = pNext;
    m_RemainderSize = remainder;
  }
  profileTime[4] = timeGetTime();

  unsigned int t[4];
  t[0] = profileTime[1] - profileTime[0];
  t[1] = profileTime[2] - profileTime[1];
  t[2] = profileTime[3] - profileTime[2];
  t[3] = profileTime[4] - profileTime[3];
  if (t[0] > 1 || t[1] > 1 || t[2] > 1 || t[3] > 1)
    CLog::Log(LOGDEBUG, "CoreAudio: Profile times = %u %u %u %u. Loop Count = %u", t[0], t[1], t[2], t[3], loopCount);
  return bufLen;
}

void CSliceQueue::Clear()
{
  while (audio_slice* pSlice = Pop())
     m_pAllocator->Free(pSlice);
  m_pAllocator->Free(m_pPartialSlice);
  m_pPartialSlice = NULL;
  m_RemainderSize = 0;
}
