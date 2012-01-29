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

#include "GUIUserMessages.h"
#include "GUIWindowVideoNav.h"
#include "music/windows/GUIWindowMusicNav.h"
#include "utils/FileUtils.h"
#include "Util.h"
#include "utils/RegExp.h"
#include "PlayListPlayer.h"
#include "GUIPassword.h"
#include "dialogs/GUIDialogFileBrowser.h"
#include "filesystem/VideoDatabaseDirectory.h"
#include "playlists/PlayListFactory.h"
#include "video/dialogs/GUIDialogVideoScan.h"
#include "dialogs/GUIDialogOK.h"
#include "addons/AddonManager.h"
#include "PartyModeManager.h"
#include "music/MusicDatabase.h"
#include "guilib/GUIWindowManager.h"
#include "dialogs/GUIDialogYesNo.h"
#include "dialogs/GUIDialogSelect.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "FileItem.h"
#include "Application.h"
#include "settings/Settings.h"
#include "settings/AdvancedSettings.h"
#include "settings/GUISettings.h"
#include "guilib/LocalizeStrings.h"
#include "storage/MediaManager.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include "pictures/Picture.h"
#include "TextureCache.h"

using namespace XFILE;
using namespace VIDEODATABASEDIRECTORY;
using namespace std;

#define CONTROL_BTNVIEWASICONS     2
#define CONTROL_BTNSORTBY          3
#define CONTROL_BTNSORTASC         4
#define CONTROL_BTNTYPE            5
#define CONTROL_BTNSEARCH          8
#define CONTROL_LABELFILES        12

#define CONTROL_BTN_FILTER        19
#define CONTROL_BTNSHOWMODE       10
#define CONTROL_BTNSHOWALL        14
#define CONTROL_UNLOCK            11

#define CONTROL_FILTER            15
#define CONTROL_BTNPARTYMODE      16
#define CONTROL_BTNFLATTEN        17
#define CONTROL_LABELEMPTY        18

CGUIWindowVideoNav::CGUIWindowVideoNav(void)
    : CGUIWindowVideoBase(WINDOW_VIDEO_NAV, "MyVideoNav.xml")
{
  m_thumbLoader.SetObserver(this);
}

CGUIWindowVideoNav::~CGUIWindowVideoNav(void)
{
}

bool CGUIWindowVideoNav::OnAction(const CAction &action)
{
  if (action.GetID() == ACTION_TOGGLE_WATCHED)
  {
    CFileItemPtr pItem = m_vecItems->Get(m_viewControl.GetSelectedItem());
    if (pItem->IsParentFolder())
      return false;
    if (pItem && pItem->GetVideoInfoTag()->m_playCount == 0)
      return OnContextButton(m_viewControl.GetSelectedItem(),CONTEXT_BUTTON_MARK_WATCHED);
    if (pItem && pItem->GetVideoInfoTag()->m_playCount > 0)
      return OnContextButton(m_viewControl.GetSelectedItem(),CONTEXT_BUTTON_MARK_UNWATCHED);
  }
  return CGUIWindowVideoBase::OnAction(action);
}

