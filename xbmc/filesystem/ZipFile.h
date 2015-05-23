#ifndef FILE_ZIP_H_
#define FILE_ZIP_H_
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
    virtual ~CZipFile();

    virtual int64_t GetPosition();
    virtual int64_t GetLength();
    virtual bool Open(const CURL& url);
    virtual bool Exists(const CURL& url);
    virtual int Stat(struct __stat64* buffer);
    virtual int Stat(const CURL& url, struct __stat64* buffer);
    virtual ssize_t Read(void* lpBuf, size_t uiBufSize);
    //virtual bool ReadString(char *szLine, int iLineLength);
    virtual int64_t Seek(int64_t iFilePosition, int iWhence = SEEK_SET);
    virtual void Close();

    int UnpackFromMemory(std::string& strDest, const std::string& strInput, bool isGZ=false);
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

#endif
