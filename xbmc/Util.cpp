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
#include "threads/SystemClock.h"
#include "system.h"
#include "CompileInfo.h"
#if defined(TARGET_DARWIN)
#include <sys/param.h>
#include <mach-o/dyld.h>
#endif

#if defined(TARGET_FREEBSD)
#include <sys/param.h>
#include <sys/sysctl.h>
#endif

#ifdef TARGET_POSIX
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/wait.h>
#endif
#if defined(TARGET_ANDROID)
#include "android/bionic_supplement/bionic_supplement.h"
#endif
#include <stdlib.h>

#include "Application.h"
#include "Util.h"
#include "addons/Addon.h"
#include "filesystem/PVRDirectory.h"
#include "filesystem/Directory.h"
#include "filesystem/StackDirectory.h"
#include "filesystem/MultiPathDirectory.h"
#include "filesystem/SpecialProtocol.h"
#include "filesystem/RSSDirectory.h"
#ifdef HAS_FILESYSTEM_RAR
#include "filesystem/RarManager.h"
#endif
#include "filesystem/MythDirectory.h"
#ifdef HAS_UPNP
#include "filesystem/UPnPDirectory.h"
#endif
#include "profiles/ProfilesManager.h"
#include "utils/RegExp.h"
#include "guilib/GraphicContext.h"
#include "guilib/TextureManager.h"
#include "utils/fstrcmp.h"
#include "storage/MediaManager.h"
#ifdef TARGET_WINDOWS
#include "utils/CharsetConverter.h"
#include <shlobj.h>
#include "WIN32Util.h"
#endif
#if defined(TARGET_DARWIN)
#include "osx/DarwinUtils.h"
#endif
#include "GUIUserMessages.h"
#include "filesystem/File.h"
#include "settings/MediaSettings.h"
#include "settings/Settings.h"
#include "utils/StringUtils.h"
#include "settings/AdvancedSettings.h"
#ifdef HAS_IRSERVERSUITE
  #include "input/windows/IRServerSuite.h"
#endif
<<<<<<< HEAD
#include "guilib/LocalizeStrings.h"
=======
#include "WindowingFactory.h"
#include "LocalizeStrings.h"
>>>>>>> FETCH_HEAD
#include "utils/md5.h"
#include "utils/TimeUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#include "utils/Environment.h"

#include "cores/dvdplayer/DVDSubtitles/DVDSubtitleTagSami.h"
#include "cores/dvdplayer/DVDSubtitles/DVDSubtitleStream.h"
#include "URL.h"
#include "utils/LangCodeExpander.h"
#ifdef HAVE_LIBCAP
  #include <sys/capability.h>
#endif

#include "cores/dvdplayer/DVDDemuxers/DVDDemux.h"

using namespace std;

#ifdef HAS_DVD_DRIVE
using namespace MEDIA_DETECT;
#endif

#define clamp(x) (x) > 255.f ? 255 : ((x) < 0 ? 0 : (BYTE)(x+0.5f)) // Valid ranges: brightness[-1 -> 1 (0 is default)] contrast[0 -> 2 (1 is default)]  gamma[0.5 -> 3.5 (1 is default)] default[ramp is linear]

using namespace XFILE;
using namespace PLAYLIST;

#ifdef HAS_DX
static D3DGAMMARAMP oldramp, flashramp;
#elif defined(HAS_SDL_2D)
static uint16_t oldrampRed[256];
static uint16_t oldrampGreen[256];
static uint16_t oldrampBlue[256];
static uint16_t flashrampRed[256];
static uint16_t flashrampGreen[256];
static uint16_t flashrampBlue[256];
#endif

#if !defined(TARGET_WINDOWS)
unsigned int CUtil::s_randomSeed = time(NULL);
#endif

CUtil::CUtil(void)
{
}

CUtil::~CUtil(void)
{}

CStdString CUtil::GetTitleFromPath(const CStdString& strFileNameAndPath, bool bIsFolder /* = false */)
{
  CURL pathToUrl(strFileNameAndPath);
  return GetTitleFromPath(pathToUrl, bIsFolder);
}

CStdString CUtil::GetTitleFromPath(const CURL& url, bool bIsFolder /* = false */)
{
  // use above to get the filename
  CStdString path(url.Get());
  URIUtils::RemoveSlashAtEnd(path);
  CStdString strFilename = URIUtils::GetFileName(path);

  CStdString strHostname = url.GetHostName();

#ifdef HAS_UPNP
  // UPNP
  if (url.IsProtocol("upnp"))
    strFilename = CUPnPDirectory::GetFriendlyName(url);
#endif

  if (url.IsProtocol("rss"))
  {
    CRSSDirectory dir;
    CFileItemList items;
    if(dir.GetDirectory(url, items) && !items.m_strTitle.empty())
      return items.m_strTitle;
  }

  // Shoutcast
  else if (url.IsProtocol("shout"))
  {
    const CStdString strFileNameAndPath = url.Get();
    const int genre = strFileNameAndPath.find_first_of('=');
    if(genre <0)
      strFilename = g_localizeStrings.Get(260);
    else
      strFilename = g_localizeStrings.Get(260) + " - " + strFileNameAndPath.substr(genre+1).c_str();
  }

  // Windows SMB Network (SMB)
  else if (url.IsProtocol("smb") && strFilename.empty())
  {
    if (url.GetHostName().empty())
    {
      strFilename = g_localizeStrings.Get(20171);
    }
    else
    {
      strFilename = url.GetHostName();
    }
  }
  // iTunes music share (DAAP)
  else if (url.IsProtocol("daap") && strFilename.empty())
    strFilename = g_localizeStrings.Get(20174);

  // HDHomerun Devices
  else if (url.IsProtocol("hdhomerun") && strFilename.empty())
    strFilename = "HDHomerun Devices";

  // Slingbox Devices
  else if (url.IsProtocol("sling"))
    strFilename = "Slingbox";

  // ReplayTV Devices
  else if (url.IsProtocol("rtv"))
    strFilename = "ReplayTV Devices";

  // HTS Tvheadend client
  else if (url.IsProtocol("htsp"))
    strFilename = g_localizeStrings.Get(20256);

  // VDR Streamdev client
  else if (url.IsProtocol("vtp"))
    strFilename = g_localizeStrings.Get(20257);
  
  // MythTV client
  else if (url.IsProtocol("myth"))
    strFilename = g_localizeStrings.Get(20258);

  // SAP Streams
  else if (url.IsProtocol("sap") && strFilename.empty())
    strFilename = "SAP Streams";

  // Root file views
  else if (url.IsProtocol("sources"))
    strFilename = g_localizeStrings.Get(744);

  // Music Playlists
  else if (URIUtils::PathStarts(path, "special://musicplaylists"))
    strFilename = g_localizeStrings.Get(136);

  // Video Playlists
  else if (URIUtils::PathStarts(path, "special://videoplaylists"))
    strFilename = g_localizeStrings.Get(136);

  else if (URIUtils::HasParentInHostname(url) && strFilename.empty())
    strFilename = URIUtils::GetFileName(url.GetHostName());

  // now remove the extension if needed
  if (!CSettings::Get().GetBool("filelists.showextensions") && !bIsFolder)
  {
    URIUtils::RemoveExtension(strFilename);
    return strFilename;
  }
  
  // URLDecode since the original path may be an URL
<<<<<<< HEAD
  strFilename = CURL::Decode(strFilename);
  return strFilename;
}

void CUtil::CleanString(const std::string& strFileName,
                        std::string& strTitle,
                        std::string& strTitleAndYear,
                        std::string& strYear,
                        bool bRemoveExtension /* = false */,
                        bool bCleanChars /* = true */)
=======
  URLDecode(strFilename);
  return strFilename;
}

bool CUtil::GetVolumeFromFileName(const CStdString& strFileName, CStdString& strFileTitle, CStdString& strVolumeNumber)
{
  const CStdStringArray &regexps = g_advancedSettings.m_videoStackRegExps;

  CStdString strFileNameTemp = strFileName;

  CRegExp reg(true);

  for (unsigned int i = 0; i < regexps.size(); i++)
  {
    CStdString strRegExp = regexps[i];
    if (!reg.RegComp(strRegExp.c_str()))
    { // invalid regexp - complain in logs
      CLog::Log(LOGERROR, "Invalid RegExp:[%s]", regexps[i].c_str());
      continue;
    }
//    CLog::Log(LOGDEBUG, "Regexp:[%s]", regexps[i].c_str());

    int iFoundToken = reg.RegFind(strFileName.c_str());
    if (iFoundToken >= 0)
    {
      int iRegLength = reg.GetFindLen();
      int iCount = reg.GetSubCount();

      /*
      reg.DumpOvector(LOGDEBUG);
      CLog::Log(LOGDEBUG, "Subcount=%i", iCount);
      for (int j = 0; j <= iCount; j++)
      {
        CStdString str = reg.GetMatch(j);
        CLog::Log(LOGDEBUG, "Sub(%i):[%s]", j, str.c_str());
      }
      */

      // simple regexp, only the volume is captured
      if (iCount == 1)
      {
        strVolumeNumber = reg.GetMatch(1);
        if (strVolumeNumber.IsEmpty()) return false;

        // Remove the extension (if any).  We do this on the base filename, as the regexp
        // match may include some of the extension (eg the "." in particular).
        // The extension will then be added back on at the end - there is no reason
        // to clean it off here. It will be cleaned off during the display routine, if
        // the settings to hide extensions are turned on.
        CStdString strFileNoExt = strFileNameTemp;
        RemoveExtension(strFileNoExt);
        CStdString strFileExt = strFileNameTemp.Right(strFileNameTemp.length() - strFileNoExt.length());
        CStdString strFileRight = strFileNoExt.Mid(iFoundToken + iRegLength);
        strFileTitle = strFileName.Left(iFoundToken) + strFileRight + strFileExt;

        return true;
      }

      // advanced regexp with prefix (1), volume (2), and suffix (3)
      else if (iCount == 3)
      {
        // second subpatten contains the stacking volume
        strVolumeNumber = reg.GetMatch(2);
        if (strVolumeNumber.IsEmpty()) return false;

        // everything before the regexp match
        strFileTitle = strFileName.Left(iFoundToken);

        // first subpattern contains prefix
        strFileTitle += reg.GetMatch(1);

        // third subpattern contains suffix
        strFileTitle += reg.GetMatch(3);

        // everything after the regexp match
        strFileTitle += strFileNameTemp.Mid(iFoundToken + iRegLength);

        return true;
      }

      // unknown regexp format
      else
      {
        CLog::Log(LOGERROR, "Incorrect movie stacking regexp format:[%s]", regexps[i].c_str());
      }
    }
  }
  return false;
}

void CUtil::RemoveExtension(CStdString& strFileName)
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

  int iPos = strFileName.ReverseFind(".");
  // Extension found
  if (iPos > 0)
  {
    CStdString strExtension;
    CUtil::GetExtension(strFileName, strExtension);
    strExtension.ToLower();
    strExtension += "|";

    CStdString strFileMask;
    strFileMask = g_settings.m_pictureExtensions;
    strFileMask += "|" + g_settings.m_musicExtensions;
    strFileMask += "|" + g_settings.m_videoExtensions;
#if defined(__APPLE__)
    strFileMask += "|.py|.xml|.milk|.xpr|.xbt|.cdg|.app|.applescript|.workflow";
#else
    strFileMask += "|.py|.xml|.milk|.xpr|.xbt|.cdg";
#endif
    strFileMask += "|";

    if (strFileMask.Find(strExtension) >= 0)
      strFileName = strFileName.Left(iPos);
  }
}

void CUtil::CleanString(CStdString& strFileName, CStdString& strTitle, CStdString& strTitleAndYear, CStdString& strYear, bool bRemoveExtension /* = false */, bool bCleanChars /* = true */)
>>>>>>> FETCH_HEAD
{
  strTitleAndYear = strFileName;

  if (strFileName == "..")
   return;

  const vector<string> &regexps = g_advancedSettings.m_videoCleanStringRegExps;

  CRegExp reTags(true, CRegExp::autoUtf8);
  CRegExp reYear(false, CRegExp::autoUtf8);

  if (!reYear.RegComp(g_advancedSettings.m_videoCleanDateTimeRegExp))
  {
    CLog::Log(LOGERROR, "%s: Invalid datetime clean RegExp:'%s'", __FUNCTION__, g_advancedSettings.m_videoCleanDateTimeRegExp.c_str());
  }
  else
  {
    if (reYear.RegFind(strTitleAndYear.c_str()) >= 0)
    {
      strTitleAndYear = reYear.GetMatch(1);
      strYear = reYear.GetMatch(2);
    }
  }

  URIUtils::RemoveExtension(strTitleAndYear);

  for (unsigned int i = 0; i < regexps.size(); i++)
  {
    if (!reTags.RegComp(regexps[i].c_str()))
    { // invalid regexp - complain in logs
      CLog::Log(LOGERROR, "%s: Invalid string clean RegExp:'%s'", __FUNCTION__, regexps[i].c_str());
      continue;
    }
    int j=0;
    if ((j=reTags.RegFind(strTitleAndYear.c_str())) > 0)
      strTitleAndYear = strTitleAndYear.substr(0, j);
  }

  // final cleanup - special characters used instead of spaces:
  // all '_' tokens should be replaced by spaces
  // if the file contains no spaces, all '.' tokens should be replaced by
  // spaces - one possibility of a mistake here could be something like:
  // "Dr..StrangeLove" - hopefully no one would have anything like this.
  if (bCleanChars)
  {
    bool initialDots = true;
    bool alreadyContainsSpace = (strTitleAndYear.find(' ') != std::string::npos);

    for (size_t i = 0; i < strTitleAndYear.size(); i++)
    {
      char c = strTitleAndYear[i];

      if (c != '.')
        initialDots = false;

      if ((c == '_') || ((!alreadyContainsSpace) && !initialDots && (c == '.')))
      {
        strTitleAndYear[i] = ' ';
      }
    }
  }

  StringUtils::Trim(strTitleAndYear);
  strTitle = strTitleAndYear;

  // append year
  if (!strYear.empty())
    strTitleAndYear = strTitle + " (" + strYear + ")";

  // restore extension if needed
  if (!bRemoveExtension)
    strTitleAndYear += URIUtils::GetExtension(strFileName);
}

void CUtil::GetQualifiedFilename(const std::string &strBasePath, std::string &strFilename)
{
  // Check if the filename is a fully qualified URL such as protocol://path/to/file
  CURL plItemUrl(strFilename);
  if (!plItemUrl.GetProtocol().empty())
    return;

  // If the filename starts "x:", "\\" or "/" it's already fully qualified so return
  if (strFilename.size() > 1)
#ifdef TARGET_POSIX
    if ( (strFilename[1] == ':') || (strFilename[0] == '/') )
#else
    if ( strFilename[1] == ':' || (strFilename[0] == '\\' && strFilename[1] == '\\'))
#endif
      return;

  // add to base path and then clean
  strFilename = URIUtils::AddFileToFolder(strBasePath, strFilename);

  // get rid of any /./ or \.\ that happen to be there
  StringUtils::Replace(strFilename, "\\.\\", "\\");
  StringUtils::Replace(strFilename, "/./", "/");

  // now find any "\\..\\" and remove them via GetParentPath
  size_t pos;
  while ((pos = strFilename.find("/../")) != std::string::npos)
  {
    CStdString basePath = strFilename.substr(0, pos + 1);
    strFilename.erase(0, pos + 4);
    basePath = URIUtils::GetParentPath(basePath);
    strFilename = URIUtils::AddFileToFolder(basePath, strFilename);
  }
  while ((pos = strFilename.find("\\..\\")) != std::string::npos)
  {
    CStdString basePath = strFilename.substr(0, pos + 1);
    strFilename.erase(0, pos + 4);
    basePath = URIUtils::GetParentPath(basePath);
    strFilename = URIUtils::AddFileToFolder(basePath, strFilename);
  }
}

