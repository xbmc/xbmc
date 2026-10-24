/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "URL.h"

#include "ServiceBroker.h"
#include "Util.h"
#include "filesystem/File.h"
#include "network/Network.h"
#include "utils/FileExtensionProvider.h"
#include "utils/StringUtils.h"
#include "utils/URIPolicy.h"
#include "utils/URIRedactor.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#ifndef TARGET_POSIX
#include <sys/stat.h>
#endif

#include <array>
#include <string>

#include <fmt/xchar.h>

CURL::CURL(CURL&& other)
{
  *this = std::move(other);
}

CURL& CURL::operator=(CURL&& other)
{
  if (this != &other)
  {
    m_uri = std::move(other.m_uri);
    m_iPort = other.m_iPort;
    m_strHostName = std::move(other.m_strHostName);
    m_strShareName = std::move(other.m_strShareName);
    m_strDomain = std::move(other.m_strDomain);
    m_strUserName = std::move(other.m_strUserName);
    m_strPassword = std::move(other.m_strPassword);
    m_strFileName = std::move(other.m_strFileName);
    m_strProtocol = std::move(other.m_strProtocol);
    m_strFileType = std::move(other.m_strFileType);
    m_strOptions = std::move(other.m_strOptions);
    m_strProtocolOptions = std::move(other.m_strProtocolOptions);
    m_options = std::move(other.m_options);
    m_protocolOptions = std::move(other.m_protocolOptions);
    other.Reset();
  }
  return *this;
}

CURL::CURL(std::string strURL)
{
  Parse(std::move(strURL));
}

