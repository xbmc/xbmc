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
#include "filefactory.h"
#include "../url.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
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
bool CFile::Cache(const char* strFileName, const char* szDest, XFILE::IFileCallback* pCallback, void* pContext)
{
	if ( Open(strFileName,true))
	{
		::DeleteFile(szDest);
		CAutoPtrHandle hMovie ( CreateFile( szDest, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL ) );
		if (!hMovie.isValid() )
		{
			delete m_pFile;
			m_pFile=NULL;
			return false;
		}
		auto_ptr<char> buffer ( new char[16384]);
		int iRead;

		UINT64 llFileSize=GetLength();
		UINT64 llFileSizeOrg=llFileSize;
		UINT64 llPos=0;
		DWORD  ipercent=0;
		char *szFileName = strrchr(strFileName,'\\');
		if (!szFileName) szFileName = strrchr(strFileName,'/');
		while (llFileSize>0)
		{
			int iBytesToRead=16384;
			if (iBytesToRead>llFileSize) 
			{
				iBytesToRead=llFileSize;
			}
			iRead=Read(buffer.get(),iBytesToRead);
			if (iRead > 0)
			{
				DWORD dwWrote;
				WriteFile( (HANDLE)hMovie,buffer.get(),iRead,&dwWrote,NULL);
				llFileSize -= iRead;
				llPos+= iRead;
				float fPercent=(float)llPos;
				fPercent /= ((float)llFileSizeOrg);
				fPercent*=100.0;
				if ( (int)fPercent != ipercent)
				{
					ipercent=(int)fPercent;
					if (pCallback)
					{
						if (!pCallback->OnFileCallback(pContext,ipercent))
						{
							// canceled
							Close();
							::DeleteFile(szDest);
							return false;
						}
					}
				}				
			}
			if (iRead != iBytesToRead) 
			{
				Close();
				::DeleteFile(szDest);
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
	CFileFactory factory;
	m_pFile=factory.CreateLoader(strFileName);
	if (!m_pFile) return false;

  CURL url(strFileName);

  return m_pFile->Open(url.GetUserName().c_str(),url.GetPassWord().c_str(),url.GetHostName().c_str(),url.GetFileName().c_str(), url.GetPort(),bBinary);
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