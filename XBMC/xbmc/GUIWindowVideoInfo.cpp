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
#include "GUIWindow.h"
#include "GUIWindowVideoInfo.h"
#include "Util.h"
#include "Picture.h"
#include "GUIImage.h"
#include "StringUtils.h"
#include "GUIWindowVideoBase.h"
#include "GUIWindowVideoFiles.h"
#include "GUIDialogFileBrowser.h"
#include "utils/GUIInfoManager.h"
#include "VideoInfoScanner.h"
#include "Application.h"
#include "VideoInfoTag.h"
#include "GUIWindowManager.h"
#include "GUIDialogOK.h"
#include "GUIDialogYesNo.h"
#include "GUIDialogSelect.h"
#include "GUIDialogProgress.h"
#include "FileSystem/Directory.h"
#include "FileSystem/File.h"
#include "FileItem.h"
#include "MediaManager.h"
#include "utils/AsyncFileCopy.h"

using namespace std;
using namespace XFILE;

#define CONTROL_TITLE               20
#define CONTROL_DIRECTOR            21
#define CONTROL_CREDITS             22
#define CONTROL_GENRE               23
#define CONTROL_YEAR                24
#define CONTROL_TAGLINE             25
#define CONTROL_PLOTOUTLINE         26
#define CONTROL_CAST                29
#define CONTROL_RATING_AND_VOTES    30
#define CONTROL_RUNTIME             31
#define CONTROL_MPAARATING          32
#define CONTROL_TITLE_AND_YEAR      33
#define CONTROL_STUDIO              36
#define CONTROL_TOP250              37
#define CONTROL_TRAILER             38


#define CONTROL_IMAGE                3
#define CONTROL_TEXTAREA             4
#define CONTROL_BTN_TRACKS           5
#define CONTROL_BTN_REFRESH          6
#define CONTROL_DISC                 7
#define CONTROL_BTN_PLAY             8
#define CONTROL_BTN_RESUME           9
#define CONTROL_BTN_GET_THUMB       10
#define CONTROL_BTN_PLAY_TRAILER    11
#define CONTROL_BTN_GET_FANART      12

#define CONTROL_LIST                50

CGUIWindowVideoInfo::CGUIWindowVideoInfo(void)
    : CGUIDialog(WINDOW_VIDEO_INFO, "DialogVideoInfo.xml")
    , m_movieItem(new CFileItem)
{
  m_bRefreshAll = true;
  m_bRefresh = false;
  m_hasUpdatedThumb = false;
  m_castList = new CFileItemList;
}

CGUIWindowVideoInfo::~CGUIWindowVideoInfo(void)
{
  delete m_castList;
}

bool CGUIWindowVideoInfo::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_DEINIT:
    {
      ClearCastList();
    }
    break;

  case GUI_MSG_WINDOW_INIT:
    {
      m_dlgProgress = (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);

      m_bRefresh = false;
      m_bRefreshAll = true;
      m_hasUpdatedThumb = false;

      CGUIDialog::OnMessage(message);
      m_bViewReview = true;
      CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), CONTROL_DISC);
      OnMessage(msg);
      for (int i = 0; i < 1000; ++i)
      {
        CStdString strItem;
        strItem.Format("DVD#%03i", i);
        CGUIMessage msg2(GUI_MSG_LABEL_ADD, GetID(), CONTROL_DISC);
        msg2.SetLabel(strItem);
        OnMessage(msg2);
      }

      SET_CONTROL_HIDDEN(CONTROL_DISC);
      Refresh();

      CONTROL_ENABLE_ON_CONDITION(CONTROL_BTN_REFRESH, (g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].canWriteDatabases() || g_passwordManager.bMasterUser) && !m_movieItem->GetVideoInfoTag()->m_strIMDBNumber.Left(2).Equals("xx"));
      CONTROL_ENABLE_ON_CONDITION(CONTROL_BTN_GET_THUMB, (g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].canWriteDatabases() || g_passwordManager.bMasterUser) && !m_movieItem->GetVideoInfoTag()->m_strIMDBNumber.Mid(2).Equals("plugin"));

      VIDEODB_CONTENT_TYPE type = GetContentType(m_movieItem.get());
      if (type == VIDEODB_CONTENT_TVSHOWS || type == VIDEODB_CONTENT_MOVIES)
        CONTROL_ENABLE_ON_CONDITION(CONTROL_BTN_GET_FANART, (g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].canWriteDatabases() || g_passwordManager.bMasterUser) && !m_movieItem->GetVideoInfoTag()->m_strIMDBNumber.Mid(2).Equals("plugin"));
      else
        CONTROL_DISABLE(CONTROL_BTN_GET_FANART);

      return true;
    }
    break;


  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == CONTROL_BTN_REFRESH)
      {
        if (m_movieItem->GetVideoInfoTag()->m_iSeason < 0 && !m_movieItem->GetVideoInfoTag()->m_strShowTitle.IsEmpty()) // tv show
        {
          bool bCanceled=false;
          if (CGUIDialogYesNo::ShowAndGetInput(20377,20378,-1,-1,bCanceled))
          {
            m_bRefreshAll = true;
            CVideoDatabase db;
            if (db.Open())
            {
              db.SetPathHash(m_movieItem->GetVideoInfoTag()->m_strPath,"");
              db.Close();
            }
          }
          else
            m_bRefreshAll = false;

          if (bCanceled)
            return false;
        }
        m_bRefresh = true;
        Close();
        return true;
      }
      else if (iControl == CONTROL_BTN_TRACKS)
      {
        m_bViewReview = !m_bViewReview;
        Update();
      }
      else if (iControl == CONTROL_BTN_PLAY)
      {
        Play();
      }
      else if (iControl == CONTROL_BTN_RESUME)
      {
        Play(true);
      }
      else if (iControl == CONTROL_BTN_GET_THUMB)
      {
        OnGetThumb();
      }
      else if (iControl == CONTROL_BTN_PLAY_TRAILER)
      {
        PlayTrailer();
      }
      else if (iControl == CONTROL_BTN_GET_FANART)
      {
        OnGetFanart();
      }
