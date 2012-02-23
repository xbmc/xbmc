/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */
#include "threads/SystemClock.h"
#include "system.h"
#ifdef __APPLE__
#include <sys/param.h>
#include <mach-o/dyld.h>
#endif

#if defined(__FreeBSD__)
#include <sys/param.h>
#include <sys/sysctl.h>
#endif

#ifdef _LINUX
#include <sys/types.h>
#include <dirent.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#endif

#include "Application.h"
#include "utils/AutoPtrHandle.h"
#include "Util.h"
#include "addons/Addon.h"
#include "storage/IoSupport.h"
#include "filesystem/PVRDirectory.h"
#include "filesystem/StackDirectory.h"
#include "filesystem/MultiPathDirectory.h"
#include "filesystem/DirectoryCache.h"
#include "filesystem/SpecialProtocol.h"
#include "filesystem/RSSDirectory.h"
#include "ThumbnailCache.h"
#ifdef HAS_FILESYSTEM_RAR
#include "filesystem/RarManager.h"
#endif
#include "filesystem/MythDirectory.h"
#ifdef HAS_UPNP
#include "filesystem/UPnPDirectory.h"
#endif
#ifdef HAS_VIDEO_PLAYBACK
#include "cores/VideoRenderers/RenderManager.h"
#endif
#include "utils/RegExp.h"
#include "settings/GUISettings.h"
#include "guilib/TextureManager.h"
#include "utils/fstrcmp.h"
#include "storage/MediaManager.h"
#include "guilib/DirectXGraphics.h"
#include "network/DNSNameCache.h"
#include "guilib/GUIWindowManager.h"
#ifdef _WIN32
#include <shlobj.h>
#include "WIN32Util.h"
#endif
#if defined(__APPLE__)
#include "osx/DarwinUtils.h"
#endif
#include "GUIUserMessages.h"
#include "filesystem/File.h"
#include "utils/Crc32.h"
#include "settings/Settings.h"
#include "utils/StringUtils.h"
#include "settings/AdvancedSettings.h"
#ifdef HAS_IRSERVERSUITE
  #include "input/windows/IRServerSuite.h"
#endif
#include "guilib/LocalizeStrings.h"
#include "utils/md5.h"
#include "utils/TimeUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#include "pictures/Picture.h"
#include "utils/JobManager.h"
#include "cores/dvdplayer/DVDSubtitles/DVDSubtitleTagSami.h"
#include "cores/dvdplayer/DVDSubtitles/DVDSubtitleStream.h"
#include "windowing/WindowingFactory.h"
#include "video/VideoInfoTag.h"

using namespace std;
using namespace XFILE;

#define clamp(x) (x) > 255.f ? 255 : ((x) < 0 ? 0 : (BYTE)(x+0.5f)) // Valid ranges: brightness[-1 -> 1 (0 is default)] contrast[0 -> 2 (1 is default)]  gamma[0.5 -> 3.5 (1 is default)] default[ramp is linear]
static const __int64 SECS_BETWEEN_EPOCHS = 11644473600LL;
static const __int64 SECS_TO_100NS = 10000000;

using namespace AUTOPTR;
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

CUtil::CUtil(void)
{
}

CUtil::~CUtil(void)
{}

