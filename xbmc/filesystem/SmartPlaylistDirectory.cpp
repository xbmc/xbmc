/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <math.h>

#include "SmartPlaylistDirectory.h"
#include "utils/log.h"
#include "playlists/SmartPlayList.h"
#include "music/MusicDatabase.h"
#include "video/VideoDatabase.h"
#include "Directory.h"
#include "File.h"
#include "FileItem.h"
#include "settings/GUISettings.h"
#include "utils/URIUtils.h"

#define PROPERTY_PATH_DB            "path.db"
#define PROPERTY_SORT_ORDER         "sort.order"
#define PROPERTY_SORT_ASCENDING     "sort.ascending"

namespace XFILE
{
  CSmartPlaylistDirectory::CSmartPlaylistDirectory()
  {
  }

  CSmartPlaylistDirectory::~CSmartPlaylistDirectory()
  {
  }

  bool CSmartPlaylistDirectory::GetDirectory(const CStdString& strPath, CFileItemList& items)
  {
    // Load in the SmartPlaylist and get the WHERE query
    CSmartPlaylist playlist;
    if (!playlist.Load(strPath))
      return false;
    bool result = GetDirectory(playlist, items);
    if (result)
      items.SetProperty("library.smartplaylist", true);
    
    return result;
  }
  
  bool CSmartPlaylistDirectory::GetDirectory(const CSmartPlaylist &playlist, CFileItemList& items, const CStdString &strBaseDir /* = "" */, bool filter /* = false */)
  {
    bool success = false, success2 = false;
    std::set<CStdString> playlists;

    SortDescription sorting;
    sorting.limitEnd = playlist.GetLimit();
    sorting.sortBy = playlist.GetOrder();
    sorting.sortOrder = playlist.GetOrderAscending() ? SortOrderAscending : SortOrderDescending;
    if (g_guiSettings.GetBool("filelists.ignorethewhensorting"))
      sorting.sortAttributes = SortAttributeIgnoreArticle;

    std::string option = !filter ? "xsp" : "filter";

    if (playlist.GetType().Equals("movies") ||
        playlist.GetType().Equals("tvshows") ||
        playlist.GetType().Equals("episodes"))
    {
      CVideoDatabase db;
      if (db.Open())
      {
        MediaType mediaType = DatabaseUtils::MediaTypeFromString(playlist.GetType());

        CStdString baseDir = strBaseDir;
        if (strBaseDir.empty())
        {
          switch (mediaType)
          {
          case MediaTypeTvShow:
          case MediaTypeEpisode:
            baseDir = "videodb://2/2/";
            break;

          case MediaTypeMovie:
            baseDir = "videodb://1/2/";
            break;

          default:
            return false;
          }
        }

        CVideoDbUrl videoUrl;
        if (!videoUrl.FromString(baseDir))
          return false;

        // store the smartplaylist as JSON in the URL as well
        CStdString xsp;
        if (!playlist.IsEmpty(filter))
        {
          if (!playlist.SaveAsJson(xsp, !filter))
            return false;
        }
        videoUrl.AddOption(option, xsp);
        
        CDatabase::Filter dbfilter;
        success = db.GetSortedVideos(mediaType, videoUrl.ToString(), sorting, items, dbfilter, true);
        db.Close();

        // if we retrieve a list of episodes and we didn't receive
        // a pre-defined base path, we need to fix it
        if (strBaseDir.empty() && mediaType == MediaTypeEpisode)
          videoUrl.AppendPath("-1/-1/");
        items.SetProperty(PROPERTY_PATH_DB, videoUrl.ToString());
      }
    }
    else if (playlist.GetType().Equals("albums"))
    {
      CMusicDatabase db;
      if (db.Open())
      {
        CMusicDbUrl musicUrl;
        if (!musicUrl.FromString(!strBaseDir.empty() ? strBaseDir : "musicdb://3/"))
          return false;

        // store the smartplaylist as JSON in the URL as well
        CStdString xsp;
        if (!playlist.IsEmpty(filter))
        {
          if (!playlist.SaveAsJson(xsp, !filter))
            return false;
        }
        musicUrl.AddOption(option, xsp);

        CDatabase::Filter dbfilter;
        success = db.GetAlbumsByWhere(musicUrl.ToString(), dbfilter, items, sorting);
        db.Close();
        items.SetContent("albums");
        items.SetProperty(PROPERTY_PATH_DB, musicUrl.ToString());
      }
    }
    else if (playlist.GetType().Equals("artists"))
    {
      CMusicDatabase db;
      if (db.Open())
      {
        CMusicDbUrl musicUrl;
        if (!musicUrl.FromString("musicdb://2/"))
          return false;

        // store the smartplaylist as JSON in the URL as well
        CStdString xsp;
        if (!playlist.IsEmpty(filter))
        {
          if (!playlist.SaveAsJson(xsp, !filter))
            return false;
        }
        musicUrl.AddOption(option, xsp);

        CDatabase::Filter dbfilter;
        success = db.GetArtistsNav(musicUrl.ToString(), items, !g_guiSettings.GetBool("musiclibrary.showcompilationartists"), -1, -1, -1, dbfilter, sorting);
        db.Close();
        items.SetContent("artists");
        items.SetProperty(PROPERTY_PATH_DB, musicUrl.ToString());
      }
    }

    if (playlist.GetType().Equals("songs") || playlist.GetType().Equals("mixed") || playlist.GetType().IsEmpty())
    {
      CMusicDatabase db;
      if (db.Open())
      {
        CSmartPlaylist songPlaylist(playlist);
        if (playlist.GetType().IsEmpty() || playlist.GetType().Equals("mixed"))
          songPlaylist.SetType("songs");
        
        CMusicDbUrl musicUrl;
        if (!musicUrl.FromString(!strBaseDir.empty() ? strBaseDir : "musicdb://4/"))
          return false;

        // store the smartplaylist as JSON in the URL as well
        CStdString xsp;
        if (!songPlaylist.IsEmpty(filter))
        {
          if (!songPlaylist.SaveAsJson(xsp, !filter))
            return false;
        }
        musicUrl.AddOption(option, xsp);

        CDatabase::Filter dbfilter;
        success = db.GetSongsByWhere(musicUrl.ToString(), dbfilter, items, sorting);
        db.Close();
        items.SetContent("songs");
        items.SetProperty(PROPERTY_PATH_DB, musicUrl.ToString());
      }
    }
    if (playlist.GetType().Equals("musicvideos") || playlist.GetType().Equals("mixed"))
    {
      CVideoDatabase db;
      if (db.Open())
      {
        CSmartPlaylist mvidPlaylist(playlist);
        if (playlist.GetType().Equals("mixed"))
          mvidPlaylist.SetType("musicvideos");

        CVideoDbUrl videoUrl;
        if (!videoUrl.FromString(!strBaseDir.empty() ? strBaseDir : "videodb://3/2/"))
          return false;

        // store the smartplaylist as JSON in the URL as well
        CStdString xsp;
        if (!mvidPlaylist.IsEmpty(filter))
        {
          if (!mvidPlaylist.SaveAsJson(xsp, !filter))
            return false;
        }
        videoUrl.AddOption(option, xsp);
        
        CFileItemList items2;
        success2 = db.GetSortedVideos(MediaTypeMusicVideo, videoUrl.ToString(), sorting, items2);
        db.Close();

        items.Append(items2);
        if (items2.Size())
        {
          if (items.Size() > items2.Size())
            items.SetContent("mixed");
          else
            items.SetContent("musicvideos");
        }
        items.SetProperty(PROPERTY_PATH_DB, videoUrl.ToString());
      }
    }
    items.SetLabel(playlist.GetName());
    items.SetProperty(PROPERTY_SORT_ORDER, (int)playlist.GetOrder());
    items.SetProperty(PROPERTY_SORT_ASCENDING, playlist.GetOrderDirection() == SortOrderAscending);

    // go through and set the playlist order
    for (int i = 0; i < items.Size(); i++)
    {
      CFileItemPtr item = items[i];
      item->m_iprogramCount = i;  // hack for playlist order
    }

    if (playlist.GetType().Equals("mixed"))
      return success || success2;
    else if (playlist.GetType().Equals("musicvideos"))
      return success2;
    else
      return success;
  }

