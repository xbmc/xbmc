/*
* XBMC
* 7z Filesystem
* Copyright (c) 2008 topfs2
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "IFile.h"
#include "File.h"
#include "7zManager.h"

namespace XFILE
{
  class CFile7z : public IFile  
	{
	public:
		CFile7z();
//    CFileRar(bool bSeekable); // used for caching files
		virtual ~CFile7z();
		virtual __int64			  GetPosition();
		virtual __int64			  GetLength();
		virtual bool					Open(const CURL& url, bool bBinary=true);
		virtual bool					Exists(const CURL& url);
		virtual int						Stat(const CURL& url, struct __stat64* buffer);
		virtual unsigned int	Read(void* lpBuf, __int64 uiBufSize);
		virtual int						Write(const void* lpBuf, __int64 uiBufSize);
		virtual __int64			  Seek(__int64 iFilePosition, int iWhence=SEEK_SET);
    virtual void					Close();
		virtual void          Flush();

		virtual bool					OpenForWrite(const CURL& url, bool bBinary=true);
		unsigned int					Write(void *lpBuf, __int64 uiBufSize);
  private:
    __int64               m_FilePosition;
    __int64               m_FileLength;
    bool                  m_Open;

    // Read
    CThread               *m_ExtractThread;
    CFile7zExtractThread  *m_ExtractInfo;

    __int64                m_Processed;
  };
}
