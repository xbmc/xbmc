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

#include "DVDInputStreamMemory.h"

CDVDInputStreamMemory::CDVDInputStreamMemory() : CDVDInputStream(DVDSTREAM_TYPE_MEMORY)
{
  m_pData = NULL;
  m_iDataSize = 0;
  m_iDataPos = 0;
}

CDVDInputStreamMemory::~CDVDInputStreamMemory()
{
  Close();
}

bool CDVDInputStreamMemory::IsEOF()
{
  if(m_iDataPos >= m_iDataSize)
    return true;

  return false;
}

bool CDVDInputStreamMemory::Open(const char* strFile, const std::string& content)
{
  if (!CDVDInputStream::Open(strFile, content)) return false;

  return true;
}

// close file and reset everyting
void CDVDInputStreamMemory::Close()
{
  if (m_pData) delete[] m_pData;
  m_pData = NULL;
  m_iDataSize = 0;
  m_iDataPos = 0;

  CDVDInputStream::Close();
}

int CDVDInputStreamMemory::Read(BYTE* buf, int buf_size)
{
  int iBytesToCopy = buf_size;
  int iBytesLeft = m_iDataSize - m_iDataPos;
  if (iBytesToCopy > iBytesLeft) iBytesToCopy = iBytesLeft;

  if (iBytesToCopy > 0)
  {
    memcpy(buf, m_pData + m_iDataPos, iBytesToCopy);
    m_iDataPos += iBytesToCopy;
  }

  return iBytesToCopy;
}

__int64 CDVDInputStreamMemory::Seek(__int64 offset, int whence)
{
  switch (whence)
  {
    case SEEK_CUR:
    {
      if ((m_iDataPos + offset) > m_iDataSize) return -1;
      else m_iDataPos += (int)offset;
      break;
    }
    case SEEK_END:
    {
      m_iDataPos = m_iDataSize;
      break;
    }
    case SEEK_SET:
    {
      if (offset > m_iDataSize || offset < 0) return -1;
      else m_iDataPos = (int)offset;
      break;
    }
    default:
      return -1;
  }
  return m_iDataPos;
}

__int64 CDVDInputStreamMemory::GetLength()
{
  return m_iDataSize;
}

