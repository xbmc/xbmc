#include "../stdafx.h"
#include "FileRar.h"
#include <sys/stat.h>
#include "../util.h"
#include <process.h>

using namespace XFILE;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

//*********************************************************************************************

CFileRarExtractThread::CFileRarExtractThread()
{
  m_pArc = NULL;
  m_pCmd = NULL;
  m_pExtract = NULL;
  hRunning = CreateEvent(NULL,true,false,NULL);
  hRestart = CreateEvent(NULL,false,false,NULL);
  hQuit = CreateEvent(NULL,true,false,NULL);
  StopThread();
  Create();
}

CFileRarExtractThread::~CFileRarExtractThread()
{
  SetEvent(hQuit);
  WaitForSingleObject(hRestart,INFINITE);
  StopThread();
  
  CloseHandle(hRunning);
  CloseHandle(hQuit);
  CloseHandle(hRestart);
}

void CFileRarExtractThread::Start(Archive* pArc, CommandData* pCmd, CmdExtract* pExtract, int iSize)
{
  m_pArc = pArc;
  m_pCmd = pCmd;
  m_pExtract = pExtract;
  m_iSize = iSize;
  m_pExtract->GetDataIO().hBufferFilled = CreateEvent(NULL,false,false,NULL);
  m_pExtract->GetDataIO().hBufferEmpty = CreateEvent(NULL,false,false,NULL);
  m_pExtract->GetDataIO().hSeek = CreateEvent(NULL,true,false,NULL);
  m_pExtract->GetDataIO().hSeekDone = CreateEvent(NULL,false,false,NULL);
  m_pExtract->GetDataIO().hQuit = CreateEvent(NULL,true,false,NULL);
  m_pExtract->GetDataIO().hProgressBar = CreateEvent(NULL,true,false,NULL);
  SetEvent(hRunning);
  SetEvent(hRestart);
}

void CFileRarExtractThread::OnStartup()
{
}

void CFileRarExtractThread::OnExit()
{
}

void CFileRarExtractThread::Process()
{
  while (WaitForSingleObject(hQuit,1) != WAIT_OBJECT_0)
    if (WaitForSingleObject(hRestart,1) == WAIT_OBJECT_0)
    {
      bool Repeat = false;
      m_pExtract->ExtractCurrentFile(m_pCmd,*m_pArc,m_iSize,Repeat);
      ResetEvent(hRunning);
    }

  SetEvent(hRestart);
}

CFileRar::CFileRar()
{
	m_strCacheDir.Empty();
	m_strRarPath.Empty();
	m_strPassword.Empty();
	m_strPathInRar.Empty();
	m_bRarOptions = 0;
	m_bFileOptions = 0;
  m_pArc = NULL;
  m_pCmd = NULL;
  m_pExtract = NULL;
  m_pExtractThread = NULL;
  m_szBuffer = NULL;
  m_szStartOfBuffer = NULL;
  m_iDataInBuffer = 0;
  m_bUseFile = false;
  m_bOpen = false;
  m_bSeekable = true;
}

