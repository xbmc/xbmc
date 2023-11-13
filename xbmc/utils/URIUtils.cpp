/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "network/Network.h"
#include "URIUtils.h"
#include "FileItem.h"
#include "filesystem/MultiPathDirectory.h"
#include "filesystem/SpecialProtocol.h"
#include "filesystem/StackDirectory.h"
#include "network/DNSNameCache.h"
#include "pvr/channels/PVRChannelsPath.h"
#include "settings/AdvancedSettings.h"
#include "URL.h"
#include "utils/FileExtensionProvider.h"
#include "ServiceBroker.h"
#include "StringUtils.h"
#include "utils/log.h"

#if defined(TARGET_WINDOWS)
#include "platform/win32/CharsetConverter.h"
#endif

#include <algorithm>
#include <cassert>
#include <netinet/in.h>
#include <arpa/inet.h>

using namespace PVR;
using namespace XFILE;

const CAdvancedSettings* URIUtils::m_advancedSettings = nullptr;

void URIUtils::RegisterAdvancedSettings(const CAdvancedSettings& advancedSettings)
{
  m_advancedSettings = &advancedSettings;
}

void URIUtils::UnregisterAdvancedSettings()
{
  m_advancedSettings = nullptr;
}

/* returns filename extension including period of filename */
std::string URIUtils::GetExtension(const CURL& url)
{
  return URIUtils::GetExtension(url.GetFileName());
}

std::string URIUtils::GetExtension(const std::string& strFileName)
{
  if (IsURL(strFileName))
  {
    CURL url(strFileName);
    return GetExtension(url.GetFileName());
  }

  size_t period = strFileName.find_last_of("./\\");
  if (period == std::string::npos || strFileName[period] != '.')
    return std::string();

  return strFileName.substr(period);
}

bool URIUtils::HasPluginPath(const CFileItem& item)
{
  return IsPlugin(item.GetPath()) || IsPlugin(item.GetDynPath());
}

bool URIUtils::HasExtension(const std::string& strFileName)
{
  if (IsURL(strFileName))
  {
    CURL url(strFileName);
    return HasExtension(url.GetFileName());
  }

  size_t iPeriod = strFileName.find_last_of("./\\");
  return iPeriod != std::string::npos && strFileName[iPeriod] == '.';
}

bool URIUtils::HasExtension(const CURL& url, const std::string& strExtensions)
{
  return HasExtension(url.GetFileName(), strExtensions);
}

bool URIUtils::HasExtension(const std::string& strFileName, const std::string& strExtensions)
{
  if (IsURL(strFileName))
  {
    const CURL url(strFileName);
    return HasExtension(url.GetFileName(), strExtensions);
  }

  const size_t pos = strFileName.find_last_of("./\\");
  if (pos == std::string::npos || strFileName[pos] != '.')
    return false;

  const std::string extensionLower = StringUtils::ToLower(strFileName.substr(pos));

  const std::vector<std::string> extensionsLower =
      StringUtils::Split(StringUtils::ToLower(strExtensions), '|');

  for (const auto& ext : extensionsLower)
  {
    if (StringUtils::EndsWith(ext, extensionLower))
      return true;
  }

  return false;
}

void URIUtils::RemoveExtension(std::string& strFileName)
{
  if(IsURL(strFileName))
  {
    CURL url(strFileName);
    strFileName = url.GetFileName();
    RemoveExtension(strFileName);
    url.SetFileName(strFileName);
    strFileName = url.Get();
    return;
  }

  size_t period = strFileName.find_last_of("./\\");
  if (period != std::string::npos && strFileName[period] == '.')
  {
    std::string strExtension = strFileName.substr(period);
    StringUtils::ToLower(strExtension);
    strExtension += "|";

    std::string strFileMask;
    strFileMask = CServiceBroker::GetFileExtensionProvider().GetPictureExtensions();
    strFileMask += "|" + CServiceBroker::GetFileExtensionProvider().GetMusicExtensions();
    strFileMask += "|" + CServiceBroker::GetFileExtensionProvider().GetVideoExtensions();
    strFileMask += "|" + CServiceBroker::GetFileExtensionProvider().GetSubtitleExtensions();
#if defined(TARGET_DARWIN)
    strFileMask += "|.py|.xml|.milk|.xbt|.cdg|.app|.applescript|.workflow";
#else
    strFileMask += "|.py|.xml|.milk|.xbt|.cdg";
#endif
    strFileMask += "|";

    if (strFileMask.find(strExtension) != std::string::npos)
      strFileName.erase(period);
  }
}

std::string URIUtils::ReplaceExtension(const std::string& strFile,
                                      const std::string& strNewExtension)
{
  if(IsURL(strFile))
  {
    CURL url(strFile);
    url.SetFileName(ReplaceExtension(url.GetFileName(), strNewExtension));
    return url.Get();
  }

  std::string strChangedFile;
  std::string strExtension = GetExtension(strFile);
  if ( strExtension.size() )
  {
    strChangedFile = strFile.substr(0, strFile.size() - strExtension.size()) ;
    strChangedFile += strNewExtension;
  }
  else
  {
    strChangedFile = strFile;
    strChangedFile += strNewExtension;
  }
  return strChangedFile;
}

std::string URIUtils::GetFileName(const CURL& url)
{
  return GetFileName(url.GetFileName());
}

/* returns a filename given an url */
/* handles both / and \, and options in urls*/
std::string URIUtils::GetFileName(const std::string& strFileNameAndPath)
{
  if(IsURL(strFileNameAndPath))
  {
    CURL url(strFileNameAndPath);
    return GetFileName(url.GetFileName());
  }

  /* find the last slash */
  const size_t slash = strFileNameAndPath.find_last_of("/\\");
  return strFileNameAndPath.substr(slash+1);
}