bool CGUIWindowVideoNav::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_WINDOW_RESET:
    m_vecItems->SetPath("");
    break;
  case GUI_MSG_WINDOW_DEINIT:
    if (m_thumbLoader.IsLoading())
      m_thumbLoader.StopThread();
    break;
  case GUI_MSG_WINDOW_INIT:
    {
      /* We don't want to show Autosourced items (ie removable pendrives, memorycards) in Library mode */
      m_rootDir.AllowNonLocalSources(false);

      SetProperty("flattened", g_settings.m_bMyVideoNavFlatten);
      if (message.GetNumStringParams() && message.GetStringParam(0).Equals("Files") &&
          g_settings.GetSourcesFromType("video")->empty())
      {
        message.SetStringParam("");
      }
      
      if (!CGUIWindowVideoBase::OnMessage(message))
        return false;

      //  base class has opened the database, do our check
      m_database.Open();

      if (!m_database.HasContent() && m_vecItems->IsVideoDb())
      { // no library - make sure we default to the root.
        m_vecItems->SetPath("");
        SetHistoryForPath("");
        Update("");
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
          if (!g_partyModeManager.Enable(PARTYMODECONTEXT_VIDEO))
          {
            SET_CONTROL_SELECTED(GetID(),CONTROL_BTNPARTYMODE,false);
            return false;
          }

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
      else if (iControl == CONTROL_BTNSHOWMODE)
      {
        g_settings.CycleWatchMode(m_vecItems->GetContent());
        g_settings.Save();

        // TODO: Can we perhaps filter this directly?  Probably not for some of the more complicated views,
        //       but for those perhaps we can just display them all, and only filter when we get a list
        //       of actual videos?
        Update(m_vecItems->GetPath());
        return true;
      }
      else if (iControl == CONTROL_BTNFLATTEN)
      {
        g_settings.m_bMyVideoNavFlatten = !g_settings.m_bMyVideoNavFlatten;
        g_settings.Save();
        SetProperty("flattened", g_settings.m_bMyVideoNavFlatten);
        CUtil::DeleteVideoDatabaseDirectoryCache();
        SetupShares();
        Update("");
        return true;
      }
      else if (iControl == CONTROL_BTNSHOWALL)
      {
        if (g_settings.GetWatchMode(m_vecItems->GetContent()) == VIDEO_SHOW_ALL)
          g_settings.SetWatchMode(m_vecItems->GetContent(), VIDEO_SHOW_UNWATCHED);
        else
          g_settings.SetWatchMode(m_vecItems->GetContent(), VIDEO_SHOW_ALL);
        g_settings.Save();
        // TODO: Can we perhaps filter this directly?  Probably not for some of the more complicated views,
        //       but for those perhaps we can just display them all, and only filter when we get a list
        //       of actual videos?
        Update(m_vecItems->GetPath());
        return true;
      }
    }
    break;
    // update the display
    case GUI_MSG_SCAN_FINISHED:
    case GUI_MSG_REFRESH_THUMBS:
    {
      Update(m_vecItems->GetPath());
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
  else if (strPath.Equals("sources://video/"))
    return "Files";
  else
  {
    CLog::Log(LOGERROR, "  CGUIWindowVideoNav::GetQuickpathName: Unknown parameter (%s)", strPath.c_str());
    return strPath;
  }
}

void CGUIWindowVideoNav::OnItemLoaded(CFileItem* pItem)
{
  /* even though the background loader is running multiple threads and we could,
     be acting on someone else's flag, we don't care who invalidates the cache
     only that it is done.  We also don't care if it is done multiple times due
     to a race between multiple threads here at the same time */
  CUtil::DeleteVideoDatabaseDirectoryCache();
}

bool CGUIWindowVideoNav::GetDirectory(const CStdString &strDirectory, CFileItemList &items)
{
  CFileItem directory(strDirectory, true);

  if (m_thumbLoader.IsLoading())
    m_thumbLoader.StopThread();

  m_rootDir.SetCacheDirectory(DIR_CACHE_ONCE);
  items.ClearProperties();

  bool bResult = CGUIWindowVideoBase::GetDirectory(strDirectory, items);
  if (bResult)
  {
    if (items.IsVideoDb())
    {
      XFILE::CVideoDatabaseDirectory dir;
      CQueryParams params;
      dir.GetQueryParams(items.GetPath(),params);
      VIDEODATABASEDIRECTORY::NODE_TYPE node = dir.GetDirectoryChildType(items.GetPath());

      items.SetThumbnailImage("");
      if (node == VIDEODATABASEDIRECTORY::NODE_TYPE_EPISODES ||
          node == NODE_TYPE_SEASONS                          ||
          node == NODE_TYPE_RECENTLY_ADDED_EPISODES)
      {
        CLog::Log(LOGDEBUG, "WindowVideoNav::GetDirectory");
        // grab the show thumb
        CVideoInfoTag details;
        m_database.GetTvShowInfo("", details, params.GetTvShowId());
        CFileItem showItem(details.m_strShowPath, true);
        if (showItem.GetCachedVideoThumb())
          items.SetProperty("tvshowthumb", showItem.GetCachedVideoThumb());

        // Grab fanart data
        items.SetProperty("fanart_color1", details.m_fanart.GetColor(0));
        items.SetProperty("fanart_color2", details.m_fanart.GetColor(1));
        items.SetProperty("fanart_color3", details.m_fanart.GetColor(2));
        if (showItem.CacheLocalFanart())
          items.SetProperty("fanart_image", showItem.GetCachedFanart());

        // save the show description (showplot)
        items.SetProperty("showplot", details.m_strPlot);

        // set the season thumb
        CStdString strLabel;
        if (params.GetSeason() == 0)
          strLabel = g_localizeStrings.Get(20381);
        else
          strLabel.Format(g_localizeStrings.Get(20358), params.GetSeason());

        CFileItem item(strLabel);
        item.SetPath(URIUtils::GetParentPath(items.GetPath()));
        item.m_bIsFolder = true;
        item.SetCachedSeasonThumb();
        if (item.HasThumbnail())
          items.SetProperty("seasonthumb",item.GetThumbnailImage());

        // the container folder thumb is the parent (i.e. season or show)
        if (node == NODE_TYPE_EPISODES || node == NODE_TYPE_RECENTLY_ADDED_EPISODES)
        {
          items.SetContent("episodes");
          // grab the season thumb as the folder thumb
          CStdString strLabel;
          CStdString strPath;
          if (params.GetSeason() <= -1 && items.Size() > 0)
          {
            CQueryParams params2;
            dir.GetQueryParams(items[0]->GetPath(),params2);
            strLabel.Format(g_localizeStrings.Get(20358), params2.GetSeason());
            strPath = URIUtils::GetParentPath(items.GetPath());
          }
          else
          {
            if (params.GetSeason() == 0)
              strLabel = g_localizeStrings.Get(20381);
            else
              strLabel.Format(g_localizeStrings.Get(20358), params.GetSeason());
            strPath = items.GetPath();
          }

          CFileItem item(strPath, true);
          item.SetLabel(strLabel);
          item.GetVideoInfoTag()->m_strPath = showItem.GetPath();
          item.SetCachedSeasonThumb();

          items.SetThumbnailImage(item.GetThumbnailImage());
          items.SetProperty("seasonthumb",item.GetThumbnailImage());
        }
        else
        {
          items.SetContent("seasons");
          items.SetThumbnailImage(showItem.GetThumbnailImage());
        }
      }
      else if (node == NODE_TYPE_TITLE_MOVIES ||
               node == NODE_TYPE_SETS ||
               node == NODE_TYPE_RECENTLY_ADDED_MOVIES)
        items.SetContent("movies");
      else if (node == NODE_TYPE_TITLE_TVSHOWS)
        items.SetContent("tvshows");
      else if (node == NODE_TYPE_TITLE_MUSICVIDEOS ||
               node == NODE_TYPE_RECENTLY_ADDED_MUSICVIDEOS)
        items.SetContent("musicvideos");
      else if (node == NODE_TYPE_GENRE)
        items.SetContent("genres");
      else if (node == NODE_TYPE_COUNTRY)
        items.SetContent("countries");
      else if (node == NODE_TYPE_ACTOR)
      {
        if (params.GetContentType() == VIDEODB_CONTENT_MUSICVIDEOS)
          items.SetContent("artists");
        else
          items.SetContent("actors");
      }
      else if (node == NODE_TYPE_DIRECTOR)
        items.SetContent("directors");
      else if (node == NODE_TYPE_STUDIO)
        items.SetContent("studios");
      else if (node == NODE_TYPE_YEAR)
        items.SetContent("years");
      else if (node == NODE_TYPE_MUSICVIDEOS_ALBUM)
        items.SetContent("albums");
      else
        items.SetContent("");
    }
    else if (!items.IsVirtualDirectoryRoot())
    { // load info from the database
      CStdString label;
      if (items.GetLabel().IsEmpty() && m_rootDir.IsSource(items.GetPath(), g_settings.GetSourcesFromType("video"), &label)) 
        items.SetLabel(label);
      LoadVideoInfo(items);
    }
  }
  return bResult;
}

void CGUIWindowVideoNav::LoadVideoInfo(CFileItemList &items)
{
  // TODO: this could possibly be threaded as per the music info loading,
  //       we could also cache the info
  if (!items.GetContent().IsEmpty())
    return; // don't load for listings that have content set

  CStdString content = m_database.GetContentForPath(items.GetPath());
  items.SetContent(content.IsEmpty() ? "files" : content);

  /*
    If we have a matching item in the library, so we can assign the metadata to it. In addition, we can choose
    * whether the item is stacked down (eg in the case of folders representing a single item)
    * whether or not we assign the library's labels to the item, or leave the item as is.

    As certain users (read: certain developers) don't want either of these to occur, we compromise by stacking
    items down only if stacking is available and enabled.

    Similarly, we assign the "clean" library labels to the item only if the "Replace filenames with library titles"
    setting is enabled.
    */
  bool stackItems    = items.GetProperty("isstacked").asBoolean() || (StackingAvailable(items) && g_settings.m_videoStacking);
  bool replaceLabels = g_guiSettings.GetBool("myvideos.replacelabels");

  CFileItemList dbItems;
  /* NOTE: In the future when GetItemsForPath returns all items regardless of whether they're "in the library"
           we won't need the fetchedPlayCounts code, and can "simply" do this directly on absense of content. */
  bool fetchedPlayCounts = false;
  if (!content.IsEmpty())
  {
    m_database.GetItemsForPath(content, items.GetPath(), dbItems);
    dbItems.SetFastLookup(true);
  }

  for (int i = 0; i < items.Size(); i++)
  {
    CFileItemPtr pItem = items[i];
    CFileItemPtr match;
    if (!content.IsEmpty()) /* optical media will be stacked down, so it's path won't match the base path */
      match = dbItems.Get(pItem->IsOpticalMediaFile() ? pItem->GetLocalMetadataPath() : pItem->GetPath());
    if (match)
    {
      pItem->UpdateInfo(*match, replaceLabels);

      if (stackItems)
      {
        if (match->m_bIsFolder)
          pItem->SetPath(match->GetVideoInfoTag()->m_strPath);
        else
          pItem->SetPath(match->GetVideoInfoTag()->m_strFileNameAndPath);
        // if we switch from a file to a folder item it means we really shouldn't be sorting files and
        // folders separately
        if (pItem->m_bIsFolder != match->m_bIsFolder)
        {
          items.SetSortIgnoreFolders(true);
          pItem->m_bIsFolder = match->m_bIsFolder;
        }
      }
    }
    else
    {
      /* NOTE: Currently we GetPlayCounts on our items regardless of whether content is set
                as if content is set, GetItemsForPaths doesn't return anything not in the content tables.
                This code can be removed once the content tables are always filled */
      if (!pItem->m_bIsFolder && !fetchedPlayCounts)
      {
        m_database.GetPlayCounts(items.GetPath(), items);
        fetchedPlayCounts = true;
      }
      
      // set the watched overlay
      if (pItem->IsVideo())
        pItem->SetOverlayImage(CGUIListItem::ICON_OVERLAY_UNWATCHED, pItem->HasVideoInfoTag() && pItem->GetVideoInfoTag()->m_playCount > 0);

      // Since the item is not in our db but the user wants a clean label we should clean it up (stacking may do some cleaning as well)
      if (replaceLabels)
        pItem->CleanString();
    }
  }
}

void CGUIWindowVideoNav::UpdateButtons()
{
  CGUIWindowVideoBase::UpdateButtons();

  // Update object count
  int iItems = m_vecItems->Size();
  if (iItems)
  {
    // check for parent dir and "all" items
    // should always be the first two items
    for (int i = 0; i <= (iItems>=2 ? 1 : 0); i++)
    {
      CFileItemPtr pItem = m_vecItems->Get(i);
      if (pItem->IsParentFolder()) iItems--;
      if (pItem->GetPath().Left(4).Equals("/-1/")) iItems--;
    }
    // or the last item
    if (m_vecItems->Size() > 2 &&
      m_vecItems->Get(m_vecItems->Size()-1)->GetPath().Left(4).Equals("/-1/"))
      iItems--;
  }
  CStdString items;
  items.Format("%i %s", iItems, g_localizeStrings.Get(127).c_str());
  SET_CONTROL_LABEL(CONTROL_LABELFILES, items);

  // set the filter label
  CStdString strLabel;

  // "Playlists"
  if (m_vecItems->GetPath().Equals("special://videoplaylists/"))
    strLabel = g_localizeStrings.Get(136);
  // "{Playlist Name}"
  else if (m_vecItems->IsPlayList())
  {
    // get playlist name from path
    CStdString strDummy;
    URIUtils::Split(m_vecItems->GetPath(), strDummy, strLabel);
  }
  else if (m_vecItems->GetPath().Equals("sources://video/"))
    strLabel = g_localizeStrings.Get(744);
  // everything else is from a videodb:// path
  else if (m_vecItems->IsVideoDb())
  {
    CVideoDatabaseDirectory dir;
    dir.GetLabel(m_vecItems->GetPath(), strLabel);
  }
  else
    strLabel = URIUtils::GetFileName(m_vecItems->GetPath());

  SET_CONTROL_LABEL(CONTROL_FILTER, strLabel);

  int watchMode = g_settings.GetWatchMode(m_vecItems->GetContent());
  SET_CONTROL_LABEL(CONTROL_BTNSHOWMODE, g_localizeStrings.Get(16100 + watchMode));

  SET_CONTROL_SELECTED(GetID(), CONTROL_BTNSHOWALL, watchMode != VIDEO_SHOW_ALL);

  SET_CONTROL_SELECTED(GetID(),CONTROL_BTNPARTYMODE, g_partyModeManager.IsEnabled());

  SET_CONTROL_SELECTED(GetID(),CONTROL_BTNFLATTEN, g_settings.m_bMyVideoNavFlatten);
}

/// \brief Search for names, genres, artists, directors, and plots with search string \e strSearch in the
/// \brief video databases and return the found \e items
/// \param strSearch The search string
/// \param items Items Found
void CGUIWindowVideoNav::DoSearch(const CStdString& strSearch, CFileItemList& items)
{
  CFileItemList tempItems;
  CStdString strGenre = g_localizeStrings.Get(515); // Genre
  CStdString strActor = g_localizeStrings.Get(20337); // Actor
  CStdString strDirector = g_localizeStrings.Get(20339); // Director
  CStdString strMovie = g_localizeStrings.Get(20338); // Movie

  //get matching names
  m_database.GetMoviesByName(strSearch, tempItems);
  AppendAndClearSearchItems(tempItems, "[" + g_localizeStrings.Get(20338) + "] ", items);

  m_database.GetEpisodesByName(strSearch, tempItems);
  AppendAndClearSearchItems(tempItems, "[" + g_localizeStrings.Get(20359) + "] ", items);

  m_database.GetTvShowsByName(strSearch, tempItems);
  AppendAndClearSearchItems(tempItems, "[" + g_localizeStrings.Get(20364) + "] ", items);

  m_database.GetMusicVideosByName(strSearch, tempItems);
  AppendAndClearSearchItems(tempItems, "[" + g_localizeStrings.Get(20391) + "] ", items);

  m_database.GetMusicVideosByAlbum(strSearch, tempItems);
  AppendAndClearSearchItems(tempItems, "[" + g_localizeStrings.Get(558) + "] ", items);
  
  // get matching genres
  m_database.GetMovieGenresByName(strSearch, tempItems);
  AppendAndClearSearchItems(tempItems, "[" + strGenre + " - " + g_localizeStrings.Get(20342) + "] ", items);

  m_database.GetTvShowGenresByName(strSearch, tempItems);
  AppendAndClearSearchItems(tempItems, "[" + strGenre + " - " + g_localizeStrings.Get(20343) + "] ", items);

  m_database.GetMusicVideoGenresByName(strSearch, tempItems);
  AppendAndClearSearchItems(tempItems, "[" + strGenre + " - " + g_localizeStrings.Get(20389) + "] ", items);

  //get actors/artists
  m_database.GetMovieActorsByName(strSearch, tempItems);
  AppendAndClearSearchItems(tempItems, "[" + strActor + " - " + g_localizeStrings.Get(20342) + "] ", items);

  m_database.GetTvShowsActorsByName(strSearch, tempItems);
  AppendAndClearSearchItems(tempItems, "[" + strActor + " - " + g_localizeStrings.Get(20343) + "] ", items);

  m_database.GetMusicVideoArtistsByName(strSearch, tempItems);
  AppendAndClearSearchItems(tempItems, "[" + strActor + " - " + g_localizeStrings.Get(20389) + "] ", items);

  //directors
  m_database.GetMovieDirectorsByName(strSearch, tempItems);
  AppendAndClearSearchItems(tempItems, "[" + strDirector + " - " + g_localizeStrings.Get(20342) + "] ", items);

  m_database.GetTvShowsDirectorsByName(strSearch, tempItems);
  AppendAndClearSearchItems(tempItems, "[" + strDirector + " - " + g_localizeStrings.Get(20343) + "] ", items);

  m_database.GetMusicVideoDirectorsByName(strSearch, tempItems);
  AppendAndClearSearchItems(tempItems, "[" + strDirector + " - " + g_localizeStrings.Get(20389) + "] ", items);

  //plot
  m_database.GetEpisodesByPlot(strSearch, tempItems);
  AppendAndClearSearchItems(tempItems, "[" + g_localizeStrings.Get(20365) + "] ", items);

  m_database.GetMoviesByPlot(strSearch, tempItems);
  AppendAndClearSearchItems(tempItems, "[" + strMovie + " " + g_localizeStrings.Get(207) + "] ", items);
}

void CGUIWindowVideoNav::PlayItem(int iItem)
{
  // unlike additemtoplaylist, we need to check the items here
  // before calling it since the current playlist will be stopped
  // and cleared!

  // root is not allowed
  if (m_vecItems->IsVirtualDirectoryRoot())
    return;

  CGUIWindowVideoBase::PlayItem(iItem);
}

void CGUIWindowVideoNav::OnInfo(CFileItem* pItem, ADDON::ScraperPtr& scraper)
{
  m_database.Open(); // since we can be called from the music library without being inited
  if (pItem->IsVideoDb())
    scraper = m_database.GetScraperForPath(pItem->GetVideoInfoTag()->m_strPath);
  else
  {
    CStdString strPath,strFile;
    URIUtils::Split(pItem->GetPath(),strPath,strFile);
    scraper = m_database.GetScraperForPath(strPath);
  }
  m_database.Close();
  CGUIWindowVideoBase::OnInfo(pItem,scraper);
}

bool CGUIWindowVideoNav::CanDelete(const CStdString& strPath)
{
  CQueryParams params;
  CVideoDatabaseDirectory::GetQueryParams(strPath,params);

  if (params.GetMovieId()   != -1 ||
      params.GetEpisodeId() != -1 ||
      params.GetMVideoId()  != -1 ||
      (params.GetTvShowId() != -1 && params.GetSeason() <= -1
              && !CVideoDatabaseDirectory::IsAllItem(strPath)))
    return true;

  return false;
}

void CGUIWindowVideoNav::OnDeleteItem(CFileItemPtr pItem)
{
  if (m_vecItems->IsParentFolder())
    return;

  if (!m_vecItems->IsVideoDb() && !pItem->IsVideoDb())
  {
    if (!pItem->GetPath().Equals("newsmartplaylist://video") &&
        !pItem->GetPath().Equals("special://videoplaylists/") &&
        !pItem->GetPath().Equals("sources://video/"))
      CGUIWindowVideoBase::OnDeleteItem(pItem);
  }
  else if (pItem->GetPath().Left(14).Equals("videodb://1/7/") &&
           pItem->GetPath().size() > 14 && pItem->m_bIsFolder)
  {
    CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*)g_windowManager.GetWindow(WINDOW_DIALOG_YES_NO);
    pDialog->SetLine(0, g_localizeStrings.Get(432));
    CStdString strLabel;
    strLabel.Format(g_localizeStrings.Get(433),pItem->GetLabel());
    pDialog->SetLine(1, strLabel);
    pDialog->SetLine(2, "");;
    pDialog->DoModal();
    if (pDialog->IsConfirmed())
    {
      CFileItemList items;
      CDirectory::GetDirectory(pItem->GetPath(),items,"",false,false,DIR_CACHE_ONCE,true,true);
      for (int i=0;i<items.Size();++i)
        OnDeleteItem(items[i]);

       CVideoDatabaseDirectory dir;
       CQueryParams params;
       dir.GetQueryParams(pItem->GetPath(),params);
       m_database.DeleteSet(params.GetSetId());
    }
  }
  else if (m_vecItems->GetPath().Equals(CUtil::VideoPlaylistsLocation()) ||
           m_vecItems->GetPath().Equals("special://videoplaylists/"))
  {
    pItem->m_bIsFolder = false;
    CFileUtils::DeleteItem(pItem);
  }
  else
  {
    if (!DeleteItem(pItem.get()))
      return;

    CStdString strDeletePath;
    if (pItem->m_bIsFolder)
      strDeletePath=pItem->GetVideoInfoTag()->m_strPath;
    else
      strDeletePath=pItem->GetVideoInfoTag()->m_strFileNameAndPath;

    if (URIUtils::GetFileName(strDeletePath).Equals("VIDEO_TS.IFO"))
    {
      URIUtils::GetDirectory(strDeletePath.Mid(0),strDeletePath);
      if (strDeletePath.Right(9).Equals("VIDEO_TS/"))
      {
        URIUtils::RemoveSlashAtEnd(strDeletePath);
        URIUtils::GetDirectory(strDeletePath.Mid(0),strDeletePath);
      }
    }
    if (URIUtils::HasSlashAtEnd(strDeletePath))
      pItem->m_bIsFolder=true;

    if (g_guiSettings.GetBool("filelists.allowfiledeletion") &&
        CUtil::SupportsFileOperations(strDeletePath))
    {
      pItem->SetPath(strDeletePath);
      CGUIWindowVideoBase::OnDeleteItem(pItem);
    }
  }

  CUtil::DeleteVideoDatabaseDirectoryCache();
}

