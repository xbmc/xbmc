/*
 * DAAP Support for XBox Media Center
 * Copyright (c) 2004 Forza (Chris Barnett)
 * Portions Copyright (c) by the authors of libOpenDAAP
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

#include "stdafx.h"
#include "FileDAAP.h"
#include "../sectionLoader.h"
#include "../util.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>

static UINT64 strtouint64(const char *s)
{
	UINT64 r = 0;

	while ((*s != 0) && (isspace(*s)))
		s++;
	if (*s == '+')
		s++;
	while ((*s != 0) && (isdigit(*s)))
	{
		r = r * ((UINT64)10);
		r += ((UINT64)(*s)) - ((UINT64)'0');
		s++;
	}
	return r;
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CFileDAAP::CFileDAAP()
{
	CSectionLoader::Load("LIBXDAAP");
	m_fileSize = 0;
	m_filePos = 0;
	m_song.size = 0;
	m_song.data = NULL;
	m_bOpened=false;
	m_thisHost = NULL;
	m_thisClient = NULL;
}

CFileDAAP::~CFileDAAP()
{
	DestroyDAAP();
	CSectionLoader::Unload("LIBXDAAP");
}

void CFileDAAP::DestroyDAAP()
{
	Close();
}

//*********************************************************************************************
bool CFileDAAP::Open(const CURL& url, bool bBinary)
{
	const char* strUserName = url.GetUserName().c_str();
	const char* strPassword = url.GetPassWord().c_str();
	const char* strHostName = url.GetHostName().c_str();
	const char* strFileName = url.GetFileName().c_str();
	const char* strFileFormat = url.GetFileType().c_str();
	
	strncpy(m_strFileFormat, strFileFormat, 16);
	// only able to open mp3's or m4a's ...
	if (url.GetFileType() != "mp3" && url.GetFileType() != "m4a") return(false);
	// not protected drm'd iTunes songs yet
	//	&& url.GetFileType() != "m4p") return(false);
	if (url.GetFileName().length() == 0) return(false);
	
	if (m_bOpened) Close();
  
	m_bOpened = false;
	m_song.size = 0;
	m_filePos = 0;
	m_fileSize = 0;

	//int fileID;
	//int dbID;
	char szFormat[120];
	sscanf(strFileName,"%i.%s", &m_iFileID, szFormat);
	//dbID = 0x22;
	
	OutputDebugString("daap:open:");
	OutputDebugString(strFileName);

	// If we already have a song open, use that instead!
	if (g_application.m_DAAPSong)
	{
		m_bOpened = true;
		m_song.size = (int) g_application.m_DAAPSongSize;
		m_song.data = malloc(m_song.size);
		memcpy(m_song.data, g_application.m_DAAPSong, m_song.size);
		m_filePos = 0;
		m_fileSize = m_song.size;
		OutputDebugString(" (cached)\n");
		return true;
	}

	OutputDebugString("\n");

	if (g_application.m_DAAPPtr)
	{
		m_thisClient = (DAAP_SClient *) g_application.m_DAAPPtr;
		m_thisHost = m_thisClient->hosts;
	}
	else
	{
		// Create a client object if we don't already have one
		if (!m_thisClient)
			m_thisClient = DAAP_Client_Create(NULL, NULL);
		g_application.m_DAAPPtr = m_thisClient;
	}

	// Add the defined host to the client object if we don't already have one
	if (!m_thisHost)
	{
		m_thisHost = DAAP_Client_AddHost(m_thisClient, (char *) url.GetHostName().c_str(), "A", "A");

		// If no host object returned then the connection failed
		if (!m_thisHost) return false;		// tidy ups?

		if (DAAP_ClientHost_Connect(m_thisHost) < 0) return false;		// tidy ups?
	}

	/*
	if (m_thisHost)
	{	
		//if (DAAP_ClientHost_GetAudioFile(m_thisHost, g_application.m_DAAPDBID, fileID, (char *) strFileFormat, &m_song) < 0)
		if (DAAP_ClientHost_GetAudioFileAsync(m_thisHost, g_application.m_DAAPDBID, fileID, (char *) strFileFormat, &m_song) < 0)
		{
			DestroyDAAP();
			return false;
		}

		// sleep for a moment to allow the thread to start
		Sleep(100);
	}


	// if the stream thread is active wait until
	// we have the intended size of the file ...
	while(GetStreamThreadStatus() && m_song.size == 0)
	{
		Sleep(100);
	}

	m_fileSize = m_song.size;
	g_application.m_DAAPSongSize = m_song.size;
	g_application.m_DAAPSong = m_song.data;
	*/

	// don't start streaming yet, we'll only do that once 
	// part of the file is actually required.
	m_bStreaming = false;
	m_bOpened = true;
	return true;
}

