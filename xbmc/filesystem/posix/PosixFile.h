#pragma once
/*
 *      Copyright (C) 2014 Team XBMC
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

#include "filesystem/IFile.h"

namespace XFILE
{
  
  class CPosixFile : public IFile
  {
  public:
    CPosixFile();
    virtual ~CPosixFile();
    
    virtual bool Open(const CURL& url);
    virtual bool OpenForWrite(const CURL& url, bool bOverWrite = false);
    virtual void Close();
    
    virtual ssize_t Read(void* lpBuf, size_t uiBufSize);
    virtual ssize_t Write(const void* lpBuf, size_t uiBufSize);
    virtual int64_t Seek(int64_t iFilePosition, int iWhence = SEEK_SET);
    virtual int Truncate(int64_t size);
    virtual int64_t GetPosition();
    virtual int64_t GetLength();
    virtual void Flush();
    virtual int IoControl(EIoControl request, void* param);
    
    virtual bool Delete(const CURL& url);
    virtual bool Rename(const CURL& url, const CURL& urlnew);
    virtual bool Exists(const CURL& url);
    virtual int Stat(const CURL& url, struct __stat64* buffer);
    virtual int Stat(struct __stat64* buffer);

  protected:
    int     m_fd;
    int64_t m_filePos;
    int64_t m_lastDropPos;
    bool    m_allowWrite;
  };
  
}
