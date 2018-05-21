#pragma once
/*
 *      Copyright (C) 2011-present Team Kodi
 *      http://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */

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
