/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "CacheStrategy.h"
#include "File.h"
#include "IFile.h"
#include "threads/CriticalSection.h"
#include "threads/Thread.h"

#include <atomic>
#include <memory>

namespace XFILE
{

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

    int IoControl(EIoControl request, void* param) override;

    IFile *GetFileImp();

    const std::string GetProperty(XFILE::FileProperty type, const std::string &name = "") const override;

    const std::vector<std::string> GetPropertyValues(XFILE::FileProperty type, const std::string& name = "") const override
    {
      return std::vector<std::string>();
    }

  private:
    std::unique_ptr<CCacheStrategy> m_pCache;
    int m_seekPossible;
    CFile m_source;
    std::string m_sourcePath;
    CEvent m_seekEvent;
    CEvent m_seekEnded;
    int64_t m_nSeekResult;
    int64_t m_seekPos;
    int64_t m_readPos;
    int64_t m_writePos;
    unsigned m_chunkSize;
    unsigned m_writeRate;
    unsigned m_writeRateActual;
    int64_t m_forwardCacheSize;
    bool m_bFilling;
    bool m_bLowSpeedDetected;
    std::atomic<int64_t> m_fileSize;
    unsigned int m_flags;
    CCriticalSection m_sync;
  };

}
