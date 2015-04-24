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
#include "URL.h"
#include "Util.h"
#include "guilib/GraphicContext.h"
#include "profiles/ProfilesManager.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "utils/log.h"
#include "utils/URIUtils.h"

#include <cassert>
#ifdef TARGET_POSIX
#include <dirent.h>
#include "utils/StringUtils.h"
#endif

using namespace std;

map<std::string, std::string> CSpecialProtocol::m_pathMap;

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

void CSpecialProtocol::SetMasterProfilePath(const std::string &dir)
{
  SetPath("masterprofile", dir);
}

void CSpecialProtocol::SetTempPath(const std::string &dir)
{
  SetPath("temp", dir);
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
    translatedPath = URIUtils::AddFileToFolder(CSettings::Get().GetString("subtitles.custompath"), FileName);
  else if (RootDir == "userdata")
    translatedPath = URIUtils::AddFileToFolder(CProfilesManager::Get().GetUserDataFolder(), FileName);
  else if (RootDir == "database")
    translatedPath = URIUtils::AddFileToFolder(CProfilesManager::Get().GetDatabaseFolder(), FileName);
  else if (RootDir == "thumbnails")
    translatedPath = URIUtils::AddFileToFolder(CProfilesManager::Get().GetThumbnailsFolder(), FileName);
  else if (RootDir == "recordings" || RootDir == "cdrips")
    translatedPath = URIUtils::AddFileToFolder(CSettings::Get().GetString("audiocds.recordingpath"), FileName);
  else if (RootDir == "screenshots")
    translatedPath = URIUtils::AddFileToFolder(CSettings::Get().GetString("debug.screenshotpath"), FileName);
  else if (RootDir == "musicplaylists")
    translatedPath = URIUtils::AddFileToFolder(CUtil::MusicPlaylistsLocation(), FileName);
  else if (RootDir == "videoplaylists")
    translatedPath = URIUtils::AddFileToFolder(CUtil::VideoPlaylistsLocation(), FileName);
  else if (RootDir == "skin")
    translatedPath = URIUtils::AddFileToFolder(g_graphicsContext.GetMediaDir(), FileName);
  else if (RootDir == "logpath")
    translatedPath = URIUtils::AddFileToFolder(g_advancedSettings.m_logFolder, FileName);


  // from here on, we have our "real" special paths
  else if (RootDir == "xbmc" ||
           RootDir == "xbmcbin" ||
           RootDir == "home" ||
           RootDir == "userhome" ||
           RootDir == "temp" ||
           RootDir == "profile" ||
           RootDir == "masterprofile" ||
           RootDir == "frameworks")
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
  CLog::Log(LOGNOTICE, "special://xbmc/ is mapped to: %s", GetPath("xbmc").c_str());
  CLog::Log(LOGNOTICE, "special://xbmcbin/ is mapped to: %s", GetPath("xbmcbin").c_str());
  CLog::Log(LOGNOTICE, "special://masterprofile/ is mapped to: %s", GetPath("masterprofile").c_str());
  CLog::Log(LOGNOTICE, "special://home/ is mapped to: %s", GetPath("home").c_str());
  CLog::Log(LOGNOTICE, "special://temp/ is mapped to: %s", GetPath("temp").c_str());
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
  map<std::string, std::string>::iterator it = m_pathMap.find(key);
  if (it != m_pathMap.end())
    return it->second;
  assert(false);
  return "";
}
