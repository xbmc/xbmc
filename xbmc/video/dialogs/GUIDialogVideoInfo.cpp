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

#include "GUIDialogVideoInfo.h"
#include "guilib/GUIWindow.h"
#include "Util.h"
#include "pictures/Picture.h"
#include "guilib/GUIImage.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "video/windows/GUIWindowVideoNav.h"
#include "dialogs/GUIDialogFileBrowser.h"
#include "video/VideoInfoScanner.h"
#include "Application.h"
#include "video/VideoInfoTag.h"
#include "guilib/GUIWindowManager.h"
#include "dialogs/GUIDialogOK.h"
#include "dialogs/GUIDialogYesNo.h"
#include "dialogs/GUIDialogSelect.h"
#include "dialogs/GUIDialogProgress.h"
#include "filesystem/File.h"
#include "FileItem.h"
#include "storage/MediaManager.h"
#include "utils/AsyncFileCopy.h"
#include "settings/Settings.h"
#include "settings/AdvancedSettings.h"
#include "settings/GUISettings.h"
#include "guilib/LocalizeStrings.h"
#include "GUIUserMessages.h"
#include "TextureCache.h"

using namespace std;
using namespace XFILE;

#define CONTROL_IMAGE                3
#define CONTROL_TEXTAREA             4
#define CONTROL_BTN_TRACKS           5
#define CONTROL_BTN_REFRESH          6
#define CONTROL_BTN_PLAY             8
#define CONTROL_BTN_RESUME           9
#define CONTROL_BTN_GET_THUMB       10
#define CONTROL_BTN_PLAY_TRAILER    11
#define CONTROL_BTN_GET_FANART      12
#define CONTROL_BTN_DIRECTOR        13

#define CONTROL_LIST                50

CGUIDialogVideoInfo::CGUIDialogVideoInfo(void)
    : CGUIDialog(WINDOW_DIALOG_VIDEO_INFO, "DialogVideoInfo.xml")
    , m_movieItem(new CFileItem)
{
  m_bRefreshAll = true;
  m_bRefresh = false;
  m_hasUpdatedThumb = false;
  m_castList = new CFileItemList;
}

CGUIDialogVideoInfo::~CGUIDialogVideoInfo(void)
{
  delete m_castList;
}

bool CGUIDialogVideoInfo::OnMessage(CGUIMessage& message)
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
      m_dlgProgress = (CGUIDialogProgress*)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);

      m_bRefresh = false;
      m_bRefreshAll = true;
      m_hasUpdatedThumb = false;

      CGUIDialog::OnMessage(message);
      m_bViewReview = true;
      Refresh();

      CVideoDatabase database;
      ADDON::ScraperPtr scraper;

      if(database.Open())
      {
        scraper = database.GetScraperForPath(m_movieItem->GetVideoInfoTag()->GetPath());
        database.Close();
      }

      CONTROL_ENABLE_ON_CONDITION(CONTROL_BTN_REFRESH, (g_settings.GetCurrentProfile().canWriteDatabases() || g_passwordManager.bMasterUser) && !m_movieItem->GetVideoInfoTag()->m_strIMDBNumber.Left(2).Equals("xx") && scraper);
      CONTROL_ENABLE_ON_CONDITION(CONTROL_BTN_GET_THUMB, (g_settings.GetCurrentProfile().canWriteDatabases() || g_passwordManager.bMasterUser) && !m_movieItem->GetVideoInfoTag()->m_strIMDBNumber.Mid(2).Equals("plugin"));

      VIDEODB_CONTENT_TYPE type = (VIDEODB_CONTENT_TYPE)m_movieItem->GetVideoContentType();
      if (type == VIDEODB_CONTENT_TVSHOWS || type == VIDEODB_CONTENT_MOVIES)
        CONTROL_ENABLE_ON_CONDITION(CONTROL_BTN_GET_FANART, (g_settings.GetCurrentProfile().canWriteDatabases() || g_passwordManager.bMasterUser) && !m_movieItem->GetVideoInfoTag()->m_strIMDBNumber.Mid(2).Equals("plugin"));
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
      else if (iControl == CONTROL_BTN_DIRECTOR)
      {
        OnSearch(m_movieItem->GetVideoInfoTag()->m_strDirector);
      }
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
  case GUI_MSG_NOTIFY_ALL:
    {
      if (IsActive() && message.GetParam1() == GUI_MSG_UPDATE_ITEM && message.GetItem())
      {
        CFileItemPtr item = boost::static_pointer_cast<CFileItem>(message.GetItem());
        if (item && m_movieItem->GetPath().Equals(item->GetPath()))
        { // Just copy over the stream details and the thumb if we don't already have one
          if (!m_movieItem->HasThumbnail())
            m_movieItem->SetThumbnailImage(item->GetThumbnailImage());
          m_movieItem->GetVideoInfoTag()->m_streamDetails = item->GetVideoInfoTag()->m_streamDetails;
        }
        return true;
      }
    }
  }

  return CGUIDialog::OnMessage(message);
}

