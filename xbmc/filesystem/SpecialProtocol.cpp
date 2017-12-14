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

#include "SpecialProtocol.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "Util.h"
#include "guilib/GraphicContext.h"
#include "profiles/ProfilesManager.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include "Application.h"
#include "filesystem/Directory.h"
#include <cassert>

#ifdef TARGET_POSIX
#include <dirent.h>
#include "utils/StringUtils.h"
#include "CompileInfo.h"
#endif

#ifdef TARGET_WINDOWS
#include "platform/win32/WIN32Util.h"
#endif

#ifdef TARGET_DARWIN
#include "platform/darwin/DarwinUtils.h"
#endif

#ifdef TARGET_ANDROID
#include "platform/android/activity/XBMCApp.h"
#include <androidjni/ApplicationInfo.h>
#include "platform/android/bionic_supplement/bionic_supplement.h"
#endif

using namespace XFILE;

/*Specific Path Getters*/

std::string CSpecialProtocol::GetProfilePath()
{
  return CProfilesManager::GetInstance().GetProfileUserDataFolder();
}

std::string CSpecialProtocol::GetProfilePath(const std::string &fileName)
{
  return URIUtils::AddFileToFolder(GetProfilePath(), fileName);
}


std::string CSpecialProtocol::GetMasterProfilePath()
{
  std::string path;
#if defined (TARGET_WINDOWS)
  path = URIUtils::AddFileToFolder(CWIN32Util::GetProfilePath(), "userdata");
#endif
#if defined(TARGET_POSIX) && !defined(TARGET_DARWIN)
  if (g_application.PlatformDirectoriesEnabled())
  {
    path = GetHomePath() + "/userdata";
  }
  else
  {
    path = URIUtils::AddFileToFolder(GetXBMCPath(), "portable_data/userdata");
  }
#endif

#ifdef TARGET_DARWIN
  if (g_application.PlatformDirectoriesEnabled())
  {
    // REVIEW: Make a get function for userHome calc
    std::string userHome;
    if (getenv("HOME"))
      userHome = getenv("HOME");
    else
      userHome = "/root";

#if defined(TARGET_DARWIN_IOS)
    path = userHome + "/" + CDarwinUtils::GetAppRootFolder() + "/" + CCompileInfo::GetAppName() + "/userdata";
#else
    path = userHome + "/Library/Application Support/" + CCompileInfo::GetAppName() + "/userdata";
#endif
  }
  else
  {
    path = URIUtils::AddFileToFolder(GetXBMCPath(), "portable_data");
  }

#endif
  return path;
}

std::string CSpecialProtocol::GetMasterProfilePath(const std::string &fileName)
{
  return URIUtils::AddFileToFolder(GetMasterProfilePath(), fileName);
}


std::string CSpecialProtocol::GetXBMCPath()
{
  std::string path;
#if defined (TARGET_WINDOWS)
  path = CUtil::GetHomePath();
#endif

#if defined(TARGET_POSIX) && !defined(TARGET_DARWIN)
  path = INSTALL_PATH;
  /* Check if binaries and arch independent data files are being kept in
  * separate locations. */
  if (!CDirectory::Exists(URIUtils::AddFileToFolder(path, "userdata")))
  {
    /* Attempt to locate arch independent data files. */
    auto appBinPath = CUtil::GetHomePath("KODI_BIN_HOME");
    path = CUtil::GetHomePath(appBinPath);
    if (!g_application.PlatformDirectoriesEnabled())
    {
      URIUtils::AddSlashAtEnd(path);
    }
  }
#endif
#if defined (TARGET_DARWIN)
  path = CUtil::GetHomePath();
  if (!g_application.PlatformDirectoriesEnabled())
  {
    URIUtils::AddSlashAtEnd(path);
  }
#endif
  return path;
}

std::string CSpecialProtocol::GetXBMCPath(const std::string &fileName)
{
  return URIUtils::AddFileToFolder(GetXBMCPath(), fileName);
}


std::string CSpecialProtocol::GetXBMCBinPath()
{
  std::string path;
#ifdef TARGET_WINDOWS
  path = GetXBMCPath();
#endif

#if defined(TARGET_POSIX) && !defined(TARGET_DARWIN)
  path = CUtil::GetHomePath("KODI_BIN_HOME");
  if (!g_application.PlatformDirectoriesEnabled())
  {
    URIUtils::AddSlashAtEnd(path);
  }
#endif
#ifdef TARGET_DARWIN
  path = GetXBMCPath();
  if (!g_application.PlatformDirectoriesEnabled())
  {
    URIUtils::AddSlashAtEnd(path);
  }
#endif
  return path;
}

