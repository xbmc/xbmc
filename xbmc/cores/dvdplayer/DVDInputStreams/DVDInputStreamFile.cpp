
#include "../../../stdafx.h"
#include "DVDInputStreamFile.h"

#include "..\..\..\util.h"


CDVDInputStreamFile::CDVDInputStreamFile() : CDVDInputStream()
{
  m_streamType = DVDSTREAM_TYPE_FILE;
  m_pFile = NULL;
}

CDVDInputStreamFile::~CDVDInputStreamFile()
{
  Close();
}

bool CDVDInputStreamFile::Open(const char* strFile)
{
  if (!CDVDInputStream::Open(strFile)) return false;

  m_pFile = new CFile();
  if (!m_pFile) return false;

  // open file in binary mode
  if (!m_pFile->Open(strFile, true))
  {
    delete m_pFile;
    m_pFile = NULL;
    return false;
  }
  m_bEOF = false;

  return true;
}

// close file and reset everyting
void CDVDInputStreamFile::Close()
{
  if (m_pFile)
  {
    m_pFile->Close();
    delete m_pFile;
  }

  CDVDInputStream::Close();
  m_pFile = NULL;
  m_bEOF = true;
}

int CDVDInputStreamFile::Read(BYTE* buf, int buf_size)
{
  int ret = 0;
  if (m_pFile) ret = m_pFile->Read(buf, buf_size);
  else return -1;

  if( ret <= 0 ) {
    if( m_pFile->GetPosition() >= m_pFile->GetLength() ) 
      m_bEOF = true;
  }

  return (int)(ret & 0xFFFFFFFF);
}

__int64 CDVDInputStreamFile::Seek(__int64 offset, int whence)
{
  __int64 ret = 0;
  if (m_pFile) ret = m_pFile->Seek(offset, whence);

  else return -1;

  return ret;
}
