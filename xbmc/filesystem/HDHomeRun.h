#pragma once
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

#include "IDirectory.h"
#include "IFile.h"

struct hdhomerun_device;
class DllHdHomeRun;

namespace XFILE
{
  class CDirectoryHomeRun : public IDirectory
  {
    public:
      CDirectoryHomeRun(void);
      virtual ~CDirectoryHomeRun(void);
      virtual bool IsAllowed(const CStdString &strFile) const { return true; };
      virtual bool GetDirectory(const CStdString& strPath, CFileItemList &items);
    private:
      DllHdHomeRun* m_pdll;
  };
}

namespace XFILE
{
  class CFileHomeRun : public IFile
  {
    public:
      CFileHomeRun();
      ~CFileHomeRun();

      virtual bool          Exists(const CURL& url);
      virtual int64_t       Seek(int64_t iFilePosition, int iWhence);
      virtual int           Stat(const CURL& url, struct __stat64* buffer);
      virtual int64_t       GetPosition();
      virtual int64_t       GetLength();

      virtual bool          Open(const CURL& url);
      virtual void          Close();
      virtual unsigned int  Read(void* lpBuf, int64_t uiBufSize);
      virtual int           GetChunkSize();
    private:
      struct hdhomerun_device_t* m_device;
      DllHdHomeRun* m_pdll;
  };
}