bool CFileDAAP::StartAudioStream()
{
	if (m_thisHost)
	{	
		//if (DAAP_ClientHost_GetAudioFile(m_thisHost, g_application.m_DAAPDBID, fileID, (char *) strFileFormat, &m_song) < 0)
		if (DAAP_ClientHost_GetAudioFileAsync(m_thisHost, g_application.m_DAAPDBID, m_iFileID, (char *) m_strFileFormat, &m_song) < 0)
		{
			DestroyDAAP();
			return false;
		}

		// sleep for a moment to allow the thread to start
		Sleep(100);
	}


	// if the stream thread is active wait until
	// we have the intended size of the file ...
	while(GetStreamThreadStatus() && m_song.size == 0)
	{
		Sleep(100);
	}

	// wait until we have at least 250kb of this file, or all of it if smaller
	while(GetStreamThreadStatus() && m_song.streamlen < 256000 && m_song.streamlen < m_song.size)
	{
		Sleep(100);
	}

	m_fileSize = m_song.size;
	g_application.m_DAAPSongSize = m_song.size;
	g_application.m_DAAPSong = m_song.data;
	m_bOpened = true;
	m_bStreaming = true;
	return true;
}

bool CFileDAAP::Exists(const CURL& url)
{
	
	bool exist(true);
	//exist=CFileDAAP::Open(url, true);
	//Close();
	return exist;
}

int CFileDAAP::Stat(const CURL& url, struct __stat64* buffer)
{
	/*
	if (Open(url, true))
	{
		buffer->st_size = this->m_fileSize;
		buffer->st_mode = _S_IFREG;
		Close();
	}
	errno = ENOENT;
	*/
	return -1;
}

//*********************************************************************************************
unsigned int CFileDAAP::Read(void *lpBuf, __int64 uiBufSize)
{
	unsigned char *buf;
	//char* pBuffer=(char* )lpBuf;
	size_t buflen;

	if (!m_bOpened) return 0;

	if (!m_bStreaming) StartAudioStream();

	// check to see we have enough data in the stream buffer
	if ((m_filePos + uiBufSize) > m_song.streamlen) return 0;

	buf = (unsigned char *) m_song.data;
	buf += m_filePos;

	if (uiBufSize > (__int64) (m_song.size - m_filePos))
		buflen = (size_t) (m_song.size - m_filePos);
	else
		buflen = (size_t) uiBufSize;

	fast_memcpy(lpBuf, buf, buflen);
	//pBuffer[uiBufSize] = 0x00;
	m_filePos += buflen;
	return buflen;
}

//*********************************************************************************************
void CFileDAAP::Close()
{
	if (m_bOpened) 
	{
		OutputDebugString("daap:close:\n");

		// if we are still streaming we need to wait until
		// we've finished. Not ideal, but iTunes does NOT
		// like having it's stream cut mid-flow :)
		if(m_bStreaming)
		{
			while(GetStreamThreadStatus())
			{
				Sleep(100);
			}
		}

		/*
		if (GetStreamThreadStatus())
		{
			DAAP_ClientHost_StopAudioFileAsync(m_thisHost);
			if (m_thisClient) DAAP_Client_Release(m_thisClient);
		}
		*/

		if (m_song.data) free(m_song.data);
		m_song.size = 0;
		m_fileSize = 0;

		//if (m_thisClient) DAAP_Client_Release(m_thisClient);
		m_thisHost = NULL;
		m_thisClient = NULL;
		g_application.m_DAAPSong = NULL;
	}
	m_bOpened=false;
}

//*********************************************************************************************
__int64 CFileDAAP::Seek(__int64 iFilePosition, int iWhence)
{
	UINT64 newpos;

	if (!m_bOpened) return -1;
	if (!m_bStreaming) StartAudioStream();

	switch(iWhence) 
	{
		case SEEK_SET:
			// cur = pos
			newpos = iFilePosition;
			break;
		case SEEK_CUR:
			// cur += pos
			newpos = m_filePos + iFilePosition;
			break;
		case SEEK_END:
			// end -= pos
			newpos = m_song.size - iFilePosition;
			break;
	}
	if (newpos < 0)       		newpos = 0;
	if (newpos >= m_song.size)	newpos = m_song.size;

	// if we try to seek beyond the data we have streamed so far
	// we should return with error (?)
	if (newpos > m_song.streamlen) return -1;
	
	m_filePos = newpos;
	return m_filePos;
}

//*********************************************************************************************
__int64 CFileDAAP::GetLength()
{
	if (!m_bOpened) return 0;
	return m_song.size;
}

//*********************************************************************************************
__int64 CFileDAAP::GetPosition()
{
	if (!m_bOpened) return 0;
	return m_filePos;
}


//*********************************************************************************************
bool CFileDAAP::ReadString(char *szLine, int iLineLength)
{
	if (!m_bOpened) return false;
	__int64 iFilePos=GetPosition();

	int iBytesRead=Read( (unsigned char*)szLine, iLineLength);
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
