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

#include "File.h"

namespace XFILE
{
class CFileSpecialProtocol : public IFile
{
public:
  CFileSpecialProtocol(void);
  virtual ~CFileSpecialProtocol(void);
  virtual bool Open(const CURL& url, bool bBinary = true);
  virtual bool Exists(const CURL& url);
  virtual int Stat(const CURL& url, struct __stat64* buffer);
  virtual int Stat(struct __stat64* buffer);
  virtual bool OpenForWrite(const CURL& url, bool bBinary = true, bool bOverWrite = false);
  virtual bool Delete(const CURL& url);

  virtual unsigned int Read(void* lpBuf, __int64 uiBufSize);
  virtual int Write(const void* lpBuf, __int64 uiBufSize);
  virtual __int64 Seek(__int64 iFilePosition, int iWhence = SEEK_SET);
  virtual void Close();
  virtual __int64 GetPosition();
  virtual __int64 GetLength();

protected:
  CFile m_file;
};
}
