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

#include "GUIViewState.h"
#include "pvr/windows/GUIViewStatePVR.h"
#include "addons/GUIViewStateAddonBrowser.h"
#include "music/GUIViewStateMusic.h"
#include "video/GUIViewStateVideo.h"
#include "pictures/GUIViewStatePictures.h"
#include "programs/GUIViewStatePrograms.h"
#include "PlayListPlayer.h"
#include "utils/URIUtils.h"
#include "URL.h"
#include "GUIPassword.h"
#include "ViewDatabase.h"
#include "AutoSwitch.h"
#include "guilib/GUIWindowManager.h"
#include "addons/Addon.h"
#include "addons/AddonManager.h"
#include "addons/PluginSource.h"
#include "ViewState.h"
#include "settings/GUISettings.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "FileItem.h"
#include "guilib/Key.h"
#include "filesystem/AddonsDirectory.h"
#include "guilib/TextureManager.h"

#if defined(TARGET_ANDROID)
#include "filesystem/AndroidAppDirectory.h"
#endif
using namespace std;
using namespace ADDON;
using namespace PVR;

CStdString CGUIViewState::m_strPlaylistDirectory;
VECSOURCES CGUIViewState::m_sources;

CGUIViewState* CGUIViewState::GetViewState(int windowId, const CFileItemList& items)
{
  // don't expect derived classes to clear the sources
  m_sources.clear();

  if (windowId == 0)
    return GetViewState(g_windowManager.GetActiveWindow(),items);

  const CURL url=items.GetAsUrl();

  if (items.IsAddonsPath())
    return new CGUIViewStateAddonBrowser(items);

  if (items.HasSortDetails())
    return new CGUIViewStateFromItems(items);

  if (url.GetProtocol()=="musicdb")
    return new CGUIViewStateMusicDatabase(items);

  if (url.GetProtocol()=="musicsearch")
    return new CGUIViewStateMusicSearch(items);

  if (items.IsSmartPlayList() || url.GetProtocol() == "upnp" ||
      items.GetProperty("library.filter").asBoolean())
  {
    if (items.GetContent() == "songs" ||
        items.GetContent() == "albums" ||
        items.GetContent() == "mixed")
      return new CGUIViewStateMusicSmartPlaylist(items);
    else if (items.GetContent() == "musicvideos")
      return new CGUIViewStateVideoMusicVideos(items);
    else if (items.GetContent() == "tvshows")
      return new CGUIViewStateVideoTVShows(items);
    else if (items.GetContent() == "episodes")
      return new CGUIViewStateVideoEpisodes(items);
    else if (items.GetContent() == "movies")
      return new CGUIViewStateVideoMovies(items);
  }

  if (url.GetProtocol() == "library")
    return new CGUIViewStateLibrary(items);

  if (items.IsPlayList())
    return new CGUIViewStateMusicPlaylist(items);

  if (url.GetProtocol() == "lastfm")
    return new CGUIViewStateMusicLastFM(items);

  if (items.GetPath() == "special://musicplaylists/")
    return new CGUIViewStateWindowMusicSongs(items);

  if (url.GetProtocol() == "androidapp")
    return new CGUIViewStateWindowPrograms(items);

  if (windowId==WINDOW_MUSIC_NAV)
    return new CGUIViewStateWindowMusicNav(items);

  if (windowId==WINDOW_MUSIC_FILES)
    return new CGUIViewStateWindowMusicSongs(items);

  if (windowId==WINDOW_MUSIC_PLAYLIST)
    return new CGUIViewStateWindowMusicPlaylist(items);

  if (windowId==WINDOW_MUSIC_PLAYLIST_EDITOR)
    return new CGUIViewStateWindowMusicSongs(items);

  if (windowId==WINDOW_VIDEO_FILES)
    return new CGUIViewStateWindowVideoFiles(items);

  if (windowId==WINDOW_VIDEO_NAV)
    return new CGUIViewStateWindowVideoNav(items);

  if (windowId==WINDOW_VIDEO_PLAYLIST)
    return new CGUIViewStateWindowVideoPlaylist(items);

  if (windowId==WINDOW_PVR)
    return new CGUIViewStatePVR(items);

  if (windowId==WINDOW_PICTURES)
    return new CGUIViewStateWindowPictures(items);

  if (windowId==WINDOW_PROGRAMS)
    return new CGUIViewStateWindowPrograms(items);
  
  if (windowId==WINDOW_ADDON_BROWSER)
    return new CGUIViewStateAddonBrowser(items);

  //  Use as fallback/default
  return new CGUIViewStateGeneral(items);
}

