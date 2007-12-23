#include "stdafx.h"
#include "FileMMS.h"

using namespace XFILE;

CFileMMS::CFileMMS()
{
}

CFileMMS::~CFileMMS()
{
}

__int64 CFileMMS::GetPosition()
{
  return mms_get_current_pos(m_mms);
}

__int64 CFileMMS::GetLength()
{
  return mms_get_length(m_mms);
}


bool CFileMMS::Open(const CURL& url, bool bBinary)
{
  CStdString strUrl;
  url.GetURL(strUrl);
 
  m_mms = mms_connect(NULL, NULL, strUrl.c_str(), 128*1024);
  if (!m_mms)
  {
     return false;
  }

  return true;
}

unsigned int CFileMMS::Read(void* lpBuf, __int64 uiBufSize)
{
  int s = mms_read(NULL, m_mms, (char*) lpBuf, uiBufSize);  
  return s;
}

__int64 CFileMMS::Seek(__int64 iFilePosition, int iWhence)
{
   return mms_seek(NULL, m_mms, iFilePosition, 0);
}

void CFileMMS::Close()
{
   mms_close(m_mms);
}

CStdString CFileMMS::GetContent()
{
   return "audio/x-ms-wma";
}
