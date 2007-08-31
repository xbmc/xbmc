
#include "stdafx.h"
#include "DVDInputStreamHttp.h"

using namespace XFILE;

CDVDInputStreamHttp::CDVDInputStreamHttp() : CDVDInputStream(DVDSTREAM_TYPE_HTTP)
{
  m_pFile = NULL;
  m_eof = true;
}

CDVDInputStreamHttp::~CDVDInputStreamHttp()
{
  Close();
}

bool CDVDInputStreamHttp::IsEOF()
{  
  if(m_pFile && !m_eof)
  {
    __int64 size = m_pFile->GetLength();
    if( size > 0 && m_pFile->GetPosition() >= size )
    {
      m_eof = true;
      return true;
    }
    return false;
  }
  return true;
}

bool CDVDInputStreamHttp::Open(const char* strFile, const std::string& content)
{
  if (!CDVDInputStream::Open(strFile, content)) return false;

  m_pFile = new CFileCurl();
  if (!m_pFile) return false;

  std::string filename = strFile;

  // shout protocol is same thing as http, but curl doesn't know what it is
  if( filename.substr(0, 8) == "shout://" )
    filename.replace(0, 8, "http://");

  m_pFile->SetHttpHeaderCallback(this);
  
  // this should go to the demuxer
  m_pFile->SetUserAgent("WinampMPEG/5.09");  
  m_pFile->SetRequestHeader("Icy-MetaData", "1");
  m_eof = false;

  // open file in binary mode
  if (!m_pFile->Open(CURL(filename), true))
  {
    delete m_pFile;
    m_pFile = NULL;
    return false;
  }

  return true;
}

void CDVDInputStreamHttp::Close()
{
  if (m_pFile)
  {
    m_pFile->Close();
    delete m_pFile;
  }

  m_httpHeader.Clear();
  
  CDVDInputStream::Close();
  m_pFile = NULL;
}

int CDVDInputStreamHttp::Read(BYTE* buf, int buf_size)
{
  unsigned int ret = 0;
  if (m_pFile) ret = m_pFile->Read(buf, buf_size);
  else return -1;

  if( ret <= 0 ) m_eof = true;

  return (int)(ret & 0xFFFFFFFF);
}

__int64 CDVDInputStreamHttp::Seek(__int64 offset, int whence)
{
  __int64 ret = 0;
  if (m_pFile) ret = m_pFile->Seek(offset, whence);
  else return -1;

  m_eof = false;

  return ret;
}

void CDVDInputStreamHttp::ParseHeaderData(CStdString strData)
{
  m_httpHeader.Parse(strData);
}

__int64 CDVDInputStreamHttp::GetLength()
{
  if (m_pFile)
    return m_pFile->GetLength();
  return 0;
}
