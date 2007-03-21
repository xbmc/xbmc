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
#include "utils/GUIInfoManager.h"
#include "util.h"
#include "PlayListM3U.h"
#include "application.h"
#include "playlistplayer.h"
#include "GUIPassword.h"
#include "GUILabelControl.h"
#include "GUIDialogContextMenu.h"
#include "GUIDialogFileBrowser.h"
#include "Picture.h"
#include "FileSystem/VideoDatabaseDirectory.h"
#include "PlaylistFactory.h"
#include "GUIFontManager.h"

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

#define CONTROL_UNLOCK            11

#define CONTROL_FILTER            15
#define CONTROL_BTNMANUALINFO     17
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
      if (m_vecItems.m_strPath=="?")
        m_vecItems.m_strPath = "";

      // check for valid quickpath parameter
      CStdString strDestination = message.GetStringParam();
      if (!strDestination.IsEmpty())
      {
        message.SetStringParam("");
        CLog::Log(LOGINFO, "Attempting to quickpath to: %s", strDestination.c_str());

        if (strDestination.Equals("$ROOT") || strDestination.Equals("Root"))
        {
          m_vecItems.m_strPath = "";
        }
        else if (strDestination.Equals("MovieGenres"))
        {
          m_vecItems.m_strPath = "videodb://1/1/";
          SetHistoryForPath(m_vecItems.m_strPath);
        }
        else if (strDestination.Equals("MovieTitles"))
        {
          m_vecItems.m_strPath = "videodb://1/2/";
          SetHistoryForPath(m_vecItems.m_strPath);
        }
        else if (strDestination.Equals("MovieYears"))
        {
          m_vecItems.m_strPath = "videodb://1/3/";
          SetHistoryForPath(m_vecItems.m_strPath);
        }
        else if (strDestination.Equals("MovieActors"))
        {
          m_vecItems.m_strPath = "videodb://1/4/";
          SetHistoryForPath(m_vecItems.m_strPath);
        }
        else if (strDestination.Equals("MovieDirectors"))
        {
          m_vecItems.m_strPath = "videodb://1/5/";
          SetHistoryForPath(m_vecItems.m_strPath);
        }
        else if (strDestination.Equals("Movies"))
        {
          m_vecItems.m_strPath = "videodb://1/";
          SetHistoryForPath(m_vecItems.m_strPath);          
        }
        else if (strDestination.Equals("TvShowGenres"))
        {
          m_vecItems.m_strPath = "videodb://2/1/";
          SetHistoryForPath(m_vecItems.m_strPath);
        }
        else if (strDestination.Equals("TvShowTitles"))
        {
          m_vecItems.m_strPath = "videodb://2/2/";
          SetHistoryForPath(m_vecItems.m_strPath);
        }
        else if (strDestination.Equals("TvShowYears"))
        {
          m_vecItems.m_strPath = "videodb://2/3/";
          SetHistoryForPath(m_vecItems.m_strPath);
        }
        else if (strDestination.Equals("TvShowActors"))
        {
          m_vecItems.m_strPath = "videodb://2/4/";
          SetHistoryForPath(m_vecItems.m_strPath);
        }
        else if (strDestination.Equals("TvShows"))
        {
          m_vecItems.m_strPath = "videodb://2/";
          SetHistoryForPath(m_vecItems.m_strPath);          
        }
        else if (strDestination.Equals("Playlists"))
        {
          m_vecItems.m_strPath = "special://videoplaylists/";
          SetHistoryForPath(m_vecItems.m_strPath);
        }
        else
        {
          CLog::Log(LOGERROR, "  Failed! Destination parameter (%s) is not valid!", strDestination.c_str());
          break;
        }
      }
      
      DisplayEmptyDatabaseMessage(false); // reset message state

      if (!CGUIWindowVideoBase::OnMessage(message))
        return false;

      //  base class has opened the database, do our check
      m_database.Open();
      DisplayEmptyDatabaseMessage(m_database.GetMovieCount() <= 0 && m_database.GetTvShowCount() <= 0);

      if (m_bDisplayEmptyDatabaseMessage)
      {
        SET_CONTROL_FOCUS(CONTROL_BTNTYPE, 0);
        Update(m_vecItems.m_strPath);  // Will remove content from the list/thumb control
      }

      return true;
    }
    break;

  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == CONTROL_BTNSEARCH)
      {
        OnSearch();
      }
    }
    break;
  }
  return CGUIWindowVideoBase::OnMessage(message);
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
      dir.GetQueryParams(strDirectory,params);
      VIDEODATABASEDIRECTORY::NODE_TYPE node = dir.GetDirectoryChildType(strDirectory);
      
      items.SetThumbnailImage("");
      if (node == VIDEODATABASEDIRECTORY::NODE_TYPE_EPISODES || node == NODE_TYPE_SEASONS)
      {
        if (node == NODE_TYPE_EPISODES)
          g_infoManager.m_content = "episodes";
        else
          g_infoManager.m_content = "seasons";

        CFileItem item;
        m_database.GetFilePath(params.GetTvShowId(),item.m_strPath,2);
        item.SetVideoThumb();
        items.SetThumbnailImage(item.GetThumbnailImage());
      }
      else if (node == NODE_TYPE_TITLE_MOVIES)
        g_infoManager.m_content = "movies";
      else if (node == NODE_TYPE_TITLE_TVSHOWS)
        g_infoManager.m_content = "tvshows";
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

