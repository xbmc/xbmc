
#include "stdafx.h"
#include "DVDInputStreamFile.h"

using namespace XFILE;

CDVDInputStreamFile::CDVDInputStreamFile() : CDVDInputStream(DVDSTREAM_TYPE_FILE)
{
  m_pFile = NULL;
  m_eof = true;
}

CDVDInputStreamFile::~CDVDInputStreamFile()
{
  Close();
}

bool CDVDInputStreamFile::IsEOF()
{
  return !m_pFile || m_eof;
}

bool CDVDInputStreamFile::Open(const char* strFile, const std::string& content)
{
  if (!CDVDInputStream::Open(strFile, content)) return false;

  m_pFile = new CFile();
  if (!m_pFile) return false;

  int nFlags = READ_TRUNCATED;

  // only cache http.
  // TODO: cache smb too but only when a cache strategy that enables large seek jumps back
  // and forth is available.
  if (CStdString(strFile).Left(5) == "http:")
	nFlags |= READ_CACHED;

  // open file in binary mode
  if (!m_pFile->Open(strFile, true, nFlags ))
  {
    delete m_pFile;
    m_pFile = NULL;
    return false;
  }
  m_eof = true;
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
  m_eof = true;
}

int CDVDInputStreamFile::Read(BYTE* buf, int buf_size)
{
  if(!m_pFile) return -1;

  unsigned int ret = m_pFile->Read(buf, buf_size);

  /* we currently don't support non completing reads */
  if( ret <= 0 ) m_eof = true;

  /* on error close file */
  if( ret < 0 ) Close();

  return (int)(ret & 0xFFFFFFFF);
}

__int64 CDVDInputStreamFile::Seek(__int64 offset, int whence)
{
  if(!m_pFile) return -1;
  __int64 ret = m_pFile->Seek(offset, whence);

  /* if we succeed, we are not eof anymore */
  if( ret >= 0 ) m_eof = false;

  return ret;
}

__int64 CDVDInputStreamFile::GetLength()
{
  if (m_pFile)
    return m_pFile->GetLength();
  return 0;
}

BitstreamStats CDVDInputStreamFile::GetBitstreamStats() const 
{
  if (!m_pFile)
    return m_stats; // dummy return. defined in CDVDInputStream

  return m_pFile->GetBitstreamStats();
}