bool CGUIWindowVideoNav::DeleteItem(CFileItem* pItem, bool bUnavailable /* = false */)
{
  if (!pItem->HasVideoInfoTag() || !CanDelete(pItem->GetPath()))
    return false;

  VIDEODB_CONTENT_TYPE iType=VIDEODB_CONTENT_MOVIES;
  if (pItem->HasVideoInfoTag() && !pItem->GetVideoInfoTag()->m_strShowTitle.IsEmpty())
    iType = VIDEODB_CONTENT_TVSHOWS;
  if (pItem->HasVideoInfoTag() && pItem->GetVideoInfoTag()->m_iSeason > -1 && !pItem->m_bIsFolder)
    iType = VIDEODB_CONTENT_EPISODES;
  if (pItem->HasVideoInfoTag() && !pItem->GetVideoInfoTag()->m_strArtist.IsEmpty())
    iType = VIDEODB_CONTENT_MUSICVIDEOS;

  // dont allow update while scanning
  CGUIDialogVideoScan* pDialogScan = (CGUIDialogVideoScan*)g_windowManager.GetWindow(WINDOW_DIALOG_VIDEO_SCAN);
  if (pDialogScan && pDialogScan->IsScanning())
  {
    CGUIDialogOK::ShowAndGetInput(257, 0, 14057, 0);
    return false;
  }


  CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*)g_windowManager.GetWindow(WINDOW_DIALOG_YES_NO);
  if (!pDialog)
    return false;
  if (iType == VIDEODB_CONTENT_MOVIES)
    pDialog->SetHeading(432);
  if (iType == VIDEODB_CONTENT_EPISODES)
    pDialog->SetHeading(20362);
  if (iType == VIDEODB_CONTENT_TVSHOWS)
    pDialog->SetHeading(20363);
  if (iType == VIDEODB_CONTENT_MUSICVIDEOS)
    pDialog->SetHeading(20392);

  if(bUnavailable)
  {
    pDialog->SetLine(0, g_localizeStrings.Get(662));
    pDialog->SetLine(1, g_localizeStrings.Get(663));
    pDialog->SetLine(2, "");;
    pDialog->DoModal();
  }
  else
  {
    CStdString strLine;
    strLine.Format(g_localizeStrings.Get(433),pItem->GetLabel());
    pDialog->SetLine(0, strLine);
    pDialog->SetLine(1, "");
    pDialog->SetLine(2, "");;
    pDialog->DoModal();
  }

  if (!pDialog->IsConfirmed())
    return false;

  CStdString path;
  CVideoDatabase database;
  database.Open();

  database.GetFilePathById(pItem->GetVideoInfoTag()->m_iDbId, path, iType);
  if (path.IsEmpty())
    return false;
  if (iType == VIDEODB_CONTENT_MOVIES)
    database.DeleteMovie(path);
  if (iType == VIDEODB_CONTENT_EPISODES)
    database.DeleteEpisode(path, pItem->GetVideoInfoTag()->m_iDbId);
  if (iType == VIDEODB_CONTENT_TVSHOWS)
    database.DeleteTvShow(path);
  if (iType == VIDEODB_CONTENT_MUSICVIDEOS)
    database.DeleteMusicVideo(path);

  if (iType == VIDEODB_CONTENT_TVSHOWS)
    database.SetPathHash(path,"");
  else
  {
    CStdString strDirectory;
    URIUtils::GetDirectory(path,strDirectory);
    database.SetPathHash(strDirectory,"");
  }

  return true;
}

