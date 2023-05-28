/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SpecialProtocol.h"

#include "ServiceBroker.h"
#include "URL.h"
#include "Util.h"
#include "profiles/ProfileManager.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#include "windowing/GraphicContext.h"

#include "PlatformDefs.h"
#ifdef TARGET_POSIX
#include <dirent.h>
#include "utils/StringUtils.h"
#endif

const CProfileManager *CSpecialProtocol::m_profileManager = nullptr;

void CSpecialProtocol::RegisterProfileManager(const CProfileManager &profileManager)
{
  m_profileManager = &profileManager;
}

void CSpecialProtocol::UnregisterProfileManager()
{
  m_profileManager = nullptr;
}

void CSpecialProtocol::SetProfilePath(const std::string &dir)
{
  SetPath("profile", dir);
  CLog::Log(LOGINFO, "special://profile/ is mapped to: {}", GetPath("profile"));
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
      CLog::Log(LOGWARNING, "Trying to access old style dir: {}", path);
      // printf("Trying to access old style dir: %s\n", path.c_str());
    }
#endif

    return url.Get();
  }

  const std::string& FullFileName = url.GetFileName();

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
    translatedPath = URIUtils::AddFileToFolder(CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(CSettings::SETTING_SUBTITLES_CUSTOMPATH), FileName);
  else if (RootDir == "userdata" && m_profileManager)
    translatedPath = URIUtils::AddFileToFolder(m_profileManager->GetUserDataFolder(), FileName);
  else if (RootDir == "database" && m_profileManager)
    translatedPath = URIUtils::AddFileToFolder(m_profileManager->GetDatabaseFolder(), FileName);
  else if (RootDir == "thumbnails" && m_profileManager)
    translatedPath = URIUtils::AddFileToFolder(m_profileManager->GetThumbnailsFolder(), FileName);
  else if (RootDir == "recordings" || RootDir == "cdrips")
    translatedPath = URIUtils::AddFileToFolder(CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(CSettings::SETTING_AUDIOCDS_RECORDINGPATH), FileName);
  else if (RootDir == "screenshots")
    translatedPath = URIUtils::AddFileToFolder(CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(CSettings::SETTING_DEBUG_SCREENSHOTPATH), FileName);
  else if (RootDir == "musicartistsinfo")
    translatedPath = URIUtils::AddFileToFolder(CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(CSettings::SETTING_MUSICLIBRARY_ARTISTSFOLDER), FileName);
  else if (RootDir == "musicplaylists")
    translatedPath = URIUtils::AddFileToFolder(CUtil::MusicPlaylistsLocation(), FileName);
  else if (RootDir == "videoplaylists")
    translatedPath = URIUtils::AddFileToFolder(CUtil::VideoPlaylistsLocation(), FileName);
  else if (RootDir == "skin")
  {
    auto winSystem = CServiceBroker::GetWinSystem();
    // windowing may not have been initialized yet
    if (winSystem)
      translatedPath = URIUtils::AddFileToFolder(winSystem->GetGfxContext().GetMediaDir(), FileName);
  }
  // from here on, we have our "real" special paths
  else if (RootDir == "xbmc" ||
           RootDir == "xbmcbin" ||
           RootDir == "xbmcbinaddons" ||
           RootDir == "xbmcaltbinaddons" ||
           RootDir == "home" ||
           RootDir == "envhome" ||
           RootDir == "userhome" ||
           RootDir == "temp" ||
           RootDir == "profile" ||
           RootDir == "masterprofile" ||
           RootDir == "frameworks" ||
           RootDir == "logpath")
  {
    std::string basePath = GetPath(RootDir);
    if (!basePath.empty())
      translatedPath = URIUtils::AddFileToFolder(basePath, FileName);
    else
      translatedPath.clear();
  }

  // check if we need to recurse in
  if (URIUtils::IsSpecial(translatedPath))
  { // we need to recurse in, as there may be multiple translations required
    return TranslatePath(translatedPath);
  }

  // Validate the final path, just in case
  return CUtil::ValidatePath(std::move(translatedPath));
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
          if (StringUtils::CompareNoCase(de->d_name, tokens[i]) == 0)
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
  CLog::Log(LOGINFO, "special://xbmc/ is mapped to: {}", GetPath("xbmc"));
  CLog::Log(LOGINFO, "special://xbmcbin/ is mapped to: {}", GetPath("xbmcbin"));
  CLog::Log(LOGINFO, "special://xbmcbinaddons/ is mapped to: {}", GetPath("xbmcbinaddons"));
  CLog::Log(LOGINFO, "special://masterprofile/ is mapped to: {}", GetPath("masterprofile"));
#if defined(TARGET_POSIX)
  CLog::Log(LOGINFO, "special://envhome/ is mapped to: {}", GetPath("envhome"));
#endif
  CLog::Log(LOGINFO, "special://home/ is mapped to: {}", GetPath("home"));
  CLog::Log(LOGINFO, "special://temp/ is mapped to: {}", GetPath("temp"));
  CLog::Log(LOGINFO, "special://logpath/ is mapped to: {}", GetPath("logpath"));
  //CLog::Log(LOGINFO, "special://userhome/ is mapped to: {}", GetPath("userhome"));
  if (!CUtil::GetFrameworksPath().empty())
    CLog::Log(LOGINFO, "special://frameworks/ is mapped to: {}", GetPath("frameworks"));
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

  return "";
}