/*      else if (iControl == CONTROL_DISC)
      {
        int iItem = 0;
        CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), iControl, 0, 0, NULL);
        OnMessage(msg);
        CStdString strItem = msg.GetLabel();
        if (strItem != "HD" && strItem != "share")
        {
          long lMovieId;
          sscanf(m_Movie.m_strSearchString.c_str(), "%i", &lMovieId);
          if (lMovieId > 0)
          {
            CStdString label;
            //m_database.GetDVDLabel(lMovieId, label);
            int iPos = label.Find("DVD#");
            if (iPos >= 0)
            {
              label.Delete(iPos, label.GetLength());
            }
            label = label.TrimRight(" ");
            label += " ";
            label += strItem;
            //m_database.SetDVDLabel( lMovieId, label);
          }
        }
      }*/
      else if (iControl == CONTROL_LIST)
      {
        int iAction = message.GetParam1();
        if (ACTION_SELECT_ITEM == iAction || ACTION_MOUSE_LEFT_CLICK == iAction)
        {
          CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), iControl);
          OnMessage(msg);
          int iItem = msg.GetParam1();
          if (iItem < 0 || iItem >= m_castList->Size())
            break;
          CStdString strItem = m_castList->Get(iItem)->GetLabel();
          CStdString strFind;
          strFind.Format(" %s ",g_localizeStrings.Get(20347));
          int iPos = strItem.Find(strFind);
          if (iPos == -1)
            iPos = strItem.size();
          CStdString tmp = strItem.Left(iPos);
          OnSearch(tmp);
        }
      }
    }
    break;
  }

  return CGUIDialog::OnMessage(message);
}