#ifdef UNIT_TESTING
bool CUtil::TestGetQualifiedFilename()
{
  CStdString file = "../foo"; GetQualifiedFilename("smb://", file);
  if (file != "foo") return false;
  file = "C:\\foo\\bar"; GetQualifiedFilename("smb://", file);
  if (file != "C:\\foo\\bar") return false;
  file = "../foo/./bar"; GetQualifiedFilename("smb://my/path", file);
  if (file != "smb://my/foo/bar") return false;
  file = "smb://foo/bar/"; GetQualifiedFilename("upnp://", file);
  if (file != "smb://foo/bar/") return false;
  return true;
}

bool CUtil::TestMakeLegalPath()
{
  CStdString path;
#ifdef TARGET_WINDOWS
  path = "C:\\foo\\bar"; path = MakeLegalPath(path);
  if (path != "C:\\foo\\bar") return false;
  path = "C:\\foo:\\bar\\"; path = MakeLegalPath(path);
  if (path != "C:\\foo_\\bar\\") return false;
#elif
  path = "/foo/bar/"; path = MakeLegalPath(path);
  if (path != "/foo/bar/") return false;
  path = "/foo?/bar"; path = MakeLegalPath(path);
  if (path != "/foo_/bar") return false;
#endif
  path = "smb://foo/bar"; path = MakeLegalPath(path);
  if (path != "smb://foo/bar") return false;
  path = "smb://foo/bar?/"; path = MakeLegalPath(path);
  if (path != "smb://foo/bar_/") return false;
  return true;
}
#endif

void CUtil::RunShortcut(const char* szShortcutPath)
{
}

void CUtil::GetHomePath(std::string& strPath, const std::string& strTarget)
{
  if (strTarget.empty())
    strPath = CEnvironment::getenv("KODI_HOME");
  else
    strPath = CEnvironment::getenv(strTarget);

#ifdef TARGET_WINDOWS
  if (strPath.find("..") != std::string::npos)
  {
    //expand potential relative path to full path
    CStdStringW strPathW;
    g_charsetConverter.utf8ToW(strPath, strPathW, false);
    CWIN32Util::AddExtraLongPathPrefix(strPathW);
    const unsigned int bufSize = GetFullPathNameW(strPathW, 0, NULL, NULL);
    if (bufSize != 0)
    {
      wchar_t * buf = new wchar_t[bufSize];
      if (GetFullPathNameW(strPathW, bufSize, buf, NULL) <= bufSize-1)
      {
        std::wstring expandedPathW(buf);
        CWIN32Util::RemoveExtraLongPathPrefix(expandedPathW);
        g_charsetConverter.wToUTF8(expandedPathW, strPath);
      }

      delete [] buf;
    }
  }
#endif

  if (strPath.empty())
  {
    std::string strHomePath = ResolveExecutablePath();
#if defined(TARGET_DARWIN)
    int      result = -1;
    char     given_path[2*MAXPATHLEN];
    uint32_t path_size =2*MAXPATHLEN;

    result = CDarwinUtils::GetExecutablePath(given_path, &path_size);
    if (result == 0)
    {
      // Move backwards to last /.
      for (int n=strlen(given_path)-1; given_path[n] != '/'; n--)
        given_path[n] = '\0';

      #if defined(TARGET_DARWIN_IOS)
        strcat(given_path, "/AppData/AppHome/");
      #else
        // Assume local path inside application bundle.
        strcat(given_path, "../Resources/");
        strcat(given_path, CCompileInfo::GetAppName());
        strcat(given_path, "/");
      #endif

      // Convert to real path.
      char real_path[2*MAXPATHLEN];
      if (realpath(given_path, real_path) != NULL)
      {
        strPath = real_path;
        return;
      }
    }
#endif
    size_t last_sep = strHomePath.find_last_of(PATH_SEPARATOR_CHAR);
    if (last_sep != string::npos)
      strPath = strHomePath.substr(0, last_sep);
    else
      strPath = strHomePath;
  }
<<<<<<< HEAD
=======

#if defined(_LINUX) && !defined(__APPLE__)
  /* Change strPath accordingly when target is XBMC_HOME and when INSTALL_PATH
   * and BIN_INSTALL_PATH differ
   */
  CStdString installPath = INSTALL_PATH;
  CStdString binInstallPath = BIN_INSTALL_PATH;
  if (!strTarget.compare("XBMC_HOME") && installPath.compare(binInstallPath))
  {
    int pos = strPath.length() - binInstallPath.length();
    CStdString tmp = strPath;
    tmp.erase(0, pos);
    if (!tmp.compare(binInstallPath))
    {
      strPath.erase(pos, strPath.length());
      strPath.append(installPath);
    }
  }
#endif
}
>>>>>>> FETCH_HEAD

#if defined(TARGET_POSIX) && !defined(TARGET_DARWIN)
  /* Change strPath accordingly when target is KODI_HOME and when INSTALL_PATH
   * and BIN_INSTALL_PATH differ
   */
  std::string installPath = INSTALL_PATH;
  std::string binInstallPath = BIN_INSTALL_PATH;

  if (strTarget.empty() && installPath.compare(binInstallPath))
  {
    int pos = strPath.length() - binInstallPath.length();
    CStdString tmp = strPath;
    tmp.erase(0, pos);
    if (!tmp.compare(binInstallPath))
    {
      strPath.erase(pos, strPath.length());
      strPath.append(installPath);
    }
  }
#endif
}

bool CUtil::IsPVR(const CStdString& strFile)
{
  return StringUtils::StartsWithNoCase(strFile, "pvr:");
}

bool CUtil::IsHTSP(const CStdString& strFile)
{
  return StringUtils::StartsWithNoCase(strFile, "htsp:");
}

bool CUtil::IsLiveTV(const CStdString& strFile)
{
  if (StringUtils::StartsWithNoCase(strFile, "pvr://channels"))
    return true;

  if(URIUtils::IsTuxBox(strFile)
  || URIUtils::IsVTP(strFile)
  || URIUtils::IsHDHomeRun(strFile)
  || URIUtils::IsHTSP(strFile)
  || StringUtils::StartsWithNoCase(strFile, "sap:"))
    return true;

  if (URIUtils::IsMythTV(strFile) && CMythDirectory::IsLiveTV(strFile))
    return true;

  return false;
}

bool CUtil::IsTVRecording(const CStdString& strFile)
{
  return StringUtils::StartsWithNoCase(strFile, "pvr://recording");
}

bool CUtil::IsPicture(const CStdString& strFile)
{
  return URIUtils::HasExtension(strFile,
                  g_advancedSettings.m_pictureExtensions + "|.tbn|.dds");
}

<<<<<<< HEAD
bool CUtil::ExcludeFileOrFolder(const CStdString& strFileOrFolder, const vector<string>& regexps)
{
  if (strFileOrFolder.empty())
=======
  if(IsPlugin(strPath))
    return false;

  CStdString host = url.GetHostName();
  if(host.length() == 0)
>>>>>>> FETCH_HEAD
    return false;

  CRegExp regExExcludes(true, CRegExp::autoUtf8);  // case insensitive regex

<<<<<<< HEAD
=======
  if(address != INADDR_NONE)
  {
    // check if we are on the local subnet
    if (!g_application.getNetwork().GetFirstConnectedInterface())
      return false;

    if (g_application.getNetwork().HasInterfaceForIP(address))
      return true;
  }

  return false;
}

bool CUtil::IsMultiPath(const CStdString& strPath)
{
  return strPath.Left(10).Equals("multipath:");
}

bool CUtil::IsHD(const CStdString& strFileName)
{
  CURL url(strFileName);

  if (IsSpecial(strFileName))
    return IsHD(CSpecialProtocol::TranslatePath(strFileName));

  if(IsStack(strFileName))
    return IsHD(CStackDirectory::GetFirstStackedFile(strFileName));

  if (IsInArchive(strFileName))
    return IsHD(url.GetHostName());

  return url.IsLocal();
}

bool CUtil::IsDVD(const CStdString& strFile)
{
#if defined(_WIN32)
  if(strFile.Mid(1) != ":\\"
  && strFile.Mid(1) != ":")
    return false;

  if((GetDriveType(strFile.c_str()) == DRIVE_CDROM) || strFile.Left(6).Equals("dvd://"))
    return true;
#else
  CStdString strFileLow = strFile;
  strFileLow.MakeLower();
  if (strFileLow == "d:/"  || strFileLow == "d:\\"  || strFileLow == "d:" || strFileLow == "iso9660://" || strFileLow == "udf://" || strFileLow == "dvd://1" )
    return true;
#endif

  return false;
}

bool CUtil::IsVirtualPath(const CStdString& strFile)
{
  return strFile.Left(12).Equals("virtualpath:");
}

bool CUtil::IsStack(const CStdString& strFile)
{
  return strFile.Left(6).Equals("stack:");
}

bool CUtil::IsRAR(const CStdString& strFile)
{
  CStdString strExtension;
  CUtil::GetExtension(strFile,strExtension);

  if (strExtension.Equals(".001") && strFile.Mid(strFile.length()-7,7).CompareNoCase(".ts.001"))
    return true;

  if (strExtension.CompareNoCase(".cbr") == 0)
    return true;

  if (strExtension.CompareNoCase(".rar") == 0)
    return true;

  return false;
}

bool CUtil::IsInArchive(const CStdString &strFile)
{
  return IsInZIP(strFile) || IsInRAR(strFile);
}

bool CUtil::IsInZIP(const CStdString& strFile)
{
  CURL url(strFile);

  return url.GetProtocol() == "zip" && url.GetFileName() != "";
}

bool CUtil::IsInRAR(const CStdString& strFile)
{
  CURL url(strFile);

  return url.GetProtocol() == "rar" && url.GetFileName() != "";
}

bool CUtil::IsZIP(const CStdString& strFile) // also checks for comic books!
{
  CStdString strExtension;
  CUtil::GetExtension(strFile,strExtension);

  if (strExtension.CompareNoCase(".zip") == 0)
    return true;

  if (strExtension.CompareNoCase(".cbz") == 0)
    return true;

  return false;
}

bool CUtil::IsSpecial(const CStdString& strFile)
{
  CStdString strFile2(strFile);

  if (IsStack(strFile))
    strFile2 = CStackDirectory::GetFirstStackedFile(strFile);

  return strFile2.Left(8).Equals("special:");
}

bool CUtil::IsPlugin(const CStdString& strFile)
{
  CURL url(strFile);
  return url.GetProtocol().Equals("plugin");
}

bool CUtil::IsAddonsPath(const CStdString& strFile)
{
  CURL url(strFile);
  return url.GetProtocol().Equals("addons");
}

bool CUtil::IsCDDA(const CStdString& strFile)
{
  return strFile.Left(5).Equals("cdda:");
}

bool CUtil::IsISO9660(const CStdString& strFile)
{
  return strFile.Left(8).Equals("iso9660:");
}

bool CUtil::IsSmb(const CStdString& strFile)
{
  CStdString strFile2(strFile);

  if (IsStack(strFile))
    strFile2 = CStackDirectory::GetFirstStackedFile(strFile);

  return strFile2.Left(4).Equals("smb:");
}

bool CUtil::IsURL(const CStdString& strFile)
{
  return strFile.Find("://") >= 0;
}

bool CUtil::IsXBMS(const CStdString& strFile)
{
  CStdString strFile2(strFile);

  if (IsStack(strFile))
    strFile2 = CStackDirectory::GetFirstStackedFile(strFile);

  return strFile2.Left(5).Equals("xbms:");
}

bool CUtil::IsFTP(const CStdString& strFile)
{
  CStdString strFile2(strFile);

  if (IsStack(strFile))
    strFile2 = CStackDirectory::GetFirstStackedFile(strFile);

  CURL url(strFile2);

  return url.GetTranslatedProtocol() == "ftp"  ||
         url.GetTranslatedProtocol() == "ftps";
}

bool CUtil::IsInternetStream(const CStdString& strFile, bool bStrictCheck /* = false */)
{
  CURL url(strFile);
  CStdString strProtocol = url.GetProtocol();
  
  if (strProtocol.IsEmpty())
    return false;

  // there's nothing to stop internet streams from being stacked
  if (strProtocol == "stack")
  {
    CStdString strFile2 = CStackDirectory::GetFirstStackedFile(strFile);
    return IsInternetStream(strFile2);
  }

  CStdString strProtocol2 = url.GetTranslatedProtocol();

  // Special case these
  if (strProtocol2 == "ftp" || strProtocol2 == "ftps" ||
      strProtocol  == "dav" || strProtocol  == "davs")
    return bStrictCheck;

  if (strProtocol2 == "http" || strProtocol2 == "https" ||
      strProtocol  == "rtp"  || strProtocol  == "udp"   ||
      strProtocol  == "rtmp" || strProtocol  == "rtsp")
    return true;

  return false;
}

bool CUtil::IsDAAP(const CStdString& strFile)
{
  return strFile.Left(5).Equals("daap:");
}

bool CUtil::IsUPnP(const CStdString& strFile)
{
  return strFile.Left(5).Equals("upnp:");
}

bool CUtil::IsTuxBox(const CStdString& strFile)
{
  return strFile.Left(7).Equals("tuxbox:");
}

bool CUtil::IsMythTV(const CStdString& strFile)
{
  return strFile.Left(5).Equals("myth:");
}

bool CUtil::IsHDHomeRun(const CStdString& strFile)
{
  return strFile.Left(10).Equals("hdhomerun:");
}

bool CUtil::IsVTP(const CStdString& strFile)
{
  return strFile.Left(4).Equals("vtp:");
}

bool CUtil::IsHTSP(const CStdString& strFile)
{
  return strFile.Left(5).Equals("htsp:");
}

bool CUtil::IsLiveTV(const CStdString& strFile)
{
  if(IsTuxBox(strFile)
  || IsVTP(strFile)
  || IsHDHomeRun(strFile)
  || IsHTSP(strFile)
  || strFile.Left(4).Equals("sap:"))
    return true;

  if (IsMythTV(strFile) && CMythDirectory::IsLiveTV(strFile))
    return true;

  return false;
}

bool CUtil::IsMusicDb(const CStdString& strFile)
{
  return strFile.Left(8).Equals("musicdb:");
}

bool CUtil::IsVideoDb(const CStdString& strFile)
{
  return strFile.Left(8).Equals("videodb:");
}

bool CUtil::IsLastFM(const CStdString& strFile)
{
  return strFile.Left(7).Equals("lastfm:");
}

bool CUtil::IsWritable(const CStdString& strFile)
{
  return ( IsHD(strFile) || IsSmb(strFile) ) && !IsDVD(strFile);
}

bool CUtil::IsPicture(const CStdString& strFile)
{
  CStdString extension = GetExtension(strFile);

  if (extension.IsEmpty())
    return false;

  extension.ToLower();
  if (g_settings.m_pictureExtensions.Find(extension) != -1)
    return true;

  if (extension == ".tbn" || extension == ".dds")
    return true;

  return false;
}

bool CUtil::ExcludeFileOrFolder(const CStdString& strFileOrFolder, const CStdStringArray& regexps)
{
  if (strFileOrFolder.IsEmpty())
    return false;

  CRegExp regExExcludes(true);  // case insensitive regex

>>>>>>> FETCH_HEAD
  for (unsigned int i = 0; i < regexps.size(); i++)
  {
    if (!regExExcludes.RegComp(regexps[i].c_str()))
    { // invalid regexp - complain in logs
      CLog::Log(LOGERROR, "%s: Invalid exclude RegExp:'%s'", __FUNCTION__, regexps[i].c_str());
      continue;
    }
    if (regExExcludes.RegFind(strFileOrFolder) > -1)
    {
      CLog::Log(LOGDEBUG, "%s: File '%s' excluded. (Matches exclude rule RegExp:'%s')", __FUNCTION__, strFileOrFolder.c_str(), regexps[i].c_str());
      return true;
    }
  }
  return false;
}