//*********************************************************************************************
CFileRar::~CFileRar()
{
  if (!m_bOpen)
    return;
  
  if (m_bUseFile)
  {
    m_File.Close();
    g_RarManager.ClearCachedFile(m_strRarPath,m_strPathInRar); 
  }
  else
  {
    CleanUp();
    if (m_pExtractThread)
    {
      delete m_pExtractThread;
      m_pExtractThread = NULL;
    }
  }
}
//*********************************************************************************************
bool CFileRar::Open(const CURL& url, bool bBinary)
{
  CStdString strFile; 
  url.GetURL(strFile);
  
  InitFromUrl(url);
  CFileItemList items;
  g_RarManager.GetFilesInRar(items,m_strRarPath,false); // fixme, query manager!
  int i;
  for (i=0;i<items.Size();++i)
    if (items[i]->GetLabel() == m_strPathInRar)
      break;

  if (i<items.Size())
    if (items[i]->m_lStartOffset == 0x30) // stored
    {
      if (!OpenInArchive())
        return false;

      m_iFileSize = items[i]->m_dwSize;
      m_bOpen = true;
      
      // perform 'noidx' check
      CFileInfo* pFile = g_RarManager.GetFileInRar(m_strRarPath,m_strPathInRar);
      if (pFile)
        if (pFile->m_iIsSeekable == -1)
        {
          if (Seek(-1,SEEK_END) == -1)
          {
            m_bSeekable = false;
            pFile->m_iIsSeekable = 0;
          }
        }
        else
         m_bSeekable = (pFile->m_iIsSeekable == 1);

        return true;
    }
    else
    {
      m_bUseFile = true;
      CStdString strPathInCache;
      
      if (!g_RarManager.CacheRarredFile(strPathInCache, m_strRarPath, m_strPathInRar, EXFILE_AUTODELETE, m_strCacheDir, items[i]->m_dwSize))
      {
        CLog::Log(LOGERROR,"filerar::open failed to cache file %s",m_strPathInRar.c_str());
        return false;
      }
      
      if (!m_File.Open( strPathInCache, bBinary)) 
      {
        CLog::Log(LOGERROR,"filerar::open failed to open file in cache: %s",strPathInCache.c_str());
        return false;
      }
      
      m_bOpen = true;
      return true;
    }

  return false;
}

bool CFileRar::Exists(const CURL& url)
{
	InitFromUrl(url);
	CStdString strPathInCache;
	bool bResult;
	
  if (!g_RarManager.IsFileInRar(bResult, m_strRarPath, m_strPathInRar)) 
    return false;
	
  return bResult;
}
//*********************************************************************************************
int CFileRar::Stat(const CURL& url, struct __stat64* buffer)
{
	/*InitFromUrl( url );
	CStdString strPathInCache;
	if( !g_RarManager.CacheRarredFile(strPathInCache, m_strRarPath, m_strPathInRar,  m_bFileOptions,  m_strCacheDir) ) 
		return false;
	CFile file;
	return file.Stat(strPathInCache, buffer);*/
  return -1;
}


//*********************************************************************************************
bool CFileRar::OpenForWrite(const CURL& url, bool bBinary)
{
/*	InitFromUrl( url );
	CStdString strPathInCache;
	if( !g_RarManager.CacheRarredFile(strPathInCache, m_strRarPath, m_strPathInRar,  m_bFileOptions,  m_strCacheDir) ) */
		return false;
	/*if(!m_File.OpenForWrite(strPathInCache, bBinary))  return false;
	return true;*/
}

//*********************************************************************************************
unsigned int CFileRar::Read(void *lpBuf, __int64 uiBufSize)
{
  if (!m_bOpen)
    return 0;
  
  if (m_bUseFile)
    return m_File.Read(lpBuf,uiBufSize);
  
  if (m_iFilePosition >= GetLength()) // we are done
    return 0;
  
  WaitForSingleObject(m_pExtract->GetDataIO().hBufferEmpty,INFINITE);
  byte* pBuf = (byte*)lpBuf;
  __int64 uicBufSize = uiBufSize;
  if (m_iDataInBuffer > 0)
  {
    __int64 iCopy = uiBufSize<m_iDataInBuffer?uiBufSize:m_iDataInBuffer;
    memcpy(lpBuf,m_szStartOfBuffer,size_t(iCopy));
    m_szStartOfBuffer += iCopy;
    m_iDataInBuffer -= int(iCopy);
    pBuf += iCopy;
    uicBufSize -= iCopy;
    m_iFilePosition += iCopy;
  }
  while ((uicBufSize > 0) && m_iFilePosition < GetLength() )
  {
    if (m_iDataInBuffer <= 0)
    {
      m_pExtract->GetDataIO().SetUnpackToMemory(m_szBuffer,MAXWINMEMSIZE);
      m_szStartOfBuffer = m_szBuffer;
      m_iBufferStart = m_iFilePosition;
    }
    
    SetEvent(m_pExtract->GetDataIO().hBufferFilled);
    WaitForSingleObject(m_pExtract->GetDataIO().hBufferEmpty,INFINITE);
    
    if (m_pExtract->GetDataIO().NextVolumeMissing)
      break;

    m_iDataInBuffer = MAXWINMEMSIZE-m_pExtract->GetDataIO().UnpackToMemorySize;
    
    if (m_iDataInBuffer == 0)
      break;
    
    if (m_iDataInBuffer > uicBufSize)
    {
      memcpy(pBuf,m_szStartOfBuffer,int(uicBufSize));
      m_szStartOfBuffer += uicBufSize;
      pBuf += int(uicBufSize);
      m_iFilePosition += uicBufSize;
      m_iDataInBuffer -= int(uicBufSize);
      uicBufSize = 0;
    }
    else
    {
      memcpy(pBuf,m_szStartOfBuffer,size_t(m_iDataInBuffer));
      m_iFilePosition += m_iDataInBuffer;
      m_szStartOfBuffer += m_iDataInBuffer;
      uicBufSize -= m_iDataInBuffer;
      pBuf += m_iDataInBuffer;
      m_iDataInBuffer = 0;
    }
  }
  SetEvent(m_pExtract->GetDataIO().hBufferEmpty);
  
  return static_cast<unsigned int>(uiBufSize-uicBufSize);
}

