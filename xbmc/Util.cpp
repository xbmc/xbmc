/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "network/Network.h"
#include "network/NetworkFileItemClassify.h"
#include "playlists/PlayListFileItemClassify.h"
#include "video/VideoFileItemClassify.h"
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
#include <androidjni/ApplicationInfo.h>
#include "platform/android/activity/XBMCApp.h"
#include "CompileInfo.h"
#endif
#include "ServiceBroker.h"
#include "Util.h"
#include "addons/VFSEntry.h"
#include "filesystem/Directory.h"
#include "filesystem/MultiPathDirectory.h"
#include "filesystem/PVRDirectory.h"
#include "filesystem/RSSDirectory.h"
#include "filesystem/SpecialProtocol.h"
#include "filesystem/StackDirectory.h"

#include <algorithm>
#include <array>
#include <stdlib.h>
#ifdef HAS_UPNP
#include "filesystem/UPnPDirectory.h"
#endif
#include "profiles/ProfileManager.h"
#include "utils/RegExp.h"
#include "windowing/GraphicContext.h"
#include "guilib/TextureManager.h"
#include "storage/MediaManager.h"
#ifdef TARGET_WINDOWS
#include "utils/CharsetConverter.h"
#include "WIN32Util.h"
#endif
#if defined(TARGET_DARWIN)
#include "CompileInfo.h"
#include "platform/darwin/DarwinUtils.h"
#endif
#include "URL.h"
#include "cores/VideoPlayer/DVDSubtitles/DVDSubtitleStream.h"
#include "cores/VideoPlayer/DVDSubtitles/DVDSubtitleTagSami.h"
#include "filesystem/File.h"
#include "guilib/LocalizeStrings.h"
#include "platform/Environment.h"
#include "settings/AdvancedSettings.h"
#include "settings/MediaSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/Digest.h"
#include "utils/FileExtensionProvider.h"
#include "utils/LangCodeExpander.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#include "video/VideoDatabase.h"
#ifdef HAVE_LIBCAP
  #include <sys/capability.h>
#endif

#include "cores/VideoPlayer/DVDDemuxers/DVDDemux.h"

#include <fstrcmp.h>

#ifdef HAS_OPTICAL_DRIVE
using namespace MEDIA_DETECT;
#endif

using namespace XFILE;
using KODI::UTILITY::CDigest;
using namespace KODI;

#if !defined(TARGET_WINDOWS)
unsigned int CUtil::s_randomSeed = time(NULL);
#endif

