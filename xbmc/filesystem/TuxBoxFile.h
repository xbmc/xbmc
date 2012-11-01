#pragma once
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

#include "IFile.h"

namespace XFILE
{
  class CTuxBoxFile : public IFile
  {
    public:
      CTuxBoxFile();
      virtual ~CTuxBoxFile();
      virtual int64_t GetPosition();
      virtual int64_t GetLength();
      virtual bool Open(const CURL& url);
      virtual void Close();
      virtual bool Exists(const CURL& url);
      virtual int Stat(const CURL& url, struct __stat64* buffer);
      virtual unsigned int Read(void* lpBuf, int64_t uiBufSize);
      virtual int64_t Seek(int64_t iFilePosition, int iWhence = SEEK_SET);
    protected:
  };
}