CGUIViewState::CGUIViewState(const CFileItemList& items) : m_items(items)
{
  m_currentViewAsControl=0;
  m_currentSortMethod=0;
  m_playlist = PLAYLIST_NONE;
  m_sortOrder = SortOrderAscending;
}

CGUIViewState::~CGUIViewState()
{
}

SortOrder CGUIViewState::GetDisplaySortOrder() const
{
  // we actually treat some sort orders in reverse, so that we can have
  // the one sort order variable to save but it can be ascending usually,
  // and descending for the views which should be usually descending.
  // default sort order for date, size, program count + rating is reversed
  SORT_METHOD sortMethod = GetSortMethod();
  if (sortMethod == SORT_METHOD_DATE || sortMethod == SORT_METHOD_SIZE || sortMethod == SORT_METHOD_PLAYCOUNT ||
      sortMethod == SORT_METHOD_VIDEO_RATING || sortMethod == SORT_METHOD_PROGRAM_COUNT ||
      sortMethod == SORT_METHOD_SONG_RATING || sortMethod == SORT_METHOD_BITRATE || sortMethod == SORT_METHOD_LISTENERS)
  {
    if (m_sortOrder == SortOrderAscending) return SortOrderDescending;
    if (m_sortOrder == SortOrderDescending) return SortOrderAscending;
  }
  return m_sortOrder;
}

SortOrder CGUIViewState::SetNextSortOrder()
{
  if (m_sortOrder == SortOrderAscending)
    SetSortOrder(SortOrderDescending);
  else
    SetSortOrder(SortOrderAscending);

  SaveViewState();

  return m_sortOrder;
}

int CGUIViewState::GetViewAsControl() const
{
  return m_currentViewAsControl;
}

void CGUIViewState::SetViewAsControl(int viewAsControl)
{
  if (viewAsControl == DEFAULT_VIEW_AUTO)
    m_currentViewAsControl = CAutoSwitch::GetView(m_items);
  else
    m_currentViewAsControl = viewAsControl;
}

void CGUIViewState::SaveViewAsControl(int viewAsControl)
{
  SetViewAsControl(viewAsControl);
  SaveViewState();
}

SORT_METHOD CGUIViewState::GetSortMethod() const
{
  if (m_currentSortMethod>=0 && m_currentSortMethod<(int)m_sortMethods.size())
    return m_sortMethods[m_currentSortMethod].m_sortMethod;

  return SORT_METHOD_NONE;
}

int CGUIViewState::GetSortMethodLabel() const
{
  if (m_currentSortMethod>=0 && m_currentSortMethod<(int)m_sortMethods.size())
    return m_sortMethods[m_currentSortMethod].m_buttonLabel;

  return 103; // Sort By: Name
}

void CGUIViewState::GetSortMethods(vector< pair<int,int> > &sortMethods) const
{
  for (unsigned int i = 0; i < m_sortMethods.size(); i++)
    sortMethods.push_back(make_pair(m_sortMethods[i].m_sortMethod, m_sortMethods[i].m_buttonLabel));
}

void CGUIViewState::GetSortMethodLabelMasks(LABEL_MASKS& masks) const
{
  if (m_currentSortMethod>=0 && m_currentSortMethod<(int)m_sortMethods.size())
  {
    masks=m_sortMethods[m_currentSortMethod].m_labelMasks;
    return;
  }

  masks.m_strLabelFile.Empty();
  masks.m_strLabel2File.Empty();
  masks.m_strLabelFolder.Empty();
  masks.m_strLabel2Folder.Empty();
  return;
}

void CGUIViewState::AddSortMethod(SORT_METHOD sortMethod, int buttonLabel, LABEL_MASKS labelmasks)
{
  SORT_METHOD_DETAILS sort;
  sort.m_sortMethod=sortMethod;
  sort.m_buttonLabel=buttonLabel;
  sort.m_labelMasks=labelmasks;

  m_sortMethods.push_back(sort);
}