namespace
{
#ifdef TARGET_WINDOWS
bool IsDirectoryValidRoot(std::wstring path)
{
  path += L"\\system\\settings\\settings.xml";
#if defined(TARGET_WINDOWS_STORE)
  auto h = CreateFile2(path.c_str(), GENERIC_READ, 0, OPEN_EXISTING, NULL);
#else
  auto h = CreateFileW(path.c_str(), GENERIC_READ, 0, nullptr,
    OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
#endif
  if (h != INVALID_HANDLE_VALUE)
  {
    CloseHandle(h);
    return true;
  }

  return false;
}

std::string GetHomePath(const std::string& strTarget, std::string strPath)
{
  std::wstring strPathW;

  // Environment variable was set and we have a path
  // Let's make sure it's not relative and test it
  // so it's actually pointing to a directory containing
  // our stuff
  if (strPath.find("..") != std::string::npos)
  {
    //expand potential relative path to full path
    g_charsetConverter.utf8ToW(strPath, strPathW, false);
    CWIN32Util::AddExtraLongPathPrefix(strPathW);
    auto bufSize = GetFullPathNameW(strPathW.c_str(), 0, nullptr, nullptr);
    if (bufSize != 0)
    {
      auto buf = std::make_unique<wchar_t[]>(bufSize);
      if (GetFullPathNameW(strPathW.c_str(), bufSize, buf.get(), nullptr) <= bufSize - 1)
      {
        strPathW = buf.get();
        CWIN32Util::RemoveExtraLongPathPrefix(strPathW);

        if (IsDirectoryValidRoot(strPathW))
        {
          g_charsetConverter.wToUTF8(strPathW, strPath);
          return strPath;
        }
      }
    }
  }

  // Okay se no environment variable is set, let's
  // grab the executable path and check if it's being
  // run from a directory containing our stuff
  strPath = CUtil::ResolveExecutablePath();
  auto last_sep = strPath.find_last_of(PATH_SEPARATOR_CHAR);
  if (last_sep != std::string::npos)
    strPath.resize(last_sep);

  g_charsetConverter.utf8ToW(strPath, strPathW);
  if (IsDirectoryValidRoot(strPathW))
    return strPath;

  // Still nothing, let's check the current working
  // directory and see if it points to a directory
  // with our stuff in it. This bit should never be
  // needed when running on a users system, it's intended
  // to make our dev environment easier.
  auto bufSize = GetCurrentDirectoryW(0, nullptr);
  if (bufSize > 0)
  {
    auto buf = std::make_unique<wchar_t[]>(bufSize);
    if (0 != GetCurrentDirectoryW(bufSize, buf.get()))
    {
      std::string currentDirectory;
      std::wstring currentDirectoryW(buf.get());
      CWIN32Util::RemoveExtraLongPathPrefix(currentDirectoryW);

      if (IsDirectoryValidRoot(currentDirectoryW))
      {
        g_charsetConverter.wToUTF8(currentDirectoryW, currentDirectory);
        return currentDirectory;
      }
    }
  }

  // If we ended up here we're most likely screwed
  // we will crash in a few seconds
  return strPath;
}
#endif
#if defined(TARGET_DARWIN)
#if !defined(TARGET_DARWIN_EMBEDDED)
bool IsDirectoryValidRoot(std::string path)
{
  path += "/system/settings/settings.xml";
  return CFile::Exists(path);
}
#endif

std::string GetHomePath(const std::string& strTarget, std::string strPath)
{
  if (strPath.empty())
  {
    auto strHomePath = CUtil::ResolveExecutablePath();
    int      result = -1;
    char     given_path[2 * MAXPATHLEN];
    size_t path_size = 2 * MAXPATHLEN;

    result = CDarwinUtils::GetExecutablePath(given_path, &path_size);
    if (result == 0)
    {
      // Move backwards to last /.
      for (int n = strlen(given_path) - 1; given_path[n] != '/'; n--)
        given_path[n] = '\0';

#if defined(TARGET_DARWIN_EMBEDDED)
      strcat(given_path, "/AppData/AppHome/");
#else
      // Assume local path inside application bundle.
      strcat(given_path, "../Resources/");
      strcat(given_path, CCompileInfo::GetAppName());
      strcat(given_path, "/");

      // if this path doesn't exist we
      // might not be started from the app bundle
      // but from the debugger/xcode. Lets
      // see if this assumption is valid
      if (!CDirectory::Exists(given_path))
      {
        std::string given_path_stdstr = CUtil::ResolveExecutablePath();
        // try to find the correct folder by going back
        // in the executable path until settings.xml was found
        bool validRoot = false;
        do
        {
          given_path_stdstr = URIUtils::GetParentPath(given_path_stdstr);
          validRoot = IsDirectoryValidRoot(given_path_stdstr);
        }
        while(given_path_stdstr.length() > 0 && !validRoot);
        strncpy(given_path, given_path_stdstr.c_str(), sizeof(given_path)-1);
      }

#endif

      // Convert to real path.
      char real_path[2 * MAXPATHLEN];
      if (realpath(given_path, real_path) != NULL)
      {
        strPath = real_path;
        return strPath;
      }
    }
    size_t last_sep = strHomePath.find_last_of(PATH_SEPARATOR_CHAR);
    if (last_sep != std::string::npos)
      strPath = strHomePath.substr(0, last_sep);
    else
      strPath = strHomePath;

  }
  return strPath;
}
#endif
#if defined(TARGET_POSIX) && !defined(TARGET_DARWIN)
std::string GetHomePath(const std::string& strTarget, std::string strPath)
{
  if (strPath.empty())
  {
    auto strHomePath = CUtil::ResolveExecutablePath();
    size_t last_sep = strHomePath.find_last_of(PATH_SEPARATOR_CHAR);
    if (last_sep != std::string::npos)
      strPath = strHomePath.substr(0, last_sep);
    else
      strPath = strHomePath;
  }
  /* Change strPath accordingly when target is KODI_HOME and when INSTALL_PATH
   * and BIN_INSTALL_PATH differ
   */
  std::string installPath = INSTALL_PATH;
  std::string binInstallPath = BIN_INSTALL_PATH;

  if (strTarget.empty() && installPath.compare(binInstallPath))
  {
    int pos = strPath.length() - binInstallPath.length();
    std::string tmp = strPath;
    tmp.erase(0, pos);
    if (!tmp.compare(binInstallPath))
    {
      strPath.erase(pos, strPath.length());
      strPath.append(installPath);
    }
  }

  return strPath;
}
#endif
}

std::string CUtil::GetTitleFromPath(const std::string& strFileNameAndPath, bool bIsFolder /* = false */)
{
  CURL pathToUrl(strFileNameAndPath);
  return GetTitleFromPath(pathToUrl, bIsFolder);
}

std::string CUtil::GetTitleFromPath(const CURL& url, bool bIsFolder /* = false */)
{
  // use above to get the filename
  std::string path(url.Get());
  URIUtils::RemoveSlashAtEnd(path);
  std::string strFilename = URIUtils::GetFileName(path);

#ifdef HAS_UPNP
  // UPNP
  if (url.IsProtocol("upnp"))
    strFilename = CUPnPDirectory::GetFriendlyName(url);
#endif

  if (url.IsProtocol("rss") || url.IsProtocol("rsss"))
  {
    CRSSDirectory dir;
    CFileItemList items;
    if(dir.GetDirectory(url, items) && !items.m_strTitle.empty())
      return items.m_strTitle;
  }

  // Shoutcast
  else if (url.IsProtocol("shout"))
  {
    const std::string strFileNameAndPath = url.Get();
    const size_t genre = strFileNameAndPath.find_first_of('=');
    if(genre == std::string::npos)
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

  // Root file views
  else if (url.IsProtocol("sources"))
    strFilename = g_localizeStrings.Get(744);

  // Music Playlists
  else if (StringUtils::StartsWith(path, "special://musicplaylists"))
    strFilename = g_localizeStrings.Get(136);

  // Video Playlists
  else if (StringUtils::StartsWith(path, "special://videoplaylists"))
    strFilename = g_localizeStrings.Get(136);

  else if (URIUtils::HasParentInHostname(url) && strFilename.empty())
    strFilename = URIUtils::GetFileName(url.GetHostName());

  // now remove the extension if needed
  if (!CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_FILELISTS_SHOWEXTENSIONS) && !bIsFolder)
  {
    URIUtils::RemoveExtension(strFilename);
    return strFilename;
  }

  // URLDecode since the original path may be an URL
  strFilename = CURL::Decode(strFilename);
  return strFilename;
}

namespace
{
void GetTrailingDiscNumberSegmentInfoFromPath(const std::string& pathIn,
                                              size_t& pos,
                                              std::string& number)
{
  std::string path{pathIn};
  URIUtils::RemoveSlashAtEnd(path);

  pos = std::string::npos;
  number.clear();

  // Handle Disc, Disk and locale specific spellings
  std::string discStr{StringUtils::Format("/{} ", g_localizeStrings.Get(427))};
  size_t discPos = path.rfind(discStr);

  if (discPos == std::string::npos)
  {
    discStr = "/Disc ";
    discPos = path.rfind(discStr);
  }

  if (discPos == std::string::npos)
  {
    discStr = "/Disk ";
    discPos = path.rfind(discStr);
  }

  if (discPos != std::string::npos)
  {
    // Check remainder of path is numeric (eg. Disc 1)
    const std::string discNum{path.substr(discPos + discStr.size())};
    if (discNum.find_first_not_of("0123456789") == std::string::npos)
    {
      pos = discPos;
      number = discNum;
    }
  }
}
} // unnamed namespace

std::string CUtil::RemoveTrailingDiscNumberSegmentFromPath(std::string path)
{
  size_t discPos{std::string::npos};
  std::string discNum;
  GetTrailingDiscNumberSegmentInfoFromPath(path, discPos, discNum);

  if (discPos != std::string::npos)
    path.erase(discPos);

  return path;
}

std::string CUtil::GetDiscNumberFromPath(const std::string& path)
{
  size_t discPos{std::string::npos};
  std::string discNum;
  GetTrailingDiscNumberSegmentInfoFromPath(path, discPos, discNum);
  return discNum;
}

bool CUtil::GetFilenameIdentifier(const std::string& fileName,
                                  std::string& identifierType,
                                  std::string& identifier)
{
  std::string match;
  return GetFilenameIdentifier(fileName, identifierType, identifier, match);
}

bool CUtil::GetFilenameIdentifier(const std::string& fileName,
                                  std::string& identifierType,
                                  std::string& identifier,
                                  std::string& match)
{
  CRegExp reIdentifier(true, CRegExp::autoUtf8);

  const std::shared_ptr<CAdvancedSettings> advancedSettings =
      CServiceBroker::GetSettingsComponent()->GetAdvancedSettings();
  if (!reIdentifier.RegComp(advancedSettings->m_videoFilenameIdentifierRegExp))
  {
    CLog::LogF(LOGERROR, "Invalid filename identifier RegExp:'{}'",
               advancedSettings->m_videoFilenameIdentifierRegExp);
    return false;
  }
  else
  {
    if (reIdentifier.RegComp(advancedSettings->m_videoFilenameIdentifierRegExp))
    {
      if (reIdentifier.RegFind(fileName) >= 0)
      {
        match = reIdentifier.GetMatch(0);
        identifierType = reIdentifier.GetMatch(1);
        identifier = reIdentifier.GetMatch(2);
        StringUtils::ToLower(identifierType);
        return true;
      }
    }
  }
  return false;
}

bool CUtil::HasFilenameIdentifier(const std::string& fileName)
{
  std::string identifierType;
  std::string identifier;
  return GetFilenameIdentifier(fileName, identifierType, identifier);
}

void CUtil::CleanString(const std::string& strFileName,
                        std::string& strTitle,
                        std::string& strTitleAndYear,
                        std::string& strYear,
                        bool bRemoveExtension /* = false */,
                        bool bCleanChars /* = true */)
{
  strTitleAndYear = strFileName;

  if (strFileName == "..")
   return;

  std::string identifier;
  std::string identifierType;
  std::string identifierMatch;
  if (GetFilenameIdentifier(strFileName, identifierType, identifier, identifierMatch))
    StringUtils::Replace(strTitleAndYear, identifierMatch, "");

  const std::shared_ptr<CAdvancedSettings> advancedSettings = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings();
  const std::vector<std::string> &regexps = advancedSettings->m_videoCleanStringRegExps;

  CRegExp reTags(true, CRegExp::autoUtf8);
  CRegExp reYear(false, CRegExp::autoUtf8);

  if (!reYear.RegComp(advancedSettings->m_videoCleanDateTimeRegExp))
  {
    CLog::Log(LOGERROR, "{}: Invalid datetime clean RegExp:'{}'", __FUNCTION__,
              advancedSettings->m_videoCleanDateTimeRegExp);
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

  for (const auto &regexp : regexps)
  {
    if (!reTags.RegComp(regexp.c_str()))
    { // invalid regexp - complain in logs
      CLog::Log(LOGERROR, "{}: Invalid string clean RegExp:'{}'", __FUNCTION__, regexp);
      continue;
    }
    int j=0;
    if ((j=reTags.RegFind(strTitleAndYear.c_str())) > 0)
      strTitleAndYear.resize(j);
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

    for (char &c : strTitleAndYear)
    {
      if (c != '.')
        initialDots = false;

      if ((c == '_') || ((!alreadyContainsSpace) && !initialDots && (c == '.')))
      {
        c = ' ';
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
    std::string basePath = strFilename.substr(0, pos + 1);
    strFilename.erase(0, pos + 4);
    basePath = URIUtils::GetParentPath(basePath);
    strFilename = URIUtils::AddFileToFolder(basePath, strFilename);
  }
  while ((pos = strFilename.find("\\..\\")) != std::string::npos)
  {
    std::string basePath = strFilename.substr(0, pos + 1);
    strFilename.erase(0, pos + 4);
    basePath = URIUtils::GetParentPath(basePath);
    strFilename = URIUtils::AddFileToFolder(basePath, strFilename);
  }
}

void CUtil::RunShortcut(const char* szShortcutPath)
{
}

std::string CUtil::GetHomePath(const std::string& strTarget)
{
  auto strPath = CEnvironment::getenv(strTarget);

  return ::GetHomePath(strTarget, strPath);
}

bool CUtil::IsPicture(const std::string& strFile)
{
  return URIUtils::HasExtension(strFile,
                  CServiceBroker::GetFileExtensionProvider().GetPictureExtensions()+ "|.tbn|.dds");
}

std::string CUtil::GetSplashPath()
{
  std::array<std::string, 4> candidates {{ "special://home/media/splash.jpg", "special://home/media/splash.png", "special://xbmc/media/splash.jpg", "special://xbmc/media/splash.png" }};
  auto it = std::find_if(candidates.begin(), candidates.end(), [](std::string const& file) { return XFILE::CFile::Exists(file); });
  if (it == candidates.end())
    throw std::runtime_error("No splash image found");
  return CSpecialProtocol::TranslatePathConvertCase(*it);
}

bool CUtil::ExcludeFileOrFolder(const std::string& strFileOrFolder, const std::vector<std::string>& regexps)
{
  if (strFileOrFolder.empty())
    return false;

  CRegExp regExExcludes(true, CRegExp::autoUtf8);  // case insensitive regex

  for (const auto &regexp : regexps)
  {
    if (!regExExcludes.RegComp(regexp.c_str()))
    { // invalid regexp - complain in logs
      CLog::Log(LOGERROR, "{}: Invalid exclude RegExp:'{}'", __FUNCTION__, regexp);
      continue;
    }
    if (regExExcludes.RegFind(strFileOrFolder) > -1)
    {
      CLog::LogF(LOGDEBUG, "File '{}' excluded. (Matches exclude rule RegExp: '{}')", CURL::GetRedacted(strFileOrFolder), regexp);
      return true;
    }
  }
  return false;
}

void CUtil::GetFileAndProtocol(const std::string& strURL, std::string& strDir)
{
  strDir = strURL;
  if (!URIUtils::IsRemote(strURL)) return ;
  if (URIUtils::IsDVD(strURL)) return ;

  CURL url(strURL);
  strDir = StringUtils::Format("{}://{}", url.GetProtocol(), url.GetFileName());
}

int CUtil::GetDVDIfoTitle(const std::string& strFile)
{
  std::string strFilename = URIUtils::GetFileName(strFile);
  if (StringUtils::EqualsNoCase(strFilename, "video_ts.ifo")) return 0;
  //VTS_[TITLE]_0.IFO
  return atoi(strFilename.substr(4, 2).c_str());
}

std::string CUtil::GetFileDigest(const std::string& strPath, KODI::UTILITY::CDigest::Type type)
{
  CFile file;
  std::string result;
  if (file.Open(strPath))
  {
    CDigest digest{type};
    char temp[1024];
    while (true)
    {
      ssize_t read = file.Read(temp,1024);
      if (read <= 0)
        break;
      digest.Update(temp,read);
    }
    result = digest.Finalize();
    file.Close();
  }

  return result;
}

bool CUtil::GetDirectoryName(const std::string& strFileName, std::string& strDescription)
{
  std::string strFName = URIUtils::GetFileName(strFileName);
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
  if (!CServiceBroker::GetMediaManager().IsDiscInDrive(strPath))
  {
    strIcon = "DefaultDVDEmpty.png";
    return ;
  }

  CFileItem item = CFileItem(strPath, false);

  if (item.IsBluray())
  {
    strIcon = "DefaultBluray.png";
    return;
  }

  if ( URIUtils::IsDVD(strPath) )
  {
    strIcon = "DefaultDVDFull.png";
    return ;
  }

  if ( URIUtils::IsISO9660(strPath) )
  {
#ifdef HAS_OPTICAL_DRIVE
    CCdInfo* pInfo = CServiceBroker::GetMediaManager().GetCdInfo();
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
  const std::shared_ptr<CProfileManager> profileManager = CServiceBroker::GetSettingsComponent()->GetProfileManager();

  std::string searchPath = profileManager->GetDatabaseFolder();
  CFileItemList items;
  if (!XFILE::CDirectory::GetDirectory(searchPath, items, ".tmp", DIR_FLAG_NO_FILE_DIRS))
    return;

  for (const auto &item : items)
  {
    if (item->m_bIsFolder)
      continue;
    XFILE::CFile::Delete(item->GetPath());
  }
}

void CUtil::ClearSubtitles()
{
  //delete cached subs
  CFileItemList items;
  CDirectory::GetDirectory("special://temp/",items, "", DIR_FLAG_DEFAULTS);
  for (const auto &item : items)
  {
    if (!item->m_bIsFolder)
    {
      if (item->GetPath().find("subtitle") != std::string::npos ||
          item->GetPath().find("vobsub_queue") != std::string::npos)
      {
        CLog::Log(LOGDEBUG, "{} - Deleting temporary subtitle {}", __FUNCTION__, item->GetPath());
        CFile::Delete(item->GetPath());
      }
    }
  }
}

int64_t CUtil::ToInt64(uint32_t high, uint32_t low)
{
  int64_t n;
  n = high;
  n <<= 32;
  n += low;
  return n;
}

/*!
  \brief Finds next unused filename that matches padded int format identifier provided
  \param[in]  fn_template    filename template consisting of a padded int format identifier (eg screenshot%03d)
  \param[in]  max            maximum number to search for available name
  \return "" on failure, string next available name matching format identifier on success
*/

std::string CUtil::GetNextFilename(const std::string &fn_template, int max)
{
  std::string searchPath = URIUtils::GetDirectory(fn_template);
  std::string mask = URIUtils::GetExtension(fn_template);
  std::string name = StringUtils::Format(fn_template, 0);

  CFileItemList items;
  if (!CDirectory::GetDirectory(searchPath, items, mask, DIR_FLAG_NO_FILE_DIRS))
    return name;

  items.SetFastLookup(true);
  for (int i = 0; i <= max; i++)
  {
    std::string name = StringUtils::Format(fn_template, i);
    if (!items.Get(name))
      return name;
  }
  return "";
}

std::string CUtil::GetNextPathname(const std::string &path_template, int max)
{
  if (path_template.find("%04d") == std::string::npos)
    return "";

  for (int i = 0; i <= max; i++)
  {
    std::string name = StringUtils::Format(path_template, i);
    if (!CFile::Exists(name) && !CDirectory::Exists(name))
      return name;
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

bool CUtil::CreateDirectoryEx(const std::string& strPath)
{
  // Function to create all directories at once instead
  // of calling CreateDirectory for every subdir.
  // Creates the directory and subdirectories if needed.

  // return true if directory already exist
  if (CDirectory::Exists(strPath)) return true;

  // we currently only allow HD and smb and nfs paths
  if (!URIUtils::IsHD(strPath) && !URIUtils::IsSmb(strPath) && !URIUtils::IsNfs(strPath))
  {
    CLog::Log(LOGERROR, "{} called with an unsupported path: {}", __FUNCTION__, strPath);
    return false;
  }

  std::vector<std::string> dirs = URIUtils::SplitPath(strPath);
  if (dirs.empty())
    return false;
  std::string dir(dirs.front());
  URIUtils::AddSlashAtEnd(dir);
  for (std::vector<std::string>::const_iterator it = dirs.begin() + 1; it != dirs.end(); ++it)
  {
    dir = URIUtils::AddFileToFolder(dir, *it);
    CDirectory::Create(dir);
  }

  // was the final destination directory successfully created ?
  return CDirectory::Exists(strPath);
}

std::string CUtil::MakeLegalFileName(std::string strFile, LegalPath LegalType)
{
  StringUtils::Replace(strFile, '/', '_');
  StringUtils::Replace(strFile, '\\', '_');
  StringUtils::Replace(strFile, '?', '_');

  if (LegalType == LegalPath::WIN32_COMPAT)
  {
    // just filter out some illegal characters on windows
    StringUtils::Replace(strFile, ':', '_');
    StringUtils::Replace(strFile, '*', '_');
    StringUtils::Replace(strFile, '?', '_');
    StringUtils::Replace(strFile, '\"', '_');
    StringUtils::Replace(strFile, '<', '_');
    StringUtils::Replace(strFile, '>', '_');
    StringUtils::Replace(strFile, '|', '_');
    StringUtils::TrimRight(strFile, ". ");
  }
  return strFile;
}

// legalize entire path
std::string CUtil::MakeLegalPath(std::string strPathAndFile, LegalPath LegalType)
{
  if (URIUtils::IsStack(strPathAndFile))
    return MakeLegalPath(CStackDirectory::GetFirstStackedFile(strPathAndFile));
  if (URIUtils::IsMultiPath(strPathAndFile))
    return MakeLegalPath(CMultiPathDirectory::GetFirstPath(strPathAndFile));
  if (!URIUtils::IsHD(strPathAndFile) && !URIUtils::IsSmb(strPathAndFile) && !URIUtils::IsNfs(strPathAndFile))
    return strPathAndFile; // we don't support writing anywhere except HD, SMB and NFS - no need to legalize path

  bool trailingSlash = URIUtils::HasSlashAtEnd(strPathAndFile);
  std::vector<std::string> dirs = URIUtils::SplitPath(strPathAndFile);
  if (dirs.empty())
    return strPathAndFile;
  // we just add first token to path and don't legalize it - possible values:
  // "X:" (local win32), "" (local unix - empty string before '/') or
  // "protocol://domain"
  std::string dir(dirs.front());
  URIUtils::AddSlashAtEnd(dir);
  for (std::vector<std::string>::const_iterator it = dirs.begin() + 1; it != dirs.end(); ++it)
    dir = URIUtils::AddFileToFolder(dir, MakeLegalFileName(*it, LegalType));
  if (trailingSlash) URIUtils::AddSlashAtEnd(dir);
  return dir;
}

std::string CUtil::ValidatePath(std::string path, bool bFixDoubleSlashes /* = false */)
{

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
    return path;

    // check the path for incorrect slashes
#ifdef TARGET_WINDOWS
  if (URIUtils::IsDOSPath(path))
  {
    StringUtils::Replace(path, '/', '\\');
    /* The double slash correction should only be used when *absolutely*
       necessary! This applies to certain DLLs or use from Python DLLs/scripts
       that incorrectly generate double (back) slashes.
    */
    if (bFixDoubleSlashes && !path.empty())
    {
      // Fixup for double back slashes (but ignore the \\ of unc-paths)
      for (size_t x = 1; x < path.size() - 1; x++)
      {
        if (path[x] == '\\' && path[x + 1] == '\\')
          path.erase(x, 1);
      }
    }
  }
  else if (path.find("://") != std::string::npos || path.find(":\\\\") != std::string::npos)
#endif
  {
    StringUtils::Replace(path, '\\', '/');
    /* The double slash correction should only be used when *absolutely*
       necessary! This applies to certain DLLs or use from Python DLLs/scripts
       that incorrectly generate double (back) slashes.
    */
    if (bFixDoubleSlashes && !path.empty())
    {
      // Fixup for double forward slashes(/) but don't touch the :// of URLs
      for (size_t x = 2; x < path.size() - 1; x++)
      {
        if (path[x] == '/' && path[x + 1] == '/' &&
            !(path[x - 1] == ':' || (path[x - 1] == '/' && path[x - 2] == ':')))
          path.erase(x, 1);
      }
    }
  }
  return path;
}

void CUtil::SplitParams(const std::string &paramString, std::vector<std::string> &parameters)
{
  bool inQuotes = false;
  bool lastEscaped = false; // only every second character can be escaped
  int inFunction = 0;
  size_t whiteSpacePos = 0;
  std::string parameter;
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
      { // not in a function, so a comma signifies the end of this parameter
        if (whiteSpacePos)
          parameter.resize(whiteSpacePos);
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
            parameter.erase(quotaPos, 1);
          }
        }
        parameters.push_back(parameter);
        parameter.clear();
        whiteSpacePos = 0;
        continue;
      }
    }
    if ((ch == '"' || ch == '\\') && escaped)
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
    CLog::Log(LOGWARNING, "{}({}) - end of string while searching for ) or \"", __FUNCTION__,
              paramString);
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
      parameter.erase(quotaPos, 1);
    }
  }
  if (!parameter.empty() || parameters.size())
    parameters.push_back(parameter);
}

int CUtil::GetMatchingSource(const std::string& strPath1,
                             std::vector<CMediaSource>& sources,
                             bool& bIsSourceName)
{
  if (strPath1.empty())
    return -1;

  // copy as we may change strPath
  std::string strPath = strPath1;

  // Check for special protocols
  CURL checkURL(strPath);

  if (StringUtils::StartsWith(strPath, "special://skin/"))
    return 1;

  // do not return early if URL protocol is "plugin"
  // since video- and/or audio-plugins can be configured as mediasource

  // stack://
  if (checkURL.IsProtocol("stack"))
    strPath.erase(0, 8); // remove the stack protocol

  if (checkURL.IsProtocol("shout"))
    strPath = checkURL.GetHostName();

  if (checkURL.IsProtocol("multipath"))
    strPath = CMultiPathDirectory::GetFirstPath(strPath);

  bIsSourceName = false;
  int iIndex = -1;

  // we first test the NAME of a source
  for (int i = 0; i < static_cast<int>(sources.size()); ++i)
  {
    const CMediaSource& share = sources[i];
    std::string strName = share.strName;

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
        strName.resize(iPos - 1);
    }
    if (StringUtils::EqualsNoCase(strPath, strName))
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
  urlDest.SetProtocolOptions("");
  std::string strDest = urlDest.GetWithoutUserDetails();
  ForceForwardSlashes(strDest);
  if (!URIUtils::HasSlashAtEnd(strDest))
    strDest += "/";

  size_t iLength = 0;
  size_t iLenPath = strDest.size();
  for (int i = 0; i < static_cast<int>(sources.size()); ++i)
  {
    const CMediaSource& share = sources[i];

    // does it match a source name?
    if (share.strPath.substr(0,8) == "shout://")
    {
      CURL url(share.strPath);
      if (strPath == url.GetHostName())
        return i;
    }

    // doesn't match a name, so try the source path
    std::vector<std::string> vecPaths;

    // add any concatenated paths if they exist
    if (!share.vecPaths.empty())
      vecPaths = share.vecPaths;

    // add the actual share path at the front of the vector
    vecPaths.insert(vecPaths.begin(), share.strPath);

    // test each path
    for (const auto &path : vecPaths)
    {
      // remove user details, and ensure path only uses forward slashes
      // and ends with a trailing slash so as not to match a substring
      CURL urlShare(path);
      urlShare.SetOptions("");
      urlShare.SetProtocolOptions("");
      std::string strShare = urlShare.GetWithoutUserDetails();
      ForceForwardSlashes(strShare);
      if (!URIUtils::HasSlashAtEnd(strShare))
        strShare += "/";
      size_t iLenShare = strShare.size();

      if ((iLenPath >= iLenShare) && StringUtils::StartsWithNoCase(strDest, strShare) && (iLenShare > iLength))
      {
        // if exact match, return it immediately
        if (iLenPath == iLenShare)
        {
          // if the path EXACTLY matches an item in a concatenated path
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
      return GetMatchingSource(strPath, sources, bDummy);
    }

    CLog::Log(LOGDEBUG, "CUtil::GetMatchingSource: no matching source found for [{}]", strPath1);
  }
  return iIndex;
}

std::string CUtil::TranslateSpecialSource(const std::string &strSpecial)
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
      return URIUtils::AddFileToFolder(CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(CSettings::SETTING_SYSTEM_PLAYLISTSPATH), strSpecial.substr(10));
  }
  return strSpecial;
}

std::string CUtil::MusicPlaylistsLocation()
{
  const std::string path = CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(CSettings::SETTING_SYSTEM_PLAYLISTSPATH);
  std::vector<std::string> vec;
  vec.push_back(URIUtils::AddFileToFolder(path, "music"));
  vec.push_back(URIUtils::AddFileToFolder(path, "mixed"));
  return XFILE::CMultiPathDirectory::ConstructMultiPath(vec);
}

std::string CUtil::VideoPlaylistsLocation()
{
  const std::string path = CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(CSettings::SETTING_SYSTEM_PLAYLISTSPATH);
  std::vector<std::string> vec;
  vec.push_back(URIUtils::AddFileToFolder(path, "video"));
  vec.push_back(URIUtils::AddFileToFolder(path, "mixed"));
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

void CUtil::DeleteDirectoryCache(const std::string &prefix)
{
  std::string searchPath = "special://temp/";
  CFileItemList items;
  if (!XFILE::CDirectory::GetDirectory(searchPath, items, ".fi", DIR_FLAG_NO_FILE_DIRS))
    return;

  for (const auto &item : items)
  {
    if (item->m_bIsFolder)
      continue;
    std::string fileName = URIUtils::GetFileName(item->GetPath());
    if (StringUtils::StartsWith(fileName, prefix))
      XFILE::CFile::Delete(item->GetPath());
  }
}


void CUtil::GetRecursiveListing(const std::string& strPath, CFileItemList& items, const std::string& strMask, unsigned int flags /* = DIR_FLAG_DEFAULTS */)
{
  CFileItemList myItems;
  CDirectory::GetDirectory(strPath,myItems,strMask,flags);
  for (const auto &item : myItems)
  {
    if (item->m_bIsFolder)
      CUtil::GetRecursiveListing(item->GetPath(),items,strMask,flags);
    else
      items.Add(item);
  }
}

void CUtil::GetRecursiveDirsListing(const std::string& strPath, CFileItemList& item, unsigned int flags /* = DIR_FLAG_DEFAULTS */)
{
  CFileItemList myItems;
  CDirectory::GetDirectory(strPath,myItems,"",flags);
  for (const auto &i : myItems)
  {
    if (i->m_bIsFolder && !i->IsPath(".."))
    {
      item.Add(i);
      CUtil::GetRecursiveDirsListing(i->GetPath(),item,flags);
    }
  }
}

void CUtil::ForceForwardSlashes(std::string& strPath)
{
  size_t iPos = strPath.rfind('\\');
  while (iPos != std::string::npos)
  {
    strPath.at(iPos) = '/';
    iPos = strPath.rfind('\\');
  }
}

double CUtil::AlbumRelevance(const std::string& strAlbumTemp1, const std::string& strAlbum1, const std::string& strArtistTemp1, const std::string& strArtist1)
{
  // case-insensitive fuzzy string comparison on the album and artist for relevance
  // weighting is identical, both album and artist are 50% of the total relevance
  // a missing artist means the maximum relevance can only be 0.50
  std::string strAlbumTemp = strAlbumTemp1;
  StringUtils::ToLower(strAlbumTemp);
  std::string strAlbum = strAlbum1;
  StringUtils::ToLower(strAlbum);
  double fAlbumPercentage = fstrcmp(strAlbumTemp.c_str(), strAlbum.c_str());
  double fArtistPercentage = 0.0;
  if (!strArtist1.empty())
  {
    std::string strArtistTemp = strArtistTemp1;
    StringUtils::ToLower(strArtistTemp);
    std::string strArtist = strArtist1;
    StringUtils::ToLower(strArtist);
    fArtistPercentage = fstrcmp(strArtistTemp.c_str(), strArtist.c_str());
  }
  double fRelevance = fAlbumPercentage * 0.5 + fArtistPercentage * 0.5;
  return fRelevance;
}

bool CUtil::MakeShortenPath(std::string StrInput, std::string& StrOutput, size_t iTextMaxLength)
{
  size_t iStrInputSize = StrInput.size();
  if(iStrInputSize <= 0 || iTextMaxLength >= iStrInputSize)
  {
    StrOutput = StrInput;
    return true;
  }

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

    if (nPos == std::string::npos || nPos == 0)
      break;

    nPos = StrInput.find_last_of( cDelim, nPos - 1 );

    if ( nPos == std::string::npos )
      break;
    if ( nGreaterDelim > nPos )
      StrInput.replace( nPos + 1, nGreaterDelim - nPos - 1, ".." );
    iStrInputSize = StrInput.size();
  }
  // replace any additional /../../ with just /../ if necessary
  std::string replaceDots = StringUtils::Format("..{}..", cDelim);
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

bool CUtil::SupportsWriteFileOperations(const std::string& strPath)
{
  // currently only hd, smb, nfs and dav support delete and rename
  if (URIUtils::IsHD(strPath))
    return true;
  if (URIUtils::IsSmb(strPath))
    return true;
  if (URIUtils::IsPVRRecording(strPath))
    return CPVRDirectory::SupportsWriteFileOperations(strPath);
  if (URIUtils::IsNfs(strPath))
    return true;
  if (URIUtils::IsDAV(strPath))
    return true;
  if (URIUtils::IsStack(strPath))
    return SupportsWriteFileOperations(CStackDirectory::GetFirstStackedFile(strPath));
  if (URIUtils::IsMultiPath(strPath))
    return CMultiPathDirectory::SupportsWriteFileOperations(strPath);

  if (CServiceBroker::IsAddonInterfaceUp())
  {
    CURL url(strPath);
    for (const auto& addon : CServiceBroker::GetVFSAddonCache().GetAddonInstances())
    {
      const auto& info = addon->GetProtocolInfo();
      auto prots = StringUtils::Split(info.type, "|");
      if (info.supportWrite &&
          std::find(prots.begin(), prots.end(), url.GetProtocol()) != prots.end())
        return true;
    }
  }

  return false;
}

bool CUtil::SupportsReadFileOperations(const std::string& strPath)
{
  return !URIUtils::IsVideoDb(strPath);
}

std::string CUtil::GetDefaultFolderThumb(const std::string &folderThumb)
{
  if (CServiceBroker::GetGUI()->GetTextureManager().HasTexture(folderThumb))
    return folderThumb;
  return "";
}

void CUtil::GetSkinThemes(std::vector<std::string>& vecTheme)
{
  static const std::string TexturesXbt = "Textures.xbt";

  std::string strPath = URIUtils::AddFileToFolder(CServiceBroker::GetWinSystem()->GetGfxContext().GetMediaDir(), "media");
  CFileItemList items;
  CDirectory::GetDirectory(strPath, items, "", DIR_FLAG_DEFAULTS);
  // Search for Themes in the Current skin!
  for (const auto &pItem : items)
  {
    if (!pItem->m_bIsFolder)
    {
      std::string strExtension = URIUtils::GetExtension(pItem->GetPath());
      std::string strLabel = pItem->GetLabel();
      if ((strExtension == ".xbt" && !StringUtils::EqualsNoCase(strLabel, TexturesXbt)))
        vecTheme.push_back(StringUtils::Left(strLabel, strLabel.size() - strExtension.size()));
    }
    else
    {
      // check if this is an xbt:// VFS path
      CURL itemUrl(pItem->GetPath());
      if (!itemUrl.IsProtocol("xbt") || !itemUrl.GetFileName().empty())
        continue;

      std::string strLabel = URIUtils::GetFileName(itemUrl.GetHostName());
      if (!StringUtils::EqualsNoCase(strLabel, TexturesXbt))
        vecTheme.push_back(StringUtils::Left(strLabel, strLabel.size() - URIUtils::GetExtension(strLabel).size()));
    }
  }
  std::sort(vecTheme.begin(), vecTheme.end(), sortstringbyname());
}

void CUtil::InitRandomSeed()
{
  // Init random seed
  auto now = std::chrono::steady_clock::now();
  auto seed = now.time_since_epoch();

  srand(static_cast<unsigned int>(seed.count()));
}

#if defined(TARGET_POSIX) && !defined(TARGET_DARWIN_TVOS)
bool CUtil::RunCommandLine(const std::string& cmdLine, bool waitExit)
{
  std::vector<std::string> args = StringUtils::Split(cmdLine, ",");

  // Strip quotes and whitespace around the arguments, or exec will fail.
  // This allows the python invocation to be written more naturally with any amount of whitespace around the args.
  // But it's still limited, for example quotes inside the strings are not expanded, etc.
  //! @todo Maybe some python library routine can parse this more properly ?
  for (std::vector<std::string>::iterator it = args.begin(); it != args.end(); ++it)
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

bool CUtil::Command(const std::vector<std::string>& arrArgs, bool waitExit)
{
#ifdef _DEBUG
  printf("Executing: ");
  for (const auto &arg : arrArgs)
    printf("%s ", arg.c_str());
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
    if (!arrArgs.empty())
    {
      char **args = (char **)alloca(sizeof(char *) * (arrArgs.size() + 3));
      memset(args, 0, (sizeof(char *) * (arrArgs.size() + 3)));
      for (size_t i=0; i<arrArgs.size(); i++)
        args[i] = const_cast<char *>(arrArgs[i].c_str());
      execvp(args[0], args);
    }
  }
  else
  {
    waitpid(child, &n, 0);
  }

  return (waitExit) ? (WEXITSTATUS(n) == 0) : true;
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

std::string CUtil::ResolveExecutablePath()
{
  std::string strExecutablePath;
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
  size_t path_size =2*MAXPATHLEN;

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
#elif defined(TARGET_ANDROID)
  strExecutablePath = CXBMCApp::getApplicationInfo().nativeLibraryDir;

  std::string appName = CCompileInfo::GetAppName();
  std::string libName = "lib" + appName + ".so";
  StringUtils::ToLower(libName);
  strExecutablePath += "/" + libName;
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

std::string CUtil::GetFrameworksPath(bool forPython)
{
  std::string strFrameworksPath;
#if defined(TARGET_DARWIN)
  strFrameworksPath = CDarwinUtils::GetFrameworkPath(forPython);
#endif
  return strFrameworksPath;
}

void CUtil::GetVideoBasePathAndFileName(const std::string& videoPath,
                                        std::string& basePath,
                                        std::string& videoFileName)
{
  const CFileItem item(videoPath, false);

  if (item.IsOpticalMediaFile() || URIUtils::IsBlurayPath(item.GetDynPath()))
  {
    basePath = item.GetLocalMetadataPath();
    videoFileName = URIUtils::ReplaceExtension(GetTitleFromPath(basePath), "");
  }
  else
  {
    videoFileName = URIUtils::ReplaceExtension(URIUtils::GetFileName(videoPath), "");
    basePath = URIUtils::GetBasePath(videoPath);
  }
}

void CUtil::GetItemsToScan(const std::string& videoPath,
                           const std::string& item_exts,
                           const std::vector<std::string>& sub_dirs,
                           CFileItemList& items)
{
  int flags = DIR_FLAG_NO_FILE_DIRS | DIR_FLAG_NO_FILE_INFO;

  if (!videoPath.empty())
    CDirectory::GetDirectory(videoPath, items, item_exts, flags);

  std::vector<std::string> additionalPaths;
  for (const auto &item : items)
  {
    for (const auto& subdir : sub_dirs)
    {
      if (StringUtils::EqualsNoCase(item->GetLabel(), subdir))
        additionalPaths.push_back(item->GetPath());
    }
  }

  for (std::vector<std::string>::const_iterator it = additionalPaths.begin(); it != additionalPaths.end(); ++it)
  {
    CFileItemList moreItems;
    CDirectory::GetDirectory(*it, moreItems, item_exts, flags);
    items.Append(moreItems);
  }
}


void CUtil::ScanPathsForAssociatedItems(const std::string& videoName,
                                        const CFileItemList& items,
                                        const std::vector<std::string>& item_exts,
                                        std::vector<std::string>& associatedFiles)
{
  for (const auto &pItem : items)
  {
    if (pItem->m_bIsFolder)
      continue;

    std::string strCandidate = URIUtils::GetFileName(pItem->GetPath());

    // skip duplicates
    if (std::find(associatedFiles.begin(), associatedFiles.end(), pItem->GetPath()) != associatedFiles.end())
      continue;

    URIUtils::RemoveExtension(strCandidate);
    // NOTE: We don't know if one of videoName or strCandidate is URL-encoded and the other is not, so try both
    if (StringUtils::StartsWithNoCase(strCandidate, videoName) || (StringUtils::StartsWithNoCase(strCandidate, CURL::Decode(videoName))))
    {
      if (URIUtils::IsRAR(pItem->GetPath()) || URIUtils::IsZIP(pItem->GetPath()))
        CUtil::ScanArchiveForAssociatedItems(pItem->GetPath(), "", item_exts, associatedFiles);
      else
      {
        associatedFiles.push_back(pItem->GetPath());
        CLog::Log(LOGINFO, "{}: found associated file {}", __FUNCTION__,
                  CURL::GetRedacted(pItem->GetPath()));
      }
    }
    else
    {
      if (URIUtils::IsRAR(pItem->GetPath()) || URIUtils::IsZIP(pItem->GetPath()))
        CUtil::ScanArchiveForAssociatedItems(pItem->GetPath(), videoName, item_exts, associatedFiles);
    }
  }
}

int CUtil::ScanArchiveForAssociatedItems(const std::string& strArchivePath,
                                         const std::string& videoNameNoExt,
                                         const std::vector<std::string>& item_exts,
                                         std::vector<std::string>& associatedFiles)
{
  CLog::LogF(LOGDEBUG, "Scanning archive {}", CURL::GetRedacted(strArchivePath));
  int nItemsAdded = 0;
  CFileItemList ItemList;

  // zip only gets the root dir
  if (URIUtils::HasExtension(strArchivePath, ".zip"))
  {
    CURL pathToUrl(strArchivePath);
    CURL zipURL = URIUtils::CreateArchivePath("zip", pathToUrl, "");
    if (!CDirectory::GetDirectory(zipURL, ItemList, "", DIR_FLAG_NO_FILE_DIRS))
      return false;
  }
  else if (URIUtils::HasExtension(strArchivePath, ".rar"))
  {
    CURL pathToUrl(strArchivePath);
    CURL rarURL = URIUtils::CreateArchivePath("rar", pathToUrl, "");
    if (!CDirectory::GetDirectory(rarURL, ItemList, "", DIR_FLAG_NO_FILE_DIRS))
      return false;
  }
  for (const auto &item : ItemList)
  {
    std::string strPathInRar = item->GetPath();
    std::string strExt = URIUtils::GetExtension(strPathInRar);

    // Check another archive in archive
    if (strExt == ".zip" || strExt == ".rar")
    {
      nItemsAdded +=
          ScanArchiveForAssociatedItems(strPathInRar, videoNameNoExt, item_exts, associatedFiles);
      continue;
    }

    // check that the found filename matches the movie filename
    size_t fnl = videoNameNoExt.size();
    // NOTE: We don't know if videoNameNoExt is URL-encoded, so try both
    if (fnl &&
      !(StringUtils::StartsWithNoCase(URIUtils::GetFileName(strPathInRar), videoNameNoExt) ||
        StringUtils::StartsWithNoCase(URIUtils::GetFileName(strPathInRar), CURL::Decode(videoNameNoExt))))
      continue;

    for (const auto& ext : item_exts)
    {
      if (StringUtils::EqualsNoCase(strExt, ext))
      {
        CLog::Log(LOGINFO, "{}: found associated file {}", __FUNCTION__,
                  CURL::GetRedacted(strPathInRar));
        associatedFiles.push_back(strPathInRar);
        nItemsAdded++;
        break;
      }
    }
  }

  return nItemsAdded;
}

void CUtil::ScanForExternalSubtitles(const std::string& strMovie, std::vector<std::string>& vecSubtitles)
{
  auto start = std::chrono::steady_clock::now();

  CFileItem item(strMovie, false);
  if ((NETWORK::IsInternetStream(item) && !URIUtils::IsOnLAN(item.GetDynPath())) ||
      PLAYLIST::IsPlayList(item) || item.IsPVR() || !VIDEO::IsVideo(item))
    return;

  CLog::Log(LOGDEBUG, "{}: Searching for subtitles...", __FUNCTION__);

  std::string strBasePath;
  std::string strSubtitle;

  GetVideoBasePathAndFileName(strMovie, strBasePath, strSubtitle);

  CFileItemList items;
  const std::vector<std::string> common_sub_dirs = { "subs", "subtitles", "vobsubs", "sub", "vobsub", "subtitle" };
  const std::string subtitleExtensions = CServiceBroker::GetFileExtensionProvider().GetSubtitleExtensions();
  GetItemsToScan(strBasePath, subtitleExtensions, common_sub_dirs, items);

  const std::string customPath = CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(CSettings::SETTING_SUBTITLES_CUSTOMPATH);

  if (!CMediaSettings::GetInstance().GetAdditionalSubtitleDirectoryChecked() && !customPath.empty()) // to avoid checking non-existent directories (network) every time..
  {
    if (!CServiceBroker::GetNetwork().IsAvailable() && !URIUtils::IsHD(customPath))
    {
      CLog::Log(LOGINFO, "CUtil::CacheSubtitles: disabling alternate subtitle directory for this session, it's inaccessible");
      CMediaSettings::GetInstance().SetAdditionalSubtitleDirectoryChecked(-1); // disabled
    }
    else if (!CDirectory::Exists(customPath))
    {
      CLog::Log(LOGINFO, "CUtil::CacheSubtitles: disabling alternate subtitle directory for this session, it's nonexistent");
      CMediaSettings::GetInstance().SetAdditionalSubtitleDirectoryChecked(-1); // disabled
    }

    CMediaSettings::GetInstance().SetAdditionalSubtitleDirectoryChecked(1);
  }

  std::vector<std::string> strLookInPaths;
  // this is last because we dont want to check any common subdirs or cd-dirs in the alternate <subtitles> dir.
  if (CMediaSettings::GetInstance().GetAdditionalSubtitleDirectoryChecked() == 1)
  {
    std::string strPath2 = customPath;
    URIUtils::AddSlashAtEnd(strPath2);
    strLookInPaths.push_back(strPath2);
  }

  int flags = DIR_FLAG_NO_FILE_DIRS | DIR_FLAG_NO_FILE_INFO;
  for (const std::string& path : strLookInPaths)
  {
    CFileItemList moreItems;
    CDirectory::GetDirectory(path, moreItems, subtitleExtensions, flags);
    items.Append(moreItems);
  }

  std::vector<std::string> exts = StringUtils::Split(subtitleExtensions, '|');
  exts.erase(std::remove(exts.begin(), exts.end(), ".zip"), exts.end());
  exts.erase(std::remove(exts.begin(), exts.end(), ".rar"), exts.end());

  ScanPathsForAssociatedItems(strSubtitle, items, exts, vecSubtitles);

  size_t iSize = vecSubtitles.size();
  for (size_t i = 0; i < iSize; i++)
  {
    if (URIUtils::HasExtension(vecSubtitles[i], ".smi"))
    {
      //Cache multi-language sami subtitle
      CDVDSubtitleStream stream;
      if (stream.Open(vecSubtitles[i]))
      {
        CDVDSubtitleTagSami TagConv;
        TagConv.LoadHead(&stream);
        if (TagConv.m_Langclass.size() >= 2)
        {
          for (const auto &lang : TagConv.m_Langclass)
          {
            std::string strDest =
                StringUtils::Format("special://temp/subtitle.{}.{}.smi", lang.Name, i);
            if (CFile::Copy(vecSubtitles[i], strDest))
            {
              CLog::Log(LOGINFO, " cached subtitle {}->{}", CURL::GetRedacted(vecSubtitles[i]),
                        strDest);
              vecSubtitles.push_back(strDest);
            }
          }
        }
      }
    }
  }

  auto end = std::chrono::steady_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
  CLog::Log(LOGDEBUG, "{}: END (total time: {} ms)", __FUNCTION__, duration.count());
}

ExternalStreamInfo CUtil::GetExternalStreamDetailsFromFilename(const std::string& videoPath, const std::string& associatedFile)
{
  ExternalStreamInfo info;

  std::string videoBaseName = URIUtils::GetFileName(videoPath);
  URIUtils::RemoveExtension(videoBaseName);

  std::string toParse = URIUtils::GetFileName(associatedFile);
  URIUtils::RemoveExtension(toParse);

  // we check left part - if it's same as video base name - strip it
  if (StringUtils::StartsWithNoCase(toParse, videoBaseName))
    toParse = toParse.substr(videoBaseName.length());
  else if (URIUtils::GetExtension(associatedFile) == ".sub" && URIUtils::IsInArchive(associatedFile))
  {
    // exclude parsing of vobsub file names that are embedded in an archive
    CLog::Log(LOGDEBUG, "{} - skipping archived vobsub filename parsing: {}", __FUNCTION__,
              CURL::GetRedacted(associatedFile));
    toParse.clear();
  }

  // trim any non-alphanumeric char in the beginning
  std::string::iterator result = std::find_if(toParse.begin(), toParse.end(), StringUtils::isasciialphanum);

  std::string name;
  if (result != toParse.end()) // if we have anything to parse
  {
    std::string inputString(result, toParse.end());
    std::string delimiters(" .-");
    std::vector<std::string> tokens;
    StringUtils::Tokenize(inputString, tokens, delimiters);

    for (auto it = tokens.rbegin(); it != tokens.rend(); ++it)
    {
      // try to recognize a flag
      std::string flag_tmp(*it);
      StringUtils::ToLower(flag_tmp);
      if (!flag_tmp.compare("none"))
      {
        info.flag |= StreamFlags::FLAG_NONE;
        continue;
      }
      else if (!flag_tmp.compare("default"))
      {
        info.flag |= StreamFlags::FLAG_DEFAULT;
        continue;
      }
      else if (!flag_tmp.compare("forced"))
      {
        info.flag |= StreamFlags::FLAG_FORCED;
        continue;
      }
      else if (!flag_tmp.compare("original"))
      {
        info.flag |= StreamFlags::FLAG_ORIGINAL;
        continue;
      }
      else if (!flag_tmp.compare("impaired"))
      {
        info.flag |= StreamFlags::FLAG_HEARING_IMPAIRED;
        continue;
      }

      if (info.language.empty())
      {
        std::string langCode;
        // try to recognize language
        if (g_LangCodeExpander.ConvertToISO6392B(*it, langCode))
        {
          info.language = langCode;
          continue;
        }
      }

      name = (*it) + " " + name;
    }
  }
  name += " ";
  name += g_localizeStrings.Get(21602); // External
  StringUtils::Trim(name);
  info.name = StringUtils::RemoveDuplicatedSpacesAndTabs(name);
  if (info.flag == 0)
    info.flag = StreamFlags::FLAG_NONE;

  CLog::Log(LOGDEBUG, "{} - Language = '{}' / Name = '{}' / Flag = '{}' from {}", __FUNCTION__,
            info.language, info.name, info.flag, CURL::GetRedacted(associatedFile));

  return info;
}

/*! \brief in a vector of subtitles finds the corresponding .sub file for a given .idx file
 */
bool CUtil::FindVobSubPair(const std::vector<std::string>& vecSubtitles, const std::string& strIdxPath, std::string& strSubPath)
{
  if (URIUtils::HasExtension(strIdxPath, ".idx"))
  {
    std::string strIdxFile;
    std::string strIdxDirectory;
    URIUtils::Split(strIdxPath, strIdxDirectory, strIdxFile);
    for (const auto &subtitlePath : vecSubtitles)
    {
      std::string strSubFile;
      std::string strSubDirectory;
      URIUtils::Split(subtitlePath, strSubDirectory, strSubFile);
      if (URIUtils::IsInArchive(subtitlePath))
        strSubDirectory = CURL::Decode(strSubDirectory);
      if (URIUtils::HasExtension(strSubFile, ".sub") &&
          (URIUtils::PathEquals(URIUtils::ReplaceExtension(strIdxPath,""),
                                URIUtils::ReplaceExtension(subtitlePath,"")) ||
           (strSubDirectory.size() >= 11 &&
            StringUtils::EqualsNoCase(strSubDirectory.substr(6, strSubDirectory.length()-11), URIUtils::ReplaceExtension(strIdxPath,"")))))
      {
        strSubPath = subtitlePath;
        return true;
      }
    }
  }
  return false;
}

/*! \brief checks if in the vector of subtitles the given .sub file has a corresponding idx and hence is a vobsub file
 */
bool CUtil::IsVobSub(const std::vector<std::string>& vecSubtitles, const std::string& strSubPath)
{
  if (URIUtils::HasExtension(strSubPath, ".sub"))
  {
    std::string strSubFile;
    std::string strSubDirectory;
    URIUtils::Split(strSubPath, strSubDirectory, strSubFile);
    if (URIUtils::IsInArchive(strSubPath))
      strSubDirectory = CURL::Decode(strSubDirectory);
    for (const auto &subtitlePath : vecSubtitles)
    {
      std::string strIdxFile;
      std::string strIdxDirectory;
      URIUtils::Split(subtitlePath, strIdxDirectory, strIdxFile);
      if (URIUtils::HasExtension(strIdxFile, ".idx") &&
          (URIUtils::PathEquals(URIUtils::ReplaceExtension(subtitlePath,""),
                                URIUtils::ReplaceExtension(strSubPath,"")) ||
           (strSubDirectory.size() >= 11 &&
            StringUtils::EqualsNoCase(strSubDirectory.substr(6, strSubDirectory.length()-11), URIUtils::ReplaceExtension(subtitlePath,"")))))
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
  for (const std::string& archType : archTypes)
  {
    vobSub = URIUtils::CreateArchivePath(archType,
                                         CURL(URIUtils::ReplaceExtension(vobSubIdx, std::string(".") + archType)),
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

void CUtil::ScanForExternalAudio(const std::string& videoPath, std::vector<std::string>& vecAudio)
{
  CFileItem item(videoPath, false);
  if (NETWORK::IsInternetStream(item) || PLAYLIST::IsPlayList(item) || item.IsLiveTV() ||
      item.IsPVR() || !VIDEO::IsVideo(item))
    return;

  std::string strBasePath;
  std::string strAudio;

  GetVideoBasePathAndFileName(videoPath, strBasePath, strAudio);

  CFileItemList items;
  const std::vector<std::string> common_sub_dirs = { "audio", "tracks"};
  GetItemsToScan(strBasePath, CServiceBroker::GetFileExtensionProvider().GetMusicExtensions(), common_sub_dirs, items);

  std::vector<std::string> exts = StringUtils::Split(CServiceBroker::GetFileExtensionProvider().GetMusicExtensions(), "|");

  CVideoDatabase database;
  database.Open();
  bool useAllExternalAudio = database.GetUseAllExternalAudioForVideo(videoPath);

  if (useAllExternalAudio)
  {
    for (const auto& audioItem : items.GetList())
    {
      vecAudio.push_back(audioItem.get()->GetPath());
    }
  }
  else
    ScanPathsForAssociatedItems(strAudio, items, exts, vecAudio);
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
#if !defined(TARGET_WINDOWS)
  return rand_r(&s_randomSeed);
#else
  unsigned int number;
  if (rand_s(&number) == 0)
    return (int)number;

  return rand();
#endif
}

void CUtil::CopyUserDataIfNeeded(const std::string& strPath,
                                 const std::string& file,
                                 const std::string& destname)
{
  std::string destPath;
  if (destname.empty())
    destPath = URIUtils::AddFileToFolder(strPath, file);
  else
    destPath = URIUtils::AddFileToFolder(strPath, destname);

  if (!CFile::Exists(destPath))
  {
    // need to copy it across
    std::string srcPath = URIUtils::AddFileToFolder("special://xbmc/userdata/", file);
    CFile::Copy(srcPath, destPath);
  }
}