void CGUIWindowVideoNav::OnPrepareFileItems(CFileItemList &items)
{
  CGUIWindowVideoBase::OnPrepareFileItems(items);

  // set fanart
  CQueryParams params;
  CVideoDatabaseDirectory dir;
  dir.GetQueryParams(items.GetPath(),params);
  if (params.GetContentType() == VIDEODB_CONTENT_MUSICVIDEOS)
    CGUIWindowMusicNav::SetupFanart(items);

  NODE_TYPE node = dir.GetDirectoryChildType(items.GetPath());

  // now filter as necessary
  bool filterWatched=false;
  if (node == NODE_TYPE_EPISODES
  ||  node == NODE_TYPE_SEASONS
  ||  node == NODE_TYPE_SETS
  ||  node == NODE_TYPE_TITLE_MOVIES
  ||  node == NODE_TYPE_TITLE_TVSHOWS
  ||  node == NODE_TYPE_TITLE_MUSICVIDEOS
  ||  node == NODE_TYPE_RECENTLY_ADDED_EPISODES
  ||  node == NODE_TYPE_RECENTLY_ADDED_MOVIES
  ||  node == NODE_TYPE_RECENTLY_ADDED_MUSICVIDEOS)
    filterWatched = true;
  if (!items.IsVideoDb())
    filterWatched = true;
  if (items.IsSmartPlayList() && items.GetContent() == "tvshows")
    node = NODE_TYPE_TITLE_TVSHOWS; // so that the check below works

  int watchMode = g_settings.GetWatchMode(m_vecItems->GetContent());

  for (int i = 0; i < items.Size(); i++)
  {
    CFileItemPtr item = items.Get(i);
    if(item->HasVideoInfoTag() && (node == NODE_TYPE_TITLE_TVSHOWS || node == NODE_TYPE_SEASONS))
    {
      if (watchMode == VIDEO_SHOW_UNWATCHED)
        item->GetVideoInfoTag()->m_iEpisode = (int)item->GetProperty("unwatchedepisodes").asInteger();
      if (watchMode == VIDEO_SHOW_WATCHED)
        item->GetVideoInfoTag()->m_iEpisode = (int)item->GetProperty("watchedepisodes").asInteger();
      item->SetProperty("numepisodes", item->GetVideoInfoTag()->m_iEpisode);
    }

    if(filterWatched)
    {
      if((watchMode==VIDEO_SHOW_WATCHED   && item->GetVideoInfoTag()->m_playCount== 0)
      || (watchMode==VIDEO_SHOW_UNWATCHED && item->GetVideoInfoTag()->m_playCount > 0))
      {
        items.Remove(i);
        i--;
      }
    }
  }
}

