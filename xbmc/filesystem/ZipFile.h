/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IFile.h"
#include <zlib.h>
#include "File.h"
#include "ZipManager.h"

namespace XFILE
{
  class CZipFile : public IFile
  {
  public:
    CZipFile();
    ~CZipFile() override;

    int64_t GetPosition() override;
    int64_t GetLength() override;
    bool Open(const CURL& url) override;
    bool Exists(const CURL& url) override;
    int Stat(struct __stat64* buffer) override;
    int Stat(const CURL& url, struct __stat64* buffer) override;
    ssize_t Read(void* lpBuf, size_t uiBufSize) override;
    //virtual bool ReadString(char *szLine, int iLineLength);
    int64_t Seek(int64_t iFilePosition, int iWhence = SEEK_SET) override;
    void Close() override;

    //NOTE: gzip doesn't work. use DecompressGzip() instead
    int UnpackFromMemory(std::string& strDest, const std::string& strInput, bool isGZ=false);

    /*! Decompress gzip encoded buffer in-memory */
    static bool DecompressGzip(const std::string& in, std::string& out);

  private:
    bool InitDecompress();
    bool FillBuffer();
    void DestroyBuffer(void* lpBuffer, int iBufSize);
    CFile mFile;
    SZipEntry mZipItem;
    int64_t m_iFilePos = 0; // position in _uncompressed_ data read
    int64_t m_iZipFilePos = 0; // position in _compressed_ data
    int m_iAvailBuffer = 0;
    z_stream m_ZStream;
    char m_szBuffer[65535];     // 64k buffer for compressed data
    char* m_szStringBuffer;
    char* m_szStartOfStringBuffer; // never allocated!
    size_t m_iDataInStringBuffer;
    int m_iRead;
    bool m_bFlush = false;
    bool m_bCached;
  };
}