void URIUtils::Split(const std::string& strFileNameAndPath,
                     std::string& strPath, std::string& strFileName)
{
  //Splits a full filename in path and file.
  //ex. smb://computer/share/directory/filename.ext -> strPath:smb://computer/share/directory/ and strFileName:filename.ext
  //Trailing slash will be preserved
  strFileName = "";
  strPath = "";
  int i = strFileNameAndPath.size() - 1;
  while (i > 0)
  {
    char ch = strFileNameAndPath[i];
    // Only break on ':' if it's a drive separator for DOS (ie d:foo)
    if (ch == '/' || ch == '\\' || (ch == ':' && i == 1)) break;
    else i--;
  }
  if (i == 0)
    i--;

  // take left including the directory separator
  strPath = strFileNameAndPath.substr(0, i+1);
  // everything to the right of the directory separator
  strFileName = strFileNameAndPath.substr(i+1);

  // if actual uri, ignore options
  if (IsURL(strFileNameAndPath))
  {
    i = strFileName.size() - 1;
    while (i > 0)
    {
      char ch = strFileName[i];
      if (ch == '?' || ch == '|') break;
      else i--;
    }
    if (i > 0)
      strFileName.resize(i);
  }
}

std::vector<std::string> URIUtils::SplitPath(const std::string& strPath)
{
  CURL url(strPath);

  // silly std::string can't take a char in the constructor
  std::string sep(1, url.GetDirectorySeparator());

  // split the filename portion of the URL up into separate dirs
  std::vector<std::string> dirs = StringUtils::Split(url.GetFileName(), sep);

  // we start with the root path
  std::string dir = url.GetWithoutFilename();

  if (!dir.empty())
    dirs.insert(dirs.begin(), dir);

  // we don't need empty token on the end
  if (dirs.size() > 1 && dirs.back().empty())
    dirs.erase(dirs.end() - 1);

  return dirs;
}

void URIUtils::GetCommonPath(std::string& strParent, const std::string& strPath)
{
  // find the common path of parent and path
  unsigned int j = 1;
  while (j <= std::min(strParent.size(), strPath.size()) &&
         StringUtils::CompareNoCase(strParent, strPath, j) == 0)
    j++;
  strParent.erase(j - 1);
  // they should at least share a / at the end, though for things such as path/cd1 and path/cd2 there won't be
  if (!HasSlashAtEnd(strParent))
  {
    strParent = GetDirectory(strParent);
    AddSlashAtEnd(strParent);
  }
}

bool URIUtils::HasParentInHostname(const CURL& url)
{
  return url.IsProtocol("zip") || url.IsProtocol("apk") || url.IsProtocol("bluray") ||
         url.IsProtocol("udf") || url.IsProtocol("iso9660") || url.IsProtocol("xbt") ||
         (CServiceBroker::IsAddonInterfaceUp() &&
          CServiceBroker::GetFileExtensionProvider().EncodedHostName(url.GetProtocol()));
}

bool URIUtils::HasEncodedHostname(const CURL& url)
{
  return HasParentInHostname(url)
      || url.IsProtocol("musicsearch")
      || url.IsProtocol( "image");
}

bool URIUtils::HasEncodedFilename(const CURL& url)
{
  const std::string prot2 = url.GetTranslatedProtocol();

  // For now assume only (quasi) http internet streams use URL encoding
  return CURL::IsProtocolEqual(prot2, "http")  ||
         CURL::IsProtocolEqual(prot2, "https");
}

std::string URIUtils::GetParentPath(const std::string& strPath)
{
  std::string strReturn;
  GetParentPath(strPath, strReturn);
  return strReturn;
}

bool URIUtils::GetParentPath(const std::string& strPath, std::string& strParent)
{
  strParent.clear();

  CURL url(strPath);
  std::string strFile = url.GetFileName();
  if ( URIUtils::HasParentInHostname(url) && strFile.empty())
  {
    strFile = url.GetHostName();
    return GetParentPath(strFile, strParent);
  }
  else if (url.IsProtocol("stack"))
  {
    CStackDirectory dir;
    CFileItemList items;
    if (!dir.GetDirectory(url, items))
      return false;
    CURL url2(GetDirectory(items[0]->GetPath()));
    if (HasParentInHostname(url2))
      GetParentPath(url2.Get(), strParent);
    else
      strParent = url2.Get();
    for( int i=1;i<items.Size();++i)
    {
      items[i]->m_strDVDLabel = GetDirectory(items[i]->GetPath());
      if (HasParentInHostname(url2))
        items[i]->SetPath(GetParentPath(items[i]->m_strDVDLabel));
      else
        items[i]->SetPath(items[i]->m_strDVDLabel);

      GetCommonPath(strParent,items[i]->GetPath());
    }
    return true;
  }
  else if (url.IsProtocol("multipath"))
  {
    // get the parent path of the first item
    return GetParentPath(CMultiPathDirectory::GetFirstPath(strPath), strParent);
  }
  else if (url.IsProtocol("plugin"))
  {
    if (!url.GetOptions().empty())
    {
      //! @todo Make a new python call to get the plugin content type and remove this temporary hack
      // When a plugin provides multiple types, it has "plugin://addon.id/?content_type=xxx" root URL
      if (url.GetFileName().empty() && url.HasOption("content_type") && url.GetOptions().find('&') == std::string::npos)
        url.SetHostName("");
      //
      url.SetOptions("");
      strParent = url.Get();
      return true;
    }
    if (!url.GetFileName().empty())
    {
      url.SetFileName("");
      strParent = url.Get();
      return true;
    }
    if (!url.GetHostName().empty())
    {
      url.SetHostName("");
      strParent = url.Get();
      return true;
    }
    return true;  // already at root
  }
  else if (url.IsProtocol("special"))
  {
    if (HasSlashAtEnd(strFile))
      strFile.erase(strFile.size() - 1);
    if(strFile.rfind('/') == std::string::npos)
      return false;
  }
  else if (strFile.empty())
  {
    if (!url.GetHostName().empty())
    {
      // we have an share with only server or workgroup name
      // set hostname to "" and return true to get back to root
      url.SetHostName("");
      strParent = url.Get();
      return true;
    }
    return false;
  }

  if (HasSlashAtEnd(strFile) )
  {
    strFile.erase(strFile.size() - 1);
  }

  size_t iPos = strFile.rfind('/');
#ifndef TARGET_POSIX
  if (iPos == std::string::npos)
  {
    iPos = strFile.rfind('\\');
  }
#endif
  if (iPos == std::string::npos)
  {
    url.SetFileName("");
    strParent = url.Get();
    return true;
  }

  strFile.erase(iPos);

  AddSlashAtEnd(strFile);

  url.SetFileName(strFile);
  strParent = url.Get();
  return true;
}

