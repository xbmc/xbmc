/*
 *      Copyright (C) 2005-2013 Team XBMC
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
#include "FileItem.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "music/MusicDatabase.h"
#include "playlists/SmartPlayList.h"
#include "settings/GUISettings.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include "video/VideoDatabase.h"

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
    const CStdString& group = playlist.GetGroup();

    if (playlist.GetType().Equals("movies") ||
        playlist.GetType().Equals("tvshows") ||
        playlist.GetType().Equals("episodes"))
    {
      CVideoDatabase db;
      if (db.Open())
      {
        MediaType mediaType = DatabaseUtils::MediaTypeFromString(playlist.GetType());

        CStdString baseDir = strBaseDir;
        VIDEODB_CONTENT_TYPE type;
        if (strBaseDir.empty())
        {
          switch (mediaType)
          {
            case MediaTypeTvShow:
              baseDir = "videodb://tvshows/";
              type = VIDEODB_CONTENT_TVSHOWS;
              break;

            case MediaTypeEpisode:
              baseDir = "videodb://tvshows/";
              type = VIDEODB_CONTENT_EPISODES;
              break;

            case MediaTypeMovie:
              baseDir = "videodb://movies/";
              type = VIDEODB_CONTENT_MOVIES;
              break;

            default:
              return false;
          }

          if (group.empty())
            baseDir += "titles";
          else
            baseDir += group;
          URIUtils::AddSlashAtEnd(baseDir);
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

        if (!xsp.empty())
          videoUrl.AddOption(option, xsp);
        else
          videoUrl.RemoveOption(option);
        
        CDatabase::Filter dbfilter;
        if (group.empty())
          success = db.GetSortedVideos(mediaType, videoUrl.ToString(), sorting, items, dbfilter);
        else
        {
          dbfilter.where = playlist.GetWhereClause(db, playlists);

          if (group.Equals("genres"))
            success = db.GetGenresNav(videoUrl.ToString(), items, type, dbfilter);
          else if (group.Equals("years"))
            success = db.GetYearsNav(videoUrl.ToString(), items, type, dbfilter);
          else if (group.Equals("actors"))
            success = db.GetActorsNav(videoUrl.ToString(), items, type, dbfilter);
          else if (group.Equals("directors"))
            success = db.GetDirectorsNav(videoUrl.ToString(), items, type, dbfilter);
          else if (group.Equals("studios"))
            success = db.GetStudiosNav(videoUrl.ToString(), items, type, dbfilter);
          else if (group.Equals("sets"))
            success = db.GetSetsNav(videoUrl.ToString(), items, type, dbfilter);
          else if (group.Equals("countries"))
            success = db.GetCountriesNav(videoUrl.ToString(), items, type, dbfilter);
          else if (group.Equals("tags"))
            success = db.GetTagsNav(videoUrl.ToString(), items, type, dbfilter);
        }
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
        if (!musicUrl.FromString(!strBaseDir.empty() ? strBaseDir : "musicdb://albums/"))
          return false;

        // store the smartplaylist as JSON in the URL as well
        CStdString xsp;
        if (!playlist.IsEmpty(filter))
        {
          if (!playlist.SaveAsJson(xsp, !filter))
            return false;
        }

        if (!xsp.empty())
          musicUrl.AddOption(option, xsp);
        else
          musicUrl.RemoveOption(option);

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
        if (!musicUrl.FromString("musicdb://artists/"))
          return false;

        // store the smartplaylist as JSON in the URL as well
        CStdString xsp;
        if (!playlist.IsEmpty(filter))
        {
          if (!playlist.SaveAsJson(xsp, !filter))
            return false;
        }

        if (!xsp.empty())
          musicUrl.AddOption(option, xsp);
        else
          musicUrl.RemoveOption(option);

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
        if (!musicUrl.FromString(!strBaseDir.empty() ? strBaseDir : "musicdb://songs/"))
          return false;

        // store the smartplaylist as JSON in the URL as well
        CStdString xsp;
        if (!songPlaylist.IsEmpty(filter))
        {
          if (!songPlaylist.SaveAsJson(xsp, !filter))
            return false;
        }

        if (!xsp.empty())
          musicUrl.AddOption(option, xsp);
        else
          musicUrl.RemoveOption(option);

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

        CStdString baseDir = strBaseDir;
        if (baseDir.empty())
        {
          baseDir = "videodb://musicvideos/";

          if (group.empty())
            baseDir += "titles";
          else
            baseDir += group;
          URIUtils::AddSlashAtEnd(baseDir);
        }

        CVideoDbUrl videoUrl;
        if (!videoUrl.FromString(baseDir))
          return false;

        // store the smartplaylist as JSON in the URL as well
        CStdString xsp;
        if (!mvidPlaylist.IsEmpty(filter))
        {
          if (!mvidPlaylist.SaveAsJson(xsp, !filter))
            return false;
        }

        if (!xsp.empty())
          videoUrl.AddOption(option, xsp);
        else
          videoUrl.RemoveOption(option);
        
        CFileItemList items2;
        CDatabase::Filter dbfilter;
        if (group.empty())
          success2 = db.GetSortedVideos(MediaTypeMusicVideo, videoUrl.ToString(), sorting, items2, dbfilter);
        else
        {
          dbfilter.where = playlist.GetWhereClause(db, playlists);

          if (group.Equals("genres"))
            success2 = db.GetGenresNav(videoUrl.ToString(), items2, VIDEODB_CONTENT_MUSICVIDEOS, dbfilter);
          else if (group.Equals("years"))
            success2 = db.GetYearsNav(videoUrl.ToString(), items2, VIDEODB_CONTENT_MUSICVIDEOS, dbfilter);
          else if (group.Equals("artists"))
            success2 = db.GetActorsNav(videoUrl.ToString(), items2, VIDEODB_CONTENT_MUSICVIDEOS, dbfilter);
          else if (group.Equals("albums"))
            success2 = db.GetMusicVideoAlbumsNav(videoUrl.ToString(), items2, -1, dbfilter);
          else if (group.Equals("directors"))
            success2 = db.GetDirectorsNav(videoUrl.ToString(), items2, VIDEODB_CONTENT_MUSICVIDEOS, dbfilter);
          else if (group.Equals("studios"))
            success2 = db.GetStudiosNav(videoUrl.ToString(), items2, VIDEODB_CONTENT_MUSICVIDEOS, dbfilter);
        }

        db.Close();
        if (items.Size() <= 0)
          items.SetPath(videoUrl.ToString());

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
    if (!playlist.GetGroup().empty())
      items.SetContent(playlist.GetGroup());
    else
      items.SetContent(playlist.GetType());
    items.SetProperty(PROPERTY_SORT_ORDER, (int)playlist.GetOrder());
    items.SetProperty(PROPERTY_SORT_ASCENDING, playlist.GetOrderDirection() == SortOrderAscending);

    // sort grouped list by label
    if (items.Size() > 1 && !group.empty())
      items.Sort(SORT_METHOD_LABEL_IGNORE_THE, SortOrderAscending);

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