void CGUIWindowVideoInfo::SetMovie(const CFileItem *item)
{
  CVideoThumbLoader loader;
  *m_movieItem = *item;
  // setup cast list + determine type.  We need to do this here as it makes
  // sure that content type (among other things) is set correctly for the
  // old fixed id labels that we have floating around (they may be using
  // content type to determine visibility, so we'll set the wrong label)
  ClearCastList();
  VIDEODB_CONTENT_TYPE type = GetContentType(m_movieItem.get());
  if (type == VIDEODB_CONTENT_MUSICVIDEOS)
  { // music video
    CStdStringArray artists;
    StringUtils::SplitString(m_movieItem->GetVideoInfoTag()->m_strArtist, g_advancedSettings.m_videoItemSeparator, artists);
    for (std::vector<CStdString>::const_iterator it = artists.begin(); it != artists.end(); ++it)
    {
      CFileItemPtr item(new CFileItem(*it));
      if (CFile::Exists(item->GetCachedArtistThumb()))
        item->SetThumbnailImage(item->GetCachedArtistThumb());
      item->SetIconImage("DefaultArtist.png");
      m_castList->Add(item);
    }
    m_castList->SetContent("musicvideos");
  }
  else
  { // movie/show/episode
    for (CVideoInfoTag::iCast it = m_movieItem->GetVideoInfoTag()->m_cast.begin(); it != m_movieItem->GetVideoInfoTag()->m_cast.end(); ++it)
    {
      CStdString character;
      if (it->strRole.IsEmpty())
        character = it->strName;
      else
        character.Format("%s %s %s", it->strName.c_str(), g_localizeStrings.Get(20347).c_str(), it->strRole.c_str());
      CFileItemPtr item(new CFileItem(it->strName));
      if (CFile::Exists(item->GetCachedActorThumb()))
        item->SetThumbnailImage(item->GetCachedActorThumb());
      item->SetIconImage("DefaultActor.png");
      item->SetLabel(character);
      m_castList->Add(item);
    }
    // set fanart property for tvshows and movies
    if (type == VIDEODB_CONTENT_TVSHOWS || type == VIDEODB_CONTENT_MOVIES)
    {
      m_movieItem->CacheFanart();
      if (CFile::Exists(m_movieItem->GetCachedFanart()))
        m_movieItem->SetProperty("fanart_image",m_movieItem->GetCachedFanart());
    }
    // determine type:
    if (type == VIDEODB_CONTENT_TVSHOWS)
    {
      m_castList->SetContent("tvshows");
      // special case stuff for shows (not currently retrieved from the library in filemode (ref: GetTvShowInfo vs GetTVShowsByWhere)
      m_movieItem->m_dateTime.SetFromDateString(m_movieItem->GetVideoInfoTag()->m_strPremiered);
      m_movieItem->GetVideoInfoTag()->m_iYear = m_movieItem->m_dateTime.GetYear();
      m_movieItem->SetProperty("watchedepisodes", m_movieItem->GetVideoInfoTag()->m_playCount);
      m_movieItem->SetProperty("unwatchedepisodes", m_movieItem->GetVideoInfoTag()->m_iEpisode - m_movieItem->GetVideoInfoTag()->m_playCount);
      m_movieItem->GetVideoInfoTag()->m_playCount = (m_movieItem->GetVideoInfoTag()->m_iEpisode == m_movieItem->GetVideoInfoTag()->m_playCount) ? 1 : 0;
    }
    else if (type == VIDEODB_CONTENT_EPISODES)
    {
      m_castList->SetContent("episodes");
      // special case stuff for episodes (not currently retrieved from the library in filemode (ref: GetEpisodeInfo vs GetEpisodesByWhere)
      m_movieItem->m_dateTime.SetFromDateString(m_movieItem->GetVideoInfoTag()->m_strFirstAired);
      m_movieItem->GetVideoInfoTag()->m_iYear = m_movieItem->m_dateTime.GetYear();
      if (CFile::Exists(m_movieItem->GetCachedEpisodeThumb()))
        m_movieItem->SetThumbnailImage(m_movieItem->GetCachedEpisodeThumb());
      // retrieve the season thumb.
      // NOTE: This is overly complicated. Perhaps we should cache season thumbs by showtitle and season number,
      //       rather than bothering with show path and the localized strings involved?
      if (m_movieItem->GetVideoInfoTag()->m_iSeason > -1)
      {
        CStdString label;
        if (m_movieItem->GetVideoInfoTag()->m_iSeason == 0)
          label = g_localizeStrings.Get(20381);
        else
          label.Format(g_localizeStrings.Get(20358), m_movieItem->GetVideoInfoTag()->m_iSeason);
        CFileItem season(label);
        season.m_bIsFolder = true;
        // grab show path
        CVideoDatabase db;
        if (db.Open())
        {
          CFileItemList items;
          CStdString where = db.FormatSQL("where c%02d='%s'", VIDEODB_ID_TV_TITLE, m_movieItem->GetVideoInfoTag()->m_strShowTitle.c_str());
          if (db.GetTvShowsByWhere("", where, items) && items.Size())
            season.GetVideoInfoTag()->m_strPath = items[0]->GetVideoInfoTag()->m_strPath;
          db.Close();
        }
        season.SetCachedSeasonThumb();
        if (season.HasThumbnail())
          m_movieItem->SetProperty("seasonthumb", season.GetThumbnailImage());
      }
    }
    else if (type == VIDEODB_CONTENT_MOVIES)
      m_castList->SetContent("movies");
  }
  loader.LoadItem(m_movieItem.get());
}