//*********************************************************************************************
unsigned int CFileRar::Write(void *lpBuf, __int64 uiBufSize)
{
	//return m_File.Write(lpBuf, uiBufSize);
  return 0;
}

//*********************************************************************************************
void CFileRar::Close()
{
  if (!m_bOpen)
    return;

  if (m_bUseFile)
  {
    m_File.Close();
    g_RarManager.ClearCachedFile(m_strRarPath,m_strPathInRar);
    m_bOpen = false;
  }
  else
  {
    CleanUp();
    if (m_pExtractThread)
    {
      delete m_pExtractThread;
      m_pExtractThread = NULL;
    }
    m_bOpen = false;
  }
}

//*********************************************************************************************
__int64 CFileRar::Seek(__int64 iFilePosition, int iWhence)
{ 
  if (!m_bOpen)
    return -1;

  if (!m_bSeekable)
    return -1;

  if (m_bUseFile)
    return m_File.Seek(iFilePosition,iWhence);
  
  WaitForSingleObject(m_pExtract->GetDataIO().hBufferEmpty,INFINITE);
  
  SetEvent(m_pExtract->GetDataIO().hBufferEmpty);
  switch (iWhence)
  {
    case SEEK_CUR:
      if (iFilePosition == 0)
        return m_iFilePosition; // happens sometimes

      iFilePosition += m_iFilePosition;
      break;
    case SEEK_END:
      if (iFilePosition == 0) // do not seek to end
      { 
        m_iFilePosition = this->GetLength();
        m_iDataInBuffer = 0;
        m_iBufferStart = this->GetLength();
        return this->GetLength();
      }

      iFilePosition += GetLength();
    default:
      break;
  }
  
  if (iFilePosition > this->GetLength()) 
    return -1;
  
  if (iFilePosition == m_iFilePosition) // happens a lot
    return m_iFilePosition; 
  
  if ((iFilePosition >= m_iBufferStart) && (iFilePosition < m_iBufferStart+MAXWINMEMSIZE) && (m_iDataInBuffer > 0)) // we are within current buffer
  {
    m_iDataInBuffer = MAXWINMEMSIZE-(iFilePosition-m_iBufferStart);
    m_szStartOfBuffer = m_szBuffer+MAXWINMEMSIZE-m_iDataInBuffer;
    m_iFilePosition = iFilePosition;
    
    return m_iFilePosition;
  }
  
  if (iFilePosition < m_iBufferStart )
  {
    CleanUp();
    if (!OpenInArchive())
      return -1;
    
    WaitForSingleObject(m_pExtract->GetDataIO().hBufferEmpty,INFINITE);
    SetEvent(m_pExtract->GetDataIO().hBufferEmpty);
    m_pExtract->GetDataIO().m_iSeekTo = iFilePosition;
  }
  else
    m_pExtract->GetDataIO().m_iSeekTo = iFilePosition;
  
  m_pExtract->GetDataIO().SetUnpackToMemory(m_szBuffer,MAXWINMEMSIZE);
  SetEvent(m_pExtract->GetDataIO().hSeek);
  SetEvent(m_pExtract->GetDataIO().hBufferFilled);
  WaitForSingleObject(m_pExtract->GetDataIO().hSeekDone,INFINITE);

  if (m_pExtract->GetDataIO().NextVolumeMissing)
  {
    m_iFilePosition = m_iFileSize;
    return -1;
  }

  WaitForSingleObject(m_pExtract->GetDataIO().hBufferEmpty,INFINITE);
  m_iDataInBuffer = m_pExtract->GetDataIO().m_iSeekTo; // keep data
  m_iBufferStart = m_pExtract->GetDataIO().m_iStartOfBuffer;
  m_szStartOfBuffer = m_szBuffer+MAXWINMEMSIZE-m_iDataInBuffer;
  m_iFilePosition = iFilePosition;
  
  return m_iFilePosition;
}

