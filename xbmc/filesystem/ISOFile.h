/*
 *      Copyright (c) 2002 Frodo
 *      Portions Copyright (c) by the authors of ffmpeg and xvid
 *      Copyright (C) 2002-2013 Team XBMC
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

// FileISO.h: interface for the CISOFile class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_FILEISO_H__C2FB9C6D_3319_4182_AB45_65E57EFAC8D1__INCLUDED_)
#define AFX_FILEISO_H__C2FB9C6D_3319_4182_AB45_65E57EFAC8D1__INCLUDED_

#pragma once

#include "IFile.h"
#include "utils/RingBuffer.h"

namespace XFILE
{

class CISOFile : public IFile
{
public:
  CISOFile();
  ~CISOFile() override;
  int64_t GetPosition() override;
  int64_t GetLength() override;
  bool Open(const CURL& url) override;
  bool Exists(const CURL& url) override;
  int Stat(const CURL& url, struct __stat64* buffer) override;
  ssize_t Read(void* lpBuf, size_t uiBufSize) override;
  int64_t Seek(int64_t iFilePosition, int iWhence = SEEK_SET) override;
  void Close() override;
protected:
  bool m_bOpened;
  HANDLE m_hFile;
  CRingBuffer m_cache;
};
}

#endif // !defined(AFX_FILEISO_H__C2FB9C6D_3319_4182_AB45_65E57EFAC8D1__INCLUDED_)
