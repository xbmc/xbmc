// FileSmb.h: interface for the CFileSMB class.

//

//////////////////////////////////////////////////////////////////////



#if !defined(AFX_FILESMB_H__2C4AB5BC_0742_458D_95EA_E9C77BA5663D__INCLUDED_)

#define AFX_FILESMB_H__2C4AB5BC_0742_458D_95EA_E9C77BA5663D__INCLUDED_


#if _MSC_VER > 1000

#pragma once

#endif // _MSC_VER > 1000

#include "IFile.h"
#include "../lib/libsmb/xbLibSmb.h"

class CSMB
{
public:
	CSMB();
	~CSMB();
	void Init();
	void Purge();
	void PurgeEx(const CURL& url);
	void Lock();
	void Unlock();
private:
	bool							binitialized;
	CRITICAL_SECTION	m_critSection;
	CStdString				m_strLastHost;
	CStdString				m_strLastShare;
};

extern CSMB smb;

using namespace XFILE;

namespace XFILE
{
	class CFileSMB : public IFile  
	{
	public:
		CFileSMB();
		virtual ~CFileSMB();
		virtual void					Close();
		virtual __int64				Seek(__int64 iFilePosition, int iWhence=SEEK_SET);
		virtual bool					ReadString(char *szLine, int iLineLength);
		virtual unsigned int	Read(void* lpBuf, __int64 uiBufSize);
		virtual bool					Open(const CURL& url, bool bBinary=true);
		virtual bool					Exists(const CURL& url);
		virtual int						Stat(const CURL& url, struct __stat64* buffer);
		virtual __int64				GetLength();
		virtual __int64				GetPosition();
		virtual int						Write(const void* lpBuf, __int64 uiBufSize);
		
		virtual bool          OpenForWrite(const CURL& url, bool bBinary = true);
	  virtual bool          Delete(const char* strFileName);
    virtual bool          Rename(const char* strFileName, const char* strNewFileName);
    
	protected:
	  bool                  IsValidFile(const CStdString& strFileName);
		__int64								m_fileSize;
		bool									m_bBinary;
		int										m_fd;
	};
};

#endif // !defined(AFX_FILESMB_H__2C4AB5BC_0742_458D_95EA_E9C77BA5663D__INCLUDED_)
