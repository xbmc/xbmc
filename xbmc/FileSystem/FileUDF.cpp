#include "FileUDF.h"
#include "URL.h"
#include "Util.h"

#include <sys/stat.h>
#include <errno.h>

using namespace std;
using namespace XFILE;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
//*********************************************************************************************
CFileUDF::CFileUDF()
{
  m_bOpened = false;
}

//*********************************************************************************************
CFileUDF::~CFileUDF()
{
  if (m_bOpened)
  {
    Close();
  }
}
//*********************************************************************************************
bool CFileUDF::Open(const CURL& url)
{
  CStdString strFName = url.GetHostName();

  CUtil::URLDecode(strFName);

  m_hFile = m_udfIsoReaderLocal.OpenFile((char*)strFName.c_str());
  if (m_hFile == INVALID_HANDLE_VALUE)
  {
    m_bOpened = false;
    return false;
  }

  m_bOpened = true;
  return true;
}

//*********************************************************************************************
unsigned int CFileUDF::Read(void *lpBuf, int64_t uiBufSize)
{
  if (!m_bOpened) return 0;
  char *pData = (char *)lpBuf;

  if (m_cache.getSize() > 0)
  {
    long lTotalBytesRead = 0;
    while (uiBufSize > 0)
    {
      if (m_cache.getMaxReadSize() )
      {
        long lBytes2Read = m_cache.getMaxReadSize();
        if (lBytes2Read > uiBufSize) lBytes2Read = (long)uiBufSize;
        m_cache.ReadData(pData, lBytes2Read );
        uiBufSize -= lBytes2Read ;
        pData += lBytes2Read;
        lTotalBytesRead += lBytes2Read ;
      }

      if (m_cache.getMaxWriteSize() > 5000)
      {
        unsigned char buffer[5000];
        long lBytesRead = m_udfIsoReaderLocal.ReadFile( m_hFile, buffer, sizeof(buffer));
        if (lBytesRead > 0)
          m_cache.WriteData((char*)buffer, lBytesRead);
        else
          return 0;
      }
    }
    return lTotalBytesRead;
  }
  int iResult = m_udfIsoReaderLocal.ReadFile( m_hFile, (unsigned char*)pData, (long)uiBufSize);
  if (iResult == -1)
    return 0;
  return iResult;
}

//*********************************************************************************************
void CFileUDF::Close()
{
  if (!m_bOpened) return ;
  m_udfIsoReaderLocal.CloseFile( m_hFile);
}

//*********************************************************************************************
int64_t CFileUDF::Seek(int64_t iFilePosition, int iWhence)
{
  if (!m_bOpened) return -1;
  int64_t lNewPos = m_udfIsoReaderLocal.Seek(m_hFile, iFilePosition, iWhence);
  if(lNewPos >= 0)
    m_cache.Clear();
  return lNewPos;
}

//*********************************************************************************************
int64_t CFileUDF::GetLength()
{
  if (!m_bOpened) return -1;
  return m_udfIsoReaderLocal.GetFileSize(m_hFile);
}

//*********************************************************************************************
int64_t CFileUDF::GetPosition()
{
  if (!m_bOpened) return -1;
  return m_udfIsoReaderLocal.GetFilePosition(m_hFile);
}

bool CFileUDF::Exists(const CURL& url)
{
  string strFName = "\\";
  strFName += url.GetFileName();
  for (int i = 0; i < (int)strFName.size(); ++i )
  {
    if (strFName[i] == '/') strFName[i] = '\\';
  }
  m_hFile = m_udfIsoReaderLocal.OpenFile((char*)strFName.c_str());
  if (m_hFile == INVALID_HANDLE_VALUE)
    return false;

  m_udfIsoReaderLocal.CloseFile(m_hFile);
  return true;
}

int CFileUDF::Stat(const CURL& url, struct __stat64* buffer)
{
  string strFName = "\\";
  strFName += url.GetFileName();
  for (int i = 0; i < (int)strFName.size(); ++i )
  {
    if (strFName[i] == '/') strFName[i] = '\\';
  }
  m_hFile = m_udfIsoReaderLocal.OpenFile((char*)strFName.c_str());
  if (m_hFile != INVALID_HANDLE_VALUE)
  {
    buffer->st_size = m_udfIsoReaderLocal.GetFileSize(m_hFile);
    buffer->st_mode = _S_IFREG;
    m_udfIsoReaderLocal.CloseFile(m_hFile);
    return 0;
  }
  errno = ENOENT;
  return -1;
}
