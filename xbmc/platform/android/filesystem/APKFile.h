/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "filesystem/IFile.h"
#include "URL.h"

struct zip;
struct zip_file;

namespace XFILE
{
  class CAPKFile : public IFile
  {
  public:
    CAPKFile();
    virtual ~CAPKFile();
    virtual bool Open(const CURL& url);
    virtual void Close();
    virtual bool Exists(const CURL& url);

    virtual int64_t Seek(int64_t iFilePosition, int iWhence = SEEK_SET);
    virtual ssize_t Read(void* lpBuf, size_t uiBufSize);
    virtual int Stat(struct __stat64* buffer);
    virtual int Stat(const CURL& url, struct __stat64* buffer);
    virtual int64_t GetLength();
    virtual int64_t GetPosition();
    virtual int GetChunkSize();
  protected:
    CURL              m_url;
    int               m_zip_index;
    int64_t           m_file_pos;
    int64_t           m_file_size;
    struct zip_file   *m_zip_file;
    struct zip        *m_zip_archive;
  };
}
