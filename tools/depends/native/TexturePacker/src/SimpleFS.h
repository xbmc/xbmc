#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include <stdio.h>
#include <string>
#include <stdint.h>

class CFile
{
public:
  CFile()
  {
    m_file = NULL;
  }
  
  ~CFile()
  {
    Close();
  }

  bool Open(const std::string &file)
  {
    Close();
    m_file = fopen(file.c_str(), "rb");
    return NULL != m_file;
  }

  bool OpenForWrite(const std::string &file, bool overwrite)
  {
    Close();
    m_file = fopen(file.c_str(), "wb");
    return NULL != m_file;
  }
  void Close()
  {
    if (m_file)
      fclose(m_file);
    m_file = NULL;
  }

  uint64_t Read(void *data, uint64_t size)
  {
    if (fread(data, (size_t)size, 1, m_file) == 1)
      return size;
    return 0;
  }

  uint64_t Write(const void *data, uint64_t size)
  {
    if (fwrite(data, (size_t)size, 1, m_file) == 1)
      return size;
    return 0;
  }
  
  FILE *getFP()
  {
    return m_file;
  }

  uint64_t GetFileSize()
  {
    long curPos = ftell(m_file);
    uint64_t fileSize = 0;
    if (fseek(m_file, 0, SEEK_END) == 0)
    {
      long size = ftell(m_file);
      if (size >= 0)
        fileSize = (uint64_t)size;
    }

    // restore fileptr
    Seek(curPos);

    return fileSize;
  }

  uint64_t Seek(uint64_t offset)
  {
    uint64_t seekedBytes = 0;
    int seekRet = fseek(m_file, offset, SEEK_SET);
    if (seekRet == 0)
      seekedBytes = offset;
    return seekedBytes;
  }

private:
  FILE* m_file;
};
