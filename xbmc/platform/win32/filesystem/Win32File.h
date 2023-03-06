/*
 *  Copyright (C) 2014-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "filesystem/IFile.h"

#include <string>

typedef void* HANDLE; // forward declaration

namespace XFILE
{
  class CWin32File : public IFile
  {
  public:
    CWin32File();
    virtual ~CWin32File();

    virtual bool Open(const CURL& url);
    virtual bool OpenForWrite(const CURL& url, bool bOverWrite = false);
    virtual void Close();

    virtual ssize_t Read(void* lpBuf, size_t uiBufSize);
    virtual ssize_t Write(const void* lpBuf, size_t uiBufSize);
    virtual int64_t Seek(int64_t iFilePosition, int iWhence = SEEK_SET);
    virtual int Truncate(int64_t toSize);
    virtual int64_t GetPosition();
    virtual int64_t GetLength();
    virtual void Flush();

    virtual bool Delete(const CURL& url);
    virtual bool Rename(const CURL& urlCurrentName, const CURL& urlNewName);
    virtual bool SetHidden(const CURL& url, bool hidden);
    virtual bool Exists(const CURL& url);
    virtual int Stat(const CURL& url, struct __stat64* statData);
    virtual int Stat(struct __stat64* statData);
    virtual int GetChunkSize();

  protected:
    explicit CWin32File(bool asSmbFile);
    HANDLE  m_hFile;
    int64_t m_filePos;
    bool    m_allowWrite;
    // file path and name in win32 long form "\\?\D:\path\to\file.ext"
    std::wstring m_filepathnameW;
    const bool m_smbFile; // true for SMB file, false for local file
    unsigned long m_lastSMBFileErr; // used for SMB file operations
  };

}
