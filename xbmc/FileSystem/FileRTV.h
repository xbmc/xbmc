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

// FileRTV.h: interface for the CFileRTV class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_FILERTV_H___INCLUDED_)
#define AFX_FILERTV_H___INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "IFile.h"

typedef struct rtv_data * RTVD;

namespace XFILE
{

class CFileRTV : public IFile
{
public:
  CFileRTV();
  virtual ~CFileRTV();
  virtual __int64 GetPosition();
  virtual __int64 GetLength();
  virtual bool Open(const CURL& url);
  bool Open(const char* strHostName, const char* strFileName, int iport);
  virtual bool Exists(const CURL& url) { return true;};
  virtual int Stat(const CURL& url, struct __stat64* buffer) { errno = ENOENT; return -1; };
  virtual unsigned int Read(void* lpBuf, __int64 uiBufSize);
  virtual __int64 Seek(__int64 iFilePosition, int iWhence = SEEK_SET);
  virtual void Close();
protected:
  UINT64 m_fileSize;
  UINT64 m_filePos;
  char m_hostName[255];
  char m_fileName[255];
  int m_iport;
private:
  RTVD m_rtvd;
  bool m_bOpened;

};
}

#endif // !defined(AFX_FILERTV_H___INCLUDED_)
