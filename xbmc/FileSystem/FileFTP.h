#pragma once
#include "IFile.h"
#include "FTPUtil.h"
#include "../AutoPtrHandle.h"
using namespace XFILE;
using namespace AUTOPTR;

namespace XFILE
{
	class CFileFTP : public IFile  
	{
    public:
	    CFileFTP();
	    virtual ~CFileFTP();
	    virtual bool Open(const CURL& url, bool bBinary = true);
	    virtual bool Exists(const CURL& url);
      virtual bool Open(const char* strUserName, const char* strPassword,const char* strHostName, const char* strFileName,int iport, bool bBinary=true);
	    virtual bool Exists(const char* strUserName, const char* strPassword,const char* strHostName, const char* strFileName,int iport);
	    virtual __int64	Seek(__int64 iFilePosition, int iWhence=SEEK_SET);
      virtual __int64 GetPosition();
	    virtual __int64	GetLength();
      virtual int	Stat(const CURL& url, struct __stat64* buffer);
	    virtual void Close();
      virtual unsigned int Read(void* lpBuf, __int64 uiBufSize);
    protected:
	    UINT64					m_fileSize ;
	    UINT64					m_filePos;
	    char m_filename[255];
    private:
	    bool m_bOpened;
      CAutoPtrSocket rs;
	    __int64 Recv(byte* pBuffer,__int64 iLen);
	    CFTPUtil FTPUtil;
	};
};