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
}

//*********************************************************************************************
bool CFile::Cache(const char* strFileName, const char* szDest, XFILE::IFileCallback* pCallback, void* pContext)
{
	if ( Open(strFileName,true))
	{
		HANDLE hMovie = CreateFile( szDest, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL );
		if (hMovie==NULL)
		{
			delete m_pFile;
			return false;
		}
		char *buffer = new char[16384];
		int iRead;


		long dwFileSize=GetLength();
		long dwFileSizeOrg=dwFileSize;
		DWORD dwPos=0,ipercent=0;
		char *szFileName = strrchr(strFileName,'\\');
		if (!szFileName) szFileName = strrchr(strFileName,'/');
		while (dwFileSize>0)
		{
			int iBytesToRead=16384;
			if (iBytesToRead>dwFileSize) iBytesToRead=dwFileSize;
			iRead=Read(buffer,iBytesToRead);
			if (iRead > 0)
			{
				DWORD dwWrote;
				WriteFile(hMovie,buffer,iRead,&dwWrote,NULL);
				dwFileSize -= iRead;
				dwPos+= iRead;
				float fPercent=(float)dwPos;
				fPercent /= ((float)dwFileSizeOrg);
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
							CloseHandle(hMovie);
							delete [] buffer;
							::DeleteFile(szDest);
							return false;
						}
					}
				}				
			}
			if (iRead != iBytesToRead) break;
		}
		Close();
		CloseHandle(hMovie);
		delete [] buffer;
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
unsigned int CFile::Read(void *lpBuf, offset_t uiBufSize)
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
offset_t CFile::Seek(offset_t iFilePosition, int iWhence)
{
	if (m_pFile) return m_pFile->Seek( iFilePosition, iWhence);
	return 0;
}

//*********************************************************************************************
offset_t CFile::GetLength() 
{
	if (m_pFile) return m_pFile->GetLength();
	return 0;
}

//*********************************************************************************************
offset_t CFile::GetPosition() 
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

int CFile::Write(const void* lpBuf, offset_t uiBufSize) 
{
	if (m_pFile) 
		return m_pFile->Write(lpBuf,uiBufSize);
	return -1;
}