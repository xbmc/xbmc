//
// C++ Interface: CacheStrategy
//
// Description: 
//
//
// Author: Team XBMC <>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef XFILECACHESTRATEGY_H
#define XFILECACHESTRATEGY_H

#ifdef _LINUX
#include "PlatformDefs.h"
#include "XHandle.h"
#include "XFileUtils.h"
#endif
#include "../utils/CriticalSection.h"

namespace XFILE {

#define CACHE_RC_OK  0
#define CACHE_RC_EOF 0
#define CACHE_RC_ERROR -1
#define CACHE_RC_WOULD_BLOCK -2
#define CACHE_RC_TIMEOUT -3

/**
*/
class CCacheStrategy{
public:
  CCacheStrategy();
  virtual ~CCacheStrategy();

	virtual int Open() = 0;
	virtual int Close() = 0;

	virtual int WriteToCache(const char *pBuffer, size_t iSize) = 0;
	virtual int ReadFromCache(char *pBuffer, size_t iMaxSize) = 0;
	virtual __int64 WaitForData(unsigned int iMinAvail, unsigned int iMillis) = 0;

  virtual __int64 Seek(__int64 iFilePosition, int iWhence) = 0;
	virtual void Reset(__int64 iSourcePosition) = 0;

	virtual void EndOfInput(); // mark the end of the input stream so that Read will know when to return EOF
	virtual bool IsEndOfInput();
  virtual void ClearEndOfInput();

protected:
	bool	m_bEndOfInput;
};

/**
*/
class CSimpleFileCache : public CCacheStrategy {
public:
    CSimpleFileCache();
    virtual ~CSimpleFileCache();

	virtual int Open() ;
	virtual int Close() ;

	virtual int WriteToCache(const char *pBuffer, size_t iSize) ;
	virtual int ReadFromCache(char *pBuffer, size_t iMaxSize) ;
	virtual __int64 WaitForData(unsigned int iMinAvail, unsigned int iMillis) ;

  virtual __int64 Seek(__int64 iFilePosition, int iWhence);
	virtual void Reset(__int64 iSourcePosition);
	virtual void EndOfInput();

	__int64	GetAvailableRead();

protected:
	HANDLE	m_hCacheFileRead;
	HANDLE	m_hCacheFileWrite;
	HANDLE	m_hDataAvailEvent;
    __int64 m_nStartPosition;
	CStdString m_fileName;
	CCriticalSection m_sync;
};

}

#endif
