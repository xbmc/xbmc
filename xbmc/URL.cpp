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
#include "utils/StringUtils.h"
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

CURL::CURL(const CStdString& strURL1)
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

void CURL::Parse(const CStdString& strURL1)
{
  Reset();
  // start by validating the path
  CStdString strURL = CUtil::ValidatePath(strURL1);

  // strURL can be one of the following:
  // format 1: protocol://[username:password]@hostname[:port]/directoryandfile
  // format 2: protocol://file
  // format 3: drive:directoryandfile
  //
  // first need 2 check if this is a protocol or just a normal drive & path
  if (!strURL.size()) return ;
  if (strURL.Equals("?", true)) return;

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
    bool is_apk = (strURL.find(".apk/", iPos) != std::string::npos);
    while (1)
    {
      if (is_apk)
        iPos = strURL.find(".apk/", iPos);
      else
        iPos = strURL.find(".zip/", iPos);

      int extLen = 3;
      if (iPos == std::string::npos)
      {
        /* set filename and update extension*/
        SetFileName(strURL);
        return ;
      }
      iPos += extLen + 1;
      std::string archiveName = strURL.substr(0, iPos);
      struct __stat64 s;
      if (XFILE::CFile::Stat(archiveName, &s) == 0)
      {
#ifdef TARGET_POSIX
        if (!S_ISDIR(s.st_mode))
#else
        if (!(s.st_mode & S_IFDIR))
#endif
        {
          archiveName = Encode(archiveName);
          if (is_apk)
          {
            CURL c("apk://" + archiveName + "/" + strURL.substr(iPos + 1));
            *this = c;
          }
          else
          {
            CURL c("zip://" + archiveName + "/" + strURL.substr(iPos + 1));
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
    m_strProtocol.Equals("filereader") ||
    m_strProtocol.Equals("special")
    )
  {
    SetFileName(strURL.substr(iPos));
    return;
  }

  // check for username/password - should occur before first /
  if (iPos == std::string::npos) iPos = 0;

  // for protocols supporting options, chop that part off here
  // maybe we should invert this list instead?
  size_t iEnd = strURL.length();
  const char* sep = NULL;

  //TODO fix all Addon paths
  CStdString strProtocol2 = GetTranslatedProtocol();
  if(m_strProtocol.Equals("rss") ||
     m_strProtocol.Equals("rar") ||
     m_strProtocol.Equals("addons") ||
     m_strProtocol.Equals("image") ||
     m_strProtocol.Equals("videodb") ||
     m_strProtocol.Equals("musicdb") ||
     m_strProtocol.Equals("androidapp"))
    sep = "?";
  else
  if(strProtocol2.Equals("http")
    || strProtocol2.Equals("https")
    || strProtocol2.Equals("plugin")
    || strProtocol2.Equals("addons")
    || strProtocol2.Equals("hdhomerun")
    || strProtocol2.Equals("rtsp")
    || strProtocol2.Equals("apk")
    || strProtocol2.Equals("zip"))
    sep = "?;#|";
  else if(strProtocol2.Equals("ftp")
       || strProtocol2.Equals("ftps"))
    sep = "?;|";

  if(sep)
  {
    size_t iOptions = strURL.find_first_of(sep, iPos);
    if (iOptions != std::string::npos)
    {
      // we keep the initial char as it can be any of the above
      size_t iProto = strURL.find_first_of("|",iOptions);
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

  size_t iSlash = strURL.find("/", iPos);
  if(iSlash >= iEnd)
    iSlash = std::string::npos; // was an invalid slash as it was contained in options

  if( !m_strProtocol.Equals("iso9660") )
  {
    size_t iAlphaSign = strURL.find("@", iPos);
    if (iAlphaSign != std::string::npos && iAlphaSign < iEnd && (iAlphaSign < iSlash || iSlash == std::string::npos))
    {
      // username/password found
      CStdString strUserNamePassword = strURL.substr(iPos, iAlphaSign - iPos);

      // first extract domain, if protocol is smb
      if (m_strProtocol.Equals("smb"))
      {
        size_t iSemiColon = strUserNamePassword.find(";");

        if (iSemiColon != std::string::npos)
        {
          m_strDomain = strUserNamePassword.substr(0, iSemiColon);
          strUserNamePassword.erase(0, iSemiColon + 1);
        }
      }

      // username:password
      size_t iColon = strUserNamePassword.find(":");
      if (iColon != std::string::npos)
      {
        m_strUserName = strUserNamePassword.substr(0, iColon);
        m_strPassword = strUserNamePassword.substr(iColon + 1);
      }
      // username
      else
      {
        m_strUserName = strUserNamePassword;
      }

      iPos = iAlphaSign + 1;
      iSlash = strURL.find("/", iAlphaSign);

      if(iSlash >= iEnd)
        iSlash = std::string::npos;
    }
  }

  // detect hostname:port/
  if (iSlash == std::string::npos)
  {
    CStdString strHostNameAndPort = strURL.substr(iPos, iEnd - iPos);
    size_t iColon = strHostNameAndPort.find(":");
    if (iColon != std::string::npos)
    {
      m_strHostName = strHostNameAndPort.substr(0, iColon);
      m_iPort = atoi(strHostNameAndPort.substr(iColon + 1).c_str());
    }
    else
    {
      m_strHostName = strHostNameAndPort;
    }

  }
  else
  {
    CStdString strHostNameAndPort = strURL.substr(iPos, iSlash - iPos);
    size_t iColon = strHostNameAndPort.find(":");
    if (iColon != std::string::npos)
    {
      m_strHostName = strHostNameAndPort.substr(0, iColon);
      m_iPort = atoi(strHostNameAndPort.substr(iColon + 1).c_str());
    }
    else
    {
      m_strHostName = strHostNameAndPort;
    }
    iPos = iSlash + 1;
    if (iEnd > iPos)
    {
      m_strFileName = strURL.substr(iPos, iEnd - iPos);

      iSlash = m_strFileName.find("/");
      if(iSlash == std::string::npos)
        m_strShareName = m_strFileName;
      else
        m_strShareName = m_strFileName.substr(0, iSlash);
    }
  }

  // iso9960 doesnt have an hostname;-)
  if (m_strProtocol == "iso9660"
    || m_strProtocol == "musicdb"
    || m_strProtocol == "videodb"
    || m_strProtocol == "sources"
    || m_strProtocol == "pvr"
    || StringUtils::StartsWith(m_strProtocol, "mem"))
  {
    if (m_strHostName != "" && m_strFileName != "")
    {
      m_strFileName = StringUtils::Format("%s/%s", m_strHostName.c_str(), m_strFileName.c_str());
      m_strHostName = "";
    }
    else
    {
      if (!m_strHostName.empty() && strURL[iEnd-1]=='/')
        m_strFileName = m_strHostName + "/";
      else
        m_strFileName = m_strHostName;
      m_strHostName = "";
    }
  }

  StringUtils::Replace(m_strFileName, '\\', '/');

  /* update extension */
  SetFileName(m_strFileName);

  /* decode urlencoding on this stuff */
  if(URIUtils::ProtocolHasEncodedHostname(m_strProtocol))
  {
    m_strHostName = Decode(m_strHostName);
    SetHostName(m_strHostName);
  }

  m_strUserName = Decode(m_strUserName);
  m_strPassword = Decode(m_strPassword);
}

void CURL::SetFileName(const CStdString& strFileName)
{
  m_strFileName = strFileName;

  int slash = m_strFileName.find_last_of(GetDirectorySeparator());
  int period = m_strFileName.find_last_of('.');
  if(period != -1 && (slash == -1 || period > slash))
    m_strFileType = m_strFileName.substr(period+1);
  else
    m_strFileType = "";

  StringUtils::Trim(m_strFileType);
  StringUtils::ToLower(m_strFileType);
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
  StringUtils::ToLower(m_strProtocol);
}

void CURL::SetOptions(const CStdString& strOptions)
{
  m_strOptions.clear();
  m_options.Clear();
  if( strOptions.length() > 0)
  {
    if(strOptions[0] == '?' ||
       strOptions[0] == '#' ||
       strOptions[0] == ';' ||
       strOptions.find("xml") != std::string::npos)
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
  m_strProtocolOptions.clear();
  m_protocolOptions.Clear();
  if (strOptions.length() > 0)
  {
    if (strOptions[0] == '|')
      m_strProtocolOptions = strOptions.substr(1);
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
       m_strFileName.empty())
    return URIUtils::GetFileName(m_strHostName);

  // otherwise, we've already got the filepath, so just grab the filename portion
  CStdString file(m_strFileName);
  URIUtils::RemoveSlashAtEnd(file);
  return URIUtils::GetFileName(file);
}

char CURL::GetDirectorySeparator() const
{
#ifndef TARGET_POSIX
  if ( IsLocal() )
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

  if( !m_strOptions.empty() )
    strURL += m_strOptions;
  if (!m_strProtocolOptions.empty())
    strURL += "|"+m_strProtocolOptions;

  return strURL;
}

std::string CURL::GetWithoutUserDetails(bool redact) const
{
  std::string strURL;

  if (m_strProtocol.Equals("stack"))
  {
    CFileItemList items;
    std::string strURL2;
    strURL2 = Get();
    XFILE::CStackDirectory dir;
    dir.GetDirectory(strURL2,items);
    vector<std::string> newItems;
    for (int i=0;i<items.Size();++i)
    {
      CURL url(items[i]->GetPath());
      items[i]->SetPath(url.GetWithoutUserDetails(redact));
      newItems.push_back(items[i]->GetPath());
    }
    dir.ConstructStackPath(newItems, strURL);
    return strURL;
  }

  unsigned int sizeneed = m_strProtocol.length()
                        + m_strDomain.length()
                        + m_strHostName.length()
                        + m_strFileName.length()
                        + m_strOptions.length()
                        + m_strProtocolOptions.length()
                        + 10;

  if (redact)
    sizeneed += sizeof("USERNAME:PASSWORD@");

  strURL.reserve(sizeneed);

  if (m_strProtocol == "")
    return m_strFileName;

  strURL = m_strProtocol;
  strURL += "://";

  if (redact && !m_strUserName.empty())
  {
    strURL += "USERNAME";
    if (!m_strPassword.empty())
    {
      strURL += ":PASSWORD";
    }
    strURL += "@";
  }

  if (!m_strHostName.empty())
  {
    std::string strHostName;

    if (URIUtils::ProtocolHasParentInHostname(m_strProtocol))
      strHostName = CURL(m_strHostName).GetWithoutUserDetails();
    else
      strHostName = m_strHostName;

    if (URIUtils::ProtocolHasEncodedHostname(m_strProtocol))
      strURL += Encode(strHostName);
    else
      strURL += strHostName;

    if ( HasPort() )
    {
      strURL += StringUtils::Format(":%i", m_iPort);
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

  if (m_strUserName != "")
  {
    strURL += Encode(m_strUserName);
    if (m_strPassword != "")
    {
      strURL += ":";
      strURL += Encode(m_strPassword);
    }
    strURL += "@";
  }
  else if (m_strDomain != "")
    strURL += "@";

  if (m_strHostName != "")
  {
    if( URIUtils::ProtocolHasEncodedHostname(m_strProtocol) )
      strURL += Encode(m_strHostName);
    else
      strURL += m_strHostName;
    if (HasPort())
    {
      CStdString strPort = StringUtils::Format("%i", m_iPort);
      strURL += ":";
      strURL += strPort;
    }
    strURL += "/";
  }

  return strURL;
}

std::string CURL::GetRedacted() const
{
  return GetWithoutUserDetails(true);
}

std::string CURL::GetRedacted(const std::string& path)
{
  return CURL(path).GetRedacted();
}

bool CURL::IsLocal() const
{
  return (IsLocalHost() || m_strProtocol.empty());
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
  if (url.find("://") != std::string::npos) return true;                 //   foo://bar.ext
  if (url.size() > 1 && url[1] == ':') return true; //   c:\\foo\\bar\\bar.ext
  if (StringUtils::StartsWith(url, "\\\\")) return true;    //   \\UNC\path\to\file
  return false;
}

std::string CURL::Decode(const std::string& strURLData)
//modified to be more accomodating - if a non hex value follows a % take the characters directly and don't raise an error.
// However % characters should really be escaped like any other non safe character (www.rfc-editor.org/rfc/rfc1738.txt)
{
  std::string strResult;

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
        std::string strTmp;
        strTmp.assign(strURLData.substr(i + 1, 2));
        int dec_num=-1;
        sscanf(strTmp.c_str(), "%x", (unsigned int *)&dec_num);
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
  
  return strResult;
}

std::string CURL::Encode(const std::string& strURLData)
{
  std::string strResult;

  /* wonder what a good value is here is, depends on how often it occurs */
  strResult.reserve( strURLData.length() * 2 );

  for (size_t i = 0; i < strURLData.size(); ++i)
  {
    const char kar = strURLData[i];
    
    // Don't URL encode "-_.!()" according to RFC1738
    // TODO: Update it to "-_.~" after Gotham according to RFC3986
    if (StringUtils::isasciialphanum(kar) || kar == '-' || kar == '.' || kar == '_' || kar == '!' || kar == '(' || kar == ')')
      strResult.push_back(kar);
    else
      strResult += StringUtils::Format("%%%02.2x", (unsigned int)((unsigned char)kar)); // TODO: Change to "%%%02.2X" after Gotham
  }

  return strResult;
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
