//
// C++ Implementation: CacheStrategy
//
// Description: 
//
//
// Author: Team XBMC <>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "stdafx.h"
#include "CacheStrategy.h"
#ifdef _LINUX
#include "PlatformInclude.h"
#endif
#include "Util.h"
#include "../utils/log.h"
#include "../utils/SingleLock.h"

namespace XFILE {

CCacheStrategy::CCacheStrategy() : m_bEndOfInput(false)
{
}


CCacheStrategy::~CCacheStrategy()
{
}

void CCacheStrategy::EndOfInput() {
	m_bEndOfInput = true;
}

CSimpleFileCache::CSimpleFileCache() : m_hCacheFileRead(NULL), m_hCacheFileWrite(NULL), m_hDataAvailEvent(NULL) {
}

CSimpleFileCache::~CSimpleFileCache() {
	Close();
}

int CSimpleFileCache::Open() {
	Close();

	m_hDataAvailEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	CStdString fileName = CUtil::GetNextFilename(_P("Z:\\filecache%03d.cache"), 999);
	if(fileName.empty())
	{
		CLog::Log(LOGERROR, CStdString(__FUNCTION__) + " - Unable to generate a new filename");
		Close();
		return CACHE_RC_ERROR;
	}
	
	m_hCacheFileWrite = CreateFile(fileName.c_str()
						, GENERIC_WRITE, FILE_SHARE_READ
						, NULL
						, CREATE_ALWAYS
						, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING
						,NULL);

	if(m_hCacheFileWrite == INVALID_HANDLE_VALUE)
	{
		CLog::Log(LOGERROR, CStdString(__FUNCTION__)+" - failed to create file %s with error code %d", fileName.c_str(), GetLastError());
		Close();
		return CACHE_RC_ERROR;
	}

	m_hCacheFileRead = CreateFile(fileName.c_str()
						, GENERIC_READ, 0
						, NULL
						, OPEN_EXISTING
						, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE 
						, NULL);

	if(m_hCacheFileRead == INVALID_HANDLE_VALUE)
	{
		CLog::Log(LOGERROR, CStdString(__FUNCTION__)+" - failed to open file %s with error code %d", fileName.c_str(), GetLastError());
		Close();
		return CACHE_RC_ERROR;
	}

	return CACHE_RC_OK;
}

int CSimpleFileCache::Close() {
	CSingleLock lock(m_sync);

	if (m_hDataAvailEvent)
		CloseHandle(m_hDataAvailEvent);

	m_hDataAvailEvent = NULL;

	if (m_hCacheFileWrite)
		CloseHandle(m_hCacheFileWrite);

	m_hCacheFileWrite = NULL;

	if (m_hCacheFileRead)
		CloseHandle(m_hCacheFileRead);

	m_hCacheFileRead = NULL;

	return CACHE_RC_OK;
}

int CSimpleFileCache::WriteToCache(const char *pBuffer, size_t iSize) {
	DWORD iWritten=0;
	if (!WriteFile(m_hCacheFileWrite, pBuffer, iSize, &iWritten, NULL)) {
		CLog::Log(LOGERROR, CStdString(__FUNCTION__)+" - failed to write to file. err: %d",GetLastError());
		return CACHE_RC_ERROR;
	}
	
	//CLog::Log(LOGDEBUG,"CSimpleFileCache::WriteToCache. wrote %d bytes", iWritten);

	// when reader waits for data it will wait on the event.
	// it will reset it prior to the wait.
	// there is a possible race condition here - but the reader will worse case wait 1 second, 
	// so rather not lock (performance)
	SetEvent(m_hDataAvailEvent);

	return iWritten;
}

__int64 CSimpleFileCache::GetAvailableRead() {
	LARGE_INTEGER posRead = {}, posWrite = {};
	
	if(!SetFilePointerEx(m_hCacheFileRead, posRead, &posRead, FILE_CURRENT)) {
		return CACHE_RC_ERROR;
	}
	
	if(!SetFilePointerEx(m_hCacheFileWrite, posWrite, &posWrite, FILE_CURRENT)) {
		return CACHE_RC_ERROR;
	}
	
	return posWrite.QuadPart - posRead.QuadPart;
}

int CSimpleFileCache::ReadFromCache(char *pBuffer, size_t iMaxSize) {
	__int64 iAvailable = GetAvailableRead(); 
	if ( iAvailable < 0 ) {
		CLog::Log(LOGWARNING,"CSimpleFileCache::ReadFromCache - no available data");
		return CACHE_RC_ERROR;
	}
	
	if ( iAvailable == 0 ) {
		//CLog::Log(LOGDEBUG,"CSimpleFileCache::ReadFromCache - 0 available bytes. eof: %d", m_bEndOfInput);
		return m_bEndOfInput?CACHE_RC_EOF : CACHE_RC_WOULD_BLOCK;
	}

	DWORD iRead = 0;
	if (!ReadFile(m_hCacheFileRead, pBuffer, iMaxSize, &iRead, NULL)) {
		CLog::Log(LOGERROR,"CSimpleFileCache::ReadFromCache - failed to read %d bytes.", iMaxSize);
		return CACHE_RC_ERROR;
	}

	//CLog::Log(LOGDEBUG,"CSimpleFileCache::ReadFromCache. read %d bytes", iRead);
	return iRead;
}

__int64 CSimpleFileCache::WaitForData(unsigned int iMinAvail, unsigned int iMillis) {
	
	DWORD tmStart = timeGetTime();
	__int64 iAvail = 0;

	while ( iAvail < iMinAvail && timeGetTime() - tmStart < iMillis) {

		// possible race condition here:
		// if writer will set the event between the availability check and reset - 
		// we will hang. but only for 1 second, so better not take the performance hit
		if ( (iAvail = GetAvailableRead()) < iMinAvail )
			ResetEvent(m_hDataAvailEvent);
		else {
			return iAvail;
		}

		// busy look (sleep max 1 sec each round)
		DWORD dwRc = WaitForSingleObject(m_hDataAvailEvent, 1000);
		
		if (iAvail >= iMinAvail)
			return iAvail;

		if (dwRc == WAIT_FAILED)
			return CACHE_RC_ERROR;
	}

	return CACHE_RC_TIMEOUT;
}

__int64 CSimpleFileCache::Seek(__int64 iFilePosition, int iWhence) {
	LARGE_INTEGER pos;
	pos.QuadPart = iFilePosition;
	
	// we cant seek to a location not read yet
	// in this case we will return error and reset the read pointer
	LARGE_INTEGER posWrite = {}, posRead = {};	
	if(!SetFilePointerEx(m_hCacheFileWrite, posWrite, &posWrite, FILE_CURRENT))
		return CACHE_RC_ERROR;
	if(!SetFilePointerEx(m_hCacheFileRead, posRead, &posRead, FILE_CURRENT))
		return CACHE_RC_ERROR;

	int iDir = FILE_BEGIN;
	if (SEEK_END == iWhence) 
		iDir = FILE_END;
	else if (SEEK_CUR == iWhence)
		iDir = FILE_CURRENT;
 
	if(!SetFilePointerEx(m_hCacheFileRead, pos, &pos, iDir))
		return CACHE_RC_ERROR;

	if (pos.QuadPart > posWrite.QuadPart) {		
		// wait up to a few seconds for enough data to buffer. if not - return error
		// we dont wait if the seek is too far ahead (over 500k) - may be the case in some file formats (big jump ahead) but there is no point
		// in waiting...
		DWORD dwDiff = (DWORD)(pos.QuadPart - posWrite.QuadPart);
		if (dwDiff > 500000 || WaitForData(dwDiff, 2000) < dwDiff) {
			CLog::Log(LOGWARNING,"CSimpleFileCache::Seek - attempt to seek pass read data (seek to %lld. max: %lld. reset read pointer. (%lld:%lld)", pos.QuadPart, posWrite.QuadPart, iFilePosition, iWhence);
			SetFilePointerEx(m_hCacheFileRead, posRead, NULL, FILE_BEGIN);
			return  CACHE_RC_ERROR;
		}
	}
	
	return pos.QuadPart;
}

__int64 CSimpleFileCache::GetPosition()  {
	LARGE_INTEGER posRead = {};
	
	if(!SetFilePointerEx(m_hCacheFileRead, posRead, &posRead, FILE_CURRENT))
		return CACHE_RC_ERROR;

	return posRead.QuadPart;
}



}