void CUtil::GetFileAndProtocol(const CStdString& strURL, CStdString& strDir)
{
  strDir = strURL;
  if (!URIUtils::IsRemote(strURL)) return ;
  if (URIUtils::IsDVD(strURL)) return ;

  CURL url(strURL);
  strDir = StringUtils::Format("%s://%s", url.GetProtocol().c_str(), url.GetFileName().c_str());
}

int CUtil::GetDVDIfoTitle(const CStdString& strFile)
{
  CStdString strFilename = URIUtils::GetFileName(strFile);
  if (strFilename.Equals("video_ts.ifo")) return 0;
  //VTS_[TITLE]_0.IFO
  return atoi(strFilename.substr(4, 2).c_str());
}

CStdString CUtil::GetFileMD5(const CStdString& strPath)
{
  CFile file;
  CStdString result;
  if (file.Open(strPath))
  {
    XBMC::XBMC_MD5 md5;
    char temp[1024];
    while (true)
    {
      ssize_t read = file.Read(temp,1024);
      if (read <= 0)
        break;
      md5.append(temp,read);
    }
    result = md5.getDigest();
    file.Close();
  }

  return result;
}

CStdString CUtil::GetFileMD5(const CStdString& strPath)
{
  CFile file;
  CStdString result;
  if (file.Open(strPath))
  {
    XBMC::XBMC_MD5 md5;
    char temp[1024];
    int pos=0;
    int read=1;
    while (read > 0)
    {
      read = file.Read(temp,1024);
      pos += read;
      md5.append(temp,read);
    }
    md5.getDigest(result);
    file.Close();
  }

  return result;
}

bool CUtil::GetDirectoryName(const CStdString& strFileName, CStdString& strDescription)
{
  CStdString strFName = URIUtils::GetFileName(strFileName);
  strDescription = URIUtils::GetDirectory(strFileName);
  URIUtils::RemoveSlashAtEnd(strDescription);

  size_t iPos = strDescription.find_last_of("/\\");
  if (iPos != std::string::npos)
    strDescription = strDescription.substr(iPos + 1);
  else if (strDescription.size() <= 0)
    strDescription = strFName;
  return true;
}

void CUtil::GetDVDDriveIcon(const std::string& strPath, std::string& strIcon)
{
  if ( !g_mediaManager.IsDiscInDrive() )
  {
    strIcon = "DefaultDVDEmpty.png";
    return ;
  }

  if ( URIUtils::IsDVD(strPath) )
  {
    strIcon = "DefaultDVDRom.png";
    return ;
  }

  if ( URIUtils::IsISO9660(strPath) )
  {
#ifdef HAS_DVD_DRIVE
    CCdInfo* pInfo = g_mediaManager.GetCdInfo();
    if ( pInfo != NULL && pInfo->IsVideoCd( 1 ) )
    {
      strIcon = "DefaultVCD.png";
      return ;
    }
#endif
    strIcon = "DefaultDVDRom.png";
    return ;
  }

  if ( URIUtils::IsCDDA(strPath) )
  {
    strIcon = "DefaultCDDA.png";
    return ;
  }
}

void CUtil::RemoveTempFiles()
{
<<<<<<< HEAD
  CStdString searchPath = CProfilesManager::Get().GetDatabaseFolder();
  CFileItemList items;
  if (!XFILE::CDirectory::GetDirectory(searchPath, items, ".tmp", DIR_FLAG_NO_FILE_DIRS))
=======
  CStdString searchPath = g_settings.GetDatabaseFolder();
  CFileItemList items;
  if (!XFILE::CDirectory::GetDirectory(searchPath, items, ".tmp", false))
>>>>>>> FETCH_HEAD
    return;

  for (int i = 0; i < items.Size(); ++i)
  {
    if (items[i]->m_bIsFolder)
      continue;
<<<<<<< HEAD
    XFILE::CFile::Delete(items[i]->GetPath());
=======
    XFILE::CFile::Delete(items[i]->m_strPath);
>>>>>>> FETCH_HEAD
  }
}

void CUtil::ClearSubtitles()
{
  //delete cached subs
  CFileItemList items;
  CDirectory::GetDirectory("special://temp/",items);
  for( int i=0;i<items.Size();++i)
  {
    if (!items[i]->m_bIsFolder)
    {
      if (items[i]->GetPath().find("subtitle") != std::string::npos ||
          items[i]->GetPath().find("vobsub_queue") != std::string::npos)
      {
        CLog::Log(LOGDEBUG, "%s - Deleting temporary subtitle %s", __FUNCTION__, items[i]->GetPath().c_str());
        CFile::Delete(items[i]->GetPath());
      }
    }
  }
}

void CUtil::ClearTempFonts()
{
  CStdString searchPath = "special://temp/fonts/";

  if (!CDirectory::Exists(searchPath))
    return;

  CFileItemList items;
  CDirectory::GetDirectory(searchPath, items, "", DIR_FLAG_NO_FILE_DIRS | DIR_FLAG_BYPASS_CACHE);

  for (int i=0; i<items.Size(); ++i)
  {
    if (items[i]->m_bIsFolder)
      continue;
    CFile::Delete(items[i]->GetPath());
  }
}

static const char * sub_exts[] = { ".utf", ".utf8", ".utf-8", ".sub", ".srt", ".smi", ".rt", ".txt", ".ssa", ".aqt", ".jss", ".ass", ".idx", NULL};

int64_t CUtil::ToInt64(uint32_t high, uint32_t low)
{
  int64_t n;
  n = high;
  n <<= 32;
  n += low;
  return n;
}

CStdString CUtil::GetNextFilename(const CStdString &fn_template, int max)
{
  if (fn_template.find("%03d") == std::string::npos)
    return "";

  CStdString searchPath = URIUtils::GetDirectory(fn_template);
  CStdString mask = URIUtils::GetExtension(fn_template);
  CStdString name = StringUtils::Format(fn_template.c_str(), 0);

  CFileItemList items;
  if (!CDirectory::GetDirectory(searchPath, items, mask, DIR_FLAG_NO_FILE_DIRS))
    return name;

  items.SetFastLookup(true);
  for (int i = 0; i <= max; i++)
  {
    CStdString name = StringUtils::Format(fn_template.c_str(), i);
    if (!items.Get(name))
      return name;
  }
  return "";
}

CStdString CUtil::GetNextPathname(const CStdString &path_template, int max)
{
  if (path_template.find("%04d") == std::string::npos)
    return "";
  
  for (int i = 0; i <= max; i++)
  {
<<<<<<< HEAD
    CStdString name = StringUtils::Format(path_template.c_str(), i);
    if (!CFile::Exists(name) && !CDirectory::Exists(name))
      return name;
=======
    if (token[token.size()-1].size() == 3 && token[token.size()-1].Mid(0,2).Equals("cd"))
    {
      CStdString strPath2;
      GetParentPath(strPath,strPath2);
      strLookInPaths.push_back(strPath2);
    }
  }
  int iSize = strLookInPaths.size();
  for (int i=0;i<iSize;++i)
  {
    for (int j=0; common_sub_dirs[j]; j++)
    {
      CStdString strPath2;
      CUtil::AddFileToFolder(strLookInPaths[i],common_sub_dirs[j],strPath2);
      if (CDirectory::Exists(strPath2))
        strLookInPaths.push_back(strPath2);
    }
  }
  // .. done checking for common subdirs

  // check if there any cd-directories in the paths we have added so far
  char temp[6];
  iSize = strLookInPaths.size();
  for (int i=0;i<9;++i) // 9 cd's
  {
    sprintf(temp,"cd%i",i+1);
    for (int i=0;i<iSize;++i)
    {
      CStdString strPath2;
      CUtil::AddFileToFolder(strLookInPaths[i],temp,strPath2);
      if (CDirectory::Exists(strPath2))
        strLookInPaths.push_back(strPath2);
    }
  }
  // .. done checking for cd-dirs

  // this is last because we dont want to check any common subdirs or cd-dirs in the alternate <subtitles> dir.
  if (g_settings.iAdditionalSubtitleDirectoryChecked == 1)
  {
    strPath = g_guiSettings.GetString("subtitles.custompath");
    if (!HasSlashAtEnd(strPath))
      strPath += "/"; //Should work for both remote and local files
    strLookInPaths.push_back(strPath);
  }

  unsigned int nextTimer = CTimeUtils::GetTimeMS();
  CLog::Log(LOGDEBUG,"%s: Done (time: %i ms)", __FUNCTION__, (int)(nextTimer - startTimer));

  CStdString strLExt;
  CStdString strDest;
  CStdString strItem;

  // 2 steps for movie directory and alternate subtitles directory
  CLog::Log(LOGDEBUG,"%s: Searching for subtitles...", __FUNCTION__);
  for (unsigned int step = 0; step < strLookInPaths.size(); step++)
  {
    if (strLookInPaths[step].length() != 0)
    {
      CFileItemList items;

      CDirectory::GetDirectory(strLookInPaths[step], items,".utf|.utf8|.utf-8|.sub|.srt|.smi|.rt|.txt|.ssa|.text|.ssa|.aqt|.jss|.ass|.idx|.ifo|.rar|.zip",false);
      int fnl = strFileNameNoExt.size();

      CStdString strFileNameNoExtNoCase(strFileNameNoExt);
      strFileNameNoExtNoCase.MakeLower();
      for (int j = 0; j < (int)items.Size(); j++)
      {
        Split(items[j]->m_strPath, strPath, strItem);

        // is this a rar-file ..
        if ((CUtil::IsRAR(strItem) || CUtil::IsZIP(strItem)))
        {
          CStdString strRar, strItemWithPath;
          CUtil::AddFileToFolder(strLookInPaths[step],strFileNameNoExt+CUtil::GetExtension(strItem),strRar);
          CUtil::AddFileToFolder(strLookInPaths[step],strItem,strItemWithPath);

          unsigned int iPos = strMovie.substr(0,6)=="rar://"?1:0;
          iPos = strMovie.substr(0,6)=="zip://"?1:0;
          if ((step != iPos) || (strFileNameNoExtNoCase+".rar").Equals(strItem) || (strFileNameNoExtNoCase+".zip").Equals(strItem))
            CacheRarSubtitles(items[j]->m_strPath, strFileNameNoExtNoCase);
        }
        else
        {
          for (int i = 0; sub_exts[i]; i++)
          {
            int l = strlen(sub_exts[i]);

            //Cache any alternate subtitles.
            if (strItem.Left(9).ToLower() == "subtitle." && strItem.Right(l).ToLower() == sub_exts[i])
            {
              strLExt = strItem.Right(strItem.GetLength() - 9);
              strDest.Format("special://temp/subtitle.alt-%s", strLExt);
              if (CFile::Cache(items[j]->m_strPath, strDest, pCallback, NULL))
              {
                CLog::Log(LOGINFO, " cached subtitle %s->%s\n", strItem.c_str(), strDest.c_str());
                strExtensionCached = strLExt;
              }
            }

            //Cache subtitle with same name as movie
            if (strItem.Right(l).ToLower() == sub_exts[i] && strItem.Left(fnl).ToLower() == strFileNameNoExt.ToLower())
            {
              strLExt = strItem.Right(strItem.size() - fnl);
              strDest.Format("special://temp/subtitle%s", strLExt);
              if (CFile::Cache(items[j]->m_strPath, strDest, pCallback, NULL))
                CLog::Log(LOGINFO, " cached subtitle %s->%s\n", strItem.c_str(), strDest.c_str());
            }
          }
        }
      }
      g_directoryCache.ClearDirectory(strLookInPaths[step]);
    }
  }
  CLog::Log(LOGDEBUG,"%s: Done (time: %i ms)", __FUNCTION__, (int)(CTimeUtils::GetTimeMS() - nextTimer));

  // build the vector with extensions
  CFileItemList items;
  CDirectory::GetDirectory("special://temp/", items,".utf|.utf8|.utf-8|.sub|.srt|.smi|.rt|.txt|.ssa|.text|.ssa|.aqt|.jss|.ass|.idx|.ifo|.rar|.zip",false);
  for (int i=0;i<items.Size();++i)
  {
    if (items[i]->m_bIsFolder)
      continue;

    CStdString filename = GetFileName(items[i]->m_strPath);
    strLExt = filename.Right(filename.size()-8);
    if (filename.Left(8).Equals("subtitle"))
      vecExtensionsCached.push_back(strLExt);
    if (CUtil::GetExtension(filename).Equals(".smi"))
    {
      //Cache multi-language sami subtitle
      CDVDSubtitleStream* pStream = new CDVDSubtitleStream();
      if(pStream->Open(items[i]->m_strPath))
      {
        SamiTagConvertor TagConv;
        TagConv.LoadHead(pStream);
        if (TagConv.m_Langclass.size() >= 2)
        {
          for (unsigned int k = 0; k < TagConv.m_Langclass.size(); k++)
          {
            strDest.Format("special://temp/subtitle.%s%s", TagConv.m_Langclass[k].Name, strLExt);
            if (CFile::Cache(items[i]->m_strPath, strDest, pCallback, NULL))
              CLog::Log(LOGINFO, " cached subtitle %s->%s\n", filename.c_str(), strDest.c_str());
            CStdString strTemp;
            strTemp.Format(".%s%s", TagConv.m_Langclass[k].Name, strLExt);
            vecExtensionsCached.push_back(strTemp);
          }
        }
      }
      delete pStream;
    }
  }

  // construct string of added exts
  for (vector<CStdString>::iterator it=vecExtensionsCached.begin(); it != vecExtensionsCached.end(); ++it)
    strExtensionCached += *it+"|";

  CLog::Log(LOGDEBUG,"%s: END (total time: %i ms)", __FUNCTION__, (int)(CTimeUtils::GetTimeMS() - startTimer));
}

bool CUtil::CacheRarSubtitles(const CStdString& strRarPath, 
                              const CStdString& strCompare)
{
  bool bFoundSubs = false;
  CFileItemList ItemList;

  // zip only gets the root dir
  if (CUtil::GetExtension(strRarPath).Equals(".zip"))
  {
    CStdString strZipPath;
    CUtil::CreateArchivePath(strZipPath,"zip",strRarPath,"");
    if (!CDirectory::GetDirectory(strZipPath,ItemList,"",false))
      return false;
  }
  else
  {
#ifdef HAS_FILESYSTEM_RAR
    // get _ALL_files in the rar, even those located in subdirectories because we set the bMask to false.
    // so now we dont have to find any subdirs anymore, all files in the rar is checked.
    if( !g_RarManager.GetFilesInRar(ItemList, strRarPath, false, "") )
      return false;
#else
      return false;
#endif
  }
  for (int it= 0 ; it <ItemList.Size();++it)
  {
    CStdString strPathInRar = ItemList[it]->m_strPath;
    CStdString strExt = CUtil::GetExtension(strPathInRar);

    CLog::Log(LOGDEBUG, "CacheRarSubs:: Found file %s", strPathInRar.c_str());
    // always check any embedded rar archives
    // checking for embedded rars, I moved this outside the sub_ext[] loop. We only need to check this once for each file.
    if (CUtil::IsRAR(strPathInRar) || CUtil::IsZIP(strPathInRar))
    {
      CStdString strRarInRar;
      if (CUtil::GetExtension(strPathInRar).Equals(".rar"))
        CUtil::CreateArchivePath(strRarInRar, "rar", strRarPath, strPathInRar);
      else
        CUtil::CreateArchivePath(strRarInRar, "zip", strRarPath, strPathInRar);
      CacheRarSubtitles(strRarInRar,strCompare);
    }
    // done checking if this is a rar-in-rar

    int iPos=0;
    CStdString strFileName = CUtil::GetFileName(strPathInRar);
    CStdString strFileNameNoCase(strFileName);
    strFileNameNoCase.MakeLower();
    if (strFileNameNoCase.Find(strCompare) >= 0)
      while (sub_exts[iPos])
      {
        if (strExt.CompareNoCase(sub_exts[iPos]) == 0)
        {
          CStdString strSourceUrl;
          if (CUtil::GetExtension(strRarPath).Equals(".rar"))
            CUtil::CreateArchivePath(strSourceUrl, "rar", strRarPath, strPathInRar);
          else
            strSourceUrl = strPathInRar;

          CStdString strDestFile;
          strDestFile.Format("special://temp/subtitle%s", sub_exts[iPos]);

          if (CFile::Cache(strSourceUrl,strDestFile))
          {
            CLog::Log(LOGINFO, " cached subtitle %s->%s", strPathInRar.c_str(), strDestFile.c_str());
            bFoundSubs = true;
            break;
          }
        }

        iPos++;
      }
  }
  return bFoundSubs;
}