void CGUIWindowVideoNav::GetContextButtons(int itemNumber, CContextButtons &buttons)
{
  CFileItemPtr item;
  if (itemNumber >= 0 && itemNumber < m_vecItems->Size())
    item = m_vecItems->Get(itemNumber);

  CGUIWindowVideoBase::GetContextButtons(itemNumber, buttons);

  if (item && item->GetProperty("pluginreplacecontextitems").asBoolean())
    return;

  CVideoDatabaseDirectory dir;
  NODE_TYPE node = dir.GetDirectoryChildType(m_vecItems->GetPath());

  if (!item)
  {
    CGUIDialogVideoScan *pScanDlg = (CGUIDialogVideoScan *)g_windowManager.GetWindow(WINDOW_DIALOG_VIDEO_SCAN);
    if (pScanDlg && pScanDlg->IsScanning())
      buttons.Add(CONTEXT_BUTTON_STOP_SCANNING, 13353);
    else
      buttons.Add(CONTEXT_BUTTON_UPDATE_LIBRARY, 653);
  }
  else if (m_vecItems->GetPath().Equals("sources://video/"))
  {
    // get the usual shares
    CGUIDialogContextMenu::GetContextButtons("video", item, buttons);
    // add scan button somewhere here
    CGUIDialogVideoScan *pScanDlg = (CGUIDialogVideoScan *)g_windowManager.GetWindow(WINDOW_DIALOG_VIDEO_SCAN);
    if (pScanDlg && pScanDlg->IsScanning())
      buttons.Add(CONTEXT_BUTTON_STOP_SCANNING, 13353);  // Stop Scanning
    if (!item->IsDVD() && item->GetPath() != "add" &&
        (g_settings.GetCurrentProfile().canWriteDatabases() || g_passwordManager.bMasterUser))
    {
      CVideoDatabase database;
      database.Open();
      ADDON::ScraperPtr info = database.GetScraperForPath(item->GetPath());

      if (!pScanDlg || (pScanDlg && !pScanDlg->IsScanning()))
      {
        if (!item->IsLiveTV() && !item->IsPlugin() && !item->IsAddonsPath())
        {
          if (info && info->Content() != CONTENT_NONE)
            buttons.Add(CONTEXT_BUTTON_SET_CONTENT, 20442);
          else
            buttons.Add(CONTEXT_BUTTON_SET_CONTENT, 20333);
        }
      }

      if (info && (!pScanDlg || (pScanDlg && !pScanDlg->IsScanning())))
        buttons.Add(CONTEXT_BUTTON_SCAN, 13349);
    }
  }
  else
  {
    // are we in the playlists location?
    bool inPlaylists = m_vecItems->GetPath().Equals(CUtil::VideoPlaylistsLocation()) ||
                       m_vecItems->GetPath().Equals("special://videoplaylists/");

    if (item->HasVideoInfoTag() && !item->GetVideoInfoTag()->m_strArtist.IsEmpty())
    {
      CMusicDatabase database;
      database.Open();
      if (database.GetArtistByName(item->GetVideoInfoTag()->m_strArtist) > -1)
        buttons.Add(CONTEXT_BUTTON_GO_TO_ARTIST, 20396);
    }
    if (item->HasVideoInfoTag() && item->GetVideoInfoTag()->m_strAlbum.size() > 0)
    {
      CMusicDatabase database;
      database.Open();
      if (database.GetAlbumByName(item->GetVideoInfoTag()->m_strAlbum) > -1)
        buttons.Add(CONTEXT_BUTTON_GO_TO_ALBUM, 20397);
    }
    if (item->HasVideoInfoTag() && item->GetVideoInfoTag()->m_strAlbum.size() > 0 &&
        item->GetVideoInfoTag()->m_strArtist.size() > 0                           &&
        item->GetVideoInfoTag()->m_strTitle.size() > 0)
    {
      CMusicDatabase database;
      database.Open();
      if (database.GetSongByArtistAndAlbumAndTitle(item->GetVideoInfoTag()->m_strArtist,
                                                   item->GetVideoInfoTag()->m_strAlbum,
                                                   item->GetVideoInfoTag()->m_strTitle) > -1)
      {
        buttons.Add(CONTEXT_BUTTON_PLAY_OTHER, 20398);
      }
    }
    if (!item->IsParentFolder())
    {
      ADDON::ScraperPtr info;
      VIDEO::SScanSettings settings;
      GetScraperForItem(item.get(), info, settings);

      if (info && info->Content() == CONTENT_TVSHOWS)
        buttons.Add(CONTEXT_BUTTON_INFO, item->m_bIsFolder ? 20351 : 20352);
      else if (info && info->Content() == CONTENT_MUSICVIDEOS)
        buttons.Add(CONTEXT_BUTTON_INFO,20393);
      else if (info && info->Content() == CONTENT_MOVIES)
        buttons.Add(CONTEXT_BUTTON_INFO, 13346);

      // can we update the database?
      if (g_settings.GetCurrentProfile().canWriteDatabases() || g_passwordManager.bMasterUser)
      {
        if (node == NODE_TYPE_TITLE_TVSHOWS)
        {
          CGUIDialogVideoScan *pScanDlg = (CGUIDialogVideoScan *)g_windowManager.GetWindow(WINDOW_DIALOG_VIDEO_SCAN);
          if (pScanDlg && pScanDlg->IsScanning())
            buttons.Add(CONTEXT_BUTTON_STOP_SCANNING, 13353);
          else
            buttons.Add(CONTEXT_BUTTON_UPDATE_TVSHOW, 13349);
        }
        if (!item->IsPlugin() && !item->IsScript() && !item->IsLiveTV() && !item->IsAddonsPath() &&
             item->GetPath() != "sources://video/" && item->GetPath() != "special://videoplaylists/")
        {
          if (item->m_bIsFolder)
          {
            // Have both options for folders since we don't know whether all childs are watched/unwatched
            buttons.Add(CONTEXT_BUTTON_MARK_UNWATCHED, 16104); //Mark as UnWatched
            buttons.Add(CONTEXT_BUTTON_MARK_WATCHED, 16103);   //Mark as Watched
          }
          else
          {
            if (item->GetOverlayImage().Equals("OverlayWatched.png"))
              buttons.Add(CONTEXT_BUTTON_MARK_UNWATCHED, 16104); //Mark as UnWatched
            else
              buttons.Add(CONTEXT_BUTTON_MARK_WATCHED, 16103);   //Mark as Watched
          }
        }
        if ((node == NODE_TYPE_TITLE_TVSHOWS) ||
            (item->IsVideoDb() && item->HasVideoInfoTag() && !item->m_bIsFolder))
        {
          buttons.Add(CONTEXT_BUTTON_EDIT, 16105); //Edit Title
        }
        if (m_database.HasContent(VIDEODB_CONTENT_TVSHOWS) && item->HasVideoInfoTag() &&
           !item->m_bIsFolder && item->GetVideoInfoTag()->m_iEpisode == -1 &&
            item->GetVideoInfoTag()->m_strArtist.IsEmpty() && item->GetVideoInfoTag()->m_iDbId >= 0) // movie entry
        {
          if (m_database.IsLinkedToTvshow(item->GetVideoInfoTag()->m_iDbId))
            buttons.Add(CONTEXT_BUTTON_UNLINK_MOVIE,20385);
          buttons.Add(CONTEXT_BUTTON_LINK_MOVIE,20384);
        }

        if (node == NODE_TYPE_SEASONS && item->m_bIsFolder)
          buttons.Add(CONTEXT_BUTTON_SET_SEASON_THUMB, 20371);

        if (item->GetPath().Left(14).Equals("videodb://1/7/") && item->GetPath().size() > 14 && item->m_bIsFolder) // sets
        {
          buttons.Add(CONTEXT_BUTTON_EDIT, 16105);
          buttons.Add(CONTEXT_BUTTON_SET_MOVIESET_THUMB, 20435);
          buttons.Add(CONTEXT_BUTTON_SET_MOVIESET_FANART, 20456);
          buttons.Add(CONTEXT_BUTTON_DELETE, 646);
        }

        if (node == NODE_TYPE_ACTOR && !dir.IsAllItem(item->GetPath()) && item->m_bIsFolder)
        {
          if (m_vecItems->GetPath().Left(11).Equals("videodb://3")) // mvids
            buttons.Add(CONTEXT_BUTTON_SET_ARTIST_THUMB, 13359);
          else
            buttons.Add(CONTEXT_BUTTON_SET_ACTOR_THUMB, 20403);
        }
        if (item->IsVideoDb() && item->HasVideoInfoTag() &&
          (!item->m_bIsFolder || node == NODE_TYPE_TITLE_TVSHOWS))
        {
          if (info && info->Content() == CONTENT_TVSHOWS)
          {
            if(item->GetVideoInfoTag()->m_iBookmarkId != -1 &&
               item->GetVideoInfoTag()->m_iBookmarkId != 0)
            {
              buttons.Add(CONTEXT_BUTTON_UNLINK_BOOKMARK, 20405);
            }
          }
          buttons.Add(CONTEXT_BUTTON_DELETE, 646);
        }

        // this should ideally be non-contextual (though we need some context for non-tv show node I guess)
        CGUIDialogVideoScan *pScanDlg = (CGUIDialogVideoScan *)g_windowManager.GetWindow(WINDOW_DIALOG_VIDEO_SCAN);
        if (pScanDlg && pScanDlg->IsScanning())
        {
          if (node != NODE_TYPE_TITLE_TVSHOWS)
            buttons.Add(CONTEXT_BUTTON_STOP_SCANNING, 13353);
        }
        else
          buttons.Add(CONTEXT_BUTTON_UPDATE_LIBRARY, 653);
      }

      if (!m_vecItems->IsVideoDb() && !m_vecItems->IsVirtualDirectoryRoot())
      { // non-video db items, file operations are allowed
        if ((g_guiSettings.GetBool("filelists.allowfiledeletion") &&
            CUtil::SupportsFileOperations(item->GetPath())) ||
            (inPlaylists && !URIUtils::GetFileName(item->GetPath()).Equals("PartyMode-Video.xsp")
                         && (item->IsPlayList() || item->IsSmartPlayList())))
        {
          buttons.Add(CONTEXT_BUTTON_DELETE, 117);
          buttons.Add(CONTEXT_BUTTON_RENAME, 118);
        }
        // add "Set/Change content" to folders
        if (item->m_bIsFolder && !item->IsPlayList() && !item->IsSmartPlayList() && !item->IsLiveTV() && !item->IsPlugin() && !item->IsAddonsPath())
        {
          CGUIDialogVideoScan *pScanDlg = (CGUIDialogVideoScan *)g_windowManager.GetWindow(WINDOW_DIALOG_VIDEO_SCAN);
          if (!pScanDlg || (pScanDlg && !pScanDlg->IsScanning()))
          {
            if (info && info->Content() != CONTENT_NONE)
            {
              buttons.Add(CONTEXT_BUTTON_SET_CONTENT, 20442);
              if (info && (!pScanDlg || (pScanDlg && !pScanDlg->IsScanning())))
                buttons.Add(CONTEXT_BUTTON_SCAN, 13349);
            }
            else
              buttons.Add(CONTEXT_BUTTON_SET_CONTENT, 20333);
          }
        }
      }
      if (item->IsPlugin() || item->IsScript() || m_vecItems->IsPlugin())
        buttons.Add(CONTEXT_BUTTON_PLUGIN_SETTINGS, 1045);
    }
  }
  CGUIWindowVideoBase::GetNonContextButtons(itemNumber, buttons);
}

