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

#include "stdafx.h"
#include "SpecialProtocol.h"
#include "URL.h"
#include "Util.h"
#include "GUISettings.h"
#include "Settings.h"

#ifdef _LINUX
#include <dirent.h>
#endif

using namespace std;

map<CStdString, CStdString> CSpecialProtocol::m_pathMap;

void CSpecialProtocol::SetProfilePath(const CStdString &dir)
{
  SetPath("profile", dir);
  CLog::Log(LOGNOTICE, "special://profile/ is mapped to: %s", GetPath("profile").c_str());
}

void CSpecialProtocol::SetXBMCPath(const CStdString &dir)
{
  SetPath("xbmc", dir);
}

void CSpecialProtocol::SetHomePath(const CStdString &dir)
{
  SetPath("home", dir);
}

void CSpecialProtocol::SetMasterProfilePath(const CStdString &dir)
{
  SetPath("masterprofile", dir);
}

void CSpecialProtocol::SetTempPath(const CStdString &dir)
{
  SetPath("temp", dir);
}

bool CSpecialProtocol::XBMCIsHome()
{
  return TranslatePath("special://xbmc") == TranslatePath("special://home");
}

bool CSpecialProtocol::ComparePath(const CStdString &path1, const CStdString &path2)
{
  return TranslatePath(path1) == TranslatePath(path2);
}

CStdString CSpecialProtocol::TranslatePath(const CStdString &path)
{
  // check for a special:// protocol
  CStdString validPath = CURL::ValidatePath(path);
  if (validPath.Left(10) != "special://")
    return path;  // nothing to translate

  // have a translateable path
  CStdString translatedPath;

  // add a slash at end to ensure we have a full path that we can compare (note all the comparisons end with a slash)
  CStdString specialPath(validPath);
  CUtil::AddSlashAtEnd(specialPath);
  
  if (specialPath.Left(20).Equals("special://subtitles/"))
    CUtil::AddFileToFolder(g_guiSettings.GetString("subtitles.custompath"), validPath.Mid(20), translatedPath);
  else if (specialPath.Left(19).Equals("special://userdata/"))
    CUtil::AddFileToFolder(g_settings.GetUserDataFolder(), validPath.Mid(19), translatedPath);
  else if (specialPath.Left(19).Equals("special://database/"))
    CUtil::AddFileToFolder(g_settings.GetDatabaseFolder(), validPath.Mid(19), translatedPath);
  else if (specialPath.Left(21).Equals("special://thumbnails/"))
    CUtil::AddFileToFolder(g_settings.GetThumbnailsFolder(), validPath.Mid(21), translatedPath);
  else if (specialPath.Left(21).Equals("special://recordings/"))
    CUtil::AddFileToFolder(g_guiSettings.GetString("mymusic.recordingpath",false), validPath.Mid(21), translatedPath);
  else if (specialPath.Left(22).Equals("special://screenshots/"))
    CUtil::AddFileToFolder(g_guiSettings.GetString("pictures.screenshotpath",false), validPath.Mid(22), translatedPath);
  else if (specialPath.Left(25).Equals("special://musicplaylists/"))
    CUtil::AddFileToFolder(CUtil::MusicPlaylistsLocation(), validPath.Mid(25), translatedPath);
  else if (specialPath.Left(25).Equals("special://videoplaylists/"))
    CUtil::AddFileToFolder(CUtil::VideoPlaylistsLocation(), validPath.Mid(25), translatedPath);
  else if (specialPath.Left(17).Equals("special://cdrips/"))
    CUtil::AddFileToFolder(g_guiSettings.GetString("cddaripper.path"), validPath.Mid(17), translatedPath);

  // from here on, we have our "real" special paths
  else if (specialPath.Left(15).Equals("special://xbmc/"))
    CUtil::AddFileToFolder(GetPath("xbmc"), validPath.Mid(15), translatedPath);
  else if (specialPath.Left(15).Equals("special://home/"))
    CUtil::AddFileToFolder(GetPath("home"), validPath.Mid(15), translatedPath);
  else if (specialPath.Left(15).Equals("special://temp/"))
    CUtil::AddFileToFolder(GetPath("temp"), validPath.Mid(15), translatedPath);
  else if (specialPath.Left(18).Equals("special://profile/"))
    CUtil::AddFileToFolder(GetPath("profile"), validPath.Mid(18), translatedPath);
  else if (specialPath.Left(24).Equals("special://masterprofile/"))
    CUtil::AddFileToFolder(GetPath("masterprofile"), validPath.Mid(24), translatedPath);
  else 
    translatedPath = ""; // invalid path to translate

  // check if we need to recurse in
  if (translatedPath.Left(10).Equals("special://"))
  { // we need to recurse in, as there may be multiple translations required
    return TranslatePath(translatedPath);
  }

  // fix up the slash direction on win32
#ifdef _WIN32PC
  if(CUtil::IsDOSPath(translatedPath))
    translatedPath.Replace("/","\\");
#endif

  return translatedPath;
}

