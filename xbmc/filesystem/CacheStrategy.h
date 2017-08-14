/*
 *      Copyright (C) 2005-2014 Team XBMC
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
#include <string>
#include "threads/Event.h"

namespace XFILE {

#define CACHE_RC_OK  0
#define CACHE_RC_ERROR -1
#define CACHE_RC_WOULD_BLOCK -2
#define CACHE_RC_TIMEOUT -3

class IFile; // forward declaration

class CCacheStrategy{
public:
  CCacheStrategy();
  virtual ~CCacheStrategy();

  virtual int Open() = 0;
  virtual void Close() = 0;

  virtual size_t GetMaxWriteSize(const size_t& iRequestSize) = 0;
  virtual int WriteToCache(const char *pBuffer, size_t iSize) = 0;
  virtual int ReadFromCache(char *pBuffer, size_t iMaxSize) = 0;
  virtual int64_t WaitForData(unsigned int iMinAvail, unsigned int iMillis) = 0;

  virtual int64_t Seek(int64_t iFilePosition) = 0;

  /*!
   \brief Reset cache position
   \param iSourcePosition position to reset to
   \param clearAnyway whether to perform a full reset regardless of in cached range already
   \return Whether a full reset was performed, or not (e.g. only cache swap)
   \sa CCacheStrategy
   */
  virtual bool Reset(int64_t iSourcePosition, bool clearAnyway=true) = 0;

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
  ~CSimpleFileCache() override;

  int Open() override;
  void Close() override;

  size_t GetMaxWriteSize(const size_t& iRequestSize) override;
  int WriteToCache(const char *pBuffer, size_t iSize) override;
  int ReadFromCache(char *pBuffer, size_t iMaxSize) override;
  int64_t WaitForData(unsigned int iMinAvail, unsigned int iMillis) override;

  int64_t Seek(int64_t iFilePosition) override;
  bool Reset(int64_t iSourcePosition, bool clearAnyway=true) override;
  void EndOfInput() override;

  int64_t CachedDataEndPosIfSeekTo(int64_t iFilePosition) override;
  int64_t CachedDataEndPos() override;
  bool IsCachedPosition(int64_t iFilePosition) override;

  CCacheStrategy *CreateNew() override;

  int64_t  GetAvailableRead();

protected:
  std::string m_filename;
  IFile*   m_cacheFileRead;
  IFile*   m_cacheFileWrite;
  CEvent*  m_hDataAvailEvent;
  volatile int64_t m_nStartPosition;
  volatile int64_t m_nWritePosition;
  volatile int64_t m_nReadPosition;
};

class CDoubleCache : public CCacheStrategy{
public:
  explicit CDoubleCache(CCacheStrategy *impl);
  ~CDoubleCache() override;

  int Open() override;
  void Close() override;

  size_t GetMaxWriteSize(const size_t& iRequestSize) override;
  int WriteToCache(const char *pBuffer, size_t iSize) override;
  int ReadFromCache(char *pBuffer, size_t iMaxSize) override;
  int64_t WaitForData(unsigned int iMinAvail, unsigned int iMillis) override;

  int64_t Seek(int64_t iFilePosition) override;
  bool Reset(int64_t iSourcePosition, bool clearAnyway=true) override;
  void EndOfInput() override;
  bool IsEndOfInput() override;
  void ClearEndOfInput() override;

  int64_t CachedDataEndPosIfSeekTo(int64_t iFilePosition) override;
  int64_t CachedDataEndPos() override;
  bool IsCachedPosition(int64_t iFilePosition) override;

  CCacheStrategy *CreateNew() override;

protected:
  CCacheStrategy *m_pCache;
  CCacheStrategy *m_pCacheOld;
};

}

#endif