void CGUIDialogVideoInfo::SetMovie(const CFileItem *item)
{
  *m_movieItem = *item;
  // setup cast list + determine type.  We need to do this here as it makes
  // sure that content type (among other things) is set correctly for the
  // old fixed id labels that we have floating around (they may be using
  // content type to determine visibility, so we'll set the wrong label)
  ClearCastList();
  VIDEODB_CONTENT_TYPE type = (VIDEODB_CONTENT_TYPE)m_movieItem->GetVideoContentType();
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
      if (m_movieItem->CacheLocalFanart())
        m_movieItem->SetProperty("fanart_image",m_movieItem->GetCachedFanart());
    }
    // determine type:
    if (type == VIDEODB_CONTENT_TVSHOWS)
    {
      m_castList->SetContent("tvshows");
      // special case stuff for shows (not currently retrieved from the library in filemode (ref: GetTvShowInfo vs GetTVShowsByWhere)
      m_movieItem->m_dateTime.SetFromDateString(m_movieItem->GetVideoInfoTag()->m_strPremiered);
      if(m_movieItem->GetVideoInfoTag()->m_iYear == 0 && m_movieItem->m_dateTime.IsValid())
        m_movieItem->GetVideoInfoTag()->m_iYear = m_movieItem->m_dateTime.GetYear();
      m_movieItem->SetProperty("totalepisodes", m_movieItem->GetVideoInfoTag()->m_iEpisode);
      m_movieItem->SetProperty("numepisodes", m_movieItem->GetVideoInfoTag()->m_iEpisode); // info view has no concept of current watched/unwatched filter as we could come here from files view, but set for consistency
      m_movieItem->SetProperty("watchedepisodes", m_movieItem->GetVideoInfoTag()->m_playCount);
      m_movieItem->SetProperty("unwatchedepisodes", m_movieItem->GetVideoInfoTag()->m_iEpisode - m_movieItem->GetVideoInfoTag()->m_playCount);
      m_movieItem->GetVideoInfoTag()->m_playCount = (m_movieItem->GetVideoInfoTag()->m_iEpisode == m_movieItem->GetVideoInfoTag()->m_playCount) ? 1 : 0;
    }
    else if (type == VIDEODB_CONTENT_EPISODES)
    {
      m_castList->SetContent("episodes");
      // special case stuff for episodes (not currently retrieved from the library in filemode (ref: GetEpisodeInfo vs GetEpisodesByWhere)
      m_movieItem->m_dateTime.SetFromDateString(m_movieItem->GetVideoInfoTag()->m_strFirstAired);
      if(m_movieItem->GetVideoInfoTag()->m_iYear == 0 && m_movieItem->m_dateTime.IsValid())
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
        season.GetVideoInfoTag()->m_strPath = item->GetVideoInfoTag()->m_strShowPath;
        if (CFile::Exists(season.GetCachedSeasonThumb()))
          m_movieItem->SetProperty("seasonthumb", season.GetCachedSeasonThumb());
      }
    }
    else if (type == VIDEODB_CONTENT_MOVIES)
    {
      m_castList->SetContent("movies");

      // local trailers should always override non-local, so check 
      // for a local one if the registered trailer is online
      if (m_movieItem->GetVideoInfoTag()->m_strTrailer.IsEmpty() ||
          URIUtils::IsInternetStream(m_movieItem->GetVideoInfoTag()->m_strTrailer))
      {
        CStdString localTrailer = m_movieItem->FindTrailer();
        if (!localTrailer.IsEmpty())
        {
          m_movieItem->GetVideoInfoTag()->m_strTrailer = localTrailer;
          CVideoDatabase database;
          if(database.Open())
          {
            database.SetDetail(m_movieItem->GetVideoInfoTag()->m_strTrailer,
                               m_movieItem->GetVideoInfoTag()->m_iDbId,
                               VIDEODB_ID_TRAILER, VIDEODB_CONTENT_MOVIES);
            database.Close();
            CUtil::DeleteVideoDatabaseDirectoryCache();
          }
        }
      }
    }
  }
  m_loader.LoadItem(m_movieItem.get());
}

