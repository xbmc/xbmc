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
#include "FileISO.h"
#include "../sectionLoader.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
//*********************************************************************************************
CFileISO::CFileISO()
{
	m_pIsoReader=NULL;
}

//*********************************************************************************************
CFileISO::~CFileISO()
{
	if (m_pIsoReader)
	{
		Close();
	}
}
//*********************************************************************************************
bool CFileISO::Open(const char* strUserName, const char* strPassword,const char* strHostName, const char* strFileName, int iport,bool bBinary)
{
	m_pIsoReader = new iso9660();
	string strFName="\\";
	strFName+=strFileName;
	for (int i=0; i < (int)strFName.size(); ++i )
	{
		if (strFName[i]=='/') strFName[i]='\\';
	}
	if ( m_pIsoReader->OpenFile((char*)strFName.c_str()) == INVALID_HANDLE_VALUE)
	{
		delete m_pIsoReader;
		m_pIsoReader=NULL;
    return false;
	}

	// cache text files...
	if (!bBinary)
	{
		m_cache.Create(10000);
	}
	return true;
}

//*********************************************************************************************
unsigned int CFileISO::Read(void *lpBuf, __int64 uiBufSize)
{
	if (!m_pIsoReader) return 0;
	if (m_cache.Size() > 0)
	{
		long lTotalBytesRead=0;
		while (uiBufSize> 0)
		{
			if (m_cache.GetMaxReadSize() )
			{
				long lBytes2Read=m_cache.GetMaxReadSize();
				if (lBytes2Read>uiBufSize) lBytes2Read = (long)uiBufSize;
				m_cache.ReadBinary((char*)lpBuf,lBytes2Read );
				uiBufSize-=lBytes2Read ;
				lTotalBytesRead+=lBytes2Read ;
			}

			if (m_cache.GetMaxWriteSize() > 5000)
			{
				byte buffer[5000];
				long lBytesRead=m_pIsoReader->ReadFile( 1,buffer,sizeof(buffer));
				if (lBytesRead > 0)
				{
					m_cache.WriteBinary((char*)buffer,lBytesRead);
				}
				else
				{
					break;
				}
			}
		}
		return lTotalBytesRead;
	}
	return m_pIsoReader->ReadFile( 1,(byte*)lpBuf,(long)uiBufSize);
}

//*********************************************************************************************
void CFileISO::Close()
{
	if (!m_pIsoReader) return ;
	m_pIsoReader->CloseFile( );
	delete m_pIsoReader;
	m_pIsoReader=NULL;
}

//*********************************************************************************************
__int64 CFileISO::Seek(__int64 iFilePosition, int iWhence)
{
	if (!m_pIsoReader) return -1;
	m_pIsoReader->Seek(1,iFilePosition,iWhence);
	m_cache.Clear();
	return iFilePosition;
}

//*********************************************************************************************
__int64 CFileISO::GetLength()
{
	if (!m_pIsoReader) return -1;
	return m_pIsoReader->GetFileSize();
}

//*********************************************************************************************
__int64 CFileISO::GetPosition()
{
	if (!m_pIsoReader) return -1;
	return m_pIsoReader->GetFilePosition();
}


//*********************************************************************************************
bool CFileISO::ReadString(char *szLine, int iLineLength)
{
	if (!m_pIsoReader) return false;
	int iBytesRead=0;
	__int64 iPrevFilePos = GetPosition();
	strcpy(szLine,"");
	while (1)
	{
		char chKar;
		int iRead=Read((unsigned char*)&chKar, 1);
		if (iRead <= 0)
		{
			return false;
		}

		szLine[iBytesRead]=chKar;

		if ('\n' == szLine[iBytesRead])
		{
			szLine[iBytesRead]=0;
			m_cache.PeakBinary(&chKar,1);
			if (chKar== '\r')
			{
				Read((unsigned char*)&chKar, 1);
			}
			return true;
		}

		if ('\r' == szLine[iBytesRead])
		{
			szLine[iBytesRead]=0;
			m_cache.PeakBinary(&chKar,1);
			if(chKar== '\n')
			{
				Read((unsigned char*)&chKar, 1);
			}
			return true;
		}
		iBytesRead++;
		if (iBytesRead>=iLineLength)
		{
			Seek(iPrevFilePos);
			return false;
		}
	}
	return false;
}