std::string URIUtils::GetBasePath(const std::string& strPath)
{
  std::string strCheck(strPath);
  if (IsStack(strPath))
    strCheck = CStackDirectory::GetFirstStackedFile(strPath);

  std::string strDirectory = GetDirectory(strCheck);
  if (IsInRAR(strCheck))
  {
    std::string strPath=strDirectory;
    GetParentPath(strPath, strDirectory);
  }
  if (IsStack(strPath))
  {
    strCheck = strDirectory;
    RemoveSlashAtEnd(strCheck);
    if (GetFileName(strCheck).size() == 3 && StringUtils::StartsWithNoCase(GetFileName(strCheck), "cd"))
      strDirectory = GetDirectory(strCheck);
  }
  return strDirectory;
}

std::string URLEncodePath(const std::string& strPath)
{
  std::vector<std::string> segments = StringUtils::Split(strPath, "/");
  for (std::vector<std::string>::iterator i = segments.begin(); i != segments.end(); ++i)
    *i = CURL::Encode(*i);

  return StringUtils::Join(segments, "/");
}

std::string URLDecodePath(const std::string& strPath)
{
  std::vector<std::string> segments = StringUtils::Split(strPath, "/");
  for (std::vector<std::string>::iterator i = segments.begin(); i != segments.end(); ++i)
    *i = CURL::Decode(*i);

  return StringUtils::Join(segments, "/");
}

std::string URIUtils::ChangeBasePath(const std::string &fromPath, const std::string &fromFile, const std::string &toPath, const bool &bAddPath /* = true */)
{
  std::string toFile = fromFile;

  // Convert back slashes to forward slashes, if required
  if (IsDOSPath(fromPath) && !IsDOSPath(toPath))
    StringUtils::Replace(toFile, "\\", "/");

  // Handle difference in URL encoded vs. not encoded
  if ( HasEncodedFilename(CURL(fromPath))
   && !HasEncodedFilename(CURL(toPath)) )
  {
    toFile = URLDecodePath(toFile); // Decode path
  }
  else if (!HasEncodedFilename(CURL(fromPath))
         && HasEncodedFilename(CURL(toPath)) )
  {
    toFile = URLEncodePath(toFile); // Encode path
  }

  // Convert forward slashes to back slashes, if required
  if (!IsDOSPath(fromPath) && IsDOSPath(toPath))
    StringUtils::Replace(toFile, "/", "\\");

  if (bAddPath)
    return AddFileToFolder(toPath, toFile);

  return toFile;
}

CURL URIUtils::SubstitutePath(const CURL& url, bool reverse /* = false */)
{
  const std::string pathToUrl = url.Get();
  return CURL(SubstitutePath(pathToUrl, reverse));
}

std::string URIUtils::SubstitutePath(const std::string& strPath, bool reverse /* = false */)
{
  if (!m_advancedSettings)
  {
    // path substitution not needed / not working during Kodi bootstrap.
    return strPath;
  }

  for (const auto& pathPair : m_advancedSettings->m_pathSubstitutions)
  {
    const std::string fromPath = reverse ? pathPair.second : pathPair.first;
    std::string toPath = reverse ? pathPair.first : pathPair.second;

    if (strncmp(strPath.c_str(), fromPath.c_str(), HasSlashAtEnd(fromPath) ? fromPath.size() - 1 : fromPath.size()) == 0)
    {
      if (strPath.size() > fromPath.size())
      {
        std::string strSubPathAndFileName = strPath.substr(fromPath.size());
        return ChangeBasePath(fromPath, strSubPathAndFileName, toPath); // Fix encoding + slash direction
      }
      else
      {
        return toPath;
      }
    }
  }
  return strPath;
}

bool URIUtils::IsProtocol(const std::string& url, const std::string &type)
{
  return StringUtils::StartsWithNoCase(url, type + "://");
}

bool URIUtils::PathHasParent(std::string path, std::string parent, bool translate /* = false */)
{
  if (translate)
  {
    path = CSpecialProtocol::TranslatePath(path);
    parent = CSpecialProtocol::TranslatePath(parent);
  }

  if (parent.empty())
    return false;

  if (path == parent)
    return true;

  // Make sure parent has a trailing slash
  AddSlashAtEnd(parent);

  return StringUtils::StartsWith(path, parent);
}

bool URIUtils::PathEquals(std::string path1, std::string path2, bool ignoreTrailingSlash /* = false */, bool ignoreURLOptions /* = false */)
{
  if (ignoreURLOptions)
  {
    path1 = CURL(path1).GetWithoutOptions();
    path2 = CURL(path2).GetWithoutOptions();
  }

  if (ignoreTrailingSlash)
  {
    RemoveSlashAtEnd(path1);
    RemoveSlashAtEnd(path2);
  }

  return (path1 == path2);
}