CStdString CUtil::GetTitleFromPath(const CStdString& strFileNameAndPath, bool bIsFolder /* = false */)
{
  // use above to get the filename
  CStdString path(strFileNameAndPath);
  URIUtils::RemoveSlashAtEnd(path);
  CStdString strFilename = URIUtils::GetFileName(path);

  CURL url(strFileNameAndPath);
  CStdString strHostname = url.GetHostName();

#ifdef HAS_UPNP
  // UPNP
  if (url.GetProtocol() == "upnp")
    strFilename = CUPnPDirectory::GetFriendlyName(strFileNameAndPath.c_str());
#endif

  if (url.GetProtocol() == "rss")
  {
    CRSSDirectory dir;
    CFileItemList items;
    if(dir.GetDirectory(strFileNameAndPath, items) && !items.m_strTitle.IsEmpty())
      return items.m_strTitle;
  }

  // LastFM
  if (url.GetProtocol() == "lastfm")
  {
    if (strFilename.IsEmpty())
      strFilename = g_localizeStrings.Get(15200);
    else
      strFilename = g_localizeStrings.Get(15200) + " - " + strFilename;
  }

  // Shoutcast
  else if (url.GetProtocol() == "shout")
  {
    const int genre = strFileNameAndPath.find_first_of('=');
    if(genre <0)
      strFilename = g_localizeStrings.Get(260);
    else
      strFilename = g_localizeStrings.Get(260) + " - " + strFileNameAndPath.substr(genre+1).c_str();
  }

  // Windows SMB Network (SMB)
  else if (url.GetProtocol() == "smb" && strFilename.IsEmpty())
  {
    if (url.GetHostName().IsEmpty())
    {
      strFilename = g_localizeStrings.Get(20171);
    }
    else
    {
      strFilename = url.GetHostName();
    }
  }
  // iTunes music share (DAAP)
  else if (url.GetProtocol() == "daap" && strFilename.IsEmpty())
    strFilename = g_localizeStrings.Get(20174);

  // HDHomerun Devices
  else if (url.GetProtocol() == "hdhomerun" && strFilename.IsEmpty())
    strFilename = "HDHomerun Devices";

  // Slingbox Devices
  else if (url.GetProtocol() == "sling")
    strFilename = "Slingbox";

  // ReplayTV Devices
  else if (url.GetProtocol() == "rtv")
    strFilename = "ReplayTV Devices";

  // HTS Tvheadend client
  else if (url.GetProtocol() == "htsp")
    strFilename = g_localizeStrings.Get(20256);

  // VDR Streamdev client
  else if (url.GetProtocol() == "vtp")
    strFilename = g_localizeStrings.Get(20257);
  
  // MythTV client
  else if (url.GetProtocol() == "myth")
    strFilename = g_localizeStrings.Get(20258);

  // SAP Streams
  else if (url.GetProtocol() == "sap" && strFilename.IsEmpty())
    strFilename = "SAP Streams";

  // Root file views
  else if (url.GetProtocol() == "sources")
    strFilename = g_localizeStrings.Get(744);

  // Music Playlists
  else if (path.Left(24).Equals("special://musicplaylists"))
    strFilename = g_localizeStrings.Get(136);

  // Video Playlists
  else if (path.Left(24).Equals("special://videoplaylists"))
    strFilename = g_localizeStrings.Get(136);

  else if (URIUtils::ProtocolHasParentInHostname(url.GetProtocol()) && strFilename.IsEmpty())
    strFilename = URIUtils::GetFileName(url.GetHostName());

  // now remove the extension if needed
  if (!g_guiSettings.GetBool("filelists.showextensions") && !bIsFolder)
  {
    URIUtils::RemoveExtension(strFilename);
    return strFilename;
  }
  
  // URLDecode since the original path may be an URL
  CURL::Decode(strFilename);
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
        URIUtils::RemoveExtension(strFileNoExt);
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

void CUtil::CleanString(const CStdString& strFileName, CStdString& strTitle, CStdString& strTitleAndYear, CStdString& strYear, bool bRemoveExtension /* = false */, bool bCleanChars /* = true */)
{
  strTitleAndYear = strFileName;

  if (strFileName.Equals(".."))
   return;

  const CStdStringArray &regexps = g_advancedSettings.m_videoCleanStringRegExps;

  CRegExp reTags(true);
  CRegExp reYear;
  CStdString strExtension;
  URIUtils::GetExtension(strFileName, strExtension);

  if (!reYear.RegComp(g_advancedSettings.m_videoCleanDateTimeRegExp))
  {
    CLog::Log(LOGERROR, "%s: Invalid datetime clean RegExp:'%s'", __FUNCTION__, g_advancedSettings.m_videoCleanDateTimeRegExp.c_str());
  }
  else
  {
    if (reYear.RegFind(strTitleAndYear.c_str()) >= 0)
    {
      strTitleAndYear = reYear.GetReplaceString("\\1");
      strYear = reYear.GetReplaceString("\\2");
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
    if ((j=reTags.RegFind(strFileName.c_str())) > 0)
      strTitleAndYear = strTitleAndYear.Mid(0, j);
  }

  // final cleanup - special characters used instead of spaces:
  // all '_' tokens should be replaced by spaces
  // if the file contains no spaces, all '.' tokens should be replaced by
  // spaces - one possibility of a mistake here could be something like:
  // "Dr..StrangeLove" - hopefully no one would have anything like this.
  if (bCleanChars)
  {
    bool initialDots = true;
    bool alreadyContainsSpace = (strTitleAndYear.Find(' ') >= 0);

    for (int i = 0; i < (int)strTitleAndYear.size(); i++)
    {
      char c = strTitleAndYear.GetAt(i);

      if (c != '.')
        initialDots = false;

      if ((c == '_') || ((!alreadyContainsSpace) && !initialDots && (c == '.')))
      {
        strTitleAndYear.SetAt(i, ' ');
      }
    }
  }

  strTitle = strTitleAndYear.Trim();

  // append year
  if (!strYear.IsEmpty())
    strTitleAndYear = strTitle + " (" + strYear + ")";

  // restore extension if needed
  if (!bRemoveExtension)
    strTitleAndYear += strExtension;
}

void CUtil::GetQualifiedFilename(const CStdString &strBasePath, CStdString &strFilename)
{
  // Check if the filename is a fully qualified URL such as protocol://path/to/file
  CURL plItemUrl(strFilename);
  if (!plItemUrl.GetProtocol().IsEmpty())
    return;

  // If the filename starts "x:", "\\" or "/" it's already fully qualified so return
  if (strFilename.size() > 1)
#ifdef _LINUX
    if ( (strFilename[1] == ':') || (strFilename[0] == '/') )
#else
    if ( strFilename[1] == ':' || (strFilename[0] == '\\' && strFilename[1] == '\\'))
#endif
      return;

  // add to base path and then clean
  strFilename = URIUtils::AddFileToFolder(strBasePath, strFilename);

  // get rid of any /./ or \.\ that happen to be there
  strFilename.Replace("\\.\\", "\\");
  strFilename.Replace("/./", "/");

  // now find any "\\..\\" and remove them via GetParentPath
  int pos;
  while ((pos = strFilename.Find("/../")) > 0)
  {
    CStdString basePath = strFilename.Left(pos+1);
    strFilename = strFilename.Mid(pos+4);
    basePath = URIUtils::GetParentPath(basePath);
    strFilename = URIUtils::AddFileToFolder(basePath, strFilename);
  }
  while ((pos = strFilename.Find("\\..\\")) > 0)
  {
    CStdString basePath = strFilename.Left(pos+1);
    strFilename = strFilename.Mid(pos+4);
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
#ifdef _WIN32
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

void CUtil::GetHomePath(CStdString& strPath, const CStdString& strTarget)
{
  CStdString strHomePath;
  strHomePath = ResolveExecutablePath();
#ifdef _WIN32
  CStdStringW strPathW, strTargetW;
  g_charsetConverter.utf8ToW(strTarget, strTargetW);
  strPathW = _wgetenv(strTargetW);
  g_charsetConverter.wToUTF8(strPathW,strPath);
#else
  strPath = getenv(strTarget);
#endif

  if (strPath != NULL && !strPath.IsEmpty())
  {
#ifdef _WIN32
    char tmp[1024];
    //expand potential relative path to full path
    if(GetFullPathName(strPath, 1024, tmp, 0) != 0)
    {
      strPath = tmp;
    }
#endif
  }
  else
  {
#ifdef __APPLE__
    int      result = -1;
    char     given_path[2*MAXPATHLEN];
    uint32_t path_size =2*MAXPATHLEN;

    result = GetDarwinExecutablePath(given_path, &path_size);
    if (result == 0)
    {
      // Move backwards to last /.
      for (int n=strlen(given_path)-1; given_path[n] != '/'; n--)
        given_path[n] = '\0';

      #if defined(__arm__)
        strcat(given_path, "/XBMCData/XBMCHome/");
      #else
        // Assume local path inside application bundle.
        strcat(given_path, "../Resources/XBMC/");
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
      strPath = strHomePath.Left(last_sep);
    else
      strPath = strHomePath;
  }

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

bool CUtil::IsPVR(const CStdString& strFile)
{
  return strFile.Left(4).Equals("pvr:");
}

bool CUtil::IsHTSP(const CStdString& strFile)
{
  return strFile.Left(5).Equals("htsp:");
}

bool CUtil::IsLiveTV(const CStdString& strFile)
{
  if (strFile.Left(14).Equals("pvr://channels"))
    return true;

  if(URIUtils::IsTuxBox(strFile)
  || URIUtils::IsVTP(strFile)
  || URIUtils::IsHDHomeRun(strFile)
  || URIUtils::IsHTSP(strFile)
  || strFile.Left(4).Equals("sap:"))
    return true;

  if (URIUtils::IsMythTV(strFile) && CMythDirectory::IsLiveTV(strFile))
    return true;

  return false;
}

bool CUtil::IsTVRecording(const CStdString& strFile)
{
  return strFile.Left(15).Equals("pvr://recording");
}

bool CUtil::IsPicture(const CStdString& strFile)
{
  CStdString extension = URIUtils::GetExtension(strFile);

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
  strDir.Format("%s://%s", url.GetProtocol().c_str(), url.GetFileName().c_str());
}

int CUtil::GetDVDIfoTitle(const CStdString& strFile)
{
  CStdString strFilename = URIUtils::GetFileName(strFile);
  if (strFilename.Equals("video_ts.ifo")) return 0;
  //VTS_[TITLE]_0.IFO
  return atoi(strFilename.Mid(4, 2).c_str());
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
  strDescription = strFileName.Left(strFileName.size() - strFName.size());
  URIUtils::RemoveSlashAtEnd(strDescription);

  int iPos = strDescription.ReverseFind("\\");
  if (iPos < 0)
    iPos = strDescription.ReverseFind("/");
  if (iPos >= 0)
  {
    CStdString strTmp = strDescription.Right(strDescription.size()-iPos-1);
    strDescription = strTmp;//strDescription.Right(strDescription.size() - iPos - 1);
  }
  else if (strDescription.size() <= 0)
    strDescription = strFName;
  return true;
}

void CUtil::GetDVDDriveIcon( const CStdString& strPath, CStdString& strIcon )
{
  if ( !g_mediaManager.IsDiscInDrive() )
  {
    strIcon = "DefaultDVDEmpty.png";
    return ;
  }

  if ( URIUtils::IsDVD(strPath) )
  {
#ifdef HAS_DVD_DRIVE
    CCdInfo* pInfo = g_mediaManager.GetCdInfo();
    //  xbox DVD
    if ( pInfo != NULL && pInfo->IsUDFX( 1 ) )
    {
      strIcon = "DefaultXboxDVD.png";
      return ;
    }
#endif
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
  CStdString searchPath = g_settings.GetDatabaseFolder();
  CFileItemList items;
  if (!XFILE::CDirectory::GetDirectory(searchPath, items, ".tmp", false))
    return;

  for (int i = 0; i < items.Size(); ++i)
  {
    if (items[i]->m_bIsFolder)
      continue;
    XFILE::CFile::Delete(items[i]->GetPath());
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
      if ( items[i]->GetPath().Find("subtitle") >= 0 || items[i]->GetPath().Find("vobsub_queue") >= 0 )
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

  if (!CFile::Exists(searchPath))
    return;

  CFileItemList items;
  CDirectory::GetDirectory(searchPath, items, "", false, false, XFILE::DIR_CACHE_NEVER);

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

CStdString CUtil::GetNextFilename(const CStdString &fn_template, int max)
{
  if (!fn_template.Find("%03d"))
    return "";

  CStdString searchPath;
  URIUtils::GetDirectory(fn_template, searchPath);
  CStdString mask = URIUtils::GetExtension(fn_template);

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
  {
#ifdef HAS_GLES
    // we need to save in BGRA order so XOR Swap RGBA -> BGRA
    unsigned char* swap_pixels = pixels + (height - y - 1) * stride;
    for (int x = 0; x < width; x++, swap_pixels+=4)
    {
      std::swap(swap_pixels[0], swap_pixels[2]);
    }   
#endif
    memcpy(outpixels + y * stride, pixels + (height - y - 1) * stride, stride);
  }

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
  URIUtils::RemoveSlashAtEnd(strDir);

  if (!strDir.IsEmpty())
  {
    CStdString file = CUtil::GetNextFilename(URIUtils::AddFileToFolder(strDir, "screenshot%03d.png"), 999);

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
            CStdString file = CUtil::GetNextFilename(URIUtils::AddFileToFolder(newDir, "screenshot%03d.png"), 999);
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
  }
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

#ifndef _LINUX
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
#ifndef _LINUX
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
#ifndef _LINUX
  result->st_atime = stat->st_atime;
  result->st_mtime = stat->st_mtime;
  result->st_ctime = stat->st_ctime;
#else
  result->st_atime = stat->_st_atime;
  result->st_mtime = stat->_st_mtime;
  result->st_ctime = stat->_st_ctime;
#endif
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
#ifndef _LINUX
  if (stat->st_size <= LONG_MAX)
    result->st_size = (_off_t)stat->st_size;
#else
  if (sizeof(stat->st_size) <= sizeof(result->st_size) )
    result->st_size = (off_t)stat->st_size;
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

#ifdef _WIN32
void CUtil::Stat64ToStat64i32(struct _stat64i32 *result, struct __stat64 *stat)
{
  result->st_dev = stat->st_dev;
  result->st_ino = stat->st_ino;
  result->st_mode = stat->st_mode;
  result->st_nlink = stat->st_nlink;
  result->st_uid = stat->st_uid;
  result->st_gid = stat->st_gid;
  result->st_rdev = stat->st_rdev;
#ifndef _LINUX
  if (stat->st_size <= LONG_MAX)
    result->st_size = (_off_t)stat->st_size;
#else
  if (sizeof(stat->st_size) <= sizeof(result->st_size) )
    result->st_size = (off_t)stat->st_size;
#endif
  else
  {
    result->st_size = 0;
    CLog::Log(LOGWARNING, "WARNING: File is larger than 32bit stat can handle, file size will be reported as 0 bytes");
  }
#ifndef _LINUX
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

  CStdStringArray dirs = URIUtils::SplitPath(strPath);
  CStdString dir(dirs.front());
  URIUtils::AddSlashAtEnd(dir);
  for (CStdStringArray::iterator it = dirs.begin() + 1; it != dirs.end(); it ++)
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

  result.Replace('/', '_');
  result.Replace('\\', '_');
  result.Replace('?', '_');

  if (LegalType == LEGAL_WIN32_COMPAT)
  {
    // just filter out some illegal characters on windows
    result.Replace(':', '_');
    result.Replace('*', '_');
    result.Replace('?', '_');
    result.Replace('\"', '_');
    result.Replace('<', '_');
    result.Replace('>', '_');
    result.Replace('|', '_');
    result.TrimRight(".");
    result.TrimRight(" ");
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
  CStdStringArray dirs = URIUtils::SplitPath(strPathAndFile);
  // we just add first token to path and don't legalize it - possible values: 
  // "X:" (local win32), "" (local unix - empty string before '/') or
  // "protocol://domain"
  CStdString dir(dirs.front());
  URIUtils::AddSlashAtEnd(dir);
  for (CStdStringArray::iterator it = dirs.begin() + 1; it != dirs.end(); it ++)
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
     (path.Find('%') >= 0 ||
      path.Left(4).Equals("zip:") ||
      path.Left(4).Equals("rar:") ||
      path.Left(6).Equals("stack:") ||
      path.Left(10).Equals("multipath:") ))
    return result;

  // check the path for incorrect slashes
#ifdef _WIN32
  if (URIUtils::IsDOSPath(path))
  {
    result.Replace('/', '\\');
    /* The double slash correction should only be used when *absolutely*
       necessary! This applies to certain DLLs or use from Python DLLs/scripts
       that incorrectly generate double (back) slashes.
    */
    if (bFixDoubleSlashes)
    {
      // Fixup for double back slashes (but ignore the \\ of unc-paths)
      for (int x = 1; x < result.GetLength() - 1; x++)
      {
        if (result[x] == '\\' && result[x+1] == '\\')
          result.Delete(x);
      }
    }
  }
  else if (path.Find("://") >= 0 || path.Find(":\\\\") >= 0)
#endif
  {
    result.Replace('\\', '/');
    /* The double slash correction should only be used when *absolutely*
       necessary! This applies to certain DLLs or use from Python DLLs/scripts
       that incorrectly generate double (back) slashes.
    */
    if (bFixDoubleSlashes)
    {
      // Fixup for double forward slashes(/) but don't touch the :// of URLs
      for (int x = 2; x < result.GetLength() - 1; x++)
      {
        if ( result[x] == '/' && result[x + 1] == '/' && !(result[x - 1] == ':' || (result[x - 1] == '/' && result[x - 2] == ':')) )
          result.Delete(x);
      }
    }
  }
  return result;
}

bool CUtil::IsUsingTTFSubtitles()
{
  return URIUtils::GetExtension(g_guiSettings.GetString("subtitles.font")).Equals(".ttf");
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

void CUtil::SplitExecFunction(const CStdString &execString, CStdString &function, vector<CStdString> &parameters)
{
  CStdString paramString;

  int iPos = execString.Find("(");
  int iPos2 = execString.ReverseFind(")");
  if (iPos > 0 && iPos2 > 0)
  {
    paramString = execString.Mid(iPos + 1, iPos2 - iPos - 1);
    function = execString.Left(iPos);
  }
  else
    function = execString;

  // remove any whitespace, and the standard prefix (if it exists)
  function.Trim();
  if( function.Left(5).Equals("xbmc.", false) )
    function.Delete(0, 5);

  SplitParams(paramString, parameters);
}

void CUtil::SplitParams(const CStdString &paramString, std::vector<CStdString> &parameters)
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
      if (ch == '\"' && !escaped)
      { // finished a quote - no need to add the end quote to our string
        inQuotes = false;
      }
    }
    else
    { // not in a quote, so check if we should be starting one
      if (ch == '\"' && !escaped)
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
          parameter = parameter.Left(whiteSpacePos);
        // trim off start and end quotes
        if (parameter.GetLength() > 1 && parameter[0] == '\"' && parameter[parameter.GetLength() - 1] == '\"')
          parameter = parameter.Mid(1,parameter.GetLength() - 2);
        parameters.push_back(parameter);
        parameter.Empty();
        whiteSpacePos = 0;
        continue;
      }
    }
    if ((ch == '\"' || ch == '\\') && escaped)
    { // escaped quote or backslash
      parameter[parameter.size()-1] = ch;
      continue;
    }
    // whitespace handling - we skip any whitespace at the left or right of an unquoted parameter
    if (ch == ' ' && !inQuotes)
    {
      if (parameter.IsEmpty()) // skip whitespace on left
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
    parameter = parameter.Left(whiteSpacePos);
  // trim off start and end quotes
  if (parameter.GetLength() > 1 && parameter[0] == '\"' && parameter[parameter.GetLength() - 1] == '\"')
    parameter = parameter.Mid(1,parameter.GetLength() - 2);
  if (!parameter.IsEmpty() || parameters.size())
    parameters.push_back(parameter);
}

int CUtil::GetMatchingSource(const CStdString& strPath1, VECSOURCES& VECSOURCES, bool& bIsSourceName)
{
  if (strPath1.IsEmpty())
    return -1;

  //CLog::Log(LOGDEBUG,"CUtil::GetMatchingSource, testing original path/name [%s]", strPath1.c_str());

  // copy as we may change strPath
  CStdString strPath = strPath1;

  // Check for special protocols
  CURL checkURL(strPath);

  // stack://
  if (checkURL.GetProtocol() == "stack")
    strPath.Delete(0, 8); // remove the stack protocol

  if (checkURL.GetProtocol() == "shout")
    strPath = checkURL.GetHostName();
  if (checkURL.GetProtocol() == "lastfm")
    return 1;
  if (checkURL.GetProtocol() == "tuxbox")
    return 1;
  if (checkURL.GetProtocol() == "plugin")
    return 1;
  if (checkURL.GetProtocol() == "multipath")
    strPath = CMultiPathDirectory::GetFirstPath(strPath);

  //CLog::Log(LOGDEBUG,"CUtil::GetMatchingSource, testing for matching name [%s]", strPath.c_str());
  bIsSourceName = false;
  int iIndex = -1;
  int iLength = -1;
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
      int iPos = strName.ReverseFind('(');
      if (iPos > 1)
        strName = strName.Mid(0, iPos - 1);
    }
    //CLog::Log(LOGDEBUG,"CUtil::GetMatchingSource, comparing name [%s]", strName.c_str());
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
  int iLenPath = strDest.size();

  //CLog::Log(LOGDEBUG,"CUtil::GetMatchingSource, testing url [%s]", strDest.c_str());

  for (int i = 0; i < (int)VECSOURCES.size(); ++i)
  {
    CMediaSource share = VECSOURCES.at(i);

    // does it match a source name?
    if (share.strPath.substr(0,8) == "shout://")
    {
      CURL url(share.strPath);
      if (strPath.Equals(url.GetHostName()))
        return i;
    }

    // doesnt match a name, so try the source path
    vector<CStdString> vecPaths;

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
      int iLenShare = strShare.size();
      //CLog::Log(LOGDEBUG,"CUtil::GetMatchingSource, comparing url [%s]", strShare.c_str());

      if ((iLenPath >= iLenShare) && (strDest.Left(iLenShare).Equals(strShare)) && (iLenShare > iLength))
      {
        //CLog::Log(LOGDEBUG,"Found matching source at index %i: [%s], Len = [%i]", i, strShare.c_str(), iLenShare);

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
    if( strPath.Left(6).Equals("rar://") || strPath.Left(6).Equals("zip://") )
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
  CStdString strReturn=strSpecial;
  if (!strSpecial.IsEmpty() && strSpecial[0] == '$')
  {
    if (strSpecial.Left(5).Equals("$HOME"))
      URIUtils::AddFileToFolder("special://home/", strSpecial.Mid(5), strReturn);
    else if (strSpecial.Left(10).Equals("$SUBTITLES"))
      URIUtils::AddFileToFolder("special://subtitles/", strSpecial.Mid(10), strReturn);
    else if (strSpecial.Left(9).Equals("$USERDATA"))
      URIUtils::AddFileToFolder("special://userdata/", strSpecial.Mid(9), strReturn);
    else if (strSpecial.Left(9).Equals("$DATABASE"))
      URIUtils::AddFileToFolder("special://database/", strSpecial.Mid(9), strReturn);
    else if (strSpecial.Left(11).Equals("$THUMBNAILS"))
      URIUtils::AddFileToFolder("special://thumbnails/", strSpecial.Mid(11), strReturn);
    else if (strSpecial.Left(11).Equals("$RECORDINGS"))
      URIUtils::AddFileToFolder("special://recordings/", strSpecial.Mid(11), strReturn);
    else if (strSpecial.Left(12).Equals("$SCREENSHOTS"))
      URIUtils::AddFileToFolder("special://screenshots/", strSpecial.Mid(12), strReturn);
    else if (strSpecial.Left(15).Equals("$MUSICPLAYLISTS"))
      URIUtils::AddFileToFolder("special://musicplaylists/", strSpecial.Mid(15), strReturn);
    else if (strSpecial.Left(15).Equals("$VIDEOPLAYLISTS"))
      URIUtils::AddFileToFolder("special://videoplaylists/", strSpecial.Mid(15), strReturn);
    else if (strSpecial.Left(7).Equals("$CDRIPS"))
      URIUtils::AddFileToFolder("special://cdrips/", strSpecial.Mid(7), strReturn);
    // this one will be removed post 2.0
    else if (strSpecial.Left(10).Equals("$PLAYLISTS"))
      URIUtils::AddFileToFolder(g_guiSettings.GetString("system.playlistspath",false), strSpecial.Mid(10), strReturn);
  }
  return strReturn;
}

CStdString CUtil::MusicPlaylistsLocation()
{
  vector<CStdString> vec;
  CStdString strReturn;
  URIUtils::AddFileToFolder(g_guiSettings.GetString("system.playlistspath"), "music", strReturn);
  vec.push_back(strReturn);
  URIUtils::AddFileToFolder(g_guiSettings.GetString("system.playlistspath"), "mixed", strReturn);
  vec.push_back(strReturn);
  return XFILE::CMultiPathDirectory::ConstructMultiPath(vec);;
}

CStdString CUtil::VideoPlaylistsLocation()
{
  vector<CStdString> vec;
  CStdString strReturn;
  URIUtils::AddFileToFolder(g_guiSettings.GetString("system.playlistspath"), "video", strReturn);
  vec.push_back(strReturn);
  URIUtils::AddFileToFolder(g_guiSettings.GetString("system.playlistspath"), "mixed", strReturn);
  vec.push_back(strReturn);
  return XFILE::CMultiPathDirectory::ConstructMultiPath(vec);;
}

void CUtil::DeleteMusicDatabaseDirectoryCache()
{
  CUtil::DeleteDirectoryCache("mdb-");
}

void CUtil::DeleteVideoDatabaseDirectoryCache()
{
  CUtil::DeleteDirectoryCache("vdb-");
}

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
    CStdString fileName = URIUtils::GetFileName(items[i]->GetPath());
    if (fileName.Left(prefix.GetLength()) == prefix)
      XFILE::CFile::Delete(items[i]->GetPath());
  }
}

bool CUtil::SetSysDateTimeYear(int iYear, int iMonth, int iDay, int iHour, int iMinute)
{
  TIME_ZONE_INFORMATION tziNew;
  SYSTEMTIME CurTime;
  SYSTEMTIME NewTime;
  GetLocalTime(&CurTime);
  GetLocalTime(&NewTime);
  int iRescBiases, iHourUTC;
  int iMinuteNew;

  DWORD dwRet = GetTimeZoneInformation(&tziNew);  // Get TimeZone Informations
  float iGMTZone = (float(tziNew.Bias)/(60));     // Calc's the GMT Time

  CLog::Log(LOGDEBUG, "------------ TimeZone -------------");
  CLog::Log(LOGDEBUG, "-      GMT Zone: GMT %.1f",iGMTZone);
  CLog::Log(LOGDEBUG, "-          Bias: %lu minutes",tziNew.Bias);
  CLog::Log(LOGDEBUG, "-  DaylightBias: %lu",tziNew.DaylightBias);
  CLog::Log(LOGDEBUG, "-  StandardBias: %lu",tziNew.StandardBias);

  switch (dwRet)
  {
    case TIME_ZONE_ID_STANDARD:
      {
        iRescBiases   = tziNew.Bias + tziNew.StandardBias;
        CLog::Log(LOGDEBUG, "-   Timezone ID: 1, Standart");
      }
      break;
    case TIME_ZONE_ID_DAYLIGHT:
      {
        iRescBiases   = tziNew.Bias + tziNew.StandardBias + tziNew.DaylightBias;
        CLog::Log(LOGDEBUG, "-   Timezone ID: 2, Daylight");
      }
      break;
    case TIME_ZONE_ID_UNKNOWN:
      {
        iRescBiases   = tziNew.Bias + tziNew.StandardBias;
        CLog::Log(LOGDEBUG, "-   Timezone ID: 0, Unknown");
      }
      break;
    case TIME_ZONE_ID_INVALID:
      {
        iRescBiases   = tziNew.Bias + tziNew.StandardBias;
        CLog::Log(LOGDEBUG, "-   Timezone ID: Invalid");
      }
      break;
    default:
      iRescBiases   = tziNew.Bias + tziNew.StandardBias;
  }
    CLog::Log(LOGDEBUG, "--------------- END ---------------");

  // Calculation
  iHourUTC = GMTZoneCalc(iRescBiases, iHour, iMinute, iMinuteNew);
  iMinute = iMinuteNew;
  if(iHourUTC <0)
  {
    iDay = iDay - 1;
    iHourUTC =iHourUTC + 24;
  }
  if(iHourUTC >23)
  {
    iDay = iDay + 1;
    iHourUTC =iHourUTC - 24;
  }

  // Set the New-,Detected Time Values to System Time!
  NewTime.wYear     = (WORD)iYear;
  NewTime.wMonth    = (WORD)iMonth;
  NewTime.wDay      = (WORD)iDay;
  NewTime.wHour     = (WORD)iHourUTC;
  NewTime.wMinute   = (WORD)iMinute;

  FILETIME stNewTime, stCurTime;
  SystemTimeToFileTime(&NewTime, &stNewTime);
  SystemTimeToFileTime(&CurTime, &stCurTime);
  return false;
}
int CUtil::GMTZoneCalc(int iRescBiases, int iHour, int iMinute, int &iMinuteNew)
{
  int iHourUTC, iTemp;
  iMinuteNew = iMinute;
  iTemp = iRescBiases/60;

  if (iRescBiases == 0 )return iHour;   // GMT Zone 0, no need calculate
  if (iRescBiases > 0)
    iHourUTC = iHour + abs(iTemp);
  else
    iHourUTC = iHour - abs(iTemp);

  if ((iTemp*60) != iRescBiases)
  {
    if (iRescBiases > 0)
      iMinuteNew = iMinute + abs(iTemp*60 - iRescBiases);
    else
      iMinuteNew = iMinute - abs(iTemp*60 - iRescBiases);

    if (iMinuteNew >= 60)
    {
      iMinuteNew = iMinuteNew -60;
      iHourUTC = iHourUTC + 1;
    }
    else if (iMinuteNew < 0)
    {
      iMinuteNew = iMinuteNew +60;
      iHourUTC = iHourUTC - 1;
    }
  }
  return iHourUTC;
}

void CUtil::GetRecursiveListing(const CStdString& strPath, CFileItemList& items, const CStdString& strMask, bool bUseFileDirectories)
{
  CFileItemList myItems;
  CDirectory::GetDirectory(strPath,myItems,strMask,bUseFileDirectories);
  for (int i=0;i<myItems.Size();++i)
  {
    if (myItems[i]->m_bIsFolder)
      CUtil::GetRecursiveListing(myItems[i]->GetPath(),items,strMask,bUseFileDirectories);
    else
      items.Add(myItems[i]);
  }
}

void CUtil::GetRecursiveDirsListing(const CStdString& strPath, CFileItemList& item)
{
  CFileItemList myItems;
  CDirectory::GetDirectory(strPath,myItems,"",false);
  for (int i=0;i<myItems.Size();++i)
  {
    if (myItems[i]->m_bIsFolder && !myItems[i]->GetPath().Equals(".."))
    {
      item.Add(myItems[i]);
      CUtil::GetRecursiveDirsListing(myItems[i]->GetPath(),item);
    }
  }
}

void CUtil::ForceForwardSlashes(CStdString& strPath)
{
  int iPos = strPath.ReverseFind('\\');
  while (iPos > 0)
  {
    strPath.at(iPos) = '/';
    iPos = strPath.ReverseFind('\\');
  }
}

double CUtil::AlbumRelevance(const CStdString& strAlbumTemp1, const CStdString& strAlbum1, const CStdString& strArtistTemp1, const CStdString& strArtist1)
{
  // case-insensitive fuzzy string comparison on the album and artist for relevance
  // weighting is identical, both album and artist are 50% of the total relevance
  // a missing artist means the maximum relevance can only be 0.50
  CStdString strAlbumTemp = strAlbumTemp1;
  strAlbumTemp.MakeLower();
  CStdString strAlbum = strAlbum1;
  strAlbum.MakeLower();
  double fAlbumPercentage = fstrcmp(strAlbumTemp, strAlbum, 0.0f);
  double fArtistPercentage = 0.0f;
  if (!strArtist1.IsEmpty())
  {
    CStdString strArtistTemp = strArtistTemp1;
    strArtistTemp.MakeLower();
    CStdString strArtist = strArtist1;
    strArtist.MakeLower();
    fArtistPercentage = fstrcmp(strArtistTemp, strArtist, 0.0f);
  }
  double fRelevance = fAlbumPercentage * 0.5f + fArtistPercentage * 0.5f;
  return fRelevance;
}

bool CUtil::MakeShortenPath(CStdString StrInput, CStdString& StrOutput, int iTextMaxLength)
{
  int iStrInputSize = StrInput.size();
  if((iStrInputSize <= 0) || (iTextMaxLength >= iStrInputSize))
    return false;

  char cDelim = '\0';
  size_t nGreaterDelim, nPos;

  nPos = StrInput.find_last_of( '\\' );
  if ( nPos != CStdString::npos )
    cDelim = '\\';
  else
  {
    nPos = StrInput.find_last_of( '/' );
    if ( nPos != CStdString::npos )
      cDelim = '/';
  }
  if ( cDelim == '\0' )
    return false;

  if (nPos == StrInput.size() - 1)
  {
    StrInput.erase(StrInput.size() - 1);
    nPos = StrInput.find_last_of( cDelim );
  }
  while( iTextMaxLength < iStrInputSize )
  {
    nPos = StrInput.find_last_of( cDelim, nPos );
    nGreaterDelim = nPos;
    if ( nPos != CStdString::npos )
      nPos = StrInput.find_last_of( cDelim, nPos - 1 );
    if ( nPos == CStdString::npos ) break;
    if ( nGreaterDelim > nPos ) StrInput.replace( nPos + 1, nGreaterDelim - nPos - 1, ".." );
    iStrInputSize = StrInput.size();
  }
  // replace any additional /../../ with just /../ if necessary
  CStdString replaceDots;
  replaceDots.Format("..%c..", cDelim);
  while (StrInput.size() > (unsigned int)iTextMaxLength)
    if (!StrInput.Replace(replaceDots, ".."))
      break;
  // finally, truncate our string to force inside our max text length,
  // replacing the last 2 characters with ".."

  // eg end up with:
  // "smb://../Playboy Swimsuit Cal.."
  if (iTextMaxLength > 2 && StrInput.size() > (unsigned int)iTextMaxLength)
  {
    StrInput = StrInput.Left(iTextMaxLength - 2);
    StrInput += "..";
  }
  StrOutput = StrInput;
  return true;
}

bool CUtil::SupportsFileOperations(const CStdString& strPath)
{
  // currently only hd, smb, nfs and afp support delete and rename
  if (URIUtils::IsHD(strPath))
    return true;
  if (URIUtils::IsSmb(strPath))
    return true;
  if (CUtil::IsTVRecording(strPath))
    return CPVRDirectory::SupportsFileOperations(strPath);
  if (URIUtils::IsNfs(strPath))
    return true;
  if (URIUtils::IsAfp(strPath))
    return true;
  if (URIUtils::IsMythTV(strPath))
  {
    /*
     * Can't use CFile::Exists() to check whether the myth:// path supports file operations because
     * it hits the directory cache on the way through, which has the Live Channels and Guide
     * items cached.
     */
    return CMythDirectory::SupportsFileOperations(strPath);
  }
  if (URIUtils::IsStack(strPath))
    return SupportsFileOperations(CStackDirectory::GetFirstStackedFile(strPath));
  if (URIUtils::IsMultiPath(strPath))
    return CMultiPathDirectory::SupportsFileOperations(strPath);

  return false;
}

CStdString CUtil::GetDefaultFolderThumb(const CStdString &folderThumb)
{
  if (g_TextureManager.HasTexture(folderThumb))
    return folderThumb;
  return "";
}

void CUtil::GetSkinThemes(vector<CStdString>& vecTheme)
{
  CStdString strPath;
  URIUtils::AddFileToFolder(g_graphicsContext.GetMediaDir(),"media",strPath);
  CFileItemList items;
  CDirectory::GetDirectory(strPath, items);
  // Search for Themes in the Current skin!
  for (int i = 0; i < items.Size(); ++i)
  {
    CFileItemPtr pItem = items[i];
    if (!pItem->m_bIsFolder)
    {
      CStdString strExtension;
      URIUtils::GetExtension(pItem->GetPath(), strExtension);
      if ((strExtension == ".xpr" && pItem->GetLabel().CompareNoCase("Textures.xpr")) ||
          (strExtension == ".xbt" && pItem->GetLabel().CompareNoCase("Textures.xbt")))
      {
        CStdString strLabel = pItem->GetLabel();
        vecTheme.push_back(strLabel.Mid(0, strLabel.size() - 4));
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

#ifdef _LINUX
bool CUtil::RunCommandLine(const CStdString& cmdLine, bool waitExit)
{
  CStdStringArray args;

  StringUtils::SplitString(cmdLine, ",", args);

  // Strip quotes and whitespace around the arguments, or exec will fail.
  // This allows the python invocation to be written more naturally with any amount of whitespace around the args.
  // But it's still limited, for example quotes inside the strings are not expanded, etc.
  // TODO: Maybe some python library routine can parse this more properly ?
  for (size_t i=0; i<args.size(); i++)
  {
    CStdString &s = args[i];
    CStdString stripd = s.Trim();
    if (stripd[0] == '"' || stripd[0] == '\'')
    {
      s = s.TrimLeft();
      s = s.Right(s.size() - 1);
    }
    if (stripd[stripd.size() - 1] == '"' || stripd[stripd.size() - 1] == '\'')
    {
      s = s.TrimRight();
      s = s.Left(s.size() - 1);
    }
  }

  return Command(args, waitExit);
}

//
// FIXME, this should be merged with the function below.
//
bool CUtil::Command(const CStdStringArray& arrArgs, bool waitExit)
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
    if (waitExit) waitpid(child, &n, 0);
  }

  return (waitExit) ? (WEXITSTATUS(n) == 0) : true;
}

bool CUtil::SudoCommand(const CStdString &strCommand)
{
  CLog::Log(LOGDEBUG, "Executing sudo command: <%s>", strCommand.c_str());
  pid_t child = fork();
  int n = 0;
  if (child == 0)
  {
    close(0); // close stdin to avoid sudo request password
    close(1);
    close(2);
    CStdStringArray arrArgs;
    StringUtils::SplitString(strCommand, " ", arrArgs);
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
}
#endif

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
#ifdef WIN32
  wchar_t szAppPathW[MAX_PATH] = L"";
  ::GetModuleFileNameW(0, szAppPathW, sizeof(szAppPathW)/sizeof(szAppPathW[0]) - 1);
  CStdStringW strPathW = szAppPathW;
  g_charsetConverter.wToUTF8(strPathW,strExecutablePath);
#elif defined(__APPLE__)
  char     given_path[2*MAXPATHLEN];
  uint32_t path_size =2*MAXPATHLEN;

  GetDarwinExecutablePath(given_path, &path_size);
  strExecutablePath = given_path;
#elif defined(__FreeBSD__)                                                                                                                                                                   
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
  char buf[PATH_MAX];
  int ret = readlink(linkname, buf, PATH_MAX);
  buf[ret] = 0;

  strExecutablePath = buf;
#endif
  return strExecutablePath;
}

CStdString CUtil::GetFrameworksPath(bool forPython)
{
  CStdString strFrameworksPath;
#if defined(__APPLE__)
  char     given_path[2*MAXPATHLEN];
  uint32_t path_size =2*MAXPATHLEN;

  GetDarwinFrameworkPath(forPython, given_path, &path_size);
  strFrameworksPath = given_path;
#endif
  return strFrameworksPath;
}

void CUtil::ScanForExternalSubtitles(const CStdString& strMovie, std::vector<CStdString>& vecSubtitles )
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
  
  vector<CStdString> vecExtensionsCached;
  //strExtensionCached = "";
  
  CFileItem item(strMovie, false);
  if (item.IsInternetStream()) return ;
  if (item.IsHDHomeRun()) return ;
  if (item.IsSlingbox()) return ;
  if (item.IsPlayList()) return ;
  if (!item.IsVideo()) return ;
  
  vector<CStdString> strLookInPaths;
  
  CStdString strMovieFileName;
  CStdString strPath;
  
  URIUtils::Split(strMovie, strPath, strMovieFileName);
  CStdString strMovieFileNameNoExt(URIUtils::ReplaceExtension(strMovieFileName, ""));
  strLookInPaths.push_back(strPath);
  
  if (!g_settings.iAdditionalSubtitleDirectoryChecked && !g_guiSettings.GetString("subtitles.custompath").IsEmpty()) // to avoid checking non-existent directories (network) every time..
  {
    if (!g_application.getNetwork().IsAvailable() && !URIUtils::IsHD(g_guiSettings.GetString("subtitles.custompath")))
    {
      CLog::Log(LOGINFO,"CUtil::CacheSubtitles: disabling alternate subtitle directory for this session, it's nonaccessible");
      g_settings.iAdditionalSubtitleDirectoryChecked = -1; // disabled
    }
    else if (!CDirectory::Exists(g_guiSettings.GetString("subtitles.custompath")))
    {
      CLog::Log(LOGINFO,"CUtil::CacheSubtitles: disabling alternate subtitle directory for this session, it's nonexistant");
      g_settings.iAdditionalSubtitleDirectoryChecked = -1; // disabled
    }
    
    g_settings.iAdditionalSubtitleDirectoryChecked = 1;
  }
  
  if (strMovie.Left(6) == "rar://") // <--- if this is found in main path then ignore it!
  {
    CURL url(strMovie);
    CStdString strArchive = url.GetHostName();
    URIUtils::Split(strArchive, strPath, strMovieFileName);
    strLookInPaths.push_back(strPath);
  }
  
  // checking if any of the common subdirs exist ..
  CStdStringArray directories;
  int nTokens = StringUtils::SplitString( strPath, "/", directories );
  if (nTokens == 1)
    StringUtils::SplitString( strPath, "\\", directories );
  
  // if it's inside a cdX dir, add parent path
  if (directories.size() >= 2 && directories[directories.size()-2].size() == 3 && directories[directories.size()-2].Left(2).Equals("cd")) // SplitString returns empty token as last item, hence size-2
  {
    CStdString strPath2;
    URIUtils::GetParentPath(strPath,strPath2);
    strLookInPaths.push_back(strPath2);
  }
  int iSize = strLookInPaths.size();
  for (int i=0;i<iSize;++i)
  {
    for (int j=0; common_sub_dirs[j]; j++)
    {
      CStdString strPath2 = URIUtils::AddFileToFolder(strLookInPaths[i],common_sub_dirs[j]);
      if (CDirectory::Exists(strPath2))
        strLookInPaths.push_back(strPath2);
    }
  }
  // .. done checking for common subdirs
  
  // check if there any cd-directories in the paths we have added so far
  iSize = strLookInPaths.size();
  for (int i=0;i<9;++i) // 9 cd's
  {
    CStdString cdDir;
    cdDir.Format("cd%i",i+1);
    for (int i=0;i<iSize;++i)
    {
      CStdString strPath2 = URIUtils::AddFileToFolder(strLookInPaths[i],cdDir);
      URIUtils::AddSlashAtEnd(strPath2);
      bool pathAlreadyAdded = false;
      for (unsigned int i=0; i<strLookInPaths.size(); i++)
      {
        // if movie file is inside cd-dir, this directory can exist in vector already
        if (strLookInPaths[i].Equals( strPath2 ) )
          pathAlreadyAdded = true;
      }
      if (CDirectory::Exists(strPath2) && !pathAlreadyAdded) 
        strLookInPaths.push_back(strPath2);
    }
  }
  // .. done checking for cd-dirs
  
  // this is last because we dont want to check any common subdirs or cd-dirs in the alternate <subtitles> dir.
  if (g_settings.iAdditionalSubtitleDirectoryChecked == 1)
  {
    strPath = g_guiSettings.GetString("subtitles.custompath");
    URIUtils::AddSlashAtEnd(strPath);
    strLookInPaths.push_back(strPath);
  }
  
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
      int fnl = strMovieFileNameNoExt.size();
      
      for (int j = 0; j < items.Size(); j++)
      {
        URIUtils::Split(items[j]->GetPath(), strPath, strItem);
        
        // is this a rar or zip-file
        if (URIUtils::IsRAR(strItem) || URIUtils::IsZIP(strItem))
        {
          ScanArchiveForSubtitles( items[j]->GetPath(), strMovieFileNameNoExt, vecSubtitles );
        }
        else    // not a rar/zip file
        {
          for (int i = 0; sub_exts[i]; i++)
          {
            //Cache subtitle with same name as movie
            if (URIUtils::GetExtension(strItem).Equals(sub_exts[i]) && strItem.Left(fnl).Equals(strMovieFileNameNoExt))
            {
              vecSubtitles.push_back( items[j]->GetPath() ); 
              CLog::Log(LOGINFO, "%s: found subtitle file %s\n", __FUNCTION__, items[j]->GetPath().c_str() );
            }
          }
        }
      }
      g_directoryCache.ClearDirectory(strLookInPaths[step]);
    }
  }

  iSize = vecSubtitles.size();
  for (int i = 0; i < iSize; i++)
  {
    if (URIUtils::GetExtension(vecSubtitles[i]).Equals(".smi"))
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
            strDest.Format("special://temp/subtitle.%s.%d.smi", TagConv.m_Langclass[k].Name, i);
            if (CFile::Cache(vecSubtitles[i], strDest))
            {
              CLog::Log(LOGINFO, " cached subtitle %s->%s\n", vecSubtitles[i].c_str(), strDest.c_str());
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

int CUtil::ScanArchiveForSubtitles( const CStdString& strArchivePath, const CStdString& strMovieFileNameNoExt, std::vector<CStdString>& vecSubtitles )
{
  int nSubtitlesAdded = 0;
  CFileItemList ItemList;
 
  // zip only gets the root dir
  if (URIUtils::GetExtension(strArchivePath).Equals(".zip"))
  {
   CStdString strZipPath;
   URIUtils::CreateArchivePath(strZipPath,"zip",strArchivePath,"");
   if (!CDirectory::GetDirectory(strZipPath,ItemList,"",false))
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
   CStdString strPathInRar = ItemList[it]->GetPath();
   CStdString strExt = URIUtils::GetExtension(strPathInRar);
   
   CLog::Log(LOGDEBUG, "ScanArchiveForSubtitles:: Found file %s", strPathInRar.c_str());
   // always check any embedded rar archives
   // checking for embedded rars, I moved this outside the sub_ext[] loop. We only need to check this once for each file.
   if (URIUtils::IsRAR(strPathInRar) || URIUtils::IsZIP(strPathInRar))
   {
    CStdString strRarInRar;
    if (URIUtils::GetExtension(strPathInRar).Equals(".rar"))
      URIUtils::CreateArchivePath(strRarInRar, "rar", strArchivePath, strPathInRar);
    else
      URIUtils::CreateArchivePath(strRarInRar, "zip", strArchivePath, strPathInRar);
    ScanArchiveForSubtitles(strRarInRar,strMovieFileNameNoExt,vecSubtitles);
   }
   // done checking if this is a rar-in-rar
   
   int iPos=0;
    while (sub_exts[iPos])
    {
     if (strExt.CompareNoCase(sub_exts[iPos]) == 0)
     {
      CStdString strSourceUrl;
      if (URIUtils::GetExtension(strArchivePath).Equals(".rar"))
       URIUtils::CreateArchivePath(strSourceUrl, "rar", strArchivePath, strPathInRar);
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

/*! \brief in a vector of subtitles finds the corresponding .sub file for a given .idx file
 */
bool CUtil::FindVobSubPair( const std::vector<CStdString>& vecSubtitles, const CStdString& strIdxPath, CStdString& strSubPath )
{
  if (URIUtils::GetExtension(strIdxPath) == ".idx")
  {
    CStdString strIdxFile;
    CStdString strIdxDirectory;
    URIUtils::Split(strIdxPath, strIdxDirectory, strIdxFile);
    for (unsigned int j=0; j < vecSubtitles.size(); j++)
    {
      CStdString strSubFile;
      CStdString strSubDirectory;
      URIUtils::Split(vecSubtitles[j], strSubDirectory, strSubFile);
      if (URIUtils::GetExtension(strSubFile) == ".sub" &&
          URIUtils::ReplaceExtension(strIdxFile,"").Equals(URIUtils::ReplaceExtension(strSubFile,"")))
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
bool CUtil::IsVobSub( const std::vector<CStdString>& vecSubtitles, const CStdString& strSubPath )
{
  if (URIUtils::GetExtension(strSubPath) == ".sub")
  {
    CStdString strSubFile;
    CStdString strSubDirectory;
    URIUtils::Split(strSubPath, strSubDirectory, strSubFile);
    for (unsigned int j=0; j < vecSubtitles.size(); j++)
    {
      CStdString strIdxFile;
      CStdString strIdxDirectory;
      URIUtils::Split(vecSubtitles[j], strIdxDirectory, strIdxFile);
      if (URIUtils::GetExtension(strIdxFile) == ".idx" &&
          URIUtils::ReplaceExtension(strIdxFile,"").Equals(URIUtils::ReplaceExtension(strSubFile,"")))
        return true;
    }
  }
  return false;
}

