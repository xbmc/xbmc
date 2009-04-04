#ifndef FILE_ZIP_H_
#define FILE_ZIP_H_
/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */


#include "IFile.h"
#include "lib/zlib/zlib.h"
#include "utils/log.h"
#include "GUIWindowManager.h"
#include "FileSystem/File.h"
#include "FileSystem/ZipManager.h"

namespace XFILE
{
  class CFileZip : public IFile
  {
  public:
    CFileZip();
    virtual ~CFileZip();

    virtual __int64 GetPosition();
    virtual __int64 GetLength();
    virtual bool Open(const CURL& url);
    virtual bool Exists(const CURL& url);
    virtual int Stat(struct __stat64* buffer);
    virtual int Stat(const CURL& url, struct __stat64* buffer);
    virtual unsigned int Read(void* lpBuf, __int64 uiBufSize);
    //virtual bool ReadString(char *szLine, int iLineLength);
    virtual __int64 Seek(__int64 iFilePosition, int iWhence = SEEK_SET);
    virtual void Close();

    int UnpackFromMemory(std::string& strDest, const std::string& strInput, bool isGZ=false);
  private:
    bool InitDecompress();
    bool FillBuffer();
    void DestroyBuffer(void* lpBuffer, int iBufSize);
    CFile mFile;
    SZipEntry mZipItem;
    __int64 m_iFilePos; // position in _uncompressed_ data read
    __int64 m_iZipFilePos; // position in _compressed_ data
    int m_iAvailBuffer;
    z_stream m_ZStream;
    char m_szBuffer[65535];     // 64k buffer for compressed data
    char* m_szStringBuffer;
    char* m_szStartOfStringBuffer; // never allocated!
    int m_iDataInStringBuffer;
    int m_iRead;
    bool m_bFlush;
    bool m_bCached;
  };
}

#endif
