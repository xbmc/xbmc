/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "URL.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "Util.h"
#include "filesystem/File.h"
#include "filesystem/StackDirectory.h"
#include "network/Network.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#ifndef TARGET_POSIX
#include <sys\stat.h>
#endif

#include <charconv>
#include <iterator>
#include <string>
#include <system_error>
#include <vector>

#include <fmt/xchar.h>

using namespace ADDON;

CURL::~CURL() = default;

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

void CURL::Parse(std::string strURL1)
{
  Reset();
  // start by validating the path
  std::string strURL = CUtil::ValidatePath(std::move(strURL1));

  // strURL can be one of the following:
  // format 1: protocol://[username:password]@hostname[:port]/directoryandfile
  // format 2: protocol://file
  // format 3: drive:directoryandfile
  //
  // first need 2 check if this is a protocol or just a normal drive & path
  if (!strURL.size()) return ;
  if (strURL == "?") return;

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
    while (true)
    {
      if (is_apk)
        iPos = strURL.find(".apk/", iPos);
      else
        iPos = strURL.find(".zip/", iPos);

      int extLen = 3;
      if (iPos == std::string::npos)
      {
        /* set filename and update extension*/
        SetFileName(std::move(strURL));
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
            CURL c("apk://" + archiveName + "/" + std::move(strURL).substr(iPos + 1));
            *this = c;
          }
          else
          {
            CURL c("zip://" + archiveName + "/" + std::move(strURL).substr(iPos + 1));
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
    IsProtocol("stack") ||
    IsProtocol("virtualpath") ||
    IsProtocol("multipath") ||
    IsProtocol("special") ||
    IsProtocol("resource")
    )
  {
    SetFileName(std::move(strURL).substr(iPos));
    return;
  }

  if (IsProtocol("udf") || IsProtocol("iso9660"))
  {
    std::string lower(strURL);
    StringUtils::ToLower(lower);
    size_t isoPos = lower.find(".iso\\", iPos);
    if (isoPos == std::string::npos)
      isoPos = lower.find(".udf\\", iPos);
    if (isoPos != std::string::npos)
    {
      strURL.replace(isoPos + 4, 1, "/");
    }
  }

  // check for username/password - should occur before first /
  if (iPos == std::string::npos) iPos = 0;

  // for protocols supporting options, chop that part off here
  // maybe we should invert this list instead?
  size_t iEnd = strURL.length();
  const char* sep = NULL;

  //! @todo fix all Addon paths
  std::string strProtocol2 = GetTranslatedProtocol();
  if(IsProtocol("rss") ||
     IsProtocol("rsss") ||
     IsProtocol("rar") ||
     IsProtocol("apk") ||
     IsProtocol("xbt") ||
     IsProtocol("zip") ||
     IsProtocol("addons") ||
     IsProtocol("image") ||
     IsProtocol("videodb") ||
     IsProtocol("musicdb") ||
     IsProtocol("androidapp") ||
     IsProtocol("pvr"))
    sep = "?";
  else
  if(  IsProtocolEqual(strProtocol2, "http")
    || IsProtocolEqual(strProtocol2, "https")
    || IsProtocolEqual(strProtocol2, "plugin")
    || IsProtocolEqual(strProtocol2, "addons")
    || IsProtocolEqual(strProtocol2, "rtsp"))
    sep = "?;#|";
  else if(IsProtocolEqual(strProtocol2, "ftp")
       || IsProtocolEqual(strProtocol2, "ftps"))
    sep = "?;|";

  if(sep)
  {
    size_t iOptions = strURL.find_first_of(sep, iPos);
    if (iOptions != std::string::npos)
    {
      // we keep the initial char as it can be any of the above
      size_t iProto = strURL.find_first_of('|', iOptions);
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
  if(iSlash >= iEnd)
    iSlash = std::string::npos; // was an invalid slash as it was contained in options

  // also skip parsing username:password@ for udp/rtp as it not valid
  // and conflicts with the following example: rtp://sourceip@multicastip
  size_t iAlphaSign = strURL.find('@', iPos);
  if (iAlphaSign != std::string::npos && iAlphaSign < iEnd &&
      (iAlphaSign < iSlash || iSlash == std::string::npos) &&
      !IsProtocol("udp") && !IsProtocol("rtp"))
  {
    // username/password found
    std::string strUserNamePassword = strURL.substr(iPos, iAlphaSign - iPos);

    // first extract domain, if protocol is smb
    if (IsProtocol("smb"))
    {
      size_t iSemiColon = strUserNamePassword.find(';');

      if (iSemiColon != std::string::npos)
      {
        m_strDomain = strUserNamePassword.substr(0, iSemiColon);
        strUserNamePassword.erase(0, iSemiColon + 1);
      }
    }

    // username:password
    size_t iColon = strUserNamePassword.find(':');
    if (iColon != std::string::npos)
    {
      m_strUserName = strUserNamePassword.substr(0, iColon);
      m_strPassword = strUserNamePassword.substr(iColon + 1);
    }
    // username
    else
    {
      m_strUserName = std::move(strUserNamePassword);
    }

    iPos = iAlphaSign + 1;
    iSlash = strURL.find('/', iAlphaSign);

    if (iSlash >= iEnd)
      iSlash = std::string::npos;
  }

  std::string strHostNameAndPort = strURL.substr(iPos, (iSlash == std::string::npos) ? iEnd - iPos : iSlash - iPos);
  // check for IPv6 numerical representation inside [].
  // if [] found, let's store string inside as hostname
  // and remove that parsed part from strHostNameAndPort
  size_t iBrk = strHostNameAndPort.rfind(']');
  if (iBrk != std::string::npos && strHostNameAndPort.find('[') == 0)
  {
    m_strHostName = strHostNameAndPort.substr(1, iBrk-1);
    strHostNameAndPort.erase(0, iBrk+1);
  }

  // detect hostname:port/ or just :port/ if previous step found [IPv6] format
  size_t iColon = strHostNameAndPort.rfind(':');
  if (iColon != std::string::npos && iColon == strHostNameAndPort.find(':'))
  {
    if (m_strHostName.empty())
      m_strHostName = strHostNameAndPort.substr(0, iColon);
    m_iPort = atoi(strHostNameAndPort.substr(iColon + 1).c_str());
  }

  // if we still don't have hostname, the strHostNameAndPort substring
  // is 'just' hostname without :port specification - so use it as is.
  if (m_strHostName.empty())
    m_strHostName = std::move(strHostNameAndPort);

  if (iSlash != std::string::npos)
  {
    iPos = iSlash + 1;
    if (iEnd > iPos)
      m_strFileName = strURL.substr(iPos, iEnd - iPos);
  }

  if (IsProtocol("musicdb") || IsProtocol("videodb") || IsProtocol("sources") || IsProtocol("pvr"))
  {
    if (m_strHostName != "" && m_strFileName != "")
    {
      m_strFileName = StringUtils::Format("{}/{}", m_strHostName, m_strFileName);
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

  /* update extension + sharename */
  SetFileName(m_strFileName);

  /* decode urlencoding on this stuff */
  if(URIUtils::HasEncodedHostname(*this))
  {
    m_strHostName = Decode(m_strHostName);
    SetHostName(m_strHostName);
  }

  m_strUserName = Decode(m_strUserName);
  m_strPassword = Decode(m_strPassword);
}

void CURL::SetFileName(std::string strFileName)
{
  m_strFileName = std::move(strFileName);

  size_t slash = m_strFileName.find_last_of(GetDirectorySeparator());
  size_t period = m_strFileName.find_last_of('.');
  if(period != std::string::npos && (slash == std::string::npos || period > slash))
    m_strFileType = m_strFileName.substr(period+1);
  else
    m_strFileType = "";

  slash = m_strFileName.find_first_of(GetDirectorySeparator());
  if(slash == std::string::npos)
    m_strShareName = m_strFileName;
  else
    m_strShareName = m_strFileName.substr(0, slash);

  StringUtils::Trim(m_strFileType);
  StringUtils::ToLower(m_strFileType);
}

void CURL::SetProtocol(std::string strProtocol)
{
  m_strProtocol = std::move(strProtocol);
  StringUtils::ToLower(m_strProtocol);
}

void CURL::SetOptions(std::string strOptions)
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
      m_strOptions = std::move(strOptions);
      m_options.AddOptions(m_strOptions);
    }
    else
      CLog::Log(LOGWARNING, "{} - Invalid options specified for url {}", __FUNCTION__, strOptions);
  }
}

void CURL::SetProtocolOptions(std::string strOptions)
{
  m_strProtocolOptions.clear();
  m_protocolOptions.Clear();
  if (strOptions.length() > 0)
  {
    if (strOptions[0] == '|')
      m_strProtocolOptions = std::move(strOptions).substr(1);
    else
      m_strProtocolOptions = std::move(strOptions);
    m_protocolOptions.AddOptions(m_strProtocolOptions);
  }
}

std::string CURL::GetTranslatedProtocol() const
{
  if (IsProtocol("shout")
   || IsProtocol("dav")
   || IsProtocol("rss"))
    return "http";

  if (IsProtocol("davs")
   || IsProtocol("rsss"))
    return "https";

  return GetProtocol();
}

std::string CURL::GetFileNameWithoutPath() const
{
  // *.zip and *.rar store the actual zip/rar path in the hostname of the url
  if ((IsProtocol("rar")  ||
       IsProtocol("zip")  ||
       IsProtocol("xbt")  ||
       IsProtocol("apk")) &&
       m_strFileName.empty())
    return URIUtils::GetFileName(m_strHostName);

  // otherwise, we've already got the filepath, so just grab the filename portion
  std::string file(m_strFileName);
  URIUtils::RemoveSlashAtEnd(file);
  return URIUtils::GetFileName(file);
}

inline
void protectIPv6(std::string &hn)
{
  if (!hn.empty() && hn.find(':') != hn.rfind(':') && hn.find(':') != std::string::npos)
  {
    hn = '[' + hn + ']';
  }
}

char CURL::GetDirectorySeparator() const
{
#ifndef TARGET_POSIX
  //We don't want to use IsLocal here, it can return true
  //for network protocols that matches localhost or hostname
  //we only ever want to use \ for win32 local filesystem
  if ( m_strProtocol.empty() )
    return '\\';
  else
#endif
    return '/';
}

std::string CURL::Get() const
{
  if (m_strProtocol.empty())
    return m_strFileName;

  unsigned int sizeneed = m_strProtocol.length()
                        + m_strDomain.length()
                        + m_strUserName.length()
                        + m_strPassword.length()
                        + m_strHostName.length()
                        + m_strFileName.length()
                        + m_strOptions.length()
                        + m_strProtocolOptions.length()
                        + 10;

  std::string strURL;
  strURL.reserve(sizeneed);

  strURL = GetWithoutOptions();

  if( !m_strOptions.empty() )
    strURL += m_strOptions;

  if (!m_strProtocolOptions.empty())
    strURL += "|"+m_strProtocolOptions;

  return strURL;
}

std::string CURL::GetWithoutOptions() const
{
  if (m_strProtocol.empty())
    return m_strFileName;

  std::string strGet = GetWithoutFilename();

  // Prevent double slash when concatenating host part and filename part
  if (m_strFileName.size() && (m_strFileName[0] == '/' || m_strFileName[0] == '\\') && URIUtils::HasSlashAtEnd(strGet))
    URIUtils::RemoveSlashAtEnd(strGet);

  return strGet + m_strFileName;
}

std::string CURL::GetWithoutUserDetails(bool redact) const
{
  std::string strURL;

  if (IsProtocol("stack"))
  {
    CFileItemList items;
    XFILE::CStackDirectory dir;
    dir.GetDirectory(*this,items);
    std::vector<std::string> newItems;
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
                        + m_strHostName.length()
                        + m_strFileName.length()
                        + m_strOptions.length()
                        + m_strProtocolOptions.length()
                        + 10;

  if (redact && !m_strUserName.empty())
  {
    sizeneed += sizeof("USERNAME");
    if (!m_strPassword.empty())
      sizeneed += sizeof(":PASSWORD@");
    if (!m_strDomain.empty())
      sizeneed += sizeof("DOMAIN;");
  }

  strURL.reserve(sizeneed);

  if (m_strProtocol.empty())
    return m_strFileName;

  strURL = m_strProtocol;
  strURL += "://";

  if (redact && !m_strUserName.empty())
  {
    if (!m_strDomain.empty())
      strURL += "DOMAIN;";
    strURL += "USERNAME";
    if (!m_strPassword.empty())
      strURL += ":PASSWORD";
    strURL += "@";
  }

  if (!m_strHostName.empty())
  {
    std::string strHostName;

    if (URIUtils::HasParentInHostname(*this))
      strHostName = CURL(m_strHostName).GetWithoutUserDetails();
    else
      strHostName = m_strHostName;

    if (URIUtils::HasEncodedHostname(*this))
      strHostName = Encode(strHostName);

    if ( HasPort() )
    {
      protectIPv6(strHostName);
      strURL += strHostName + StringUtils::Format(":{}", m_iPort);
    }
    else
      strURL += strHostName;

    strURL += "/";
  }
  strURL += m_strFileName;

  if( m_strOptions.length() > 0 )
    strURL += m_strOptions;
  if( m_strProtocolOptions.length() > 0 )
    strURL += "|"+m_strProtocolOptions;

  return strURL;
}

std::string CURL::GetWithoutFilename() const
{
  if (m_strProtocol.empty())
    return "";

  unsigned int sizeneed = m_strProtocol.length()
                        + m_strDomain.length()
                        + m_strUserName.length()
                        + m_strPassword.length()
                        + m_strHostName.length()
                        + 10;

  std::string strURL;
  strURL.reserve(sizeneed);

  strURL = m_strProtocol;
  strURL += "://";

  if (!m_strUserName.empty())
  {
    if (!m_strDomain.empty())
    {
      strURL += Encode(m_strDomain);
      strURL += ";";
    }
    strURL += Encode(m_strUserName);
    if (!m_strPassword.empty())
    {
      strURL += ":";
      strURL += Encode(m_strPassword);
    }
    strURL += "@";
  }

  if (!m_strHostName.empty())
  {
    std::string hostname;

    if( URIUtils::HasEncodedHostname(*this) )
      hostname = Encode(m_strHostName);
    else
      hostname = m_strHostName;

    if (HasPort())
    {
      protectIPv6(hostname);
      strURL += hostname + StringUtils::Format(":{}", m_iPort);
    }
    else
      strURL += hostname;

    strURL += "/";
  }

  return strURL;
}

std::string CURL::GetRedacted() const
{
  return GetWithoutUserDetails(true);
}

std::string CURL::GetRedacted(std::string path)
{
  return CURL(std::move(path)).GetRedacted();
}

bool CURL::IsLocal() const
{
  return (m_strProtocol.empty() || IsLocalHost() || IsProtocol("win-lib"));
}

bool CURL::IsLocalHost() const
{
  return CServiceBroker::GetNetwork().IsLocalHost(m_strHostName);
}

bool CURL::IsFileOnly(const std::string &url)
{
  return url.find_first_of("/\\") == std::string::npos;
}

bool CURL::IsFullPath(const std::string &url)
{
  if (url.size() && url[0] == '/') return true;     //   /foo/bar.ext
  if (url.find("://") != std::string::npos) return true;                 //   foo://bar.ext
  if (url.size() > 1 && url[1] == ':') return true; //   c:\\foo\\bar\\bar.ext
  if (StringUtils::StartsWith(url, "\\\\")) return true;    //   \\UNC\path\to\file
  return false;
}

std::string CURL::Decode(std::string_view strURLData)
//modified to be more accommodating - if a non hex value follows a % take the characters directly and don't raise an error.
// However % characters should really be escaped like any other non safe character (www.rfc-editor.org/rfc/rfc1738.txt)
{
  std::string strResult;

  /* result will always be less than source */
  strResult.reserve( strURLData.length() );

  const char* const iterEnd = strURLData.data() + strURLData.size();
  for (const char* iter = strURLData.data(); iter < iterEnd; ++iter)
  {
    if (*iter == '+')
      strResult += ' ';
    else if (*iter == '%')
    {
      if (std::distance(iter, iterEnd) >= 3)
      {
        uint8_t dec_num{};
        const std::from_chars_result res = std::from_chars(iter + 1, iter + 3, dec_num, 16);
        if (res.ec != std::errc() || res.ptr != iter + 3)
          strResult += *iter;
        else
        {
          strResult += (char)dec_num;
          iter += 2;
        }
      }
      else
        strResult += *iter;
    }
    else
      strResult += *iter;
  }

  return strResult;
}

std::string CURL::Encode(std::string_view strURLData)
{
  std::string strResult;

  /* wonder what a good value is here is, depends on how often it occurs */
  strResult.reserve( strURLData.length() * 2 );

  for (auto kar : strURLData)
  {
    // Don't URL encode "-_.!()" according to RFC1738
    //! @todo Update it to "-_.~" after Gotham according to RFC3986
    if (StringUtils::isasciialphanum(kar) || kar == '-' || kar == '.' || kar == '_' || kar == '!' || kar == '(' || kar == ')')
      strResult.push_back(kar);
    else
      fmt::format_to(std::back_insert_iterator(strResult), "%{:02x}",
                     (unsigned int)((unsigned char)kar));
  }

  return strResult;
}

bool CURL::IsProtocolEqual(const std::string &protocol, const char *type)
{
  /*
   NOTE: We're currently using == here as m_strProtocol is assigned as lower-case in SetProtocol(),
   and we've assumed all other callers are calling with protocol lower-case otherwise.
   We possibly shouldn't do this (as CURL(foo).Get() != foo, though there are other reasons for this as well)
   but it handles the requirements of RFC-1738 which allows the scheme to be case-insensitive.
   */
  if (type)
    return protocol == type;
  return false;
}

void CURL::GetOptions(std::map<std::string, std::string> &options) const
{
  CUrlOptions::UrlOptions optionsMap = m_options.GetOptions();
  for (CUrlOptions::UrlOptions::const_iterator option = optionsMap.begin();
       option != optionsMap.end(); ++option)
    options[option->first] = option->second.asString();
}

bool CURL::HasOption(const std::string &key) const
{
  return m_options.HasOption(key);
}

bool CURL::GetOption(const std::string &key, std::string &value) const
{
  CVariant valueObj;
  if (!m_options.GetOption(key, valueObj))
    return false;

  value = valueObj.asString();
  return true;
}

std::string CURL::GetOption(const std::string &key) const
{
  std::string value;
  if (!GetOption(key, value))
    return "";

  return value;
}

void CURL::SetOption(const std::string &key, const std::string &value)
{
  m_options.AddOption(key, value);
  SetOptions(m_options.GetOptionsString(true));
}

void CURL::RemoveOption(const std::string &key)
{
  m_options.RemoveOption(key);
  SetOptions(m_options.GetOptionsString(true));
}

void CURL::GetProtocolOptions(std::map<std::string, std::string> &options) const
{
  CUrlOptions::UrlOptions optionsMap = m_protocolOptions.GetOptions();
  for (CUrlOptions::UrlOptions::const_iterator option = optionsMap.begin();
       option != optionsMap.end(); ++option)
    options[option->first] = option->second.asString();
}

bool CURL::HasProtocolOption(const std::string &key) const
{
  return m_protocolOptions.HasOption(key);
}

bool CURL::GetProtocolOption(const std::string &key, std::string &value) const
{
  CVariant valueObj;
  if (!m_protocolOptions.GetOption(key, valueObj))
    return false;

  value = valueObj.asString();
  return true;
}

std::string CURL::GetProtocolOption(const std::string &key) const
{
  std::string value;
  if (!GetProtocolOption(key, value))
    return "";

  return value;
}

void CURL::SetProtocolOption(const std::string &key, const std::string &value)
{
  m_protocolOptions.AddOption(key, value);
  m_strProtocolOptions = m_protocolOptions.GetOptionsString(false);
}

void CURL::RemoveProtocolOption(const std::string &key)
{
  m_protocolOptions.RemoveOption(key);
  m_strProtocolOptions = m_protocolOptions.GetOptionsString(false);
}