void CGUIDialogVideoInfo::Update()
{
  // setup plot text area
  CStdString strTmp = m_movieItem->GetVideoInfoTag()->m_strPlot;
  if (!(!m_movieItem->GetVideoInfoTag()->m_strShowTitle.IsEmpty() && m_movieItem->GetVideoInfoTag()->m_iSeason == 0)) // dont apply to tvshows
    if (m_movieItem->GetVideoInfoTag()->m_playCount == 0 && !g_guiSettings.GetBool("videolibrary.showunwatchedplots"))
      strTmp = g_localizeStrings.Get(20370);

  strTmp.Trim();
  SetLabel(CONTROL_TEXTAREA, strTmp);

  CGUIMessage msg(GUI_MSG_LABEL_BIND, GetID(), CONTROL_LIST, 0, 0, m_castList);
  OnMessage(msg);

  if (GetControl(CONTROL_BTN_TRACKS)) // if no CONTROL_BTN_TRACKS found - allow skinner full visibility control over CONTROL_TEXTAREA and CONTROL_LIST
  {
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
  }

  // Check for resumability
  if (CGUIWindowVideoBase::GetResumeItemOffset(m_movieItem.get()) > 0)
    CONTROL_ENABLE(CONTROL_BTN_RESUME);
  else
    CONTROL_DISABLE(CONTROL_BTN_RESUME);

  CONTROL_ENABLE(CONTROL_BTN_PLAY);

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
  if (m_hasUpdatedThumb)
  {
    CGUIMessage reload(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_REFRESH_THUMBS);
    g_windowManager.SendMessage(reload);
  }
}

void CGUIDialogVideoInfo::Refresh()
{
  try
  {
    OutputDebugString("Refresh\n");

    CStdString strImage = m_movieItem->GetVideoInfoTag()->m_strPictureURL.GetFirstThumb().m_url;

    bool hasUpdatedThumb = false;
    CStdString thumbImage = m_movieItem->GetThumbnailImage();
    if (thumbImage.IsEmpty())
      thumbImage = m_movieItem->GetCachedVideoThumb();

    if (!CFile::Exists(thumbImage) || m_movieItem->GetProperty("HasAutoThumb") == "1")
    { // don't have a thumb already, try and grab one
      m_movieItem->SetUserVideoThumb();
      if (m_movieItem->GetThumbnailImage() != thumbImage)
        thumbImage = m_movieItem->GetThumbnailImage();
      if (!CFile::Exists(thumbImage) && strImage.size() > 0)
        CScraperUrl::DownloadThumbnail(thumbImage,m_movieItem->GetVideoInfoTag()->m_strPictureURL.GetFirstThumb());

      if (CFile::Exists(thumbImage))
      {
        if (m_movieItem->HasProperty("set_folder_thumb"))
          VIDEO::CVideoInfoScanner::ApplyThumbToFolder(m_movieItem->GetProperty("set_folder_thumb").asString(), thumbImage);
        hasUpdatedThumb = true;
      }
    }

    if (hasUpdatedThumb)
    {
      m_movieItem->SetThumbnailImage(thumbImage);
      CUtil::DeleteVideoDatabaseDirectoryCache();
      m_hasUpdatedThumb = true;
    }

    Update();
  }
  catch (...)
  {}
}
bool CGUIDialogVideoInfo::NeedRefresh() const
{
  return m_bRefresh;
}

bool CGUIDialogVideoInfo::RefreshAll() const
{
  return m_bRefreshAll;
}

/// \brief Search the current directory for a string got from the virtual keyboard
void CGUIDialogVideoInfo::OnSearch(CStdString& strSearch)
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

  if (m_dlgProgress)
    m_dlgProgress->Close();

  if (items.Size())
  {
    CGUIDialogSelect* pDlgSelect = (CGUIDialogSelect*)g_windowManager.GetWindow(WINDOW_DIALOG_SELECT);
    pDlgSelect->Reset();
    pDlgSelect->SetHeading(283);

    for (int i = 0; i < (int)items.Size(); i++)
    {
      CFileItemPtr pItem = items[i];
      pDlgSelect->Add(pItem->GetLabel());
    }

    pDlgSelect->DoModal();

    int iItem = pDlgSelect->GetSelectedLabel();
    if (iItem < 0)
      return;

    CFileItem* pSelItem = new CFileItem(*items[iItem]);

    OnSearchItemFound(pSelItem);

    delete pSelItem;
  }
  else
  {
    CGUIDialogOK::ShowAndGetInput(194, 284, 0, 0);
  }
}

