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
#include "utils/StringUtils.h"

#ifdef TARGET_POSIX
#include <dirent.h>
#endif

using namespace std;

map<string, string> CSpecialProtocol::m_pathMap;

void CSpecialProtocol::SetProfilePath(const string &dir)
{
  SetPath("profile", dir);
  CLog::Log(LOGNOTICE, "special://profile/ is mapped to: %s", GetPath("profile").c_str());
}

void CSpecialProtocol::SetXBMCPath(const string &dir)
{
  SetPath("xbmc", dir);
}

void CSpecialProtocol::SetXBMCBinPath(const string &dir)
{
  SetPath("xbmcbin", dir);
}

void CSpecialProtocol::SetXBMCFrameworksPath(const string &dir)
{
  SetPath("frameworks", dir);
}

void CSpecialProtocol::SetHomePath(const string &dir)
{
  SetPath("home", dir);
}

void CSpecialProtocol::SetUserHomePath(const string &dir)
{
  SetPath("userhome", dir);
}

void CSpecialProtocol::SetMasterProfilePath(const string &dir)
{
  SetPath("masterprofile", dir);
}

void CSpecialProtocol::SetTempPath(const string &dir)
{
  SetPath("temp", dir);
}

bool CSpecialProtocol::ComparePath(const string &path1, const string &path2)
{
  return TranslatePath(path1) == TranslatePath(path2);
}

string CSpecialProtocol::TranslatePath(const string &path)
{
  CURL url(path);
  // check for special-protocol, if not, return
  if (!StringUtils::EqualsNoCase(url.GetProtocol(), "special"))
  {
    return path;
  }
  return TranslatePath(url);
}

string CSpecialProtocol::TranslatePath(const CURL &url)
{
  // check for special-protocol, if not, return
  if (!StringUtils::EqualsNoCase(url.GetProtocol(), "special"))
  {
#if defined(TARGET_POSIX) && defined(_DEBUG)
    string path(url.Get());
    if (path.length() >= 2 && path[1] == ':')
    {
      CLog::Log(LOGWARNING, "Trying to access old style dir: %s\n", path.c_str());
     // printf("Trying to access old style dir: %s\n", path.c_str());
    }
#endif

    return url.Get();
  }

  string FullFileName = url.GetFileName();

  string translatedPath;
  string FileName;
  string RootDir;

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

  if (StringUtils::EqualsNoCase(RootDir, "subtitles"))
    translatedPath = URIUtils::AddFileToFolder(CSettings::Get().GetString("subtitles.custompath"), FileName);
  else if (StringUtils::EqualsNoCase(RootDir, "userdata"))
    translatedPath = URIUtils::AddFileToFolder(CProfilesManager::Get().GetUserDataFolder(), FileName);
  else if (StringUtils::EqualsNoCase(RootDir, "database"))
    translatedPath = URIUtils::AddFileToFolder(CProfilesManager::Get().GetDatabaseFolder(), FileName);
  else if (StringUtils::EqualsNoCase(RootDir, "thumbnails"))
    translatedPath = URIUtils::AddFileToFolder(CProfilesManager::Get().GetThumbnailsFolder(), FileName);
  else if (StringUtils::EqualsNoCase(RootDir, "recordings") || StringUtils::EqualsNoCase(RootDir, "cdrips"))
    translatedPath = URIUtils::AddFileToFolder(CSettings::Get().GetString("audiocds.recordingpath"), FileName);
  else if (StringUtils::EqualsNoCase(RootDir, "screenshots"))
    translatedPath = URIUtils::AddFileToFolder(CSettings::Get().GetString("debug.screenshotpath"), FileName);
  else if (StringUtils::EqualsNoCase(RootDir, "musicplaylists"))
    translatedPath = URIUtils::AddFileToFolder(CUtil::MusicPlaylistsLocation(), FileName);
  else if (StringUtils::EqualsNoCase(RootDir, "videoplaylists"))
    translatedPath = URIUtils::AddFileToFolder(CUtil::VideoPlaylistsLocation(), FileName);
  else if (StringUtils::EqualsNoCase(RootDir, "skin"))
    translatedPath = URIUtils::AddFileToFolder(g_graphicsContext.GetMediaDir(), FileName);
  else if (StringUtils::EqualsNoCase(RootDir, "logpath"))
    translatedPath = URIUtils::AddFileToFolder(g_advancedSettings.m_logFolder, FileName);


  // from here on, we have our "real" special paths
  else if (StringUtils::EqualsNoCase(RootDir, "xbmc") ||
           StringUtils::EqualsNoCase(RootDir, "xbmcbin") ||
           StringUtils::EqualsNoCase(RootDir, "home") ||
           StringUtils::EqualsNoCase(RootDir, "userhome") ||
           StringUtils::EqualsNoCase(RootDir, "temp") ||
           StringUtils::EqualsNoCase(RootDir, "profile") ||
           StringUtils::EqualsNoCase(RootDir, "masterprofile") ||
           StringUtils::EqualsNoCase(RootDir, "frameworks"))
  {
    string basePath = GetPath(RootDir);
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

string CSpecialProtocol::TranslatePathConvertCase(const string& path)
{
  string translatedPath = TranslatePath(path);

#ifdef TARGET_POSIX
  if (translatedPath.find("://") != std::string::npos)
    return translatedPath;

  // If the file exists with the requested name, simply return it
  struct stat stat_buf;
  if (stat(translatedPath.c_str(), &stat_buf) == 0)
    return translatedPath;

  string result;
  std::vector<std::string> tokens;
  StringUtils::Tokenize(translatedPath, tokens, "/");
  string file;
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
void CSpecialProtocol::SetPath(const string &key, const string &path)
{
  m_pathMap[key] = path;
}

string CSpecialProtocol::GetPath(const string &key)
{
  map<string, string>::iterator it = m_pathMap.find(key);
  if (it != m_pathMap.end())
    return it->second;
  assert(false);
  return "";
}
