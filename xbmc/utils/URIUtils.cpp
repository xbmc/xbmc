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

#include "network/Network.h"
#include "URIUtils.h"
#include "Application.h"
#include "FileItem.h"
#include "filesystem/MultiPathDirectory.h"
#include "filesystem/MythDirectory.h"
#include "filesystem/SpecialProtocol.h"
#include "filesystem/StackDirectory.h"
#include "network/DNSNameCache.h"
#include "settings/AdvancedSettings.h"
#include "settings/MediaSettings.h"
#include "URL.h"
#include "StringUtils.h"

#include <netinet/in.h>
#include <arpa/inet.h>

using namespace std;
using namespace XFILE;

bool URIUtils::IsInPath(const CStdString &uri, const CStdString &baseURI)
{
  CStdString uriPath = CSpecialProtocol::TranslatePath(uri);
  CStdString basePath = CSpecialProtocol::TranslatePath(baseURI);
  return StringUtils::StartsWith(uriPath, basePath);
}

/* returns filename extension including period of filename */
CStdString URIUtils::GetExtension(const CStdString& strFileName)
{
  if (IsURL(strFileName))
  {
    CURL url(strFileName);
    return GetExtension(url.GetFileName());
  }

  size_t period = strFileName.find_last_of("./\\");
  if (period == string::npos || strFileName[period] != '.')
    return CStdString();

  return strFileName.substr(period);
}

bool URIUtils::HasExtension(const CStdString& strFileName)
{
  if (IsURL(strFileName))
  {
    CURL url(strFileName);
    return HasExtension(url.GetFileName());
  }

  size_t iPeriod = strFileName.find_last_of("./\\");
  return iPeriod != string::npos && strFileName[iPeriod] == '.';
}

bool URIUtils::HasExtension(const CStdString& strFileName, const CStdString& strExtensions)
{
  if (IsURL(strFileName))
  {
    CURL url(strFileName);
    return HasExtension(url.GetFileName(), strExtensions);
  }

  // Search backwards so that '.' can be used as a search terminator.
  CStdString::const_reverse_iterator itExtensions = strExtensions.rbegin();
  while (itExtensions != strExtensions.rend())
  {
    // Iterate backwards over strFileName untill we hit a '.' or a mismatch
    for (CStdString::const_reverse_iterator itFileName = strFileName.rbegin();
         itFileName != strFileName.rend() && itExtensions != strExtensions.rend() &&
         tolower(*itFileName) == *itExtensions;
         ++itFileName, ++itExtensions)
    {
      if (*itExtensions == '.')
        return true; // Match
    }

    // No match. Look for more extensions to try.
    while (itExtensions != strExtensions.rend() && *itExtensions != '|')
      ++itExtensions;

    while (itExtensions != strExtensions.rend() && *itExtensions == '|')
      ++itExtensions;
  }

  return false;
}

void URIUtils::RemoveExtension(CStdString& strFileName)
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
  if (period != string::npos && strFileName[period] == '.')
  {
    CStdString strExtension = strFileName.substr(period);
    StringUtils::ToLower(strExtension);
    strExtension += "|";

    CStdString strFileMask;
    strFileMask = g_advancedSettings.m_pictureExtensions;
    strFileMask += "|" + g_advancedSettings.m_musicExtensions;
    strFileMask += "|" + g_advancedSettings.m_videoExtensions;
    strFileMask += "|" + g_advancedSettings.m_subtitlesExtensions;
#if defined(TARGET_DARWIN)
    strFileMask += "|.py|.xml|.milk|.xpr|.xbt|.cdg|.app|.applescript|.workflow";
#else
    strFileMask += "|.py|.xml|.milk|.xpr|.xbt|.cdg";
#endif
    strFileMask += "|";

    if (strFileMask.find(strExtension) != std::string::npos)
      strFileName.erase(period);
  }
}

