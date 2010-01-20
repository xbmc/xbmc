#pragma once

/*
 *      Copyright (C) 2005-2009 Team XBMC
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
    if (fread(data, size, 1, m_file) == 1)
      return size;
    return 0;
  }

  uint64_t Write(const void *data, uint64_t size)
  {
    if (fwrite(data, size, 1, m_file) == 1)
      return size;
    return 0;
  }

private:
  FILE* m_file;
};