bool URIUtils::IsRemote(const std::string& strFile)
{
  if (IsCDDA(strFile) || IsISO9660(strFile))
    return false;

  if (IsStack(strFile))
    return IsRemote(CStackDirectory::GetFirstStackedFile(strFile));

  if (IsSpecial(strFile))
    return IsRemote(CSpecialProtocol::TranslatePath(strFile));

  if(IsMultiPath(strFile))
  { // virtual paths need to be checked separately
    std::vector<std::string> paths;
    if (CMultiPathDirectory::GetPaths(strFile, paths))
    {
      for (unsigned int i = 0; i < paths.size(); i++)
        if (IsRemote(paths[i])) return true;
    }
    return false;
  }

  CURL url(strFile);
  if(HasParentInHostname(url))
    return IsRemote(url.GetHostName());

  if (IsAddonsPath(strFile))
    return false;

  if (IsSourcesPath(strFile))
    return false;

  if (IsVideoDb(strFile) || IsMusicDb(strFile))
    return false;

  if (IsLibraryFolder(strFile))
    return false;

  if (IsPlugin(strFile))
    return false;

  if (IsAndroidApp(strFile))
    return false;

  if (!url.IsLocal())
    return true;

  return false;
}

bool URIUtils::IsOnDVD(const std::string& strFile)
{
  if (IsProtocol(strFile, "dvd"))
    return true;

  if (IsProtocol(strFile, "udf"))
    return true;

  if (IsProtocol(strFile, "iso9660"))
    return true;

  if (IsProtocol(strFile, "cdda"))
    return true;

#if defined(TARGET_WINDOWS_STORE)
  CLog::Log(LOGDEBUG, "{} is not implemented", __FUNCTION__);
#elif defined(TARGET_WINDOWS_DESKTOP)
  using KODI::PLATFORM::WINDOWS::ToW;
  if (strFile.size() >= 2 && strFile.substr(1, 1) == ":")
    return (GetDriveType(ToW(strFile.substr(0, 3)).c_str()) == DRIVE_CDROM);
#endif
  return false;
}

bool URIUtils::IsOnLAN(const std::string& strPath, LanCheckMode lanCheckMode)
{
  if(IsMultiPath(strPath))
    return IsOnLAN(CMultiPathDirectory::GetFirstPath(strPath), lanCheckMode);

  if(IsStack(strPath))
    return IsOnLAN(CStackDirectory::GetFirstStackedFile(strPath), lanCheckMode);

  if(IsSpecial(strPath))
    return IsOnLAN(CSpecialProtocol::TranslatePath(strPath), lanCheckMode);

  if(IsPlugin(strPath))
    return false;

  if(IsUPnP(strPath))
    return true;

  CURL url(strPath);
  if (HasParentInHostname(url))
    return IsOnLAN(url.GetHostName(), lanCheckMode);

  if(!IsRemote(strPath))
    return false;

  const std::string& host = url.GetHostName();

  return IsHostOnLAN(host, lanCheckMode);
}

static bool addr_match(uint32_t addr, const char* target, const char* submask)
{
  uint32_t addr2 = ntohl(inet_addr(target));
  uint32_t mask = ntohl(inet_addr(submask));
  return (addr & mask) == (addr2 & mask);
}

bool URIUtils::IsHostOnLAN(const std::string& host, LanCheckMode lanCheckMode)
{
  if(host.length() == 0)
    return false;

  // assume a hostname without dot's
  // is local (smb netbios hostnames)
  if(host.find('.') == std::string::npos)
    return true;

  uint32_t address = ntohl(inet_addr(host.c_str()));
  if(address == INADDR_NONE)
  {
    std::string ip;
    if(CDNSNameCache::Lookup(host, ip))
      address = ntohl(inet_addr(ip.c_str()));
  }

  if(address != INADDR_NONE)
  {
    if (lanCheckMode ==
        LanCheckMode::
            ANY_PRIVATE_SUBNET) // check if in private range, ref https://en.wikipedia.org/wiki/Private_network
    {
      if (
        addr_match(address, "192.168.0.0", "255.255.0.0") ||
        addr_match(address, "10.0.0.0", "255.0.0.0") ||
        addr_match(address, "172.16.0.0", "255.240.0.0")
        )
        return true;
    }
    // check if we are on the local subnet
    if (!CServiceBroker::GetNetwork().GetFirstConnectedInterface())
      return false;

    if (CServiceBroker::GetNetwork().HasInterfaceForIP(address))
      return true;
  }

  return false;
}

bool URIUtils::IsMultiPath(const std::string& strPath)
{
  return IsProtocol(strPath, "multipath");
}

bool URIUtils::IsHD(const std::string& strFileName)
{
  CURL url(strFileName);

  if (IsStack(strFileName))
    return IsHD(CStackDirectory::GetFirstStackedFile(strFileName));

  if (IsSpecial(strFileName))
    return IsHD(CSpecialProtocol::TranslatePath(strFileName));

  if (HasParentInHostname(url))
    return IsHD(url.GetHostName());

  return url.GetProtocol().empty() || url.IsProtocol("file") || url.IsProtocol("win-lib");
}

