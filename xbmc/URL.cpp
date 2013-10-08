/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "URL.h"
#include "utils/RegExp.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include "Util.h"
#include "filesystem/File.h"
#include "FileItem.h"
#include "filesystem/StackDirectory.h"
#include "addons/Addon.h"
#include "utils/StringUtils.h"
#ifndef TARGET_POSIX
#include <sys\types.h>
#include <sys\stat.h>
#endif

using namespace std;
using namespace ADDON;

CStdString URLEncodeInline(const CStdString& strData)
{
  CStdString buffer = strData;
  CURL::Encode(buffer);
  return buffer;
}

CURL::CURL(const std::string& strURL1)
{
  Parse(strURL1);
}

CURL::CURL()
{
  m_iPort = 0;
}

CURL::~CURL()
{
}

void CURL::Reset()
{
  m_strHostName.clear();
  m_strDomain.clear();
  m_strUserName.clear();
  m_strPassword.clear();
  m_strShareName.clear();
  m_strFileName.clear();
  m_strProtocol.clear();
  m_strFileType.clear();
  m_strOptions.clear();
  m_strProtocolOptions.clear();
  m_options.Clear();
  m_protocolOptions.Clear();
  m_iPort = 0;
}

void CURL::Parse(const std::string& strURL1)
{
  Reset();
  // start by validating the path
  std::string strURL(CUtil::ValidatePath(strURL1));

  // strURL can be one of the following:
  // format 1: protocol://[username:password]@hostname[:port]/directoryandfile
  // format 2: protocol://file
  // format 3: drive:directoryandfile (or (win32 only) \\?\drive:directoryandfile)
  // format 4 (win32 only): \\Server\Share\directoryandfile (or \\?\UNC\Server\Share\directoryandfile)
  //
  // first need 2 check if this is a protocol or just a normal drive & path
  if (strURL.empty())
    return;
  if (strURL == "?")
    return;

#ifdef TARGET_WINDOWS
  if (StringUtils::StartsWith(strURL, "\\\\"))
  {
    size_t serverNamePos;
    if (strURL.length() > 8 && strURL.compare(2, 6, "?\\UNC\\", 6) == 0) // win32 long UNC path
      serverNamePos = 8;
    else if (strURL.length() > 3 && strURL.compare(2, 2, "?\\", 2) == 0) // win32 long local path
    {
      SetFileName(strURL);
      return;
    }
    else
      serverNamePos = 2; // win32 "\\server\share\file" path

    strURL = "smb://" + CUtil::FixSlashes(strURL.substr(serverNamePos), false, true); // handling of "smb://" require forward slashes
  }
#endif // TARGET_WINDOWS

  // form is format 1 or 2
  // format 1: protocol://[domain;][username:password]@hostname[:port]/directoryandfile
  // format 2: protocol://file

  // decode protocol
  size_t iPos = strURL.find("://");
  if (iPos == std::string::npos)
  {
    // This is an ugly hack that needs some work.
    // example: filename /foo/bar.zip/alice.rar/bob.avi
    // This should turn into zip://rar:///foo/bar.zip/alice.rar/bob.avi
    iPos = 0;
    while (1)
    {
      iPos = strURL.find(".apk/", iPos);
      const bool is_apk = (iPos != std::string::npos);

      if (!is_apk)
        iPos = strURL.find(".zip/", iPos);

      if (iPos == std::string::npos)
      {
        /* set filename and update extension*/
        SetFileName(strURL);
        return;
      }
      iPos += 4; // length of ".apk" or ".zip"
      std::string archiveName(strURL, 0, iPos);
      struct __stat64 s;
      if (XFILE::CFile::Stat(archiveName, &s) == 0)
      {
#ifdef TARGET_POSIX
        if (!S_ISDIR(s.st_mode))
#else
        if (!(s.st_mode & S_IFDIR))
#endif
        {
          Encode(archiveName);
          if (is_apk)
          {
            CURL c((std::string)"apk" + "://" + archiveName + '/' + strURL.substr(iPos + 1));
            *this = c;
          }
          else
          {
            CURL c((std::string)"zip" + "://" + archiveName + '/' + strURL.substr(iPos + 1));
            *this = c;
          }
          return;
        }
      }
    }
  }
  else
  {
    SetProtocol(strURL.substr(0, iPos));
    iPos += 3; // length of "://"
  }

  // virtual protocols / local protocols
  // they have no server part, port number, special options, etc.
  // FIXME: add more protocols here?
  if ( m_strProtocol == "stack"         ||
        m_strProtocol == "virtualpath"  ||
        m_strProtocol == "multipath"    ||
        m_strProtocol == "filereader"   ||
        m_strProtocol == "special"      ||
        m_strProtocol == "iso9660"      ||
        m_strProtocol == "musicdb"      ||
        m_strProtocol == "videodb"      ||
        m_strProtocol == "sources"      ||
        m_strProtocol == "pvr"          ||
        m_strProtocol == "cdda"         ||
        m_strProtocol.compare(0, 3, "mem", 3) == 0)
  {
    SetFileName(strURL.substr(iPos));
    return;
  }

  const size_t strLen = strURL1.length();

  if (iPos >= strLen)
    return;

  if (iPos == std::string::npos) 
    iPos = 0;

  // for protocols supporting options, chop that part off here
  size_t iEnd = strLen;
  const char* sep = NULL;

  //TODO fix all Addon paths
  std::string strProtocol2(GetTranslatedProtocol());
  if (m_strProtocol == "rss" ||
       m_strProtocol == "rar" ||
       m_strProtocol == "addons" ||
       m_strProtocol == "image" ||
       m_strProtocol == "videodb" ||
       m_strProtocol == "musicdb" ||
       m_strProtocol == "androidapp")
    sep = "?";
  else if (strProtocol2 == "http"
            || strProtocol2 == "https"
            || strProtocol2 == "plugin"
            || strProtocol2 == "addons"
            || strProtocol2 == "hdhomerun"
            || strProtocol2 == "rtsp"
            || strProtocol2 == "apk"
            || strProtocol2 == "zip")
    sep = "?;#|";
  else if (strProtocol2 == "ftp"
        || strProtocol2 == "ftps")
    sep = "?;|";

  if (sep)
  {
    const size_t iOptions = strURL.find_first_of(sep, iPos);
    if (iOptions != std::string::npos)
    {
      // we keep the initial char as it can be any of the above
      const size_t iProto = strURL.find('|', iOptions);
      if (iProto != std::string::npos)
      {
        SetProtocolOptions(strURL.substr(iProto+1));
        SetOptions(strURL.substr(iOptions,iProto-iOptions));
      }
      else
        SetOptions(strURL.substr(iOptions));
      iEnd = iOptions;
    }
  }

  size_t iSlash = strURL.find('/', iPos);
  if (iSlash != std::string::npos && iSlash >= iEnd)
    iSlash = std::string::npos; // was an invalid slash as it was contained in options

  // check for username/password - should occur before first '/'
  const size_t iAlphaSign = strURL.find('@', iPos);
  if (iAlphaSign != std::string::npos && iAlphaSign < iEnd && iAlphaSign < iSlash)
  {
    // username/password found
    std::string strUserNamePassword(strURL, iPos, iAlphaSign - iPos);

    // first extract domain, if protocol is smb
    if (m_strProtocol.Equals("smb"))
    {
      const size_t iSemiColon = strUserNamePassword.find(';');

      if (iSemiColon != std::string::npos && iSemiColon < strUserNamePassword.find(':'))
      { // domain;username
        m_strDomain = strUserNamePassword.substr(0, iSemiColon);
        strUserNamePassword.erase(0, iSemiColon + 1);
      }
    }

    const size_t iColon = strUserNamePassword.find(':');
    if (iColon != std::string::npos)
    {   // username:password
      m_strUserName = strUserNamePassword.substr(0, iColon);
      m_strPassword = strUserNamePassword.substr(iColon + 1);
    }
    else // username
      m_strUserName = strUserNamePassword;

    iPos = iAlphaSign + 1;
    iSlash = strURL.find('/', iAlphaSign);

    if (iSlash >= iEnd)
      iSlash = std::string::npos;
  }

  if (iPos >= strLen)
    return;

  // detect hostname:port/
  if (iSlash == std::string::npos)
  {
    std::string strHostNameAndPort(strURL, iPos, iEnd - iPos);
    const size_t iColon = strHostNameAndPort.find(':');
    if (iColon != std::string::npos)
    {
      m_strHostName = strHostNameAndPort.substr(0, iColon);
      std::string strPort(strHostNameAndPort, iColon + 1);
      m_iPort = atoi(strPort.c_str());
    }
    else
      m_strHostName = strHostNameAndPort;

  }
  else
  {
    std::string strHostNameAndPort(strURL, iPos, iSlash - iPos);
    const size_t iColon = strHostNameAndPort.find(':');
    if (iColon != std::string::npos)
    {
      m_strHostName = strHostNameAndPort.substr(iColon);
      std::string strPort(strHostNameAndPort, 0, iColon + 1);
      m_iPort = atoi(strPort.c_str());
    }
    else
      m_strHostName = strHostNameAndPort;

    iPos = iSlash + 1;
    if (iEnd > iPos)
    {
      m_strFileName = strURL.substr(iPos, iEnd - iPos);

      iSlash = m_strFileName.find('/');
      if (iSlash == std::string::npos)
        m_strShareName = m_strFileName;
      else
        m_strShareName = m_strFileName.substr(0, iSlash);
    }
  }

  /* update extension */
  SetFileName(m_strFileName);

  /* decode urlencoding on this stuff */
  if (URIUtils::ProtocolHasEncodedHostname(m_strProtocol))
  {
    Decode(m_strHostName);
    // Validate it as it is likely to contain a filename
    SetHostName(CUtil::ValidatePath(m_strHostName));
  }

  Decode(m_strUserName);
  Decode(m_strPassword);
}

