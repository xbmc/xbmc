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

malloc_zone_t* g_CoreAudioPrivateZone = NULL;
long g_CoreAudioAllocatorCount = 0;

// TODO: 

// CSliceQueue
//////////////////////////////////////////////////////////////////////////////////////
CSliceQueue::CSliceQueue() :
  m_TotalBytes(0),
  m_pPartialSlice(NULL),
  m_RemainderSize(0),
  m_QueueLock(0)
{
  m_pAllocator = new CSliceAllocator();
}

CSliceQueue::~CSliceQueue()
{
  Clear();
  delete m_pAllocator;
}

void CSliceQueue::Push(audio_slice* pSlice) 
{
  if (pSlice)
  {
    CAtomicSpinLock lock(m_QueueLock);
    m_Slices.push(pSlice);
    m_TotalBytes += pSlice->header.data_len;
  }
}

audio_slice* CSliceQueue::Pop()
{
  audio_slice* pSlice = NULL;
  if (!m_Slices.empty())
  {
    pSlice = m_Slices.front();
    CAtomicSpinLock lock(m_QueueLock);
    m_Slices.pop();
    m_TotalBytes -= pSlice->header.data_len;
  }
  return pSlice;
}

audio_slice* CSliceQueue::GetSlice(size_t alignSize, size_t maxSize)
{
  if (GetTotalBytes() < alignSize) // Need to have enough data to give
    return NULL;
  
  size_t bytesUsed = 0;
  size_t remainder = 0;
  audio_slice* pNext = NULL;
  size_t sliceSize = std::min<size_t>(GetTotalBytes(),maxSize); // Set the size of the slice to be created, limit it to maxSize
  
  // If we have no remainder and the first whole slice can satisfy the requirements, pass it along as-is.
  // This reduces allocs and writes
  if (!m_RemainderSize) 
  {
    size_t headLen = m_Slices.front()->header.data_len;
    if (!(headLen % alignSize) && headLen <= maxSize)
    {
      pNext = Pop();
      return pNext;
    }
  }
  
  sliceSize -= sliceSize % alignSize; // Make sure we are properly aligned.
  audio_slice* pOutSlice = m_pAllocator->NewSlice(sliceSize); // Create a new slice
  
  // See if we can fill the new slice out of our partial slice (if there is one)
  if (m_RemainderSize >= sliceSize)
  {
    memcpy(pOutSlice->get_data(), m_pPartialSlice->get_data() + m_pPartialSlice->header.data_len - m_RemainderSize , sliceSize);
    m_RemainderSize -= sliceSize;
  }
  else // Pull what we can from the partial slice and get the rest from complete slices
  {
    // Take what we can from the partial slice (if there is one)
    if (m_RemainderSize)
    {
      memcpy(pOutSlice->get_data(), m_pPartialSlice->get_data() + m_pPartialSlice->header.data_len - m_RemainderSize , m_RemainderSize);
      bytesUsed += m_RemainderSize;
      m_RemainderSize = 0;
    }
    
    // Pull slices from the fifo until we have enough data
    do // TODO: The efficiency of this loop can be improved (a lot I imagine)
    {
      pNext = Pop();
      size_t nextLen = pNext->header.data_len;
      if (bytesUsed + nextLen > sliceSize) // Check for a partial slice
        remainder = nextLen - (sliceSize - bytesUsed);
      memcpy(pOutSlice->get_data() + bytesUsed, pNext->get_data(), nextLen - remainder);
      bytesUsed += nextLen; // Increment output size (remainder will be captured separately)
      if (!remainder)
        m_pAllocator->FreeSlice(pNext); // Free the copied slice
    } while (bytesUsed < sliceSize);
  }
  
  // Clean up the previous partial slice
  if (!m_RemainderSize)
  {
    delete m_pPartialSlice;
    m_pPartialSlice = NULL;
  }
  
  // Save off a new partial slice (if there is one)
  if (remainder)
  {
    m_pPartialSlice = pNext;
    m_RemainderSize = remainder;
  }
  
  return pOutSlice;  
}

size_t CSliceQueue::AddData(void* pBuf, size_t bufLen)
{
  if (pBuf && bufLen)
  {
  
    audio_slice* pSlice = m_pAllocator->NewSlice(bufLen);
    if (pSlice)
    {
      memcpy(pSlice->get_data(), pBuf, bufLen);
      Push(pSlice);
      return bufLen;
    }
  }
  return 0;
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
      size_t nextLen = pNext->header.data_len;
      if (bytesUsed + nextLen > bufLen) // Check for a partial slice
        remainder = nextLen - (bufLen - bytesUsed);
      memcpy((BYTE*)pBuf + bytesUsed, pNext->get_data(), nextLen - remainder);
      bytesUsed += nextLen; // Increment output size (remainder will be captured separately)
      if (!remainder)
        m_pAllocator->FreeSlice(pNext); // Free the copied slice
      unsigned int microDelta = timeGetTime() - microProfile;
      if (microDelta > 1)
        CLog::Log(LOGDEBUG, "CoreAudio: Microtime = %u.", microDelta);
      loopCount++;
    } while (bytesUsed < bufLen);    
    profileTime[3] = timeGetTime(); 
  }

  // Clean up the previous partial slice
  if (!m_RemainderSize && m_pPartialSlice)
  {
    m_pAllocator->FreeSlice(m_pPartialSlice);
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
  while (!m_Slices.empty())
     m_pAllocator->FreeSlice(Pop());

  m_pAllocator->FreeSlice(m_pPartialSlice);
  m_pPartialSlice = NULL;
  m_RemainderSize = 0;
}
