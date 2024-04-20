/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "guilib/guiinfo/LibraryGUIInfo.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "filesystem/Directory.h"
#include "guilib/guiinfo/GUIInfo.h"
#include "guilib/guiinfo/GUIInfoLabels.h"
#include "music/MusicDatabase.h"
#include "music/MusicLibraryQueue.h"
#include "profiles/ProfileManager.h"
#include "settings/SettingsComponent.h"
#include "utils/FileUtils.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "video/VideoDatabase.h"
#include "video/VideoLibraryQueue.h"

using namespace KODI::GUILIB::GUIINFO;

CLibraryGUIInfo::CLibraryGUIInfo()
{
  ResetLibraryBools();
}

bool CLibraryGUIInfo::GetLibraryBool(int condition) const
{
  bool value = false;
  GetBool(value, nullptr, 0, CGUIInfo(condition));
  return value;
}

void CLibraryGUIInfo::SetLibraryBool(int condition, bool value)
{
  switch (condition)
  {
    case LIBRARY_HAS_MUSIC:
      m_libraryHasMusic = value ? 1 : 0;
      break;
    case LIBRARY_HAS_MOVIES:
      m_libraryHasMovies = value ? 1 : 0;
      break;
    case LIBRARY_HAS_MOVIE_SETS:
      m_libraryHasMovieSets = value ? 1 : 0;
      break;
    case LIBRARY_HAS_TVSHOWS:
      m_libraryHasTVShows = value ? 1 : 0;
      break;
    case LIBRARY_HAS_MUSICVIDEOS:
      m_libraryHasMusicVideos = value ? 1 : 0;
      break;
    case LIBRARY_HAS_SINGLES:
      m_libraryHasSingles = value ? 1 : 0;
      break;
    case LIBRARY_HAS_COMPILATIONS:
      m_libraryHasCompilations = value ? 1 : 0;
      break;
    case LIBRARY_HAS_BOXSETS:
      m_libraryHasBoxsets = value ? 1 : 0;
      break;
    default:
      break;
  }
}

void CLibraryGUIInfo::ResetLibraryBools()
{
  m_libraryHasMusic = -1;
  m_libraryHasMovies = -1;
  m_libraryHasTVShows = -1;
  m_libraryHasMusicVideos = -1;
  m_libraryHasMovieSets = -1;
  m_libraryHasSingles = -1;
  m_libraryHasCompilations = -1;
  m_libraryHasBoxsets = -1;
  m_libraryRoleCounts.clear();
}

bool CLibraryGUIInfo::InitCurrentItem(CFileItem *item)
{
  return false;
}

bool CLibraryGUIInfo::GetLabel(std::string& value, const CFileItem *item, int contextWindow, const CGUIInfo &info, std::string *fallback) const
{
  return false;
}

bool CLibraryGUIInfo::GetInt(int& value, const CGUIListItem *gitem, int contextWindow, const CGUIInfo &info) const
{
  return false;
}

