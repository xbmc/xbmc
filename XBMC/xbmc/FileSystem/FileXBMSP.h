/*
* XBMC Media Center
* Copyright (c) 2002 Frodo
* Portions Copyright (c) by the authors of ffmpeg and xvid
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
// FileXBMSP.h: interface for the CFileXBMSP class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_FILEXMBMSP_H___INCLUDED_)
#define AFX_FILEXMBMSP_H___INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "IFile.h"

extern "C"
{
#include "lib/libXBMS/ccincludes.h"
#include "lib/libXBMS/ccxclient.h"
}

namespace XFILE
{

class CFileXBMSP : public IFile
{
public:
  CFileXBMSP();
  virtual ~CFileXBMSP();
  virtual __int64 GetPosition();
  virtual __int64 GetLength();
  virtual bool Open(const CURL& url);
  virtual bool Exists(const CURL& url);
  virtual int Stat(const CURL& url, struct __stat64* buffer);
  virtual unsigned int Read(void* lpBuf, __int64 uiBufSize);
  virtual __int64 Seek(__int64 iFilePosition, int iWhence = SEEK_SET);
  virtual void Close();
protected:
  UINT64 m_fileSize;
  UINT64 m_filePos;
  SOCKET m_socket;
private:
  CcXstreamServerConnection m_connection;
  unsigned long m_handle;
  bool m_bOpened;

};
}

#endif // !defined(AFX_FILEXMBMSP_H___INCLUDED_)
