#include "../stdafx.h"
#include "FileFTP.h"
#include "FTPParse.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/Stat.h>


CFileFTP::~CFileFTP() 
{ 
  Close(); 
}
CFileFTP::CFileFTP():rs(INVALID_SOCKET)
{
	m_filePos=0;
	m_fileSize=0;
	m_bOpened=false;
}
void CFileFTP::Close()
{
  m_bOpened=false;
	m_filePos=0;
	m_fileSize=0;
	FTPUtil.closesockets();
  rs.release();
}
bool CFileFTP::Exists(const char* strUserName, const char* strPassword,const char* strHostName, const char* strFileName,int iport)
{
	bool exist(true);
	exist = CFileFTP::Open(strUserName, strPassword, strHostName, strFileName, iport, true);
	Close();
	return exist;
}
bool CFileFTP::Open(const CURL& url, bool bBinary)
{
  return Open(url.GetUserName(), url.GetPassWord(), url.GetHostName(), url.GetFileName(), url.GetPort(), bBinary);
}
bool CFileFTP::Open(const char* strUserName, const char* strPassword,const char* strHostName, const char* strFileName,int iport, bool bBinary)
{
  if (m_bOpened) Close();
  
  //struct ftpparse lp;
  m_bOpened  =  false;
	m_filePos  =  0;
	m_fileSize =  0;
  char result[MAX_BUFFSIZE];
  memset(result,0,MAX_BUFFSIZE);
  CStdString szPath;
	
  sprintf(m_filename,"%s",strFileName);
	if (iport==0) iport=21;
	szPath.Format("ftp://%s:%s@%s:%i/%s",strUserName,strPassword,strHostName,iport,strFileName);
	CStdString strNewPath;
  
  //if (!FTPUtil.GetFTPList(szPath,result, strNewPath))     return false;
  //FTPUtil.GetFTPList(szPath,result, strNewPath);
  if (!FTPUtil.OpenFTP(szPath))  return false;

  //if (ftpparse(&lp,result,sizeof(result))==0) return false;
	if (!FTPUtil.ConnectFTPdata())              return false;

  FTPUtil.sendrecCMD("TYPE I",result);

  //If the Target FTP Server does not support LIST /filename! Do this!
  char cVar[1024]; CStdString strTest; int iVar;
  sprintf(result,"SIZE %s",m_filename);
  FTPUtil.sendrecCMD(result,(char*)cVar);
  strTest = cVar;
  iVar = atoi(strTest.TrimLeft("213").c_str()); 
  //if (lp.size < iVar)  m_fileSize = iVar;
  //else m_fileSize = lp.size;
  m_fileSize = iVar;
  //
  sprintf(result,"RETR %s",m_filename);
  FTPUtil.sendrecCMD(result,result);
  if (FTPUtil.CMDerr(result)) return false;
  if (FTPUtil.AMode == true)  rs.attach(accept((SOCKET)FTPUtil.s2,NULL,NULL));
	if (FTPUtil.AMode == false)  rs.attach(FTPUtil.s2);
	
  m_bOpened=true;
  return true;
}

bool CFileFTP::Exists(const CURL& url)
{
	return Exists(url.GetUserName(), url.GetPassWord(), url.GetHostName(), url.GetFileName(), url.GetPort());
}
__int64 CFileFTP::Seek(__int64 iFilePosition, int iWhence)
{
	if (!m_bOpened) return 0;
	switch(iWhence) 
	{
		case SEEK_SET:
			m_filePos = iFilePosition;
			break;
		case SEEK_CUR:
			m_filePos += iFilePosition;
			break;
		case SEEK_END:
			m_filePos = m_fileSize - iFilePosition;
			break;
	}
	if (m_filePos < 0) m_filePos =0;
	if (m_filePos > m_fileSize) m_filePos=m_fileSize;
	char cmd[1024];
	char result[MAX_BUFFSIZE];
	char szOffset[32];
	FTPUtil.sendrecCMD("TYPE I",result);
	_i64toa(m_filePos,szOffset,10);
	
  sprintf(cmd,"REST %s",szOffset);
	FTPUtil.sendrecCMD(cmd,result);
	
  //sprintf(cmd,"RETR %s\r\n",m_filename);
  //FTPUtil.sendrecCMD(cmd,result);
  //FTPUtil.sending(cmd);
  
  if (FTPUtil.CMDerr(result))
	{
		FTPUtil.sendrecCMD("ABOR",result);
		FTPUtil.receiveMsg(result);	
		rs.release();
    Close();
		FTPUtil.closeDataSocket();
		FTPUtil.ConnectFTPdata();
		FTPUtil.sendrecCMD(cmd,result);
		if (FTPUtil.AMode) rs.attach(accept((SOCKET)FTPUtil.s2,NULL,NULL));
		else rs.attach((SOCKET)FTPUtil.s2);
	}
	return m_filePos;
}
__int64 CFileFTP::GetLength()
{
	if (!m_bOpened) return 0;
	return m_fileSize;
}
__int64 CFileFTP::GetPosition()
{
	if (!m_bOpened) return 0;
	return m_filePos;
}
__int64 CFileFTP::Recv(byte* pBuffer, __int64 BufCap)
{
	__int64 total=0;
	int lenRead=0;
	while ((BufCap>0) && (m_filePos<m_fileSize))
	{
	lenRead=recv((SOCKET)rs,(char*)&pBuffer[total],(int)BufCap,0);
		total	  += lenRead;
		m_filePos += lenRead;
		BufCap    -= lenRead;
	}
	return total;
}
int CFileFTP::Stat(const CURL& url, struct __stat64* buffer)
{
	if (Open(url, true))
	{
		buffer->st_size = GetLength();
		buffer->st_mode = _S_IFREG;
		Close();
		return 0;
	}
	errno = ENOENT;
	return -1;
}

unsigned int CFileFTP::Read(void *lpBuf, __int64 uiBufSize)
{
	char* pBuffer=(char* )lpBuf;
	if (!m_bOpened) return 0;
	if (m_filePos>=m_fileSize)  return 0;
	__int64 tot=Recv((unsigned char*)pBuffer,uiBufSize);
	return (unsigned int)tot;
}