//*********************************************************************************************
__int64 CFileRar::GetLength()
{
  if (!m_bOpen)
    return 0;

  if (m_bUseFile)
    return m_File.GetLength();

  return m_iFileSize;
}

//*********************************************************************************************
__int64 CFileRar::GetPosition()
{
  if (!m_bOpen)
    return -1;

	if (m_bUseFile)
    return m_File.GetPosition();

  return m_iFilePosition;
}


//*********************************************************************************************
bool CFileRar::ReadString(char *szLine, int iLineLength)
{
	if (m_bUseFile)
    return m_File.ReadString(szLine,iLineLength);
    
  return 0;
}


int CFileRar::Write(const void* lpBuf, __int64 uiBufSize)
{
  return -1;
}

bool CFileRar::Delete(const char* strFileName)
{
	/*//Deletes the cached rar and/or file according to the autodel values. 
	CURL url(strFileName);
	InitFromUrl( url );
	bool bAutoDelFile = (m_bFileOptions & EXFILE_AUTODELETE) !=0;

	CStdString strFileCachedPath, strRarCachedPath;
	g_RarManager.MakeCachedPath(strFileCachedPath, m_strCacheDir, m_strPathInRar);
	g_RarManager.MakeCachedPath(strRarCachedPath, m_strCacheDir, m_strRarPath);
	CFile file;
	bool bRes = true;
	if(bAutoDelFile)
		if( !file.Delete(strFileCachedPath) )	bRes = false;

	return bRes;*/
  return false;
}

bool CFileRar::Rename(const char* strFileName, const char* strNewFileName)
{
	return false;
}

void CFileRar::Flush()
{
	if (m_bUseFile)
    m_File.Flush();
}
void CFileRar::InitFromUrl(const CURL& url)
{
  url.GetURL(m_strUrl);
  m_strCacheDir = g_stSettings.m_szCacheDirectory;//url.GetDomain();
  if (!CUtil::HasSlashAtEnd(m_strCacheDir))
    m_strCacheDir += "/"; // should work local and remote..
	m_strRarPath = url.GetHostName();
	m_strPassword = url.GetPassWord();
	m_strPathInRar = url.GetFileName();
	int iAutoDelMask = url.GetPort();

	m_bRarOptions = 0;

	bool bAutoDelFile = (iAutoDelMask & EXFILE_AUTODELETE) !=0;
	bool bOverwriteFile = (iAutoDelMask & EXFILE_OVERWRITE) !=0;
	m_bFileOptions = 0;
}

