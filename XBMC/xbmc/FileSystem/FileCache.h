#pragma once
#include "IFile.h"
#include "CacheStrategy.h"
#include "../utils/CriticalSection.h"

namespace XFILE
{	

	class CFileCache : public IFile, public CThread
	{
	public:
		CFileCache();
		CFileCache(CCacheStrategy *pCache, bool bDeleteCache=true);
		virtual ~CFileCache();
		
		void SetCacheStrategy(CCacheStrategy *pCache, bool bDeleteCache=true);

		// CThread methods
		virtual void Process();
		
		// IFIle methods
		virtual bool          Open(const CURL& url, bool bBinary = true);
		virtual bool          Attach(IFile *pFile);
		virtual void          Close();
		virtual bool          Exists(const CURL& url);
		virtual int           Stat(const CURL& url, struct __stat64* buffer);
		
		virtual unsigned int  Read(void* lpBuf, __int64 uiBufSize);
		
		virtual __int64       Seek(__int64 iFilePosition, int iWhence);
		virtual __int64       GetPosition();
		virtual __int64       GetLength();
		
		IFile *GetFileImp();

	private:
		CCacheStrategy *m_pCache;
		bool			m_bDeleteCache;
		CFile			m_source;
		CStdString		m_sourcePath;
		CEvent			m_seekEvent;
		CEvent			m_seekEnded;
		__int64			m_nSeekResult;
		__int64			m_seekPos;
		__int64			m_readPos;
	    CCriticalSection m_sync;
	};

}
