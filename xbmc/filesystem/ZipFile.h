/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://kodi.tv
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
    int64_t m_iFilePos; // position in _uncompressed_ data read
    int64_t m_iZipFilePos; // position in _compressed_ data
    int m_iAvailBuffer;
    z_stream m_ZStream;
    char m_szBuffer[65535];     // 64k buffer for compressed data
    char* m_szStringBuffer;
    char* m_szStartOfStringBuffer; // never allocated!
    size_t m_iDataInStringBuffer;
    int m_iRead;
    bool m_bFlush;
    bool m_bCached;
  };
}