int64_t CUtil::ToInt64(uint32_t high, uint32_t low)
{
  int64_t n;
  n = high;
  n <<= 32;
  n += low;
  return n;
}

bool CUtil::IsDOSPath(const CStdString &path)
{
  if (path.size() > 1 && path[1] == ':' && isalpha(path[0]))
    return true;

  return false;
}

void CUtil::AddFileToFolder(const CStdString& strFolder, const CStdString& strFile, CStdString& strResult)
{
  if (IsURL(strFolder))
  {
    CURL url(strFolder);
    if (url.GetFileName() != strFolder)
    {
      AddFileToFolder(url.GetFileName(), strFile, strResult);
      url.SetFileName(strResult);
      strResult = url.Get();
      return;
    }
  }

  strResult = strFolder;
  if(!strResult.IsEmpty())
    AddSlashAtEnd(strResult);

  // Remove any slash at the start of the file
  if (strFile.size() && (strFile[0] == '/' || strFile[0] == '\\'))
    strResult += strFile.Mid(1);
  else
    strResult += strFile;

  // correct any slash directions
  if (!IsDOSPath(strFolder))
    strResult.Replace('\\', '/');
  else
    strResult.Replace('/', '\\');
}

void CUtil::AddSlashAtEnd(CStdString& strFolder)
{
  if (IsURL(strFolder))
  {
    CURL url(strFolder);
    CStdString file = url.GetFileName();
    if(!file.IsEmpty() && file != strFolder)
    {
      AddSlashAtEnd(file);
      url.SetFileName(file);
      strFolder = url.Get();
      return;
    }
  }

  if (!CUtil::HasSlashAtEnd(strFolder))
  {
    if (IsDOSPath(strFolder))
      strFolder += '\\';
    else
      strFolder += '/';
  }
}

void CUtil::RemoveSlashAtEnd(CStdString& strFolder)
{
  if (IsURL(strFolder))
  {
    CURL url(strFolder);
    CStdString file = url.GetFileName();
    if (!file.IsEmpty() && file != strFolder)
    {
      RemoveSlashAtEnd(file);
      url.SetFileName(file);
      strFolder = url.Get();
      return;
    }
    if(url.GetHostName().IsEmpty())
      return;
  }

  while (CUtil::HasSlashAtEnd(strFolder))
    strFolder.Delete(strFolder.size() - 1);
}

void CUtil::GetDirectory(const CStdString& strFilePath, CStdString& strDirectoryPath)
{
  // Will from a full filename return the directory the file resides in.
  // Keeps the final slash at end

  int iPos1 = strFilePath.ReverseFind('/');
  int iPos2 = strFilePath.ReverseFind('\\');

  if (iPos2 > iPos1)
  {
    iPos1 = iPos2;
  }

  if (iPos1 > 0)
  {
    strDirectoryPath = strFilePath.Left(iPos1 + 1); // include the slash
  }
}

void CUtil::Split(const CStdString& strFileNameAndPath, CStdString& strPath, CStdString& strFileName)
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

  strPath = strFileNameAndPath.Left(i + 1);
  strFileName = strFileNameAndPath.Right(strFileNameAndPath.size() - i - 1);
}

void CUtil::CreateArchivePath(CStdString& strUrlPath, const CStdString& strType,
                              const CStdString& strArchivePath,
                              const CStdString& strFilePathInArchive,
                              const CStdString& strPwd)
{
  CStdString strBuffer;

  strUrlPath = strType+"://";

  if( !strPwd.IsEmpty() )
  {
    strBuffer = strPwd;
    CUtil::URLEncode(strBuffer);
    strUrlPath += strBuffer;
    strUrlPath += "@";
  }

  strBuffer = strArchivePath;
  CUtil::URLEncode(strBuffer);

  strUrlPath += strBuffer;

  strBuffer = strFilePathInArchive;
  strBuffer.Replace('\\', '/');
  strBuffer.TrimLeft('/');

  strUrlPath += "/";
  strUrlPath += strBuffer;

#if 0 // options are not used
  strBuffer = strCachePath;
  CUtil::URLEncode(strBuffer);

  strUrlPath += "?cache=";
  strUrlPath += strBuffer;

  strBuffer.Format("%i", wOptions);
  strUrlPath += "&flags=";
  strUrlPath += strBuffer;
#endif
}

bool CUtil::ThumbExists(const CStdString& strFileName, bool bAddCache)
{
  return CThumbnailCache::GetThumbnailCache()->ThumbExists(strFileName, bAddCache);
}

void CUtil::ThumbCacheAdd(const CStdString& strFileName, bool bFileExists)
{
  CThumbnailCache::GetThumbnailCache()->Add(strFileName, bFileExists);
}

void CUtil::ThumbCacheClear()
{
  CThumbnailCache::GetThumbnailCache()->Clear();
}

bool CUtil::ThumbCached(const CStdString& strFileName)
{
  return CThumbnailCache::GetThumbnailCache()->IsCached(strFileName);
}

void CUtil::PlayDVD(const CStdString& strProtocol)
{
#ifdef HAS_DVDPLAYER
  CIoSupport::Dismount("Cdrom0");
  CIoSupport::RemapDriveLetter('D', "Cdrom0");
  CStdString strPath;
  strPath.Format("%s://1", strProtocol.c_str());
  CFileItem item(strPath, false);
  item.SetLabel(g_mediaManager.GetDiskLabel());
  g_application.PlayFile(item);
#endif
}

CStdString CUtil::GetNextFilename(const CStdString &fn_template, int max)
{
  if (!fn_template.Find("%03d"))
    return "";

  CStdString searchPath;
  CUtil::GetDirectory(fn_template, searchPath);
  CStdString mask = CUtil::GetExtension(fn_template);

  CStdString name;
  name.Format(fn_template.c_str(), 0);

  CFileItemList items;
  if (!CDirectory::GetDirectory(searchPath, items, mask, false))
    return name;

  items.SetFastLookup(true);
  for (int i = 0; i <= max; i++)
  {
    CStdString name;
    name.Format(fn_template.c_str(), i);
    if (!items.Get(name))
      return name;
  }
  return "";
}

CStdString CUtil::GetNextPathname(const CStdString &path_template, int max)
{
  if (!path_template.Find("%04d"))
    return "";
  
  for (int i = 0; i <= max; i++)
  {
    CStdString name;
    name.Format(path_template.c_str(), i);
    if (!CFile::Exists(name))
      return name;
  }
  return "";
}

void CUtil::Tokenize(const CStdString& path, vector<CStdString>& tokens, const string& delimiters)
{
  // Tokenize ripped from http://www.linuxselfhelp.com/HOWTO/C++Programming-HOWTO-7.html
  // Skip delimiters at beginning.
  string::size_type lastPos = path.find_first_not_of(delimiters, 0);
  // Find first "non-delimiter".
  string::size_type pos = path.find_first_of(delimiters, lastPos);

  while (string::npos != pos || string::npos != lastPos)
  {
    // Found a token, add it to the vector.
    tokens.push_back(path.substr(lastPos, pos - lastPos));
    // Skip delimiters.  Note the "not_of"
    lastPos = path.find_first_not_of(delimiters, pos);
    // Find next "non-delimiter"
    pos = path.find_first_of(delimiters, lastPos);
  }
}

void CUtil::TakeScreenshot(const CStdString &filename, bool sync)
{
  int            width;
  int            height;
  int            stride;
  unsigned char* outpixels = NULL;

#ifdef HAS_DX
  LPDIRECT3DSURFACE9 lpSurface = NULL, lpBackbuffer = NULL;
  g_graphicsContext.Lock();
  if (g_application.IsPlayingVideo())
  {
#ifdef HAS_VIDEO_PLAYBACK
    g_renderManager.SetupScreenshot();
#endif
  }
  g_application.RenderNoPresent();

  if (FAILED(g_Windowing.Get3DDevice()->CreateOffscreenPlainSurface(g_Windowing.GetWidth(), g_Windowing.GetHeight(), D3DFMT_X8R8G8B8, D3DPOOL_SYSTEMMEM, &lpSurface, NULL)))
    return;

  if (FAILED(g_Windowing.Get3DDevice()->GetRenderTarget(0, &lpBackbuffer)))
    return;

  // now take screenshot
  if (SUCCEEDED(g_Windowing.Get3DDevice()->GetRenderTargetData(lpBackbuffer, lpSurface)))
  {
    D3DLOCKED_RECT lr;
    D3DSURFACE_DESC desc;
    lpSurface->GetDesc(&desc);
    if (SUCCEEDED(lpSurface->LockRect(&lr, NULL, D3DLOCK_READONLY)))
    {
      width = desc.Width;
      height = desc.Height;
      stride = lr.Pitch;
      outpixels = new unsigned char[height * stride];
      memcpy(outpixels, lr.pBits, height * stride);
      lpSurface->UnlockRect();
    }
    else
    {
      CLog::Log(LOGERROR, "%s LockRect failed", __FUNCTION__);
    }
  }
  else
  {
    CLog::Log(LOGERROR, "%s GetBackBuffer failed", __FUNCTION__);
  }
  lpSurface->Release();
  lpBackbuffer->Release();

  g_graphicsContext.Unlock();

#elif defined(HAS_GL) || defined(HAS_GLES)

  g_graphicsContext.BeginPaint();
  if (g_application.IsPlayingVideo())
  {
#ifdef HAS_VIDEO_PLAYBACK
    g_renderManager.SetupScreenshot();
#endif
  }
  g_application.RenderNoPresent();
#ifndef HAS_GLES
  glReadBuffer(GL_BACK);
#endif
  //get current viewport
  GLint viewport[4];
  glGetIntegerv(GL_VIEWPORT, viewport);

  width  = viewport[2] - viewport[0];
  height = viewport[3] - viewport[1];
  stride = width * 4;
  unsigned char* pixels = new unsigned char[stride * height];

  //read pixels from the backbuffer
#if HAS_GLES == 2
  glReadPixels(viewport[0], viewport[1], viewport[2], viewport[3], GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)pixels);
#else
  glReadPixels(viewport[0], viewport[1], viewport[2], viewport[3], GL_BGRA, GL_UNSIGNED_BYTE, (GLvoid*)pixels);
#endif
  g_graphicsContext.EndPaint();

  //make a new buffer and copy the read image to it with the Y axis inverted
  outpixels = new unsigned char[stride * height];
  for (int y = 0; y < height; y++)
    memcpy(outpixels + y * stride, pixels + (height - y - 1) * stride, stride);

  delete [] pixels;

#else
  //nothing to take a screenshot from
  return;
#endif

  if (!outpixels)
  {
    CLog::Log(LOGERROR, "Screenshot %s failed", filename.c_str());
    return;
  }

  CLog::Log(LOGDEBUG, "Saving screenshot %s", filename.c_str());

  //set alpha byte to 0xFF
  for (int y = 0; y < height; y++)
  {
    unsigned char* alphaptr = outpixels - 1 + y * stride;
    for (int x = 0; x < width; x++)
      *(alphaptr += 4) = 0xFF;
  }

  //if sync is true, the png file needs to be completely written when this function returns
  if (sync)
  {
    if (!CPicture::CreateThumbnailFromSurface(outpixels, width, height, stride, filename))
      CLog::Log(LOGERROR, "Unable to write screenshot %s", filename.c_str());

    delete [] outpixels;
  }
  else
  {
    //make sure the file exists to avoid concurrency issues
    FILE* fp = fopen(filename.c_str(), "w");
    if (fp)
      fclose(fp);
    else
      CLog::Log(LOGERROR, "Unable to create file %s", filename.c_str());

    //write .png file asynchronous with CThumbnailWriter, prevents stalling of the render thread
    //outpixels is deleted from CThumbnailWriter
    CThumbnailWriter* thumbnailwriter = new CThumbnailWriter(outpixels, width, height, stride, filename);
    CJobManager::GetInstance().AddJob(thumbnailwriter, NULL);
  }
}

void CUtil::TakeScreenshot()
{
  static bool savingScreenshots = false;
  static vector<CStdString> screenShots;

  bool promptUser = false;
  // check to see if we have a screenshot folder yet
  CStdString strDir = g_guiSettings.GetString("debug.screenshotpath", false);
  if (strDir.IsEmpty())
  {
    strDir = "special://temp/";
    if (!savingScreenshots)
    {
      promptUser = true;
      savingScreenshots = true;
      screenShots.clear();
    }
  }
  CUtil::RemoveSlashAtEnd(strDir);

  if (!strDir.IsEmpty())
  {
    CStdString file = CUtil::GetNextFilename(CUtil::AddFileToFolder(strDir, "screenshot%03d.png"), 999);

    if (!file.IsEmpty())
    {
      TakeScreenshot(file, false);
      if (savingScreenshots)
        screenShots.push_back(file);
      if (promptUser)
      { // grab the real directory
        CStdString newDir = g_guiSettings.GetString("debug.screenshotpath");
        if (!newDir.IsEmpty())
        {
          for (unsigned int i = 0; i < screenShots.size(); i++)
          {
            CStdString file = CUtil::GetNextFilename(CUtil::AddFileToFolder(newDir, "screenshot%03d.png"), 999);
            CFile::Cache(screenShots[i], file);
          }
          screenShots.clear();
        }
        savingScreenshots = false;
      }
    }
    else
    {
      CLog::Log(LOGWARNING, "Too many screen shots or invalid folder");
    }
>>>>>>> FETCH_HEAD
  }
  return "";
}

void CUtil::StatToStatI64(struct _stati64 *result, struct stat *stat)
{
  result->st_dev = stat->st_dev;
  result->st_ino = stat->st_ino;
  result->st_mode = stat->st_mode;
  result->st_nlink = stat->st_nlink;
  result->st_uid = stat->st_uid;
  result->st_gid = stat->st_gid;
  result->st_rdev = stat->st_rdev;
  result->st_size = (int64_t)stat->st_size;

#ifndef TARGET_POSIX
  result->st_atime = (long)(stat->st_atime & 0xFFFFFFFF);
  result->st_mtime = (long)(stat->st_mtime & 0xFFFFFFFF);
  result->st_ctime = (long)(stat->st_ctime & 0xFFFFFFFF);
#else
  result->_st_atime = (long)(stat->st_atime & 0xFFFFFFFF);
  result->_st_mtime = (long)(stat->st_mtime & 0xFFFFFFFF);
  result->_st_ctime = (long)(stat->st_ctime & 0xFFFFFFFF);
#endif
}