bool URIUtils::IsDVD(const std::string& strFile)
{
  std::string strFileLow = strFile;
  StringUtils::ToLower(strFileLow);
  if (strFileLow.find("video_ts.ifo") != std::string::npos && IsOnDVD(strFile))
    return true;

#if defined(TARGET_WINDOWS)
  if (IsProtocol(strFile, "dvd"))
    return true;

  if(strFile.size() < 2 || (strFile.substr(1) != ":\\" && strFile.substr(1) != ":"))
    return false;

#ifndef TARGET_WINDOWS_STORE
  if(GetDriveType(KODI::PLATFORM::WINDOWS::ToW(strFile).c_str()) == DRIVE_CDROM)
    return true;
#endif
#else
  if (strFileLow == "iso9660://" || strFileLow == "udf://" || strFileLow == "dvd://1" )
    return true;
#endif

  return false;
}

bool URIUtils::IsStack(const std::string& strFile)
{
  return IsProtocol(strFile, "stack");
}

bool URIUtils::IsFavourite(const std::string& strFile)
{
  return IsProtocol(strFile, "favourites");
}

bool URIUtils::IsRAR(const std::string& strFile)
{
  std::string strExtension = GetExtension(strFile);

  if (strExtension == ".001" && !StringUtils::EndsWithNoCase(strFile, ".ts.001"))
    return true;

  if (StringUtils::EqualsNoCase(strExtension, ".cbr"))
    return true;

  if (StringUtils::EqualsNoCase(strExtension, ".rar"))
    return true;

  return false;
}

bool URIUtils::IsInArchive(const std::string &strFile)
{
  CURL url(strFile);

  bool archiveProto = url.IsProtocol("archive") && !url.GetFileName().empty();
  return archiveProto || IsInZIP(strFile) || IsInRAR(strFile) || IsInAPK(strFile);
}

bool URIUtils::IsInAPK(const std::string& strFile)
{
  CURL url(strFile);

  return url.IsProtocol("apk") && !url.GetFileName().empty();
}

bool URIUtils::IsInZIP(const std::string& strFile)
{
  CURL url(strFile);

  if (url.GetFileName().empty())
    return false;

  if (url.IsProtocol("archive"))
    return IsZIP(url.GetHostName());

  return url.IsProtocol("zip");
}

bool URIUtils::IsInRAR(const std::string& strFile)
{
  CURL url(strFile);

  if (url.GetFileName().empty())
    return false;

  if (url.IsProtocol("archive"))
    return IsRAR(url.GetHostName());

  return url.IsProtocol("rar");
}

bool URIUtils::IsAPK(const std::string& strFile)
{
  return HasExtension(strFile, ".apk");
}

bool URIUtils::IsZIP(const std::string& strFile) // also checks for comic books!
{
  return HasExtension(strFile, ".zip|.cbz");
}

bool URIUtils::IsArchive(const std::string& strFile)
{
  return HasExtension(strFile, ".zip|.rar|.apk|.cbz|.cbr");
}

bool URIUtils::IsDiscImage(const std::string& file)
{
  return HasExtension(file, ".img|.iso|.nrg|.udf");
}

bool URIUtils::IsDiscImageStack(const std::string& file)
{
  return IsStack(file) && IsDiscImage(CStackDirectory::GetFirstStackedFile(file));
}

bool URIUtils::IsSpecial(const std::string& strFile)
{
  if (IsStack(strFile))
    return IsSpecial(CStackDirectory::GetFirstStackedFile(strFile));

  return IsProtocol(strFile, "special");
}

bool URIUtils::IsPlugin(const std::string& strFile)
{
  CURL url(strFile);
  return url.IsProtocol("plugin");
}

bool URIUtils::IsScript(const std::string& strFile)
{
  CURL url(strFile);
  return url.IsProtocol("script");
}

bool URIUtils::IsAddonsPath(const std::string& strFile)
{
  CURL url(strFile);
  return url.IsProtocol("addons");
}

bool URIUtils::IsSourcesPath(const std::string& strPath)
{
  CURL url(strPath);
  return url.IsProtocol("sources");
}

bool URIUtils::IsCDDA(const std::string& strFile)
{
  return IsProtocol(strFile, "cdda");
}

bool URIUtils::IsISO9660(const std::string& strFile)
{
  return IsProtocol(strFile, "iso9660");
}

bool URIUtils::IsSmb(const std::string& strFile)
{
  if (IsStack(strFile))
    return IsSmb(CStackDirectory::GetFirstStackedFile(strFile));

  if (IsSpecial(strFile))
    return IsSmb(CSpecialProtocol::TranslatePath(strFile));

  CURL url(strFile);
  if (HasParentInHostname(url))
    return IsSmb(url.GetHostName());

  return IsProtocol(strFile, "smb");
}

bool URIUtils::IsURL(const std::string& strFile)
{
  return strFile.find("://") != std::string::npos;
}

bool URIUtils::IsFTP(const std::string& strFile)
{
  if (IsStack(strFile))
    return IsFTP(CStackDirectory::GetFirstStackedFile(strFile));

  if (IsSpecial(strFile))
    return IsFTP(CSpecialProtocol::TranslatePath(strFile));

  CURL url(strFile);
  if (HasParentInHostname(url))
    return IsFTP(url.GetHostName());

  return IsProtocol(strFile, "ftp") ||
         IsProtocol(strFile, "ftps");
}

bool URIUtils::IsHTTP(const std::string& strFile, bool bTranslate /* = false */)
{
  if (IsStack(strFile))
    return IsHTTP(CStackDirectory::GetFirstStackedFile(strFile));

  if (IsSpecial(strFile))
    return IsHTTP(CSpecialProtocol::TranslatePath(strFile));

  CURL url(strFile);
  if (HasParentInHostname(url))
    return IsHTTP(url.GetHostName());

  const std::string strProtocol = (bTranslate ? url.GetTranslatedProtocol() : url.GetProtocol());

  return (strProtocol == "http" || strProtocol == "https");
}

