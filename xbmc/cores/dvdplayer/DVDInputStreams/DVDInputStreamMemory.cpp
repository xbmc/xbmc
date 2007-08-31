
#include "stdafx.h"
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
  }
  return m_iDataPos;
}

__int64 CDVDInputStreamMemory::GetLength()
{
  return m_iDataSize;
}