/// \brief Make the actual search for the OnSearch function.
/// \param strSearch The search string
/// \param items Items Found
void CGUIDialogVideoInfo::DoSearch(CStdString& strSearch, CFileItemList& items)
{
  CVideoDatabase db;
  if (!db.Open())
    return;

  CFileItemList movies;
  db.GetMoviesByActor(strSearch, movies);
  for (int i = 0; i < movies.Size(); ++i)
  {
    CStdString label = movies[i]->GetVideoInfoTag()->m_strTitle;
    if (movies[i]->GetVideoInfoTag()->m_iYear > 0)
      label.AppendFormat(" (%i)", movies[i]->GetVideoInfoTag()->m_iYear);
    movies[i]->SetLabel(label);
  }
  CGUIWindowVideoBase::AppendAndClearSearchItems(movies, "[" + g_localizeStrings.Get(20338) + "] ", items);

  db.GetTvShowsByActor(strSearch, movies);
  for (int i = 0; i < movies.Size(); ++i)
  {
    CStdString label = movies[i]->GetVideoInfoTag()->m_strShowTitle;
    if (movies[i]->GetVideoInfoTag()->m_iYear > 0)
      label.AppendFormat(" (%i)", movies[i]->GetVideoInfoTag()->m_iYear);
    movies[i]->SetLabel(label);
  }
  CGUIWindowVideoBase::AppendAndClearSearchItems(movies, "[" + g_localizeStrings.Get(20364) + "] ", items);

  db.GetEpisodesByActor(strSearch, movies);
  for (int i = 0; i < movies.Size(); ++i)
  {
    CStdString label = movies[i]->GetVideoInfoTag()->m_strTitle + " (" +  movies[i]->GetVideoInfoTag()->m_strShowTitle + ")";
    movies[i]->SetLabel(label);
  }
  CGUIWindowVideoBase::AppendAndClearSearchItems(movies, "[" + g_localizeStrings.Get(20359) + "] ", items);

  db.GetMusicVideosByArtist(strSearch, movies);
  for (int i = 0; i < movies.Size(); ++i)
  {
    CStdString label = movies[i]->GetVideoInfoTag()->m_strArtist + " - " + movies[i]->GetVideoInfoTag()->m_strTitle;
    if (movies[i]->GetVideoInfoTag()->m_iYear > 0)
      label.AppendFormat(" (%i)", movies[i]->GetVideoInfoTag()->m_iYear);
    movies[i]->SetLabel(label);
  }
  CGUIWindowVideoBase::AppendAndClearSearchItems(movies, "[" + g_localizeStrings.Get(20391) + "] ", items);
  db.Close();
}

/// \brief React on the selected search item
/// \param pItem Search result item
void CGUIDialogVideoInfo::OnSearchItemFound(const CFileItem* pItem)
{
  VIDEODB_CONTENT_TYPE type = (VIDEODB_CONTENT_TYPE)pItem->GetVideoContentType();

  CVideoDatabase db;
  if (!db.Open())
    return;

  CVideoInfoTag movieDetails;
  if (type == VIDEODB_CONTENT_MOVIES)
    db.GetMovieInfo(pItem->GetPath(), movieDetails, pItem->GetVideoInfoTag()->m_iDbId);
  if (type == VIDEODB_CONTENT_EPISODES)
    db.GetEpisodeInfo(pItem->GetPath(), movieDetails, pItem->GetVideoInfoTag()->m_iDbId);
  if (type == VIDEODB_CONTENT_TVSHOWS)
    db.GetTvShowInfo(pItem->GetPath(), movieDetails, pItem->GetVideoInfoTag()->m_iDbId);
  if (type == VIDEODB_CONTENT_MUSICVIDEOS)
    db.GetMusicVideoInfo(pItem->GetPath(), movieDetails, pItem->GetVideoInfoTag()->m_iDbId);
  db.Close();

  CFileItem item(*pItem);
  *item.GetVideoInfoTag() = movieDetails;
  SetMovie(&item);
  // refresh our window entirely
  Close();
  DoModal();
}