bool URIUtils::IsUDP(const std::string& strFile)
{
  if (IsStack(strFile))
    return IsUDP(CStackDirectory::GetFirstStackedFile(strFile));

  return IsProtocol(strFile, "udp");
}

bool URIUtils::IsTCP(const std::string& strFile)
{
  if (IsStack(strFile))
    return IsTCP(CStackDirectory::GetFirstStackedFile(strFile));

  return IsProtocol(strFile, "tcp");
}

bool URIUtils::IsPVR(const std::string& strFile)
{
  if (IsStack(strFile))
    return IsPVR(CStackDirectory::GetFirstStackedFile(strFile));

  return IsProtocol(strFile, "pvr");
}

bool URIUtils::IsPVRChannel(const std::string& strFile)
{
  if (IsStack(strFile))
    return IsPVRChannel(CStackDirectory::GetFirstStackedFile(strFile));

  return IsProtocol(strFile, "pvr") && CPVRChannelsPath(strFile).IsChannel();
}

bool URIUtils::IsPVRChannelGroup(const std::string& strFile)
{
  if (IsStack(strFile))
    return IsPVRChannelGroup(CStackDirectory::GetFirstStackedFile(strFile));

  return IsProtocol(strFile, "pvr") && CPVRChannelsPath(strFile).IsChannelGroup();
}

bool URIUtils::IsPVRGuideItem(const std::string& strFile)
{
  if (IsStack(strFile))
    return IsPVRGuideItem(CStackDirectory::GetFirstStackedFile(strFile));

  return StringUtils::StartsWithNoCase(strFile, "pvr://guide");
}

bool URIUtils::IsDAV(const std::string& strFile)
{
  if (IsStack(strFile))
    return IsDAV(CStackDirectory::GetFirstStackedFile(strFile));

  if (IsSpecial(strFile))
    return IsDAV(CSpecialProtocol::TranslatePath(strFile));

  CURL url(strFile);
  if (HasParentInHostname(url))
    return IsDAV(url.GetHostName());

  return IsProtocol(strFile, "dav") ||
         IsProtocol(strFile, "davs");
}

bool URIUtils::IsInternetStream(const std::string &path, bool bStrictCheck /* = false */)
{
  const CURL pathToUrl(path);
  return IsInternetStream(pathToUrl, bStrictCheck);
}

bool URIUtils::IsInternetStream(const CURL& url, bool bStrictCheck /* = false */)
{
  if (url.GetProtocol().empty())
    return false;

  // there's nothing to stop internet streams from being stacked
  if (url.IsProtocol("stack"))
    return IsInternetStream(CStackDirectory::GetFirstStackedFile(url.Get()), bStrictCheck);

  // Only consider "streamed" filesystems internet streams when being strict
  if (bStrictCheck && IsStreamedFilesystem(url.Get()))
    return true;

  // Check for true internetstreams
  const std::string& protocol = url.GetProtocol();
  if (CURL::IsProtocolEqual(protocol, "http") || CURL::IsProtocolEqual(protocol, "https") ||
      CURL::IsProtocolEqual(protocol, "tcp") || CURL::IsProtocolEqual(protocol, "udp") ||
      CURL::IsProtocolEqual(protocol, "rtp") || CURL::IsProtocolEqual(protocol, "sdp") ||
      CURL::IsProtocolEqual(protocol, "mms") || CURL::IsProtocolEqual(protocol, "mmst") ||
      CURL::IsProtocolEqual(protocol, "mmsh") || CURL::IsProtocolEqual(protocol, "rtsp") ||
      CURL::IsProtocolEqual(protocol, "rtmp") || CURL::IsProtocolEqual(protocol, "rtmpt") ||
      CURL::IsProtocolEqual(protocol, "rtmpe") || CURL::IsProtocolEqual(protocol, "rtmpte") ||
      CURL::IsProtocolEqual(protocol, "rtmps") || CURL::IsProtocolEqual(protocol, "shout") ||
      CURL::IsProtocolEqual(protocol, "rss") || CURL::IsProtocolEqual(protocol, "rsss"))
    return true;

  return false;
}

bool URIUtils::IsStreamedFilesystem(const std::string& strPath)
{
  CURL url(strPath);

  if (url.GetProtocol().empty())
    return false;

  if (url.IsProtocol("stack"))
    return IsStreamedFilesystem(CStackDirectory::GetFirstStackedFile(strPath));

  if (IsUPnP(strPath) || IsFTP(strPath) || IsHTTP(strPath, true))
    return true;

  //! @todo sftp/ssh special case has to be handled by vfs addon
  if (url.IsProtocol("sftp") || url.IsProtocol("ssh"))
    return true;

  return false;
}

bool URIUtils::IsNetworkFilesystem(const std::string& strPath)
{
  CURL url(strPath);

  if (url.GetProtocol().empty())
    return false;

  if (url.IsProtocol("stack"))
    return IsNetworkFilesystem(CStackDirectory::GetFirstStackedFile(strPath));

  if (IsStreamedFilesystem(strPath))
    return true;

  if (IsSmb(strPath) || IsNfs(strPath))
    return true;

  return false;
}

bool URIUtils::IsUPnP(const std::string& strFile)
{
  return IsProtocol(strFile, "upnp");
}

bool URIUtils::IsLiveTV(const std::string& strFile)
{
  std::string strFileWithoutSlash(strFile);
  RemoveSlashAtEnd(strFileWithoutSlash);

  if (StringUtils::EndsWithNoCase(strFileWithoutSlash, ".pvr") &&
      !StringUtils::StartsWith(strFileWithoutSlash, "pvr://recordings"))
    return true;

  return false;
}

