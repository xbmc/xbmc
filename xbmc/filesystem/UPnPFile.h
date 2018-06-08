/*
 *      Copyright (C) 2011-2013 Team XBMC
 *      http://kodi.tv
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

#pragma once

#include "IFile.h"

namespace XFILE
{
  class CUPnPFile : public IFile
  {
    public:
      CUPnPFile();
      ~CUPnPFile() override;
      bool Open(const CURL& url) override;
      bool Exists(const CURL& url) override;
      int Stat(const CURL& url, struct __stat64* buffer) override;

      ssize_t Read(void* lpBuf, size_t uiBufSize) override {return -1;}
      int64_t Seek(int64_t iFilePosition, int iWhence = SEEK_SET) override {return -1;}
      void Close() override{}
      int64_t GetPosition() override {return -1;}
      int64_t GetLength() override {return -1;}
  };
}
