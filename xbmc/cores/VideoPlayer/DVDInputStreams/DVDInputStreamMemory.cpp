/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DVDInputStreamMemory.h"

CDVDInputStreamMemory::CDVDInputStreamMemory(CFileItem& fileitem) : CDVDInputStream(DVDSTREAM_TYPE_MEMORY, fileitem)
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

bool CDVDInputStreamMemory::Open()
{
  if (!CDVDInputStream::Open())
    return false;

  return true;
}

// close file and reset everything
void CDVDInputStreamMemory::Close()
{
  if (m_pData) delete[] m_pData;
  m_pData = NULL;
  m_iDataSize = 0;
  m_iDataPos = 0;

  CDVDInputStream::Close();
}

int CDVDInputStreamMemory::Read(uint8_t* buf, int buf_size)
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

int64_t CDVDInputStreamMemory::Seek(int64_t offset, int whence)
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

int64_t CDVDInputStreamMemory::GetLength()
{
  return m_iDataSize;
}

