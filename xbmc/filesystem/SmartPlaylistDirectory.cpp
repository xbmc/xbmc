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

#include <math.h>

#include "SmartPlaylistDirectory.h"
#include "utils/log.h"
#include "playlists/SmartPlayList.h"
#include "music/MusicDatabase.h"
#include "video/VideoDatabase.h"
#include "Directory.h"
#include "File.h"
#include "FileItem.h"
#include "utils/URIUtils.h"

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
    return GetDirectory(playlist, items);
  }
  
  bool CSmartPlaylistDirectory::GetDirectory(const CSmartPlaylist &playlist, CFileItemList& items)
  {
    bool success = false, success2 = false;
    std::set<CStdString> playlists;

    SortDescription sorting;
    sorting.limitEnd = playlist.GetLimit();
    sorting.sortBy = playlist.GetOrder();
    sorting.sortOrder = playlist.GetOrderAscending() ? SortOrderAscending : SortOrderDescending;
    sorting.sortAttributes = SortAttributeIgnoreArticle;

    if (playlist.GetType().Equals("movies") ||
        playlist.GetType().Equals("tvshows") ||
        playlist.GetType().Equals("episodes"))
    {
      CVideoDatabase db;
      if (db.Open())
      {
        MediaType mediaType = DatabaseUtils::MediaTypeFromString(playlist.GetType());
        CVideoDatabase::Filter filter;
        filter.where = playlist.GetWhereClause(db, playlists);

        CStdString strBaseDir;
        switch (mediaType)
        {
        case MediaTypeTvShow:
        case MediaTypeEpisode:
          strBaseDir = "videodb://2/2/";
          break;

        case MediaTypeMovie:
          strBaseDir = "videodb://1/2/";
          break;

        default:
          return false;
        }

        success = db.GetSortedVideos(mediaType, strBaseDir, sorting, items, filter, true);
        db.Close();
      }
    }
    else if (playlist.GetType().Equals("albums"))
    {
      CMusicDatabase db;
      if (db.Open())
      {
        CStdString whereClause = playlist.GetWhereClause(db, playlists);
        if (!whereClause.empty())
          whereClause = "WHERE " + whereClause;
        success = db.GetSortedAlbums("musicdb://3/", sorting, items, whereClause);
        items.SetContent("albums");
        db.Close();
      }
    }
    if (playlist.GetType().Equals("songs") || playlist.GetType().Equals("mixed") || playlist.GetType().IsEmpty())
    {
      CMusicDatabase db;
      if (db.Open())
      {
        CStdString whereClause;
        if (playlist.GetType().IsEmpty() || playlist.GetType().Equals("mixed"))
        {
          CSmartPlaylist songPlaylist(playlist);
          songPlaylist.SetType("songs");
          whereClause = songPlaylist.GetWhereClause(db, playlists);
        }
        else
          whereClause = playlist.GetWhereClause(db, playlists);

        if (!whereClause.empty())
          whereClause = "WHERE " + whereClause;

        success = db.GetSortedSongs("", sorting, items, whereClause);
        items.SetContent("songs");
        db.Close();
      }
    }
    if (playlist.GetType().Equals("musicvideos") || playlist.GetType().Equals("mixed"))
    {
      CVideoDatabase db;
      if (db.Open())
      {
        CVideoDatabase::Filter filter;
        if (playlist.GetType().Equals("mixed"))
        {
          CSmartPlaylist mvidPlaylist(playlist);
          mvidPlaylist.SetType("musicvideos");
          filter.where = mvidPlaylist.GetWhereClause(db, playlists);
        }
        else
          filter.where = playlist.GetWhereClause(db, playlists);

        CFileItemList items2;
        success2 = db.GetSortedVideos(MediaTypeMusicVideo, "videodb://3/2/", sorting, items2, filter);
        db.Close();

        items.Append(items2);
        if (items2.Size())
        {
          if (items.Size() > items2.Size())
            items.SetContent("mixed");
          else
            items.SetContent("musicvideos");
        }
      }
    }
    items.SetLabel(playlist.GetName());

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



