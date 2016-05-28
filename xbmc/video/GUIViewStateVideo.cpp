/*
 *      Copyright (C) 2016 Team Kodi
 *      http://kodi.tv
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "GUIViewStateVideo.h"
#include "PlayListPlayer.h"
#include "filesystem/VideoDatabaseDirectory.h"
#include "filesystem/Directory.h"
#include "VideoDatabase.h"
#include "settings/AdvancedSettings.h"
#include "settings/MediaSettings.h"
#include "settings/MediaSourceSettings.h"
#include "settings/Settings.h"
#include "FileItem.h"
#include "guilib/WindowIDs.h"
#include "view/ViewStateSettings.h"
#include "utils/SortUtils.h"

using namespace XFILE;
using namespace VIDEODATABASEDIRECTORY;

std::string CGUIViewStateWindowVideo::GetLockType()
{
  return "video";
}

std::string CGUIViewStateWindowVideo::GetExtensions()
{
  return g_advancedSettings.m_videoExtensions;
}

int CGUIViewStateWindowVideo::GetPlaylist()
{
  return PLAYLIST_VIDEO;
}

VECSOURCES& CGUIViewStateWindowVideo::GetSources()
{
  AddLiveTVSources();
  return CGUIViewState::GetSources();
}

CGUIViewStateWindowVideoNav::CGUIViewStateWindowVideoNav(const CFileItemList& items) : CGUIViewStateWindowVideo(items)
{
  SortAttribute sortAttributes = SortAttributeNone;
  if (CSettings::GetInstance().GetBool(CSettings::SETTING_FILELISTS_IGNORETHEWHENSORTING))
    sortAttributes = SortAttributeIgnoreArticle;

  if (items.IsVirtualDirectoryRoot())
  {
    AddSortMethod(SortByNone, 551, LABEL_MASKS("%F", "%I", "%L", ""));  // Filename, Size | Label, empty
    SetSortMethod(SortByNone);

    SetViewAsControl(DEFAULT_VIEW_LIST);

    SetSortOrder(SortOrderNone);
  }
  else if (items.IsVideoDb())
  {
    NODE_TYPE NodeType=CVideoDatabaseDirectory::GetDirectoryChildType(items.GetPath());
    CQueryParams params;
    CVideoDatabaseDirectory::GetQueryParams(items.GetPath(),params);

    switch (NodeType)
    {
    case NODE_TYPE_MOVIES_OVERVIEW:
    case NODE_TYPE_TVSHOWS_OVERVIEW:
    case NODE_TYPE_MUSICVIDEOS_OVERVIEW:
    case NODE_TYPE_OVERVIEW:
      {
        AddSortMethod(SortByNone, 551, LABEL_MASKS("%F", "%I", "%L", ""));  // Filename, Size | Label, empty

        SetSortMethod(SortByNone);

        SetViewAsControl(DEFAULT_VIEW_LIST);

        SetSortOrder(SortOrderNone);
      }
      break;
    case NODE_TYPE_DIRECTOR:
    case NODE_TYPE_ACTOR:
      {
        AddSortMethod(SortByLabel, 551, LABEL_MASKS("%T", "%R", "%L", ""));  // Title, Rating | Label, empty
        AddSortMethod(SortByRelevance, 38026, LABEL_MASKS("%T", "%c", "%L", "%c"));  // Title, Actor's appearences (Relevance) | Label, Actor's appearences (Relevance)
        SetSortMethod(SortByLabel);

        const CViewState *viewState = CViewStateSettings::GetInstance().Get("videonavactors");
        SetViewAsControl(viewState->m_viewMode);
        SetSortOrder(viewState->m_sortDescription.sortOrder);
      }
      break;
    case NODE_TYPE_YEAR:
      {
        AddSortMethod(SortByLabel, 562, LABEL_MASKS("%T", "%R", "%L", ""));  // Title, Rating | Label, empty
        SetSortMethod(SortByLabel);

        const CViewState *viewState = CViewStateSettings::GetInstance().Get("videonavyears");
        SetViewAsControl(viewState->m_viewMode);
        SetSortOrder(viewState->m_sortDescription.sortOrder);
      }
      break;
    case NODE_TYPE_SEASONS:
      {
        AddSortMethod(SortBySortTitle, 556, LABEL_MASKS("%L", "","%L",""));  // Label, empty | Label, empty
        SetSortMethod(SortBySortTitle);

        const CViewState *viewState = CViewStateSettings::GetInstance().Get("videonavseasons");
        SetViewAsControl(viewState->m_viewMode);
        SetSortOrder(viewState->m_sortDescription.sortOrder);
      }
      break;
    case NODE_TYPE_TITLE_TVSHOWS:
    case NODE_TYPE_INPROGRESS_TVSHOWS:
      {
        AddSortMethod(SortBySortTitle, sortAttributes, 556, LABEL_MASKS("%T", "%M", "%T", "%M"));  // Title, #Episodes | Title, #Episodes

        AddSortMethod(SortByNumberOfEpisodes, 20360, LABEL_MASKS("%L", "%M", "%L", "%M"));  // Label, #Episodes | Label, #Episodes
        AddSortMethod(SortByLastPlayed, 568, LABEL_MASKS("%T", "%p", "%T", "%p"));  // Title, #Last played | Title, #Last played
        AddSortMethod(SortByDateAdded, 570, LABEL_MASKS("%T", "%a", "%T", "%a"));  // Title, DateAdded | Title, DateAdded
        AddSortMethod(SortByYear, 562, LABEL_MASKS("%L","%Y","%L","%Y")); // Label, Year | Label, Year
        AddSortMethod(SortByUserRating, 38018, LABEL_MASKS("%T", "%r", "%T", "%r"));  // Title, Userrating | Title, Userrating
        SetSortMethod(SortByLabel);

        const CViewState *viewState = CViewStateSettings::GetInstance().Get("videonavtvshows");
        SetViewAsControl(viewState->m_viewMode);
        SetSortOrder(viewState->m_sortDescription.sortOrder);
      }
      break;
    case NODE_TYPE_MUSICVIDEOS_ALBUM:
    case NODE_TYPE_GENRE:
    case NODE_TYPE_COUNTRY:
    case NODE_TYPE_STUDIO:
      {
        AddSortMethod(SortByLabel, 551, LABEL_MASKS("%T", "%R", "%L", ""));  // Title, Rating | Label, empty
        SetSortMethod(SortByLabel);

        const CViewState *viewState = CViewStateSettings::GetInstance().Get("videonavgenres");
        SetViewAsControl(viewState->m_viewMode);
        SetSortOrder(viewState->m_sortDescription.sortOrder);
      }
      break;
    case NODE_TYPE_SETS:
      {
        AddSortMethod(SortByLabel, sortAttributes, 551, LABEL_MASKS("%T","%R", "%T","%R"));  // Title, Rating | Title, Rating

        AddSortMethod(SortByYear, 562, LABEL_MASKS("%T", "%Y", "%T", "%Y"));  // Title, Year | Title, Year
        AddSortMethod(SortByRating, 563, LABEL_MASKS("%T", "%R", "%T", "%R"));  // Title, Rating | Title, Rating
        AddSortMethod(SortByDateAdded, 570, LABEL_MASKS("%T", "%a", "%T", "%a"));  // Title, DateAdded | Title, DateAdded

        if (CMediaSettings::GetInstance().GetWatchedMode(items.GetContent()) == WatchedModeAll)
          AddSortMethod(SortByPlaycount, 567, LABEL_MASKS("%T", "%V", "%T", "%V"));  // Title, Playcount | Title, Playcount

        SetSortMethod(SortByLabel);

        const CViewState *viewState = CViewStateSettings::GetInstance().Get("videonavgenres");
        SetViewAsControl(viewState->m_viewMode);
        SetSortOrder(viewState->m_sortDescription.sortOrder);
      }
      break;
    case NODE_TYPE_TAGS:
      {
        AddSortMethod(SortByLabel, sortAttributes, 551, LABEL_MASKS("%T","", "%T",""));  // Title, empty | Title, empty
        SetSortMethod(SortByLabel);
        
        const CViewState *viewState = CViewStateSettings::GetInstance().Get("videonavgenres");
        SetViewAsControl(viewState->m_viewMode);
        SetSortOrder(viewState->m_sortDescription.sortOrder);
      }
      break;
    case NODE_TYPE_EPISODES:
      {
        if (params.GetSeason() > -1)
        {
          AddSortMethod(SortByEpisodeNumber, 20359, LABEL_MASKS("%E. %T","%R"));  // Episode. Title, Rating | empty, empty
          AddSortMethod(SortByRating, 563, LABEL_MASKS("%E. %T", "%R"));  // Episode. Title, Rating | empty, empty
          AddSortMethod(SortByUserRating, 38018, LABEL_MASKS("%E. %T", "%r"));  // Episode. Title, Userrating | empty, empty
          AddSortMethod(SortByMPAA, 20074, LABEL_MASKS("%E. %T", "%O"));  // Episode. Title, MPAA | empty, empty
          AddSortMethod(SortByProductionCode, 20368, LABEL_MASKS("%E. %T","%P", "%E. %T","%P"));  // Episode. Title, ProductionCode | Episode. Title, ProductionCode
          AddSortMethod(SortByDate, 552, LABEL_MASKS("%E. %T","%J","%E. %T","%J"));  // Episode. Title, Date | Episode. Title, Date

          if (CMediaSettings::GetInstance().GetWatchedMode(items.GetContent()) == WatchedModeAll)
            AddSortMethod(SortByPlaycount, 567, LABEL_MASKS("%E. %T", "%V"));  // Episode. Title, Playcount | empty, empty
        }
        else
        {
          AddSortMethod(SortByEpisodeNumber, 20359, LABEL_MASKS("%H. %T","%R"));  // Order. Title, Rating | emtpy, empty
          AddSortMethod(SortByRating, 563, LABEL_MASKS("%H. %T", "%R"));  // Order. Title, Rating | emtpy, empty
          AddSortMethod(SortByUserRating, 38018, LABEL_MASKS("%H. %T", "%r"));  // Order. Title, Userrating | emtpy, empty
          AddSortMethod(SortByMPAA, 20074, LABEL_MASKS("%H. %T", "%O"));  // Order. Title, MPAA | emtpy, empty
          AddSortMethod(SortByProductionCode, 20368, LABEL_MASKS("%H. %T","%P", "%H. %T","%P"));  // Order. Title, ProductionCode | Episode. Title, ProductionCode
          AddSortMethod(SortByDate, 552, LABEL_MASKS("%H. %T","%J","%H. %T","%J"));  // Order. Title, Date | Episode. Title, Date

          if (CMediaSettings::GetInstance().GetWatchedMode(items.GetContent()) == WatchedModeAll)
            AddSortMethod(SortByPlaycount, 567, LABEL_MASKS("%H. %T", "%V"));  // Order. Title, Playcount | empty, empty
        }
        AddSortMethod(SortByLabel, sortAttributes, 551, LABEL_MASKS("%T","%R"));  // Title, Rating | empty, empty

        const CViewState *viewState = CViewStateSettings::GetInstance().Get("videonavepisodes");
        SetSortMethod(viewState->m_sortDescription);
        SetViewAsControl(viewState->m_viewMode);
        SetSortOrder(viewState->m_sortDescription.sortOrder);
        break;
      }
    case NODE_TYPE_RECENTLY_ADDED_EPISODES:
      {
        AddSortMethod(SortByNone, 552, LABEL_MASKS("%Z - %H. %T", "%R"));  // TvShow - Order. Title, Rating | empty, empty
        SetSortMethod(SortByNone);

        SetViewAsControl(CViewStateSettings::GetInstance().Get("videonavepisodes")->m_viewMode);
        SetSortOrder(SortOrderNone);

        break;
      }
    case NODE_TYPE_TITLE_MOVIES:
      {
        if (params.GetSetId() > -1) // Is this a listing within a set?
        {
          AddSortMethod(SortByYear, 562, LABEL_MASKS("%T", "%Y"));  // Title, Year | empty, empty
          AddSortMethod(SortBySortTitle, sortAttributes, 556, LABEL_MASKS("%T", "%R"));  // Title, Rating | empty, empty
        }
        else
        {
          AddSortMethod(SortBySortTitle, sortAttributes, 556, LABEL_MASKS("%T", "%R", "%T", "%R"));  // Title, Rating | Title, Rating
          AddSortMethod(SortByYear, 562, LABEL_MASKS("%T", "%Y", "%T", "%Y"));  // Title, Year | Title, Year
        }
        AddSortMethod(SortByRating, 563, LABEL_MASKS("%T", "%R", "%T", "%R"));  // Title, Rating | Title, Rating
        AddSortMethod(SortByUserRating, 38018, LABEL_MASKS("%T", "%r", "%T", "%r"));  // Title, Userrating | Title, Userrating
        AddSortMethod(SortByMPAA, 20074, LABEL_MASKS("%T", "%O"));  // Title, MPAA | empty, empty
        AddSortMethod(SortByTime, 180, LABEL_MASKS("%T", "%D"));  // Title, Duration | empty, empty
        AddSortMethod(SortByDateAdded, 570, LABEL_MASKS("%T", "%a", "%T", "%a"));  // Title, DateAdded | Title, DateAdded

        if (CMediaSettings::GetInstance().GetWatchedMode(items.GetContent()) == WatchedModeAll)
          AddSortMethod(SortByPlaycount, 567, LABEL_MASKS("%T", "%V", "%T", "%V"));  // Title, Playcount | Title, Playcount

        const CViewState *viewState = CViewStateSettings::GetInstance().Get("videonavtitles");
        if (params.GetSetId() > -1)
        {
          SetSortMethod(SortByYear);
          SetSortOrder(SortOrderAscending);
        }
        else
        {
          SetSortMethod(viewState->m_sortDescription);
          SetSortOrder(viewState->m_sortDescription.sortOrder);
        }

        SetViewAsControl(viewState->m_viewMode);
      }
      break;
      case NODE_TYPE_TITLE_MUSICVIDEOS:
      {
        AddSortMethod(SortByLabel, sortAttributes, 551, LABEL_MASKS("%T", "%Y"));  // Title, Year | empty, empty
        AddSortMethod(SortByMPAA, 20074, LABEL_MASKS("%T", "%O"));
        AddSortMethod(SortByYear, 562, LABEL_MASKS("%T", "%Y"));  // Title, Year | empty, empty
        AddSortMethod(SortByArtist, sortAttributes, 557, LABEL_MASKS("%A - %T", "%Y"));  // Artist - Title, Year | empty, empty
        AddSortMethod(SortByAlbum, sortAttributes, 558, LABEL_MASKS("%B - %T", "%Y"));  // Album - Title, Year | empty, empty

        if (CMediaSettings::GetInstance().GetWatchedMode(items.GetContent()) == WatchedModeAll)
          AddSortMethod(SortByPlaycount, 567, LABEL_MASKS("%T", "%V"));  // Title, Playcount | empty, empty

        std::string strTrack=CSettings::GetInstance().GetString(CSettings::SETTING_MUSICFILES_TRACKFORMAT);
        AddSortMethod(SortByTrackNumber, 554, LABEL_MASKS(strTrack, "%N"));  // Userdefined, Track Number | empty, empty

        const CViewState *viewState = CViewStateSettings::GetInstance().Get("videonavmusicvideos");
        SetSortMethod(viewState->m_sortDescription);
        SetViewAsControl(viewState->m_viewMode);
        SetSortOrder(viewState->m_sortDescription.sortOrder);
      }
      break;
    case NODE_TYPE_RECENTLY_ADDED_MOVIES:
      {
        AddSortMethod(SortByNone, 552, LABEL_MASKS("%T", "%R"));  // Title, Rating | empty, empty
        SetSortMethod(SortByNone);

        SetViewAsControl(CViewStateSettings::GetInstance().Get("videonavtitles")->m_viewMode);

        SetSortOrder(SortOrderNone);
      }
      break;
    case NODE_TYPE_RECENTLY_ADDED_MUSICVIDEOS:
      {
        AddSortMethod(SortByNone, 552, LABEL_MASKS("%A - %T", "%Y"));  // Artist - Title, Year | empty, empty
        SetSortMethod(SortByNone);

        SetViewAsControl(CViewStateSettings::GetInstance().Get("videonavmusicvideos")->m_viewMode);

        SetSortOrder(SortOrderNone);
      }
      break;
    default:
      break;
    }
  }
  else
  {
    AddSortMethod(SortByLabel, sortAttributes, 551, LABEL_MASKS("%L", "%I", "%L", ""));  // Label, Size | Label, empty
    AddSortMethod(SortBySize, 553, LABEL_MASKS("%L", "%I", "%L", "%I"));  // Label, Size | Label, Size
    AddSortMethod(SortByDate, 552, LABEL_MASKS("%L", "%J", "%L", "%J"));  // Label, Date | Label, Date
    AddSortMethod(SortByFile, 561, LABEL_MASKS("%L", "%I", "%L", ""));  // Label, Size | Label, empty
    
    const CViewState *viewState = CViewStateSettings::GetInstance().Get("videofiles");
    SetSortMethod(viewState->m_sortDescription);
    SetViewAsControl(viewState->m_viewMode);
    SetSortOrder(viewState->m_sortDescription.sortOrder);
  }
  LoadViewState(items.GetPath(), WINDOW_VIDEO_NAV);
}

void CGUIViewStateWindowVideoNav::SaveViewState()
{
  if (m_items.IsVideoDb())
  {
    NODE_TYPE NodeType = CVideoDatabaseDirectory::GetDirectoryChildType(m_items.GetPath());
    CQueryParams params;
    CVideoDatabaseDirectory::GetQueryParams(m_items.GetPath(),params);
    switch (NodeType)
    {
    case NODE_TYPE_ACTOR:
      SaveViewToDb(m_items.GetPath(), WINDOW_VIDEO_NAV, CViewStateSettings::GetInstance().Get("videonavactors"));
      break;
    case NODE_TYPE_YEAR:
      SaveViewToDb(m_items.GetPath(), WINDOW_VIDEO_NAV, CViewStateSettings::GetInstance().Get("videonavyears"));
      break;
    case NODE_TYPE_GENRE:
      SaveViewToDb(m_items.GetPath(), WINDOW_VIDEO_NAV, CViewStateSettings::GetInstance().Get("videonavgenres"));
      break;
    case NODE_TYPE_TITLE_MOVIES:
      SaveViewToDb(m_items.GetPath(), WINDOW_VIDEO_NAV, params.GetSetId() > -1 ? NULL : CViewStateSettings::GetInstance().Get("videonavtitles"));
      break;
    case NODE_TYPE_EPISODES:
      SaveViewToDb(m_items.GetPath(), WINDOW_VIDEO_NAV, CViewStateSettings::GetInstance().Get("videonavepisodes"));
      break;
    case NODE_TYPE_TITLE_TVSHOWS:
      SaveViewToDb(m_items.GetPath(), WINDOW_VIDEO_NAV, CViewStateSettings::GetInstance().Get("videonavtvshows"));
      break;
    case NODE_TYPE_SEASONS:
      SaveViewToDb(m_items.GetPath(), WINDOW_VIDEO_NAV, CViewStateSettings::GetInstance().Get("videonavseasons"));
      break;
    case NODE_TYPE_TITLE_MUSICVIDEOS:
      SaveViewToDb(m_items.GetPath(), WINDOW_VIDEO_NAV, CViewStateSettings::GetInstance().Get("videonavmusicvideos"));
      break;
    default:
      SaveViewToDb(m_items.GetPath(), WINDOW_VIDEO_NAV);
      break;
    }
  }
  else
  {
    SaveViewToDb(m_items.GetPath(), WINDOW_VIDEO_NAV, CViewStateSettings::GetInstance().Get("videofiles"));
  }
}

VECSOURCES& CGUIViewStateWindowVideoNav::GetSources()
{
  //  Setup shares we want to have
  m_sources.clear();
  CFileItemList items;
  if (CSettings::GetInstance().GetBool(CSettings::SETTING_MYVIDEOS_FLATTEN))
    CDirectory::GetDirectory("library://video_flat/", items, "");
  else
    CDirectory::GetDirectory("library://video/", items, "");
  for (int i=0; i<items.Size(); ++i)
  {
    CFileItemPtr item=items[i];
    CMediaSource share;
    share.strName=item->GetLabel();
    share.strPath = item->GetPath();
    share.m_strThumbnailImage= item->GetIconImage();
    share.m_iDriveType = CMediaSource::SOURCE_TYPE_LOCAL;
    m_sources.push_back(share);
  }
  return CGUIViewStateWindowVideo::GetSources();
}

bool CGUIViewStateWindowVideoNav::AutoPlayNextItem()
{
  CQueryParams params;
  CVideoDatabaseDirectory::GetQueryParams(m_items.GetPath(),params);
  if (params.GetContentType() == VIDEODB_CONTENT_MUSICVIDEOS || params.GetContentType() == 6) // recently added musicvideos
    return CSettings::GetInstance().GetBool(CSettings::SETTING_MUSICPLAYER_AUTOPLAYNEXTITEM);

  return CSettings::GetInstance().GetBool(CSettings::SETTING_VIDEOPLAYER_AUTOPLAYNEXTITEM);
}

CGUIViewStateWindowVideoPlaylist::CGUIViewStateWindowVideoPlaylist(const CFileItemList& items) : CGUIViewStateWindowVideo(items)
{
  AddSortMethod(SortByNone, 551, LABEL_MASKS("%L", "", "%L", ""));  // Label, empty | Label, empty
  SetSortMethod(SortByNone);

  SetViewAsControl(DEFAULT_VIEW_LIST);

  SetSortOrder(SortOrderNone);

  LoadViewState(items.GetPath(), WINDOW_VIDEO_PLAYLIST);
}

void CGUIViewStateWindowVideoPlaylist::SaveViewState()
{
  SaveViewToDb(m_items.GetPath(), WINDOW_VIDEO_PLAYLIST);
}

bool CGUIViewStateWindowVideoPlaylist::HideExtensions()
{
  return true;
}

bool CGUIViewStateWindowVideoPlaylist::HideParentDirItems()
{
  return true;
}

VECSOURCES& CGUIViewStateWindowVideoPlaylist::GetSources()
{
  m_sources.clear();
  //  Playlist share
  CMediaSource share;
  share.strPath= "playlistvideo://";
  share.m_iDriveType = CMediaSource::SOURCE_TYPE_LOCAL;
  m_sources.push_back(share);

  // no plugins in playlist window
  return m_sources;
}

CGUIViewStateVideoMovies::CGUIViewStateVideoMovies(const CFileItemList& items) : CGUIViewStateWindowVideo(items)
{
  AddSortMethod(SortBySortTitle, 556, LABEL_MASKS("%T", "%R", "%T", "%R"),  // Title, Rating | Title, Rating
    CSettings::GetInstance().GetBool(CSettings::SETTING_FILELISTS_IGNORETHEWHENSORTING) ? SortAttributeIgnoreArticle : SortAttributeNone);
  AddSortMethod(SortByYear, 562, LABEL_MASKS("%T", "%Y", "%T", "%Y"));  // Title, Year | Title, Year
  AddSortMethod(SortByRating, 563, LABEL_MASKS("%T", "%R", "%T", "%R"));  // Title, Rating | Title, Rating
  AddSortMethod(SortByUserRating, 38018, LABEL_MASKS("%T", "%r", "%T", "%r"));  // Title, Userrating | Title, Userrating
  AddSortMethod(SortByMPAA, 20074, LABEL_MASKS("%T", "%O"));  // Title, MPAA | empty, empty
  AddSortMethod(SortByTime, 180, LABEL_MASKS("%T", "%D"));  // Title, Duration | empty, empty
  AddSortMethod(SortByDateAdded, 570, LABEL_MASKS("%T", "%a", "%T", "%a"));  // Title, DateAdded | Title, DateAdded

  if (CMediaSettings::GetInstance().GetWatchedMode(items.GetContent()) == WatchedModeAll)
    AddSortMethod(SortByPlaycount, 567, LABEL_MASKS("%T", "%V", "%T", "%V"));  // Title, Playcount | Title, Playcount

  const CViewState *viewState = CViewStateSettings::GetInstance().Get("videonavtitles");
  if (items.IsSmartPlayList() || items.IsLibraryFolder())
    AddPlaylistOrder(items, LABEL_MASKS("%T", "%R", "%T", "%R"));  // Title, Rating | Title, Rating
  else
  {
    SetSortMethod(viewState->m_sortDescription);
    SetSortOrder(viewState->m_sortDescription.sortOrder);
  }

  SetViewAsControl(viewState->m_viewMode);

  LoadViewState(items.GetPath(), WINDOW_VIDEO_NAV);
}

void CGUIViewStateVideoMovies::SaveViewState()
{
  SaveViewToDb(m_items.GetPath(), WINDOW_VIDEO_NAV, CViewStateSettings::GetInstance().Get("videonavtitles"));
}

CGUIViewStateVideoMusicVideos::CGUIViewStateVideoMusicVideos(const CFileItemList& items) : CGUIViewStateWindowVideo(items)
{
  SortAttribute sortAttributes = SortAttributeNone;
  if (CSettings::GetInstance().GetBool(CSettings::SETTING_FILELISTS_IGNORETHEWHENSORTING))
    sortAttributes = SortAttributeIgnoreArticle;

  AddSortMethod(SortByLabel, sortAttributes, 551, LABEL_MASKS("%T", "%Y"));  // Title, Year | empty, empty
  AddSortMethod(SortByMPAA, 20074, LABEL_MASKS("%T", "%O"));
  AddSortMethod(SortByYear, 562, LABEL_MASKS("%T", "%Y"));  // Title, Year | empty, empty
  AddSortMethod(SortByArtist, sortAttributes, 557, LABEL_MASKS("%A - %T", "%Y"));  // Artist - Title, Year | empty, empty
  AddSortMethod(SortByAlbum, sortAttributes, 558, LABEL_MASKS("%B - %T", "%Y"));  // Album - Title, Year | empty, empty

   if (CMediaSettings::GetInstance().GetWatchedMode(items.GetContent()) == WatchedModeAll)
    AddSortMethod(SortByPlaycount, 567, LABEL_MASKS("%T", "%V"));  // Title, Playcount | empty, empty
  
  std::string strTrack=CSettings::GetInstance().GetString(CSettings::SETTING_MUSICFILES_TRACKFORMAT);
  AddSortMethod(SortByTrackNumber, 554, LABEL_MASKS(strTrack, "%N"));  // Userdefined, Track Number | empty, empty

  const CViewState *viewState = CViewStateSettings::GetInstance().Get("videonavmusicvideos");
  if (items.IsSmartPlayList() || items.IsLibraryFolder())
    AddPlaylistOrder(items, LABEL_MASKS("%A - %T", "%Y"));  // Artist - Title, Year | empty, empty
  else
  {
    SetSortMethod(viewState->m_sortDescription);
    SetSortOrder(viewState->m_sortDescription.sortOrder);
  }

  SetViewAsControl(viewState->m_viewMode);

  LoadViewState(items.GetPath(), WINDOW_VIDEO_NAV);
}

void CGUIViewStateVideoMusicVideos::SaveViewState()
{
  SaveViewToDb(m_items.GetPath(), WINDOW_VIDEO_NAV, CViewStateSettings::GetInstance().Get("videonavmusicvideos"));
}

CGUIViewStateVideoTVShows::CGUIViewStateVideoTVShows(const CFileItemList& items) : CGUIViewStateWindowVideo(items)
{
  AddSortMethod(SortBySortTitle, 556, LABEL_MASKS("%T", "%M", "%T", "%M"),  // Title, #Episodes | Title, #Episodes
    CSettings::GetInstance().GetBool(CSettings::SETTING_FILELISTS_IGNORETHEWHENSORTING) ? SortAttributeIgnoreArticle : SortAttributeNone);

  AddSortMethod(SortByNumberOfEpisodes, 20360, LABEL_MASKS("%L", "%M", "%L", "%M"));  // Label, #Episodes | Label, #Episodes
  AddSortMethod(SortByLastPlayed, 568, LABEL_MASKS("%T", "%p", "%T", "%p"));  // Title, #Last played | Title, #Last played
  AddSortMethod(SortByYear, 562, LABEL_MASKS("%T", "%Y", "%T", "%Y"));  // Title, Year | Title, Year
  AddSortMethod(SortByUserRating, 38018, LABEL_MASKS("%T", "%r", "%T", "%r"));  // Title, Userrating | Title, Userrating

  const CViewState *viewState = CViewStateSettings::GetInstance().Get("videonavtvshows");
  if (items.IsSmartPlayList() || items.IsLibraryFolder())
    AddPlaylistOrder(items, LABEL_MASKS("%T", "%M", "%T", "%M"));  // Title, #Episodes | Title, #Episodes
  else
  {
    SetSortMethod(viewState->m_sortDescription);
    SetSortOrder(viewState->m_sortDescription.sortOrder);
  }

  SetViewAsControl(viewState->m_viewMode);

  LoadViewState(items.GetPath(), WINDOW_VIDEO_NAV);
}

void CGUIViewStateVideoTVShows::SaveViewState()
{
  SaveViewToDb(m_items.GetPath(), WINDOW_VIDEO_NAV, CViewStateSettings::GetInstance().Get("videonavtvshows"));
}

CGUIViewStateVideoEpisodes::CGUIViewStateVideoEpisodes(const CFileItemList& items) : CGUIViewStateWindowVideo(items)
{
  if (0)//params.GetSeason() > -1)
  {
    AddSortMethod(SortByEpisodeNumber, 20359, LABEL_MASKS("%E. %T","%R"));  // Episode. Title, Rating | empty, empty
    AddSortMethod(SortByRating, 563, LABEL_MASKS("%E. %T", "%R"));  // Episode. Title, Rating | empty, empty
    AddSortMethod(SortByUserRating, 38018, LABEL_MASKS("%E. %T", "%r"));  // Episode. Title, Userrating | empty, empty
    AddSortMethod(SortByMPAA, 20074, LABEL_MASKS("%E. %T", "%O"));  // Episode. Title, MPAA | empty, empty
    AddSortMethod(SortByProductionCode, 20368, LABEL_MASKS("%E. %T","%P", "%E. %T","%P"));  // Episode. Title, Production Code | Episode. Title, Production Code
    AddSortMethod(SortByDate, 552, LABEL_MASKS("%E. %T","%J","E. %T","%J"));  // Episode. Title, Date | Episode. Title, Date

    if (CMediaSettings::GetInstance().GetWatchedMode(items.GetContent()) == WatchedModeAll)
      AddSortMethod(SortByPlaycount, 567, LABEL_MASKS("%E. %T", "%V"));  // Episode. Title, Playcount | empty, empty
  }
  else
  { // format here is tvshowtitle - season/episode number. episode title
    AddSortMethod(SortByEpisodeNumber, 20359, LABEL_MASKS("%Z - %H. %T","%R"));  // TvShow - Order. Title, Rating | empty, empty
    AddSortMethod(SortByRating, 563, LABEL_MASKS("%Z - %H. %T", "%R"));  // TvShow - Order. Title, Rating | empty, empty
    AddSortMethod(SortByUserRating, 38018, LABEL_MASKS("%Z - %H. %T", "%r"));  // TvShow - Order. Title, Userrating | empty, empty
    AddSortMethod(SortByMPAA, 20074, LABEL_MASKS("%Z - %H. %T", "%O"));  // TvShow - Order. Title, MPAA | empty, empty
    AddSortMethod(SortByProductionCode, 20368, LABEL_MASKS("%Z - %H. %T","%P"));  // TvShow - Order. Title, Production Code | empty, empty
    AddSortMethod(SortByDate, 552, LABEL_MASKS("%Z - %H. %T","%J"));  // TvShow - Order. Title, Date | empty, empty

    if (CMediaSettings::GetInstance().GetWatchedMode(items.GetContent()) == WatchedModeAll)
      AddSortMethod(SortByPlaycount, 567, LABEL_MASKS("%H. %T", "%V"));  // Order. Title, Playcount | empty, empty
  }

  AddSortMethod(SortByLabel, 551, LABEL_MASKS("%Z - %H. %T","%R"),  // TvShow - Order. Title, Rating | empty, empty
    CSettings::GetInstance().GetBool(CSettings::SETTING_FILELISTS_IGNORETHEWHENSORTING) ? SortAttributeIgnoreArticle : SortAttributeNone);

  const CViewState *viewState = CViewStateSettings::GetInstance().Get("videonavepisodes");
  if (items.IsSmartPlayList() || items.IsLibraryFolder())
    AddPlaylistOrder(items, LABEL_MASKS("%Z - %H. %T", "%R"));  // TvShow - Order. Title, Rating | empty, empty
  else
  {
    SetSortMethod(viewState->m_sortDescription);
    SetSortOrder(viewState->m_sortDescription.sortOrder);
  }

  SetViewAsControl(viewState->m_viewMode);

  LoadViewState(items.GetPath(), WINDOW_VIDEO_NAV);
}

void CGUIViewStateVideoEpisodes::SaveViewState()
{
  SaveViewToDb(m_items.GetPath(), WINDOW_VIDEO_NAV, CViewStateSettings::GetInstance().Get("videonavepisodes"));
}

