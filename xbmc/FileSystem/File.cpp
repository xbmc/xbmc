
#include "stdafx.h"
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
#include "File.h"
#include "../utils/log.h"
#include "filefactory.h"
#include "../url.h"
#include "../application.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include "DirectoryCache.h"
#include "../autoptrhandle.h"
using namespace AUTOPTR;
using namespace XFILE;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
#pragma warning (disable:4244)

//*********************************************************************************************
CFile::CFile()
{
	m_pFile=NULL;
}

//*********************************************************************************************
CFile::~CFile()
{
	if (m_pFile) delete m_pFile;
	m_pFile=NULL;
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

bool CFile::Cache(const char* strFileName, const char* szDest, XFILE::IFileCallback* pCallback, void* pContext)
{
	if (Open(strFileName, true))
	{
		if (GetLength()<=0)
		{
			CLog::Log(LOGWARNING, "FILE::cache: the file %s has a length of 0 bytes", strFileName);
			Close();
			return false;
		}

		CFile::Delete(szDest);
		CFile newFile;
		if (!newFile.OpenForWrite(szDest))
		{
			delete m_pFile;
			m_pFile = NULL;
			return false;
		}

		// 128k is optimal for xbox
		int iBufferSize = 128*1024;

		//	WORKAROUND: 
		//	Protocol is xbms? Must use a smaller buffer
		//	else nothing will be cached and the file is
		//	filled with garbage. :(
		CURL url(strFileName);
		CStdString strProtocol = url.GetProtocol();
		strProtocol.ToLower();
		if (strProtocol=="xbms") iBufferSize=120*1024; 

		// ouch, auto_ptr doesn't work for arrays!
		//auto_ptr<char> buffer ( new char[16384]);
		CAutoBuffer buffer(iBufferSize);
		int iRead;

		UINT64 llFileSize = GetLength();
		UINT64 llFileSizeOrg = llFileSize;
		UINT64 llPos = 0;
		DWORD  ipercent = 0;
		char *szFileName = strrchr(strFileName, '\\');
		if (!szFileName) szFileName = strrchr(strFileName, '/');

		// Presize file for faster writes
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
			iRead = Read(buffer.get(), iBytesToRead);
			if (iRead > 0)
			{
				//DWORD dwWrote;
				newFile.Write(buffer.get(), iRead);
				llFileSize -= iRead;
				llPos += iRead;
				float fPercent =(float)llPos;
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
							Close();
							CFile::Delete(szDest);
							return false;
						}
					}
				}				
			}
			if (iRead != iBytesToRead) 
			{
				Close();
				CFile::Delete(szDest);
				return false;
			}
		}
		Close();
		return true;
	}
	return false;
}

//*********************************************************************************************
bool CFile::Open(const char* strFileName, bool bBinary)
{
  bool bPathInCache;
  if (!g_directoryCache.FileExists(strFileName,bPathInCache) )
  {
    if (bPathInCache) 
      return false;
  }

	CFileFactory factory;
	m_pFile=factory.CreateLoader(strFileName);
	if (!m_pFile) return false;

  CURL url(strFileName);

  return m_pFile->Open(url,bBinary);
}

bool CFile::OpenForWrite(const char* strFileName, bool bBinary)
{
	CFileFactory factory;
	m_pFile = factory.CreateLoader(strFileName);
	if (!m_pFile) return false;

  CURL url(strFileName);

  return m_pFile->OpenForWrite(url, bBinary);
}

bool CFile::Exists(const char* strFileName)
{
  bool bPathInCache;
  if (g_directoryCache.FileExists(strFileName,bPathInCache) )
    return true;
  if (bPathInCache) 
    return false;

	CFileFactory factory;
	m_pFile=factory.CreateLoader(strFileName);
	if (!m_pFile) return false;
	
	CURL url(strFileName);

	return m_pFile->Exists(url);
}

int CFile::Stat(const char* strFileName, struct __stat64* buffer)
{
	CFileFactory factory;
	m_pFile=factory.CreateLoader(strFileName);
	if (!m_pFile) return false;
	
	CURL url(strFileName);
	return m_pFile->Stat(url, buffer);
}

//*********************************************************************************************
unsigned int CFile::Read(void *lpBuf, __int64 uiBufSize)
{
	if (m_pFile) return m_pFile->Read(lpBuf,uiBufSize);
	return 0;
}

//*********************************************************************************************
void CFile::Close()
{
	if (m_pFile) 
	{
		m_pFile->Close();
		delete m_pFile;
		m_pFile=NULL;
	}
}

//*********************************************************************************************
__int64 CFile::Seek(__int64 iFilePosition, int iWhence)
{
	if (m_pFile) return m_pFile->Seek( iFilePosition, iWhence);
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
	if (m_pFile) return m_pFile->ReadString(szLine,iLineLength);
	return false;
}

int CFile::Write(const void* lpBuf, __int64 uiBufSize) 
{
	if (m_pFile) 
		return m_pFile->Write(lpBuf,uiBufSize);
	return -1;
}

bool CFile::Delete(const char* strFileName)
{
  bool bSucces;
  IFile* pFile = CFileFactory().CreateLoader(strFileName);
  if (!pFile) return false;
  
  bSucces = pFile->Delete(strFileName);
  delete pFile;
  
  return bSucces;
}

bool CFile::Rename(const char* strFileName, const char* strNewFileName)
{
  bool bSucces;
  IFile* pFile = CFileFactory().CreateLoader(strNewFileName);
  if (!pFile) return false;
  
  bSucces = pFile->Rename(strFileName, strNewFileName);
  delete pFile;
  
  return bSucces;
}
