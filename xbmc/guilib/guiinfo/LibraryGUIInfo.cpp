/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      http://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "guilib/guiinfo/LibraryGUIInfo.h"

#include "Application.h"
#include "music/MusicDatabase.h"
#include "utils/StringUtils.h"
#include "video/VideoDatabase.h"

#include "guilib/guiinfo/GUIInfo.h"
#include "guilib/guiinfo/GUIInfoLabels.h"

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
          m_libraryHasMovies = db.HasContent(VIDEODB_CONTENT_MOVIES) ? 1 : 0;
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
          m_libraryHasTVShows = db.HasContent(VIDEODB_CONTENT_TVSHOWS) ? 1 : 0;
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
          m_libraryHasMusicVideos = db.HasContent(VIDEODB_CONTENT_MUSICVIDEOS) ? 1 : 0;
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
          m_libraryRoleCounts.emplace_back(std::make_pair(strRole, artistcount));
        }
      }
      value = artistcount > 0;
      return true;
    }
    case LIBRARY_IS_SCANNING:
    {
      value = (g_application.IsMusicScanning() || g_application.IsVideoScanning());
      return true;
    }
    case LIBRARY_IS_SCANNING_VIDEO:
    {
      value = g_application.IsVideoScanning();
      return true;
    }
    case LIBRARY_IS_SCANNING_MUSIC:
    {
      value = g_application.IsMusicScanning();
      return true;
    }
  }

  return false;
}