void CGUIWindowVideoNav::OnInfo(int iItem, const SScraperInfo& info)
{
  SScraperInfo info2(info);
  CStdString strPath,strFile;
  if (m_vecItems[iItem]->IsVideoDb())
  {
    m_database.GetScraperForPath(m_vecItems[iItem]->GetVideoInfoTag()->m_strPath,info2.strPath,info2.strContent);
  }
  else
  {
    CUtil::Split(m_vecItems[iItem]->m_strPath,strPath,strFile);
    m_database.GetScraperForPath(strPath,info2.strPath,info2.strContent);
  }
  CGUIWindowVideoBase::OnInfo(iItem,info2);
}

void CGUIWindowVideoNav::OnDeleteItem(int iItem)
{
  if (iItem < 0 || iItem >= (int)m_vecItems.Size()) return;

  CFileItem* pItem = m_vecItems[iItem];

  int iType=0;
  if (pItem->HasVideoInfoTag() && !pItem->GetVideoInfoTag()->m_strShowTitle.IsEmpty()) // tvshow
    iType = 2;
  if (pItem->HasVideoInfoTag() && pItem->GetVideoInfoTag()->m_iSeason > 0) // episode
    iType = 1;

  CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*)m_gWindowManager.GetWindow(WINDOW_DIALOG_YES_NO);
  if (!pDialog) return;
  if (iType == 0)
    pDialog->SetHeading(432);
  if (iType == 1)
    pDialog->SetHeading(20362);
  if (iType == 2)
    pDialog->SetHeading(20363);
  CStdString strLine;
  strLine.Format(g_localizeStrings.Get(433),pItem->GetLabel());
  pDialog->SetLine(0, strLine);
  pDialog->SetLine(1, "");
  pDialog->SetLine(2, "");;
  pDialog->DoModal();
  if (!pDialog->IsConfirmed()) return;

  CStdString path;
  m_database.GetFilePath(atol(pItem->GetVideoInfoTag()->m_strSearchString), path, iType);
  if (path.IsEmpty()) return;
  if (iType == 0)
  {
    m_database.DeleteMovie(path);
  }
  if (iType == 1)
  {
    m_database.DeleteEpisode(path);
  }
  if (iType == 2)
  {
    m_database.DeleteTvShow(path);
  }

  // delete the cached thumb for this item (it will regenerate if it is a user thumb)
  CStdString thumb(pItem->GetCachedVideoThumb());
  CFile::Delete(thumb);

  CUtil::DeleteVideoDatabaseDirectoryCache();

  DisplayEmptyDatabaseMessage(m_database.GetMovieCount() <= 0 && m_database.GetTvShowCount() <= 0);
  Update( m_vecItems.m_strPath );
  m_viewControl.SetSelectedItem(iItem);
  return;
}

void CGUIWindowVideoNav::OnFinalizeFileItems(CFileItemList& items)
{
  int iItem=0;
  CVideoDatabaseDirectory dir;
  if ((dir.GetDirectoryChildType(items.m_strPath) == NODE_TYPE_TITLE_MOVIES || dir.GetDirectoryChildType(items.m_strPath) == NODE_TYPE_EPISODES) && g_stSettings.m_iMyVideoWatchMode != VIDEO_SHOW_ALL)
  {
    while (iItem < items.Size())
    {
      if (items[iItem]->GetVideoInfoTag()->m_bWatched != (g_stSettings.m_iMyVideoWatchMode==2))
        items.Remove(iItem);
      else
        iItem++;
    }
  }
}
