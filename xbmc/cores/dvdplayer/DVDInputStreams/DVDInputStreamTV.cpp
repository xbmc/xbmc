#include "stdafx.h"
#include "DVDInputStreamTV.h"
#include "FileSystem/GMythFile.h"
#include "FileSystem/CMythFile.h"


using namespace XFILE;

CDVDInputStreamTV::CDVDInputStreamTV() : CDVDInputStream(DVDSTREAM_TYPE_TV)
{
  m_pFile = NULL;
  m_pLiveTV = NULL;
  m_eof = true;
}

CDVDInputStreamTV::~CDVDInputStreamTV()
{
  Close();
}

bool CDVDInputStreamTV::IsEOF()
{
  return !m_pFile || m_eof;
}

bool CDVDInputStreamTV::Open(const char* strFile, const std::string& content)
{
  if (!CDVDInputStream::Open(strFile, content)) return false;

  m_pFile = new CCMythFile();
  if (!m_pFile) return false;
  m_pLiveTV = ((CCMythFile*)m_pFile)->GetLiveTV();

  CURL url(strFile);
  // open file in binary mode
  if (!m_pFile->Open(url, true))
  {
    delete m_pFile;
    m_pFile = NULL;
    return false;
  }
  m_eof = false;
  return true;
}

// close file and reset everyting
void CDVDInputStreamTV::Close()
{
  if (m_pFile)
  {
    m_pFile->Close();
    delete m_pFile;
  }

  CDVDInputStream::Close();
  m_pFile = NULL;
  m_pLiveTV = NULL;
  m_eof = true;
}

int CDVDInputStreamTV::Read(BYTE* buf, int buf_size)
{
  if(!m_pFile) return -1;

  unsigned int ret = m_pFile->Read(buf, buf_size);

  /* we currently don't support non completing reads */
  if( ret <= 0 ) m_eof = true;

  return (int)(ret & 0xFFFFFFFF);
}

__int64 CDVDInputStreamTV::Seek(__int64 offset, int whence)
{
  if(!m_pFile) return -1;
  __int64 ret = m_pFile->Seek(offset, whence);

  /* if we succeed, we are not eof anymore */
  if( ret >= 0 ) m_eof = false;

  return ret;
}

__int64 CDVDInputStreamTV::GetLength()
{
  if (!m_pFile) return 0;
  return m_pFile->GetLength();
}


int CDVDInputStreamTV::GetTotalTime()
{
  if(!m_pLiveTV) return -1;
  return m_pLiveTV->GetTotalTime();
}

int CDVDInputStreamTV::GetStartTime()
{
  if(!m_pLiveTV) return -1;
  return m_pLiveTV->GetStartTime();
}

bool CDVDInputStreamTV::NextChannel()
{
  if(!m_pLiveTV) return false;
  return m_pLiveTV->NextChannel();
}

bool CDVDInputStreamTV::PrevChannel()
{
  if(!m_pLiveTV) return false;
  return m_pLiveTV->PrevChannel();
}

CVideoInfoTag* CDVDInputStreamTV::GetVideoInfoTag()
{
  if(m_pLiveTV)
    return m_pLiveTV->GetVideoInfoTag();
  return NULL;
}

bool CDVDInputStreamTV::SeekTime(int iTimeInMsec)
{
  return false;
}

bool CDVDInputStreamTV::NextStream()
{
  if(!m_pFile) return false;
  if(m_pFile->SkipNext())
  {
    m_eof = false;
    return true;
  }
  return false;
}