void CUtil::Stat64ToStatI64(struct _stati64 *result, struct __stat64 *stat)
{
  result->st_dev = stat->st_dev;
  result->st_ino = stat->st_ino;
  result->st_mode = stat->st_mode;
  result->st_nlink = stat->st_nlink;
  result->st_uid = stat->st_uid;
  result->st_gid = stat->st_gid;
  result->st_rdev = stat->st_rdev;
  result->st_size = stat->st_size;
#ifndef TARGET_POSIX
  result->st_atime = (long)(stat->st_atime & 0xFFFFFFFF);
  result->st_mtime = (long)(stat->st_mtime & 0xFFFFFFFF);
  result->st_ctime = (long)(stat->st_ctime & 0xFFFFFFFF);
#else
  result->_st_atime = (long)(stat->st_atime & 0xFFFFFFFF);
  result->_st_mtime = (long)(stat->st_mtime & 0xFFFFFFFF);
  result->_st_ctime = (long)(stat->st_ctime & 0xFFFFFFFF);
#endif
}

void CUtil::StatI64ToStat64(struct __stat64 *result, struct _stati64 *stat)
{
  result->st_dev = stat->st_dev;
  result->st_ino = stat->st_ino;
  result->st_mode = stat->st_mode;
  result->st_nlink = stat->st_nlink;
  result->st_uid = stat->st_uid;
  result->st_gid = stat->st_gid;
  result->st_rdev = stat->st_rdev;
  result->st_size = stat->st_size;
#ifndef TARGET_POSIX
  result->st_atime = stat->st_atime;
  result->st_mtime = stat->st_mtime;
  result->st_ctime = stat->st_ctime;
#else
  result->st_atime = stat->_st_atime;
  result->st_mtime = stat->_st_mtime;
  result->st_ctime = stat->_st_ctime;
#endif
}

void CUtil::StatToStat64(struct __stat64 *result, const struct stat *stat)
{
  memset(result, 0, sizeof(*result));
  result->st_dev   = stat->st_dev;
  result->st_ino   = stat->st_ino;
  result->st_mode  = stat->st_mode;
  result->st_nlink = stat->st_nlink;
  result->st_uid   = stat->st_uid;
  result->st_gid   = stat->st_gid;
  result->st_rdev  = stat->st_rdev;
  result->st_size  = stat->st_size;
  result->st_atime = stat->st_atime;
  result->st_mtime = stat->st_mtime;
  result->st_ctime = stat->st_ctime;
}

void CUtil::Stat64ToStat(struct stat *result, struct __stat64 *stat)
{
  result->st_dev = stat->st_dev;
  result->st_ino = stat->st_ino;
  result->st_mode = stat->st_mode;
  result->st_nlink = stat->st_nlink;
  result->st_uid = stat->st_uid;
  result->st_gid = stat->st_gid;
  result->st_rdev = stat->st_rdev;
#ifndef TARGET_POSIX
  if (stat->st_size <= LONG_MAX)
    result->st_size = (_off_t)stat->st_size;
#else
  if (sizeof(stat->st_size) <= sizeof(result->st_size) )
    result->st_size = stat->st_size;
#endif
  else
  {
    result->st_size = 0;
    CLog::Log(LOGWARNING, "WARNING: File is larger than 32bit stat can handle, file size will be reported as 0 bytes");
  }
  result->st_atime = (time_t)(stat->st_atime & 0xFFFFFFFF);
  result->st_mtime = (time_t)(stat->st_mtime & 0xFFFFFFFF);
  result->st_ctime = (time_t)(stat->st_ctime & 0xFFFFFFFF);
}

#ifdef TARGET_WINDOWS
void CUtil::Stat64ToStat64i32(struct _stat64i32 *result, struct __stat64 *stat)
{
  result->st_dev = stat->st_dev;
  result->st_ino = stat->st_ino;
  result->st_mode = stat->st_mode;
  result->st_nlink = stat->st_nlink;
  result->st_uid = stat->st_uid;
  result->st_gid = stat->st_gid;
  result->st_rdev = stat->st_rdev;
#ifndef TARGET_POSIX
  if (stat->st_size <= LONG_MAX)
    result->st_size = (_off_t)stat->st_size;
#else
  if (sizeof(stat->st_size) <= sizeof(result->st_size) )
    result->st_size = stat->st_size;
#endif
  else
  {
    result->st_size = 0;
    CLog::Log(LOGWARNING, "WARNING: File is larger than 32bit stat can handle, file size will be reported as 0 bytes");
  }
#ifndef TARGET_POSIX
  result->st_atime = stat->st_atime;
  result->st_mtime = stat->st_mtime;
  result->st_ctime = stat->st_ctime;
#else
  result->st_atime = stat->_st_atime;
  result->st_mtime = stat->_st_mtime;
  result->st_ctime = stat->_st_ctime;
#endif
}
#endif

bool CUtil::CreateDirectoryEx(const CStdString& strPath)
{
  // Function to create all directories at once instead
  // of calling CreateDirectory for every subdir.
  // Creates the directory and subdirectories if needed.

  // return true if directory already exist
  if (CDirectory::Exists(strPath)) return true;

  // we currently only allow HD and smb, nfs and afp paths
  if (!URIUtils::IsHD(strPath) && !URIUtils::IsSmb(strPath) && !URIUtils::IsNfs(strPath) && !URIUtils::IsAfp(strPath))
  {
    CLog::Log(LOGERROR,"%s called with an unsupported path: %s", __FUNCTION__, strPath.c_str());
    return false;
  }

  vector<string> dirs = URIUtils::SplitPath(strPath);
  if (dirs.empty())
    return false;
  CStdString dir(dirs.front());
  URIUtils::AddSlashAtEnd(dir);
  for (vector<string>::const_iterator it = dirs.begin() + 1; it != dirs.end(); it ++)
  {
    dir = URIUtils::AddFileToFolder(dir, *it);
    CDirectory::Create(dir);
  }

  // was the final destination directory successfully created ?
  if (!CDirectory::Exists(strPath)) return false;
  return true;
}

CStdString CUtil::MakeLegalFileName(const CStdString &strFile, int LegalType)
{
  CStdString result = strFile;

  StringUtils::Replace(result, '/', '_');
  StringUtils::Replace(result, '\\', '_');
  StringUtils::Replace(result, '?', '_');

  if (LegalType == LEGAL_WIN32_COMPAT)
  {
    // just filter out some illegal characters on windows
    StringUtils::Replace(result, ':', '_');
    StringUtils::Replace(result, '*', '_');
    StringUtils::Replace(result, '?', '_');
    StringUtils::Replace(result, '\"', '_');
    StringUtils::Replace(result, '<', '_');
    StringUtils::Replace(result, '>', '_');
    StringUtils::Replace(result, '|', '_');
    StringUtils::TrimRight(result, ". ");
  }
  return result;
}

// legalize entire path
CStdString CUtil::MakeLegalPath(const CStdString &strPathAndFile, int LegalType)
{
  if (URIUtils::IsStack(strPathAndFile))
    return MakeLegalPath(CStackDirectory::GetFirstStackedFile(strPathAndFile));
  if (URIUtils::IsMultiPath(strPathAndFile))
    return MakeLegalPath(CMultiPathDirectory::GetFirstPath(strPathAndFile));
  if (!URIUtils::IsHD(strPathAndFile) && !URIUtils::IsSmb(strPathAndFile) && !URIUtils::IsNfs(strPathAndFile) && !URIUtils::IsAfp(strPathAndFile))
    return strPathAndFile; // we don't support writing anywhere except HD, SMB, NFS and AFP - no need to legalize path

  bool trailingSlash = URIUtils::HasSlashAtEnd(strPathAndFile);
  vector<string> dirs = URIUtils::SplitPath(strPathAndFile);
  if (dirs.empty())
    return strPathAndFile;
  // we just add first token to path and don't legalize it - possible values: 
  // "X:" (local win32), "" (local unix - empty string before '/') or
  // "protocol://domain"
  CStdString dir(dirs.front());
  URIUtils::AddSlashAtEnd(dir);
  for (vector<string>::const_iterator it = dirs.begin() + 1; it != dirs.end(); it ++)
    dir = URIUtils::AddFileToFolder(dir, MakeLegalFileName(*it, LegalType));
  if (trailingSlash) URIUtils::AddSlashAtEnd(dir);
  return dir;
}

CStdString CUtil::ValidatePath(const CStdString &path, bool bFixDoubleSlashes /* = false */)
{
  CStdString result = path;

  // Don't do any stuff on URLs containing %-characters or protocols that embed
  // filenames. NOTE: Don't use IsInZip or IsInRar here since it will infinitely
  // recurse and crash XBMC
  if (URIUtils::IsURL(path) && 
      (path.find('%') != std::string::npos ||
      StringUtils::StartsWithNoCase(path, "apk:") ||
      StringUtils::StartsWithNoCase(path, "zip:") ||
      StringUtils::StartsWithNoCase(path, "rar:") ||
      StringUtils::StartsWithNoCase(path, "stack:") ||
      StringUtils::StartsWithNoCase(path, "bluray:") ||
      StringUtils::StartsWithNoCase(path, "multipath:") ))
    return result;

  // check the path for incorrect slashes
#ifdef TARGET_WINDOWS
  if (URIUtils::IsDOSPath(path))
  {
    StringUtils::Replace(result, '/', '\\');
    /* The double slash correction should only be used when *absolutely*
       necessary! This applies to certain DLLs or use from Python DLLs/scripts
       that incorrectly generate double (back) slashes.
    */
    if (bFixDoubleSlashes && !result.empty())
    {
      // Fixup for double back slashes (but ignore the \\ of unc-paths)
      for (size_t x = 1; x < result.size() - 1; x++)
      {
        if (result[x] == '\\' && result[x+1] == '\\')
          result.erase(x);
      }
    }
  }
  else if (path.find("://") != std::string::npos || path.find(":\\\\") != std::string::npos)
#endif
  {
    StringUtils::Replace(result, '\\', '/');
    /* The double slash correction should only be used when *absolutely*
       necessary! This applies to certain DLLs or use from Python DLLs/scripts
       that incorrectly generate double (back) slashes.
    */
    if (bFixDoubleSlashes && !result.empty())
    {
      // Fixup for double forward slashes(/) but don't touch the :// of URLs
      for (size_t x = 2; x < result.size() - 1; x++)
      {
        if ( result[x] == '/' && result[x + 1] == '/' && !(result[x - 1] == ':' || (result[x - 1] == '/' && result[x - 2] == ':')) )
          result.erase(x);
      }
    }
  }
  return result;
}

bool CUtil::IsUsingTTFSubtitles()
{
  return URIUtils::HasExtension(CSettings::Get().GetString("subtitles.font"), ".ttf");
}

#ifdef UNIT_TESTING
bool CUtil::TestSplitExec()
{
  CStdString function;
  vector<CStdString> params;
  CUtil::SplitExecFunction("ActivateWindow(Video, \"C:\\test\\foo\")", function, params);
  if (function != "ActivateWindow" || params.size() != 2 || params[0] != "Video" || params[1] != "C:\\test\\foo")
    return false;
  params.clear();
  CUtil::SplitExecFunction("ActivateWindow(Video, \"C:\\test\\foo\\\")", function, params);
  if (function != "ActivateWindow" || params.size() != 2 || params[0] != "Video" || params[1] != "C:\\test\\foo\"")
    return false;
  CUtil::SplitExecFunction("ActivateWindow(Video, \"C:\\\\test\\\\foo\\\\\")", function, params);
  if (function != "ActivateWindow" || params.size() != 2 || params[0] != "Video" || params[1] != "C:\\test\\foo\\")
    return false;
  CUtil::SplitExecFunction("ActivateWindow(Video, \"C:\\\\\\\\test\\\\\\foo\\\\\")", function, params);
  if (function != "ActivateWindow" || params.size() != 2 || params[0] != "Video" || params[1] != "C:\\\\test\\\\foo\\")
    return false;
  CUtil::SplitExecFunction("SetProperty(Foo,\"\")", function, params);
  if (function != "SetProperty" || params.size() != 2 || params[0] != "Foo" || params[1] != "")
   return false;
  CUtil::SplitExecFunction("SetProperty(foo,ba(\"ba black )\",sheep))", function, params);
  if (function != "SetProperty" || params.size() != 2 || params[0] != "foo" || params[1] != "ba(\"ba black )\",sheep)")
    return false;
  return true;
}
#endif

<<<<<<< HEAD
void CUtil::SplitExecFunction(const std::string &execString, std::string &function, vector<string> &parameters)
=======
#ifdef UNIT_TESTING
bool CUtil::TestSplitExec()
{
  CStdString function;
  vector<CStdString> params;
  CUtil::SplitExecFunction("ActivateWindow(Video, \"C:\\test\\foo\")", function, params);
  if (function != "ActivateWindow" || params.size() != 2 || params[0] != "Video" || params[1] != "C:\\test\\foo")
    return false;
  params.clear();
  CUtil::SplitExecFunction("ActivateWindow(Video, \"C:\\test\\foo\\\")", function, params);
  if (function != "ActivateWindow" || params.size() != 2 || params[0] != "Video" || params[1] != "C:\\test\\foo\"")
    return false;
  CUtil::SplitExecFunction("ActivateWindow(Video, \"C:\\\\test\\\\foo\\\\\")", function, params);
  if (function != "ActivateWindow" || params.size() != 2 || params[0] != "Video" || params[1] != "C:\\test\\foo\\")
    return false;
  CUtil::SplitExecFunction("ActivateWindow(Video, \"C:\\\\\\\\test\\\\\\foo\\\\\")", function, params);
  if (function != "ActivateWindow" || params.size() != 2 || params[0] != "Video" || params[1] != "C:\\\\test\\\\foo\\")
    return false;
  return true;
}
#endif

void CUtil::SplitExecFunction(const CStdString &execString, CStdString &function, vector<CStdString> &parameters)
>>>>>>> FETCH_HEAD
{
  std::string paramString;

  size_t iPos = execString.find("(");
  size_t iPos2 = execString.rfind(")");
  if (iPos != std::string::npos && iPos2 != std::string::npos)
  {
    paramString = execString.substr(iPos + 1, iPos2 - iPos - 1);
    function = execString.substr(0, iPos);
  }
  else
    function = execString;

  // remove any whitespace, and the standard prefix (if it exists)
  StringUtils::Trim(function);
  if( StringUtils::StartsWithNoCase(function, "xbmc.") )
    function.erase(0, 5);

  SplitParams(paramString, parameters);
}

void CUtil::SplitParams(const CStdString &paramString, std::vector<std::string> &parameters)
{
  bool inQuotes = false;
  bool lastEscaped = false; // only every second character can be escaped
  int inFunction = 0;
  size_t whiteSpacePos = 0;
  CStdString parameter;
  parameters.clear();
  for (size_t pos = 0; pos < paramString.size(); pos++)
  {
    char ch = paramString[pos];
    bool escaped = (pos > 0 && paramString[pos - 1] == '\\' && !lastEscaped);
    lastEscaped = escaped;
    if (inQuotes)
    { // if we're in a quote, we accept everything until the closing quote
      if (ch == '"' && !escaped)
      { // finished a quote - no need to add the end quote to our string
        inQuotes = false;
      }
    }
    else
    { // not in a quote, so check if we should be starting one
      if (ch == '"' && !escaped)
      { // start of quote - no need to add the quote to our string
        inQuotes = true;
      }
      if (inFunction && ch == ')')
      { // end of a function
        inFunction--;
      }
      if (ch == '(')
      { // start of function
        inFunction++;
      }
      if (!inFunction && ch == ',')
      { // not in a function, so a comma signfies the end of this parameter
        if (whiteSpacePos)
          parameter = parameter.substr(0, whiteSpacePos);
        // trim off start and end quotes
        if (parameter.length() > 1 && parameter[0] == '"' && parameter[parameter.length() - 1] == '"')
          parameter = parameter.substr(1, parameter.length() - 2);
        else if (parameter.length() > 3 && parameter[parameter.length() - 1] == '"')
        {
          // check name="value" style param.
          size_t quotaPos = parameter.find('"');
          if (quotaPos > 1 && quotaPos < parameter.length() - 1 && parameter[quotaPos - 1] == '=')
          {
            parameter.erase(parameter.length() - 1);
            parameter.erase(quotaPos);
          }
        }
        parameters.push_back(parameter);
        parameter.clear();
        whiteSpacePos = 0;
        continue;
      }
    }
<<<<<<< HEAD
    if ((ch == '"' || ch == '\\') && escaped)
=======
    if ((ch == '\"' || ch == '\\') && escaped)
>>>>>>> FETCH_HEAD
    { // escaped quote or backslash
      parameter[parameter.size()-1] = ch;
      continue;
    }
    // whitespace handling - we skip any whitespace at the left or right of an unquoted parameter
    if (ch == ' ' && !inQuotes)
    {
      if (parameter.empty()) // skip whitespace on left
        continue;
      if (!whiteSpacePos) // make a note of where whitespace starts on the right
        whiteSpacePos = parameter.size();
    }
    else
      whiteSpacePos = 0;
    parameter += ch;
  }
  if (inFunction || inQuotes)
    CLog::Log(LOGWARNING, "%s(%s) - end of string while searching for ) or \"", __FUNCTION__, paramString.c_str());
  if (whiteSpacePos)
    parameter.erase(whiteSpacePos);
  // trim off start and end quotes
  if (parameter.size() > 1 && parameter[0] == '"' && parameter[parameter.size() - 1] == '"')
    parameter = parameter.substr(1,parameter.size() - 2);
  else if (parameter.size() > 3 && parameter[parameter.size() - 1] == '"')
  {
    // check name="value" style param.
    size_t quotaPos = parameter.find('"');
    if (quotaPos > 1 && quotaPos < parameter.length() - 1 && parameter[quotaPos - 1] == '=')
    {
      parameter.erase(parameter.length() - 1);
      parameter.erase(quotaPos);
    }
  }
  if (!parameter.empty() || parameters.size())
    parameters.push_back(parameter);
}

