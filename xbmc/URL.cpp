
#include "url.h"

CURL::CURL(const CStdString& strURL)
{
	m_strHostName="";
	m_strUserName="";
	m_strPassword="";
	m_strFileName="";
	m_strProtocol="";
	m_strFileType="";
	m_iPort=0;

	// strURL can be one of the following:
	// format 1: protocol://[username:password]@hostname[:port]/directoryandfile
	// format 2: protocol://file
	// format 3: drive:directoryandfile
	//
	// first need 2 check if this is a protocol or just a normal drive & path
  if (!strURL.size()) return;
	char szURL[1024];
	strcpy(szURL,strURL.c_str());
	if ( isalpha(szURL[0]) && szURL[1] == ':')
	{
		// form is drive:directoryandfile
		m_strFileName=strURL;
		int iFileType = m_strFileName.ReverseFind('.') + 1;
		if (iFileType)
			m_strFileType = m_strFileName.Right(m_strFileName.size()-iFileType);
		return;
	}

	// form is format 1 or 2
	// format 1: protocol://[username:password]@hostname[:port]/directoryandfile
	// format 2: protocol://file

	// decode protocol
	int iPos=strURL.Find("://");
	if (iPos < 0) return;
	m_strProtocol=strURL.Left(iPos);
	iPos+=3;

	// check for username/password
	int iAlphaSign=strURL.Find("@",iPos);
	if (iAlphaSign>=0)
	{
		// username/password found
		CStdString strUserNamePassword=strURL.Mid(iPos,iAlphaSign-iPos);
		int iColon=strUserNamePassword.Find(":");
		if (iColon > 0)
		{
			m_strUserName=strUserNamePassword.Left(iColon);
			iColon++;
			m_strPassword=strUserNamePassword.Right(strUserNamePassword.size()-iColon);
		}
		iPos=iAlphaSign+1;
	}
	
	// detect hostname:port/
  int iSlash=strURL.Find("/",iPos);
	if (iSlash<0) 
	{
		CStdString strHostNameAndPort=strURL.Right(strURL.size()-iPos);
		int iColon=strHostNameAndPort.Find(":");
		if (iColon > 0)
		{
			m_strHostName=strHostNameAndPort.Left(iColon);
			iColon++;
			CStdString strPort=strHostNameAndPort.Right(strHostNameAndPort.size()-iColon);
			m_iPort=atoi(strPort.c_str());
		}
		else
		{
			m_strHostName=strHostNameAndPort;
		}

	}
	else
	{
		CStdString strHostNameAndPort=strURL.Mid(iPos, iSlash-iPos);
		int iColon=strHostNameAndPort.Find(":");
		if (iColon > 0)
		{
			m_strHostName=strHostNameAndPort.Left(iColon);
			iColon++;
			CStdString strPort=strHostNameAndPort.Right(strHostNameAndPort.size()-iColon);
			m_iPort=atoi(strPort.c_str());
		}
		else
		{
			m_strHostName=strHostNameAndPort;
		}
		iPos=iSlash+1;
		if ((int)strURL.size() > iPos) 
		{
			m_strFileName=strURL.Right(strURL.size() - iPos);
		}
	}

	// iso9960 doesnt have an hostname;-)
	if (m_strProtocol.CompareNoCase("iso9660")==0)
	{
		if (m_strHostName!="" && m_strFileName !="")
		{
			CStdString strFileName = m_strFileName;
			m_strFileName.Format("%s/%s", m_strHostName.c_str(),strFileName.c_str());
			m_strHostName="";
		}
		else
		{
			m_strFileName=m_strHostName;
			m_strHostName="";
		}
	}

	int iFileType = m_strFileName.ReverseFind('.') + 1;
	if (iFileType)
		m_strFileType = m_strFileName.Right(m_strFileName.size()-iFileType);
}

CURL::~CURL()
{
}

void CURL::SetFileName(const CStdString& strFileName)
{
  m_strFileName=strFileName;

	int iFileType = m_strFileName.ReverseFind('.');
	if (iFileType != -1)
		m_strFileType = m_strFileName.Right(m_strFileName.size()-iFileType);
}

void CURL::SetHostName(const CStdString& strHostName)
{
  m_strHostName=strHostName;
}

bool  CURL::HasPort() const
{
	return (m_iPort!=0);
}

int	 CURL::GetPort() const
{
	return m_iPort;
}


const CStdString&  CURL::GetHostName() const
{
	return m_strHostName;
}

const CStdString&  CURL::GetUserName() const
{
	return m_strUserName;
}

const CStdString&  CURL::GetPassWord() const
{
	return m_strPassword;
}

const CStdString&  CURL::GetFileName() const
{
	return m_strFileName;
}

const CStdString&  CURL::GetProtocol() const
{
	return m_strProtocol;
}

const CStdString&  CURL::GetFileType() const
{
	return m_strFileType;
}

void  CURL::GetURL(CStdString& strURL)
{
	if (m_strProtocol=="")
	{
		strURL=m_strFileName;
		return;
	}
	strURL=m_strProtocol;
	strURL+="://";
	if (m_strUserName!="" && m_strPassword!="")
	{	
		strURL+=m_strUserName;
		strURL+=":";
		strURL+=m_strPassword;
		strURL+="@";
	}
	if (m_strHostName!="")
	{
		strURL+=m_strHostName;
		if ( HasPort() )
		{
			CStdString strPort;
			strPort.Format("%i", m_iPort);
			strURL += ":";
			strURL += strPort;
		}
		strURL+="/";
	}
	strURL+=m_strFileName;

}

void CURL::GetURLWithoutUserDetails(CStdString& strURL)
{
	if (m_strProtocol=="")
	{
		strURL=m_strFileName;
		return;
	}
	strURL=m_strProtocol;
	strURL+="://";

	if (m_strHostName!="")
	{
		strURL+=m_strHostName;
		if ( HasPort() )
		{
			CStdString strPort;
			strPort.Format("%i", m_iPort);
			strURL += ":";
			strURL += strPort;
		}
		strURL+="/";
	}
	strURL+=m_strFileName;
}