std::string CSpecialProtocol::GetXBMCBinPath(const std::string &fileName)
{
  return URIUtils::AddFileToFolder(GetXBMCBinPath(), fileName);
}


std::string CSpecialProtocol::GetXBMCBinAddonPath()
{
  std::string path;
#ifdef TARGET_WINDOWS
  path = GetXBMCPath("/addons");
#endif
#if defined(TARGET_POSIX) && !defined(TARGET_ANDROID)
  path = GetXBMCBinPath() + "/addons";
#endif
#ifdef TARGET_ANDROID
  path = CXBMCApp::getApplicationInfo().nativeLibraryDir.c_str();
#endif
  return path;
}

std::string CSpecialProtocol::GetXBMCBinAddonPath(const std::string &fileName)
{
  return URIUtils::AddFileToFolder(GetXBMCBinAddonPath(), fileName);
}


std::string CSpecialProtocol::GetXBMCAltBinAddonPath()
{
  std::string path;
#if defined(TARGET_POSIX)
  /* REVIEW:
  if (getenv("KODI_BINADDON_PATH")) // don't see why to do this check. what to do if false?
  */
  path = getenv("KODI_BINADDON_PATH");
  if (!g_application.PlatformDirectoriesEnabled())
  {
    URIUtils::AddSlashAtEnd(path);
  }
#endif
  // REVIEW: Maybe it is appropriete to ASSERT/throw exception here.
  return path;
}

std::string CSpecialProtocol::GetXBMCAltBinAddonPath(const std::string &fileName)
{
  return URIUtils::AddFileToFolder(GetXBMCAltBinAddonPath(), fileName);
}


std::string CSpecialProtocol::GetHomePath() 
{
  std::string path;
#ifdef TARGET_WINDOWS
  path = CWIN32Util::GetProfilePath();
#endif
#if defined(TARGET_POSIX) && !defined(TARGET_DARWIN)
  if (g_application.PlatformDirectoriesEnabled()) 
  {
    std::string userHome;
    std::string appName = CCompileInfo::GetAppName();
    std::string dotLowerAppName = "." + appName;
    StringUtils::ToLower(dotLowerAppName);
    // REVIEW: Make a get function for userHome calc
    if (getenv("HOME"))
      userHome = getenv("HOME");
    else
      userHome = "/root";
    path = userHome + "/" + dotLowerAppName;
  }
  else
  {
    path = URIUtils::AddFileToFolder(GetXBMCPath(), "portable_data");
  }
#endif
#if defined(TARGET_DARWIN)
  if (g_application.PlatformDirectoriesEnabled())
  {
    // REVIEW: Make a get function for userHome calc
    std::string userHome;
    if (getenv("HOME"))
      userHome = getenv("HOME");
    else
      userHome = "/root";
#if defined(TARGET_DARWIN_IOS)
    path = userHome + "/" + CDarwinUtils::GetAppRootFolder() + "/" + CCompileInfo::GetAppName();
#else
    path = userHome + "/Library/Application Support/" + CCompileInfo::GetAppName();
#endif
  }
  else
  {
    path = URIUtils::AddFileToFolder(GetXBMCPath(), "portable_data");
  }
#endif
  return path;
}

std::string CSpecialProtocol::GetHomePath(const std::string &fileName) 
{
  return URIUtils::AddFileToFolder(GetHomePath(), fileName);
}

std::string CSpecialProtocol::GetEnvHomePath()
{
  std::string path;
#if defined(TARGET_POSIX)
  path =  getenv("HOME");
#endif
  return path;
}

std::string CSpecialProtocol::GetEnvHomePath(const std::string &fileName)
{
  return URIUtils::AddFileToFolder(GetEnvHomePath(), fileName);
}


std::string CSpecialProtocol::GetTmpPath(const std::string &fileName)
{
  return URIUtils::AddFileToFolder(GetTmpPath(), fileName);
}