int CUtil::GetMatchingSource(const CStdString& strPath1, VECSOURCES& VECSOURCES, bool& bIsSourceName)
{
  if (strPath1.empty())
    return -1;

  // copy as we may change strPath
  CStdString strPath = strPath1;

  // Check for special protocols
  CURL checkURL(strPath);

  if (StringUtils::StartsWith(strPath, "special://skin/"))
    return 1;

  // stack://
  if (checkURL.IsProtocol("stack"))
    strPath.erase(0, 8); // remove the stack protocol

  if (checkURL.IsProtocol("shout"))
    strPath = checkURL.GetHostName();
  if (checkURL.IsProtocol("tuxbox"))
    return 1;
  if (checkURL.IsProtocol("plugin"))
    return 1;
  if (checkURL.IsProtocol("multipath"))
    strPath = CMultiPathDirectory::GetFirstPath(strPath);

  bIsSourceName = false;
  int iIndex = -1;

  // we first test the NAME of a source
  for (int i = 0; i < (int)VECSOURCES.size(); ++i)
  {
    CMediaSource share = VECSOURCES.at(i);
    CStdString strName = share.strName;

    // special cases for dvds
    if (URIUtils::IsOnDVD(share.strPath))
    {
      if (URIUtils::IsOnDVD(strPath))
        return i;

      // not a path, so we need to modify the source name
      // since we add the drive status and disc name to the source
      // "Name (Drive Status/Disc Name)"
      size_t iPos = strName.rfind('(');
      if (iPos != std::string::npos && iPos > 1)
        strName = strName.substr(0, iPos - 1);
    }
    if (strPath.Equals(strName))
    {
      bIsSourceName = true;
      return i;
    }
  }

  // now test the paths

  // remove user details, and ensure path only uses forward slashes
  // and ends with a trailing slash so as not to match a substring
  CURL urlDest(strPath);
  urlDest.SetOptions("");
  CStdString strDest = urlDest.GetWithoutUserDetails();
  ForceForwardSlashes(strDest);
  if (!URIUtils::HasSlashAtEnd(strDest))
    strDest += "/";

  size_t iLength = 0;
  size_t iLenPath = strDest.size();
  for (int i = 0; i < (int)VECSOURCES.size(); ++i)
  {
    CMediaSource share = VECSOURCES.at(i);

    // does it match a source name?
    if (share.strPath.substr(0,8) == "shout://")
    {
      CURL url(share.strPath);
      if (strPath == url.GetHostName())
        return i;
    }

    // doesnt match a name, so try the source path
    vector<string> vecPaths;

    // add any concatenated paths if they exist
    if (share.vecPaths.size() > 0)
      vecPaths = share.vecPaths;

    // add the actual share path at the front of the vector
    vecPaths.insert(vecPaths.begin(), share.strPath);

    // test each path
    for (int j = 0; j < (int)vecPaths.size(); ++j)
    {
      // remove user details, and ensure path only uses forward slashes
      // and ends with a trailing slash so as not to match a substring
      CURL urlShare(vecPaths[j]);
      urlShare.SetOptions("");
      CStdString strShare = urlShare.GetWithoutUserDetails();
      ForceForwardSlashes(strShare);
      if (!URIUtils::HasSlashAtEnd(strShare))
        strShare += "/";
      size_t iLenShare = strShare.size();

      if ((iLenPath >= iLenShare) && StringUtils::StartsWithNoCase(strDest, strShare) && (iLenShare > iLength))
      {
        // if exact match, return it immediately
        if (iLenPath == iLenShare)
        {
          // if the path EXACTLY matches an item in a concatentated path
          // set source name to true to load the full virtualpath
          bIsSourceName = false;
          if (vecPaths.size() > 1)
            bIsSourceName = true;
          return i;
        }
        iIndex = i;
        iLength = iLenShare;
      }
    }
  }

  // return the index of the share with the longest match
  if (iIndex == -1)
  {

    // rar:// and zip://
    // if archive wasn't mounted, look for a matching share for the archive instead
    if( StringUtils::StartsWithNoCase(strPath, "rar://") || StringUtils::StartsWithNoCase(strPath, "zip://") )
    {
      // get the hostname portion of the url since it contains the archive file
      strPath = checkURL.GetHostName();

      bIsSourceName = false;
      bool bDummy;
      return GetMatchingSource(strPath, VECSOURCES, bDummy);
    }

    CLog::Log(LOGDEBUG,"CUtil::GetMatchingSource: no matching source found for [%s]", strPath1.c_str());
  }
  return iIndex;
}

CStdString CUtil::TranslateSpecialSource(const CStdString &strSpecial)
{
  if (!strSpecial.empty() && strSpecial[0] == '$')
  {
    if (StringUtils::StartsWithNoCase(strSpecial, "$home"))
      return URIUtils::AddFileToFolder("special://home/", strSpecial.substr(5));
    else if (StringUtils::StartsWithNoCase(strSpecial, "$subtitles"))
      return URIUtils::AddFileToFolder("special://subtitles/", strSpecial.substr(10));
    else if (StringUtils::StartsWithNoCase(strSpecial, "$userdata"))
      return URIUtils::AddFileToFolder("special://userdata/", strSpecial.substr(9));
    else if (StringUtils::StartsWithNoCase(strSpecial, "$database"))
      return URIUtils::AddFileToFolder("special://database/", strSpecial.substr(9));
    else if (StringUtils::StartsWithNoCase(strSpecial, "$thumbnails"))
      return URIUtils::AddFileToFolder("special://thumbnails/", strSpecial.substr(11));
    else if (StringUtils::StartsWithNoCase(strSpecial, "$recordings"))
      return URIUtils::AddFileToFolder("special://recordings/", strSpecial.substr(11));
    else if (StringUtils::StartsWithNoCase(strSpecial, "$screenshots"))
      return URIUtils::AddFileToFolder("special://screenshots/", strSpecial.substr(12));
    else if (StringUtils::StartsWithNoCase(strSpecial, "$musicplaylists"))
      return URIUtils::AddFileToFolder("special://musicplaylists/", strSpecial.substr(15));
    else if (StringUtils::StartsWithNoCase(strSpecial, "$videoplaylists"))
      return URIUtils::AddFileToFolder("special://videoplaylists/", strSpecial.substr(15));
    else if (StringUtils::StartsWithNoCase(strSpecial, "$cdrips"))
      return URIUtils::AddFileToFolder("special://cdrips/", strSpecial.substr(7));
    // this one will be removed post 2.0
    else if (StringUtils::StartsWithNoCase(strSpecial, "$playlists"))
      return URIUtils::AddFileToFolder(CSettings::Get().GetString("system.playlistspath"), strSpecial.substr(10));
  }
  return strSpecial;
}

CStdString CUtil::MusicPlaylistsLocation()
{
  vector<string> vec;
  vec.push_back(URIUtils::AddFileToFolder(CSettings::Get().GetString("system.playlistspath"), "music"));
  vec.push_back(URIUtils::AddFileToFolder(CSettings::Get().GetString("system.playlistspath"), "mixed"));
  return XFILE::CMultiPathDirectory::ConstructMultiPath(vec);
}

CStdString CUtil::VideoPlaylistsLocation()
{
  vector<string> vec;
  vec.push_back(URIUtils::AddFileToFolder(CSettings::Get().GetString("system.playlistspath"), "video"));
  vec.push_back(URIUtils::AddFileToFolder(CSettings::Get().GetString("system.playlistspath"), "mixed"));
  return XFILE::CMultiPathDirectory::ConstructMultiPath(vec);
}

void CUtil::DeleteMusicDatabaseDirectoryCache()
{
  CUtil::DeleteDirectoryCache("mdb-");
  CUtil::DeleteDirectoryCache("sp-"); // overkill as it will delete video smartplaylists, but as we can't differentiate based on URL...
}

void CUtil::DeleteVideoDatabaseDirectoryCache()
{
  CUtil::DeleteDirectoryCache("vdb-");
  CUtil::DeleteDirectoryCache("sp-"); // overkill as it will delete music smartplaylists, but as we can't differentiate based on URL...
}

void CUtil::DeleteDirectoryCache(const CStdString &prefix)
{
  CStdString searchPath = "special://temp/";
  CFileItemList items;
  if (!XFILE::CDirectory::GetDirectory(searchPath, items, ".fi", DIR_FLAG_NO_FILE_DIRS))
    return;

  for (int i = 0; i < items.Size(); ++i)
  {
    if (items[i]->m_bIsFolder)
      continue;
    CStdString fileName = URIUtils::GetFileName(items[i]->GetPath());
    if (StringUtils::StartsWith(fileName, prefix))
      XFILE::CFile::Delete(items[i]->GetPath());
  }
}


void CUtil::GetRecursiveListing(const CStdString& strPath, CFileItemList& items, const CStdString& strMask, unsigned int flags /* = DIR_FLAG_DEFAULTS */)
{
  CFileItemList myItems;
  CDirectory::GetDirectory(strPath,myItems,strMask,flags);
  for (int i=0;i<myItems.Size();++i)
  {
    if (myItems[i]->m_bIsFolder)
      CUtil::GetRecursiveListing(myItems[i]->GetPath(),items,strMask,flags);
    else
      items.Add(myItems[i]);
  }
}

void CUtil::GetRecursiveDirsListing(const CStdString& strPath, CFileItemList& item, unsigned int flags /* = DIR_FLAG_DEFAULTS */)
{
  CFileItemList myItems;
  CDirectory::GetDirectory(strPath,myItems,"",flags);
  for (int i=0;i<myItems.Size();++i)
  {
    if (myItems[i]->m_bIsFolder && !myItems[i]->IsPath(".."))
    {
      item.Add(myItems[i]);
      CUtil::GetRecursiveDirsListing(myItems[i]->GetPath(),item,flags);
    }
  }
}

void CUtil::ForceForwardSlashes(CStdString& strPath)
{
  size_t iPos = strPath.rfind('\\');
  while (iPos != string::npos)
  {
    strPath.at(iPos) = '/';
    iPos = strPath.rfind('\\');
  }
}

double CUtil::AlbumRelevance(const CStdString& strAlbumTemp1, const CStdString& strAlbum1, const CStdString& strArtistTemp1, const CStdString& strArtist1)
{
  // case-insensitive fuzzy string comparison on the album and artist for relevance
  // weighting is identical, both album and artist are 50% of the total relevance
  // a missing artist means the maximum relevance can only be 0.50
  CStdString strAlbumTemp = strAlbumTemp1;
  StringUtils::ToLower(strAlbumTemp);
  CStdString strAlbum = strAlbum1;
  StringUtils::ToLower(strAlbum);
  double fAlbumPercentage = fstrcmp(strAlbumTemp, strAlbum, 0.0f);
  double fArtistPercentage = 0.0f;
  if (!strArtist1.empty())
  {
    CStdString strArtistTemp = strArtistTemp1;
    StringUtils::ToLower(strArtistTemp);
    CStdString strArtist = strArtist1;
    StringUtils::ToLower(strArtist);
    fArtistPercentage = fstrcmp(strArtistTemp, strArtist, 0.0f);
  }
  double fRelevance = fAlbumPercentage * 0.5f + fArtistPercentage * 0.5f;
  return fRelevance;
}

bool CUtil::MakeShortenPath(std::string StrInput, std::string& StrOutput, size_t iTextMaxLength)
{
  size_t iStrInputSize = StrInput.size();
  if(iStrInputSize <= 0 || iTextMaxLength >= iStrInputSize)
    return false;

  char cDelim = '\0';
  size_t nGreaterDelim, nPos;

  nPos = StrInput.find_last_of( '\\' );
  if (nPos != std::string::npos)
    cDelim = '\\';
  else
  {
    nPos = StrInput.find_last_of( '/' );
    if (nPos != std::string::npos)
      cDelim = '/';
  }
  if ( cDelim == '\0' )
    return false;

  if (nPos == StrInput.size() - 1)
  {
    StrInput.erase(StrInput.size() - 1);
    nPos = StrInput.find_last_of(cDelim);
  }
  while( iTextMaxLength < iStrInputSize )
  {
    nPos = StrInput.find_last_of( cDelim, nPos );
    nGreaterDelim = nPos;
    if ( nPos != std::string::npos )
      nPos = StrInput.find_last_of( cDelim, nPos - 1 );
    if ( nPos == CStdString::npos ) break;
    if ( nGreaterDelim > nPos ) StrInput.replace( nPos + 1, nGreaterDelim - nPos - 1, ".." );
    iStrInputSize = StrInput.size();
  }
  // replace any additional /../../ with just /../ if necessary
  std::string replaceDots = StringUtils::Format("..%c..", cDelim);
  while (StrInput.size() > (unsigned int)iTextMaxLength)
    if (!StringUtils::Replace(StrInput, replaceDots, ".."))
      break;
  // finally, truncate our string to force inside our max text length,
  // replacing the last 2 characters with ".."

  // eg end up with:
  // "smb://../Playboy Swimsuit Cal.."
  if (iTextMaxLength > 2 && StrInput.size() > (unsigned int)iTextMaxLength)
  {
    StrInput.erase(iTextMaxLength - 2);
    StrInput += "..";
  }
  StrOutput = StrInput;
  return true;
}

bool CUtil::SupportsWriteFileOperations(const CStdString& strPath)
{
  // currently only hd, smb, nfs, afp and dav support delete and rename
  if (URIUtils::IsHD(strPath))
    return true;
  if (URIUtils::IsSmb(strPath))
    return true;
  if (CUtil::IsTVRecording(strPath))
    return CPVRDirectory::SupportsWriteFileOperations(strPath);
  if (URIUtils::IsNfs(strPath))
    return true;
  if (URIUtils::IsAfp(strPath))
    return true;
  if (URIUtils::IsDAV(strPath))
    return true;
  if (URIUtils::IsMythTV(strPath))
  {
    /*
     * Can't use CFile::Exists() to check whether the myth:// path supports file operations because
     * it hits the directory cache on the way through, which has the Live Channels and Guide
     * items cached.
     */
    return CMythDirectory::SupportsWriteFileOperations(strPath);
  }
  if (URIUtils::IsStack(strPath))
    return SupportsWriteFileOperations(CStackDirectory::GetFirstStackedFile(strPath));
  if (URIUtils::IsMultiPath(strPath))
    return CMultiPathDirectory::SupportsWriteFileOperations(strPath);

  return false;
}

