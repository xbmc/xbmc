/*
* XBoxMediaPlayer
* Copyright (c) 2002 Frodo
* Portions Copyright (c) by the authors of ffmpeg and xvid
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "../stdafx.h"
#include "File.h"
#include "filefactory.h"
#include "../application.h"
#include "../Util.h"
#include "DirectoryCache.h"


using namespace XFILE;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
#pragma warning (disable:4244)

//*********************************************************************************************
CFile::CFile()
{
  m_pFile = NULL;
}

//*********************************************************************************************
CFile::~CFile()
{
  if (m_pFile) delete m_pFile;
  m_pFile = NULL;
}

//*********************************************************************************************

class CAutoBuffer
{
  char* p;
public:
  explicit CAutoBuffer(size_t s) { p = (char*)malloc(s); }
  ~CAutoBuffer() { if (p) free(p); }
char* get() { return p; }
};

bool CFile::Cache(const CStdString& strFileName, const CStdString& strDest, XFILE::IFileCallback* pCallback, void* pContext)
{
  CFile file;
  if (file.Open(strFileName, true))
  {
    if (file.GetLength() <= 0)
    {
      CLog::Log(LOGWARNING, "FILE::cache: the file %s has a length of 0 bytes", strFileName.c_str());
      file.Close();
      // no need to return false here.  Technically, we should create the new file and leave it at that
//      return false;
    }

    CFile newFile;
    if (CUtil::IsHD(strDest)) // create possible missing dirs
    {
      vector<CStdString> tokens;
      CStdString strDirectory;
      CUtil::GetDirectory(strDest,strDirectory);
      CUtil::Tokenize(strDirectory,tokens,"\\");
      CStdString strCurrPath = tokens[0]+"\\";
      for (vector<CStdString>::iterator iter=tokens.begin()+1;iter!=tokens.end();++iter)
      {
        strCurrPath += *iter+"\\";
        CDirectory::Create(strCurrPath);
      }
    }
    CFile::Delete(strDest);
    if (!newFile.OpenForWrite(strDest, true, true))  // overwrite always
    {
      file.Close();
      return false;
    }

    // 128k is optimal for xbox
    int iBufferSize = 128 * 1024;

    // WORKAROUND:
    // Protocol is xbms? Must use a smaller buffer
    // else nothing will be cached and the file is
    // filled with garbage. :(
    CURL url(strFileName);
    CStdString strProtocol = url.GetProtocol();
    strProtocol.ToLower();
    if (strProtocol == "xbms") iBufferSize = 120 * 1024;

    // ouch, auto_ptr doesn't work for arrays!
    //auto_ptr<char> buffer ( new char[16384]);
    CAutoBuffer buffer(iBufferSize);
    int iRead, iWrite;

    UINT64 llFileSize = file.GetLength();
    UINT64 llFileSizeOrg = llFileSize;
    UINT64 llPos = 0;
    DWORD ipercent = 0;
    char *szFileName = strrchr(strFileName.c_str(), '\\');
    if (!szFileName) szFileName = strrchr(strFileName.c_str(), '/');

    // Presize file for faster writes
    // removed it since this isn't supported by samba
    /*LARGE_INTEGER size;
    size.QuadPart = llFileSizeOrg;
    ::SetFilePointerEx(hMovie, size, 0, FILE_BEGIN);
    ::SetEndOfFile(hMovie);
    size.QuadPart = 0;
    ::SetFilePointerEx(hMovie, size, 0, FILE_BEGIN);*/    
    while (llFileSize > 0)
    {
      g_application.ResetScreenSaver();
      int iBytesToRead = iBufferSize;
      
      /* make sure we don't try to read more than filesize*/
      if (iBytesToRead > llFileSize) iBytesToRead = llFileSize;

      iRead = file.Read(buffer.get(), iBytesToRead);
      if (iRead == 0) break;
      else if (iRead < 0) 
      {
        CLog::Log(LOGERROR, __FUNCTION__" - Failed read from file %s", strFileName.c_str());
        break;
      }

      /* write data and make sure we managed to write it all */
      iWrite = newFile.Write(buffer.get(), iRead); 
      if (iWrite != iRead)
      {
        CLog::Log(LOGERROR, __FUNCTION__" - Failed write to file %s", strDest.c_str());
        break;
      }
      
      llFileSize -= iRead;
      llPos += iRead;

      float fPercent = (float)llPos;
      fPercent /= ((float)llFileSizeOrg);
      fPercent *= 100.0;
      if ((int)fPercent != ipercent)
      {
        ipercent = (int)fPercent;
        if (pCallback)
          if (!pCallback->OnFileCallback(pContext, ipercent)) break;
      }
    }

    /* close both files */
    newFile.Close();
    file.Close();

    /* verify that we managed to completed the file */
    if (llPos != llFileSizeOrg)
    {      
      CFile::Delete(strDest);
      return false;
    }
    return true;
  }
  return false;
}