void CURL::Reset()
{
  m_uri.reset();
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

bool CURL::ParseURI(std::string uri)
{
  if (!URIParser::GetScheme(uri))
  {
    Reset();
    return false;
  }
  StoreURI(std::move(uri));
  return m_uri->IsValid();
}

void CURL::ParseNativePath(std::string path)
{
  Reset();
  UpdateFileName(std::move(path));
}

void CURL::StoreURI(std::string uri)
{
  Reset();
  m_uri.emplace(std::move(uri));
  UpdateURIProjection();
}

void CURL::UpdateURIProjection()
{
  m_strProtocol = m_uri->GetRawScheme();
  StringUtils::ToLower(m_strProtocol);
  m_strFileName.clear();
  m_strFileType.clear();
  m_strShareName.clear();
  m_strOptions.clear();
  if (m_uri->HasQuery())
    m_strOptions = "?" + std::string(m_uri->GetRawQuery());
  if (SupportsPathOperations())
    UpdateFileName(std::string(m_uri->GetRawPath()));
}

bool CURL::SupportsPathOperations() const
{
  return !m_uri || URIPolicy::SupportsPathOperations(*m_uri);
}

bool CURL::PreservesQueryIdentity() const
{
  return m_uri && URIPolicy::GetPolicy(m_strProtocol).queryIdentity;
}

std::string CURL::GetRawScheme() const
{
  return m_uri ? std::string(m_uri->GetRawScheme()) : m_strProtocol;
}

std::string CURL::GetRawAuthority() const
{
  if (m_uri)
    return std::string(m_uri->GetRawAuthority());
  if (m_strProtocol.empty())
    return {};
  const auto source = Get();
  const auto parts = URIParser::ScanReference(source);
  return parts.authority ? std::string(parts.authority->Get(source)) : std::string{};
}

std::string CURL::GetRawPath() const
{
  if (m_uri)
    return std::string(m_uri->GetRawPath());
  if (m_strProtocol.empty())
    return m_strFileName;
  const auto source = Get();
  return std::string(URIParser::ScanReference(source).path.Get(source));
}

std::string CURL::GetRawQuery() const
{
  if (m_uri)
    return std::string(m_uri->GetRawQuery());
  if (m_strProtocol.empty())
    return {};
  const auto source = Get();
  const auto parts = URIParser::ScanReference(source);
  return parts.query ? std::string(parts.query->Get(source)) : std::string{};
}

std::string CURL::GetRawFragment() const
{
  if (m_uri)
    return std::string(m_uri->GetRawFragment());
  if (m_strProtocol.empty())
    return {};
  const auto source = Get();
  const auto parts = URIParser::ScanReference(source);
  return parts.fragment ? std::string(parts.fragment->Get(source)) : std::string{};
}

bool CURL::HasAuthority() const
{
  return !m_strProtocol.empty() &&
         (m_uri ? m_uri->HasAuthority() : URIParser::ScanReference(Get()).authority.has_value());
}

bool CURL::HasQuery() const
{
  return !m_strProtocol.empty() &&
         (m_uri ? m_uri->HasQuery() : URIParser::ScanReference(Get()).query.has_value());
}

bool CURL::HasFragment() const
{
  return !m_strProtocol.empty() &&
         (m_uri ? m_uri->HasFragment() : URIParser::ScanReference(Get()).fragment.has_value());
}

std::string CURL::GetSchemeSpecificPart() const
{
  if (m_uri)
    return std::string(m_uri->GetSchemeSpecificPart());
  if (m_strProtocol.empty())
    return {};
  const auto source = Get();
  const auto parts = URIParser::ScanReference(source);
  const auto begin = parts.scheme->length + 1;
  const auto end = parts.fragment ? parts.fragment->offset - 1 : source.size();
  return source.substr(begin, end - begin);
}

bool CURL::SetURIPath(std::string path)
{
  if (!m_uri || !URIPolicy::CanSetPath(*m_uri, path) || !m_uri->SetPath(std::move(path)))
    return false;
  UpdateURIProjection();
  return true;
}

bool CURL::SetRawQuery(std::optional<std::string> query)
{
  if (!m_uri || !m_uri->SetRawQuery(std::move(query)))
    return false;
  UpdateURIProjection();
  return true;
}

void CURL::Parse(std::string strURL1)
{
  Reset();
  const bool isURI = URIPolicy::IsURILocation(strURL1);
  if (isURI && !URIPolicy::UsesLegacyURL(strURL1))
  {
    StoreURI(std::move(strURL1));
    return;
  }
  // Identify native intent before compatibility normalization can rewrite a URI body.
  std::string strURL = CUtil::ValidatePath(std::move(strURL1));
  auto components = URIParser::ScanReference(strURL);

  // strURL can be one of the following:
  // format 1: protocol://[username:password]@hostname[:port]/directoryandfile
  // format 2: protocol://file
  // format 3: drive:directoryandfile
  //
  // first need 2 check if this is a protocol or just a normal drive & path
  if (strURL.empty())
    return;

  // form is format 1 or 2
  // format 1: protocol://[domain;][username:password]@hostname[:port]/directoryandfile
  // format 2: protocol://file

  // decode protocol
  size_t iPos = isURI && components.authority ? components.scheme->length : std::string::npos;
  if (iPos == std::string::npos)
  {
    /* Consider files with the extensions {.zip .apk} as special if they exist
     * on the filesystem and convert the archive paths into archive protocols.
     * Example:
     *   [input]  /foo/bar.zip/alice.apk/bob.avi
     *   [result] zip://apk:///foo/bar.zip/alice.apk/bob.avi
     */
    static constexpr const std::array<std::array<std::string_view, 2>, 4> protocolReplacements{{
        {{".zip/", "zip://"}},
        {{".apk/", "apk://"}},
        {{".zip\\", "zip://"}},
        {{".apk\\", "apk://"}},
    }};

    for (const auto& [ext, proto] : protocolReplacements)
    {
      for (size_t extPos = strURL.find(ext); extPos != std::string::npos;
           extPos = strURL.find(ext, extPos))
      {
        extPos += 4; // std::strlen(ext) -1;
        const auto archiveName = strURL.substr(0, extPos);

        if (XFILE::CFile::FileExists(archiveName))
        {
          // The protocols require / separator on all platforms
          *this = CURL(std::string(proto) + Encode(archiveName) + '/' + strURL.substr(extPos + 1));
          return;
        }
      }
    }

    /* set filename and update extension*/
    SetFileName(std::move(strURL));
    return;
  }
  m_strProtocol = strURL.substr(0, iPos);
  StringUtils::ToLower(m_strProtocol);
  iPos += 3;

  // virtual protocols
  // why not handle all format 2 (protocol://file) style urls here?
  // ones that come to mind are iso9660, cdda, musicdb, etc.
  // they are all local protocols and have no server part, port number, special options, etc.
  // this removes the need for special handling below.
  if (IsProtocol("stack") || IsProtocol("virtualpath") || IsProtocol("multipath") ||
      IsProtocol("special") || IsProtocol("resource") || IsProtocol("file"))
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
      components = URIParser::ScanReference(strURL);
    }
  }

  // for protocols supporting options, chop that part off here
  // maybe we should invert this list instead?
  size_t iEnd = strURL.length();
  const char* sep = NULL;

  //! @todo fix all Addon paths
  std::string strProtocol2 = GetTranslatedProtocol();
  if (IsProtocol("rss") || IsProtocol("rsss") || IsProtocol("rar") || IsProtocol("apk") ||
      IsProtocol("xbt") || IsProtocol("zip") || IsProtocol("addons") || IsProtocol("image") ||
      IsProtocol("videodb") || IsProtocol("musicdb") || IsProtocol("androidapp") ||
      IsProtocol("pvr") || IsProtocol("bluray") || IsProtocol("episodes"))
    sep = "?";
  else if (IsProtocolEqual(strProtocol2, "http") || IsProtocolEqual(strProtocol2, "https") ||
           IsProtocolEqual(strProtocol2, "plugin") || IsProtocolEqual(strProtocol2, "rtsp"))
    sep = "?;#|";
  else if (IsProtocolEqual(strProtocol2, "ftp") || IsProtocolEqual(strProtocol2, "ftps") ||
           IsProtocolEqual(strProtocol2, "nfs"))
    sep = "?|";

  if (sep)
  {
    size_t iOptions = strURL.find_first_of(sep, iPos);
    if (iOptions != std::string::npos)
    {
      // we keep the initial char as it can be any of the above
      size_t iProto = URIPolicy::GetPolicy(m_strProtocol).protocolOptions
                          ? strURL.find_first_of('|', iOptions)
                          : std::string::npos;
      if (iProto != std::string::npos)
      {
        SetProtocolOptions(strURL.substr(iProto + 1));
        SetOptions(strURL.substr(iOptions, iProto - iOptions));
      }
      else
        SetOptions(strURL.substr(iOptions));
      iEnd = iOptions;
    }
  }

  size_t iSlash = components.path.length > 0 && strURL[components.path.offset] == '/'
                      ? components.path.offset
                      : std::string::npos;
  if (iSlash >= iEnd)
    iSlash = std::string::npos; // was an invalid slash as it was contained in options

  // also skip parsing username:password@ for udp/rtp as it not valid
  // and conflicts with the following example: rtp://sourceip@multicastip
  size_t iAlphaSign = strURL.find('@', iPos);
  if (iAlphaSign != std::string::npos && iAlphaSign < iEnd &&
      (iAlphaSign < iSlash || iSlash == std::string::npos) && !IsProtocol("udp") &&
      !IsProtocol("rtp"))
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

  std::string strHostNameAndPort =
      strURL.substr(iPos, (iSlash == std::string::npos) ? iEnd - iPos : iSlash - iPos);
  // check for IPv6 numerical representation inside [].
  // if [] found, let's store string inside as hostname
  // and remove that parsed part from strHostNameAndPort
  size_t iBrk = strHostNameAndPort.rfind(']');
  if (iBrk != std::string::npos && strHostNameAndPort.starts_with('['))
  {
    m_strHostName = strHostNameAndPort.substr(1, iBrk - 1);
    strHostNameAndPort.erase(0, iBrk + 1);
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
    if (!m_strHostName.empty() && !m_strFileName.empty())
    {
      m_strFileName = StringUtils::Format("{}/{}", m_strHostName, m_strFileName);
      m_strHostName.clear();
    }
    else
    {
      if (!m_strHostName.empty() && strURL[iEnd - 1] == '/')
        m_strFileName = m_strHostName + "/";
      else
        m_strFileName = m_strHostName;
      m_strHostName.clear();
    }
  }

  StringUtils::Replace(m_strFileName, '\\', '/');

  /* update extension + sharename */
  SetFileName(m_strFileName);

  /* decode urlencoding on this stuff */
  if (HasEncodedHostname())
  {
    m_strHostName = Decode(m_strHostName);
    SetHostName(m_strHostName);
  }

  m_strDomain = Decode(m_strDomain);
  m_strUserName = Decode(m_strUserName);
  m_strPassword = Decode(m_strPassword);
}