CStdString CSpecialProtocol::TranslatePathConvertCase(const CStdString& path)
{
  CStdString translatedPath = TranslatePath(path);

#ifdef _LINUX
  if (translatedPath.Find("://") > 0)
    return translatedPath;

  // If the file exists with the requested name, simply return it
  struct stat stat_buf;
  if (stat(translatedPath.c_str(), &stat_buf) == 0)
    return translatedPath;

  CStdString result;
  vector<CStdString> tokens;
  CUtil::Tokenize(translatedPath, tokens, "/");
  CStdString file;
  DIR* dir;
  struct dirent* de;

  for (unsigned int i = 0; i < tokens.size(); i++)
  {
    file = result + "/" + tokens[i];
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
          if (strcasecmp(de->d_name, tokens[i]) == 0)
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

CStdString CSpecialProtocol::ReplaceOldPath(const CStdString &oldPath, int pathVersion)
{
  if (pathVersion < 1)
  {
    if (oldPath.Left(2).CompareNoCase("P:") == 0)
      return CUtil::AddFileToFolder("special://profile/", oldPath.Mid(2));
    else if (oldPath.Left(2).CompareNoCase("Q:") == 0)
      return CUtil::AddFileToFolder("special://xbmc/", oldPath.Mid(2));
    else if (oldPath.Left(2).CompareNoCase("T:") == 0)
      return CUtil::AddFileToFolder("special://masterprofile/", oldPath.Mid(2));
    else if (oldPath.Left(2).CompareNoCase("U:") == 0)
      return CUtil::AddFileToFolder("special://home/", oldPath.Mid(2));
    else if (oldPath.Left(2).CompareNoCase("Z:") == 0)
      return CUtil::AddFileToFolder("special://temp/", oldPath.Mid(2));
  }
  return oldPath;
}

void CSpecialProtocol::LogPaths()
{
  CLog::Log(LOGNOTICE, "special://xbmc/ is mapped to: %s", GetPath("xbmc").c_str());
  CLog::Log(LOGNOTICE, "special://masterprofile/ is mapped to: %s", GetPath("masterprofile").c_str());
  CLog::Log(LOGNOTICE, "special://home/ is mapped to: %s", GetPath("home").c_str());
  CLog::Log(LOGNOTICE, "special://temp/ is mapped to: %s", GetPath("temp").c_str());
}

// private routines, to ensure we only set/get an appropriate path
void CSpecialProtocol::SetPath(const CStdString &key, const CStdString &path)
{
  m_pathMap[key] = path;
}

CStdString CSpecialProtocol::GetPath(const CStdString &key)
{
  map<CStdString, CStdString>::iterator it = m_pathMap.find(key);
  if (it != m_pathMap.end())
    return it->second;
  assert(false);
  return "";
}