void CGUIWindowVideoInfo::Update()
{
  CStdString strTmp;
  strTmp = m_movieItem->GetVideoInfoTag()->m_strTitle; strTmp.Trim();
  SetLabel(CONTROL_TITLE, strTmp);

  strTmp = m_movieItem->GetVideoInfoTag()->m_strDirector; strTmp.Trim();
  SetLabel(CONTROL_DIRECTOR, strTmp);

  strTmp = m_movieItem->GetVideoInfoTag()->m_strStudio; strTmp.Trim();
  SetLabel(CONTROL_STUDIO, strTmp);

  strTmp = m_movieItem->GetVideoInfoTag()->m_strWritingCredits; strTmp.Trim();
  SetLabel(CONTROL_CREDITS, strTmp);

  strTmp = m_movieItem->GetVideoInfoTag()->m_strGenre; strTmp.Trim();
  SetLabel(CONTROL_GENRE, strTmp);

  strTmp = m_movieItem->GetVideoInfoTag()->m_strTagLine; strTmp.Trim();
  SetLabel(CONTROL_TAGLINE, strTmp);

  strTmp = m_movieItem->GetVideoInfoTag()->m_strPlotOutline; strTmp.Trim();
  SetLabel(CONTROL_PLOTOUTLINE, strTmp);

  strTmp = m_movieItem->GetVideoInfoTag()->m_strTrailer; strTmp.Trim();
  SetLabel(CONTROL_TRAILER, strTmp);

  strTmp = m_movieItem->GetVideoInfoTag()->m_strMPAARating; strTmp.Trim();
  SetLabel(CONTROL_MPAARATING, strTmp);

  CStdString strTop250;
  if (m_movieItem->GetVideoInfoTag()->m_iTop250)
    strTop250.Format("%i", m_movieItem->GetVideoInfoTag()->m_iTop250);
  SetLabel(CONTROL_TOP250, strTop250);

  CStdString strYear;
  if (m_movieItem->GetVideoInfoTag()->m_iYear)
    strYear.Format("%i", m_movieItem->GetVideoInfoTag()->m_iYear);
  else
    strYear = g_infoManager.GetItemLabel(m_movieItem.get(),LISTITEM_PREMIERED);
  SetLabel(CONTROL_YEAR, strYear);

  CStdString strRating_And_Votes;
  if (m_movieItem->GetVideoInfoTag()->m_fRating != 0.0f)  // only non-zero ratings are of interest
    strRating_And_Votes.Format("%03.1f (%s %s)", m_movieItem->GetVideoInfoTag()->m_fRating, m_movieItem->GetVideoInfoTag()->m_strVotes, g_localizeStrings.Get(20350));
  SetLabel(CONTROL_RATING_AND_VOTES, strRating_And_Votes);

  strTmp = m_movieItem->GetVideoInfoTag()->m_strRuntime; strTmp.Trim();
  SetLabel(CONTROL_RUNTIME, strTmp);

  // setup plot text area
  strTmp = m_movieItem->GetVideoInfoTag()->m_strPlot;
  if (!(!m_movieItem->GetVideoInfoTag()->m_strShowTitle.IsEmpty() && m_movieItem->GetVideoInfoTag()->m_iSeason == 0)) // dont apply to tvshows
    if (m_movieItem->GetVideoInfoTag()->m_playCount == 0 && g_guiSettings.GetBool("videolibrary.hideplots"))
      strTmp = g_localizeStrings.Get(20370);

  strTmp.Trim();
  SetLabel(CONTROL_TEXTAREA, strTmp);

  CGUIMessage msg(GUI_MSG_LABEL_BIND, GetID(), CONTROL_LIST, 0, 0, m_castList);
  OnMessage(msg);

  if (m_bViewReview)
  {
    if (!m_movieItem->GetVideoInfoTag()->m_strArtist.IsEmpty())
    {
      SET_CONTROL_LABEL(CONTROL_BTN_TRACKS, 133);
    }
    else
    {
      SET_CONTROL_LABEL(CONTROL_BTN_TRACKS, 206);
    }

    SET_CONTROL_HIDDEN(CONTROL_LIST);
    SET_CONTROL_VISIBLE(CONTROL_TEXTAREA);
  }
  else
  {
    SET_CONTROL_LABEL(CONTROL_BTN_TRACKS, 207);

    SET_CONTROL_HIDDEN(CONTROL_TEXTAREA);
    SET_CONTROL_VISIBLE(CONTROL_LIST);
  }

  // Check for resumability
  CGUIWindowVideoFiles *window = (CGUIWindowVideoFiles *)m_gWindowManager.GetWindow(WINDOW_VIDEO_FILES);
  if (window && window->GetResumeItemOffset(m_movieItem.get()) > 0)
    CONTROL_ENABLE(CONTROL_BTN_RESUME);
  else
    CONTROL_DISABLE(CONTROL_BTN_RESUME);

  if (m_movieItem->GetVideoInfoTag()->m_strEpisodeGuide.IsEmpty()) // disable the play button for tv show info
    CONTROL_ENABLE(CONTROL_BTN_PLAY);
  else
    CONTROL_DISABLE(CONTROL_BTN_PLAY);

  // update the thumbnail
  const CGUIControl* pControl = GetControl(CONTROL_IMAGE);
  if (pControl)
  {
    CGUIImage* pImageControl = (CGUIImage*)pControl;
    pImageControl->FreeResources();
    pImageControl->SetFileName(m_movieItem->GetThumbnailImage());
  }
  // tell our GUI to completely reload all controls (as some of them
  // are likely to have had this image in use so will need refreshing)
  CGUIMessage reload(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_REFRESH_THUMBS);
  g_graphicsContext.SendMessage(reload);
}

