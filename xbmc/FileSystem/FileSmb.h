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
	void Lock();
	void Unlock();
private:
	bool binitialized;
	CRITICAL_SECTION	m_critSection;
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
		int										Write(const void* lpBuf, __int64 uiBufSize);
	protected:
		__int64								m_fileSize;
		bool									m_bBinary;
		int										m_fd;
	};
};

#endif // !defined(AFX_FILESMB_H__2C4AB5BC_0742_458D_95EA_E9C77BA5663D__INCLUDED_)
