
#include "../../../stdafx.h"
#include "DVDInputStreamHttp.h"

#include "..\..\..\util.h"

CDVDInputStreamHttp::CDVDInputStreamHttp() : CDVDInputStream()
{
  m_streamType = DVDSTREAM_TYPE_HTTP;
  m_pFile = NULL;
}

CDVDInputStreamHttp::~CDVDInputStreamHttp()
{
  Close();
}

bool CDVDInputStreamHttp::Open(const char* strFile)
{
  if (!CDVDInputStream::Open(strFile)) return false;

  m_pFile = new CFileCurl();
  if (!m_pFile) return false;

  m_pFile->SetHttpHeaderCallback(this);
  
  // this should go to the demuxer
  m_pFile->SetUserAgent("WinampMPEG/5.09");
  m_pFile->AddHeaderParam("Icy-MetaData:1");
  
  // open file in binary mode
  if (!m_pFile->Open(CURL(strFile), true))
  {
    delete m_pFile;
    m_pFile = NULL;
    return false;
  }
  m_bEOF = false;

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
  m_bEOF = true;
}

int CDVDInputStreamHttp::Read(BYTE* buf, int buf_size)
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

__int64 CDVDInputStreamHttp::Seek(__int64 offset, int whence)
{
  __int64 ret = 0;
  if (m_pFile) ret = m_pFile->Seek(offset, whence);

  else return -1;

  return ret;
}

void CDVDInputStreamHttp::ParseHeaderData(CStdString strData)
{
  m_httpHeader.Parse(strData);
}
  