void CGUIViewState::SetCurrentSortMethod(int method)
{
  bool ignoreThe = g_guiSettings.GetBool("filelists.ignorethewhensorting");

  if (method < SORT_METHOD_NONE || method >= SORT_METHOD_MAX)
    return; // invalid

  // compensate for "Ignore The" options to make it easier on the skin
  if (ignoreThe && (method == SORT_METHOD_LABEL || method == SORT_METHOD_TITLE || method == SORT_METHOD_ARTIST || method == SORT_METHOD_ALBUM || method == SORT_METHOD_STUDIO || method == SORT_METHOD_VIDEO_SORT_TITLE))
    method++;
  else if (!ignoreThe && (method == SORT_METHOD_LABEL_IGNORE_THE || method == SORT_METHOD_TITLE_IGNORE_THE || method == SORT_METHOD_ARTIST_IGNORE_THE || method==SORT_METHOD_ALBUM_IGNORE_THE || method == SORT_METHOD_STUDIO_IGNORE_THE || method == SORT_METHOD_VIDEO_SORT_TITLE_IGNORE_THE))
    method--;

  SetSortMethod((SORT_METHOD)method);
  SaveViewState();
}

void CGUIViewState::SetSortMethod(SORT_METHOD sortMethod)
{
  for (int i=0; i<(int)m_sortMethods.size(); ++i)
  {
    if (m_sortMethods[i].m_sortMethod==sortMethod)
    {
      m_currentSortMethod=i;
      break;
    }
  }
}

SORT_METHOD CGUIViewState::SetNextSortMethod(int direction /* = 1 */)
{
  m_currentSortMethod += direction;

  if (m_currentSortMethod >= (int)m_sortMethods.size())
    m_currentSortMethod=0;
  if (m_currentSortMethod < 0)
    m_currentSortMethod = m_sortMethods.size() ? (int)m_sortMethods.size() - 1 : 0;

  SaveViewState();

  return GetSortMethod();
}

bool CGUIViewState::HideExtensions()
{
  return !g_guiSettings.GetBool("filelists.showextensions");
}

bool CGUIViewState::HideParentDirItems()
{
  return !g_guiSettings.GetBool("filelists.showparentdiritems");
}

bool CGUIViewState::DisableAddSourceButtons()
{
  if (g_settings.GetCurrentProfile().canWriteSources() || g_passwordManager.bMasterUser)
    return !g_guiSettings.GetBool("filelists.showaddsourcebuttons");

  return true;
}

int CGUIViewState::GetPlaylist()
{
  return m_playlist;
}

const CStdString& CGUIViewState::GetPlaylistDirectory()
{
  return m_strPlaylistDirectory;
}

void CGUIViewState::SetPlaylistDirectory(const CStdString& strDirectory)
{
  m_strPlaylistDirectory=strDirectory;
  URIUtils::RemoveSlashAtEnd(m_strPlaylistDirectory);
}

bool CGUIViewState::IsCurrentPlaylistDirectory(const CStdString& strDirectory)
{
  if (g_playlistPlayer.GetCurrentPlaylist()!=GetPlaylist())
    return false;

  CStdString strDir=strDirectory;
  URIUtils::RemoveSlashAtEnd(strDir);

  return (m_strPlaylistDirectory==strDir);
}


bool CGUIViewState::AutoPlayNextItem()
{
  return false;
}

CStdString CGUIViewState::GetLockType()
{
  return "";
}

CStdString CGUIViewState::GetExtensions()
{
  return "";
}

VECSOURCES& CGUIViewState::GetSources()
{
  return m_sources;
}

void CGUIViewState::AddAddonsSource(const CStdString &content, const CStdString &label, const CStdString &thumb)
{
  if (!g_advancedSettings.m_bVirtualShares)
    return;

  CFileItemList items;
  if (XFILE::CAddonsDirectory::GetScriptsAndPlugins(content, items))
  { // add the plugin source
    CMediaSource source;
    source.strPath = "addons://sources/" + content + "/";    
    source.strName = label;
    if (!thumb.IsEmpty() && g_TextureManager.HasTexture(thumb))
      source.m_strThumbnailImage = thumb;
    source.m_iDriveType = CMediaSource::SOURCE_TYPE_LOCAL;
    source.m_ignore = true;
    m_sources.push_back(source);
  }
}

#if defined(TARGET_ANDROID)
void CGUIViewState::AddAndroidSource(const CStdString &content, const CStdString &label, const CStdString &thumb)
{
  CFileItemList items;
  XFILE::CAndroidAppDirectory apps;
  if (apps.GetDirectory(content, items))
  {
    CMediaSource source;
    source.strPath = "androidapp://sources/" + content + "/";
    source.strName = label;
    if (!thumb.IsEmpty() && g_TextureManager.HasTexture(thumb))
      source.m_strThumbnailImage = thumb;
    source.m_iDriveType = CMediaSource::SOURCE_TYPE_LOCAL;
    source.m_ignore = true;
    m_sources.push_back(source);
  }
}
#endif