void CURL::SetFileName(const std::string& strFileName)
{
  m_strFileName = strFileName;

  size_t slash = m_strFileName.find_last_of(GetDirectorySeparator());
  size_t period = m_strFileName.find_last_of('.');
  if (period != std::string::npos && (slash == std::string::npos || period > slash))
  {
    m_strFileType = m_strFileName.substr(period+1);
    StringUtils::Trim(m_strFileType);
    StringUtils::ToLower(m_strFileType);
  }
  else
    m_strFileType.clear();
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
  m_strProtocol.ToLower();
}

void CURL::SetOptions(const CStdString& strOptions)
{
  m_strOptions.Empty();
  m_options.Clear();
  if( strOptions.length() > 0)
  {
    if( strOptions[0] == '?' || strOptions[0] == '#' || strOptions[0] == ';' || strOptions.Find("xml") >=0 )
    {
      m_strOptions = strOptions;
      m_options.AddOptions(m_strOptions);
    }
    else
      CLog::Log(LOGWARNING, "%s - Invalid options specified for url %s", __FUNCTION__, strOptions.c_str());
  }
}

void CURL::SetProtocolOptions(const CStdString& strOptions)
{
  m_strProtocolOptions.Empty();
  m_protocolOptions.Clear();
  if (strOptions.length() > 0)
  {
    if (strOptions[0] == '|')
      m_strProtocolOptions = strOptions.Mid(1);
    else
      m_strProtocolOptions = strOptions;
    m_protocolOptions.AddOptions(m_strProtocolOptions);
  }
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

const CStdString CURL::GetTranslatedProtocol() const
{
  return TranslateProtocol(m_strProtocol);
}

const CStdString& CURL::GetFileType() const
{
  return m_strFileType;
}

const CStdString& CURL::GetOptions() const
{
  return m_strOptions;
}

const CStdString& CURL::GetProtocolOptions() const
{
  return m_strProtocolOptions;
}

const CStdString CURL::GetFileNameWithoutPath() const
{
  // *.zip and *.rar store the actual zip/rar path in the hostname of the url
  if ((m_strProtocol == "rar"  || 
       m_strProtocol == "zip"  ||
       m_strProtocol == "apk") &&
       m_strFileName.IsEmpty())
    return URIUtils::GetFileName(m_strHostName);

  // otherwise, we've already got the filepath, so just grab the filename portion
  CStdString file(m_strFileName);
  URIUtils::RemoveSlashAtEnd(file);
  return URIUtils::GetFileName(file);
}

char CURL::GetDirectorySeparator() const
{
#ifdef TARGET_WINDOWS
  if (m_strProtocol.empty() || m_strProtocol == "file")
    return '\\';
  else
#endif
    return '/';
}

CStdString CURL::Get() const
{
  unsigned int sizeneed = m_strProtocol.length()
                        + m_strDomain.length()
                        + m_strUserName.length()
                        + m_strPassword.length()
                        + m_strHostName.length()
                        + m_strFileName.length()
                        + m_strOptions.length()
                        + m_strProtocolOptions.length()
                        + 10;

  if (m_strProtocol == "")
    return m_strFileName;

  CStdString strURL;
  strURL.reserve(sizeneed);

  strURL = GetWithoutFilename();
  strURL += m_strFileName;

  if( m_strOptions.length() > 0 )
    strURL += m_strOptions;
  if (m_strProtocolOptions.length() > 0)
    strURL += "|"+m_strProtocolOptions;

  return strURL;
}

CStdString CURL::GetWithoutUserDetails() const
{
  CStdString strURL;

  if (m_strProtocol.Equals("stack"))
  {
    CFileItemList items;
    CStdString strURL2;
    strURL2 = Get();
    XFILE::CStackDirectory dir;
    dir.GetDirectory(strURL2,items);
    vector<CStdString> newItems;
    for (int i=0;i<items.Size();++i)
    {
      CURL url(items[i]->GetPath());
      items[i]->SetPath(url.GetWithoutUserDetails());
      newItems.push_back(items[i]->GetPath());
    }
    dir.ConstructStackPath(newItems,strURL);
    return strURL;
  }

  unsigned int sizeneed = m_strProtocol.length()
                        + m_strDomain.length()
                        + m_strHostName.length()
                        + m_strFileName.length()
                        + m_strOptions.length()
                        + m_strProtocolOptions.length()
                        + 10;

  strURL.reserve(sizeneed);

  if (m_strProtocol == "")
    return m_strFileName;

  strURL = m_strProtocol;
  strURL += "://";

  if (m_strHostName != "")
  {
    CStdString strHostName;

    if (URIUtils::ProtocolHasParentInHostname(m_strProtocol))
      strHostName = CURL(m_strHostName).GetWithoutUserDetails();
    else
      strHostName = m_strHostName;

    if (URIUtils::ProtocolHasEncodedHostname(m_strProtocol))
      strURL += URLEncodeInline(strHostName);
    else
      strURL += strHostName;

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
  if( m_strProtocolOptions.length() > 0 )
    strURL += "|"+m_strProtocolOptions;

  return strURL;
}

CStdString CURL::GetWithoutFilename() const
{
  if (m_strProtocol == "")
    return "";

  unsigned int sizeneed = m_strProtocol.length()
                        + m_strDomain.length()
                        + m_strUserName.length()
                        + m_strPassword.length()
                        + m_strHostName.length()
                        + 10;

  CStdString strURL;
  strURL.reserve(sizeneed);

  strURL = m_strProtocol;
  strURL += "://";

  if (m_strDomain != "")
  {
    strURL += m_strDomain;
    strURL += ";";
  }
  else if (m_strUserName != "")
  {
    strURL += URLEncodeInline(m_strUserName);
    if (m_strPassword != "")
    {
      strURL += ":";
      strURL += URLEncodeInline(m_strPassword);
    }
    strURL += "@";
  }
  else if (m_strDomain != "")
    strURL += "@";

  if (m_strHostName != "")
  {
    if( URIUtils::ProtocolHasEncodedHostname(m_strProtocol) )
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

  return strURL;
}

bool CURL::IsLocal() const
{
  return (IsLocalHost() || m_strProtocol.IsEmpty());
}

bool CURL::IsLocalHost() const
{
  return (m_strHostName.Equals("localhost") || m_strHostName.Equals("127.0.0.1"));
}

bool CURL::IsFileOnly(const CStdString &url)
{
  return url.find_first_of("/\\") == CStdString::npos;
}

bool CURL::IsFullPath(const CStdString &url)
{
  if (url.size() && url[0] == '/') return true;     //   /foo/bar.ext
  if (url.Find("://") >= 0) return true;                 //   foo://bar.ext
  if (url.size() > 1 && url[1] == ':') return true; //   c:\\foo\\bar\\bar.ext
  if (StringUtils::StartsWith(url, "\\\\")) return true;    //   \\UNC\path\to\file
  return false;
}

void CURL::Decode(CStdString& strURLData)
//modified to be more accomodating - if a non hex value follows a % take the characters directly and don't raise an error.
// However % characters should really be escaped like any other non safe character (www.rfc-editor.org/rfc/rfc1738.txt)
{
  CStdString strResult;

  /* result will always be less than source */
  strResult.reserve( strURLData.length() );

  for (unsigned int i = 0; i < strURLData.size(); ++i)
  {
    int kar = (unsigned char)strURLData[i];
    if (kar == '+') strResult += ' ';
    else if (kar == '%')
    {
      if (i < strURLData.size() - 2)
      {
        CStdString strTmp;
        strTmp.assign(strURLData.substr(i + 1, 2));
        int dec_num=-1;
        sscanf(strTmp,"%x",(unsigned int *)&dec_num);
        if (dec_num<0 || dec_num>255)
          strResult += kar;
        else
        {
          strResult += (char)dec_num;
          i += 2;
        }
      }
      else
        strResult += kar;
    }
    else strResult += kar;
  }
  strURLData = strResult;
}

void CURL::Encode(std::string& strURLData)
{
  std::string strResult;

  /* wonder what a good value is here is, depends on how often it occurs */
  strResult.reserve(strURLData.length() * 2);

  for (size_t i = 0; i < strURLData.length(); ++i)
  {
    const int kar = (unsigned char)strURLData[i];
    if (isalnum(kar) || strchr("-_.!()" , kar) ) // Don't URL encode these according to RFC1738
      strResult.push_back((unsigned char)kar);
    else
      strResult += StringUtils::Format("%%%02.2x", kar);
  }

  strURLData = strResult;
}

std::string CURL::Decode(const std::string& strURLData)
{
  CStdString url = strURLData;
  Decode(url);
  return url;
}

std::string CURL::Encode(const std::string& strURLData)
{
  std::string url = strURLData;
  Encode(url);
  return url;
}

CStdString CURL::TranslateProtocol(const CStdString& prot)
{
  if (prot == "shout"
   || prot == "daap"
   || prot == "dav"
   || prot == "tuxbox"
   || prot == "rss")
   return "http";

  if (prot == "davs")
    return "https";

  return prot;
}

void CURL::GetOptions(std::map<CStdString, CStdString> &options) const
{
  CUrlOptions::UrlOptions optionsMap = m_options.GetOptions();
  for (CUrlOptions::UrlOptions::const_iterator option = optionsMap.begin(); option != optionsMap.end(); option++)
    options[option->first] = option->second.asString();
}

bool CURL::HasOption(const CStdString &key) const
{
  return m_options.HasOption(key);
}

bool CURL::GetOption(const CStdString &key, CStdString &value) const
{
  CVariant valueObj;
  if (!m_options.GetOption(key, valueObj))
    return false;

  value = valueObj.asString();
  return true;
}

CStdString CURL::GetOption(const CStdString &key) const
{
  CStdString value;
  if (!GetOption(key, value))
    return "";

  return value;
}

void CURL::SetOption(const CStdString &key, const CStdString &value)
{
  m_options.AddOption(key, value);
  SetOptions(m_options.GetOptionsString(true));
}

void CURL::RemoveOption(const CStdString &key)
{
  m_options.RemoveOption(key);
  SetOptions(m_options.GetOptionsString(true));
}

void CURL::GetProtocolOptions(std::map<CStdString, CStdString> &options) const
{
  CUrlOptions::UrlOptions optionsMap = m_protocolOptions.GetOptions();
  for (CUrlOptions::UrlOptions::const_iterator option = optionsMap.begin(); option != optionsMap.end(); option++)
    options[option->first] = option->second.asString();
}

bool CURL::HasProtocolOption(const CStdString &key) const
{
  return m_protocolOptions.HasOption(key);
}

bool CURL::GetProtocolOption(const CStdString &key, CStdString &value) const
{
  CVariant valueObj;
  if (!m_protocolOptions.GetOption(key, valueObj))
    return false;
  
  value = valueObj.asString();
  return true;
}

CStdString CURL::GetProtocolOption(const CStdString &key) const
{
  CStdString value;
  if (!GetProtocolOption(key, value))
    return "";
  
  return value;
}

void CURL::SetProtocolOption(const CStdString &key, const CStdString &value)
{
  m_protocolOptions.AddOption(key, value);
  m_strProtocolOptions = m_protocolOptions.GetOptionsString(false);
}

void CURL::RemoveProtocolOption(const CStdString &key)
{
  m_protocolOptions.RemoveOption(key);
  m_strProtocolOptions = m_protocolOptions.GetOptionsString(false);
}