bool CUtil::SupportsReadFileOperations(const CStdString& strPath)
{
  if (URIUtils::IsVideoDb(strPath))
    return false;

  return true;
}

CStdString CUtil::GetDefaultFolderThumb(const CStdString &folderThumb)
{
  if (g_TextureManager.HasTexture(folderThumb))
    return folderThumb;
  return "";
}

void CUtil::GetSkinThemes(vector<std::string>& vecTheme)
{
  std::string strPath = URIUtils::AddFileToFolder(g_graphicsContext.GetMediaDir(), "media");
  CFileItemList items;
  CDirectory::GetDirectory(strPath, items);
  // Search for Themes in the Current skin!
  for (int i = 0; i < items.Size(); ++i)
  {
    CFileItemPtr pItem = items[i];
    if (!pItem->m_bIsFolder)
    {
      CStdString strExtension = URIUtils::GetExtension(pItem->GetPath());
      if ((strExtension == ".xpr" && !StringUtils::EqualsNoCase(pItem->GetLabel(), "Textures.xpr")) ||
          (strExtension == ".xbt" && !StringUtils::EqualsNoCase(pItem->GetLabel(), "Textures.xbt")))
      {
        std::string strLabel = pItem->GetLabel();
        vecTheme.push_back(strLabel.substr(0, strLabel.size() - 4));
      }
    }
  }
  sort(vecTheme.begin(), vecTheme.end(), sortstringbyname());
}

void CUtil::InitRandomSeed()
{
  // Init random seed
  int64_t now;
  now = CurrentHostCounter();
  unsigned int seed = (unsigned int)now;
//  CLog::Log(LOGDEBUG, "%s - Initializing random seed with %u", __FUNCTION__, seed);
  srand(seed);
}

#ifdef TARGET_POSIX
bool CUtil::RunCommandLine(const CStdString& cmdLine, bool waitExit)
{
  vector<string> args = StringUtils::Split(cmdLine, ",");

  // Strip quotes and whitespace around the arguments, or exec will fail.
  // This allows the python invocation to be written more naturally with any amount of whitespace around the args.
  // But it's still limited, for example quotes inside the strings are not expanded, etc.
  // TODO: Maybe some python library routine can parse this more properly ?
  for (vector<string>::iterator it = args.begin(); it != args.end(); ++it)
  {
    size_t pos;
    pos = it->find_first_not_of(" \t\n\"'");
    if (pos != std::string::npos)
    {
      it->erase(0, pos);
    }

    pos = it->find_last_not_of(" \t\n\"'"); // if it returns npos we'll end up with an empty string which is OK
    {
      it->erase(++pos, it->size());
    }
  }

  return Command(args, waitExit);
}

