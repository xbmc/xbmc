/*
 *      Copyright (C) 2012 Team XBMC
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

#ifndef FILE_ARCHIVE_H_
#define FILE_ARCHIVE_H_

#if (defined HAVE_CONFIG_H) && (!defined WIN32)
  #include "config.h"
#endif

#ifdef HAVE_LIBARCHIVE

#include "IFile.h"
#include "DllLibArchive.h"

namespace XFILE
{
  class CArchiveFile : public IFile
  {
  public:
    CArchiveFile();
    CArchiveFile(int format, int filter);
    virtual ~CArchiveFile();

    virtual bool Open(const CURL& url);
    virtual bool Exists(const CURL& url);
    virtual int Stat(const CURL& url, struct __stat64* buffer);
    virtual int Stat(struct __stat64* buffer);
    virtual unsigned int Read(void* lpBuf, int64_t uiBufSize);
    virtual bool ReadString(char *szLine, int iLineLength);
    virtual int64_t Seek(int64_t iFilePosition, int iWhence = SEEK_SET);
    virtual void Close();
    virtual int64_t GetPosition();
    virtual int64_t GetLength();
  private:
    int m_format;
    int m_filter;
    CStdString m_strUrl;
    struct archive *m_archive;
    struct archive_entry *m_archive_entry;
    int64_t m_position;
    struct __stat64 *m_fakeDirStatBuffer;
    DllLibArchive m_dll;
    bool CreateArchive();
    bool Load(CStdString const& function);
  };
}

#endif

#endif
