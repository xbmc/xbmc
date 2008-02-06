#ifdef HAS_GMYTH
#include "stdafx.h"
#include "DVDInputStreamMyth.h"
#include "FileSystem/GMythDirectory.h"

using namespace XFILE;

CDVDInputStreamMyth::CDVDInputStreamMyth() : CDVDInputStream(DVDSTREAM_TYPE_MYTH)
{
  m_pFile = NULL;
  m_eof = true;
}

CDVDInputStreamMyth::~CDVDInputStreamMyth()
{
  Close();
}

bool CDVDInputStreamMyth::IsEOF()
{
  return !m_pFile || m_eof;
}

bool CDVDInputStreamMyth::Open(const char* strFile, const std::string& content)
{
  if (!CDVDInputStream::Open(strFile, content)) return false;

  m_pFile = new CGMythFile();
  if (!m_pFile) return false;

  CURL url(strFile);
  // open file in binary mode
  if (!m_pFile->Open(url, true))
  {
    delete m_pFile;
    m_pFile = NULL;
    return false;
  }
  m_eof = true;
  return true;
}

// close file and reset everyting
void CDVDInputStreamMyth::Close()
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

int CDVDInputStreamMyth::Read(BYTE* buf, int buf_size)
{
  if(!m_pFile) return -1;

  unsigned int ret = m_pFile->Read(buf, buf_size);

  /* we currently don't support non completing reads */
  if( ret <= 0 ) m_eof = true;

  return (int)(ret & 0xFFFFFFFF);
}

__int64 CDVDInputStreamMyth::Seek(__int64 offset, int whence)
{
  if(!m_pFile) return -1;
  __int64 ret = m_pFile->Seek(offset, whence);

  /* if we succeed, we are not eof anymore */
  if( ret >= 0 ) m_eof = false;

  return ret;
}

__int64 CDVDInputStreamMyth::GetLength()
{
  if (!m_pFile) return 0;
  return m_pFile->GetLength();
}


int CDVDInputStreamMyth::GetTotalTime()
{
  if(!m_pFile) return -1;
  return m_pFile->GetTotalTime();
}

int CDVDInputStreamMyth::GetStartTime()
{
  if(!m_pFile) return -1;
  return m_pFile->GetStartTime();
}

bool CDVDInputStreamMyth::NextChannel()
{
  if(!m_pFile) return false;
  return m_pFile->NextChannel();
}

bool CDVDInputStreamMyth::PrevChannel()
{
  if(!m_pFile) return false;
  return m_pFile->PrevChannel();
}

std::string CDVDInputStreamMyth::GetTitle()
{
  CVideoInfoTag tag;
  if(m_pFile)
    m_pFile->GetVideoInfoTag(tag);
  return tag.m_strTitle;
}
#endif
