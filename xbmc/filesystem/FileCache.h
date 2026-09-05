/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "CacheStrategy.h"
#include "IFile.h"
#include "threads/CriticalSection.h"
#include "threads/Thread.h"

#include <atomic>
#include <chrono>
#include <memory>

using namespace std::chrono_literals;

namespace XFILE
{
/*!
 * \brief Source interface used by CFileCache for backing file access.
 *
 * Allows CFileCache to operate on sources other than CFile while preserving
 * normal file semantics. The source is owned by CFileCache, opened before the
 * cache worker starts, and closed after it stops.
 *
 * Implementations must return the resulting absolute position from Seek(), or
 * -1 on failure, and expose an underlying IFile from GetImplementation() when
 * one exists.
 */
class IFileCacheSource
{
public:
  virtual ~IFileCacheSource() = default;

  virtual bool Open(const CURL& url, unsigned int flags) = 0;
  virtual void Close() = 0;
  virtual ssize_t Read(void* buffer, size_t size) = 0;
  virtual int64_t Seek(int64_t position, int whence) = 0;
  virtual int64_t GetLength() = 0;
  virtual int GetChunkSize() = 0;
  virtual int IoControl(IOControl request, void* param) = 0;
  virtual IFile* GetImplementation() = 0;
};

  class CFileCache : public IFile, public CThread
  {
  public:
    explicit CFileCache(const unsigned int flags);
    ~CFileCache() override;

    // CThread methods
    void Process() override;
    void OnExit() override;
    void StopThread(bool bWait = true) override;

    // IFIle methods
    bool Open(const CURL& url) override;
    void Close() override;
    bool Exists(const CURL& url) override;
    int Stat(const CURL& url, struct __stat64* buffer) override;

    ssize_t Read(void* lpBuf, size_t uiBufSize) override;

    int64_t Seek(int64_t iFilePosition, int iWhence) override;
    int64_t GetPosition() override;
    int64_t GetLength() override;

    int IoControl(IOControl request, void* param) override;

    IFile *GetFileImp();

    const std::string GetProperty(XFILE::FileProperty type, const std::string &name = "") const override;

    const std::vector<std::string> GetPropertyValues(XFILE::FileProperty type, const std::string& name = "") const override
    {
      return std::vector<std::string>();
    }

  protected:
    /*!
    * \brief Construct a file cache with a custom backing source.
    *
    * Intended for specialized CFileCache implementations and tests.
    */
    CFileCache(unsigned int flags, std::unique_ptr<IFileCacheSource> source);

  private:
    std::unique_ptr<CCacheStrategy> m_pCache;
    int m_seekPossible = 0;
    std::unique_ptr<IFileCacheSource> m_source;
    std::string m_sourcePath;
    CEvent m_seekEvent;
    CEvent m_seekEnded;
    int64_t m_nSeekResult = 0;
    DWORD m_seekError = 0;
    std::atomic<bool> m_sourcePositionValid{true};
    int64_t m_seekPos = 0;
    int64_t m_readPos = 0;
    int64_t m_writePos = 0;
    unsigned m_chunkSize = 0;
    uint32_t m_writeRate = 0;
    uint32_t m_writeRateActual = 0;
    uint32_t m_writeRateLowSpeed = 0;
    int64_t m_forwardCacheSize = 0;
    int64_t m_maxForward = 0;
    bool m_bFilling = false;
    std::atomic<int64_t> m_fileSize;
    unsigned int m_flags;
    CCriticalSection m_sync;
    std::chrono::milliseconds m_processWait{100ms};
  };

}