void CGUIDialogVideoInfo::ClearCastList()
{
  CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), CONTROL_LIST);
  OnMessage(msg);
  m_castList->Clear();
}

void CGUIDialogVideoInfo::Play(bool resume)
{
  if (!m_movieItem->GetVideoInfoTag()->m_strEpisodeGuide.IsEmpty())
  {
    CStdString strPath;
    strPath.Format("videodb://2/2/%i/",m_movieItem->GetVideoInfoTag()->m_iDbId);
    Close();
    g_windowManager.ActivateWindow(WINDOW_VIDEO_NAV,strPath);
    return;
  }

  CFileItem movie(*m_movieItem->GetVideoInfoTag());
  if (m_movieItem->GetVideoInfoTag()->m_strFileNameAndPath.IsEmpty())
    movie.SetPath(m_movieItem->GetPath());
  CGUIWindowVideoNav* pWindow = (CGUIWindowVideoNav*)g_windowManager.GetWindow(WINDOW_VIDEO_NAV);
  if (pWindow)
  {
    // close our dialog
    Close(true);
    if (resume)
      movie.m_lStartOffset = STARTOFFSET_RESUME;
    else if (!CGUIWindowVideoBase::ShowResumeMenu(movie)) 
    {
      // The Resume dialog was closed without any choice
      DoModal();
      return;
    }
    pWindow->PlayMovie(&movie);
  }
}

// Get Thumb from user choice.
// Options are:
// 1.  Current thumb
// 2.  IMDb thumb
// 3.  Local thumb
// 4.  No thumb (if no Local thumb is available)
void CGUIDialogVideoInfo::OnGetThumb()
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
  vector<CStdString> thumbs;
  m_movieItem->GetVideoInfoTag()->m_strPictureURL.GetThumbURLs(thumbs);

  for (unsigned int i = 0; i < thumbs.size(); ++i)
  {
    CStdString strItemPath;
    strItemPath.Format("thumb://Remote%i", i);
    CFileItemPtr item(new CFileItem(strItemPath, false));
    item->SetThumbnailImage(thumbs[i]);
    item->SetIconImage("DefaultPicture.png");
    item->SetLabel(g_localizeStrings.Get(20015));

    // TODO: Do we need to clear the cached image?
    //    CTextureCache::Get().ClearCachedImage(thumb);
    items.Add(item);
  }

  CStdString localThumb(m_movieItem->GetUserVideoThumb());
  if (CFile::Exists(localThumb))
  {
    CFileItemPtr item(new CFileItem("thumb://Local", false));
    item->SetThumbnailImage(localThumb);
    item->SetLabel(g_localizeStrings.Get(20017));
    items.Add(item);
  }
  else
  { // no local thumb exists, so we are just using the IMDb thumb or cached thumb
    // which is probably the IMDb thumb.  These could be wrong, so allow the user
    // to delete the incorrect thumb
    CFileItemPtr item(new CFileItem("thumb://None", false));
    item->SetIconImage("DefaultVideo.png");
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
  CTextureCache::Get().ClearCachedImage(cachedThumb, true);

  if (result.Left(14) == "thumb://Remote")
  {
    int number = atoi(result.Mid(14));
    CFile::Cache(thumbs[number], cachedThumb);
  }
  else if (result == "thumb://Local")
    CFile::Cache(localThumb, cachedThumb);
  else if (CFile::Exists(result))
    CPicture::CreateThumbnail(result, cachedThumb);
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
    VIDEO::CVideoInfoScanner::ApplyThumbToFolder(m_movieItem->GetProperty("set_folder_thumb").asString(), cachedThumb);
  }
  m_hasUpdatedThumb = true;

  // Update our screen
  Update();
}