std::string CSpecialProtocol::GetTmpPath() 
{
  std::string path;
#ifdef TARGET_WINDOWS
  path = URIUtils::AddFileToFolder(CWIN32Util::GetProfilePath(), "cache");
#endif
#if defined(TARGET_POSIX) && !defined(TARGET_DARWIN)
  const char* envAppTemp = "KODI_TEMP";
  // REVIEW: Make a get function for userHome calc
  std::string userHome;
  if (getenv("HOME"))
    userHome = getenv("HOME");
  else
    userHome = "/root";

  if (g_application.PlatformDirectoriesEnabled())
  {
    std::string strTempPath = userHome;
    std::string appName = CCompileInfo::GetAppName();
    // REVIEW: Make a get function for dotLowerAppName calc
    std::string dotLowerAppName = "." + appName;
    StringUtils::ToLower(dotLowerAppName);
    strTempPath = URIUtils::AddFileToFolder(strTempPath, dotLowerAppName + "/temp");
    if (getenv(envAppTemp))
      strTempPath = getenv(envAppTemp);
    path = strTempPath;
  }
  else
  {
    std::string strTempPath = GetXBMCPath();
    strTempPath = URIUtils::AddFileToFolder(strTempPath, "portable_data/temp");
    if (getenv(envAppTemp))
      strTempPath = getenv(envAppTemp);
    path = strTempPath;
  }
#endif
#ifdef TARGET_DARWIN
  if (g_application.PlatformDirectoriesEnabled())
  {
    // REVIEW: Make a get function for dotLowerAppName calc
    std::string appName = CCompileInfo::GetAppName();
    std::string dotLowerAppName = "." + appName;
    StringUtils::ToLower(dotLowerAppName);
    // REVIEW: Make a get function for userHome calc
    std::string userHome;
    if (getenv("HOME"))
      userHome = getenv("HOME");
    else
      userHome = "/root";

#if defined(TARGET_DARWIN_IOS)
    std::string strTempPath = URIUtils::AddFileToFolder(userHome, std::string(CDarwinUtils::GetAppRootFolder()) + "/" + CCompileInfo::GetAppName() + "/temp");
#else
    std::string strTempPath = URIUtils::AddFileToFolder(userHome, dotLowerAppName + "/");
    CDirectory::Create(strTempPath);
    strTempPath = URIUtils::AddFileToFolder(userHome, dotLowerAppName + "/temp");
#endif
    path = strTempPath;
  }
  else
  {
    path = URIUtils::AddFileToFolder(GetXBMCPath(), "portable_data/temp");
  }
#endif
  return path;
}


std::string CSpecialProtocol::GetLogPath()
{
  std::string path;
#ifdef TARGET_WINDOWS
  path = GetHomePath();
#endif

#if defined(TARGET_POSIX) && !defined(TARGET_DARWIN)
  path = GetTmpPath();
#endif

#if defined(TARGET_DARWIN)
  if (g_application.PlatformDirectoriesEnabled())
  {
    // REVIEW: Make a get function for userHome calc
    std::string userHome;
    if (getenv("HOME"))
      userHome = getenv("HOME");
    else
      userHome = "/root";

#if defined(TARGET_DARWIN_IOS)
    std::string strTempPath = userHome + "/" + std::string(CDarwinUtils::GetAppRootFolder());
#else
    std::string strTempPath = userHome + "/Library/Logs";
#endif
    path = strTempPath;
  }
  else
  {
    path = GetTmpPath();
  }
#endif
  return path;
}

std::string CSpecialProtocol::GetLogPath(const std::string &fileName)
{
  return URIUtils::AddFileToFolder(GetLogPath(), fileName);
}

// returns path to our internal dylibs
std::string CSpecialProtocol::GetXBMCFrameworksPath(const std::string &fileName)
{
  return URIUtils::AddFileToFolder(GetXBMCFrameworksPath(), fileName);
}

std::string CSpecialProtocol::GetXBMCFrameworksPath()
{
  std::string path;
#ifdef TARGET_DARWIN
  path = CUtil::GetFrameworksPath();
#endif
  return path;
}


void CSpecialProtocol::SetProfilePath(const std::string &dir)
{
  SetPath("profile", dir);
  CLog::Log(LOGNOTICE, "special://profile/ is mapped to: %s", GetPath("profile").c_str());
}

void CSpecialProtocol::SetXBMCPath(const std::string &dir)
{
  SetPath("xbmc", dir);
}

void CSpecialProtocol::SetXBMCBinPath(const std::string &dir)
{
  SetPath("xbmcbin", dir);
}

void CSpecialProtocol::SetXBMCBinAddonPath(const std::string &dir)
{
  SetPath("xbmcbinaddons", dir);
}

void CSpecialProtocol::SetXBMCAltBinAddonPath(const std::string &dir)
{
  SetPath("xbmcaltbinaddons", dir);
}