  bool CSmartPlaylistDirectory::ContainsFiles(const CStdString& strPath)
  {
    // smart playlists always have files??
    return true;
  }

  CStdString CSmartPlaylistDirectory::GetPlaylistByName(const CStdString& name, const CStdString& playlistType)
  {
    CFileItemList list;
    bool filesExist = false;
    if (playlistType == "songs" || playlistType == "albums")
      filesExist = CDirectory::GetDirectory("special://musicplaylists/", list, ".xsp", false);
    else // all others are video
      filesExist = CDirectory::GetDirectory("special://videoplaylists/", list, ".xsp", false);
    if (filesExist)
    {
      for (int i = 0; i < list.Size(); i++)
      {
        CFileItemPtr item = list[i];
        CSmartPlaylist playlist;
        if (playlist.OpenAndReadName(item->GetPath()))
        {
          if (playlist.GetName().CompareNoCase(name) == 0)
            return item->GetPath();
        }
      }
      for (int i = 0; i < list.Size(); i++)
      { // check based on filename
        CFileItemPtr item = list[i];
        if (URIUtils::GetFileName(item->GetPath()) == name)
        { // found :)
          return item->GetPath();
        }
      }
    }
    return "";
  }

  bool CSmartPlaylistDirectory::Remove(const char *strPath)
  {
    return XFILE::CFile::Delete(strPath);
  }
}



