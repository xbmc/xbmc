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

#include "IFile.h"
#include "FTPUtil.h"
#include "AutoPtrHandle.h"
using namespace XFILE;
using namespace AUTOPTR;

namespace XFILE
{
	class CFileFTP : public IFile  
	{
    public:
	    CFileFTP();
	    virtual ~CFileFTP();
	    virtual bool Open(const CURL& url, bool bBinary = true);
	    virtual bool Exists(const CURL& url);
      virtual bool Open(const char* strUserName, const char* strPassword,const char* strHostName, const char* strFileName,int iport, bool bBinary=true);
	    virtual bool Exists(const char* strUserName, const char* strPassword,const char* strHostName, const char* strFileName,int iport);
	    virtual __int64	Seek(__int64 iFilePosition, int iWhence=SEEK_SET);
      virtual __int64 GetPosition();
	    virtual __int64	GetLength();
      virtual int	Stat(const CURL& url, struct __stat64* buffer);
	    virtual void Close();
      virtual unsigned int Read(void* lpBuf, __int64 uiBufSize);
    protected:
	    UINT64					m_fileSize ;
	    UINT64					m_filePos;
	    char m_filename[255];
    private:
	    bool m_bOpened;
      CAutoPtrSocket rs;
	    __int64 Recv(byte* pBuffer,__int64 iLen);
	    CFTPUtil FTPUtil;
	};
};