bool URIUtils::IsPVRRecording(const std::string& strFile)
{
  std::string strFileWithoutSlash(strFile);
  RemoveSlashAtEnd(strFileWithoutSlash);

  return StringUtils::EndsWithNoCase(strFileWithoutSlash, ".pvr") &&
         StringUtils::StartsWith(strFile, "pvr://recordings");
}

bool URIUtils::IsPVRRecordingFileOrFolder(const std::string& strFile)
{
  return StringUtils::StartsWith(strFile, "pvr://recordings");
}

bool URIUtils::IsPVRTVRecordingFileOrFolder(const std::string& strFile)
{
  return StringUtils::StartsWith(strFile, "pvr://recordings/tv");
}

bool URIUtils::IsPVRRadioRecordingFileOrFolder(const std::string& strFile)
{
  return StringUtils::StartsWith(strFile, "pvr://recordings/radio");
}

bool URIUtils::IsMusicDb(const std::string& strFile)
{
  return IsProtocol(strFile, "musicdb");
}

bool URIUtils::IsNfs(const std::string& strFile)
{
  if (IsStack(strFile))
    return IsNfs(CStackDirectory::GetFirstStackedFile(strFile));

  if (IsSpecial(strFile))
    return IsNfs(CSpecialProtocol::TranslatePath(strFile));

  CURL url(strFile);
  if (HasParentInHostname(url))
    return IsNfs(url.GetHostName());

  return IsProtocol(strFile, "nfs");
}

bool URIUtils::IsVideoDb(const std::string& strFile)
{
  return IsProtocol(strFile, "videodb");
}

bool URIUtils::IsBluray(const std::string& strFile)
{
  return IsProtocol(strFile, "bluray");
}

bool URIUtils::IsAndroidApp(const std::string &path)
{
  return IsProtocol(path, "androidapp");
}

bool URIUtils::IsLibraryFolder(const std::string& strFile)
{
  CURL url(strFile);
  return url.IsProtocol("library");
}

bool URIUtils::IsLibraryContent(const std::string &strFile)
{
  return (IsProtocol(strFile, "library") ||
          IsProtocol(strFile, "videodb") ||
          IsProtocol(strFile, "musicdb") ||
          StringUtils::EndsWith(strFile, ".xsp"));
}

bool URIUtils::IsDOSPath(const std::string &path)
{
  if (path.size() > 1 && path[1] == ':' && isalpha(path[0]))
    return true;

  // windows network drives
  if (path.size() > 1 && path[0] == '\\' && path[1] == '\\')
    return true;

  return false;
}

std::string URIUtils::AppendSlash(std::string strFolder)
{
  AddSlashAtEnd(strFolder);
  return strFolder;
}

void URIUtils::AddSlashAtEnd(std::string& strFolder)
{
  if (IsURL(strFolder))
  {
    CURL url(strFolder);
    std::string file = url.GetFileName();
    if(!file.empty() && file != strFolder)
    {
      AddSlashAtEnd(file);
      url.SetFileName(file);
      strFolder = url.Get();
    }
    return;
  }

  if (!HasSlashAtEnd(strFolder))
  {
    if (IsDOSPath(strFolder))
      strFolder += '\\';
    else
      strFolder += '/';
  }
}

bool URIUtils::HasSlashAtEnd(const std::string& strFile, bool checkURL /* = false */)
{
  if (strFile.empty()) return false;
  if (checkURL && IsURL(strFile))
  {
    CURL url(strFile);
    const std::string& file = url.GetFileName();
    return file.empty() || HasSlashAtEnd(file, false);
  }
  char kar = strFile.c_str()[strFile.size() - 1];

  if (kar == '/' || kar == '\\')
    return true;

  return false;
}

void URIUtils::RemoveSlashAtEnd(std::string& strFolder)
{
  // performance optimization. pvr guide items are mass objects, uri never has a slash at end, and this method is quite expensive...
  if (IsPVRGuideItem(strFolder))
    return;

  if (IsURL(strFolder))
  {
    CURL url(strFolder);
    std::string file = url.GetFileName();
    if (!file.empty() && file != strFolder)
    {
      RemoveSlashAtEnd(file);
      url.SetFileName(file);
      strFolder = url.Get();
      return;
    }
    if(url.GetHostName().empty())
      return;
  }

  while (HasSlashAtEnd(strFolder))
    strFolder.erase(strFolder.size()-1, 1);
}

bool URIUtils::CompareWithoutSlashAtEnd(const std::string& strPath1, const std::string& strPath2)
{
  std::string strc1 = strPath1, strc2 = strPath2;
  RemoveSlashAtEnd(strc1);
  RemoveSlashAtEnd(strc2);
  return StringUtils::EqualsNoCase(strc1, strc2);
}


std::string URIUtils::FixSlashesAndDups(const std::string& path, const char slashCharacter /* = '/' */, const size_t startFrom /*= 0*/)
{
  const size_t len = path.length();
  if (startFrom >= len)
    return path;

  std::string result(path, 0, startFrom);
  result.reserve(len);

  const char* const str = path.c_str();
  size_t pos = startFrom;
  do
  {
    if (str[pos] == '\\' || str[pos] == '/')
    {
      result.push_back(slashCharacter);  // append one slash
      pos++;
      // skip any following slashes
      while (str[pos] == '\\' || str[pos] == '/') // str is null-terminated, no need to check for buffer overrun
        pos++;
    }
    else
      result.push_back(str[pos++]);   // append current char and advance pos to next char

  } while (pos < len);

  return result;
}


