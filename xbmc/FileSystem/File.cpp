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

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

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
      CLog::Log(LOGWARNING, "FILE::cache: the file %s has a length of 0 bytes", strFileName);
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
    int iRead;

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
      if (iBytesToRead > llFileSize)
      {
        iBytesToRead = llFileSize;
      }
      iRead = file.Read(buffer.get(), iBytesToRead);
      if (iRead > 0)
      {
        int iWrote = newFile.Write(buffer.get(), iRead);
        // iWrote contains num bytes read - make sure it's the same as numbytes written
        if (iWrote != iRead)
        { // failed!
          file.Close();
          newFile.Close();
          CFile::Delete(strDest);
          return false;
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
          {
            if (!pCallback->OnFileCallback(pContext, ipercent))
            {
              // canceled
              file.Close();
              newFile.Close();
              CFile::Delete(strDest);
              return false;
            }
          }
        }
      }
      if (iRead != iBytesToRead)
      {
        file.Close();
        newFile.Close();
        CFile::Delete(strDest);
        return false;
      }
    }
    file.Close();
    return true;
  }
  return false;
}

//*********************************************************************************************
bool CFile::Open(const CStdString& strFileName, bool bBinary)
{
  bool bPathInCache;
  if (!g_directoryCache.FileExists(strFileName, bPathInCache) )
  {
    if (bPathInCache)
      return false;
  }

  m_pFile = CFileFactory::CreateLoader(strFileName);
  if (!m_pFile) return false;

  CURL url(strFileName);

  return m_pFile->Open(url, bBinary);
}

bool CFile::OpenForWrite(const CStdString& strFileName, bool bBinary, bool bOverWrite)
{
  m_pFile = CFileFactory::CreateLoader(strFileName);
  if (!m_pFile) return false;

  CURL url(strFileName);

  return m_pFile->OpenForWrite(url, bBinary, bOverWrite);
}

bool CFile::Exists(const CStdString& strFileName)
{
  if (strFileName.IsEmpty()) return false;

  bool bPathInCache;
  if (g_directoryCache.FileExists(strFileName, bPathInCache) )
    return true;
  if (bPathInCache)
    return false;

  auto_ptr<IFile> pFile(CFileFactory::CreateLoader(strFileName));
  if (!pFile.get()) return false;

  CURL url(strFileName);

  return pFile->Exists(url);
}

int CFile::Stat(const CStdString& strFileName, struct __stat64* buffer)
{
  auto_ptr<IFile> pFile(CFileFactory::CreateLoader(strFileName));
  if (!pFile.get()) return false;

  CURL url(strFileName);
  return pFile->Stat(url, buffer);
}

//*********************************************************************************************
unsigned int CFile::Read(void *lpBuf, __int64 uiBufSize)
{
  if (m_pFile) return m_pFile->Read(lpBuf, uiBufSize);
  return 0;
}

//*********************************************************************************************
void CFile::Close()
{
  if (m_pFile)
  {
    m_pFile->Close();
    delete m_pFile;
    m_pFile = NULL;
  }
}

void CFile::Flush()
{
  if (m_pFile) m_pFile->Flush();
}

//*********************************************************************************************
__int64 CFile::Seek(__int64 iFilePosition, int iWhence)
{
  if (m_pFile) return m_pFile->Seek(iFilePosition, iWhence);
  return 0;
}

//*********************************************************************************************
__int64 CFile::GetLength()
{
  if (m_pFile) return m_pFile->GetLength();
  return 0;
}

//*********************************************************************************************
__int64 CFile::GetPosition()
{
  if (m_pFile) return m_pFile->GetPosition();
  return -1;
}


//*********************************************************************************************
bool CFile::ReadString(char *szLine, int iLineLength)
{
  if (m_pFile) return m_pFile->ReadString(szLine, iLineLength);
  return false;
}

int CFile::Write(const void* lpBuf, __int64 uiBufSize)
{
  if (m_pFile)
    return m_pFile->Write(lpBuf, uiBufSize);
  return -1;
}

bool CFile::Delete(const CStdString& strFileName)
{
  auto_ptr<IFile> pFile(CFileFactory::CreateLoader(strFileName));
  if (!pFile.get()) return false;

  return pFile->Delete(strFileName);
}

bool CFile::Rename(const CStdString& strFileName, const CStdString& strNewFileName)
{
  auto_ptr<IFile> pFile(CFileFactory::CreateLoader(strFileName));
  if (!pFile.get()) return false;

  return pFile->Rename(strFileName, strNewFileName);
}
