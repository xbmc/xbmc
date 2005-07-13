#include "stdafx.h"
#include "url.h"
#include "utils/RegExp.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CURL::CURL(const CStdString& strURL)
{
  m_strHostName = "";
  m_strDomain = "";
  m_strUserName = "";
  m_strPassword = "";
  m_strShareName="";
  m_strFileName = "";
  m_strProtocol = "";
  m_strFileType = "";
  m_iPort = 0;

  // strURL can be one of the following:
  // format 1: protocol://[username:password]@hostname[:port]/directoryandfile
  // format 2: protocol://file
  // format 3: drive:directoryandfile
  //
  // first need 2 check if this is a protocol or just a normal drive & path
  if (!strURL.size()) return ;
  char szURL[1024];
  strcpy(szURL, strURL.c_str());
  if ( szURL[1] == ':')
  {
    // form is drive:directoryandfile
    m_strFileName = strURL;
    int iFileType = m_strFileName.ReverseFind('.') + 1;
    if (iFileType)
    {
      m_strFileType = m_strFileName.Right(m_strFileName.size() - iFileType);
      m_strFileType.Normalize();
    }
    return ;
  }

  // form is format 1 or 2
  // format 1: protocol://[domain;][username:password]@hostname[:port]/directoryandfile
  // format 2: protocol://file

  // decode protocol
  int iPos = strURL.Find("://");
  if (iPos < 0)
  { // check for misconstructed protocols
    iPos = strURL.Find(":");
    if (iPos == strURL.GetLength() - 1)
    {
      m_strProtocol = strURL.Left(iPos);
      iPos += 1;
    }
    else
      return;
  }
  else
  {
    m_strProtocol = strURL.Left(iPos);
    iPos += 3;
  }

  //archive subpaths may contain delimiters so they need special processing
  //format 4: zip://CachePath,AutoDelMask,Password, RarPath,\FilePathInRar
  if ((m_strProtocol.CompareNoCase("rar")==0) || (m_strProtocol.CompareNoCase("zip")==0))
	{
    CRegExp reg;
    reg.RegComp("...://([^,]*),([0-9]*),([^,]*),(.*),\\\\(.*)$");
    if( (reg.RegFind(strURL.c_str()) >= 0))
    {
      char* szDomain = reg.GetReplaceString("\\1");
      char* szPort = reg.GetReplaceString("\\2");
      char* szPassword = reg.GetReplaceString("\\3");
      char* szHostName = reg.GetReplaceString("\\4");
      char* szFileName = reg.GetReplaceString("\\5");
      m_strDomain = szDomain;
      m_iPort = atoi(szPort);
      m_strPassword = szPassword;
      m_strHostName = szHostName;
      m_strFileName = szFileName;
      int iFileType = m_strFileName.ReverseFind('.') + 1;
      if (iFileType)
      {
        m_strFileType = m_strFileName.Right(m_strFileName.size() - iFileType);
        m_strFileType.Normalize();
      }
      if (szDomain)
        free(szDomain);
      if (szPort)
        free(szPort);
      if (szPassword)
        free(szPassword);
      if (szHostName)
        free(szHostName);
      if (szFileName)
        free(szFileName);

      return;
    }
	}

  // check for username/password
  int iAlphaSign = strURL.Find("@", iPos);
  int iSlash = strURL.Find("/", iPos); //to allow for @'s in filenames

  if (iAlphaSign >= 0 && (iAlphaSign < iSlash || iSlash < 0))
  {
    // username/password found
    CStdString strUserNamePassword = strURL.Mid(iPos, iAlphaSign - iPos);
    int iColon = strUserNamePassword.Find(":");
    if (iColon > 0)
    {
      m_strUserName = strUserNamePassword.Left(iColon);
      iColon++;
      m_strPassword = strUserNamePassword.Right(strUserNamePassword.size() - iColon);
      // smb domain in username
      iColon = m_strUserName.Find(";");
      if (iColon > 0)
      {
        m_strDomain = m_strUserName.Left(iColon);
        m_strUserName.Delete(0, iColon + 1);
      }
    }
    else
    {
      // smb domain without password
      m_strUserName = strUserNamePassword;
      int iColon = strUserNamePassword.Find(";");
      if (iColon > 0)
      {
        m_strDomain = strUserNamePassword.Left(iColon);
        m_strUserName.Delete(0, iColon + 1);
      }
    }

    iPos = iAlphaSign + 1;
  }

  // detect hostname:port/
  if (iSlash < 0)
  {
    CStdString strHostNameAndPort = strURL.Right(strURL.size() - iPos);
    int iColon = strHostNameAndPort.Find(":");
    if (iColon > 0)
    {
      m_strHostName = strHostNameAndPort.Left(iColon);
      iColon++;
      CStdString strPort = strHostNameAndPort.Right(strHostNameAndPort.size() - iColon);
      m_iPort = atoi(strPort.c_str());
    }
    else
    {
      m_strHostName = strHostNameAndPort;
    }

  }
  else
  {
    CStdString strHostNameAndPort = strURL.Mid(iPos, iSlash - iPos);
    int iColon = strHostNameAndPort.Find(":");
    if (iColon > 0)
    {
      m_strHostName = strHostNameAndPort.Left(iColon);
      iColon++;
      CStdString strPort = strHostNameAndPort.Right(strHostNameAndPort.size() - iColon);
      m_iPort = atoi(strPort.c_str());
    }
    else
    {
      m_strHostName = strHostNameAndPort;
    }
    iPos = iSlash + 1;
    if ((int)strURL.size() > iPos)
    {
      m_strFileName = strURL.Right(strURL.size() - iPos);
      m_strShareName = m_strHostName;
      m_strShareName += "/";
      iSlash = m_strFileName.Find("/"); 
      if(iSlash < 0)
        m_strShareName += m_strFileName;
      else
        m_strShareName += m_strFileName.Left(iSlash);
    }
  }

  // iso9960 doesnt have an hostname;-)
  if (m_strProtocol.CompareNoCase("iso9660") == 0)
  {
    if (m_strHostName != "" && m_strFileName != "")
    {
      CStdString strFileName = m_strFileName;
      m_strFileName.Format("%s/%s", m_strHostName.c_str(), strFileName.c_str());
      m_strHostName = "";
    }
    else
    {
      m_strFileName = m_strHostName;
      m_strHostName = "";
    }
  }

  int iFileType = m_strFileName.ReverseFind('.') + 1;
  if (iFileType)
  {
    m_strFileType = m_strFileName.Right(m_strFileName.size() - iFileType);
    m_strFileType.Normalize();
  }

  m_strFileName.Replace("\\", "/");
}

