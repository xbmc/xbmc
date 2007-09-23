/*
 *      Copyright (C) 2005-2007 Team XboxMediaCenter
 *      http://www.xboxmediacenter.com
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
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "GUIWindowVideoNav.h"
#include "GUIWIndowVideoFiles.h"
#include "utils/GUIInfoManager.h"
#include "util.h"
#include "PlayListM3U.h"
#include "application.h"
#include "playlistplayer.h"
#include "GUIPassword.h"
#include "GUILabelControl.h"
#include "GUIDialogFileBrowser.h"
#include "Picture.h"
#include "FileSystem/VideoDatabaseDirectory.h"
#include "PlaylistFactory.h"
#include "GUIFontManager.h"
#include "GUIDialogVideoScan.h"
#include "PartyModeManager.h"
#include "MusicDatabase.h"

using namespace XFILE;
using namespace DIRECTORY;
using namespace VIDEODATABASEDIRECTORY;

#define CONTROL_BTNVIEWASICONS     2
#define CONTROL_BTNSORTBY          3
#define CONTROL_BTNSORTASC         4
#define CONTROL_BTNTYPE            5
#define CONTROL_BTNSEARCH          8
#define CONTROL_LIST              50
#define CONTROL_THUMBS            51
#define CONTROL_BIGLIST           52
#define CONTROL_LABELFILES        12

#define CONTROL_BTN_FILTER        19
#define CONTROL_BTNSHOWMODE       10
#define CONTROL_BTNSHOWALL        14
#define CONTROL_UNLOCK            11

#define CONTROL_FILTER            15
#define CONTROL_BTNPARTYMODE      16
#define CONTROL_LABELEMPTY        18

CGUIWindowVideoNav::CGUIWindowVideoNav(void)
    : CGUIWindowVideoBase(WINDOW_VIDEO_NAV, "MyVideoNav.xml")
{
  m_vecItems.m_strPath = "?";
  m_bDisplayEmptyDatabaseMessage = false;
  m_thumbLoader.SetObserver(this);
}

CGUIWindowVideoNav::~CGUIWindowVideoNav(void)
{
}

bool CGUIWindowVideoNav::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_WINDOW_RESET:
    m_vecItems.m_strPath = "?";
    break;
  case GUI_MSG_WINDOW_DEINIT:
    if (m_thumbLoader.IsLoading())
      m_thumbLoader.StopThread();
    break;
  case GUI_MSG_WINDOW_INIT:
    {
      // check for valid quickpath parameter
      CStdString strDestination = message.GetStringParam();
      if (!strDestination.IsEmpty())
      {
        message.SetStringParam("");
        CLog::Log(LOGINFO, "Attempting to quickpath to: %s", strDestination.c_str());
      }

      // is this the first time the window is opened?
      if (m_vecItems.m_strPath == "?" && strDestination.IsEmpty())
      {
        strDestination = g_settings.m_defaultVideoLibSource;
        m_vecItems.m_strPath = strDestination;
        CLog::Log(LOGINFO, "Attempting to default to: %s", strDestination.c_str());
      }

      if (!strDestination.IsEmpty())
      {
        if (strDestination.Equals("$ROOT") || strDestination.Equals("Root"))
          m_vecItems.m_strPath = "";
        else if (strDestination.Equals("MovieGenres"))
          m_vecItems.m_strPath = "videodb://1/1/";
        else if (strDestination.Equals("MovieTitles"))
          m_vecItems.m_strPath = "videodb://1/2/";
        else if (strDestination.Equals("MovieYears"))
          m_vecItems.m_strPath = "videodb://1/3/";
        else if (strDestination.Equals("MovieActors"))
          m_vecItems.m_strPath = "videodb://1/4/";
        else if (strDestination.Equals("MovieDirectors"))
          m_vecItems.m_strPath = "videodb://1/5/";
        else if (strDestination.Equals("MovieStudios"))
          m_vecItems.m_strPath = "videodb://1/6/";
        else if (strDestination.Equals("Movies"))
          m_vecItems.m_strPath = "videodb://1/";
        else if (strDestination.Equals("TvShowGenres"))
          m_vecItems.m_strPath = "videodb://2/1/";
        else if (strDestination.Equals("TvShowTitles"))
          m_vecItems.m_strPath = "videodb://2/2/";
        else if (strDestination.Equals("TvShowYears"))
          m_vecItems.m_strPath = "videodb://2/3/";
        else if (strDestination.Equals("TvShowActors"))
          m_vecItems.m_strPath = "videodb://2/4/";
        else if (strDestination.Equals("TvShows"))
          m_vecItems.m_strPath = "videodb://2/";
        else if (strDestination.Equals("MusicVideoGenres"))
          m_vecItems.m_strPath = "videodb://3/1/";
        else if (strDestination.Equals("MusicVideoTitles"))
          m_vecItems.m_strPath = "videodb://3/2/";
        else if (strDestination.Equals("MusicVideoYears"))
          m_vecItems.m_strPath = "videodb://3/3/";
        else if (strDestination.Equals("MusicVideoArtists"))
          m_vecItems.m_strPath = "videodb://3/4/";
        else if (strDestination.Equals("MusicVideoDirectors"))
          m_vecItems.m_strPath = "videodb://3/5/";
        else if (strDestination.Equals("MusicVideoStudios"))
          m_vecItems.m_strPath = "videodb://3/6/";
        else if (strDestination.Equals("MusicVideos"))
          m_vecItems.m_strPath = "videodb://3/";
        else if (strDestination.Equals("Playlists"))
          m_vecItems.m_strPath = "special://videoplaylists/";
        else
        {
          CLog::Log(LOGWARNING, "Warning, destination parameter (%s) may not be valid", strDestination.c_str());
          m_vecItems.m_strPath = strDestination;
        }
        SetHistoryForPath(m_vecItems.m_strPath);
      }
      
      DisplayEmptyDatabaseMessage(false); // reset message state

      if (!CGUIWindowVideoBase::OnMessage(message))
        return false;

      //  base class has opened the database, do our check
      m_database.Open();
      DisplayEmptyDatabaseMessage(m_database.GetMovieCount() <= 0 && m_database.GetTvShowCount() <= 0 && m_database.GetMusicVideoCount() <= 0);

      if (m_bDisplayEmptyDatabaseMessage)
      {
        SET_CONTROL_FOCUS(CONTROL_BTNTYPE, 0);
        Update(m_vecItems.m_strPath);  // Will remove content from the list/thumb control
      }

      m_database.Close();
      return true;
    }
    break;

  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == CONTROL_BTNPARTYMODE)
      {
        if (g_partyModeManager.IsEnabled())
          g_partyModeManager.Disable();
        else
        {
          g_partyModeManager.Enable(true);

          // Playlist directory is the root of the playlist window
          if (m_guiState.get()) m_guiState->SetPlaylistDirectory("playlistvideo://");

          return true;
        }
        UpdateButtons();
      }

      if (iControl == CONTROL_BTNSEARCH)
      {
        OnSearch();
      }
      else if (iControl == CONTROL_BTN_FILTER)
      {
        CGUIDialogKeyboard::ShowAndGetFilter(m_filter, false);
        return true;
      }
      else if (iControl == CONTROL_BTNSHOWMODE)
      {
        g_stSettings.m_iMyVideoWatchMode++;
        if (g_stSettings.m_iMyVideoWatchMode > VIDEO_SHOW_WATCHED)
          g_stSettings.m_iMyVideoWatchMode = VIDEO_SHOW_ALL;
        g_settings.Save();
        // TODO: Can we perhaps filter this directly?  Probably not for some of the more complicated views,
        //       but for those perhaps we can just display them all, and only filter when we get a list
        //       of actual videos?
        Update(m_vecItems.m_strPath);
        return true;
      }
      else if (iControl == CONTROL_BTNSHOWALL)
      {
        if (g_stSettings.m_iMyVideoWatchMode == VIDEO_SHOW_ALL)
          g_stSettings.m_iMyVideoWatchMode = VIDEO_SHOW_UNWATCHED;
        else
          g_stSettings.m_iMyVideoWatchMode = VIDEO_SHOW_ALL;
        g_settings.Save();
        // TODO: Can we perhaps filter this directly?  Probably not for some of the more complicated views,
        //       but for those perhaps we can just display them all, and only filter when we get a list
        //       of actual videos?
        Update(m_vecItems.m_strPath);
        return true;
      }
    }
    break;
    // update the display
    case GUI_MSG_SCAN_FINISHED:
    case GUI_MSG_REFRESH_THUMBS:
    {
      Update(m_vecItems.m_strPath);
    }
    break;

  case GUI_MSG_NOTIFY_ALL:
    {
      if (message.GetParam1() == GUI_MSG_FILTER_ITEMS && IsActive())
      {
        m_filter = message.GetStringParam();
        m_filter.TrimLeft().ToLower();
        OnFilterItems();
      }
    }
    break;
  }
  return CGUIWindowVideoBase::OnMessage(message);
}

CStdString CGUIWindowVideoNav::GetQuickpathName(const CStdString& strPath) const
{
  if (strPath.Equals("videodb://1/1/"))
    return "MovieGenres";
  else if (strPath.Equals("videodb://1/2/"))
    return "MovieTitles";
  else if (strPath.Equals("videodb://1/3/"))
    return "MovieYears";
  else if (strPath.Equals("videodb://1/4/"))
    return "MovieActors";
  else if (strPath.Equals("videodb://1/5/"))
    return "MovieDirectors";
  else if (strPath.Equals("videodb://1/"))
    return "Movies";
  else if (strPath.Equals("videodb://2/1/"))
    return "TvShowGenres";
  else if (strPath.Equals("videodb://2/2/"))
    return "TvShowTitles";
  else if (strPath.Equals("videodb://2/3/"))
    return "TvShowYears";
  else if (strPath.Equals("videodb://2/4/"))
    return "TvShowActors";
  else if (strPath.Equals("videodb://2/"))
    return "TvShows";
  else if (strPath.Equals("videodb://3/1/"))
    return "MusicVideoGenres";
  else if (strPath.Equals("videodb://3/2/"))
    return "MusicVideoTitles";
  else if (strPath.Equals("videodb://3/3/"))
    return "MusicVideoYears";
  else if (strPath.Equals("videodb://3/4/"))
    return "MusicVideoArtists";
  else if (strPath.Equals("videodb://3/5/"))
    return "MusicVideoDirectors";
  else if (strPath.Equals("videodb://3/"))
    return "MusicVideos";
  else if (strPath.Equals("videodb://4/"))
    return "RecentlyAddedMovies";
  else if (strPath.Equals("videodb://5/"))
    return "RecentlyAddedEpisodes";
  else if (strPath.Equals("videodb://6/"))
    return "RecentlyAddedMusicVideos";
  else if (strPath.Equals("special://videoplaylists/"))
    return "Playlists";
  else
  {
    CLog::Log(LOGERROR, "  CGUIWindowVideoNav::GetQuickpathName: Unknown parameter (%s)", strPath.c_str());
    return strPath;
  }
}

bool CGUIWindowVideoNav::GetDirectory(const CStdString &strDirectory, CFileItemList &items)
{
  if (m_bDisplayEmptyDatabaseMessage)
    return true;

  CFileItem directory(strDirectory, true);

  if (m_thumbLoader.IsLoading())
    m_thumbLoader.StopThread();

  m_rootDir.SetCacheDirectory(false);
  bool bResult = CGUIWindowVideoBase::GetDirectory(strDirectory, items);
  if (bResult)
  {
    if (!items.IsVideoDb())  // don't need to do this for playlist, as OnRetrieveMusicInfo() should ideally set thumbs
    {
      items.SetCachedVideoThumbs();
      m_thumbLoader.Load(m_vecItems);
      g_infoManager.m_content = "";
    }
    else
    {
      DIRECTORY::CVideoDatabaseDirectory dir;
      CQueryParams params;
      dir.GetQueryParams(items.m_strPath,params);
      VIDEODATABASEDIRECTORY::NODE_TYPE node = dir.GetDirectoryChildType(items.m_strPath);
      
      items.SetThumbnailImage("");
      if (node == VIDEODATABASEDIRECTORY::NODE_TYPE_EPISODES || node == NODE_TYPE_SEASONS || node == NODE_TYPE_RECENTLY_ADDED_EPISODES)
      {
        CFileItem item;
        if (node == NODE_TYPE_EPISODES || node == NODE_TYPE_RECENTLY_ADDED_EPISODES)
        {
          g_infoManager.m_content = "episodes";
          item.m_strPath = items.m_strPath;
          item.SetCachedSeasonThumb();
        }
        else
          g_infoManager.m_content = "seasons";

        if (!item.HasThumbnail())
        {
          m_database.GetFilePath(params.GetTvShowId(),item.m_strPath,2);
          item.SetVideoThumb();
        }
        items.SetThumbnailImage(item.GetThumbnailImage());
      }
      else if (node == NODE_TYPE_TITLE_MOVIES || node == NODE_TYPE_RECENTLY_ADDED_MOVIES)
        g_infoManager.m_content = "movies";
      else if (node == NODE_TYPE_TITLE_TVSHOWS)
        g_infoManager.m_content = "tvshows";
      else if (node == NODE_TYPE_TITLE_MUSICVIDEOS || node == NODE_TYPE_RECENTLY_ADDED_MUSICVIDEOS)
        g_infoManager.m_content = "musicvideos";
      else
        g_infoManager.m_content = "";
    }
  }

  return bResult;
}

void CGUIWindowVideoNav::UpdateButtons()
{
  CGUIWindowVideoBase::UpdateButtons();

  // Update object count
  int iItems = m_vecItems.Size();
  if (iItems)
  {
    // check for parent dir and "all" items
    // should always be the first two items
    for (int i = 0; i <= (iItems>=2 ? 1 : 0); i++)
    {
      CFileItem* pItem = m_vecItems[i];
      if (pItem->IsParentFolder()) iItems--;
      if (pItem->m_strPath.Left(4).Equals("/-1/")) iItems--;
    }
    // or the last item
    if (m_vecItems.Size() > 2 &&
      m_vecItems[m_vecItems.Size()-1]->m_strPath.Left(4).Equals("/-1/"))
      iItems--;
  }
  CStdString items;
  items.Format("%i %s", iItems, g_localizeStrings.Get(127).c_str());
  SET_CONTROL_LABEL(CONTROL_LABELFILES, items);

  // set the filter label
  CStdString strLabel;

  // "Playlists"
  if (m_vecItems.m_strPath.Equals("special://videoplaylists/"))
    strLabel = g_localizeStrings.Get(136);
  // "{Playlist Name}"
  else if (m_vecItems.IsPlayList())
  {
    // get playlist name from path
    CStdString strDummy;
    CUtil::Split(m_vecItems.m_strPath, strDummy, strLabel);
  }
  // everything else is from a videodb:// path
  else
  {
    CVideoDatabaseDirectory dir;
    dir.GetLabel(m_vecItems.m_strPath, strLabel);
  }

  SET_CONTROL_LABEL(CONTROL_FILTER, strLabel);

  SET_CONTROL_LABEL(CONTROL_BTNSHOWMODE, g_localizeStrings.Get(16100 + g_stSettings.m_iMyVideoWatchMode));

  SET_CONTROL_SELECTED(GetID(),CONTROL_BTNSHOWALL,g_stSettings.m_iMyVideoWatchMode != VIDEO_SHOW_ALL);

  SET_CONTROL_SELECTED(GetID(),CONTROL_BTN_FILTER, !m_filter.IsEmpty());

  SET_CONTROL_SELECTED(GetID(),CONTROL_BTNPARTYMODE, g_partyModeManager.IsEnabled());
}

/// \brief Search for genres, artists and albums with search string \e strSearch in the musicdatabase and return the found \e items
/// \param strSearch The search string
/// \param items Items Found
void CGUIWindowVideoNav::DoSearch(const CStdString& strSearch, CFileItemList& items)
{
  // get matching genres
  CFileItemList tempItems;

  m_database.GetMovieGenresByName(strSearch, tempItems);
  if (tempItems.Size())
  {
    CStdString strGenre = g_localizeStrings.Get(515); // Genre
    for (int i = 0; i < (int)tempItems.Size(); i++)
    {
      tempItems[i]->SetLabel("[" + strGenre + " - "+g_localizeStrings.Get(20342)+"] " + tempItems[i]->GetLabel());
    }
    items.Append(tempItems);
  }
  
  tempItems.Clear();
  m_database.GetTvShowGenresByName(strSearch, tempItems);
  if (tempItems.Size())
  {
    CStdString strGenre = g_localizeStrings.Get(515); // Genre
    for (int i = 0; i < (int)tempItems.Size(); i++)
    {
      tempItems[i]->SetLabel("[" + strGenre + " - "+g_localizeStrings.Get(20343)+"] " + tempItems[i]->GetLabel());
    }
    items.Append(tempItems);
  }

  tempItems.Clear();
  m_database.GetMovieActorsByName(strSearch, tempItems);
  if (tempItems.Size())
  {
    CStdString strActor = g_localizeStrings.Get(20337); // Actor
    for (int i = 0; i < (int)tempItems.Size(); i++)
    {
      tempItems[i]->SetLabel("[" + strActor + " - "+g_localizeStrings.Get(20342)+"] " + tempItems[i]->GetLabel());
    }
    items.Append(tempItems);
  }

  tempItems.Clear();
  m_database.GetTvShowsActorsByName(strSearch, tempItems);
  if (tempItems.Size())
  {
    CStdString strActor = g_localizeStrings.Get(20337); // Actor
    for (int i = 0; i < (int)tempItems.Size(); i++)
    {
      tempItems[i]->SetLabel("[" + strActor + " - "+g_localizeStrings.Get(20343)+"] " + tempItems[i]->GetLabel());
    }
    items.Append(tempItems);
  }

  tempItems.Clear();
  m_database.GetMusicVideoArtistsByName(strSearch, tempItems);
  if (tempItems.Size())
  {
    CStdString strActor = g_localizeStrings.Get(484); // Actor
    for (int i = 0; i < (int)tempItems.Size(); i++)
    {
      tempItems[i]->SetLabel("[" + strActor + " - "+g_localizeStrings.Get(20389)+"] " + tempItems[i]->GetLabel());
    }
    items.Append(tempItems);
  }

  tempItems.Clear();
  m_database.GetMovieDirectorsByName(strSearch, tempItems);
  if (tempItems.Size())
  {
    CStdString strMovie = g_localizeStrings.Get(20339); // Director
    for (int i = 0; i < (int)tempItems.Size(); i++)
    {
      tempItems[i]->SetLabel("[" + strMovie + " - "+g_localizeStrings.Get(20342)+"] " + tempItems[i]->GetLabel());
    }
    items.Append(tempItems);
  }

  tempItems.Clear();
  m_database.GetTvShowsDirectorsByName(strSearch, tempItems);
  if (tempItems.Size())
  {
    CStdString strMovie = g_localizeStrings.Get(20339); // Director
    for (int i = 0; i < (int)tempItems.Size(); i++)
    {
      tempItems[i]->SetLabel("[" + strMovie + " - "+g_localizeStrings.Get(20343)+"] " + tempItems[i]->GetLabel());
    }
    items.Append(tempItems);
  }

  tempItems.Clear();
  m_database.GetMusicVideoDirectorsByName(strSearch, tempItems);
  if (tempItems.Size())
  {
    CStdString strMovie = g_localizeStrings.Get(20339); // Director
    for (int i = 0; i < (int)tempItems.Size(); i++)
    {
      tempItems[i]->SetLabel("[" + strMovie + " - "+g_localizeStrings.Get(20389)+"] " + tempItems[i]->GetLabel());
    }
    items.Append(tempItems);
  }

  tempItems.Clear();
  m_database.GetMoviesByName(strSearch, tempItems);

  if (tempItems.Size())
  {
    for (int i = 0; i < (int)tempItems.Size(); i++)
    {
      tempItems[i]->SetLabel("[" + g_localizeStrings.Get(20338) + "] " + tempItems[i]->GetLabel());
    }
    items.Append(tempItems);
  }

  tempItems.Clear();
  m_database.GetEpisodesByName(strSearch, tempItems);

  if (tempItems.Size())
  {
    for (int i = 0; i < (int)tempItems.Size(); i++)
    {
      tempItems[i]->SetLabel("[" + g_localizeStrings.Get(20359) + "] " + tempItems[i]->GetLabel());
    }
    items.Append(tempItems);
  }

  tempItems.Clear();
  m_database.GetMusicVideosByName(strSearch, tempItems);

  if (tempItems.Size())
  {
    for (int i = 0; i < (int)tempItems.Size(); i++)
    {
      tempItems[i]->SetLabel("[" + g_localizeStrings.Get(20391) + "] " + tempItems[i]->GetLabel());
    }
    items.Append(tempItems);
  }

  tempItems.Clear();
  m_database.GetMusicVideosByAlbum(strSearch, tempItems);

  if (tempItems.Size())
  {
    for (int i = 0; i < (int)tempItems.Size(); i++)
    {
      tempItems[i]->SetLabel("[" + g_localizeStrings.Get(483) + "] " + tempItems[i]->GetLabel());
    }
    items.Append(tempItems);
  }

  tempItems.Clear();
  m_database.GetEpisodesByPlot(strSearch, tempItems);

  if (tempItems.Size())
  {
    for (int i = 0; i < (int)tempItems.Size(); i++)
    {
      tempItems[i]->SetLabel("[" + g_localizeStrings.Get(20365) + "] " + tempItems[i]->GetLabel());
    }
    items.Append(tempItems);
  }
}

void CGUIWindowVideoNav::PlayItem(int iItem)
{
  // unlike additemtoplaylist, we need to check the items here
  // before calling it since the current playlist will be stopped
  // and cleared!

  // root is not allowed
  if (m_vecItems.IsVirtualDirectoryRoot())
    return;

  CGUIWindowVideoBase::PlayItem(iItem);
}

void CGUIWindowVideoNav::OnWindowLoaded()
{
#ifdef PRE_SKIN_VERSION_2_1_COMPATIBILITY
  const CGUIControl *pList = GetControl(CONTROL_LIST);
  if (pList && !GetControl(CONTROL_LABELEMPTY))
  {
    CLabelInfo info;
    info.align = XBFONT_CENTER_X | XBFONT_CENTER_Y;
    info.font = g_fontManager.GetFont("font13");
    info.textColor = 0xffffffff;
    CGUILabelControl *pLabel = new CGUILabelControl(GetID(),CONTROL_LABELEMPTY,pList->GetXPosition(),pList->GetYPosition(),pList->GetWidth(),pList->GetHeight(),"",info,false);
    pLabel->SetAnimations(pList->GetAnimations());
    Add(pLabel);
  }
#endif
  CGUIWindowVideoBase::OnWindowLoaded();
}

void CGUIWindowVideoNav::DisplayEmptyDatabaseMessage(bool bDisplay)
{
  m_bDisplayEmptyDatabaseMessage = bDisplay;
}

void CGUIWindowVideoNav::Render()
{
  if (m_bDisplayEmptyDatabaseMessage)
  {
    SET_CONTROL_LABEL(CONTROL_LABELEMPTY,g_localizeStrings.Get(745)+'\n'+g_localizeStrings.Get(746))
  }
  else
  {
    SET_CONTROL_LABEL(CONTROL_LABELEMPTY,"")
  }
  CGUIWindowVideoBase::Render();
}

void CGUIWindowVideoNav::OnInfo(CFileItem* pItem, const SScraperInfo& info)
{
  SScraperInfo info2(info);
  CStdString strPath,strFile;
  m_database.Open(); // since we can be called from the music library without being inited
  if (pItem->IsVideoDb())
    m_database.GetScraperForPath(pItem->GetVideoInfoTag()->m_strPath,info2);
  else
  {
    CUtil::Split(pItem->m_strPath,strPath,strFile);
    m_database.GetScraperForPath(strPath,info2);
  }
  m_database.Close();
  CGUIWindowVideoBase::OnInfo(pItem,info2);
}

void CGUIWindowVideoNav::OnDeleteItem(int iItem)
{
  if (iItem < 0 || iItem >= (int)m_vecItems.Size()) return;

  if (m_vecItems.m_strPath.Equals("special://videoplaylists/"))
  {
    CGUIWindowVideoBase::OnDeleteItem(iItem);
    return;
  }

  CFileItem* pItem = m_vecItems[iItem];

  int iType=0;
  if (pItem->HasVideoInfoTag() && !pItem->GetVideoInfoTag()->m_strShowTitle.IsEmpty()) // tvshow
    iType = 2;
  if (pItem->HasVideoInfoTag() && pItem->GetVideoInfoTag()->m_iSeason > -1 && !pItem->m_bIsFolder) // episode
    iType = 1;
  if (pItem->HasVideoInfoTag() && pItem->GetVideoInfoTag()->m_artist.size() > 0)
    iType = 3;

  CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*)m_gWindowManager.GetWindow(WINDOW_DIALOG_YES_NO);
  if (!pDialog) return;
  if (iType == 0)
    pDialog->SetHeading(432);
  if (iType == 1)
    pDialog->SetHeading(20362);
  if (iType == 2)
    pDialog->SetHeading(20363);
  if (iType == 3)
    pDialog->SetHeading(20392);
  CStdString strLine;
  strLine.Format(g_localizeStrings.Get(433),pItem->GetLabel());
  pDialog->SetLine(0, strLine);
  pDialog->SetLine(1, "");
  pDialog->SetLine(2, "");;
  pDialog->DoModal();
  if (!pDialog->IsConfirmed()) return;

  CStdString path;
  m_database.GetFilePath(pItem->GetVideoInfoTag()->m_iDbId, path, iType);
  if (path.IsEmpty()) return;
  if (iType == 0)
    m_database.DeleteMovie(path);
  if (iType == 1)
    m_database.DeleteEpisode(path);
  if (iType == 2)
    m_database.DeleteTvShow(path);
  if (iType == 3)
    m_database.DeleteMusicVideo(path);

  if (iType == 2)
    m_database.SetPathHash(path,"");  
  else
  {
    CStdString strDirectory;
    CUtil::GetDirectory(path,strDirectory);
    m_database.SetPathHash(strDirectory,"");
  }

  // delete the cached thumb for this item (it will regenerate if it is a user thumb)
  CStdString thumb(pItem->GetCachedVideoThumb());
  CFile::Delete(thumb);

  CUtil::DeleteVideoDatabaseDirectoryCache();

  DisplayEmptyDatabaseMessage(m_database.GetMovieCount() <= 0 && m_database.GetTvShowCount() <= 0 && m_database.GetMusicVideoCount() <= 0);
  Update( m_vecItems.m_strPath );
  m_viewControl.SetSelectedItem(iItem);
  return;
}

void CGUIWindowVideoNav::OnFinalizeFileItems(CFileItemList& items)
{
  int iItem=0;
  m_unfilteredItems.AppendPointer(items);
  // now filter as necessary
  CVideoDatabaseDirectory dir;
  CQueryParams params;
  dir.GetQueryParams(items.m_strPath,params);
  bool filterWatched=false;
  if (params.GetContentType() == VIDEODB_CONTENT_TVSHOWS && dir.GetDirectoryChildType(items.m_strPath) == NODE_TYPE_EPISODES)
    filterWatched = true;
  if (g_stSettings.m_iMyVideoWatchMode == VIDEO_SHOW_ALL)
    filterWatched = false;
  if (params.GetContentType() == VIDEODB_CONTENT_MOVIES || params.GetContentType() == VIDEODB_CONTENT_MUSICVIDEOS) // need to filter no matter to get rid of duplicates - price to pay for not filtering in db
    filterWatched = true;

  if (filterWatched || !m_filter.IsEmpty())
    FilterItems(items);
}

void CGUIWindowVideoNav::ClearFileItems()
{
  m_viewControl.Clear();
  m_vecItems.ClearKeepPointer();
  m_unfilteredItems.Clear();
}

void CGUIWindowVideoNav::OnFilterItems()
{
  CStdString currentItem;
  int item = m_viewControl.GetSelectedItem();
  if (item >= 0)
    currentItem = m_vecItems[item]->m_strPath;

  m_viewControl.Clear();

  FilterItems(m_vecItems);

  // and update our view control + buttons
  m_viewControl.SetItems(m_vecItems);
  m_viewControl.SetSelectedItem(currentItem);
  UpdateButtons();
}

void CGUIWindowVideoNav::FilterItems(CFileItemList &items)
{
  CVideoDatabaseDirectory dir;
  CQueryParams params;
  dir.GetQueryParams(items.m_strPath,params);
  bool filterWatched=false;
  if (params.GetContentType() == VIDEODB_CONTENT_TVSHOWS && dir.GetDirectoryChildType(items.m_strPath) == NODE_TYPE_EPISODES)
    filterWatched = true;
  if (params.GetContentType() == VIDEODB_CONTENT_MOVIES || params.GetContentType() == VIDEODB_CONTENT_MUSICVIDEOS)
    filterWatched = true;
  if (g_stSettings.m_iMyVideoWatchMode == VIDEO_SHOW_ALL)
    filterWatched = false;
  
  NODE_TYPE node = dir.GetDirectoryChildType(items.m_strPath);
  if (m_vecItems.IsVirtualDirectoryRoot() || node == NODE_TYPE_MOVIES_OVERVIEW || node == NODE_TYPE_TVSHOWS_OVERVIEW || node == NODE_TYPE_MUSICVIDEOS_OVERVIEW)
    return;

  items.ClearKeepPointer();
  for (int i = 0; i < m_unfilteredItems.Size(); i++)
  {
    CFileItem *item = m_unfilteredItems[i];
    if (item->IsParentFolder() || CVideoDatabaseDirectory::IsAllItem(item->m_strPath) ||
        (m_filter.IsEmpty() && (!filterWatched || item->GetVideoInfoTag()->m_bWatched == (g_stSettings.m_iMyVideoWatchMode==2))))
    {
      if ((params.GetContentType() != VIDEODB_CONTENT_MOVIES  && params.GetContentType() != VIDEODB_CONTENT_MUSICVIDEOS) || !items.Contains(item->m_strPath))
        items.Add(item);
      continue;
    }
    // TODO: Need to update this to get all labels, ideally out of the displayed info (ie from m_layout and m_focusedLayout)
    // though that isn't practical.  Perhaps a better idea would be to just grab the info that we should filter on based on
    // where we are in the library tree.
    // Another idea is tying the filter string to the current level of the tree, so that going deeper disables the filter,
    // but it's re-enabled on the way back out.
    CStdString match;
/*    if (item->GetFocusedLayout())
      match = item->GetFocusedLayout()->GetAllText();
    else if (item->GetLayout())
      match = item->GetLayout()->GetAllText();
    else*/
      match = item->GetLabel() + " " + item->GetLabel2();
    if (StringUtils::FindWords(match.c_str(), m_filter.c_str()) && 
        (!filterWatched || item->GetVideoInfoTag()->m_bWatched == (g_stSettings.m_iMyVideoWatchMode==2)))
    {
      if ((params.GetContentType() != VIDEODB_CONTENT_MOVIES && params.GetContentType() != VIDEODB_CONTENT_MUSICVIDEOS) || !items.Contains(item->m_strPath))
        items.Add(item);
    }
  }
}