//*********************************************************************************************
bool CFile::Open(const CStdString& strFileName, bool bBinary)
{
  try 
  {
    bool bPathInCache;
    if (!g_directoryCache.FileExists(strFileName, bPathInCache) )
    {
      if (bPathInCache)
        return false;
    }
   
    CURL url(strFileName);

    m_pFile = CFileFactory::CreateLoader(url);
    if (!m_pFile) return false;

    return m_pFile->Open(url, bBinary);
  }
  catch(...)
  {
    CLog::Log(LOGERROR, __FUNCTION__" - Unhandled exception opening %s", strFileName.c_str());
    return false;
  }
}

bool CFile::OpenForWrite(const CStdString& strFileName, bool bBinary, bool bOverWrite)
{
  try 
  {
    CURL url(strFileName);

    m_pFile = CFileFactory::CreateLoader(url);
    if (!m_pFile) return false;

    return m_pFile->OpenForWrite(url, bBinary, bOverWrite);
  }
  catch(...)
  {
    CLog::Log(LOGERROR, __FUNCTION__" - Unhandled exception opening %s", strFileName.c_str());
    return false;
  }
}

bool CFile::Exists(const CStdString& strFileName)
{
  try
  {
    if (strFileName.IsEmpty()) return false;

    bool bPathInCache;
    if (g_directoryCache.FileExists(strFileName, bPathInCache) )
      return true;
    if (bPathInCache)
      return false;

    CURL url(strFileName);

    auto_ptr<IFile> pFile(CFileFactory::CreateLoader(url));
    if (!pFile.get()) return false;

    return pFile->Exists(url);
  }
  catch(...)
  {
    CLog::Log(LOGERROR, __FUNCTION__" - Unhandled exception checking %s", strFileName.c_str());
    return false;
  }

}

int CFile::Stat(const CStdString& strFileName, struct __stat64* buffer)
{
  try
  {
    CURL url(strFileName);

    auto_ptr<IFile> pFile(CFileFactory::CreateLoader(url));
    if (!pFile.get()) return false;

    return pFile->Stat(url, buffer);
  }
  catch(...)
  {
    CLog::Log(LOGERROR, __FUNCTION__" - Unhandled exception statting %s", strFileName.c_str());
    return -1;
  }
}

//*********************************************************************************************
unsigned int CFile::Read(void *lpBuf, __int64 uiBufSize)
{
  try
  {
    if (m_pFile) return m_pFile->Read(lpBuf, uiBufSize);
    return 0;
  }
  catch(...)
  {
    CLog::Log(LOGERROR, __FUNCTION__" - Unhandled exception");
    return 0;
  }
}

//*********************************************************************************************
void CFile::Close()
{
  try
  {
    if (m_pFile)
    {
      m_pFile->Close();
      delete m_pFile;
      m_pFile = NULL;
    }
  }
  catch(...)
  {
    CLog::Log(LOGERROR, __FUNCTION__" - Unhandled exception");
    return;
  }

}

