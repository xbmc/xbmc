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

#include <math.h>

#include "SmartPlaylistDirectory.h"
#include "FileItem.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "filesystem/FileDirectoryFactory.h"
#include "music/MusicDatabase.h"
#include "playlists/SmartPlayList.h"
#include "settings/Settings.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "video/VideoDatabase.h"

#define PROPERTY_PATH_DB            "path.db"
#define PROPERTY_SORT_ORDER         "sort.order"
#define PROPERTY_SORT_ASCENDING     "sort.ascending"
#define PROPERTY_GROUP_BY           "group.by"
#define PROPERTY_GROUP_MIXED        "group.mixed"

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
    std::vector<CStdString> virtualFolders;

    SortDescription sorting;
    sorting.limitEnd = playlist.GetLimit();
    sorting.sortBy = playlist.GetOrder();
    sorting.sortOrder = playlist.GetOrderAscending() ? SortOrderAscending : SortOrderDescending;
    sorting.sortAttributes = playlist.GetOrderAttributes();
    if (CSettings::Get().GetBool("filelists.ignorethewhensorting"))
      sorting.sortAttributes = (SortAttribute)(sorting.sortAttributes | SortAttributeIgnoreArticle);
    items.SetSortIgnoreFolders((sorting.sortAttributes & SortAttributeIgnoreFolders) == SortAttributeIgnoreFolders);

    std::string option = !filter ? "xsp" : "filter";
    const CStdString& group = playlist.GetGroup();
    bool isGrouped = !group.empty() && !StringUtils::EqualsNoCase(group, "none") && !playlist.IsGroupMixed();

    // get all virtual folders and add them to the item list
    playlist.GetVirtualFolders(virtualFolders);
    for (std::vector<CStdString>::const_iterator virtualFolder = virtualFolders.begin(); virtualFolder != virtualFolders.end(); virtualFolder++)
    {
      CFileItemPtr pItem = CFileItemPtr(new CFileItem(*virtualFolder, true));
      if (CFileDirectoryFactory::Create(*virtualFolder, pItem.get()) != NULL)
      {
        pItem->SetSpecialSort(SortSpecialOnTop);
        items.Add(pItem);
      }
    }

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
              baseDir = "videodb://tvshows/";
              break;

            case MediaTypeEpisode:
              baseDir = "videodb://tvshows/";
              break;

            case MediaTypeMovie:
              baseDir = "videodb://movies/";
              break;

            default:
              return false;
          }

          if (!isGrouped)
            baseDir += "titles";
          else
            baseDir += group;
          URIUtils::AddSlashAtEnd(baseDir);

          if (mediaType == MediaTypeEpisode)
            baseDir += "-1/-1/";
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
        success = db.GetItems(videoUrl.ToString(), items, dbfilter, sorting);
        db.Close();

        // if we retrieve a list of episodes and we didn't receive
        // a pre-defined base path, we need to fix it
        if (strBaseDir.empty() && mediaType == MediaTypeEpisode && !isGrouped)
          videoUrl.AppendPath("-1/-1/");
        items.SetProperty(PROPERTY_PATH_DB, videoUrl.ToString());
      }
    }
    else if (playlist.IsMusicType() || playlist.GetType().empty())
    {
      CMusicDatabase db;
      if (db.Open())
      {
        CSmartPlaylist plist(playlist);
        if (playlist.GetType().Equals("mixed") || playlist.GetType().empty())
          plist.SetType("songs");

        MediaType mediaType = DatabaseUtils::MediaTypeFromString(plist.GetType());

        CStdString baseDir = strBaseDir;
        if (strBaseDir.empty())
        {
          baseDir = "musicdb://";
          if (!isGrouped)
          {
            switch (mediaType)
            {
              case MediaTypeArtist:
                baseDir += "artists";
                break;

              case MediaTypeAlbum:
                baseDir += "albums";
                break;

              case MediaTypeSong:
                baseDir += "songs";
                break;

              default:
                return false;
            }
          }
          else
            baseDir += group;

          URIUtils::AddSlashAtEnd(baseDir);
        }

        CMusicDbUrl musicUrl;
        if (!musicUrl.FromString(baseDir))
          return false;

        // store the smartplaylist as JSON in the URL as well
        CStdString xsp;
        if (!plist.IsEmpty(filter))
        {
          if (!plist.SaveAsJson(xsp, !filter))
            return false;
        }

        if (!xsp.empty())
          musicUrl.AddOption(option, xsp);
        else
          musicUrl.RemoveOption(option);

        CDatabase::Filter dbfilter;
        success = db.GetItems(musicUrl.ToString(), items, dbfilter, sorting);
        db.Close();

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

          if (!isGrouped)
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
        success2 = db.GetItems(videoUrl.ToString(), items2, dbfilter, sorting);

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
    if (isGrouped)
      items.SetContent(group);
    else
      items.SetContent(playlist.GetType());

    items.SetProperty(PROPERTY_SORT_ORDER, (int)playlist.GetOrder());
    items.SetProperty(PROPERTY_SORT_ASCENDING, playlist.GetOrderDirection() == SortOrderAscending);
    if (!group.empty())
    {
      items.SetProperty(PROPERTY_GROUP_BY, group);
      items.SetProperty(PROPERTY_GROUP_MIXED, playlist.IsGroupMixed());
    }

    // sort grouped list by label
    if (items.Size() > 1 && !group.empty())
      items.Sort(SortByLabel, SortOrderAscending, SortAttributeIgnoreArticle);

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
    if (CSmartPlaylist::IsMusicType(playlistType))
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
          if (StringUtils::EqualsNoCase(playlist.GetName(), name))
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



