//
// C++ Interface: CacheMemBuffer
//
// Description: 
//
//
// Author: Team XBMC <>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef CACHEMEMBUFFER_H
#define CACHEMEMBUFFER_H

#include "CacheStrategy.h"
#include "../utils/CriticalSection.h"
#include "RingBuffer.h"

/**
	@author Team XBMC
*/
namespace XFILE {

class CacheMemBuffer : public CCacheStrategy
{
public:
    CacheMemBuffer();
    virtual ~CacheMemBuffer();

    virtual int Open() ;
    virtual int Close();

    virtual int WriteToCache(const char *pBuffer, size_t iSize) ;
    virtual int ReadFromCache(char *pBuffer, size_t iMaxSize) ;
    virtual __int64 WaitForData(__int64 iMinAvail, unsigned int iMillis) ;

    virtual __int64 Seek(__int64 iFilePosition, int iWhence) ;
	virtual void Reset(__int64 iSourcePosition) ;

protected:
    __int64 m_nStartPosition;
    CRingBuffer m_buffer;
    CRingBuffer m_HistoryBuffer;
    CRingBuffer m_forwardBuffer; // for seek cases, to store data already read
    CCriticalSection m_sync;
};

} // namespace XFILE
#endif