void CGUIWindowVideoInfo::Refresh()
{
  // quietly return if Internet lookups are disabled
  if (!g_guiSettings.GetBool("network.enableinternet"))
  {
    Update();
    return ;
  }

  try
  {
    OutputDebugString("Refresh\n");

    CStdString strImage = m_movieItem->GetVideoInfoTag()->m_strPictureURL.GetFirstThumb().m_url;

    bool hasUpdatedThumb = false;
    CStdString thumbImage = m_movieItem->GetThumbnailImage();
    if (thumbImage.IsEmpty())
      thumbImage = m_movieItem->GetCachedVideoThumb();

    if (!CFile::Exists(thumbImage) || m_movieItem->GetProperty("HasAutoThumb") == "1")
    {
      m_movieItem->SetUserVideoThumb();
      if (m_movieItem->GetThumbnailImage() != thumbImage)
      {
        thumbImage = m_movieItem->GetThumbnailImage();
        hasUpdatedThumb = true;
      }
    }
    if (!CFile::Exists(thumbImage) && strImage.size() > 0)
    {
      CScraperUrl::DownloadThumbnail(thumbImage,m_movieItem->GetVideoInfoTag()->m_strPictureURL.GetFirstThumb());
      hasUpdatedThumb = true;
    }

    if (CFile::Exists(thumbImage))
    {
      VIDEO::CVideoInfoScanner::ApplyIMDBThumbToFolder(m_movieItem->GetProperty("set_folder_thumb"), thumbImage);
      hasUpdatedThumb = true;
    }

    if (hasUpdatedThumb)
    {
      m_movieItem->SetThumbnailImage(thumbImage);
      CUtil::DeleteVideoDatabaseDirectoryCache();
      m_hasUpdatedThumb = true;
    }

    Update();
    //OutputDebugString("update\n");
    //OutputDebugString("updated\n");
  }
  catch (...)
  {}
}
bool CGUIWindowVideoInfo::NeedRefresh() const
{
  return m_bRefresh;
}

bool CGUIWindowVideoInfo::RefreshAll() const
{
  return m_bRefreshAll;
}

/// \brief Search the current directory for a string got from the virtual keyboard
void CGUIWindowVideoInfo::OnSearch(CStdString& strSearch)
{
  if (m_dlgProgress)
  {
    m_dlgProgress->SetHeading(194);
    m_dlgProgress->SetLine(0, strSearch);
    m_dlgProgress->SetLine(1, "");
    m_dlgProgress->SetLine(2, "");
    m_dlgProgress->StartModal();
    m_dlgProgress->Progress();
  }
  CFileItemList items;
  DoSearch(strSearch, items);

  if (items.Size())
  {
    CGUIDialogSelect* pDlgSelect = (CGUIDialogSelect*)m_gWindowManager.GetWindow(WINDOW_DIALOG_SELECT);
    pDlgSelect->Reset();
    pDlgSelect->SetHeading(283);
    items.Sort(SORT_METHOD_LABEL, SORT_ORDER_ASC);

    for (int i = 0; i < (int)items.Size(); i++)
    {
      CFileItemPtr pItem = items[i];
      pDlgSelect->Add(pItem->GetLabel());
    }

    pDlgSelect->DoModal();

    int iItem = pDlgSelect->GetSelectedLabel();
    if (iItem < 0)
    {
      if (m_dlgProgress) m_dlgProgress->Close();
      return ;
    }

    CFileItem* pSelItem = new CFileItem(*items[iItem]);

    OnSearchItemFound(pSelItem);

    delete pSelItem;
    if (m_dlgProgress) m_dlgProgress->Close();
  }
  else
  {
    if (m_dlgProgress) m_dlgProgress->Close();
    CGUIDialogOK::ShowAndGetInput(194, 284, 0, 0);
  }
}

/// \brief Make the actual search for the OnSearch function.
/// \param strSearch The search string
/// \param items Items Found
void CGUIWindowVideoInfo::DoSearch(CStdString& strSearch, CFileItemList& items)
{
  VECMOVIES movies;
  CVideoDatabase db;
  if (!db.Open())
    return;

  db.GetMoviesByActor(strSearch, movies);
  for (int i = 0; i < (int)movies.size(); ++i)
  {
    CStdString strItem;
    if (movies[i].m_iYear > 0)
      strItem.Format("[%s] %s (%i)", g_localizeStrings.Get(20338), movies[i].m_strTitle, movies[i].m_iYear);  // Movie
    else
      strItem.Format("[%s] %s", g_localizeStrings.Get(20338), movies[i].m_strTitle);  // Movie

    CFileItemPtr pItem(new CFileItem(strItem));
    *pItem->GetVideoInfoTag() = movies[i];
    pItem->m_strPath = movies[i].m_strFileNameAndPath;
    items.Add(pItem);
  }
  movies.clear();
  db.GetTvShowsByActor(strSearch, movies);
  for (int i = 0; i < (int)movies.size(); ++i)
  {
    CStdString strItem;
    strItem.Format("[%s] %s", g_localizeStrings.Get(20364), movies[i].m_strTitle);  // Movie
    CFileItemPtr pItem(new CFileItem(strItem));
    *pItem->GetVideoInfoTag() = movies[i];
    pItem->m_strPath.Format("videodb://2/3/%i/",movies[i].m_iDbId);
    items.Add(pItem);
  }
  movies.clear();
  db.GetEpisodesByActor(strSearch, movies);
  for (int i = 0; i < (int)movies.size(); ++i)
  {
    CStdString strItem;
    strItem.Format("[%s] %s", g_localizeStrings.Get(20359), movies[i].m_strTitle);  // Movie
    CFileItemPtr pItem(new CFileItem(strItem));
    *pItem->GetVideoInfoTag() = movies[i];
    pItem->m_strPath = movies[i].m_strFileNameAndPath;
    items.Add(pItem);
  }

  CFileItemList mvids;
  db.GetMusicVideosByArtist(strSearch, mvids);
  for (int i = 0; i < (int)mvids.Size(); ++i)
  {
    CStdString strItem;
    strItem.Format("[%s] %s", g_localizeStrings.Get(20391), mvids[i]->GetVideoInfoTag()->m_strTitle);  // Movie
    CFileItemPtr pItem(new CFileItem(strItem));
    *pItem->GetVideoInfoTag() = *mvids[i]->GetVideoInfoTag();
    pItem->m_strPath = mvids[i]->GetVideoInfoTag()->m_strFileNameAndPath;
    items.Add(pItem);
  }
  db.Close();
}