CStdString URIUtils::ReplaceExtension(const CStdString& strFile,
                                      const CStdString& strNewExtension)
{
  if(IsURL(strFile))
  {
    CURL url(strFile);
    url.SetFileName(ReplaceExtension(url.GetFileName(), strNewExtension));
    return url.Get();
  }

  CStdString strChangedFile;
  CStdString strExtension = GetExtension(strFile);
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

/* returns a filename given an url */
/* handles both / and \, and options in urls*/
const CStdString URIUtils::GetFileName(const CStdString& strFileNameAndPath)
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

void URIUtils::Split(const CStdString& strFileNameAndPath,
                     CStdString& strPath, CStdString& strFileName)
{
  std::string strPathT, strFileNameT;
  Split(strFileNameAndPath, strPathT, strFileNameT);
  strPath = strPathT;
  strFileName = strFileNameT;
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
}

CStdStringArray URIUtils::SplitPath(const CStdString& strPath)
{
  CURL url(strPath);

  // silly CStdString can't take a char in the constructor
  CStdString sep(1, url.GetDirectorySeparator());

  // split the filename portion of the URL up into separate dirs
  CStdStringArray dirs;
  StringUtils::SplitString(url.GetFileName(), sep, dirs);
  
  // we start with the root path
  CStdString dir = url.GetWithoutFilename();
  
  if (!dir.empty())
    dirs.insert(dirs.begin(), dir);

  // we don't need empty token on the end
  if (dirs.size() > 1 && dirs.back().empty())
    dirs.erase(dirs.end() - 1);

  return dirs;
}

void URIUtils::GetCommonPath(CStdString& strParent, const CStdString& strPath)
{
  // find the common path of parent and path
  unsigned int j = 1;
  while (j <= min(strParent.size(), strPath.size()) && strnicmp(strParent.c_str(), strPath.c_str(), j) == 0)
    j++;
  strParent.erase(j - 1);
  // they should at least share a / at the end, though for things such as path/cd1 and path/cd2 there won't be
  if (!HasSlashAtEnd(strParent))
  {
    strParent = GetDirectory(strParent);
    AddSlashAtEnd(strParent);
  }
}

bool URIUtils::ProtocolHasParentInHostname(const CStdString& prot)
{
  return prot.Equals("zip")
      || prot.Equals("rar")
      || prot.Equals("apk")
      || prot.Equals("bluray")
      || prot.Equals("udf");
}

bool URIUtils::ProtocolHasEncodedHostname(const CStdString& prot)
{
  return ProtocolHasParentInHostname(prot)
      || prot.Equals("musicsearch")
      || prot.Equals("image");
}

bool URIUtils::ProtocolHasEncodedFilename(const CStdString& prot)
{
  CStdString prot2 = CURL::TranslateProtocol(prot);

  // For now assume only (quasi) http internet streams use URL encoding
  return prot2 == "http"  ||
         prot2 == "https";
}

CStdString URIUtils::GetParentPath(const CStdString& strPath)
{
  CStdString strReturn;
  GetParentPath(strPath, strReturn);
  return strReturn;
}

bool URIUtils::GetParentPath(const CStdString& strPath, CStdString& strParent)
{
  strParent = "";

  CURL url(strPath);
  CStdString strFile = url.GetFileName();
  if ( URIUtils::ProtocolHasParentInHostname(url.GetProtocol()) && strFile.empty())
  {
    strFile = url.GetHostName();
    return GetParentPath(strFile, strParent);
  }
  else if (url.GetProtocol() == "stack")
  {
    CStackDirectory dir;
    CFileItemList items;
    dir.GetDirectory(strPath,items);
    items[0]->m_strDVDLabel = GetDirectory(items[0]->GetPath());
    if (StringUtils::StartsWithNoCase(items[0]->m_strDVDLabel, "rar://") || StringUtils::StartsWithNoCase(items[0]->m_strDVDLabel, "zip://"))
      GetParentPath(items[0]->m_strDVDLabel, strParent);
    else
      strParent = items[0]->m_strDVDLabel;
    for( int i=1;i<items.Size();++i)
    {
      items[i]->m_strDVDLabel = GetDirectory(items[i]->GetPath());
      if (StringUtils::StartsWithNoCase(items[0]->m_strDVDLabel, "rar://") || StringUtils::StartsWithNoCase(items[0]->m_strDVDLabel, "zip://"))
        items[i]->SetPath(GetParentPath(items[i]->m_strDVDLabel));
      else
        items[i]->SetPath(items[i]->m_strDVDLabel);

      GetCommonPath(strParent,items[i]->GetPath());
    }
    return true;
  }
  else if (url.GetProtocol() == "multipath")
  {
    // get the parent path of the first item
    return GetParentPath(CMultiPathDirectory::GetFirstPath(strPath), strParent);
  }
  else if (url.GetProtocol() == "plugin")
  {
    if (!url.GetOptions().empty())
    {
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
  else if (url.GetProtocol() == "special")
  {
    if (HasSlashAtEnd(strFile))
      strFile.erase(strFile.size() - 1);
    if(strFile.rfind('/') == std::string::npos)
      return false;
  }
  else if (strFile.size() == 0)
  {
    if (url.GetHostName().size() > 0)
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

std::string URLEncodePath(const std::string& strPath)
{
  vector<string> segments = StringUtils::Split(strPath, "/");
  for (vector<string>::iterator i = segments.begin(); i != segments.end(); ++i)
    *i = CURL::Encode(*i);

  return StringUtils::Join(segments, "/");
}

std::string URLDecodePath(const std::string& strPath)
{
  vector<string> segments = StringUtils::Split(strPath, "/");
  for (vector<string>::iterator i = segments.begin(); i != segments.end(); ++i)
    *i = CURL::Decode(*i);

  return StringUtils::Join(segments, "/");
}

std::string URIUtils::ChangeBasePath(const std::string &fromPath, const std::string &fromFile, const std::string &toPath)
{
  std::string toFile = fromFile;

  // Convert back slashes to forward slashes, if required
  if (IsDOSPath(fromPath) && !IsDOSPath(toPath))
    StringUtils::Replace(toFile, "\\", "/");

  // Handle difference in URL encoded vs. not encoded
  if ( ProtocolHasEncodedFilename(CURL(fromPath).GetProtocol() )
   && !ProtocolHasEncodedFilename(CURL(toPath).GetProtocol() ) )
  {
    toFile = URLDecodePath(toFile); // Decode path
  }
  else if (!ProtocolHasEncodedFilename(CURL(fromPath).GetProtocol() )
         && ProtocolHasEncodedFilename(CURL(toPath).GetProtocol() ) )
  {
    toFile = URLEncodePath(toFile); // Encode path
  }

  // Convert forward slashes to back slashes, if required
  if (!IsDOSPath(fromPath) && IsDOSPath(toPath))
    StringUtils::Replace(toFile, "/", "\\");

  return AddFileToFolder(toPath, toFile);
}

CStdString URIUtils::SubstitutePath(const CStdString& strPath, bool reverse /* = false */)
{
  for (CAdvancedSettings::StringMapping::iterator i = g_advancedSettings.m_pathSubstitutions.begin();
      i != g_advancedSettings.m_pathSubstitutions.end(); i++)
  {
    CStdString fromPath;
    CStdString toPath;

    if (!reverse)
    {
      fromPath = i->first;  // Fake path
      toPath = i->second;   // Real path
    }
    else
    {
      fromPath = i->second; // Real path
      toPath = i->first;    // Fake path
    }

    if (strncmp(strPath.c_str(), fromPath.c_str(), HasSlashAtEnd(fromPath) ? fromPath.size() - 1 : fromPath.size()) == 0)
    {
      if (strPath.size() > fromPath.size())
      {
        CStdString strSubPathAndFileName = strPath.substr(fromPath.size());
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

bool URIUtils::IsRemote(const CStdString& strFile)
{
  if (IsCDDA(strFile) || IsISO9660(strFile))
    return false;

  if (IsSpecial(strFile))
    return IsRemote(CSpecialProtocol::TranslatePath(strFile));

  if(IsStack(strFile))
    return IsRemote(CStackDirectory::GetFirstStackedFile(strFile));

  if(IsMultiPath(strFile))
  { // virtual paths need to be checked separately
    vector<CStdString> paths;
    if (CMultiPathDirectory::GetPaths(strFile, paths))
    {
      for (unsigned int i = 0; i < paths.size(); i++)
        if (IsRemote(paths[i])) return true;
    }
    return false;
  }

  CURL url(strFile);
  if(ProtocolHasParentInHostname(url.GetProtocol()))
    return IsRemote(url.GetHostName());

  if (!url.IsLocal())
    return true;

  return false;
}

bool URIUtils::IsOnDVD(const CStdString& strFile)
{
#ifdef TARGET_WINDOWS
  if (strFile.size() >= 2 && strFile.substr(1,1) == ":")
    return (GetDriveType(strFile.substr(0, 3).c_str()) == DRIVE_CDROM);
#endif

  if (StringUtils::StartsWith(strFile, "dvd:"))
    return true;

  if (StringUtils::StartsWith(strFile, "udf:"))
    return true;

  if (StringUtils::StartsWith(strFile, "iso9660:"))
    return true;

  if (StringUtils::StartsWith(strFile, "cdda:"))
    return true;

  return false;
}

bool URIUtils::IsOnLAN(const CStdString& strPath)
{
  if(IsMultiPath(strPath))
    return IsOnLAN(CMultiPathDirectory::GetFirstPath(strPath));

  if(IsStack(strPath))
    return IsOnLAN(CStackDirectory::GetFirstStackedFile(strPath));

  if(IsSpecial(strPath))
    return IsOnLAN(CSpecialProtocol::TranslatePath(strPath));

  if(IsDAAP(strPath))
    return true;
  
  if(IsPlugin(strPath))
    return false;

  if(IsTuxBox(strPath))
    return true;

  if(IsUPnP(strPath))
    return true;

  CURL url(strPath);
  if (ProtocolHasParentInHostname(url.GetProtocol()))
    return IsOnLAN(url.GetHostName());

  if(!IsRemote(strPath))
    return false;

  CStdString host = url.GetHostName();

  return IsHostOnLAN(host);
}

static bool addr_match(uint32_t addr, const char* target, const char* submask)
{
  uint32_t addr2 = ntohl(inet_addr(target));
  uint32_t mask = ntohl(inet_addr(submask));
  return (addr & mask) == (addr2 & mask);
}

bool URIUtils::IsHostOnLAN(const CStdString& host, bool offLineCheck)
{
  if(host.length() == 0)
    return false;

  // assume a hostname without dot's
  // is local (smb netbios hostnames)
  if(host.find('.') == string::npos)
    return true;

  uint32_t address = ntohl(inet_addr(host.c_str()));
  if(address == INADDR_NONE)
  {
    CStdString ip;
    if(CDNSNameCache::Lookup(host, ip))
      address = ntohl(inet_addr(ip.c_str()));
  }

  if(address != INADDR_NONE)
  {
    if (offLineCheck) // check if in private range, ref https://en.wikipedia.org/wiki/Private_network
    {
      if (
        addr_match(address, "192.168.0.0", "255.255.0.0") ||
        addr_match(address, "10.0.0.0", "255.0.0.0") ||
        addr_match(address, "172.16.0.0", "255.240.0.0")
        )
        return true;
    }
    // check if we are on the local subnet
    if (!g_application.getNetwork().GetFirstConnectedInterface())
      return false;

    if (g_application.getNetwork().HasInterfaceForIP(address))
      return true;
  }

  return false;
}

bool URIUtils::IsMultiPath(const CStdString& strPath)
{
  return StringUtils::StartsWithNoCase(strPath, "multipath:");
}

bool URIUtils::IsHD(const CStdString& strFileName)
{
  CURL url(strFileName);

  if (IsSpecial(strFileName))
    return IsHD(CSpecialProtocol::TranslatePath(strFileName));

  if(IsStack(strFileName))
    return IsHD(CStackDirectory::GetFirstStackedFile(strFileName));

  if (ProtocolHasParentInHostname(url.GetProtocol()))
    return IsHD(url.GetHostName());

  return url.GetProtocol().empty() || url.GetProtocol() == "file";
}

bool URIUtils::IsDVD(const CStdString& strFile)
{
  CStdString strFileLow = strFile;
  StringUtils::ToLower(strFileLow);
  if (strFileLow.find("video_ts.ifo") != std::string::npos && IsOnDVD(strFile))
    return true;

#if defined(TARGET_WINDOWS)
  if (StringUtils::StartsWithNoCase(strFile, "dvd://"))
    return true;

  if(strFile.size() < 2 || (strFile.substr(1) != ":\\" && strFile.substr(1) != ":"))
    return false;

  if(GetDriveType(strFile.c_str()) == DRIVE_CDROM)
    return true;
#else
  if (strFileLow == "iso9660://" || strFileLow == "udf://" || strFileLow == "dvd://1" )
    return true;
#endif

  return false;
}

bool URIUtils::IsStack(const CStdString& strFile)
{
  return StringUtils::StartsWithNoCase(strFile, "stack:");
}

bool URIUtils::IsRAR(const CStdString& strFile)
{
  CStdString strExtension = GetExtension(strFile);

  if (strExtension.Equals(".001") && !StringUtils::EndsWithNoCase(strFile, ".ts.001"))
    return true;

  if (StringUtils::EqualsNoCase(strExtension, ".cbr"))
    return true;

  if (StringUtils::EqualsNoCase(strExtension, ".rar"))
    return true;

  return false;
}

bool URIUtils::IsInArchive(const CStdString &strFile)
{
  return IsInZIP(strFile) || IsInRAR(strFile) || IsInAPK(strFile);
}

bool URIUtils::IsInAPK(const CStdString& strFile)
{
  CURL url(strFile);

  return url.GetProtocol() == "apk" && url.GetFileName() != "";
}

bool URIUtils::IsInZIP(const CStdString& strFile)
{
  CURL url(strFile);

  return url.GetProtocol() == "zip" && url.GetFileName() != "";
}

bool URIUtils::IsInRAR(const CStdString& strFile)
{
  CURL url(strFile);

  return url.GetProtocol() == "rar" && url.GetFileName() != "";
}

bool URIUtils::IsAPK(const CStdString& strFile)
{
  return HasExtension(strFile, ".apk");
}

bool URIUtils::IsZIP(const CStdString& strFile) // also checks for comic books!
{
  return HasExtension(strFile, ".zip|.cbz");
}

bool URIUtils::IsArchive(const CStdString& strFile)
{
  return HasExtension(strFile, ".zip|.rar|.apk|.cbz|.cbr");
}

bool URIUtils::IsSpecial(const CStdString& strFile)
{
  CStdString strFile2(strFile);

  if (IsStack(strFile))
    strFile2 = CStackDirectory::GetFirstStackedFile(strFile);

  return StringUtils::StartsWithNoCase(strFile2, "special:");
}

bool URIUtils::IsPlugin(const CStdString& strFile)
{
  CURL url(strFile);
  return url.GetProtocol().Equals("plugin");
}

bool URIUtils::IsScript(const CStdString& strFile)
{
  CURL url(strFile);
  return url.GetProtocol().Equals("script");
}

bool URIUtils::IsAddonsPath(const CStdString& strFile)
{
  CURL url(strFile);
  return url.GetProtocol().Equals("addons");
}

bool URIUtils::IsSourcesPath(const CStdString& strPath)
{
  CURL url(strPath);
  return url.GetProtocol().Equals("sources");
}

bool URIUtils::IsCDDA(const CStdString& strFile)
{
  return StringUtils::StartsWithNoCase(strFile, "cdda:");
}

bool URIUtils::IsISO9660(const CStdString& strFile)
{
  return StringUtils::StartsWithNoCase(strFile, "iso9660:");
}

bool URIUtils::IsSmb(const CStdString& strFile)
{
  CStdString strFile2(strFile);

  if (IsStack(strFile))
    strFile2 = CStackDirectory::GetFirstStackedFile(strFile);

  return StringUtils::StartsWithNoCase(strFile2, "smb:");
}

bool URIUtils::IsURL(const CStdString& strFile)
{
  return strFile.find("://") != std::string::npos;
}

bool URIUtils::IsFTP(const CStdString& strFile)
{
  CStdString strFile2(strFile);

  if (IsStack(strFile))
    strFile2 = CStackDirectory::GetFirstStackedFile(strFile);

  return StringUtils::StartsWithNoCase(strFile2, "ftp:")  ||
         StringUtils::StartsWithNoCase(strFile2, "ftps:");
}

bool URIUtils::IsUDP(const CStdString& strFile)
{
  CStdString strFile2(strFile);

  if (IsStack(strFile))
    strFile2 = CStackDirectory::GetFirstStackedFile(strFile);

  return StringUtils::StartsWithNoCase(strFile2, "udp:");
}

bool URIUtils::IsTCP(const CStdString& strFile)
{
  CStdString strFile2(strFile);

  if (IsStack(strFile))
    strFile2 = CStackDirectory::GetFirstStackedFile(strFile);

  return StringUtils::StartsWithNoCase(strFile2, "tcp:");
}

bool URIUtils::IsPVRChannel(const CStdString& strFile)
{
  CStdString strFile2(strFile);

  if (IsStack(strFile))
    strFile2 = CStackDirectory::GetFirstStackedFile(strFile);

  return StringUtils::StartsWithNoCase(strFile2, "pvr://channels");
}

bool URIUtils::IsDAV(const CStdString& strFile)
{
  CStdString strFile2(strFile);

  if (IsStack(strFile))
    strFile2 = CStackDirectory::GetFirstStackedFile(strFile);

  return StringUtils::StartsWithNoCase(strFile2, "dav:")  ||
         StringUtils::StartsWithNoCase(strFile2, "davs:");
}

bool URIUtils::IsInternetStream(const CURL& url, bool bStrictCheck /* = false */)
{
  CStdString strProtocol = url.GetProtocol();
  
  if (strProtocol.empty())
    return false;

  // there's nothing to stop internet streams from being stacked
  if (strProtocol == "stack")
    return IsInternetStream(CStackDirectory::GetFirstStackedFile(url.Get()));

  CStdString strProtocol2 = url.GetTranslatedProtocol();

  // Special case these
  if (strProtocol  == "ftp"   || strProtocol  == "ftps"   ||
      strProtocol  == "dav"   || strProtocol  == "davs")
    return bStrictCheck;

  if (strProtocol2 == "http"  || strProtocol2 == "https"  ||
      strProtocol2 == "tcp"   || strProtocol2 == "udp"    ||
      strProtocol2 == "rtp"   || strProtocol2 == "sdp"    ||
      strProtocol2 == "mms"   || strProtocol2 == "mmst"   ||
      strProtocol2 == "mmsh"  || strProtocol2 == "rtsp"   ||
      strProtocol2 == "rtmp"  || strProtocol2 == "rtmpt"  ||
      strProtocol2 == "rtmpe" || strProtocol2 == "rtmpte" ||
      strProtocol2 == "rtmps")
    return true;

  return false;
}

bool URIUtils::IsDAAP(const CStdString& strFile)
{
  return StringUtils::StartsWithNoCase(strFile, "daap:");
}

bool URIUtils::IsUPnP(const CStdString& strFile)
{
  return StringUtils::StartsWithNoCase(strFile, "upnp:");
}

bool URIUtils::IsTuxBox(const CStdString& strFile)
{
  return StringUtils::StartsWithNoCase(strFile, "tuxbox:");
}

bool URIUtils::IsMythTV(const CStdString& strFile)
{
  return StringUtils::StartsWithNoCase(strFile, "myth:");
}

bool URIUtils::IsHDHomeRun(const CStdString& strFile)
{
  return StringUtils::StartsWithNoCase(strFile, "hdhomerun:");
}

bool URIUtils::IsSlingbox(const CStdString& strFile)
{
  return StringUtils::StartsWithNoCase(strFile, "sling:");
}

bool URIUtils::IsVTP(const CStdString& strFile)
{
  return StringUtils::StartsWithNoCase(strFile, "vtp:");
}

bool URIUtils::IsHTSP(const CStdString& strFile)
{
  return StringUtils::StartsWithNoCase(strFile, "htsp:");
}

bool URIUtils::IsLiveTV(const CStdString& strFile)
{
  CStdString strFileWithoutSlash(strFile);
  RemoveSlashAtEnd(strFileWithoutSlash);

  if(IsTuxBox(strFile)
  || IsVTP(strFile)
  || IsHDHomeRun(strFile)
  || IsSlingbox(strFile)
  || IsHTSP(strFile)
  || StringUtils::StartsWithNoCase(strFile, "sap:")
  ||(StringUtils::EndsWithNoCase(strFileWithoutSlash, ".pvr") && !StringUtils::StartsWithNoCase(strFileWithoutSlash, "pvr://recordings")))
    return true;

  if (IsMythTV(strFile) && CMythDirectory::IsLiveTV(strFile))
    return true;

  return false;
}

bool URIUtils::IsPVRRecording(const CStdString& strFile)
{
  CStdString strFileWithoutSlash(strFile);
  RemoveSlashAtEnd(strFileWithoutSlash);

  return StringUtils::EndsWithNoCase(strFileWithoutSlash, ".pvr") &&
         StringUtils::StartsWithNoCase(strFile, "pvr://recordings");
}

bool URIUtils::IsMusicDb(const CStdString& strFile)
{
  return StringUtils::StartsWithNoCase(strFile, "musicdb:");
}

bool URIUtils::IsNfs(const CStdString& strFile)
{
  CStdString strFile2(strFile);
  
  if (IsStack(strFile))
    strFile2 = CStackDirectory::GetFirstStackedFile(strFile);
  
  return StringUtils::StartsWithNoCase(strFile2, "nfs:");
}

bool URIUtils::IsAfp(const CStdString& strFile)
{
  CStdString strFile2(strFile);
  
  if (IsStack(strFile))
    strFile2 = CStackDirectory::GetFirstStackedFile(strFile);
  
  return StringUtils::StartsWithNoCase(strFile2, "afp:");
}


bool URIUtils::IsVideoDb(const CStdString& strFile)
{
  return StringUtils::StartsWithNoCase(strFile, "videodb:");
}

bool URIUtils::IsBluray(const CStdString& strFile)
{
  return StringUtils::StartsWithNoCase(strFile, "bluray:");
}

bool URIUtils::IsAndroidApp(const CStdString &path)
{
  return StringUtils::StartsWithNoCase(path, "androidapp:");
}

bool URIUtils::IsLibraryFolder(const CStdString& strFile)
{
  CURL url(strFile);
  return url.GetProtocol().Equals("library");
}

bool URIUtils::IsDOSPath(const CStdString &path)
{
  if (path.size() > 1 && path[1] == ':' && isalpha(path[0]))
    return true;

  // windows network drives
  if (path.size() > 1 && path[0] == '\\' && path[1] == '\\')
    return true;

  return false;
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
    CStdString file = url.GetFileName();
    return file.empty() || HasSlashAtEnd(file, false);
  }
  char kar = strFile.c_str()[strFile.size() - 1];

  if (kar == '/' || kar == '\\')
    return true;

  return false;
}

void URIUtils::RemoveSlashAtEnd(std::string& strFolder)
{
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

bool URIUtils::CompareWithoutSlashAtEnd(const CStdString& strPath1, const CStdString& strPath2)
{
  CStdString strc1 = strPath1, strc2 = strPath2;
  RemoveSlashAtEnd(strc1);
  RemoveSlashAtEnd(strc2);
  return strc1.Equals(strc2);
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


CStdString URIUtils::AddFileToFolder(const CStdString& strFolder, 
                                const CStdString& strFile)
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

  CStdString strResult = strFolder;
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

CStdString URIUtils::GetDirectory(const CStdString &strFilePath)
{
  // Will from a full filename return the directory the file resides in.
  // Keeps the final slash at end and possible |option=foo options.

  size_t iPosSlash = strFilePath.find_last_of("/\\");
  if (iPosSlash == string::npos)
    return ""; // No slash, so no path (ignore any options)

  size_t iPosBar = strFilePath.rfind('|');
  if (iPosBar == string::npos)
    return strFilePath.substr(0, iPosSlash + 1); // Only path

  return strFilePath.substr(0, iPosSlash + 1) + strFilePath.substr(iPosBar); // Path + options
}

void URIUtils::CreateArchivePath(CStdString& strUrlPath,
                                 const CStdString& strType,
                                 const CStdString& strArchivePath,
                                 const CStdString& strFilePathInArchive,
                                 const CStdString& strPwd)
{
  strUrlPath = strType+"://";

  if( !strPwd.empty() )
  {
    strUrlPath += CURL::Encode(strPwd);
    strUrlPath += "@";
  }

  strUrlPath += CURL::Encode(strArchivePath);

  CStdString strBuffer(strFilePathInArchive);
  StringUtils::Replace(strBuffer, '\\', '/');
  StringUtils::TrimLeft(strBuffer, "/");

  strUrlPath += "/";
  strUrlPath += strBuffer;

#if 0 // options are not used
  strBuffer = strCachePath;
  strBuffer = CURL::Encode(strBuffer);

  strUrlPath += "?cache=";
  strUrlPath += strBuffer;

  strBuffer = StringUtils::Format("%i", wOptions);
  strUrlPath += "&flags=";
  strUrlPath += strBuffer;
#endif
}

string URIUtils::GetRealPath(const string &path)
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
  string delim = posSlash < posBackslash ? "/" : "\\";
  vector<string> parts = StringUtils::Split(path, delim);
  vector<string> realParts;

  for (vector<string>::const_iterator part = parts.begin(); part != parts.end(); part++)
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

  CStdString realPath;
  int i = 0;
  // re-add any / or \ at the beginning
  while (path.at(i) == delim.at(0))
  {
    realPath += delim;
    i++;
  }
  // put together the path
  realPath += StringUtils::Join(realParts, delim);
  // re-add any / or \ at the end
  if (path.at(path.size() - 1) == delim.at(0) && realPath.at(realPath.size() - 1) != delim.at(0))
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
    vector<CStdString> files;
    if (!CStackDirectory::GetPaths(strFilename, files))
      return false;

    for (vector<CStdString>::iterator file = files.begin(); file != files.end(); file++)
    {
      std::string filePath = *file;
      UpdateUrlEncoding(filePath);
      *file = filePath;
    }

    CStdString stackPath;
    if (!CStackDirectory::ConstructStackPath(files, stackPath))
      return false;

    url.Parse(stackPath);
  }
  // if the protocol has an encoded hostname we need to work with its hostname
  else if (URIUtils::ProtocolHasEncodedHostname(url.GetProtocol()))
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

bool URIUtils::IsUsingFastSwitch(const CStdString& strFile)
{
  return IsUDP(strFile) || IsTCP(strFile) || IsPVRChannel(strFile);
}
