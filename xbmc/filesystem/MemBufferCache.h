/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#ifndef MemBufferCache_H
#define MemBufferCache_H

#include "CacheStrategy.h"
#include "threads/CriticalSection.h"
#include "threads/Event.h"
#include "utils/RingBuffer.h"

/**
  @author Team XBMC
*/
namespace XFILE {

class MemBufferCache : public CCacheStrategy
{
public:
    MemBufferCache();
    virtual ~MemBufferCache();

    virtual int Open() ;
    virtual void Close();

    virtual int WriteToCache(const char *pBuffer, size_t iSize) ;
    virtual int ReadFromCache(char *pBuffer, size_t iMaxSize) ;
    virtual int64_t WaitForData(unsigned int iMinAvail, unsigned int iMillis) ;

    virtual int64_t Seek(int64_t iFilePosition) ;
    virtual void Reset(int64_t iSourcePosition) ;

protected:
    int64_t m_nStartPosition;
    CRingBuffer m_buffer;
    CRingBuffer m_HistoryBuffer;
    CRingBuffer m_forwardBuffer; // for seek cases, to store data already read
    CCriticalSection m_sync;
    CEvent m_written;
};

} // namespace XFILE
#endif