VIDEODB_CONTENT_TYPE CGUIWindowVideoInfo::GetContentType(const CFileItem *pItem) const
{
  VIDEODB_CONTENT_TYPE type = VIDEODB_CONTENT_MOVIES;
  if (pItem->HasVideoInfoTag() && !pItem->GetVideoInfoTag()->m_strShowTitle.IsEmpty()) // tvshow
    type = VIDEODB_CONTENT_TVSHOWS;
  if (pItem->HasVideoInfoTag() && pItem->GetVideoInfoTag()->m_iSeason > -1 && !pItem->m_bIsFolder) // episode
    type = VIDEODB_CONTENT_EPISODES;
  if (pItem->HasVideoInfoTag() && !pItem->GetVideoInfoTag()->m_strArtist.IsEmpty())
    type = VIDEODB_CONTENT_MUSICVIDEOS;
  return type;
}

/// \brief React on the selected search item
/// \param pItem Search result item
void CGUIWindowVideoInfo::OnSearchItemFound(const CFileItem* pItem)
{
  VIDEODB_CONTENT_TYPE type = GetContentType(pItem);

  CVideoDatabase db;
  if (!db.Open())
    return;

  CVideoInfoTag movieDetails;
  if (type == VIDEODB_CONTENT_MOVIES)
    db.GetMovieInfo(pItem->m_strPath, movieDetails, pItem->GetVideoInfoTag()->m_iDbId);
  if (type == VIDEODB_CONTENT_EPISODES)
    db.GetEpisodeInfo(pItem->m_strPath, movieDetails, pItem->GetVideoInfoTag()->m_iDbId);
  if (type == VIDEODB_CONTENT_TVSHOWS)
    db.GetTvShowInfo(pItem->m_strPath, movieDetails, pItem->GetVideoInfoTag()->m_iDbId);
  if (type == VIDEODB_CONTENT_MUSICVIDEOS)
    db.GetMusicVideoInfo(pItem->m_strPath, movieDetails, pItem->GetVideoInfoTag()->m_iDbId);
  db.Close();

  CFileItem item(*pItem);
  *item.GetVideoInfoTag() = movieDetails;
  SetMovie(&item);
  Refresh();
}

void CGUIWindowVideoInfo::ClearCastList()
{
  CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), CONTROL_LIST);
  OnMessage(msg);
  m_castList->Clear();
}

void CGUIWindowVideoInfo::Play(bool resume)
{
  CFileItem movie(m_movieItem->GetVideoInfoTag()->m_strFileNameAndPath, false);
  if (m_movieItem->GetVideoInfoTag()->m_strFileNameAndPath.IsEmpty())
    movie.m_strPath = m_movieItem->m_strPath;
  CGUIWindowVideoFiles* pWindow = (CGUIWindowVideoFiles*)m_gWindowManager.GetWindow(WINDOW_VIDEO_FILES);
  if (pWindow)
  {
    // close our dialog
    Close(true);
    if (resume)
      movie.m_lStartOffset = STARTOFFSET_RESUME;
    pWindow->PlayMovie(&movie);
  }
}