void CURL::SetFileName(std::string strFileName)
{
  if (m_uri)
  {
    SetURIPath(std::move(strFileName));
    return;
  }
  UpdateFileName(std::move(strFileName));
}

void CURL::UpdateFileName(std::string strFileName)
{
  m_strFileName = std::move(strFileName);

  size_t slash = m_strFileName.find_last_of(GetDirectorySeparator());
  size_t period = m_strFileName.find_last_of('.');
  if (period != std::string::npos && (slash == std::string::npos || period > slash))
    m_strFileType = m_strFileName.substr(period + 1);
  else
    m_strFileType = "";

  slash = m_strFileName.find_first_of(GetDirectorySeparator());
  if (slash == std::string::npos)
    m_strShareName = m_strFileName;
  else
    m_strShareName = m_strFileName.substr(0, slash);

  StringUtils::Trim(m_strFileType);
  StringUtils::ToLower(m_strFileType);
}

void CURL::SetProtocol(std::string strProtocol)
{
  if (!strProtocol.empty())
  {
    const std::string candidate = strProtocol + ":";
    const auto scheme = URIParser::GetScheme(candidate);
    if (!scheme || scheme->size() != strProtocol.size())
      return;
  }
  StringUtils::ToLower(strProtocol);
  if (strProtocol == m_strProtocol)
    return;
  if (strProtocol.empty())
  {
    ParseNativePath(m_uri ? GetRawPath() : m_strFileName);
    return;
  }
  if (m_uri)
  {
    const std::string changed = strProtocol + m_uri->Get().substr(m_uri->GetRawScheme().size());
    if (URIPolicy::GetPolicy(strProtocol).legacy && m_uri->HasAuthority())
      Parse(changed);
    else
      StoreURI(changed);
    return;
  }
  if (!strProtocol.empty() && !URIPolicy::GetPolicy(strProtocol).legacy &&
      (strProtocol.size() != 1 || !m_strProtocol.empty()))
  {
    const auto source = Get();
    if (m_strProtocol.empty())
      StoreURI(strProtocol + ":" + source);
    else
      StoreURI(strProtocol + source.substr(m_strProtocol.size()));
    return;
  }
  m_strProtocol = std::move(strProtocol);
}

