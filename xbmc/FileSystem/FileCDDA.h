#pragma once
#include "ifile.h"
#include "../lib/libcdio/cdio.h"
#include "../utils/SingleLock.h"

namespace XFILE
{
	class CFileCDDA : public IFile
	{
	public:
		CFileCDDA(void);
		virtual ~CFileCDDA(void);
	  virtual bool							Open(const CURL& url, bool bBinary=true);
	  virtual bool							Exists(const CURL& url);
	  virtual int						    Stat(const CURL& url, struct __stat64* buffer);

	  virtual unsigned int			Read(void* lpBuf, __int64 uiBufSize);
		virtual bool							ReadString(char *szLine, int iLineLength) { return false; }
	  virtual __int64					  Seek(__int64 iFilePosition, int iWhence=SEEK_SET);
	  virtual void							Close();
	  virtual __int64					  GetPosition();
	  virtual __int64					  GetLength();
	  virtual char					    GetDirectorySeperator() { return '/'; }

	protected:
						bool							IsValidFile(const CURL& url);
						int								GetTrackNum(const CURL& url);

	protected:
						CdIo_t*						m_pCdIo;
						lsn_t 						m_lsnStart;		//	Start of m_iTrack in logical sector number
						lsn_t 						m_lsnCurrent;	//	Position inside the track in logical sector number
						lsn_t							m_lsnEnd;			//	End of m_iTrack in logical sector number
						CCriticalSection	m_cs;
	};
};