std::string URIUtils::CanonicalizePath(const std::string& path, const char slashCharacter /*= '\\'*/)
{
  assert(slashCharacter == '\\' || slashCharacter == '/');

  if (path.empty())
    return path;

  const std::string slashStr(1, slashCharacter);
  std::vector<std::string> pathVec, resultVec;
  StringUtils::Tokenize(path, pathVec, slashStr);

  for (std::vector<std::string>::const_iterator it = pathVec.begin(); it != pathVec.end(); ++it)
  {
    if (*it == ".")
    { /* skip - do nothing */ }
    else if (*it == ".." && !resultVec.empty() && resultVec.back() != "..")
      resultVec.pop_back();
    else
      resultVec.push_back(*it);
  }

  std::string result;
  if (path[0] == slashCharacter)
    result.push_back(slashCharacter); // add slash at the begin

  result += StringUtils::Join(resultVec, slashStr);

  if (path[path.length() - 1] == slashCharacter  && !result.empty() && result[result.length() - 1] != slashCharacter)
    result.push_back(slashCharacter); // add slash at the end if result isn't empty and result isn't "/"

  return result;
}

std::string URIUtils::AddFileToFolder(const std::string& strFolder,
                                const std::string& strFile)
{
  if (IsURL(strFolder))
  {
    CURL url(strFolder);
    if (url.GetFileName() != strFolder)
    {
      url.SetFileName(AddFileToFolder(url.GetFileName(), strFile));
      return url.Get();
    }
  }

  std::string strResult = strFolder;
  if (!strResult.empty())
    AddSlashAtEnd(strResult);

  // Remove any slash at the start of the file
  if (strFile.size() && (strFile[0] == '/' || strFile[0] == '\\'))
    strResult += strFile.substr(1);
  else
    strResult += strFile;

  // correct any slash directions
  if (!IsDOSPath(strFolder))
    StringUtils::Replace(strResult, '\\', '/');
  else
    StringUtils::Replace(strResult, '/', '\\');

  return strResult;
}

std::string URIUtils::GetDirectory(const std::string &strFilePath)
{
  // Will from a full filename return the directory the file resides in.
  // Keeps the final slash at end and possible |option=foo options.

  size_t iPosSlash = strFilePath.find_last_of("/\\");
  if (iPosSlash == std::string::npos)
    return ""; // No slash, so no path (ignore any options)

  size_t iPosBar = strFilePath.rfind('|');
  if (iPosBar == std::string::npos)
    return strFilePath.substr(0, iPosSlash + 1); // Only path

  return strFilePath.substr(0, iPosSlash + 1) + strFilePath.substr(iPosBar); // Path + options
}

CURL URIUtils::CreateArchivePath(const std::string& type,
                                 const CURL& archiveUrl,
                                 const std::string& pathInArchive,
                                 const std::string& password)
{
  CURL url;
  url.SetProtocol(type);
  if (!password.empty())
    url.SetUserName(password);
  url.SetHostName(archiveUrl.Get());

  /* NOTE: on posix systems, the replacement of \ with / is incorrect.
     Ideally this would not be done. We need to check that the ZipManager
     code (and elsewhere) doesn't pass in non-posix paths.
   */
  std::string strBuffer(pathInArchive);
  StringUtils::Replace(strBuffer, '\\', '/');
  StringUtils::TrimLeft(strBuffer, "/");
  url.SetFileName(strBuffer);

  return url;
}

std::string URIUtils::GetRealPath(const std::string &path)
{
  if (path.empty())
    return path;

  CURL url(path);
  url.SetHostName(GetRealPath(url.GetHostName()));
  url.SetFileName(resolvePath(url.GetFileName()));

  return url.Get();
}

std::string URIUtils::resolvePath(const std::string &path)
{
  if (path.empty())
    return path;

  size_t posSlash = path.find('/');
  size_t posBackslash = path.find('\\');
  std::string delim = posSlash < posBackslash ? "/" : "\\";
  std::vector<std::string> parts = StringUtils::Split(path, delim);
  std::vector<std::string> realParts;

  for (std::vector<std::string>::const_iterator part = parts.begin(); part != parts.end(); ++part)
  {
    if (part->empty() || part->compare(".") == 0)
      continue;

    // go one level back up
    if (part->compare("..") == 0)
    {
      if (!realParts.empty())
        realParts.pop_back();
      continue;
    }

    realParts.push_back(*part);
  }

  std::string realPath;
  // re-add any / or \ at the beginning
  for (std::string::const_iterator itPath = path.begin(); itPath != path.end(); ++itPath)
  {
    if (*itPath != delim.at(0))
      break;

    realPath += delim;
  }
  // put together the path
  realPath += StringUtils::Join(realParts, delim);
  // re-add any / or \ at the end
  if (path.at(path.size() - 1) == delim.at(0) &&
      realPath.size() > 0 && realPath.at(realPath.size() - 1) != delim.at(0))
    realPath += delim;

  return realPath;
}

bool URIUtils::UpdateUrlEncoding(std::string &strFilename)
{
  if (strFilename.empty())
    return false;

  CURL url(strFilename);
  // if this is a stack:// URL we need to work with its filename
  if (URIUtils::IsStack(strFilename))
  {
    std::vector<std::string> files;
    if (!CStackDirectory::GetPaths(strFilename, files))
      return false;

    for (std::vector<std::string>::iterator file = files.begin(); file != files.end(); ++file)
      UpdateUrlEncoding(*file);

    std::string stackPath;
    if (!CStackDirectory::ConstructStackPath(files, stackPath))
      return false;

    url.Parse(stackPath);
  }
  // if the protocol has an encoded hostname we need to work with its hostname
  else if (URIUtils::HasEncodedHostname(url))
  {
    std::string hostname = url.GetHostName();
    UpdateUrlEncoding(hostname);
    url.SetHostName(hostname);
  }
  else
    return false;

  std::string newFilename = url.Get();
  if (newFilename == strFilename)
    return false;

  strFilename = newFilename;
  return true;
}
