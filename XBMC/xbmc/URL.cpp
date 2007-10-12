/*
 *      Copyright (C) 2005-2007 Team XboxMediaCenter
 *      http://www.xboxmediacenter.com
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "URL.h"
#include "utils/RegExp.h"
#include "Util.h"

CStdString URLEncodeInline(const CStdString& strData)
{
  CStdString buffer = strData;
  CUtil::URLEncode(buffer);
  return buffer;
}

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
  if (strURL.Equals("?", true)) return;
#ifndef _LINUX  
  if (strURL[1] == ':')
  {
    // form is drive:directoryandfile

    /* set filename and update extension*/
    SetFileName(strURL);
    return ;
  }
#endif

  // form is format 1 or 2
  // format 1: protocol://[domain;][username:password]@hostname[:port]/directoryandfile
  // format 2: protocol://file

  // decode protocol
  int iPos = strURL.Find("://");
  if (iPos < 0)  
  {
#ifndef _LINUX  
    // check for misconstructed protocols
    iPos = strURL.Find(":");
    if (iPos == strURL.GetLength() - 1)
    {
      m_strProtocol = strURL.Left(iPos);
      iPos += 1;
    }
    else
    {
      //CLog::Log(LOGDEBUG, "%s - Url has no protocol %s, empty CURL created", __FUNCTION__, strURL.c_str());
      return;
    }
#else
    {
      /* set filename and update extension*/
      SetFileName(strURL);
      return ;
    }    
#endif    
  }
  else
  {
    m_strProtocol = strURL.Left(iPos);
    iPos += 3;
  }

  // virtual protocols
  // why not handle all format 2 (protocol://file) style urls here?
  // ones that come to mind are iso9660, cdda, musicdb, etc.
  // they are all local protocols and have no server part, port number, special options, etc.
  // this removes the need for special handling below.
  if (
    m_strProtocol.Equals("stack") ||
    m_strProtocol.Equals("virtualpath") ||
    m_strProtocol.Equals("multipath") ||
    m_strProtocol.Equals("filereader")
    )
  {
    m_strFileName = strURL.Mid(iPos);
    return;
  }

  //check for old archive format, dll's might use it
  if (m_strProtocol.Equals("rar") || m_strProtocol.Equals("zip"))
  {
    //archive subpaths may contain delimiters so they need special processing
    //format 4: zip://CachePath,AutoDelMask,Password, RarPath,\FilePathInRar

    CRegExp reg;
    reg.RegComp("...://([^,]*),([0-9]*),([^,]*),([^,]*),[\\/]+(.*)$");

    if(reg.RegFind(strURL) == 0) /* if found at position 0 */
    {
      char* szCache = reg.GetReplaceString("\\1");
      char* szFlags = reg.GetReplaceString("\\2");
      char* szPassword = reg.GetReplaceString("\\3");
      char* szArchive = reg.GetReplaceString("\\4");
      char* szFileName = reg.GetReplaceString("\\5");

      m_strHostName = szArchive;
      m_strPassword = szPassword;
      if (szFileName)
        SetFileName(szFileName);

      // currently neither zip nor rar code cares for the
      // flags or cache dir, so just ignore them for now

      if (szCache) free(szCache);
      if (szFlags) free(szFlags);
      if (szPassword) free(szPassword);
      if (szArchive) free(szArchive);
      if (szFileName) free(szFileName);

      return;
    }
  }

  // check for username/password - should occur before first /
  if (iPos == -1) iPos = 0;


  // for protocols supporting options, chop that part off here
  // maybe we should invert this list instead?
  int iEnd = strURL.length();
  if(m_strProtocol.Equals("http")
    || m_strProtocol.Equals("https")
    || m_strProtocol.Equals("ftp")
    || m_strProtocol.Equals("ftps")
    || m_strProtocol.Equals("ftpx")
    || m_strProtocol.Equals("shout")
    || m_strProtocol.Equals("tuxbox")
    || m_strProtocol.Equals("daap")
    || m_strProtocol.Equals("plugin")
    || m_strProtocol.Equals("hdhomerun")
    || m_strProtocol.Equals("rtsp"))
  {
    int iOptions = strURL.find_first_of("?;#", iPos);
    if (iOptions >= 0 )
    {
      // we keep the initial char as it can be any of the above
      m_strOptions = strURL.substr(iOptions);
      iEnd = iOptions;
    }
  }

  int iSlash = strURL.Find("/", iPos);
  if(iSlash >= iEnd)
    iSlash = -1; // was an invalid slash as it was contained in options

  if( !m_strProtocol.Equals("iso9660") )
  {
    int iAlphaSign = strURL.Find("@", iPos);
    if (iAlphaSign >= 0 && iAlphaSign < iEnd && (iAlphaSign < iSlash || iSlash < 0))
    {
      // username/password found
      CStdString strUserNamePassword = strURL.Mid(iPos, iAlphaSign - iPos);

      // first extract domain, if protocol is smb
      if (m_strProtocol.Equals("smb"))
      {
        int iSemiColon = strUserNamePassword.Find(";");

        if (iSemiColon >= 0)
        {
          m_strDomain = strUserNamePassword.Left(iSemiColon);
          strUserNamePassword.Delete(0, iSemiColon + 1);
        }
      }

      // username:password
      int iColon = strUserNamePassword.Find(":");
      if (iColon >= 0)
      {
        m_strUserName = strUserNamePassword.Left(iColon);
        iColon++;
        m_strPassword = strUserNamePassword.Right(strUserNamePassword.size() - iColon);
      }
      // username
      else
      {
        m_strUserName = strUserNamePassword;
      }

      iPos = iAlphaSign + 1;
      iSlash = strURL.Find("/", iAlphaSign);

      if(iSlash >= iEnd)
        iSlash = -1;
    }
  }

  // detect hostname:port/
  if (iSlash < 0)
  {
    CStdString strHostNameAndPort = strURL.Mid(iPos, iEnd - iPos);
    int iColon = strHostNameAndPort.Find(":");
    if (iColon >= 0)
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
    if (iColon >= 0)
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
    if (iEnd > iPos)
    {
      m_strFileName = strURL.Mid(iPos, iEnd - iPos);

      iSlash = m_strFileName.Find("/");
      if(iSlash < 0)
        m_strShareName = m_strFileName;
      else
        m_strShareName = m_strFileName.Left(iSlash);
    }
  }

  // iso9960 doesnt have an hostname;-)
  if (m_strProtocol.CompareNoCase("iso9660") == 0
    || m_strProtocol.CompareNoCase("musicdb") == 0
    || m_strProtocol.CompareNoCase("videodb") == 0
    || m_strProtocol.CompareNoCase("lastfm") == 0
    || m_strProtocol.Left(3).CompareNoCase("mem") == 0)
  {
    if (m_strHostName != "" && m_strFileName != "")
    {
      CStdString strFileName = m_strFileName;
      m_strFileName.Format("%s/%s", m_strHostName.c_str(), strFileName.c_str());
      m_strHostName = "";
    }
    else
    {
      if (!m_strHostName.IsEmpty() && strURL[iEnd-1]=='/')
        m_strFileName=m_strHostName + "/";
      else
        m_strFileName = m_strHostName;
      m_strHostName = "";
    }
  }

  m_strFileName.Replace("\\", "/");

  /* update extension */
  SetFileName(m_strFileName);

  /* decode urlencoding on this stuff */
  if( m_strProtocol.Equals("rar") || m_strProtocol.Equals("zip") || m_strProtocol.Equals("musicsearch"))
    CUtil::UrlDecode(m_strHostName);

  CUtil::UrlDecode(m_strUserName);
  CUtil::UrlDecode(m_strPassword);
}

