#include "stdafx.h" 

#include "FileCache.h"
#include "utils/Thread.h"
#include "Util.h"
#include "File.h"

using namespace XFILE;

#define READ_CACHE_CHUNK_SIZE (64*1024)

// when buffering we will wait for BUFFER_BYTES amount of bytes. need to experiment about the size and
// in general implement better buffering mechanism. 
#define BUFFER_BYTES (1048576/2)

CFileCache::CFileCache() : m_bDeleteCache(false)
{
	m_pCache = new CSimpleFileCache;
}

CFileCache::CFileCache(CCacheStrategy *pCache, bool bDeleteCache) : 
			m_pCache(pCache), m_bDeleteCache(bDeleteCache)
{
}

CFileCache::~CFileCache()
{
 	Close();

	if (m_bDeleteCache && m_pCache)
		delete m_pCache;

	m_pCache = NULL;
}

void CFileCache::SetCacheStrategy(CCacheStrategy *pCache, bool bDeleteCache) 
{
	if (m_bDeleteCache && m_pCache)
		delete m_pCache;

	m_pCache = pCache;
	m_bDeleteCache = bDeleteCache;
}

IFile *CFileCache::GetFileImp() {
	return m_source.GetImplemenation();
}

bool CFileCache::Open(const CURL& url, bool bBinary)
{
	Close();
 
	CLog::Log(LOGDEBUG,"CFileCache::Open - opening <%s> using cache", url.GetFileName().c_str());

	if (!m_pCache) {
		CLog::Log(LOGERROR,"CFileCache::Open - no cache strategy defined");
		return false;
	}

	url.GetURL(m_sourcePath);

	if (m_pCache->Open() != CACHE_RC_OK) {
		CLog::Log(LOGERROR,"CFileCache::Open - failed to open cache");
		Close();
		return false;
	}

	CThread::Create(false);

	return true;
}

bool CFileCache::Attach(IFile *pFile) {
	if (!pFile || !m_pCache)
		return false;

	m_source.Attach(pFile);

	if (m_pCache->Open() != CACHE_RC_OK) {
		CLog::Log(LOGERROR,"CFileCache::Attach - failed to open cache");
		Close();
		return false;
	}

	CThread::Create(false);

	return true;
}

void CFileCache::Process() {	
	char   buf[READ_CACHE_CHUNK_SIZE];
	int    iRead=0;

	if (!m_pCache) {
		CLog::Log(LOGERROR,"CFileCache::Process - sanity failed. no cache strategy");
		return;
	}

	// opening the source file.
	// this better be done in this thread since its the same thread that will do the reading.
	// e.g. libcurl do not like sharing handles across thread boundaries.
	// empty source path string means a open handle was attached so no open required (ugly).
	if(!m_sourcePath.IsEmpty() && !m_source.Open(m_sourcePath, true)) {
		CLog::Log(LOGERROR,"CFileCache::Process - failed to open source <%s>", m_sourcePath.c_str());
		return ;
	}

	BOOL bError=FALSE;
	while(!m_bStop && !bError)
	{
		iRead = m_source.Read(buf, READ_CACHE_CHUNK_SIZE);
		if(iRead == 0)
		{
			CLog::Log(LOGINFO, "CFileCache::Process - Hit eof, exiting");
			break;
		}
		else if (iRead < 0)
			bError = TRUE;

		DWORD iTotalWrite=0;
		while (!m_bStop && iTotalWrite < iRead) {
			int iWrite = m_pCache->WriteToCache(buf+iTotalWrite, iRead - iTotalWrite);
			// write should always work. all handling of buffering and errors should be
			// done inside the cache strategy. only if unrecoverable error happened, WriteToCache would return error and we break.
			if (iWrite < 0) {
				CLog::Log(LOGERROR,"CFileCache::Process - error writing to cache");
				bError = TRUE;
				break;
			}

			iTotalWrite += iWrite;
		}
	}

	m_pCache->EndOfInput();
	
}

bool CFileCache::Exists(const CURL& url)
{
	CStdString strPath;
	url.GetURL(strPath);
	return CFile::Exists(strPath);
}

int CFileCache::Stat(const CURL& url, struct __stat64* buffer)
{
	CStdString strPath;
	url.GetURL(strPath);
	return CFile::Stat(strPath, buffer);
}

unsigned int CFileCache::Read(void* lpBuf, __int64 uiBufSize)
{
	if (!m_pCache) {
		CLog::Log(LOGERROR,"CFileCache::Read - sanity failed. no cache strategy!");
		return 0;
	}

	unsigned int uiBytes = 0;
	int iRc = m_pCache->ReadFromCache((char *)lpBuf, (size_t)uiBufSize);
	if (iRc == CACHE_RC_WOULD_BLOCK) {
		// we dont have enough data to read - start buffering.
		// buffering is simply waiting for enough data to be available (hardcoded amount for now).
		// TODO: smarted implementations of buffering
		CLog::Log(LOGDEBUG,"CFileCache::Read - not enough data. buffering till %d bytes available", BUFFER_BYTES);
		if (m_pCache->WaitForData(BUFFER_BYTES, 10000) < 1024)  { // if after 10 seconds there is less than 1k available will close the connection as eof.
			CLog::Log(LOGWARNING,"CFileCache::Read - stream broke.");
			return 0;
		}

		iRc = m_pCache->ReadFromCache((char *)lpBuf, (size_t)uiBufSize);

	}
	
	if (iRc > 0)
		uiBytes = iRc;

	return uiBytes;
}

__int64 CFileCache::Seek(__int64 iFilePosition, int iWhence)
{  
	if (!m_pCache) {
		CLog::Log(LOGERROR,"CFileCache::Seek- sanity failed. no cache strategy!");
		return -1;
	}

	__int64 iCurPos = m_pCache->GetPosition();
	__int64 iTarget = iFilePosition;
	if (iWhence == SEEK_END)
		iTarget = GetLength() + iTarget;
	else if (iWhence == SEEK_CUR)
		iTarget = iCurPos + iTarget;

	__int64 iDiff = iTarget - iCurPos;
	return m_pCache->Seek(iFilePosition, iWhence);
}

void CFileCache::Close()
{
	StopThread();

	if (m_pCache)
		m_pCache->Close();
	
	m_source.Close();
}

__int64 CFileCache::GetPosition()
{
	if (!m_pCache) {
		CLog::Log(LOGERROR,"CFileCache::GetPosition- sanity failed. no cache strategy!");
		return -1;
	}

	return m_pCache->GetPosition();
}

__int64 CFileCache::GetLength()
{
	return m_source.GetLength();
}