void CSpecialProtocol::SetXBMCFrameworksPath(const std::string &dir)
{
  SetPath("frameworks", dir);
}

void CSpecialProtocol::SetHomePath(const std::string &dir)
{
  SetPath("home", dir);
}

void CSpecialProtocol::SetUserHomePath(const std::string &dir)
{
  SetPath("userhome", dir);
}

void CSpecialProtocol::SetEnvHomePath(const std::string &dir)
{
  SetPath("envhome", dir);
}

void CSpecialProtocol::SetMasterProfilePath(const std::string &dir)
{
  SetPath("masterprofile", dir);
}

void CSpecialProtocol::SetTempPath(const std::string &dir)
{
  SetPath("temp", dir);
}

void CSpecialProtocol::SetLogPath(const std::string &dir)
{
  SetPath("logpath", dir);
}

bool CSpecialProtocol::ComparePath(const std::string &path1, const std::string &path2)
{
  return TranslatePath(path1) == TranslatePath(path2);
}

std::string CSpecialProtocol::TranslatePath(const std::string &path)
{
  CURL url(path);
  // check for special-protocol, if not, return
  if (!url.IsProtocol("special"))
  {
    return path;
  }
  return TranslatePath(url);
}

std::string CSpecialProtocol::TranslatePath(const CURL &url)
{
  // check for special-protocol, if not, return
  if (!url.IsProtocol("special"))
  {
#if defined(TARGET_POSIX) && defined(_DEBUG)
    std::string path(url.Get());
    if (path.length() >= 2 && path[1] == ':')
    {
      CLog::Log(LOGWARNING, "Trying to access old style dir: %s\n", path.c_str());
     // printf("Trying to access old style dir: %s\n", path.c_str());
    }
#endif

    return url.Get();
  }

  std::string FullFileName = url.GetFileName();
  std::string translatedPath;
  std::string FileName;
  std::string RootDir;

  // Split up into the special://root and the rest of the filename
  size_t pos = FullFileName.find('/');
  if (pos != std::string::npos && pos > 1)
  {
    RootDir = FullFileName.substr(0, pos);

    if (pos < FullFileName.size())
      FileName = FullFileName.substr(pos + 1);
  }
  else
    RootDir = FullFileName;

  if (RootDir == "subtitles")
    translatedPath = URIUtils::AddFileToFolder(CServiceBroker::GetSettings().GetString(CSettings::SETTING_SUBTITLES_CUSTOMPATH), FileName);
  else if (RootDir == "userdata")
    translatedPath = URIUtils::AddFileToFolder(CProfilesManager::GetInstance().GetUserDataFolder(), FileName);
  else if (RootDir == "database")
    translatedPath = URIUtils::AddFileToFolder(CProfilesManager::GetInstance().GetDatabaseFolder(), FileName);
  else if (RootDir == "thumbnails")
    translatedPath = URIUtils::AddFileToFolder(CProfilesManager::GetInstance().GetThumbnailsFolder(), FileName);
  else if (RootDir == "recordings" || RootDir == "cdrips")
    translatedPath = URIUtils::AddFileToFolder(CServiceBroker::GetSettings().GetString(CSettings::SETTING_AUDIOCDS_RECORDINGPATH), FileName);
  else if (RootDir == "screenshots")
    translatedPath = URIUtils::AddFileToFolder(CServiceBroker::GetSettings().GetString(CSettings::SETTING_DEBUG_SCREENSHOTPATH), FileName);
  else if (RootDir == "musicplaylists")
    translatedPath = URIUtils::AddFileToFolder(CUtil::MusicPlaylistsLocation(), FileName);
  else if (RootDir == "videoplaylists")
    translatedPath = URIUtils::AddFileToFolder(CUtil::VideoPlaylistsLocation(), FileName);
  else if (RootDir == "skin")
    translatedPath = URIUtils::AddFileToFolder(g_graphicsContext.GetMediaDir(), FileName);

  /* Static path map isn't used anymore. Instead we use specific getters.
  (for alleviating the early destruction of the map when exiting Kodi)
  Note:  "userhome" is not included since its setter is not called from anywhere.
  */
  else if (RootDir == "profile")
  {
    translatedPath = GetProfilePath(FileName);
  }
  else if (RootDir == "masterprofile")
  {
    translatedPath = GetMasterProfilePath(FileName);
  }
  else if (RootDir == "xbmc")
  {
    translatedPath = GetXBMCPath(FileName);
  }
  else if (RootDir == "xbmcbin")
  {
    translatedPath = GetXBMCBinPath(FileName);
  }
  else if (RootDir == "xbmcbinaddons")
  {
    translatedPath = GetXBMCBinAddonPath(FileName);
  }
  else if (RootDir == "xbmcaltbinaddons")
  {
    translatedPath = GetXBMCAltBinAddonPath(FileName);
  }
  else if (RootDir == "home")
  {
    translatedPath = GetHomePath(FileName);
  }
  else if (RootDir == "envhome")
  {
    translatedPath = GetEnvHomePath(FileName);
  }
  else if (RootDir == "logpath")
  {
    translatedPath = GetLogPath(FileName);
  }
  else if (RootDir == "temp")
  {
    translatedPath = GetTmpPath(FileName);
  }

  // check if we need to recurse in
  if (URIUtils::IsSpecial(translatedPath))
  { // we need to recurse in, as there may be multiple translations required
    return TranslatePath(translatedPath);
  }

  // Validate the final path, just in case
  return CUtil::ValidatePath(translatedPath);
}

