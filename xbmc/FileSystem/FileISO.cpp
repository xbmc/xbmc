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
	CSectionLoader::Load("ISO9660");
	m_pIsoReader=NULL;
}

//*********************************************************************************************
CFileISO::~CFileISO()
{
	if (m_pIsoReader)
	{
		Close();
	}
	CSectionLoader::Unload("ISO9660");
}
//*********************************************************************************************
bool CFileISO::Open(const char* strUserName, const char* strPassword,const char* strHostName, const char* strFileName, int iport,bool bBinary)
{
  
	m_pIsoReader = new CISO9660(m_cdrom);
	if (!m_pIsoReader->OpenDisc() )
	{
		delete m_pIsoReader;
		m_pIsoReader=NULL;
		
    return false;
	}

	if ( m_pIsoReader->OpenFile(strFileName) < 0)
	{
		delete m_pIsoReader;
		m_pIsoReader=NULL;
		
    return false;
	}
	return true;
}

//*********************************************************************************************
unsigned int CFileISO::Read(void *lpBuf, offset_t uiBufSize)
{
	if (!m_pIsoReader) return -1;
	return m_pIsoReader->ReadFile(1,(byte*)lpBuf,(long)uiBufSize);
}

//*********************************************************************************************
void CFileISO::Close()
{
	if (!m_pIsoReader) return ;
	m_pIsoReader->CloseFile(1);
	delete m_pIsoReader;
	m_pIsoReader=NULL;
  
  
}

//*********************************************************************************************
offset_t CFileISO::Seek(offset_t iFilePosition, int iWhence)
{
	if (!m_pIsoReader) return -1;
	m_pIsoReader->Seek(1,iFilePosition,iWhence);
	return iFilePosition;
}

//*********************************************************************************************
offset_t CFileISO::GetLength()
{
	if (!m_pIsoReader) return -1;
	return m_pIsoReader->GetFileSize();
}

//*********************************************************************************************
offset_t CFileISO::GetPosition()
{
	if (!m_pIsoReader) return -1;
	return m_pIsoReader->GetFilePosition();
}


//*********************************************************************************************
bool CFileISO::ReadString(char *szLine, int iLineLength)
{
	if (!m_pIsoReader) return false;
	offset_t iFilePos=GetPosition();

	int iBytesRead=m_pIsoReader->ReadFile(1, (unsigned char*)szLine, iLineLength);
	if (iBytesRead <= 0)
	{
		return false;
	}

	szLine[iBytesRead]=0;

	for (int i=0; i < iBytesRead; i++)
	{
		if ('\n' == szLine[i])
		{
			if ('\r' == szLine[i+1])
			{
				szLine[i+2]=0;
				Seek(iFilePos+i+2,SEEK_SET);
			}
			else
			{
				// end of line
				szLine[i+1]=0;
				Seek(iFilePos+i+1,SEEK_SET);
			}
			break;
		}
		else if ('\r'==szLine[i])
		{
			if ('\n' == szLine[i+1])
			{
				szLine[i+2]=0;
				Seek(iFilePos+i+2,SEEK_SET);
			}
			else
			{
				// end of line
				szLine[i+1]=0;
				Seek(iFilePos+i+1,SEEK_SET);
			}
			break;
		}
	}
	if (iBytesRead>0) 
	{
		return true;
	}
	return false;
}