void CGUIViewState::AddLiveTVSources()
{
  VECSOURCES *sources = g_settings.GetSourcesFromType("video");
  for (IVECSOURCES it = sources->begin(); it != sources->end(); it++)
  {
    if (URIUtils::IsLiveTV((*it).strPath))
    {
      CMediaSource source;
      source.strPath = (*it).strPath;
      source.strName = (*it).strName;
      source.vecPaths = (*it).vecPaths;
      source.m_strThumbnailImage = "";
      source.FromNameAndPaths("video", source.strName, source.vecPaths);
      m_sources.push_back(source);
    }
  }
}

CGUIViewStateGeneral::CGUIViewStateGeneral(const CFileItemList& items) : CGUIViewState(items)
{
  AddSortMethod(SORT_METHOD_LABEL, 551, LABEL_MASKS("%F", "%I", "%L", ""));  // Filename, size | Foldername, empty
  SetSortMethod(SORT_METHOD_LABEL);

  SetViewAsControl(DEFAULT_VIEW_LIST);

  SetSortOrder(SortOrderAscending);
}

void CGUIViewState::SetSortOrder(SortOrder sortOrder)
{
  if (GetSortMethod() == SORT_METHOD_NONE)
    m_sortOrder = SortOrderNone;
  else if (sortOrder == SortOrderNone)
    m_sortOrder = SortOrderAscending;
  else
    m_sortOrder = sortOrder;
}

void CGUIViewState::LoadViewState(const CStdString &path, int windowID)
{ // get our view state from the db
  CViewDatabase db;
  if (db.Open())
  {
    CViewState state;
    if (db.GetViewState(path, windowID, state, g_guiSettings.GetString("lookandfeel.skin")) ||
        db.GetViewState(path, windowID, state, ""))
    {
      SetViewAsControl(state.m_viewMode);
      SetSortMethod(state.m_sortMethod);
      SetSortOrder(state.m_sortOrder);
    }
    db.Close();
  }
}

void CGUIViewState::SaveViewToDb(const CStdString &path, int windowID, CViewState *viewState)
{
  CViewDatabase db;
  if (db.Open())
  {
    CViewState state(m_currentViewAsControl, GetSortMethod(), m_sortOrder);
    if (viewState)
      *viewState = state;
    db.SetViewState(path, windowID, state, g_guiSettings.GetString("lookandfeel.skin"));
    db.Close();
    if (viewState)
      g_settings.Save();
  }
}

CGUIViewStateFromItems::CGUIViewStateFromItems(const CFileItemList &items) : CGUIViewState(items)
{
  const vector<SORT_METHOD_DETAILS> &details = items.GetSortDetails();
  for (unsigned int i = 0; i < details.size(); i++)
  {
    const SORT_METHOD_DETAILS sort = details[i];
    AddSortMethod(sort.m_sortMethod, sort.m_buttonLabel, sort.m_labelMasks);
  }
  // TODO: Should default sort/view mode be specified?
  m_currentSortMethod = 0;

  SetViewAsControl(DEFAULT_VIEW_LIST);

  SetSortOrder(SortOrderAscending);
  if (items.IsPlugin())
  {
    CURL url(items.GetPath());
    AddonPtr addon;
    if (CAddonMgr::Get().GetAddon(url.GetHostName(),addon) && addon)
    {
      PluginPtr plugin = boost::static_pointer_cast<CPluginSource>(addon);
      if (plugin->Provides(CPluginSource::AUDIO))
        m_playlist = PLAYLIST_MUSIC;
      if (plugin->Provides(CPluginSource::VIDEO))
        m_playlist = PLAYLIST_VIDEO;
    }
  }
  LoadViewState(items.GetPath(), g_windowManager.GetActiveWindow());
}

void CGUIViewStateFromItems::SaveViewState()
{
  SaveViewToDb(m_items.GetPath(), g_windowManager.GetActiveWindow());
}

CGUIViewStateLibrary::CGUIViewStateLibrary(const CFileItemList &items) : CGUIViewState(items)
{
  AddSortMethod(SORT_METHOD_NONE, 551, LABEL_MASKS("%F", "%I", "%L", ""));  // Filename, Size | Foldername, empty
  SetSortMethod(SORT_METHOD_NONE);
  SetSortOrder(SortOrderNone);

  SetViewAsControl(DEFAULT_VIEW_LIST);

  LoadViewState(items.GetPath(), g_windowManager.GetActiveWindow());
}

void CGUIViewStateLibrary::SaveViewState()
{
  SaveViewToDb(m_items.GetPath(), g_windowManager.GetActiveWindow());
}