CURL::CURL(const CURL &url)
{
  *this = url;
}

CURL::CURL()
{
  m_iPort = 0;
}

CURL::~CURL()
{
}

CURL& CURL::operator= (const CURL& source)
{
  m_iPort        = source.m_iPort;
  m_strHostName  = source.m_strHostName;
  m_strDomain    = source.m_strDomain;
  m_strShareName = source.m_strShareName;
  m_strUserName  = source.m_strUserName;
  m_strPassword  = source.m_strPassword;
  m_strFileName  = source.m_strFileName;
  m_strProtocol  = source.m_strProtocol;
  m_strFileType  = source.m_strFileType;
  m_strOptions   = source.m_strOptions;
  return *this;
}

void CURL::SetFileName(const CStdString& strFileName)
{
  m_strFileName = _P(strFileName);

  int slash = m_strFileName.find_last_of(GetDirectorySeparator());
  int period = m_strFileName.find_last_of('.');
  if(period != -1 && (slash == -1 || period > slash))
    m_strFileType = m_strFileName.substr(period+1);
  else
    m_strFileType = "";

  m_strFileType.Normalize();
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

void CURL::SetProtocol(const CStdString& strProtocol)
{
  m_strProtocol = strProtocol;
}

void CURL::SetOptions(const CStdString& strOptions)
{
  m_strOptions.Empty();
  if( strOptions.length() > 0)
    if( strOptions[0] == '?' || strOptions[0] == '#' || strOptions[0] == ';' || strOptions.Find("xml") >=0 )
    {
      m_strOptions = strOptions;
    }
    else
      CLog::Log(LOGWARNING, "%s - Invalid options specified for url %s", __FUNCTION__, strOptions.c_str());
}

void CURL::SetPort(int port)
{
  m_iPort = port;
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

const CStdString& CURL::GetOptions() const
{
  return m_strOptions;
}

const CStdString CURL::GetFileNameWithoutPath() const
{
  // *.zip and *.rar store the actual zip/rar path in the hostname of the url
  if ((m_strProtocol == "rar" || m_strProtocol == "zip") && m_strFileName.IsEmpty())
    return CUtil::GetFileName(m_strHostName);

  // otherwise, we've already got the filepath, so just grab the filename portion
  CStdString file(m_strFileName);
  CUtil::RemoveSlashAtEnd(file);
  return CUtil::GetFileName(file);
}

const char CURL::GetDirectorySeparator() const
{
#ifndef _LINUX
  if ( IsLocal() )
    return '\\';
  else
#endif
    return '/';
}

void CURL::GetURL(CStdString& strURL) const
{
  unsigned int sizeneed = m_strProtocol.length()
                        + m_strDomain.length()
                        + m_strUserName.length()
                        + m_strPassword.length()
                        + m_strHostName.length()
                        + m_strFileName.length()
                        + m_strOptions.length()
                        + 10;

  if( strURL.capacity() < sizeneed )
    strURL.reserve(sizeneed);

  if (m_strProtocol == "")
  {
    strURL = m_strFileName;
    return ;
  }
  GetURLWithoutFilename(strURL);
  strURL += m_strFileName;

  if( m_strOptions.length() > 0 )
    strURL += m_strOptions;
}

void CURL::GetURLWithoutUserDetails(CStdString& strURL) const
{
  unsigned int sizeneed = m_strProtocol.length()
                        + m_strDomain.length()
                        + m_strHostName.length()
                        + m_strFileName.length()
                        + m_strOptions.length()
                        + 10;

  if( strURL.capacity() < sizeneed )
    strURL.reserve(sizeneed);


  if (m_strProtocol == "")
  {
    strURL = m_strFileName;
    return ;
  }

  strURL = m_strProtocol;
  strURL += "://";

  if (m_strHostName != "")
  {
    if (m_strProtocol.Equals("rar") || m_strProtocol.Equals("zip"))
    {
      CURL url2(m_strHostName);
      CStdString strHost;
      url2.GetURLWithoutUserDetails(strHost);
      strURL += strHost;
    }
    else
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

  if( m_strOptions.length() > 0 )
    strURL += m_strOptions;
}

void CURL::GetURLWithoutFilename(CStdString& strURL) const
{
  unsigned int sizeneed = m_strProtocol.length()
                        + m_strDomain.length()
                        + m_strUserName.length()
                        + m_strPassword.length()
                        + m_strHostName.length()
                        + 10;

  if( strURL.capacity() < sizeneed )
    strURL.reserve(sizeneed);


  if (m_strProtocol == "")
  {
#ifdef _LINUX
    strURL.Empty();
#else
    strURL = m_strFileName.substr(0, 2); // only copy 'e:'
#endif
    return ;
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
    strURL += URLEncodeInline(m_strUserName);
    strURL += ":";
    strURL += URLEncodeInline(m_strPassword);
    strURL += "@";
  }
  else if (m_strUserName != "")
  {
    strURL += URLEncodeInline(m_strUserName);
    strURL += ":";
    strURL += "@";
  }
  else if (m_strDomain != "")
    strURL += "@";

  if (m_strHostName != "")
  {
    if( m_strProtocol.Equals("rar") || m_strProtocol.Equals("zip") || m_strProtocol.Equals("musicsearch"))
      strURL += URLEncodeInline(m_strHostName);
    else
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

bool CURL::IsLocal() const
{
  return m_strProtocol.IsEmpty();
}

bool CURL::IsFileOnly(const CStdString &url)
{
  return url.find_first_of("/\\") == CStdString::npos;
}