std::string CSpecialProtocol::TranslatePathConvertCase(const std::string& path)
{
  std::string translatedPath = TranslatePath(path);

#ifdef TARGET_POSIX
  if (translatedPath.find("://") != std::string::npos)
    return translatedPath;

  // If the file exists with the requested name, simply return it
  struct stat stat_buf;
  if (stat(translatedPath.c_str(), &stat_buf) == 0)
    return translatedPath;

  std::string result;
  std::vector<std::string> tokens;
  StringUtils::Tokenize(translatedPath, tokens, "/");
  std::string file;
  DIR* dir;
  struct dirent* de;

  for (unsigned int i = 0; i < tokens.size(); i++)
  {
    file = result + "/";
    file += tokens[i];
    if (stat(file.c_str(), &stat_buf) == 0)
    {
      result += "/" + tokens[i];
    }
    else
    {
      dir = opendir(result.c_str());
      if (dir)
      {
        while ((de = readdir(dir)) != NULL)
        {
          // check if there's a file with same name but different case
          if (strcasecmp(de->d_name, tokens[i].c_str()) == 0)
          {
            result += "/";
            result += de->d_name;
            break;
          }
        }

        // if we did not find any file that somewhat matches, just
        // fallback but we know it's not gonna be a good ending
        if (de == NULL)
          result += "/" + tokens[i];

        closedir(dir);
      }
      else
      { // this is just fallback, we won't succeed anyway...
        result += "/" + tokens[i];
      }
    }
 }

  return result;
#else
  return translatedPath;
#endif
}

void CSpecialProtocol::LogPaths()
{
//#ifdef TARGET_WINDOWS
  CLog::Log(LOGNOTICE, "special://xbmc/ is mapped to: %s", GetXBMCPath().c_str());
  CLog::Log(LOGNOTICE, "special://xbmcbin/ is mapped to: %s", GetXBMCBinPath().c_str());
  CLog::Log(LOGNOTICE, "special://xbmcbinaddons/ is mapped to: %s", GetXBMCBinAddonPath().c_str());
  CLog::Log(LOGNOTICE, "special://masterprofile/ is mapped to: %s", GetMasterProfilePath().c_str());
  CLog::Log(LOGNOTICE, "special://home/ is mapped to: %s", GetHomePath().c_str());
  CLog::Log(LOGNOTICE, "special://temp/ is mapped to: %s", GetTmpPath().c_str());
  CLog::Log(LOGNOTICE, "special://logpath/ is mapped to: %s", GetHomePath().c_str());
 #if defined(TARGET_POSIX)
  CLog::Log(LOGNOTICE, "special://envhome/ is mapped to: %s", GetHomePath().c_str());
 #endif
  if (!CUtil::GetFrameworksPath().empty())
    CLog::Log(LOGNOTICE, "special://frameworks/ is mapped to: %s", GetPath("frameworks").c_str());
} 

// private routines, to ensure we only set/get an appropriate path
void CSpecialProtocol::SetPath(const std::string &key, const std::string &path)
{
  m_pathMap[key] = path;
}

std::string CSpecialProtocol::GetPath(const std::string &key)
{
  std::map<std::string, std::string>::iterator it = m_pathMap.find(key);
  if (it != m_pathMap.end())
    return it->second;
  assert(false);
  return "";
}