void CFileRar::CleanUp()
{
  if (m_pExtractThread)
  {
    if (WaitForSingleObject(m_pExtractThread->hRunning,1) == WAIT_OBJECT_0)
    {
      SetEvent(m_pExtract->GetDataIO().hQuit);
      while (WaitForSingleObject(m_pExtractThread->hRunning,1) == WAIT_OBJECT_0) 
        Sleep(1);
    
    }
    CloseHandle(m_pExtract->GetDataIO().hBufferFilled);
    CloseHandle(m_pExtract->GetDataIO().hBufferEmpty);
    CloseHandle(m_pExtract->GetDataIO().hSeek);
    CloseHandle(m_pExtract->GetDataIO().hSeekDone);
    CloseHandle(m_pExtract->GetDataIO().hQuit);
    CloseHandle(m_pExtract->GetDataIO().hProgressBar);
  }
  if (m_pExtract)
  {
    delete m_pExtract;
    m_pExtract = NULL;
  }
  if (m_pArc)
  {
    delete m_pArc;
    m_pArc = NULL;
  }
  if (m_pCmd)
  {
    delete m_pCmd;
    m_pCmd = NULL;
  }
  if (m_szBuffer)
  {
    delete[] m_szBuffer;
    m_szBuffer = NULL;
    m_szStartOfBuffer = NULL;
  }
}

bool CFileRar::OpenInArchive()
{
  InitCRC();
  // Set the arguments for the extract command
  m_pCmd = new CommandData;
  if (!m_pCmd)
  {
    CleanUp();
    return false;
  }

  strcpy(m_pCmd->Command, "X");
  m_pCmd->AddArcName(const_cast<char*>(m_strRarPath.c_str()),NULL);
  strncpy(m_pCmd->ExtrPath, m_strCacheDir.c_str(), sizeof(m_pCmd->Command) - 2);
  m_pCmd->ExtrPath[sizeof(m_pCmd->Command) - 2] = '\0';
  AddEndSlash(m_pCmd->ExtrPath);
  m_pCmd->FileArgs->AddString(m_strPathInRar.c_str());

  // Set password for encrypted archives
  if ((m_strPassword.size() > 0) && (m_strPassword.size() < 128))
  {
    strncpy(m_pCmd->Password, m_strPassword.c_str(),m_strPassword.size());
    m_pCmd->Password[m_strPassword.size()] = '\0';
  }

  // Open the archive
  m_pArc = new Archive(m_pCmd);
  if (!m_pArc)
  {
    CleanUp();
    return false;
  }
  if (!m_pArc->WOpen(m_strRarPath.c_str(),NULL))
  {
    CleanUp();
    return false;
  }
  if (!(m_pArc->IsOpened() && m_pArc->IsArchive(true)))
  {
    CleanUp();
    return false;
  }

  m_pExtract = new CmdExtract;
  if (!m_pExtract)
  {
    CleanUp();
    return false;
  }
  m_pExtract->GetDataIO().SetUnpackToMemory(m_szBuffer,0);
  m_pExtract->GetDataIO().SetCurrentCommand(*(m_pCmd->Command));
  struct FindData FD;
  if (FindFile::FastFind(m_strRarPath.c_str(),NULL,&FD))
    m_pExtract->GetDataIO().TotalArcSize+=FD.Size;
  m_pExtract->ExtractArchiveInit(m_pCmd,*m_pArc);
  bool bRes = false;
  while(1)
  {
   m_iSize=m_pArc->ReadHeader();
    if (stricmp(m_pArc->NewLhd.FileName,m_strPathInRar.c_str()) == 0)
    {
      bRes = true;
      break;
    }
    bool Repeat=false;
    if (!m_pExtract->ExtractCurrentFile(m_pCmd,*m_pArc,m_iSize,Repeat)) // this does NO extraction, only skips and handles solid volumes
    {
      bRes = FALSE;
      break;
    }
  }
  if (!bRes)
  {
    CleanUp();
    return false;
  }
  
  m_szBuffer = new byte[MAXWINMEMSIZE];
  m_szStartOfBuffer = m_szBuffer;
  m_pExtract->GetDataIO().SetUnpackToMemory(m_szBuffer,0);
  m_iDataInBuffer = -1;
  m_iFilePosition = 0;
  m_iBufferStart = 0;
  
  if (m_pExtractThread)
    delete m_pExtractThread;
  m_pExtractThread = new CFileRarExtractThread();
  m_pExtractThread->Start(m_pArc,m_pCmd,m_pExtract,m_iSize);
  
  return true;
}