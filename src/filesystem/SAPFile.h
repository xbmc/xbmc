/*
 *  SAP-Announcement Support for XBMC
 *      Copyright (c) 2008 elupus (Joakim Plate)
 *      Copyright (C) 2008-2013 Team XBMC
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

// SAPFile.h: interface for the CSAPFile class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "IFile.h"
#include <sstream>

namespace XFILE
{
class CSAPFile : public IFile
{
public:
  CSAPFile();
  virtual ~CSAPFile();
  virtual int64_t GetPosition();
  virtual int64_t GetLength();
  virtual bool Open(const CURL& url);
  virtual bool Exists(const CURL& url);
  virtual int Stat(const CURL& url, struct __stat64* buffer);
  virtual ssize_t Read(void* lpBuf, size_t uiBufSize);
  virtual int64_t Seek(int64_t iFilePosition, int iWhence = SEEK_SET);
  virtual void Close();

  virtual bool Delete(const CURL& url);
  virtual bool Rename(const CURL& url, const CURL& urlnew);

  virtual int  IoControl(EIoControl request, void* param);
protected:
  std::stringstream m_stream;
  int               m_len;
};

}
