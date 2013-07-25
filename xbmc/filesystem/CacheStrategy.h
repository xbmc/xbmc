/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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


#ifndef XFILECACHESTRATEGY_H
#define XFILECACHESTRATEGY_H

#include <stdint.h>
#ifdef TARGET_POSIX
#include "PlatformDefs.h"
#include "XHandlePublic.h"
#include "XFileUtils.h"
#endif
#include "threads/CriticalSection.h"
#include "threads/Event.h"

namespace XFILE {

#define CACHE_RC_OK  0
#define CACHE_RC_ERROR -1
#define CACHE_RC_WOULD_BLOCK -2
#define CACHE_RC_TIMEOUT -3

class CCacheStrategy{
public:
  CCacheStrategy();
  virtual ~CCacheStrategy();

  virtual int Open() = 0;
  virtual void Close() = 0;

  virtual int WriteToCache(const char *pBuffer, size_t iSize) = 0;
  virtual int ReadFromCache(char *pBuffer, size_t iMaxSize) = 0;
  virtual int64_t WaitForData(unsigned int iMinAvail, unsigned int iMillis) = 0;

  virtual int64_t Seek(int64_t iFilePosition) = 0;
  virtual void Reset(int64_t iSourcePosition, bool clearAnyway=true) = 0;

  virtual void EndOfInput(); // mark the end of the input stream so that Read will know when to return EOF
  virtual bool IsEndOfInput();
  virtual void ClearEndOfInput();

  virtual int64_t CachedDataEndPosIfSeekTo(int64_t iFilePosition) = 0;
  virtual int64_t CachedDataEndPos() = 0;
  virtual bool IsCachedPosition(int64_t iFilePosition) = 0;

  virtual CCacheStrategy *CreateNew() = 0;

  CEvent m_space;
protected:
  bool  m_bEndOfInput;
};

/**
*/
class CSimpleFileCache : public CCacheStrategy {
public:
  CSimpleFileCache();
  virtual ~CSimpleFileCache();

  virtual int Open() ;
  virtual void Close() ;

  virtual int WriteToCache(const char *pBuffer, size_t iSize) ;
  virtual int ReadFromCache(char *pBuffer, size_t iMaxSize) ;
  virtual int64_t WaitForData(unsigned int iMinAvail, unsigned int iMillis) ;

  virtual int64_t Seek(int64_t iFilePosition);
  virtual void Reset(int64_t iSourcePosition, bool clearAnyway=true);
  virtual void EndOfInput();

  virtual int64_t CachedDataEndPosIfSeekTo(int64_t iFilePosition);
  virtual int64_t CachedDataEndPos();
  virtual bool IsCachedPosition(int64_t iFilePosition);

  virtual CCacheStrategy *CreateNew();

  int64_t  GetAvailableRead();

protected:
  HANDLE  m_hCacheFileRead;
  HANDLE  m_hCacheFileWrite;
  CEvent*  m_hDataAvailEvent;
  volatile int64_t m_nStartPosition;
  volatile int64_t m_nWritePosition;
  volatile int64_t m_nReadPosition;
};

class CSimpleDoubleCache : public CCacheStrategy{
public:
  CSimpleDoubleCache(CCacheStrategy *impl);
  virtual ~CSimpleDoubleCache();

  virtual int Open() ;
  virtual void Close() ;

  virtual int WriteToCache(const char *pBuffer, size_t iSize) ;
  virtual int ReadFromCache(char *pBuffer, size_t iMaxSize) ;
  virtual int64_t WaitForData(unsigned int iMinAvail, unsigned int iMillis) ;

  virtual int64_t Seek(int64_t iFilePosition);
  virtual void Reset(int64_t iSourcePosition, bool clearAnyway=true);
  virtual void EndOfInput();
  virtual bool IsEndOfInput();
  virtual void ClearEndOfInput();

  virtual int64_t CachedDataEndPosIfSeekTo(int64_t iFilePosition);
  virtual int64_t CachedDataEndPos();
  virtual bool IsCachedPosition(int64_t iFilePosition);

  virtual CCacheStrategy *CreateNew();

protected:
  CCacheStrategy *m_pCache;
  CCacheStrategy *m_pCacheOld;
};

}

#endif
