/*
 *      Copyright (C) 2005-2010 Team XBMC
 *      http://www.xbmc.org
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <iostream>
#include "IFile.h"

namespace PLATFORM
{
class CFile: public IFile
{
public:
  CFile();
  virtual ~CFile();

  bool Open(const CStdString& strFileName, unsigned int flags = 0);
  bool IsInvalid()
  {
    return (m_hFile == INVALID_HANDLE_VALUE);
  };

  unsigned long Read(void* lpBuf, int64_t uiBufSize);
  int64_t Seek(int64_t iFilePosition, int iWhence = SEEK_SET);
  int64_t GetPosition();
  int64_t GetLength();
  void Close();
  static bool Exists(const CStdString& strFileName, bool bUseCache = true);
  //int Stat(struct __stat64 *buffer);

private:
  unsigned int m_flags;
  HANDLE       m_hFile;
  int64_t      m_fileSize;
  bool         m_bReadOnly;
};

} // namespace PLATFORM