bool CGUIWindowVideoNav::OnContextButton(int itemNumber, CONTEXT_BUTTON button)
{
  CFileItemPtr item;
  if (itemNumber >= 0 && itemNumber < m_vecItems->Size())
    item = m_vecItems->Get(itemNumber);
  if (CGUIDialogContextMenu::OnContextButton("video", item, button))
  {
    //TODO should we search DB for entries from plugins?
    if (button == CONTEXT_BUTTON_REMOVE_SOURCE && !item->IsPlugin()
        && !item->IsLiveTV() &&!item->IsRSS())
    {
      OnUnAssignContent(item->GetPath(),20375,20340,20341);
    }
    Update(m_vecItems->GetPath());
    return true;
  }
  switch (button)
  {
  case CONTEXT_BUTTON_EDIT:
    UpdateVideoTitle(item.get());
    CUtil::DeleteVideoDatabaseDirectoryCache();
    Update(m_vecItems->GetPath());
    return true;

  case CONTEXT_BUTTON_SET_SEASON_THUMB:
  case CONTEXT_BUTTON_SET_ACTOR_THUMB:
  case CONTEXT_BUTTON_SET_ARTIST_THUMB:
  case CONTEXT_BUTTON_SET_MOVIESET_THUMB:
    {
      // Grab the thumbnails from the web
      CStdString strPath;
      CFileItemList items;
      URIUtils::AddFileToFolder(g_advancedSettings.m_cachePath,"imdbthumbs",strPath);
      CFileItemPtr cacheItem(new CFileItem(strPath,true));
      CFileUtils::DeleteItem(cacheItem,true);
      XFILE::CDirectory::Create(strPath);
      CFileItemPtr noneitem(new CFileItem("thumb://None", false));
      CStdString cachedThumb = m_vecItems->Get(itemNumber)->GetCachedSeasonThumb();
      if (button == CONTEXT_BUTTON_SET_ACTOR_THUMB)
        cachedThumb = m_vecItems->Get(itemNumber)->GetCachedActorThumb();
      if (button == CONTEXT_BUTTON_SET_ARTIST_THUMB)
        cachedThumb = m_vecItems->Get(itemNumber)->GetCachedArtistThumb();
      if (button == CONTEXT_BUTTON_SET_MOVIESET_THUMB)
        cachedThumb = m_vecItems->Get(itemNumber)->GetCachedVideoThumb();
      if (CFile::Exists(cachedThumb))
      {
        CFileItemPtr item(new CFileItem("thumb://Current", false));
        item->SetThumbnailImage(cachedThumb);
        item->SetLabel(g_localizeStrings.Get(20016));
        items.Add(item);
      }
      noneitem->SetIconImage("DefaultFolder.png");
      noneitem->SetLabel(g_localizeStrings.Get(20018));

      vector<CStdString> thumbs;
      if (button != CONTEXT_BUTTON_SET_ARTIST_THUMB)
      {
        CVideoInfoTag tag;
        if (button == CONTEXT_BUTTON_SET_SEASON_THUMB)
          m_database.GetTvShowInfo("",tag,m_vecItems->Get(itemNumber)->GetVideoInfoTag()->m_iDbId);
        else
          tag = *m_vecItems->Get(itemNumber)->GetVideoInfoTag();
        if (button == CONTEXT_BUTTON_SET_SEASON_THUMB)
          tag.m_strPictureURL.GetThumbURLs(thumbs, m_vecItems->Get(itemNumber)->GetVideoInfoTag()->m_iSeason);
        else
          tag.m_strPictureURL.GetThumbURLs(thumbs);

        for (unsigned int i = 0; i < thumbs.size(); i++)
        {
          CStdString strItemPath;
          strItemPath.Format("thumb://Remote%i",i);
          CFileItemPtr item(new CFileItem(strItemPath, false));
          item->SetThumbnailImage(thumbs[i]);
          item->SetIconImage("DefaultPicture.png");
          item->SetLabel(g_localizeStrings.Get(20015));
          items.Add(item);

          // TODO: Do we need to clear the cached image?
          //    CTextureCache::Get().ClearCachedImage(thumbs[i]);
        }
      }

      bool local=false;
      if (button == CONTEXT_BUTTON_SET_ARTIST_THUMB)
      {
        CStdString picturePath;

        CStdString strPath = m_vecItems->Get(itemNumber)->GetPath();
        URIUtils::RemoveSlashAtEnd(strPath);

        int nPos=strPath.ReverseFind("/");
        if (nPos>-1)
        {
          //  try to guess where the user should start
          //  browsing for the artist thumb
          CMusicDatabase database;
          database.Open();
          long idArtist=database.GetArtistByName(m_vecItems->Get(itemNumber)->GetLabel());
          database.GetArtistPath(idArtist, picturePath);
        }

        CStdString strThumb;
        URIUtils::AddFileToFolder(picturePath,"folder.jpg",strThumb);
        if (XFILE::CFile::Exists(strThumb))
        {
          CFileItemPtr pItem(new CFileItem(strThumb,false));
          pItem->SetLabel(g_localizeStrings.Get(20017));
          pItem->SetThumbnailImage(strThumb);
          items.Add(pItem);
          local = true;
        }
        else
          noneitem->SetIconImage("DefaultArtist.png");
      }

      if (button == CONTEXT_BUTTON_SET_ACTOR_THUMB)
      {
        CStdString picturePath;
        CStdString strThumb;
        URIUtils::AddFileToFolder(picturePath,"folder.jpg",strThumb);
        if (XFILE::CFile::Exists(strThumb))
        {
          CFileItemPtr pItem(new CFileItem(strThumb,false));
          pItem->SetLabel(g_localizeStrings.Get(20017));
          pItem->SetThumbnailImage(strThumb);
          items.Add(pItem);
          local = true;
        }
        else
          noneitem->SetIconImage("DefaultActor.png");
      }

      if (button == CONTEXT_BUTTON_SET_MOVIESET_THUMB)
        noneitem->SetIconImage("DefaultVideo.png");

      if (!local)
        items.Add(noneitem);

      VECSOURCES sources=g_settings.m_videoSources;
      g_mediaManager.GetLocalDrives(sources);
      CStdString result;
      if (!CGUIDialogFileBrowser::ShowAndGetImage(items, sources,
                                                  g_localizeStrings.Get(20019), result))
      {
        return false;   // user cancelled
      }

      if (result == "thumb://Current")
        return false;   // user chose the one they have

      // delete the thumbnail if that's what the user wants, else overwrite with the
      // new thumbnail
      CTextureCache::Get().ClearCachedImage(cachedThumb, true);
      if (result.Left(14) == "thumb://Remote")
      {
        int number = atoi(result.Mid(14));
        CFile::Cache(thumbs[number], cachedThumb);
      }
      if (result == "thumb://None")
        CTextureCache::Get().ClearCachedImage(cachedThumb, true);
      else
        CFile::Cache(result,cachedThumb);

      CUtil::DeleteVideoDatabaseDirectoryCache();
      CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_REFRESH_THUMBS);
      g_windowManager.SendMessage(msg);
      Update(m_vecItems->GetPath());

      return true;
    }
  case CONTEXT_BUTTON_SET_MOVIESET_FANART:
    {
      CFileItemList items;
      CStdString cachedFanart(item->GetCachedFanart());

      if (CFile::Exists(cachedFanart))
      {
        CFileItemPtr itemCurrent(new CFileItem("fanart://Current",false));
        itemCurrent->SetThumbnailImage(cachedFanart);
        itemCurrent->SetLabel(g_localizeStrings.Get(20440));
        items.Add(itemCurrent);
      }

      CStdString localFanart(item->GetLocalFanart());
      if (!localFanart.IsEmpty())
      {
        CFileItemPtr itemLocal(new CFileItem("fanart://Local",false));
        itemLocal->SetThumbnailImage(localFanart);
        itemLocal->SetLabel(g_localizeStrings.Get(20438));
        CTextureCache::Get().ClearCachedImage(localFanart);
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
        return false;

      CTextureCache::Get().ClearCachedImage(cachedFanart, true);

      if (result.Equals("fanart://Local"))
        result = localFanart;

      if (CFile::Exists(result))
      {
        if (flip)
          CPicture::ConvertFile(result, cachedFanart,0,1920,-1,100,true);
        else
          CPicture::CacheFanart(result, cachedFanart);
      }

      // clear view cache and reload images
      CUtil::DeleteVideoDatabaseDirectoryCache();

      if (CFile::Exists(cachedFanart))
        item->SetProperty("fanart_image", cachedFanart);
      else
        item->ClearProperty("fanart_image");

      Update(m_vecItems->GetPath());
      return true;
    }
  case CONTEXT_BUTTON_UPDATE_LIBRARY:
    {
      OnScan("",true);
      return true;
    }
  case CONTEXT_BUTTON_UNLINK_MOVIE:
    {
      OnLinkMovieToTvShow(itemNumber, true);
      Update(m_vecItems->GetPath());
      return true;
    }
  case CONTEXT_BUTTON_LINK_MOVIE:
    {
      OnLinkMovieToTvShow(itemNumber, false);
      return true;
    }
  case CONTEXT_BUTTON_GO_TO_ARTIST:
    {
      CStdString strPath;
      CMusicDatabase database;
      database.Open();
      strPath.Format("musicdb://2/%ld/",database.GetArtistByName(m_vecItems->Get(itemNumber)->GetVideoInfoTag()->m_strArtist));
      g_windowManager.ActivateWindow(WINDOW_MUSIC_NAV,strPath);
      return true;
    }
  case CONTEXT_BUTTON_GO_TO_ALBUM:
    {
      CStdString strPath;
      CMusicDatabase database;
      database.Open();
      strPath.Format("musicdb://3/%ld/",database.GetAlbumByName(m_vecItems->Get(itemNumber)->GetVideoInfoTag()->m_strAlbum));
      g_windowManager.ActivateWindow(WINDOW_MUSIC_NAV,strPath);
      return true;
    }
  case CONTEXT_BUTTON_PLAY_OTHER:
    {
      CMusicDatabase database;
      database.Open();
      CSong song;
      if (database.GetSongById(database.GetSongByArtistAndAlbumAndTitle(m_vecItems->Get(itemNumber)->GetVideoInfoTag()->m_strArtist,m_vecItems->Get(itemNumber)->GetVideoInfoTag()->m_strAlbum,
                                                                        m_vecItems->Get(itemNumber)->GetVideoInfoTag()->m_strTitle),
                                                                        song))
      {
        g_application.getApplicationMessenger().PlayFile(song);
      }
      return true;
    }

  case CONTEXT_BUTTON_UNLINK_BOOKMARK:
    {
      m_database.Open();
      m_database.DeleteBookMarkForEpisode(*m_vecItems->Get(itemNumber)->GetVideoInfoTag());
      m_database.Close();
      CUtil::DeleteVideoDatabaseDirectoryCache();
      Update(m_vecItems->GetPath());
      return true;
    }

  default:
    break;

  }
  return CGUIWindowVideoBase::OnContextButton(itemNumber, button);
}