//
// FIXME, this should be merged with the function below.
//
bool CUtil::Command(const std::vector<std::string>& arrArgs, bool waitExit)
{
#ifdef _DEBUG
  printf("Executing: ");
  for (size_t i=0; i<arrArgs.size(); i++)
    printf("%s ", arrArgs[i].c_str());
  printf("\n");
#endif

  pid_t child = fork();
  int n = 0;
  if (child == 0)
  {
    if (!waitExit)
    {
      // fork again in order not to leave a zombie process
      child = fork();
      if (child == -1)
        _exit(2);
      else if (child != 0)
        _exit(0);
    }
    close(0);
    close(1);
    close(2);
    if (arrArgs.size() > 0)
    {
      char **args = (char **)alloca(sizeof(char *) * (arrArgs.size() + 3));
      memset(args, 0, (sizeof(char *) * (arrArgs.size() + 3)));
      for (size_t i=0; i<arrArgs.size(); i++)
        args[i] = (char *)arrArgs[i].c_str();
      execvp(args[0], args);
    }
  }
  else
  {
    waitpid(child, &n, 0);
  }

<<<<<<< HEAD
  return (waitExit) ? (WEXITSTATUS(n) == 0) : true;
=======
void CUtil::DeleteMusicDatabaseDirectoryCache()
{
  CUtil::DeleteDirectoryCache("mdb-");
>>>>>>> FETCH_HEAD
}

bool CUtil::SudoCommand(const CStdString &strCommand)
{
<<<<<<< HEAD
  CLog::Log(LOGDEBUG, "Executing sudo command: <%s>", strCommand.c_str());
  pid_t child = fork();
  int n = 0;
  if (child == 0)
  {
    close(0); // close stdin to avoid sudo request password
    close(1);
    close(2);
    vector<string> arrArgs = StringUtils::Split(strCommand, " ");
    if (arrArgs.size() > 0)
    {
      char **args = (char **)alloca(sizeof(char *) * (arrArgs.size() + 3));
      memset(args, 0, (sizeof(char *) * (arrArgs.size() + 3)));
      args[0] = (char *)"/usr/bin/sudo";
      args[1] = (char *)"-S";
      for (size_t i=0; i<arrArgs.size(); i++)
      {
        args[i+2] = (char *)arrArgs[i].c_str();
      }
      execvp("/usr/bin/sudo", args);
    }
  }
  else
    waitpid(child, &n, 0);

  return WEXITSTATUS(n) == 0;
=======
  CUtil::DeleteDirectoryCache("vdb-");
>>>>>>> FETCH_HEAD
}
#endif

<<<<<<< HEAD
int CUtil::LookupRomanDigit(char roman_digit)
{
  switch (roman_digit)
  {
    case 'i':
    case 'I':
      return 1;
    case 'v':
    case 'V':
      return 5;
    case 'x':
    case 'X':
      return 10;
    case 'l':
    case 'L':
      return 50;
    case 'c':
    case 'C':
      return 100;
    case 'd':
    case 'D':
      return 500;
    case 'm':
    case 'M':
      return 1000;
    default:
      return 0;
=======
void CUtil::DeleteDirectoryCache(const CStdString &prefix)
{
  CStdString searchPath = "special://temp/";
  CFileItemList items;
  if (!XFILE::CDirectory::GetDirectory(searchPath, items, ".fi", false))
    return;

  for (int i = 0; i < items.Size(); ++i)
  {
    if (items[i]->m_bIsFolder)
      continue;
    CStdString fileName = CUtil::GetFileName(items[i]->m_strPath);
    if (fileName.Left(prefix.GetLength()) == prefix)
      XFILE::CFile::Delete(items[i]->m_strPath);
>>>>>>> FETCH_HEAD
  }
}

int CUtil::TranslateRomanNumeral(const char* roman_numeral)
{
  
  int decimal = -1;

  if (roman_numeral && roman_numeral[0])
  {
    int temp_sum  = 0,
        last      = 0,
        repeat    = 0,
        trend     = 1;
    decimal = 0;
    while (*roman_numeral)
    {
      int digit = CUtil::LookupRomanDigit(*roman_numeral);
      int test  = last;
      
      // General sanity checks

      // numeral not in LUT
      if (!digit)
        return -1;
      
      while (test > 5)
        test /= 10;
      
      // N = 10^n may not precede (N+1) > 10^(N+1)
      if (test == 1 && digit > last * 10)
        return -1;
      
      // N = 5*10^n may not precede (N+1) >= N
      if (test == 5 && digit >= last) 
        return -1;

      // End general sanity checks

      if (last < digit)
      {
        // smaller numerals may not repeat before a larger one
        if (repeat) 
          return -1;

        temp_sum += digit;
        
        repeat  = 0;
        trend   = 0;
      }
      else if (last == digit)
      {
        temp_sum += digit;
        repeat++;
        trend = 1;
      }
      else
      {
        if (!repeat)
          decimal += 2 * last - temp_sum;
        else
          decimal += temp_sum;
        
        temp_sum = digit;

        trend   = 1;
        repeat  = 0;
      }
      // Post general sanity checks

      // numerals may not repeat more than thrice
      if (repeat == 3)
        return -1;

      last = digit;
      roman_numeral++;
    }

    if (trend)
      decimal += temp_sum;
    else
      decimal += 2 * last - temp_sum;
  }
  return decimal;
}

CStdString CUtil::ResolveExecutablePath()
{
  CStdString strExecutablePath;
#ifdef TARGET_WINDOWS
  static const size_t bufSize = MAX_PATH * 2;
  wchar_t* buf = new wchar_t[bufSize];
  buf[0] = 0;
  ::GetModuleFileNameW(0, buf, bufSize);
  buf[bufSize-1] = 0;
  g_charsetConverter.wToUTF8(buf,strExecutablePath);
  delete[] buf;
#elif defined(TARGET_DARWIN)
  char     given_path[2*MAXPATHLEN];
  uint32_t path_size =2*MAXPATHLEN;

  CDarwinUtils::GetExecutablePath(given_path, &path_size);
  strExecutablePath = given_path;
#elif defined(TARGET_FREEBSD)                                                                                                                                                                   
  char buf[PATH_MAX];
  size_t buflen;
  int mib[4];

  mib[0] = CTL_KERN;
  mib[1] = KERN_PROC;
  mib[2] = KERN_PROC_PATHNAME;
  mib[3] = getpid();

  buflen = sizeof(buf) - 1;
  if(sysctl(mib, 4, buf, &buflen, NULL, 0) < 0)
    strExecutablePath = "";
  else
    strExecutablePath = buf;
#else
  /* Get our PID and build the name of the link in /proc */
  pid_t pid = getpid();
  char linkname[64]; /* /proc/<pid>/exe */
  snprintf(linkname, sizeof(linkname), "/proc/%i/exe", pid);

  /* Now read the symbolic link */
  char buf[PATH_MAX + 1];
  buf[0] = 0;

  int ret = readlink(linkname, buf, sizeof(buf) - 1);
  if (ret != -1)
    buf[ret] = 0;

  strExecutablePath = buf;
#endif
  return strExecutablePath;
}

CStdString CUtil::GetFrameworksPath(bool forPython)
{
  CStdString strFrameworksPath;
#if defined(TARGET_DARWIN)
  char     given_path[2*MAXPATHLEN];
  uint32_t path_size =2*MAXPATHLEN;

  CDarwinUtils::GetFrameworkPath(forPython, given_path, &path_size);
  strFrameworksPath = given_path;
#endif
  return strFrameworksPath;
}

void CUtil::ScanForExternalSubtitles(const std::string& strMovie, std::vector<std::string>& vecSubtitles )
{
  unsigned int startTimer = XbmcThreads::SystemClockMillis();
  
  // new array for commons sub dirs
  const char * common_sub_dirs[] = {"subs",
    "Subs",
    "subtitles",
    "Subtitles",
    "vobsubs",
    "Vobsubs",
    "sub",
    "Sub",
    "vobsub",
    "Vobsub",
    "subtitle",
    "Subtitle",
    NULL};
  
  CFileItem item(strMovie, false);
  if ( item.IsInternetStream()
    || item.IsHDHomeRun()
    || item.IsSlingbox()
    || item.IsPlayList()
    || item.IsLiveTV()
    || !item.IsVideo())
    return;
  
  vector<std::string> strLookInPaths;
  
  std::string strMovieFileName;
  std::string strPath;
  
  URIUtils::Split(strMovie, strPath, strMovieFileName);
  std::string strMovieFileNameNoExt(URIUtils::ReplaceExtension(strMovieFileName, ""));
  strLookInPaths.push_back(strPath);
  
  CURL url(strMovie);
  std::string isoFileNameNoExt;
  if (url.IsProtocol("bluray"))
      url = CURL(url.GetHostName());
  //a path inside an iso
  if (url.IsProtocol("udf"))
  {
    std::string isoPath = url.GetHostName();
    URIUtils::RemoveSlashAtEnd(isoPath);
    CFileItem iso(isoPath, false);

    if (iso.IsDiscImage())
    {
      std::string isoFileName;
      URIUtils::Split(iso.GetPath(), isoPath, isoFileName);
      isoFileNameNoExt = URIUtils::ReplaceExtension(isoFileName, "");
      strLookInPaths.push_back(isoPath);
    }
  }
  
  if (!CMediaSettings::Get().GetAdditionalSubtitleDirectoryChecked() && !CSettings::Get().GetString("subtitles.custompath").empty()) // to avoid checking non-existent directories (network) every time..
  {
    if (!g_application.getNetwork().IsAvailable() && !URIUtils::IsHD(CSettings::Get().GetString("subtitles.custompath")))
    {
      CLog::Log(LOGINFO,"CUtil::CacheSubtitles: disabling alternate subtitle directory for this session, it's nonaccessible");
      CMediaSettings::Get().SetAdditionalSubtitleDirectoryChecked(-1); // disabled
    }
    else if (!CDirectory::Exists(CSettings::Get().GetString("subtitles.custompath")))
    {
      CLog::Log(LOGINFO,"CUtil::CacheSubtitles: disabling alternate subtitle directory for this session, it's nonexistant");
      CMediaSettings::Get().SetAdditionalSubtitleDirectoryChecked(-1); // disabled
    }
    
    CMediaSettings::Get().SetAdditionalSubtitleDirectoryChecked(1);
  }
  
  if (StringUtils::StartsWith(strMovie, "rar://")) // <--- if this is found in main path then ignore it!
  {
    CURL url(strMovie);
    std::string strArchive = url.GetHostName();
    URIUtils::Split(strArchive, strPath, strMovieFileName);
    strLookInPaths.push_back(strPath);
  }
  
  int iSize = strLookInPaths.size();
  for (int i=0; i<iSize; ++i)
  {
    vector<string> directories = StringUtils::Split( strLookInPaths[i], "/" );
    if (directories.size() == 1)
      directories = StringUtils::Split( strLookInPaths[i], "\\" );

    // if it's inside a cdX dir, add parent path
    if (directories.size() >= 2 && directories[directories.size()-2].size() == 3 && StringUtils::StartsWithNoCase(directories[directories.size()-2], "cd")) // SplitString returns empty token as last item, hence size-2
    {
      std::string strPath2;
      URIUtils::GetParentPath(strLookInPaths[i], strPath2);
      strLookInPaths.push_back(strPath2);
    }
  }

  // checking if any of the common subdirs exist ..
  iSize = strLookInPaths.size();
  for (int i=0;i<iSize;++i)
  {
    for (int j=0; common_sub_dirs[j]; j++)
    {
      std::string strPath2 = URIUtils::AddFileToFolder(strLookInPaths[i],common_sub_dirs[j]);
      URIUtils::AddSlashAtEnd(strPath2);
      if (CDirectory::Exists(strPath2))
        strLookInPaths.push_back(strPath2);
    }
  }
  // .. done checking for common subdirs
  
  // check if there any cd-directories in the paths we have added so far
  iSize = strLookInPaths.size();
  for (int i=0;i<9;++i) // 9 cd's
  {
    std::string cdDir = StringUtils::Format("cd%i",i+1);
    for (int i=0;i<iSize;++i)
    {
      std::string strPath2 = URIUtils::AddFileToFolder(strLookInPaths[i],cdDir);
      URIUtils::AddSlashAtEnd(strPath2);
      bool pathAlreadyAdded = false;
      for (unsigned int i=0; i<strLookInPaths.size(); i++)
      {
        // if movie file is inside cd-dir, this directory can exist in vector already
        if (StringUtils::EqualsNoCase(strLookInPaths[i], strPath2))
          pathAlreadyAdded = true;
      }
      if (CDirectory::Exists(strPath2) && !pathAlreadyAdded) 
        strLookInPaths.push_back(strPath2);
    }
  }
  // .. done checking for cd-dirs
  
  // this is last because we dont want to check any common subdirs or cd-dirs in the alternate <subtitles> dir.
  if (CMediaSettings::Get().GetAdditionalSubtitleDirectoryChecked() == 1)
  {
    strPath = CSettings::Get().GetString("subtitles.custompath");
    URIUtils::AddSlashAtEnd(strPath);
    strLookInPaths.push_back(strPath);
  }
  
  std::string strDest;
  std::string strItem;
  
  // 2 steps for movie directory and alternate subtitles directory
  CLog::Log(LOGDEBUG,"%s: Searching for subtitles...", __FUNCTION__);
  for (unsigned int step = 0; step < strLookInPaths.size(); step++)
  {
    if (strLookInPaths[step].length() != 0)
    {
      CFileItemList items;
      
      CDirectory::GetDirectory(strLookInPaths[step], items, g_advancedSettings.m_subtitlesExtensions, DIR_FLAG_NO_FILE_DIRS);
      
      for (int j = 0; j < items.Size(); j++)
      {
        URIUtils::Split(items[j]->GetPath(), strPath, strItem);
        
        if (StringUtils::StartsWithNoCase(strItem, strMovieFileNameNoExt)
          || (!isoFileNameNoExt.empty() && StringUtils::StartsWithNoCase(strItem, isoFileNameNoExt)))
        {
          // is this a rar or zip-file
          if (URIUtils::IsRAR(strItem) || URIUtils::IsZIP(strItem))
          {
            // zip-file name equals strMovieFileNameNoExt, don't check in zip-file
            ScanArchiveForSubtitles( items[j]->GetPath(), "", vecSubtitles );
          }
          else    // not a rar/zip file
          {
            for (int i = 0; sub_exts[i]; i++)
            {
              //Cache subtitle with same name as movie
              if (URIUtils::HasExtension(strItem, sub_exts[i]))
              {
                vecSubtitles.push_back( items[j]->GetPath() ); 
                CLog::Log(LOGINFO, "%s: found subtitle file %s\n", __FUNCTION__, CURL::GetRedacted(items[j]->GetPath()).c_str() );
              }
            }
          }
        }
        else
        {
          // is this a rar or zip-file
          if (URIUtils::IsRAR(strItem) || URIUtils::IsZIP(strItem))
          {
            // check strMovieFileNameNoExt in zip-file
            ScanArchiveForSubtitles( items[j]->GetPath(), strMovieFileNameNoExt, vecSubtitles );
          }
        }
      }
    }
  }

  iSize = vecSubtitles.size();
  for (int i = 0; i < iSize; i++)
  {
    if (URIUtils::HasExtension(vecSubtitles[i], ".smi"))
    {
      //Cache multi-language sami subtitle
      CDVDSubtitleStream* pStream = new CDVDSubtitleStream();
      if(pStream->Open(vecSubtitles[i]))
      {
        CDVDSubtitleTagSami TagConv;
        TagConv.LoadHead(pStream);
        if (TagConv.m_Langclass.size() >= 2)
        {
          for (unsigned int k = 0; k < TagConv.m_Langclass.size(); k++)
          {
            strDest = StringUtils::Format("special://temp/subtitle.%s.%d.smi", TagConv.m_Langclass[k].Name.c_str(), i);
            if (CFile::Copy(vecSubtitles[i], strDest))
            {
              CLog::Log(LOGINFO, " cached subtitle %s->%s\n", CURL::GetRedacted(vecSubtitles[i]).c_str(), strDest.c_str());
              vecSubtitles.push_back(strDest);
            }
          }
        }
      }
      delete pStream;
    }
  }
  CLog::Log(LOGDEBUG,"%s: END (total time: %i ms)", __FUNCTION__, (int)(XbmcThreads::SystemClockMillis() - startTimer));
}

int CUtil::ScanArchiveForSubtitles( const std::string& strArchivePath, const std::string& strMovieFileNameNoExt, std::vector<std::string>& vecSubtitles )
{
  int nSubtitlesAdded = 0;
  CFileItemList ItemList;
 
  // zip only gets the root dir
  if (URIUtils::HasExtension(strArchivePath, ".zip"))
  {
   CURL pathToUrl(strArchivePath);
   CURL zipURL = URIUtils::CreateArchivePath("zip", pathToUrl, "");
   if (!CDirectory::GetDirectory(zipURL, ItemList, "", DIR_FLAG_NO_FILE_DIRS))
    return false;
  }
  else
  {
 #ifdef HAS_FILESYSTEM_RAR
   // get _ALL_files in the rar, even those located in subdirectories because we set the bMask to false.
   // so now we dont have to find any subdirs anymore, all files in the rar is checked.
   if( !g_RarManager.GetFilesInRar(ItemList, strArchivePath, false, "") )
    return false;
 #else
   return false;
 #endif
  }
  for (int it= 0 ; it <ItemList.Size();++it)
  {
   std::string strPathInRar = ItemList[it]->GetPath();
   std::string strExt = URIUtils::GetExtension(strPathInRar);
   
   CLog::Log(LOGDEBUG, "ScanArchiveForSubtitles:: Found file %s", strPathInRar.c_str());
   // always check any embedded rar archives
   // checking for embedded rars, I moved this outside the sub_ext[] loop. We only need to check this once for each file.
   if (URIUtils::IsRAR(strPathInRar) || URIUtils::IsZIP(strPathInRar))
   {
    CURL urlRar;
    CURL pathToUrl(strArchivePath);
    if (strExt == ".rar")
      urlRar = URIUtils::CreateArchivePath("rar", pathToUrl, strPathInRar);
    else
      urlRar = URIUtils::CreateArchivePath("zip", pathToUrl, strPathInRar);
    ScanArchiveForSubtitles(urlRar.Get(), strMovieFileNameNoExt, vecSubtitles);
   }
   // done checking if this is a rar-in-rar

   // check that the found filename matches the movie filename
   int fnl = strMovieFileNameNoExt.size();
   if (fnl && !StringUtils::StartsWithNoCase(URIUtils::GetFileName(strPathInRar), strMovieFileNameNoExt))
     continue;

   int iPos=0;
    while (sub_exts[iPos])
    {
     if (StringUtils::EqualsNoCase(strExt, sub_exts[iPos]))
     {
       CURL pathToURL(strArchivePath);
      CStdString strSourceUrl;
      if (URIUtils::HasExtension(strArchivePath, ".rar"))
       strSourceUrl = URIUtils::CreateArchivePath("rar", pathToURL, strPathInRar).Get();
      else
       strSourceUrl = strPathInRar;
      
       CLog::Log(LOGINFO, "%s: found subtitle file %s\n", __FUNCTION__, strSourceUrl.c_str() );
       vecSubtitles.push_back( strSourceUrl );
       nSubtitlesAdded++;
     }
     
     iPos++;
    }
  }

  return nSubtitlesAdded;
}

void CUtil::GetExternalStreamDetailsFromFilename(const CStdString& strVideo, const CStdString& strStream, ExternalStreamInfo& info)
{
  CStdString videoBaseName = URIUtils::GetFileName(strVideo);
  URIUtils::RemoveExtension(videoBaseName);

  CStdString toParse = URIUtils::GetFileName(strStream);
  URIUtils::RemoveExtension(toParse);

  // we check left part - if it's same as video base name - strip it
  if (StringUtils::StartsWithNoCase(toParse, videoBaseName))
    toParse = toParse.substr(videoBaseName.length());
  else if (URIUtils::GetExtension(strStream) == ".sub" && URIUtils::IsInArchive(strStream))
  {
    // exclude parsing of vobsub file names that embedded in an archive
    CLog::Log(LOGDEBUG, "%s - skipping archived vobsub filename parsing: %s", __FUNCTION__, CURL::GetRedacted(strStream).c_str());
    toParse.clear();
  }

  // trim any non-alphanumeric char in the begining
  std::string::iterator result = std::find_if(toParse.begin(), toParse.end(), ::isalnum);

  std::string name;
  if (result != toParse.end()) // if we have anything to parse
  {
    std::string inputString(result, toParse.end());
    std::string delimiters(" .-");
    std::vector<std::string> tokens;
    StringUtils::Tokenize(inputString, tokens, delimiters);

    for (std::vector<std::string>::iterator it = tokens.begin(); it != tokens.end(); ++it)
    {
      if (info.language.empty())
      {
        CStdString langTmp(*it);
        CStdString langCode;
        // try to recognize language
        if (g_LangCodeExpander.ConvertToThreeCharCode(langCode, langTmp))
        {
          info.language = langCode;
          continue;
        }
      }

      // try to recognize a flag
      std::string flag_tmp(*it);
      StringUtils::ToLower(flag_tmp);
      if (!flag_tmp.compare("none"))
      {
        info.flag |= CDemuxStream::FLAG_NONE;
        continue;
      }
      else if (!flag_tmp.compare("default"))
      {
        info.flag |= CDemuxStream::FLAG_DEFAULT;
        continue;
      }
      else if (!flag_tmp.compare("forced"))
      {
        info.flag |= CDemuxStream::FLAG_FORCED;
        continue;
      }

      name += " " + (*it);
    }
  }
  name += " ";
  name += g_localizeStrings.Get(21602); // External
  StringUtils::Trim(name);
  info.name = StringUtils::RemoveDuplicatedSpacesAndTabs(name);
  if (info.flag == 0)
    info.flag = CDemuxStream::FLAG_NONE;

  CLog::Log(LOGDEBUG, "%s - Language = '%s' / Name = '%s' / Flag = '%u' from %s",
             __FUNCTION__, info.language.c_str(), info.name.c_str(), info.flag, CURL::GetRedacted(strStream).c_str());
}

<<<<<<< HEAD
/*! \brief in a vector of subtitles finds the corresponding .sub file for a given .idx file
 */
bool CUtil::FindVobSubPair(const std::vector<std::string>& vecSubtitles, const std::string& strIdxPath, std::string& strSubPath)
{
  if (URIUtils::HasExtension(strIdxPath, ".idx"))
  {
    std::string strIdxFile;
    std::string strIdxDirectory;
    URIUtils::Split(strIdxPath, strIdxDirectory, strIdxFile);
    for (unsigned int j=0; j < vecSubtitles.size(); j++)
    {
      std::string strSubFile;
      std::string strSubDirectory;
      URIUtils::Split(vecSubtitles[j], strSubDirectory, strSubFile);
      if (URIUtils::IsInArchive(vecSubtitles[j]))
        strSubDirectory = CURL::Decode(strSubDirectory);
      if (URIUtils::HasExtension(strSubFile, ".sub") &&
          (URIUtils::ReplaceExtension(strIdxPath,"").Equals(URIUtils::ReplaceExtension(vecSubtitles[j],"")) ||
           (strSubDirectory.size() >= 11 &&
            StringUtils::EqualsNoCase(strSubDirectory.substr(6, strSubDirectory.length()-11), URIUtils::ReplaceExtension(strIdxPath,"")))))
      {
        strSubPath = vecSubtitles[j];
        return true;
      }
    }
  }
  return false;
}

/*! \brief checks if in the vector of subtitles the given .sub file has a corresponding idx and hence is a vobsub file
 */
bool CUtil::IsVobSub(const std::vector<std::string>& vecSubtitles, const std::string& strSubPath)
=======
void CUtil::InitRandomSeed()
{
  // Init random seed
  int64_t now;
  now = CurrentHostCounter();
  unsigned int seed = (unsigned int)now;
//  CLog::Log(LOGDEBUG, "%s - Initializing random seed with %u", __FUNCTION__, seed);
  srand(seed);
}

#ifdef _LINUX
bool CUtil::RunCommandLine(const CStdString& cmdLine, bool waitExit)
>>>>>>> FETCH_HEAD
{
  if (URIUtils::HasExtension(strSubPath, ".sub"))
  {
    std::string strSubFile;
    std::string strSubDirectory;
    URIUtils::Split(strSubPath, strSubDirectory, strSubFile);
    if (URIUtils::IsInArchive(strSubPath))
      strSubDirectory = CURL::Decode(strSubDirectory);
    for (unsigned int j=0; j < vecSubtitles.size(); j++)
    {
      std::string strIdxFile;
      std::string strIdxDirectory;
      URIUtils::Split(vecSubtitles[j], strIdxDirectory, strIdxFile);
      if (URIUtils::HasExtension(strIdxFile, ".idx") &&
          (URIUtils::ReplaceExtension(vecSubtitles[j],"").Equals(URIUtils::ReplaceExtension(strSubPath,"")) ||
           (strSubDirectory.size() >= 11 &&
            StringUtils::EqualsNoCase(strSubDirectory.substr(6, strSubDirectory.length()-11), URIUtils::ReplaceExtension(vecSubtitles[j],"")))))
        return true;
    }
  }
  return false;
}

/*! \brief find a plain or archived vobsub .sub file corresponding to an .idx file
 */
std::string CUtil::GetVobSubSubFromIdx(const std::string& vobSubIdx)
{
  std::string vobSub = URIUtils::ReplaceExtension(vobSubIdx, ".sub");

  // check if a .sub file exists in the same directory
  if (CFile::Exists(vobSub))
  {
    return vobSub;
  }

  // look inside a .rar or .zip in the same directory
  const std::string archTypes[] = { "rar", "zip" };
  std::string vobSubFilename = URIUtils::GetFileName(vobSub);
  for (unsigned int i = 0; i < ARRAY_SIZE(archTypes); i++)
  {
    vobSub = URIUtils::CreateArchivePath(archTypes[i],
                                         CURL(URIUtils::ReplaceExtension(vobSubIdx, std::string(".") + archTypes[i])),
                                         vobSubFilename).Get();
    if (CFile::Exists(vobSub))
      return vobSub;
  }

  return std::string();
}

/*! \brief find a .idx file from a path of a plain or archived vobsub .sub file
 */
std::string CUtil::GetVobSubIdxFromSub(const std::string& vobSub)
{
  std::string vobSubIdx = URIUtils::ReplaceExtension(vobSub, ".idx");

  // check if a .idx file exists in the same directory
  if (CFile::Exists(vobSubIdx))
  {
    return vobSubIdx;
  }

  // look outside archive (usually .rar) if the .sub is inside one
  if (URIUtils::IsInArchive(vobSub))
  {

    std::string archiveFile = URIUtils::GetDirectory(vobSub);
    std::string vobSubIdxDir = URIUtils::GetParentPath(archiveFile);

    if (!vobSubIdxDir.empty())
    {
      std::string vobSubIdxFilename = URIUtils::GetFileName(vobSubIdx);
      std::string vobSubIdx = URIUtils::AddFileToFolder(vobSubIdxDir, vobSubIdxFilename);

      if (CFile::Exists(vobSubIdx))
        return vobSubIdx;
    }
  }

  return std::string();
}

bool CUtil::CanBindPrivileged()
{
#ifdef TARGET_POSIX

  if (geteuid() == 0)
    return true; //root user can always bind to privileged ports

#ifdef HAVE_LIBCAP

  //check if CAP_NET_BIND_SERVICE is enabled, this allows non-root users to bind to privileged ports
  bool canbind = false;
  cap_t capabilities = cap_get_proc();
  if (capabilities)
  {
    cap_flag_value_t value;
    if (cap_get_flag(capabilities, CAP_NET_BIND_SERVICE, CAP_EFFECTIVE, &value) == 0)
      canbind = value;

    cap_free(capabilities);
  }

  return canbind;

#else //HAVE_LIBCAP

  return false;

#endif //HAVE_LIBCAP

#else //TARGET_POSIX

  return true;

#endif //TARGET_POSIX
}

bool CUtil::ValidatePort(int port)
{
  // check that it's a valid port
#ifdef TARGET_POSIX
  if (!CUtil::CanBindPrivileged() && (port < 1024 || port > 65535))
    return false;
  else
#endif
  if (port <= 0 || port > 65535)
    return false;

  return true;
}

int CUtil::GetRandomNumber()
{
#ifdef TARGET_WINDOWS
  unsigned int number;
  if (rand_s(&number) == 0)
    return (int)number;
#else
  return rand_r(&s_randomSeed);
#endif

  return rand();
}

