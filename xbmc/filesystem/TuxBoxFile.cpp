/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "TuxBoxFile.h"
#include <errno.h>

//Reserved for TuxBox Recording!

using namespace XFILE;

CTuxBoxFile::CTuxBoxFile()
{}

CTuxBoxFile::~CTuxBoxFile()
{
}

int64_t CTuxBoxFile::GetPosition()
{
  return 0;
}

int64_t CTuxBoxFile::GetLength()
{
  return 0;
}

bool CTuxBoxFile::Open(const CURL& url)
{
  return true;
}

unsigned int CTuxBoxFile::Read(void* lpBuf, int64_t uiBufSize)
{
  return 0;
}

int64_t CTuxBoxFile::Seek(int64_t iFilePosition, int iWhence)
{
  return 0;
}

void CTuxBoxFile::Close()
{
}

bool CTuxBoxFile::Exists(const CURL& url)
{
  return true;
}

int CTuxBoxFile::Stat(const CURL& url, struct __stat64* buffer)
{
  errno = ENOENT;
  return -1;
}