void CFile::Flush()
{
  try
  {
    if (m_pFile) m_pFile->Flush();
  }
  catch(...)
  {
    CLog::Log(LOGERROR, __FUNCTION__" - Unhandled exception");
    return;
  }

}

//*********************************************************************************************
__int64 CFile::Seek(__int64 iFilePosition, int iWhence)
{
  try
  {
    if (m_pFile) return m_pFile->Seek(iFilePosition, iWhence);
    return 0;
  }
  catch(...)
  {
    CLog::Log(LOGERROR, __FUNCTION__" - Unhandled exception");
    return 0;
  }
}

//*********************************************************************************************
__int64 CFile::GetLength()
{
  try
  {
    if (m_pFile) return m_pFile->GetLength();
    return 0;
  }
  catch(...)
  {
    CLog::Log(LOGERROR, __FUNCTION__" - Unhandled exception");
    return 0;
  }
}

//*********************************************************************************************
__int64 CFile::GetPosition()
{
  try
  {
    if (m_pFile) return m_pFile->GetPosition();
    return -1;
  }
  catch(...)
  {
    CLog::Log(LOGERROR, __FUNCTION__" - Unhandled exception");
    return -1;
  }
}


//*********************************************************************************************
bool CFile::ReadString(char *szLine, int iLineLength)
{
  try
  {
    if (m_pFile)
    {
      __int64 iFilePos = GetPosition();

      int iBytesRead = m_pFile->Read( (unsigned char*)szLine, iLineLength - 1);
      if (iBytesRead <= 0)
        return false;

      szLine[iBytesRead] = 0;

      for (int i = 0; i < iBytesRead; i++)
      {
        if ('\n' == szLine[i])
        {
          if ('\r' == szLine[i + 1])
          {
            szLine[i + 1] = 0;
            m_pFile->Seek(iFilePos + i + 2, SEEK_SET);
          }
          else
          {
            // end of line
            szLine[i + 1] = 0;
            m_pFile->Seek(iFilePos + i + 1, SEEK_SET);
          }
          break;
        }
        else if ('\r' == szLine[i])
        {
          if ('\n' == szLine[i + 1])
          {
            szLine[i + 1] = 0;
            m_pFile->Seek(iFilePos + i + 2, SEEK_SET);
          }
          else
          {
            // end of line
            szLine[i + 1] = 0;
            m_pFile->Seek(iFilePos + i + 1, SEEK_SET);
          }
          break;
        }
      }
      return true;
    }
    return false;
  }
  catch(...)
  {
    CLog::Log(LOGERROR, __FUNCTION__" - Unhandled exception");
    return false;
  }
}

int CFile::Write(const void* lpBuf, __int64 uiBufSize)
{
  try
  {
    if (m_pFile) return m_pFile->Write(lpBuf, uiBufSize);
    return -1;
  }
  catch(...)
  {
    CLog::Log(LOGERROR, __FUNCTION__" - Unhandled exception");
    return -1;
  }

}

bool CFile::Delete(const CStdString& strFileName)
{
  try
  {
    CURL url(strFileName);

    auto_ptr<IFile> pFile(CFileFactory::CreateLoader(url));
    if (!pFile.get()) return false;

    return pFile->Delete(url);
  }
  catch(...)
  {
    CLog::Log(LOGERROR, __FUNCTION__" - Unhandled exception deleting file %s", strFileName.c_str());
    return false;
  }
}

bool CFile::Rename(const CStdString& strFileName, const CStdString& strNewFileName)
{
  try
  {
    CURL url(strFileName);
    CURL urlnew(strNewFileName);

    auto_ptr<IFile> pFile(CFileFactory::CreateLoader(url));
    if (!pFile.get()) return false;

    return pFile->Rename(url, urlnew);
  }
  catch(...)
  {
    CLog::Log(LOGERROR, __FUNCTION__" - Unhandled exception renaming file %s", strFileName.c_str());
    return false;
  }
}
