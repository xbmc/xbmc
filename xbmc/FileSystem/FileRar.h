// FileRar.h: interface for the CFileRar class.
//
//////////////////////////////////////////////////////////////////////
#if !defined(AFX_FILERAR_H__C6E9401A_3715_11D9_8185_0050FC718317__INCLUDED_)
#define AFX_FILERAR_H__C6E9401A_3715_11D9_8185_0050FC718317__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "IFile.h"
#include "File.h"
#include "RarManager.h"
#include "../lib/UnrarXLib/rar.hpp"
#include "../utils/Thread.h"

namespace XFILE
{	
  class CFileRarExtractThread : public CThread
  {
  public:
    CFileRarExtractThread();
    ~CFileRarExtractThread();
    
    void Start(Archive* pArc, CommandData* pCmd, CmdExtract* pExtract, int iSize); 
    
    virtual void OnStartup();
     virtual void OnExit();
     virtual void Process();

     HANDLE hRunning;
     HANDLE hRestart;
     HANDLE hQuit;
  protected:
    Archive* m_pArc;
    CommandData* m_pCmd;
    CmdExtract* m_pExtract;
    int m_iSize;
  };
  
  class CFileRar : public IFile  
	{
	public:
		CFileRar();
    CFileRar(bool bSeekable); // used for caching files
		virtual ~CFileRar();
		virtual __int64			  GetPosition();
		virtual __int64			  GetLength();
		virtual bool					Open(const CURL& url, bool bBinary=true);
		virtual bool					Exists(const CURL& url);
		virtual int						Stat(const CURL& url, struct __stat64* buffer);
		virtual unsigned int	Read(void* lpBuf, __int64 uiBufSize);
		virtual int						Write(const void* lpBuf, __int64 uiBufSize);
		virtual __int64			  Seek(__int64 iFilePosition, int iWhence=SEEK_SET);
    virtual void					Close();
		virtual void          Flush();

		virtual bool					OpenForWrite(const CURL& url, bool bBinary=true);
		unsigned int					Write(void *lpBuf, __int64 uiBufSize);
		
	protected:
		CStdString	m_strCacheDir;
		CStdString	m_strRarPath;
		CStdString m_strPassword;
		CStdString m_strPathInRar;
    CStdString m_strUrl;
		BYTE m_bRarOptions;
		BYTE m_bFileOptions;
    void Init();
		void InitFromUrl(const CURL& url);
    bool OpenInArchive();
    void CleanUp();
    
    __int64 m_iFilePosition;
    __int64 m_iFileSize;
    // rar stuff
    bool m_bUseFile;
    bool m_bOpen;
    bool m_bSeekable;
    CFile m_File; // for packed source
    Archive* m_pArc;
    CommandData* m_pCmd;
    CmdExtract* m_pExtract;
    int m_iSize; // header size
    CFileRarExtractThread* m_pExtractThread;
    byte* m_szBuffer;
    byte* m_szStartOfBuffer;
    __int64 m_iDataInBuffer;
    __int64 m_iBufferStart;
	};

}
#endif // !defined(AFX_FILERAR_H__C6E9401A_3715_11D9_8185_0050FC718317__INCLUDED_)