void CGUIWindowVideoNav::OnLinkMovieToTvShow(int itemnumber, bool bRemove)
{
  CFileItemList list;
  if (bRemove)
  {
    vector<int> ids;
    if (!m_database.GetLinksToTvShow(m_vecItems->Get(itemnumber)->GetVideoInfoTag()->m_iDbId,ids))
      return;
    for (unsigned int i=0;i<ids.size();++i)
    {
      CVideoInfoTag tag;
      m_database.GetTvShowInfo("",tag,ids[i]);
      CFileItemPtr show(new CFileItem(tag));
      list.Add(show);
    }
  }
  else
  {
    m_database.GetTvShowsNav("videodb://2/2",list);

    // remove already linked shows
    vector<int> ids;
    if (!m_database.GetLinksToTvShow(m_vecItems->Get(itemnumber)->GetVideoInfoTag()->m_iDbId,ids))
      return;
    for (int i=0;i<list.Size();)
    {
      unsigned int j;
      for (j=0;j<ids.size();++j)
      {
        if (list[i]->GetVideoInfoTag()->m_iDbId == ids[j])
          break;
      }
      if (j == ids.size())
        i++;
      else
        list.Remove(i);
    }
  }
  int iSelectedLabel = 0;
  if (list.Size() > 1)
  {
    list.Sort(g_guiSettings.GetBool("filelists.ignorethewhensorting") ? SORT_METHOD_LABEL_IGNORE_THE : SORT_METHOD_LABEL, SORT_ORDER_ASC);
    CGUIDialogSelect* pDialog = (CGUIDialogSelect*)g_windowManager.GetWindow(WINDOW_DIALOG_SELECT);
    pDialog->Reset();
    pDialog->SetItems(&list);
    pDialog->SetHeading(20356);
    pDialog->DoModal();
    iSelectedLabel = pDialog->GetSelectedLabel();
  }
  if (iSelectedLabel > -1)
  {
    m_database.LinkMovieToTvshow(m_vecItems->Get(itemnumber)->GetVideoInfoTag()->m_iDbId,
                                 list[iSelectedLabel]->GetVideoInfoTag()->m_iDbId, bRemove);
    CUtil::DeleteVideoDatabaseDirectoryCache();
  }
}