void CGUIWindowVideoNav::GetContextButtons(int itemNumber, CContextButtons &buttons)
{
  CFileItem *item = (itemNumber >= 0 && itemNumber < m_vecItems.Size()) ? m_vecItems[itemNumber] : NULL;

  CGUIWindowVideoBase::GetContextButtons(itemNumber, buttons);

  CVideoDatabaseDirectory dir;
  NODE_TYPE node = dir.GetDirectoryChildType(m_vecItems.m_strPath);

  if (item)
  {
    SScraperInfo info;
    VIDEO::SScanSettings settings;
    int iFound = GetScraperForItem(item, info, settings);
    
    if (info.strContent.Equals("tvshows"))
      buttons.Add(CONTEXT_BUTTON_INFO, item->m_bIsFolder ? 20351 : 20352);
    else if (info.strContent.Equals("musicvideos"))
      buttons.Add(CONTEXT_BUTTON_INFO,20393);
    else if (!item->m_bIsFolder && !item->m_strPath.Left(19).Equals("newsmartplaylist://"))
      buttons.Add(CONTEXT_BUTTON_INFO, 13346);

    if (item->HasVideoInfoTag() && item->GetVideoInfoTag()->GetArtist().size() > 0)
    {
      CMusicDatabase database;
      database.Open();
      if (database.GetArtistByName(item->GetVideoInfoTag()->GetArtist()) > -1)
        buttons.Add(CONTEXT_BUTTON_GO_TO_ARTIST, 20396);
    }
    if (item->HasVideoInfoTag() && item->GetVideoInfoTag()->m_strAlbum.size() > 0)
    {
      CMusicDatabase database;
      database.Open();
      if (database.GetAlbumByName(item->GetVideoInfoTag()->m_strAlbum) > -1)
        buttons.Add(CONTEXT_BUTTON_GO_TO_ALBUM, 20397);
    }
    if (item->HasVideoInfoTag() && item->GetVideoInfoTag()->m_strAlbum.size() > 0 && item->GetVideoInfoTag()->GetArtist().size() > 0 && item->GetVideoInfoTag()->m_strTitle.size() > 0)
    {
      CMusicDatabase database;
      database.Open();
      if (database.GetSongByArtistAndAlbumAndTitle(item->GetVideoInfoTag()->GetArtist(),item->GetVideoInfoTag()->m_strAlbum,item->GetVideoInfoTag()->m_strTitle) > -1)
        buttons.Add(CONTEXT_BUTTON_PLAY_OTHER, 20398);
    }
    if (!item->IsParentFolder())
    {
      // can we update the database?
      if (g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].canWriteDatabases() || g_passwordManager.bMasterUser)
      {
        if (node == NODE_TYPE_TITLE_TVSHOWS)
        {
          CGUIDialogVideoScan *pScanDlg = (CGUIDialogVideoScan *)m_gWindowManager.GetWindow(WINDOW_DIALOG_VIDEO_SCAN);
          if (pScanDlg && pScanDlg->IsScanning())
            buttons.Add(CONTEXT_BUTTON_STOP_SCANNING, 13353);
          else
            buttons.Add(CONTEXT_BUTTON_UPDATE_TVSHOW, 13349);

          buttons.Add(CONTEXT_BUTTON_EDIT, 16105);
        }
        else if (item->HasVideoInfoTag() && !item->m_bIsFolder)
        {
          if (item->GetVideoInfoTag()->m_bWatched)
            buttons.Add(CONTEXT_BUTTON_MARK_UNWATCHED, 16104); //Mark as UnWatched
          else
            buttons.Add(CONTEXT_BUTTON_MARK_WATCHED, 16103);   //Mark as Watched
          buttons.Add(CONTEXT_BUTTON_EDIT, 16105); //Edit Title
        }
        if (m_database.GetTvShowCount() > 0 && item->HasVideoInfoTag() && !item->m_bIsFolder && item->GetVideoInfoTag()->m_iEpisode > -1 && item->GetVideoInfoTag()->m_artist.size() == 0) // movie entry
        {
          if (m_database.IsLinkedToTvshow(item->GetVideoInfoTag()->m_iDbId))
            buttons.Add(CONTEXT_BUTTON_UNLINK_MOVIE,20385);
          else
            buttons.Add(CONTEXT_BUTTON_LINK_MOVIE,20384);
        }

        if (dir.GetDirectoryChildType(m_vecItems.m_strPath) == NODE_TYPE_SEASONS && !dir.IsAllItem(item->m_strPath) && item->m_bIsFolder)
          buttons.Add(CONTEXT_BUTTON_SET_SEASON_THUMB, 20371);
        
        if (item->HasVideoInfoTag() && (!item->m_bIsFolder || node == NODE_TYPE_TITLE_TVSHOWS))
          buttons.Add(CONTEXT_BUTTON_DELETE, 646);

        // this should ideally be non-contextual (though we need some context for non-tv show node I guess)
        CGUIDialogVideoScan *pScanDlg = (CGUIDialogVideoScan *)m_gWindowManager.GetWindow(WINDOW_DIALOG_VIDEO_SCAN);
        if (pScanDlg && pScanDlg->IsScanning())
        {
          if (node != NODE_TYPE_TITLE_TVSHOWS)
            buttons.Add(CONTEXT_BUTTON_STOP_SCANNING, 13353);
        }
        else
          buttons.Add(CONTEXT_BUTTON_UPDATE_LIBRARY, 653);
      }

      //Set default and/or clear default
      CVideoDatabaseDirectory dir;
      NODE_TYPE nodetype = dir.GetDirectoryType(item->m_strPath);
      if (!item->IsParentFolder() && !m_vecItems.m_strPath.Equals("special://videoplaylists/") &&
        (nodetype == NODE_TYPE_ROOT || nodetype == NODE_TYPE_OVERVIEW || nodetype == NODE_TYPE_TVSHOWS_OVERVIEW || nodetype == NODE_TYPE_MOVIES_OVERVIEW || nodetype == NODE_TYPE_MUSICVIDEOS_OVERVIEW))
      {
        if (!item->m_strPath.Equals(g_settings.m_defaultVideoLibSource))
          buttons.Add(CONTEXT_BUTTON_SET_DEFAULT, 13335); // set default
        if (strcmp(g_settings.m_defaultVideoLibSource, ""))
          buttons.Add(CONTEXT_BUTTON_CLEAR_DEFAULT, 13403); // clear default
      }

      if (m_vecItems.m_strPath.Equals("special://videoplaylists/"))
      { // video playlists, file operations are allowed
        if (!item->IsReadOnly())
        {
          buttons.Add(CONTEXT_BUTTON_DELETE, 117);
          buttons.Add(CONTEXT_BUTTON_RENAME, 118);
        }
      }
    }
  }
  CGUIWindowVideoBase::GetNonContextButtons(itemNumber, buttons);
}

