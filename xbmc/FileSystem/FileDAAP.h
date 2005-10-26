// FileDAAP.h: interface for the CFileDAAP class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_FILEDAAP_H___INCLUDED_)
#define AFX_FILEDAAP_H___INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "../lib/libXDAAP/client.h"

class CDaapClient : public CCriticalSection 
{
public:

  CDaapClient();
  ~CDaapClient();

  DAAP_SClient *m_pClient;
  DAAP_SClientHost* GetHost(const CStdString &srtHost);
  std::map<CStdString, DAAP_SClientHost*> m_mapHosts;
  typedef std::map<CStdString, DAAP_SClientHost*>::iterator ITHOST;

  DAAP_Status m_Status;

  //Buffers
  int m_iDatabase;  
  void *m_pArtistsHead;  

  void Release();

protected:
  static void StatusCallback(DAAP_SClient *pClient, DAAP_Status status, int value, void* pContext);    
};

extern CDaapClient g_DaapClient;


#include "IFile.h"
#include "../cores/paplayer/RingHoldBuffer.h"
using namespace XFILE;

namespace XFILE
{
class CFileDAAP : public IFile
{
public:
  CFileDAAP();
  virtual ~CFileDAAP();
  virtual __int64 GetPosition();
  virtual __int64 GetLength();
  virtual bool Open(const CURL& url, bool bBinary = true);
  virtual bool Exists(const CURL& url);
  virtual int Stat(const CURL& url, struct __stat64* buffer);
  virtual unsigned int Read(void* lpBuf, __int64 uiBufSize);
  virtual bool ReadString(char *szLine, int iLineLength);
  virtual __int64 Seek(__int64 iFilePosition, int iWhence = SEEK_SET);
  virtual void Close();

protected:

  bool StartStreaming();
  bool StopStreaming();

  __int64 m_fileSize; //holds full size
  __int64 m_filePos; //holds current position in file


  DAAP_SClient *m_thisClient;
  DAAP_SClientHost *m_thisHost;
  DAAP_ClientHost_Song m_song;

  bool m_bOpened;
  bool m_bStreaming;

  int m_iFileID;
  char m_sFileFormat[16];
  
  CRingHoldBuffer m_DataBuffer;


  CEvent m_eventCallback;
  CEvent m_eventNewData;
  HANDLE m_hStreamThread;
  bool m_bInterupt;  
  int m_httpHeaderLength;
  int m_httpContentLenght;

  static int DAAPHttpCallback(const char *data, int size, int flag, int contentlength, void* context);
};
};

#endif // !defined(AFX_FILEDAAP_H___INCLUDED_)

