// FileSmb.cpp: implementation of the CFileSMB class.

//

//////////////////////////////////////////////////////////////////////



#include "FileSmb.h"
#include "../sectionloader.h"
#include "../settings.h"
using namespace XFILE;

//////////////////////////////////////////////////////////////////////

// Construction/Destruction

//////////////////////////////////////////////////////////////////////
static char szPassWd[128];
static char szUserName[128];

class MyFileCallback : public SmbAnswerCallback
{
protected:
	// Warning: don't use a fixed size buffer in a real application.
	// This is a security hazard.
	char buf[200];
public:
	char *getAnswer(int type, const char *optmessage) {
		switch (type) {
			case ANSWER_USER_NAME:
				strcpy(buf, szUserName);
				break;
			case ANSWER_USER_PASSWORD:
//				cout<<"Password for user "<<optmessage<<": ";
//				cin>>buf;
				strcpy(buf, szPassWd);
				break;
			case ANSWER_SERVICE_PASSWORD:
				strcpy(buf, szPassWd);
				break;
		}
		return buf;
	}
} cb;

CFileSMB::CFileSMB()
{
	CSectionLoader::Load("LIBSMB");
	m_pSMB=NULL;
	m_fd = -1;
}

CFileSMB::~CFileSMB()
{
	Close();
	CSectionLoader::Unload("LIBSMB");
}



offset_t CFileSMB::GetPosition()
{
	int pos = m_pSMB->lseek(m_fd, 0, SEEK_CUR);
	if( pos < 0 )
		return 0;
	return (offset_t)pos;
}

offset_t CFileSMB::GetLength()
{
	return m_fileSize;
}

bool CFileSMB::Open(const char* strUserName, const char* strPassword,const char *strHostName, const char *strFileName,int iport, bool bBinary)
{

	// the samba code does not interpret binary/text.  We may
	
  m_pSMB = new SMB();

	if( g_stSettings.m_strNameServer[0] != 0x00 )
		m_pSMB->setNBNSAddress(g_stSettings.m_strNameServer);

	m_pSMB->setPasswordCallback(&cb);

	// have to convert dos line termination ourselves.
	char szFileName[1024];
	m_bBinary = bBinary;
  if (strPassword)
	  strcpy(szPassWd,strPassword);
  else
    strcpy(szPassWd,"");

  if (strUserName)
	  strcpy(szUserName,strUserName);
  else
    strcpy(szUserName,"");

	sprintf(szFileName,"%s/%s", strHostName,strFileName);
	if (strPassword)
	{
		strcpy(szPassWd,strPassword);
		if (strPassword && strUserName)
		{
			sprintf(szFileName,"%s:%s@%s/%s", strUserName,strPassword,strHostName,strFileName);
		}
	}	

	m_fd = m_pSMB->open(szFileName, O_RDONLY);

	if( m_fd < 0 )
	{
		
		return false;
	}
	INT64 ret = m_pSMB->lseek(m_fd, 0, SEEK_END);

	if( ret < 0 )
	{
		m_pSMB->close(m_fd);
		m_fd = -1;
		return false;
	}

	m_fileSize = ret;
	ret = m_pSMB->lseek(m_fd, 0, SEEK_SET);
	if( ret < 0 )
	{
		m_pSMB->close(m_fd);
		m_fd = -1;
		return false;
	}
	// We've successfully opened the file!
	return true;
}

unsigned int CFileSMB::Read(void *lpBuf, offset_t uiBufSize)
{
	int bytesRead = m_pSMB->read(m_fd, lpBuf, (int)uiBufSize);
	if( bytesRead <= 0 )
	{
		char szTmp[128];
		sprintf(szTmp,"SMB returned %i errno:%i\n",bytesRead,errno);
		OutputDebugString(szTmp);
		return 0;
	}
	return (unsigned int)bytesRead;
}


bool CFileSMB::ReadString(char *szLine, int iLineLength)
{
	offset_t iFilePos=GetPosition();

	int iBytesRead=m_pSMB->read(m_fd, (unsigned char*)szLine, iLineLength);
	if (iBytesRead <= 0)
	{
		return false;
	}

	szLine[iBytesRead]=0;

	for (int i=0; i < iBytesRead; i++)
	{
		if ('\n' == szLine[i])
		{
			if ('\r' == szLine[i+1])
			{
				szLine[i+2]=0;
				Seek(iFilePos+i+2,SEEK_SET);
			}
			else
			{
				// end of line
				szLine[i+1]=0;
				Seek(iFilePos+i+1,SEEK_SET);
			}
			break;
		}
		else if ('\r'==szLine[i])
		{
			if ('\n' == szLine[i+1])
			{
				szLine[i+2]=0;
				Seek(iFilePos+i+2,SEEK_SET);
			}
			else
			{
				// end of line
				szLine[i+1]=0;
				Seek(iFilePos+i+1,SEEK_SET);
			}
			break;
		}
	}
	if (iBytesRead>0) 
	{
		return true;
	}
	return false;

}



offset_t CFileSMB::Seek(offset_t iFilePosition, int iWhence)

{
	INT64 pos = m_pSMB->lseek(m_fd, (int)iFilePosition, iWhence);

	if( pos < 0 )

		return 0;

	return (offset_t)pos;

}



void CFileSMB::Close()

{
	if (m_pSMB)
	{
		if (m_fd>=0)
			m_pSMB->close(m_fd);
		delete m_pSMB;
	}
	m_pSMB=NULL;
	m_fd = -1;
	
}

