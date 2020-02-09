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
#include "windowing/GraphicContext.h"
#include "profiles/ProfileManager.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/log.h"
#include "utils/URIUtils.h"

#include <cassert>
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
      CLog::Log(LOGWARNING, "Trying to access old style dir: %s", path.c_str());
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
  CLog::Log(LOGNOTICE, "special://xbmc/ is mapped to: %s", GetPath("xbmc").c_str());
  CLog::Log(LOGNOTICE, "special://xbmcbin/ is mapped to: %s", GetPath("xbmcbin").c_str());
  CLog::Log(LOGNOTICE, "special://xbmcbinaddons/ is mapped to: %s", GetPath("xbmcbinaddons").c_str());
  CLog::Log(LOGNOTICE, "special://masterprofile/ is mapped to: %s", GetPath("masterprofile").c_str());
#if defined(TARGET_POSIX)
  CLog::Log(LOGNOTICE, "special://envhome/ is mapped to: %s", GetPath("envhome").c_str());
#endif
  CLog::Log(LOGNOTICE, "special://home/ is mapped to: %s", GetPath("home").c_str());
  CLog::Log(LOGNOTICE, "special://temp/ is mapped to: %s", GetPath("temp").c_str());
  CLog::Log(LOGNOTICE, "special://logpath/ is mapped to: %s", GetPath("logpath").c_str());
  //CLog::Log(LOGNOTICE, "special://userhome/ is mapped to: %s", GetPath("userhome").c_str());
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