// Get Thumb from user choice.
// Options are:
// 1.  Current thumb
// 2.  IMDb thumb
// 3.  Local thumb
// 4.  No thumb (if no Local thumb is available)
void CGUIWindowVideoInfo::OnGetThumb()
{
  CFileItemList items;

  // Current thumb
  if (CFile::Exists(m_movieItem->GetThumbnailImage()))
  {
    CFileItemPtr item(new CFileItem("thumb://Current", false));
    item->SetThumbnailImage(m_movieItem->GetThumbnailImage());
    item->SetLabel(g_localizeStrings.Get(20016));
    items.Add(item);
  }

  // Grab the thumbnails from the web
  int i=1;
  for (std::vector<CScraperUrl::SUrlEntry>::iterator iter=m_movieItem->GetVideoInfoTag()->m_strPictureURL.m_url.begin();iter != m_movieItem->GetVideoInfoTag()->m_strPictureURL.m_url.end();++iter)
  {
    if (iter->m_type == CScraperUrl::URL_TYPE_SEASON)
      continue;
    CStdString strItemPath;
    strItemPath.Format("thumb://Remote%i",i++);
    CFileItemPtr item(new CFileItem(strItemPath, false));
    item->SetThumbnailImage("http://this.is/a/thumb/from/the/web");
    item->SetIconImage("defaultPicture.png");
    item->GetVideoInfoTag()->m_strPictureURL.m_url.push_back(*iter);
    item->SetLabel(g_localizeStrings.Get(415));
    item->SetProperty("labelonthumbload", g_localizeStrings.Get(20015));

    // make sure any previously cached thumb is removed
    if (CFile::Exists(item->GetCachedPictureThumb()))
      CFile::Delete(item->GetCachedPictureThumb());
    items.Add(item);
  }

  CStdString cachedLocalThumb;
  CStdString localThumb(m_movieItem->GetUserVideoThumb());
  if (CFile::Exists(localThumb))
  {
    CUtil::AddFileToFolder(g_advancedSettings.m_cachePath, "localthumb.jpg", cachedLocalThumb);
    CPicture pic;
    pic.DoCreateThumbnail(localThumb, cachedLocalThumb);
    CFileItemPtr item(new CFileItem("thumb://Local", false));
    item->SetThumbnailImage(cachedLocalThumb);
    item->SetLabel(g_localizeStrings.Get(20017));
    items.Add(item);
  }
  else
  { // no local thumb exists, so we are just using the IMDb thumb or cached thumb
    // which is probably the IMDb thumb.  These could be wrong, so allow the user
    // to delete the incorrect thumb
    CFileItemPtr item(new CFileItem("thumb://None", false));
    item->SetThumbnailImage("defaultVideoBig.png");
    item->SetLabel(g_localizeStrings.Get(20018));
    items.Add(item);
  }

  CStdString result;
  VECSOURCES sources(g_settings.m_videoSources);
  g_mediaManager.GetLocalDrives(sources);
  if (!CGUIDialogFileBrowser::ShowAndGetImage(items, sources, g_localizeStrings.Get(20019), result))
    return;   // user cancelled

  if (result == "thumb://Current")
    return;   // user chose the one they have

  // delete the thumbnail if that's what the user wants, else overwrite with the
  // new thumbnail
  CFileItem item(*m_movieItem->GetVideoInfoTag());
  CStdString cachedThumb(item.GetCachedVideoThumb());
  if (!m_movieItem->m_bIsFolder && m_movieItem->GetVideoInfoTag()->m_iSeason > -1)
    cachedThumb = item.GetCachedEpisodeThumb();

  if (result.Left(14) == "thumb://Remote")
  {
    CStdString strFile;
    CFileItem chosen(result, false);
    CStdString thumb = chosen.GetCachedPictureThumb();
    if (CFile::Exists(thumb))
    {
      // NOTE: This could fail if the thumbloader was too slow and the user too impatient
      CFile::Cache(thumb, cachedThumb);
    }
    else
      result = "thumb://None";
  }
  else if (result == "thumb://Local")
    CFile::Cache(cachedLocalThumb, cachedThumb);
  else if (CFile::Exists(result))
  {
    CPicture pic;
    pic.DoCreateThumbnail(result, cachedThumb);
  }
  else
    result = "thumb://None";

  if (result == "thumb://None")
  {
    CFile::Delete(m_movieItem->GetCachedVideoThumb());
    CFile::Delete(m_movieItem->GetCachedEpisodeThumb());
    cachedThumb.Empty();
  }

  CUtil::DeleteVideoDatabaseDirectoryCache(); // to get them new thumbs to show
  m_movieItem->SetThumbnailImage(cachedThumb);
  if (m_movieItem->HasProperty("set_folder_thumb"))
  { // have a folder thumb to set as well
    VIDEO::CVideoInfoScanner::ApplyIMDBThumbToFolder(m_movieItem->GetProperty("set_folder_thumb"), cachedThumb);
  }
  m_hasUpdatedThumb = true;

  // Update our screen
  Update();
}