bool CGUIWindowVideoNav::OnContextButton(int itemNumber, CONTEXT_BUTTON button)
{
  switch (button)
  {
  case CONTEXT_BUTTON_SET_DEFAULT:
    g_settings.m_defaultVideoLibSource = GetQuickpathName(m_vecItems[itemNumber]->m_strPath);
    g_settings.Save();
    return true;

  case CONTEXT_BUTTON_CLEAR_DEFAULT:
    g_settings.m_defaultVideoLibSource.Empty();
    g_settings.Save();
    return true;

  case CONTEXT_BUTTON_MARK_WATCHED:
    MarkWatched(itemNumber);
    return true;

  case CONTEXT_BUTTON_MARK_UNWATCHED:
    MarkUnWatched(itemNumber);
    return true;

  case CONTEXT_BUTTON_EDIT:
    UpdateVideoTitle(itemNumber);
    return true;

  case CONTEXT_BUTTON_SET_SEASON_THUMB:
    {
      // Grab the thumbnails from the web
      CStdString strPath;
      CFileItemList items;
      CUtil::AddFileToFolder(g_advancedSettings.m_cachePath,"imdbthumbs",strPath);
      CUtil::WipeDir(strPath);
      DIRECTORY::CDirectory::Create(strPath);
      int i=1;
      CVideoInfoTag tag;
      m_database.GetTvShowInfo("",tag,m_vecItems[itemNumber]->GetVideoInfoTag()->m_iDbId);
      for (std::vector<CScraperUrl::SUrlEntry>::iterator iter=tag.m_strPictureURL.m_url.begin();iter != tag.m_strPictureURL.m_url.end();++iter)
      {
        if (iter->m_type != CScraperUrl::URL_TYPE_SEASON || iter->m_season != m_vecItems[itemNumber]->GetVideoInfoTag()->m_iSeason)
          continue;
        CStdString thumbFromWeb;
        CStdString strLabel;
        strLabel.Format("imdbthumb%i.jpg",i);
        CUtil::AddFileToFolder(strPath, strLabel, thumbFromWeb);
        if (VIDEO::CVideoInfoScanner::DownloadThumbnail(thumbFromWeb,*iter))
        {
          CStdString strItemPath;
          strItemPath.Format("thumb://IMDb%i",i++);
          CFileItem *item = new CFileItem(strItemPath, false);
          item->SetThumbnailImage(thumbFromWeb);
          CStdString strLabel;
          item->SetLabel(g_localizeStrings.Get(20015));
          items.Add(item);
        }
      }
      if (CFile::Exists(m_vecItems[itemNumber]->GetCachedSeasonThumb()))
      {
        CFileItem *item = new CFileItem("thumb://Current", false);
        item->SetThumbnailImage(m_vecItems[itemNumber]->GetCachedSeasonThumb());
        item->SetLabel(g_localizeStrings.Get(20016));
        items.Add(item);
      }

      CFileItem *item = new CFileItem("thumb://None", false);
      item->SetThumbnailImage("defaultFolderBig.png");
      item->SetLabel(g_localizeStrings.Get(20018));
      items.Add(item);

      CStdString result;
      if (!CGUIDialogFileBrowser::ShowAndGetImage(items, g_settings.m_videoSources, g_localizeStrings.Get(20019), result))
        return false;   // user cancelled

      if (result == "thumb://Current")
        return false;   // user chose the one they have

      // delete the thumbnail if that's what the user wants, else overwrite with the
      // new thumbnail
      CStdString cachedThumb(m_vecItems[itemNumber]->GetCachedSeasonThumb());

      if (result.Mid(0,12) == "thumb://IMDb")
      {
        CStdString strFile;
        CUtil::AddFileToFolder(strPath,"imdbthumb"+result.Mid(12)+".jpg",strFile);
        if (CFile::Exists(strFile))
          CFile::Cache(strFile, cachedThumb);
        else
          result = "thumb://None";
      }
      else if (result == "thumb://None")
        CFile::Delete(m_vecItems[itemNumber]->GetCachedSeasonThumb());
      else
        CFile::Cache(result,cachedThumb);

      CUtil::DeleteVideoDatabaseDirectoryCache();
      Update(m_vecItems.m_strPath);

      return true;
    }
  case CONTEXT_BUTTON_UPDATE_LIBRARY:
    {
      SScraperInfo info;
      VIDEO::SScanSettings settings;
      OnScan("",info,settings);
      return true;
    }
  case CONTEXT_BUTTON_UNLINK_MOVIE:
    {
      m_database.LinkMovieToTvshow(m_vecItems[itemNumber]->GetVideoInfoTag()->m_iDbId,-1);
      CUtil::DeleteVideoDatabaseDirectoryCache();
      return true;
    }
  case CONTEXT_BUTTON_LINK_MOVIE:
    {
      OnLinkMovieToTvShow(itemNumber);
      return true;
    }
  case CONTEXT_BUTTON_GO_TO_ARTIST:
    {
      CStdString strPath;
      CMusicDatabase database;
      database.Open();
      strPath.Format("musicdb://2/%ld/",database.GetArtistByName(m_vecItems[itemNumber]->GetVideoInfoTag()->GetArtist()));
      m_gWindowManager.ActivateWindow(WINDOW_MUSIC_NAV,strPath);
      return true;
    }
  case CONTEXT_BUTTON_GO_TO_ALBUM:
    {
      CStdString strPath;
      CMusicDatabase database;
      database.Open();
      strPath.Format("musicdb://3/%ld/",database.GetAlbumByName(m_vecItems[itemNumber]->GetVideoInfoTag()->m_strAlbum));
      m_gWindowManager.ActivateWindow(WINDOW_MUSIC_NAV,strPath);
      return true;
    }
  case CONTEXT_BUTTON_PLAY_OTHER:
    {
      CMusicDatabase database;
      database.Open();
      CSong song;
      if (database.GetSongById(database.GetSongByArtistAndAlbumAndTitle(m_vecItems[itemNumber]->GetVideoInfoTag()->GetArtist(),m_vecItems[itemNumber]->GetVideoInfoTag()->m_strAlbum,m_vecItems[itemNumber]->GetVideoInfoTag()->m_strTitle),song))
        g_applicationMessenger.PlayFile(song);
      return true;
    }

  }
  return CGUIWindowVideoBase::OnContextButton(itemNumber, button);
}

void CGUIWindowVideoNav::OnLinkMovieToTvShow(int itemnumber)
{
  CFileItemList list;
  m_database.GetTvShowsNav("videodb://2/2",list);
  list.Sort(SORT_METHOD_LABEL,SORT_ORDER_ASC);
  CGUIDialogSelect* pDialog = (CGUIDialogSelect*)m_gWindowManager.GetWindow(WINDOW_DIALOG_SELECT);
  pDialog->Reset();
  pDialog->SetItems(&list);
  pDialog->SetHeading(20356);
  pDialog->DoModal();
  if (pDialog->GetSelectedLabel() > -1)
  {
    m_database.LinkMovieToTvshow(m_vecItems[itemnumber]->GetVideoInfoTag()->m_iDbId,pDialog->GetSelectedItem().GetVideoInfoTag()->m_iDbId);
    CUtil::DeleteVideoDatabaseDirectoryCache();
  }
}