bool CGUIWindowVideoNav::OnClick(int iItem)
{
  CFileItemPtr item = m_vecItems->Get(iItem);
  if (!item->m_bIsFolder && item->IsVideoDb() && !item->Exists())
  {
    CLog::Log(LOGDEBUG, "%s called on '%s' but file doesn't exist", __FUNCTION__, item->GetPath().c_str());
    if (!DeleteItem(item.get(), true))
      return true;

    // update list
    m_vecItems->RemoveDiscCache(GetID());
    Update(m_vecItems->GetPath());
    m_viewControl.SetSelectedItem(iItem);
    return true;
  }

  return CGUIWindowVideoBase::OnClick(iItem);
}

CStdString CGUIWindowVideoNav::GetStartFolder(const CStdString &dir)
{
  if (dir.Equals("MovieGenres"))
    return "videodb://1/1/";
  else if (dir.Equals("MovieTitles"))
    return "videodb://1/2/";
  else if (dir.Equals("MovieYears"))
    return "videodb://1/3/";
  else if (dir.Equals("MovieActors"))
    return "videodb://1/4/";
  else if (dir.Equals("MovieDirectors"))
    return "videodb://1/5/";
  else if (dir.Equals("MovieStudios"))
    return "videodb://1/6/";
  else if (dir.Equals("MovieSets"))
    return "videodb://1/7/";
  else if (dir.Equals("MovieCountries"))
    return "videodb://1/8/";
  else if (dir.Equals("Movies"))
    return "videodb://1/";
  else if (dir.Equals("TvShowGenres"))
    return "videodb://2/1/";
  else if (dir.Equals("TvShowTitles"))
    return "videodb://2/2/";
  else if (dir.Equals("TvShowYears"))
    return "videodb://2/3/";
  else if (dir.Equals("TvShowActors"))
    return "videodb://2/4/";
  else if (dir.Equals("TvShowStudios"))
    return "videodb://2/5/";
  else if (dir.Equals("TvShows"))
    return "videodb://2/";
  else if (dir.Equals("MusicVideoGenres"))
    return "videodb://3/1/";
  else if (dir.Equals("MusicVideoTitles"))
    return "videodb://3/2/";
  else if (dir.Equals("MusicVideoYears"))
    return "videodb://3/3/";
  else if (dir.Equals("MusicVideoArtists"))
    return "videodb://3/4/";
  else if (dir.Equals("MusicVideoDirectors"))
    return "videodb://3/5/";
  else if (dir.Equals("MusicVideoStudios"))
    return "videodb://3/6/";
  else if (dir.Equals("MusicVideos"))
    return "videodb://3/";
  else if (dir.Equals("RecentlyAddedMovies"))
    return "videodb://4/";
  else if (dir.Equals("RecentlyAddedEpisodes"))
    return "videodb://5/";
  else if (dir.Equals("RecentlyAddedMusicVideos"))
    return "videodb://6/";
  else if (dir.Equals("Files"))
    return "sources://video/";
  return CGUIWindowVideoBase::GetStartFolder(dir);
}