CURL::CURL(const CURL &url)
{
  *this = url;
}

CURL::~CURL()
{
}

CURL& CURL::operator= (const CURL& source)
{
  m_iPort = source.m_iPort;
  m_strHostName = source.m_strHostName;
  m_strDomain = source.m_strDomain;
  m_strUserName = source.m_strUserName;
  m_strPassword = source.m_strPassword;
  m_strFileName = source.m_strFileName;
  m_strProtocol = source.m_strProtocol;
  m_strFileType = source.m_strFileType;
  return *this;
}

void CURL::SetFileName(const CStdString& strFileName)
{
  m_strFileName = strFileName;

  int iFileType = m_strFileName.ReverseFind('.');
  if (iFileType != -1)
    m_strFileType = m_strFileName.Right(m_strFileName.size() - iFileType);
}

void CURL::SetHostName(const CStdString& strHostName)
{
  m_strHostName = strHostName;
}

void CURL::SetUserName(const CStdString& strUserName)
{
  m_strUserName = strUserName;
}

void CURL::SetPassword(const CStdString& strPassword)
{
  m_strPassword = strPassword;
}

bool CURL::HasPort() const
{
  return (m_iPort != 0);
}

int CURL::GetPort() const
{
  return m_iPort;
}


const CStdString& CURL::GetHostName() const
{
  return m_strHostName;
}

const CStdString&  CURL::GetShareName() const
{
  return m_strShareName;
}

const CStdString& CURL::GetDomain() const
{
  return m_strDomain;
}

const CStdString& CURL::GetUserName() const
{
  return m_strUserName;
}

const CStdString& CURL::GetPassWord() const
{
  return m_strPassword;
}

const CStdString& CURL::GetFileName() const
{
  return m_strFileName;
}

const CStdString& CURL::GetProtocol() const
{
  return m_strProtocol;
}

const CStdString& CURL::GetFileType() const
{
  return m_strFileType;
}

void CURL::GetURL(CStdString& strURL) const
{
  if (m_strProtocol == "")
  {
    strURL = m_strFileName;
    return ;
  }
  GetURLWithoutFilename(strURL);
  strURL += m_strFileName;
}

void CURL::GetURLWithoutUserDetails(CStdString& strURL) const
{
  if (m_strProtocol == "")
  {
    strURL = m_strFileName;
    return ;
  }
  if (m_strProtocol == "rar")
  {
    CStdString strNoBackSlash = m_strFileName;
    strNoBackSlash.Replace("\\","/");
    strURL.Format("rar://%s/%s",m_strHostName.c_str(),strNoBackSlash.c_str());
    return; 
  }
  if( m_strProtocol == "zip")
  {
    CStdString strNoBackSlash = m_strFileName;
    strNoBackSlash.Replace("\\","/");
    strURL.Format("zip://%s/%s",m_strHostName.c_str(),strNoBackSlash.c_str());
    return; 
  }

  strURL = m_strProtocol;
  strURL += "://";

  if (m_strHostName != "")
  {
    strURL += m_strHostName;
    if ( HasPort() )
    {
      CStdString strPort;
      strPort.Format("%i", m_iPort);
      strURL += ":";
      strURL += strPort;
    }
    strURL += "/";
  }
  strURL += m_strFileName;
}

void CURL::GetURLWithoutFilename(CStdString& strURL) const
{
  if (m_strProtocol == "")
  {
    strURL = m_strFileName.substr(0, 2); // only copy 'e:'
    return ;
  }
  if (m_strProtocol == "rar")
  {
    strURL.Format("rar://%s,%i,%s,%s,\\",m_strDomain,m_iPort,m_strPassword,m_strHostName);
    return; 
  }
  if (m_strProtocol == "zip")
  {
    strURL.Format("zip://%s,%i,%s,%s,\\",m_strDomain,m_iPort,m_strPassword,m_strHostName);
    return; 
  }

  strURL = m_strProtocol;
  strURL += "://";

  if (m_strDomain != "")
  {
    strURL += m_strDomain;
    strURL += ";";
  }
  if (m_strUserName != "" && m_strPassword != "")
  {
    strURL += m_strUserName;
    strURL += ":";
    strURL += m_strPassword;
    strURL += "@";
  }
  else if (m_strUserName != "")
  {
    strURL += m_strUserName;
    strURL += ":";
    strURL += "@";
  }
  else if (m_strDomain != "")
    strURL += "@";

  if (m_strHostName != "")
  {
    strURL += m_strHostName;
    if (HasPort())
    {
      CStdString strPort;
      strPort.Format("%i", m_iPort);
      strURL += ":";
      strURL += strPort;
    }
    strURL += "/";
  }
}