// Allow user to select a Fanart
void CGUIWindowVideoInfo::OnGetFanart()
{
  CFileItemList items;

  // ensure the fanart is unpacked
  m_movieItem->GetVideoInfoTag()->m_fanart.Unpack();

  // Grab the thumbnails from the web
  CStdString strPath;
  CUtil::AddFileToFolder(g_advancedSettings.m_cachePath,"fanartthumbs",strPath);
  CUtil::WipeDir(strPath);
  DIRECTORY::CDirectory::Create(strPath);
  for (unsigned int i = 0; i < m_movieItem->GetVideoInfoTag()->m_fanart.GetNumFanarts(); i++)
  {
    CStdString strItemPath;
    strItemPath.Format("fanart://Remote%i",i);
    CFileItemPtr item(new CFileItem(strItemPath, false));
    item->SetThumbnailImage("http://this.is/a/thumb/from/the/web");
    item->SetIconImage("defaultPicture.png");
    item->GetVideoInfoTag()->m_fanart = m_movieItem->GetVideoInfoTag()->m_fanart;
    item->SetProperty("fanart_number", (int)i);
    item->SetLabel(g_localizeStrings.Get(415));
    item->SetProperty("labelonthumbload", g_localizeStrings.Get(20015));

    // make sure any previously cached thumb is removed
    if (CFile::Exists(item->GetCachedPictureThumb()))
      CFile::Delete(item->GetCachedPictureThumb());
    items.Add(item);
  }

  CFileItem item(*m_movieItem->GetVideoInfoTag());
  CStdString cachedThumb(item.GetCachedFanart());

  CStdString strLocal = item.CacheFanart(true);
  if (!strLocal.IsEmpty())
  {
    CFileItemPtr itemLocal(new CFileItem("fanart://Local",false));
    itemLocal->SetThumbnailImage(strLocal);
    itemLocal->SetLabel(g_localizeStrings.Get(20017));
    items.Add(itemLocal);
  }

  if (CFile::Exists(cachedThumb))
  {
    CFileItemPtr itemCurrent(new CFileItem("fanart://Current",false));
    itemCurrent->SetThumbnailImage(cachedThumb);
    itemCurrent->SetLabel(g_localizeStrings.Get(20016));
    items.Add(itemCurrent);
  }

  CFileItemPtr itemNone(new CFileItem("fanart://None", false));
  itemNone->SetThumbnailImage("defaultVideoBig.png");
  itemNone->SetLabel(g_localizeStrings.Get(20018));
  items.Add(itemNone);

  CStdString result;
  VECSOURCES sources(g_settings.m_videoSources);
  g_mediaManager.GetLocalDrives(sources);
  bool flip=false;
  if (!CGUIDialogFileBrowser::ShowAndGetImage(items, sources, g_localizeStrings.Get(20019), result, &flip) || result.Equals("fanart://Current"))
    return;   // user cancelled

  if (CFile::Exists(cachedThumb))
    CFile::Delete(cachedThumb);

  if (result.Equals("fanart://Local"))
    result = strLocal;

  if (result.Left(15) == "fanart://Remote")
  {
    int iFanart = atoi(result.Mid(15).c_str());
    // set new primary fanart, and update our database accordingly
    m_movieItem->GetVideoInfoTag()->m_fanart.SetPrimaryFanart(iFanart);
    CVideoDatabase db;
    if (db.Open())
    {
      db.UpdateFanart(*m_movieItem, GetContentType(m_movieItem.get()));
      db.Close();
    }

    // download the fullres fanart image
    CStdString tempFile = "special://temp/fanart_download.jpg";
    CAsyncFileCopy downloader;
    bool succeeded = downloader.Copy(m_movieItem->GetVideoInfoTag()->m_fanart.GetImageURL(), tempFile, g_localizeStrings.Get(13413));
    if (succeeded)
    {
      CPicture pic;
      if (flip)
        pic.ConvertFile(tempFile, cachedThumb,0,1920,-1,100,true);
      else
        pic.CacheImage(tempFile, cachedThumb);
    }
    CFile::Delete(tempFile);
    if (!succeeded)
      return; // failed or cancelled download, so don't do anything
  }
  else if (CFile::Exists(result))
  { // local file
    CPicture pic;
    if (flip)
      pic.ConvertFile(result, cachedThumb,0,1920,-1,100,true);
    else
      pic.CacheImage(result, cachedThumb);
  }

  CUtil::DeleteVideoDatabaseDirectoryCache(); // to get them new thumbs to show
  if (CFile::Exists(cachedThumb))
    m_movieItem->SetProperty("fanart_image", cachedThumb);
  else
    m_movieItem->ClearProperty("fanart_image");
  m_hasUpdatedThumb = true;

  // Update our screen
  Update();
}

void CGUIWindowVideoInfo::PlayTrailer()
{
  CFileItem item;
  item.m_strPath = m_movieItem->GetVideoInfoTag()->m_strTrailer;
  *item.GetVideoInfoTag() = *m_movieItem->GetVideoInfoTag();
  item.GetVideoInfoTag()->m_strTitle.Format("%s (%s)",m_movieItem->GetVideoInfoTag()->m_strTitle.c_str(),g_localizeStrings.Get(20410));
  item.SetThumbnailImage(m_movieItem->GetThumbnailImage());
  item.GetVideoInfoTag()->m_iDbId = -1;

  // Close the dialog.
  Close(true);
  g_application.getApplicationMessenger().PlayFile(item);
}

void CGUIWindowVideoInfo::SetLabel(int iControl, const CStdString &strLabel)
{
  if (strLabel.IsEmpty())
  {
    SET_CONTROL_LABEL(iControl, 416);  // "Not available"
  }
  else
  {
    SET_CONTROL_LABEL(iControl, strLabel);
  }
}

const CStdString& CGUIWindowVideoInfo::GetThumbnail() const
{
  return m_movieItem->GetThumbnailImage();
}
