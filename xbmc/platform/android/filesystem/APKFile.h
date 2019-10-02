/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "URL.h"
#include "filesystem/IFile.h"

struct zip;
struct zip_file;

namespace XFILE
{
  class CAPKFile : public IFile
  {
  public:
    CAPKFile();
    ~CAPKFile() override = default;
    bool Open(const CURL& url) override;
    void Close() override;
    bool Exists(const CURL& url) override;

    int64_t Seek(int64_t iFilePosition, int iWhence = SEEK_SET) override;
    ssize_t Read(void* lpBuf, size_t uiBufSize) override;
    int Stat(struct __stat64* buffer) override;
    int Stat(const CURL& url, struct __stat64* buffer) override;
    int64_t GetLength() override;
    int64_t GetPosition() override;
    int GetChunkSize() override;

  protected:
    CURL              m_url;
    int               m_zip_index;
    int64_t           m_file_pos;
    int64_t           m_file_size;
    struct zip_file   *m_zip_file;
    struct zip        *m_zip_archive;
  };
}