void CURL::SetOptions(std::string strOptions)
{
  if (m_uri)
  {
    CLog::Log(LOGWARNING, "{} - lossless URI options require explicit raw mutation", __FUNCTION__);
    return;
  }
  m_strOptions.clear();
  m_options.Clear();
  if (!strOptions.empty())
  {
    if (strOptions[0] == '?' || strOptions[0] == '#' || strOptions[0] == ';' ||
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
  if (m_uri)
  {
    CLog::Log(LOGWARNING, "{} - lossless URI options require explicit raw mutation", __FUNCTION__);
    return;
  }
  m_strProtocolOptions.clear();
  m_protocolOptions.Clear();
  if (!strOptions.empty())
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
  if (IsProtocol("shout") || IsProtocol("dav") || IsProtocol("rss"))
    return "http";

  if (IsProtocol("shouts") || IsProtocol("davs") || IsProtocol("rsss"))
    return "https";

  return GetProtocol();
}

std::string CURL::GetFileNameWithoutPath() const
{
  if (!SupportsPathOperations())
    return {};
  // *.zip and *.rar store the actual zip/rar path in the hostname of the url
  if ((IsProtocol("rar") || IsProtocol("zip") || IsProtocol("xbt") || IsProtocol("apk")) &&
      m_strFileName.empty())
    return URIUtils::GetFileName(m_strHostName);

  // otherwise, we've already got the filepath, so just grab the filename portion
  std::string file(m_strFileName);
  while (!file.empty() && (file.back() == '/' || file.back() == '\\'))
    file.pop_back();
  const auto slash = file.find_last_of("/\\");
  const size_t begin = slash != std::string::npos                                   ? slash + 1
                       : m_strProtocol.empty() && file.size() > 1 && file[1] == ':' ? 2
                                                                                    : 0;
  return file.substr(begin);
}

inline void protectIPv6(std::string& hn)
{
  if (!hn.empty() && hn.find(':') != hn.rfind(':') && hn.find(':') != std::string::npos)
  {
    hn = '[' + hn + ']';
  }
}

char CURL::GetDirectorySeparator() const
{
  if (m_uri || (!m_strProtocol.empty() && !IsProtocol("file") && !IsProtocol("stack") &&
                !IsProtocol("multipath")))
    return '/';
  if (URIUtils::IsDOSPath(m_strFileName))
    return '\\';
  if (URIUtils::IsAbsolutePOSIXPath(m_strFileName))
    return '/';
#ifndef TARGET_POSIX
  //We don't want to use IsLocal here, it can return true
  //for network protocols that matches localhost or hostname
  //we only ever want to use \ for win32 local filesystem
  if (m_strProtocol.empty())
    return '\\';
#endif
  return '/';
}

std::string CURL::Get() const
{
  if (m_uri)
    return m_uri->Get();
  if (m_strProtocol.empty())
    return m_strFileName;

  std::string strURL = GetWithoutOptions();

  if (!m_strOptions.empty())
    strURL += m_strOptions;

  if (!m_strProtocolOptions.empty())
    strURL += "|" + m_strProtocolOptions;

  return strURL;
}

std::string CURL::GetWithoutOptions() const
{
  if (m_uri)
    return std::string(m_uri->GetWithoutQueryAndFragment());
  if (m_strProtocol.empty())
    return m_strFileName;

  std::string strGet = GetWithoutFilename();

  // Prevent double slash when concatenating host part and filename part
  if (!m_strFileName.empty() && (m_strFileName[0] == '/' || m_strFileName[0] == '\\') &&
      URIUtils::HasSlashAtEnd(strGet) && !(IsProtocol("http") || IsProtocol("https")))
  {
    URIUtils::RemoveSlashAtEnd(strGet);
  }

  return strGet + m_strFileName;
}

std::string CURL::GetWithoutUserDetails(bool redact) const
{
  return CURIRedactor::GetWithoutUserDetails(*this, redact);
}

std::string CURL::GetWithoutFilename() const
{
  if (m_uri)
  {
    if (!SupportsPathOperations())
      return {};
    return std::string(m_uri->GetPathPrefix());
  }
  if (m_strProtocol.empty())
    return "";

  unsigned int sizeneed = m_strProtocol.length() + m_strDomain.length() + m_strUserName.length() +
                          m_strPassword.length() + m_strHostName.length() + 10;

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

    if (HasEncodedHostname())
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

std::string CURL::GetRedacted(std::string_view path)
{
  return CURIRedactor::GetRedacted(path);
}

bool CURL::IsLocal() const
{
  return m_strProtocol.empty() || (!m_uri && IsLocalHost()) || IsProtocol("win-lib") ||
         (m_uri && IsProtocol("file") && !m_uri->HasAuthority());
}

bool CURL::IsLocalHost() const
{
  return CServiceBroker::GetNetwork().IsLocalHost(m_strHostName);
}

bool CURL::IsFullPath(const std::string& url)
{
  if (!url.empty() && url[0] == '/') // /foo/bar.ext
    return true;
  // Identifiers are absolute locations without requiring a backend.
  if (URIPolicy::IsURILocation(url))
    return true;
  if (url.size() > 1 && url[1] == ':') // c:\\foo\\bar\\bar.ext
    return true;
  if (StringUtils::StartsWith(url, "\\\\")) // \\UNC\path\to\file
    return true;
  return false;
}

std::string CURL::Decode(std::string_view strURLData)
{
  return URIUtils::URLDecode(strURLData);
}

std::string CURL::Encode(std::string_view strURLData)
{
  return URIUtils::URLEncode(strURLData);
}

bool CURL::IsProtocolEqual(const std::string& protocol, std::string_view type)
{
  /*
   NOTE: We're currently using == here as m_strProtocol is assigned as lower-case in SetProtocol(),
   and we've assumed all other callers are calling with protocol lower-case otherwise.
   We possibly shouldn't do this (as CURL(foo).Get() != foo, though there are other reasons for this as well)
   but it handles the requirements of RFC-1738 which allows the scheme to be case-insensitive.
   */
  return protocol == type;
}

void CURL::GetOptions(std::map<std::string, std::string>& options) const
{
  CUrlOptions::UrlOptions optionsMap = m_options.GetOptions();
  for (CUrlOptions::UrlOptions::const_iterator option = optionsMap.begin();
       option != optionsMap.end(); ++option)
    options[option->first] = option->second.asString();
}

bool CURL::HasOption(const std::string& key) const
{
  return m_options.HasOption(key);
}

bool CURL::GetOption(const std::string& key, std::string& value) const
{
  CVariant valueObj;
  if (!m_options.GetOption(key, valueObj))
    return false;

  value = valueObj.asString();
  return true;
}

std::string CURL::GetOption(const std::string& key) const
{
  std::string value;
  if (!GetOption(key, value))
    return "";

  return value;
}

void CURL::SetOption(const std::string& key, const std::string& value)
{
  if (m_uri)
  {
    CLog::Log(LOGWARNING, "{} - lossless URI options require explicit raw mutation", __FUNCTION__);
    return;
  }
  m_options.AddOption(key, value);
  SetOptions(m_options.GetOptionsString(true));
}

void CURL::RemoveOption(const std::string& key)
{
  if (m_uri)
  {
    CLog::Log(LOGWARNING, "{} - lossless URI options require explicit raw mutation", __FUNCTION__);
    return;
  }
  m_options.RemoveOption(key);
  SetOptions(m_options.GetOptionsString(true));
}

void CURL::GetProtocolOptions(std::map<std::string, std::string>& options) const
{
  CUrlOptions::UrlOptions optionsMap = m_protocolOptions.GetOptions();
  for (CUrlOptions::UrlOptions::const_iterator option = optionsMap.begin();
       option != optionsMap.end(); ++option)
    options[option->first] = option->second.asString();
}

bool CURL::HasProtocolOption(const std::string& key) const
{
  return m_protocolOptions.HasOption(key);
}

bool CURL::GetProtocolOption(const std::string& key, std::string& value) const
{
  CVariant valueObj;
  if (!m_protocolOptions.GetOption(key, valueObj))
    return false;

  value = valueObj.asString();
  return true;
}

std::string CURL::GetProtocolOption(const std::string& key) const
{
  std::string value;
  if (!GetProtocolOption(key, value))
    return "";

  return value;
}

void CURL::SetProtocolOption(const std::string& key, const std::string& value)
{
  if (m_uri)
  {
    CLog::Log(LOGWARNING, "{} - lossless URI options require explicit raw mutation", __FUNCTION__);
    return;
  }
  m_protocolOptions.AddOption(key, value);
  m_strProtocolOptions = m_protocolOptions.GetOptionsString(false);
}

void CURL::RemoveProtocolOption(const std::string& key)
{
  if (m_uri)
  {
    CLog::Log(LOGWARNING, "{} - lossless URI options require explicit raw mutation", __FUNCTION__);
    return;
  }
  m_protocolOptions.RemoveOption(key);
  m_strProtocolOptions = m_protocolOptions.GetOptionsString(false);
}

bool CURL::HasExtension(std::string_view extensions) const
{
  return URIUtils::HasExtension(*this, extensions);
}

std::string CURL::GetExtension() const
{
  return URIUtils::GetExtension(*this);
}

bool CURL::IsStack() const
{
  return IsProtocol("stack");
}

bool CURL::IsMultiPath() const
{
  return IsProtocol("multipath");
}

bool CURL::IsFavourite() const
{
  return IsProtocol("favourites");
}

bool CURL::IsPlugin() const
{
  return IsProtocol("plugin");
}

bool CURL::IsScript() const
{
  return IsProtocol("script");
}

bool CURL::IsAddonsPath() const
{
  return IsProtocol("addons");
}

bool CURL::IsSourcesPath() const
{
  return IsProtocol("sources");
}

bool CURL::IsCDDA() const
{
  return IsProtocol("cdda");
}

bool CURL::IsISO9660() const
{
  return IsProtocol("iso9660");
}

bool CURL::IsMusicDb() const
{
  return IsProtocol("musicdb");
}

bool CURL::IsVideoDb() const
{
  return IsProtocol("videodb");
}

bool CURL::IsBlurayPath() const
{
  return IsProtocol("bluray");
}

bool CURL::IsAndroidApp() const
{
  return IsProtocol("androidapp");
}

bool CURL::IsLibraryFolder() const
{
  return IsProtocol("library");
}

bool CURL::IsUPnP() const
{
  return IsProtocol("upnp");
}

bool CURL::IsAPK() const
{
  return HasExtension(".apk");
}

bool CURL::IsZIP() const // also checks for comic books!
{
  return HasExtension(".zip|.cbz");
}

bool CURL::IsArchive() const
{
  return HasExtension(".zip|.rar|.apk|.cbz|.cbr");
}

bool CURL::IsCBZ() const
{
  return HasExtension(".cbz");
}

bool CURL::IsCBR() const
{
  return HasExtension(".cbr");
}

bool CURL::IsDiscImage() const
{
  if (IsStack() || IsMultiPath())
    return false;

  return HasExtension(".img|.iso|.nrg|.udf");
}

bool CURL::IsPicture() const
{
  return HasExtension(CServiceBroker::GetFileExtensionProvider().GetPictureExtensions() +
                      "|.tbn|.dds");
}

bool CURL::HasParentInHostname() const
{
  return IsProtocol("zip") || IsProtocol("apk") || IsProtocol("bluray") || IsProtocol("udf") ||
         IsProtocol("iso9660") || IsProtocol("xbt") || IsProtocol("rar") || IsProtocol("archive") ||
         (CServiceBroker::IsAddonInterfaceUp() &&
          CServiceBroker::GetFileExtensionProvider().EncodedHostName(GetProtocol()));
}

bool CURL::HasEncodedHostname() const
{
  return HasParentInHostname() || IsProtocol("musicsearch") || IsProtocol("image") ||
         IsProtocol("episodes") || IsProtocol("cdda");
}

bool CURL::HasEncodedFilename() const
{
  const std::string prot2 = GetTranslatedProtocol();

  // For now assume only (quasi) http internet streams use URL encoding
  return IsProtocolEqual(prot2, "http") || IsProtocolEqual(prot2, "https");
}

bool CURL::IsLibraryContent() const
{
  return (IsLibraryFolder() || IsVideoDb() || IsMusicDb() || HasExtension(".xsp"));
}

bool CURL::IsBDFile() const
{
  if (IsStack() || IsMultiPath())
    return false;

  const std::string fileName{GetFileNameWithoutPath()};
  return StringUtils::EqualsNoCase(fileName, "index.bdmv") ||
         StringUtils::EqualsNoCase(fileName, "MovieObject.bdmv") ||
         StringUtils::EqualsNoCase(fileName, "INDEX.BDM") ||
         StringUtils::EqualsNoCase(fileName, "MOVIEOBJ.BDM");
}

bool CURL::IsDVDFile() const
{
  if (IsStack() || IsMultiPath())
    return false;

  const std::string fileName{GetFileNameWithoutPath()};
  return StringUtils::EqualsNoCase(fileName, "video_ts.ifo") ||
         (StringUtils::StartsWithNoCase(fileName, "vts_") &&
          StringUtils::EndsWithNoCase(fileName, "_0.ifo") && fileName.length() == 12);
}

bool CURL::IsOpticalMediaFile() const
{
  return IsBDFile() || IsDVDFile();
}