bool CLibraryGUIInfo::GetBool(bool& value, const CGUIListItem *gitem, int contextWindow, const CGUIInfo &info) const
{
  switch (info.m_info)
  {
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // LIBRARY_*
    ///////////////////////////////////////////////////////////////////////////////////////////////
    case LIBRARY_HAS_MUSIC:
    {
      if (m_libraryHasMusic < 0)
      { // query
        CMusicDatabase db;
        if (db.Open())
        {
          m_libraryHasMusic = (db.GetSongsCount() > 0) ? 1 : 0;
          db.Close();
        }
      }
      value = m_libraryHasMusic > 0;
      return true;
    }
    case LIBRARY_HAS_MOVIES:
    {
      if (m_libraryHasMovies < 0)
      {
        CVideoDatabase db;
        if (db.Open())
        {
          m_libraryHasMovies = db.HasContent(VideoDbContentType::MOVIES) ? 1 : 0;
          db.Close();
        }
      }
      value = m_libraryHasMovies > 0;
      return true;
    }
    case LIBRARY_HAS_MOVIE_SETS:
    {
      if (m_libraryHasMovieSets < 0)
      {
        CVideoDatabase db;
        if (db.Open())
        {
          m_libraryHasMovieSets = db.HasSets() ? 1 : 0;
          db.Close();
        }
      }
      value = m_libraryHasMovieSets > 0;
      return true;
    }
    case LIBRARY_HAS_TVSHOWS:
    {
      if (m_libraryHasTVShows < 0)
      {
        CVideoDatabase db;
        if (db.Open())
        {
          m_libraryHasTVShows = db.HasContent(VideoDbContentType::TVSHOWS) ? 1 : 0;
          db.Close();
        }
      }
      value = m_libraryHasTVShows > 0;
      return true;
    }
    case LIBRARY_HAS_MUSICVIDEOS:
    {
      if (m_libraryHasMusicVideos < 0)
      {
        CVideoDatabase db;
        if (db.Open())
        {
          m_libraryHasMusicVideos = db.HasContent(VideoDbContentType::MUSICVIDEOS) ? 1 : 0;
          db.Close();
        }
      }
      value = m_libraryHasMusicVideos > 0;
      return true;
    }
    case LIBRARY_HAS_SINGLES:
    {
      if (m_libraryHasSingles < 0)
      {
        CMusicDatabase db;
        if (db.Open())
        {
          m_libraryHasSingles = (db.GetSinglesCount() > 0) ? 1 : 0;
          db.Close();
        }
      }
      value = m_libraryHasSingles > 0;
      return true;
    }
    case LIBRARY_HAS_COMPILATIONS:
    {
      if (m_libraryHasCompilations < 0)
      {
        CMusicDatabase db;
        if (db.Open())
        {
          m_libraryHasCompilations = (db.GetCompilationAlbumsCount() > 0) ? 1 : 0;
          db.Close();
        }
      }
      value = m_libraryHasCompilations > 0;
      return true;
    }
    case LIBRARY_HAS_BOXSETS:
    {
      if (m_libraryHasBoxsets < 0)
      {
        CMusicDatabase db;
        if (db.Open())
        {
          m_libraryHasBoxsets = (db.GetBoxsetsCount() > 0) ? 1 : 0;
          db.Close();
        }
      }
      value = m_libraryHasBoxsets > 0;
      return true;
    }
    case LIBRARY_HAS_VIDEO:
    {
      return (GetBool(value, gitem, contextWindow, CGUIInfo(LIBRARY_HAS_MOVIES)) ||
              GetBool(value, gitem, contextWindow, CGUIInfo(LIBRARY_HAS_TVSHOWS)) ||
              GetBool(value, gitem, contextWindow, CGUIInfo(LIBRARY_HAS_MUSICVIDEOS)));
    }
    case LIBRARY_HAS_ROLE:
    {
      std::string strRole = info.GetData3();
      // Find value for role if already stored
      int artistcount = -1;
      for (const auto &role : m_libraryRoleCounts)
      {
        if (StringUtils::EqualsNoCase(strRole, role.first))
        {
          artistcount = role.second;
          break;
        }
      }
      // Otherwise get from DB and store
      if (artistcount < 0)
      {
        CMusicDatabase db;
        if (db.Open())
        {
          artistcount = db.GetArtistCountForRole(strRole);
          db.Close();
          m_libraryRoleCounts.emplace_back(strRole, artistcount);
        }
      }
      value = artistcount > 0;
      return true;
    }
    case LIBRARY_HAS_NODE:
    {
      const CURL url(info.GetData3());
      const std::shared_ptr<CProfileManager> profileManager =
            CServiceBroker::GetSettingsComponent()->GetProfileManager();
      CFileItemList items;

      std::string libDir = profileManager->GetLibraryFolder();
      XFILE::CDirectory::GetDirectory(libDir, items, "", XFILE::DIR_FLAG_NO_FILE_DIRS);
      if (items.Size() == 0)
        libDir = "special://xbmc/system/library/";

      std::string nodePath = URIUtils::AddFileToFolder(libDir, url.GetHostName() + "/");
      nodePath = URIUtils::AddFileToFolder(nodePath, url.GetFileName());
      value = CFileUtils::Exists(nodePath);
      return true;
    }
    case LIBRARY_IS_SCANNING:
    {
      value = (CMusicLibraryQueue::GetInstance().IsScanningLibrary() ||
               CVideoLibraryQueue::GetInstance().IsScanningLibrary());
      return true;
    }
    case LIBRARY_IS_SCANNING_VIDEO:
    {
      value = CVideoLibraryQueue::GetInstance().IsScanningLibrary();
      return true;
    }
    case LIBRARY_IS_SCANNING_MUSIC:
    {
      value = CMusicLibraryQueue::GetInstance().IsScanningLibrary();
      return true;
    }
  }

  return false;
}