// Allow user to select a Fanart
void CGUIDialogVideoInfo::OnGetFanart()
{
  CFileItemList items;

  CFileItem item(*m_movieItem->GetVideoInfoTag());
  CStdString cachedThumb(item.GetCachedFanart());

  if (CFile::Exists(cachedThumb))
  {
    CFileItemPtr itemCurrent(new CFileItem("fanart://Current",false));
    itemCurrent->SetThumbnailImage(cachedThumb);
    itemCurrent->SetLabel(g_localizeStrings.Get(20440));
    items.Add(itemCurrent);
  }

  // ensure the fanart is unpacked
  m_movieItem->GetVideoInfoTag()->m_fanart.Unpack();

  // Grab the thumbnails from the web
  for (unsigned int i = 0; i < m_movieItem->GetVideoInfoTag()->m_fanart.GetNumFanarts(); i++)
  {
    CStdString strItemPath;
    strItemPath.Format("fanart://Remote%i",i);
    CFileItemPtr item(new CFileItem(strItemPath, false));
    CStdString thumb = m_movieItem->GetVideoInfoTag()->m_fanart.GetPreviewURL(i);
    item->SetThumbnailImage(CTextureCache::GetWrappedThumbURL(thumb));
    item->SetIconImage("DefaultPicture.png");
    item->SetLabel(g_localizeStrings.Get(20441));

    // TODO: Do we need to clear the cached image?
//    CTextureCache::Get().ClearCachedImage(thumb);
    items.Add(item);
  }

  CStdString strLocal = item.GetLocalFanart();
  if (!strLocal.IsEmpty())
  {
    CFileItemPtr itemLocal(new CFileItem("fanart://Local",false));
    itemLocal->SetThumbnailImage(strLocal);
    itemLocal->SetLabel(g_localizeStrings.Get(20438));

    // TODO: Do we need to clear the cached image?
    CTextureCache::Get().ClearCachedImage(strLocal);
    items.Add(itemLocal);
  }
  else
  {
    CFileItemPtr itemNone(new CFileItem("fanart://None", false));
    itemNone->SetIconImage("DefaultVideo.png");
    itemNone->SetLabel(g_localizeStrings.Get(20439));
    items.Add(itemNone);
  }

  CStdString result;
  VECSOURCES sources(g_settings.m_videoSources);
  g_mediaManager.GetLocalDrives(sources);
  bool flip=false;
  if (!CGUIDialogFileBrowser::ShowAndGetImage(items, sources, g_localizeStrings.Get(20437), result, &flip, 20445) || result.Equals("fanart://Current"))
    return;   // user cancelled

  CTextureCache::Get().ClearCachedImage(cachedThumb, true);

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
      db.UpdateFanart(*m_movieItem, (VIDEODB_CONTENT_TYPE)m_movieItem->GetVideoContentType());
      db.Close();
    }

    // download the fullres fanart image
    CStdString tempFile = "special://temp/fanart_download.jpg";
    CAsyncFileCopy downloader;
    bool succeeded = downloader.Copy(m_movieItem->GetVideoInfoTag()->m_fanart.GetImageURL(), tempFile, g_localizeStrings.Get(13413));
    if (succeeded)
    {
      if (flip)
        CPicture::ConvertFile(tempFile, cachedThumb,0,1920,-1,100,true);
      else
        CPicture::CacheFanart(tempFile, cachedThumb);
    }
    CFile::Delete(tempFile);
    if (!succeeded)
      return; // failed or cancelled download, so don't do anything
  }
  else if (CFile::Exists(result))
  { // local file
    if (flip)
      CPicture::ConvertFile(result, cachedThumb,0,1920,-1,100,true);
    else
      CPicture::CacheFanart(result, cachedThumb);
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

void CGUIDialogVideoInfo::PlayTrailer()
{
  CFileItem item;
  item.SetPath(m_movieItem->GetVideoInfoTag()->m_strTrailer);
  *item.GetVideoInfoTag() = *m_movieItem->GetVideoInfoTag();
  item.GetVideoInfoTag()->m_streamDetails.Reset();
  item.GetVideoInfoTag()->m_strTitle.Format("%s (%s)",m_movieItem->GetVideoInfoTag()->m_strTitle.c_str(),g_localizeStrings.Get(20410));
  item.SetThumbnailImage(m_movieItem->GetThumbnailImage());
  item.GetVideoInfoTag()->m_iDbId = -1;
  item.GetVideoInfoTag()->m_iFileId = -1;

  // Close the dialog.
  Close(true);

  if (item.IsPlayList())
    g_application.getApplicationMessenger().MediaPlay(item);
  else
    g_application.getApplicationMessenger().PlayFile(item);
}

void CGUIDialogVideoInfo::SetLabel(int iControl, const CStdString &strLabel)
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

const CStdString& CGUIDialogVideoInfo::GetThumbnail() const
{
  return m_movieItem->GetThumbnailImage();
}
